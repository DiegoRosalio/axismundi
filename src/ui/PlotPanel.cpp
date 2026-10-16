#include "ui/PlotPanel.hpp"
#include "ui/Widgets.hpp"
#include <raygui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace linalg {

namespace {

struct Example {
    const char* f;           // single-expression types
    const char* px;          // parametric
    const char* py;
    const char* pz;
    float tMin, tMax;
};

const Example kFunction2DExamples[] = {
    {"sin(x)", "", "", "", 0, 0},
    {"x^3/10 - x", "", "", "", 0, 0},
    {"3exp(-x^2/2)", "", "", "", 0, 0},
    {"tan(x)", "", "", "", 0, 0},
    {"abs(x) - 2", "", "", "", 0, 0},
};

const Example kParametricExamples[] = {
    {"", "2cos(t)", "2sin(t)", "t/4", 0.0f, 12.566f},                               // helix
    {"", "sin(t) + 2sin(2t)", "cos(t) - 2cos(2t)", "-sin(3t)", 0.0f, 6.2832f},      // trefoil knot
    {"", "3sin(2t)", "3sin(3t)", "0", 0.0f, 6.2832f},                               // Lissajous
    {"", "t cos(t)/2", "t sin(t)/2", "0", 0.0f, 18.85f},                           // spiral
};

const Example kSurfaceExamples[] = {
    {"sin(sqrt(x^2 + y^2))", "", "", "", 0, 0},
    {"(x^2 - y^2)/5", "", "", "", 0, 0},
    {"3exp(-(x^2 + y^2)/4)", "", "", "", 0, 0},
    {"sin(x) cos(y)", "", "", "", 0, 0},
};

const Example kImplicitExamples[] = {
    {"x^2 + y^2 - 9", "", "", "", 0, 0},
    {"x^2/16 + y^2/4 - 1", "", "", "", 0, 0},
    {"y^2 - x^3 + 3x", "", "", "", 0, 0},
    {"(x^2 + y^2)^2 - 8(x^2 - y^2)", "", "", "", 0, 0},
    {"sin(x) - cos(y)", "", "", "", 0, 0},
};

template <size_t N>
int count(const Example (&)[N]) { return static_cast<int>(N); }

void drawText(const std::string& text, float x, float& y, Color color) {
    DrawText(text.c_str(), static_cast<int>(x), static_cast<int>(y), kUiTextSmall, color);
    y += 26.0f;
}

} // namespace

PlotPanel::PlotPanel()
    : m_type(static_cast<int>(PlotType::Surface)), m_resolution(50.0f), m_heatmap(true),
      m_exampleIndex{0, 0, 0, 0}, m_activeField(-1), m_changed(true) {
    snprintf(m_function2D, kExprSize, "%s", kFunction2DExamples[0].f);
    snprintf(m_param[0], kExprSize, "%s", kParametricExamples[0].px);
    snprintf(m_param[1], kExprSize, "%s", kParametricExamples[0].py);
    snprintf(m_param[2], kExprSize, "%s", kParametricExamples[0].pz);
    snprintf(m_surface, kExprSize, "%s", kSurfaceExamples[0].f);
    snprintf(m_implicit, kExprSize, "%s", kImplicitExamples[0].f);

    m_range[XMin] = -6.0f; m_range[XMax] = 6.0f;
    m_range[YMin] = -6.0f; m_range[YMax] = 6.0f;
    m_range[TMin] = kParametricExamples[0].tMin;
    m_range[TMax] = kParametricExamples[0].tMax;
    syncRangeText();
}

void PlotPanel::syncRangeText() {
    for (int i = 0; i < RangeCount; i++) formatNumber(m_rangeText[i], kNumSize, m_range[i]);
}

char* PlotPanel::exprText(int slot) {
    switch (type()) {
        case PlotType::Function2D: return m_function2D;
        case PlotType::Parametric: return m_param[std::clamp(slot, 0, 2)];
        case PlotType::Surface:    return m_surface;
        case PlotType::Implicit:   return m_implicit;
    }
    return m_function2D;
}

std::vector<int> PlotPanel::visibleFields() const {
    switch (type()) {
        case PlotType::Function2D:
            return {0, kRangeBase + XMin, kRangeBase + XMax};
        case PlotType::Parametric:
            return {0, 1, 2, kRangeBase + TMin, kRangeBase + TMax};
        case PlotType::Surface:
        case PlotType::Implicit:
            return {0, kRangeBase + XMin, kRangeBase + XMax, kRangeBase + YMin, kRangeBase + YMax};
    }
    return {};
}

void PlotPanel::finishField(int id) {
    if (id >= kRangeBase) {
        int r = id - kRangeBase;
        float v;
        if (parseNumber(m_rangeText[r], v)) m_range[r] = v;
        formatNumber(m_rangeText[r], kNumSize, m_range[r]);
    }
    // Expressions are kept as typed; parse errors are shown under the inputs
    m_changed = true;
}

void PlotPanel::finishEditing() {
    if (m_activeField < 0) return;
    int id = m_activeField;
    m_activeField = -1;
    finishField(id);
}

void PlotPanel::update() {
    if (m_activeField < 0 || !IsKeyPressed(KEY_TAB)) return;

    std::vector<int> ids = visibleFields();
    auto it = std::find(ids.begin(), ids.end(), m_activeField);
    size_t idx = (it == ids.end()) ? 0 : static_cast<size_t>(it - ids.begin());
    bool back = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    idx = back ? (idx + ids.size() - 1) % ids.size() : (idx + 1) % ids.size();

    finishField(m_activeField);
    m_activeField = ids[idx];
}

void PlotPanel::loadExample() {
    m_activeField = -1;
    int& index = m_exampleIndex[m_type];
    switch (type()) {
        case PlotType::Function2D:
            index = (index + 1) % count(kFunction2DExamples);
            snprintf(m_function2D, kExprSize, "%s", kFunction2DExamples[index].f);
            break;
        case PlotType::Parametric: {
            index = (index + 1) % count(kParametricExamples);
            const Example& e = kParametricExamples[index];
            snprintf(m_param[0], kExprSize, "%s", e.px);
            snprintf(m_param[1], kExprSize, "%s", e.py);
            snprintf(m_param[2], kExprSize, "%s", e.pz);
            m_range[TMin] = e.tMin;
            m_range[TMax] = e.tMax;
            syncRangeText();
            break;
        }
        case PlotType::Surface:
            index = (index + 1) % count(kSurfaceExamples);
            snprintf(m_surface, kExprSize, "%s", kSurfaceExamples[index].f);
            break;
        case PlotType::Implicit:
            index = (index + 1) % count(kImplicitExamples);
            snprintf(m_implicit, kExprSize, "%s", kImplicitExamples[index].f);
            break;
    }
    m_changed = true;
}

PlotSpec PlotPanel::spec() const {
    PlotSpec s;
    s.type = type();
    switch (s.type) {
        case PlotType::Function2D: s.f = m_function2D; break;
        case PlotType::Surface:    s.f = m_surface; break;
        case PlotType::Implicit:   s.f = m_implicit; break;
        case PlotType::Parametric:
            s.px = m_param[0];
            s.py = m_param[1];
            s.pz = m_param[2];
            break;
    }
    s.xMin = m_range[XMin]; s.xMax = m_range[XMax];
    s.yMin = m_range[YMin]; s.yMax = m_range[YMax];
    s.tMin = m_range[TMin]; s.tMax = m_range[TMax];
    s.resolution = static_cast<int>(m_resolution);
    s.heatmap = m_heatmap;
    return s;
}

float PlotPanel::drawRange(float x, float y, float inner, const char* name, Range lo, Range hi) {
    float labelW = 40.0f, toW = 50.0f;
    float boxW = (inner - labelW - toW) / 2.0f;
    GuiLabel({x, y + 8, labelW, 32}, name);
    Range ids[2] = {lo, hi};
    for (int k = 0; k < 2; k++) {
        int r = ids[k];
        float bx = x + labelW + k * (boxW + toW);
        float dummy;
        bool invalid = !parseNumber(m_rangeText[r], dummy);
        if (textField({bx, y, boxW, 44}, m_rangeText[r], kNumSize, kRangeBase + r, m_activeField, invalid)) {
            finishField(kRangeBase + r);
        }
    }
    GuiLabel({x + labelW + boxW + 8, y + 8, toW, 32}, "to");
    return y + 52.0f;
}

float PlotPanel::draw(float x, float y, float width, const PlotRenderer& renderer) {
    const float pad = 15.0f;
    float inner = width - 2 * pad;
    float cx = x + pad;
    float top = y;
    float cy = y + 30.0f;

    // Plot type selector
    setGuiTextSize(kUiTextSmall);
    int type = m_type;
    float tabW = (inner - 3 * 4.0f) / kPlotTypeCount;
    GuiSetStyle(TOGGLE, GROUP_PADDING, 4);
    GuiToggleGroup({cx, cy, tabW, 36}, "y=f(x);Curve;Surface;Implicit", &type);
    if (type != m_type) {
        finishEditing();
        m_type = type;
        m_changed = true;
    }
    cy += 48.0f;
    setGuiTextSize(kUiTextLarge);

    // Expression inputs
    auto exprField = [&](int slot, const char* label, float labelW) {
        GuiLabel({cx, cy + 8, labelW, 32}, label);
        char* text = exprText(slot);
        Expression probe;
        bool invalid = !probe.parse(text);
        if (textField({cx + labelW, cy, inner - labelW, 48}, text, kExprSize, slot, m_activeField, invalid)) {
            finishField(slot);
        }
        cy += 56.0f;
    };

    switch (this->type()) {
        case PlotType::Function2D:
            exprField(0, "y =", 70);
            cy = drawRange(cx, cy, inner, "x", XMin, XMax);
            break;
        case PlotType::Parametric:
            exprField(0, "x =", 70);
            exprField(1, "y =", 70);
            exprField(2, "z =", 70);
            cy = drawRange(cx, cy, inner, "t", TMin, TMax);
            break;
        case PlotType::Surface:
            exprField(0, "z =", 70);
            cy = drawRange(cx, cy, inner, "x", XMin, XMax);
            cy = drawRange(cx, cy, inner, "y", YMin, YMax);
            break;
        case PlotType::Implicit:
            GuiLabel({cx, cy + 8, 200, 32}, "f(x, y) = 0 with f =");
            cy += 42.0f;
            exprField(0, "", 0);
            cy = drawRange(cx, cy, inner, "x", XMin, XMax);
            cy = drawRange(cx, cy, inner, "y", YMin, YMax);
            break;
    }

    // Resolution and heat map
    char resText[16];
    snprintf(resText, sizeof(resText), "%d", static_cast<int>(m_resolution));
    float before = m_resolution;
    GuiLabel({cx, cy, 100, 32}, "Detail");
    GuiSlider({cx + 100, cy, inner - 160, 32}, nullptr, resText, &m_resolution, 10.0f, 100.0f);
    if (static_cast<int>(before) != static_cast<int>(m_resolution)) m_changed = true;
    cy += 44.0f;

    bool heat = m_heatmap;
    GuiCheckBox({cx, cy, 28, 28}, "Heat map", &heat);
    if (heat != m_heatmap) {
        m_heatmap = heat;
        m_changed = true;
    }
    if (GuiButton({cx + inner / 2, cy - 6, inner / 2, 42}, "Example")) loadExample();
    cy += 48.0f;

    // Status: parse error or value range
    if (!renderer.error().empty()) {
        drawText(renderer.error(), cx, cy, MAROON);
    } else {
        const char* name = this->type() == PlotType::Function2D ? "y"
                         : this->type() == PlotType::Implicit   ? "f" : "z";
        char buf[96];
        snprintf(buf, sizeof(buf), "%s range: [%.2f, %.2f]", name, renderer.minValue(), renderer.maxValue());
        drawText(buf, cx, cy, GRAY);
    }
    drawText("Variables: x, y, t   Constants: pi, e", cx, cy, GRAY);
    drawText("sin cos tan exp log sqrt abs ...", cx, cy, GRAY);
    if (this->type() == PlotType::Surface || this->type() == PlotType::Parametric)
        drawText("3D plots: z is up", cx, cy, GRAY);
    cy += 6.0f;

    GuiGroupBox({x, top, width, cy - top}, "Function Plot");
    return cy;
}

void PlotPanel::saveState(SceneData& data) const {
    putInt(data, "plot.type", m_type);
    data["plot.function2d"] = m_function2D;
    data["plot.param.x"] = m_param[0];
    data["plot.param.y"] = m_param[1];
    data["plot.param.z"] = m_param[2];
    data["plot.surface"] = m_surface;
    data["plot.implicit"] = m_implicit;
    const char* names[RangeCount] = {"x_min", "x_max", "y_min", "y_max", "t_min", "t_max"};
    for (int i = 0; i < RangeCount; i++) putFloat(data, std::string("plot.") + names[i], m_range[i]);
    putFloat(data, "plot.resolution", m_resolution);
    putBool(data, "plot.heatmap", m_heatmap);
}

void PlotPanel::loadState(const SceneData& data) {
    int type = m_type;
    if (getInt(data, "plot.type", type) && type >= 0 && type < kPlotTypeCount) m_type = type;
    getText(data, "plot.function2d", m_function2D, kExprSize);
    getText(data, "plot.param.x", m_param[0], kExprSize);
    getText(data, "plot.param.y", m_param[1], kExprSize);
    getText(data, "plot.param.z", m_param[2], kExprSize);
    getText(data, "plot.surface", m_surface, kExprSize);
    getText(data, "plot.implicit", m_implicit, kExprSize);
    const char* names[RangeCount] = {"x_min", "x_max", "y_min", "y_max", "t_min", "t_max"};
    for (int i = 0; i < RangeCount; i++) getFloat(data, std::string("plot.") + names[i], m_range[i]);
    if (getFloat(data, "plot.resolution", m_resolution)) m_resolution = std::clamp(m_resolution, 10.0f, 100.0f);
    getBool(data, "plot.heatmap", m_heatmap);
    syncRangeText();
    m_activeField = -1;
    m_changed = true;
}

} // namespace linalg
