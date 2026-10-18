// Tests: The formula engine: parsing, values, functions, blending, the field variable z, quaternion formulas and the formula reference
#include "TestSupport.hpp"

void testExpression() {
    Expression e;
    CHECK(e.parse("1 + 2 * 3"));
    CHECK_NEAR(e.eval(0), 7.0, 1e-12);

    CHECK(e.parse("2^3^2"));  // right-associative
    CHECK_NEAR(e.eval(0), 512.0, 1e-9);

    CHECK(e.parse("-x^2"));
    CHECK_NEAR(e.eval(3), -9.0, 1e-12);

    CHECK(e.parse("2x + 3(x+1)"));
    CHECK_NEAR(e.eval(2), 13.0, 1e-12);

    CHECK(e.parse("sin(pi/2) + cos(0) + sqrt(16) + abs(-2)"));
    CHECK_NEAR(e.eval(0), 8.0, 1e-12);

    CHECK(e.parse("x*y + t"));
    CHECK_NEAR(e.eval(2, 3, 4), 10.0, 1e-12);

    CHECK(e.parse("exp(-(x^2 + y^2))"));
    CHECK_NEAR(e.eval(0, 0), 1.0, 1e-12);

    CHECK(e.parse("1/x"));
    CHECK(std::isinf(e.eval(0)));

    CHECK(e.parse("2 ^ -1"));
    CHECK_NEAR(e.eval(0), 0.5, 1e-12);

    CHECK(e.parse("1.5e2"));
    CHECK_NEAR(e.eval(0), 150.0, 1e-12);

    // Errors
    CHECK(!e.parse("1 +"));
    CHECK(!e.error().empty());
    CHECK(!e.valid());
    CHECK(std::isnan(e.eval(0)));
    CHECK(!e.parse("sin x"));
    CHECK(!e.parse("(1 + 2"));
    CHECK(!e.parse("foo(1)"));
    CHECK(!e.parse("1 $ 2"));
    CHECK(!e.parse(""));
}

void testExprEngine() {
    using expr::Type;
    using expr::Value;

    // Numbers and complex numbers
    CHECK_NEAR(evalText("2 + 3*4").re, 14.0, 1e-12);
    Value z = evalText("(1 + 2i)(3 - i)");
    CHECK(z.type == Type::Complex);
    CHECK_NEAR(z.re, 5.0, 1e-12);
    CHECK_NEAR(z.im, 5.0, 1e-12);
    Value i2 = evalText("i^2");
    CHECK_NEAR(i2.re, -1.0, 1e-12);
    CHECK_NEAR(i2.im, 0.0, 1e-12);
    Value euler = evalText("exp(i pi)");
    CHECK_NEAR(euler.re, -1.0, 1e-9);
    CHECK_NEAR(std::fabs(euler.im), 0.0, 1e-9);
    CHECK_NEAR(evalText("abs(3 + 4i)").re, 5.0, 1e-12);
    CHECK_NEAR(evalText("arg(i)").re, 3.14159265 / 2, 1e-6);
    Value p = evalText("polar(2, pi/2)");
    CHECK_NEAR(p.im, 2.0, 1e-9);

    // Vectors
    Value v = evalText("[1, 2, 3] + 2[0, 1, 0]");
    CHECK(v.type == Type::Vector);
    CHECK_NEAR(v.vector.y, 4.0f, 1e-6f);
    CHECK(evalText("cross([1,0,0], [0,1,0])").vector.z == 1.0f);
    CHECK_NEAR(evalText("dot([1,2,3], [4,5,6])").re, 32.0, 1e-9);
    CHECK_NEAR(evalText("norm([3, 4, 0])").re, 5.0, 1e-6);
    CHECK_NEAR(evalText("[1, 2, 3].y").re, 2.0, 1e-9);
    CHECK(!evalText("[1,0,0] * [0,1,0]").ok());  // suggests dot / cross
    CHECK(evalText("[1,0,0] * [0,1,0]").error.find("dot") != std::string::npos);

    // Matrices
    Value m = evalText("[[1, 2, 0], [0, 1, 0], [0, 0, 1]]");
    CHECK(m.type == Type::Matrix && m.size == 3);
    CHECK_NEAR(m.matrix.m[0][1], 2.0f, 1e-6f);
    Value mv = evalText("[[0,-1,0],[1,0,0],[0,0,1]] * [1, 0, 0]");
    CHECK_NEAR(mv.vector.y, 1.0f, 1e-6f);
    CHECK_NEAR(evalText("det(scale(2))").re, 8.0, 1e-6);
    CHECK_NEAR(evalText("tr(eye(4))").re, 4.0, 1e-9);
    Value r = evalText("rotz(pi/2) * [1, 0, 0]");
    CHECK_NEAR(r.vector.y, 1.0f, 1e-6f);
    Value ra = evalText("rot([0, 0, 1], pi/2) * [1, 0, 0]");
    CHECK_NEAR(ra.vector.y, 1.0f, 1e-6f);
    Value cols = evalText("[[1,0,0],[0,1,0],[0,0,1]]");
    CHECK(cols.ok());
    Value fromColumns = evalText("[[0,1,0], [1,0,0], [0,0,1]]");
    CHECK(fromColumns.ok());
    Value colMatrix = evalText("[cross([1,0,0],[0,1,0]), [1,0,0], [0,1,0]]");  // columns
    CHECK(colMatrix.type == Type::Matrix);
    CHECK_NEAR(colMatrix.matrix.m[2][0], 1.0f, 1e-6f);
    Value t = evalText("translate(1, 2, 3) * [0, 0, 0]");
    CHECK_NEAR(t.vector.z, 3.0f, 1e-6f);
    CHECK(!evalText("inv([[1,1,0],[1,1,0],[0,0,1]])").ok());
    Value mi = evalText("scale(2)^-1 * [2, 4, 6]");
    CHECK_NEAR(mi.vector.z, 3.0f, 1e-5f);
    CHECK(!evalText("[[1,2],[3,4]]").ok());           // 2x2 not supported
    CHECK(!evalText("[[1,2,3],[4,5,6]]").ok());       // not square

    // Names, dependencies, slots
    MapResolver res;
    res.values["a"] = Value::vec(Vector3D(1, 0, 0));
    res.values["b"] = Value::vec(Vector3D(0, 2, 0));
    res.values["M"] = Value::mat3(Matrix3D::scale(3, 3));
    res.values["theta"] = Value::scalar(3.14159265 / 2);
    expr::Program prog;
    CHECK(prog.compile("M*(a + b) + cross(a, b) * cos(theta)"));
    CHECK(prog.names() == std::set<std::string>({"M", "a", "b", "theta"}));
    Value pv = prog.eval(&res);
    CHECK(pv.type == Type::Vector);
    CHECK_NEAR(pv.vector.x, 3.0f, 1e-5f);
    CHECK_NEAR(pv.vector.y, 6.0f, 1e-5f);
    CHECK(evalText("a + q", &res).error.find("'q'") != std::string::npos);

    expr::Program plot;
    CHECK(plot.compile("x^2 + y*t + theta"));
    CHECK(plot.usesX() && plot.usesY() && plot.usesT());
    CHECK(plot.names() == std::set<std::string>({"theta"}));
    expr::Slots slots;
    slots.x = 2;
    slots.y = 3;
    slots.t = 1;
    slots.hasX = slots.hasY = slots.hasT = true;
    CHECK_NEAR(plot.eval(&res, &slots).re, 4 + 3 + 3.14159265 / 2, 1e-6);
    CHECK(!plot.eval(&res).ok());  // x outside a plot

    // Parse errors and names
    expr::Program bad;
    CHECK(!bad.compile("dot([1,0,0])"));   // wrong argument count
    CHECK(!bad.compile("[1, 2"));
    CHECK(!bad.compile("a.u"));
    CHECK(bad.compile("[1, 0, 0].w"));  // .w is for quaternions: an evaluation error on a vector
    CHECK(bad.eval().error.find(".w is for quaternions") == 0);
    CHECK(expr::isValidName("z1"));
    CHECK(expr::isValidName("M_2"));
    CHECK(!expr::isValidName("sin"));
    CHECK(!expr::isValidName("2a"));
    CHECK(!expr::isValidName("x"));
    CHECK(evalText("[1, 2, 3]").summary() == "(1, 2, 3)");
    CHECK(evalText("1 - 2i").summary() == "1 − 2i");
    CHECK(evalText("-i").summary() == "−i");
}

void testBlendFunctions() {
    using namespace expr;
    auto run = [](const std::string& text) {
        Program p;
        if (!p.compile(text)) return Value::fail(p.error());
        return p.eval(nullptr);
    };
    CHECK_NEAR(run("lerp(2, 4, 0.25)").re, 2.5, 1e-12);
    CHECK_NEAR(run("mod(7, 5)").re, 2.0, 1e-12);
    CHECK_NEAR(run("mod(-1, 5)").re, 4.0, 1e-12);
    CHECK(!run("mod(1, 0)").ok());
    Value v = run("lerp([0, 0, 0], [2, 4, 6], 0.5)");
    CHECK(v.type == Type::Vector && std::fabs(v.vector.y - 2.0f) < 1e-6f);
    v = run("lerp(0, [2, 4, 6], 0.5)");  // 0 = zero vector
    CHECK(v.type == Type::Vector && std::fabs(v.vector.z - 3.0f) < 1e-6f);
    v = run("lerp(1, i, 0.5)");
    CHECK(v.type == Type::Complex && std::fabs(v.re - 0.5) < 1e-12 && std::fabs(v.im - 0.5) < 1e-12);
    v = run("lerp(eye(), 3eye(), 0.5)");
    CHECK(v.type == Type::Matrix && std::fabs(v.matrix.m[1][1] - 2.0f) < 1e-6f && std::fabs(v.matrix.m[0][1]) < 1e-6f);
    CHECK(!run("lerp([1, 0, 0], eye(), 0.5)").ok());
    CHECK(!run("lerp(1, 2, i)").ok());
    // keys: s = 0, 0.5, 1 hit the values; in between it blends
    CHECK_NEAR(run("keys(0, 1, 5, 3)").re, 1.0, 1e-12);
    CHECK_NEAR(run("keys(0.5, 1, 5, 3)").re, 5.0, 1e-12);
    CHECK_NEAR(run("keys(1, 1, 5, 3)").re, 3.0, 1e-12);
    CHECK_NEAR(run("keys(0.75, 1, 5, 3)").re, 4.0, 1e-12);
    CHECK_NEAR(run("keys(2, 1, 5, 3)").re, 3.0, 1e-12);  // clamped
    v = run("keys(0.25, eye(), rotz(pi/2), 2eye())");
    CHECK(v.type == Type::Matrix);
}

void testFieldVariable() {
    using namespace expr;
    // z is a variable only with kVarZ; otherwise a name (objects may be called z)
    Program f;
    CHECK(f.compile("[-y, x, z/2]", kVarZ));
    CHECK(f.usesX() && f.usesY() && f.usesZ());
    CHECK(f.names().empty());
    Slots s;
    s.x = 1, s.y = 2, s.z = 4;
    s.hasX = s.hasY = s.hasZ = true;
    Value v = f.eval(nullptr, &s);
    CHECK(v.type == Type::Vector);
    CHECK_NEAR(v.vector.x, -2, 1e-12);
    CHECK_NEAR(v.vector.y, 1, 1e-12);
    CHECK_NEAR(v.vector.z, 2, 1e-12);
    Slots noZ = s;
    noZ.hasZ = false;
    CHECK(f.eval(nullptr, &noZ).error == "z is only available in vector fields");

    Program plain;
    CHECK(plain.compile("z + 1"));
    CHECK(!plain.usesZ());
    CHECK(plain.names().count("z") == 1);
    Program both;
    CHECK(both.compile("u + z", kVarU | kVarZ));
    CHECK(both.usesU() && both.usesZ() && both.names().empty());
    CHECK(both.compile("u + z", true));  // true = curves: u only
    CHECK(both.usesU() && !both.usesZ() && both.names().count("z") == 1);

    // Field formulas must give vectors
    CHECK(scene::checkExpect(scene::Expect::Field, Value::vec({1, 2, 3})).empty());
    CHECK(!scene::checkExpect(scene::Expect::Field, Value::scalar(1)).empty());
}

void testQuaternions() {
    using namespace expr;
    auto ev = [](const char* text) {
        Program p;
        if (!p.compile(text)) return Value::fail("parse: " + p.error());
        return p.eval();
    };
    auto near = [](const Value& v, double w, double x, double y, double z) {
        return v.type == Type::Quaternion && std::fabs(v.re - w) < 1e-9 && std::fabs(v.qi - x) < 1e-9 &&
               std::fabs(v.qj - y) < 1e-9 && std::fabs(v.qk - z) < 1e-9;
    };
    // Hamilton's rules: i·j = k, j·i = −k, i² = −1
    CHECK(near(ev("quat(0, 1, 0, 0) * quat(0, 0, 1, 0)"), 0, 0, 0, 1));
    CHECK(near(ev("quat(0, 0, 1, 0) * quat(0, 1, 0, 0)"), 0, 0, 0, -1));
    CHECK(near(ev("quat(0, 1, 0, 0)^2"), -1, 0, 0, 0));
    CHECK(near(ev("2 * quat(1, 2, 3, 4) - quat(1, 2, 3, 4)"), 1, 2, 3, 4));
    CHECK(near(ev("quat(1, 2, 3, 4) + 1"), 2, 2, 3, 4));
    CHECK(near(ev("conj(quat(1, 2, 3, 4))"), 1, -2, -3, -4));
    CHECK(near(ev("quat(1, 2, 3, 4) * inv(quat(1, 2, 3, 4))"), 1, 0, 0, 0));
    CHECK(near(ev("quat(1, 2, 3, 4) / quat(1, 2, 3, 4)"), 1, 0, 0, 0));
    CHECK_NEAR(ev("abs(quat(1, 1, 1, 1))").re, 2, 1e-12);
    CHECK_NEAR(ev("abs(unit(quat(1, 2, 3, 4)))").re, 1, 1e-12);
    CHECK_NEAR(ev("quat(1, 2, 3, 4).y").re, 3, 1e-12);
    CHECK_NEAR(ev("quat(1, 2, 3, 4).w").re, 1, 1e-12);
    CHECK(ev("quat(1, 0, 0, 0.7071)").summary() == "1 + 0.707k");
    CHECK(ev("quat(0, -1, 0, 0)").summary() == "−i");
    CHECK(ev("quat(0, 0, 0, 0)").summary() == "0");
    // Rotations: qaxis agrees with rotz, rotate turns vectors, angle/axis read it back
    const double s2 = std::sqrt(0.5);
    CHECK(near(ev("qaxis([0, 0, 2], pi/2)"), s2, 0, 0, s2));
    Value turned = ev("rotate(qaxis([0, 0, 1], pi/2), [1, 0, 0])");
    CHECK(turned.type == Type::Vector && std::fabs(turned.vector.y - 1) < 1e-6 && std::fabs(turned.vector.x) < 1e-6);
    Value m = ev("mat(qaxis([0, 0, 1], 0.7)) - rotz(0.7)");
    double worst = 0;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) worst = std::max(worst, static_cast<double>(std::fabs(m.matrix.m[i][j])));
    CHECK(worst < 1e-6);
    CHECK_NEAR(ev("angle(qaxis([1, 1, 0], 2))").re, 2, 1e-6);  // vectors are single precision
    CHECK_NEAR(ev("angle(-qaxis([1, 1, 0], 2))").re, 2, 1e-6);  // −q is the same rotation
    CHECK_NEAR(ev("axis(qaxis([0, 3, 0], 1)).y").re, 1, 1e-6);
    CHECK(ev("axis(quat(1, 0, 0, 0))").error.find("no rotation") == 0);
    // quat(M) round trip (w ≥ 0), and only for rotation matrices
    CHECK(near(ev("quat(mat(qaxis([1, 2, 2], 1.2)))"), ev("qaxis([1, 2, 2], 1.2)").re,
               ev("qaxis([1, 2, 2], 1.2)").qi, ev("qaxis([1, 2, 2], 1.2)").qj, ev("qaxis([1, 2, 2], 1.2)").qk) ||
          std::fabs(ev("abs(quat(mat(qaxis([1, 2, 2], 1.2))) - qaxis([1, 2, 2], 1.2))").re) < 1e-5);
    CHECK(!ev("quat(scale(2))").ok());
    // Order matters: rotating about x then z differs from z then x
    CHECK(ev("abs(qaxis([0, 0, 1], pi/2) * qaxis([1, 0, 0], pi/2) - qaxis([1, 0, 0], pi/2) * qaxis([0, 0, 1], pi/2))").re > 0.5);
    // slerp: halfway between the identity and a half turn about z is a quarter turn; lerp/keys slerp too
    CHECK(near(ev("slerp(quat(1, 0, 0, 0), qaxis([0, 0, 1], pi), 0.5)"), s2, 0, 0, s2));
    CHECK(near(ev("lerp(quat(1, 0, 0, 0), qaxis([0, 0, 1], pi), 0.5)"), s2, 0, 0, s2));
    CHECK(near(ev("keys(0.5, quat(1, 0, 0, 0), qaxis([0, 0, 1], pi/2), qaxis([0, 0, 1], pi))"), s2, 0, 0, s2));
    CHECK_NEAR(ev("abs(slerp(quat(2, 0, 0, 0), quat(0, 0, 0, 3), 0.3))").re, 1, 1e-9);  // unit result
    // Mixing with other kinds explains itself
    CHECK(ev("qaxis([0, 0, 1], 1) * [1, 0, 0]").error.find("rotate(q, v)") != std::string::npos);
    CHECK(ev("qaxis([0, 0, 1], 1) * rotz(1)").error.find("mat(q)") != std::string::npos);
    CHECK(ev("qaxis([0, 0, 1], 1) + (1 + 2i)").error.find("quat(re, im, 0, 0)") != std::string::npos);
    CHECK(!ev("quat(1, 2, 3, 4)^0.5").ok());
    // Common words stay usable as object names: angle( is the function, angle alone a name
    CHECK(isValidName("angle") && isValidName("axis") && isValidName("mat") && isValidName("rotate"));
    CHECK(!isValidName("qaxis") && !isValidName("slerp"));
    Program named;
    CHECK(named.compile("2*angle + angle(qaxis([0, 0, 1], 1))"));
    CHECK(named.names().count("angle") == 1);
}

void testReference() {
    using namespace expr;
    // Every example compiles; those that need no objects evaluate without errors
    Slots slots;
    slots.x = 1;
    slots.y = 2;
    slots.t = 0.5;
    slots.u = 0.5;
    slots.z = 3;
    slots.hasX = slots.hasY = slots.hasT = slots.hasU = slots.hasZ = true;
    std::string documented;
    int examples = 0;
    for (const auto& section : reference()) {
        CHECK(!section.title.empty() && !section.entries.empty());
        for (const auto& e : section.entries) {
            documented += " " + e.syntax + " ";
            if (e.example.empty()) continue;
            examples++;
            Program p;
            bool ok = p.compile(e.example, kVarU | kVarZ);
            if (!ok) fprintf(stderr, "  reference example does not compile: %s (%s)\n", e.example.c_str(), p.error().c_str());
            CHECK(ok);
            if (!ok || e.usesObjects) continue;
            Value v = p.eval(nullptr, &slots);
            if (!v.ok()) fprintf(stderr, "  reference example fails: %s -> %s\n", e.example.c_str(), v.error.c_str());
            CHECK(v.ok());
        }
    }
    CHECK(examples > 40);
    // Every built-in function is documented somewhere
    auto isWordChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    for (const auto& name : functionNames()) {
        bool found = false;
        for (size_t at = documented.find(name); at != std::string::npos && !found; at = documented.find(name, at + 1)) {
            bool before = at > 0 && isWordChar(documented[at - 1]);
            bool after = at + name.size() < documented.size() && isWordChar(documented[at + name.size()]);
            found = !before && !after;
        }
        if (!found) fprintf(stderr, "  function not in the reference: %s\n", name.c_str());
        CHECK(found);
    }
    // A few results the text promises
    auto run = [&](const char* text) {
        Program p;
        p.compile(text, true);
        return p.eval(nullptr, &slots);
    };
    CHECK_NEAR(run("2^3^2").re, 512.0, 1e-9);
    CHECK_NEAR(run("-2^2").re, -4.0, 1e-12);
    CHECK_NEAR(run("[1, 2, 3].y").re, 2.0, 1e-12);
    CHECK_NEAR(run("atan2(1, -1)").re, 3 * 3.14159265358979 / 4, 1e-9);
    CHECK_NEAR(run("keys(0.25, 0, 10, 0)").re, 5.0, 1e-9);
}
