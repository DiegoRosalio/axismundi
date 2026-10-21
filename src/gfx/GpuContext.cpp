#include "gfx/GpuContext.hpp"
#include <raylib.h>

namespace linalg {

namespace {
bool g_external = false;
}

bool gpuContextReady() { return g_external || IsWindowReady(); }
void setExternalGpuContext(bool alive) { g_external = alive; }

} // namespace linalg
