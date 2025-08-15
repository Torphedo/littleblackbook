#include "tags.hxx"
#include <cstring>

#include <common/int.h>
#include <common/crc32.h>

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

tag_hash_t create_tag_sql(sqlite3* db, const char* tag, tag_hash_t hash, unsigned char encoding) {
    if (hash == 0) {
        // No hash provided, calculate it
        hash = crc32fast((const u8*)tag, strlen(tag));
    }
    const char tag_insert[] = "INSERT OR IGNORE INTO tags (tag, hash, namespace_hash) VALUES (?, ?, ?);";
    sqlite3_stmt* tag_stmt = compile_sql(tag_insert, sizeof(tag_insert), db);
    if (tag_stmt == nullptr) {
        return 0;
    }

    const char* colon = strchr(tag, ':');
    const ptrdiff_t namespace_len = ptrdiff_t(colon) - ptrdiff_t(tag);
    if (colon != nullptr && namespace_len > 0) {
        // This tag has a namespace, split it up.
        const tag_hash_t namespace_hash = crc32fast((const u8*)tag, namespace_len);
        // We don't rehash because the tag hash includes namespace
        sql_bind(tag_stmt, 3, namespace_hash);

        // Compile & execute INSERT
        const char ns_insert[] = "INSERT OR IGNORE INTO namespaces (namespace, hash) VALUES (?, ?);";
        sqlite3_stmt* ns_stmt = compile_sql(ns_insert, sizeof(ns_insert), db);
        if (ns_stmt != nullptr) {
            sql_bind(ns_stmt, 1, tag, namespace_len, encoding);
            sql_bind(ns_stmt, 2, namespace_hash);
            sqlite3_step(ns_stmt);
            sqlite3_finalize(ns_stmt);
        }

        // Make sure tag string doesn't include the namespace
        const u32 char_size = (encoding == SQLITE_UTF8) ? 1 : 2;
        tag = colon + char_size;
    } else {
        sqlite3_bind_null(tag_stmt, 3);
    }
    sql_bind(tag_stmt, 1, tag, strlen(tag), encoding);
    sql_bind(tag_stmt, 2, hash);

    sqlite3_step(tag_stmt);
    sqlite3_finalize(tag_stmt);
    return hash;
}

void add_tag_to_song_sql(sqlite3* db, const char* tag, song_hash_t song_hash, std::string& sql_out) {
    // We need to create the tag if it doesn't exist
    const tag_hash_t tag_hash = create_tag_sql(db, tag);

    // Actually add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_SONG_TABLE " (song_hash, tag_hash) VALUES (%d, %d);\n", song_hash, tag_hash);
}

void del_tag_from_song_sql(const char* tag, song_hash_t song_hash, std::string& sql_out) {
    const tag_hash_t tag_hash = crc32fast((const u8*)tag, strlen(tag));
    sqlgen(sql_out,
           "DELETE FROM " TAG_SONG_TABLE " WHERE " TAG_SONG_TABLE ".song_hash = %d AND " TAG_SONG_TABLE ".tag_hash = %d;\n",
           song_hash, tag_hash);
}

void link_tags_sql(sqlite3* db, const char* parent, const char* child, std::string& sql_out) {
    // We need to create the tags if they don't exist
    const tag_hash_t child_hash = create_tag_sql(db, child);
    const tag_hash_t parent_hash = create_tag_sql(db, parent);

    // Add the tag association
    sqlgen(sql_out, "INSERT INTO " TAG_PARENT_TABLE " (child_hash, parent_hash) VALUES (%d, %d);\n", child_hash, parent_hash);
}

void unlink_tags_sql(const char* parent, const char* child, std::string& sql_out) {
    const tag_hash_t child_hash = crc32fast((const u8*)child, strlen(child));
    const tag_hash_t parent_hash = crc32fast((const u8*)parent, strlen(parent));

    // Delete the tag association
    sqlgen(sql_out, "DELETE FROM " TAG_PARENT_TABLE " WHERE child_hash = %d AND parent_hash = %d\n", child_hash, parent_hash);
}

const char* strnchr(const char* text, char c, u32 len) {
    const char* result = strchr(text, c);
    if (result > &text[len]) {
        result = nullptr;
    }

    return result;
}

void search_tag(const char* tag, std::string& sql_out, bool standalone_query, s32 tag_len) {
    if (tag_len < 0) {
        tag_len = strlen(tag);
    }
    const char* colon = strnchr(tag, ':', tag_len);
    const ptrdiff_t namespace_len = ptrdiff_t(colon) - ptrdiff_t(tag);

    bool is_year = false;
    if (colon != nullptr) {
        // This tag has a namespace, split it up.
        if (strncmp(tag, "year", namespace_len) == 0) {
            is_year = true;
        }
    }

    if (!is_year) {
        // Generate the normal SQL
        const tag_hash_t tag_hash = crc32fast((u8*)tag, tag_len);
        sqlgen(sql_out, "SELECT song_hash FROM " RESOLVED_TAG_SONG_TABLE " WHERE tag_hash = %d", tag_hash);

        if (standalone_query) {
            // Terminate the statement
            sql_out.append(";\n");
        }
        return;
    }

    const char* tag_isolated = colon + 1;
    char* endptr = nullptr;
    const long year = strtol(tag_isolated, &endptr, 10);
    if (endptr != nullptr && *endptr == 's') {
        // This is a decade, do a range check
        const long decade = year - (year % 10);
        sqlgen(sql_out, "SELECT hash FROM songs WHERE year >= %ld AND year <= %ld", decade, decade + 9);
    } else {
        // Normal year query
        sqlgen(sql_out, "SELECT hash FROM songs WHERE year = %ld", year);
    }

    if (standalone_query) {
        // Terminate the statement
        sql_out.append(";\n");
    }
}

void search_many_tags_and(const char* const* tags, u64 num_tags, std::string& sql_out) {
    // TODO: Parent search optimization:
    // If we search for 2 tags where 1 is parented to the other, we could skip
    // one of them. e.g. "mos def AND rap" can be simplified to "rap", since the
    // parent/child table tells us that all results for "mos def" will be
    // included in "rap".

    // SELECTing all hashes lets us put a compound keyword in front of all
    // future SELECTs. The code is simpler, and it makes single negated tags
    // work correctly.
    sql_out.append("SELECT hash FROM songs\n");

    for (u64 i = 0; i < num_tags; i++) {
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
    sql_out.append(";\n");
}
