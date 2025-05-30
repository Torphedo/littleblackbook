#pragma once
#include <cassert>
#include <common/int.h>
#include <common/file.h>

namespace id3 {

struct header {
    char magic[3]; // Should be "ID3"
    u8 version_major;
    u8 version_revision;
    bool using_unsynchronisation: 1;
    bool extended_header: 1;
    bool is_experimental_header: 1;
    u8: 0;

    // 28-bit size value
    u8 size_bytes[4];

    u32 size() const noexcept;
};
static_assert(sizeof(header) == 10);

enum frame_id : u32 {
    FRAME_TITLE = MAGIC('T', 'I', 'T', '2'),
    FRAME_PICTURE = MAGIC('A', 'P', 'I', 'C'),
    FRAME_COMMENT = MAGIC('C', 'O', 'M', 'M'),
    FRAME_ALBUM = MAGIC('T', 'A', 'L', 'B'),
    FRAME_ARTIST = MAGIC('T', 'P', 'E', '1'),
    FRAME_YEAR = MAGIC('T', 'Y', 'E', 'R'),
};

enum : u8 {
    TEXT_ASCII = 0,
    TEXT_UCS2 = 1, // 2-byte Unicode format which is *not* UTF-16
};

// Struct packing will make this header the wrong size
#pragma pack(push, r1, 1)
struct frame_header {
    frame_id id;
    u32 size;
    u16 flags;
};
#pragma pack(pop, r1)
static_assert(sizeof(frame_header) == 10);

} // namespace id3
