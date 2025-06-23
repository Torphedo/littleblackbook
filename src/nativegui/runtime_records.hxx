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
    u32 cur_idx = 0;
    std::vector<std::string> candidates;

    bool update_results(const char* user_str, sqlite3* db);
};

struct tag_search {
    // The tags currently being searched for
    std::vector<std::string> tags;

    // Buffer for the tag the user is currently typing, or an autocomplete result.
    std::string current_tag;
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

    // Input fields the user will submit
    std::string input_child;
    std::string input_parent;
};
