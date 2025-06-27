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
#include <tags.hxx>
#include "runtime_records.hxx"
#include "nfde_wrapper.hxx"

nativegui::nativegui(sqlite3* db, const char* files_dir) : core(blackbook_core(db, files_dir)) {
    initialized = core.initialized;
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
            const std::string& tag = core.tags[hash];
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
    tac->need_refocus = true;

    return 0;
}

bool nativegui::InputTagAutocompleted(const char* label, const char* hint, ImGuiInputTextFlags flags, tag_autocomplete& tac) {
    bool result = false;
    const std::string real_label = label + std::to_string(tac.cur_idx);
    const auto old_idx = tac.cur_idx;

    // We need a callback to make this work
    flags |= ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackEdit;
    if (ImGui::InputTextWithHint(real_label.c_str(), hint, &tac.current(), flags, autocomplete_update_selection, &tac)) {
        result = true;
        tac.need_refocus = true;
    }

    // This is done via flag since it can invalidate pointers, which is a problem
    // in callbacks.
    if (tac.need_apply) {
        tac.apply_selection();
        tac.need_apply = false;
    }

    // This is done via flag so we have the db ptr and access to timer output
    if (tac.need_refresh) {
        const scope_timer main_timer(core.timer_map, "tag_autocomplete");
        tac.update_results(core.db);
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
    for (const std::string& tag : core.search.tags) {
        ImGui::Text("%s", tag.c_str());
    }

    // Focus text input so user can keep typing
    if (core.search.tac.need_refocus) {
        core.search.tac.need_refocus = false; // Reset flag
        ImGui::SetKeyboardFocusHere();
    }

    // Input for next tag
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (InputTagAutocompleted("##tag", "Input a tag", flags, core.search.tac)) {
        const scope_timer main_timer(core.timer_map, "last_search");
        // This also executes the search and updates our state
        core.search.finalize_current_tag(core.db);
        core.search.tac.reset(); // Must come 2nd since it contains the current tag
    }

    // Display results
    for (song_hash_t hash : core.search.result_hashes) {
        const runtime_song& s = core.song_map[hash];
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
        for (const auto& pair : core.song_map) {
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

void nativegui::draw_tag_parents() noexcept {
    if (!show_tag_parents) {
        return;
    }

    ImGui::Begin("Tag Parents", &show_tag_parents);
    // TODO: This is too many layers.
    if (core.tag_parents.tac_child.need_refocus) {
        core.tag_parents.tac_child.need_refocus = false;
        ImGui::SetKeyboardFocusHere();
    }

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (InputTagAutocompleted("##c", "Child tag", flags, core.tag_parents.tac_child)) {
        // Child is done, focus next box
        core.tag_parents.tac_child.need_refocus = false;
        core.tag_parents.tac_parent.need_refocus = true;
    }

    if (core.tag_parents.tac_parent.need_refocus) {
        core.tag_parents.tac_parent.need_refocus = false;
        ImGui::SetKeyboardFocusHere();
    }
    bool apply = InputTagAutocompleted("##p", "Parent tag", flags, core.tag_parents.tac_parent);
    // Let user apply by hitting Enter or using the button
    apply |= ImGui::Button("Apply");
    if (apply) {
        // TODO: This is all on core now, it should be handling the reload flag behaviour.
        core.need_reload |= core.tag_parents.apply_current_pair(core.db);
    }

    if (ImGui::BeginTable("tag parent table", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Setup table header
        ImGui::TableSetupColumn("Child");
        ImGui::TableSetupColumn("Parent");
        ImGui::TableHeadersRow();

        // Draw a row for each pair
        for (const auto& pair : core.tag_parents.pairs) {
            const char* parent_str = "[hash %d]";
            const char* child_str = parent_str;
            if (core.tags.count(pair.child)) {
                child_str = core.tags[pair.child].c_str();
            }
            if (core.tags.count(pair.parent)) {
                parent_str = core.tags[pair.parent].c_str();
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
                sqlite3_exec(core.db, sql.c_str(), nullptr, nullptr, nullptr);
                core.need_reload = true;
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
    core.need_reload |= ImGui::IsKeyPressed(ImGuiKey_F5, false);
    core.need_reload |= ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_R, false);

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                import_files |= ImGui::MenuItem("Import files", "Ctrl-I");
                core.need_reload |= ImGui::MenuItem("Reload from database", "F5 / Ctrl-R");
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


    // This should probably be its own method, right? - torph
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
            import_thread = std::thread(import_many_files_many_threads, paths, num_paths, core.files_dir, core.db, &import_stats);
            import_thread.detach(); // Otherwise dtor will try to kill it later and crash
            show_import_window = true;
        }
    }
}

void nativegui::draw_timers() noexcept {
    if (!show_timers) {
        return;
    }

    ImGui::Begin("Performance Timers", &show_timers);
    for (const auto& entry : core.timer_map) {
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
    const scope_timer main_timer(gui->core.timer_map, "main_draw");
    if (gui->core.need_reload) {
        gui->core.load_from_db();
    }

    gui->draw_toolbar();
    gui->draw_tag_parents();
    gui->draw_timers();
    gui->draw_song_list();
    gui->draw_search_menu();
    gui->draw_import_progress();

    std::vector<song_hash_t> editors_to_close(0); // Reserve 0 since this is rare
    for (song_hash_t song_hash : gui->song_editors) {
        if (!gui->draw_song_editor(gui->core.song_map[song_hash])) {
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
