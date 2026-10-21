#pragma once

// Painting the 3D view's overlay (gfx/Overlay.hpp: label pills, axis numbers, leader lines) with the UI's own
// fonts and shapes, and measuring text for its layout.

#include "gfx/Overlay.hpp"

namespace linalg::ui {

class OverlayMeasure : public gfx::TextMeasure {
public:
    float width(const char* text, gfx::TextStyle style) const override;
    float lineHeight(gfx::TextStyle style) const override;
};

void paintOverlay(const gfx::Overlay& overlay);

} // namespace linalg::ui
