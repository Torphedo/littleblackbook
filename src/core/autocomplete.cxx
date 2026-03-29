#include "autocomplete.hxx"

#include "tags.hxx"
#include "expression.hxx"

std::string_view tag_autocomplete::tac_substr() const noexcept {
    std::queue<substr_t> tokens = shatter_str(user_str->c_str(), user_str->length());
    const std::string_view empty(user_str->c_str(), 0);
    const std::string_view tok = tokens.empty() ? empty : tokens.back();
    return tok;
}

bool tag_autocomplete::update_results(sqlite3* db) noexcept {
    return autocomplete_tag(db, tac_substr(), candidates);
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
    if (cur_idx == 0) {
        // No suggestion selected, nothing to apply.
        return;
    }

    // User selected a result. Copy to user buffer and wipe results.
    const std::string_view tok = tac_substr();
    s64 pos = tok.data() - user_str->data();

    if (pos < 0) {
        return; // Wrong pointer or something
    }

    user_str->replace(pos, tok.length(), current());
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