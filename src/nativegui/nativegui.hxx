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

    std::string lyric_search_input;
    std::set<song_hash_t> lyric_search_results;

    struct window_def {
        const char* window_name;
        bool (nativegui::*draw)();
    };

    bool draw_song_editor(runtime_song& song);

    bool draw_search_menu(const char* win_title, tag_search& search) noexcept;

    bool draw_song_list() noexcept;

    bool draw_tag_parents() noexcept;

    bool draw_toolbar() noexcept;

    bool draw_import_progress() noexcept;

    bool draw_timers() noexcept;

    bool draw_lyric_search() noexcept;

    bool draw_player() noexcept;

    static constexpr window_def windows[] = {
        {   .window_name = "Song List",
            .draw = &nativegui::draw_song_list,
        },
        {   .window_name = "Tag Parents",
            .draw = &nativegui::draw_tag_parents,
        },
        {   .window_name = "Import Progress",
            .draw = &nativegui::draw_import_progress,
        },
        {   .window_name = "Performance Timers",
            .draw = &nativegui::draw_timers,
        },
        {   .window_name = "Lyric Search",
            .draw = &nativegui::draw_lyric_search,
        },
        {   .window_name = "Music Player",
            .draw = &nativegui::draw_player,
        },
    };
    bool windows_active[ARRAY_SIZE(windows)] = {};

    // We need to toggle this window from another function
    static constexpr u8 IMPORT_WINDOW_IDX = 2;
    static_assert(windows[IMPORT_WINDOW_IDX].draw == &nativegui::draw_import_progress);

    // File import state
    import_stats_t import_stats;
    std::thread import_thread;

    // Temporary storage for import process
    std::vector<std::string> import_paths;
    std::vector<const char*> import_path_ptrs;

    bool InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac);

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db, const char* files_dir) noexcept;


    /// @brief Main function for the native PC frontend
    bool gui_main(GLFWwindow* window) noexcept;

    /// This is a simple wrapper to be used as a C function pointer for
    /// gui_loop(). The [void* ctx] should be an instance of this class.
    static bool gui_main_static(void* ctx, GLFWwindow* window) noexcept {
        return ((nativegui*)ctx)->gui_main(window);
    }
};
