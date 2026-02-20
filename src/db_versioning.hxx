#pragma once
#include <sqlite3.h>

enum upgrade_status {
    DB_VERSION_UNKNOWN,
    DB_NEEDS_UPGRADE,
    DB_UP_TO_DATE,
    DB_TOO_NEW,
};

/// @brief Check if the database schema needs to be updated
/// @param db The database to check
upgrade_status get_upgrade_status(sqlite3* db);

/// @brief Upgrade the database if needed
/// @param db The database to check
/// @return Whether the upgrade succeeded
bool run_upgrade(sqlite3* db);