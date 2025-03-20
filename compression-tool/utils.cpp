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
unsigned int decode_utf8_char_to_utf32(unsigned char c, unsigned int state, char32_t &utf32_codepoint)
{
    // Reference: https://writings.sh/post/en/utf8

    switch (state)
    {
        case state_0_accept:
            if (c >= 0x0 && c <= 0x7f)
            {
                utf32_codepoint = c;
            }
            else if (c >= 0xc2 && c <= 0xdf)
            {
                state = state_1; //1;
                utf32_codepoint = c & 0x1f;
            }
            else if (c == 0xe0)
            {
                state = state_4; //4;
                utf32_codepoint = c & 0xf;
            }
            else if (c >= 0xe1 && c <= 0xef)
            {
                state = state_2; //2;
                utf32_codepoint = c & 0xf;
            }
            else if (c == 0xf0)
            {
                state = state_5; //5;
                utf32_codepoint = c & 0x7;
            }
            else if (c >= 0xf1 && c <= 0xf3)
            {
                state = state_3; //3;
                utf32_codepoint = c & 0x7;
            }
            else if (c == 0xf4)
            {
                state = state_6; //6;
                utf32_codepoint = c & 0x7;
            }
            else
            {
                state = state_8_failure; //8;
            }
            break;
        case state_1:
        case state_2:
        case state_3:
            if (c >= 0x80 && c <= 0xbf)
            {
                state--;
                utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
            }
            else
            {
                state = state_8_failure; //8;
            }
            break;
        case state_4:
            if (c >= 0xa0 && c <= 0xbf)
            {
                state = state_1;
                utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
            }
            else
            {
                state = state_8_failure; //8;
            }
            break;
        case state_5:
            if (c >= 0x90 && c <= 0xbf)
            {
                state = state_2;
                utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
            }
            else
            {
                state = state_8_failure; //8;
            }
            break;
        case state_6:
            if (c >= 0x80 && c <= 0x8f)
            {
                state = state_2;
                utf32_codepoint = (utf32_codepoint << 6) | (c & 0x3f);
            }
            else
            {
                state = state_8_failure; //8;
            }
            break;
        default:
            state = state_8_failure; //8;
            break;
    }
    return state;
}
} // namespace kcompress