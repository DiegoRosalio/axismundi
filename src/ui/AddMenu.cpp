#include "ui/AddMenu.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>

namespace linalg {

using scene::SceneObject;

bool AddMenu::draw(scene::Scene& sc, scene::SpaceKind kind, AddChoice& choice) {
    if (!m_open) return false;
    const ui::Theme& t = ui::theme();

    float w = ui::px(310.0f);
    float maxH = GetScreenHeight() - m_anchor.y - m_anchor.height - ui::px(20);
    float h = std::min(m_contentHeight + ui::px(16), maxH);
    m_bounds = {m_anchor.x + m_anchor.width - w, m_anchor.y + m_anchor.height + ui::px(6), w, h};
    if (m_bounds.x < ui::px(8)) m_bounds.x = ui::px(8);

    if (!m_justOpened && ui::mousePressed(MOUSE_BUTTON_LEFT) && !CheckCollisionPointRec(GetMousePosition(), m_bounds)) {
        m_open = false;
        return false;
    }
    m_justOpened = false;

    ui::card(m_bounds);
    float maxScroll = std::max(0.0f, m_contentHeight + ui::px(16) - m_bounds.height);
    if (CheckCollisionPointRec(GetMousePosition(), m_bounds)) m_scroll -= GetMouseWheelMove() * ui::px(40);
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);
    BeginScissorMode(static_cast<int>(m_bounds.x), static_cast<int>(m_bounds.y), static_cast<int>(m_bounds.width),
                     static_cast<int>(m_bounds.height));
    ui::setClip(m_bounds);

    float x = m_bounds.x + ui::px(8), width = m_bounds.width - ui::px(16);
    float y = m_bounds.y + ui::px(8) - m_scroll;
    float startY = y;
    bool picked = false;

    auto heading = [&](const char* text) {
        ui::textIn({x + ui::px(8), y, width, ui::px(26)}, ui::upper(text).c_str(), ui::FontRole::Caption, t.textMuted);
        y += ui::px(28);
    };
    auto entry = [&](const std::string& label, const char* description, const AddChoice& c) {
        float rowH = description ? ui::px(46) : ui::px(32);
        Rectangle row = {x, y, width, rowH};
        bool hover = ui::hovered(row);
        if (hover) ui::fillRounded(row, ui::px(ui::kRadius), t.hover);
        ui::text(label.c_str(), row.x + ui::px(10), row.y + ui::px(6), ui::FontRole::BodyStrong, t.text);
        if (description) {
            std::string d = ui::ellipsize(description, ui::FontRole::Small, width - ui::px(20));
            ui::text(d.c_str(), row.x + ui::px(10), row.y + ui::px(26), ui::FontRole::Small, t.textMuted);
        }
        if (hover && ui::mousePressed(MOUSE_BUTTON_LEFT)) {
            choice = c;
            picked = true;
        }
        y += rowH + ui::px(2);
    };

    auto tmpl = addTemplates(sc, kind);
    if (!tmpl.empty()) {
        heading("From selection");
        for (const auto& [label, c] : tmpl) entry(label, nullptr, c);
        y += ui::px(6);
    }

    const char* lastCategory = "";
    for (const auto& type : scene::objectTypes()) {
        if (!(type.spaces & scene::spaceBit(kind))) continue;
        if (std::string(type.category) != lastCategory) {
            heading(type.category);
            lastCategory = type.category;
        }
        AddChoice c;
        c.typeId = type.id;
        entry(type.label, type.description, c);
    }
    if (tmpl.empty()) {
        y += ui::px(4);
        float th = ui::textWrapped({x + ui::px(8), y, width - ui::px(16), 0},
                                   "Tip: select objects first (Ctrl+click for several) to get templates such as "
                                   "a + b, cross(a, b) or M·v.",
                                   ui::FontRole::Small, t.textFaint);
        y += th + ui::px(4);
    }
    m_contentHeight = y - startY;

    ui::clearClip();
    EndScissorMode();
    if (picked) m_open = false;
    return picked;
}

} // namespace linalg
