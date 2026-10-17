#include "ui/ReferencePanel.hpp"
#include "math/expr/Reference.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace linalg {

namespace {

constexpr int kSearchId = 1, kTryId = 2;

// Short names for the section chips, in the order of expr::reference()
const char* kChips[] = {"Basics", "Names", "Time", "Vectors", "Matrices", "Complex", "Functions", "Blending", "Errors"};

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string num(double v) {
    if (std::fabs(v) < 1e-6) v = 0.0;  // float noise such as cos(pi/2)
    char buf[32];
    snprintf(buf, sizeof(buf), "%.4g", v);
    return buf;
}

// Compact one-line result: 512, 1 + 2i, (1, 0, 0), [1 2 0; 0 1 0; 0 0 1]
std::string compact(const expr::Value& v) {
    using expr::Type;
    switch (v.type) {
        case Type::Scalar: return num(v.re);
        case Type::Complex: {
            if (std::fabs(v.im) < 1e-6) return num(v.re);
            std::string im = num(std::fabs(v.im)) + "i";
            if (std::fabs(v.re) < 1e-6) return (v.im < 0 ? "−" : "") + im;
            return num(v.re) + (v.im < 0 ? " − " : " + ") + im;
        }
        case Type::Vector:
            return "(" + num(v.vector.x) + ", " + num(v.vector.y) + ", " + num(v.vector.z) + ")";
        case Type::Matrix: {
            std::string s = "[";
            for (int i = 0; i < v.size; i++) {
                if (i) s += "; ";
                for (int j = 0; j < v.size; j++) s += (j ? " " : "") + num(v.matrix.m[i][j]);
            }
            return s + "]";
        }
        case Type::Error: return v.error;
    }
    return "";
}

expr::Value evaluate(const std::string& text, const expr::Resolver* resolver, double time, unsigned variables,
                     std::string& error) {
    expr::Program p;
    if (!p.compile(text, variables)) {
        error = p.error();
        return expr::Value::fail(error);
    }
    expr::Slots slots;
    slots.x = 1.0;
    slots.y = 2.0;
    slots.t = time;
    slots.u = 0.5;
    slots.z = 3.0;
    slots.hasX = slots.hasY = slots.hasT = slots.hasU = slots.hasZ = true;
    expr::Value v = p.eval(resolver, &slots);
    if (!v.ok()) error = v.error;
    return v;
}

} // namespace

void ReferencePanel::setTryText(const std::string& text) {
    std::snprintf(m_try, sizeof(m_try), "%s", text.c_str());
}

void ReferencePanel::setSearch(const std::string& text) {
    std::snprintf(m_search, sizeof(m_search), "%s", text.c_str());
}

float ReferencePanel::drawContent(Rectangle area, const expr::Resolver* resolver, double time) {
    const ui::Theme& t = ui::theme();
    float pad = ui::px(ui::kPad);
    ui::Column col(area.x + pad, area.y + ui::px(12.0f), area.width - 2 * pad, 6.0f);

    // Try a formula: evaluated against the objects of the current space
    ui::textIn(col.row(18), ui::upper("Try a formula").c_str(), ui::FontRole::Caption, t.textMuted);
    Rectangle tryBox = col.row(34.0f);
    std::string tryError;
    expr::Value tried;
    bool hasTry = m_try[0] != '\0';
    if (hasTry) tried = evaluate(m_try, resolver, time, 0, tryError);
    ui::textField(tryBox, m_try, sizeof(m_try), kTryId, m_activeId, hasTry && !tried.ok());
    ui::anchor("reference.try", tryBox);
    if (!hasTry && m_activeId != kTryId)
        ui::textIn({tryBox.x + ui::px(10), tryBox.y, tryBox.width - ui::px(20), tryBox.height},
                   "e.g. rotz(pi/4)*[1, 0, 0] or cross(a, b)", ui::FontRole::Mono, t.textFaint);
    ui::tooltip(tryBox, "Type any formula. Names refer to objects in the current space, t is the scene time, "
                        "x = 1 and y = 2. Click an example below to copy it here.", "Try a formula");
    if (hasTry) {
        if (tried.ok() && tried.type == expr::Type::Matrix) {
            for (int i = 0; i < tried.size; i++) {
                std::string line = i == 0 ? "= [ " : "  [ ";
                for (int j = 0; j < tried.size; j++) {
                    char cell[24];
                    double v = tried.matrix.m[i][j];
                    snprintf(cell, sizeof(cell), "%8.3f", std::fabs(v) < 1e-6 ? 0.0 : v);
                    line += cell;
                }
                ui::textIn(col.row(18), (line + " ]").c_str(), ui::FontRole::Mono, t.text);
            }
        } else {
            std::string line = tried.ok() ? "= " + compact(tried) + "   (" + tried.typeName() + ")" : tryError;
            float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, line.c_str(),
                                      tried.ok() ? ui::FontRole::Mono : ui::FontRole::Small,
                                      tried.ok() ? t.text : t.danger);
            col.space(h / ui::scale());
        }
    }
    col.space(6.0f);

    // Search and section chips
    Rectangle searchBox = col.row(32.0f);
    ui::textField(searchBox, m_search, sizeof(m_search), kSearchId, m_activeId);
    if (!m_search[0] && m_activeId != kSearchId)
        ui::textIn({searchBox.x + ui::px(10), searchBox.y, searchBox.width - ui::px(20), searchBox.height},
                   "Search: cross, matrix, time, complex…", ui::FontRole::Body, t.textFaint);
    ui::anchor("reference.search", searchBox);

    const auto& sections = expr::reference();
    {
        float x = col.x(), y = col.y(), h = ui::px(28.0f), gap = ui::px(6.0f);
        auto chip = [&](const char* label, bool selected) {
            float w = ui::textWidth(label, ui::FontRole::BodyStrong) + ui::px(30.0f);
            if (x + w > col.x() + col.width()) {
                x = col.x();
                y += h + gap;
            }
            Rectangle r = {x, y, w, h};
            x += w + gap;
            return ui::button(r, label, selected ? ui::ButtonKind::Primary : ui::ButtonKind::Secondary);
        };
        if (chip("All", m_section < 0)) m_section = -1;
        for (int i = 0; i < static_cast<int>(sections.size()) && i < static_cast<int>(std::size(kChips)); i++)
            if (chip(kChips[i], m_section == i)) m_section = (m_section == i) ? -1 : i;
        col.setY(y + h + ui::px(10.0f));
    }

    // Sections and entries
    std::string query = lower(m_search);
    int shownEntries = 0;
    for (int si = 0; si < static_cast<int>(sections.size()); si++) {
        if (m_section >= 0 && si != m_section) continue;
        const auto& sec = sections[si];
        bool sectionMatch = !query.empty() && lower(sec.title).find(query) != std::string::npos;
        std::vector<const expr::ReferenceEntry*> entries;
        for (const auto& e : sec.entries) {
            if (query.empty() || sectionMatch || lower(e.syntax + " " + e.text + " " + e.example).find(query) != std::string::npos)
                entries.push_back(&e);
        }
        if (entries.empty()) continue;

        ui::textIn(col.row(26), sec.title.c_str(), ui::FontRole::Title, t.text);
        if (query.empty()) {
            float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, sec.intro.c_str(), ui::FontRole::Small,
                                      t.textMuted);
            col.space(h / ui::scale() + 4.0f);
        }
        for (const auto* e : entries) {
            shownEntries++;
            ui::textIn(col.row(22), ui::ellipsize(e->syntax.c_str(), ui::FontRole::Mono, col.width()).c_str(),
                       ui::FontRole::Mono, t.accent);
            float h = ui::textWrapped({col.x() + ui::px(12), col.y(), col.width() - ui::px(12), 0}, e->text.c_str(),
                                      ui::FontRole::Body, t.text);
            col.space(h / ui::scale());
            if (!e->example.empty()) {
                Rectangle r = col.row(26);
                bool hover = ui::hovered(r);
                if (hover) ui::fillRounded(r, ui::px(6.0f), t.hover);
                if (ui::clicked(r)) setTryText(e->example);
                std::string error;
                expr::Value v = evaluate(e->example, resolver, time,
                                         sec.title.find("Time") == 0 ? (expr::kVarU | expr::kVarZ) : 0u, error);
                Rectangle inner = {r.x + ui::px(12), r.y, r.width - ui::px(20), r.height};
                Rectangle eg = ui::takeLeft(inner, 30.0f, 4.0f);
                ui::textIn(eg, "e.g.", ui::FontRole::Small, t.textFaint);
                std::string shown = e->example + "   →   " +
                                    (v.ok() ? compact(v) : (e->usesObjects ? "needs your objects" : error));
                ui::textIn(inner, ui::ellipsize(shown.c_str(), ui::FontRole::Mono, inner.width).c_str(),
                           ui::FontRole::Mono, v.ok() ? t.textMuted : t.textFaint);
                ui::tooltip(r, (v.ok() ? "= " + compact(v) + ". Click to copy it into the box above."
                                       : (e->usesObjects ? "Uses objects such as a, b or M; they must exist in this "
                                                           "space. Click to try it."
                                                         : error))
                                   .c_str(),
                            e->example.c_str());
            }
            col.space(8.0f);
        }
        col.space(6.0f);
    }
    if (shownEntries == 0) {
        float h = ui::textWrapped({col.x(), col.y(), col.width(), 0}, "Nothing matches. Try a function name such as "
                                  "rot or lerp, or a word such as vector.", ui::FontRole::Small, t.textMuted);
        col.space(h / ui::scale());
    }
    return col.y() - area.y + ui::px(8.0f);
}

} // namespace linalg
