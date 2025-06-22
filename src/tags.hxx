#pragma once
#include <vector>
#include <string>
#include <sqlite3.h>

#include <common/int.h>

#include "schema.hxx"

std::vector<std::string> parse_artists(const char* str);

/// Create a tag, but don't add it to any songs
///
/// @param tag The name of the tag to add
/// @param sql_out The buffer to store the generated SQL code
/// @param hash If you already know the tag's hash, you can provide it to prevent a redundant calculation
/// @return The newly calculated hash, or the hash you provided
tag_hash_t create_tag_sql(const char* tag, std::string& sql_out, tag_hash_t hash = 0);

// Add a tag to a song, adding it to the tag table if needed
void add_tag_sql(const char* tag, song_hash_t song_hash, std::string& sql_out);

/// @brief Add a parent-child relationship between 2 tags
///
/// If the child tag is added to a song, the parent will appear to be
/// automatically added too. It'll also appear to be automatically removed if the
/// relationship is deleted.
///
/// Internally this behaviour is implemented with table joins, so adding/removing
// a pair in the parent table will instantly apply the change to the next search.
void link_tags_sql(const char* parent, const char* child, std::string& sql_out);

/// @brief Delete a parent-child relationship between 2 tags
void unlink_tags_sql(const char* parent, const char* child, std::string& sql_out);

/// @brief Generate SQL to search for songs with a specific tag
///
/// The generated SQL queries for a set of song hashes, not the whole record.
/// @param tag The tag to search for
/// @param sql_out The buffer to store the generated SQL in
/// @param standalone_query Whether the generated SQL will be executed as its own
///        query (rather than being used to build a larger complex query).
void search_tag(const char* tag, std::string& sql_out, bool standalone_query = true);

/// @brief Generate SQL to search for songs that have all the specified tags
///
/// The generated SQL queries for a the entire song record, not just the hash.
/// The "_and" suffix means it has to have *all* the tags (as opposed to an OR,
/// where it only needs to have at least 1).
/// @param tags An array of tags a song must have
/// @param num_tags Size of the tag array
/// @param sql_out The buffer to store the generated SQL in
void search_many_tags_and(const char* const* tags, u32 num_tags, std::string& sql_out);
