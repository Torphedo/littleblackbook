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
CREATE TRIGGER update_tag_search_insert AFTER INSERT ON tags BEGIN
    INSERT INTO tag_search (tag) VALUES(NEW.tag);
END;

CREATE TRIGGER update_tag_search_update AFTER UPDATE OF tag ON tags BEGIN
    UPDATE tag_search SET tag = NEW.tag WHERE tag_search.tag = NEW.tag;
END;

CREATE TRIGGER update_tag_search_delete AFTER DELETE ON tags BEGIN
    DELETE FROM tag_search WHERE tag_search.tag = OLD.tag;
END;
