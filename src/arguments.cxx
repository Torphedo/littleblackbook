#include "arguments.hxx"
#include <cstring>
#include <cassert>
#include <common/int.h>

static const char* const flags[] = {
    "--db",
};

arguments::arguments(int argc, char** argv) {
    for (u32 i = 0; i < argc; i++) {
        for (u32 j = 0; j < ARG_ENUM_MAX; j++) {
            seen_flags[j] = strcmp(argv[i], flags[j]) == 0;
            if (seen_flags[j]) {
                // We found an argument flag!
                assert(i + 1 < argc && "A flag had no value!"); // Bounds check
                // Next argument is the value
                flag_values[j] = argv[i + 1];
                i++;
                first_non_flag = i;
            }
        }
    }
}
