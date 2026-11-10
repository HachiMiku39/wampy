#ifndef WAMPY_DISPLAY_TOUCH_MAPPING_H
#define WAMPY_DISPLAY_TOUCH_MAPPING_H

// C-compatible because the evdev reader lives in GLFW. Raw touch ranges need
// not equal pixel dimensions (WM1A/Z can report twice the panel resolution).
static inline int wampy_map_touch_axis(int value, int minimum, int maximum, int pixels, double *result) {
    if (!result || maximum <= minimum || pixels <= 0) return 0;
    double normalized = ((double)value - minimum) / ((double)maximum - minimum);
    if (normalized < 0.0) normalized = 0.0;
    if (normalized > 1.0) normalized = 1.0;
    *result = normalized * pixels;
    return 1;
}

static inline int wampy_map_touch(int raw_x, int raw_y,
                                int minimum_x, int maximum_x, int minimum_y, int maximum_y,
                                int long_side, int short_side, double *x, double *y) {
    double portrait_x, portrait_y;
    if (!x || !y ||
        !wampy_map_touch_axis(raw_x, minimum_x, maximum_x, short_side, &portrait_x) ||
        !wampy_map_touch_axis(raw_y, minimum_y, maximum_y, long_side, &portrait_y)) return 0;
    *x = portrait_y;
    *y = short_side - portrait_x;
    return 1;
}
#endif
