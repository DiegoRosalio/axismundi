#include "rendering/GridShader.hpp"
#include "gfx/Canvas3D.hpp"
#include <cstdio>
#include <cstdlib>

namespace linalg {

namespace {

// The vertex shader passes each corner's world position on; the GPU interpolates it across the square,
// so the fragment shader knows where on the plane every pixel is.
const char* kVertex = R"(#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
out vec3 worldPos;
void main() {
    worldPos = vertexPosition;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

// The fragment shader runs for every pixel of the square and decides how much grid line is there.
// fwidth(c) is how much c changes from one pixel to the next: dividing the distance to the nearest line by
// it gives the distance in pixels, which makes lines one pixel wide and smooth at any zoom or angle.
const char* kFragment = R"(#version 330
in vec3 worldPos;
out vec4 finalColor;
uniform vec4 gridColor;
uniform vec3 center;
uniform float radius;
uniform float spacing;
uniform float fade;
uniform float ratio;
uniform int plane;   // 0: world XZ, 1: world XY, 2: world YZ
uniform int polar;

// The same formula as grid::familyStrength (math/GridMath.cpp)
float familyStrength(float j) {
    float level = j + clamp(fade, 0.0, 1.0);
    return 0.55 * clamp(level, 0.0, 1.0) + 0.45 * clamp(level - 1.0, 0.0, 1.0);
}

// How much of a line passes through this pixel (0..1) for lines at every `s` units of c
float lines1(float c, float s) {
    float u = c / s;
    float w = max(fwidth(u), 1e-6);
    float cover = 1.0 - min(abs(fract(u - 0.5) - 0.5) / w, 1.0);
    return cover * (1.0 - smoothstep(0.2, 0.5, w));  // lines closer than ~3 px fade out (no moire)
}
float lines2(vec2 p, float s) {
    vec2 u = p / s;
    vec2 w = max(fwidth(u), vec2(1e-6));
    vec2 g = abs(fract(u - 0.5) - 0.5) / w;
    float cover = 1.0 - min(min(g.x, g.y), 1.0);
    return cover * (1.0 - smoothstep(0.2, 0.5, max(w.x, w.y)));
}

void main() {
    vec2 p = plane == 0 ? worldPos.xz : (plane == 1 ? worldPos.xy : worldPos.zy);
    vec2 c = plane == 0 ? center.xz : (plane == 1 ? center.xy : center.zy);
    float a = 0.0;
    for (int j = 0; j < 3; j++) {
        float s = spacing * pow(ratio, float(j));
        float cover = polar == 1 ? lines1(length(p), s) : lines2(p, s);
        a = max(a, cover * familyStrength(float(j)));
    }
    if (polar == 1) {
        // Rays every 15 degrees, stronger every 45; their pixel width from how fast the angle changes
        float r = length(p);
        float turns = atan(p.y, p.x) / 6.2831853 * 24.0;
        float w = length(fwidth(p)) / max(r, 1e-6) * 24.0 / 6.2831853;
        float ray = (1.0 - min(abs(fract(turns - 0.5) - 0.5) / max(w, 1e-6), 1.0)) * (1.0 - smoothstep(0.15, 0.4, w));
        float major = abs(mod(floor(turns + 0.5), 3.0)) < 0.5 ? 1.0 : 0.55;
        a = max(a, ray * major);
    }
    // Fade out toward the edge of the square
    a *= 1.0 - smoothstep(0.55 * radius, radius, length(p - c));
    // Minor lines at the style's alpha, the strongest ones about 1.8 times as strong
    finalColor = vec4(gridColor.rgb, clamp(gridColor.a * a / 0.55, 0.0, 1.0));
    if (finalColor.a < 0.004) discard;
}
)";

}  // namespace

GridShader::~GridShader() {
    if (m_ok) gfx::canvas().unloadShader(m_shader);  // frees the GPU copy only while the GL context exists
}

void GridShader::release() {
    if (m_ok) gfx::canvas().unloadShader(m_shader);
    m_shader = 0;
    m_ok = false;
    m_tried = false;
}

bool GridShader::valid() {
    if (m_tried) return m_ok;
    m_tried = true;
    if (std::getenv("AXISMUNDI_NO_SHADER")) {  // for testing the fallback
        std::fprintf(stderr, "INFO: GRID: shader grid disabled (AXISMUNDI_NO_SHADER), using lines\n");
        return false;
    }
    gfx::Canvas3D& c = gfx::canvas();
    m_shader = c.loadShader(kVertex, kFragment);
    m_ok = m_shader != 0;
    if (!m_ok) {
        std::fprintf(stderr, "WARNING: GRID: the grid shader did not compile; using the line grid instead\n");
        return false;
    }
    m_locColor = c.uniformLocation(m_shader, "gridColor");
    m_locCenter = c.uniformLocation(m_shader, "center");
    m_locRadius = c.uniformLocation(m_shader, "radius");
    m_locSpacing = c.uniformLocation(m_shader, "spacing");
    m_locFade = c.uniformLocation(m_shader, "fade");
    m_locRatio = c.uniformLocation(m_shader, "ratio");
    m_locPlane = c.uniformLocation(m_shader, "plane");
    m_locPolar = c.uniformLocation(m_shader, "polar");
    return true;
}

void GridShader::draw(GridPlane plane, const GridSpec& spec) {
    if (!valid()) return;
    float color[4] = {spec.color.r / 255.0f, spec.color.g / 255.0f, spec.color.b / 255.0f, spec.color.a / 255.0f};
    float center[3] = {spec.center.x, spec.center.y, spec.center.z};
    float spacing = static_cast<float>(spec.level.spacing), fade = static_cast<float>(spec.level.fade);
    float ratio = static_cast<float>(spec.level.ratio), radius = spec.radius;
    int planeIndex = static_cast<int>(plane), polar = spec.polar ? 1 : 0;

    gfx::Canvas3D& canvas = gfx::canvas();
    canvas.beginShader(m_shader);  // flushes what was drawn before with the default shader
    canvas.setUniform(m_shader, m_locColor, color, gfx::UniformType::Vec4);
    canvas.setUniform(m_shader, m_locCenter, center, gfx::UniformType::Vec3);
    canvas.setUniform(m_shader, m_locRadius, &radius, gfx::UniformType::Float);
    canvas.setUniform(m_shader, m_locSpacing, &spacing, gfx::UniformType::Float);
    canvas.setUniform(m_shader, m_locFade, &fade, gfx::UniformType::Float);
    canvas.setUniform(m_shader, m_locRatio, &ratio, gfx::UniformType::Float);
    canvas.setUniform(m_shader, m_locPlane, &planeIndex, gfx::UniformType::Int);
    canvas.setUniform(m_shader, m_locPolar, &polar, gfx::UniformType::Int);
    gfx::setDepthWrite(false);
    gfx::setBackfaceCulling(false);
    // One square, `radius` from the center in each direction, in the chosen plane
    const float r = radius;
    Vector3 c = spec.center;
    Vector3 corners[4];
    for (int k = 0; k < 4; k++) {
        float du = (k == 0 || k == 3) ? -r : r, dv = (k < 2) ? -r : r;
        switch (plane) {
            case GridPlane::WorldXZ: corners[k] = {c.x + du, c.y, c.z + dv}; break;
            case GridPlane::WorldXY: corners[k] = {c.x + du, c.y + dv, c.z}; break;
            case GridPlane::WorldYZ: corners[k] = {c.x, c.y + dv, c.z + du}; break;
        }
    }
    gfx::begin(gfx::Primitive::Quads);
    gfx::color(255, 255, 255, 255);
    for (const Vector3& v : corners) gfx::vertex(v.x, v.y, v.z);
    gfx::end();
    canvas.endShader();  // draws the square with the grid shader
    gfx::setBackfaceCulling(true);
    gfx::setDepthWrite(true);
}

} // namespace linalg
