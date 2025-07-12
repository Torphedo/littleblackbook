#pragma once
#include <string>
#include <sqlite3.h>
#include <id3.hxx>

#include <common/logging.h>

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

// Function overloads for most SQLite statement binding functions
#define DEF_SQL_BIND(args...) static int sql_bind(sqlite3_stmt* stmt, int pos, args) noexcept
#define CALL_SQL_BIND(funcT, args...) sqlite3_bind_##funcT(stmt, pos, args)
#define VALUE_BIND_TYPE(funcT, inT)   \
DEF_SQL_BIND(inT val) {               \
    return CALL_SQL_BIND(funcT, val); \
}
#define VALUE_BIND(T) VALUE_BIND_TYPE(T, T)
#define BUF_BIND_TYPE(bufT, sizeT, funcT)            \
DEF_SQL_BIND(bufT buf, sizeT size) {                 \
    return CALL_SQL_BIND(funcT, buf, size, nullptr); \
}

// Define overloads for many different types
VALUE_BIND(int)
VALUE_BIND(double)
VALUE_BIND_TYPE(value, const sqlite3_value*)
VALUE_BIND_TYPE(int64, sqlite3_int64)
BUF_BIND_TYPE(const char*, int, text)
BUF_BIND_TYPE(const c16*, int, text16)
BUF_BIND_TYPE(const void*, int, blob)
BUF_BIND_TYPE(const void*, sqlite_uint64, blob64)

// Overload for sqlite3_bind_text64 which takes encoding as a param
DEF_SQL_BIND(const char* buf, sqlite_uint64 size, unsigned char encoding) {
    return CALL_SQL_BIND(text64, buf, size, nullptr, encoding);
}

// ID3 overload is too different to make a macro for
static int sql_bind(sqlite3_stmt* stmt, int pos, const id3::text& str) noexcept {
    void (*const callback)(void*) = SQLITE_STATIC;
    if (str.encoding == id3::TEXT_UCS2) {
        // Length multiplied by 2 since our length is in characters, not bytes
        return sqlite3_bind_text16(stmt, pos, str.ucs2, str.length * 2, callback);
    } else {
        return sqlite3_bind_text(stmt, pos, str.ascii, str.length, callback);
    }
}

#undef VALUE_BIND_TYPE
#undef VALUE_BIND
#undef BUF_BIND_TYPE

