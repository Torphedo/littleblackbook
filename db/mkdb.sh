#!/bin/sh

# Accept a custom database filename
export DB_FILE="blackbook.db3"

if [ $# -eq 1 ]
then
    export DB_FILE=$1;
elif [ $# -gt 1 ]
then
    echo "This script only takes one argument... continuing anyway."
    echo $#
fi

# Setup tables on a fresh database via SQLite command-line
cat song_schema.sql | sqlite3 $DB_FILE
cat tag_schema.sql | sqlite3 $DB_FILE
cat tagmap_schema.sql | sqlite3 $DB_FILE

