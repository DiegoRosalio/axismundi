#pragma once

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

// Time-based animation of a transformation matrix. Time t runs from 0 to 1
// over `duration / speed` seconds. Elements are interpolated linearly,
// except the rotation preset, which stays a true rotation at every t.
class Animator {
public:
    static constexpr float kDuration = 4.0f;  // seconds per run at 1x speed

    void start(AnimationPreset preset);
    void stop();
    void togglePlay();
    void update(float deltaTime);

    bool active() const { return m_preset != AnimationPreset::None; }
    bool playing() const { return m_playing; }
    AnimationPreset preset() const { return m_preset; }

    float time() const { return m_time; }
    void setTime(float t);

    float speed() const { return m_speed; }
    void setSpeed(float s);
    bool loop() const { return m_loop; }
    void setLoop(bool loop) { m_loop = loop; }

    // Target of the morph preset (the user's input matrix)
    void setMorphTarget(const Matrix4D& m) { m_morphTarget = m; }

    void addKeyframe(const Matrix4D& m) { m_keyframes.push_back(m); }
    void clearKeyframes();
    const std::vector<Matrix4D>& keyframes() const { return m_keyframes; }

    Matrix4D matrixAt(float t) const;
    Matrix4D current() const { return matrixAt(m_time); }

    static Matrix4D lerp(const Matrix4D& a, const Matrix4D& b, float t);

private:
    AnimationPreset m_preset = AnimationPreset::None;
    float m_time = 0.0f;
    float m_speed = 1.0f;
    bool m_playing = false;
    bool m_loop = true;
    Matrix4D m_morphTarget = Matrix4D::identity();
    std::vector<Matrix4D> m_keyframes;
};

const char* animationPresetName(AnimationPreset preset);

} // namespace linalg
