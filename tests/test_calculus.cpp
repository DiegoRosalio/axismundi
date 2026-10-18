// Tests: Calculus and geometry: vector fields, integrals, curves (Frenet frame), derivatives, Fourier series, contours, the orbit camera math
#include "TestSupport.hpp"
#include "math/GridMath.hpp"
#include "math/Taylor.hpp"

void testVectorField() {
    using namespace field;
    auto program = [](const char* text) {
        auto p = std::make_unique<expr::Program>();
        CHECK(p->compile(text, expr::kVarZ));
        return p;
    };
    Box box{{-4, -4, -2}, {4, 4, 2}};

    // Arrows: grid centers, the longest fits 90% of a cell, zero vectors skipped
    auto rot = program("[-y, x, 0]");
    FieldFunction plane(*rot, nullptr, 0.0, true);
    ArrowSet arrows = sampleArrows(plane, box, 8, 8, 1, false);
    CHECK(arrows.arrows.size() == 64);
    CHECK_NEAR(arrows.cell, 1.0, 1e-9);
    double longest = 0;
    for (const Arrow& a : arrows.arrows) longest = std::max(longest, static_cast<double>(a.vec.magnitude()));
    CHECK_NEAR(longest, 0.9, 1e-5);
    CHECK_NEAR(arrows.maxMagnitude, std::sqrt(3.5 * 3.5 * 2), 1e-5);
    CHECK_NEAR(arrows.arrows[0].base.x, -3.5, 1e-6);
    CHECK_NEAR(arrows.arrows[0].base.z, 0.0, 1e-9);
    ArrowSet same = sampleArrows(plane, box, 8, 8, 1, true);
    CHECK_NEAR(same.arrows[0].vec.magnitude(), same.arrows[10].vec.magnitude(), 1e-5);
    auto zero = program("[x - x, 0, 0]");
    CHECK(sampleArrows(FieldFunction(*zero, nullptr, 0, true), box, 4, 4, 1, false).arrows.empty());
    // Volume: nx * ny * nz arrows; plane fields drop F's z component
    auto up = program("[0, 0, 1 + z*z]");
    CHECK(sampleArrows(FieldFunction(*up, nullptr, 0, false), box, 3, 3, 4, false).arrows.size() == 36);
    CHECK(sampleArrows(FieldFunction(*up, nullptr, 0, true), box, 3, 3, 4, false).arrows.empty());

    // Flow line of a rotation: a closed circle that keeps its radius, duration ≈ 2π
    FlowLine circle = traceFlowLine(plane, {2, 0, 0}, box, 0.05, 2000);
    CHECK(circle.closed);
    double worst = 0;
    for (const Vector3D& p : circle.points) worst = std::max(worst, std::fabs(p.magnitude() - 2.0));
    CHECK(worst < 1e-3);
    CHECK_NEAR(circle.duration(), 2 * 3.14159265358979, 0.05);
    CHECK_NEAR(particleAt(circle, circle.duration() / 2).x, -2.0, 0.02);  // half a turn later

    // Source: the line runs out of the box (both ends clipped at the edge or stopped at 0)
    auto source = program("[x, y, 0]");
    FieldFunction src(*source, nullptr, 0, true);
    FlowLine out = traceFlowLine(src, {1, 0.5, 0}, box, 0.05, 2000);
    CHECK(!out.closed);
    CHECK(out.points.size() > 10);
    CHECK_NEAR(std::max(std::fabs(out.points.back().x), std::fabs(out.points.back().y)), 4.0, 1e-3);
    CHECK(out.points.front().magnitude() < 0.05);  // traced back to the source at the origin
    for (size_t i = 1; i < out.times.size(); i++) CHECK(out.times[i] >= out.times[i - 1]);

    // Kept apart: spirals into a sink don't pile up; a rotation still gives whole circles
    auto sink = program("[-y - x/4, x - y/4, 0]");
    FieldFunction vortex(*sink, nullptr, 0, true);
    auto seeds = flowSeeds(box, true, 5);
    auto apart = traceFlowLines(vortex, seeds, box, 0.05, 600, 0.4);
    CHECK(!apart.empty() && apart.size() <= seeds.size());
    bool spaced = true;
    for (size_t a = 0; a < apart.size(); a++)
        for (size_t b = a + 1; b < apart.size(); b++)
            for (const Vector3D& p : apart[b].points)
                for (const Vector3D& q : apart[a].points)
                    if ((p - q).magnitude() < 0.4 - 1e-4) spaced = false;
    CHECK(spaced);
    Box big{{-6, -6, -2}, {6, 6, 2}};  // every circle through the seeds fits
    auto rings = traceFlowLines(plane, flowSeeds({{-4, -4, 0}, {4, 4, 0}}, true, 4), big, 0.05, 2000, 0.3);
    bool allClosed = !rings.empty();
    for (const FlowLine& r : rings) allClosed = allClosed && r.closed;
    CHECK(allClosed);
    CHECK(rings.size() == 3);  // radii √2, √10, √18: seeds on a circle already drawn are skipped

    // Seeds and particle times (periodic in the scene time, spread along the line)
    CHECK(flowSeeds(box, true, 5).size() == 25);
    CHECK(flowSeeds(box, false, 3).size() == 27);
    double T = circle.duration();
    CHECK_NEAR(particleTime(circle, 0, 3, 0.0, 1.0), 0.0, 1e-12);
    CHECK_NEAR(particleTime(circle, 1, 3, 0.0, 1.0), T / 3, 1e-9);
    CHECK_NEAR(particleTime(circle, 0, 3, T + 0.25, 1.0), 0.25, 1e-9);
    CHECK_NEAR(particleTime(circle, 0, 3, 0.25, 2.0), 0.5, 1e-9);
}

void testIntegrals() {
    using namespace integrals;
    constexpr double pi = 3.14159265358979;
    Surface xy = [](double x, double y, double& z) { z = x * y; return true; };
    Rect unit{0, 1, 0, 1};
    bool ok = false;
    CHECK_NEAR(simpson2D(xy, unit, 10, ok), 0.25, 1e-12);
    CHECK(ok);
    // Midpoint boxes are exact for x·y; left boxes undershoot
    RiemannSum mid = riemann2D(xy, unit, 4, 4, Rule::Middle);
    CHECK(mid.boxes.size() == 16);
    CHECK_NEAR(mid.sum, 0.25, 1e-12);
    CHECK(riemann2D(xy, unit, 4, 4, Rule::Left).sum < 0.25);
    CHECK(riemann2D(xy, unit, 4, 4, Rule::Right).sum > 0.25);
    // The midpoint error shrinks about 4x when the boxes double (second order)
    Surface bowl = [](double x, double y, double& z) { z = x * x + y * y; return true; };
    Rect sq{-1, 1, -1, 1};
    double exact = 8.0 / 3.0;
    double e4 = std::fabs(riemann2D(bowl, sq, 4, 4, Rule::Middle).sum - exact);
    double e8 = std::fabs(riemann2D(bowl, sq, 8, 8, Rule::Middle).sum - exact);
    CHECK(e8 > 0 && e4 / e8 > 3.5 && e4 / e8 < 4.5);
    CHECK_NEAR(simpson2D(bowl, sq, 20, ok), exact, 1e-9);
    // Sign change: x over [−1, 1] integrates to 0 (boxes below the floor count negative)
    Surface lin = [](double x, double, double& z) { z = x; return true; };
    CHECK_NEAR(riemann2D(lin, sq, 6, 6, Rule::Middle).sum, 0.0, 1e-12);
    // Undefined cells are skipped (and reported)
    Surface hole = [](double x, double y, double& z) { z = 1.0; return x * x + y * y > 0.1; };
    RiemannSum h = riemann2D(hole, sq, 4, 4, Rule::Middle);
    CHECK(h.skipped == 0);  // centers ±0.25, ±0.75: none within the hole
    simpson2D(hole, sq, 4, ok);
    CHECK(!ok);  // the grid hits (0, 0)

    // Line integrals: circulation of a rotation around the unit circle = 2π
    Field rot = [](const D3& p, D3& o) { o = {-p.y, p.x, 0}; return true; };
    Path circle = [](double u, D3& o) { o = {std::cos(u), std::sin(u), 0}; return true; };
    CHECK_NEAR(lineIntegral(rot, circle, 0, 2 * pi, 1.0, 200, ok), 2 * pi, 1e-6);
    CHECK(ok);
    CHECK_NEAR(lineIntegral(rot, circle, 0, 2 * pi, 0.5, 200, ok), pi, 1e-6);  // halfway: half
    // A gradient field ∇(x·y·z) = [yz, xz, xy]: the work is the endpoint difference, on any path
    Field grad = [](const D3& p, D3& o) { o = {p.y * p.z, p.x * p.z, p.x * p.y}; return true; };
    Path straight = [](double u, D3& o) { o = {u, u, u}; return true; };
    Path wiggly = [](double u, D3& o) { o = {u, u * u, u + std::sin(pi * u)}; return true; };
    CHECK_NEAR(lineIntegral(grad, straight, 0, 1, 1.0, 100, ok), 1.0, 1e-8);
    CHECK_NEAR(lineIntegral(grad, wiggly, 0, 1, 1.0, 200, ok), 1.0, 1e-6);
}

void testCurveGeometry() {
    using namespace geometry;
    // Helix (a cos u, a sin u, b u): κ = a/(a² + b²), τ = b/(a² + b²) everywhere
    const double a = 2.0, b = 0.5;
    Path helix = [a, b](double u, D3& o) { o = {a * std::cos(u), a * std::sin(u), b * u}; return true; };
    Frame f;
    CHECK(frenet(helix, 1.3, 1e-3, f));
    CHECK_NEAR(f.kappa, a / (a * a + b * b), 1e-5);
    CHECK_NEAR(f.tau, b / (a * a + b * b), 1e-4);
    CHECK_NEAR(f.speed, std::sqrt(a * a + b * b), 1e-6);
    // Orthonormal and right-handed
    CHECK_NEAR(length(f.T), 1, 1e-9);
    CHECK_NEAR(length(f.N), 1, 1e-9);
    CHECK_NEAR(dot(f.T, f.N), 0, 1e-9);
    D3 tn = cross(f.T, f.N);
    CHECK_NEAR(length(sub(tn, f.B)), 0, 1e-9);
    // N points to the axis of the helix (towards the center of curvature)
    CHECK(dot(f.N, D3{-f.p.x, -f.p.y, 0}) > 0);
    // Circle of radius 3 in a plane: κ = 1/3, τ = 0; traced at another speed, the same
    Path circle = [](double u, D3& o) { o = {3 * std::cos(2 * u), 3 * std::sin(2 * u), 1}; return true; };
    CHECK(frenet(circle, 0.4, 1e-3, f));
    CHECK_NEAR(f.kappa, 1.0 / 3.0, 1e-6);
    CHECK_NEAR(f.tau, 0, 1e-5);
    CHECK_NEAR(f.speed, 6, 1e-5);
    // Parabola y = x²/2 at the vertex: κ = 1
    Path parabola = [](double u, D3& o) { o = {u, u * u / 2, 0}; return true; };
    CHECK(frenet(parabola, 0.0, 1e-3, f));
    CHECK_NEAR(f.kappa, 1.0, 1e-6);
    // A straight line has no normal
    Path line = [](double u, D3& o) { o = {u, 2 * u, -u}; return true; };
    CHECK(frenet(line, 0.7, 1e-3, f));
    CHECK(!f.hasNormal && f.kappa == 0.0);
    CHECK_NEAR(dot(f.T, f.N), 0, 1e-9);
    // Standing still (r' = 0) fails
    Path still = [](double, D3& o) { o = {1, 1, 1}; return true; };
    CHECK(!frenet(still, 0, 1e-3, f));
}

void testDerivatives() {
    using namespace derivatives;
    Function sine = [](double x, double& y) { y = std::sin(x); return true; };
    double d = 0;
    for (double x : {-2.0, 0.0, 0.7, 3.0}) {
        CHECK(derivative1(sine, x, stepFor(1), d));
        CHECK_NEAR(d, std::cos(x), 1e-9);
    }
    Function cube = [](double x, double& y) { y = x * x * x; return true; };
    CHECK(derivative2(cube, 1.5, stepFor(1), d));
    CHECK_NEAR(d, 9.0, 1e-4);  // (x³)″ = 6x
    // Undefined near the point: sqrt at 0 needs f(−h)
    Function root = [](double x, double& y) { if (x < 0) return false; y = std::sqrt(x); return true; };
    CHECK(!derivative1(root, 0.0, stepFor(1), d));
    CHECK(derivative1(root, 4.0, stepFor(1), d));
    CHECK_NEAR(d, 0.25, 1e-8);
    // ∂/∂x (x²y) = 2xy, ∂/∂y = x²
    Surface xxy = [](double x, double y, double& z) { z = x * x * y; return true; };
    double fx = 0, fy = 0;
    CHECK(partials(xxy, 1.5, -2.0, stepFor(1), fx, fy));
    CHECK_NEAR(fx, -6.0, 1e-8);
    CHECK_NEAR(fy, 2.25, 1e-8);
    // Helix (2cos u, 2sin u, u/2): |r′| = √(4 + 1/4), r″ = −(2cos u, 2sin u, 0) points to the axis
    Path helix = [](double u, D3& p) { p = {2 * std::cos(u), 2 * std::sin(u), u / 2}; return true; };
    D3 v, a;
    CHECK(curveDerivatives(helix, 0.8, stepFor(1), v, a));
    CHECK_NEAR(std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z), std::sqrt(4.25), 1e-5);
    CHECK_NEAR(a.x, -2 * std::cos(0.8), 1e-4);
    CHECK_NEAR(a.y, -2 * std::sin(0.8), 1e-4);
    CHECK_NEAR(a.z, 0.0, 1e-4);
    CHECK_NEAR(v.x * a.x + v.y * a.y + v.z * a.z, 0.0, 1e-4);  // constant speed: a ⊥ v
}

void testFourier() {
    using namespace fourier;
    constexpr double pi = 3.14159265358979;
    int bad = 0;
    // Square wave on [−π, π]: b_k = 4/(πk) for odd k, 0 for even, no cosines
    auto square = [](double x, double& y) { y = x < 0 ? -1.0 : 1.0; return true; };
    Series sq = seriesCoefficients(square, -pi, pi, 7, 4000, bad);
    CHECK(bad == 0);
    CHECK_NEAR(sq.a0, 0, 1e-3);
    for (int k = 1; k <= 7; k++) {
        CHECK_NEAR(sq.bk[k - 1], k % 2 ? 4.0 / (pi * k) : 0.0, 2e-3);
        CHECK_NEAR(sq.ak[k - 1], 0, 2e-3);
    }
    // Sawtooth f = x: b_k = 2(−1)^(k+1)/k
    auto saw = [](double x, double& y) { y = x; return true; };
    Series sw = seriesCoefficients(saw, -pi, pi, 5, 4000, bad);
    for (int k = 1; k <= 5; k++) CHECK_NEAR(sw.bk[k - 1], 2.0 * (k % 2 ? 1 : -1) / k, 1e-6);
    // A pure cosine: a₃ = 1, nothing else; and the partial sum reproduces it
    auto c3 = [](double x, double& y) { y = std::cos(3 * x) + 0.5; return true; };
    Series cs = seriesCoefficients(c3, -pi, pi, 5, 2000, bad);
    CHECK_NEAR(cs.a0, 1.0, 1e-9);
    CHECK_NEAR(cs.ak[2], 1.0, 1e-9);
    CHECK_NEAR(cs.ak[0], 0, 1e-9);
    CHECK_NEAR(cs.bk[2], 0, 1e-9);
    CHECK_NEAR(partialSum(cs, 0.7), std::cos(2.1) + 0.5, 1e-8);
    CHECK_NEAR(partialSum(cs, 0.7 + 2 * pi), partialSum(cs, 0.7), 1e-9);  // periodic
    // Another interval: [0, 2] has period 2 (ω = π)
    auto sin1 = [](double x, double& y) { y = std::sin(pi * x); return true; };
    Series s01 = seriesCoefficients(sin1, 0, 2, 3, 2000, bad);
    CHECK_NEAR(s01.bk[0], 1.0, 1e-8);
    // More terms: closer (mean square error of the square wave decreases)
    auto err = [&](int n) {
        double e = 0;
        for (int i = 1; i < 400; i++) {
            double x = -pi + 2 * pi * i / 400, y = 0;
            square(x, y);
            e += std::pow(partialSum(sq, x, n) - y, 2);
        }
        return e;
    };
    CHECK(err(7) < err(3) && err(3) < err(1));
    // Undefined points count as 0 and are reported
    auto hole = [](double x, double& y) { y = 1; return std::fabs(x) > 1e-9; };
    seriesCoefficients(hole, -pi, pi, 2, 100, bad);
    CHECK(bad == 1);

    // Curves: a circle e^{iu} has c₁ = 1 only; the center comes first
    std::vector<cplx> circle;
    for (int j = 0; j < 64; j++) circle.push_back(std::polar(1.0, 2 * pi * j / 64) + cplx(2, 1));
    auto terms = curveCoefficients(circle, 8);
    CHECK(terms.size() == 17 && terms[0].k == 0);
    CHECK_NEAR(std::abs(terms[0].c - cplx(2, 1)), 0, 1e-9);
    CHECK(terms[1].k == 1 && std::abs(terms[1].c - cplx(1, 0)) < 1e-9);
    CHECK(std::abs(terms[2].c) < 1e-9);
    // The epicycle tip is the partial sum, and with all terms it passes through the samples
    std::vector<cplx> shape;
    for (int j = 0; j < 128; j++) {
        double u = 2 * pi * j / 128;
        shape.push_back(cplx(std::cos(u) + 0.3 * std::cos(3 * u), std::sin(u) - 0.2 * std::sin(2 * u)));
    }
    auto st = curveCoefficients(shape, 6);
    auto pts = chain(st, 0.9, 5);
    CHECK(pts.size() == 5);
    CHECK_NEAR(std::abs(pts.back() - evaluate(st, 0.9, 5)), 0, 1e-12);
    CHECK_NEAR(std::abs(evaluate(st, 2 * pi * 10 / 128) - shape[10]), 0, 1e-9);
}

void testContour() {
    // Circle x^2 + y^2 = 4 on a grid: points lie on the circle, segments chain into one closed line
    int n = 40;
    std::vector<double> v((n + 1) * (n + 1));
    for (int j = 0; j <= n; j++)
        for (int i = 0; i <= n; i++) {
            double x = -3 + 6.0 * i / n, y = -3 + 6.0 * j / n;
            v[j * (n + 1) + i] = x * x + y * y - 4;
        }
    auto segs = marchingSquares(v, n, n, -3, 3, -3, 3);
    CHECK(segs.size() > 20);
    for (const auto& sg : segs) CHECK_NEAR(std::hypot(sg.x0, sg.y0), 2.0, 0.05);
    auto lines = chainSegments(segs, 1e-6);
    CHECK(lines.size() == 1);
    if (!lines.empty()) {
        CHECK(lines[0].size() == segs.size() + 1);
        CHECK_NEAR(lines[0].front().first, lines[0].back().first, 1e-9);  // closed
    }
}

void testOrbitCamera() {
    OrbitCamera cam;
    // fromView -> position round trip, distance and angles
    cam.fromView({3, 4, 5}, {1, 1, 1});
    Vector3D p = cam.position();
    CHECK_NEAR(p.x, 3.0, 1e-4);
    CHECK_NEAR(p.y, 4.0, 1e-4);
    CHECK_NEAR(p.z, 5.0, 1e-4);
    CHECK_NEAR(cam.distance(), std::sqrt(4.0 + 9.0 + 16.0), 1e-4);
    // The eye basis is orthonormal and right-handed (right × up = −forward, as for a viewer)
    Vector3D f = cam.forward(), r = cam.right(), u = cam.up();
    CHECK_NEAR(f.magnitude(), 1.0, 1e-5);
    CHECK_NEAR(r.magnitude(), 1.0, 1e-5);
    CHECK_NEAR(u.magnitude(), 1.0, 1e-5);
    CHECK_NEAR(f.dot(r), 0.0, 1e-5);
    CHECK_NEAR(f.dot(u), 0.0, 1e-5);
    CHECK(u.y > 0.0f);  // screen up points upward
    // Orbit keeps the pivot and distance
    float d = cam.distance();
    cam.orbit(0.7f, 0.3f);
    CHECK_NEAR((cam.position() - cam.pivot()).magnitude(), d, 1e-4);
    CHECK_NEAR(cam.pivot().x, 1.0, 1e-6);
    // The pitch is clamped short of straight up/down
    cam.orbit(0.0f, 10.0f);
    CHECK_NEAR(cam.pitch(), OrbitCamera::kMaxPitch, 1e-6);
    cam.orbit(0.0f, -20.0f);
    CHECK_NEAR(cam.pitch(), -OrbitCamera::kMaxPitch, 1e-6);
    // Pan moves the pivot across the view, not along it, and keeps the orientation
    cam.fromView({0, 0, 10}, {0, 0, 0});
    cam.pan(100.0f, 0.0f, 1000.0f, 60.0f);
    Vector3D moved = cam.pivot();
    CHECK(moved.x < 0.0f);  // dragging right moves the scene right: the pivot goes left
    CHECK_NEAR(moved.z, 0.0, 1e-5);
    // 100 px of a 1000 px view at distance 10, fovy 60°: 2·10·tan 30° / 10
    CHECK_NEAR(-moved.x, 2.0 * 10.0 * std::tan(3.14159265 / 6) / 10.0, 1e-4);
    cam.pan(0.0f, 50.0f, 1000.0f, 60.0f);
    CHECK(cam.pivot().y > 0.0f);  // dragging down moves the scene down: the pivot goes up
    // Zoom: closer per positive step, within limits
    cam.setDistance(10.0f);
    cam.zoom(1.0f);
    CHECK_NEAR(cam.distance(), 8.5, 1e-4);
    cam.zoom(1000.0f);
    CHECK_NEAR(cam.distance(), OrbitCamera::kMinDistance, 1e-6);
    cam.zoom(-1000.0f);
    CHECK_NEAR(cam.distance(), OrbitCamera::kMaxDistance, 1e-3);
    // Numpad views
    cam.setDistance(5.0f);
    cam.setPivot({0, 0, 0});
    cam.frontView();
    CHECK_NEAR(cam.position().z, 5.0, 1e-4);
    cam.rightView();
    CHECK_NEAR(cam.position().x, 5.0, 1e-4);
    cam.topView();
    CHECK(cam.position().y > 4.99f);
    // A degenerate view (eye on the target) stays usable
    cam.fromView({1, 1, 1}, {1, 1, 1});
    CHECK(std::isfinite(cam.position().x) && cam.distance() >= OrbitCamera::kMinDistance);
}

void testGridMath() {
    using namespace grid;
    // 900 px tall view, fovy 60°, 10 units away: the view is 2·10·tan 30° ≈ 11.55 units tall
    CHECK_NEAR(pixelsPerUnit(10, 60, 900), 900 / (20 * std::tan(3.14159265358979 / 6)), 1e-9);
    // Spacing: a power of 10, never closer than minPixels
    for (double ppu : {0.37, 3.0, 12.0, 40.0, 77.7, 1000.0, 25000.0}) {
        Level l = autoLevel(ppu, 12);
        CHECK(l.spacing * ppu >= 12 * (1 - 1e-9));
        CHECK(l.spacing * ppu < 120 * (1 + 1e-9));
        double k = std::log10(l.spacing);
        CHECK_NEAR(k, std::round(k), 1e-9);
        CHECK(l.fade >= 0 && l.fade <= 1);
    }
    CHECK_NEAR(autoLevel(12, 12).spacing, 1.0, 1e-12);   // exactly at the limit: spacing 1, just fading in
    CHECK_NEAR(autoLevel(12, 12).fade, 0.0, 1e-9);
    CHECK_NEAR(autoLevel(120, 12).spacing, 0.1, 1e-12);  // zoomed in 10x: 0.1
    CHECK_NEAR(autoLevel(1.2, 12).spacing, 10.0, 1e-9);  // zoomed out: 10
    // Continuity: the line x = 10 keeps its strength while zooming across a power of 10
    auto strengthOfTen = [](double ppu) {
        Level l = autoLevel(ppu, 12);
        double best = 0;
        for (int j = 0; j < 3; j++) {
            double s = l.spacing * std::pow(l.ratio, j);
            double r = 10.0 / s;
            if (std::fabs(r - std::round(r)) < 1e-6) best = std::max(best, familyStrength(j, l.fade));
        }
        return best;
    };
    for (double ppu : {1.2, 12.0, 120.0}) CHECK_NEAR(strengthOfTen(ppu * 1.0001), strengthOfTen(ppu * 0.9999), 1e-3);
    CHECK_NEAR(familyStrength(0, 0), 0.0, 1e-12);   // the finest lines start invisible
    CHECK_NEAR(familyStrength(2, 0.3), 1.0, 1e-12); // the widest are full
    CHECK(familyStrength(1, 0.5) > familyStrength(0, 0.5));
    Level f = fixedLevel(1, 5);
    CHECK(f.spacing == 1 && f.fade == 1 && f.ratio == 5);
    // Tick steps: 1, 2 or 5 × 10^k, at least minPixels apart
    CHECK_NEAR(tickStep(100, 60), 1.0, 1e-12);
    CHECK_NEAR(tickStep(40, 60), 2.0, 1e-12);
    CHECK_NEAR(tickStep(20, 60), 5.0, 1e-12);
    CHECK_NEAR(tickStep(7, 60), 10.0, 1e-12);
    CHECK_NEAR(tickStep(500, 60), 0.2, 1e-12);
    // Formatting
    CHECK(formatTick(0.30000001, 0.1) == "0.3");
    CHECK(formatTick(-2, 1) == "−2");
    CHECK(formatTick(100, 50) == "100");
    CHECK(formatTick(-1e-12, 0.5) == "0");
    CHECK(formatTick(0.25, 0.05) == "0.25");
    CHECK(formatTick(1.5, 0.5) == "1.5");
}

void testTaylor() {
    using namespace taylor;
    auto pair = [](auto realFn, auto complexFn) {
        RealFunction f = [=](double x, double& y) { y = realFn(x); return std::isfinite(y); };
        ComplexFunction fc = [=](cplx z, cplx& w) { w = complexFn(z); return true; };
        return std::make_pair(f, fc);
    };
    // sin about 0: 0, 1, 0, -1/6, 0, 1/120, ... up to degree 15
    auto [sinR, sinC] = pair([](double x) { return std::sin(x); }, [](cplx z) { return std::sin(z); });
    Series s;
    CHECK(coefficients(sinC, sinR, 0.0, 15, s));
    CHECK(s.complexMethod && s.coeffs.size() == 16);
    double fact = 1;
    for (int k = 0; k <= 15; k++) {
        if (k > 0) fact *= k;
        double expect = (k % 2 == 0) ? 0.0 : ((k / 2) % 2 == 0 ? 1.0 : -1.0) / fact;
        CHECK_NEAR(s.coeffs[static_cast<size_t>(k)], expect, 1e-11);
    }
    CHECK_NEAR(evaluate(s, 2.0), std::sin(2.0), 1e-6);  // degree 15 is very close at x = 2
    CHECK(std::fabs(evaluate(s, 2.0, 3) - std::sin(2.0)) > 0.1);  // degree 3 isn't
    // e^x about 1: e / k!
    auto [expR, expC] = pair([](double x) { return std::exp(x); }, [](cplx z) { return std::exp(z); });
    CHECK(coefficients(expC, expR, 1.0, 10, s));
    fact = 1;
    for (int k = 0; k <= 10; k++) {
        if (k > 0) fact *= k;
        CHECK_NEAR(s.coeffs[static_cast<size_t>(k)], std::exp(1.0) / fact, 1e-9);
    }
    // A polynomial is its own Taylor polynomial: x^3 - 2x about 2 -> 4 + 10(x-2) + 6(x-2)^2 + (x-2)^3
    auto [pR, pC] = pair([](double x) { return x * x * x - 2 * x; }, [](cplx z) { return z * z * z - 2.0 * z; });
    CHECK(coefficients(pC, pR, 2.0, 5, s));
    CHECK_NEAR(s.coeffs[0], 4, 1e-10);
    CHECK_NEAR(s.coeffs[1], 10, 1e-10);
    CHECK_NEAR(s.coeffs[2], 6, 1e-10);
    CHECK_NEAR(s.coeffs[3], 1, 1e-10);
    CHECK_NEAR(s.coeffs[4], 0, 1e-10);
    // 1/(1+x^2) about 0: 1, 0, -1, 0, 1, ...; the circle must stay inside the poles at ±i
    auto [rR, rC] = pair([](double x) { return 1 / (1 + x * x); }, [](cplx z) { return 1.0 / (1.0 + z * z); });
    CHECK(coefficients(rC, rR, 0.0, 10, s));
    CHECK(s.complexMethod && s.radius < 1.0);
    for (int k = 0; k < static_cast<int>(s.coeffs.size()); k++)
        CHECK_NEAR(s.coeffs[static_cast<size_t>(k)], k % 2 ? 0.0 : (k % 4 == 0 ? 1.0 : -1.0), 1e-7);
    // abs: not analytic -> finite differences up to degree 4; floor: no complex values at all
    auto [aR, aC] = pair([](double x) { return std::fabs(x); }, [](cplx z) { return cplx(std::abs(z), 0); });
    CHECK(coefficients(aC, aR, 1.0, 8, s));
    CHECK(!s.complexMethod && s.coeffs.size() == 5);
    CHECK_NEAR(s.coeffs[1], 1.0, 1e-8);
    RealFunction floorR = [](double x, double& y) { y = std::floor(x); return true; };
    ComplexFunction noComplex = [](cplx, cplx&) { return false; };
    CHECK(coefficients(noComplex, floorR, 0.5, 6, s));
    CHECK(!s.complexMethod && s.note.find("complex") != std::string::npos);
    RealFunction lnR = [](double x, double& y) { if (x <= 0) return false; y = std::log(x); return true; };
    CHECK(!coefficients(noComplex, lnR, -1.0, 3, s));  // undefined at the center

    // Newton: x^2 - 2 from 1 -> sqrt 2, the error roughly squaring each step
    RealFunction g = [](double x, double& y) { y = x * x - 2; return true; };
    NewtonRun run = newton(g, 1.0, 8);
    CHECK(run.stop == NewtonStop::Converged);
    CHECK_NEAR(run.steps.back().x, std::sqrt(2.0), 1e-12);
    CHECK(run.steps.size() <= 8);
    double e2 = std::fabs(run.steps[2].x - std::sqrt(2.0)), e3 = std::fabs(run.steps[3].x - std::sqrt(2.0));
    CHECK(e3 < 10 * e2 * e2);  // quadratic convergence
    NewtonRun two = newton(g, 1.0, 2);
    CHECK(two.stop == NewtonStop::Steps && two.steps.size() == 3);  // x0, x1, x2
    CHECK_NEAR(two.steps[1].x, 1.5, 1e-9);
    // From x0 = 0 the tangent is flat
    CHECK(newton(g, 0.0, 5).stop == NewtonStop::FlatTangent);
    // atan from 1.5 overshoots farther and farther
    RealFunction at = [](double x, double& y) { y = std::atan(x); return true; };
    NewtonRun away = newton(at, 1.5, 30);
    CHECK(away.stop == NewtonStop::Diverging || std::fabs(away.steps.back().x) > 100);
    CHECK(std::fabs(away.steps[2].x) > std::fabs(away.steps[1].x));
}
