#pragma once

#include "animation/Playback.hpp"
#include "math/Matrix4D.hpp"
#include <vector>

namespace linalg {

enum class AnimationPreset {
    None,       // no animation: the input matrix is shown as is
    Rotation,   // full turn about the Y axis
    Scaling,    // uniform scale from 1x to 2x
    Morph,      // identity -> current input matrix
    Keyframes,  // through user-captured matrices
};

// Time-based animation of a transformation matrix. Progress runs from 0 to 1
// over `kDuration / speed` seconds with a repeat style (once, loop, ping-pong)
// and an easing curve. Elements are interpolated linearly, except the rotation
// preset, which stays a true rotation at every t.
class Animator {
public:
    static constexpr float kDuration = 4.0f;  // seconds per run at 1x speed

    void start(AnimationPreset preset);
    void stop();
    void togglePlay();
    void update(float deltaTime);

    bool active() const { return m_preset != AnimationPreset::None; }
    bool playing() const { return m_playback.playing; }
    AnimationPreset preset() const { return m_preset; }

    float time() const { return m_playback.phase; }  // raw progress in [0, 1]
    void setTime(float t);

    float speed() const { return m_speed; }
    void setSpeed(float s);
    bool loop() const { return m_playback.repeat != Repeat::Once; }
    void setLoop(bool loop) { m_playback.repeat = loop ? Repeat::Loop : Repeat::Once; }
    Repeat repeat() const { return m_playback.repeat; }
    void setRepeat(Repeat r) { m_playback.repeat = r; }
    Easing easing() const { return m_playback.easing; }
    void setEasing(Easing e) { m_playback.easing = e; }

    // Target of the morph preset (the user's input matrix)
    void setMorphTarget(const Matrix4D& m) { m_morphTarget = m; }

    void addKeyframe(const Matrix4D& m) { m_keyframes.push_back(m); }
    void clearKeyframes();
    const std::vector<Matrix4D>& keyframes() const { return m_keyframes; }

    Matrix4D matrixAt(float t) const;
    // Matrix at the current (eased) progress
    Matrix4D current() const { return matrixAt(m_playback.value()); }

    static Matrix4D lerp(const Matrix4D& a, const Matrix4D& b, float t);

private:
    AnimationPreset m_preset = AnimationPreset::None;
    Playback m_playback;  // duration kDuration; speed scales time
    float m_speed = 1.0f;
    Matrix4D m_morphTarget = Matrix4D::identity();
    std::vector<Matrix4D> m_keyframes;
};

const char* animationPresetName(AnimationPreset preset);

} // namespace linalg
