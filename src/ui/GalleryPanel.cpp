#include "ui/GalleryPanel.hpp"
#include "edu/Examples.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace linalg {

namespace {

Color categoryColor(ExampleCategory c) {
    const ui::Theme& t = ui::theme();
    switch (c) {
        case ExampleCategory::Matrices: return t.accent;
        case ExampleCategory::Complex:  return t.z2;
        case ExampleCategory::Plots:    return t.success;
    }
    return t.accent;
}

} // namespace

float GalleryPanel::drawContent(Rectangle area, int& chosen) {
    const ui::Theme& t = ui::theme();
    float pad = ui::px(ui::kPad);
    ui::Column col(area.x + pad, area.y + ui::px(12.0f), area.width - 2 * pad);

    ui::segmented(col.row(32), {"All", "Matrices", "Complex", "Plots"}, m_category);
    col.space(4.0f);

    // Examples in the selected category
    const auto& list = examples();
    std::vector<int> shown;
    for (int i = 0; i < static_cast<int>(list.size()); i++) {
        if (m_category == 0 || static_cast<int>(list[i].category) == m_category - 1) shown.push_back(i);
    }

    float gap = ui::px(10.0f);
    int columns = std::max(1, static_cast<int>((col.width() + gap) / (ui::px(190.0f) + gap)));
    float cardW = (col.width() - gap * (columns - 1)) / columns;
    float y = col.y();
    float innerPad = ui::px(12.0f);

    for (size_t start = 0; start < shown.size(); start += columns) {
        // Row height: tallest card in the row
        float rowH = 0.0f;
        size_t end = std::min(shown.size(), start + columns);
        for (size_t k = start; k < end; k++) {
            const Example& e = list[shown[k]];
            float textW = cardW - 2 * innerPad;
            float h = ui::px(30.0f) + ui::textWrapped({0, 0, textW, 0}, e.title, ui::FontRole::BodyStrong, t.text, false) +
                      ui::textWrapped({0, 0, textW, 0}, e.description, ui::FontRole::Small, t.text, false) +
                      2 * innerPad;
            rowH = std::max(rowH, h);
        }

        for (size_t k = start; k < end; k++) {
            int index = shown[k];
            const Example& e = list[index];
            Rectangle card = {col.x() + (k - start) * (cardW + gap), y, cardW, rowH};
            bool hover = ui::hovered(card);
            if (hover && ui::mousePressed(MOUSE_BUTTON_LEFT)) chosen = index;

            if (hover) ui::shadow(card, ui::px(ui::kCardRadius), ui::px(10.0f));
            ui::fillRounded(card, ui::px(ui::kCardRadius), hover ? t.hover : t.panelAlt);
            ui::strokeRounded(card, ui::px(ui::kCardRadius), ui::pxi(1.0f), hover ? t.accent : t.border);

            // Category chip
            Color cc = categoryColor(e.category);
            const char* name = categoryName(e.category);
            float chipW = ui::textWidth(name, ui::FontRole::Caption) + ui::px(14.0f);
            Rectangle chip = {card.x + innerPad, card.y + innerPad, chipW, ui::px(20.0f)};
            Color soft = cc;
            soft.a = 40;
            ui::fillRounded(chip, chip.height / 2, soft);
            ui::textIn(chip, name, ui::FontRole::Caption, cc, ui::Align::Center);

            float ty = chip.y + chip.height + ui::px(8.0f);
            float textW = cardW - 2 * innerPad;
            ty += ui::textWrapped({card.x + innerPad, ty, textW, 0}, e.title, ui::FontRole::BodyStrong, t.text);
            ui::textWrapped({card.x + innerPad, ty, textW, 0}, e.description, ui::FontRole::Small, t.textMuted);
        }
        y += rowH + gap;
    }
    return y - area.y + ui::px(6.0f);
}

} // namespace linalg
