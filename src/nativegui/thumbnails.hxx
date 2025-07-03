#pragma once
#include <map>

#include <common/image.h>

#include <schema.hxx>

using image_hash_t = u32;

void update_gl_tex(texture img, gl_obj gl_tex);

struct thumbnail_storage {
    std::map<image_hash_t, gl_obj> thumbnails;
    std::map<song_hash_t, image_hash_t> song_map;

    gl_obj operator[](song_hash_t song_hash) const noexcept;
    bool load_from_mp3(const char* path, song_hash_t hash) noexcept;

};
