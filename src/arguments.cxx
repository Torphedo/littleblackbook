#include "arguments.hxx"
#include <cstring>
#include <cassert>
#include <common/int.h>

static const char* const value_flags[VALUE_ARG_ENUM_MAX] = {
    "--db",
    "--new-tag",
    "--parent",
    "--child",
};

static const char* const setting_flags[SETTING_ARG_ENUM_MAX] = {
    "--help",
    "--version",
    "--import",
    "--search",
    "--link-tags",
};

// Shortcut to check that the provided index isn't the last command-line flag
#define NOT_FINAL_FLAG(i, argc) ((i) < ((argc) - 1))

arguments::arguments(int argc, const char* const* const argv) {
    for (u32 i = 0; i < argc; i++) {
        // Check every argument against every known value flag
        for (u32 j = 0; j < VALUE_ARG_ENUM_MAX; j++) {
            if (strcmp(argv[i], value_flags[j]) == 0) {
                // If the flag was the last argument, there's no string value to get out...
                assert(NOT_FINAL_FLAG(i, argc) && "One of the flags is missing a value!");

                // Everything's good, mark that we found the flag and store it
                seen_values[j] = true;
                values[j] = argv[i + 1];
                i++;
                first_non_flag = i;
            }
        }

        // Check every argument against every known setting flag
        for (u32 j = 0; j < SETTING_ARG_ENUM_MAX; j++) {
            if (strcmp(argv[i], setting_flags[j]) == 0) {
                settings[j] = true;
                if (NOT_FINAL_FLAG(i, argc)) {
                    first_non_flag = i + 1;
                }
            }
        }
    }
}
