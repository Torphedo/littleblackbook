#pragma once

/// @brief Copy a file as efficiently as possible
///
/// On Linux, if both files are on the same filesystem which supports
/// copy-on-write (e.g. btrfs), the copy will share the same physical disk space
/// as the original file until one of them is edited. This makes copies way
/// faster, reduces hard drive wear, and saves disk space.
///
/// In all other cases, this does normal copying by reading and writing
/// chunks of data one at a time.
/// @param source File to copy
/// @param target Location to copy the file to
/// @return Result
bool copy_file(const char* source, const char* target);