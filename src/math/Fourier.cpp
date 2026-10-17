#include "math/Fourier.hpp"
#include <algorithm>
#include <cmath>

namespace linalg::fourier {

namespace {
constexpr double kTwoPi = 6.28318530717958647692;
}

std::vector<Term> curveCoefficients(const std::vector<cplx>& samples, int maxK) {
    std::vector<Term> terms;
    const size_t M = samples.size();
    if (M == 0) return terms;
    maxK = std::max(0, maxK);
    for (int k = -maxK; k <= maxK; k++) {
        cplx sum = 0.0;
        for (size_t j = 0; j < M; j++) {
            double u = kTwoPi * static_cast<double>(j) / static_cast<double>(M);
            sum += samples[j] * std::polar(1.0, -k * u);
        }
        terms.push_back({k, sum / static_cast<double>(M)});
    }
    // Center first, then by radius (largest first; ties: lower |k| first, so the order is stable)
    std::stable_sort(terms.begin(), terms.end(), [](const Term& x, const Term& y) {
        if (x.k == 0 || y.k == 0) return x.k == 0 && y.k != 0;
        double ax = std::abs(x.c), ay = std::abs(y.c);
        if (std::fabs(ax - ay) > 1e-12) return ax > ay;
        return std::abs(x.k) < std::abs(y.k);
    });
    return terms;
}

cplx evaluate(const std::vector<Term>& terms, double u, int n) {
    size_t count = n < 0 ? terms.size() : std::min(terms.size(), static_cast<size_t>(n));
    cplx z = 0.0;
    for (size_t i = 0; i < count; i++) z += terms[i].c * std::polar(1.0, terms[i].k * u);
    return z;
}

std::vector<cplx> chain(const std::vector<Term>& terms, double u, int n) {
    size_t count = n < 0 ? terms.size() : std::min(terms.size(), static_cast<size_t>(n));
    std::vector<cplx> points;
    cplx z = 0.0;
    for (size_t i = 0; i < count; i++) {
        z += terms[i].c * std::polar(1.0, terms[i].k * u);
        points.push_back(z);
    }
    return points;
}

Series seriesCoefficients(const std::function<bool(double x, double& y)>& f, double a, double b, int N,
                          int samples, int& undefinedPoints) {
    Series s;
    s.a = a;
    s.b = b;
    undefinedPoints = 0;
    N = std::max(0, N);
    samples = std::max(2, samples + (samples % 2));
    double h = (b - a) / samples, w = s.omega();
    std::vector<double> ys(static_cast<size_t>(samples) + 1);
    for (int j = 0; j <= samples; j++) {
        double y = 0;
        if (!f(a + j * h, y) || !std::isfinite(y)) {
            y = 0;
            undefinedPoints++;
        }
        ys[static_cast<size_t>(j)] = y;
    }
    auto simpson = [&](const std::function<double(double)>& weight) {
        double total = 0;
        for (int j = 0; j <= samples; j++) {
            double c = (j == 0 || j == samples) ? 1.0 : (j % 2 ? 4.0 : 2.0);
            total += c * ys[static_cast<size_t>(j)] * weight(a + j * h);
        }
        return total * h / 3.0;
    };
    double scale = 2.0 / (b - a);
    s.a0 = scale * simpson([](double) { return 1.0; });
    for (int k = 1; k <= N; k++) {
        s.ak.push_back(scale * simpson([&](double x) { return std::cos(k * w * x); }));
        s.bk.push_back(scale * simpson([&](double x) { return std::sin(k * w * x); }));
    }
    return s;
}

double harmonic(const Series& s, int k, double x) {
    if (k < 1 || k > static_cast<int>(s.ak.size())) return 0.0;
    double w = s.omega();
    return s.ak[static_cast<size_t>(k - 1)] * std::cos(k * w * x) + s.bk[static_cast<size_t>(k - 1)] * std::sin(k * w * x);
}

double partialSum(const Series& s, double x, int n) {
    int count = n < 0 ? static_cast<int>(s.ak.size()) : std::min(n, static_cast<int>(s.ak.size()));
    double y = s.a0 / 2.0;
    for (int k = 1; k <= count; k++) y += harmonic(s, k, x);
    return y;
}

} // namespace linalg::fourier
