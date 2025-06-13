CREATE TRIGGER auto_album_tag AFTER INSERT on SONGS
BEGIN
    INSERT OR IGNORE INTO tags (tag, hash) VALUES (NEW.album, crc32(NEW.album));
    INSERT OR IGNORE INTO tagmap (tag_hash, song_hash) VALUES (crc32(NEW.album), NEW.hash);
END
