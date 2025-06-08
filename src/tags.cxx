#include "tags.hxx"
#include <cstring>

#include <common/int.h>
#include <common/crc32.h>
#include "sqlgen.hxx"
#include "schema.hxx"

u32 create_tag_sql(const char* tag, std::string& sql_out, u32 hash) {
    if (hash == 0) {
        // No hash provided, calculate it
        hash = crc32buf((u8*)tag, strlen(tag));
    }

    sqlgen(sql_out, "INSERT INTO tags (tag, hash) VALUES ('%s', %u);\n", tag, hash);
    return hash;
}

void add_tag_sql(const char* tag, u32 song_hash, std::string& sql_out) {
    // We need to create the tag if it doesn't exist
    const u32 tag_hash = create_tag_sql(tag, sql_out);

    // Actually add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_SONG_TABLE " (song_hash, tag_hash) VALUES (%u, %u);\n", song_hash, tag_hash);
}

void link_tags_sql(const char* parent, const char* child, std::string& sql_out) {
    // We need to create the tags if they don't exist
    const u32 child_hash = create_tag_sql(child, sql_out);
    const u32 parent_hash = create_tag_sql(parent, sql_out);

    // Add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_PARENT_TABLE " (child_hash, parent_hash) VALUES (%u, %u);\n", child_hash, parent_hash);
}

void search_tag(const char* tag, std::string& sql_out, bool standalone_query) {
    const u32 tag_hash = crc32buf((u8*)tag, strlen(tag));

    // Generate the SQL
    sqlgen(sql_out,
        "SELECT hash FROM songs s JOIN " TAG_SONG_TABLE " junction ON s.hash = junction.song_hash WHERE (junction.tag_hash = %u)",
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
