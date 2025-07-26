#include "expression.hxx"
#include <cctype>
#include <cstring>
#include <cstdlib>

#include <queue>

#include <common/int.h>
#include <common/logging.h>

static const char reserved_chars[] = "()-";

std::queue<std::string_view> shatter_str(const char* text, s32 len) {
    // Remove trailing whitespace
    while (isspace(text[MAX(0, len - 1)]) && len > 0) {
        len--;
    }

    // Skip leading whitespace
    while (isspace(*text) && len > 0) {
        text++;
        len--;
    }

    std::queue<std::string_view> out;
    s32 last_token_end = 0;
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

        // Reserved characters always end the last token, and form their own
        // 1-character tokens
        for (char c : reserved_chars) {
            if (cur_ch == c || prev_ch == c) {
                is_token_end = true;
            }
        }

        const char* token_begin = &text[last_token_end];
        const s32 token_len = i - last_token_end;
        if (is_token_end && token_len > 0) {
            out.emplace(token_begin, token_len);
            last_token_end = i;
        }
    }

    const u32 token_len = MAX(0, len - last_token_end);
    if (token_len > 0) {
        out.emplace(&text[last_token_end], token_len);
    }

    return out;
}

// Operators for use in Pratt parsing.
// See the following articles for details:
// https://abarker.github.io/typped/pratt_parsing_intro.html
// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
typedef struct {
    const char* token;
    tag_op op_enum;
    u8 left_binding;
    u8 right_binding;
}operator_t;

const operator_t pratt_ops[] = {
    {
        .token = "AND",
        .op_enum = tag_op::AND,
        .left_binding = 3,
        .right_binding = 2,
    },
    {
        .token = "OR",
        .op_enum = tag_op::OR,
        .left_binding = 2,
        .right_binding = 1,
    },
    {
        .token = "NOT",
        .op_enum = tag_op::NOT,
        .left_binding = 0,
        .right_binding = 5,
    },
    {
        .token = "-",
        .op_enum = tag_op::NOT,
        .left_binding = 0,
        .right_binding = 5,
    },
};

operator_t op_from_token(const std::string_view& str) {
    operator_t result = {};
    for (operator_t op : pratt_ops) {
        if (strncmp(str.data(), op.token, str.length()) == 0) {
            result = op;
            break;
        }
    }

    return result;
}

tag_expression parse_head_tokens(std::string_view cur_tok, std::queue<std::string_view>& lex) {
    tag_expression result = {};
    const operator_t cur_op = op_from_token(cur_tok);
    switch (cur_op.op_enum) {
    case tag_op::NOT:
        // NOT operator is unary and uses an extra token
        result.op = cur_op.op_enum;
        cur_tok = lex.front();
        lex.pop();
        // fallthrough
    default:
        result.lhs.tag = cur_tok;
        break;
    }

    return result;
}

void parse_tail_tokens(const std::string_view& cur_tok, std::queue<std::string_view>& lex, tag_expression& partial_expr) {

    const operator_t cur_op = op_from_token(cur_tok);
    tag_expression next_expr = recurse_parse(lex, cur_op.left_binding);
    if (cur_op.op_enum != tag_op::NONE) {
        // Build a new expression with the previous and next expression as children
        tag_expression temp = {};
        temp.lhs.expr = (tag_expression*)calloc(1, sizeof(tag_expression));
        temp.rhs.expr = (tag_expression*)calloc(1, sizeof(tag_expression));
        *temp.lhs.expr = partial_expr;
        *temp.rhs.expr = next_expr;
        temp.rhs_recursive = true;

        partial_expr = temp;
    } else {
        partial_expr.rhs.tag = next_expr.lhs.tag;
    }
    partial_expr.op = cur_op.op_enum;
}

tag_expression recurse_parse(std::queue<std::string_view>& lex, u8 subexpr_precedence) {
    auto& cur_tok = lex.front();
    lex.pop();
    tag_expression processed_left = parse_head_tokens(cur_tok, lex);

    while (!lex.empty() && op_from_token(lex.front()).left_binding > subexpr_precedence) {
        std::string_view& tok = lex.front();
        lex.pop();
        parse_tail_tokens(tok, lex, processed_left);
    }

    return processed_left;
}

tag_expression::tag_expression(const char* text, u32 len) {
    auto tokens = shatter_str(text, len);
    *this = recurse_parse(tokens, 0);
    // TODO:
    // - Split by "AND" / "OR" / "NOT" / "-"

    // - Do very basic parsing not accounting for parens
    // - Add SQL generator method, use in the search bar for testing
    // - Add support for non-nested parens like (A AND B) OR (C AND D)
    // - Call ctor recursively to handle nested parens
}
