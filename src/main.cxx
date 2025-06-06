#include <cstdio>
#include <cstdlib>
#include <vector>

#include <sqlite3.h>

#include <common/logging.h>
#include <common/int.h>
#include <common/file.h>
#include <common/path.h>

#include "arguments.hxx"
#include "database.hxx"
#include "scope_timer.hxx"

int main(int argc, char** argv) {
    arguments args(argc, argv);

    // Default DB path
    const char* db_path = "../db/blackbook.db3";
    if (args.seen_values[VALUE_ARG_DB_PATH]) {
        // Get DB path from user
        db_path = args.values[VALUE_ARG_DB_PATH];
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

    if (args.settings[SETTING_ARG_IMPORT]) {
        const u32 num_files = argc - args.first_non_flag;
        // Why do we need a cast to get a const * from a non-const *??
        const char** files = (const char**)&argv[args.first_non_flag];
        import_many_files(files, num_files, "..", db);
    }

    sqlite3_close(db);
}
