#!/bin/sh

export DB_FILE="blackbook.db3"

# Accept a custom database filename
if [ $# -eq 1 ]
then
    export DB_FILE=$1;
elif [ $# -gt 1 ]
then
    echo "This script only takes one argument... continuing anyway."
    echo $#
fi

# Setup tables on a fresh database via SQLite command-line
cat table_songs.sql | sqlite3 $DB_FILE
cat table_tags.sql | sqlite3 $DB_FILE
cat table_tagmap.sql | sqlite3 $DB_FILE
cat table_tag_parents.sql | sqlite3 $DB_FILE

# Setup triggers
cat trigger_albumtag.sql | sqlite3 $DB_FILE

