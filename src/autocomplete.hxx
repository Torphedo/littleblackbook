#pragma once
#include <string>
#include <vector>

#include <sqlite3.h>
#include <common/int.h>

struct tag_autocomplete {
    // String the user typed into the text box
    std::string* user_str = nullptr;

    // Autocomplete results
    std::vector<std::string> candidates;

    // A hint to the UI that it should refocus the text box
    bool need_refocus = false;

    // A hint to call apply_selection() ASAP
    bool need_apply = false;

    // A hint to update the TAC results ASAP
    bool need_refresh = false;

    s32 cur_idx = 0;

    /// @brief Change the selected result
    ///
    /// @param diff The direction the index should change in. Only the sign is
    /// kept, so any positive value adds 1, and any negative value subtracts 1.
    /// Automatically keeps the index in range for you.
    void update_selection(s8 diff) noexcept;

    /// @brief Method for when the user confirms they want to use the autocomplete result
    void apply_selection() noexcept;

    /// @brief Get the current string that should be in the text box
    ///
    /// The only reason this isn't const is that it returns a mutable reference.
    std::string& current() noexcept;

    /// @brief Update the autocomplete candidates using the contents of @ref [user_str].
    /// @param db The database to query for results. The database won't be modified.
    bool update_results(sqlite3* db) noexcept;

    // Wipe all text/state
    void reset() noexcept;

    tag_autocomplete() = default;
    explicit tag_autocomplete(std::string* user_str) : user_str(user_str) {}
};


