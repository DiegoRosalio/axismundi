#include "math/GridMath.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg::grid {

double pixelsPerUnit(double distance, double fovyDegrees, double viewportHeight) {
    double visible = 2.0 * std::max(distance, 1e-6) * std::tan(fovyDegrees * 3.14159265358979 / 360.0);
    return viewportHeight / visible;
}

Level autoLevel(double pixelsPerUnit, double minPixels) {
    double target = minPixels / std::max(pixelsPerUnit, 1e-9);  // the closest the lines may be, in units
    double exponent = std::log10(target);
    Level l;
    l.spacing = std::pow(10.0, std::ceil(exponent));
    l.fade = std::clamp(std::log10(l.spacing) - exponent, 0.0, 1.0);  // 0 at the limit, 1 ten times wider
    l.ratio = 10.0;
    return l;
}

Level fixedLevel(double spacing, double ratio) {
    Level l;
    l.spacing = spacing;
    l.fade = 1.0;
    l.ratio = ratio;
    return l;
}

double familyStrength(int j, double fade, double minor, double major) {
    // Family j sits j powers of `ratio` above the finest one: its "level" is j + fade.
    // Up to level 1 it fades in to `minor`; from 1 to 2 it gains `major` more.
    double level = j + std::clamp(fade, 0.0, 1.0);
    return minor * std::clamp(level, 0.0, 1.0) + major * std::clamp(level - 1.0, 0.0, 1.0);
}

double tickStep(double pixelsPerUnit, double minPixels) {
    double target = minPixels / std::max(pixelsPerUnit, 1e-9);
    double base = std::pow(10.0, std::floor(std::log10(target)));
    for (double m : {1.0, 2.0, 5.0, 10.0})
        if (m * base >= target * (1 - 1e-9)) return m * base;
    return 10.0 * base;
}

std::string formatTick(double value, double step) {
    int decimals = std::max(0, static_cast<int>(-std::floor(std::log10(step) + 1e-9)));
    double rounded = std::round(value / step) * step;
    if (std::fabs(rounded) < step * 1e-6) rounded = 0.0;
    char buf[48];
    snprintf(buf, sizeof(buf), "%.*f", decimals, std::fabs(rounded));
    std::string text = buf;
    // 1.0 -> 1, 2.50 -> 2.5 (a 0.5 step still prints whole numbers without decimals)
    if (text.find('.') != std::string::npos) {
        while (text.back() == '0') text.pop_back();
        if (text.back() == '.') text.pop_back();
    }
    return rounded < 0 ? "−" + text : text;
}

} // namespace linalg::grid
