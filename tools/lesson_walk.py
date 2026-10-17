# Inserts a temporary lesson walk into src/app/Tour.cpp (restore the file afterwards)
import sys
p = sys.argv[1]
s = open(p).read()
walk = r'''    auto solve = [](App& app, size_t L, size_t K) -> bool {
        using PM = scene::ParameterObject::Mode;
        auto param = [&](const char* n, AppMode m = AppMode::Space3D) {
            return dynamic_cast<scene::ParameterObject*>(app.object(n, m));
        };
        const std::string title = lessons()[L].steps[K].title;
        if (title == "Two spaces") { if (auto* p = app.panel("gallery")) p->setVisible(true); return true; }
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
        if (title == "Watch the steps") { if (auto* p = app.panel("breakdown")) p->setVisible(true); return true; }
        if (title == "Multiply by i") { if (auto* o = app.object("z2", AppMode::Complex)) o->setField("value", "i"); return true; }
        if (title == "Double it") { if (auto* o = app.object("z2", AppMode::Complex)) o->setField("value", "2i"); return true; }
        if (title == "Half a turn") { if (auto* p = param("theta", AppMode::Complex)) p->setNumber(3.14159265); return true; }
        if (title == "Add a surface") { app.addObject(AppMode::Space3D, "surface", "S"); return true; }
        if (title == "Use a parameter") { app.addObject(AppMode::Space3D, "parameter", "k"); if (auto* o = app.object("S")) o->setField("f", "sin(k*x)"); return true; }
        return false;
    };
    for (size_t L = 0; L < lessons().size(); L++) {
        for (size_t K = 0; K < lessons()[L].steps.size(); K++) {
            char nm[64];
            snprintf(nm, sizeof(nm), "L%02zu_%02zu", L, K);
            step(nm, [L, K](App& app) {
                if (auto* p = app.panel("tutorial")) p->setVisible(true);
                app.tutorialPanel().startLesson(static_cast<int>(L));
                app.tutorialPanel().goToStep(static_cast<int>(K));
            }, 8);
            step((std::string(nm) + "_sol").c_str(), [L, K, solve](App& app) {
                const auto& st = lessons()[L].steps[K];
                bool early = st.check && st.check(app, TutorialContext{});
                bool scripted = solve(app, L, K);
                fprintf(stderr, "WALK %zu.%zu task=%d early=%d scripted=%d %s\n", L, K, st.check ? 1 : 0, early ? 1 : 0, scripted ? 1 : 0, st.title);
            }, (std::string(lessons()[L].steps[K].title) == "Watch the steps") ? 500 : 6);
            step((std::string(nm) + "_chk").c_str(), [L, K](App& app) {
                const auto& st = lessons()[L].steps[K];
                fprintf(stderr, "SOLVED %zu.%zu %d\n", L, K, st.check && st.check(app, TutorialContext{}) ? 1 : 0);
            }, 1);
        }
    }
'''
anchor = '    step("00_'
i = s.find(anchor)
if i < 0:
    anchor = '    step("01_space_default"'
    i = s.find(anchor)
s = s[:i] + walk + s[i:]
if '#include "edu/Tutorial.hpp"' not in s:
    s = s.replace('#include "edu/Examples.hpp"', '#include "edu/Examples.hpp"\n#include "edu/Tutorial.hpp"', 1)
open(p, "w").write(s)
