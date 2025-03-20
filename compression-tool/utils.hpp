#pragma once
#include <string>
#include <array>

namespace kcompress
{
constexpr unsigned int state_0_accept = 0;
constexpr unsigned int state_1 = 1;
constexpr unsigned int state_2 = 2;
constexpr unsigned int state_3 = 3;
constexpr unsigned int state_4 = 4;
constexpr unsigned int state_5 = 5;
constexpr unsigned int state_6 = 6;
constexpr unsigned int state_8_failure = 8;

// Converts a UTF-32 codepoint into UTF-8 bytes
// @return No. of bytes in the converted character
unsigned int convert_utf32_to_utf8_char(char32_t code, std::array<unsigned char, 4>& output_utf8);

// Decodes a UTF-8 byte into a UTF-32 codepoint using a state machine based on the current state
// @return The next state number. If returns state number is 0, then the conversion was successful.
// Else if the state number is 8, the conversion failed. Any other state means more processing is required.
// In case, state number 0 is returned, utf32_codepoint variable is filled.
unsigned int decode_utf8_char_to_utf32(unsigned char c, unsigned int state, char32_t& utf32_codepoint);
}   // kcompress