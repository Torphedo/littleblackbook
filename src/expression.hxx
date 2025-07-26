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
    PAREN, // Internal to the parser, will never show up in a tree
};

// Recursive expression structure used for complex queries like:
// (artist:beastie boys OR artist:a tribe called quest) AND (year:1990s OR year:1980s) AND -year:1992
//
// That would get songs from the 80s or 90s (except 1992) by the Beastie Boys or A Tribe Called Quest.
struct tag_expression {
    struct value {
        union {
            tag_expression* expr;
            std::string_view tag;
        };
        bool recursive = false;
        value(const char* text, u32 len) : tag(text, len) {}
        value(const tag_expression& other_ex);
        value() : expr(nullptr) {}
    };

    // Left/right hand side
    value lhs;
    value rhs;

    tag_op op;

    tag_expression(const char* text, u32 len);
    tag_expression(const char* text) : tag_expression(text, strlen(text)) {}
    tag_expression() = default;
private:
    tag_expression(std::queue<std::string_view> tokens);
};

tag_expression recurse_parse(std::queue<std::string_view>& lex, u8 subexpr_precedence);
void parse_tail_tokens(const std::string_view& cur_tok, std::queue<std::string_view>& lex, tag_expression& partial_expr);
