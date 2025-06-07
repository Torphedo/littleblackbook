#include "search.hxx"
#include <cstring>

#include <common/int.h>
#include <common/crc32.h>
#include "sql.hxx"

void search_tag(const char* tag, std::string& sql_out, bool standalone_query) {
    const u32 tag_hash = crc32buf((u8*)tag, strlen(tag));

    // Generate the SQL
    sqlgen(sql_out,
        "SELECT hash FROM songs s JOIN tagmap junction ON s.hash = junction.song_hash WHERE (junction.tag_hash = %u)",
        tag_hash);

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
            sql_out.append(" INTERSECT ");
        }
    }

    // Terminate statement
    sql_out.append(");\n");
}
