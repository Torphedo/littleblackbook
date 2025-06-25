CREATE TABLE IF NOT EXISTS "songs" (
    title  TEXT    NOT NULL,
    artist TEXT    NOT NULL,
    album  TEXT    NOT NULL DEFAULT 'single',
    year   INTEGER NOT NULL,
    lyrics TEXT,
    -- TODO: Make NOT NULL once we can get this data from the MP3
    duration_secs INTEGER,
    hash             INTEGER NOT NULL UNIQUE, -- CRC32 hash
    import_timestamp INTEGER NOT NULL DEFAULT 0 -- Unix timestamp, set by a trigger
)STRICT;

-- Tags can have a namespace (denoted by a ":"), which helps handle conflicts.
-- e.g. "album:1979" != "year:1979", and "artist:sublime" != "album:sublime".

-- The reason this is a separate database construct is that SQLite's FTS system
-- doesn't support full substring searches, only token prefixes (e.g. "w"
-- matches "where", but "ere" doesn't). When searching, we want to type "sub"
-- and see suggestions for "artist:sublime" and "album:sublime". Keeping the
-- namespace separate from the tag makes this easy.
CREATE TABLE IF NOT EXISTS tags (
    -- The tag without the namespace (e.g. just "elton john").
    tag TEXT NOT NULL UNIQUE ON CONFLICT IGNORE,

    -- CRC32 hash of the whole tag name, (e.g. crc32("artist:elton john")).
    -- This lets us have a single unique tag column, and makes it easy for the
    -- GUI code to look up tags from user input.
    hash INTEGER NOT NULL UNIQUE ON CONFLICT IGNORE,

    -- Many tags will be in the same namespace (or no namespace), so this is
    -- nullable and non-unique. e.g. crc32("artist") (no colon!)
    namespace_hash INTEGER
)STRICT;

-- The set of tag namespaces
CREATE TABLE IF NOT EXISTS namespaces (
    namespace TEXT NOT NULL UNIQUE ON CONFLICT IGNORE,
    hash INTEGER NOT NULL UNIQUE ON CONFLICT IGNORE, -- crc32 of other column
)STRICT;

-- Many-to-many table for parent-child tag relationship.
-- For example: Madvillian is a collab between MF DOOM and Madlib. If we had a
-- song that's only labelled "madvillian", it should also be labelled "mf doom"
-- and "madlib". We can make that happen automatically by listing "madvillian"
-- as a child of both artists in the parent table:
--
-- child: crc32("madvillian"), parent: crc32("mf doom")
-- child: crc32("madvillian"), parent: crc32("madlib")
CREATE TABLE IF NOT EXISTS tag_parents (
    child_hash INTEGER NOT NULL,
    parent_hash INTEGER NOT NULL
)STRICT;

-- Many-to-many table attaching tags to songs
CREATE TABLE IF NOT EXISTS tagmap (
    song_hash INTEGER NOT NULL,
    tag_hash INTEGER NOT NULL
)STRICT;
