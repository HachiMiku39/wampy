#include "text/utf8.h"
#include "text/unicode.h"
#include "text/font_text.h"
#include "text/font_fallback.h"
#include "util/getCharRange.h"
#include "wstring.h"
#include <unicode/locid.h>
#include <sqlite3.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

static void sql(sqlite3 *db, const char *query) { assert(sqlite3_exec(db, query, nullptr, nullptr, nullptr) == SQLITE_OK); }
static void contains(const std::vector<uint32_t> &points, const std::string &text) {
    size_t offset = 0;
    while (offset < text.size()) assert(std::binary_search(points.begin(), points.end(), WampyText::NextUtf8(text, offset)));
}

int main(int argc, char **argv) {
    using namespace WampyText;
    static_assert(sizeof(ImWchar) == 4, "Supplementary CJK requires the actual project wchar32 configuration");
    const std::string mixed = "初音ミク・洛天依／紅蓮華／한글／ภาษาไทย／Beyoncé／Привет／𠮷野家";
    assert(ValidUtf8(mixed) == mixed);
    for (const std::string bad : {std::string("\xff"), std::string("\xe4\xb8"), std::string("\xc0\xaf"),
                                  std::string("\xed\xa0\x80"), std::string("\xf4\x90\x80\x80")}) {
        size_t offset = 0;
        while (offset < bad.size()) {
            const auto before = offset;
            assert(utfToPoint(bad, offset) == 0xfffd);
            assert(offset > before && offset <= bad.size());
        }
        const auto clean = ValidUtf8(bad);
        assert(ValidUtf8(clean) == clean);
    }
    for (size_t capacity = 1; capacity < mixed.size() + 2; ++capacity) {
        std::vector<char> buffer(capacity + 2, '#');
        CopyUtf8(buffer.data(), capacity, mixed);
        assert(buffer[capacity] == '#' && buffer[capacity + 1] == '#');
        assert(std::strlen(buffer.data()) < capacity);
        assert(ValidUtf8(buffer.data()) == buffer.data());
        assert(mixed.compare(0, std::strlen(buffer.data()), buffer.data()) == 0);
    }
    UErrorCode error = U_ZERO_ERROR;
    icu::Locale::setDefault(icu::Locale("tr_TR"), error);
    assert(U_SUCCESS(error));
    assert(UpperUtf8("初音ミク straße café i") == "初音ミク STRASSE CAFÉ I");

    sqlite3 *db = nullptr;
    assert(sqlite3_open(":memory:", &db) == SQLITE_OK);
    sql(db, "CREATE TABLE artists(value TEXT); CREATE TABLE object_body(title TEXT,filename TEXT); CREATE TABLE albums(value TEXT);");
    sql(db, "INSERT INTO artists VALUES('初音ミク'),(NULL); INSERT INTO object_body VALUES('紅蓮華 straße','𠮷野家.flac'); INSERT INTO albums VALUES('專輯限定 ภาษาไทย ﬃ');");
    std::vector<uint32_t> points;
    getCharRange(db, &points);
    contains(points, "初音ミク紅蓮華STRASSE𠮷野家專輯限定ภาษาไทยFFI");
    const auto previous = points;
    getCharRange(db, &points);
    assert(points == previous);
    sql(db, "DROP TABLE albums;");
    points.clear();
    getCharRange(db, &points); // optional album table must not erase title glyphs
    contains(points, "初音ミク紅蓮華𠮷野家");
    sqlite3_close(db);

    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddText(mixed.c_str());
    ImVector<ImWchar> ranges;
    builder.BuildRanges(&ranges);
    auto *font = io.Fonts->AddFontDefault();
    if (argc > 1) {
        io.Fonts->Clear();
        const std::string root = argv[1];
        font = AddPrimaryFont(io.Fonts, root + "/NotoSansKR-Regular.otf", 34, ranges.Data);
        assert(font);
        const int added = AddFontFallbacks(io.Fonts, font, 34, ranges.Data,
            {root + "/SSTJpPro-Regular.otf", root + "/DFHeiW5-A-SONY-CTC.ttf", root + "/NotoSansThai-Regular.ttf"});
        assert(added == 3);
    }
    assert(io.Fonts->Build());
    if (argc > 1) {
        size_t offset = 0;
        while (offset < mixed.size()) assert(font->FindGlyphNoFallback(NextUtf8(mixed, offset)));
        assert(io.Fonts->TexWidth <= 4096 && io.Fonts->TexHeight <= 4096);
        std::printf("Actual Sony font atlas: %d x %d; mixed-script glyphs present\n", io.Fonts->TexWidth, io.Fonts->TexHeight);
    }
    for (float width : {0.f, 1.f, 8.f, 24.f, 80.f, 180.f, 600.f}) {
        const auto fitted = EllipsizeUtf8(mixed, font, font->FontSize, width);
        assert(ValidUtf8(fitted) == fitted);
        assert(font->CalcTextSizeA(font->FontSize, FLT_MAX, -1, fitted.c_str()).x <= width + 0.01f);
    }
    char tiny[3] = {'W', '\0', '#'};
    CropUtf8(tiny, 2, font, font->FontSize, 1);
    assert(tiny[2] == '#'); // regression: old crop wrote two dots past a one-byte title
    if (argc > 1) {
        ImFontAtlas stress;
        const ImWchar stressRanges[] = {0x20, 0x7e, 0x4e00, 0x6dff, 0};
        auto *stressFont = AddPrimaryFont(&stress, std::string(argv[1]) + "/NotoSansKR-Regular.otf", 34, stressRanges);
        assert(stressFont);
        assert(AddFontFallbacks(&stress, stressFont, 34, stressRanges,
            {std::string(argv[1]) + "/SSTJpPro-Regular.otf", std::string(argv[1]) + "/DFHeiW5-A-SONY-CTC.ttf"}) == 2);
        assert(stress.Build());
        assert(stressFont->Glyphs.Size > 7000);
        assert(stress.TexWidth <= 4096 && stress.TexHeight <= 4096);
        std::printf("Large CJK atlas: %d glyphs, %d x %d\n", stressFont->Glyphs.Size, stress.TexWidth, stress.TexHeight);
    }
    ImGui::DestroyContext();
    puts("UTF-8, supplementary CJK, metadata glyph collection and clipping passed");
}
