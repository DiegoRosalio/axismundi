#include "animation/Playback.hpp"
#include <algorithm>
#include <cmath>

namespace linalg {

namespace {
constexpr float kPi = 3.14159265358979f;

float bounceOut(float u) {
    const float n1 = 7.5625f, d1 = 2.75f;
    if (u < 1.0f / d1) return n1 * u * u;
    if (u < 2.0f / d1) {
        u -= 1.5f / d1;
        return n1 * u * u + 0.75f;
    }
    if (u < 2.5f / d1) {
        u -= 2.25f / d1;
        return n1 * u * u + 0.9375f;
    }
    u -= 2.625f / d1;
    return n1 * u * u + 0.984375f;
}
} // namespace

float ease(Easing easing, float u) {
    u = std::clamp(u, 0.0f, 1.0f);
    switch (easing) {
        case Easing::Linear: return u;
        case Easing::EaseIn: return u * u;
        case Easing::EaseOut: return 1.0f - (1.0f - u) * (1.0f - u);
        case Easing::EaseInOut:
            return u < 0.5f ? 4.0f * u * u * u : 1.0f - std::pow(-2.0f * u + 2.0f, 3.0f) / 2.0f;
        case Easing::Sine: return -(std::cos(kPi * u) - 1.0f) / 2.0f;
        case Easing::Back: {
            const float c1 = 1.70158f, c3 = c1 + 1.0f;
            float v = u - 1.0f;
            return 1.0f + c3 * v * v * v + c1 * v * v;
        }
        case Easing::Elastic: {
            if (u <= 0.0f || u >= 1.0f) return u;
            const float c4 = 2.0f * kPi / 3.0f;
            return std::pow(2.0f, -10.0f * u) * std::sin((10.0f * u - 0.75f) * c4) + 1.0f;
        }
        case Easing::Bounce: return bounceOut(u);
        case Easing::Step: return u >= 1.0f ? 1.0f : std::floor(u * 5.0f) / 5.0f;
    }
    return u;
}

const char* easingName(Easing easing) {
    switch (easing) {
        case Easing::Linear: return "Linear";
        case Easing::EaseIn: return "Ease in";
        case Easing::EaseOut: return "Ease out";
        case Easing::EaseInOut: return "Ease in-out";
        case Easing::Sine: return "Sine";
        case Easing::Back: return "Back";
        case Easing::Elastic: return "Elastic";
        case Easing::Bounce: return "Bounce";
        case Easing::Step: return "Steps";
    }
    return "";
}

const char* easingDescription(Easing easing) {
    switch (easing) {
        case Easing::Linear: return "Constant speed from start to end.";
        case Easing::EaseIn: return "Starts slowly and speeds up (quadratic).";
        case Easing::EaseOut: return "Starts fast and slows down at the end.";
        case Easing::EaseInOut: return "Slow at both ends, fast in the middle (cubic).";
        case Easing::Sine: return "A gentle in-out shaped like half a cosine wave.";
        case Easing::Back: return "Overshoots the end a little, then settles back.";
        case Easing::Elastic: return "Springs past the end and wobbles into place.";
        case Easing::Bounce: return "Bounces at the end like a dropped ball.";
        case Easing::Step: return "Jumps in five equal steps.";
    }
    return "";
}

const char* repeatName(Repeat repeat) {
    switch (repeat) {
        case Repeat::Once: return "Once";
        case Repeat::Loop: return "Loop";
        case Repeat::PingPong: return "Ping-pong";
    }
    return "";
}

void Playback::restart() {
    phase = 0.0f;
    direction = 1;
    playing = true;
}

void Playback::toggle() {
    if (!playing && repeat == Repeat::Once && phase >= 1.0f) {
        restart();
        return;
    }
    playing = !playing;
}

void Playback::setPhase(float u) {
    phase = std::clamp(u, 0.0f, 1.0f);
}

void Playback::advance(float dt) {
    if (!playing || duration <= 0.0f) return;
    phase += direction * dt / duration;
    switch (repeat) {
        case Repeat::Once:
            if (phase >= 1.0f) {
                phase = 1.0f;
                playing = false;
            } else if (phase < 0.0f) {
                phase = 0.0f;
            }
            break;
        case Repeat::Loop:
            if (phase >= 1.0f || phase < 0.0f) phase -= std::floor(phase);
            break;
        case Repeat::PingPong:
            // Reflect at the ends (a long frame may need several reflections)
            for (int i = 0; i < 8 && (phase > 1.0f || phase < 0.0f); i++) {
                if (phase > 1.0f) {
                    phase = 2.0f - phase;
                    direction = -1;
                } else {
                    phase = -phase;
                    direction = 1;
                }
            }
            phase = std::clamp(phase, 0.0f, 1.0f);
            break;
    }
}

void Playback::save(SceneData& data, const std::string& prefix) const {
    putInt(data, prefix + "repeat", static_cast<int>(repeat));
    putInt(data, prefix + "easing", static_cast<int>(easing));
    putFloat(data, prefix + "duration", duration);
}

void Playback::load(const SceneData& data, const std::string& prefix) {
    int v = 0;
    if (getInt(data, prefix + "repeat", v) && v >= 0 && v < kRepeatCount) repeat = static_cast<Repeat>(v);
    if (getInt(data, prefix + "easing", v) && v >= 0 && v < kEasingCount) easing = static_cast<Easing>(v);
    float d = duration;
    if (getFloat(data, prefix + "duration", d) && d > 0.05f) duration = d;
}

} // namespace linalg
