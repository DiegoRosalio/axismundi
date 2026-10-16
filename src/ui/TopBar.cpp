#include "ui/TopBar.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"

namespace linalg {

float TopBar::height() { return ui::px(52.0f); }

TopBarActions TopBar::draw(AppMode& mode, bool helpVisible) {
    const ui::Theme& t = ui::theme();
    TopBarActions actions;
    float w = static_cast<float>(GetScreenWidth());
    float h = height();

    Color bg = t.panel;
    bg.a = 255;
    DrawRectangleRec({0, 0, w, h}, bg);
    DrawRectangleRec({0, h - ui::pxi(1.0f), w, ui::pxi(1.0f)}, t.border);

    Rectangle bar = {ui::px(ui::kPad), 0, w - 2 * ui::px(ui::kPad), h};

    // Brand: a small accent mark and the name
    Rectangle mark = ui::takeLeft(bar, 22.0f, 10.0f);
    float m = ui::px(18.0f);
    Rectangle markBox = {mark.x, (h - m) / 2, m, m};
    ui::fillRounded(markBox, ui::px(5.0f), t.accent);
    ui::drawIcon(ui::Icon::Layers, ui::inset(markBox, 3.0f), t.onAccent);
    Rectangle name = ui::takeLeft(bar, 104.0f, 18.0f);
    ui::textIn(name, "axismundi", ui::FontRole::Title, t.text);

    // Mode tabs
    Rectangle tabs = ui::takeLeft(bar, 330.0f, 16.0f);
    tabs.y = (h - ui::px(34.0f)) / 2;
    tabs.height = ui::px(34.0f);
    int active = static_cast<int>(mode);
    if (ui::segmented(tabs, {"Matrices", "Complex", "Plots"}, active)) mode = static_cast<AppMode>(active);

    // Actions on the right
    float btn = ui::px(36.0f);
    auto rightButton = [&](ui::Icon icon, bool selected) {
        Rectangle r = ui::takeRight(bar, 36.0f, 4.0f);
        r.y = (h - btn) / 2;
        r.height = btn;
        return ui::iconButton(r, icon, selected);
    };
    if (rightButton(ui::Icon::Help, helpVisible)) actions.toggleHelp = true;
    if (rightButton(t.dark ? ui::Icon::Sun : ui::Icon::Moon, false)) actions.toggleTheme = true;

    return actions;
}

} // namespace linalg
