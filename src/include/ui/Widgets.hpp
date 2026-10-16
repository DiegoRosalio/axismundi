#pragma once

#include "ui/core/Icons.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>
#include <cstddef>
#include <initializer_list>

namespace linalg {

// Parse a numeric field. Accepts plain numbers and math expressions
// ("sqrt(2)/2", "cos(pi/4)", "-1.5e2"). Returns false if invalid or not finite.
bool parseNumber(const char* text, float& out);

// Format with up to 4 decimals, trimming trailing zeros ("1.0", "0.7071", "-2.5").
void formatNumber(char* buf, size_t size, float value);

namespace ui {

// Shared metrics (design units)
constexpr float kRadius = 6.0f;        // controls
constexpr float kCardRadius = 10.0f;   // panels and cards
constexpr float kRowHeight = 32.0f;    // standard control height
constexpr float kPad = 16.0f;          // panel padding

enum class ButtonKind { Primary, Secondary, Ghost, Danger };

// Push button. `hint` is a small right-aligned shortcut label (e.g. "I").
bool button(Rectangle r, const char* label, ButtonKind kind = ButtonKind::Secondary, bool enabled = true,
            Icon icon = Icon::None, const char* hint = nullptr);

// Square icon button; `active` shows it as selected.
bool iconButton(Rectangle r, Icon icon, bool active = false, bool enabled = true);

// Segmented control. Returns true when the selection changed.
bool segmented(Rectangle r, std::initializer_list<const char*> labels, int& active);

// Label on the left, switch on the right; clicking the row toggles. Returns true on change.
bool toggleRow(Rectangle r, const char* label, bool& value, const char* hint = nullptr);

// Horizontal slider. Returns true while the value changes.
bool slider(Rectangle r, float& value, float minValue, float maxValue);

// Collapsible section header. Returns true when toggled.
bool sectionHeader(Rectangle r, const char* title, bool& expanded, const char* rightText = nullptr);

// Text box (raygui underneath) sharing an "active field" id with its siblings.
// Clicking activates it; Enter or clicking elsewhere finishes. Returns true on
// the frame the edit finishes. `invalid` draws an error outline.
bool textField(Rectangle r, char* text, int textSize, int id, int& activeId, bool invalid = false);

// Card surface: shadow, rounded fill and hairline border
void card(Rectangle r, bool withShadow = true);

// Small key label such as "Ctrl+Z" drawn as a keycap; returns its width
float keycap(float x, float y, const char* label);

// Color swatch button
bool swatch(Rectangle r, Color color, bool selected = false);

} // namespace ui
} // namespace linalg
