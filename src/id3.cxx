#include "id3.hxx"
#include <common/utf8.h>
#include <cstdio>
#include <cstring>
#include <locale>
#include <string>
#include <string_view>

#include "common/int.h"

namespace id3 {

void text::print() const noexcept {
    if (encoding == TEXT_ASCII) {
        printf("%s", ascii);
    } else {
        print_c16s(ucs2);
    }
}

std::string text::to_utf8() const noexcept {
    std::string output;
    for (u16 i = 0; i < length; i++) {
        const char16_t c = ucs2[i];
        // UTF8 struct isn't null-terminated, so we need to copy it...
        // TODO: Fix bobtail so that the UTF-8 struct is null-terminated
        char converted[5] = {0};
        // Sorry for 1-letter variable, I couldn't think of a name.
        const utf8 u = codepoint_to_utf8(c);
        strncpy(converted, u.data, sizeof(u));

        // Actually copy the UTF-8 data into the string
        output.append(converted);
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
