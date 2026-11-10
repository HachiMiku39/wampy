#ifndef WAMPY_GETCHARRANGE_H
#define WAMPY_GETCHARRANGE_H

#include <cstdint>
#include <vector>

struct sqlite3;
void getCharRange(sqlite3 *database, std::vector<uint32_t> *points);

void getCharRange(std::vector<uint32_t> *points);

#endif // WAMPY_GETCHARRANGE_H
