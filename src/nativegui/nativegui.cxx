#include "nativegui.hxx"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <common/logging.h>
#include <common/vfile.h>

#include <sqlgen.hxx>
#include <schema.hxx>
#include <scope_timer.hxx>

bool nativegui::load_songs_by_query(sqlite3* db, const char* query) {
    // We don't bother getting album/artist, since those are stored as tags.
    static const char fetchsongs_sql[] = "SELECT title, year, lyrics, hash, import_timestamp, duration_secs FROM songs";
    if (!query) {
        query = fetchsongs_sql;
    }

    sqlite3_stmt* fetchsongs = compile_sql(fetchsongs_sql, ARRAY_SIZE(fetchsongs_sql) + 1, db);
    if (!fetchsongs) {
        sqlite3_finalize(fetchsongs);
        return false; // Error already printed for us
    }

    // Load songs
    int exec_result = 0;
    while ((exec_result = sqlite3_step(fetchsongs)) == SQLITE_ROW) {
        const unsigned char* title = sqlite3_column_text(fetchsongs, 0);
        const u32 year = sqlite3_column_int(fetchsongs, 1);
        const song_hash_t hash = sqlite3_column_int(fetchsongs, 3);
        const time_t time = sqlite3_column_int(fetchsongs, 4);

        // Construct in-place to encourage use of the move ctor, to avoid cloning strings
        song_map[hash] = (runtime_song) {
            .name = (char*)title,
            .import_timestamp = time,
            .hash = hash,
            .release_year = year,
        };
    }

    sqlite3_finalize(fetchsongs);
    return true;
}

bool nativegui::load_from_db() {
    // Wipe current state
    song_map.clear();
    tags.clear();
    tag_parents.clear();

    static const char tags_sql[] = "SELECT tag, hash FROM tags";
    static const char tagmap_sql[] = "SELECT tag_hash, song_hash FROM " TAG_SONG_TABLE;
    static const char tagparents_sql[] = "SELECT parent_hash, child_hash FROM " TAG_PARENT_TABLE;

    // Try to load songs
    if (!load_songs_by_query(db)) {
        return false; // Error printed for us
    }

    // Compile all of our basic SQL queries
    sqlite3_stmt* fetchtags = compile_sql(tags_sql, ARRAY_SIZE(tags_sql) + 1, db);
    sqlite3_stmt* fetchtagmap = compile_sql(tagmap_sql, ARRAY_SIZE(tagmap_sql) + 1, db);
    sqlite3_stmt* fetchtagparents = compile_sql(tagparents_sql, ARRAY_SIZE(tagparents_sql) + 1, db);
    if (!fetchtags || !fetchtagmap || !fetchtagparents) {
        sqlite3_finalize(fetchtags);
        sqlite3_finalize(fetchtagmap);
        sqlite3_finalize(fetchtagparents);
        return false; // Error already printed for us
    }

    // TODO: Check for errors after each sqlite3_step() loop so we can get detailed error messages

    // Load tags
    int exec_result = SQLITE_OK;
    while ((exec_result = sqlite3_step(fetchtags)) == SQLITE_ROW) {
        const unsigned char* tag = sqlite3_column_text(fetchtags, 0);
        const tag_hash_t hash = sqlite3_column_int(fetchtags, 1);

        // Add to the map
        tags[hash] = (char*)tag;
    }

    // Attach tags to their corresponding songs
    while ((exec_result = sqlite3_step(fetchtagmap)) == SQLITE_ROW) {
        const tag_hash_t tag_hash = sqlite3_column_int(fetchtagmap, 0);
        const song_hash_t song_hash = sqlite3_column_int(fetchtagmap, 1);

        // Add the tag to the song
        if (song_map.count(song_hash)) {
            song_map[song_hash].tags.insert(tag_hash);
        }
    }

    // Add parented tags to songs as needed
    while ((exec_result = sqlite3_step(fetchtagparents)) == SQLITE_ROW) {
        const tag_hash_t parent_hash = sqlite3_column_int(fetchtagparents, 0);
        const tag_hash_t child_hash = sqlite3_column_int(fetchtagparents, 1);

        // Very inefficiently, add all tag parents.
        // We probably can just do a more complex query to do this more efficiently:
        // SELECT song_hash FROM tagmap WHERE tag_hash IN (SELECT child_hash FROM tag_parents)
        // That should filter out songs that don't need parent tags added.
        for (auto& pair : song_map) {
            auto& song = pair.second;
            for (tag_hash_t tag_hash : song.tags) {
                if (tag_hash == child_hash) {
                    song.tags.insert(parent_hash);
                }
            }
        }
    }

    // Free our compiled SQL queries
    sqlite3_finalize(fetchtags);
    sqlite3_finalize(fetchtagmap);
    sqlite3_finalize(fetchtagparents);
    return true;
}

nativegui::nativegui(sqlite3* db) {
    bool result = true;
    this->db = db;

    {
        const scope_timer load_timer(timer_map, "initial_load");
        result &= load_from_db();
    }
    LOG_MSG(info, "Finished loading from database in %.3fms\n", timer_map["initial_load"]);
    if (!result) {
        db = nullptr;
    }

    initialized = result;
}

bool nativegui::draw_song_editor(runtime_song& song) {
    char win_title_buf[512] = {0};
    snprintf(win_title_buf, sizeof(win_title_buf), "Song editor [%d]", song.hash);
    bool open = true;
    open &= ImGui::Begin(win_title_buf, &open);
    if (!open) {
        // User closed the window or it's not visible
        ImGui::End();
        return false;
    }

    ImGui::Text("Title: %s", song.name.c_str());
    ImGui::Text("Released: %u", song.release_year);

    if (song.tags.size() > 0) {
        ImGui::Text("Tags:");
        for (song_hash_t hash : song.tags) {
            const std::string& tag = tags[hash];
            ImGui::Text("%s", tag.c_str());
        }
        ImGui::Text("\n");
    }

    ImGui::Text("Hash: %d", song.hash);
    ImGui::Text("Imported @ %lu", song.import_timestamp);

    ImGui::End();
    return true;
}

void nativegui::draw_search_menu() noexcept {
    ImGui::Begin("Search");

    // Show current tags and input box
    for (const std::string& tag : search.tags) {
        ImGui::Text("%s", tag.c_str());
    }

    // Focus text input so user can keep typing
    if (search_focus_next_frame) {
        search_focus_next_frame = false; // Reset flag
        ImGui::SetKeyboardFocusHere();
    }

    // Input for next tag
    if (ImGui::InputText("Input tag: ", &search.current_tag, ImGuiInputTextFlags_EnterReturnsTrue)) {
        search_focus_next_frame = true;
        const scope_timer main_timer(timer_map, "last_search");
        // This also executes the search and updates our state
        search.finalize_current_tag(db);
    }

    // Display results
    for (song_hash_t hash : search.result_hashes) {
        const runtime_song& s = song_map[hash];
        if (ImGui::Selectable(s.name.c_str())) {
            song_editors.insert(s.hash);
        }
    }

    ImGui::End();
}

void nativegui::draw_song_list() noexcept {
    ImGui::Begin("Song List");
    for (const auto& pair : song_map) {
        const runtime_song& s = pair.second;
        if (ImGui::Selectable(s.name.c_str())) {
            song_editors.insert(s.hash);
        }
    }

    ImGui::End();
}

void nativegui::draw_toolbar() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool import_files = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_I, false);

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                import_files |= ImGui::MenuItem("Import files", "Ctrl-I");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Performance Timers", nullptr, &this->show_timers);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }


    if (import_files) {
        // TODO: Implement file import
    }
}

void nativegui::draw_timers() noexcept {
    if (!show_timers) {
        return;
    }

    ImGui::Begin("Performance Timers", &show_timers);
    for (const auto& entry : timer_map) {
        ImGui::Text("%s: %.2lfms", entry.first, entry.second);
    }
    ImGui::End();
}

bool gui_main(void* ctx, GLFWwindow* window) {
    nativegui* gui = (nativegui*)ctx;
    const scope_timer main_timer(gui->timer_map, "main_draw");

    gui->draw_toolbar();
    gui->draw_timers();
    gui->draw_song_list();
    gui->draw_search_menu();

    std::vector<song_hash_t> editors_to_close(0);
    for (song_hash_t song_hash : gui->song_editors) {
        if (!gui->draw_song_editor(gui->song_map[song_hash])) {
            editors_to_close.push_back(song_hash);
        }
    }

    // We can't edit the set while iterating over it
    for (song_hash_t hash : editors_to_close) {
        gui->song_editors.erase(hash);
    }

    ImGui::ShowDemoWindow();

    return true;
}
