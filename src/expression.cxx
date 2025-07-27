#include "expression.hxx"
#include <cctype>
#include <cstring>
#include <cassert>
#include <cstdlib>

#include <queue>
#include <string>

#include <common/int.h>
#include <common/logging.h>
#include "blackbook_core.hxx"
#include "tags.hxx"

tag_expression::value::value(const tag_expression& other_ex) {
    if (other_ex.op == tag_op::NONE) {
        // Trivial expression, we can "inline" it as an immediate value
        tag = other_ex.lhs.tag;
    } else {
        // This requires its own expression
        recursive = true;
        expr = (tag_expression*)calloc(1, sizeof(*expr));
        if (expr) {
            *expr = other_ex;
        }
    }
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
    {   .token = "AND",
        .op_enum = tag_op::AND,
        .left_binding = 3,
        .right_binding = 2,
    },
    {   .token = "OR",
        .op_enum = tag_op::OR,
        .left_binding = 2,
        .right_binding = 1,
    },
    {   .token = "NOT",
        .op_enum = tag_op::NOT,
        .left_binding = 0,
        .right_binding = 5,
    },
    {   .token = "-",
        .op_enum = tag_op::NOT,
        .left_binding = 0,
        .right_binding = 5,
    },

    // Making parens an operator stops the tokenizer from trying to merge it with
    // nearby tokens.
    {   .token = "(",
        .op_enum = tag_op::PAREN,
    },
    {   .token = ")",
        .op_enum = tag_op::PAREN,
    },
};

operator_t op_from_token(const char* text, u32 len) {
    operator_t result = {};
    for (operator_t op : pratt_ops) {
        if (strncmp(text, op.token, len) == 0) {
            result = op;
            break;
        }
    }

    return result;
}

operator_t op_from_token(const substr_t& str) {
    return op_from_token(str.data, str.length);
}

static const char reserved_chars[] = "()-";

std::queue<substr_t> shatter_str(const char* text, s32 len) {
    // Remove trailing whitespace
    while (isspace(text[MAX(0, len - 1)]) && len > 0) {
        len--;
    }

    // Skip leading whitespace
    while (isspace(*text) && len > 0) {
        text++;
        len--;
    }

    std::queue<substr_t> out;
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
        } else {
            // Reserved characters always end the previous token, and form their
            // own 1-character tokens
            // TODO: '-' might get used in normal text, should probably be handled another way
            for (char c : reserved_chars) {
                if (cur_ch == c || prev_ch == c) {
                    is_token_end = true;
                }
            }
        }

        const char* token_begin = &text[last_token_end];
        const s32 token_len = i - last_token_end;
        if (is_token_end && token_len > 0) {
            const tag_op cur_op = op_from_token(token_begin, token_len).op_enum;
            tag_op prev_op = tag_op::NONE;
            if (out.size() > 0) {
                prev_op = op_from_token(out.back()).op_enum;
            }
            // Merge non-operator tokens (allows tokens to have whitespace)
            if (cur_op == tag_op::NONE && prev_op == tag_op::NONE) {
                const char* cur_token_end = token_begin + MAX(0, token_len);
                const s32 combined_len = MAX(0, cur_token_end - out.back().data);
                out.back().length = combined_len;
            } else {
                // Proceed as normal
                out.emplace(token_begin, token_len);
            }
            last_token_end = i;
        }
    }

    // Make the rest of the string a token
    const u32 token_len = MAX(0, len - last_token_end);
    if (token_len > 0) {
        out.emplace(&text[last_token_end], token_len);
    }

    return out;
}

tag_expression parse_head_tokens(substr_t cur_tok, std::queue<substr_t>& lex) {
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

void parse_tail_tokens(const substr_t& cur_tok, std::queue<substr_t>& lex, tag_expression& partial_expr) {
    const operator_t cur_op = op_from_token(cur_tok);
    tag_expression next_expr = recurse_parse(lex, cur_op.left_binding);
    if (cur_op.op_enum != tag_op::NONE) {
        // Build a new expression with the previous and next expression as children
        tag_expression temp = {};
        temp.lhs = tag_expression::value(partial_expr);
        temp.rhs = tag_expression::value(next_expr);
        partial_expr = temp;
    } else {
        partial_expr.rhs.tag = next_expr.lhs.tag;
    }
    partial_expr.op = cur_op.op_enum;
}

tag_expression recurse_parse(std::queue<substr_t>& lex, u8 subexpr_precedence) {
    substr_t& cur_tok = lex.front();
    lex.pop();
    tag_expression processed_left;
    if (cur_tok.data[0] == '(') {
        // Treat everything inside parens as a totally independent expression
        processed_left = recurse_parse(lex, 0);

        substr_t& closing_tok = lex.front();
        lex.pop();
        // TODO: Do safe error handling (here and throughout parsing)
        assert(closing_tok.data[0] == ')');
    } else {
        processed_left = parse_head_tokens(cur_tok, lex);
    }

    while (!lex.empty() && op_from_token(lex.front()).left_binding > subexpr_precedence) {
        substr_t& tok = lex.front();
        lex.pop();
        parse_tail_tokens(tok, lex, processed_left);
    }

    return processed_left;
}

tag_expression::tag_expression(const char* text, u32 len) {
    auto tokens = shatter_str(text, len);
    *this = recurse_parse(tokens, 0);
    // TODO:
    // - Add SQL generator method, use in the search bar for testing

void sqlgen_value(const tag_expression::value& val, bool is_negated, std::string& sql_out) {
    if (val.tag.data == nullptr) {
        return; // No valid expression or text here
    }

    if (val.recursive) {
        sql_out.append("SELECT * FROM (");
        sqlgen_expression(*val.expr, sql_out);
        sql_out.append(")");
    } else if (val.tag.data) {
        if (is_negated) {
            sql_out.append("SELECT hash FROM songs EXCEPT ");
        }
        LOG_MSG(debug, "Hashing tag %d chars from \"%*s\"\n", val.tag.length, val.tag.length, val.tag.data);
        search_tag(val.tag.data, sql_out, false, val.tag.length);
    }
}

void sqlgen_expression(const tag_expression& expr, std::string& sql_out) {
    static u32 recurse_depth = 0;
    if (recurse_depth == 0) {
        sql_out.append("SELECT hash FROM songs INTERSECT\n");
    }
    recurse_depth++;


    sqlgen_value(expr.lhs, (expr.op == tag_op::NOT), sql_out);

    if (expr.op == tag_op::AND) {
        sql_out.append("\nINTERSECT ");
    } else if (expr.op == tag_op::OR) {
        sql_out.append("\nUNION ");
    }
    sqlgen_value(expr.rhs, false, sql_out);

    recurse_depth--;

    if (recurse_depth == 0) {
        // Terminate statement
        sql_out.append(";\n");
    }
}
