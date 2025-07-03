#include "thumbnails.hxx"
#include "common/int.h"
#include <stdio.h>

#include <glad/glad.h>
#include <stb_image.h>

#include <common/file.h>
#include <common/image.h>
#include <common/endian.h>
#include <common/crc32.h>

#include <id3.hxx>


void update_gl_tex(texture img, gl_obj gl_tex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl_tex);

    // Wrapping & filtering settings
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLint res = (img.height * img.width);
    if (img.compressed) {
        GLenum format = 0;
        GLint size = res;
        switch (img.fmt) {
            case DXT3:
                format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;
            case DXT5:
                format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;
            default:
            case DXT1:
                format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                size /= 2;
                break;
        };

        // Re-upload the texture
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, size, img.data);

    } else {
        // "Raw" uncompressed image
        const GLenum gl_size = GL_UNSIGNED_BYTE + (img.unit_size * 2);
        GLint format;
        switch (img.channels) {
            case 1:
                format = GL_RED;
                break;
            case 2:
                format = GL_RG;
                break;
            case 3:
                format = GL_RGB;
                break;
            default:
                format = GL_RGBA;
                break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, gl_size, img.data);
    }

    // Reset state
    glBindTexture(GL_TEXTURE_2D, 0);
}

gl_obj thumbnail_storage::operator[](song_hash_t song_hash) const noexcept {
    if (song_map.count(song_hash) == 0) {
        return 0;
    }
    assert(thumbnails.count(song_map.at(song_hash)) && "Song exists in thumbnail map but has no thumbnail texture!");
    // std::map::operator[] is not const, we need .at()
    return thumbnails.at(song_map.at(song_hash));
}

bool thumbnail_storage::load_from_mp3(const char* path, song_hash_t song_hash) noexcept {
    u32 size = file_size(path);
    FILE* f = fopen(path, "rb");
    if (!f) {
        return false;
    }

    const id3::header header = {0};
    fread((void*)&header, sizeof(header), 1, f);
    assert(header.correct_magic() && "File is not an MP3!");
    assert(header.size() <= size && "Metadata claims to be larger than the MP3!");

    // This makes our loop simpler and avoids reading into the audio data
    size = header.size();

    // Find picture frame
    u32 frame_size = 0;
    while (!feof(f)) {
        id3::frame_header frame = {};
        fread((void*)&frame, sizeof(frame), 1, f);
        ENDIAN_FLIP(u32, frame.size);
        frame_size = frame.size;
        if (frame.id != id3::FRAME_PICTURE) {
            fseek(f, frame.size, SEEK_CUR);
        } else {
            break;
        }
    }
    if (frame_size == 0) {
        // Didn't find any image data.
        fclose(f);
        return false;
    }
    const long frame_start = ftell(f);

    // Skip 2 strings
    id3::text_encoding encoding;
    fread(&encoding, sizeof(encoding), 1, f);
    const u8 char_size = encoding == id3::TEXT_ASCII ? sizeof(u8) : sizeof(u16);

    u16 c = ' ';
    while (c != 0) {
        fread(&c, char_size, 1, f);
    }
    fseek(f, 1, SEEK_CUR); // Skip picture type byte

    c = ' ';
    while (c != 0) {
        fread(&c, char_size, 1, f);
    }
    const long image_start = ftell(f);
    const long image_size = (frame_start + frame_size) - image_start;

    // Now we're finally at the JPEG data
    u8* buf = (u8*)malloc(image_size);
    if (!buf) {
        fclose(f);
        return false;
    }
    fread(buf, image_size, 1, f);
    fclose(f);

    const image_hash_t ihash = crc32buf(buf, image_size);
    if (thumbnails.count(song_hash)) {
        this->song_map[song_hash] = ihash;
        return true; // Already loaded
    }

    int x = 0;
    int y = 0;
    int channels = 0;
    u8* data = stbi_load_from_memory(buf, image_size, &x, &y, &channels, 3);
    free(buf);

    if (!data) {
        return false;
    }

    gl_obj gl_tex = 0;
    glGenTextures(1, &gl_tex);
    if (gl_tex == 0) {
        return false;
    }

    texture tex = {
        .data = data,
        .width = (u16)x,
        .height = (u16)y,
        .channels = (u8)channels,
    };

    this->thumbnails[ihash] = gl_tex;
    this->song_map[song_hash] = ihash;
    update_gl_tex(tex, gl_tex);
    free(tex.data);

    return true;
}
