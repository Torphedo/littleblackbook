#pragma once
#include <string_view>

#include <common/int.h>

enum class tag_op : u8 {
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
            const tag_expression* expr;
            const std::string_view tag;
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

    tag_expression(const char* text);
    tag_expression(const char* text, u32 len);
};
