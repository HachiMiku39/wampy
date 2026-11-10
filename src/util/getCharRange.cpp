#include "getCharRange.h"

#include "dlog.h"
#include "text/unicode.h"
#include "text/utf8.h"
#include <cstdio>
#include <set>
#include <sqlite3.h>

void getCharRange(sqlite3 *db, std::vector<uint32_t> *points) {
    if (!db || !points) return;
    std::set<uint32_t> characters(points->begin(), points->end());
    // A missing optional table must not prevent titles/artists contributing.
    const char *queries[] = {"SELECT value FROM artists", "SELECT title FROM object_body",
                            "SELECT value FROM albums", "SELECT filename FROM object_body"};
    for (const auto *query : queries) {
        sqlite3_stmt *statement = nullptr;
        if (sqlite3_prepare_v2(db, query, -1, &statement, nullptr) != SQLITE_OK) {
            DLOG("Glyph query unavailable: %s (%s)\n", query, sqlite3_errmsg(db));
            sqlite3_finalize(statement);
            continue;
        }
        int result;
        while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
            const auto *value = sqlite3_column_text(statement, 0);
            if (!value) continue;
            const std::string text(reinterpret_cast<const char *>(value), sqlite3_column_bytes(statement, 0));
            for (const auto &variant : {text, WampyText::UpperUtf8(text)}) {
                size_t offset = 0;
                while (offset < variant.size()) {
                    const auto point = WampyText::NextUtf8(variant, offset);
                    if (point) characters.insert(point);
                }
            }
        }
        if (result != SQLITE_DONE) DLOG("Glyph query failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(statement);
    }
    points->assign(characters.begin(), characters.end());
}

void getCharRange(std::vector<uint32_t> *points) {
#ifdef DESKTOP
    const char *path = "../MTPDB.dat";
#else
    const char *path = "/db/MTPDB.dat";
#endif
    sqlite3 *db = nullptr;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        DLOG("Can't open glyph database: %s\n", sqlite3_errmsg(db));
    } else {
        getCharRange(db, points);
    }
    sqlite3_close(db);
}
