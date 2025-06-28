#pragma once
#include <GLFW/glfw3.h>
#include <sqlite3.h>
#include <imgui.h>

#include <thread>

#include <blackbook_core.hxx>
#include <import.hxx>
#include <schema.hxx>

// Struct for all GUI state
struct nativegui {
    // Set by ctor to indicate results (instead of an exception)
    bool initialized = false;

    blackbook_core core;

    // All song hashes that need their editing window drawn
    std::set<song_hash_t> song_editors;

    // Window visibility states
    bool show_search = false;
    bool show_tag_parents = false;
    bool show_import_window = false;
    bool show_timers = false;

    // File import state
    import_stats_t import_stats;
    std::thread import_thread;

    // Temporary storage for import process
    std::vector<std::string> import_paths;
    std::vector<const char*> import_path_ptrs;

    bool draw_song_editor(runtime_song& song);

    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac);
    void draw_search_menu(const char* win_title, tag_search& search) noexcept;

    void draw_song_list() noexcept;

    void draw_tag_parents() noexcept;

    void draw_toolbar() noexcept;

    void draw_import_progress() noexcept;

    void draw_timers() noexcept;

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db, const char* files_dir) noexcept;


    /// @brief Main function for the native PC frontend
    ///
    /// Don't call this function directly. Pass it as a function pointer to
    /// gui_loop(), along with an instance of this class as the context pointer.
    static bool gui_main(void* ctx, GLFWwindow* window) noexcept;
};
