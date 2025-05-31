#include <cstdio>
#include <cstdlib>
#include <sqlite3.h>

#include <common/logging.h>
#include "arguments.hxx"
#include "common/file.h"
#include "common/int.h"
#include "common/path.h"
#include "import.hxx"

int main(int argc, char** argv) {
    arguments args(argc, argv);

    // Default DB path
    const char* db_path = "../db/blackbook.db3";
    if (args.seen_flags[ARG_DB_PATH]) {
        // Get DB path from user
        db_path = args.flag_values[ARG_DB_PATH];
        LOG_MSG(debug, "Got database path \"%s\"\n", db_path);
    }

    sqlite3* db = nullptr;
    const int res = sqlite3_open(db_path, &db);
    if (res != SQLITE_OK) {
        const char* msg = sqlite3_errmsg(db);
        LOG_MSG(error, "Failed to open database \"%s\" (reason: \"%s\")!\n", db_path, msg);
        sqlite3_close(db);
        return 1;
    }
    LOG_MSG(info, "Opened database \"%s\"\n", db_path);

    // There's more args that aren't settings flags...
    // Treat them as filenames.
    std::string sql = "BEGIN TRANSACTION;\n";
    u32 num_songs = 0;
    {
        for (u32 i = args.first_non_flag; i < argc; i++) {
            if (!file_exists(argv[i])) {
                LOG_MSG(debug, "Skipping \"%s\" (it doesn't exist)\n", argv[i]);
                continue;
            }
            if (!path_has_extension(argv[i], ".mp3")) {
                LOG_MSG(debug, "Skipping \"%s\" (not an MP3)\n", argv[i]);
                continue;
            }

            import_single_file(argv[i], db, "..", sql);
            num_songs++;
        }
        sql.append("\nCOMMIT;\n");
    }

    if (num_songs > 0) {
        char* errmsg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
            if (errmsg) {
                LOG_MSG(error, "SQLite error: %s\n", errmsg);
            }
        }
    } else {
        LOG_MSG(info, "It doesn't seem like you provided any MP3 files.\n");
    }

    sqlite3_close(db);
}
