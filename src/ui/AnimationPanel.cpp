#include "ui/AnimationPanel.hpp"
#include "ui/Widgets.hpp"
#include <raygui.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>

namespace linalg {

void AnimationPanel::draw(Animator& animator, const Matrix4D& inputMatrix, float left, float right) {
    const float minWidth = 640.0f;
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());

    // Use the space between the panels, widening over the info panel if needed
    float width = right - left;
    if (width < minWidth) width = std::min(minWidth, screenW - left - 10.0f);
    Rectangle bar = {left, screenH - kHeight - 10.0f, width, kHeight};

    DrawRectangleRec(bar, Fade(RAYWHITE, 0.92f));
    DrawRectangleLinesEx(bar, 1.0f, GRAY);
    setGuiTextSize(kUiTextSmall);

    float x = bar.x + 10.0f;
    float y = bar.y + 10.0f;
    const float h = 36.0f;

    // Row 1: transport controls and timeline
    const char* playLabel = animator.playing() ? "Pause [P]" : "Play [P]";
    if (GuiButton({x, y, 110, h}, playLabel)) {
        if (animator.active()) animator.togglePlay();
        else animator.start(AnimationPreset::Morph);
    }
    if (!animator.active()) GuiDisable();
    if (GuiButton({x + 118, y, 70, h}, "Stop")) animator.stop();
    GuiEnable();

    bool loop = animator.loop();
    GuiCheckBox({x + 200, y + 8, 20, 20}, "Loop", &loop);
    animator.setLoop(loop);

    float t = animator.time();
    char timeText[32];
    snprintf(timeText, sizeof(timeText), "t = %.2f", t);
    float sliderX = x + 290;
    float sliderW = bar.x + bar.width - sliderX - 100;
    if (!animator.active()) GuiDisable();
    GuiSlider({sliderX, y + 4, sliderW, h - 8}, nullptr, timeText, &t, 0.0f, 1.0f);
    GuiEnable();
    if (t != animator.time()) animator.setTime(t);

    // Row 2: presets, keyframes, speed
    y += h + 12.0f;
    struct PresetButton { const char* label; AnimationPreset preset; };
    const PresetButton presets[] = {
        {"Rotate", AnimationPreset::Rotation},
        {"Scale", AnimationPreset::Scaling},
        {"Morph", AnimationPreset::Morph},
    };
    float bx = x;
    for (const auto& p : presets) {
        bool current = animator.preset() == p.preset;
        if (current) GuiSetState(STATE_PRESSED);
        if (GuiButton({bx, y, 80, h}, p.label)) animator.start(p.preset);
        GuiSetState(STATE_NORMAL);
        bx += 86.0f;
    }

    bx += 8.0f;
    if (GuiButton({bx, y, 80, h}, "+ Key")) {
        animator.addKeyframe(inputMatrix);
        if (animator.keyframes().size() >= 2 && animator.preset() != AnimationPreset::Keyframes) {
            animator.start(AnimationPreset::Keyframes);
        }
    }
    bx += 86.0f;
    if (animator.keyframes().empty()) GuiDisable();
    if (GuiButton({bx, y, 70, h}, "Clear")) animator.clearKeyframes();
    GuiEnable();
    bx += 76.0f;

    char keyText[32];
    snprintf(keyText, sizeof(keyText), "%zu keys", animator.keyframes().size());
    bool keysActive = animator.preset() == AnimationPreset::Keyframes;
    DrawText(keyText, static_cast<int>(bx), static_cast<int>(y + 9), kUiTextSmall, keysActive ? DARKBLUE : GRAY);
    bx += 80.0f;

    float speed = animator.speed();
    char speedText[32];
    snprintf(speedText, sizeof(speedText), "%.1fx", speed);
    float speedW = bar.x + bar.width - bx - 60.0f;
    if (speedW > 60.0f) {
        GuiSlider({bx, y + 4, speedW, h - 8}, nullptr, speedText, &speed, 0.1f, 4.0f);
        animator.setSpeed(speed);
    }

    setGuiTextSize(kUiTextLarge);
}

void AnimationPanel::saveState(const Animator& animator, SceneData& data) {
    putFloat(data, "anim.speed", animator.speed());
    putBool(data, "anim.loop", animator.loop());
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
