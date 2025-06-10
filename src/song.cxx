#include "song.hxx"

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
#include "sqlgen.hxx"

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
    if (!file_exists(files_dir)) {
        std::filesystem::create_directory(files_dir);
    }

    printf("Phase 1:\n");
    printf("\t- Extracting metadata\n");
    printf("\t- Generating SQL code\n");
    printf("\t- Hashing your files\n");
    printf("\t- Copying your files\n");
    if (num_paths > 100) {
        printf("You're importing a lot of files, this might take a while.\n");
    }
    printf("\n");

    bool result = true;
    std::string sql = "BEGIN TRANSACTION;\n";
    u32 imported_songs = 0;
    u32 skipped_songs = 0;
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

            const char* extension = path_get_extension(paths[i]);
            snprintf(pathbuf, ARRAY_SIZE(pathbuf), "%s%c%u%s", files_dir, PLATFORM_DIRSEP, song.crc32, extension);

            if (file_exists(pathbuf)) {
                // File with this hash already exists in the database. Either a
                // duplicate (very likely) or a hash conflict.
                import_conflicts.push_back(i);
                skipped_songs++;
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
    // printf("Phase 2: Checking for duplicates & 'remastered' / 'deluxe' copies");

    if (skipped_songs > 0) {
        LOG_MSG(info, "I found %u new songs, and skipped %u that were already in the database.\n", imported_songs, skipped_songs);
    } else {
        LOG_MSG(info, "Found %u new songs for import.\n", imported_songs, skipped_songs);
    }

    float sqlexec_time = 0.0f;
    if (imported_songs == 0) {
        LOG_MSG(info, "I couldn't find any songs to import.\n");
        return false;
    }
    LOG_MSG(info, "Finished generating SQL (%u INSERTs) in %.3fms\n", imported_songs, sqlgen_time);

    printf("Phase 2: Updating SQLite database\n\n");
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

    LOG_MSG(debug, "SQLite compile/execute finished in %.3fms\n", sqlexec_time);

    return result;
}
