#pragma once
#include <common/int.h>
#include <sqlite3.h>

typedef struct {
    const c16* title; // Song title
    const c16* album;
    const c16* artist;
    u32 release_year;
}song_record;

u32 crc32file(const char* path);
song_record mp3_load_metadata(u8* mp3, u32 size);
void print_song(const song_record& song);
bool import_single_file(const char* path, sqlite3* db, const char* files_dir);
