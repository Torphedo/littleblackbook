-- View for parent tags applied after 3 levels of recursion (great-grandparents)
CREATE VIEW applied_great_grandparents (song_hash, tag_hash) AS
SELECT song_hash, parent_hash FROM applied_grandparents grandparents
INNER JOIN tag_parents ON
grandparents.tag_hash = tag_parents.child_hash;
