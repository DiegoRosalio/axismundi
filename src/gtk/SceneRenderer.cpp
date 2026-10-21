#include "gtk/SceneRenderer.hpp"
#include "gfx/GpuContext.hpp"
#include "ui/core/Overlay.hpp"
#include "ui/core/Theme.hpp"
#include "ui/core/Ui.hpp"
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
    m_started = true;
    return true;
}

void SceneRenderer::stop() {
    if (!m_started) return;
    m_session.releaseGpu();
    ui::shutdown();
    rlglClose();
    setExternalGpuContext(false);
    m_started = false;
}

void SceneRenderer::orbit(float dx, float dy, float viewHeight) {
    OrbitInput in;
    in.orbitDrag = {dx, dy};
    in.viewportHeight = viewHeight;
    m_session.camera().applyOrbit(in);
}

void SceneRenderer::pan(float dx, float dy, float viewHeight) {
    OrbitInput in;
    in.panDrag = {dx, dy};
    in.viewportHeight = viewHeight;
    m_session.camera().applyOrbit(in);
}

void SceneRenderer::zoom(float wheelSteps, float viewHeight) {
    OrbitInput in;
    in.wheel = wheelSteps;
    in.viewportHeight = viewHeight;
    m_session.camera().applyOrbit(in);
}

void SceneRenderer::render(int pixelWidth, int pixelHeight, double dt) {
    if (!m_started || pixelWidth <= 0 || pixelHeight <= 0) return;
    ui::setScreenSize(static_cast<float>(pixelWidth), static_cast<float>(pixelHeight));
    ui::beginFrame();
    scene::Scene& scene = m_session.scene();
    scene.advance(dt);
    scene.evaluate();
    m_session.applyViewStyle();
    m_session.camera().update(static_cast<float>(dt), FlyInput{});  // no fly mode in the GTK app yet

    begin2D(pixelWidth, pixelHeight);
    Color bg = view::activeStyle().background;
    rlClearColor(bg.r, bg.g, bg.b, 255);
    rlClearScreenBuffers();

    view::DrawOptions options = m_session.drawOptions(pixelHeight);
    begin3D(m_session.camera().camera(), pixelWidth, pixelHeight);
    m_session.drawGrid(pixelHeight);
    m_session.draw3D(options);
    end3D();
    ui::OverlayMeasure measure;
    gfx::Overlay overlay;
    m_session.labels(options, pixelHeight, measure, overlay);
    ui::paintOverlay(overlay);  // with raylib's text inside the GL area, for now
    rlDrawRenderBatchActive();
}

} // namespace linalg::gtk
