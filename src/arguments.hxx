#pragma once
#include <common/int.h>

typedef enum {
    ARG_DB_PATH,
    ARG_ENUM_MAX
}setting_idx;

struct arguments {
    bool seen_flags[ARG_ENUM_MAX] = {0};
    char* flag_values[ARG_ENUM_MAX] = {0};
    u32 first_non_flag = 1;

    arguments(int argc, char** argv);
};
