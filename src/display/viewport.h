#ifndef WAMPY_DISPLAY_VIEWPORT_H
#define WAMPY_DISPLAY_VIEWPORT_H

#include <algorithm>
#include <array>
#include <cmath>

namespace WampyDisplay {

struct Point { double x, y; };
struct Rect { double left, top, right, bottom; };
struct Scissor { int x, y, width, height; };

// Skins retain their original 800 x 480 coordinates. Rendering, clipping and
// pointer input must all use this transform, including popup windows.
class Viewport {
  public:
    static constexpr int CanvasWidth = 800;
    static constexpr int CanvasHeight = 480;

    void ConfigurePanel(int width, int height, bool fillWM1AZ) {
        if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
            width = CanvasWidth;
            height = CanvasHeight;
        }
        longSide_ = std::max(width, height);
        shortSide_ = std::min(width, height);
        wm1az_ = longSide_ == 854 && shortSide_ == 480;
        fill_ = wm1az_ && fillWM1AZ;
        surfaceWidth_ = surfaceHeight_ = longSide_;
    }

    void SetSurface(int width, int height) {
        if (width > 0 && height > 0) {
            surfaceWidth_ = width;
            surfaceHeight_ = height;
        }
    }

    bool IsWM1AZ() const { return wm1az_; }
    bool FillsScreen() const { return fill_; }
    int LongSide() const { return longSide_; }
    int ShortSide() const { return shortSide_; }
    int SurfaceWidth() const { return surfaceWidth_; }
    int SurfaceHeight() const { return surfaceHeight_; }
    int LogicalWidth() const { return fill_ ? CanvasWidth : longSide_; }
    int LogicalHeight() const { return CanvasHeight; }
    double ScaleX() const { return fill_ ? double(longSide_) / CanvasWidth : 1.0; }
    double ScaleY() const { return fill_ ? double(shortSide_) / CanvasHeight : 1.0; }

    // GLFW reports display coordinates already rotated into landscape space.
    // Do not clamp to the canvas: a tap in a legacy margin must remain outside.
    Point ToLogical(Point physical) const {
        return {physical.x / ScaleX(), physical.y / ScaleY()};
    }

    // GL's origin is at the bottom left; the Walkman panel is portrait.
    Point ToGL(Point logical) const {
        return {shortSide_ - logical.y * ScaleY(), longSide_ - logical.x * ScaleX()};
    }

    // Column-major OpenGL projection, used by the renderer for every draw list.
    std::array<float, 16> Projection() const {
        std::array<float, 16> matrix{};
        matrix[1] = float(-2.0 * ScaleX() / surfaceHeight_);
        matrix[4] = float(-2.0 * ScaleY() / surfaceWidth_);
        matrix[10] = -1.0f;
        matrix[12] = float(2.0 * shortSide_ / surfaceWidth_ - 1.0);
        matrix[13] = float(2.0 * longSide_ / surfaceHeight_ - 1.0);
        matrix[15] = 1.0f;
        return matrix;
    }

    Scissor Clip(Rect rect) const {
        if (!std::isfinite(rect.left) || !std::isfinite(rect.top) ||
            !std::isfinite(rect.right) || !std::isfinite(rect.bottom) ||
            rect.right <= rect.left || rect.bottom <= rect.top) return {0, 0, 0, 0};
        const auto minimum = ToGL({rect.right, rect.bottom});
        const auto maximum = ToGL({rect.left, rect.top});
        const double width = std::min(shortSide_, surfaceWidth_);
        const double height = std::min(longSide_, surfaceHeight_);
        const int x0 = int(std::floor(std::max(0.0, std::min(width, minimum.x))));
        const int y0 = int(std::floor(std::max(0.0, std::min(height, minimum.y))));
        const int x1 = int(std::ceil(std::max(0.0, std::min(width, maximum.x))));
        const int y1 = int(std::ceil(std::max(0.0, std::min(height, maximum.y))));
        return {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
    }

  private:
    int longSide_ = CanvasWidth, shortSide_ = CanvasHeight;
    int surfaceWidth_ = CanvasWidth, surfaceHeight_ = CanvasWidth;
    bool wm1az_ = false, fill_ = false;
};

// Defined by main.cpp; shared with the patched ImGui renderer/input backend.
extern Viewport viewport;

} // namespace WampyDisplay
#endif
