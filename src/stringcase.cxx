#include "stringcase.hxx"
#include <algorithm>

// TODO: This will break for any non-ASCII characters. We need to use the ICU
// library to get correct behaviour.
// https://stackoverflow.com/questions/313970/how-to-convert-an-instance-of-stdstring-to-lower-case#313990
// https://icu.unicode.org
//
// We might also want to just convert to the closest ASCII character, since
// I can't type Unicode without a numpad anyway. - torph
void str_tolower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

std::string str_tolower_copy(std::string& str) {
    std::string copy = str;
    str_tolower(copy);
    return copy;
}

std::string str_tolower_copy(const char* str) {
    std::string copy = str;
    str_tolower(copy);
    return copy;
}

