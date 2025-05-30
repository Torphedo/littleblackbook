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

    if (args.first_non_flag < argc) {
        // There's more args that aren't settings flags...
        // Treat them as filenames.
        for (u32 i = args.first_non_flag; i < argc; i++) {
            if (!file_exists(argv[i]) || !path_has_extension(argv[i], ".mp3")) {
                continue;
            }

            import_single_file(argv[i], db, "..");
            /*
            u8* buf = file_load(argv[i]);
            if (buf == nullptr) {
                continue;
            }
            LOG_MSG(info, "%s:\n", argv[i]);
            song_record song = mp3_load_metadata(buf, file_size(argv[i]));
            print_song(song);
            printf("\n");
            free(buf);
             */
        }
    }

    sqlite3_close(db);
}
