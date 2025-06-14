CREATE TRIGGER auto_timestamps AFTER INSERT ON songs
BEGIN
    UPDATE songs SET import_timestamp = unixepoch() WHERE hash = NEW.hash;
END
