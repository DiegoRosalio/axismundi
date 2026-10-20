#pragma once

// The app's icon (drawn by tools/icon.py, embedded as PNG bytes in ui/generated/icon.inc): the window and
// taskbar icon, and the logo in the top bar.

#include <raylib.h>

namespace linalg::ui {

// Gives the window its icon at several sizes (call after InitWindow)
void setWindowIcons();
// The icon drawn into r (a texture made once, smoothed for small sizes)
void drawAppIcon(Rectangle r);

} // namespace linalg::ui
