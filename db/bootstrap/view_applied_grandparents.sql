-- View for parent tags applied after 2 levels of recursion (grandparents)
CREATE VIEW applied_grandparents (song_hash, tag_hash) AS
SELECT song_hash, parent_hash FROM applied_parents parents
INNER JOIN tag_parents ON
parents.tag_hash = tag_parents.child_hash;
