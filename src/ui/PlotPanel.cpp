#include "ui/PlotPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
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

void PlotPanel::cancelEditing() {
    if (m_activeField < 0) return;
    if (m_activeField >= kRangeBase) {
        int r = m_activeField - kRangeBase;
        formatNumber(m_rangeText[r], kNumSize, m_range[r]);
    }
    // Expressions have no separate committed copy: keep the text as typed
    m_activeField = -1;
    m_changed = true;
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

void PlotPanel::setSpec(const PlotSpec& s) {
    m_activeField = -1;
    m_type = static_cast<int>(s.type);
    switch (s.type) {
        case PlotType::Function2D: snprintf(m_function2D, kExprSize, "%s", s.f.c_str()); break;
        case PlotType::Surface:    snprintf(m_surface, kExprSize, "%s", s.f.c_str()); break;
        case PlotType::Implicit:   snprintf(m_implicit, kExprSize, "%s", s.f.c_str()); break;
        case PlotType::Parametric:
            snprintf(m_param[0], kExprSize, "%s", s.px.c_str());
            snprintf(m_param[1], kExprSize, "%s", s.py.c_str());
            snprintf(m_param[2], kExprSize, "%s", s.pz.c_str());
            break;
    }
    m_range[XMin] = s.xMin; m_range[XMax] = s.xMax;
    m_range[YMin] = s.yMin; m_range[YMax] = s.yMax;
    m_range[TMin] = s.tMin; m_range[TMax] = s.tMax;
    m_resolution = std::clamp(static_cast<float>(s.resolution), 10.0f, 100.0f);
    m_heatmap = s.heatmap;
    syncRangeText();
    m_changed = true;
}

float PlotPanel::drawRange(float x, float y, float width, const char* name, Range lo, Range hi) {
    const ui::Theme& t = ui::theme();
    ui::Column col(x, y, width);
    Rectangle r = col.row(34.0f);
    Rectangle label = ui::takeLeft(r, 34.0f, 6.0f);
    ui::textIn(label, name, ui::FontRole::Body, t.textMuted);
    auto cells = ui::splitRow(r, {1, 1}, 30.0f);
    Range ids[2] = {lo, hi};
    for (int k = 0; k < 2; k++) {
        int idx = ids[k];
        float dummy;
        bool invalid = !parseNumber(m_rangeText[idx], dummy);
        if (ui::textField(cells[k], m_rangeText[idx], kNumSize, kRangeBase + idx, m_activeField, invalid)) {
            finishField(kRangeBase + idx);
        }
    }
    ui::textIn({cells[0].x + cells[0].width, r.y, ui::px(30.0f), r.height}, "to", ui::FontRole::Small,
               t.textMuted, ui::Align::Center);
    return col.y();
}

float PlotPanel::draw(float x, float y, float width, const PlotRenderer& renderer) {
    const ui::Theme& t = ui::theme();
    int previous = m_activeField;
    ui::Column col(x, y, width);

    ui::sectionHeader(col.row(24), "Function", m_expanded);
    if (!m_expanded) return col.y() + ui::px(6.0f);

    // Plot type selector
    int type = m_type;
    if (ui::segmented(col.row(34.0f), {"y = f(x)", "Curve", "Surface", "Implicit"}, type)) {
        finishEditing();
        m_type = type;
        m_changed = true;
    }
    col.space(4.0f);

    // Expression inputs
    auto exprField = [&](int slot, const char* label) {
        Rectangle r = col.row(34.0f);
        Rectangle l = ui::takeLeft(r, 34.0f, 6.0f);
        ui::textIn(l, label, ui::FontRole::Body, t.textMuted);
        char* text = exprText(slot);
        Expression probe;
        bool invalid = !probe.parse(text);
        if (ui::textField(r, text, kExprSize, slot, m_activeField, invalid)) finishField(slot);
    };

    switch (this->type()) {
        case PlotType::Function2D:
            exprField(0, "y =");
            col.setY(drawRange(col.x(), col.y(), col.width(), "x", XMin, XMax));
            break;
        case PlotType::Parametric:
            exprField(0, "x =");
            exprField(1, "y =");
            exprField(2, "z =");
            col.setY(drawRange(col.x(), col.y(), col.width(), "t", TMin, TMax));
            break;
        case PlotType::Surface:
            exprField(0, "z =");
            col.setY(drawRange(col.x(), col.y(), col.width(), "x", XMin, XMax));
            col.setY(drawRange(col.x(), col.y(), col.width(), "y", YMin, YMax));
            break;
        case PlotType::Implicit:
            ui::textIn(col.row(20), "Curve where f(x, y) = 0, with f =", ui::FontRole::Small, t.textMuted);
            exprField(0, "f =");
            col.setY(drawRange(col.x(), col.y(), col.width(), "x", XMin, XMax));
            col.setY(drawRange(col.x(), col.y(), col.width(), "y", YMin, YMax));
            break;
    }

    // Focus moved straight to another field: finish the old one too
    if (previous >= 0 && m_activeField != previous) finishField(previous);

    // Detail and heat map
    col.space(2.0f);
    Rectangle r = col.row(30);
    Rectangle label = ui::takeLeft(r, 52.0f, 8.0f);
    Rectangle value = ui::takeRight(r, 30.0f, 8.0f);
    ui::textIn(label, "Detail", ui::FontRole::Body, t.text);
    float before = m_resolution;
    ui::slider(r, m_resolution, 10.0f, 100.0f);
    if (static_cast<int>(before) != static_cast<int>(m_resolution)) m_changed = true;
    char resText[16];
    snprintf(resText, sizeof(resText), "%d", static_cast<int>(m_resolution));
    ui::textIn(value, resText, ui::FontRole::Mono, t.textMuted, ui::Align::Right);

    if (ui::toggleRow(col.row(30), "Heat map", m_heatmap)) m_changed = true;
    if (ui::button(col.row(ui::kRowHeight), "Next example", ui::ButtonKind::Secondary, true, ui::Icon::Gallery)) {
        loadExample();
    }
    col.space(4.0f);

    // Status: parse error or value range
    char buf[128];
    Color statusColor = t.textMuted;
    std::string status;
    if (!renderer.error().empty()) {
        status = renderer.error();
        statusColor = t.danger;
    } else {
        const char* name = this->type() == PlotType::Function2D ? "y"
                         : this->type() == PlotType::Implicit   ? "f" : "z";
        snprintf(buf, sizeof(buf), "%s ranges over [%.2f, %.2f]", name, renderer.minValue(), renderer.maxValue());
        status = buf;
    }
    float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, status.c_str(), ui::FontRole::Small, statusColor);
    col.space(h / ui::scale() + 4.0f);

    const char* help = "Use x, y, t, pi, e and sin cos tan exp log sqrt abs. "
                       "Write 2x for 2·x and x^2 for powers.";
    h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, help, ui::FontRole::Small, t.textFaint);
    col.space(h / ui::scale());
    if (this->type() == PlotType::Surface || this->type() == PlotType::Parametric) {
        ui::textIn(col.row(18), "3D plots use z as the up axis.", ui::FontRole::Small, t.textFaint);
    }
    col.space(10.0f);
    return col.y();
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
