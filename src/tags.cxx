#include "tags.hxx"
#include <cstring>

#include <common/int.h>
#include <common/crc32.h>
#include <common/path.h>
#include "sqlgen.hxx"
#include "schema.hxx"
#include "stringcase.hxx"

// TODO: This could return an array of positions instead, which would be a lot simpler
std::vector<std::string> parse_artists(const char* str) {
    // Clone string so we can make it lowercase
    const std::string str_lower = str_tolower_copy(str);

    std::vector<std::string> results;
    const char* delim = "/";

    size_t prev_pos = 0;
    size_t pos = 0;
    while (true) {
        pos = str_lower.find(delim, pos);

        const std::string temp = str_lower.substr(prev_pos, pos - prev_pos);
        results.push_back(temp);
        if (pos == str_lower.npos) {
            break;
        }

        prev_pos = ++pos;
    }

    return results;
}

tag_hash_t create_tag_sql(const char* tag, std::string& sql_out, tag_hash_t hash) {
    if (hash == 0) {
        // No hash provided, calculate it
        hash = crc32buf((u8*)tag, strlen(tag));
    }

    sqlgen(sql_out, "INSERT INTO tags (tag, hash) VALUES ('%s', %d);\n", tag, hash);
    return hash;
}

void add_tag_sql(const char* tag, song_hash_t song_hash, std::string& sql_out) {
    // We need to create the tag if it doesn't exist
    const tag_hash_t tag_hash = create_tag_sql(tag, sql_out);

    // Actually add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_SONG_TABLE " (song_hash, tag_hash) VALUES (%d, %d);\n", song_hash, tag_hash);
}

void link_tags_sql(const char* parent, const char* child, std::string& sql_out) {
    // We need to create the tags if they don't exist
    const tag_hash_t child_hash = create_tag_sql(child, sql_out);
    const tag_hash_t parent_hash = create_tag_sql(parent, sql_out);

    // Add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_PARENT_TABLE " (child_hash, parent_hash) VALUES (%d, %d);\n", child_hash, parent_hash);
}

void search_tag(const char* tag, std::string& sql_out, bool standalone_query) {
    const tag_hash_t tag_hash = crc32buf((u8*)tag, strlen(tag));

    // Generate the SQL
    sqlgen(sql_out, R"(
SELECT song_hash FROM (
    SELECT * FROM tagmap
    UNION SELECT * FROM applied_parents
    UNION SELECT * FROM applied_grandparents
    UNION SELECT * FROM applied_great_grandparents
) WHERE tag_hash = %d
)", tag_hash);

    if (standalone_query) {
        // Terminate the statement
        sql_out.append(";\n");
    }
}

void search_many_tags_and(const char* const* tags, u32 num_tags, std::string& sql_out) {
    // Tag search gives us a list of IDs, but we want a query to pull out the actual song entries
    sql_out.append("SELECT * FROM songs s WHERE s.hash IN (\n");

    for (u32 i = 0; i < num_tags; i++) {
        search_tag(tags[i], sql_out, false);
        // Add intersection between each SELECT, but not after the last one
        if (i < num_tags - 1) {
            sql_out.append("\nINTERSECT ");
        }
    }

    // Terminate statement
    sql_out.append(");\n");
}
