#include "edu/Tutorial.hpp"
#include "app/App.hpp"
#include <cmath>
#include <set>

namespace linalg {

namespace {

constexpr float kPi = 3.14159265f;

bool near(float a, float b, float tol = 0.02f) { return std::fabs(a - b) <= tol; }

// The default picture in the 3D space, with M selected and the analysis open
void fresh3D(App& app) {
    app.setMode(AppMode::Space3D);
    app.resetScene();
    app.select("M");
    if (auto* p = app.panel("analysis")) p->setVisible(true);
}

Vector3D column(const Matrix4D& m, int c) { return {m.m[0][c], m.m[1][c], m.m[2][c]}; }

float det(const App& app) {
    Matrix4D m = app.matrix();
    if (app.using4x4()) return m.determinant();
    Matrix3D l;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) l.m[i][j] = m.m[i][j];
    return l.determinant();
}

// Names of the vectors the default scene starts with
const std::set<std::string> kDefaultVectors = {"a", "b", "s", "c", "Ma", "Mb"};

bool anyVector(App& app, bool (*test)(const scene::SceneObject&)) {
    for (const auto& o : app.space(AppMode::Space3D).objects())
        if (o->typeId() == "vector" && o->value().type == expr::Type::Vector && test(*o)) return true;
    return false;
}

std::vector<Lesson> build() {
    std::vector<Lesson> list;

    list.push_back({"Getting around", "Camera, panels and spaces", {
        {"Welcome",
         "axismundi turns linear algebra, complex numbers and functions into 3D pictures. The side bar lists "
         "the objects in the scene and edits the selected one; tools open as panels you can drag around.",
         nullptr, nullptr, nullptr, nullptr, nullptr},
        {"Move the camera",
         "There are two input modes. In camera mode the mouse looks around and W A S D move you; in UI mode the "
         "cursor is free to click fields and buttons. Tab switches between them; Esc always gives the mouse back.",
         nullptr, "Press Tab, look around, then press Esc (or Tab) to get the cursor back.",
         [](App& app, const TutorialContext& ctx) { return ctx.cameraModeUsed && app.uiMode(); },
         "Tab toggles the mode. The dot in the bottom-left status turns green in camera mode.", nullptr},
        {"Two spaces",
         "The tabs at the top switch between the 3D space (vectors, matrices, plots) and the complex plane. "
         "The gallery has a ready-made example for each idea.",
         nullptr, "Open the example gallery.",
         [](App& app, const TutorialContext&) {
             auto* p = app.panel("gallery");
             return p && p->visible();
         },
         "Click the grid icon in the top bar, or press E.", "topbar.gallery"},
    }});

    list.push_back({"Objects", "Add, select and edit", {
        {"The objects list",
         "Everything you see is an object: the matrix M, the vectors a and b, and vectors computed from them "
         "such as s = a + b. Click a row to select it; the inspector below shows its settings.",
         fresh3D, nullptr, nullptr, nullptr, "objects.list"},
        {"Add a vector",
         "The Add button lists every kind of object, plus templates built from the selection.",
         nullptr, "Add a new vector.",
         [](App& app, const TutorialContext&) {
             return anyVector(app, [](const scene::SceneObject& o) { return kDefaultVectors.count(o.name()) == 0; });
         },
         "Click Add (or press N) and choose Vector.", "objects.add"},
        {"Edit it",
         "The new vector is selected, so the inspector shows its components. Every field accepts numbers or "
         "formulas such as sqrt(2).",
         nullptr, "Make a vector that points straight up: (0, 0, 2).",
         [](App& app, const TutorialContext&) {
             return anyVector(app, [](const scene::SceneObject& o) {
                 Vector3D v = o.value().vector;
                 return near(v.x, 0) && near(v.y, 0) && near(v.z, 2);
             });
         },
         "Set x = 0, y = 0, z = 2. In the 3D space z points up.", "inspector"},
    }});

    list.push_back({"Vectors and formulas", "Objects that follow other objects", {
        {"Formulas link objects",
         "s is defined by the formula a + b: change a or b and s follows. Any field can use other objects by "
         "name.",
         [](App& app) {
             fresh3D(app);
             app.select("s");
         },
         "Make a vector defined by the formula 2a - b.",
         [](App& app, const TutorialContext&) {
             Vector3D target = app.vectorA() * 2.0f - app.vectorB();
             for (const auto& o : app.space(AppMode::Space3D).objects()) {
                 auto* v = dynamic_cast<const scene::VectorObject*>(o.get());
                 if (v && v->byFormula() && o->value().type == expr::Type::Vector &&
                     (o->value().vector - target).magnitude() < 1e-3f && o->name() != "a" && o->name() != "b")
                     return true;
             }
             return false;
         },
         "Add a vector, switch it to Formula and type 2a - b.", "objects.add"},
        {"The dot product",
         "a · b multiplies matching components and adds them up; it is zero exactly when the vectors are "
         "perpendicular. Select a and b together (Ctrl+click) to see it in the Analysis panel.",
         nullptr, "Set b to (1, 1, 0).",
         [](App& app, const TutorialContext&) {
             Vector3D b = app.vectorB();
             return near(b.x, 1) && near(b.y, 1) && near(b.z, 0);
         },
         "Select b, then type 1 in its x field. a · b becomes 1 and the angle 45°.", "objects.list"},
        {"Right-hand rule",
         "c = cross(a, b) follows the right-hand rule: curl your fingers from a to b and your thumb points "
         "along a × b.",
         nullptr, "Make c point straight down the z-axis.",
         [](App& app, const TutorialContext&) {
             Vector3D c = app.vectorA().cross(app.vectorB());
             return c.magnitude() > 0.1f && c.normalized().z < -0.99f;
         },
         "With a = (1, 0, 0), try b = (0, -1, 0).", "objects.list"},
    }});

    list.push_back({"Matrices move space", "Columns are where the axes go", {
        {"Columns are images",
         "A matrix moves every point of space. Its first column is where the x-axis arrow (1, 0, 0) lands, the "
         "second where the y-axis lands, the third where z lands. The bright arrows show the moved axes.",
         fresh3D, nullptr, nullptr, nullptr, "matrix.grid"},
        {"Send x to y",
         "Change the first column and watch the orange arrow (the image of the x-axis) follow.",
         nullptr, "Make the first column (0, 1, 0).",
         [](App& app, const TutorialContext&) {
             Vector3D c = column(app.matrix(), 0);
             return near(c.x, 0) && near(c.y, 1) && near(c.z, 0);
         },
         "Top-left cell 0, the cell below it 1, the bottom-left cell 0.", "matrix.grid"},
        {"A rotation",
         "Rotations keep lengths and angles. Their columns are unit vectors at right angles to each other.",
         nullptr, "Apply the Rotate 45° preset.",
         [](App& app, const TutorialContext&) {
             Matrix4D m = app.matrix();
             return near(m.m[0][0], 0.7071f, 0.01f) && near(m.m[1][0], 0.7071f, 0.01f) &&
                    near(m.m[0][1], -0.7071f, 0.01f);
         },
         "Click Rotate 45° in the inspector or press R.", "preset.rotate"},
    }});

    list.push_back({"The determinant", "How volumes change", {
        {"Squash it flat",
         "det M is the factor by which M scales volumes. When it is 0, space is squashed into a plane, a line or "
         "a point, and the matrix cannot be undone.",
         fresh3D, "Make det M = 0.",
         [](App& app, const TutorialContext&) { return std::fabs(det(app)) < 1e-3f; },
         "Set every cell of the third column to 0, or make two columns equal.", "matrix.grid"},
        {"Flip it",
         "A negative determinant means M also mirrors space: a right hand becomes a left hand.",
         fresh3D, "Make det M negative.",
         [](App& app, const TutorialContext&) { return det(app) < -1e-3f; },
         "The Reflect preset (F) does it, or put −1 in one diagonal cell.", "preset.reflect"},
    }});

    list.push_back({"Eigenvectors", "Directions a matrix only stretches", {
        {"Purple lines",
         "Along an eigenvector v the matrix only stretches: M v = λ v. The object E draws these directions as "
         "dashed lines with dots at λv. This symmetric matrix has three, at right angles.",
         [](App& app) {
             fresh3D(app);
             app.setMatrix3(Matrix3D({{{{2, 1, 0}}, {{1, 2, 0}}, {{0, 0, 3}}}}));
             app.select("M");
         },
         nullptr, nullptr, nullptr, "panel.analysis"},
        {"Find λ = 5",
         "A diagonal matrix stretches each axis by its diagonal entry, so the axes are eigenvectors.",
         nullptr, "Make a matrix with an eigenvalue of 5.",
         [](App& app, const TutorialContext&) {
             for (const auto& ev : app.eigen().values)
                 if (ev.isReal() && near(ev.re, 5.0f, 0.01f)) return true;
             return false;
         },
         "Put 5 in a diagonal cell of a diagonal matrix.", "matrix.grid"},
    }});

    list.push_back({"Parameters", "Sliders that drive formulas", {
        {"A slider in a formula",
         "theta is a parameter: a named number with a slider. Here M is defined by the formula rotz(theta), so "
         "M, and everything that uses it, turns when theta changes.",
         [](App& app) {
             fresh3D(app);
             app.addObject(AppMode::Space3D, "parameter", "theta", {}, {{"value", "0.5"}, {"min", "0"}, {"max", "6.2832"}});
             if (auto* m = dynamic_cast<scene::MatrixObject*>(app.object("M"))) {
                 m->setByFormula(true);
                 m->setField("value", "rotz(theta)");
                 app.scene().commit();
             }
             app.select("theta");
         },
         "Press Play on theta and watch the scene turn.",
         [](App& app, const TutorialContext&) {
             auto* p = dynamic_cast<scene::ParameterObject*>(app.object("theta"));
             return p && std::fabs(p->number() - 0.5) > 0.2;
         },
         "The play button is in theta's inspector. Dragging the slider works too.", "parameter.play"},
        {"Repeat and easing",
         "Playing switched theta to Animate: it runs from min to max. Ping-pong runs back and forth, and the easing "
         "curve shapes the motion (Ease in-out starts and stops gently, Bounce bounces at the end).",
         nullptr, "Choose Ping-pong and a different easing for theta.",
         [](App& app, const TutorialContext&) {
             auto* p = dynamic_cast<scene::ParameterObject*>(app.object("theta"));
             if (!p) return false;
             Playback pb = p->playback();
             return pb.repeat == Repeat::PingPong && pb.easing != Easing::Linear;
         },
         "Both are in theta's inspector, under the range, when the mode is Animate.", "parameter.easing"},
        {"A formula of time",
         "A parameter can also be a formula of t, its own clock in seconds. pi/4*sin(2t) swings back and forth "
         "between ±45°; t/2 grows forever.",
         nullptr, "Switch theta to Formula and type a formula that uses t, e.g. pi/4*sin(2t).",
         [](App& app, const TutorialContext&) {
             auto* p = dynamic_cast<scene::ParameterObject*>(app.object("theta"));
             if (!p || p->mode() != scene::ParameterObject::Mode::Formula || !p->error().empty()) return false;
             const scene::Field* f = p->field("formula");
             return f && f->program.valid();
         },
         "The mode switch is at the top of theta's inspector. Press Play if the clock is paused.", "parameter.mode"},
    }});

    list.push_back({"Breaking a matrix down", "Rotate, stretch, rotate", {
        {"Watch the steps",
         "Every matrix can be done in three simple moves: a rotation, a stretch along the axes, and another "
         "rotation (the singular value decomposition).",
         [](App& app) {
             fresh3D(app);
             app.setMatrix3(Matrix3D({{{{1, 0.8f, 0}}, {{0, 1, 0}}, {{0, 0, 1.4f}}}}));
             app.select("M");
         },
         "Open the transformation breakdown and let it play to the end.",
         [](App& app, const TutorialContext&) {
             auto* p = app.panel("breakdown");
             if (!p || !p->visible()) return false;
             auto& b = app.breakdownPanel();
             return b.progress() >= static_cast<float>(b.breakdown().steps.size()) - 1e-3f;
         },
         "Click the layers icon in the top bar or press B.", "topbar.breakdown"},
    }});

    list.push_back({"Complex multiplication", "Rotate and scale", {
        {"Multiply by i",
         "Complex numbers are points of the plane. w = z1·z2 turns z1 by the angle of z2 and stretches it by the "
         "length of z2.",
         [](App& app) {
             app.setMode(AppMode::Complex);
             app.resetScene();
             if (auto* z = app.object("z2", AppMode::Complex)) z->setField("value", "1");
             app.scene().commit();
             app.select("z2", AppMode::Complex);
             app.resetCamera();
         },
         "Set z2 = i.",
         [](App& app, const TutorialContext&) {
             Complex z = app.complexValue("z2");
             return near(z.re, 0) && near(z.im, 1);
         },
         "Type i in z2's value field. w jumps a quarter turn counter-clockwise.", "inspector"},
        {"Double it",
         "The length of the product is the product of the lengths: |z1·z2| = |z1| |z2|.",
         nullptr, "Give z2 length 2 (for example 2i or 2).",
         [](App& app, const TutorialContext&) { return near(app.complexValue("z2").abs(), 2.0f); },
         "Try z2 = 2i: the product turns 90° and doubles.", "inspector"},
    }});

    list.push_back({"Euler's formula", "e^(iθ) walks around the circle", {
        {"Half a turn",
         "u = exp(i*theta) is the point at angle theta on the unit circle. Its shadows on the axes are cos θ "
         "and sin θ.",
         [](App& app) {
             app.setMode(AppMode::Complex);
             app.resetScene();
             app.select("theta", AppMode::Complex);
             app.resetCamera();
         },
         "Drag theta to π (about 3.14).",
         [](App& app, const TutorialContext&) {
             return near(static_cast<float>(app.parameterValue("theta", AppMode::Complex)), kPi, 0.03f);
         },
         "At θ = π the point reaches −1: e^(iπ) = −1.", "parameter.slider"},
    }});

    list.push_back({"Plotting", "Functions, curves and surfaces", {
        {"Add a surface",
         "Plots are objects too. A surface z = f(x, y) is a height above every point of the floor.",
         fresh3D, "Add a surface.",
         [](App& app, const TutorialContext&) {
             for (const auto& o : app.space(AppMode::Space3D).objects())
                 if (o->typeId() == "surface" && o->error().empty()) return true;
             return false;
         },
         "Add → Surface z = f(x, y).", "objects.add"},
        {"Use a parameter",
         "Formulas in plots can use parameters, so a slider can reshape a surface.",
         nullptr, "Make a surface whose formula uses a parameter (add one, e.g. k, then use it).",
         [](App& app, const TutorialContext&) {
             for (const auto& o : app.space(AppMode::Space3D).objects())
                 if (o->typeId() == "surface" && o->error().empty() && !o->dependencies().empty()) return true;
             return false;
         },
         "Add → Parameter, then set the surface to sin(k*x) or k*exp(-(x^2+y^2)/4).", "objects.add"},
    }});
    return list;
}

} // namespace

const std::vector<Lesson>& lessons() {
    static const std::vector<Lesson> list = build();
    return list;
}

} // namespace linalg
