#include "db_versioning.hxx"
#include "sqlgen.hxx"
#include "schema.hxx"

upgrade_status get_upgrade_status(sqlite3* db, int* version_out) {
    const char sqltext[] = "SELECT version FROM db_meta;";
    sqlite3_stmt* query = compile_sql(sqltext, sizeof(sqltext), db);
    if (!query) {
        return DB_VERSION_UNKNOWN; // Error printed for us
    }

    int version = 0;
    int result = SQLITE_OK;
    int rows_seen = 0;
    while ((result = sqlite3_step(query)) == SQLITE_ROW) {
        version = sqlite3_column_int(query, 0);
        rows_seen++;
    }

    if (result == SQLITE_DONE && rows_seen == 0 && version == 0) {
        // Table had no rows, it must be a v1 database.
        sqlgen_exec(db, "INSERT INTO db_meta (version) VALUES(1);");
        version = 1;
    }

    sql_handle_error("Failed to get DB version because: ", db, result);
    sqlite3_finalize(query);

    if (version_out) {
        *version_out = version;
    }

    if (version == 0) {
        return DB_VERSION_UNKNOWN;
    }
    else if (version == CURRENT_DB_VERSION) {
        return DB_UP_TO_DATE;
    }
    else if (version < CURRENT_DB_VERSION) {
        return DB_NEEDS_UPGRADE;
    } else {
        return DB_TOO_NEW;
    }
}

// SQL upgrade scripts for each database version. The version is the index, so
// the script at index 1 upgrades from v1 -> v2, the script at index 2 upgrades
// from v2 -> v3, etc.
static const char* UPGRADE_SCRIPTS[CURRENT_DB_VERSION] = {
    "", // There's no version 0, so no upgrade script to run
    R"(ALTER TABLE songs ADD COLUMN extension TEXT NOT NULL DEFAULT 'mp3';
       UPDATE db_meta SET version = 2;)",
};

bool run_upgrade(sqlite3* db) {
    int version = 0;
    const upgrade_status status = get_upgrade_status(db, &version);

    switch (status) {
    case DB_VERSION_UNKNOWN:
        LOG_MSG(error, "Unable to detect database version, it's not safe to load!\n");
        return false;
    case DB_TOO_NEW:
        LOG_MSG(error, "Database is too new (v%d, this build supports up to v%d). It's not safe to load!\n", version, CURRENT_DB_VERSION);
        return false;
    case DB_UP_TO_DATE:
        return true;
    case DB_NEEDS_UPGRADE:
        break;
    }

    char* errmsg = nullptr;
    int res = sqlite3_exec(db, UPGRADE_SCRIPTS[version], nullptr, nullptr, &errmsg);
    if (res != SQLITE_OK) {
        LOG_MSG(error, "Failed to upgrade database from v%d to v%d because: %s\n", version, version + 1, errmsg);
        return false;
    }
    LOG_MSG(info, "Successfully upgraded database to v%d\n", version + 1);

    // Keep trying to upgrade until something breaks or we get up to date
    return run_upgrade(db);
}
