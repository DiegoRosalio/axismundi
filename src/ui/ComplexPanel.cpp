#include "ui/ComplexPanel.hpp"
#include "ui/Widgets.hpp"
#include <raygui.h>
#include <cmath>
#include <cstdio>
#include <string>

namespace linalg {

namespace {

void drawLine(const std::string& text, float x, float& y, Color color = {40, 40, 40, 255}) {
    DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y), kUiTextSmall, color);
    y += 26.0f;
}

float degrees(float radians) { return radians * 180.0f / PI; }

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
    setGuiTextSize(kUiTextLarge);
    const float pad = 15.0f;
    float inner = width - 2 * pad;
    float cx = x + pad;
    int previous = m_activeField;

    // --- Complex numbers ---------------------------------------------------
    float top = y;
    float cy = y + 30.0f;
    float labelW = 50.0f;
    float boxW = (inner - labelW - 60.0f) / 2.0f;

    const char* names[2] = {"z1", "z2"};
    for (int row = 0; row < 2; row++) {
        GuiLabel({cx, cy + 8, labelW, 32}, names[row]);
        for (int part = 0; part < 2; part++) {
            int id = row * 2 + part;
            float bx = cx + labelW + part * (boxW + 30.0f);
            float dummy;
            bool invalid = !parseNumber(m_text[id], dummy);
            if (textField({bx, cy, boxW, 48}, m_text[id], kTextSize, id, m_activeField, invalid)) {
                finishField(id);
            }
        }
        GuiLabel({cx + labelW + boxW + 6, cy + 8, 24, 32}, "+");
        GuiLabel({cx + labelW + 2 * boxW + 36, cy + 8, 24, 32}, "i");
        cy += 58.0f;
    }

    const Complex& z1 = m_scene.z1;
    const Complex& z2 = m_scene.z2;
    Complex p = z1 * z2;

    cy += 4.0f;
    drawLine("z1 = " + z1.toRectString(), cx, cy);
    drawLine("   = " + z1.toPolarString(), cx, cy, GRAY);
    drawLine("z2 = " + z2.toRectString(), cx, cy);
    drawLine("   = " + z2.toPolarString(), cx, cy, GRAY);
    drawLine("z1 z2 = " + p.toRectString(), cx, cy, ColorBrightness(ORANGE, -0.3f));
    drawLine("      = " + p.toPolarString(), cx, cy, GRAY);
    cy += 6.0f;

    char buf[128];
    drawLine("Multiplying = rotate + scale:", cx, cy, DARKBLUE);
    snprintf(buf, sizeof(buf), "|z1 z2| = %.2f x %.2f = %.2f", z1.abs(), z2.abs(), p.abs());
    drawLine(buf, cx, cy);
    snprintf(buf, sizeof(buf), "arg: %.1f + %.1f = %.1f deg", degrees(z1.arg()), degrees(z2.arg()),
             degrees(z1.arg() + z2.arg()));
    drawLine(buf, cx, cy);
    cy += 8.0f;

    GuiCheckBox({cx, cy, 28, 28}, "Show z1 z2 and angles", &m_scene.showProduct);
    cy += 45.0f;

    GuiGroupBox({x, top, width, cy - top}, "Complex Numbers");
    y = cy + 10.0f;

    // Focus moved straight to another field: validate the old one too
    if (previous >= 0 && m_activeField != previous) finishField(previous);

    // --- Euler's formula -------------------------------------------------
    top = y;
    cy = y + 30.0f;
    GuiCheckBox({cx, cy, 28, 28}, "Show", &m_scene.showEuler);
    GuiCheckBox({cx + inner / 2, cy, 28, 28}, "Animate", &m_animateTheta);
    cy += 45.0f;

    snprintf(buf, sizeof(buf), "%.2f", m_scene.theta);
    GuiLabel({cx, cy, 40, 32}, "t");
    GuiSlider({cx + 40, cy, inner - 120, 32}, nullptr, buf, &m_scene.theta, 0.0f, 2.0f * PI);
    cy += 45.0f;

    float c = std::cos(m_scene.theta), s = std::sin(m_scene.theta);
    drawLine("e^(i t) = cos t + i sin t", cx, cy, DARKBLUE);
    snprintf(buf, sizeof(buf), "        = %.3f %c %.3fi", c, s < 0 ? '-' : '+', std::fabs(s));
    drawLine(buf, cx, cy);
    drawLine("t = pi gives e^(i pi) = -1", cx, cy, GRAY);
    drawLine("Blue helix: e^(it) with t along Z", cx, cy, GRAY);
    cy += 8.0f;

    GuiGroupBox({x, top, width, cy - top}, "Euler's Formula");
    return cy;
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
