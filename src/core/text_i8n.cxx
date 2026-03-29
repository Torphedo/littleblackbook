#include "text_i8n.hxx"
#include <algorithm>

#ifdef HAVE_ICU
#include <unicode/translit.h>
#include <unicode/unistr.h>
#include <unicode/utrans.h>
#include <unicode/utypes.h>
#endif

#include <common/logging.h>
#include <common/int.h>

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

void run_transliterator(std::string& str, const char* translit_rules) {
#ifdef HAVE_ICU
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::Transliterator* trans = icu::Transliterator::createInstance(translit_rules, UTRANS_FORWARD, status);
    if (U_FAILURE(status)) {
        LOG_MSG(error, "Failed to initialize transliterator because: %s\n", u_errorName(status));
        return;
    }

    // This causes copying...
    icu::UnicodeString unistr(str.c_str());
    trans->transliterate(unistr);

    str.clear();
    unistr.toUTF8String(str);
#endif
}

std::string run_transliterator(const char* str, const char* translit_rules) {
#ifdef HAVE_ICU
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::Transliterator* trans = icu::Transliterator::createInstance(translit_rules, UTRANS_FORWARD, status);
    if (U_FAILURE(status)) {
        LOG_MSG(error, "Failed to initialize transliterator because: %s\n", u_errorName(status));
        return str;
    }

    icu::UnicodeString unistr(str);
    trans->transliterate(unistr);

    std::string stdstr;
    unistr.toUTF8String(stdstr);

    return stdstr;
#else
    return str;
#endif
}

std::string romanize_japanese(const char* str) {
    // ja_Hrkt = Mixed Hiragana + Katakana.
    // ja_Latn = Japanese romanized to Latin alphabet (not sure what "/BGN" does)
    const std::string result = run_transliterator(str, RULES_JAPANESE);

    LOG_MSG(debug, "Romanized \"%s\" -> \"%s\"\n", str, result.c_str());
    return result;
}
