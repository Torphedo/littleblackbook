#include "nativegui.hxx"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>

#include <common/logging.h>
#include <common/vfile.h>
#include <common/crc32.h>

#include <sqlgen.hxx>
#include <schema.hxx>
#include <scope_timer.hxx>
#include <song.hxx>
#include "runtime_records.hxx"
#include "tags.hxx"
#include "nfde_wrapper.hxx"

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
    { // Scope for timer
    const scope_timer load_timer(timer_map, "db_load");

    // Wipe current state
    song_map.clear();
    tags.clear();
    tag_parents.pairs.clear();

    static const char namespaces_sql[] = "SELECT namespace, hash FROM namespaces";
    static const char tags_sql[] = "SELECT tag, hash, namespace_hash FROM tags";
    // This ensures that tags displayed on each song include parents up to 3 layers deep
    static const char tagmap_sql[] = "SELECT tag_hash, song_hash FROM " RESOLVED_TAG_SONG_TABLE ";";
    static const char tagparents_sql[] = "SELECT parent_hash, child_hash FROM " TAG_PARENT_TABLE;

    // Try to load songs
    if (!load_songs_by_query(db)) {
        return false; // Error printed for us
    }

    // Compile all of our basic SQL queries
    sqlite3_stmt* fetchnamespaces = compile_sql(namespaces_sql, ARRAY_SIZE(namespaces_sql) + 1, db);
    sqlite3_stmt* fetchtags = compile_sql(tags_sql, ARRAY_SIZE(tags_sql) + 1, db);
    sqlite3_stmt* fetchtagmap = compile_sql(tagmap_sql, ARRAY_SIZE(tagmap_sql) + 1, db);
    sqlite3_stmt* fetchtagparents = compile_sql(tagparents_sql, ARRAY_SIZE(tagparents_sql) + 1, db);
    if (!fetchnamespaces || !fetchtags || !fetchtagmap || !fetchtagparents) {
        sqlite3_finalize(fetchnamespaces);
        sqlite3_finalize(fetchtags);
        sqlite3_finalize(fetchtagmap);
        sqlite3_finalize(fetchtagparents);
        return false; // Error already printed for us
    }

    // TODO: Check for errors after each sqlite3_step() loop so we can get detailed error messages

    // Load tag namespaces
    int exec_result = SQLITE_OK;
    while ((exec_result = sqlite3_step(fetchnamespaces)) == SQLITE_ROW) {
        const unsigned char* nspace = sqlite3_column_text(fetchnamespaces, 0);
        const tag_hash_t hash = sqlite3_column_int(fetchnamespaces, 1);

        namespaces[hash] = (char*)nspace;
    }
    sql_handle_error("Error while loading namespaces:", db, exec_result);

    // Load tags
    while ((exec_result = sqlite3_step(fetchtags)) == SQLITE_ROW) {
        const unsigned char* tag = sqlite3_column_text(fetchtags, 0);
        const tag_hash_t hash = sqlite3_column_int(fetchtags, 1);
        const tag_hash_t namespace_hash = sqlite3_column_int(fetchtags, 2);
        std::string nspace = "";
        // We could probably handle this in SQL with a more complicated query
        // doing a join, but this is fine. This also handles NULL ns hashes,
        // since they return 0 and we won't have a hash of 0 (probably).
        if (namespaces.count(namespace_hash)) {
            nspace += namespaces[namespace_hash] + ":";
        }

        tags[hash] = nspace + std::string((char*)tag);
    }
    sql_handle_error("Error while loading tags:", db, exec_result);

    // Attach tags to their corresponding songs
    while ((exec_result = sqlite3_step(fetchtagmap)) == SQLITE_ROW) {
        const tag_hash_t tag_hash = sqlite3_column_int(fetchtagmap, 0);
        const song_hash_t song_hash = sqlite3_column_int(fetchtagmap, 1);

        // Add the tag to the song
        if (song_map.count(song_hash)) {
            song_map[song_hash].tags.insert(tag_hash);
        }
    }
    sql_handle_error("Error while loading songs:", db, exec_result);

    // Add parented tags to songs as needed
    while ((exec_result = sqlite3_step(fetchtagparents)) == SQLITE_ROW) {
        const tag_hash_t parent_hash = sqlite3_column_int(fetchtagparents, 0);
        const tag_hash_t child_hash = sqlite3_column_int(fetchtagparents, 1);

        // Our tag query handles parents up to 3 layers deep, no need to handle here.
        tag_parents.pairs.push_back((linked_tags){parent_hash, child_hash});
    }
    sql_handle_error("Error while loading tag parents:", db, exec_result);

    // Free our compiled SQL queries
    sqlite3_finalize(fetchnamespaces);
    sqlite3_finalize(fetchtags);
    sqlite3_finalize(fetchtagmap);
    sqlite3_finalize(fetchtagparents);

    need_reload = false; // Reset reload flag

    } // Scope for timer
    LOG_MSG(info, "Finished loading from database in %.3fms\n", timer_map["db_load"]);
    return true;
}

nativegui::nativegui(sqlite3* db, const char* files_dir) : files_dir(files_dir) {
    this->db = db;

    bool result = true;
    if (!load_from_db()) {
        db = nullptr;
        result = false;
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

static int autocomplete_update_selection(ImGuiInputTextCallbackData* data) {
    auto tac = (tag_autocomplete*) data->UserData;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
        if (tac->cur_idx != 0) {
            // User edited a different buffer, it should become the new main buffer
            tac->need_apply = true;
        }
        tac->need_refresh = true; // Need to refresh results
        return 0;
    }

    if (data->EventFlag != ImGuiInputTextFlags_CallbackHistory) {
        return 0;
    }

    // 1 if down, -1 if up, 0 if both.
    const s8 diff = (data->EventKey == ImGuiKey_DownArrow) - (data->EventKey == ImGuiKey_UpArrow);
    const bool prefix_minus = data->Buf[0] == '-';

    tac->update_selection(diff);
    tac->should_refocus_input = true;

    return 0;
}

bool nativegui::InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac) {
    bool result = false;
    const std::string real_label = label + std::to_string(tac.cur_idx);
    const auto old_idx = tac.cur_idx;

    // We need a callback to make this work
    flags |= ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit;
    if (ImGui::InputTextWithHint(real_label.c_str(), hint, &tac.get_current(), flags, autocomplete_update_selection, &tac)) {
        result = true;
        tac.should_refocus_input = true;
    }

    // This is done via flag since it can invalidate pointers, which is a problem
    // in callbacks.
    if (tac.need_apply) {
        tac.apply_selection();
        tac.need_apply = false;
    }

    // This is done via flag so we have the db ptr and access to timer output
    if (tac.need_refresh) {
        const scope_timer main_timer(timer_map, "tag_autocomplete");
        tac.update_results(db);
        tac.need_refresh = false;
    }

    // Draw results
    for (const std::string& candidate : tac.candidates) {
        ImGui::Text("%s", candidate.c_str());
    }
    ImGui::Separator();

    return result;
}

void nativegui::draw_search_menu() noexcept {
    ImGui::Begin("Search");

    // Show current tags and input box
    for (const std::string& tag : search.tags) {
        ImGui::Text("%s", tag.c_str());
    }

    // Focus text input so user can keep typing
    if (search.tac.should_refocus_input) {
        search.tac.should_refocus_input = false; // Reset flag
        ImGui::SetKeyboardFocusHere();
    }

    // Input for next tag
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (InputTagAutocompleted("##tag", "Input a tag", flags, search.tac)) {
        const scope_timer main_timer(timer_map, "last_search");
        // This also executes the search and updates our state
        search.finalize_current_tag(db);
        search.tac.reset(); // Must come 2nd since it contains the current tag
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
    if (ImGui::BeginTable("song table", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Setup table header
        ImGui::TableSetupColumn("Title");
        ImGui::TableSetupColumn("Year");
        ImGui::TableHeadersRow();

        // Draw a row for each chunk
        for (const auto& pair : song_map) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            // TODO: Maybe we should have the open windows determined by a flag
            // on each song (like on Polaris ALR chunks)? Easier to store.
            // Although, that could suck on reloads since we wipe the vector...
            const runtime_song& s = pair.second;
            // The 2nd arg is whether the row is selected (for highlighting)
            if (ImGui::Selectable(s.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                song_editors.insert(s.hash);
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", s.release_year);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

// This was made a nativegui method instead of going in runtime_records.hxx
// because it needs set the database reload flag. I guess we could do that
// manually in the one place we use this, but whatever. - torph
void nativegui::apply_parent_child_pair() noexcept {
    const auto& child = tag_parents.autocomp_child.get_current();
    const auto& parent = tag_parents.autocomp_parent.get_current();

    std::string sql;
    link_tags_sql(parent.c_str(), child.c_str(), sql);

    char* errmsg = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (result != SQLITE_OK && errmsg != nullptr) {
        LOG_MSG(error, "SQLite error: %s\n", errmsg);
    }

    // Reset and reload
    tag_parents.autocomp_child.reset();
    tag_parents.autocomp_parent.reset();
    this->need_reload = true;
}

void nativegui::draw_tag_parents() noexcept {
    if (!show_tag_parents) {
        return;
    }

    ImGui::Begin("Tag Parents");
    if (tag_parents.autocomp_child.should_refocus_input) {
        tag_parents.autocomp_child.should_refocus_input = false;
        ImGui::SetKeyboardFocusHere();
    }

    const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (InputTagAutocompleted("##c", "Child tag", flags, tag_parents.autocomp_child)) {
        // Child is done, focus next box
        tag_parents.autocomp_child.should_refocus_input = false;
        tag_parents.autocomp_parent.should_refocus_input = true;
    }

    if (tag_parents.autocomp_parent.should_refocus_input) {
        tag_parents.autocomp_parent.should_refocus_input = false;
        ImGui::SetKeyboardFocusHere();
    }
    bool apply = InputTagAutocompleted("##p", "Parent tag", flags, tag_parents.autocomp_parent);
    // Let user apply by hitting Enter or using the button
    apply |= ImGui::Button("Apply");
    if (apply) {
        apply_parent_child_pair();
    }

    if (ImGui::BeginTable("tag parent table", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Setup table header
        ImGui::TableSetupColumn("Child");
        ImGui::TableSetupColumn("Parent");
        ImGui::TableHeadersRow();

        // Draw a row for each pair
        for (const auto& pair : tag_parents.pairs) {
            const char* parent_str = "[hash %d]";
            const char* child_str = parent_str;
            if (tags.count(pair.child)) {
                child_str = tags[pair.child].c_str();
            }
            if (tags.count(pair.parent)) {
                parent_str = tags[pair.parent].c_str();
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(child_str, pair.child);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text(parent_str, pair.parent);

            ImGui::TableSetColumnIndex(2);
            ImGui::PushID(pair.child ^ pair.parent); // Button needs a unique ID, this is good enough
            if (ImGui::Button("Delete pair") && parent_str && child_str) {
                std::string sql;
                unlink_tags_sql(parent_str, child_str, sql);
                sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
                need_reload = true;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void nativegui::draw_toolbar() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool import_files = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_I, false);
    need_reload |= ImGui::IsKeyPressed(ImGuiKey_F5, false);
    need_reload |= ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_R, false);

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                import_files |= ImGui::MenuItem("Import files", "Ctrl-I");
                need_reload |= ImGui::MenuItem("Reload from database", "F5 / Ctrl-R");
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Tag Parents", nullptr, &this->show_tag_parents);
                ImGui::MenuItem("Performance Timers", nullptr, &this->show_timers);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }


    if (import_files) {
        import_stats.reset();
        import_path_ptrs.clear();
        import_paths.clear();

        const nfdu8filteritem_t filters[] = { { "Song files", "mp3"} };
        nfdresult_t res = NFD_OpenDialogMultipleAutoFree(import_paths, filters, ARRAY_SIZE(filters), nullptr);

        // Import function requires a pointer array and I can't be bothered to
        // refactor right now - torph
        for (const auto& path : import_paths) {
            import_path_ptrs.push_back(path.c_str());
        }

        if (res == NFD_OKAY && import_path_ptrs.size() > 0) {
            const char* const* paths = import_path_ptrs.data();
            const u32 num_paths = import_path_ptrs.size();
            // Run imports on another thread so UI doesn't lock up
            import_thread = std::thread(import_many_files_many_threads, paths, num_paths, files_dir, db, &import_stats);
            import_thread.detach();
            show_import_window = true;
        }
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

void nativegui::draw_import_progress() noexcept {
    if (!show_import_window) {
        return;
    }

    const import_stats_t& s = import_stats; // Shorthand
    ImGui::Begin("Import");

    ImGui::Text("Phase 1:");
    ImGui::Separator();
    ImGui::Text("Songs loaded: %d \nSongs hashed: %d \n", s.num_loaded.load(), s.num_hashed.load());
    ImGui::Text("Songs scraped: %d \nSongs copied: %d \n", s.num_metadata_grabbed.load(), s.num_copied.load());
    ImGui::Text("Songs skipped: %d \n\n", s.num_skipped.load());

    ImGui::Text("Phase 2:");
    ImGui::Separator();
    ImGui::Text("SQL entries generated: %d \nSongs total: %d \n", s.num_generated_sql.load(), s.total_songs.load());

    // All done
    if (s.total_songs == s.num_generated_sql + s.num_skipped) {
        if (ImGui::Button("Close")) {
            show_import_window = false;
        }
    }

    ImGui::End();
}

bool gui_main(void* ctx, GLFWwindow* window) {
    nativegui* gui = (nativegui*)ctx;
    const scope_timer main_timer(gui->timer_map, "main_draw");
    if (gui->need_reload) {
        gui->load_from_db();
    }

    gui->draw_toolbar();
    gui->draw_tag_parents();
    gui->draw_timers();
    gui->draw_song_list();
    gui->draw_search_menu();
    gui->draw_import_progress();

    std::vector<song_hash_t> editors_to_close(0); // Reserve 0 since this is rare
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
