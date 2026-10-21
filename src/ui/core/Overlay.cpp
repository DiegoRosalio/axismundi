#include "ui/core/Overlay.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>

namespace linalg::ui {

namespace {
FontRole roleOf(gfx::TextStyle style) {
    switch (style) {
        case gfx::TextStyle::Label: return FontRole::Label3D;
        case gfx::TextStyle::LabelLarge: return FontRole::Title;
        case gfx::TextStyle::Small: return FontRole::Small;
    }
    return FontRole::Label3D;
}
} // namespace

float OverlayMeasure::width(const char* text, gfx::TextStyle style) const { return textWidth(text, roleOf(style)); }
float OverlayMeasure::lineHeight(gfx::TextStyle style) const { return fontSize(roleOf(style)); }

void paintOverlay(const gfx::Overlay& overlay) {
    for (const gfx::OverlayItem& it : overlay) {
        switch (it.kind) {
            case gfx::OverlayItem::Kind::Line:
                DrawLineEx(it.from, it.to, it.width, it.color);
                break;
            case gfx::OverlayItem::Kind::Text:
                textIn(it.rect, it.text.c_str(), roleOf(it.style), it.color);
                break;
            case gfx::OverlayItem::Kind::Pill: {
                const Rectangle& pill = it.rect;
                fillRounded(pill, pill.height / 2, it.background.a ? it.background : theme().labelBg);
                if (it.ring.a) strokeRounded(pill, pill.height / 2, px(2.0f), it.ring);
                text(it.text.c_str(), pill.x + px(6.0f), pill.y + px(2.0f), roleOf(it.style), it.color);
                break;
            }
        }
    }
}

} // namespace linalg::ui
