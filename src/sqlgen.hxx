#pragma once
#include <string>
#include <sqlite3.h>
#include <common/int.h>

/// @brief Generate SQL and immediately execute it against the database
///
/// Format string acts like sprintf (since it's a wrapper)
bool sqlgen_exec(sqlite3* db, const char* fmt, ...);

/// @brief Generate SQL and send it to an output buffer
///
/// Format string acts like sprintf (since it's a wrapper)
bool sqlgen(std::string& sql_out, const char* fmt, ...);

// Compile SQL and print detailed error messages on failure
sqlite3_stmt* compile_sql(const char* sql, s32 sql_len, sqlite3* db);

/// @brief Print an error message with an optional prefix if needed.
///
/// If the result code isn't an error, nothing is printed.
/// @param msg_prefix A custom message to put at the start of the message. A
///        colon will be added to separate it from the message.
/// @param db Database context to retrieve the message from
/// @param errcode The result code from an sqlite3 function
/// @return Whether the given result code was an error
bool sql_handle_error(const char* msg_prefix, sqlite3* db, int errcode);
