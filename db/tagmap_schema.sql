-- Many-to-many table attaching tags to songs
CREATE TABLE IF NOT EXISTS "tagmap" (
    "song_hash" INTEGER NOT NULL,
    "tag_id" INTEGER NOT NULL
)STRICT;
