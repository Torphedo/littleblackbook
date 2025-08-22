#include <glad/glad.h>
#include "nativegui.hxx"
#include <cstdio>
#include <ranges>

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <raudio.h>

#include <common/logging.h>
#include <common/vfile.h>
#include <common/crc32.h>
#include <common/int.h>

#include <defaults.hxx>
#include <blackbook_core.hxx>
#include <schema.hxx>
#include <tags.hxx>
#include <sqlgen.hxx>
#include <scope_timer.hxx>
#include "thumbnails.hxx"
#include "nfde_wrapper.hxx"
#include "imgui_utils.hxx"

// Autocomplete callback for ImGui::InputText() and related functions.
void nativegui::draw_song_info(const runtime_song& song) noexcept {
    ImGui::Text("Title: %s", song.name.c_str());
    ImGui::Text("Released: %u", song.release_year);

    if (song.tags.size() > 0) {
        ImGui::Text("Tags:");
        for (song_hash_t hash : song.tags) {
            const std::string& tag = core.tags.at(hash); // operator[] isn't const
            ImGui::Text("%s", tag.c_str());
        }
        ImGui::Text("\n");
    }

    ImGui::Text("Hash: %d", song.hash);
    ImGui::Text("Imported @ %lu", song.import_timestamp);

    gl_obj thumbnail = thumbnails.at(song.hash);
    if (thumbnail != 0) {
        ImGui::Image(thumbnail, ImVec2(512, 512));
    }
}

bool nativegui::window_song_editor(runtime_song& song) {
    char win_title_buf[64] = {0};
    snprintf(win_title_buf, sizeof(win_title_buf), "Song editor [%d]", song.hash);
    bool open = true;
    open &= ImGui::Begin(win_title_buf, &open);
    if (!open) {
        // User closed the window or it's not visible
        ImGui::End();
        return false;
    }
    draw_song_info(song);

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    snprintf(win_title_buf, sizeof(win_title_buf), "##editor_input%d", song.hash);
    if (ImGui::InputTagAutocompleted(win_title_buf, "Input a tag", flags, song.tac, core)) {
        const std::string& tag = song.tac.current();
        const tag_hash_t tag_hash = crc32fast((const u8*)tag.c_str() + (tag[0] == '-'), tag.size());

        bool found = false;
        for (auto iter = song.tags.begin(); iter != song.tags.end(); iter++) {
            found |= (*iter == tag_hash);
            if (found) {
                song.tags.erase(iter);
                break; // We're done here (and iterator is now invalidated anyway)
            }
        }

        std::string sql;
        if (found) {
            del_tag_from_song_sql(tag.c_str(), song.hash, sql);
        } else {
            add_tag_to_song_sql(core.db, tag.c_str(), song.hash, sql);
        }
        int result = sqlite3_exec(core.db, sql.c_str(), nullptr, nullptr, nullptr);
        sql_handle_error("Failed to add/remove tag", core.db, result);
        core.load_from_db(); // Reload
    }

    ImGui::End();
    return true;
}

bool nativegui::draw_song_row(song_hash_t hash, float thumb_size) noexcept {
    bool result = false;

    ImGui::TableNextRow(0, thumb_size);
    ImGui::TableSetColumnIndex(0);

    const runtime_song& s = core.song_map.at(hash);
    ImGui::Image(thumbnails.at(s.hash), ImVec2(thumb_size, thumb_size));
    ImGui::SameLine();
    // The 2nd arg is whether the row is selected (for highlighting)
    if (ImGui::Selectable(s.name.c_str(), false, 0, ImVec2(0, thumb_size))) {
        result = true;
    }
    const s32 cur_row = ImGui::TableGetRowIndex();

    blackbook_core::playlist_add_type type = blackbook_core::PLAYLIST_APPEND;
    bool playlist_add = false;
    char popup_name[128] = {0};
    snprintf(popup_name, sizeof(popup_name), "song popup [%d] [%d]", hash, cur_row);
    if (ImGui::BeginPopupContextItem(popup_name)) {
        if (ImGui::MenuItem("Append to playlist")) {
            playlist_add = true;
        }
        if (ImGui::MenuItem("Prepend to playlist")) {
            playlist_add = true;
            type = blackbook_core::PLAYLIST_PREPEND;
        }
        if (ImGui::MenuItem("Play next")) {
            playlist_add = true;
            type = blackbook_core::PLAYLIST_NEXT;
        }
        if (ImGui::MenuItem("Open editor")) {
            result = true;
        }
        ImGui::EndPopup();
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%d", s.release_year);

    if (playlist_add) {
        core.add_to_playlist(&s.hash, 1, type);
    }

    return result;
}

bool nativegui::draw_search_menu(const char* win_title, tag_search& search) noexcept {
    bool open = true;
    if (ImGui::Begin(win_title, &open)) {
        if (ImGui::EditExpression(search.expr, search.tac, core)) {
            search.update_results(core.db);
        }

        if (ImGui::Button("Add to playlist")) {
            const auto& results = search.result_hashes;
            core.add_to_playlist(results.data(), results.size());
        }

        // Display results
        draw_songs(search.result_hashes);
        ImGui::End();
    }
    return open;
}

bool nativegui::window_songs() noexcept {
    if (ImGui::Button("Add to playlist")) {
        // TODO: Have the playlist append function take a generic C++ iterator/collection to reduce duplication
        const bool need_init = core.playlist.empty();
        core.playlist.reserve(core.playlist.size() + core.song_map.size());
        for (const auto& pair : core.song_map) {
            core.playlist.push_back(pair.first);
        }
        core.playlist_change_song(0);
    }

    draw_songs(std::views::keys(core.song_map));
    return true;
}

bool nativegui::window_playlist() noexcept {
    ImGui::Text("%ld songs", core.playlist.size());

    // Outer border gives a tiny bit of padding to make the year column more readable
    const int flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable | ImGuiTableFlags_BordersOuterV;
    if (ImGui::BeginTable("song table", 2, flags)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Title column takes up as much space as possible, but won't truncate the year column
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
        // Fixed width makes sure the column never gets cut off
        ImGui::TableSetupColumn("Year", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow(); // Show headers

        for (u32 i = 0; i < core.playlist.size(); i++) {
            const song_hash_t hash = core.playlist[i];
            if (draw_song_row(hash)) {
                song_editors.insert(hash);
            }

            const s32 hovered = ImGui::TableGetHoveredRow() - 1;
            ImGui::TableSetColumnIndex(0);
            if (i == hovered) {
                // This shows where the song will end up during drag & drop
                ImGui::Separator();
            }

            const bool m2 = ImGui::IsMouseClicked(ImGuiMouseButton_Right, true);
            const bool esc = ImGui::IsKeyPressed(ImGuiKey_Escape, true);
            if (m2 || esc) {
                playlist_drag_start = -1; // User wants to cancel
            }

            const bool m1_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            const bool m1_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            if (m1_click) {
                if (playlist_drag_start < 0) {
                    playlist_drag_start = hovered;
                }
            } else if (!m1_down && playlist_drag_start >= 0 && hovered >= 0) {
                // User had been dragging, and just released.
                core.playlist_move_song(playlist_drag_start, hovered);
                playlist_drag_start = -1;
            }
        }

        ImGui::EndTable();
    }
    return true;
}

bool nativegui::toolbar_player() noexcept {
    song_hash_t cur_hash = 0;
    if (!core.playlist.empty()) {
        cur_hash = core.playlist.at(core.playlist_pos);
    }

    const bool ctrl = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    const bool shift = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    bool toggle_play = ImGui::IsKeyPressed(ImGuiKey_Space, false);
    s32 skip_song = (shift && ImGui::IsKeyPressed(ImGuiKey_N, false)) || ImGui::IsKeyPressed(ImGuiKey_J, false);
    s32 prev_song = (shift && ImGui::IsKeyPressed(ImGuiKey_P, false)) || ImGui::IsKeyPressed(ImGuiKey_K, false);
    s32 seek_ahead = ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)   || ImGui::IsKeyPressed(ImGuiKey_L, true);
    s32 seek_back  = ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)    || ImGui::IsKeyPressed(ImGuiKey_H, true);

    // Disable everything when ImGui is using the keyboard
    const u8 disable_shortcuts = ImGui::GetIO().WantTextInput;
    toggle_play *= !disable_shortcuts;
    prev_song *= !disable_shortcuts;
    skip_song *= !disable_shortcuts;
    seek_ahead *= !disable_shortcuts;
    seek_back *= !disable_shortcuts;


    const bool playing = IsMusicStreamPlaying(core.audio_stream);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;
    float progress = GetMusicTimePlayed(core.audio_stream);
    float total = 0;
    if (IsMusicReady(core.audio_stream)) {
        total = GetMusicTimeLength(core.audio_stream);
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(0, 100), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginViewportSideBar("PlayerBar", viewport, ImGuiDir_Down, 100, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (core.song_map.count(cur_hash)) {
                const runtime_song& song = core.song_map[cur_hash];
                ImGui::Image(thumbnails[song.hash], ImVec2(100, 100));
            }

            ImGui::Text("[%d / %ld]", core.playlist_pos, core.playlist.size());
            prev_song += ImGui::Button("<");
            const char* button_label = playing ? "Pause" : "Play";
            toggle_play |= ImGui::Button(button_label);
            skip_song += ImGui::Button(">");

            if (core.song_map.count(cur_hash)) {
                const runtime_song& song = core.song_map[cur_hash];
                ImGui::Text("%s", song.name.c_str());
            }

            char tmpbuf[128] = {0};
            snprintf(tmpbuf, sizeof(tmpbuf) - 1, "%d:%02d / %d:%02d", u32(progress) / 60, u32(progress) % 60, u32(total) / 60, u32(total) % 60);

            if (ImGui::SliderFloat("##progress", &progress, 0.0f, total, tmpbuf)) {
                SeekMusicStream(core.audio_stream, progress);
            }

            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    if (toggle_play) {
        if (playing) {
            PauseMusicStream(core.audio_stream);
        } else {
            ResumeMusicStream(core.audio_stream);
        }
    }
    if (prev_song || skip_song) {
        core.playlist_change_song(skip_song - prev_song);
    }

    if (seek_ahead || seek_back) {
        const float diff = 5.0f * (seek_ahead - seek_back);
        const float new_pos = CLAMP(0.1f, progress + diff, total);
        if (IsMusicReady(core.audio_stream)) {
            SeekMusicStream(core.audio_stream, new_pos);
        }
    }

    return true;
}

bool nativegui::window_tag_parents() noexcept {
    // TODO: This is too many layers.
    if (core.tac_child.need_refocus) {
        core.tac_child.need_refocus = false;
        ImGui::SetKeyboardFocusHere();
    }

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (ImGui::InputTagAutocompleted("##c", "Child tag", flags, core.tac_child, core)) {
        // Child is done, focus next box
        core.tac_child.need_refocus = false;
        core.tac_parent.need_refocus = true;
    }

    if (core.tac_parent.need_refocus) {
        core.tac_parent.need_refocus = false;
        ImGui::SetKeyboardFocusHere();
    }
    bool apply = ImGui::InputTagAutocompleted("##p", "Parent tag", flags, core.tac_parent, core);
    // Let user apply by hitting Enter or using the button
    apply |= ImGui::Button("Apply");
    if (apply) {
        core.apply_tag_pair();
    }

    if (ImGui::BeginTable("tag parent table", 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Reorderable)) {
        // Make header row that never scrolls away
        ImGui::TableSetupScrollFreeze(0, 1);

        // Setup table header
        ImGui::TableSetupColumn("Child");
        ImGui::TableSetupColumn("Parent");
        ImGui::TableHeadersRow();

        // Draw a row for each pair
        for (const auto& pair : core.parent_pairs) {
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
    return true;
}

bool nativegui::toolbar_main() noexcept {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float height = ImGui::GetFrameHeight();
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

    const bool ctrl_pressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool import_files = ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_I, false);
    bool new_search = ImGui::IsKeyChordPressed(ImGuiKey_T | ImGuiMod_Ctrl);
    core.need_reload |= ImGui::IsKeyPressed(ImGuiKey_F5, false);
    core.need_reload |= ctrl_pressed && ImGui::IsKeyPressed(ImGuiKey_R, false);

    if (ImGui::BeginViewportSideBar("MainMenu", viewport, ImGuiDir_Up, height, flags)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                import_files |= ImGui::MenuItem("Import files", "Ctrl-I");
                core.need_reload |= ImGui::MenuItem("Reload from database", "F5 / Ctrl-R");
                need_thumbnail_reload |= ImGui::MenuItem("Reload thumbnails");
                if (ImGui::MenuItem("Apply default tag parents")) {
                    core.apply_defaults();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::InputFloat("Font Size", &io.FontGlobalScale, 0.1f);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows")) {
                for (u32 i = 0; i < ARRAY_SIZE(windows); i++) {
                   ImGui::MenuItem(windows[i].window_name, nullptr, &windows_active[i]);
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        ImGui::End();
    }

    if (new_search) {
        core.searches.emplace_back();
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

        if (res == NFD_OKAY && !import_path_ptrs.empty()) {
            const char* const* paths = import_path_ptrs.data();
            const u32 num_paths = (u32)import_path_ptrs.size();
            // Run imports on another thread so UI doesn't lock up
            import_thread = std::thread(import_many_files_many_threads, paths, num_paths, core.files_dir, core.db, &import_stats);
            import_thread.detach(); // Otherwise dtor will try to kill it later and crash
            windows_active[IMPORT_WINDOW_IDX] = true;
        }
    }

    return true;
}

bool nativegui::window_timers() noexcept {
    for (const auto& entry : core.timer_map) {
        ImGui::Text("%s: %.2lfms", entry.first, entry.second);
    }
    return true;
}

bool nativegui::window_draw_import_progress() noexcept {
    const import_stats_t& s = import_stats; // Shorthand
    const u32 total = s.total_songs.load();
    const u32 skipped = s.num_skipped.load();
    const u32 total_unskipped = total - skipped;

    const float progress_hashed = s.num_hashed.load() / (float)total_unskipped;
    const float progress_copy   = s.num_copied.load() / (float)total_unskipped;
    const float progress_sql    = s.num_generated_sql.load() / (float)total_unskipped;
    const float progress_total  = (s.num_generated_sql.load() + skipped) / (float)total;

    ImGui::Text("Phase 1:");
    ImGui::Separator();
    ImGui::ProgressBar(progress_hashed, ImVec2(0, 0), "Hashing");
    ImGui::ProgressBar(progress_copy, ImVec2(0, 0), "Copying");
    ImGui::Text("Songs skipped: %d \n\n", skipped);

    ImGui::Text("Phase 2:");
    ImGui::Separator();
    ImGui::ProgressBar(progress_sql, ImVec2(0, 0), "Generating SQL");
    ImGui::ProgressBar(progress_total, ImVec2(0, 0), "Overall Progress");
    ImGui::Text("Songs imported: %d\n", s.num_generated_sql.load());

    // All done, hopefully
    if (ImGui::Button("Close")) {
        return false;
    } else {
        return true;
    }
}

bool nativegui::window_lyric_search() noexcept {
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
    if (ImGui::InputText("##lsearch", &lyric_search_input, flags)) {
        const scope_timer lyric_timer(core.timer_map, "lyric_search");
        lyric_search_results.clear();
        std::string sql;
        sqlgen(sql, "SELECT song_hash FROM lyric_search('\"%s\" *') ORDER BY rank LIMIT %d;",
                    lyric_search_input.c_str(), 10);
        sqlite3_stmt* stmt = compile_sql(sql.c_str(), -1, core.db);
        if (stmt == nullptr) {
            return true; // Error printed for us
        }

        int res = SQLITE_OK;
        while ((res = sqlite3_step(stmt)) == SQLITE_ROW) {
            const song_hash_t hash = sqlite3_column_int(stmt, 0);

            lyric_search_results.insert(hash);
        }
    }

    // Draw results
    draw_songs(lyric_search_results);
    return true;
}

bool nativegui::gui_main(GLFWwindow *window) noexcept {
    const scope_timer main_timer(core.timer_map, "main_draw");
    if (core.need_reload) {
        core.load_from_db();
        need_thumbnail_reload = true;
    }

    // Load thumbnails only on first load. We can't do this in ctor since OpenGL
    // may not be loaded yet
    if (need_thumbnail_reload) {
        const scope_timer thumb_load(core.timer_map, "load_thumbnails");
        thumbnails.clear();
        need_thumbnail_reload = false;
        const auto& key_iter = std::views::keys(core.song_map);
        thumbnails.load_many_mp3s_many_threads(key_iter, &thumbnails);
    }
    thumbnails.upload_deferred_textures();

    toolbar_main();
    toolbar_player();

    for (u32 i = 0; i < ARRAY_SIZE(windows); i++) {
        if (!windows_active[i]) {
            continue;
        }
        // This returns false when it's safe to skip rendering for any reason
        if (ImGui::Begin(windows[i].window_name, &windows_active[i])) {
            // Allow the window to close itself by returning false (but don't
            // let it override a false value set by clicking the X button).
            windows_active[i] &= (this->*windows[i].draw)();
        }
        ImGui::End();
    }

    // Search windows each need a unique ID and special handling to access their
    // context, so we handle them differently than "one-off" windows.
    u32 idx = 0;
    std::vector<tag_search> searches_to_close(0); // Reserve 0 since this is rare
    for (u32 i = 0; i < core.searches.size(); i++) {
        tag_search& entry = core.searches[i];
        char win_title_buf[64] = {0};
        snprintf(win_title_buf, sizeof(win_title_buf), "Search ##%d", idx++);
        if (!draw_search_menu(win_title_buf, entry)) {
            core.searches.erase(core.searches.begin() + i);
            break;
        }
    }

    // Same as above is true for song editor windows
    std::vector<song_hash_t> editors_to_close(0); // Reserve 0 since this is rare
    for (song_hash_t song_hash : song_editors) {
        if (!window_song_editor(core.song_map[song_hash])) {
            editors_to_close.push_back(song_hash);
        }
    }

    // We can't edit the set while iterating over it
    for (song_hash_t hash : editors_to_close) {
        song_editors.erase(hash);
    }

    ImGui::ShowDemoWindow();

    return true;
}

static void music_loop(blackbook_core* core) {
    while (true) {
        core->playlist_update_stream();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

nativegui::nativegui(sqlite3* db, const char* files_dir) noexcept
    : core(blackbook_core(db, files_dir)), thumbnails(thumbnail_storage(files_dir))
{
    InitAudioDevice();
    // On Windows, minimizing stops all rendering and gui_main() won't run, so
    // we need to handle playback on another thread
    music_thread = std::thread(music_loop, &core);
    initialized = core.initialized;
}

nativegui::~nativegui() noexcept {
    thumbnails.thread_stop_flag = true;
    // Gather up texture IDs to be deleted in 1 call
    // TODO: Should this be done in a thumbnail object dtor?
    std::vector<gl_obj> textures(thumbnails.thumbnails.size());
    for (const auto& pair : thumbnails.thumbnails) {
        textures.push_back(pair.second);
    }
    glDeleteTextures((u32)textures.size(), textures.data());
}
