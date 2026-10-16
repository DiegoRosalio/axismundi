#include "ui/BreakdownPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg {

namespace {

constexpr float kStepsPerSecond = 0.6f;
constexpr float kHoldSeconds = 0.6f;

bool sameMatrix(const Matrix4D& a, const Matrix4D& b) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (std::fabs(a.m[i][j] - b.m[i][j]) > 1e-6f) return false;
    return true;
}

float smooth(float f) { return f * f * (3.0f - 2.0f * f); }

} // namespace

void BreakdownPanel::setMatrix(const Matrix4D& m) {
    if (m_hasMatrix && sameMatrix(m, m_matrix)) return;
    m_matrix = m;
    m_hasMatrix = true;
    m_breakdown = linalg::breakdown(m);
    m_t = std::min(m_t, static_cast<float>(m_breakdown.steps.size()));
}

void BreakdownPanel::restart() {
    m_t = 0.0f;
    m_hold = 0.0f;
    m_playing = true;
}

int BreakdownPanel::completedSteps() const {
    return static_cast<int>(std::floor(m_t + 1e-4f));
}

Matrix4D BreakdownPanel::current() const {
    float k = std::floor(m_t);
    return m_breakdown.at(k + smooth(m_t - k));
}

void BreakdownPanel::update(float dt) {
    float n = static_cast<float>(m_breakdown.steps.size());
    if (!m_playing) return;
    if (m_t >= n) {
        m_playing = false;
        return;
    }
    if (m_hold > 0.0f) {
        m_hold -= dt;
        return;
    }
    float before = m_t;
    m_t = std::min(n, m_t + dt * kStepsPerSecond);
    // Pause briefly whenever a step finishes
    if (std::floor(m_t) > std::floor(before) && m_t < n) {
        m_t = std::floor(m_t);
        m_hold = kHoldSeconds;
    }
}

float BreakdownPanel::drawContent(Rectangle area) {
    const ui::Theme& t = ui::theme();
    ui::Column col(area.x + ui::px(ui::kPad), area.y + ui::px(12.0f), area.width - 2 * ui::px(ui::kPad));
    const auto& steps = m_breakdown.steps;
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
        bool done = m_t >= i + 1 - 1e-4f;
        bool current = !done && m_t > i - 1e-4f && m_t < i + 1;
        bool hover = ui::hovered(row);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            m_t = static_cast<float>(i + 1);
            m_playing = false;
        }
        if (current || hover) ui::fillRounded(row, ui::px(ui::kRadius), current ? t.accentSoft : t.hover);
        if (current) {
            // Progress of the running step as a bar along the bottom edge
            float f = m_t - i;
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
    if (ui::iconButton(restartBtn, ui::Icon::Reset)) restart();
    if (ui::iconButton(prevBtn, ui::Icon::ChevronLeft, false, m_t > 0)) {
        m_t = std::max(0.0f, std::ceil(m_t - 1.0f - 1e-4f));
        m_playing = false;
    }
    if (ui::iconButton(playBtn, m_playing ? ui::Icon::Pause : ui::Icon::Play, m_playing)) {
        if (m_playing) m_playing = false;
        else if (m_t >= n) restart();
        else m_playing = true;
    }
    if (ui::iconButton(nextBtn, ui::Icon::ChevronRight, false, m_t < n)) {
        m_t = std::min(static_cast<float>(n), std::floor(m_t + 1e-4f) + 1.0f);
        m_playing = false;
    }
    ui::tooltip(restartBtn, "Start again from the identity and play", "Restart");
    ui::tooltip(prevBtn, "Back one step", "Previous");
    ui::tooltip(playBtn, "Play or pause", "Play");
    ui::tooltip(nextBtn, "Jump to the end of the next step", "Next");

    float tValue = m_t;
    if (ui::slider(row, tValue, 0.0f, static_cast<float>(n))) {
        m_t = tValue;
        m_playing = false;
    }
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
