#include "ui/ExportPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace linalg {

latex::Result ExportPanel::generate(const scene::Space& space, const scene::Selection& selection, double azimuth,
                                    double elevation) const {
    latex::Options o = m_options;
    o.azimuth = std::round(azimuth);
    o.elevation = std::round(elevation);
    return latex::exportLatex(space, selection, o);
}

float ExportPanel::drawContent(Rectangle area, const scene::Space& space, const scene::Selection& selection,
                               double azimuth, double elevation, Actions& actions) {
    const ui::Theme& t = ui::theme();
    float pad = ui::px(ui::kPad);
    ui::Column col(area.x + pad, area.y + ui::px(12.0f), area.width - 2 * pad, 6.0f);
    bool complexPlane = space.kind() == scene::SpaceKind::Complex;

    int scope = static_cast<int>(m_options.scope);
    if (ui::segmented(col.row(30), {"Visible objects", "Selection"}, scope,
                      {"Every object you can see in this space.", "Only the selected objects (Ctrl+click to pick several)."}))
        m_options.scope = static_cast<latex::Scope>(scope);
    ui::anchor("export.scope", {col.x(), col.y() - ui::px(36), col.width(), ui::px(30)});
    if (!complexPlane) {
        int view = static_cast<int>(m_options.view);
        if (ui::segmented(col.row(30), {"Auto", "2D", "3D"}, view,
                          {"2D when everything lies in the floor plane (functions, flat vectors), otherwise 3D.",
                           "A flat figure of the xy-plane.", "A 3D figure seen from the current camera angle."}))
            m_options.view = static_cast<latex::View>(view);
    }

    // Width
    Rectangle row = col.row(30);
    Rectangle label = ui::takeLeft(row, 54.0f, 6.0f);
    Rectangle value = ui::takeRight(row, 96.0f, 6.0f);
    ui::textIn(label, "Width", ui::FontRole::Body, t.text);
    float w = static_cast<float>(m_options.width);
    if (ui::slider(row, w, 0.3f, 1.0f)) m_options.width = std::round(w * 20.0f) / 20.0f;
    char buf[160];
    snprintf(buf, sizeof(buf), "%.2f\\linewidth", m_options.width);
    ui::textIn(value, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    ui::tooltip({label.x, row.y, value.x + value.width - label.x, row.height},
                "Figure width as a fraction of the text width of your document.", "Width");

    auto cells = ui::splitRow(col.row(30), {1, 1}, 12.0f);
    ui::toggleRow(cells[0], "Axes", m_options.axes);
    ui::toggleRow(cells[1], "Legend", m_options.legend);
    cells = ui::splitRow(col.row(30), {1, 1}, 12.0f);
    ui::toggleRow(cells[0], "Grid", m_options.grid);
    ui::toggleRow(cells[1], "Gray surfaces", m_options.grayShading);
    ui::tooltip(cells[1], "Shade surfaces in grays by height; off: white with a black mesh.", "Gray surfaces");
    cells = ui::splitRow(col.row(30), {1, 1}, 12.0f);
    ui::toggleRow(cells[0], "Standalone", m_options.standalone);
    ui::tooltip(cells[0], "A complete document you can compile on its own (documentclass standalone). Off: just the "
                          "tikzpicture, to paste into your document.", "Standalone document");
    ui::toggleRow(cells[1], "B/W preview", m_preview);
    ui::tooltip(cells[1], "Draw the 3D view in the export's black-and-white styles while you design the figure.",
                "Print preview");
    ui::anchor("export.preview", cells[1]);

    // Regenerate a few times per second (sampling plots is not free)
    double now = GetTime();
    if (m_lastTime < 0 || now - m_lastTime > 0.25) {
        m_last = generate(space, selection, azimuth, elevation);
        m_lastTime = now;
    }

    // What will be exported
    col.space(2.0f);
    std::string summary;
    if (m_last.exported == 0) {
        summary = m_options.scope == latex::Scope::Selection ? "Nothing selected to export."
                                                             : "Nothing visible to export in this space.";
    } else {
        snprintf(buf, sizeof(buf), "%d object%s as a %s figure", m_last.exported, m_last.exported == 1 ? "" : "s",
                 m_last.flat ? "2D" : "3D");
        summary = buf;
        if (!m_last.flat) {
            snprintf(buf, sizeof(buf), ", seen from the camera (view %.0f°, %.0f°)", std::round(azimuth),
                     std::round(elevation));
            summary += buf;
        }
        summary += ".";
    }
    if (!m_last.skipped.empty()) {
        summary += " Left out: ";
        for (size_t i = 0; i < m_last.skipped.size(); i++) summary += (i ? ", " : "") + m_last.skipped[i];
        summary += ".";
    }
    float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, summary.c_str(), ui::FontRole::Small,
                              m_last.exported ? t.textMuted : t.warning);
    col.space(h / ui::scale() + 4.0f);

    // Code preview (first lines)
    Rectangle code = col.row(150.0f);
    ui::fillRounded(code, ui::px(ui::kRadius), t.panelAlt);
    ui::strokeRounded(code, ui::px(ui::kRadius), ui::pxi(1.0f), t.border);
    std::istringstream lines(m_last.code);
    std::string ln;
    float y = code.y + ui::px(6);
    float lineH = ui::fontSize(ui::FontRole::Mono) * 1.25f;
    int total = 0;
    while (std::getline(lines, ln)) {
        total++;
        if (y + lineH > code.y + code.height - ui::px(4)) continue;
        ui::text(ui::ellipsize(ln.c_str(), ui::FontRole::Mono, code.width - ui::px(16)).c_str(), code.x + ui::px(8), y,
                 ui::FontRole::Mono, ln.rfind("%", 0) == 0 ? t.textFaint : t.textMuted);
        y += lineH;
    }
    snprintf(buf, sizeof(buf), "%d lines, %.1f kB", total, m_last.code.size() / 1024.0);
    ui::textIn(col.row(18), buf, ui::FontRole::Small, t.textFaint, ui::Align::Right);

    auto buttons = ui::splitRow(col.row(ui::kRowHeight), {1, 1}, 8.0f);
    if (ui::button(buttons[0], "Copy LaTeX", ui::ButtonKind::Primary, m_last.exported > 0, ui::Icon::Steps))
        actions.copy = true;
    if (ui::button(buttons[1], "Save .tex…", ui::ButtonKind::Secondary, m_last.exported > 0, ui::Icon::Save))
        actions.save = true;
    ui::anchor("export.copy", buttons[0]);
    ui::tooltip(buttons[0], "Copy the figure to the clipboard, then paste it into your document.", "Copy LaTeX");
    ui::tooltip(buttons[1], "Save the figure as a .tex file to \\input in your document.", "Save .tex");
    float nh = ui::textWrapped({col.x(), col.y(), col.width(), 0},
                               "Needs \\usepackage{pgfplots}, \\pgfplotsset{compat=1.18} and "
                               "\\usetikzlibrary{arrows.meta} in your preamble (they are listed in the first comment).",
                               ui::FontRole::Small, t.textFaint);
    col.space(nh / ui::scale());
    return col.y() - area.y + ui::px(8.0f);
}

void ExportPanel::saveState(SceneData& data) const {
    putInt(data, "export.scope", static_cast<int>(m_options.scope));
    putInt(data, "export.view", static_cast<int>(m_options.view));
    putFloat(data, "export.width", static_cast<float>(m_options.width));
    putBool(data, "export.axes", m_options.axes);
    putBool(data, "export.legend", m_options.legend);
    putBool(data, "export.grid", m_options.grid);
    putBool(data, "export.gray", m_options.grayShading);
    putBool(data, "export.standalone", m_options.standalone);
}

void ExportPanel::loadState(const SceneData& data) {
    int v = 0;
    if (getInt(data, "export.scope", v) && v >= 0 && v <= 1) m_options.scope = static_cast<latex::Scope>(v);
    if (getInt(data, "export.view", v) && v >= 0 && v <= 2) m_options.view = static_cast<latex::View>(v);
    float w = 0;
    if (getFloat(data, "export.width", w)) m_options.width = std::clamp(static_cast<double>(w), 0.3, 1.0);
    getBool(data, "export.axes", m_options.axes);
    getBool(data, "export.legend", m_options.legend);
    getBool(data, "export.grid", m_options.grid);
    getBool(data, "export.gray", m_options.grayShading);
    getBool(data, "export.standalone", m_options.standalone);
}

} // namespace linalg
