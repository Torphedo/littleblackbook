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
