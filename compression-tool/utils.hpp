#pragma once
#include <string>
#include <array>

namespace kcompress
{
unsigned int convert_utf32_to_utf8_char(char32_t code, std::array<unsigned char, 4>& output_utf8);
}   // kcompress