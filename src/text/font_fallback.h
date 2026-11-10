#ifndef WAMPY_TEXT_FONT_FALLBACK_H
#define WAMPY_TEXT_FONT_FALLBACK_H

#include "imgui.h"
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace WampyText {
// Native-size rasterization keeps large CJK libraries within the 4096-pixel
// texture limit without allocating every character in the CJK block.
inline ImFont *AddPrimaryFont(ImFontAtlas *atlas, const std::string &path, float size, const ImWchar *ranges) {
    ImFontConfig config;
    config.OversampleH = config.OversampleV = 1;
    return atlas->AddFontFromFileTTF(path.c_str(), size, &config, ranges);
}

inline int AddFontFallbacks(ImFontAtlas *atlas, ImFont *destination, float size,
                            const ImWchar *ranges, const std::vector<std::string> &paths) {
    if (!destination) return 0;
    int added = 0;
    for (const auto &path : paths) {
        std::ifstream file(path, std::ios::binary);
        if (!file.good()) continue;
        file.close();
        ImFontConfig config;
        config.MergeMode = true;
        config.DstFont = destination;
        config.OversampleH = config.OversampleV = 1;
        if (atlas->AddFontFromFileTTF(path.c_str(), size, &config, ranges)) ++added;
    }
    return added;
}

inline int AddSonyFontFallbacks(ImFontAtlas *atlas, ImFont *destination, float size, const ImWchar *ranges, const std::string &primaryPath) {
#ifdef DESKTOP
    const std::string root = "../";
#else
    const std::string root = "/system/vendor/sony/lib/fonts/";
#endif
    std::vector<std::string> paths = {root + "NotoSansKR-Regular.otf", root + "SSTJpPro-Regular.otf",
                                      root + "DFHeiW5-A-SONY-CTC.ttf", root + "NotoSansThai-Regular.ttf", root + "SSTUI-Roman.ttf"};
    // Avoid retaining a second copy of the multi-megabyte primary font.
    paths.erase(std::remove(paths.begin(), paths.end(), primaryPath), paths.end());
    return AddFontFallbacks(atlas, destination, size, ranges, paths);
}
} // namespace WampyText
#endif
