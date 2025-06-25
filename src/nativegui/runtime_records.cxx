#include "runtime_records.hxx"
#include <algorithm>
#include <cassert>

#include <common/crc32.h>

#include <sqlgen.hxx>
#include <stringcase.hxx>
#include <tags.hxx>
#include <schema.hxx>

void tag_search::finalize_current_tag(sqlite3* db) noexcept {
    if (tac.get_current().empty()) {
        update_results(db);
        return;
    }

    // Go to lowercase to make it case-insensitive
    str_tolower(tac.get_current());

    // Remove the tag if it was already in the list
    const auto iter = std::find(tags.begin(), tags.end(), tac.get_current());
    if (iter != tags.end()) {
        tags.erase(iter);
    } else {
        // Add the tag as normal
        tags.push_back(tac.get_current());
    }

    tac.reset();
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

bool tag_autocomplete::update_results(sqlite3* db) noexcept {
    // Wipe previous results
    candidates.clear();

    // We use this to skip the minus sign in the generated SQL
    const bool minus = user_str.c_str()[0] == '-';
    std::string sql;
    // This searches the tag table, then uses the result to find the namespace string
    sqlgen(sql, R"(
        SELECT DISTINCT ns.namespace, t.tag FROM
        (SELECT * FROM tag_search('"%s" *') ORDER BY rank LIMIT %d) result
        JOIN tags t ON t.hash = result.hash
        JOIN namespaces ns ON ns.hash = t.namespace_hash;
    )",
    user_str.c_str() + minus, AUTOCOMPLETE_SIZE);

    sqlite3_stmt* stmt = compile_sql(sql.c_str(), -1, db);
    if (stmt == nullptr) {
        return false; // Error printed for us
    }

    int res = SQLITE_OK;
    while ((res = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string result = minus ? "-" : ""; // Use - prefix if needed
        const char* nspace = (const char*)sqlite3_column_text(stmt, 0);
        const char* tag = (const char*)sqlite3_column_text(stmt, 1);

        result += std::string(nspace) + ":" + tag;
        candidates.push_back(result);
    }

    return true;
}

void tag_autocomplete::update_selection(s8 diff) noexcept {
    cur_idx += diff / abs(diff); // Add value clamped to -1 or 1

    if (cur_idx < 0) {
        // Wrap negatives around
        cur_idx = candidates.size();
    } else {
        // Wrap overflows around
        cur_idx %= candidates.size() + 1;
    }
}

void tag_autocomplete::apply_selection() noexcept {
    // User selected a result. Copy to user buffer and wipe results.
    user_str = get_current();
    candidates.clear();
    cur_idx = 0;
}

std::string& tag_autocomplete::get_current() noexcept {
    assert(cur_idx <= candidates.size() && cur_idx >= 0 && "Autocomplete index out of bounds!");

    if (cur_idx == 0) {
        return user_str;
    } else {
        return candidates[cur_idx - 1];
    }
}

void tag_autocomplete::reset() noexcept {
    candidates.clear();
    user_str = "";
    cur_idx = 0;
}
