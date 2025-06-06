-- Schema for the table of all tags
CREATE TABLE IF NOT EXISTS "tags" (
    "tag" TEXT NOT NULL UNIQUE,
    "hash"  INTEGER NOT NULL UNIQUE
)STRICT;
