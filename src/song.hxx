#pragma once
#include <string>
#include <atomic>
#include <sqlite3.h>

#include <common/int.h>

#include "id3.hxx"
#include "schema.hxx"

// C++ representation of a row in the song table
struct song_record {
    id3::text title; // Song title
    id3::text album;
    id3::text artist;

    u32 release_year = 0;
    song_hash_t crc32 = 0;

    /// @brief Collect MP3 metadata into song info.
    ///
    /// This structure stores pointers into the MP3 buffer, so make sure it's
    /// freed only once this structure is destroyed/unused.
    song_record(u8* mp3, u32 size);

    /// @brief Generate an INSERT statement that will add the song to the database
    ///
    /// @param out A text buffer where the generated SQL should be stored
    void insert_sql(std::string& out) const noexcept;
};

// Statistics about an in-progress import operation
struct import_stats_t {
    std::atomic<u32> total_songs = 0;
    std::atomic<u32> num_skipped = 0;
    std::atomic<u32> num_loaded = 0;
    std::atomic<u32> num_metadata_grabbed = 0;
    std::atomic<u32> num_hashed = 0;
    std::atomic<u32> num_copied = 0;
    std::atomic<u32> num_generated_sql = 0;
    std::atomic<u32> sqlgen_time_us = 0; // SQL code generation time in microseconds

    void reset() noexcept {
        total_songs = 0;
        num_skipped = 0;
        num_loaded = 0;
        num_metadata_grabbed = 0;
        num_hashed = 0;
        num_copied = 0;
        num_generated_sql = 0;
        sqlgen_time_us = 0;
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
bool import_many_files_many_threads(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db, import_stats_t* stats);
