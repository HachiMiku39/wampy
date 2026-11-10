#include "display/touch_mapping.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>

int main(void) {
    double x, y;
    assert(wampy_map_touch(480, 854, 0, 960, 0, 1708, 854, 480, &x, &y));
    assert(x == 427 && y == 240);
    assert(wampy_map_touch_axis(INT_MAX, INT_MIN, INT_MAX, 854, &x));
    assert(x == 854); /* subtraction must not overflow in integer arithmetic */
    assert(!wampy_map_touch_axis(0, 1, 1, 854, &x));
    assert(!wampy_map_touch_axis(0, 0, 960, 0, &x));
    puts("C evdev touch mapping passed");
    return 0;
}
