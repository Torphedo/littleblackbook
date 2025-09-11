#pragma once
#include <map>
#include <queue>
#include <mutex>
#include <thread>

#include <common/image.h>

#include <schema.hxx>

// Upload a texture to OpenGL using the provided texture ID
void update_gl_tex(texture img, gl_obj gl_tex);

using image_hash_t = u32;

struct texture_entry {
    texture tex;
    image_hash_t ihash;
    song_hash_t song_hash;
};

/// @brief Decode a thumbnail from an MP3 file
///
/// This function is intended for internal use, but you can use it if you
/// like. The MP3 file is loaded from "[files_dir]/[song_hash].mp3". Remember
/// that song hashes are *signed* values.
/// @param song_hash The hash of the MP3. This is used to find the file, and
///                  to look up the thumbnail later.
/// @param image_out An output variable for the decoded texture buffer and
///                  texture+song hash.
/// @return Whether the texture successfully loaded.
bool image_from_mp3(song_hash_t song_hash, const char* files_dir, texture_entry* image_out);

class thumbnail_storage {
public:
    std::map<image_hash_t, gl_obj> thumbnails;
    std::map<song_hash_t, image_hash_t> song_map;
    const char* files_dir;
    static bool thread_stop_flag;

    thumbnail_storage(const char* files_dir) noexcept : files_dir(files_dir) {
        return;
    };

    /// @brief Get the OpenGL texture for a song's thumbnail by hash
    ///
    /// This may do more than .at() in the future (like queue a request to load
    /// the thumbnail for this hash).
    gl_obj operator[](song_hash_t song_hash) noexcept;

    // Same as operator[], but will never do anything except a basic lookup.
    gl_obj at(song_hash_t song_hash) const noexcept;

    // Wipe all state from the instance
    void clear() noexcept;

    /// @brief Decode a thumbnail from an MP3 file
    ///
    /// This function is intended for internal use, but you can use it if you
    /// like. The MP3 file is loaded from "[files_dir]/[song_hash].mp3". Remember
    /// that song hashes are *signed* values.
    /// @param song_hash The hash of the MP3. This is used to find the file, and
    ///                  to look up the thumbnail later.
    /// @param image_out An output variable for the decoded texture buffer and
    ///                  texture+song hash.
    /// @return Whether a new texture was loaded. If true, you're responsible
    ///         for freeing the memory in the texture entry.
    bool image_from_mp3(song_hash_t song_hash, texture_entry* image_out) const noexcept;

    /// @brief Upload textures from the internal queue to OpenGL
    ///
    /// This will always lock an internal mutex, but otherwise does no work when
    /// the queue is empty.
    void upload_deferred_textures() noexcept;

private:

    std::queue<texture_entry> texqueue;
    std::mutex texqueue_lock;
};
