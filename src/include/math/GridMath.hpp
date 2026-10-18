#pragma once

#include <string>

// The numbers behind the adaptive grid and the axis numbers (no raylib, unit-tested): how many pixels one
// unit covers at the current zoom, which line spacing fits, how strongly each family of lines shows, and
// how to print a tick value.
namespace linalg::grid {

// Pixels per unit at a point `distance` away from a perspective camera (fovy in degrees)
double pixelsPerUnit(double distance, double fovyDegrees, double viewportHeight);

// The finest line spacing that is still at least `minPixels` apart on screen: a power of 10 (auto) or a
// fixed spacing. fade (0..1) says how far those lines have faded in: 0 right at the limit, 1 when they are
// `ratio` times wider apart than needed.
struct Level {
    double spacing = 1.0;
    double fade = 1.0;
    double ratio = 10.0;  // the next family of lines is `ratio` times wider apart
};
Level autoLevel(double pixelsPerUnit, double minPixels);
Level fixedLevel(double spacing, double ratio);  // fixed spacing, fully shown

// Strength (0..1) of the lines of family j = 0, 1, 2 (spacing · ratio^j): the finest family fades in with
// `fade`, wider families are stronger. A line that belongs to several families takes the strongest, which
// keeps the picture continuous when the spacing jumps to the next power of 10. The shader uses the same
// formula (rendering/GridShader.cpp).
double familyStrength(int j, double fade, double minor = 0.55, double major = 0.45);

// A step of 1, 2 or 5 × 10^k for the axis numbers, at least `minPixels` apart
double tickStep(double pixelsPerUnit, double minPixels);

// A tick value as text, with as many decimals as the step needs: 0.5, −2, 100 (never 0.30000001 or −0)
std::string formatTick(double value, double step);

} // namespace linalg::grid
