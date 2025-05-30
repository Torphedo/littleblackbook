DROP TABLE lyric_search;
CREATE VIRTUAL TABLE IF NOT EXISTS lyric_search USING fts5(name, artist, album, lyrics);
INSERT OR REPLACE INTO lyric_search SELECT * FROM songs;
