#include "arguments.hxx"
#include <cstring>
#include <cassert>
#include <common/int.h>

static const char* const flags[ARG_ENUM_MAX] = {
    "--db",
};

arguments::arguments(int argc, char** argv) {
    for (u32 i = 0; i < argc; i++) {
        for (u32 j = 0; j < ARG_ENUM_MAX; j++) {
            // Check every argument against every known flag
            if (strcmp(argv[i], flags[j]) == 0) {
                // If the flag was the last argument, there's no string value to get out...
                assert((i + 1) < (argc - 1) && "One of the flags is missing a value!");

                // Everything's good, mark that we found the flag and store it
                seen_flags[j] = true;
                flag_values[j] = argv[i + 1];
                i++;
                first_non_flag = i;
            }
        }
    }
}
