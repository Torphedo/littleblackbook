-- This trigger automatically adds the album name to the song as a tag
CREATE TRIGGER auto_album_tag AFTER INSERT ON songs
BEGIN
    -- Tag may already exist, so it's fine to ignore it in that case
    INSERT OR IGNORE INTO tags (tag, hash) VALUES (lower(NEW.album), crc32(lower(NEW.album)));
    INSERT INTO tagmap (tag_hash, song_hash) VALUES (crc32(lower(NEW.album)), NEW.hash);
END;
