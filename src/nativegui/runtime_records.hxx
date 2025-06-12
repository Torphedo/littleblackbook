#pragma once
// This file is for structures representing database records, in the format most
// convenient for use at runtime.

#include <string>
#include <vector>
#include <set>

#include <common/int.h>

struct runtime_song {
    std::string name; // Song name
    time_t import_timestamp = 0;
    u32 hash = 0; // Hash of the underlying audio file
    u32 release_year = 0;

    // @brief All tags attached to the song (by hash)
    // These are "pre-calculated", in that implied tags (parents) in the database
    // are included.
    // This is intended for immediate display.
    std::set<u32> tags;
};
