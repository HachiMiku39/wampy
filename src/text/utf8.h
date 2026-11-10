#ifndef WAMPY_TEXT_UTF8_H
#define WAMPY_TEXT_UTF8_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace WampyText {

// Decode one Unicode scalar. Malformed input always consumes at least one
// byte, so callers collecting glyphs cannot loop forever or read past the end.
inline uint32_t NextUtf8(const std::string &text, size_t &offset) {
    if (offset >= text.size()) return 0;
    const size_t start = offset++;
    const unsigned char first = static_cast<unsigned char>(text[start]);
    if (first < 0x80) return first;
    unsigned count;
    uint32_t value, minimum;
    if (first >= 0xc2 && first <= 0xdf) { count = 2; value = first & 0x1f; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { count = 3; value = first & 0x0f; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { count = 4; value = first & 7; minimum = 0x10000; }
    else return 0xfffd;
    if (text.size() - start < count) return 0xfffd;
    for (unsigned i = 1; i < count; ++i) {
        const unsigned char next = static_cast<unsigned char>(text[start + i]);
        if ((next & 0xc0) != 0x80) return 0xfffd;
        value = (value << 6) | (next & 0x3f);
    }
    if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) return 0xfffd;
    offset = start + count;
    return value;
}

inline void AppendUtf8(std::string &text, uint32_t value) {
    if (value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff)) value = 0xfffd;
    if (value < 0x80) text += static_cast<char>(value);
    else if (value < 0x800) {
        text += static_cast<char>(0xc0 | (value >> 6));
        text += static_cast<char>(0x80 | (value & 0x3f));
    } else if (value < 0x10000) {
        text += static_cast<char>(0xe0 | (value >> 12));
        text += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
        text += static_cast<char>(0x80 | (value & 0x3f));
    } else {
        text += static_cast<char>(0xf0 | (value >> 18));
        text += static_cast<char>(0x80 | ((value >> 12) & 0x3f));
        text += static_cast<char>(0x80 | ((value >> 6) & 0x3f));
        text += static_cast<char>(0x80 | (value & 0x3f));
    }
}

inline std::string ValidUtf8(const std::string &text) {
    std::string result;
    result.reserve(text.size());
    size_t offset = 0;
    while (offset < text.size()) AppendUtf8(result, NextUtf8(text, offset));
    return result;
}

// Capacity includes the terminator. Never split a character at the boundary.
inline void CopyUtf8(char *destination, size_t capacity, const std::string &text) {
    if (!destination || capacity == 0) return;
    size_t offset = 0, used = 0;
    while (offset < text.size()) {
        std::string character;
        const auto value = NextUtf8(text, offset);
        if (value == 0) break;
        AppendUtf8(character, value);
        if (character.size() > capacity - 1 - used) break;
        for (char byte : character) destination[used++] = byte;
    }
    destination[used] = '\0';
}

} // namespace WampyText
#endif
