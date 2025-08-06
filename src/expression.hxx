#pragma once
#include <string>
#include <variant>
#include <queue>
#include <cstring>

#include <common/int.h>

enum class tag_op : u8 {
    NONE,  // Not an operator
    AND,   // SQL INTERSECT
    OR,    // SQL UNION
    NOT,   // SQL EXCEPT
    PAREN, // Internal to the parser, will never show up in a tree
};

struct substr_t {
    const char* data;
    u32 length;

    // Implicit conversion
    operator std::string() {
        return std::string(data, length);
    }
};

// Recursive expression structure used for complex queries like:
// (artist:beastie boys OR artist:a tribe called quest) AND (year:1990s OR year:1980s) AND -year:1992
//
// That would get songs from the 80s or 90s (except 1992) by the Beastie Boys or A Tribe Called Quest.
struct tag_expression {
    using value = std::variant<tag_expression*, std::string>;

    #define VAL_IS_EXPR(val) std::holds_alternative<tag_expression*>(val)
    #define VAL_IS_IMM(val) std::holds_alternative<std::string>(val)
    #define VAL_IS_EMPTY_EXPR(val) (VAL_IS_EXPR(val) && std::get<tag_expression*>(val) == nullptr)
    #define VAL_IS_EMPTY_IMM(val) (VAL_IS_IMM(val) && std::get<std::string>(val).size() == 0)

    // Left/right hand side
    value lhs;
    value rhs;

    tag_op op;

    operator value() {
        tag_expression expr;
        if (op == tag_op::NONE) {
            // Trivial expression, we can "inline" it as an immediate value
            return lhs;
        } else {
            // This requires its own expression
            return new tag_expression;
        }
    }

    tag_expression(const char* text, u32 len);
    tag_expression(const char* text) : tag_expression(text, strlen(text)) {}
    tag_expression() = default;
private:
    tag_expression(std::queue<substr_t> tokens);
};

tag_expression recurse_parse(std::queue<substr_t>& lex, u8 subexpr_precedence);
void parse_tail_tokens(const substr_t& cur_tok, std::queue<substr_t>& lex, tag_expression& partial_expr);

void sqlgen_expression(const tag_expression& expr, std::string& sql_out);
