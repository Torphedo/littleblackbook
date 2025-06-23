#include "runtime_records.hxx"
#include <algorithm>

#include <common/crc32.h>

#include <sqlgen.hxx>
#include <stringcase.hxx>
#include <tags.hxx>
#include <schema.hxx>

void tag_search::finalize_current_tag(sqlite3* db) noexcept {
    if (current_tag.empty()) {
        update_results(db);
        return;
    }

    // Go to lowercase to make it case-insensitive
    str_tolower(current_tag);

    // Remove the tag if it was already in the list
    const auto iter = std::find(tags.begin(), tags.end(), current_tag);
    if (iter != tags.end()) {
        tags.erase(iter);
    } else {
        // Add the tag as normal
        tags.push_back(current_tag);
    }

    current_tag = "";
    update_results(db);
}

void tag_search::update_results(sqlite3* db) noexcept {
    std::string sql;

    // Clear existing results
    result_hashes.clear();

    // Get a pointer array for underlying function to use
    std::vector<const char*> tags_temp;
    tags_temp.reserve(tags.size());

    for (const std::string& tag : tags) {
        tags_temp.push_back(tag.c_str());
    }

    // Generate SQL query
    search_many_tags_and(tags_temp.data(), tags.size(), sql);

    sqlite3_stmt* query = compile_sql(sql.c_str(), sql.size(), db);
    if (!query) {
        return; // Error printed for us
    }

    int result = SQLITE_OK;
    while ((result = sqlite3_step(query)) == SQLITE_ROW) {
        const tag_hash_t hash = sqlite3_column_int(query, 6);
        result_hashes.push_back(hash);
    }

    sqlite3_finalize(query);
}

bool tag_autocomplete::update_results(const char* user_str, sqlite3* db) {
    // Wipe previous results
    candidates.clear();

    std::string sql;
    sqlgen(sql, "SELECT * FROM tag_search('\"%s\" *') ORDER BY rank;", user_str);

    sqlite3_stmt* stmt = compile_sql(sql.c_str(), -1, db);
    if (stmt == nullptr) {
        return false; // Error printed for us
    }

    int res = SQLITE_OK;
    while ((res = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* result = (const char*)sqlite3_column_text(stmt, 0);
        candidates.push_back(result);
    }

    return true;
}
