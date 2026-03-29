#pragma once
/// An SQLite extension that adds a new crc32() function usable in SQL queries.

/// The function takes one argument (a TEXT, BLOB, or INTEGER value) and returns
/// an INTEGER value containing the CRC32C hash (note the "C" on the end, this
/// is a different algorithm than plain CRC32!)

#ifdef __cplusplus
extern "C" {
#endif
#include <sqlite3.h>

/// @brief Callback to load the CRC32 extension.
int sqlite3_sqlitecrc_init(sqlite3 *const db, char **const pzErrMsg, const sqlite3_api_routines *pApi);

#ifdef __cplusplus
}
#endif
