#include "edu/Tutorial.hpp"
#include "app/App.hpp"
#include <cmath>

namespace linalg {

namespace {

constexpr float kPi = 3.14159265f;

bool near(float a, float b, float tol = 0.02f) { return std::fabs(a - b) <= tol; }

void freshMatrices(App& app) {
    app.setMode(AppMode::LinearAlgebra);
    app.stopAnimation();
    app.setMatrix3(Matrix3D::identity());
    app.setVectors(Vector3D(1, 0, 0), Vector3D(0, 1, 0));
    if (auto* p = app.panel("analysis")) p->setVisible(true);
}

Vector3D column(const Matrix4D& m, int c) { return {m.m[0][c], m.m[1][c], m.m[2][c]}; }

float det3(const App& app) {
    const Matrix4D& m = app.matrix();
    Matrix3D l;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) l.m[i][j] = m.m[i][j];
    return app.using4x4() ? m.determinant() : l.determinant();
}

std::vector<Lesson> build() {
    std::vector<Lesson> list;

    list.push_back({"Getting around", "Camera, panels and modes", {
        {"Welcome",
         "axismundi turns linear algebra, complex numbers and functions into 3D pictures. Inputs live in the side "
         "bar on the left; results and tools open as panels you can drag around.",
         nullptr, nullptr, nullptr, nullptr, nullptr},
        {"Move the camera",
         "There are two input modes. In camera mode the mouse looks around and W A S D move you; in UI mode the "
         "cursor is free to click fields and buttons. Tab switches between them.",
         nullptr, "Press Tab, look around, then press Tab again to come back.",
         [](App& app, const TutorialContext& ctx) { return ctx.cameraModeUsed && app.uiMode(); },
         "Tab toggles the mode. The dot in the bottom-left status turns green in camera mode.", nullptr},
        {"Three modes",
         "The tabs at the top switch between Matrices, Complex and Plots. The gallery has a ready-made example "
         "for each idea.",
         nullptr, "Open the example gallery.",
         [](App& app, const TutorialContext&) {
             auto* p = app.panel("gallery");
             return p && p->visible();
         },
         "Click the grid icon in the top bar, or press E.", "topbar.gallery"},
    }});

    list.push_back({"Vectors", "Sum, dot product and cross product", {
        {"Arrows from the origin",
         "A and B are drawn as arrows. A+B is the diagonal of the parallelogram they span, and A×B sticks out "
         "perpendicular to both. Each label shows the vector's length.",
         freshMatrices, nullptr, nullptr, nullptr, "vectors"},
        {"The dot product",
         "A · B multiplies matching components and adds them up. It is zero exactly when the vectors are "
         "perpendicular. Watch the Analysis panel while you edit.",
         freshMatrices, "Set B to (1, 1, 0).",
         [](App& app, const TutorialContext&) {
             Vector3D b = app.vectorB();
             return near(b.x, 1) && near(b.y, 1) && near(b.z, 0);
         },
         "Click the x field of B, type 1 and press Enter. A · B becomes 1 and the angle 45°.", "vector.b"},
        {"Right-hand rule",
         "A × B follows the right-hand rule: curl your fingers from A to B and your thumb points along A × B.",
         nullptr, "Make A × B point straight down the z-axis (negative z).",
         [](App& app, const TutorialContext&) {
             Vector3D c = app.vectorA().cross(app.vectorB());
             return c.magnitude() > 0.1f && c.normalized().z < -0.99f;
         },
         "With A = (1, 0, 0), try B = (0, -1, 0).", "vector.b"},
    }});

    list.push_back({"Matrices move space", "Columns are where the axes go", {
        {"Columns are images",
         "A matrix moves every point of space. Its first column is where the x-axis arrow (1, 0, 0) lands, the "
         "second where the y-axis lands, the third where z lands. The bright arrows show the moved axes.",
         freshMatrices, nullptr, nullptr, nullptr, "matrix.grid"},
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
             const Matrix4D& m = app.matrix();
             return near(m.m[0][0], 0.7071f, 0.01f) && near(m.m[1][0], 0.7071f, 0.01f) && near(m.m[0][1], -0.7071f, 0.01f);
         },
         "Click Rotate 45° or press R. The first column is (cos 45°, sin 45°, 0).", "preset.rotate"},
    }});

    list.push_back({"The determinant", "How volumes change", {
        {"Squash it flat",
         "det M is the factor by which M scales volumes. When it is 0, space is squashed into a plane, a line or "
         "a point, and the matrix cannot be undone.",
         freshMatrices, "Make det M = 0.",
         [](App& app, const TutorialContext&) { return std::fabs(det3(app)) < 1e-3f; },
         "Set every cell of the third column to 0, or make two columns equal.", "matrix.grid"},
        {"Flip it",
         "A negative determinant means M also mirrors space: a right hand becomes a left hand.",
         freshMatrices, "Make det M negative.",
         [](App& app, const TutorialContext&) { return det3(app) < -1e-3f; },
         "The Reflect preset (F) does it, or put −1 in one diagonal cell.", "preset.reflect"},
    }});

    list.push_back({"Eigenvectors", "Directions a matrix only stretches", {
        {"Purple lines",
         "Along an eigenvector v the matrix only stretches: M v = λ v. The purple dashed lines show these "
         "directions and the dots sit at λv. This symmetric matrix has three, at right angles.",
         [](App& app) {
             freshMatrices(app);
             app.setMatrix3(Matrix3D({{{{2, 1, 0}}, {{1, 2, 0}}, {{0, 0, 3}}}}));
             app.view().showEigenvectors = true;
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

    list.push_back({"Breaking a matrix down", "Rotate, stretch, rotate", {
        {"Watch the steps",
         "Every matrix can be done in three simple moves: a rotation, a stretch along the axes, and another "
         "rotation (the singular value decomposition).",
         [](App& app) {
             freshMatrices(app);
             app.setMatrix3(Matrix3D({{{{1, 0.8f, 0}}, {{0, 1, 0}}, {{0, 0, 1.4f}}}}));
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
         "Complex numbers are points of the plane. Multiplying by z₂ turns z₁ by the angle of z₂ and stretches it "
         "by the length of z₂.",
         [](App& app) {
             app.setMode(AppMode::Complex);
             ComplexScene s;
             s.z1 = Complex(2, 1);
             s.z2 = Complex(1, 0);
             s.showEuler = false;
             app.setComplexScene(s);
             app.resetCamera();
         },
         "Set z₂ = i (real part 0, imaginary part 1).",
         [](App& app, const TutorialContext&) {
             Complex z = app.complexScene().z2;
             return near(z.re, 0) && near(z.im, 1);
         },
         "z₁z₂ jumps a quarter turn counter-clockwise.", "complex.z2"},
        {"Double it",
         "The length of the product is the product of the lengths: |z₁z₂| = |z₁| |z₂|.",
         nullptr, "Give z₂ length 2 (for example 2i or 2).",
         [](App& app, const TutorialContext&) { return near(app.complexScene().z2.abs(), 2.0f); },
         "Try z₂ = 0 + 2i: the product turns 90° and doubles.", "complex.z2"},
    }});

    list.push_back({"Euler's formula", "e^(iθ) walks around the circle", {
        {"Half a turn",
         "e^(iθ) = cos θ + i sin θ is the point at angle θ on the unit circle. Its shadows on the axes are cos θ "
         "and sin θ.",
         [](App& app) {
             app.setMode(AppMode::Complex);
             ComplexScene s;
             s.showProduct = false;
             s.showEuler = true;
             s.theta = 0.5f;
             app.setComplexScene(s);
             app.resetCamera();
         },
         "Drag θ to π (about 3.14).",
         [](App& app, const TutorialContext&) { return near(app.complexScene().theta, kPi, 0.03f); },
         "At θ = π the point reaches −1: e^(iπ) = −1.", "complex.theta"},
    }});

    list.push_back({"Plotting", "Functions, curves and surfaces", {
        {"Your own function",
         "Type any formula in x. Use ^ for powers, and functions like sin, exp and sqrt.",
         [](App& app) {
             app.setMode(AppMode::Plotting);
             PlotSpec s;
             s.type = PlotType::Function2D;
             s.f = "sin(x)";
             app.setPlotSpec(s);
             app.resetCamera();
         },
         "Replace sin(x) with a function of your own and press Enter.",
         [](App& app, const TutorialContext&) {
             PlotSpec s = app.plotSpec();
             return s.type == PlotType::Function2D && s.f != "sin(x)" && app.plot().error().empty();
         },
         "For example x^2/4 - 2 or 3sin(x)/x.", "plot.expr"},
        {"A surface",
         "With two inputs, z = f(x, y) is a height above every point of the floor.",
         nullptr, "Switch to Surface and plot a hill, such as 4exp(-(x^2 + y^2)/5).",
         [](App& app, const TutorialContext&) {
             PlotSpec s = app.plotSpec();
             return s.type == PlotType::Surface && s.f != "sin(sqrt(x^2 + y^2))" && app.plot().error().empty();
         },
         "Pick Surface in the plot type tabs, then edit the z = field.", "plot.type"},
    }});
    return list;
}

} // namespace

const std::vector<Lesson>& lessons() {
    static const std::vector<Lesson> list = build();
    return list;
}

} // namespace linalg
