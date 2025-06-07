#include "database.hxx"

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
#include "sql.hxx"
#include "schema.hxx"

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
    printf("\tCRC32 Hash: %u\n", crc32);
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
    // SQLite only wants UTF8 strings
    const std::string title_str = title.to_utf8();
    const std::string artist_str = artist.to_utf8();
    const std::string album_str = album.to_utf8();

    sqlgen(out,
        "INSERT INTO songs (title, artist, album, year, hash, import_timestamp) VALUES ('%s', '%s', '%s', %u, %u, %lu);\n",
        title_str.c_str(), artist_str.c_str(), album_str.c_str(), release_year, crc32, import_timestamp
    );
}

bool import_many_files(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db) {
    bool result = true;
    std::string sql = "BEGIN TRANSACTION;\n";
    u32 imported_songs = 0;
    float sqlgen_time = 0.0f; // Elapsed runtime for importing and generating SQL INSERTs
    {
        const scope_timer generator_timer(sqlgen_time);
        std::vector<u8> mp3_buf(5 * 1024 * 1024);
        // Indices of all paths that were found to already be in the database
        std::vector<u32> import_conflicts;
        for (u32 i = 0; i < num_paths; i++) {
            // Early exit for simple errors
            if (!path_has_extension(paths[i], ".mp3")) {
                LOG_MSG(debug, "Skipping \"%s\" (not an MP3)\n", paths[i]);
                continue;
            }
            if (!file_exists(paths[i])) {
                LOG_MSG(debug, "Skipping \"%s\" (it doesn't exist)\n", paths[i]);
                continue;
            }

            // Load the file
            const u32 size = file_size(paths[i]);
            if (size > mp3_buf.capacity()) {
                mp3_buf.reserve(size + 1);
            }
            file_load_existing(paths[i], mp3_buf.data(), size);

            // Extract metadata & hash the file
            const song_record song(mp3_buf.data(), size);

            // Copy file into database folder with hash for its name

            // Buffer for the path where the file will be copied to
            // [files_dir]/[hash].mp3
            char pathbuf[512] = {0};

            // Make sure the path will fit in our static sized buffer.
            // hash -> [up to] 10 chars, extension -> 4 chars, dirsep -> 1 char
            assert(ARRAY_SIZE(pathbuf) > (strlen(files_dir) + 10 + 4 + 1) && "Path is too long to fit!");

            // TODO: Update bobtail with a function to grab the file extension, and only
            // use this default when there's no file extension.
            const char* extension = ".mp3";
            snprintf(pathbuf, ARRAY_SIZE(pathbuf), "%s%c%u%s", files_dir, PLATFORM_DIRSEP, song.crc32, extension);

            if (file_exists(pathbuf)) {
                // File with this hash already exists in the database. Either a
                // duplicate (very likely) or a hash conflict.
                import_conflicts.push_back(i);
            } else {
                // Copy file into the database folder for import
                std::filesystem::copy_file(paths[i], pathbuf);

                // Generate INSERT statement
                song.insert_sql(sql);
                imported_songs++;
            }
        }
    }
    sql.append("\nCOMMIT;\n");

    // TODO: Try to figure out if any of the failed imports are real hash conflicts (not duplicates)

    float sqlexec_time = 0.0f;
    if (imported_songs == 0) {
        LOG_MSG(info, "It doesn't seem like you provided any MP3 files.\n");
        return false;
    }
    LOG_MSG(info, "Finished generating SQL code (%d inserts) in %.3fms!\n", imported_songs, sqlgen_time);

    char* errmsg = nullptr;
    int sql_result = SQLITE_OK;
    { // Scope to control the timer
        const scope_timer sql_timer(sqlexec_time);
        sql_result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
    }
    if (sql_result != SQLITE_OK) {
        result = false;
        if (errmsg) {
            LOG_MSG(error, "SQLite error: %s\n", errmsg);
        }
    }

    LOG_MSG(debug, "SQL compile/execute finished in %.3fms\n", sqlexec_time);

    return result;
}

u32 create_tag_sql(const char* tag, std::string& sql_out, u32 hash = 0) {
    if (hash == 0) {
        // No hash provided, calculate it
        hash = crc32buf((u8*)tag, strlen(tag));
    }

    sqlgen(sql_out, "INSERT INTO tags (tag, hash) VALUES ('%s', %u);\n", tag, hash);
    return hash;
}

void add_tag_sql(const char* tag, u32 song_hash, std::string& sql_out) {
    // We need to create the tag if it doesn't exist
    const u32 tag_hash = create_tag_sql(tag, sql_out);

    // Actually add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_SONG_TABLE " (song_hash, tag_hash) VALUES (%u, %u);\n", song_hash, tag_hash);
}

void link_tags_sql(const char* parent, const char* child, std::string& sql_out) {
    // We need to create the tags if they don't exist
    const u32 child_hash = create_tag_sql(child, sql_out);
    const u32 parent_hash = create_tag_sql(parent, sql_out);

    // Add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_PARENT_TABLE " (child_hash, parent_hash) VALUES (%u, %u);\n", child_hash, parent_hash);
}
