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
#include "search.hxx"
#include "sql.hxx"

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
        const char* const* files = &argv[args.first_non_flag];
        import_many_files(files, num_files, "..", db);
    }

    if (args.settings[SETTING_ARG_SEARCH]) {
        std::string sqlbuf;
        const u32 num_tags = argc - args.first_non_flag;
        const char* const* tags = &argv[args.first_non_flag];
        search_many_tags_and(tags, num_tags, sqlbuf);

        // Dump generated SQL to a file
        const char* sqlpath = "query.sql";
        FILE* f = fopen(sqlpath, "wb");
        if (f) {
            fprintf(f, "%s\n", sqlbuf.c_str());
            fclose(f);
            LOG_MSG(info, "Saved search query to \"%s\"\n", sqlpath);
            LOG_MSG(info, "You can get the results by piping the file into the \"sqlite3\" utility [e.g. 'cat query.sql | sqlite3 file.db']\n", sqlpath);
        } else {
            LOG_MSG(error, "Failed to save search query to \"%s\"\n", sqlpath);
        }
    }

    sqlite3_close(db);
}
