#pragma once

#include "ui/SceneIO.hpp"
#include <string>

namespace linalg {

// What happens when a playing value reaches the end of its run
enum class Repeat {
    Once = 0,      // stop at the end
    Loop = 1,      // jump back to the start
    PingPong = 2,  // run backwards, then forwards again
};
constexpr int kRepeatCount = 3;

// How the value moves between start and end
enum class Easing {
    Linear = 0,
    EaseIn,      // starts slowly (quadratic)
    EaseOut,     // ends slowly
    EaseInOut,   // slow, fast, slow (cubic)
    Sine,        // gentle sine-shaped in-out
    Back,        // overshoots the end a little, then settles
    Elastic,     // springs past the end and wobbles
    Bounce,      // bounces at the end like a dropped ball
    Step,        // jumps in five steps
};
constexpr int kEasingCount = 9;

// Eased progress for u in [0, 1]; 0 -> 0 and 1 -> 1 for every easing
float ease(Easing easing, float u);
const char* easingName(Easing easing);
const char* easingDescription(Easing easing);
const char* repeatName(Repeat repeat);

// A run from 0 to 1 over `duration` seconds with a repeat style and easing
struct Playback {
    Repeat repeat = Repeat::Loop;
    Easing easing = Easing::Linear;
    float duration = 4.0f;  // seconds for one run
    float phase = 0.0f;     // raw progress in [0, 1]
    int direction = 1;      // +1 forwards, -1 backwards (ping-pong)
    bool playing = false;

    void restart();
    void toggle();
    void advance(float deltaSeconds);
    void setPhase(float u);
    float value() const { return ease(easing, phase); }
    // Raw phase after `seconds` from the start (pure: no state), for a clock you can scrub
    float phaseAt(double seconds) const;
    float valueAt(double seconds) const { return ease(easing, phaseAt(seconds)); }

    void save(SceneData& data, const std::string& prefix) const;
    void load(const SceneData& data, const std::string& prefix);
};

} // namespace linalg
