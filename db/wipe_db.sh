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

rm files/*.mp3
sqlite3 $DB_FILE "\
DROP TABLE IF EXISTS db_meta;\
DROP TABLE IF EXISTS songs;\
DROP TABLE IF EXISTS tags;\
DROP TABLE IF EXISTS namespaces;\
DROP TABLE IF EXISTS tagmap;\
DROP TABLE IF EXISTS tag_parents;\
DROP TABLE IF EXISTS tag_search;\
DROP TABLE IF EXISTS lyric_search;\
DROP VIEW IF EXISTS applied_parents;\
DROP VIEW IF EXISTS applied_grandparents;\
DROP VIEW IF EXISTS applied_great_grandparents;\
DROP VIEW IF EXISTS resolved_tagmap;\
"
