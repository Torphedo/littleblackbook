#pragma once
#include <GLFW/glfw3.h>
#include <sqlite3.h>
#include <imgui.h>

#include <map>
#include <unordered_map>

#include "runtime_records.hxx"
#include "schema.hxx"

// Struct for all GUI state
struct nativegui {
    // Set by ctor to indicate results (instead of an exception)
    bool initialized = false;

    sqlite3* db = nullptr;

    // Doubles as song storage, and a lookup by hash
    std::map<song_hash_t, runtime_song> song_map;

    // Doubles as tag storage, and a lookup by hash
    std::map<tag_hash_t, std::string> tags;

    // Set this flag to trigger a reload at the start of the next frame
    bool need_reload = false;

    bool show_tag_parents = false;
    tag_parents_t tag_parents;

    // Add the tags the user typed to the database as a parent/child pair
    void apply_parent_child_pair() noexcept;

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

    /* ======================================================================= */
    /*                   ImGui Drawing Functions & UI State                    */
    /* ======================================================================= */

    // Debug performance timers
    bool show_timers = false;
    std::unordered_map<const char*, float> timer_map;

    // All song hashes that need their editing window drawn
    std::set<song_hash_t> song_editors;

    // Set after the user enters a tag to keep keyboard focus in the text input
    bool show_search = false; // Toggle for search window
    bool search_focus_next_frame = false;
    tag_search search;

    bool draw_song_editor(runtime_song& song);

    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, std::string& tag, tag_autocomplete& tac);
    void draw_search_menu() noexcept;

    void draw_song_list() noexcept;

    void draw_tag_parents() noexcept;

    void draw_toolbar() noexcept;

    void draw_timers() noexcept;

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db);
};

/// @brief Main function for the native PC frontend
///
/// Don't call this function directly. Pass it as a function pointer to
/// gui_loop(), along with a nativegui* for the context.
bool gui_main(void* ctx, GLFWwindow* window);
