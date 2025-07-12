#include "id3.hxx"
#include <common/utf8.h>
#include <common/vfile.h>
#include <cstdio>
#include <cstring>
#include <locale>
#include <string>
#include <string_view>

#include "common/int.h"

namespace id3 {

text::text(u8* frame_data, u32 frame_size) {
    vfile vf = vfile_open(frame_data, frame_size);

    encoding = VFILE_READ(id3::text_encoding, &vf);
    vfile_seek(&vf, sizeof(u16)); // Skip byte order marker
    ascii = vf.pos;
    const u32 remaining_size = vf.size - vf.pos;
    const u8 char_size = (encoding == TEXT_ASCII) ? 1 : 2;
    length = remaining_size / char_size;
}

std::string text::to_utf8(u8* frame_data) const noexcept {
    std::string output;
    output.reserve(length);
    for (u16 i = 0; i < length; i++) {
        const c16 c = *(c16*)(frame_data + ascii);
        // Sorry for 1-letter variable, I couldn't think of a name.
        const utf8 u = codepoint_to_utf8(c);
        output.append(u.data);
    }

    return output;
}

u32 header::size() const noexcept {
    // See https://id3.org/id3v2.3.0#ID3v2_header
    u32 sum = 0;
    for (s32 i = ARRAY_SIZE(size_bytes) - 1; i > -1; i--) {
        // Cut off the top bit of each byte as the spec says
        const u8 byte = size_bytes[i] & 0x7F;

        // Each byte only has 7 bits of data
        const s32 pos = (ARRAY_SIZE(size_bytes) - 1 - i);
        sum += (byte << (pos * 7));
    }
    return sum;
}

} // namespace id3
