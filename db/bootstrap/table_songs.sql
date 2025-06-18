-- Schema for the table of songs
CREATE TABLE IF NOT EXISTS "songs" (
    title     TEXT NOT NULL,
    artist    TEXT NOT NULL,
    album     TEXT NOT NULL DEFAULT 'single',
    year      INTEGER NOT NULL,
    lyrics    TEXT,
    -- TODO: Make NOT NULL once we can get this data from the MP3
    duration_secs    INTEGER,
    -- CRC32 hash, left as just "hash" in case the algorithm changes later
    hash      INTEGER NOT NULL UNIQUE,
    import_timestamp INTEGER NOT NULL DEFAULT 0 -- Unix timestamp, set by a trigger
)STRICT;
