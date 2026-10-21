#include "edu/Tutorial.hpp"
#include "animation/Playback.hpp"
#include "app/AppApi.hpp"
#include "scene/Types.hpp"

namespace linalg {

// One way to do each step's task through the AppApi, as a user would get there by hand. The lesson walks
// (tools/dev/lesson-walk.sh for the raylib app, --lesson-walk in the GTK app) run it after a step's setup and
// then check that the task counts as done. Steps are found by title: add a line for a new step with a task.
bool solveTutorialStep(AppApi& app, int lesson, int step) {
    using PM = scene::ParameterObject::Mode;
    auto param = [&](const char* n, AppMode m = AppMode::Space3D) {
        return dynamic_cast<scene::ParameterObject*>(app.object(n, m));
    };
    const std::string title = lessons()[lesson].steps[step].title;
    if (title == "Two spaces") { app.showPanel("gallery", true); return true; }
    if (title == "Add a vector") { app.addObject(AppMode::Space3D, "vector", "v"); return true; }
    if (title == "Edit it") { if (auto* o = app.object("v")) { o->setField("x", "0"); o->setField("y", "0"); o->setField("z", "2"); } return true; }
    if (title == "Formulas link objects") { app.addObject(AppMode::Space3D, "vector", "w", {{"value", "2a - b"}}, {{"mode", "formula"}}); return true; }
    if (title == "The dot product") { app.setVectors(Vector3D(1, 0, 0), Vector3D(1, 1, 0)); return true; }
    if (title == "Right-hand rule") { app.setVectors(Vector3D(1, 0, 0), Vector3D(0, -1, 0)); return true; }
    if (title == "Send x to y") { app.setMatrix3(Matrix3D({{{{0, 0, 0}}, {{1, 1, 0}}, {{0, 0, 1}}}})); return true; }
    if (title == "A rotation") { app.setMatrix3(Matrix3D::identity()); app.applyMatrixPreset(Matrix3D::rotation(3.14159265f / 4)); return true; }
    if (title == "Presets add up") { app.applyMatrixPreset(Matrix3D::rotation(3.14159265f / 4)); return true; }
    if (title == "Squash it flat") { app.setMatrix3(Matrix3D({{{{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 0}}}})); return true; }
    if (title == "Flip it") { app.applyMatrixPreset(Matrix3D::scale(-1.0f, 1.0f)); return true; }
    if (title.find("5") != std::string::npos && title.find("Find") == 0) { app.setMatrix3(Matrix3D({{{{5, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}}})); return true; }
    if (title == "A slider in a formula") { if (auto* p = param("theta")) p->setNumber(2.0); return true; }
    if (title == "Animate with the clock") { if (auto* p = param("theta")) { p->setMode(PM::Animate); Playback pb = p->playback(); pb.repeat = Repeat::PingPong; pb.easing = Easing::Bounce; p->setPlayback(pb); app.restartTime(true); } return true; }
    if (title == "A formula of time") { if (auto* p = param("theta")) { p->setMode(PM::Formula); p->setField("formula", "pi/4*sin(2t)"); } return true; }
    if (title == "Morph with a slider") { if (auto* p = param("s")) p->setNumber(1.0); return true; }
    if (title == "Watch the steps") { app.showPanel("breakdown", true); return true; }
    if (title == "Multiply by i") { if (auto* o = app.object("z2", AppMode::Complex)) o->setField("value", "i"); return true; }
    if (title == "Double it") { if (auto* o = app.object("z2", AppMode::Complex)) o->setField("value", "2i"); return true; }
    if (title == "Half a turn") { if (auto* p = param("theta", AppMode::Complex)) p->setNumber(3.14159265); return true; }
    if (title == "Turn further") { if (auto* o = app.object("q")) o->setField("value", "qaxis([0, 0, 1], pi)"); return true; }
    if (title == "Acting on a vector") { if (auto* o = app.object("q")) o->setProp("act", true); return true; }
    if (title == "One after the other") { app.addObject(AppMode::Space3D, "quaternion", "p", {{"value", "qaxis([1, 0, 0], pi/2)"}}); app.addObject(AppMode::Space3D, "quaternion", "r", {{"value", "q*p"}}); return true; }
    if (title == "Order matters") { app.select("q"); app.select("p", AppMode::Space3D, true); return true; }
    if (title == "Blend with slerp") { app.addObject(AppMode::Space3D, "parameter", "s"); app.addObject(AppMode::Space3D, "quaternion", "b", {{"value", "slerp(q, p, s)"}}); return true; }
    if (title == "More boxes") { if (auto* o = app.object("V")) o->setField("n", "12"); return true; }
    if (title == "Where to sample") { if (auto* o = app.object("V")) o->setProp("rule", "left"); return true; }
    if (title == "Work along a path") { app.addObject(AppMode::Space3D, "lineintegral", "W"); return true; }
    if (title == "Against the flow") { if (auto* o = app.object("W")) o->setField("F", "[y, -x, 0]"); return true; }
    if (title == "Slide along") { if (auto* o = app.object("P")) o->setField("u0", "mod(t, 4pi)"); return true; }
    if (title == "Flatten it") { if (auto* o = app.object("P")) o->setField("z", "0"); return true; }
    if (title == "Tighter turns") { if (auto* o = app.object("P")) { o->setField("x", "0.5cos(u)"); o->setField("y", "0.5sin(u)"); } return true; }
    if (title == "Comb the curvature") { if (auto* o = app.object("P")) o->setProp("comb", true); return true; }
    if (title == "Add waves") { if (auto* o = app.object("S")) o->setField("N", "9"); return true; }
    if (title == "Draw with circles") { app.setMode(AppMode::Complex); app.addObject(AppMode::Complex, "epicycles", "E"); return true; }
    if (title == "Sharper drawing") { if (auto* o = app.object("E", AppMode::Complex)) o->setField("N", "30"); return true; }
    if (title == "Find a flat spot") { if (auto* o = app.object("T")) o->setField("x0", "sqrt(2)"); return true; }
    if (title == "Secant to tangent") { if (auto* o = app.object("T")) o->setField("h", "0.05"); return true; }
    if (title == "The graph of f'") { if (auto* o = app.object("T")) o->setProp("derivative", true); return true; }
    if (title == "Slopes of a surface") { if (auto* o = app.object("G")) { o->setField("x0", "0"); o->setField("y0", "0"); } return true; }
    if (title == "Velocity and acceleration") { app.addObject(AppMode::Space3D, "velocity", "R"); return true; }
    if (title == "Speeding up or turning") { if (auto* o = app.object("R")) o->setProp("split", true); return true; }
    if (title == "Add terms") { if (auto* o = app.object("p")) o->setField("N", "5"); return true; }
    if (title == "Move the center") { if (auto* o = app.object("p")) o->setField("a", "2"); return true; }
    if (title == "Where it stops working") { if (auto* o = app.object("p")) o->setField("N", "16"); return true; }
    if (title == "A Newton step") { if (auto* o = app.object("r")) o->setField("steps", "1"); return true; }
    if (title == "More steps") { if (auto* o = app.object("r")) o->setField("steps", "6"); return true; }
    if (title == "A bad start") { if (auto* o = app.object("r")) o->setField("x0", "0"); return true; }
    if (title == "Add a surface") { app.addObject(AppMode::Space3D, "surface", "S"); return true; }
    if (title == "Use a parameter") { app.addObject(AppMode::Space3D, "parameter", "k"); if (auto* o = app.object("S")) o->setField("f", "sin(k*x)"); return true; }
    return false;
}

} // namespace linalg
