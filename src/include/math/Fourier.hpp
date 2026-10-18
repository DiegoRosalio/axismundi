#pragma once

#include <complex>
#include <functional>
#include <vector>

namespace linalg::fourier {

// Fourier series (no raylib): the coefficients of a closed curve z(u) in the complex plane
// (epicycles), and the sine/cosine series of a real function on an interval.

using cplx = std::complex<double>;

// c_k for a closed curve z(u), u in [0, 2π): z(u) = Σ c_k e^{iku}
struct Term {
    int k = 0;      // frequency (turns per lap, negative: clockwise)
    cplx c;         // coefficient: |c| is the circle's radius, arg c its starting angle
};

// Coefficients for k = −maxK..maxK from M equally spaced samples z(2πj/M) (a discrete Fourier transform),
// sorted by size (largest first), with c_0 (the center) always first.
std::vector<Term> curveCoefficients(const std::vector<cplx>& samples, int maxK);

// Σ c_k e^{iku} over the first n terms (n < 0: all)
cplx evaluate(const std::vector<Term>& terms, double u, int n = -1);

// The chain of circle centers at angle u: centers[0] = c_0, then each term added in order;
// the last point is the tip (= evaluate(terms, u, n))
std::vector<cplx> chain(const std::vector<Term>& terms, double u, int n);

// f(x) ≈ a₀/2 + Σ a_k cos(kωx) + b_k sin(kωx), ω = 2π/(b − a), from f on [a, b] (repeated with period
// b − a; on [−π, π] these are the textbook coefficients)
struct Series {
    double a = -3.14159265358979, b = 3.14159265358979;
    double a0 = 0.0;
    std::vector<double> ak, bk;  // index k − 1 for k = 1..N
    double omega() const { return 2.0 * 3.14159265358979323846 / (b - a); }
};

// N terms by Simpson's rule with `samples` intervals (made even). ok is false where f is undefined
// (those points count as 0, so a jump at a single point does not matter).
Series seriesCoefficients(const std::function<bool(double x, double& y)>& f, double a, double b, int N,
                          int samples, int& undefinedPoints);

// The partial sum with the first n terms (n < 0: all); periodic outside [a, b]
double partialSum(const Series& s, double x, int n = -1);
// One harmonic: a_k cos(kωx) + b_k sin(kωx)
double harmonic(const Series& s, int k, double x);

} // namespace linalg::fourier
