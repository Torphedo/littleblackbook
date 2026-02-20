#include "filecopy.hxx"
#include <filesystem>

#include <common/platform.h>
#ifdef PLATFORM_LINUX
    #include <fcntl.h>
    #include <linux/fs.h>
    #include <sys/ioctl.h>
#endif

/// On Linux, try to take advantage of a copy-on-write filesystem.
bool try_cow_copy(const char* source, const char* target) {
#ifdef PLATFORM_LINUX
    const int source_fd = open(source, O_RDONLY);
    if (source_fd < 0) {
        return false;
    }

    const int dst_fd = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dst_fd < 0) {
        close(source_fd);
        return false;
    }

    // Try to do a clone operation which will use CoW to share data between the
    // original and the copy
    const int ret = ioctl(dst_fd, FICLONE, source_fd);
    if (ret < 0) {
        close(source_fd);
        close(dst_fd);
        return false;
    }

    close(dst_fd);
    close(source_fd);
    return true;
#else
    return false; // Assume CoW doesn't exist outside of Linux
#endif

}

bool fallback_copy(const char* source, const char* target) {
    return std::filesystem::copy_file(source, target);
}

bool copy_file(const char* source, const char* target) {
    if (try_cow_copy(source, target)) {
        return true;
    }

    return fallback_copy(source, target);
}