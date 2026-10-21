#pragma once

// Whether the OpenGL context that holds our meshes and shaders still exists, so GPU memory can be freed.
// The raylib app owns its window (IsWindowReady); the GTK app draws in a Gtk::GLArea and says so here
// while that area's context is alive.
namespace linalg {

bool gpuContextReady();
void setExternalGpuContext(bool alive);

} // namespace linalg
