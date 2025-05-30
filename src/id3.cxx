#include "id3.hxx"

namespace id3 {

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
