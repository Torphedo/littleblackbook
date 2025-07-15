#include "import.hxx"
#include <cstdio>

#include <filesystem>
#include <thread>
#include <vector>

#include <common/file.h>
#include <common/endian.h>
#include <common/vfile.h>
#include <common/crc32.h>
#include <common/logging.h>
#include <common/platform.h>
#include <common/path.h>

#include "id3.hxx"
#include "schema.hxx"
#include "scope_timer.hxx"
#include "sqlgen.hxx"
#include "tags.hxx"
#include "text_i8n.hxx"

song_record::song_record(u8* mp3, u32 size, u16 path_idx) : mp3(mp3), path_idx(path_idx) {
    assert(size >= sizeof(id3::header) && "MP3 file is impossibly small!");

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
            album = id3::text((u8*)vfile_cur(id3), frame.size, id3.pos);
            break;
        case id3::FRAME_ARTIST:
            artist = id3::text((u8*)vfile_cur(id3), frame.size, id3.pos);
            break;
        case id3::FRAME_TITLE:
            title = id3::text((u8*)vfile_cur(id3), frame.size, id3.pos);
            break;
        default:
            break;
        }

        // Skip to next frame
        id3.pos = next_pos;
    }
}

void song_record::insert_sql(sqlite3* db, sqlite3_stmt* stmt) const noexcept {
    sql_bind(stmt, 1, mp3, title);
    sql_bind(stmt, 2, mp3, artist);
    sql_bind(stmt, 3, mp3, album);
    sql_bind(stmt, 4, int(release_year));
    sql_bind(stmt, 5, crc32);
    sqlite3_step(stmt);
    sqlite3_reset(stmt);

    std::vector<std::string> artist_tags;
    {
        std::string artist_copy;
        // Try to avoid copying if possible
        const char* artist_str = (char*)(mp3 + artist.ascii);
        if (artist.encoding == id3::TEXT_UCS2) {
            artist_copy = artist.to_utf8(mp3);
            artist_str = artist_copy.c_str();
        }
        artist_tags = parse_artists(artist_str);
    }
    std::string sqlbuf;
    for (const std::string& tag : artist_tags) {
        create_tag_sql(db, tag.c_str());
        add_tag_to_song_sql(db, tag.c_str(), this->crc32, sqlbuf);
    }
    char* errmsg = nullptr;
    if (sqlite3_exec(db, sqlbuf.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
        LOG_MSG(error, "SQLite error: %s\n", errmsg);
        LOG_MSG(debug, "Offending statement: \"%s\"\n", sqlbuf.c_str());
    }
}

sqlite3_stmt* song_record::prepare_sql(sqlite3* db) noexcept {
    const char sql[] = "INSERT INTO songs (title, artist, album, year, hash) VALUES (?, ?, ?, ?, ?);";
    return compile_sql(sql, sizeof(sql), db);
}

void song_record::adjust_offsets(u32 offset) noexcept {
    title.ascii += offset;
    album.ascii += offset;
    artist.ascii += offset;
}

u32 copy_id3(u8* mp3, u32 size, std::vector<u8>& id3_out) {
    assert(size >= sizeof(id3::header) && "MP3 file is impossibly small!");

    vfile id3 = vfile_open(mp3, size);
    const id3::header header = VFILE_READ(id3::header, &id3);
    assert(header.correct_magic() && "File is not an MP3!");
    assert(header.size() <= size && "Metadata claims to be larger than the MP3!");

    // This makes our loop simpler and avoids reading into the audio data
    id3.size = header.size();

    u32 id3_size = id3.size;
    while (!vfile_eof(id3)) {
        u32 frame_start = id3.pos;
        auto frame = VFILE_READ(id3::frame_header, &id3);
        ENDIAN_FLIP(u32, frame.size);
        const u32 next_pos = id3.pos + frame.size;
        if (frame.id == id3::FRAME_PICTURE) {
            id3_size = frame_start;
            break;
        }

        // Skip to next frame
        id3.pos = next_pos;
    }

    const u32 old_size = id3_out.size();
    const u32 new_size = old_size + id3_size;
    if (id3_out.capacity() < new_size) {
        id3_out.reserve(new_size);
    }

    // Reserve before copying, otherwise it assumes that extra space can be
    // ignored when re-allocating
    id3_out.resize(new_size);

    // Append ID3 data to buffer
    u8* dest = id3_out.data() + old_size;
    memcpy(dest, mp3, id3_size);

    return old_size;
}

// Generate SQL and gather some basic stats about the import process
void import_many_files(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db, import_stats_t* stats) {
    if (!file_exists(files_dir)) {
        std::filesystem::create_directory(files_dir);
    }

    std::vector<song_record> songs;
    songs.reserve(num_paths);

    std::vector<u8> file_buf(5 * 1024 * 1024); // Buffer is reused for many files
    std::vector<u8> id3_buf;

    for (u32 i = 0; i < num_paths; i++) {
        // Early exit for simple errors
        if (!path_has_extension(paths[i], ".mp3")) {
            LOG_MSG(debug, "Skipping \"%s\" (not an MP3)\n", paths[i]);
            stats->num_skipped++;
            continue;
        }
        if (!file_exists(paths[i])) {
            LOG_MSG(debug, "Skipping \"%s\" (it doesn't exist)\n", paths[i]);
            stats->num_skipped++;
            continue;
        }

        // Load the file
        const u32 size = file_size(paths[i]);
        if (size > file_buf.capacity()) {
            file_buf.reserve(size + 1);
        }
        file_load_existing(paths[i], file_buf.data(), size);
        stats->num_loaded++;

        // Extract metadata & hash the file
        song_record song(file_buf.data(), size, i);
        stats->num_hashed++;
        stats->num_metadata_grabbed++;

        // Copy ID3 data into our buffer and make the offsets relative to the
        // whole ID3 buffer.
        const u32 offset = copy_id3(file_buf.data(), size, id3_buf);
        song.adjust_offsets(offset);

        songs.push_back(song);
    }

    sqlite3_stmt* song_stmt = song_record::prepare_sql(db);
    if (!song_stmt) {
        return;
    }
    for (song_record& song : songs) {
        song.mp3 = id3_buf.data(); // All offsets are relative to this buffer

        // Make sure the path will fit in our static sized buffer.
        // hash -> [up to] 10 chars, extension -> 4 chars, dirsep -> 1 char
        char destpath[512] = {0};
        assert(ARRAY_SIZE(destpath) > (strlen(files_dir) + 10 + 4 + 1) && "Destination path too long [programmer error]!");

        // [files_dir]/[hash].mp3
        const char *extension = path_get_extension(paths[song.path_idx]);
        snprintf(destpath, ARRAY_SIZE(destpath), "%s%c%d%s", files_dir, PLATFORM_DIRSEP, song.crc32, extension);

        if (file_exists(destpath)) {
            // File with this hash already exists in the database. Either a
            // duplicate (very likely) or a hash conflict.
            stats->num_skipped++;
            continue;
        } else {
            std::filesystem::copy_file(paths[song.path_idx], destpath);
            stats->num_copied++;
        }

        song.insert_sql(db, song_stmt);
        stats->num_generated_sql++;
    }
    sqlite3_finalize(song_stmt);
}

bool import_many_files_many_threads(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db, import_stats_t* stats) {
    if (num_paths == 0) {
        LOG_MSG(info, "You didn't give any paths. Not much of an import, is it?\n");
        return true;
    }
    LOG_MSG(info, "Starting importer for %d files\n", num_paths);
    if (num_paths > 100) {
        LOG_MSG(info, "You're importing a lot of files, this might take a while.\n");
    }
    printf("\n");

    static const u32 max_threads = 64;
    const u32 num_threads = 16;

    std::thread threads[max_threads];
    const u32 paths_per_thread = num_paths / num_threads;
    stats->total_songs = num_paths;

    float phase1_elapsed = 0.0f;
    {
    const scope_timer phase1_timer(phase1_elapsed);
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    // Dispatch a bunch of threads, assigning an even amount to each one
    u32 pos = 0;
    for (u32 i = 0; i < num_threads; i++) {
        const char* const* thread_paths = &paths[pos];
        if (paths_per_thread == 0) {
            continue;
        }
        threads[i] = std::thread(import_many_files, thread_paths, paths_per_thread, files_dir, db, stats);
        pos += paths_per_thread;
    }

    for (u32 i = 0; i < num_threads; i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    // Do the remainder sequentially
    const u32 remainder = num_paths % num_threads;
    if (remainder > 0) {
        import_many_files(&paths[num_threads * paths_per_thread], remainder, files_dir, db, stats);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    } // Timer scope


    LOG_MSG(info, "Finished importing in %.3fms\n", phase1_elapsed);

    if (stats->num_skipped > 0) {
        u32 num_skipped = stats->num_skipped.load();
        const char* past_tense_word = (num_skipped == 1) ? "was" : "were";
        LOG_MSG(info, "I found %u new songs to import, but skipped %u that %s already in the database.\n",
                stats->num_generated_sql.load(), stats->num_skipped.load(), past_tense_word);
    } else {
        LOG_MSG(info, "Importing %u new songs.\n", stats->num_generated_sql.load());
    }

    return true;
}
