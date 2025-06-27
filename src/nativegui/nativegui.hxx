#pragma once
#include <GLFW/glfw3.h>
#include <sqlite3.h>
#include <imgui.h>

#include <map>
#include <thread>
#include <unordered_map>

#include <schema.hxx>
#include <song.hxx>
#include "runtime_records.hxx"

// Struct for all GUI state
struct nativegui {
    // Set by ctor to indicate results (instead of an exception)
    bool initialized = false;

    sqlite3* db = nullptr;
    const char* files_dir;

    // Doubles as song storage, and a lookup by hash
    std::map<song_hash_t, runtime_song> song_map;

    std::map<tag_hash_t, std::string> namespaces;

    // Doubles as tag storage, and a lookup by hash
    std::map<tag_hash_t, std::string> tags;

    // Set this flag to trigger a reload at the start of the next frame
    bool need_reload = false;

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

    // Search window state
    bool show_search = false; // Toggle for search window
    tag_search search;

    // Import window state
    bool show_import_window = false;
    import_stats_t import_stats;
    std::thread import_thread;

    // Temporary storage for import process
    std::vector<std::string> import_paths;
    std::vector<const char*> import_path_ptrs;

    // Tag parent window state
    bool show_tag_parents = false;
    tag_parents_t tag_parents;

    bool draw_song_editor(runtime_song& song);

    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac);
    void draw_search_menu() noexcept;

    void draw_song_list() noexcept;

    void draw_tag_parents() noexcept;

    void draw_toolbar() noexcept;

    void draw_import_progress() noexcept;

    void draw_timers() noexcept;

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db, const char* files_dir);
};

/// @brief Main function for the native PC frontend
///
/// Don't call this function directly. Pass it as a function pointer to
/// gui_loop(), along with a nativegui* for the context.
bool gui_main(void* ctx, GLFWwindow* window);
