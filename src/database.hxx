#pragma once
#include "id3.hxx"
#include <ctime>
#include <string>
#include <sqlite3.h>

#include <common/int.h>
#include "schema.hxx"

struct song_record : schema {
    id3::text title; // Song title
    id3::text album;
    id3::text artist;

    u32 release_year = 0;
    u32 crc32 = 0;
    time_t import_timestamp = time(nullptr);

    /// @brief Collect MP3 metadata into song info.
    ///
    /// This structure stores pointers into the MP3 buffer, so make sure it's
    /// freed only once this structure is destroyed/unused.
    song_record(u8* mp3, u32 size);

    // Print song fields in human-readable form to stdout
    void print() const noexcept;

    // SQL generation methods, see schema interface

    // Unimplemented for now
    static const char* table_sql() noexcept;

    virtual void insert_sql(std::string& out) const noexcept;
};

// Add a tag to a song
void add_tag_sql(const char* tag, u32 song_hash, std::string& sql_out);

bool import_many_files(const char** paths, u32 num_paths, const char* files_dir, sqlite3* db);
