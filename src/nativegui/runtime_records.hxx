#pragma once
// This file is for structures representing database records, in the format most
// convenient for use at runtime. Class methods are meant to isolate application
// logic without being tied to any GUI code.

#include <string>
#include <vector>
#include <set>

#include <sqlite3.h>

#include <common/int.h>
#include <schema.hxx>

struct runtime_song {
    std::string name; // Song name
    time_t import_timestamp = 0;
    song_hash_t hash = 0; // Hash of the underlying audio file
    u32 release_year = 0;

    // @brief All tags attached to the song (by hash)
    // These are "pre-calculated", in that implied tags (parents) in the database
    // are included.
    // This is intended for immediate display.
    std::set<tag_hash_t> tags;
};

struct tag_autocomplete {
    // Number of results we show
    static const u8 AUTOCOMPLETE_SIZE = 5;

    // A hint to the UI that it should refocus the text box
    bool should_refocus_input = false;

    // A hint to call apply_selection() ASAP
    bool need_apply = false;

    // A hint to update the results ASAP
    bool need_refresh = false;

    s32 cur_idx = 0;
    // String the user typed into the text box
    std::string user_str;

    // Autocomplete results
    std::vector<std::string> candidates;

    /// @brief Change the selected result
    ///
    /// @param diff The direction the index should change in. Only the sign is
    /// kept, so any positive value adds 1, and any negative value subtracts 1.
    /// Automatically keeps the index in range for you.
    void update_selection(s8 diff) noexcept;

    // @brief Method for when the user confirms they want to use the autocomplete result
    void apply_selection() noexcept;

    /// @brief Get the current string that should be in the text box
    ///
    /// The only reason this isn't const is that it returns a mutable reference.
    std::string& get_current() noexcept;

    /// @brief Update the autocomplete candidates using the contents of @ref [user_str].
    /// @param db The database to query for results
    bool update_results(sqlite3* db) noexcept;

    // Wipe all text/state
    void reset() noexcept;
};

struct tag_search {
    // The tags currently being searched for
    std::vector<std::string> tags;

    // Autocomplete results and tag input buffer
    tag_autocomplete tac;

    std::vector<song_hash_t> result_hashes;

    /// @brief Add the current tag to the list of tags, or delete it if already there
    ///
    /// This should run after the user hits Enter (or equivalent) on the text
    /// input for the current tag. The string is added to the list of tags, or if
    /// it's already in the list, removed. Either way, the current tag is cleared.
    void finalize_current_tag(sqlite3* db) noexcept;

    /// @brief Run a query against the database and update the search results
    ///
    /// The database is not modified by this method.
    /// Clears the search results, then searches the database using the current
    /// list of tags. The "current tag" (text input state) is ignored.
    void update_results(sqlite3* db) noexcept;
};

struct tag_parents_t {
    std::vector<linked_tags> pairs;

    // Tag input fields the user will submit
    tag_autocomplete autocomp_child;
    tag_autocomplete autocomp_parent;

    // Add the tags the user typed to the database as a parent/child pair
    bool apply_current_pair(sqlite3* db) noexcept;
};
