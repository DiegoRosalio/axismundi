#include "ui/StepsPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>

namespace linalg {

float chips(Rectangle area, const char* const* labels, int count, int& active) {
    const ui::Theme& t = ui::theme();
    float h = ui::px(28.0f), gap = ui::px(6.0f), padX = ui::px(12.0f);
    float x = area.x, y = area.y;
    for (int i = 0; i < count; i++) {
        float w = ui::textWidth(labels[i], ui::FontRole::Small) + 2 * padX;
        if (x + w > area.x + area.width && x > area.x) {
            x = area.x;
            y += h + gap;
        }
        Rectangle r = {x, y, w, h};
        bool selected = (active == i);
        bool hover = ui::hovered(r);
        if (hover && ui::mousePressed(MOUSE_BUTTON_LEFT)) active = i;
        ui::fillRounded(r, h / 2, selected ? t.accent : hover ? t.hover : t.panelAlt);
        if (!selected) ui::strokeRounded(r, h / 2, ui::pxi(1.0f), t.border);
        ui::textIn(r, labels[i], ui::FontRole::Small, selected ? t.onAccent : t.text, ui::Align::Center);
        x += w + gap;
    }
    return y + h - area.y;
}

float StepsPanel::drawContent(Rectangle area, const StepsInput& in) {
    const ui::Theme& t = ui::theme();
    float pad = ui::px(ui::kPad);
    float x = area.x + pad, w = area.width - 2 * pad;
    float y = area.y + ui::px(12.0f);

    const auto& topics = stepTopics(in.complexPlane);
    int& topic = in.complexPlane ? m_complexTopic : m_matrixTopic;
    y += chips({x, y, w, 0}, topics.data(), static_cast<int>(topics.size()), topic) + ui::px(14.0f);
    Explanation e = explainTopic(in, topic);
    bool first = true;
    for (const auto& line : e) {
        switch (line.kind) {
            case ExplainLine::Kind::Heading: {
                if (!first) y += ui::px(10.0f);
                y += ui::textWrapped({x, y, w, 0}, line.text.c_str(), ui::FontRole::BodyStrong, t.text) +
                     ui::px(2.0f);
                break;
            }
            case ExplainLine::Kind::Formula:
                y += ui::textWrapped({x, y, w, 0}, line.text.c_str(), ui::FontRole::Mono, t.accent, true, 1.5f);
                break;
            case ExplainLine::Kind::Step:
                y += ui::textWrapped({x, y, w, 0}, line.text.c_str(), ui::FontRole::Mono, t.text, true, 1.5f);
                break;
            case ExplainLine::Kind::Result: {
                float h = ui::textWrapped({x + ui::px(8), y, w - ui::px(16), 0}, line.text.c_str(),
                                          ui::FontRole::Mono, t.success, false, 1.5f);
                Rectangle bg = {x, y - ui::px(2), w, h + ui::px(4)};
                Color soft = t.success;
                soft.a = 34;
                ui::fillRounded(bg, ui::px(6), soft);
                ui::textWrapped({x + ui::px(8), y + ui::px(1), w - ui::px(16), 0}, line.text.c_str(),
                                ui::FontRole::Mono, t.success, true, 1.5f);
                y += h + ui::px(6.0f);
                break;
            }
            case ExplainLine::Kind::Note:
                y += ui::textWrapped({x, y, w, 0}, line.text.c_str(), ui::FontRole::Small, t.textMuted) +
                     ui::px(2.0f);
                break;
        }
        first = false;
    }
    return y + ui::px(12.0f) - area.y;
}

} // namespace linalg
