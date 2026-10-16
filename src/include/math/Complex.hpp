#pragma once

#include <ostream>
#include <string>

namespace linalg {

class Complex {
public:
    float re, im;

    Complex() : re(0.0f), im(0.0f) {}
    Complex(float re, float im = 0.0f) : re(re), im(im) {}

    Complex operator+(const Complex& o) const { return {re + o.re, im + o.im}; }
    Complex operator-(const Complex& o) const { return {re - o.re, im - o.im}; }
    Complex operator*(const Complex& o) const;
    Complex operator/(const Complex& o) const;
    Complex operator*(float s) const { return {re * s, im * s}; }
    Complex operator-() const { return {-re, -im}; }
    bool operator==(const Complex& o) const { return re == o.re && im == o.im; }

    float abs() const;            // modulus |z|
    float arg() const;            // argument in (-pi, pi]
    Complex conj() const { return {re, -im}; }

    static Complex fromPolar(float r, float theta);
    static Complex exp(const Complex& z);  // e^z
    static Complex expi(float theta) { return fromPolar(1.0f, theta); }  // e^(iθ)

    // "a + bi" and "r·e^(iθ)" (θ in degrees when degrees = true)
    std::string toRectString(int precision = 2) const;
    std::string toPolarString(int precision = 2, bool degrees = true) const;

    friend std::ostream& operator<<(std::ostream& os, const Complex& z);
};

} // namespace linalg
