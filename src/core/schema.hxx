#pragma once
#include <common/int.h>

// String constants for table/column names that might change

// C allows you to combine multiple string literals by placing them next to each
// other. e.g. "INSERT INTO songs" == "INSERT INTO " "songs".
// So, constant format strings for SQL queries can be constructed if you use a
// bit of strange syntax like:
//     "INSERT INTO " SONG_TABLE " (" SONG_NAME ", " SONG_ARTIST ", " ...
//

// Name of the many-to-many table that associates tags with songs via hash
#define TAG_SONG_TABLE "tagmap"

// Name of the many-to-many table that lets tags automatically "imply" other tags
#define TAG_PARENT_TABLE "tag_parents"

// A view that's like TAG_SONG_TABLE, but with parents automatically applied
#define RESOLVED_TAG_SONG_TABLE "resolved_tagmap"

#define CURRENT_DB_VERSION 2

// Typedefs in case we go back to unsigned eventually
using song_hash_t = s32;
using tag_hash_t = s32;

struct linked_tags {
    tag_hash_t parent;
    tag_hash_t child;
};
