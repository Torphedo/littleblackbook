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

#define RULES_JAPANESE "ja_Hrkt-ja_Latn/BGN;"
// We could use "Any-Latin;" to do best-effort conversion... but it will treat
// Japanese kanji as Pinyin Chinese characters, which is often wrong. Since I
// have no Chinese characters in my data but some kanji, we just don't convert
// Chinese at all.
#define RULES_ASCII_NO_CHINESE "Latin-ASCII;"
#define RULES_AMERICANIZE RULES_JAPANESE RULES_ASCII_NO_CHINESE "Lower;"

std::string run_transliterator(const char* str, const char* translit_rules);
void run_transliterator(std::string& str, const char* translit_rules);
