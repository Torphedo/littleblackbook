#pragma once
#include <GLFW/glfw3.h>
#include <sqlite3.h>

#include <map>

#include "runtime_records.hxx"

// Compile SQL and print detailed error messages on failure
sqlite3_stmt* compile_sql(const char* sql, s32 sql_len, sqlite3* db);

// Struct for all GUI state
struct nativegui {
    // Set by ctor to indicate results (instead of an exception)
    bool initialized = false;

    sqlite3* db = nullptr;

    // Doubles as song storage, and a lookup by hash
    std::map<u32, runtime_song> song_map;

    // Doubles as tag storage, and a lookup by hash
    std::map<u32, std::string> tags;

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

    // All song hashes that need their editing window drawn
    std::set<u32> song_editors;

    tag_search search;

    bool draw_song_editor(runtime_song& song);

    void draw_search_menu() noexcept;

    void draw_song_list() noexcept;

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db);
};

/// @brief Main function for the native PC frontend
///
/// Don't call this function directly. Pass it as a function pointer to
/// gui_loop(), along with a nativegui* for the context.
bool gui_main(void* ctx, GLFWwindow* window);
