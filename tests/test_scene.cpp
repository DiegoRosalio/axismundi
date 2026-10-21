// Tests: The scene model: objects, spaces, files, undo, the clock and animations, every object type, usability details
#include "TestSupport.hpp"

void testPlayback() {
    // Every easing starts at 0 and ends at 1
    for (int i = 0; i < kEasingCount; i++) {
        Easing e = static_cast<Easing>(i);
        CHECK_NEAR(ease(e, 0.0f), 0.0f, 1e-5f);
        CHECK_NEAR(ease(e, 1.0f), 1.0f, 1e-5f);
        CHECK(std::string(easingName(e)).size() > 0);
    }
    // The monotonic ones never go backwards
    for (Easing e : {Easing::Linear, Easing::EaseIn, Easing::EaseOut, Easing::EaseInOut, Easing::Sine, Easing::Step}) {
        float prev = 0.0f;
        for (int k = 1; k <= 100; k++) {
            float v = ease(e, k / 100.0f);
            CHECK(v >= prev - 1e-6f);
            prev = v;
        }
    }
    CHECK(ease(Easing::Back, 0.8f) > 1.0f);         // overshoots
    CHECK_NEAR(ease(Easing::EaseInOut, 0.5f), 0.5f, 1e-5f);
    CHECK(ease(Easing::EaseIn, 0.5f) < 0.5f);
    CHECK(ease(Easing::EaseOut, 0.5f) > 0.5f);

    Playback p;
    p.duration = 2.0f;
    p.repeat = Repeat::Once;
    p.restart();
    p.advance(1.0f);
    CHECK_NEAR(p.phase, 0.5f, 1e-5f);
    p.advance(5.0f);
    CHECK_NEAR(p.phase, 1.0f, 1e-6f);
    CHECK(!p.playing);
    p.toggle();  // replays from the start
    CHECK(p.playing && p.phase == 0.0f);

    p.repeat = Repeat::Loop;
    p.advance(3.0f);  // 1.5 runs
    CHECK_NEAR(p.phase, 0.5f, 1e-5f);

    p.repeat = Repeat::PingPong;
    p.restart();
    p.advance(3.0f);  // to the end and half way back
    CHECK_NEAR(p.phase, 0.5f, 1e-5f);
    CHECK(p.direction == -1);
    p.advance(1.5f);  // past the start, forwards again
    CHECK_NEAR(p.phase, 0.25f, 1e-5f);
    CHECK(p.direction == 1);

    // Pure phase from a clock (scrubbable)
    Playback c;
    c.duration = 2.0f;
    c.repeat = Repeat::Once;
    CHECK_NEAR(c.phaseAt(0.0), 0.0f, 1e-6f);
    CHECK_NEAR(c.phaseAt(1.0), 0.5f, 1e-6f);
    CHECK_NEAR(c.phaseAt(9.0), 1.0f, 1e-6f);
    c.repeat = Repeat::Loop;
    CHECK_NEAR(c.phaseAt(3.0), 0.5f, 1e-6f);
    CHECK_NEAR(c.phaseAt(4.0), 1.0f, 1e-6f);
    c.repeat = Repeat::PingPong;
    CHECK_NEAR(c.phaseAt(3.0), 0.5f, 1e-6f);   // on the way back
    CHECK_NEAR(c.phaseAt(2.0), 1.0f, 1e-6f);
    CHECK_NEAR(c.phaseAt(5.0), 0.5f, 1e-6f);   // forwards again
    c.easing = Easing::EaseIn;
    CHECK_NEAR(c.valueAt(1.0), 0.25f, 1e-6f);

    SceneData data;
    p.easing = Easing::Bounce;
    p.save(data, "x.");
    Playback q;
    q.load(data, "x.");
    CHECK(q.easing == Easing::Bounce && q.repeat == Repeat::PingPong);
    CHECK_NEAR(q.duration, 2.0f, 1e-5f);
}

void testSceneTime() {
    using namespace scene;
    Scene sc;
    Space& d = sc.space(SpaceKind::Euclidean3D);
    d.clear();
    // t is the scene time in any formula
    auto* v = static_cast<VectorObject*>(sc.add(SpaceKind::Euclidean3D, "vector", "v"));
    v->setByFormula(true);
    v->setField("value", "[cos(t), sin(t), 1]");
    sc.setTime(0.0);
    sc.evaluate();
    CHECK(v->error().empty());
    CHECK_NEAR(v->value().vector.x, 1.0f, 1e-6f);
    sc.timeline().length = 10.0;
    sc.setTime(3.14159265 / 2);
    sc.evaluate();
    CHECK_NEAR(v->value().vector.y, 1.0f, 1e-5f);
    auto* m = static_cast<MatrixObject*>(sc.add(SpaceKind::Euclidean3D, "matrix", "R"));
    m->setByFormula(true);
    m->setField("value", "rotz(t)");
    sc.evaluate();
    CHECK(m->error().empty());

    // Plots using t change with the time; plots without t do not
    auto* wave = sc.add(SpaceKind::Euclidean3D, "function", "w");
    wave->setField("f", "sin(x - t)");
    auto* still = sc.add(SpaceKind::Euclidean3D, "function", "g");
    still->setField("f", "sin(x)");
    sc.evaluate();
    std::uint64_t w0 = wave->changedAt(), g0 = still->changedAt();
    sc.evaluate();  // same time: nothing changes
    CHECK(wave->changedAt() == w0);
    sc.setTime(2.0);
    sc.evaluate();
    CHECK(wave->changedAt() > w0);
    CHECK(still->changedAt() == g0);
    CHECK(wave->error().empty());

    // Curves: u is their own variable, t the scene time
    auto* c = static_cast<PlotObject*>(sc.add(SpaceKind::Euclidean3D, "curve", "c"));
    c->setField("x", "cos(u + t)");
    sc.evaluate();
    CHECK(c->error().empty());
    CHECK(c->field("x")->program.usesU() && c->field("x")->program.usesT());
    CHECK(c->field("umin") && !c->field("tmin"));

    // An object named u stays a normal name (and a dependency) outside curves
    auto* u = static_cast<VectorObject*>(sc.add(SpaceKind::Euclidean3D, "vector", "u"));
    u->setField("x", "2");
    auto* w = static_cast<VectorObject*>(sc.add(SpaceKind::Euclidean3D, "vector", "q"));
    w->setByFormula(true);
    w->setField("value", "3u");
    sc.evaluate();
    CHECK(w->error().empty());
    CHECK(w->dependencies().count("u") == 1);
    CHECK_NEAR(w->value().vector.x, 6.0f, 1e-6f);
    CHECK(c->dependencies().count("u") == 0);  // inside the curve, u is the curve variable

    // Old files: curves with t and tmin/tmax become u and umin/umax
    SceneData data;
    sc.save(data);
    std::string cp;
    for (auto& [key, val] : data)
        if (val == "c" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0) cp = key.substr(0, key.size() - 4);
    CHECK(!cp.empty());
    data.erase(cp + "field.umin");
    data.erase(cp + "field.umax");
    data[cp + "field.tmin"] = "-1";
    data[cp + "field.tmax"] = "pi";
    data[cp + "field.x"] = "cos(t) + 2t";
    data[cp + "field.y"] = "tan(t)";
    Scene old;
    std::string err;
    CHECK(old.load(data, err));
    auto* c2 = old.space(SpaceKind::Euclidean3D).find("c");
    CHECK(c2 && c2->field("x")->text == "cos(u) + 2u" && c2->field("y")->text == "tan(u)");
    CHECK(c2 && c2->field("umin")->text == "-1" && c2->field("umax")->text == "pi");
    CHECK(c2 && c2->error().empty());

    // Engine messages
    expr::Program p;
    CHECK(p.compile("t + 1"));
    CHECK(!p.eval(nullptr).ok());
    CHECK(p.compile("u", true) && p.usesU() && p.names().empty());
    CHECK(p.compile("u") && !p.usesU() && p.names().count("u"));
}

void testScene() {
    using namespace scene;
    Scene sc;
    Space& d = sc.space(SpaceKind::Euclidean3D);
    sc.evaluate();

    // Default picture: derived vectors follow their inputs
    const SceneObject* c = d.find("c");
    CHECK(c && c->value().type == expr::Type::Vector);
    if (c) CHECK_NEAR(c->value().vector.z, 1.0f, 1e-6f);
    d.find("b")->setField("y", "2");
    sc.evaluate();
    CHECK_NEAR(d.find("c")->value().vector.z, 2.0f, 1e-6f);
    CHECK_NEAR(d.find("s")->value().vector.y, 2.0f, 1e-6f);

    // Formula matrix driven by a parameter
    auto* theta = static_cast<ParameterObject*>(sc.add(SpaceKind::Euclidean3D, "parameter", "theta"));
    auto* m = static_cast<MatrixObject*>(d.find("M"));
    m->setByFormula(true);
    m->setField("value", "rotz(theta)");
    theta->setNumber(3.14159265 / 2);
    sc.evaluate();
    CHECK_NEAR(d.find("Ma")->value().vector.y, 1.0f, 1e-5f);
    std::uint64_t before = d.find("Ma")->changedAt();
    theta->setMode(ParameterObject::Mode::Animate);
    sc.setTime(0.5);
    sc.evaluate();
    CHECK(d.find("Ma")->changedAt() > before);  // change propagated

    // Plot detail per unit
    {
        Scene ps;
        Space& e = ps.space(SpaceKind::Euclidean3D);
        auto* f = static_cast<PlotObject*>(ps.add(SpaceKind::Euclidean3D, "function", "f"));
        f->setField("xmin", "-5");
        f->setField("xmax", "5");
        f->setDensity(30);
        ps.evaluate();
        CHECK(f->samples().nx == 300);
        f->setField("xmax", "15");  // twice the range, twice the samples
        ps.evaluate();
        CHECK(f->samples().nx == 600);
        CHECK(!f->samples().capped);
        f->setField("xmax", "2000");
        ps.evaluate();
        CHECK(f->samples().capped && f->samples().nx == PlotObject::densityRange(PlotObject::Kind::Function).cap);
        auto* sf = static_cast<PlotObject*>(ps.add(SpaceKind::Euclidean3D, "surface", "S"));
        sf->setField("xmin", "-2");
        sf->setField("xmax", "2");
        sf->setField("ymin", "0");
        sf->setField("ymax", "1");
        sf->setDensity(10);
        ps.evaluate();
        CHECK(sf->samples().nx == 40 && sf->samples().ny == 10);
        sf->setDensity(1000);  // clamped to the slider range
        CHECK_NEAR(sf->density(), PlotObject::densityRange(PlotObject::Kind::Surface).max, 1e-9);
        // Old files: resolution 60 on a surface over 12 units -> 5 cells per unit
        SceneData data;
        sf->setField("xmin", "-6");
        sf->setField("xmax", "6");
        sf->setField("ymin", "-6");
        sf->setField("ymax", "6");
        ps.evaluate();
        ps.save(data);
        for (auto it = data.begin(); it != data.end();)
            it = it->first.find(".prop.density") != std::string::npos ? data.erase(it) : std::next(it);
        for (auto& [key, v] : data)
            if (v == "S" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0)
                data[key.substr(0, key.size() - 4) + "prop.resolution"] = "60";
        Scene old;
        std::string err;
        CHECK(old.load(data, err));
        auto* s2 = static_cast<PlotObject*>(old.space(SpaceKind::Euclidean3D).find("S"));
        CHECK(s2 && !s2->props().count("resolution"));
        if (s2) CHECK_NEAR(s2->density(), 5.0, 1e-9);
        (void)e;
    }

    // Fourier objects: epicycles follow the clock with cached coefficients; the series of f(x)
    {
        Scene fsc;
        fsc.clearAll();
        auto* E = static_cast<EpicyclesObject*>(fsc.add(SpaceKind::Complex, "epicycles", "E"));
        E->setField("z", "2exp(i*u) + 0.5exp(-2i*u)");
        E->setField("N", "2");
        E->setField("speed", "0.25");
        fsc.evaluate();
        CHECK(E->error().empty() && E->value().isNumber());
        CHECK_NEAR(E->value().re, 2.5, 1e-9);  // t = 0: both circles point right
        CHECK(E->terms().size() == 2 * EpicyclesObject::kMaxK + 1);
        fsc.timeline().time = 1.0;  // a quarter lap: 2i + 0.5·e^{−iπ} = −0.5 + 2i
        fsc.evaluate();
        CHECK_NEAR(E->value().re, -0.5, 1e-9);
        CHECK_NEAR(E->value().im, 2.0, 1e-9);
        E->setField("N", "1");  // only the biggest circle
        fsc.evaluate();
        CHECK_NEAR(E->value().re, 0.0, 1e-9);
        CHECK_NEAR(E->value().im, 2.0, 1e-9);
        E->setField("z", "1/(cos(u) - cos(u))");  // division by zero everywhere
        fsc.evaluate();
        CHECK(E->error().find("undefined") != std::string::npos);

        auto* S = static_cast<FourierSeriesObject*>(fsc.add(SpaceKind::Euclidean3D, "fourierseries", "S"));
        fsc.evaluate();
        CHECK(S->error().empty());
        CHECK(S->series().bk.size() == 5);
        CHECK_NEAR(S->series().bk[0], 2.0, 1e-6);  // f = x: b1 = 2
        CHECK(!S->sum().empty() && !S->original().empty() && S->harmonics().size() == 5);
        S->setField("N", "40");
        fsc.evaluate();
        CHECK(S->series().bk.size() == 40 && S->harmonics().size() == 8);  // at most 8 drawn
        S->setField("a", "4");
        fsc.evaluate();
        CHECK(S->error().find("must be less than") != std::string::npos);
    }

    // Moving frame: the value is the point; κ and τ of the default helix
    {
        Scene gs;
        gs.clearAll();
        auto* P = static_cast<FrameObject*>(gs.add(SpaceKind::Euclidean3D, "frame", "P"));
        gs.evaluate();
        CHECK(P->error().empty() && P->value().type == expr::Type::Vector);
        CHECK_NEAR(P->frame().kappa, 2.0 / (4 + 1.0 / 16), 1e-4);  // 2cos u, 2sin u, u/4
        CHECK_NEAR(P->frame().tau, 0.25 / (4 + 1.0 / 16), 1e-3);
        CHECK(P->curve().size() > 100 && P->comb().size() > 50);
        auto* q = gs.add(SpaceKind::Euclidean3D, "vector", "q");
        q->setProp("mode", "formula");
        q->setField("value", "P");  // other objects use the point
        P->setField("u0", "0");
        gs.evaluate();
        CHECK(q->error().empty() && std::fabs(q->value().vector.x - 2.0f) < 1e-5f);
        P->setField("u0", "100");  // clamped to the range
        gs.evaluate();
        CHECK_NEAR(P->u0(), 4 * 3.14159265358979, 1e-6);
        P->setField("x", "1");
        P->setField("y", "1");
        P->setField("z", "1");  // standing still
        gs.evaluate();
        CHECK(P->error().find("stops") != std::string::npos);
    }

    // Derivative objects: slopes, the secant limit, partials, velocity and acceleration
    {
        Scene ds;
        ds.clearAll();
        auto* T = static_cast<TangentObject*>(ds.add(SpaceKind::Euclidean3D, "tangent", "T"));
        ds.evaluate();
        CHECK(T->error().empty() && T->value().isRealNumber());
        CHECK_NEAR(T->value().re, std::cos(1.0), 1e-8);  // sin' = cos at x0 = 1
        CHECK_NEAR(T->y0(), std::sin(1.0), 1e-12);
        CHECK(T->hasSecond());
        CHECK_NEAR(T->second(), -std::sin(1.0), 1e-4);
        CHECK(!T->hasSecant());  // h = 0: no secant
        CHECK(T->function().size() == 401 && T->derivative().size() == 401);
        T->setField("h", "0.5");
        ds.evaluate();
        CHECK(T->hasSecant());
        CHECK_NEAR(T->secantSlope(), (std::sin(1.5) - std::sin(1.0)) / 0.5, 1e-12);
        T->setField("h", "0.001");  // the secant slope approaches the tangent's
        ds.evaluate();
        CHECK(std::fabs(T->secantSlope() - T->slope()) < 1e-3);
        auto* s = static_cast<ParameterObject*>(ds.add(SpaceKind::Euclidean3D, "parameter", "s"));
        s->setNumber(2.0);
        T->setField("f", "x^3");
        T->setField("x0", "s");  // a slider moves the point
        ds.evaluate();
        CHECK_NEAR(T->value().re, 12.0, 1e-6);
        T->setField("f", "sqrt(x)");
        T->setField("x0", "0");
        ds.evaluate();
        CHECK(T->error().find("no slope") != std::string::npos);
        T->setField("f", "ln(x)");
        T->setField("x0", "-1");
        ds.evaluate();
        CHECK(T->error().find("undefined at x = -1") != std::string::npos);

        auto* G = static_cast<TangentPlaneObject*>(ds.add(SpaceKind::Euclidean3D, "tangentplane", "G"));
        ds.evaluate();
        CHECK(G->error().empty() && G->value().type == expr::Type::Vector);
        CHECK_NEAR(G->fx(), std::cos(1.0), 1e-8);    // ∂/∂x (sin x + cos y)
        CHECK_NEAR(G->fy(), -std::sin(0.5), 1e-8);   // ∂/∂y at y0 = 0.5
        CHECK_NEAR(G->point().z, std::sin(1.0) + std::cos(0.5), 1e-12);
        CHECK(G->surfaceLines().size() == 34);  // 17 lines each way, no gaps
        G->setField("ymin", "5");
        ds.evaluate();
        CHECK(G->error().find("must be less than") != std::string::npos);

        auto* R = static_cast<VelocityObject*>(ds.add(SpaceKind::Euclidean3D, "velocity", "R"));
        ds.evaluate();
        CHECK(R->error().empty() && R->value().type == expr::Type::Vector);
        // Ellipse (3cos u, 1.5 sin u): v = (−3 sin u, 1.5 cos u), a = −r
        CHECK_NEAR(R->velocity().x, -3 * std::sin(1.0), 1e-5);
        CHECK_NEAR(R->velocity().y, 1.5 * std::cos(1.0), 1e-5);
        CHECK_NEAR(R->acceleration().x, -3 * std::cos(1.0), 1e-3);
        auto sum = geometry::add(R->tangential(), R->normal());
        CHECK_NEAR(sum.x, R->acceleration().x, 1e-9);  // the two parts add up to a
        CHECK_NEAR(geometry::dot(R->normal(), R->velocity()), 0.0, 1e-6);  // the turning part is ⊥ v
        R->setField("x", "u^2");  // speeding up along a line: no turning part
        R->setField("y", "0");
        ds.evaluate();
        CHECK(geometry::length(R->normal()) < 1e-4 && R->tangential().x > 1.9);
    }

    // Taylor polynomial and Newton's method objects
    {
        Scene ts;
        ts.clearAll();
        auto* P = static_cast<TaylorObject*>(ts.add(SpaceKind::Euclidean3D, "taylor", "P"));
        ts.evaluate();
        CHECK(P->error().empty() && P->series().complexMethod);
        CHECK(P->series().coeffs.size() == 4);  // sin about 0, degree 3: x - x^3/6
        CHECK_NEAR(P->series().coeffs[1], 1.0, 1e-10);
        CHECK_NEAR(P->series().coeffs[3], -1.0 / 6, 1e-10);
        CHECK(P->polynomial().size() == 401 && P->previous().size() == 401 && P->terms().size() == 4);
        auto* s = static_cast<ParameterObject*>(ts.add(SpaceKind::Euclidean3D, "parameter", "s"));
        s->setNumber(1.0);
        P->setField("f", "exp(x)");
        P->setField("a", "s");      // a slider moves the center
        P->setField("N", "2 + 3");  // the degree is a formula
        ts.evaluate();
        CHECK(P->degree() == 5);
        CHECK_NEAR(P->series().center, 1.0, 1e-12);
        CHECK_NEAR(P->series().coeffs[2], std::exp(1.0) / 2, 1e-9);
        P->setField("f", "floor(x)");  // no complex values: degrees up to 4 by finite differences
        P->setField("a", "0.5");
        ts.evaluate();
        CHECK(P->error().empty() && !P->series().complexMethod && P->series().coeffs.size() == 5);
        P->setField("f", "ln(x)");
        P->setField("a", "-1");
        ts.evaluate();
        CHECK(P->error().find("undefined at the center") != std::string::npos);

        auto* r = static_cast<NewtonObject*>(ts.add(SpaceKind::Euclidean3D, "newton", "r"));
        ts.evaluate();
        CHECK(r->error().empty() && r->value().isRealNumber());
        CHECK(r->run().steps.size() == 4);  // x0 = 1 and three steps
        CHECK_NEAR(r->value().re, std::sqrt(2.0), 1e-5);
        r->setField("steps", "20");
        ts.evaluate();
        CHECK(r->run().stop == taylor::NewtonStop::Converged);
        CHECK_NEAR(r->value().re, std::sqrt(2.0), 1e-12);
        auto* q = ts.add(SpaceKind::Euclidean3D, "vector", "q");
        q->setProp("mode", "formula");
        q->setField("value", "[r, 0, 0]");  // other objects use the root
        ts.evaluate();
        CHECK(q->error().empty() && std::fabs(q->value().vector.x - 1.41421356f) < 1e-5f);
        r->setField("x0", "0");
        ts.evaluate();
        CHECK(r->run().stop == taylor::NewtonStop::FlatTangent);
    }

    // Integral objects: values, boxes, partial work, and the value cache
    {
        Scene is;
        is.clearAll();
        auto* V = static_cast<DoubleIntegralObject*>(is.add(SpaceKind::Euclidean3D, "doubleintegral", "V"));
        V->setField("f", "4 - x^2 - y^2");
        for (auto [k, v] : {std::pair{"xmin", "-1"}, std::pair{"xmax", "1"}, std::pair{"ymin", "-1"}, std::pair{"ymax", "1"}})
            V->setField(k, v);
        auto* k = static_cast<ParameterObject*>(is.add(SpaceKind::Euclidean3D, "parameter", "k"));
        k->setNumber(4);
        V->setField("n", "k");
        is.evaluate();
        CHECK(V->error().empty());
        CHECK_NEAR(V->value().re, 40.0 / 3.0, 1e-6);
        CHECK(V->riemann().boxes.size() == 16);
        CHECK(std::fabs(V->riemann().sum - 40.0 / 3.0) < 0.3);
        std::uint64_t version = V->geometryVersion();
        is.evaluate();
        CHECK(V->geometryVersion() == version);  // nothing changed: not recomputed
        k->setNumber(6);  // within the slider's default range 0..2π
        is.evaluate();
        CHECK(V->geometryVersion() != version && V->riemann().boxes.size() == 36);
        V->setField("xmin", "3");
        is.evaluate();
        CHECK(V->error().find("must be less than") != std::string::npos);
        V->setField("xmin", "-1");
        V->setField("f", "1/x");  // undefined at x = 0 on the precise grid
        is.evaluate();
        CHECK(V->error().find("undefined") != std::string::npos);

        auto* W = static_cast<LineIntegralObject*>(is.add(SpaceKind::Euclidean3D, "lineintegral", "W"));
        is.evaluate();
        CHECK(W->error().empty());
        CHECK_NEAR(W->value().re, 8 * 3.14159265358979, 1e-5);  // ∮ [−y, x, 0] on the circle of radius 2
        W->setField("upto", "0.25");
        is.evaluate();
        CHECK_NEAR(W->value().re, 2 * 3.14159265358979, 1e-5);
        CHECK_NEAR(W->total(), 8 * 3.14159265358979, 1e-5);
        CHECK(!W->path().empty() && W->maxAlong() > 1.9);
        CHECK(std::fabs(W->marker().y - 2.0) < 1e-6);  // a quarter of the way round: (0, 2, 0)
        // Following the clock re-evaluates
        W->setField("upto", "mod(t/4, 1)");
        is.timeline().time = 1.0;
        is.evaluate();
        CHECK_NEAR(W->value().re, 2 * 3.14159265358979, 1e-5);
    }

    // Quaternion objects: their value feeds other objects; the acts-on field only when switched on
    {
        Scene qs;
        qs.clearAll();
        auto* q = static_cast<QuaternionObject*>(qs.add(SpaceKind::Euclidean3D, "quaternion", "q"));
        CHECK(q && q->name() == "q");
        auto* w = static_cast<VectorObject*>(qs.add(SpaceKind::Euclidean3D, "vector", "w"));
        w->setProp("mode", "formula");
        w->setField("value", "rotate(q, [1, 0, 0])");
        auto* M = qs.add(SpaceKind::Euclidean3D, "matrix", "M");
        M->setProp("mode", "formula");
        M->setField("value", "mat(q)");
        qs.evaluate();
        CHECK(q->error().empty() && q->value().type == expr::Type::Quaternion);
        CHECK(w->error().empty());
        CHECK_NEAR(w->value().vector.y, 1.0, 1e-6);  // a quarter turn about z sends x to y
        CHECK(M->error().empty() && std::fabs(M->value().matrix.m[1][0] - 1.0f) < 1e-6f);
        expr::Rotation r;
        CHECK(expr::rotationOf(q->value(), r) && r.hasAxis && std::fabs(r.angle - 3.14159265358979 / 2) < 1e-9);
        // "v" is inactive until "acts on" is switched on (a bad v is then reported)
        q->setField("v", "[1, 2]");
        qs.evaluate();
        CHECK(q->error().empty());
        q->setProp("act", true);
        qs.evaluate();
        CHECK(!q->error().empty());
        q->setField("v", "[0, 1, 0]");
        q->setField("value", "[1, 0, 0]");  // a vector is not a quaternion
        qs.evaluate();
        CHECK(q->error().find("should be a quaternion") != std::string::npos);
        // Following the clock
        q->setField("value", "qaxis([0, 0, 1], t)");
        qs.timeline().time = 1.0;
        qs.evaluate();
        CHECK(q->followsClock());
        CHECK(expr::rotationOf(q->value(), r) && std::fabs(r.angle - 1.0) < 1e-9);
    }

    // Vector fields: formula of x, y, z using other objects, counts, ranges, saving
    {
        Scene fs;
        auto* k = static_cast<ParameterObject*>(fs.add(SpaceKind::Euclidean3D, "parameter", "k"));
        k->setField("value", "2");
        auto* F = static_cast<VectorFieldObject*>(fs.add(SpaceKind::Euclidean3D, "field", "F"));
        CHECK(F && F->name() == "F");
        F->setField("F", "k*[-y, x, z]");
        fs.evaluate();
        CHECK(F->error().empty());
        CHECK(F->field("F")->program.usesZ());
        CHECK(F->dependencies().count("k") == 1);
        VectorFieldObject::Counts counts = F->counts();
        CHECK(counts.nx == 8 && counts.ny == 8 && counts.nz == 1);  // plane, 1 arrow per unit over -4..4
        CHECK(!F->followsClock());
        F->setProp("particles", "1");
        CHECK(F->followsClock());
        // z range only matters in Volume
        F->setField("zmin", "3");
        fs.evaluate();
        CHECK(F->error().empty());
        F->setProp("domain", "volume");
        fs.evaluate();
        CHECK(F->error().find("must be less than") != std::string::npos);
        F->setField("zmin", "-2");
        F->setProp("density", "100");  // capped
        fs.evaluate();
        CHECK(F->error().empty());
        counts = F->counts();
        CHECK(counts.capped && counts.nx == VectorFieldObject::kVolumeCap && counts.nz == VectorFieldObject::kVolumeCap);
        F->setField("F", "[1, 2]");  // not a vector of three? [1, 2] is invalid
        fs.evaluate();
        CHECK(!F->error().empty());
        F->setField("F", "x + y");  // a number, not a vector
        fs.evaluate();
        CHECK(F->error().find("should be a vector") != std::string::npos);
        F->setField("F", "[-y, x, z]");
        fs.evaluate();
        SceneData data;
        fs.save(data);
        Scene back;
        std::string err;
        CHECK(back.load(data, err));
        auto* G = dynamic_cast<VectorFieldObject*>(back.space(SpaceKind::Euclidean3D).find("F"));
        CHECK(G && G->volume() && G->particles());
        if (G) {
            back.evaluate();
            CHECK(G->error().empty() && G->field("F")->program.usesZ());
        }
    }

    // Parameter modes driven by the scene clock
    {
        Scene ps;
        auto* k = static_cast<ParameterObject*>(ps.add(SpaceKind::Euclidean3D, "parameter", "k"));
        auto* q = static_cast<ParameterObject*>(ps.add(SpaceKind::Euclidean3D, "parameter", "q"));
        // Formula of the scene time, using another (static) parameter
        q->setMode(ParameterObject::Mode::Formula);
        q->setField("formula", "2t + k");
        k->setNumber(1.0);
        ps.setTime(1.5);
        ps.evaluate();
        CHECK(q->error().empty());
        CHECK_NEAR(q->number(), 4.0, 1e-9);
        CHECK_NEAR(q->value().re, 4.0, 1e-9);
        ps.setTime(0.5);  // scrubbing back gives the earlier value
        ps.evaluate();
        CHECK_NEAR(q->number(), 2.0, 1e-9);
        // A slider is static: time does not change it, and its inactive formula is ignored
        k->setField("formula", "nonsense(");
        ps.setTime(3.0);
        ps.evaluate();
        CHECK(k->error().empty());
        CHECK_NEAR(k->number(), 1.0, 1e-12);
        // Animate: a pure function of time with easing, repeat and a start delay
        k->setMode(ParameterObject::Mode::Animate);
        k->setProp("min", 2.0);
        k->setProp("max", 4.0);
        k->setProp("anim.start", 1.0);
        Playback pb = k->playback();
        pb.duration = 2.0f;
        pb.repeat = Repeat::PingPong;
        pb.easing = Easing::EaseInOut;
        k->setPlayback(pb);
        CHECK_NEAR(k->animatedValue(0.5), 2.0, 1e-9);  // before its start
        CHECK_NEAR(k->animatedValue(2.0), 3.0, 1e-5);  // half way: ease-in-out(0.5) = 0.5
        CHECK_NEAR(k->animatedValue(3.0), 4.0, 1e-5);  // the end
        CHECK_NEAR(k->animatedValue(4.0), 3.0, 1e-5);  // half way back
        ps.setTime(2.0);
        ps.evaluate();
        CHECK_NEAR(k->value().re, 3.0, 1e-5);
        CHECK_NEAR(q->number(), 7.0, 1e-5);  // 2t + k follows k
        // Automatic length: start 1 s + ping-pong 2 x 2 s
        CHECK_NEAR(ps.autoLength(), 5.0, 1e-9);
        // The clock: loop wraps, otherwise it stops at the end
        ps.setTime(0.0);
        ps.timeline().playing = true;
        ps.advance(6.0);
        CHECK_NEAR(ps.timeline().time, 1.0, 1e-9);
        ps.timeline().loop = false;
        ps.advance(10.0);
        CHECK_NEAR(ps.timeline().time, 5.0, 1e-9);
        CHECK(!ps.timeline().playing);
        ps.timeline().length = 8.0;
        CHECK_NEAR(ps.timelineLength(), 8.0, 1e-12);
        // Back to a slider keeps the current value
        ps.setTime(2.0);
        ps.evaluate();
        k->setMode(ParameterObject::Mode::Slider);
        CHECK_NEAR(k->number(), 3.0, 1e-5);
        k->setMode(ParameterObject::Mode::Animate);
        // Undo snapshots leave the clock alone
        ps.commit();
        double tNow = ps.timeline().time;
        k->setProp("max", 9.0);
        ps.commit();
        ps.setTime(1.0);
        ps.undo();
        CHECK_NEAR(ps.timeline().time, 1.0, 1e-12);
        CHECK_NEAR(static_cast<ParameterObject*>(ps.space(SpaceKind::Euclidean3D).find("k"))->maximum(), 4.0, 1e-12);
        (void)tNow;
        // Round trip keeps modes, formula, style and the clock
        SceneData data;
        ps.timeline().playing = true;
        ps.save(data);
        Scene back;
        std::string err;
        CHECK(back.load(data, err));
        auto* q2 = static_cast<ParameterObject*>(back.space(SpaceKind::Euclidean3D).find("q"));
        auto* k2 = static_cast<ParameterObject*>(back.space(SpaceKind::Euclidean3D).find("k"));
        CHECK(q2 && q2->mode() == ParameterObject::Mode::Formula && q2->field("formula")->text == "2t + k");
        CHECK(k2 && k2->playback().easing == Easing::EaseInOut && k2->playback().repeat == Repeat::PingPong);
        CHECK(k2 && std::fabs(k2->startDelay() - 1.0) < 1e-9);
        CHECK(back.timeline().playing && std::fabs(back.timeline().length - 8.0) < 1e-6);
        // Old files: own clocks (speed + loop + playing), no mode and no timeline
        for (auto it = data.begin(); it != data.end();) {
            bool drop = it->first.find(".prop.mode") != std::string::npos ||
                        it->first.find(".prop.anim.") != std::string::npos || it->first.rfind("time.", 0) == 0;
            it = drop ? data.erase(it) : std::next(it);
        }
        std::string kp;
        for (auto& [key, v] : data)
            if (v == "k" && key.size() > 5 && key.compare(key.size() - 5, 5, ".name") == 0) kp = key.substr(0, key.size() - 4);
        CHECK(!kp.empty());
        data[kp + "prop.speed"] = "0.25";
        data[kp + "prop.loop"] = "0";
        data[kp + "prop.playing"] = "1";
        Scene old;
        CHECK(old.load(data, err));
        auto* k3 = static_cast<ParameterObject*>(old.space(SpaceKind::Euclidean3D).find("k"));
        CHECK(k3 && k3->mode() == ParameterObject::Mode::Animate);
        if (k3) {
            CHECK_NEAR(k3->playback().duration, 4.0f, 1e-4f);
            CHECK(k3->playback().repeat == Repeat::Once);
            CHECK(!k3->props().count("playing") && !k3->props().count("speed"));
        }
        CHECK(old.timeline().playing);
    }

    // Unique names and rename with reference rewriting
    SceneObject* v = sc.add(SpaceKind::Euclidean3D, "vector", "a");
    CHECK(v->name() == "a2");
    std::string err;
    CHECK(!d.rename(v->id(), "sin", err));
    CHECK(!d.rename(v->id(), "b", err));
    CHECK(d.rename(d.find("a")->id(), "p", err));
    CHECK(d.find("s")->field("value")->text == "p + b");
    CHECK(d.find("c")->field("value")->text == "cross(p, b)");
    sc.evaluate();
    CHECK(d.find("s")->error().empty());
    CHECK(renameInText("2a + ab + a.x + b_a", "a", "q") == "2q + ab + q.x + b_a");

    // Missing names and cycles
    d.remove(d.find("b")->id());
    sc.evaluate();
    CHECK(d.find("s")->error().find("'b'") != std::string::npos);
    SceneObject* u = sc.add(SpaceKind::Euclidean3D, "vector", "u");
    SceneObject* w = sc.add(SpaceKind::Euclidean3D, "vector", "w");
    static_cast<VectorObject*>(u)->setByFormula(true);
    static_cast<VectorObject*>(w)->setByFormula(true);
    u->setField("value", "w + [1, 0, 0]");
    w->setField("value", "2u");
    sc.evaluate();
    CHECK(u->error().find("circular") != std::string::npos);
    CHECK(w->error().find("circular") != std::string::npos);
    w->setField("value", "[0, 1, 0]");
    sc.evaluate();
    CHECK(u->error().empty() && w->error().empty());
    CHECK_NEAR(u->value().vector.x, 1.0f, 1e-6f);

    // Type checks and plots
    SceneObject* bad = sc.add(SpaceKind::Euclidean3D, "matrix", "N");
    static_cast<MatrixObject*>(bad)->setByFormula(true);
    bad->setField("value", "[1, 2, 3]");
    sc.evaluate();
    CHECK(bad->error().find("should be a matrix") != std::string::npos);
    SceneObject* f = sc.add(SpaceKind::Euclidean3D, "surface", "S");
    f->setField("f", "sin(x) * theta");
    sc.evaluate();
    CHECK(f->error().empty());
    f->setField("xmin", "10");
    sc.evaluate();
    CHECK(!f->error().empty());
    CHECK(d.find("S")->dependencies().count("theta"));

    // Save, load, undo
    SceneData data;
    sc.save(data);
    Scene copy;
    std::string loadError;
    CHECK(copy.load(data, loadError));
    CHECK(copy.space(SpaceKind::Euclidean3D).objects().size() == d.objects().size());
    CHECK(copy.space(SpaceKind::Euclidean3D).find("u") != nullptr);
    copy.evaluate();
    CHECK_NEAR(copy.space(SpaceKind::Euclidean3D).find("u")->value().vector.x, 1.0f, 1e-6f);

    Scene h;
    size_t n0 = h.space(SpaceKind::Euclidean3D).objects().size();
    h.add(SpaceKind::Euclidean3D, "vector", "k");
    h.commit();
    CHECK(h.space(SpaceKind::Euclidean3D).objects().size() == n0 + 1);
    h.undo();
    CHECK(h.space(SpaceKind::Euclidean3D).objects().size() == n0);
    h.redo();
    CHECK(h.space(SpaceKind::Euclidean3D).find("k") != nullptr);

    // Import of a v1 scene file
    SceneData v1;
    v1["matrix.use4x4"] = "0";
    v1["matrix3.00"] = "2";
    v1["vector_a.1"] = "5";
    v1["complex.z1.re"] = "3";
    v1["complex.z1.im"] = "-1";
    v1["plot.type"] = "2";
    v1["plot.surface"] = "x*y";
    Scene old;
    CHECK(old.load(v1, loadError));
    old.evaluate();
    const Space& od = old.space(SpaceKind::Euclidean3D);
    CHECK_NEAR(od.find("M")->value().matrix.m[0][0], 2.0f, 1e-6f);
    CHECK_NEAR(od.find("a")->value().vector.y, 5.0f, 1e-6f);
    bool hasSurface = false;
    for (const auto& o : od.objects())
        if (o->typeId() == "surface" && o->field("f")->text == "x*y") hasSurface = true;
    CHECK(hasSurface);
    const expr::Value& z1 = old.space(SpaceKind::Complex).find("z1")->value();
    CHECK_NEAR(z1.re, 3.0, 1e-6);
    CHECK_NEAR(z1.im, -1.0, 1e-6);

    // Complex plane defaults
    const Space& cp = sc.space(SpaceKind::Complex);
    const expr::Value& wv = cp.find("w")->value();
    CHECK_NEAR(wv.re, 0.5, 1e-6);   // (1 + i)(1 + 0.5i) = 0.5 + 1.5i
    CHECK_NEAR(wv.im, 1.5, 1e-6);
    CHECK(cp.find("u")->error().empty());
}

void testAllObjectTypes() {
    using namespace scene;
    // Every registered type: created in each space it allows, its defaults evaluate without errors, and
    // saving + loading (and cloning) keep its type, fields and properties
    for (const ObjectTypeInfo& info : objectTypes()) {
        for (SpaceKind kind : {SpaceKind::Euclidean3D, SpaceKind::Complex}) {
            if (!(info.spaces & spaceBit(kind))) continue;
            Scene sc;
            sc.clearAll();
            SceneObject* o = sc.add(kind, info.id, "");
            CHECK(o != nullptr);
            if (!o) continue;
            CHECK(o->typeId() == info.id);
            CHECK(o->name() == info.baseName);
            sc.evaluate();
            // Some defaults name other objects (eigenvectors of M, z1·z2): only that may fail in an empty scene
            auto fine = [](const std::string& e) { return e.empty() || e.find("unknown name") != std::string::npos; };
            if (!fine(o->error())) fprintf(stderr, "  %s: default has an error: %s\n", info.id, o->error().c_str());
            CHECK(fine(o->error()));
            std::unique_ptr<SceneObject> copy = o->clone();
            CHECK(copy && copy->typeId() == info.id && copy->fields().size() == o->fields().size());
            SceneData data;
            sc.save(data);
            Scene back;
            std::string err;
            CHECK(back.load(data, err));
            SceneObject* b = back.space(kind).find(info.baseName);
            CHECK(b != nullptr);
            if (!b) continue;
            CHECK(b->typeId() == info.id);
            CHECK(b->props() == o->props());
            for (const Field& f : o->fields()) {
                const Field* g = b->field(f.key);
                CHECK(g && g->text == f.text);
            }
            back.evaluate();
            CHECK(fine(b->error()));
        }
    }
}

void testUsability() {
    using namespace scene;
    Scene sc;
    Space& d = sc.space(SpaceKind::Euclidean3D);
    auto* m = static_cast<MatrixObject*>(d.find("M"));
    sc.evaluate();
    // Presets add up: rotate 45 twice = 90; then scale acts on the turned matrix
    m->applyPreset(Matrix3D::rotation(3.14159265f / 4));
    sc.evaluate();
    m->applyPreset(Matrix3D::rotation(3.14159265f / 4));
    sc.evaluate();
    Matrix3D r = m->value().matrix3();
    CHECK_NEAR(r.m[0][0], 0.0f, 1e-5f);
    CHECK_NEAR(r.m[1][0], 1.0f, 1e-5f);
    m->applyPreset(Matrix3D::scale(2, 2));
    sc.evaluate();
    r = m->value().matrix3();
    CHECK_NEAR(r.m[1][0], 2.0f, 1e-5f);  // scale after rotate
    CHECK_NEAR(r.m[2][2], 1.0f, 1e-6f);
    // Identity starts over
    m->applyPreset(Matrix3D::identity());
    sc.evaluate();
    CHECK_NEAR(m->value().matrix3().m[0][1], 0.0f, 1e-6f);
    CHECK_NEAR(m->value().matrix3().m[1][1], 1.0f, 1e-6f);
    // A formula matrix becomes cells holding the composed value
    m->setByFormula(true);
    m->setField("value", "scale(3)");
    sc.evaluate();
    m->applyPreset(Matrix3D::scale(-1, 1));
    sc.evaluate();
    CHECK(!m->byFormula());
    CHECK_NEAR(m->value().matrix3().m[0][0], -3.0f, 1e-6f);
    CHECK_NEAR(m->value().matrix3().m[1][1], 3.0f, 1e-6f);
    // 4x4 matrices keep their translation
    m->setByFormula(true);
    m->setField("value", "translate(1, 2, 3)");
    sc.evaluate();
    m->applyPreset(Matrix3D::rotation(3.14159265f / 2));
    sc.evaluate();
    CHECK(m->value().size == 4);
    CHECK_NEAR(m->value().matrix.m[0][3], -2.0f, 1e-5f);  // the rotation turns the translation too
    CHECK_NEAR(m->value().matrix.m[2][3], 3.0f, 1e-5f);

    // Empty start, clean history; the demo scene is one undo step
    sc.clearAll();
    sc.clearHistory();
    CHECK(d.objects().empty() && !sc.canUndo());
    sc.loadDefaultScene();
    CHECK(d.find("M") && sc.canUndo());
    sc.undo();
    CHECK(d.objects().empty());
    // New vectors have length 1
    auto* v = sc.add(SpaceKind::Euclidean3D, "vector", "v");
    sc.evaluate();
    CHECK_NEAR(v->value().vector.magnitude(), 1.0f, 1e-6f);
}

void testTemplates() {
    using scene::SpaceKind;
    scene::Scene sc;
    sc.clearAll();
    // Two selected vectors offer their sum, which applyAddChoice creates as a formula and selects
    scene::SceneObject* a = sc.add(SpaceKind::Euclidean3D, "vector", "a");
    scene::SceneObject* b = sc.add(SpaceKind::Euclidean3D, "vector", "b");
    a->setField("x", "1");
    b->setField("y", "2");
    sc.selection(SpaceKind::Euclidean3D).set(a->id());
    sc.selection(SpaceKind::Euclidean3D).toggle(b->id());
    auto tmpl = addTemplates(sc, SpaceKind::Euclidean3D);
    const AddChoice* sum = nullptr;
    const AddChoice* blend = nullptr;
    for (const auto& [label, choice] : tmpl) {
        if (label == "Sum a + b") sum = &choice;
        if (label.rfind("Blend a", 0) == 0) blend = &choice;
    }
    CHECK(sum != nullptr);
    CHECK(blend != nullptr);
    if (!sum || !blend) return;
    scene::SceneObject* s = applyAddChoice(sc, SpaceKind::Euclidean3D, *sum);
    CHECK(s != nullptr);
    sc.evaluate();
    CHECK(s && s->value().type == expr::Type::Vector);
    if (s) CHECK_NEAR(s->value().vector.y, 2.0f, 1e-6f);
    CHECK(s && sc.selection(SpaceKind::Euclidean3D).primary() == s->id());
    // A slider template makes its helper parameter first and puts its name in the formula
    AddChoice blendCopy = *blend;
    scene::SceneObject* m = applyAddChoice(sc, SpaceKind::Euclidean3D, blendCopy);
    CHECK(m != nullptr);
    CHECK(sc.space(SpaceKind::Euclidean3D).find("s") != nullptr);
    if (m) CHECK(m->field("value")->text.find("$s") == std::string::npos);
    // Nothing selected: no templates
    sc.selection(SpaceKind::Euclidean3D).clear();
    CHECK(addTemplates(sc, SpaceKind::Euclidean3D).empty());

    // Every "Next example" formula works in its object type
    scene::Scene ex;
    ex.clearAll();
    auto works = [&ex](SpaceKind kind, const char* type, std::map<std::string, std::string> fields) {
        scene::SceneObject* o = ex.add(kind, type, "");
        if (!o) return false;
        for (const auto& [k, v] : fields) o->setField(k, v);
        ex.evaluate();
        return o->error().empty();
    };
    for (const char* type : {"function", "surface", "implicit", "curve"}) {
        CHECK(plotExamples(type).size() >= 4);
        for (const PlotExample& e : plotExamples(type)) {
            bool curve = std::string(type) == "curve";
            CHECK(curve ? works(SpaceKind::Euclidean3D, type, {{"x", e.x}, {"y", e.y}, {"z", e.z}})
                        : works(SpaceKind::Euclidean3D, type, {{"f", e.f}}));
        }
    }
    for (const char* z : epicycleExamples()) CHECK(works(SpaceKind::Complex, "epicycles", {{"z", z}}));
    for (const char* f : fourierExamples()) CHECK(works(SpaceKind::Euclidean3D, "fourierseries", {{"f", f}}));
    for (const char* q : quaternionExamples()) CHECK(works(SpaceKind::Euclidean3D, "quaternion", {{"value", q}}));
    for (const FieldExample& e : fieldExamples()) CHECK(works(SpaceKind::Euclidean3D, "field", {{"F", e.f}}));

    // The Taylor polynomial's text: the first terms, then "…"
    taylor::Series sine;
    sine.coeffs = {0.0, 1.0, 0.0, -1.0 / 6.0, 0.0, 1.0 / 120.0, 0.0, -1.0 / 5040.0};
    CHECK(scene::polynomialText(sine, 2) == "x − 0.1667x³ + …");
    taylor::Series shifted;
    shifted.center = 1.0;
    shifted.coeffs = {2.0, -1.0};
    CHECK(scene::polynomialText(shifted, 4) == "2 − (x − 1)");
    CHECK(scene::polynomialText(taylor::Series{}, 4) == "0");
}

void testAnalysis() {
    using scene::SpaceKind;
    scene::Scene sc;
    sc.clearAll();
    auto* a = sc.add(SpaceKind::Euclidean3D, "vector", "a");
    auto* b = sc.add(SpaceKind::Euclidean3D, "vector", "b");
    auto* m = sc.add(SpaceKind::Euclidean3D, "matrix", "M");
    CHECK(a && b && m);
    if (!a || !b || !m) return;
    b->setField("x", "0");
    b->setField("y", "1");
    m->setField(scene::MatrixObject::cellKey(0, 0), "2");
    sc.evaluate();
    scene::Selection& sel = sc.selection(SpaceKind::Euclidean3D);
    auto has = [](const scene::Analysis& an, const std::string& key, const std::string& text) {
        for (const auto& s : an.sections)
            if (s.key == key)
                for (const auto& l : s.lines)
                    if (l.text == text) return true;
        return false;
    };
    // Nothing selected: one section that says so
    sel.clear();
    scene::Analysis none = scene::analyze(sc.space(SpaceKind::Euclidean3D), sel);
    CHECK(none.sections.size() == 1 && none.sections[0].key == "empty" && !none.hasMatrix);
    // Two vectors: products and the angle
    sel.set(a->id());
    sel.toggle(b->id());
    scene::Analysis two = scene::analyze(sc.space(SpaceKind::Euclidean3D), sel);
    CHECK(has(two, "vectors", "a · b = 0.000"));
    CHECK(has(two, "vectors", "angle(a, b) = 90.0°"));
    CHECK(has(two, "vectors", "a × b = (0.00, 0.00, 1.00)"));
    // A matrix: determinant, eigenvalues, factorizations closed at first
    sel.set(m->id());
    scene::Analysis mat = scene::analyze(sc.space(SpaceKind::Euclidean3D), sel);
    CHECK(mat.hasMatrix);
    CHECK(has(mat, "matrix", "det M = 2.0000"));
    CHECK(mat.eigen.values.size() == 3);
    bool luClosed = false;
    for (const auto& s : mat.sections)
        if (s.key == "lu") luClosed = !s.expanded;
    CHECK(luClosed);

    // The formula editor's live result
    bool ok = false;
    const scene::Field* value = a->field("value");
    std::string shown = scene::fieldPreview(sc.space(SpaceKind::Euclidean3D), *a, *value, "b + [1, 0, 0]", ok);
    CHECK(ok && shown.rfind("= ", 0) == 0 && shown.find("vector") != std::string::npos);
    scene::fieldPreview(sc.space(SpaceKind::Euclidean3D), *a, *value, "nothing + 1", ok);
    CHECK(!ok);
    scene::fieldPreview(sc.space(SpaceKind::Euclidean3D), *a, *value, "(1 +", ok);
    CHECK(!ok);
}

