#pragma once
#include <common/int.h>

// Arguments that involve a flag and value passed on command line
typedef enum {
    VALUE_ARG_DB_PATH,
    VALUE_ARG_NEW_TAG,
    VALUE_ARG_PARENT,
    VALUE_ARG_CHILD,
    VALUE_ARG_ENUM_MAX,
}value_arg_idx;

// Arguments that involve only a flag
typedef enum {
    SETTING_ARG_HELP,
    SETTING_ARG_VERSION,
    SETTING_ARG_IMPORT,
    SETTING_ARG_SEARCH,
    SETTING_ARG_LINK_TAGS,
    SETTING_ARG_ENUM_MAX,
}setting_arg_idx;

struct arguments {
    int argc = 0;
    const char* const* const argv = nullptr;

    // Whether the flags passed warrant the program being run in headless (CLI) mode
    bool cli_mode = false;

    // Whether each argument was found on the command-line with a corresponding value
    // e.g. whether the user provided a specific type of path
    bool seen_values[VALUE_ARG_ENUM_MAX] = {0};

    // If the corresponding bool is true, this array will have the string set as
    // the value. e.g. for a path flag, this would have the actual path string
    const char* values[VALUE_ARG_ENUM_MAX] = {0};

    // Whether the user provided the specified setting flag
    bool settings[VALUE_ARG_ENUM_MAX] = {0};

    // The index of the first argument after the last known command-line flag.
    // All arguments from this index on should be filenames or other non-flag strings
    u32 first_non_flag = 1;

    // Honestly, I just wanted to use 3 consts in some real code for fun - torph
    arguments(int argc, const char* const* const argv);
};
