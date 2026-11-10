#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/wampy-text.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
# ICU 77+ host headers need C++17; the device continues to use ICU 61/C++11.
"${CXX:-c++}" -std=c++17 -g -O1 ${TEXT_TEST_FLAGS:-} \
    -DIMGUI_USER_CONFIG='"../../src/imgui.config.h"' \
    -I"$root/src" -I"$root/src/util" -I"$root/libs/imgui" \
    $(pkg-config --cflags icu-uc sqlite3) \
    "$root/tests/text_test.cpp" "$root/src/wstring.cpp" "$root/src/util/getCharRange.cpp" \
    "$root/libs/imgui/imgui.cpp" "$root/libs/imgui/imgui_draw.cpp" \
    "$root/libs/imgui/imgui_widgets.cpp" "$root/libs/imgui/imgui_tables.cpp" \
    $(pkg-config --libs icu-uc sqlite3) -o "$test_dir/text_test"
"$test_dir/text_test" "$@"
