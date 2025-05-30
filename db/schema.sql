CREATE TABLE IF NOT EXISTS "songs" (
    "title"     TEXT NOT NULL,
    "artist"    TEXT NOT NULL,
    "album"     TEXT NOT NULL DEFAULT 'single',
    "year"      INTEGER NOT NULL,
    "lyrics"    TEXT,
    -- e.g. 1.5 = 1 minute 30 seconds
    "duration_mins"    REAL NOT NULL
) STRICT;
