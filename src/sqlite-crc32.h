#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <sqlite3.h>

int sqlite3_sqlitecrc_init(sqlite3 *const db, char **const pzErrMsg, const sqlite3_api_routines *pApi);

#ifdef __cplusplus
}
#endif
