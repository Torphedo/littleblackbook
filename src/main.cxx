#include <cstdlib>
#include <sqlite3.h>

#include <common/logging.h>

#include "nativegui/nativegui.hxx"
#include "nativegui/gui_bootstrap.hxx"
#include "cli/cli_main.hxx"
#include "arguments.hxx"

int main(int argc, char** argv) {
    // Enable ANSI escape codes (for printing in color) on Windows
    enable_win_ansi();

    const arguments args(argc, argv);

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

    // Always enable extended result codes for more detailed errors
    sqlite3_extended_result_codes(db, true);


    int result = EXIT_SUCCESS;
    if (args.cli_mode) {
        result = cli_main(args, db, db_path);
    } else {
        nativegui gui(db);
        if (!gui.initialized) {
            LOG_MSG(error, "Failed to start up the GUI!\n");
            return EXIT_FAILURE;
        }

        // We invert the return value since exit code 0 == false == EXIT_SUCCESS
        result = !gui_loop(gui_main, &gui);
    }

    sqlite3_close(db);
    return result;
}
