#include "expression.hxx"
#include <cctype>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>

#include <common/int.h>
#include <common/logging.h>

std::vector<std::string_view> shatter_str(const char* text, s32 len) {
    // Remove trailing whitespace
    while (isspace(text[MAX(0, len - 1)]) && len > 0) {
        len--;
    }

    // Skip leading whitespace
    while (isspace(*text) && len > 0) {
        text++;
        len--;
    }

    std::vector<std::string_view> out;
    u32 last_token_end = 0;
    for (s32 i = 0; i < len; i++) {
        const u32 prev_pos = MAX(0, i - 1);
        const char cur_ch = text[i];
        const char prev_ch = text[prev_pos];

        bool is_token_end = false;
        if (isspace(prev_ch)) {
            last_token_end++; // Skip spaces
        }
        else if (isspace(cur_ch)) {
            is_token_end = true; // End token when we hit whitespace
        }

        // Parens always end the last token
        if (cur_ch == '(' || cur_ch == ')') {
            is_token_end = true;
        }

        // And form their own 1-character tokens
        if (prev_ch == '(' || prev_ch == ')') {
            is_token_end = true;
        }

        const char* token_begin = &text[last_token_end];
        const s32 token_len = i - last_token_end;
        if (is_token_end && token_len > 0) {
            out.emplace_back(token_begin, token_len);
            last_token_end = i;
        }
    }

    const u32 token_len = MAX(0, len - last_token_end);
    if (token_len > 0) {
        out.emplace_back(&text[last_token_end], token_len);
    }

    return out;
}

tag_expression::tag_expression(const char* text) : tag_expression(text, strlen(text)) {}

tag_expression::tag_expression(const char* text, u32 len) {
    // Shatter input into tokens
    const std::vector<std::string_view> tokens = shatter_str(text, len);

    LOG_MSG(info, "Split \"%s\" into:\n", text);
    for (const std::string_view& str : tokens) {
        LOG_MSG(info, "");
        std::cout << "\t \"" << str << "\"\n";
    }

    // TODO:
    // - Split by "AND" / "OR" / "NOT" / "-"

    // - Do very basic parsing not accounting for parens
    // - Add SQL generator method, use in the search bar for testing
    // - Add support for non-nested parens like (A AND B) OR (C AND D)
    // - Call ctor recursively to handle nested parens
}
