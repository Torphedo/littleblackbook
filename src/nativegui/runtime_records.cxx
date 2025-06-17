#include <cctype>
#include <algorithm>

#include "runtime_records.hxx"
#include "nativegui/nativegui.hxx"
#include "tags.hxx"

void tag_search::finalize_current_tag(sqlite3* db) noexcept {
    // Go to lowercase to make it case-insensitive
    // TODO: This will break for any non-ASCII characters. We need to use the ICU
    // library to get correct behaviour.
    // https://stackoverflow.com/questions/313970/how-to-convert-an-instance-of-stdstring-to-lower-case#313990
    // https://icu.unicode.org
    // We might also want to just convert to the closest ASCII character, since
    // I can't type Unicode without a numpad anyway. - torph
    std::transform(current_tag.begin(), current_tag.end(), current_tag.begin(), ::tolower);

    // Remove the tag if it was already in the list
    auto iter = std::find(tags.begin(), tags.end(), current_tag);
    if (iter != tags.end()) {
        tags.erase(iter);
        current_tag = "";
        update_results(db);
        return;
    }

    // Add the tag as normal
    tags.push_back(current_tag);
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

    for (std::string& tag : tags) {
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
        const u32 hash = sqlite3_column_int(query, 6);
        result_hashes.push_back(hash);
    }
}
