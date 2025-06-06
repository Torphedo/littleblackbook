#include "database.hxx"
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <filesystem>
#include <vector>

#include <common/file.h>
#include <common/endian.h>
#include <common/vfile.h>
#include <common/crc32.h>
#include <common/logging.h>
#include <common/platform.h>
#include <common/path.h>

#include "id3.hxx"
#include "scope_timer.hxx"

void song_record::print() const noexcept {
    // We can't print the text fields directly because they may be UCS2
    printf("\tTitle: ");
    title.print();
    printf("\n");

    printf("\tArtist: ");
    artist.print();
    printf("\n");

    printf("\tAlbum: ");
    album.print();
    printf("\n");

    printf("\tReleased in: %d\n", release_year);
    printf("\tCRC32 Hash: %d\n", crc32);
}

song_record::song_record(u8* mp3, u32 size) {
    assert(size >= sizeof(id3::header) && "MP3 file is too small!");

    vfile id3 = vfile_open(mp3, size);
    const id3::header header = VFILE_READ(id3::header, &id3);
    assert(header.correct_magic() && "File is not an MP3!");
    assert(header.size() <= size && "Metadata claims to be larger than the MP3!");

    crc32 = crc32buf(mp3, size);

    // This makes our loop simpler and avoids reading into the audio data
    id3.size = header.size();

    while (!vfile_eof(id3)) {
        auto frame = VFILE_READ(id3::frame_header, &id3);
        ENDIAN_FLIP(u32, frame.size);
        const u32 next_pos = id3.pos + frame.size;
        switch (frame.id) {
        case id3::FRAME_YEAR: {
            auto encoding = VFILE_READ(u8, &id3); // We read this just to skip it
            release_year = strtol((char*)vfile_cur(id3), nullptr, 10);
            assert(release_year <= 9999 && "Year should only be 4 characters!");
            break;
        }

        case id3::FRAME_ALBUM:
            album = id3::text((u8*)vfile_cur(id3), frame.size);
            break;
        case id3::FRAME_ARTIST:
            artist = id3::text((u8*)vfile_cur(id3), frame.size);
            break;
        case id3::FRAME_TITLE:
            title = id3::text((u8*)vfile_cur(id3), frame.size);
            break;
        }

        // Skip to next frame
        id3.pos = next_pos;
    }
}

void song_record::insert_sql(std::string& out) const noexcept {
    // Insert the record using the metadata
    char sqlbuf[512] = {0};

    // SQLite only wants UTF8 strings
    const std::string title_str = title.to_utf8();
    const std::string artist_str = artist.to_utf8();
    const std::string album_str = album.to_utf8();
    snprintf(sqlbuf, ARRAY_SIZE(sqlbuf),
        "INSERT INTO songs (title, artist, album, year, hash, import_timestamp) VALUES ('%s', '%s', '%s', %u, %u, %lu);\n",
        title_str.c_str(), artist_str.c_str(), album_str.c_str(), release_year, crc32, import_timestamp);

    out.append(sqlbuf);
}

// TODO: Make this take a const u8*. Requires changes in other areas, and maybe a read-only variant of vfile.
bool import_single_file(const char* files_dir, u8* mp3, u32 size, std::string& sql_out) {
    // Collect metadata to fill out the record.
    // This structure has pointers into the MP3, so we can't free it yet
    const song_record song(mp3, size);

    // Buffer for the path where the file will be copied to
    // [files_dir]/[hash].mp3
    char pathbuf[512] = {0};

    // Make sure the path will fit in our static sized buffer.
    // hash -> [up to] 10 chars, extension -> 4 chars, dirsep -> 1 char
    assert(ARRAY_SIZE(pathbuf) > (strlen(files_dir) + 10 + 4 + 1) && "Path is too long to fit!");

    // TODO: Update bobtail with a function to grab the file extension, and only
    // use this default when there's no file extension.
    const char* extension = ".mp3";
    snprintf(pathbuf, ARRAY_SIZE(pathbuf), "%s%c%d%s", files_dir, PLATFORM_DIRSEP, song.crc32, extension);

    // Copy imported file into the database folder
    // std::filesystem::copy_file(path, pathbuf);

    // Generate INSERT statement from metadata
    song.insert_sql(sql_out);

    return true;
}

bool import_single_file(const char* path, const char* files_dir, std::string& sql_out) {

    assert(path_has_extension(path, ".mp3") && "Non-MP3 files aren't supported yet");

    if (!file_exists(path)) {
        return false;
    }

    const u32 size = file_size(path);
    u8* mp3 = file_load(path);
    if (mp3 == nullptr) {
        LOG_MSG(error, "Failed to load \"%s\"\n", path);
        return false;
    }

    const bool result = import_single_file(files_dir, mp3, size, sql_out);
    free(mp3);

    return result;
}

bool import_many_files(const char** paths, u32 num_paths, const char* files_dir, sqlite3* db) {
    bool result = true;
    std::string sql = "BEGIN TRANSACTION;\n";
    u32 num_songs = 0;
    float sqlgen_time = 0.0f;
    {
        const scope_timer generator_timer(sqlgen_time);
        std::vector<u8> mp3_buf(5 * 1024 * 1024);
        for (u32 i = 0; i < num_paths; i++) {
            if (!file_exists(paths[i])) {
                LOG_MSG(debug, "Skipping \"%s\" (it doesn't exist)\n", paths[i]);
                continue;
            }
            if (!path_has_extension(paths[i], ".mp3")) {
                LOG_MSG(debug, "Skipping \"%s\" (not an MP3)\n", paths[i]);
                continue;
            }

            const u32 size = file_size(paths[i]);
            if (size > mp3_buf.capacity()) {
                mp3_buf.reserve(size + 1);
            }
            file_load_existing(paths[i], mp3_buf.data(), size);

            import_single_file(files_dir, mp3_buf.data(), size, sql);
            num_songs++;
        }
    }
    sql.append("\nCOMMIT;\n");

    float sqlexec_time = 0.0f;
    if (num_songs > 0) {
        LOG_MSG(info, "Finished generating SQL code (%d inserts) in %.3fms!\n", num_songs, sqlgen_time);

        char* errmsg = nullptr;
        int result = SQLITE_OK;
        { // Scope to control the timer
            const scope_timer sql_timer(sqlexec_time);
            result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
        }
        if (result != SQLITE_OK) {
            result = false;
            if (errmsg) {
                LOG_MSG(error, "SQLite error: %s\n", errmsg);
            }
        }

        LOG_MSG(debug, "SQL compile/execute finished in %.3fms\n", sqlexec_time);
    } else {
        LOG_MSG(info, "It doesn't seem like you provided any MP3 files.\n");
        result = false;
    }

    return result;
}

void song_record::add_tag_sql(const char* tag, std::string& sql_out) const noexcept {
    const u32 hash = crc32buf((const u8*)tag, strlen(tag));

    char sqlbuf[512] = {0};
    snprintf(sqlbuf, ARRAY_SIZE(sqlbuf),
             "INSERT INTO tags (tag, hash) VALUES ('%s', %d);\n", tag, hash);
    sql_out.append(sqlbuf);
}
