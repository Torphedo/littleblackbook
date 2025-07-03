#pragma once
#include <map>

#include <common/image.h>

#include <schema.hxx>

// Upload a texture to OpenGL using the provided texture ID
void update_gl_tex(texture img, gl_obj gl_tex);

using image_hash_t = u32;

struct thumbnail_storage {
    std::map<image_hash_t, gl_obj> thumbnails;
    std::map<song_hash_t, image_hash_t> song_map;

    // Get the OpenGL texture for a song's thumbnail by hash
    gl_obj operator[](song_hash_t song_hash) const noexcept;

    /// @brief Load a thumbnail from an MP3 and associate the song hash with it
    ///
    /// Thumbnail hashes are also tracked, so a single texture is re-used for
    /// multiple songs with the same thumbnail data.
    /// Since songs are always named by their hash, we assume you can provide
    /// the song hash and avoid an expensive recalculation.
    bool load_from_mp3(const char* path, song_hash_t hash) noexcept;
};
