#include "math/Complex.hpp"
#include <cmath>
#include <cstdio>

namespace linalg {

Complex Complex::operator*(const Complex& o) const {
    return {re * o.re - im * o.im, re * o.im + im * o.re};
}

Complex Complex::operator/(const Complex& o) const {
    float d = o.re * o.re + o.im * o.im;
    return {(re * o.re + im * o.im) / d, (im * o.re - re * o.im) / d};
}

float Complex::abs() const {
    return std::hypot(re, im);
}

float Complex::arg() const {
    return std::atan2(im, re);
}

Complex Complex::fromPolar(float r, float theta) {
    return {r * std::cos(theta), r * std::sin(theta)};
}

Complex Complex::exp(const Complex& z) {
    return fromPolar(std::exp(z.re), z.im);
}

std::string Complex::toRectString(int precision) const {
    char buf[64];
    char sign = (im < 0.0f) ? '-' : '+';
    snprintf(buf, sizeof(buf), "%.*f %c %.*fi", precision, re, sign, precision, std::fabs(im));
    return buf;
}

std::string Complex::toPolarString(int precision, bool degrees) const {
    char buf[64];
    float theta = arg();
    if (degrees) {
        snprintf(buf, sizeof(buf), "%.*f e^(i %.*f deg)", precision, abs(), precision,
                 theta * 180.0f / 3.14159265f);
    } else {
        snprintf(buf, sizeof(buf), "%.*f e^(i %.*f)", precision, abs(), precision, theta);
    }
    return buf;
}

std::ostream& operator<<(std::ostream& os, const Complex& z) {
    return os << z.toRectString(4);
}

} // namespace linalg
