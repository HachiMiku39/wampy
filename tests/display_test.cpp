#include "display/viewport.h"
#include "display/touch_mapping.h"
#include <cassert>
#include <cstdio>
#include <limits>

static bool near(double a, double b) { return std::abs(a - b) < 0.0001; }

static void check_projection(const WampyDisplay::Viewport &view, WampyDisplay::Point point) {
    const auto matrix = view.Projection();
    const auto pixel = view.ToGL(point);
    const double x = (matrix[0] * point.x + matrix[4] * point.y + matrix[12] + 1) * view.SurfaceWidth() / 2;
    const double y = (matrix[1] * point.x + matrix[5] * point.y + matrix[13] + 1) * view.SurfaceHeight() / 2;
    assert(near(x, pixel.x) && near(y, pixel.y));
}

int main() {
    using namespace WampyDisplay;
    Viewport view;
    // Existing A50/ZX300 canvases remain unchanged, even with the option on.
    view.ConfigurePanel(480, 800, true);
    assert(!view.IsWM1AZ() && !view.FillsScreen());
    assert(view.ScaleX() == 1 && view.LogicalWidth() == 800);
    auto clip = view.Clip({0, 0, 800, 480});
    assert(clip.x == 0 && clip.y == 0 && clip.width == 480 && clip.height == 800);

    for (bool rotated : {false, true}) {
        view.ConfigurePanel(rotated ? 854 : 480, rotated ? 480 : 854, true);
        assert(view.IsWM1AZ() && view.FillsScreen());
        assert(view.LogicalWidth() == 800 && near(view.ScaleX(), 854.0 / 800));
        clip = view.Clip({0, 0, 800, 480});
        assert(clip.x == 0 && clip.y == 0 && clip.width == 480 && clip.height == 854);
        for (Point point : {Point{0,0}, {800,0}, {0,480}, {800,480}, {399.5,233.25}}) {
            check_projection(view, point);
            const auto touch = view.ToLogical({point.x * view.ScaleX(), point.y * view.ScaleY()});
            assert(near(touch.x, point.x) && near(touch.y, point.y));
        }
        // Fullscreen list/menu clipping matches rendering, not the old +800 hack.
        clip = view.Clip({100, 50, 300, 150});
        assert(clip.x == 330 && clip.width == 100);
        assert(clip.y == 533 && clip.height == 215);
        clip = view.Clip({-100, -100, 900, 600});
        assert(clip.x == 0 && clip.y == 0 && clip.width == 480 && clip.height == 854);
        assert(view.Clip({900, 0, 1000, 480}).height == 0);
        assert(view.Clip({0, 500, 100, 600}).width == 0);
        assert(view.Clip({10, 10, 5, 20}).width == 0);
        assert(view.Clip({0, 0, std::numeric_limits<double>::infinity(), 20}).width == 0);
    }

    // The physical EGL surface may also be portrait rather than square.
    view.SetSurface(480, 854);
    check_projection(view, {800, 480});
    clip = view.Clip({0, 0, 800, 480});
    assert(clip.width == 480 && clip.height == 854);

    // Legacy alignment retains the 54-pixel margin and never turns it into an edge tap.
    view.ConfigurePanel(480, 854, false);
    assert(!view.FillsScreen() && view.LogicalWidth() == 854);
    clip = view.Clip({0, 0, 800, 480});
    assert(clip.y == 54 && clip.height == 800);
    assert(view.ToLogical({830, 10}).x > 800);
    check_projection(view, {800, 480});
    view.ConfigurePanel(640, 960, true);
    assert(!view.IsWM1AZ() && !view.FillsScreen());
    view.ConfigurePanel(0, 0, true);
    assert(view.LongSide() == 800 && view.ShortSide() == 480);

    // Both double-resolution and offset/scaled evdev ranges cover the full panel.
    double x, y;
    assert(wampy_map_touch(0, 0, 0, 960, 0, 1708, 854, 480, &x, &y));
    assert(x == 0 && y == 480);
    assert(wampy_map_touch(960, 1708, 0, 960, 0, 1708, 854, 480, &x, &y));
    assert(x == 854 && y == 0);
    assert(wampy_map_touch(530, 964, 50, 1010, 110, 1818, 854, 480, &x, &y));
    assert(near(x, 427) && near(y, 240));
    assert(wampy_map_touch(-20, 2000, 0, 960, 0, 1708, 854, 480, &x, &y));
    assert(x == 854 && y == 480);
    assert(!wampy_map_touch(0, 0, 0, 0, 0, 1708, 854, 480, &x, &y));
    assert(!wampy_map_touch_axis(0, 0, 960, 480, nullptr));
    std::puts("WM1A/Z projection, scissor, legacy layout and touch mapping passed");
}
