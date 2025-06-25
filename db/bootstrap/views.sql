-- A version of the tagmap table that has only the list of direct parent tags.
CREATE VIEW applied_parents (song_hash, tag_hash) AS
    SELECT song_hash, parent_hash FROM (
        SELECT * FROM tagmap
        INNER JOIN tag_parents ON
        tag_parents.child_hash = tagmap.tag_hash
    );

-- View for parent tags applied after 2 levels of recursion (grandparents)
CREATE VIEW applied_grandparents (song_hash, tag_hash) AS
    SELECT song_hash, parent_hash FROM applied_parents parents
    INNER JOIN tag_parents ON
    parents.tag_hash = tag_parents.child_hash;

-- View for parent tags applied after 3 levels of recursion (great-grandparents)
CREATE VIEW applied_great_grandparents (song_hash, tag_hash) AS
    SELECT song_hash, parent_hash FROM applied_grandparents grandparents
    INNER JOIN tag_parents ON
    grandparents.tag_hash = tag_parents.child_hash;

-- A version of the tagmap table with 3 layers of parent tags pre-applied
CREATE VIEW resolved_tagmap (song_hash, tag_hash) AS
    SELECT * FROM (
        SELECT * FROM tagmap
        UNION SELECT * FROM applied_parents
        UNION SELECT * FROM applied_grandparents
        UNION SELECT * FROM applied_great_grandparents
    );
