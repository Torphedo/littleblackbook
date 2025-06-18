-- Many-to-many table for parent-child tag relationship.
-- For example: Black Star is the name of a collaboration between rappers
-- Mos Def and Talib Kweli. A song labelled "black star" should automatically
-- be labelled "mos def" and "talib kweli" as well. This would be done with 2
-- rows in the parent table:
-- parent: crc32("mos def"), child: crc32("black star")
-- parent: crc32("talib kweli"), child: crc32("black star")

-- To the user, it'll look like these are added directly to the list of tags.
-- In reality, it's a totally separate table we do a join on during search.
-- This means there's no cleanup to do when a parent is removed, and searches
-- can update immediately.
CREATE TABLE IF NOT EXISTS tag_parents (
    child_hash INTEGER NOT NULL,
    parent_hash INTEGER NOT NULL
)STRICT;
