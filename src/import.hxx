#pragma once
#include "id3.hxx"
#include <common/int.h>
#include <sqlite3.h>

struct song_record {
    id3::text title; // Song title
    id3::text album;
    id3::text artist;

    u32 release_year = 0;
    u32 crc32 = 0;
    time_t import_timestamp = 0;

    // Print song fields in human-readable form to stdout
    void print() const noexcept;
};

u32 crc32file(const char* path);
song_record mp3_load_metadata(u8* mp3, u32 size);
bool import_single_file(const char* path, sqlite3* db, const char* files_dir);
