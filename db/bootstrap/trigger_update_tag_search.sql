-- Populate search table as original table is populated
CREATE TRIGGER update_tag_search_insert AFTER INSERT ON tags
BEGIN
    INSERT INTO tag_search (tag) VALUES(NEW.tag);
END;

-- Keep search table updated
CREATE TRIGGER update_tag_search_update AFTER UPDATE OF tag ON tags
BEGIN
    UPDATE tag_search SET tag = NEW.tag WHERE tag_search.tag = NEW.tag;
END;

CREATE TRIGGER update_tag_search_delete AFTER DELETE ON tags
BEGIN
    DELETE FROM tag_search WHERE tag_search.tag = OLD.tag;
END;
