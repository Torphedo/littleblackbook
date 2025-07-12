#include <glad/glad.h>
#include "thumbnails.hxx"
#include <cstdio>
#include <cstdlib>

#include <mutex>
#include <stb_image.h>

#include <common/int.h>
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

gl_obj thumbnail_storage::operator[](song_hash_t song_hash) noexcept {
    return at(song_hash);
}

gl_obj thumbnail_storage::at(song_hash_t song_hash) const noexcept {
    if (song_map.count(song_hash) == 0) {
        return 0;
    }
    assert(thumbnails.count(song_map.at(song_hash)) && "Song exists in thumbnail map but has no thumbnail texture!");
    // std::map::operator[] is not const, we need .at()
    return thumbnails.at(song_map.at(song_hash));
}

void thumbnail_storage::clear() noexcept {
    thumbnails.clear();
    song_map.clear();
    std::lock_guard lock(texqueue_lock);
    while (!texqueue.empty()) {
        texture_entry entry = texqueue.front();
        texqueue.pop();
        free(entry.tex.data);
    }
}

bool thumbnail_storage::image_from_mp3(song_hash_t song_hash, texture_entry* image_out) const noexcept {
    if (song_map.count(song_hash)) {
        // Somehow we got a song hash whose thumbnail is already loaded, skip it
        return true;
    }

    char pathbuf[512] = {0};
    snprintf(pathbuf, ARRAY_SIZE(pathbuf), "%s/%d.mp3", files_dir, song_hash);

    u32 size = file_size(pathbuf);
    FILE* f = fopen(pathbuf, "rb");
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
        return 0;
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

    const image_hash_t ihash = crc32fast(buf, image_size);

    // This forces stbi to convert to our preferred number of channels. That
    // wastes some space on greyscale images, but stops them from being rendered
    // as red-only images (without needing a custom shader).
    const u8 desired_channels = 3;
    int x = 0;
    int y = 0;
    int channels = 0;
    u8* decoded_data = stbi_load_from_memory(buf, image_size, &x, &y, &channels, desired_channels);
    free(buf);

    if (!decoded_data) {
        return false;
    }

    *image_out = {
        .tex = (texture){
            .data = decoded_data,
            .width = (u16)x,
            .height = (u16)y,
            .channels = desired_channels,
        },
        .ihash = ihash,
        .song_hash = song_hash,
    };

    return true;
}

void thumbnail_storage::upload_deferred_textures() noexcept {
    std::lock_guard lock(texqueue_lock);
    while (!texqueue.empty()) {
        texture_entry entry = texqueue.front();
        texqueue.pop();

        // TODO: It would be nice if we could handle this without having to do a
        // full JPEG decode.
        if (thumbnails.count(entry.ihash)) {
            song_map[entry.song_hash] = entry.ihash;
            free(entry.tex.data);
            continue;
        }

        gl_obj gl_tex = 0;
        glGenTextures(1, &gl_tex);
        if (gl_tex == 0) {
            continue;
        }

        update_gl_tex(entry.tex, gl_tex);
        free(entry.tex.data);

        this->song_map[entry.song_hash] = entry.ihash;
        this->thumbnails[entry.ihash] = gl_tex;
    }
}
