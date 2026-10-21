#include "app/Session.hpp"
#include "math/GridMath.hpp"
#include "scene/Templates.hpp"
#include "text/Numbers.hpp"
#include "ui/core/Theme.hpp"
#include "view/AxisNumbers.hpp"
#include "view/ViewStyle.hpp"
#include <algorithm>
#include <cmath>

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

} // namespace linalg
