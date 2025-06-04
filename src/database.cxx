#include "database.hxx"
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
