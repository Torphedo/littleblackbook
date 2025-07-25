#pragma once

#include <common/int.h>
#include "expression.hxx"

tag_expression::tag_expression(const char* text, u32 len) {
    // TODO:
    // Shatter input into tokens (const std::string_view only!)
    // - Split by "AND" / "OR" / "NOT" / "-"
    // - Cut leading/trailing whitespace, but leave whitespace within a token

    // - Do very basic parsing not accounting for parens
    // - Add SQL generator method, use in the search bar for testing
    // - Add support for non-nested parens like (A AND B) OR (C AND D)
    // - Call ctor recursively to handle nested parens
}
