#include "ui/Widgets.hpp"
#include "math/Expression.hpp"
#include <raygui.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace linalg {

bool parseNumber(const char* text, float& out) {
    Expression expr;
    if (!expr.parse(text)) return false;
    double v = expr.eval(0.0);
    if (!std::isfinite(v)) return false;
    out = static_cast<float>(v);
    return true;
}

void formatNumber(char* buf, size_t size, float value) {
    if (std::fabs(value) < 5e-5f) value = 0.0f;  // avoid "-0.0"
    snprintf(buf, size, "%.4f", value);

    // Trim trailing zeros, keeping one digit after the decimal point
    char* dot = std::strchr(buf, '.');
    if (!dot) return;
    char* end = buf + std::strlen(buf) - 1;
    while (end > dot + 1 && *end == '0') *end-- = '\0';
}

bool textField(Rectangle bounds, char* text, int textSize, int id, int& activeId, bool invalid) {
    bool editing = (activeId == id);
    bool finished = false;

    if (GuiTextBox(bounds, text, textSize, editing)) {
        if (editing) {
            activeId = -1;
            finished = true;
        } else {
            activeId = id;
        }
    }

    if (invalid) {
        DrawRectangleLinesEx(bounds, 3.0f, RED);
    }
    return finished;
}

void setGuiTextSize(int size) {
    GuiSetStyle(DEFAULT, TEXT_SIZE, size);
    GuiSetStyle(DEFAULT, TEXT_SPACING, size >= 24 ? 2 : 1);
}

} // namespace linalg
