CREATE VIRTUAL TABLE tag_search USING fts5(tag, content=tags);
