#pragma once

#include <raylib.h>
#include <cstddef>

namespace linalg {

// Text sizes used across the UI
constexpr int kUiTextLarge = 28;   // main side panels (matches the original UI)
constexpr int kUiTextSmall = 20;   // info panel, animation bar, HUD

// Parse a numeric field. Accepts plain numbers and math expressions
// ("sqrt(2)/2", "cos(pi/4)", "-1.5e2"). Returns false if invalid or not finite.
bool parseNumber(const char* text, float& out);

// Format with up to 4 decimals, trimming trailing zeros ("1.0", "0.7071", "-2.5").
void formatNumber(char* buf, size_t size, float value);

// raygui text box that shares an "active field" id with its siblings.
// Clicking activates it; Enter or clicking elsewhere deactivates it.
// Returns true on the frame the edit is finished. When `invalid` is set the
// box gets a red outline.
bool textField(Rectangle bounds, char* text, int textSize, int id, int& activeId, bool invalid = false);

// Set raygui's default text size for the widgets that follow.
void setGuiTextSize(int size);

} // namespace linalg
