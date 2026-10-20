#include "gfx/Canvas3D.hpp"
#include "gfx/RaylibCanvas.hpp"

namespace linalg::gfx {

namespace {
Canvas3D* g_canvas = nullptr;
}

Canvas3D& canvas() {
    static RaylibCanvas raylibCanvas;
    return g_canvas ? *g_canvas : raylibCanvas;
}

void setCanvas(Canvas3D* c) { g_canvas = c; }

} // namespace linalg::gfx
