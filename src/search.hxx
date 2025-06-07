#pragma once
#include <string>
#include <sqlite3.h>

#include <common/int.h>

/// @brief Generate SQL to search for songs with a specific tag
///
/// @param tag The tag to search for
/// @param sql_out The buffer to store the generated SQL in
/// @param standalone_query Whether the generated SQL will be executed as its own
///        query (rather than being used to build a larger complex query).
void search_tag(const char* tag, std::string& sql_out, bool standalone_query = true);

void search_many_tags_and(const char* const* tags, u32 num_tags, std::string& sql_out);
