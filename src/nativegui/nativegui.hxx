#pragma once
#include <GLFW/glfw3.h>
#include <sqlite3.h>

#include <map>

#include "runtime_records.hxx"

// Struct for all GUI state
struct nativegui {
    // Set by ctor to indicate results (instead of an exception)
    bool initialized = false;

    // Doubles as song storage, and a lookup by hash
    std::map<u32, runtime_song> songs;

    // Doubles as tag storage, and a lookup by hash
    std::map<u32, std::string> tags;

    /// @brief Load songs and tags from the database
    ///
    /// Loads from scratch all songs and tags, the tag<->song mapping, and
    /// parent-child tag mappings. Automatically reloads all open searches using
    /// the new data
    bool load_from_db(sqlite3* db);
    // Maybe also add a "lazy" version that only loads new songs whose hash we
    // don't recognize

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db);
};

/// @brief Main function for the native PC frontend
///
/// Don't call this function directly. Pass it as a function pointer to
/// gui_loop(), along with a nativegui* for the context.
bool gui_main(void* ctx, GLFWwindow* window);
