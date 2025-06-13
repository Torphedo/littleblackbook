#pragma once
#include "id3.hxx"
#include <ctime>
#include <string>
#include <sqlite3.h>

#include <common/int.h>

// C++ representation of a row in the song table
struct song_record {
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

    /// @brief Generate an INSERT statement that will add the song to the database
    ///
    /// @param out A text buffer where the generated SQL should be stored
    void insert_sql(std::string& out) const noexcept;
};

struct import_result {
    u32 num_imported = 0;
    u32 num_skipped = 0;
    float sqlgen_time = 0.0f;
    std::string sql;

    import_result& operator+=(const import_result& other) {
        sql.append(other.sql);
        num_imported += other.num_imported;
        num_skipped += other.num_skipped;
        sqlgen_time += other.sqlgen_time;
        return *this;
    }
};

/// @brief Import a set of audio files into the database
///
/// At the moment, this only supports MP3 files (and will crash via assert if you
/// give it another type). Metadata is scraped from the files and used to fill in
/// database records for each song. Most of the time (~90%) is spent hashing
/// audio files and copying them to the database folder.
///
/// @param paths An array of filepaths to import from
/// @param num_paths The number of filepaths in the array
/// @param files_dir The relative or absolute path of the database directory.
///        All imported files will be copied to this folder, renamed to
///        their hash.
/// @param db The SQLite database connection to use for the import
import_result import_many_files(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db);

bool import_many_files_many_threads(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db);
