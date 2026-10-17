#include "rendering/PlotRenderer.hpp"
#include "rendering/Labels.hpp"
#include "rendering/WorldMapping.hpp"
#include "ui/core/Theme.hpp"
#include <raymath.h>
#include <rlgl.h>
#include <cstring>
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

// 2D plots sit a hair above the floor so they are not hidden by the grid
constexpr float kLift = 0.004f;

Color curveColor(const PlotSpec& s) { return s.color.a ? s.color : ui::theme().curve; }
Color surfaceColor(const PlotSpec& s) {
    if (!s.color.a) return ui::theme().surface;
    Color c = s.color;
    c.a = 230;
    return c;
}

float normalize(float v, float lo, float hi) {
    if (hi - lo < 1e-9f) return 0.5f;
    return std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
}

Color shaded(Color c, float shade) {
    shade = std::clamp(shade, 0.0f, 1.2f);
    auto ch = [&](unsigned char v) { return static_cast<unsigned char>(std::min(255.0f, v * shade)); };
    return {ch(c.r), ch(c.g), ch(c.b), c.a};
}

// Light from above and slightly in front, in world space (y up)
Vector3 lightDir() {
    return Vector3Normalize({0.35f, 0.85f, 0.4f});
}

Material& plotMaterial() {
    static Material material = LoadMaterialDefault();  // vertex colors, white texture
    return material;
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

PlotRenderer::~PlotRenderer() {
    releaseMeshes();
}

void PlotRenderer::MeshData::add(Vector3 p, Color c) {
    positions.insert(positions.end(), {p.x, p.y, p.z});
    colors.insert(colors.end(), {c.r, c.g, c.b, c.a});
}

void PlotRenderer::addTube(MeshData& mesh, Vector3 a, Vector3 b, float radius, Color color) const {
    Vector3 d = Vector3Subtract(b, a);
    float len = Vector3Length(d);
    if (len < 1e-6f) return;
    d = Vector3Scale(d, 1.0f / len);
    Vector3 up = std::fabs(d.y) < 0.9f ? Vector3{0, 1, 0} : Vector3{1, 0, 0};
    Vector3 u = Vector3Normalize(Vector3CrossProduct(d, up));
    Vector3 v = Vector3CrossProduct(d, u);
    const int sides = 6;
    Vector3 light = lightDir();
    for (int k = 0; k < sides; k++) {
        float a0 = 2.0f * PI * k / sides, a1 = 2.0f * PI * (k + 1) / sides;
        Vector3 n0 = Vector3Add(Vector3Scale(u, std::cos(a0)), Vector3Scale(v, std::sin(a0)));
        Vector3 n1 = Vector3Add(Vector3Scale(u, std::cos(a1)), Vector3Scale(v, std::sin(a1)));
        Vector3 nMid = Vector3Normalize(Vector3Add(n0, n1));
        Color c = shaded(color, 0.78f + 0.3f * Vector3DotProduct(nMid, light));
        Vector3 p0 = Vector3Add(a, Vector3Scale(n0, radius)), p1 = Vector3Add(a, Vector3Scale(n1, radius));
        Vector3 q0 = Vector3Add(b, Vector3Scale(n0, radius)), q1 = Vector3Add(b, Vector3Scale(n1, radius));
        mesh.add(p0, c);
        mesh.add(q0, c);
        mesh.add(q1, c);
        mesh.add(p0, c);
        mesh.add(q1, c);
        mesh.add(p1, c);
    }
}

void PlotRenderer::upload(MeshData& data) {
    if (data.count() < 3 || (!data.indices.empty() && data.indices.size() < 3)) return;
    Mesh mesh = {};
    mesh.vertexCount = static_cast<int>(data.count());
    mesh.triangleCount = static_cast<int>(data.indices.empty() ? data.count() / 3 : data.indices.size() / 3);
    mesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned>(data.positions.size() * sizeof(float))));
    mesh.colors = static_cast<unsigned char*>(MemAlloc(static_cast<unsigned>(data.colors.size())));
    std::memcpy(mesh.vertices, data.positions.data(), data.positions.size() * sizeof(float));
    std::memcpy(mesh.colors, data.colors.data(), data.colors.size());
    if (!data.indices.empty()) {
        mesh.indices = static_cast<unsigned short*>(
            MemAlloc(static_cast<unsigned>(data.indices.size() * sizeof(unsigned short))));
        std::memcpy(mesh.indices, data.indices.data(), data.indices.size() * sizeof(unsigned short));
    }
    UploadMesh(&mesh, false);
    m_meshes.push_back(mesh);
    data = MeshData{};
}

void PlotRenderer::releaseMeshes() {
    for (Mesh& mesh : m_meshes) {
        if (IsWindowReady()) {
            UnloadMesh(mesh);
        } else {
            // The GL context is gone (app shutdown): only free the CPU copies
            MemFree(mesh.vertices);
            MemFree(mesh.colors);
            MemFree(mesh.indices);
            MemFree(mesh.vboId);
        }
    }
    m_meshes.clear();
}

size_t PlotRenderer::vertexCount() const {
    size_t n = 0;
    for (const Mesh& m : m_meshes) n += static_cast<size_t>(m.vertexCount);
    return n;
}

bool PlotRenderer::rebuild(const PlotSpec& spec, const expr::Resolver* resolver) {
    m_resolver = resolver;
    m_type = spec.type;
    releaseMeshes();
    m_curve.clear();
    m_wireframe.clear();
    m_triangles = MeshData{};
    m_filled = 0;
    m_error.clear();

    auto parse = [&](Expression& e, const std::string& text, const char* name) {
        if (e.parse(text, resolver != nullptr)) return true;
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

    m_filled += m_triangles.count() / 3;
    if (m_curve.empty() && m_filled == 0 && m_error.empty()) {
        m_error = "nothing to draw: no finite values in range";
    }
    if (!m_error.empty()) {
        releaseMeshes();
        m_curve.clear();
        m_wireframe.clear();
        m_triangles = MeshData{};
        return false;
    }

    upload(m_triangles);
    MeshData tubes;
    tubes.positions.reserve(m_curve.size() * 36 * 3);
    tubes.colors.reserve(m_curve.size() * 36 * 4);
    for (const auto& seg : m_curve) addTube(tubes, seg.a, seg.b, 0.03f, seg.color);
    m_curve.clear();
    m_curve.shrink_to_fit();
    upload(tubes);
    return true;
}

void PlotRenderer::buildFunction2D(const PlotSpec& spec, const Expression& f) {
    int n = std::max(2, spec.samplesX);
    std::vector<float> ys(n + 1);
    std::vector<bool> ok(n + 1);
    m_minValue = kClip;
    m_maxValue = -kClip;

    for (int i = 0; i <= n; i++) {
        float x = spec.xMin + (spec.xMax - spec.xMin) * i / n;
        double y = eval(f, x, 0, 0);
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
                               : curveColor(spec);
        m_curve.push_back({mathToWorld(x0, ys[i], kLift), mathToWorld(x1, ys[i + 1], kLift), c});
    }
}

void PlotRenderer::buildParametric(const PlotSpec& spec, const Expression& ex, const Expression& ey,
                                   const Expression& ez) {
    int n = std::max(2, spec.samplesX);
    struct Sample { float x, y, z; bool ok; };
    std::vector<Sample> pts(n + 1);
    m_minValue = kClip;
    m_maxValue = -kClip;

    for (int i = 0; i <= n; i++) {
        double t = spec.tMin + (spec.tMax - spec.tMin) * i / n;
        double x = eval(ex, 0, 0, t), y = eval(ey, 0, 0, t), z = eval(ez, 0, 0, t);
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
        Color c = spec.heatmap ? heatColor(normalize((a.z + b.z) / 2, m_minValue, m_maxValue)) : curveColor(spec);
        m_curve.push_back({mathToWorld(a.x, a.y, a.z), mathToWorld(b.x, b.y, b.z), c});
    }
}

void PlotRenderer::buildSurface(const PlotSpec& spec, const Expression& f) {
    const int nx = std::max(2, spec.samplesX), ny = std::max(2, spec.samplesY);
    std::vector<float> z((nx + 1) * (ny + 1));
    std::vector<unsigned char> ok((nx + 1) * (ny + 1));
    auto idx = [nx](int i, int j) { return j * (nx + 1) + i; };
    auto xAt = [&](int i) { return spec.xMin + (spec.xMax - spec.xMin) * i / nx; };
    auto yAt = [&](int j) { return spec.yMin + (spec.yMax - spec.yMin) * j / ny; };
    const float dx = (spec.xMax - spec.xMin) / nx, dy = (spec.yMax - spec.yMin) / ny;

    m_minValue = kClip;
    m_maxValue = -kClip;
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            double v = eval(f, xAt(i), yAt(j), 0);
            ok[idx(i, j)] = usable(v);
            z[idx(i, j)] = static_cast<float>(v);
            if (usable(v)) {
                m_minValue = std::min(m_minValue, static_cast<float>(v));
                m_maxValue = std::max(m_maxValue, static_cast<float>(v));
            }
        }
    }

    // Per-vertex color: heat by height (or the plain color), shaded by the slope
    const Vector3 light = lightDir();
    const Color plain = surfaceColor(spec);
    std::vector<Color> colors((nx + 1) * (ny + 1));
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            if (!ok[idx(i, j)]) continue;
            auto zAt = [&](int ii, int jj) {
                ii = std::clamp(ii, 0, nx);
                jj = std::clamp(jj, 0, ny);
                return ok[idx(ii, jj)] ? z[idx(ii, jj)] : z[idx(i, j)];
            };
            float dzdx = (zAt(i + 1, j) - zAt(i - 1, j)) / ((i > 0 && i < nx ? 2.0f : 1.0f) * dx);
            float dzdy = (zAt(i, j + 1) - zAt(i, j - 1)) / ((j > 0 && j < ny ? 2.0f : 1.0f) * dy);
            Vector3 n = Vector3Normalize(mathToWorld(-dzdx, -dzdy, 1.0f));
            float shade = 0.6f + 0.45f * std::fabs(Vector3DotProduct(n, light));
            Color base = spec.heatmap ? heatColor(normalize(z[idx(i, j)], m_minValue, m_maxValue)) : plain;
            if (spec.heatmap) base.a = 245;
            colors[idx(i, j)] = shaded(base, shade);
        }
    }

    auto point = [&](int i, int j) { return mathToWorld(xAt(i), yAt(j), z[idx(i, j)]); };
    // About one wire line per unit, at most 60 per direction
    auto wireStep = [](int n, float span) {
        int lines = std::clamp(static_cast<int>(std::lround(span)), 8, 60);
        return std::max(1, n / lines);
    };
    const int stepX = wireStep(nx, spec.xMax - spec.xMin), stepY = wireStep(ny, spec.yMax - spec.yMin);
    const Color wire = Fade(ui::theme().dark ? BLACK : WHITE, 0.28f);
    // Indexed meshes share each vertex between its cells. raylib indices are
    // 16-bit, so the grid is split into bands of rows under 65536 vertices.
    const int rowsPerBand = std::max(1, 65535 / (nx + 1) - 1);
    for (int j0 = 0; spec.smooth && j0 < ny; j0 += rowsPerBand) {
        int j1 = std::min(ny, j0 + rowsPerBand);
        MeshData band;
        band.positions.reserve(static_cast<size_t>(j1 - j0 + 1) * (nx + 1) * 3);
        band.colors.reserve(static_cast<size_t>(j1 - j0 + 1) * (nx + 1) * 4);
        for (int j = j0; j <= j1; j++)
            for (int i = 0; i <= nx; i++) band.add(ok[idx(i, j)] ? point(i, j) : Vector3{}, colors[idx(i, j)]);
        auto local = [&](int i, int j) { return static_cast<unsigned short>((j - j0) * (nx + 1) + i); };
        for (int j = j0; j < j1; j++) {
            for (int i = 0; i < nx; i++) {
                bool cellOk = ok[idx(i, j)] && ok[idx(i + 1, j)] && ok[idx(i + 1, j + 1)] && ok[idx(i, j + 1)];
                if (!cellOk) continue;
                band.indices.insert(band.indices.end(), {local(i, j), local(i + 1, j), local(i + 1, j + 1),
                                                         local(i, j), local(i + 1, j + 1), local(i, j + 1)});
            }
        }
        m_filled += band.indices.size() / 3;
        upload(band);
    }
    if (!spec.smooth) {
        // Faceted: flat cells, one color each (heat by the cell's average height), no shading
        m_triangles.positions.reserve(static_cast<size_t>(nx) * ny * 18);
        m_triangles.colors.reserve(static_cast<size_t>(nx) * ny * 24);
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                bool cellOk = ok[idx(i, j)] && ok[idx(i + 1, j)] && ok[idx(i + 1, j + 1)] && ok[idx(i, j + 1)];
                if (!cellOk) continue;
                float avg = (z[idx(i, j)] + z[idx(i + 1, j)] + z[idx(i + 1, j + 1)] + z[idx(i, j + 1)]) / 4.0f;
                Color c = spec.heatmap ? heatColor(normalize(avg, m_minValue, m_maxValue)) : plain;
                Vector3 p00 = point(i, j), p10 = point(i + 1, j), p11 = point(i + 1, j + 1), p01 = point(i, j + 1);
                m_triangles.add(p00, c);
                m_triangles.add(p10, c);
                m_triangles.add(p11, c);
                m_triangles.add(p00, c);
                m_triangles.add(p11, c);
                m_triangles.add(p01, c);
            }
        }
    }

    // Wire lines: about one per unit, each sampled with at most ~150 segments
    // (they are drawn every frame, the surface itself is a GPU mesh)
    const int alongX = std::max(1, nx / 150), alongY = std::max(1, ny / 150);
    auto wireLine = [&](int i0, int j0, int di, int dj, int count, int stride) {
        for (int k = 0; k < count; k += stride) {
            int k1 = std::min(k + stride, count);
            int ia = i0 + di * k, ja = j0 + dj * k, ib = i0 + di * k1, jb = j0 + dj * k1;
            if (ok[idx(ia, ja)] && ok[idx(ib, jb)]) m_wireframe.push_back({point(ia, ja), point(ib, jb), wire});
        }
    };
    for (int j = 0; j <= ny; j += stepY) wireLine(0, j, 1, 0, nx, alongX);
    if (ny % stepY) wireLine(0, ny, 1, 0, nx, alongX);
    for (int i = 0; i <= nx; i += stepX) wireLine(i, 0, 0, 1, ny, alongY);
    if (nx % stepX) wireLine(nx, 0, 0, 1, ny, alongY);
}

void PlotRenderer::buildImplicit(const PlotSpec& spec, const Expression& f) {
    const int nx = std::max(2, spec.samplesX), ny = std::max(2, spec.samplesY);
    std::vector<double> v((nx + 1) * (ny + 1));
    auto idx = [nx](int i, int j) { return j * (nx + 1) + i; };
    auto xAt = [&](int i) { return spec.xMin + (spec.xMax - spec.xMin) * i / nx; };
    auto yAt = [&](int j) { return spec.yMin + (spec.yMax - spec.yMin) * j / ny; };

    double maxAbs = 0.0;
    for (int j = 0; j <= ny; j++) {
        for (int i = 0; i <= nx; i++) {
            double val = eval(f, xAt(i), yAt(j), 0);
            v[idx(i, j)] = val;
            if (std::isfinite(val)) maxAbs = std::max(maxAbs, std::fabs(val));
        }
    }
    m_minValue = static_cast<float>(-maxAbs);
    m_maxValue = static_cast<float>(maxAbs);

    // Heat map of f behind the curve: negative = blue, positive = red
    if (spec.heatmap && maxAbs > 0.0) {
        // The background does not need the full curve detail
        int stepX = std::max(1, nx / 160), stepY = std::max(1, ny / 160);
        for (int j = 0; j < ny; j += stepY) {
            for (int i = 0; i < nx; i += stepX) {
                int i1 = std::min(i + stepX, nx), j1 = std::min(j + stepY, ny);
                double c = v[idx(i, j)];
                if (!std::isfinite(c)) continue;
                // Compress the range so values near zero still show contrast
                float t = 0.5f + 0.5f * static_cast<float>(std::tanh(3.0 * c / maxAbs));
                Color col = Fade(heatColor(t), 0.35f);
                Vector3 a = mathToWorld(xAt(i), yAt(j), kLift / 2), b = mathToWorld(xAt(i1), yAt(j), kLift / 2);
                Vector3 d = mathToWorld(xAt(i1), yAt(j1), kLift / 2), e = mathToWorld(xAt(i), yAt(j1), kLift / 2);
                m_triangles.add(a, col);
                m_triangles.add(b, col);
                m_triangles.add(d, col);
                m_triangles.add(a, col);
                m_triangles.add(d, col);
                m_triangles.add(e, col);
            }
        }
    }

    // Marching squares. Corners: 0 = (i,j), 1 = (i+1,j), 2 = (i+1,j+1), 3 = (i,j+1).
    // Edges: 0 = bottom (0-1), 1 = right (1-2), 2 = top (2-3), 3 = left (3-0).
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
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
                Vector3 a = edgePoint(e0), b = edgePoint(e1);
                m_curve.push_back({mathToWorld(a.x, a.y, kLift), mathToWorld(b.x, b.y, kLift), curveColor(spec)});
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
    rlDrawRenderBatchActive();
    rlDisableBackfaceCulling();  // surfaces are visible from above and below
    for (const Mesh& mesh : m_meshes) DrawMesh(mesh, plotMaterial(), MatrixIdentity());
    rlEnableBackfaceCulling();
    for (const auto& s : m_wireframe) DrawLine3D(s.a, s.b, s.color);
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
