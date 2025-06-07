#pragma once

// String constants for table/column names that might change

// C allows you to combine multiple string literals by placing them next to each
// other. e.g. INSERT INTO songs" == "INSERT INTO " "songs".
// So, constant format strings for SQL queries can be constructed if you use a
// bit of strange syntax like:
//     "INSERT INTO " SONG_TABLE " (" SONG_NAME ", " SONG_ARTIST ", " ...
//

// Name of the many-to-many table that associates tags with songs via hash
#define TAG_SONG_TABLE "tagmap"

// Name of the many-to-many table that lets tags automatically "imply" other tags
#define TAG_PARENT_TABLE "tags_parents"
