#pragma once
#include <string>
// Basic utilities for handling strings.
// At the moment these only work on ASCII characters, and have been factored out
// so we can upgrade to UTF-8 later.
// Not much of an internationalization file yet, huh? - torph

// Change a string to all-lowercase in-place
void str_tolower(std::string& str);

// Make a lowercase clone of a string
std::string str_tolower_copy(const char* str);

// Make a lowercase clone of a string
std::string str_tolower_copy(const std::string& str);

// TODO: Add an americanize() function that turns UTF-8 to the closest ASCII equivalent
