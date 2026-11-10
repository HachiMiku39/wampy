#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_dir=$(mktemp -d "${TMPDIR:-/tmp}/wampy-display.XXXXXX")
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM

"${CXX:-c++}" -std=c++11 -Wall -Wextra -Werror -pedantic ${DISPLAY_TEST_FLAGS:-} \
    -I"$root/src" "$root/tests/display_test.cpp" -o "$test_dir/display_test"
"$test_dir/display_test"
"${CC:-cc}" -std=c99 -Wall -Wextra -Werror -pedantic ${DISPLAY_TEST_FLAGS:-} \
    -I"$root/src" "$root/tests/touch_mapping_test.c" -o "$test_dir/touch_mapping_test"
"$test_dir/touch_mapping_test"
