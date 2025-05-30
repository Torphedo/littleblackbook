#include "import.hxx"
#include "id3.hxx"
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <common/file.h>
#include <common/endian.h>
#include <common/vfile.h>
#include <common/crc32.h>
#include <common/logging.h>
#include <common/platform.h>
#include <common/path.h>

u32 crc32file(const char* path) {
    if (!file_exists(path)) {
        return 0;
    }

    const u32 size = file_size(path);
    u8* buf = (u8*)malloc(size);
    if (buf == nullptr) {
        LOG_MSG(error, "Failed to allocate %d bytes to load \"%s\"\n", size, path);
        return 0;
    }

    if (!file_load_existing(path, buf, size)) {
        LOG_MSG(error, "Failed to load \"%s\"\n", path);
        return 0;
    }

    const u32 crc = crc32buf(buf, size);

    free(buf);
    return crc;
}

song_record mp3_load_metadata(u8* mp3, u32 size) {
    assert(size >= sizeof(id3::header) && "MP3 file is too small!");
    song_record out = {0};
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
            const auto text_encoding = VFILE_READ(u8, &id3);
            out.release_year = strtol((char*)vfile_cur(id3), nullptr, 10);
            assert(out.release_year <= 9999 && "Year should only be 4 characters!");
            break;
        }
        case id3::FRAME_ALBUM: {
            const auto text_encoding = VFILE_READ(u8, &id3);
            assert(text_encoding == id3::TEXT_UCS2 && "Only UCS2 text metadata is supported for now!");
            out.album = (c16*)vfile_cur(id3);
            break;
        }
        case id3::FRAME_ARTIST: {
            const auto text_encoding = VFILE_READ(u8, &id3);
            assert(text_encoding == id3::TEXT_UCS2 && "Only UCS2 text metadata is supported for now!");
            out.artist = (c16*)vfile_cur(id3);
            break;
        }
        case id3::FRAME_TITLE: {
            const auto text_encoding = VFILE_READ(u8, &id3);
            assert(text_encoding == id3::TEXT_UCS2 && "Only UCS2 text metadata is supported for now!");
            out.title = (c16*)vfile_cur(id3);
            break;
        }
        }

        // Skip to next frame
        id3.pos = next_pos;
    }

    return out;
}

void print_song(const song_record& song) {
    LOG_MSG(info, "Title: ");
    print_c16s(song.title);
    printf("\n");

    LOG_MSG(info, "Artist: ");
    print_c16s(song.artist);
    printf("\n");

    LOG_MSG(info, "Album: ");
    print_c16s(song.album);
    printf("\n");

    LOG_MSG(info, "Released in: %d\n", song.release_year);
}

bool import_single_file(const char* path, sqlite3* db, const char* files_dir) {
    const u32 crc = crc32file(path);
    if (crc == 0) {
        LOG_MSG(error, "Failed to hash file \"%s\" for import", path);
        return false;
    }

    // TODO: Update bobtail with a function to grab the file extension, and only
    // use this when there's no file extension.
    const char* extension = ".mp3";
    assert(path_has_extension(path, ".mp3") && "Non-MP3 files aren't supported yet");

    // [path] -> [files_dir]/[hash].mp3
    char buf[512] = {0};

    // Make sure the path will fit in our static sized buffer.
    // hash -> [up to] 10 chars, extension -> 4 chars, dirsep -> 1 char
    assert(ARRAY_SIZE(buf) > strlen(files_dir) + 10 + 4 + 1 && "Path is too long to fit!");

    snprintf(buf, ARRAY_SIZE(buf), "%s%c%d%s", files_dir, PLATFORM_DIRSEP, crc, extension);

    std::filesystem::copy(path, files_dir);

    // Collect metadata to fill out the record

    // Insert the record using the metadata
}
