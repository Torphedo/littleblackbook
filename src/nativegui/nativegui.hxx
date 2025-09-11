#pragma once
#include <GLFW/glfw3.h>
#include <sqlite3.h>
#include <imgui.h>

#include <thread>

#include <blackbook_core.hxx>
#include <import.hxx>
#include <schema.hxx>
#include "imgui_internal.h"
#include "thumbnails.hxx"

// Struct for all GUI state
struct nativegui {
    // Set by ctor to indicate results (instead of an exception)
    bool initialized = false;

    blackbook_core core;
    bool music_thread_stop_flag = false;
    std::thread music_thread;
    thumbnail_storage thumbnails;

    // All song hashes that need their editing window drawn
    std::set<song_hash_t> song_editors;

    std::string lyric_search_input;
    std::set<song_hash_t> lyric_search_results;

    // When the user drags a song around in the playlist, this is the index of
    // the song they're dragging.
    s32 playlist_drag_start = -1;

    // This struct lets us loop over known windows, which makes it easy to add
    // new ones without any other boilerplate.
    struct window_def {
        const char* window_name;
        bool (nativegui::*draw)(); // Pointer to member function
    };

    // Draw read-only song metadata using ImGui::Text()
    void draw_song_info(const runtime_song& song) const noexcept;

    bool draw_song_row(song_hash_t hash, bool highlight, float thumb_size = 100.0f) noexcept;

    bool draw_search_menu(const char* win_title, tag_search& search) noexcept;

    // Draw a standalone window (with Begin/End) that may edit the song (in memory and DB)
    bool window_song_editor(runtime_song& song);

    // Standalone windows (minus Begin/End, which is handled by the caller)
    bool window_songs() noexcept;
    bool window_tag_parents() noexcept;
    bool window_draw_import_progress() noexcept;
    bool window_timers() noexcept;
    bool window_lyric_search() noexcept;
    bool window_playlist() noexcept;
    bool toolbar_player() noexcept;
    bool toolbar_main() noexcept;

    // Used to automatically draw windows, create window toggles in the toolbar, etc.
    static constexpr window_def windows[] = {
        {   .window_name = "Import Progress",
            .draw = &nativegui::window_draw_import_progress,
        },
        {   .window_name = "Song List",
            .draw = &nativegui::window_songs,
        },
        {   .window_name = "Tag Parents",
            .draw = &nativegui::window_tag_parents,
        },
        {   .window_name = "Performance Timers",
            .draw = &nativegui::window_timers,
        },
        {   .window_name = "Lyric Search",
            .draw = &nativegui::window_lyric_search,
        },
        {   .window_name = "Playlist",
            .draw = &nativegui::window_playlist,
        },
    };
    bool windows_active[ARRAY_SIZE(windows)] = {};

    // We need to toggle this window from another function, so need a constant for it
    static constexpr u8 IMPORT_WINDOW_IDX = 0;

    // File import state
    // TODO: Should this be on the core?
    import_stats_t import_stats;
    std::thread import_thread;

    // Temporary storage for import process
    std::vector<std::string> import_paths;
    // TODO: This is stupid and janky and shouldn't need to exist
    std::vector<const char*> import_path_ptrs;

    /// @brief Load everything needed to start the GUI from the database
    nativegui(sqlite3* db, const char* files_dir) noexcept;

    ~nativegui() noexcept;

    /// @brief Main function for the native PC frontend
    bool gui_main(GLFWwindow* window) noexcept;

    /// This is a simple wrapper to be used as a C function pointer for
    /// gui_loop(). The [void* ctx] should be an instance of this class.
    static bool gui_main_static(void* ctx, GLFWwindow* window) noexcept {
        return ((nativegui*)ctx)->gui_main(window);
    }

    // Templated methods are kept at the bottom for readability

    // We need a template to handle STL collections generically (since we need to
    // use this on maps and vectors and sets).
    template<typename T>
    void draw_songs(const T& hashes, u32 thumb_size = 100) noexcept {
        if (ImGui::BeginTable("song table", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
            // Make header row that never scrolls away
            ImGui::TableSetupScrollFreeze(0, 1);

            // Setup table header
            ImGui::TableSetupColumn("Title");
            ImGui::TableSetupColumn("Year");
            ImGui::TableHeadersRow();

            // Clipper allows us to only draw rows that are visible.
            // On my system w/ 1381 song entries, this reduced draw time from
            // ~1-1.2ms to ~0.1-0.2ms, and reduced CPU usage a lot.
            ImGuiListClipper clipper;
            clipper.Begin((u32)hashes.size());
            while (clipper.Step()) {
                // I'd love to use operator[] here and not have to iterate over
                // things that are skipped, but some (like map key iterators)
                // don't implement operator[].
                s32 i = 0;
                for (song_hash_t hash : hashes) {
                    if (i < clipper.DisplayStart) {
                        i++;
                        continue;
                    }
                    if (i >= clipper.DisplayEnd) {
                        i++;
                        continue;
                    }
                    if (draw_song_row(hash, false)) {
                        song_editors.insert(hash);
                    }
                    i++;
                }
            }
            clipper.End();
            ImGui::EndTable();
        }
    }
};
