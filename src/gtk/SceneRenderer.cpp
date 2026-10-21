#include "gtk/SceneRenderer.hpp"
#include "math/GridMath.hpp"
#include "gfx/GpuContext.hpp"
#include "ui/core/Overlay.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
#include "view/AxisNumbers.hpp"
#include "view/ViewStyle.hpp"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <dlfcn.h>
#include <algorithm>
#include <cmath>

namespace linalg::gtk {

namespace {

// GL function loader for rlgl (glad inside raylib). GTK makes its contexts with EGL (Wayland, X11 by default)
// or GLX (older X11 setups); ask whichever owns the current context. Both libraries are already loaded by GTK.
using ProcLoader = void* (*)(const char*);
ProcLoader g_eglProc = nullptr;
ProcLoader g_glxProc = nullptr;

void* loadGlProc(const char* name) {
    void* p = g_eglProc ? g_eglProc(name) : nullptr;
    if (!p && g_glxProc) p = g_glxProc(name);
    if (!p) p = dlsym(RTLD_DEFAULT, name);
    return p;
}

bool findLoader() {
    if (void* egl = dlopen("libEGL.so.1", RTLD_LAZY | RTLD_LOCAL)) {
        auto current = reinterpret_cast<void* (*)()>(dlsym(egl, "eglGetCurrentContext"));
        if (current && current()) g_eglProc = reinterpret_cast<ProcLoader>(dlsym(egl, "eglGetProcAddress"));
    }
    if (!g_eglProc) {
        if (void* gl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL))
            g_glxProc = reinterpret_cast<ProcLoader>(dlsym(gl, "glXGetProcAddressARB"));
    }
    return g_eglProc || g_glxProc;
}

// What raylib's BeginDrawing/BeginMode3D do, for a framebuffer raylib didn't make
void begin2D(int w, int h) {
    rlViewport(0, 0, w, h);
    rlSetFramebufferWidth(w);
    rlSetFramebufferHeight(h);
    rlMatrixMode(RL_PROJECTION);
    rlLoadIdentity();
    rlOrtho(0, w, h, 0, 0.0, 1.0);
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
}

void begin3D(const Camera3D& camera, int w, int h) {
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlPushMatrix();
    rlLoadIdentity();
    double aspect = static_cast<double>(w) / std::max(1, h);
    double nearPlane = rlGetCullDistanceNear(), farPlane = rlGetCullDistanceFar();
    if (camera.projection == CAMERA_PERSPECTIVE) {
        double top = nearPlane * std::tan(camera.fovy * 0.5 * DEG2RAD);
        rlFrustum(-top * aspect, top * aspect, -top, top, nearPlane, farPlane);
    } else {
        double top = camera.fovy / 2.0;
        rlOrtho(-top * aspect, top * aspect, -top, top, nearPlane, farPlane);
    }
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlMultMatrixf(MatrixToFloat(MatrixLookAt(camera.position, camera.target, camera.up)));
    rlEnableDepthTest();
}

void end3D() {
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION);
    rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW);
    rlLoadIdentity();
    rlDisableDepthTest();
}

} // namespace

SceneRenderer::SceneRenderer(scene::Scene& scene) : m_scene(scene) {
    m_camera.reset(m_mode);
}

SceneRenderer::~SceneRenderer() = default;

bool SceneRenderer::start(int pixelWidth, int pixelHeight, std::string& error) {
    if (m_started) return true;
    if (!findLoader()) {
        error = "no EGL or GLX library to load OpenGL functions from";
        return false;
    }
    rlLoadExtensions(reinterpret_cast<void*>(&loadGlProc));
    rlglInit(pixelWidth, pixelHeight);
    if (rlGetVersion() < RL_OPENGL_33) {
        error = "OpenGL 3.3 is needed";
        rlglClose();
        return false;
    }
    // raylib's shapes assume rlgl's white texture is GL texture 1 (true in a window raylib opened); GTK's
    // contexts share texture names, so tell raylib the real one
    SetShapesTexture(Texture2D{rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8},
                     Rectangle{0.0f, 0.0f, 1.0f, 1.0f});
    setExternalGpuContext(true);
    ui::setScreenSize(static_cast<float>(pixelWidth), static_cast<float>(pixelHeight));
    ui::init();  // the font atlases for labels (GPU textures, so after rlgl)
    m_sceneView = std::make_unique<view::SceneView>();
    m_gridShader = std::make_unique<GridShader>();
    m_started = true;
    return true;
}

void SceneRenderer::stop() {
    if (!m_started) return;
    m_sceneView.reset();
    m_gridShader.reset();
    rlglClose();
    setExternalGpuContext(false);
    m_started = false;
}

scene::SpaceKind SceneRenderer::spaceKind() const {
    return m_mode == AppMode::Complex ? scene::SpaceKind::Complex : scene::SpaceKind::Euclidean3D;
}

void SceneRenderer::setMode(AppMode mode) {
    if (mode == m_mode) return;
    m_mode = mode;
    m_camera.setSpace(mode);
    m_camera.reset(mode);
}

bool SceneRenderer::animating() const { return m_scene.timeline().playing; }

void SceneRenderer::orbit(float dx, float dy, float viewHeight) {
    OrbitInput in;
    in.orbitDrag = {dx, dy};
    in.viewportHeight = viewHeight;
    m_camera.applyOrbit(in);
}

void SceneRenderer::pan(float dx, float dy, float viewHeight) {
    OrbitInput in;
    in.panDrag = {dx, dy};
    in.viewportHeight = viewHeight;
    m_camera.applyOrbit(in);
}

void SceneRenderer::zoom(float wheelSteps, float viewHeight) {
    OrbitInput in;
    in.wheel = wheelSteps;
    in.viewportHeight = viewHeight;
    m_camera.applyOrbit(in);
}

double SceneRenderer::pixelsPerUnit(int pixelHeight) const {
    return grid::pixelsPerUnit(m_camera.viewDistance(), m_camera.camera().fovy, pixelHeight);
}

double SceneRenderer::tickStep(int pixelHeight) const {
    return grid::tickStep(pixelsPerUnit(pixelHeight), 70.0);
}

view::DrawOptions SceneRenderer::drawOptions(int pixelHeight) {
    view::DrawOptions o;
    o.view = &m_view;
    o.selection = &m_scene.selection(spaceKind());
    o.print = view::activeStyle().print;
    o.solo = m_view.soloSelection;
    o.tickStep = tickStep(pixelHeight);
    o.cameraPosition = m_camera.position();
    o.worldPerPixel = 2.0f * std::tan(m_camera.camera().fovy * DEG2RAD / 2.0f) / static_cast<float>(pixelHeight);
    return o;
}

// The same grid as App::drawGrid (to be shared once both front ends use one Session, run 3)
void SceneRenderer::drawGrid(int pixelHeight) {
    if (!m_view.showGrid) return;
    const view::ViewStyle& style = view::activeStyle();
    Color color = style.grid.a ? style.grid : ui::theme().grid;
    float reach = std::max(12.0f, m_camera.viewDistance() * 6.0f);
    if (!m_gridShader->valid()) {
        m_grid.setSize(std::clamp(static_cast<int>(reach), 10, 100));
        if (m_mode == AppMode::Space3D) m_grid.draw();
        else m_grid.drawPlaneXY();
        return;
    }
    GridSpec spec;
    spec.color = color;
    spec.radius = reach;
    spec.level = m_view.gridAuto ? grid::autoLevel(pixelsPerUnit(pixelHeight), 14.0)
                                 : grid::fixedLevel(1.0, 5.0);
    Vector3 c = m_camera.viewCenter();
    if (m_mode == AppMode::Space3D) {
        if (m_view.gridFloor) {
            spec.center = {c.x, 0.0f, c.z};
            m_gridShader->draw(GridPlane::WorldXZ, spec);
        }
        if (m_view.gridWallXZ) {
            spec.center = {c.x, c.y, 0.0f};
            m_gridShader->draw(GridPlane::WorldXY, spec);
        }
        if (m_view.gridWallYZ) {
            spec.center = {0.0f, c.y, c.z};
            m_gridShader->draw(GridPlane::WorldYZ, spec);
        }
    } else {
        spec.center = {c.x, c.y, 0.0f};
        spec.polar = m_view.gridPolar;
        m_gridShader->draw(GridPlane::WorldXY, spec);
    }
}

void SceneRenderer::drawLabels(int pixelHeight) {
    ui::OverlayMeasure measure;
    gfx::Overlay overlay;
    if (m_view.showNumbers && m_view.showAxes) {
        view::AxisNumbersSpec numbers;
        numbers.space = spaceKind();
        numbers.step = tickStep(pixelHeight);
        numbers.axisLength = m_view.axisLength;
        numbers.viewCenter = m_camera.viewCenter();
        numbers.reach = std::max(12.0f, m_camera.viewDistance() * 6.0f);
        const view::ViewStyle& style = view::activeStyle();
        numbers.color = style.print ? Color{90, 90, 90, 255} : style.muted;
        view::axisNumbers(m_camera.camera(), numbers, measure, overlay);
    }
    m_sceneView->labels(m_scene.space(spaceKind()), drawOptions(pixelHeight), m_camera.camera(), measure, overlay);
    ui::paintOverlay(overlay);  // with raylib's text inside the GL area, for now
}

void SceneRenderer::render(int pixelWidth, int pixelHeight, double dt) {
    if (!m_started || pixelWidth <= 0 || pixelHeight <= 0) return;
    ui::setScreenSize(static_cast<float>(pixelWidth), static_cast<float>(pixelHeight));
    ui::beginFrame();
    m_scene.advance(dt);
    m_scene.evaluate();
    view::setActiveStyle(view::styleFor(m_view.style, ui::theme()));
    m_camera.update(static_cast<float>(dt), FlyInput{});  // no fly mode in the GTK app yet

    begin2D(pixelWidth, pixelHeight);
    Color bg = view::activeStyle().background;
    rlClearColor(bg.r, bg.g, bg.b, 255);
    rlClearScreenBuffers();

    begin3D(m_camera.camera(), pixelWidth, pixelHeight);
    drawGrid(pixelHeight);
    m_sceneView->draw3D(m_scene.space(spaceKind()), drawOptions(pixelHeight));
    end3D();
    drawLabels(pixelHeight);
    rlDrawRenderBatchActive();
}

} // namespace linalg::gtk
