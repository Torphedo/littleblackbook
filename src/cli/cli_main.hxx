#pragma once
#include "../arguments.hxx"
#include <sqlite3.h>

int cli_main(const arguments& args, sqlite3* db, const char* db_path);
