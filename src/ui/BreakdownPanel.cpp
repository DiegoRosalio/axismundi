#include "ui/BreakdownPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg {

float BreakdownPanel::drawContent(Rectangle area) {
    const ui::Theme& t = ui::theme();
    ui::Column col(area.x + ui::px(ui::kPad), area.y + ui::px(12.0f), area.width - 2 * ui::px(ui::kPad));
    const auto& steps = m_player.breakdown().steps;
    int n = static_cast<int>(steps.size());

    float h = ui::textWrapped({col.x(), col.y(), col.width(), 0},
                              "Every matrix is a rotation, a stretch along the axes and another rotation "
                              "(plus a move for 4×4). Play it, or click a step.",
                              ui::FontRole::Small, t.textMuted);
    col.space(h / ui::scale() + 6.0f);

    if (n == 0) {
        ui::textIn(col.row(24), "M is the identity: nothing moves.", ui::FontRole::Body, t.text);
        return col.y() - area.y + ui::px(6.0f);
    }

    // Steps
    for (int i = 0; i < n; i++) {
        Rectangle row = col.row(46.0f);
        float at = m_player.progress();
        bool done = at >= i + 1 - 1e-4f;
        bool current = !done && at > i - 1e-4f && at < i + 1;
        bool hover = ui::hovered(row);
        if (hover && ui::mousePressed(MOUSE_BUTTON_LEFT)) m_player.seek(static_cast<float>(i + 1));
        if (current || hover) ui::fillRounded(row, ui::px(ui::kRadius), current ? t.accentSoft : t.hover);
        if (current) {
            // Progress of the running step as a bar along the bottom edge
            float f = m_player.progress() - i;
            Rectangle bar = {row.x + ui::px(6), row.y + row.height - ui::px(4), (row.width - ui::px(12)) * f,
                             ui::px(3)};
            ui::fillRounded(bar, ui::px(1.5f), t.accent);
        }

        // Number badge
        float r = ui::px(12.0f);
        Vector2 c = {row.x + ui::px(8) + r, row.y + row.height / 2};
        if (done) {
            DrawCircleV(c, r, t.accent);
            ui::drawIcon(ui::Icon::Check, {c.x - r * 0.6f, c.y - r * 0.6f, r * 1.2f, r * 1.2f}, t.onAccent);
        } else {
            DrawCircleV(c, r, current ? t.accent : t.panelAlt);
            if (!current) DrawRing(c, r - ui::pxi(1), r, 0, 360, 32, t.borderStrong);
            char num[8];
            snprintf(num, sizeof(num), "%d", i + 1);
            ui::textIn({c.x - r, c.y - r, 2 * r, 2 * r}, num, ui::FontRole::Small, current ? t.onAccent : t.textMuted,
                       ui::Align::Center);
        }

        float tx = c.x + r + ui::px(10);
        float tw = row.x + row.width - tx - ui::px(6);
        const auto& s = steps[i];
        ui::text(s.title.c_str(), tx, row.y + ui::px(5), ui::FontRole::BodyStrong, t.text);
        std::string d = ui::ellipsize(s.description.c_str(), ui::FontRole::Small, tw);
        ui::text(d.c_str(), tx, row.y + ui::px(25), ui::FontRole::Small, t.textMuted);
        ui::tooltip(row, s.description.c_str(), s.title.c_str());
    }
    col.space(4.0f);

    // Controls
    Rectangle row = col.row(32.0f);
    Rectangle restartBtn = ui::takeLeft(row, 32.0f, 4.0f);
    Rectangle prevBtn = ui::takeLeft(row, 32.0f, 4.0f);
    Rectangle playBtn = ui::takeLeft(row, 32.0f, 4.0f);
    Rectangle nextBtn = ui::takeLeft(row, 32.0f, 12.0f);
    if (ui::iconButton(restartBtn, ui::Icon::Reset)) m_player.restart();
    if (ui::iconButton(prevBtn, ui::Icon::ChevronLeft, false, m_player.progress() > 0)) m_player.previous();
    bool playing = m_player.playing();
    if (ui::iconButton(playBtn, playing ? ui::Icon::Pause : ui::Icon::Play, playing)) m_player.togglePlay();
    if (ui::iconButton(nextBtn, ui::Icon::ChevronRight, false, m_player.progress() < n)) m_player.next();
    ui::tooltip(restartBtn, "Start again from the identity and play", "Restart");
    ui::tooltip(prevBtn, "Back one step", "Previous");
    ui::tooltip(playBtn, "Play or pause", "Play");
    ui::tooltip(nextBtn, "Jump to the end of the next step", "Next");

    float tValue = m_player.progress();
    if (ui::slider(row, tValue, 0.0f, static_cast<float>(n))) m_player.seek(tValue);
    // Step boundaries as ticks under the slider
    for (int i = 1; i < n; i++) {
        float knob = ui::px(7.0f);
        float x = row.x + knob + (row.width - 2 * knob) * i / n;
        DrawRectangleRec({x - ui::px(1), row.y + row.height - ui::px(6), ui::pxi(2), ui::px(5)}, t.borderStrong);
    }
    col.space(4.0f);
    return col.y() - area.y;
}

} // namespace linalg
