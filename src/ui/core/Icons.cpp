#include "ui/core/Icons.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <cmath>

namespace linalg::ui {

namespace {

// Icons are designed on a 16x16 grid and mapped into the target square
struct Canvas {
    float x, y, s;  // origin and size of one grid unit
    float stroke;
    Color color;

    Vector2 p(float gx, float gy) const { return {x + gx * s, y + gy * s}; }
    void seg(float x0, float y0, float x1, float y1) const {
        DrawLineEx(p(x0, y0), p(x1, y1), stroke, color);
        // Round caps
        DrawCircleV(p(x0, y0), stroke / 2, color);
        DrawCircleV(p(x1, y1), stroke / 2, color);
    }
    void poly(std::initializer_list<Vector2> pts) const {
        const Vector2* prev = nullptr;
        for (const auto& q : pts) {
            if (prev) seg(prev->x, prev->y, q.x, q.y);
            prev = &q;
        }
    }
    void tri(float x0, float y0, float x1, float y1, float x2, float y2) const {
        // raylib wants counter-clockwise order; draw both to be safe
        DrawTriangle(p(x0, y0), p(x1, y1), p(x2, y2), color);
        DrawTriangle(p(x0, y0), p(x2, y2), p(x1, y1), color);
    }
    void box(float x0, float y0, float w, float h) const {
        DrawRectangleV(p(x0, y0), {w * s, h * s}, color);
    }
    void ring(float cx, float cy, float r, float a0, float a1) const {
        float inner = r * s - stroke / 2;
        float outer = r * s + stroke / 2;
        DrawRing(p(cx, cy), inner, outer, a0, a1, 32, color);
    }
    void dot(float cx, float cy, float r) const { DrawCircleV(p(cx, cy), r * s, color); }
    void arrowHead(float tipX, float tipY, float dirX, float dirY, float size) const {
        float len = std::sqrt(dirX * dirX + dirY * dirY);
        dirX /= len;
        dirY /= len;
        float nx = -dirY, ny = dirX;
        tri(tipX, tipY, tipX - dirX * size + nx * size * 0.75f, tipY - dirY * size + ny * size * 0.75f,
            tipX - dirX * size - nx * size * 0.75f, tipY - dirY * size - ny * size * 0.75f);
    }
};

} // namespace

void drawIcon(Icon icon, Rectangle r, Color color) {
    float size = std::min(r.width, r.height);
    float s = size / 16.0f;
    Canvas c{r.x + (r.width - size) / 2.0f, r.y + (r.height - size) / 2.0f, s,
             std::max(1.2f, 1.6f * s), color};

    switch (icon) {
        case Icon::None:
            break;
        case Icon::Play:
            c.tri(4.5f, 2.5f, 4.5f, 13.5f, 13.5f, 8.0f);
            break;
        case Icon::Pause:
            c.box(4.0f, 3.0f, 2.8f, 10.0f);
            c.box(9.2f, 3.0f, 2.8f, 10.0f);
            break;
        case Icon::Stop:
            c.box(3.5f, 3.5f, 9.0f, 9.0f);
            break;
        case Icon::Loop:
            c.ring(8, 8, 5.0f, 200.0f, 470.0f);
            c.arrowHead(3.6f, 6.2f, -0.3f, 1.0f, 2.6f);
            break;
        case Icon::Undo:
            c.ring(9, 9, 4.5f, 180.0f, 450.0f);
            c.arrowHead(2.5f, 9.6f, 0.0f, 1.0f, 2.6f);
            break;
        case Icon::Redo:
            c.ring(7, 9, 4.5f, 90.0f, 360.0f);
            c.arrowHead(13.5f, 9.6f, 0.0f, 1.0f, 2.6f);
            break;
        case Icon::Sun:
            c.dot(8, 8, 3.0f);
            for (int i = 0; i < 8; i++) {
                float a = i * PI / 4.0f;
                c.seg(8 + std::cos(a) * 5.0f, 8 + std::sin(a) * 5.0f, 8 + std::cos(a) * 6.8f, 8 + std::sin(a) * 6.8f);
            }
            break;
        case Icon::Moon: {
            // Crescent = outer disc minus an offset disc, filled with one-pixel scanlines
            const float ocx = 8.0f, ocy = 8.0f, orad = 6.0f;
            const float icx = 10.6f, icy = 5.8f, irad = 5.0f;
            float step = 1.0f / s;  // one pixel in grid units
            for (float y = ocy - orad; y <= ocy + orad; y += step) {
                float dy = y - ocy;
                float half = std::sqrt(std::max(0.0f, orad * orad - dy * dy));
                float x0 = ocx - half, x1 = ocx + half;
                float idy = y - icy;
                if (std::fabs(idy) < irad) {
                    float ihalf = std::sqrt(irad * irad - idy * idy);
                    x1 = std::min(x1, icx - ihalf);
                }
                if (x1 > x0) DrawRectangleV(c.p(x0, y), {(x1 - x0) * s, std::max(1.0f, step * s)}, color);
            }
            break;
        }
        case Icon::Help:
            c.ring(8, 6.2f, 3.2f, 180.0f, 400.0f);
            c.seg(10.4f, 8.4f, 8.0f, 10.0f);
            c.seg(8.0f, 10.0f, 8.0f, 10.6f);
            c.dot(8.0f, 13.4f, 1.0f);
            break;
        case Icon::Gear:
            c.ring(8, 8, 3.6f, 0.0f, 360.0f);
            for (int i = 0; i < 8; i++) {
                float a = i * PI / 4.0f;
                c.seg(8 + std::cos(a) * 4.8f, 8 + std::sin(a) * 4.8f, 8 + std::cos(a) * 6.6f, 8 + std::sin(a) * 6.6f);
            }
            break;
        case Icon::Fullscreen:
            c.poly({{2.5f, 6.0f}, {2.5f, 2.5f}, {6.0f, 2.5f}});
            c.poly({{10.0f, 2.5f}, {13.5f, 2.5f}, {13.5f, 6.0f}});
            c.poly({{13.5f, 10.0f}, {13.5f, 13.5f}, {10.0f, 13.5f}});
            c.poly({{6.0f, 13.5f}, {2.5f, 13.5f}, {2.5f, 10.0f}});
            break;
        case Icon::Camera:
            c.poly({{2.0f, 5.0f}, {5.0f, 5.0f}, {6.5f, 3.0f}, {9.5f, 3.0f}, {11.0f, 5.0f}, {14.0f, 5.0f},
                    {14.0f, 13.0f}, {2.0f, 13.0f}, {2.0f, 5.0f}});
            c.ring(8, 9, 2.4f, 0.0f, 360.0f);
            break;
        case Icon::Book:
            c.poly({{8.0f, 4.0f}, {5.0f, 2.8f}, {1.8f, 3.2f}, {1.8f, 13.0f}, {5.0f, 12.6f}, {8.0f, 13.8f}});
            c.poly({{8.0f, 4.0f}, {11.0f, 2.8f}, {14.2f, 3.2f}, {14.2f, 13.0f}, {11.0f, 12.6f}, {8.0f, 13.8f}});
            c.seg(8.0f, 4.0f, 8.0f, 13.8f);
            break;
        case Icon::Gallery:
            c.box(2.0f, 2.0f, 5.2f, 5.2f);
            c.box(8.8f, 2.0f, 5.2f, 5.2f);
            c.box(2.0f, 8.8f, 5.2f, 5.2f);
            c.box(8.8f, 8.8f, 5.2f, 5.2f);
            break;
        case Icon::Steps:
            c.dot(3.2f, 4.0f, 1.2f);
            c.dot(3.2f, 8.0f, 1.2f);
            c.dot(3.2f, 12.0f, 1.2f);
            c.seg(6.5f, 4.0f, 13.5f, 4.0f);
            c.seg(6.5f, 8.0f, 13.5f, 8.0f);
            c.seg(6.5f, 12.0f, 11.0f, 12.0f);
            break;
        case Icon::Layers:
            c.poly({{8.0f, 2.5f}, {14.0f, 5.8f}, {8.0f, 9.0f}, {2.0f, 5.8f}, {8.0f, 2.5f}});
            c.poly({{2.0f, 9.0f}, {8.0f, 12.3f}, {14.0f, 9.0f}});
            break;
        case Icon::Close:
            c.seg(4.0f, 4.0f, 12.0f, 12.0f);
            c.seg(12.0f, 4.0f, 4.0f, 12.0f);
            break;
        case Icon::ChevronDown:
            c.poly({{4.0f, 6.0f}, {8.0f, 10.0f}, {12.0f, 6.0f}});
            break;
        case Icon::ChevronRight:
            c.poly({{6.0f, 4.0f}, {10.0f, 8.0f}, {6.0f, 12.0f}});
            break;
        case Icon::ChevronLeft:
            c.poly({{10.0f, 4.0f}, {6.0f, 8.0f}, {10.0f, 12.0f}});
            break;
        case Icon::Plus:
            c.seg(8.0f, 3.0f, 8.0f, 13.0f);
            c.seg(3.0f, 8.0f, 13.0f, 8.0f);
            break;
        case Icon::Minus:
            c.seg(3.0f, 8.0f, 13.0f, 8.0f);
            break;
        case Icon::Check:
            c.poly({{3.0f, 8.5f}, {6.5f, 12.0f}, {13.0f, 4.5f}});
            break;
        case Icon::Save:
            c.seg(8.0f, 2.0f, 8.0f, 10.0f);
            c.poly({{4.5f, 6.5f}, {8.0f, 10.0f}, {11.5f, 6.5f}});
            c.poly({{2.5f, 11.0f}, {2.5f, 14.0f}, {13.5f, 14.0f}, {13.5f, 11.0f}});
            break;
        case Icon::Load:
            c.seg(8.0f, 10.0f, 8.0f, 2.0f);
            c.poly({{4.5f, 5.5f}, {8.0f, 2.0f}, {11.5f, 5.5f}});
            c.poly({{2.5f, 11.0f}, {2.5f, 14.0f}, {13.5f, 14.0f}, {13.5f, 11.0f}});
            break;
        case Icon::Reset:
            c.ring(8, 8, 5.0f, 230.0f, 500.0f);
            c.arrowHead(4.2f, 4.0f, -1.0f, -0.4f, 2.6f);
            break;
        case Icon::Eye:
            c.ring(8, 13.0f, 8.0f, 218.0f, 322.0f);
            c.ring(8, 3.0f, 8.0f, 38.0f, 142.0f);
            c.dot(8, 8, 2.2f);
            break;
        case Icon::Clock:
            c.ring(8, 8, 6.0f, 0.0f, 360.0f);
            c.seg(8.0f, 8.0f, 8.0f, 4.5f);
            c.seg(8.0f, 8.0f, 10.8f, 9.6f);
            break;
        case Icon::Chart:
            c.seg(2.5f, 13.5f, 13.5f, 13.5f);
            c.seg(4.8f, 13.5f, 4.8f, 9.0f);
            c.seg(8.0f, 13.5f, 8.0f, 4.0f);
            c.seg(11.2f, 13.5f, 11.2f, 7.0f);
            break;
        case Icon::Formula:  // a sigma: the formula reference
            c.poly({{12.5f, 3.0f}, {3.5f, 3.0f}, {8.5f, 8.0f}, {3.5f, 13.0f}, {12.5f, 13.0f}});
            break;
        case Icon::Grid:
            for (int i = 0; i < 4; i++) {
                float t = 2.5f + i * 3.67f;
                c.seg(t, 2.5f, t, 13.5f);
                c.seg(2.5f, t, 13.5f, t);
            }
            break;
    }
}

} // namespace linalg::ui
