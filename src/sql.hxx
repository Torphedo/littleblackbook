#pragma once
#include <string>
#include <sqlite3.h>

/// @brief Generate SQL and immediately execute it against the database
///
/// Format string acts like sprintf (since it's a wrapper)
bool sqlgen_exec(sqlite3* db, const char* fmt, ...);

/// @brief Generate SQL and send it to an output buffer
///
/// Format string acts like sprintf (since it's a wrapper)
bool sqlgen(std::string& sql_out, const char* fmt, ...);
