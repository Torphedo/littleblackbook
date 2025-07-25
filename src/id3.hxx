#pragma once
/// @file id3.hxx
/// Parsing for ID3 metadata tags used in MP3 files

#include <cassert>
#include <string>
#include <common/int.h>
#include <common/file.h>

namespace id3 {

// Metadata header at the top of the file
struct header {
    char magic[3]; // Should be "ID3"
    u8 version_major;
    u8 version_revision;
    bool using_unsynchronisation: 1;
    bool extended_header: 1;
    bool is_experimental_header: 1;
    u8: 0;

    // 28-bit size value stored in 4 bytes (see https://id3.org/id3v2.3.0#Declared_ID3v2_frames)
    u8 size_bytes[4];

    // Decodes the strangely formatted 28-bit size to a normal integer format
    u32 size() const noexcept;

    bool correct_magic() const noexcept {
        return magic[0] == 'I' && magic[1] == 'D' && magic[2] == '3';
    }
};
static_assert(sizeof(header) == 10);

enum text_encoding : u8 {
    TEXT_ASCII = 0,
    TEXT_UCS2 = 1, // 2-byte Unicode format which is *not* UTF-16
    TEXT_UTF16BE = 2, // 2-byte Unicode format which is *not* UTF-16
    TEXT_UTF8 = 3, // 2-byte Unicode format which is *not* UTF-16

};

static u8 char_size_for_encoding(text_encoding e) {
    if (e == TEXT_UCS2 || e == TEXT_UTF16BE) {
        return 2;
    }
    return 1;
}

// A wrapper for text frames, which can be either ASCII or UCS2.
struct text {
    u16 length: 16 = 0;
    // Spec says ASCII is default: https://id3.org/id3v2.3.0#ID3v2_frame_overview
    text_encoding encoding: 2 = TEXT_ASCII;
    uintptr_t ascii: 46;

    text() = default;
    text(u8* frame_data, u32 frame_size, u32 frame_offset);

    /// @brief Convert UCS-2 text to UTF-8 if needed
    std::string to_utf8(u8* frame_data) const noexcept;
};

// All relevant metadata frame types
enum frame_id : u32 {
    FRAME_TITLE =   MAGIC('T','I','T','2'),
    FRAME_PICTURE = MAGIC('A','P','I','C'),
    FRAME_COMMENT = MAGIC('C','O','M','M'),
    FRAME_ALBUM =   MAGIC('T','A','L','B'),
    FRAME_ARTIST =  MAGIC('T','P','E','1'),
    FRAME_YEAR =    MAGIC('T','Y','E','R'),
};

// Struct packing will make this header the wrong size
#pragma pack(push, r1, 1)
// Header for each frame of metadata
struct frame_header {
    frame_id id;
    u32 size;
    u16 flags;
};
#pragma pack(pop, r1)
static_assert(sizeof(frame_header) == 10);

} // namespace id3
