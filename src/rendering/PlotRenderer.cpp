#include "rendering/PlotRenderer.hpp"
#include "rendering/Labels.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg {

namespace {

// Values beyond this are treated as off the chart (asymptotes, overflow)
constexpr float kClip = 100.0f;


bool usable(double v) {
    return std::isfinite(v) && std::fabs(v) <= kClip;
}

// Math coordinates (z up) to world coordinates (y up)
Vector3 mathToWorld(float x, float y, float z) {
    return {x, z, -y};
}

float normalize(float v, float lo, float hi) {
    if (hi - lo < 1e-9f) return 0.5f;
    return std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
}

} // namespace

Color PlotRenderer::heatColor(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    struct Stop { float t; float r, g, b; };
    static const Stop stops[] = {
        {0.00f, 40, 60, 220},
        {0.25f, 30, 190, 230},
        {0.50f, 60, 200, 80},
        {0.75f, 245, 210, 40},
        {1.00f, 225, 50, 40},
    };
    for (int i = 0; i < 4; i++) {
        if (t <= stops[i + 1].t) {
            float u = (t - stops[i].t) / (stops[i + 1].t - stops[i].t);
            return {
                static_cast<unsigned char>(stops[i].r + (stops[i + 1].r - stops[i].r) * u),
                static_cast<unsigned char>(stops[i].g + (stops[i + 1].g - stops[i].g) * u),
                static_cast<unsigned char>(stops[i].b + (stops[i + 1].b - stops[i].b) * u),
                255,
            };
        }
    }
    return {225, 50, 40, 255};
}

bool PlotRenderer::rebuild(const PlotSpec& spec) {
    m_type = spec.type;
    m_curve.clear();
    m_wireframe.clear();
    m_triangles.clear();
    m_error.clear();

    auto parse = [&](Expression& e, const std::string& text, const char* name) {
        if (e.parse(text)) return true;
        m_error = std::string(name) + ": " + e.error();
        return false;
    };

    bool needsX = spec.type != PlotType::Parametric;
    bool needsY = spec.type == PlotType::Surface || spec.type == PlotType::Implicit;
    if (needsX && !(spec.xMin < spec.xMax)) {
        m_error = "x range: min must be less than max";
        return false;
    }
    if (needsY && !(spec.yMin < spec.yMax)) {
        m_error = "y range: min must be less than max";
        return false;
    }
    if (spec.type == PlotType::Parametric && !(spec.tMin < spec.tMax)) {
        m_error = "t range: min must be less than max";
        return false;
    }

    switch (spec.type) {
        case PlotType::Function2D: {
            Expression f;
            if (!parse(f, spec.f, "f(x)")) return false;
            buildFunction2D(spec, f);
            break;
        }
        case PlotType::Parametric: {
            Expression x, y, z;
            if (!parse(x, spec.px, "x(t)") || !parse(y, spec.py, "y(t)") || !parse(z, spec.pz, "z(t)"))
                return false;
            buildParametric(spec, x, y, z);
            break;
        }
        case PlotType::Surface: {
            Expression f;
            if (!parse(f, spec.f, "f(x, y)")) return false;
            buildSurface(spec, f);
            break;
        }
        case PlotType::Implicit: {
            Expression f;
            if (!parse(f, spec.f, "f(x, y)")) return false;
            buildImplicit(spec, f);
            break;
        }
    }

    if (m_curve.empty() && m_triangles.empty()) {
        m_error = "nothing to draw: no finite values in range";
    }
    return m_error.empty();
}

void PlotRenderer::buildFunction2D(const PlotSpec& spec, const Expression& f) {
    int n = std::max(100, spec.resolution * 6);
    std::vector<float> ys(n + 1);
    std::vector<bool> ok(n + 1);
    m_minValue = kClip;
    m_maxValue = -kClip;

    for (int i = 0; i <= n; i++) {
        float x = spec.xMin + (spec.xMax - spec.xMin) * i / n;
        double y = f.eval(x);
        ok[i] = usable(y);
        ys[i] = static_cast<float>(y);
        if (ok[i]) {
            m_minValue = std::min(m_minValue, ys[i]);
            m_maxValue = std::max(m_maxValue, ys[i]);
        }
    }

    for (int i = 0; i < n; i++) {
        if (!ok[i] || !ok[i + 1]) continue;
        // A big jump with a sign change is an asymptote (e.g. tan x), not a line
        if (ys[i] * ys[i + 1] < 0.0f && std::fabs(ys[i + 1] - ys[i]) > 20.0f) continue;

        float x0 = spec.xMin + (spec.xMax - spec.xMin) * i / n;
        float x1 = spec.xMin + (spec.xMax - spec.xMin) * (i + 1) / n;
        Color c = spec.heatmap ? heatColor(normalize((ys[i] + ys[i + 1]) / 2, m_minValue, m_maxValue))
                               : ui::theme().curve;
        m_curve.push_back({{x0, ys[i], 0.0f}, {x1, ys[i + 1], 0.0f}, c});
    }
}

void PlotRenderer::buildParametric(const PlotSpec& spec, const Expression& ex, const Expression& ey,
                                   const Expression& ez) {
    int n = std::max(200, spec.resolution * 10);
    struct Sample { float x, y, z; bool ok; };
    std::vector<Sample> pts(n + 1);
    m_minValue = kClip;
    m_maxValue = -kClip;

    for (int i = 0; i <= n; i++) {
        double t = spec.tMin + (spec.tMax - spec.tMin) * i / n;
        double x = ex.eval(0, 0, t), y = ey.eval(0, 0, t), z = ez.eval(0, 0, t);
        Sample& s = pts[i];
        s = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z),
             usable(x) && usable(y) && usable(z)};
        if (s.ok) {
            m_minValue = std::min(m_minValue, s.z);
            m_maxValue = std::max(m_maxValue, s.z);
        }
    }

    for (int i = 0; i < n; i++) {
        const Sample& a = pts[i];
        const Sample& b = pts[i + 1];
        if (!a.ok || !b.ok) continue;
        // Color by height (math z)
        Color c = spec.heatmap ? heatColor(normalize((a.z + b.z) / 2, m_minValue, m_maxValue)) : ui::theme().curve;
        m_curve.push_back({mathToWorld(a.x, a.y, a.z), mathToWorld(b.x, b.y, b.z), c});
    }
}

void PlotRenderer::buildSurface(const PlotSpec& spec, const Expression& f) {
    int n = std::clamp(spec.resolution, 4, 200);
    std::vector<float> z((n + 1) * (n + 1));
    std::vector<bool> ok((n + 1) * (n + 1));
    auto idx = [n](int i, int j) { return j * (n + 1) + i; };
    auto xAt = [&](int i) { return spec.xMin + (spec.xMax - spec.xMin) * i / n; };
    auto yAt = [&](int j) { return spec.yMin + (spec.yMax - spec.yMin) * j / n; };

    m_minValue = kClip;
    m_maxValue = -kClip;
    for (int j = 0; j <= n; j++) {
        for (int i = 0; i <= n; i++) {
            double v = f.eval(xAt(i), yAt(j));
            ok[idx(i, j)] = usable(v);
            z[idx(i, j)] = static_cast<float>(v);
            if (usable(v)) {
                m_minValue = std::min(m_minValue, static_cast<float>(v));
                m_maxValue = std::max(m_maxValue, static_cast<float>(v));
            }
        }
    }

    auto point = [&](int i, int j) { return mathToWorld(xAt(i), yAt(j), z[idx(i, j)]); };
    int wireStep = std::max(1, n / 30);

    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            bool cellOk = ok[idx(i, j)] && ok[idx(i + 1, j)] && ok[idx(i + 1, j + 1)] && ok[idx(i, j + 1)];
            if (!cellOk) continue;

            Vector3 p00 = point(i, j), p10 = point(i + 1, j), p11 = point(i + 1, j + 1), p01 = point(i, j + 1);
            float avg = (z[idx(i, j)] + z[idx(i + 1, j)] + z[idx(i + 1, j + 1)] + z[idx(i, j + 1)]) / 4.0f;
            Color c = spec.heatmap ? heatColor(normalize(avg, m_minValue, m_maxValue)) : ui::theme().surface;

            // Both windings so the surface is visible from above and below
            m_triangles.push_back({p00, p10, p11, c});
            m_triangles.push_back({p00, p11, p01, c});
            m_triangles.push_back({p00, p11, p10, c});
            m_triangles.push_back({p00, p01, p11, c});

            Color wire = Fade(ui::theme().dark ? BLACK : WHITE, 0.3f);
            if (j % wireStep == 0) m_wireframe.push_back({p00, p10, wire});
            if (i % wireStep == 0) m_wireframe.push_back({p00, p01, wire});
            if (j + 1 == n) m_wireframe.push_back({p01, p11, wire});
            if (i + 1 == n) m_wireframe.push_back({p10, p11, wire});
        }
    }
}

void PlotRenderer::buildImplicit(const PlotSpec& spec, const Expression& f) {
    int n = std::clamp(spec.resolution * 2, 20, 300);
    std::vector<double> v((n + 1) * (n + 1));
    auto idx = [n](int i, int j) { return j * (n + 1) + i; };
    auto xAt = [&](int i) { return spec.xMin + (spec.xMax - spec.xMin) * i / n; };
    auto yAt = [&](int j) { return spec.yMin + (spec.yMax - spec.yMin) * j / n; };

    double maxAbs = 0.0;
    for (int j = 0; j <= n; j++) {
        for (int i = 0; i <= n; i++) {
            double val = f.eval(xAt(i), yAt(j));
            v[idx(i, j)] = val;
            if (std::isfinite(val)) maxAbs = std::max(maxAbs, std::fabs(val));
        }
    }
    m_minValue = static_cast<float>(-maxAbs);
    m_maxValue = static_cast<float>(maxAbs);

    // Heat map of f behind the curve: negative = blue, positive = red
    if (spec.heatmap && maxAbs > 0.0) {
        int step = std::max(1, n / 60);
        for (int j = 0; j < n; j += step) {
            for (int i = 0; i < n; i += step) {
                int i1 = std::min(i + step, n), j1 = std::min(j + step, n);
                double c = v[idx(i, j)];
                if (!std::isfinite(c)) continue;
                // Compress the range so values near zero still show contrast
                float t = 0.5f + 0.5f * static_cast<float>(std::tanh(3.0 * c / maxAbs));
                Color col = Fade(heatColor(t), 0.35f);
                Vector3 a = {xAt(i), yAt(j), -0.01f}, b = {xAt(i1), yAt(j), -0.01f};
                Vector3 d = {xAt(i1), yAt(j1), -0.01f}, e = {xAt(i), yAt(j1), -0.01f};
                m_triangles.push_back({a, b, d, col});
                m_triangles.push_back({a, d, e, col});
                m_triangles.push_back({a, d, b, col});
                m_triangles.push_back({a, e, d, col});
            }
        }
    }

    // Marching squares. Corners: 0 = (i,j), 1 = (i+1,j), 2 = (i+1,j+1), 3 = (i,j+1).
    // Edges: 0 = bottom (0-1), 1 = right (1-2), 2 = top (2-3), 3 = left (3-0).
    for (int j = 0; j < n; j++) {
        for (int i = 0; i < n; i++) {
            double c[4] = {v[idx(i, j)], v[idx(i + 1, j)], v[idx(i + 1, j + 1)], v[idx(i, j + 1)]};
            if (!std::isfinite(c[0]) || !std::isfinite(c[1]) || !std::isfinite(c[2]) || !std::isfinite(c[3]))
                continue;
            Vector3 p[4] = {{xAt(i), yAt(j), 0}, {xAt(i + 1), yAt(j), 0},
                            {xAt(i + 1), yAt(j + 1), 0}, {xAt(i), yAt(j + 1), 0}};

            int mask = (c[0] > 0) | ((c[1] > 0) << 1) | ((c[2] > 0) << 2) | ((c[3] > 0) << 3);
            if (mask == 0 || mask == 15) continue;

            auto edgePoint = [&](int e) {
                int a = e, b = (e + 1) % 4;
                double t = c[a] / (c[a] - c[b]);
                return Vector3{
                    static_cast<float>(p[a].x + (p[b].x - p[a].x) * t),
                    static_cast<float>(p[a].y + (p[b].y - p[a].y) * t),
                    0.0f,
                };
            };
            auto emit = [&](int e0, int e1) {
                m_curve.push_back({edgePoint(e0), edgePoint(e1), ui::theme().curve});
            };

            double center = (c[0] + c[1] + c[2] + c[3]) / 4.0;
            switch (mask) {
                case 1: case 14: emit(3, 0); break;
                case 2: case 13: emit(0, 1); break;
                case 3: case 12: emit(3, 1); break;
                case 4: case 11: emit(1, 2); break;
                case 6: case 9:  emit(0, 2); break;
                case 7: case 8:  emit(3, 2); break;
                case 5:  // corners 0 and 2 positive
                    if (center > 0) { emit(0, 1); emit(2, 3); }
                    else            { emit(3, 0); emit(1, 2); }
                    break;
                case 10: // corners 1 and 3 positive
                    if (center > 0) { emit(3, 0); emit(1, 2); }
                    else            { emit(0, 1); emit(2, 3); }
                    break;
                default: break;
            }
        }
    }
}

void PlotRenderer::draw() const {
    for (const auto& t : m_triangles) DrawTriangle3D(t.a, t.b, t.c, t.color);
    for (const auto& s : m_wireframe) DrawLine3D(s.a, s.b, s.color);
    for (const auto& s : m_curve) DrawCylinderEx(s.a, s.b, 0.03f, 0.03f, 4, s.color);
}

void PlotRenderer::drawAxes(float length) const {
    // Math axes: x red, y green, z blue (3D) — mapped to world space
    const ui::Theme& t = ui::theme();
    Vector3 o = {0, 0, 0};
    if (is3D()) {
        DrawCylinderEx(o, mathToWorld(length, 0, 0), 0.02f, 0.02f, 6, t.axisX);
        DrawCylinderEx(o, mathToWorld(0, length, 0), 0.02f, 0.02f, 6, t.axisY);
        DrawCylinderEx(o, mathToWorld(0, 0, length), 0.02f, 0.02f, 6, t.axisZ);
    } else {
        DrawCylinderEx({-length, 0, 0}, {length, 0, 0}, 0.02f, 0.02f, 6, t.axisX);
        DrawCylinderEx({0, -length, 0}, {0, length, 0}, 0.02f, 0.02f, 6, t.axisY);
    }
}

void PlotRenderer::drawLabels(const Camera3D& camera, float axisLength, bool showAxes) const {
    if (!showAxes) return;
    const ui::Theme& t = ui::theme();
    if (is3D()) {
        drawWorldLabel(camera, mathToWorld(axisLength, 0, 0), "x", t.axisX);
        drawWorldLabel(camera, mathToWorld(0, axisLength, 0), "y", t.axisY);
        drawWorldLabel(camera, mathToWorld(0, 0, axisLength), "z", t.axisZ);
    } else {
        drawWorldLabel(camera, {axisLength, 0, 0}, "x", t.axisX);
        drawWorldLabel(camera, {0, axisLength, 0}, "y", t.axisY);
    }
}

} // namespace linalg
