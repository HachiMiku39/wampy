#ifndef WAMPY_TEXT_UNICODE_H
#define WAMPY_TEXT_UNICODE_H

#include <string>
#include <unicode/locid.h>
#include <unicode/unistr.h>

namespace WampyText {
inline std::string UpperUtf8(const std::string &text) {
    std::string result;
    icu::UnicodeString::fromUTF8(text).toUpper(icu::Locale::getRoot()).toUTF8String(result);
    return result;
}
} // namespace WampyText
#endif
