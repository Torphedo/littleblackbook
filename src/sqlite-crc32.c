/*
** 2017-12-17
** Taken from https://github.com/DrkShadow/sqlite-crc32, modified to only use
** CRC32 and remove external dependencies.
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
*/
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#include <common/crc32.h>

#define SQLITE_IS_OK(res) (res == SQLITE_OK || res == SQLITE_DONE || res == SQLITE_ROW)

static void crc32Func( sqlite3_context *context, const int argc, sqlite3_value **const argv) {
	assert(argc == 1);

	const int value_type = sqlite3_value_type(argv[0]);
	assert(value_type != SQLITE_BLOB);

	if (value_type == SQLITE_BLOB || value_type == SQLITE_TEXT) {
		const void *pData = sqlite3_value_blob(argv[0]);
		const int nData = sqlite3_value_bytes(argv[0]);

		const u32 crc = crc32fast(pData, nData);

		sqlite3_result_int(context, crc);
	}
	else if (sqlite3_value_type(argv[0]) == SQLITE_INTEGER) {
		const int val = sqlite3_value_int(argv[0]);

		const u32 crc = crc32fast((u8*)&val, sizeof(val));

		sqlite3_result_int(context, crc);

		SQLITE_IS_OK(1);
	} else {
		sqlite3_result_value(context, argv[0]);
	}
}

int sqlite3_sqlitecrc_init(sqlite3 *const db, char **const pzErrMsg, const sqlite3_api_routines *pApi) {
	SQLITE_EXTENSION_INIT2(pApi);

	int rc = sqlite3_create_function(db, "crc32", 1,
								SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC,
								0, crc32Func, NULL, NULL);
	return rc;
}
