-- Schema for the table of all tags
CREATE TABLE IF NOT EXISTS "tags" (
    "tag" TEXT NOT NULL UNIQUE,
    "id"  INTEGER NOT NULL UNIQUE,
    PRIMARY KEY("id" AUTOINCREMENT)
) STRICT;
