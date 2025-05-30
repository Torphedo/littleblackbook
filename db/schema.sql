CREATE TABLE IF NOT EXISTS "songs" (
    "title"     TEXT NOT NULL,
    "artist"    TEXT NOT NULL,
    "album"     TEXT NOT NULL DEFAULT 'single',
    "year"      INTEGER NOT NULL,
    "lyrics"    TEXT,
    -- TODO: Make NOT NULL once we can get this data from the MP3
    "duration_secs"    INTEGER,
    "hash"      INTEGER NOT NULL,
    -- Unix timestamp
    "import_timestamp" INTEGER NOT NULL
) STRICT;
