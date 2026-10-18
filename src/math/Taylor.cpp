#include "math/Taylor.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg::taylor {

namespace {

constexpr double kTwoPi = 6.28318530717958647692;
constexpr int kSamples = 64;

enum class Circle { Ok, NotFinite, NoComplex };

// c_k = (1/M) Σ f(a + r wʲ) w^(−jk) / r^k on a circle of radius r (w = e^(2πi/M)); also the largest |f| seen
Circle circleCoefficients(const ComplexFunction& fc, double a, double r, int n, std::vector<cplx>& c, double& maxAbs) {
    std::vector<cplx> samples(kSamples);
    maxAbs = 0.0;
    for (int j = 0; j < kSamples; j++) {
        cplx z = a + std::polar(r, kTwoPi * j / kSamples), v;
        if (!fc(z, v)) return Circle::NoComplex;
        if (!std::isfinite(v.real()) || !std::isfinite(v.imag())) return Circle::NotFinite;
        samples[static_cast<size_t>(j)] = v;
        maxAbs = std::max(maxAbs, std::abs(v));
    }
    c.assign(static_cast<size_t>(n) + 1, 0.0);
    for (int k = 0; k <= n; k++) {
        cplx sum = 0.0;
        for (int j = 0; j < kSamples; j++) sum += samples[static_cast<size_t>(j)] * std::polar(1.0, -kTwoPi * j * k / kSamples);
        c[static_cast<size_t>(k)] = sum / static_cast<double>(kSamples) / std::pow(r, k);
    }
    return Circle::Ok;
}

// Degrees 0..4 by central differences (step h = 0.01)
bool finiteDifferences(const RealFunction& f, double a, int n, Series& out) {
    const double h = 1e-2;
    double v[5];  // f(a − 2h) ... f(a + 2h)
    for (int j = -2; j <= 2; j++)
        if (!f(a + j * h, v[j + 2])) return false;
    double d[5] = {v[2], (v[0] - 8 * v[1] + 8 * v[3] - v[4]) / (12 * h),
                   (-v[0] + 16 * v[1] - 30 * v[2] + 16 * v[3] - v[4]) / (12 * h * h),
                   (-v[0] + 2 * v[1] - 2 * v[3] + v[4]) / (2 * h * h * h),
                   (v[0] - 4 * v[1] + 6 * v[2] - 4 * v[3] + v[4]) / (h * h * h * h)};
    const double factorial[5] = {1, 1, 2, 6, 24};
    int top = std::min(n, 4);
    out.coeffs.clear();
    for (int k = 0; k <= top; k++) out.coeffs.push_back(d[k] / factorial[k]);
    return true;
}

}  // namespace

bool coefficients(const ComplexFunction& fc, const RealFunction& f, double a, int n, Series& out) {
    out = Series{};
    out.center = a;
    n = std::clamp(n, 0, kMaxDegree);
    out.requested = n;
    double fa = 0.0;
    if (!f(a, fa) || !std::isfinite(fa)) return false;

    // The complex method: the largest radius whose coefficients a circle half as big confirms
    bool complexUsable = true;
    for (double r = 1.0; r > 0.03 && complexUsable; r *= 0.5) {
        std::vector<cplx> big, small;
        double maxBig = 0, maxSmall = 0;
        Circle cb = circleCoefficients(fc, a, r, n, big, maxBig);
        if (cb == Circle::NoComplex) complexUsable = false;
        if (cb != Circle::Ok) continue;
        Circle cs = circleCoefficients(fc, a, r / 2, std::min(n, 3), small, maxSmall);
        if (cs == Circle::NoComplex) complexUsable = false;
        if (cs != Circle::Ok) continue;
        double scale = 1.0 + std::fabs(fa);
        bool agree = std::abs(big[0] - fa) < 1e-8 * scale;  // the mean over the circle is f(a)
        for (size_t k = 0; k < small.size() && agree; k++)
            agree = std::abs(big[k] - small[k]) < 1e-6 * (1.0 + std::abs(big[k])) && std::fabs(big[k].imag()) < 1e-6 * (1.0 + std::abs(big[k]));
        if (!agree) continue;
        // Rounding grows like 1/r^k: keep the degrees it doesn't spoil
        out.coeffs.clear();
        for (int k = 0; k <= n; k++) {
            double noise = 1e-14 * std::max(1.0, maxBig) / std::pow(r, k);
            if (noise > 1e-5 * std::max(1.0, std::abs(big[static_cast<size_t>(k)]))) break;
            out.coeffs.push_back(big[static_cast<size_t>(k)].real());
        }
        out.complexMethod = true;
        out.radius = r;
        char buf[160];
        if (static_cast<int>(out.coeffs.size()) <= n)
            snprintf(buf, sizeof(buf), "degree capped at %d: f changes too fast near the center (a singularity is close)",
                     static_cast<int>(out.coeffs.size()) - 1);
        else
            snprintf(buf, sizeof(buf), "coefficients from f on a circle of radius %g around the center", r);
        out.note = buf;
        return true;
    }

    // Finite differences, degrees 0..4
    if (!finiteDifferences(f, a, n, out)) return false;
    out.complexMethod = false;
    out.note = complexUsable ? "f isn't smooth enough around the center for high degrees: degrees up to 4 from nearby values"
                             : "degrees up to 4 from nearby values (higher ones need a formula that also works for complex x)";
    return true;
}

double evaluate(const Series& s, double x, int n) {
    int top = n < 0 ? static_cast<int>(s.coeffs.size()) - 1 : std::min(n, static_cast<int>(s.coeffs.size()) - 1);
    // Horner's rule: c0 + d(c1 + d(c2 + ...))
    double d = x - s.center, y = 0.0;
    for (int k = top; k >= 0; k--) y = y * d + s.coeffs[static_cast<size_t>(k)];
    return y;
}

NewtonRun newton(const RealFunction& f, double x0, int maxSteps) {
    NewtonRun run;
    double x = x0;
    for (int k = 0;; k++) {
        double fx = 0.0, slope = 0.0;
        if (!f(x, fx) || !derivatives::derivative1(f, x, derivatives::stepFor(1.0 + std::fabs(x)), slope)) {
            run.stop = NewtonStop::Undefined;
            if (run.steps.empty() || run.steps.back().x != x) run.steps.push_back({x, NAN, NAN});
            return run;
        }
        run.steps.push_back({x, fx, slope});
        if (k >= maxSteps) {
            run.stop = NewtonStop::Steps;
            return run;
        }
        if (std::fabs(slope) < 1e-12) {
            run.stop = NewtonStop::FlatTangent;
            return run;
        }
        double next = x - fx / slope;
        if (!std::isfinite(next) || std::fabs(next) > 1e6) {
            run.stop = NewtonStop::Diverging;
            if (std::isfinite(next)) run.steps.push_back({next, NAN, NAN});
            return run;
        }
        if (std::fabs(next - x) < 1e-14 * (1.0 + std::fabs(x))) {
            run.stop = NewtonStop::Converged;
            return run;
        }
        x = next;
    }
}

} // namespace linalg::taylor
