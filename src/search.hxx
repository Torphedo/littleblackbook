#pragma once
#include <string>
#include <sqlite3.h>

#include <common/int.h>

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
