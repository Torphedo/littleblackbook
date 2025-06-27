-- Automatically adds the album name to the song as a tag
CREATE TRIGGER auto_album_tag AFTER INSERT ON songs BEGIN
    -- Create album namespace if needed
    INSERT OR IGNORE INTO namespaces (namespace, hash) VALUES('album', crc32('album'));
    -- Create album tag if needed (with namespace)
    INSERT OR IGNORE INTO tags (tag, hash, namespace_hash) VALUES
        (lower(NEW.album), crc32('album:' || lower(NEW.album)), crc32('album'));

    -- Attach the album tag to the song
    INSERT INTO tagmap (tag_hash, song_hash) VALUES
        (crc32('album:' || lower(NEW.album)), NEW.hash);
END;

-- Automatically add timestamps to newly inserted song records
CREATE TRIGGER auto_timestamps AFTER INSERT ON songs BEGIN
    UPDATE songs SET import_timestamp = unixepoch() WHERE hash = NEW.hash;
END;

-- Triggers to keep the tag search index in sync with the original table
CREATE TRIGGER tag_search_insert AFTER INSERT ON tags BEGIN
    INSERT INTO tag_search (tag, hash) VALUES(NEW.tag, NEW.hash);
END;

CREATE TRIGGER tag_search_update_tag AFTER UPDATE OF tag ON tags BEGIN
    UPDATE tag_search SET tag = NEW.tag WHERE tag_search.tag = NEW.tag;
END;

CREATE TRIGGER tag_search_delete AFTER DELETE ON tags BEGIN
    DELETE FROM tag_search WHERE tag_search.hash = OLD.hash;
END;

-- Triggers to keep the lyric search index in sync with the original table
CREATE TRIGGER lyric_search_insert AFTER INSERT ON songs BEGIN
    INSERT INTO lyric_search (letra, song_hash) VALUES(NEW.lyrics, NEW.hash);
END;

CREATE TRIGGER lyric_search_update_tag AFTER UPDATE OF lyrics ON songs BEGIN
    UPDATE lyric_search SET letra = NEW.lyrics;
END;

CREATE TRIGGER lyric_search_delete AFTER DELETE ON songs BEGIN
    DELETE FROM lyric_search WHERE lyric_search.song_hash = OLD.hash;
END;
