#include "autocomplete.hxx"
#include <cassert>

#include "tags.hxx"

bool tag_autocomplete::update_results(sqlite3* db) noexcept {
    return autocomplete_tag(db, *user_str, candidates);
}

void tag_autocomplete::update_selection(s8 diff) noexcept {
    cur_idx += diff / abs(diff); // Add value clamped to -1 or 1

    const s32 size = (s32)candidates.size();
    if (cur_idx < 0) {
        // Wrap negatives around
        cur_idx = size;
    } else {
        // Wrap overflows around
        cur_idx %= size + 1;
    }
}

void tag_autocomplete::apply_selection() noexcept {
    // User selected a result. Copy to user buffer and wipe results.
    *user_str = current();
    candidates.clear();
    cur_idx = 0;
}

std::string& tag_autocomplete::current() noexcept {
    // Keep in bounds
    cur_idx = CLAMP(0, cur_idx, candidates.size());

    if (cur_idx == 0) {
        return *user_str;
    } else {
        return candidates[cur_idx - 1];
    }
}

void tag_autocomplete::reset() noexcept {
    candidates.clear();
    *user_str = "";
    cur_idx = 0;
}