#pragma once
#include <string>
#include <sqlite3.h>
#include <arguments.hxx>

int cli_main(const arguments& args, sqlite3* db, const char* db_path, const std::string& db_files_folder);
