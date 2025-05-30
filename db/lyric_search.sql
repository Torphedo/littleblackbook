-- SELECT * FROM lyric_search WHERE lyric_search MATCH 'lyrics : NEAR("she", 50)' ORDER BY rank;
SELECT * FROM lyric_search WHERE lyric_search MATCH 'sheriff' ORDER BY rank;