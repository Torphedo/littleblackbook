-- A version of the tagmap table that has only the list of direct parent tags.
CREATE VIEW applied_parents (song_hash, tag_hash) AS
SELECT song_hash, parent_hash FROM (
    SELECT * FROM tagmap
    INNER JOIN tag_parents ON
    tag_parents.child_hash = tagmap.tag_hash
);
