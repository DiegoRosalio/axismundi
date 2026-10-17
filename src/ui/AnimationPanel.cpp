#include "ui/AnimationPanel.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

namespace linalg {

float AnimationPanel::drawContent(Rectangle area, Animator& animator, const Matrix4D& inputMatrix) {
    const ui::Theme& t = ui::theme();
    ui::Column col(area.x + ui::px(12.0f), area.y + ui::px(10.0f), area.width - ui::px(24.0f), 8.0f);

    // Row 1: transport controls and timeline
    Rectangle row = col.row(32.0f);
    Rectangle play = ui::takeLeft(row, 32.0f, 4.0f);
    Rectangle stop = ui::takeLeft(row, 32.0f, 12.0f);
    Rectangle timeText = ui::takeRight(row, 56.0f, 10.0f);

    if (ui::iconButton(play, animator.playing() ? ui::Icon::Pause : ui::Icon::Play, animator.playing())) {
        if (animator.active()) animator.togglePlay();
        else animator.start(AnimationPreset::Morph);
    }
    if (ui::iconButton(stop, ui::Icon::Stop, false, animator.active())) animator.stop();
    ui::tooltip(play, "Play or pause (P). With nothing selected, morphs from the identity to your matrix.",
                "Play / pause");
    ui::tooltip(stop, "Stop and show your matrix again.", "Stop");

    float tValue = animator.time();
    if (animator.active()) {
        if (ui::slider(row, tValue, 0.0f, 1.0f)) animator.setTime(tValue);
    } else {
        ui::textIn(row, "Pick an animation below, or press P to morph to your matrix",
                   ui::FontRole::Small, t.textMuted);
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "t %.2f", tValue);
    ui::textIn(timeText, buf, ui::FontRole::Mono, animator.active() ? t.text : t.textFaint, ui::Align::Right);

    // Row 2: presets, keyframes, speed
    row = col.row(32.0f);
    Rectangle presets = ui::takeLeft(row, 250.0f, 10.0f);
    int active = -1;
    switch (animator.preset()) {
        case AnimationPreset::Rotation: active = 0; break;
        case AnimationPreset::Scaling:  active = 1; break;
        case AnimationPreset::Morph:    active = 2; break;
        default: break;
    }
    int chosen = active;
    if (ui::segmented(presets, {"Rotate", "Scale", "Morph"}, chosen,
                      {"A full turn about the y-axis.", "Uniform scaling from 1× to 2×.",
                       "Blend from the identity to your matrix: watch the basis move to M's columns."}) ||
        (chosen >= 0 && chosen != active)) {
        const AnimationPreset map[3] = {AnimationPreset::Rotation, AnimationPreset::Scaling, AnimationPreset::Morph};
        animator.start(map[chosen]);
    }

    Rectangle addKey = ui::takeLeft(row, 76.0f, 4.0f);
    Rectangle clear = ui::takeLeft(row, 32.0f, 8.0f);
    if (ui::button(addKey, "Key", ui::ButtonKind::Secondary, true, ui::Icon::Plus)) {
        animator.addKeyframe(inputMatrix);
        if (animator.keyframes().size() >= 2 && animator.preset() != AnimationPreset::Keyframes) {
            animator.start(AnimationPreset::Keyframes);
        }
    }
    if (ui::iconButton(clear, ui::Icon::Close, false, !animator.keyframes().empty())) animator.clearKeyframes();
    ui::tooltip(addKey, "Capture the current matrix as a keyframe. With two or more, the animation moves "
                        "through them in order.", "Add keyframe");
    ui::tooltip(clear, "Remove all keyframes.", "Clear keyframes");

    Rectangle keyText = ui::takeLeft(row, 56.0f, 10.0f);
    snprintf(buf, sizeof(buf), "%zu key%s", animator.keyframes().size(), animator.keyframes().size() == 1 ? "" : "s");
    bool keysActive = animator.preset() == AnimationPreset::Keyframes;
    ui::textIn(keyText, buf, ui::FontRole::Small, keysActive ? t.accent : t.textMuted);

    // Row 3: repeat style, easing, speed
    row = col.row(32.0f);
    Rectangle repeatRect = ui::takeLeft(row, 250.0f, 10.0f);
    Repeat repeat = animator.repeat();
    if (ui::repeatControl(repeatRect, repeat)) animator.setRepeat(repeat);
    Rectangle easingRect = ui::takeLeft(row, 150.0f, 12.0f);
    if (ui::easingButton(easingRect, animator.easing(), m_easingOpen)) m_easingOpen = !m_easingOpen;
    if (row.width > ui::px(110.0f)) {
        Rectangle speedLabel = ui::takeLeft(row, 44.0f, 6.0f);
        Rectangle speedValue = ui::takeRight(row, 34.0f, 6.0f);
        ui::textIn(speedLabel, "Speed", ui::FontRole::Small, t.textMuted);
        float speed = animator.speed();
        if (ui::slider(row, speed, 0.1f, 4.0f)) animator.setSpeed(speed);
        snprintf(buf, sizeof(buf), "%.1f×", animator.speed());
        ui::textIn(speedValue, buf, ui::FontRole::Mono, t.textMuted, ui::Align::Right);
    }
    if (m_easingOpen) {
        Easing e = animator.easing();
        bool changed = false;
        float h = ui::easingGrid(col.x(), col.y(), col.width(), e, changed);
        if (changed) animator.setEasing(e);
        col.space(h / ui::scale() + 8.0f);
    }
    return col.y() - area.y + ui::px(2.0f);
}

void AnimationPanel::saveState(const Animator& animator, SceneData& data) {
    putFloat(data, "anim.speed", animator.speed());
    putBool(data, "anim.loop", animator.loop());
    putInt(data, "anim.repeat", static_cast<int>(animator.repeat()));
    putInt(data, "anim.easing", static_cast<int>(animator.easing()));
    putInt(data, "anim.keyframes", static_cast<int>(animator.keyframes().size()));
    for (size_t k = 0; k < animator.keyframes().size(); k++) {
        std::ostringstream row;
        const Matrix4D& m = animator.keyframes()[k];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) row << (i || j ? " " : "") << m.m[i][j];
        data["anim.key" + std::to_string(k)] = row.str();
    }
}

void AnimationPanel::loadState(Animator& animator, const SceneData& data) {
    float speed = animator.speed();
    if (getFloat(data, "anim.speed", speed)) animator.setSpeed(speed);
    bool loop = animator.loop();
    if (getBool(data, "anim.loop", loop)) animator.setLoop(loop);
    int v = 0;
    if (getInt(data, "anim.repeat", v) && v >= 0 && v < kRepeatCount) animator.setRepeat(static_cast<Repeat>(v));
    if (getInt(data, "anim.easing", v) && v >= 0 && v < kEasingCount) animator.setEasing(static_cast<Easing>(v));

    int count = 0;
    if (!getInt(data, "anim.keyframes", count)) return;
    animator.stop();
    animator.clearKeyframes();
    for (int k = 0; k < count; k++) {
        std::string row;
        if (!getString(data, "anim.key" + std::to_string(k), row)) continue;
        std::istringstream in(row);
        Matrix4D m;
        bool ok = true;
        for (int i = 0; i < 4 && ok; i++)
            for (int j = 0; j < 4 && ok; j++) ok = static_cast<bool>(in >> m.m[i][j]);
        if (ok) animator.addKeyframe(m);
    }
}

} // namespace linalg
