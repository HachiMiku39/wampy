#ifndef WAMPY_TEXT_FONT_TEXT_H
#define WAMPY_TEXT_FONT_TEXT_H

#include "imgui.h"
#include "utf8.h"
#include <cfloat>
#include <cmath>

namespace WampyText {
inline std::string EllipsizeUtf8(const std::string &input, ImFont *font, float size, float width) {
    if (!font || size <= 0 || !std::isfinite(size) || width <= 0 || std::isnan(width)) return {};
    const auto text = ValidUtf8(input);
    if (font->CalcTextSizeA(size, FLT_MAX, -1, text.c_str()).x <= width) return text;
    // ASCII dots are present in every supported font. Reserve their width.
    std::string dots = text.size() >= 2 ? ".." : ".";
    float dotsWidth = font->CalcTextSizeA(size, FLT_MAX, -1, dots.c_str()).x;
    if (dotsWidth > width) {
        dots = ".";
        dotsWidth = font->CalcTextSizeA(size, FLT_MAX, -1, dots.c_str()).x;
        if (dotsWidth > width) return {};
    }
    const char *remaining = text.c_str();
    if (width > dotsWidth)
        font->CalcTextSizeA(size, width - dotsWidth, -1, text.c_str(), nullptr, &remaining);
    return text.substr(0, static_cast<size_t>(remaining - text.c_str())) + dots;
}

inline void CropUtf8(char *text, size_t capacity, ImFont *font, float size, float width) {
    if (!text || capacity == 0) return;
    size_t length = 0;
    while (length < capacity && text[length]) ++length;
    const auto result = EllipsizeUtf8(std::string(text, length), font, size, width);
    CopyUtf8(text, capacity, result);
}
} // namespace WampyText
#endif
