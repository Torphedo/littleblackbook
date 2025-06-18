-- Many-to-many table for parent-child tag relationship.
-- For example "1992" always implies "90s", so "1992" is the child and
-- "90s" is the parent. This can also be useful for one-off collabs or name
-- changes: "black star" implies "mos def" and "talib kweli", so there would be
-- 2 entries marking the individual artist tags as parents of "black star".

-- As far as the user can tell, adding the "1992" tag automatically adds the
-- "90s" tag. But since this is a separate table that will be used during
-- search, new parents can be added/removed and appear to immediately propagate
-- through the entire database.
CREATE TABLE IF NOT EXISTS "tag_parents" (
    child_hash INTEGER NOT NULL,
    parent_hash INTEGER NOT NULL
)STRICT;
