#include "ui/TimePanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace linalg {

namespace {

std::string shortNumber(double v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.3g", std::fabs(v) < 5e-4 ? 0.0 : v);
    return buf;
}

// "animate · ping-pong · bounce · 4 s" or "f(t) = pi/4*cos(2t)"
std::string describe(const scene::ParameterObject& p) {
    if (p.mode() == scene::ParameterObject::Mode::Formula) {
        const scene::Field* f = p.field("formula");
        return "f(t) = " + (f ? f->text : std::string());
    }
    Playback pb = p.playback();
    std::string s = std::string("animate · ") + repeatName(pb.repeat) + " · " + easingName(pb.easing) + " · " +
                    shortNumber(pb.duration) + " s";
    if (p.startDelay() > 0.0) s += " from " + shortNumber(p.startDelay()) + " s";
    return s;
}

} // namespace

void TimePanel::togglePlay(scene::Scene& sc) {
    scene::Timeline& tl = sc.timeline();
    if (!tl.playing && !tl.loop && tl.time >= sc.timelineLength() - 1e-6) tl.time = 0.0;
    tl.playing = !tl.playing;
}

float TimePanel::drawContent(Rectangle area, scene::Scene& sc, scene::SpaceKind kind) {
    const ui::Theme& t = ui::theme();
    scene::Timeline& tl = sc.timeline();
    const double length = sc.timelineLength();
    ui::Column col(area.x + ui::px(12.0f), area.y + ui::px(10.0f), area.width - ui::px(24.0f), 8.0f);

    // Row 1: transport and scrubber
    Rectangle row = col.row(32.0f);
    Rectangle play = ui::takeLeft(row, 32.0f, 4.0f);
    Rectangle restart = ui::takeLeft(row, 32.0f, 12.0f);
    Rectangle readout = ui::takeRight(row, 104.0f, 10.0f);
    if (ui::iconButton(play, tl.playing ? ui::Icon::Pause : ui::Icon::Play, tl.playing)) togglePlay(sc);
    if (ui::iconButton(restart, ui::Icon::Reset)) tl.time = 0.0;
    ui::anchor("time.play", play);
    ui::tooltip(play, "Play or pause the scene clock (P). Parameters set to Animate or Formula follow it; "
                      "sliders stay where they are.", "Play / pause");
    ui::tooltip(restart, "Back to t = 0.", "Restart");
    float tv = static_cast<float>(std::min(tl.time, length));
    if (ui::slider(row, tv, 0.0f, static_cast<float>(length))) tl.time = tv;
    ui::anchor("time.scrubber", row);
    ui::tooltip(row, "Drag to see any moment: time-driven parameters jump to their value at that time.", "Scene time");
    char buf[96];
    snprintf(buf, sizeof(buf), "t %.2f / %.3g s", tl.time, length);
    ui::textIn(readout, buf, ui::FontRole::Mono, t.text, ui::Align::Right);

    // Row 2: speed, loop, length
    row = col.row(32.0f);
    Rectangle loopBtn = ui::takeLeft(row, 32.0f, 12.0f);
    if (ui::iconButton(loopBtn, ui::Icon::Loop, tl.loop)) tl.loop = !tl.loop;
    ui::tooltip(loopBtn, tl.loop ? "At the end, start again from 0 (on)." : "At the end, stop (loop is off).",
                "Loop");
    Rectangle speedLabel = ui::takeLeft(row, 44.0f, 6.0f);
    ui::textIn(speedLabel, "Speed", ui::FontRole::Small, t.textMuted);
    Rectangle speedRect = ui::takeLeft(row, 120.0f, 6.0f);
    Rectangle speedValue = ui::takeLeft(row, 36.0f, 14.0f);
    if (ui::slider(speedRect, tl.speed, 0.1f, 4.0f)) tl.speed = std::round(tl.speed * 10.0f) / 10.0f;
    snprintf(buf, sizeof(buf), "%.1f×", tl.speed);
    ui::textIn(speedValue, buf, ui::FontRole::Mono, t.textMuted);
    Rectangle lengthLabel = ui::takeLeft(row, 50.0f, 6.0f);
    ui::textIn(lengthLabel, "Length", ui::FontRole::Small, t.textMuted);
    Rectangle autoBtn = ui::takeLeft(row, 56.0f, 8.0f);
    bool automatic = tl.length <= 0.0;
    if (ui::button(autoBtn, "Auto", automatic ? ui::ButtonKind::Primary : ui::ButtonKind::Secondary)) {
        tl.length = automatic ? std::ceil(sc.autoLength()) : 0.0;
    }
    ui::tooltip(autoBtn, "Auto: as long as the longest Once or Ping-pong animation (10 s if there is none). "
                         "Click to set a length by hand.", "Length");
    if (row.width > ui::px(60.0f)) {
        Rectangle lengthValue = ui::takeRight(row, 40.0f, 6.0f);
        float len = static_cast<float>(length);
        if (!automatic && ui::slider(row, len, 1.0f, 120.0f)) tl.length = std::round(len);
        else if (automatic) ui::slider(row, len, 1.0f, 120.0f);  // shows the automatic length
        snprintf(buf, sizeof(buf), "%.3gs", length);
        ui::textIn(lengthValue, buf, ui::FontRole::Mono, automatic ? t.textFaint : t.textMuted, ui::Align::Right);
    }
    if (tl.time > length) tl.time = length;

    // Parameters that follow the clock in this space
    col.space(2.0f);
    scene::Space& space = sc.space(kind);
    int listed = 0;
    for (const auto& o : space.objects()) {
        const auto* p = dynamic_cast<const scene::ParameterObject*>(o.get());
        if (!p || !p->timeDriven()) continue;
        listed++;
        Rectangle r = col.row(30.0f);
        bool selected = sc.selection(kind).contains(p->id());
        bool hover = ui::hovered(r);
        if (selected || hover) ui::fillRounded(r, ui::px(6.0f), selected ? t.accentSoft : t.hover);
        if (ui::clicked(r)) sc.selection(kind).set(p->id());
        Rectangle inner = {r.x + ui::px(8), r.y, r.width - ui::px(16), r.height};
        Rectangle name = ui::takeLeft(inner, 70.0f, 8.0f);
        Rectangle value = ui::takeRight(inner, 70.0f, 8.0f);
        ui::textIn(name, ui::ellipsize(p->name().c_str(), ui::FontRole::Body, name.width).c_str(), ui::FontRole::Body,
                   t.text);
        ui::textIn(inner, ui::ellipsize(describe(*p).c_str(), ui::FontRole::Small, inner.width).c_str(),
                   ui::FontRole::Small, p->error().empty() ? t.textMuted : t.danger);
        ui::textIn(value, p->error().empty() ? shortNumber(p->number()).c_str() : "error", ui::FontRole::Mono,
                   p->error().empty() ? t.text : t.danger, ui::Align::Right);
        ui::tooltip(r, p->error().empty() ? "Click to select it and edit it in the inspector." : p->error().c_str(),
                    p->name().c_str());
    }
    if (listed == 0) {
        ui::textWrapped({col.x(), col.y(), col.width(), ui::px(40)},
                        "No time-driven parameters here. Add a parameter and set it to Animate or Formula; "
                        "sliders stay static.",
                        ui::FontRole::Small, t.textMuted);
        col.space(36.0f);
    }
    return col.y() - area.y + ui::px(2.0f);
}

} // namespace linalg
