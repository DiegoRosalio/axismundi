#include "app/Session.hpp"
#include "gfx/Pure.hpp"
#include "math/GridMath.hpp"
#include "view/ObjectViews.hpp"
#include "scene/Templates.hpp"
#include "text/Numbers.hpp"
#include "ui/core/Theme.hpp"
#include "view/AxisNumbers.hpp"
#include "view/ViewStyle.hpp"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <sstream>

namespace linalg {

using scene::SceneObject;
using scene::SpaceKind;

Session::Session() {
    m_camera.reset(m_mode);
}

SpaceKind Session::kindOf(AppMode mode) {
    return mode == AppMode::Space3D ? SpaceKind::Euclidean3D : SpaceKind::Complex;
}

void Session::setMode(AppMode mode) {
    if (mode == m_mode) return;
    int from = static_cast<int>(m_mode), to = static_cast<int>(mode);
    m_camera.setSpace(mode);
    m_savedPosition[from] = m_camera.position();
    m_savedTarget[from] = m_camera.target();
    m_hasSavedView[from] = true;
    if (m_hasSavedView[to]) m_camera.setView(m_savedPosition[to], m_savedTarget[to]);
    else m_camera.reset(mode);
    m_mode = mode;
}

void Session::adoptMode(AppMode mode) { m_mode = mode; }

void Session::applyViewStyle() {
    view::setActiveStyle(view::styleFor(m_view.style, ui::theme()));
    m_grid.setShowGrid(m_view.showGrid);
    m_grid.setShowAxes(m_view.showAxes);
    m_grid.setGridColor(view::activeStyle().grid);
}

void Session::releaseGpu() {
    m_sceneView.releaseGpu();
    m_gridShader.release();
}

// --- Drawing -------------------------------------------------------------------------

double Session::pixelsPerUnit(int viewHeight) const {
    return grid::pixelsPerUnit(m_camera.viewDistance(), m_camera.camera().fovy, viewHeight);
}

double Session::tickStep(int viewHeight) const {
    return grid::tickStep(pixelsPerUnit(viewHeight), 70.0);  // numbers at least 70 px apart at the view center
}

view::DrawOptions Session::drawOptions(int viewHeight) const {
    view::DrawOptions o;
    o.view = &m_view;
    o.selection = &const_cast<scene::Scene&>(m_scene).selection(spaceKind());
    o.print = view::activeStyle().print;
    o.solo = m_view.soloSelection;
    o.tickStep = tickStep(viewHeight);
    o.cameraPosition = m_camera.position();
    o.worldPerPixel = 2.0f * std::tan(m_camera.camera().fovy * DEG2RAD / 2.0f) / static_cast<float>(viewHeight);
    return o;
}

void Session::drawGrid(int viewHeight) {
    if (!m_view.showGrid) return;
    const view::ViewStyle& style = view::activeStyle();
    Color color = style.grid.a ? style.grid : ui::theme().grid;
    float distance = m_camera.viewDistance();
    float reach = std::max(12.0f, distance * 6.0f);  // far enough to fill the view
    if (!m_gridShader.valid()) {
        // The line grid: fixed lines every unit, as far as the view reaches
        m_grid.setSize(std::clamp(static_cast<int>(reach), 10, 100));
        if (m_mode == AppMode::Space3D) m_grid.draw();
        else m_grid.drawPlaneXY();
        return;
    }
    GridSpec spec;
    spec.color = color;
    spec.radius = reach;
    spec.level = m_view.gridAuto ? grid::autoLevel(pixelsPerUnit(viewHeight), 14.0) : grid::fixedLevel(1.0, 5.0);
    Vector3 c = m_camera.viewCenter();
    if (m_mode == AppMode::Space3D) {
        // Each plane's square is centered on the point of that plane closest to the view center
        if (m_view.gridFloor) {
            spec.center = {c.x, 0.0f, c.z};
            m_gridShader.draw(GridPlane::WorldXZ, spec);
        }
        if (m_view.gridWallXZ) {
            spec.center = {c.x, c.y, 0.0f};
            m_gridShader.draw(GridPlane::WorldXY, spec);
        }
        if (m_view.gridWallYZ) {
            spec.center = {0.0f, c.y, c.z};
            m_gridShader.draw(GridPlane::WorldYZ, spec);
        }
    } else {
        spec.center = {c.x, c.y, 0.0f};
        spec.polar = m_view.gridPolar;
        m_gridShader.draw(GridPlane::WorldXY, spec);
    }
}

void Session::draw3D(const view::DrawOptions& options) {
    m_sceneView.draw3D(m_scene.space(spaceKind()), options);
}

void Session::labels(const view::DrawOptions& options, int viewHeight, const gfx::TextMeasure& measure,
                     gfx::Overlay& out) {
    if (m_view.showNumbers && m_view.showAxes) {
        view::AxisNumbersSpec numbers;
        numbers.space = spaceKind();
        numbers.step = tickStep(viewHeight);
        numbers.axisLength = m_view.axisLength;
        numbers.viewCenter = m_camera.viewCenter();
        numbers.reach = std::max(12.0f, m_camera.viewDistance() * 6.0f);
        const view::ViewStyle& style = view::activeStyle();
        numbers.color = style.print ? Color{90, 90, 90, 255} : style.muted;
        view::axisNumbers(m_camera.camera(), numbers, measure, out);
    }
    m_sceneView.labels(m_scene.space(spaceKind()), options, m_camera.camera(), measure, out);
}

// --- Scripting -------------------------------------------------------------------------

SceneObject* Session::object(const std::string& name, AppMode mode) {
    return space(mode).find(name);
}

SceneObject* Session::addObject(AppMode mode, const std::string& typeId, const std::string& name,
                                const std::map<std::string, std::string>& fields,
                                const std::map<std::string, std::string>& props) {
    AddChoice c;
    c.typeId = typeId;
    c.name = name;
    c.fields = fields;
    c.props = props;
    return applyAddChoice(m_scene, kindOf(mode), c);
}

void Session::select(const std::string& name, AppMode mode, bool add) {
    SceneObject* o = object(name, mode);
    if (!o) return;
    if (add) m_scene.selection(kindOf(mode)).toggle(o->id());
    else m_scene.selection(kindOf(mode)).set(o->id());
}

void Session::clearSpace(AppMode mode) {
    m_scene.space(kindOf(mode)).clear();
    m_scene.selection(kindOf(mode)).clear();
    m_scene.commit();
}

void Session::resetScene() { m_scene.loadDefaultScene(); }

bool Session::deleteSelection() {
    SpaceKind kind = spaceKind();
    auto ids = m_scene.selection(kind).ids;
    if (ids.empty()) return false;
    for (auto id : ids) m_scene.space(kind).remove(id);
    m_scene.selection(kind).clear();
    m_scene.commit();
    return true;
}

bool Session::duplicateSelection() {
    SpaceKind kind = spaceKind();
    scene::ObjectId id = m_scene.selection(kind).primary();
    if (id == scene::kNoObject) return false;
    SceneObject* copy = m_scene.space(kind).duplicate(id);
    if (!copy) return false;
    m_scene.selection(kind).set(copy->id());
    m_scene.commit();
    return true;
}

void Session::setMatrix3(const Matrix3D& m) {
    auto* obj = dynamic_cast<scene::MatrixObject*>(object("M"));
    if (!obj) obj = static_cast<scene::MatrixObject*>(addObject(AppMode::Space3D, "matrix", "M"));
    obj->setByFormula(false);
    obj->setSize(3);
    obj->visible = true;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            char buf[32];
            formatNumber(buf, sizeof(buf), m.m[i][j]);
            obj->setField(scene::MatrixObject::cellKey(i, j), buf);
        }
    m_scene.commit();
}

void Session::setMatrix4(const Matrix4D& m) {
    auto* obj = dynamic_cast<scene::MatrixObject*>(object("M"));
    if (!obj) obj = static_cast<scene::MatrixObject*>(addObject(AppMode::Space3D, "matrix", "M"));
    obj->setByFormula(false);
    obj->setSize(4);
    obj->visible = true;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            char buf[32];
            formatNumber(buf, sizeof(buf), m.m[i][j]);
            obj->setField(scene::MatrixObject::cellKey(i, j), buf);
        }
    m_scene.commit();
}

void Session::setVectors(const Vector3D& a, const Vector3D& b) {
    const char* names[2] = {"a", "b"};
    const Vector3D values[2] = {a, b};
    for (int k = 0; k < 2; k++) {
        auto* v = dynamic_cast<scene::VectorObject*>(object(names[k]));
        if (!v) v = static_cast<scene::VectorObject*>(addObject(AppMode::Space3D, "vector", names[k]));
        v->setByFormula(false);
        char buf[32];
        formatNumber(buf, sizeof(buf), values[k].x);
        v->setField("x", buf);
        formatNumber(buf, sizeof(buf), values[k].y);
        v->setField("y", buf);
        formatNumber(buf, sizeof(buf), values[k].z);
        v->setField("z", buf);
    }
    m_scene.commit();
}

void Session::applyMatrixPreset(const Matrix3D& m) {
    auto* obj = dynamic_cast<scene::MatrixObject*>(const_cast<SceneObject*>(targetMatrix()));
    if (!obj) {
        setMatrix3(m);
        return;
    }
    m_scene.evaluate();  // presets compose with the matrix's current value
    obj->applyPreset(m);
    m_scene.evaluate();
    m_scene.commit();
}

void Session::orbitBy(float dxPixels, float dyPixels, float viewHeight) {
    OrbitInput in;
    in.orbitDrag = {dxPixels, dyPixels};
    in.viewportHeight = viewHeight;
    m_camera.applyOrbit(in);
}

bool Session::focusSelection(const view::DrawOptions& options) {
    SpaceKind kind = spaceKind();
    scene::ObjectId id = m_scene.selection(kind).primary();
    const SceneObject* o = id == scene::kNoObject ? nullptr : m_scene.space(kind).find(id);
    Vector3 p{0, 0, 0};
    bool found = o && view::SceneView::anchorPoint(*o, options, p);
    m_camera.focus(found ? p : Vector3{0, 0, 0});
    return found;
}

// --- Picking and files ----------------------------------------------------------------

namespace {

float distanceToSegment(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 ab = Vector2Subtract(b, a);
    float len2 = Vector2LengthSqr(ab);
    float t = len2 > 1e-6f ? std::clamp(Vector2DotProduct(Vector2Subtract(p, a), ab) / len2, 0.0f, 1.0f) : 0.0f;
    return Vector2Distance(p, Vector2Add(a, Vector2Scale(ab, t)));
}

bool inFront(const Camera3D& cam, Vector3 p) {
    return Vector3DotProduct(Vector3Subtract(cam.target, cam.position), Vector3Subtract(p, cam.position)) > 0.0f;
}

} // namespace

scene::ObjectId Session::pickAt(Vector2 point, int screenWidth, int screenHeight, float radius,
                                const view::DrawOptions& options) const {
    const Camera3D& cam = m_camera.camera();
    float best = radius;
    scene::ObjectId hit = scene::kNoObject;
    auto consider = [&](Vector3 a, Vector3 b, scene::ObjectId id) {
        if (!inFront(cam, a) || !inFront(cam, b)) return;
        float d = distanceToSegment(point, gfx::worldToScreen(a, cam, screenWidth, screenHeight),
                                    gfx::worldToScreen(b, cam, screenWidth, screenHeight));
        if (d < best) {
            best = d;
            hit = id;
        }
    };
    // Each type says which segments a click should be near (view/objects/*.cpp)
    std::vector<view::Segment> segments;
    for (const auto& obj : m_scene.space(spaceKind()).objects()) {
        const SceneObject& o = *obj;
        if (!view::SceneView::shown(options, o) || !o.value().ok()) continue;
        const view::ObjectView* ov = view::objectView(o.typeId());
        if (!ov || !ov->pick) continue;
        segments.clear();
        ov->pick(o, options, segments);
        for (const auto& [a, b] : segments) consider(a, b, o.id());
    }
    return hit;
}

void Session::saveTo(SceneData& data) const {
    m_scene.save(data);
    putInt(data, "mode", static_cast<int>(m_mode));
    Vector3 p = m_camera.position(), t = m_camera.target();
    putFloat(data, "camera.px", p.x);
    putFloat(data, "camera.py", p.y);
    putFloat(data, "camera.pz", p.z);
    putFloat(data, "camera.tx", t.x);
    putFloat(data, "camera.ty", t.y);
    putFloat(data, "camera.tz", t.z);
    saveViewSettings(m_view, data);
}

bool Session::loadFrom(const SceneData& data, std::string& error, std::string& note) {
    if (!m_scene.load(data, error)) return false;
    int m = static_cast<int>(m_mode);
    if (getInt(data, "mode", m) && m >= 0 && m < kAppModeCount) adoptMode(static_cast<AppMode>(m));
    Vector3 p = m_camera.position(), t = m_camera.target();
    getFloat(data, "camera.px", p.x);
    getFloat(data, "camera.py", p.y);
    getFloat(data, "camera.pz", p.z);
    getFloat(data, "camera.tx", t.x);
    getFloat(data, "camera.ty", t.y);
    getFloat(data, "camera.tz", t.z);
    m_camera.setSpace(m_mode);
    // Files saved in fly mode (and before the orbit camera) keep a target one unit ahead
    if (Vector3Distance(p, t) < 1.05f) t = CameraController::pivotAlong(p, t, m_mode);
    m_camera.setView(p, t);
    note = convertOldKeyframes(data);
    loadViewSettings(m_view, data);
    return true;
}

std::string Session::convertOldKeyframes(const SceneData& data) {
    // Files from before the Time panel stored matrix keyframes for the old Animation
    // panel; they become K = keys(s, ...) driven by an animated parameter s
    int count = 0;
    if (!getInt(data, "anim.keyframes", count) || count < 2) return "";
    std::vector<std::string> mats;
    for (int k = 0; k < count; k++) {
        auto it = data.find("anim.key" + std::to_string(k));
        if (it == data.end()) continue;
        std::istringstream in(it->second);
        float m[4][4];
        bool ok = true;
        for (int i = 0; i < 4 && ok; i++)
            for (int j = 0; j < 4 && ok; j++) ok = static_cast<bool>(in >> m[i][j]);
        if (!ok) continue;
        std::ostringstream text;
        text << "[";
        for (int i = 0; i < 4; i++) {
            text << (i ? ", [" : "[");
            for (int j = 0; j < 4; j++) text << (j ? ", " : "") << m[i][j];
            text << "]";
        }
        text << "]";
        mats.push_back(text.str());
    }
    if (mats.size() < 2) return "";
    SceneObject* s = m_scene.add(SpaceKind::Euclidean3D, "parameter", "s");
    auto* p = static_cast<scene::ParameterObject*>(s);
    p->setMode(scene::ParameterObject::Mode::Animate);
    p->setProp("min", 0.0);
    p->setProp("max", 1.0);
    Playback pb = p->playback();
    pb.repeat = Repeat::Loop;
    pb.duration = 4.0f;
    p->setPlayback(pb);
    std::string formula = "keys(" + s->name();
    for (const auto& m : mats) formula += ", " + m;
    formula += ")";
    SceneObject* k = m_scene.add(SpaceKind::Euclidean3D, "matrix", "K");
    static_cast<scene::MatrixObject*>(k)->setSize(4);
    static_cast<scene::MatrixObject*>(k)->setByFormula(true);
    k->setField("value", formula);
    m_scene.evaluate();
    m_scene.commit();
    return "Old keyframes became the matrix " + k->name() + " = keys(" + s->name() + ", ...)";
}

// --- Read-only state -----------------------------------------------------------

const SceneObject* Session::targetMatrix() const {
    const scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    const auto& sel = const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Euclidean3D);
    for (auto it = sel.ids.rbegin(); it != sel.ids.rend(); ++it) {
        const SceneObject* o = sp.find(*it);
        if (o && o->typeId() == "matrix" && o->value().type == expr::Type::Matrix) return o;
    }
    for (const auto& o : sp.objects())
        if (o->typeId() == "matrix" && o->visible && o->value().type == expr::Type::Matrix) return o.get();
    return nullptr;
}

Matrix4D Session::matrix() const {
    const SceneObject* m = targetMatrix();
    return m ? m->value().matrix4() : Matrix4D::identity();
}

bool Session::using4x4() const {
    const SceneObject* m = targetMatrix();
    return m && m->value().size == 4;
}

Vector3D Session::vectorValue(const std::string& name) const {
    const SceneObject* o = m_scene.space(SpaceKind::Euclidean3D).find(name);
    if (o && o->value().type == expr::Type::Vector) return o->value().vector;
    return Vector3D::zero();
}

Complex Session::complexValue(const std::string& name) const {
    const SceneObject* o = m_scene.space(SpaceKind::Complex).find(name);
    if (o && o->value().isNumber()) return Complex(static_cast<float>(o->value().re), static_cast<float>(o->value().im));
    return Complex(0, 0);
}

double Session::parameterValue(const std::string& name, AppMode mode) const {
    const SceneObject* o = m_scene.space(kindOf(mode)).find(name);
    if (o && o->value().isRealNumber()) return o->value().re;
    return 0.0;
}

EigenResult Session::eigen() const {
    const SceneObject* m = targetMatrix();
    return m ? eigen3(m->value().matrix3()) : EigenResult{};
}

StepsInput Session::stepsInput() const {
    StepsInput in;
    in.complexPlane = m_mode == AppMode::Complex;
    in.matrix = matrix();
    in.is4x4 = using4x4();
    // Vectors: the selected ones, else a and b
    std::vector<Vector3D> vectors;
    const scene::Space& sp = m_scene.space(SpaceKind::Euclidean3D);
    for (auto id : const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Euclidean3D).ids) {
        const SceneObject* o = sp.find(id);
        if (o && o->value().type == expr::Type::Vector) vectors.push_back(o->value().vector);
    }
    in.a = vectors.size() > 0 ? vectors[0] : vectorValue("a");
    in.b = vectors.size() > 1 ? vectors[1] : vectorValue("b");
    // Complex numbers: the selected ones, else z1 and z2
    std::vector<Complex> numbers;
    const scene::Space& cp = m_scene.space(SpaceKind::Complex);
    for (auto id : const_cast<scene::Scene&>(m_scene).selection(SpaceKind::Complex).ids) {
        const SceneObject* o = cp.find(id);
        if (o && o->value().isNumber() && o->typeId() != "parameter")
            numbers.push_back(Complex(static_cast<float>(o->value().re), static_cast<float>(o->value().im)));
    }
    in.z1 = numbers.size() > 0 ? numbers[0] : complexValue("z1");
    in.z2 = numbers.size() > 1 ? numbers[1] : complexValue("z2");
    in.theta = static_cast<float>(parameterValue("theta", AppMode::Complex));
    return in;
}

void Session::applyBreakdown(view::DrawOptions& o, const BreakdownPlayer& player) const {
    const scene::SceneObject* target = targetMatrix();
    if (!target || m_mode != AppMode::Space3D) return;
    o.overrideMatrix = target->id();
    o.overrideValue = player.current();
    int steps = player.stepCount();
    int done = std::min(player.completedSteps(), steps);
    for (int k = 1; k <= done && k < steps; k++) o.ghosts.push_back(player.breakdown().after(k));
}

} // namespace linalg

