#include <cstdarg>
#include <cstdio>

#include <sqlite3.h>

#include <common/int.h>
#include <common/logging.h>
#include <string>

#include "scope_timer.hxx"

enum {
    SQLBUF_SIZE = 512,
};

// snprintf wrapper that reports errors and gives a simple bool return code
bool sqlgen_snprintf(char* sqlbuf, u32 sqlbuf_size, const char* fmt, va_list arg_list) {
    // We use helpers from stdarg.h to handle the variadic (...) arguments.
    const int return_code = vsnprintf(sqlbuf, sqlbuf_size - 1, fmt, arg_list);

    if (return_code < 0) {
        // Error occured
        return false;
    }
    if (return_code >= sqlbuf_size) {
        // String was truncated
        LOG_MSG(error, "Failed to generate SQL because it would've exceeded %d characters.\n", sqlbuf_size);
        return false;
    }

    return true;
}

bool sqlgen_exec(sqlite3* db, const char* fmt, ...) {
    char sqlbuf[SQLBUF_SIZE] = {0};

    // Generate the SQL
    va_list arg_list = {};
    va_start(arg_list, fmt);
    if (!sqlgen_snprintf(sqlbuf, ARRAY_SIZE(sqlbuf), fmt, arg_list)) {
        va_end(arg_list);
        return false;
    }
    va_end(arg_list);

    bool result = true;

    // Compile/execute the generated SQL
    float sqlexec_time = 0.0f;
    char* errmsg = nullptr;
    int sql_result = SQLITE_OK;
    { // Scope to control the timer
        const scope_timer sql_timer(sqlexec_time);
        sql_result = sqlite3_exec(db, sqlbuf, nullptr, nullptr, &errmsg);
    }
    if (sql_result != SQLITE_OK) {
        if (errmsg) {
            LOG_MSG(error, "SQLite error: %s\n", errmsg);
        }
        result = false;
    }

    LOG_MSG(debug, "SQL compile/execute finished in %.3fms\n", sqlexec_time);

    return result;
}

bool sqlgen(std::string& sql_out, const char* fmt, ...) {
    char sqlbuf[SQLBUF_SIZE] = {0};

    // Generate the SQL
    va_list arg_list = {};
    va_start(arg_list, fmt);
    if (!sqlgen_snprintf(sqlbuf, ARRAY_SIZE(sqlbuf), fmt, arg_list)) {
        va_end(arg_list);
        return false;
    }
    va_end(arg_list);


    sql_out.append(sqlbuf);
    return true;
}

sqlite3_stmt* compile_sql(const char* sql, s32 sql_len, sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    int songres = sqlite3_prepare_v2(db, sql, sql_len, &stmt, nullptr);

    if (songres != SQLITE_OK) {
        const char* msg = sqlite3_errmsg(db);
        if (msg) {
            LOG_MSG(error, "Couldn't compile SQL statement because: \"%s\"\n", msg);
        } else {
            LOG_MSG(error, "Couldn't compile SQL statement (no error message given)\n", msg);
        }

        return nullptr;
    }

    return stmt;
}

bool sql_handle_error(const char* msg_prefix, sqlite3* db, int errcode) {
    if (errcode == SQLITE_OK || SQLITE_DONE) {
        return true; // No errors to print
    }

    const char* msg = sqlite3_errmsg(db);
    LOG_MSG(error, "%s: %s\n", msg_prefix, msg);
    return false;
}
