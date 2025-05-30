#include "import.hxx"
#include "id3.hxx"
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>

#include <common/file.h>
#include <common/endian.h>
#include <common/vfile.h>
#include <common/crc32.h>
#include <common/logging.h>
#include <common/platform.h>
#include <common/path.h>

song_record mp3_load_metadata(u8* mp3, u32 size) {
    assert(size >= sizeof(id3::header) && "MP3 file is too small!");
    song_record out = {
        .crc32 = crc32buf(mp3, size),
        .import_timestamp = time(nullptr),
    };
    vfile id3 = vfile_open(mp3, size);
    const id3::header header = VFILE_READ(id3::header, &id3);
    assert(header.size() <= size && "Metadata claims to be larger than the MP3!");

    // This makes our loop simpler and avoids reading into the audio data
    id3.size = header.size();

    while (!vfile_eof(id3)) {
        auto frame = VFILE_READ(id3::frame_header, &id3);
        ENDIAN_FLIP(u32, frame.size);
        const u32 next_pos = id3.pos + frame.size;
        switch (frame.id) {
        case id3::FRAME_YEAR: {
            auto encoding = VFILE_READ(u8, &id3); // We read this just to skip it
            out.release_year = strtol((char*)vfile_cur(id3), nullptr, 10);
            assert(out.release_year <= 9999 && "Year should only be 4 characters!");
            break;
        }

        // The union member we assign to doesn't matter since it's just a pointer.
        // The seeking
        // TODO: See if we can reduce this repetition
        case id3::FRAME_ALBUM:
            out.album.encoding = VFILE_READ(id3::text_encoding, &id3);
            vfile_seek(&id3, sizeof(u16));
            out.album.ascii = (char*)vfile_cur(id3);
            out.album.length = (frame.size - 3) / (1 + out.album.encoding);
            break;
        case id3::FRAME_ARTIST:
            out.artist.encoding = VFILE_READ(id3::text_encoding, &id3);
            vfile_seek(&id3, sizeof(u16));
            out.artist.ascii = (char*)vfile_cur(id3);
            out.artist.length = (frame.size - 3) / (1 + out.artist.encoding);
            break;
        case id3::FRAME_TITLE:
            out.title.encoding = VFILE_READ(id3::text_encoding, &id3);
            vfile_seek(&id3, sizeof(u16));
            out.title.ascii = (char*)vfile_cur(id3);
            out.title.length = (frame.size - 3) / (1 + out.title.encoding);
            break;
        }

        // Skip to next frame
        id3.pos = next_pos;
    }

    return out;
}

void print_song(const song_record& song) {
    LOG_MSG(info, "Title: ");
    song.title.print();
    printf("\n");

    LOG_MSG(info, "Artist: ");
    song.artist.print();
    printf("\n");

    LOG_MSG(info, "Album: ");
    song.album.print();
    printf("\n");

    LOG_MSG(info, "Released in: %d\n", song.release_year);
}

bool import_single_file(const char* path, sqlite3* db, const char* files_dir) {
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

    // [path] -> [files_dir]/[hash].mp3
    char pathbuf[512] = {0};

    // Make sure the path will fit in our static sized buffer.
    // hash -> [up to] 10 chars, extension -> 4 chars, dirsep -> 1 char
    assert(ARRAY_SIZE(pathbuf) > (strlen(files_dir) + 10 + 4 + 1) && "Path is too long to fit!");

    // Collect metadata to fill out the record.
    // This structure has pointers into the MP3, so we can't free it yet
    const song_record song = mp3_load_metadata(mp3, size);

    // TODO: Update bobtail with a function to grab the file extension, and only
    // use this when there's no file extension.
    const char* extension = ".mp3";
    snprintf(pathbuf, ARRAY_SIZE(pathbuf), "%s%c%d%s", files_dir, PLATFORM_DIRSEP, song.crc32, extension);

    std::filesystem::copy_file(path, pathbuf);

    // Insert the record using the metadata
    char sqlbuf[512] = {0};

    // SQLite only wants UTF8 strings
    const std::string title = song.title.to_utf8();
    const std::string artist = song.artist.to_utf8();
    const std::string album = song.album.to_utf8();
    snprintf(sqlbuf, ARRAY_SIZE(sqlbuf),
             "INSERT INTO songs (title, artist, album, year, hash, import_timestamp) \
             VALUES ('%s', '%s', '%s', %d, %d, %lu)",
             title.c_str(), artist.c_str(), album.c_str(), song.release_year, song.crc32, song.import_timestamp);
    char* errmsg = nullptr;
    if (sqlite3_exec(db, sqlbuf, nullptr, nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) {
            printf("SQLite error: %s\n", errmsg);
        }
    }

    free(mp3);
    return true;
}
