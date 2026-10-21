#include "ui/InfoPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>

namespace linalg {

InfoPanel::InfoPanel() : m_eigen{} {}

void InfoPanel::update(const scene::Space& space, const scene::Selection& selection) {
    scene::Analysis a = scene::analyze(space, selection);
    // Keep the sections' open/closed state by key
    std::vector<Section> sections;
    for (auto& section : a.sections) {
        bool expanded = section.expanded;
        for (const auto& p : m_sections)
            if (p.content.key == section.key) expanded = p.expanded;
        sections.push_back({std::move(section), expanded});
    }
    m_sections = std::move(sections);
    m_eigen = a.eigen;
    m_hasMatrix = a.hasMatrix;
}

float InfoPanel::drawContent(Rectangle area) {
    const ui::Theme& t = ui::theme();
    float x = area.x + ui::px(ui::kPad);
    float w = area.width - 2 * ui::px(ui::kPad);
    float y = area.y + ui::px(8.0f);
    float startY = area.y;

    using Tone = scene::AnalysisLine::Tone;
    using Style = scene::AnalysisLine::Style;
    for (auto& section : m_sections) {
        const scene::AnalysisSection& s = section.content;
        Rectangle headerRect = {x, y, w, ui::px(28.0f)};
        ui::sectionHeader(headerRect, s.title.c_str(), section.expanded, nullptr, true);
        if (s.tip) ui::tooltip(headerRect, s.tip, s.title.c_str());
        y += headerRect.height + ui::px(2.0f);

        if (section.expanded) {
            for (const auto& line : s.lines) {
                Color c = t.text;
                if (line.tone == Tone::Muted) c = t.textMuted;
                else if (line.tone == Tone::Eigen) c = t.eigen;
                else if (line.tone == Tone::Danger) c = t.danger;
                ui::FontRole role = line.style == Style::Mono    ? ui::FontRole::Mono
                                    : line.style == Style::Small ? ui::FontRole::Small
                                                                 : ui::FontRole::Body;
                float lineH = std::round(ui::fontSize(role) * 1.45f);
                std::string shown = ui::ellipsize(line.text.c_str(), role, w - ui::px(14.0f));
                ui::text(shown.c_str(), x + ui::px(14.0f), y + (lineH - ui::fontSize(role)) / 2, role, c);
                if (line.tip) ui::tooltip({x, y, w, lineH}, line.tip);
                y += lineH;
            }
            y += ui::px(10.0f);
        }
    }
    return y - startY + ui::px(4.0f);
}

} // namespace linalg
