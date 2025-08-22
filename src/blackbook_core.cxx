#include "blackbook_core.hxx"
#include <cstring>
#include <cassert>

#include <common/crc32.h>
#include <common/logging.h>
#include <common/int.h>

#include "sqlgen.hxx"
#include "text_i8n.hxx"
#include "tags.hxx"
#include "schema.hxx"
#include "scope_timer.hxx"
#include "defaults.hxx"

void blackbook_core::add_to_playlist(const song_hash_t* songs, u64 num_songs, playlist_add_type type) {
    std::lock_guard lock(playlist_lock);
    if (num_songs == 0) {
        return;
    }

    s32 pos = 0;
    switch (type) {
    case PLAYLIST_APPEND:
        pos = (s32)playlist.size();
        break;
    case PLAYLIST_PREPEND:
        pos = 0;
        break;
    case PLAYLIST_NEXT:
        pos = playlist_pos + 1;
        break;
    }
    // Keep in range
    pos = CLAMP(0, pos, (s32)playlist.size());

    const bool need_init = playlist.empty();

    // Append results to playlist
    playlist.reserve(playlist.size() + num_songs);
    for (s32 i = 0; i < num_songs; i++) {
        playlist.insert(playlist.begin() + pos + i, songs[i]);
        if (pos + i <= playlist_pos) {
            playlist_pos++;
        }
    }

    // Load a stream for the first song if needed
    if (need_init) {
        playlist_change_song(0);
    }
}

void blackbook_core::playlist_change_song(s8 diff) noexcept {
    std::lock_guard lock(playlist_lock);
    const s32 size = (s32)playlist.size();
    if (size <= 0) {
        playlist_pos = 0;
        return;
    }

    if (diff == 0) {
        // This special value resets playlist position
        playlist_pos = 0;
    } else {
        playlist_pos += diff;
        // Get back in range
        while (playlist_pos < 0) {
            playlist_pos += size - 1;
        }

        playlist_pos %= size;
    }

    const song_hash_t cur_hash = playlist.at(playlist_pos);
    char pathbuf[512] = {0};
    snprintf(pathbuf, ARRAY_SIZE(pathbuf), "%s/%d.mp3", files_dir, cur_hash);
    if (IsMusicReady(audio_stream)) {
        UnloadMusicStream(audio_stream);
    }
    audio_stream = LoadMusicStream(pathbuf);
    PlayMusicStream(audio_stream);
}

void blackbook_core::playlist_move_song(u32 source, u32 target) noexcept {
    std::lock_guard lock(playlist_lock);
    const song_hash_t source_hash = playlist[source];
    // Delete the song we're moving, and insert its hash at the target location
    playlist.erase(playlist.begin() + source);

    if (target > source) {
        // Erasing an element changed the target position
        target--;
    }

    playlist.insert(playlist.begin() + target, source_hash);

    // We moved the current song, and need to keep the state consistent
    if (playlist_pos == source) {
        playlist_pos = (s32)target;
    }
}

void blackbook_core::playlist_update_stream() noexcept {
    std::lock_guard lock(playlist_lock);
    const float total = GetMusicTimeLength(audio_stream);
    const float progress = GetMusicTimePlayed(audio_stream);

    // Automatically change songs
    if (total - progress < 0.1f) {
        playlist_change_song(1);
    }

    if (IsMusicStreamPlaying(audio_stream)) {
        UpdateMusicStream(audio_stream);
    }
}

bool blackbook_core::apply_tag_pair() noexcept {
    const auto& child = tac_child.current();
    const auto& parent = tac_parent.current();
    if (child.empty() || parent.empty()) {
        return false;
    }

    std::string sql;
    link_tags_sql(db, parent.c_str(), child.c_str(), sql);

    char* errmsg = nullptr;
    int sql_res = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (sql_res != SQLITE_OK && errmsg != nullptr) {
        LOG_MSG(error, "SQLite error: %s\n", errmsg);
    }

    // Reset and reload
    tac_child.reset();
    tac_parent.reset();
    need_reload = (sql_res == SQLITE_OK);
    return (sql_res == SQLITE_OK);
}

void tag_search::update_results(sqlite3* db) noexcept {
    // Clear existing results
    result_hashes.clear();

    std::string sql;
    sqlgen_expression(expr, sql);

    sqlite3_stmt* query = compile_sql(sql.c_str(), (s32)sql.size(), db);
    if (!query) {
        return; // Error printed for us
    }

    int result = SQLITE_OK;
    while ((result = sqlite3_step(query)) == SQLITE_ROW) {
        const tag_hash_t hash = sqlite3_column_int(query, 0);
        result_hashes.push_back(hash);
    }
    sql_handle_error("Search error: ", db, result);

    sqlite3_finalize(query);
}

bool blackbook_core::apply_defaults() noexcept {
    const scope_timer defaults_timer(timer_map, "apply_tag_defaults");
    std::string sql = "BEGIN TRANSACTION;\n";
    for (const default_tag_pair& pair : default_tag_parents) {
        link_tags_sql(db, pair.parent, pair.child, sql);
    }
    sql.append("\nCOMMIT;");

    char* errmsg = nullptr;
    const int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (result != SQLITE_OK) {
        LOG_MSG(error, "Failed to apply defaults because: %s\n", errmsg);
    } else {
        load_from_db();
        LOG_MSG(info, "Applied tag defaults!\n");
    }

    return (result == SQLITE_OK);
}

bool blackbook_core::load_from_db() {
    { // Scope for timer
    const scope_timer load_timer(timer_map, "db_load");

    // Wipe current state
    song_map.clear();
    tags.clear();
    parent_pairs.clear();

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
        parent_pairs.push_back({parent_hash, child_hash});
    }
    sql_handle_error("Error while loading tag parents:", db, exec_result);

    // Free our compiled SQL queries
    sqlite3_finalize(fetchnamespaces);
    sqlite3_finalize(fetchtags);
    sqlite3_finalize(fetchtagmap);
    sqlite3_finalize(fetchtagparents);

    this->need_reload = false;

    } // Scope for timer
    LOG_MSG(info, "Finished loading from database in %.3fms\n", timer_map["db_load"]);
    return true;
}

bool blackbook_core::load_songs_by_query(sqlite3* db) {
    // We don't bother getting album/artist, since those are stored as tags.
    static const char fetchsongs_sql[] = "SELECT title, year, lyrics, hash, import_timestamp, duration_secs FROM songs";
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

        song_map[hash] = {
            .name = (char*)title,
            .import_timestamp = time,
            .hash = hash,
            .release_year = year,
        };
    }
    sql_handle_error("Error while loading songs", db, exec_result);

    sqlite3_finalize(fetchsongs);
    return true;
}

blackbook_core::blackbook_core(sqlite3* db, const char* files_dir) : files_dir(files_dir), db(db)
{
    bool result = true;
    if (!load_from_db()) {
        this->db = nullptr;
        result = false;
    }

    initialized = result;
}

blackbook_core::~blackbook_core() {
   UnloadMusicStream(audio_stream);
   CloseAudioDevice();
}
