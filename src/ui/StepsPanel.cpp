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

    Explanation e;
    if (in.mode == AppMode::Space3D) {
        static const char* topics[] = {"A · B", "A × B", "M · A", "det M", "M⁻¹", "Eigenvalues"};
        y += chips({x, y, w, 0}, topics, 6, m_matrixTopic) + ui::px(14.0f);
        Matrix3D linear;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) linear.m[i][j] = in.matrix.m[i][j];
        switch (m_matrixTopic) {
            case 0: e = explainDot(in.a, in.b); break;
            case 1: e = explainCross(in.a, in.b); break;
            case 2: e = explainMatVec(in.matrix, in.is4x4, in.a); break;
            case 3: e = explainDeterminant(in.matrix, in.is4x4); break;
            case 4: e = explainInverse(in.matrix, in.is4x4); break;
            default: e = explainEigen(linear); break;
        }
        if (in.is4x4 && m_matrixTopic == 5) e.insert(e.begin(), {ExplainLine::Kind::Note,
                                                                 "Using the upper-left 3×3 (linear) part."});
    } else if (in.mode == AppMode::Complex) {
        static const char* topics[] = {"z₁ · z₂", "Polar z₁", "Polar z₂", "Euler"};
        y += chips({x, y, w, 0}, topics, 4, m_complexTopic) + ui::px(14.0f);
        switch (m_complexTopic) {
            case 0: e = explainComplexProduct(in.z1, in.z2); break;
            case 1: e = explainPolar(in.z1, "z₁"); break;
            case 2: e = explainPolar(in.z2, "z₂"); break;
            default: e = explainEuler(in.theta); break;
        }
    } else {
        return y - area.y;
    }

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
