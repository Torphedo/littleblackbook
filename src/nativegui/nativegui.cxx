#include "nativegui.hxx"
#include <imgui.h>

#include <common/logging.h>
#include <common/vfile.h>

#include "../schema.hxx"
#include "../scope_timer.hxx"

// We don't bother getting album and artists, since those will only be searched
// via namespaced tags.
static const char fetchsongs_sql[] =
"SELECT title, year, lyrics, hash, import_timestamp, duration_secs FROM songs;";

sqlite3_stmt* compile_sql(const char* sql, s32 sql_len, sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    int songres = sqlite3_prepare_v2(db, sql, sql_len, &stmt, nullptr);

    if (songres != SQLITE_OK) {
        const char* msg = sqlite3_errmsg(db);
        if (msg) {
            LOG_MSG(error, "Couldn't compile SQL statement because: \"%s\"\n", msg);
        } else {
            LOG_MSG(error, "Couldn't compile SQL statement (no error message given)\n", msg);
        }

        return nullptr;
    }

    return stmt;
}

bool nativegui::load_from_db(sqlite3* db) {
    const char tags_sql[] = "SELECT tag, hash FROM tags";
    const char tagmap_sql[] = "SELECT tag_hash, song_hash FROM " TAG_SONG_TABLE;
    const char tagparents_sql[] = "SELECT parent_hash, child_hash FROM " TAG_PARENT_TABLE;

    sqlite3_stmt* fetchsongs = compile_sql(fetchsongs_sql, ARRAY_SIZE(fetchsongs_sql) + 1, db);
    sqlite3_stmt* fetchtags = compile_sql(tags_sql, ARRAY_SIZE(tags_sql) + 1, db);
    sqlite3_stmt* fetchtagmap = compile_sql(tagmap_sql, ARRAY_SIZE(tagmap_sql) + 1, db);
    sqlite3_stmt* fetchtagparents = compile_sql(tagparents_sql, ARRAY_SIZE(tagparents_sql) + 1, db);
    if (!fetchsongs || !fetchtags || !fetchtagmap || !fetchtagparents) {
        sqlite3_finalize(fetchsongs);
        sqlite3_finalize(fetchtags);
        sqlite3_finalize(fetchtagmap);
        sqlite3_finalize(fetchtagparents);
        return false; // Error already printed for us
    }

    int exec_result = 0;
    while ((exec_result = sqlite3_step(fetchsongs)) == SQLITE_ROW) {
        const unsigned char* title = sqlite3_column_text(fetchsongs, 0);
        const u32 year = sqlite3_column_int(fetchsongs, 1);
        const u32 hash = sqlite3_column_int(fetchsongs, 3);
        const time_t time = sqlite3_column_int(fetchsongs, 4);

        // Construct in-place to encourage use of the move ctor, to avoid cloning strings
        songs[hash] = (runtime_song) {
            .name = (char*)title,
            .import_timestamp = time,
            .hash = hash,
            .release_year = year,
        };
    }

    while ((exec_result = sqlite3_step(fetchtags)) == SQLITE_ROW) {
        const unsigned char* tag = sqlite3_column_text(fetchtags, 0);
        const u32 hash = sqlite3_column_int(fetchtags, 1);

        // Add to the map
        tags[hash] = (char*)tag;
    }

    while ((exec_result = sqlite3_step(fetchtagmap)) == SQLITE_ROW) {
        const u32 tag_hash = sqlite3_column_int(fetchtagmap, 0);
        const u32 song_hash = sqlite3_column_int(fetchtagmap, 1);

        // Add the tag to the song
        if (songs.count(song_hash)) {
            songs[song_hash].tags.insert(tag_hash);
        }
    }

    while ((exec_result = sqlite3_step(fetchtagparents)) == SQLITE_ROW) {
        const u32 parent_hash = sqlite3_column_int(fetchtagparents, 0);
        const u32 child_hash = sqlite3_column_int(fetchtagparents, 1);

        // Very inefficiently, add all tag parents.
        // We probably can just do a more complex query to get the dataset to do
        // this efficiently:
        // SELECT song_hash FROM tagmap WHERE tag_hash IN (SELECT child_hash FROM tag_parents)
        // This should filter out songs that don't need parent tags added.
        for (auto& pair : songs) {
            auto& song = pair.second;
            for (u32 tag_hash : song.tags) {
                if (tag_hash == child_hash) {
                    song.tags.insert(parent_hash);
                }
            }
        }
    }

    sqlite3_finalize(fetchsongs);
    sqlite3_finalize(fetchtags);
    sqlite3_finalize(fetchtagmap);
    sqlite3_finalize(fetchtagparents);
    return true;
}

nativegui::nativegui(sqlite3* db) {
    bool result = true;

    float elapsed_loading = 0.0f;
    {
        scope_timer load_timer(elapsed_loading);
        result &= load_from_db(db);
    }
    LOG_MSG(info, "Finished loading from database in %.3fms\n", elapsed_loading);

    for (const auto& pair : songs) {
        const runtime_song& s = pair.second;
        // LOG_MSG(debug, "Got a song named \"%s\" (released %u)\n", s.name.c_str(), s.release_year);
    }

    initialized = result;
}

bool gui_main(void* ctx, GLFWwindow* window) {
    nativegui* gui = (nativegui*) ctx;

    ImGui::ShowDemoWindow();

    return true;
}
