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
#include <mutex>

#include "expression.hxx"
#include "autocomplete.hxx"

// Implementation for a tag input box with autocomplete.
// Often abbreviated as "TAC" / "tac" (looks a lot like "tag", sorry... - torph)
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
    std::string input_buf;
    tag_autocomplete tac = tag_autocomplete(&input_buf);

    void fix_ptr() {
        tac.user_str = &this->input_buf; // Fix pointer
    }

    runtime_song() = default;
    runtime_song(const char* title, time_t time, song_hash_t hash, u32 year) :
        runtime_song()
    {
        // Can't use initializer list with default ctor
        name = title;
        import_timestamp = time;
        this->hash = hash;
        release_year = year;
    }
};

// A headless search menu
struct tag_search {
    // The expression the user is currently searching for
    tag_expression expr;

    // Autocomplete results and tag input buffer
    std::string unused;
    tag_autocomplete tac;

    std::vector<song_hash_t> result_hashes;

    /// @brief Run a query against the database and update the search results
    ///
    /// The database is not modified by this method.
    /// Clears the search results, then searches the database using the current
    /// expression.
    void update_results(sqlite3* db) noexcept;
};

// Container for all "core" application state (all non-UI state). Some text box
// state is here too, but only those that involve autocomplete. Playlist
// functionality is also included, since a CLI frontend may want to play music.
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
    Music audio_stream = {};
    std::vector<song_hash_t> playlist;
    s32 playlist_pos = 0;
    std::recursive_mutex playlist_lock; // Lock for audio stream and playlist state

    // Tag parent input / display
    std::vector<linked_tags> parent_pairs;

    // Tag input fields the user will submit
    std::string child_input;
    tag_autocomplete tac_child = tag_autocomplete(&child_input);
    std::string parent_input;
    tag_autocomplete tac_parent = tag_autocomplete(&parent_input);

    // Debug performance timers
    std::unordered_map<const char*, float> timer_map;

    // Set this flag to trigger a reload at the start of the next frame
    bool need_reload = false;

    enum playlist_add_type {
        PLAYLIST_APPEND,
        PLAYLIST_NEXT,
        PLAYLIST_PREPEND,
    };

    // Add the results of a search to the current playlist
    void add_to_playlist(const song_hash_t* songs, u64 num_songs, playlist_add_type type = PLAYLIST_APPEND);

    void del_in_playlist(s32 pos);

    // Skip forward or back in the playlist, (wraps in both directions)
    void playlist_change_song(s8 diff) noexcept;

    // Move a song from one location to another in the playlist
    void playlist_move_song(u32 source, u32 target) noexcept;

    void playlist_update_stream() noexcept;

    // Apply the current tags in the parent/child inputs as a pair in the DB
    bool apply_tag_pair() noexcept;

    bool apply_defaults() noexcept;

    /// @brief Load songs from database
    bool load_songs_by_query(sqlite3* db);

    /// @brief Load songs and tags from the database
    ///
    /// Loads from scratch all songs and tags, the tag<->song mapping, and
    /// parent-child tag mappings. Automatically reloads all open searches using
    /// the new data
    bool load_from_db();

    /// @brief Load everything from the database
    blackbook_core(sqlite3* db, const char* files_dir);
    ~blackbook_core();
};
