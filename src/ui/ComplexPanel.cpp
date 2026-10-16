#include "ui/ComplexPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <cmath>
#include <cstdio>
#include <string>

namespace linalg {

namespace {

float degrees(float radians) { return radians * 180.0f / PI; }

std::string rect(const Complex& z) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %c %.2fi", z.re, z.im < 0 ? '-' : '+', std::fabs(z.im));
    return buf;
}

std::string polar(const Complex& z) {
    char buf[64];
    snprintf(buf, sizeof(buf), "|z| %.2f  arg %.1f°", z.abs(), degrees(z.arg()));
    return buf;
}

} // namespace

ComplexPanel::ComplexPanel() : m_activeField(-1), m_animateTheta(false) {
    syncText();
}

float& ComplexPanel::fieldValue(int id) {
    switch (id) {
        case 0: return m_scene.z1.re;
        case 1: return m_scene.z1.im;
        case 2: return m_scene.z2.re;
        default: return m_scene.z2.im;
    }
}

void ComplexPanel::syncText() {
    for (int i = 0; i < kFieldCount; i++) formatNumber(m_text[i], kTextSize, fieldValue(i));
}

void ComplexPanel::finishField(int id) {
    float v;
    if (parseNumber(m_text[id], v)) fieldValue(id) = v;
    formatNumber(m_text[id], kTextSize, fieldValue(id));
}

void ComplexPanel::setScene(const ComplexScene& scene) {
    m_scene = scene;
    m_activeField = -1;
    syncText();
}

void ComplexPanel::finishEditing() {
    if (m_activeField < 0) return;
    int id = m_activeField;
    m_activeField = -1;
    finishField(id);
}

void ComplexPanel::cancelEditing() {
    if (m_activeField < 0) return;
    formatNumber(m_text[m_activeField], kTextSize, fieldValue(m_activeField));
    m_activeField = -1;
}

void ComplexPanel::update(float deltaTime) {
    if (m_activeField >= 0 && IsKeyPressed(KEY_TAB)) {
        int current = m_activeField;
        bool back = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        finishField(current);
        m_activeField = (current + (back ? kFieldCount - 1 : 1)) % kFieldCount;
    }

    if (m_animateTheta && m_scene.showEuler) {
        m_scene.theta += deltaTime * 0.8f;
        if (m_scene.theta > 2.0f * PI) m_scene.theta -= 2.0f * PI;
    }
}

float ComplexPanel::draw(float x, float y, float width) {
    const ui::Theme& t = ui::theme();
    int previous = m_activeField;
    ui::Column col(x, y, width);
    char buf[128];

    // --- Numbers -----------------------------------------------------------
    ui::sectionHeader(col.row(24), "Complex numbers", m_showNumbers);
    if (m_showNumbers) {
        const char* names[2] = {"z₁", "z₂"};
        const Color colors[2] = {t.z1, t.z2};
        for (int row = 0; row < 2; row++) {
            Rectangle r = col.row(36.0f);
            Rectangle label = ui::takeLeft(r, 34.0f, 6.0f);
            float dot = ui::px(8.0f);
            DrawCircleV({label.x + dot / 2 + ui::px(2), label.y + label.height / 2}, dot / 2, colors[row]);
            ui::textIn({label.x + dot + ui::px(6), label.y, label.width, label.height}, names[row],
                       ui::FontRole::BodyStrong, t.text);
            Rectangle iLabel = ui::takeRight(r, 12.0f, 4.0f);
            auto cells = ui::splitRow(r, {1, 1}, 22.0f);
            for (int part = 0; part < 2; part++) {
                int id = row * 2 + part;
                float dummy;
                bool invalid = !parseNumber(m_text[id], dummy);
                if (ui::textField(cells[part], m_text[id], kTextSize, id, m_activeField, invalid)) finishField(id);
                if (m_activeField != id) {
                    ui::tooltip(cells[part], part == 0 ? "Real part: position along the Re (horizontal) axis."
                                                       : "Imaginary part: position along the Im (vertical) axis.");
                }
            }
            ui::textIn({cells[0].x + cells[0].width, r.y, ui::px(22.0f), r.height}, "+", ui::FontRole::Body,
                       t.textMuted, ui::Align::Center);
            ui::textIn(iLabel, "i", ui::FontRole::Body, t.textMuted, ui::Align::Left);
        }

        const Complex& z1 = m_scene.z1;
        const Complex& z2 = m_scene.z2;
        Complex p = z1 * z2;

        // Results block
        col.space(2.0f);
        float lineH = ui::px(21.0f);
        float blockH = 6 * lineH + ui::px(16.0f);
        Rectangle block = col.rowPx(blockH);
        ui::fillRounded(block, ui::px(ui::kRadius), t.panelAlt);
        Rectangle inner = ui::inset(block, 8.0f);
        struct Line { std::string a, b; Color color; };
        const Line lines[3] = {
            {"z₁ = " + rect(z1), polar(z1), t.z1},
            {"z₂ = " + rect(z2), polar(z2), t.z2},
            {"z₁z₂ = " + rect(p), polar(p), t.zProduct},
        };
        float ly = inner.y;
        for (const auto& l : lines) {
            DrawCircleV({inner.x + ui::px(3), ly + lineH / 2}, ui::px(3), l.color);
            ui::text(l.a.c_str(), inner.x + ui::px(12), ly + ui::px(2), ui::FontRole::Mono, t.text);
            ly += lineH;
            ui::text(l.b.c_str(), inner.x + ui::px(12), ly + ui::px(2), ui::FontRole::Mono, t.textMuted);
            ly += lineH;
        }

        col.space(2.0f);
        ui::textIn(col.row(20), "Multiplying rotates and scales:", ui::FontRole::Small, t.textMuted);
        snprintf(buf, sizeof(buf), "|z₁z₂| = %.2f × %.2f = %.2f", z1.abs(), z2.abs(), p.abs());
        ui::textIn(col.row(20), buf, ui::FontRole::Body, t.text);
        snprintf(buf, sizeof(buf), "arg = %.1f° + %.1f° = %.1f°", degrees(z1.arg()), degrees(z2.arg()),
                 degrees(z1.arg() + z2.arg()));
        ui::textIn(col.row(20), buf, ui::FontRole::Body, t.text);
        col.space(2.0f);
        Rectangle prod = col.row(30);
        ui::toggleRow(prod, "Show product and angles", m_scene.showProduct);
        ui::tooltip(prod,
                    "The faint arrow is z₁ turned by arg z₂; the dashed line stretches it by |z₂| to reach z₁z₂. "
                    "The two arcs show the angles adding up.");
        col.space(10.0f);
    }

    // Focus moved straight to another field: validate the old one too
    if (previous >= 0 && m_activeField != previous) finishField(previous);

    // --- Euler's formula -------------------------------------------------
    ui::sectionHeader(col.row(24), "Euler's formula", m_showEuler);
    if (m_showEuler) {
        ui::toggleRow(col.row(30), "Show e^(iθ)", m_scene.showEuler);
        ui::toggleRow(col.row(30), "Animate θ", m_animateTheta);

        Rectangle r = col.row(30);
        Rectangle label = ui::takeLeft(r, 20.0f, 8.0f);
        Rectangle value = ui::takeRight(r, 64.0f, 8.0f);
        ui::textIn(label, "θ", ui::FontRole::BodyStrong, t.text);
        ui::slider(r, m_scene.theta, 0.0f, 2.0f * PI);
        ui::tooltip(r, "Drag to move e^(iθ) around the unit circle. Its shadows on the axes are cos θ and sin θ.");
        snprintf(buf, sizeof(buf), "%.2f rad", m_scene.theta);
        ui::textIn(value, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);

        float c = std::cos(m_scene.theta), sn = std::sin(m_scene.theta);
        float lineH = ui::px(21.0f);
        Rectangle block = col.rowPx(3 * lineH + ui::px(16.0f));
        ui::fillRounded(block, ui::px(ui::kRadius), t.panelAlt);
        Rectangle inner = ui::inset(block, 8.0f);
        ui::text("e^(iθ) = cos θ + i sin θ", inner.x, inner.y + ui::px(2), ui::FontRole::Mono, t.euler);
        snprintf(buf, sizeof(buf), "       = %.3f %c %.3fi", c, sn < 0 ? '-' : '+', std::fabs(sn));
        ui::text(buf, inner.x, inner.y + lineH + ui::px(2), ui::FontRole::Mono, t.text);
        snprintf(buf, sizeof(buf), "θ = %.1f°", degrees(m_scene.theta));
        ui::text(buf, inner.x, inner.y + 2 * lineH + ui::px(2), ui::FontRole::Mono, t.textMuted);

        col.space(2.0f);
        float h = ui::textWrapped({col.x(), col.y(), col.width(), 0},
                                  "θ = π gives e^(iπ) = −1. The helix shows e^(it) with t rising along Z: "
                                  "seen from the front it is the unit circle.",
                                  ui::FontRole::Small, t.textMuted);
        col.space(h / ui::scale());
        col.space(10.0f);
    }
    return col.y();
}

void ComplexPanel::saveState(SceneData& data) const {
    putFloat(data, "complex.z1.re", m_scene.z1.re);
    putFloat(data, "complex.z1.im", m_scene.z1.im);
    putFloat(data, "complex.z2.re", m_scene.z2.re);
    putFloat(data, "complex.z2.im", m_scene.z2.im);
    putFloat(data, "complex.theta", m_scene.theta);
    putBool(data, "complex.show_product", m_scene.showProduct);
    putBool(data, "complex.show_euler", m_scene.showEuler);
    putBool(data, "complex.animate", m_animateTheta);
}

void ComplexPanel::loadState(const SceneData& data) {
    getFloat(data, "complex.z1.re", m_scene.z1.re);
    getFloat(data, "complex.z1.im", m_scene.z1.im);
    getFloat(data, "complex.z2.re", m_scene.z2.re);
    getFloat(data, "complex.z2.im", m_scene.z2.im);
    if (getFloat(data, "complex.theta", m_scene.theta)) {
        m_scene.theta = std::fmod(std::fabs(m_scene.theta), 2.0f * PI);
    }
    getBool(data, "complex.show_product", m_scene.showProduct);
    getBool(data, "complex.show_euler", m_scene.showEuler);
    getBool(data, "complex.animate", m_animateTheta);
    m_activeField = -1;
    syncText();
}

} // namespace linalg
