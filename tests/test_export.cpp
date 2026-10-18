// Tests: The LaTeX export and PDF compilation
#include "TestSupport.hpp"

void testLatexExport() {
    using namespace scene;
    CHECK(latex::nameToLatex("theta") == "\\theta");
    CHECK(latex::nameToLatex("z1") == "z_{1}");
    CHECK(latex::nameToLatex("Ma") == "\\mathit{Ma}");
    CHECK(latex::nameToLatex("a_b") == "a_{b}");
    CHECK(latex::nameToLatex("v12") == "v_{12}");
    CHECK(latex::nameToLatex("alpha2") == "\\alpha_{2}");

    double az = 0, el = 0;
    latex::viewFromDirection(Vector3D(0, -1, 0), az, el);  // viewer in front (negative y)
    CHECK_NEAR(az, 0.0, 1e-9);
    CHECK_NEAR(el, 0.0, 1e-9);
    latex::viewFromDirection(Vector3D(0, 0, 1), az, el);   // from above
    CHECK_NEAR(el, 90.0, 1e-9);
    latex::viewFromDirection(Vector3D(1, 0, 0), az, el);   // from +x
    CHECK_NEAR(az, 90.0, 1e-9);

    CHECK(std::string(latex::styleFor(0).dash) == "solid");
    CHECK(std::string(latex::styleFor(1).dash) == "dashed");
    CHECK(std::string(latex::styleFor(5).gray) == "black!65");

    const char* outDir = std::getenv("LATEX_OUT");
    auto save = [&](const char* name, const std::string& code) {
        if (!outDir) return;
        std::ofstream(std::string(outDir) + "/" + name + ".tex") << code;
    };
    auto noColors = [](const std::string& code) {
        for (const char* c : {"red", "blue", "green", "yellow", "orange", "cyan", "magenta"})
            if (code.find(c) != std::string::npos) return false;
        return true;
    };

    // Vector fields: quiver tables; a plane field is a 2D figure, a volume field a 3D one
    {
        Scene fsc;
        fsc.clearAll();
        auto* F = static_cast<VectorFieldObject*>(fsc.add(SpaceKind::Euclidean3D, "field", "F"));
        F->setField("F", "[-y - x/4, x - y/4, 0]");
        F->setProp("lines", true);
        F->setProp("particles", true);
        fsc.evaluate();
        latex::Options fo;
        fo.standalone = true;
        latex::Result fr = latex::exportLatex(fsc.space(SpaceKind::Euclidean3D), fsc.selection(SpaceKind::Euclidean3D), fo);
        CHECK(fr.exported == 1 && fr.flat);
        CHECK(fr.code.find("quiver={u=\\thisrow{u}, v=\\thisrow{v}}") != std::string::npos);
        CHECK(fr.code.find("table[row sep=crcr] {x y u v\\\\") != std::string::npos);
        CHECK(fr.code.find("forget plot") != std::string::npos);  // flow lines
        CHECK(fr.code.find("only marks") != std::string::npos);   // particles
        CHECK(fr.code.find("axis equal image") != std::string::npos);
        CHECK(noColors(fr.code));
        save("field-plane", fr.code);
        F->setProp("domain", "volume");
        F->setField("F", "[-y, x, 0.4]");
        F->setProp("particles", false);
        fsc.evaluate();
        fr = latex::exportLatex(fsc.space(SpaceKind::Euclidean3D), fsc.selection(SpaceKind::Euclidean3D), fo);
        CHECK(!fr.flat);
        CHECK(fr.code.find("\\addplot3[") != std::string::npos);
        CHECK(fr.code.find("w=\\thisrow{w}") != std::string::npos);
        CHECK(fr.code.find("only marks") == std::string::npos);
        save("field-volume", fr.code);
    }

    // Quaternions: a 3D figure with the axis, the arc and the moved basis; a turn about z with no basis
    // is flat
    {
        Scene qsc;
        qsc.clearAll();
        auto* q = static_cast<QuaternionObject*>(qsc.add(SpaceKind::Euclidean3D, "quaternion", "q"));
        q->setField("value", "qaxis([1, 1, 1], 2pi/3)");
        q->setProp("act", true);
        q->setField("v", "[1.5, 0, 0.5]");
        qsc.evaluate();
        latex::Options qo;
        qo.standalone = true;
        latex::Result qr = latex::exportLatex(qsc.space(SpaceKind::Euclidean3D), qsc.selection(SpaceKind::Euclidean3D), qo);
        CHECK(qr.exported == 1 && !qr.flat);
        CHECK(qr.code.find("q\\,e_{3}") != std::string::npos);      // moved basis
        CHECK(qr.code.find("densely dashed") != std::string::npos);    // the axis
        CHECK(qr.code.find("\\vec v") != std::string::npos);         // acting on a vector
        CHECK(noColors(qr.code));
        save("quaternion-3d", qr.code);
        q->setField("value", "qaxis([0, 0, 1], pi/2)");
        q->setProp("basis", false);
        q->setField("v", "[1.5, 0, 0]");
        qsc.evaluate();
        qr = latex::exportLatex(qsc.space(SpaceKind::Euclidean3D), qsc.selection(SpaceKind::Euclidean3D), qo);
        CHECK(qr.flat);
        save("quaternion-flat", qr.code);
    }

    // Fourier: the series is a 2D figure with f and the sum; epicycles draw circles in the complex plane
    {
        Scene fsc2;
        fsc2.clearAll();
        auto* S = fsc2.add(SpaceKind::Euclidean3D, "fourierseries", "S");
        S->setField("f", "sign(x)");
        S->setField("N", "9");
        auto* E = fsc2.add(SpaceKind::Complex, "epicycles", "E");
        fsc2.timeline().time = 4.0;
        fsc2.evaluate();
        (void)E;
        latex::Options fo2;
        fo2.standalone = true;
        latex::Result r1 = latex::exportLatex(fsc2.space(SpaceKind::Euclidean3D), fsc2.selection(SpaceKind::Euclidean3D), fo2);
        CHECK(r1.exported == 1 && r1.flat && r1.code.find("densely dashed") != std::string::npos);
        CHECK(r1.code.find("S_{9}") != std::string::npos);
        save("fourier-series", r1.code);
        latex::Result r2 = latex::exportLatex(fsc2.space(SpaceKind::Complex), fsc2.selection(SpaceKind::Complex), fo2);
        CHECK(r2.exported == 1 && r2.code.find("circle[radius=") != std::string::npos);
        save("fourier-epicycles", r2.code);
    }

    // Moving frame: T, N, B and the osculating circle; a flat curve is a 2D figure (no B)
    {
        Scene gsc;
        gsc.clearAll();
        auto* P = gsc.add(SpaceKind::Euclidean3D, "frame", "P");
        gsc.evaluate();
        latex::Options go;
        go.standalone = true;
        latex::Result gr = latex::exportLatex(gsc.space(SpaceKind::Euclidean3D), gsc.selection(SpaceKind::Euclidean3D), go);
        CHECK(gr.exported == 1 && !gr.flat && gr.code.find("{$B$}") != std::string::npos);
        save("frame-helix", gr.code);
        P->setField("x", "3cos(u)");
        P->setField("y", "1.5sin(u)");
        P->setField("z", "0");
        P->setField("umax", "2pi");
        P->setField("u0", "0.5");
        gsc.evaluate();
        gr = latex::exportLatex(gsc.space(SpaceKind::Euclidean3D), gsc.selection(SpaceKind::Euclidean3D), go);
        CHECK(gr.flat && gr.code.find("{$N$}") != std::string::npos && gr.code.find("{$B$}") == std::string::npos);
        save("frame-ellipse", gr.code);
    }

    // Derivatives: the tangent is a 2D figure, the tangent plane 3D, velocity on a flat curve 2D
    {
        Scene dsc;
        dsc.clearAll();
        auto* T = dsc.add(SpaceKind::Euclidean3D, "tangent", "T");
        T->setField("h", "1");
        dsc.evaluate();
        latex::Options dop;
        dop.standalone = true;
        latex::Result dr = latex::exportLatex(dsc.space(SpaceKind::Euclidean3D), dsc.selection(SpaceKind::Euclidean3D), dop);
        CHECK(dr.exported == 1 && dr.flat && dr.code.find("f'(1) = ") != std::string::npos);
        CHECK(dr.code.find("densely dashed") != std::string::npos);  // the graph of f'
        save("tangent", dr.code);
        dsc.space(SpaceKind::Euclidean3D).remove(T->id());
        dsc.add(SpaceKind::Euclidean3D, "tangentplane", "G");
        dsc.evaluate();
        dr = latex::exportLatex(dsc.space(SpaceKind::Euclidean3D), dsc.selection(SpaceKind::Euclidean3D), dop);
        CHECK(dr.exported == 1 && !dr.flat && dr.code.find("\\nabla f") != std::string::npos);
        CHECK(dr.code.find("fill opacity") != std::string::npos);
        save("tangent-plane", dr.code);
        dsc.clearAll();
        auto* R = dsc.add(SpaceKind::Euclidean3D, "velocity", "R");
        R->setProp("split", true);
        dsc.evaluate();
        dr = latex::exportLatex(dsc.space(SpaceKind::Euclidean3D), dsc.selection(SpaceKind::Euclidean3D), dop);
        CHECK(dr.exported == 1 && dr.flat && dr.code.find("{$r''$}") != std::string::npos);
        save("velocity", dr.code);
    }

    // Taylor and Newton: 2D figures with the polynomial in the legend and the iterates labeled
    {
        Scene tsc;
        tsc.clearAll();
        tsc.add(SpaceKind::Euclidean3D, "taylor", "P");
        tsc.evaluate();
        latex::Options top;
        top.standalone = true;
        latex::Result tr = latex::exportLatex(tsc.space(SpaceKind::Euclidean3D), tsc.selection(SpaceKind::Euclidean3D), top);
        CHECK(tr.exported == 1 && tr.flat && tr.code.find("T_{3}") != std::string::npos);
        save("taylor", tr.code);
        tsc.clearAll();
        tsc.add(SpaceKind::Euclidean3D, "newton", "r");
        tsc.evaluate();
        tr = latex::exportLatex(tsc.space(SpaceKind::Euclidean3D), tsc.selection(SpaceKind::Euclidean3D), top);
        CHECK(tr.exported == 1 && tr.flat && tr.code.find("x_{0}") != std::string::npos);
        CHECK(tr.code.find("\\approx 1.414") != std::string::npos);
        save("newton", tr.code);
    }

    // Integrals: boxes and the value; a flat work integral is a 2D figure
    {
        Scene isc;
        isc.clearAll();
        auto* V = isc.add(SpaceKind::Euclidean3D, "doubleintegral", "V");
        V->setField("n", "4");
        isc.evaluate();
        latex::Options io;
        io.standalone = true;
        latex::Result ir = latex::exportLatex(isc.space(SpaceKind::Euclidean3D), isc.selection(SpaceKind::Euclidean3D), io);
        CHECK(ir.exported == 1 && !ir.flat);
        CHECK(ir.code.find("\\fill[black!10]") != std::string::npos);
        CHECK(ir.code.find("V \\approx") != std::string::npos);
        save("integral-volume", ir.code);
        isc.clearAll();
        isc.add(SpaceKind::Euclidean3D, "lineintegral", "W");
        isc.evaluate();
        ir = latex::exportLatex(isc.space(SpaceKind::Euclidean3D), isc.selection(SpaceKind::Euclidean3D), io);
        CHECK(ir.flat && ir.code.find("W = 25.1") != std::string::npos);
        save("integral-work", ir.code);
    }

    // Default 3D scene: vectors, matrix, eigenvectors -> 3D axis with arrows and labels
    Scene sc;
    sc.evaluate();
    latex::Options opt;
    latex::Result r = latex::exportLatex(sc.space(SpaceKind::Euclidean3D), sc.selection(SpaceKind::Euclidean3D), opt);
    CHECK(!r.flat);
    CHECK(r.exported >= 7);
    CHECK(r.code.find("\\begin{tikzpicture}") != std::string::npos);
    CHECK(r.code.find("view={25}{30}") != std::string::npos);
    CHECK(r.code.find("\\vec{a}") != std::string::npos);
    CHECK(r.code.find("\\lambda") != std::string::npos);
    CHECK(r.code.find("\\usepackage{pgfplots}") != std::string::npos);  // preamble in a comment
    CHECK(r.code.find("\\documentclass") == std::string::npos);
    CHECK(noColors(r.code));
    opt.standalone = true;
    save("scene3d", latex::exportLatex(sc.space(SpaceKind::Euclidean3D), sc.selection(SpaceKind::Euclidean3D), opt).code);

    // Only flat objects -> 2D axis; selection scope
    Scene flat;
    Space& d = flat.space(SpaceKind::Euclidean3D);
    d.clear();
    auto* fn = flat.add(SpaceKind::Euclidean3D, "function", "f");
    fn->setField("f", "sin(x)");
    auto* im = flat.add(SpaceKind::Euclidean3D, "implicit", "g");
    im->setField("f", "x^2/9 + y^2/4 - 1");
    auto* ve = static_cast<VectorObject*>(flat.add(SpaceKind::Euclidean3D, "vector", "v"));
    ve->setField("x", "2");
    ve->setField("y", "1");
    flat.add(SpaceKind::Euclidean3D, "parameter", "k");
    flat.evaluate();
    r = latex::exportLatex(d, flat.selection(SpaceKind::Euclidean3D), latex::Options{});
    CHECK(r.flat);
    CHECK(r.exported == 3);
    CHECK(r.skipped.size() == 1);  // the parameter
    CHECK(r.code.find("\\addlegendentry{$y = f(x)$}") != std::string::npos);
    CHECK(r.code.find("view=") == std::string::npos);
    CHECK(r.code.find("solid") != std::string::npos && r.code.find("dashed") != std::string::npos);
    latex::Options sopt;
    sopt.standalone = true;
    sopt.grid = true;
    save("flat", latex::exportLatex(d, flat.selection(SpaceKind::Euclidean3D), sopt).code);
    flat.selection(SpaceKind::Euclidean3D).set(ve->id());
    latex::Options selOpt;
    selOpt.scope = latex::Scope::Selection;
    r = latex::exportLatex(d, flat.selection(SpaceKind::Euclidean3D), selOpt);
    CHECK(r.exported == 1);
    CHECK(latex::styleIndexOf(d, flat.selection(SpaceKind::Euclidean3D), latex::Scope::Visible, ve->id()) == 2);

    // Surface: mesh rows match the samples
    Scene surf;
    surf.space(SpaceKind::Euclidean3D).clear();
    auto* sf = surf.add(SpaceKind::Euclidean3D, "surface", "S");
    sf->setField("f", "sin(sqrt(x^2 + y^2))");
    surf.evaluate();
    latex::Options so;
    so.surfaceSamples = 20;
    r = latex::exportLatex(surf.space(SpaceKind::Euclidean3D), surf.selection(SpaceKind::Euclidean3D), so);
    CHECK(!r.flat);
    CHECK(r.code.find("mesh/rows=21") != std::string::npos);
    CHECK(r.code.find("faceted color=black") != std::string::npos);
    so.standalone = true;
    save("surface", latex::exportLatex(surf.space(SpaceKind::Euclidean3D), surf.selection(SpaceKind::Euclidean3D), so).code);
    so.grayShading = true;
    save("surface_gray", latex::exportLatex(surf.space(SpaceKind::Euclidean3D), surf.selection(SpaceKind::Euclidean3D), so).code);

    // Complex plane: always 2D, unit circle, labels as math
    r = latex::exportLatex(sc.space(SpaceKind::Complex), sc.selection(SpaceKind::Complex), latex::Options{});
    CHECK(r.flat);
    CHECK(r.code.find("z_{1}") != std::string::npos);
    CHECK(r.code.find("unit circle") != std::string::npos);
    latex::Options co;
    co.standalone = true;
    save("complex", latex::exportLatex(sc.space(SpaceKind::Complex), sc.selection(SpaceKind::Complex), co).code);

    // A curve in 3D
    Scene cv;
    cv.space(SpaceKind::Euclidean3D).clear();
    cv.add(SpaceKind::Euclidean3D, "curve", "c");
    cv.evaluate();
    latex::Options cvo;
    cvo.standalone = true;
    save("curve", latex::exportLatex(cv.space(SpaceKind::Euclidean3D), cv.selection(SpaceKind::Euclidean3D), cvo).code);
}

void testPdfCompile() {
    // Error lines of a LaTeX log: "!" lines and where they happened
    std::string log = "This is pdfTeX\n(./figure.tex\n! Undefined control sequence.\nl.12 \\foo\n  \n"
                      "! Emergency stop.\n<*> figure.tex\n";
    std::string errors = latex::firstLatexErrors(log);
    CHECK(errors.find("! Undefined control sequence.") == 0);
    CHECK(errors.find("l.12 \\foo") != std::string::npos);
    CHECK(errors.find("! Emergency stop.") != std::string::npos);
    CHECK(latex::firstLatexErrors(log, 1) == "! Undefined control sequence.");
    CHECK(latex::firstLatexErrors("all fine\n").empty());

    // Tools in PATH, no shell
    CHECK(!latex::findTool("sh").empty());
    CHECK(latex::findTool("surely-not-a-command-axismundi").empty());
    latex::PdfResult none = latex::compilePdf("x", "/tmp/never.pdf", 5, "");
    CHECK(!none.ok);
    CHECK(none.message.find("No LaTeX compiler") != std::string::npos);

    // A real compile (slow): PDF_TEST=1 make test, with pdflatex installed
    const char* run = std::getenv("PDF_TEST");
    if (!run || std::string(run) != "1" || latex::latexCompiler().empty()) return;
    std::string out = (std::filesystem::temp_directory_path() / "axismundi-test figure.pdf").string();
    latex::PdfResult ok = latex::compilePdf(
        "\\documentclass{standalone}\n\\usepackage{tikz}\n\\begin{document}\n"
        "\\tikz \\draw (0,0) -- (1,1);\n\\end{document}\n", out);
    CHECK(ok.ok);
    std::ifstream pdf(out, std::ios::binary);
    char head[5] = {};
    pdf.read(head, 4);
    CHECK(std::string(head) == "%PDF");
    std::filesystem::remove(out);
    latex::PdfResult bad = latex::compilePdf("\\documentclass{standalone}\n\\begin{document}\\nosuchmacro\n"
                                             "\\end{document}\n", out);
    CHECK(!bad.ok);
    CHECK(bad.message.find("!") == 0);
    CHECK(!std::filesystem::exists(out));
    std::printf("  (compiled a PDF with %s)\n", latex::latexCompiler().c_str());
}
