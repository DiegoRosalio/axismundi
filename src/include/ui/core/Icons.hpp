#pragma once

#include <raylib.h>

namespace linalg::ui {

// Vector icons drawn with raylib primitives, so they stay crisp at any scale.
enum class Icon {
    None,
    Play, Pause, Stop, Loop,
    Undo, Redo,
    Sun, Moon,
    Help, Gear, Fullscreen, Camera,
    Book, Gallery, Steps, Layers,
    Close, ChevronDown, ChevronRight, ChevronLeft,
    Plus, Minus, Check,
    Save, Load, Reset,
    Eye, Grid, Clock, Chart, Formula, Export, Sidebar, Solo,
};

// Draw an icon centered in `r`
void drawIcon(Icon icon, Rectangle r, Color color);

} // namespace linalg::ui
