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

static const char* tag_op_strs[] = {
    "", "AND", "OR", "NOT", "PAREN",
};

struct substr_t {
    const char* data;
    u32 length;

    // Implicit conversion
    operator std::string() {
        return std::string(data, length);
    }

    operator std::string_view() {
        return std::string_view(data, length);
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
    #define VAL_IS_EMPTY_IMM(val) (VAL_IS_IMM(val) && std::get<std::string>(val).empty())
    #define VAL_IS_EMPTY(val) (VAL_IS_EMPTY_IMM(val) || VAL_IS_EMPTY_EXPR(val))

    // Left/right hand side
    value lhs = "";
    value rhs = "";

    tag_op op = tag_op::AND;

    operator value() {
        if (op == tag_op::NONE || VAL_IS_EMPTY(rhs)) {
            // Trivial expression, we can inline it as an immediate value
            return lhs;
        } else {
            // This requires its own expression
            return new tag_expression(*this);
        }
    }

    tag_expression(const char* text, u64 len);
    explicit tag_expression(const char* text) : tag_expression(text, strlen(text)) {}
    explicit tag_expression(std::queue<substr_t> tokens);

    tag_expression() = default;
};

// Shatter text into a set of tokens
std::queue<substr_t> shatter_str(const char* text, s64 len);

// Generate a SQL query that implements the expression.
void sqlgen_expression(const tag_expression& expr, std::string& sql_out);