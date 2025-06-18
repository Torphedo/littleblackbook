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

# Setup fresh database via SQLite command-line

# Make sure to use semicolons at the end of all SQL files you add. All files
# caught by the wildcard are sent to SQLite as a single stream of text, so it
# won't know where statements start and end without them.

cat bootstrap/table_*.sql | sqlite3 $DB_FILE
cat bootstrap/trigger_*.sql | sqlite3 $DB_FILE

# Our views depend on each other, but SQLite doesn't seem to care about the
# order they're created as long as they look syntactically correct. Errors
# about missing tables won't appear until runtime.
cat bootstrap/view_*.sql | sqlite3 $DB_FILE
