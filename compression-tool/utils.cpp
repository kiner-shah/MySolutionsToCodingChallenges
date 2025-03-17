#include "utils.hpp"

namespace kcompress
{
unsigned int convert_utf32_to_utf8_char(char32_t code, std::array<unsigned char, 4>& output_utf8)
{
    // Reference: https://writings.sh/post/en/utf8

    unsigned int bytes = 0;
    if (code <= 0x7f)
    {
        // 0xxxxxxx
        output_utf8[0] = code & 0xff;
        bytes = 1;
    }
    else if (code <= 0x7ff)
    {
        // 0xxx xxyyyyyy
        output_utf8[0] = (code >> 6) | 0xc0;    // 110xxxxx
        output_utf8[1] = (code & 0x3f) | 0x80;  // 10yyyyyy
        bytes = 2;
    }
    else if (code <= 0xffff)
    {
        // xxxxyyyy yyzzzzzz
        output_utf8[0] = (code >> 12) | 0xe0;            // 1110xxxx
        output_utf8[1] = ((code >> 6) & 0x3f) | 0x80;    // 10yyyyyy
        output_utf8[2] = (code & 0x3f) | 0x80;           // 10zzzzzz
        bytes = 3;
    }
    else if (code <= 0x10ffff)
    {
        // 000xxxyy yyyyzzzz zzwwwwww
        output_utf8[0] = (code >> 18) | 0xf0;           // 11110xxx
        output_utf8[1] = ((code >> 12) & 0x3f) | 0x80;  // 10yyyyyy
        output_utf8[2] = ((code >> 6) & 0x3f) | 0x80;   // 10zzzzzz
        output_utf8[3] = (code & 0x3f) | 0x80;          // 10wwwwww
        bytes = 4;
    }
    return bytes;
}
}   // namespace kcompress