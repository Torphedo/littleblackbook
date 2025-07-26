#include <cstdlib>
#include <sqlite3.h>

#include <common/logging.h>
#include <common/path.h>

#include "expression.hxx"
#include "nativegui/nativegui.hxx"
#include "nativegui/gui_bootstrap.hxx"
#include "cli/cli_main.hxx"
#include "arguments.hxx"

int main(int argc, char** argv) {
    // Enable ANSI escape codes (for printing in color) on Windows
    enable_win_ansi();

    const arguments args(argc, argv);
    int result = EXIT_SUCCESS;

    // Default DB path
    const char* db_path = "../db/blackbook.db3";
    if (args.seen_values[VALUE_ARG_DB_PATH]) {
        // Get DB path from user
        db_path = args.values[VALUE_ARG_DB_PATH];
        LOG_MSG(debug, "Got database path \"%s\"\n", db_path);
    }
    std::string db_files_folder;
    {
        // Move the C-allocated path to a dynamic string we can append to.
        // I don't know if .c_str() returns the actual backing string ptr. So
        // just to be safe, we truncate a clone before turning to a C++ string.
        char* db_folder_ptr = (char*)path_truncate_clone(db_path);
        db_files_folder = db_folder_ptr;
        db_files_folder += "files";
        free(db_folder_ptr);
    }
    LOG_MSG(debug, "DB files folder: %s\n", db_files_folder.c_str());

    const tag_expression expr("  NOT foo chop suey  AND (bar fight OR -baz)");

    sqlite3_initialize();
    sqlite3* db = nullptr;
    int res = sqlite3_open(db_path, &db);
    char* errmsg = nullptr;
    if (res != SQLITE_OK) {
        const char* msg = sqlite3_errmsg(db);
        LOG_MSG(error, "Failed to open database \"%s\" (reason: \"%s\")!\n", db_path, msg);
        result = EXIT_FAILURE;
        goto exit;
    }
    LOG_MSG(info, "Opened database \"%s\"\n", db_path);

    // Always enable extended result codes for more detailed errors
    sqlite3_extended_result_codes(db, true);

    res = sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, &errmsg);
    if (res != SQLITE_OK) {
        LOG_MSG(error, "Failed to enable foreign key constraints because: %s\n", errmsg);
        result = EXIT_FAILURE;
        goto exit;
    } else {
        LOG_MSG(info, "Enabled foreign keys\n");
    }

    // Enable extension loading (from C only, not SQL) and try to load CRC32 module
    sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
    if (sqlite3_load_extension(db, "./libsqlite_crc32", nullptr, &errmsg) != SQLITE_OK) {
        if (errmsg) {
            LOG_MSG(error, "Unabled to load CRC32 extension because: %s\n", errmsg);
        }
        result = EXIT_FAILURE;
        goto exit;
    } else {
        LOG_MSG(info, "Successfully loaded CRC32 extension.\n");
    }

    if (args.cli_mode) {
        result = cli_main(args, db, db_path, db_files_folder);
    } else {
        nativegui gui(db, db_files_folder.c_str());
        if (!gui.initialized) {
            LOG_MSG(error, "Failed to start up the GUI!\n");
            result = EXIT_FAILURE;
            goto exit;
        }

        // We invert the return value since exit code 0 == false == EXIT_SUCCESS
        result = !gui_loop(nativegui::gui_main_static, &gui);
    }

exit:
    sqlite3_close(db);
    sqlite3_shutdown();
    return result;
}
