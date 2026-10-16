#include "animation/Animator.hpp"
#include <algorithm>
#include <cmath>

namespace linalg {

void Animator::start(AnimationPreset preset) {
    m_preset = preset;
    m_time = 0.0f;
    m_playing = (preset != AnimationPreset::None);
}

void Animator::stop() {
    m_preset = AnimationPreset::None;
    m_playing = false;
    m_time = 0.0f;
}

void Animator::togglePlay() {
    if (!active()) return;
    if (!m_playing && m_time >= 1.0f) m_time = 0.0f;  // replay from the start
    m_playing = !m_playing;
}

void Animator::update(float deltaTime) {
    if (!m_playing) return;
    m_time += deltaTime * m_speed / kDuration;
    if (m_time >= 1.0f) {
        if (m_loop) {
            m_time = std::fmod(m_time, 1.0f);
        } else {
            m_time = 1.0f;
            m_playing = false;
        }
    }
}

void Animator::setTime(float t) {
    m_time = std::clamp(t, 0.0f, 1.0f);
}

void Animator::setSpeed(float s) {
    m_speed = std::clamp(s, 0.1f, 4.0f);
}

void Animator::clearKeyframes() {
    m_keyframes.clear();
    if (m_preset == AnimationPreset::Keyframes) stop();
}

Matrix4D Animator::lerp(const Matrix4D& a, const Matrix4D& b, float t) {
    Matrix4D r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = a.m[i][j] + (b.m[i][j] - a.m[i][j]) * t;
    return r;
}

Matrix4D Animator::matrixAt(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (m_preset) {
        case AnimationPreset::None:
            return m_morphTarget;
        case AnimationPreset::Rotation:
            return Matrix4D::rotationY(t * 2.0f * 3.14159265f);
        case AnimationPreset::Scaling: {
            float s = 1.0f + t;
            return Matrix4D::scale(s, s, s);
        }
        case AnimationPreset::Morph:
            return lerp(Matrix4D::identity(), m_morphTarget, t);
        case AnimationPreset::Keyframes: {
            if (m_keyframes.empty()) return Matrix4D::identity();
            if (m_keyframes.size() == 1) return m_keyframes[0];
            // Evenly spaced keyframes over [0, 1]
            float segments = static_cast<float>(m_keyframes.size() - 1);
            float pos = t * segments;
            size_t i = std::min(static_cast<size_t>(pos), m_keyframes.size() - 2);
            return lerp(m_keyframes[i], m_keyframes[i + 1], pos - static_cast<float>(i));
        }
    }
    return Matrix4D::identity();
}

const char* animationPresetName(AnimationPreset preset) {
    switch (preset) {
        case AnimationPreset::None:      return "none";
        case AnimationPreset::Rotation:  return "rotation";
        case AnimationPreset::Scaling:   return "scaling";
        case AnimationPreset::Morph:     return "morph";
        case AnimationPreset::Keyframes: return "keyframes";
    }
    return "none";
}

} // namespace linalg
