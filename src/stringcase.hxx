#pragma once
#include <string>
// Basic utilities for handling upper/lowercase strings.
// At the moment these only work on ASCII characters, and have been factored out
// so it's easy to upgrade the codebase to work on UTF-8 later.

// Change a string to all-lowercase in-place
void str_tolower(std::string& str);

// Make a lowercase clone of a string
std::string str_tolower_copy(const char* str);

// Make a lowercase clone of a string
std::string str_tolower_copy(const std::string& str);
