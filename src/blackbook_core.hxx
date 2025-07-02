#pragma once
// This file is for structures representing database records, in the format most
// convenient for use at runtime. We almost always sacrifice memory for speed and
// rendering simplicity (unless it creates duplicate state).
//
// The goal here is to make all the behaviour "headless" and separate from GUI
// code. That way, we can easily expose features on CLI or write a new frontend
// as a thin shell around this API.

#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>

#include <sqlite3.h>
#include <raudio.h>

#include <common/int.h>
#include <schema.hxx>

// Implementation for a tag input box with autocomplete.
// Often abbreviated as "TAC" / "tac" (looks a lot like "tag", sorry... - torph)
struct tag_autocomplete {
    // Number of results we show
    static const u8 AUTOCOMPLETE_SIZE = 5;

    // A hint to the UI that it should refocus the text box
    bool need_refocus = false;

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
    std::string& current() noexcept;

    /// @brief Update the autocomplete candidates using the contents of @ref [user_str].
    /// @param db The database to query for results. The database won't be modified.
    bool update_results(sqlite3* db) noexcept;

    // Wipe all text/state
    void reset() noexcept;
};

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

    // Text input for the user to add tags to a song
    tag_autocomplete tac;
};

// A headless search menu
struct tag_search {
    // The tags currently being searched for
    // TODO: Can we make this a set of hashes? How do we deal with negated tags?
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

// Container for all "core" application state (all non-UI state). Some text box
// state is here too, but only those that involve autocomplete.
struct blackbook_core {
    bool initialized = false;
    sqlite3* db = nullptr;
    const char* files_dir;

    // Doubles as song storage, and a lookup by hash
    std::map<song_hash_t, runtime_song> song_map;

    std::map<tag_hash_t, std::string> namespaces;

    // Doubles as tag storage, and a lookup by hash
    std::map<tag_hash_t, std::string> tags;

    std::vector<tag_search> searches;

    // State for music player features
    Music audio_stream;
    std::vector<song_hash_t> playlist;
    u32 playlist_pos = 0;

    // Tag parent input / display
    std::vector<linked_tags> parent_pairs;

    // Tag input fields the user will submit
    tag_autocomplete tac_child;
    tag_autocomplete tac_parent;

    // Debug performance timers
    std::unordered_map<const char*, float> timer_map;

    // Set this flag to trigger a reload at the start of the next frame
    bool need_reload = false;

    // Add the results of a search to the current playlist
    void add_search_to_playlist(const song_hash_t* songs, u32 num_songs, bool clear_first = false);

    void playlist_change_song(s8 diff);

    // Apply the current tags in the parent/child inputs as a pair in the DB
    bool apply_tag_pair() noexcept;

    /// @brief Load songs from database, optionally with a custom query
    bool load_songs_by_query(sqlite3* db, const char* query = nullptr);

    /// @brief Load songs and tags from the database
    ///
    /// Loads from scratch all songs and tags, the tag<->song mapping, and
    /// parent-child tag mappings. Automatically reloads all open searches using
    /// the new data
    bool load_from_db();
    // Maybe also add a "lazy" version that only loads new songs whose hash we
    // don't recognize

    /// @brief Load everything from the database
    blackbook_core(sqlite3* db, const char* files_dir);
};
