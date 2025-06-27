#include "tags.hxx"
#include <cstring>

#include <common/int.h>
#include <common/crc32.h>
#include <common/path.h>
#include "sqlgen.hxx"
#include "schema.hxx"
#include "text_i8n.hxx"

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

        const std::string temp = "artist:" + str_lower.substr(prev_pos, pos - prev_pos);
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
        hash = crc32buf((const u8*)tag, strlen(tag));
    }
    const char* colon = strchr(tag, ':');

    if (colon == nullptr) {
        // No namespace found, proceed as normal
        sqlgen(sql_out, "INSERT INTO tags (tag, hash) VALUES ('%s', %d);\n", tag, hash);
        return hash;
    }

    // This tag has a namespace, split it up.
    const u32 namespace_len = colon - tag;
    const char* tag_isolated = colon + 1;
    const tag_hash_t namespace_hash = crc32buf((const u8*)tag, namespace_len);
    sqlgen(sql_out, "INSERT OR IGNORE INTO namespaces (namespace, hash) VALUES ('%.*s', %d);\n", namespace_len, tag, namespace_hash);
    sqlgen(sql_out, "INSERT INTO tags (tag, namespace_hash, hash) VALUES ('%s', %d, %d);\n", tag_isolated, namespace_hash, hash);

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

void unlink_tags_sql(const char* parent, const char* child, std::string& sql_out) {
    // We need to create the tags if they don't exist
    const tag_hash_t child_hash = crc32buf((const u8*)child, strlen(child));
    const tag_hash_t parent_hash = crc32buf((const u8*)parent, strlen(parent));

    // Delete the tag association
    sqlgen(sql_out, "DELETE FROM " TAG_PARENT_TABLE " WHERE child_hash = %d AND parent_hash = %d\n", child_hash, parent_hash);
}

void search_tag(const char* tag, std::string& sql_out, bool standalone_query) {
    const tag_hash_t tag_hash = crc32buf((u8*)tag, strlen(tag));

    // Generate the SQL
    sqlgen(sql_out, "SELECT song_hash FROM " RESOLVED_TAG_SONG_TABLE " WHERE tag_hash = %d", tag_hash);

    if (standalone_query) {
        // Terminate the statement
        sql_out.append(";\n");
    }
}

void search_many_tags_and(const char* const* tags, u32 num_tags, std::string& sql_out) {
    // TODO: Parent search optimization:
    // If we search for 2 tags where 1 is parented to the other, we could skip
    // one of them. e.g. "mos def AND rap" can be simplified to "rap", since the
    // parent/child table tells us that all results for "mos def" will be
    // included in "rap".

    // Tag search gives us hashes, so we use a compound SELECT to pull out full rows.
    // SELECT-all at the end lets us put a compound keyword in front of all future SELECTs.
    // This is both simpler and makes single negated tags work correctly.
    sql_out.append("SELECT * FROM songs s WHERE s.hash IN (SELECT hash FROM songs\n");

    for (u32 i = 0; i < num_tags; i++) {
        // If the tag starts with "-", we interpret that as "AND NOT [tag]".
        const bool invert_tag = tags[i][0] == '-';

        // Add compound operator before each SELECT.
        // EXCEPT implements the "NOT" behaviour we want if the tag is inverted.
        const char* compounder = invert_tag ? "\nEXCEPT " : "\nINTERSECT ";
        sql_out.append(compounder);

        // Increment tag pointer to skip the "-" if needed
        search_tag(tags[i] + invert_tag, sql_out, false);
    }

    // Terminate statement
    sql_out.append(");\n");
}
