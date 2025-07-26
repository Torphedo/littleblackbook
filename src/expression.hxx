#pragma once
#include <string_view>
#include <queue>
#include <cstring>

#include <common/int.h>

enum class tag_op : u8 {
    NONE, // Not an operator
    AND, // SQL INTERSECT
    OR,  // SQL UNION
    NOT, // SQL EXCEPT
};

// Recursive expression structure used for complex queries like:
// (artist:beastie boys OR artist:a tribe called quest) AND (year:1990s OR year:1980s) AND -year:1992
//
// That would get songs from the 80s or 90s (except 1992) by the Beastie Boys or A Tribe Called Quest.
struct tag_expression {
    struct value {
        union {
            tag_expression* expr;
            std::basic_string_view<char> tag;
        };
        value(const tag_expression* expr);
        value(const std::string_view& tag);
        value() : expr(nullptr) {}
    };

    // These aren't part of the value struct because they would add padding,
    // growing each value by 8 bytes. If recursive, the value is the expression
    // pointer instead of the tag.
    bool lhs_recursive = false;
    bool rhs_recursive = false;
    tag_op op;

    // Left/right hand side
    value lhs;
    value rhs;

    tag_expression(const char* text, u32 len);
    tag_expression(const char* text) : tag_expression(text, strlen(text)) {}
    tag_expression() = default;
private:
    tag_expression(std::queue<std::string_view> tokens);
};

tag_expression recurse_parse(std::queue<std::string_view> lex, u8 subexpr_precedence);
void parse_tail_tokens(const std::string_view& cur_tok, std::queue<std::string_view> lex, tag_expression& partial_expr);
