-- Text search for tags
CREATE VIRTUAL TABLE tag_search USING fts5(tag, hash);

-- Text search for lyrics
CREATE VIRTUAL TABLE lyric_search USING fts5(letra, song_hash);
