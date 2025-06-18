-- A version of the tagmap table with 3 layers of parent tags pre-applied
CREATE VIEW resolved_tagmap (song_hash, tag_hash) AS
SELECT * FROM (
    SELECT * FROM tagmap
    UNION SELECT * FROM applied_parents
    UNION SELECT * FROM applied_grandparents
    UNION SELECT * FROM applied_great_grandparents
);
