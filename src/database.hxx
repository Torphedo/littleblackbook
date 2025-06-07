#pragma once
#include "id3.hxx"
#include <ctime>
#include <string>
#include <sqlite3.h>

#include <common/int.h>

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

    // SQL generation methods, see schema interface

    // Unimplemented for now
    static const char* table_sql() noexcept;

    void insert_sql(std::string& out) const noexcept;
};

/// Create a tag, but don't add it to any songs
///
/// @param tag The name of the tag to add
/// @param sql_out The buffer to store the generated SQL code
/// @param hash If you already know the tag's hash, you can provide it to prevent a redundant calculation
/// @return The newly calculated hash, or the hash you provided
u32 create_tag_sql(const char* tag, std::string& sql_out, u32 hash = 0);

// Add a tag to a song, adding it to the tag table if needed
void add_tag_sql(const char* tag, u32 song_hash, std::string& sql_out);

/// @brief Add a parent-child relationship between 2 tags
///
/// If the child tag is added to a song, the parent will appear to be
/// automatically added too. It'll also appear to be automatically removed if the
/// relationship is deleted.
///
/// Internally this behaviour is implemented with table joins, so adding/removing
// a pair in the parent table will instantly apply the change to the next search.
void link_tags_sql(const char* parent, const char* child, std::string& sql_out);

bool import_many_files(const char* const* paths, u32 num_paths, const char* files_dir, sqlite3* db);
