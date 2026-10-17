#include "ui/InfoPanel.hpp"
#include "math/expr/Value.hpp"
#include "ui/Widgets.hpp"
#include "ui/core/Layout.hpp"
#include "ui/core/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace linalg {

namespace {

std::string fmt(const char* format, ...) __attribute__((format(printf, 1, 2)));

std::string fmt(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return buf;
}

float clean(float v) { return std::fabs(v) < 5e-5f ? 0.0f : v; }

std::string vecStr(const Vector3D& v) {
    return fmt("(%.2f, %.2f, %.2f)", clean(v.x), clean(v.y), clean(v.z));
}

const char* subscript(int i) {
    static const char* digits[] = {"₀", "₁", "₂", "₃", "₄"};
    return digits[std::clamp(i, 0, 4)];
}

} // namespace

InfoPanel::InfoPanel() : m_eigen{} {}

void InfoPanel::addMatrix(Section& s, const char* name, const float* values, int n) {
    s.lines.push_back({std::string(name) + " =", Tone::Muted, ui::FontRole::Body});
    for (int i = 0; i < n; i++) {
        std::string row = "[";
        for (int j = 0; j < n; j++) row += fmt("%8.3f", clean(values[i * n + j]));
        row += " ]";
        s.lines.push_back({row, Tone::Normal, ui::FontRole::Mono});
    }
}

void InfoPanel::update(const scene::Space& space, const scene::Selection& selection) {
    // Gather what is selected, in selection order
    std::vector<const scene::SceneObject*> picked;
    for (scene::ObjectId id : selection.ids)
        if (const scene::SceneObject* o = space.find(id)) picked.push_back(o);

    const scene::SceneObject* matrixObj = nullptr;
    std::vector<const scene::SceneObject*> vectors, numbers;
    for (const auto* o : picked) {
        if (!o->value().ok()) continue;
        if (o->value().type == expr::Type::Matrix) {
            matrixObj = o;  // the most recently selected matrix wins
        } else if (o->value().type == expr::Type::Vector && vectors.size() < 2) vectors.push_back(o);
        else if (o->value().isNumber() && o->typeId() != "parameter" && numbers.size() < 2) numbers.push_back(o);
    }

    // Rebuild sections, keeping their expanded state by key
    std::vector<Section> previous = std::move(m_sections);
    m_sections.clear();
    auto section = [&](const std::string& key, const std::string& title, bool def) -> Section& {
        bool expanded = def;
        for (const auto& p : previous)
            if (p.key == key) expanded = p.expanded;
        m_sections.push_back({key, title, expanded, {}});
        return m_sections.back();
    };
    auto body = [](Section& s, const std::string& text, Tone tone = Tone::Normal, const char* tip = nullptr) {
        s.lines.push_back({text, tone, ui::FontRole::Body, tip});
    };
    auto tipped = [&](Section& s, const std::string& text, const char* tip) { body(s, text, Tone::Normal, tip); };
    auto note = [](Section& s, const std::string& text) {
        s.lines.push_back({text, Tone::Muted, ui::FontRole::Small});
    };
    m_eigen = EigenResult{};
    m_hasMatrix = false;

    if (!matrixObj && vectors.empty() && numbers.empty()) {
        Section& s = section("empty", "Nothing to analyse", true);
        note(s, "Select a matrix, one or two vectors, or complex numbers (Ctrl+click for several).");
        return;
    }

    // --- Vectors -----------------------------------------------------------
    if (vectors.size() == 1) {
        const std::string& n = vectors[0]->name();
        Vector3D a = vectors[0]->value().vector;
        Section& s = section("vector", "Vector " + n, true);
        s.tip = "Length and direction of the selected vector. Select a second one for products and angles.";
        body(s, n + " = " + vecStr(a));
        tipped(s, fmt("|%s| = %.3f", n.c_str(), a.magnitude()), "Length of a vector: |a| = √(a·a).");
        if (a.magnitude() > 1e-6f) {
            tipped(s, "unit vector = " + vecStr(a.normalized()), "Same direction, length 1: a / |a|.");
        }
    } else if (vectors.size() == 2) {
        const std::string& na = vectors[0]->name();
        const std::string& nb = vectors[1]->name();
        Vector3D a = vectors[0]->value().vector, b = vectors[1]->value().vector;
        const char* A = na.c_str();
        const char* B = nb.c_str();
        Section& s = section("vectors", "Vectors " + na + ", " + nb, true);
        s.tip = "Products and angles of the two selected vectors.";
        float dot = a.dot(b);
        float la = a.magnitude(), lb = b.magnitude();
        Vector3D cross = a.cross(b);
        tipped(s, fmt("%s · %s = %.3f", A, B, clean(dot)),
               "Dot product: a·b = |a||b| cos θ = axbx + ayby + azbz. Zero means a and b are perpendicular.");
        tipped(s, fmt("|%s| = %.3f     |%s| = %.3f", A, la, B, lb), "Length of a vector: |a| = √(a·a).");
        if (la > 1e-6f && lb > 1e-6f) {
            float c = std::clamp(dot / (la * lb), -1.0f, 1.0f);
            tipped(s, fmt("angle(%s, %s) = %.1f°", A, B, std::acos(c) * RAD2DEG),
                   "θ = arccos(a·b / (|a||b|)), the angle between the two arrows.");
            if (std::fabs(dot) < 1e-5f) note(s, na + " and " + nb + " are perpendicular");
        }
        tipped(s, na + " × " + nb + " = " + vecStr(cross),
               "Cross product: perpendicular to both (right-hand rule). Its length is the area of the "
               "parallelogram they span.");
        note(s, fmt("|%s × %s| = %.3f, the area of the parallelogram", A, B, cross.magnitude()));
    }

    // --- Complex numbers ---------------------------------------------------
    for (size_t k = 0; k < numbers.size(); k++) {
        const scene::SceneObject* z = numbers[k];
        const expr::Value& v = z->value();
        Section& s = section("complex." + std::to_string(k), z->name(), true);
        s.tip = "Rectangular and polar form of the selected complex number.";
        body(s, z->name() + " = " + v.summary());
        tipped(s, fmt("|%s| = %.3f", z->name().c_str(), std::hypot(v.re, v.im)), "Length: √(re² + im²).");
        tipped(s, fmt("arg %s = %.1f°", z->name().c_str(), clean(static_cast<float>(std::atan2(v.im, v.re) * RAD2DEG))),
               "Angle from the positive real axis.");
        body(s, "conj = " + expr::Value::complex(v.re, -v.im).summary(), Tone::Muted);
    }
    if (numbers.size() == 2) {
        const expr::Value& a = numbers[0]->value();
        const expr::Value& b = numbers[1]->value();
        Section& s = section("complex.pair", numbers[0]->name() + " and " + numbers[1]->name(), true);
        double pr = a.re * b.re - a.im * b.im, pi = a.re * b.im + a.im * b.re;
        double d = b.re * b.re + b.im * b.im;
        body(s, "product = " + expr::Value::complex(pr, pi).summary());
        if (d > 1e-12) body(s, "quotient = " + expr::Value::complex((a.re * b.re + a.im * b.im) / d, (a.im * b.re - a.re * b.im) / d).summary());
        note(s, "Multiplying multiplies lengths and adds angles.");
    }

    if (!matrixObj) return;

    // --- Matrix properties -------------------------------------------------
    const std::string mname = matrixObj->name();
    const char* M = mname.c_str();
    Matrix4D matrix = matrixObj->value().matrix4();
    bool is4x4 = matrixObj->value().size == 4;
    Matrix3D linear = matrixObj->value().matrix3();
    m_hasMatrix = true;

    {
        Section& s = section("matrix", "Matrix " + mname, true);
        s.tip = "Properties of the selected transformation.";
        float det = is4x4 ? matrix.determinant() : linear.determinant();
        float trace = is4x4 ? matrix.trace() : linear.trace();
        tipped(s, fmt("det %s = %.4f", M, clean(det)),
               "The determinant is how much the matrix scales volumes. Negative: orientation flips (a mirror). "
               "Zero: space is squashed into a plane, line or point, and it cannot be undone.");
        if (std::fabs(det) < 1e-6f) note(s, "Space is squashed flat: the matrix is singular");
        else if (det < 0.0f) note(s, fmt("Orientation flips; volumes scale by %.3f", std::fabs(det)));
        else note(s, fmt("Volumes scale by %.3f", det));
        tipped(s, fmt("tr %s = %.4f", M, clean(trace)),
               "Trace: the sum of the diagonal entries. It always equals the sum of the eigenvalues.");

        int n = is4x4 ? 4 : 3;
        float vals[16];
        bool invertible = false;
        if (is4x4) {
            if (auto inv = matrix.inverse()) {
                invertible = true;
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++) vals[i * 4 + j] = inv->m[i][j];
            }
        } else if (auto inv = linear.inverse()) {
            invertible = true;
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++) vals[i * 3 + j] = inv->m[i][j];
        }
        if (invertible) {
            addMatrix(s, (mname + "⁻¹").c_str(), vals, n);
            s.lines[s.lines.size() - n - 1].tip = "The inverse undoes the matrix: M⁻¹M = I. It exists exactly when det ≠ 0.";
        } else {
            body(s, mname + "⁻¹ does not exist (det = 0)", Tone::Danger);
        }
    }

    // --- Eigen -------------------------------------------------------------
    m_eigen = eigen3(linear);
    {
        Section& s = section("eigen", "Eigenvalues of " + mname, true);
        s.tip = "Roots of det(M − λI) = 0 and the directions the matrix only stretches.";
        if (is4x4) note(s, "Of the upper-left 3×3 (linear) part");
        int index = 1;
        bool anyComplex = false, anyMissing = false;
        for (const auto& ev : m_eigen.values) {
            if (ev.isReal()) {
                tipped(s, fmt("λ%s = %.3f", subscript(index), clean(ev.re)),
                       "Eigenvalue: M v = λ v. Along its eigenvector v, the matrix only stretches by λ (negative λ "
                       "also flips).");
                if (ev.hasVector) {
                    s.lines.push_back({fmt("v%s = ", subscript(index)) + vecStr(ev.vector), Tone::Eigen,
                                       ui::FontRole::Body,
                                       "Eigenvector (unit length). Add an Eigenvectors object to see these "
                                       "directions in the scene."});
                } else {
                    anyMissing = true;
                }
            } else {
                tipped(s, fmt("λ%s = %.3f %c %.3fi", subscript(index), clean(ev.re), ev.im < 0 ? '-' : '+',
                              std::fabs(ev.im)),
                       "Complex eigenvalues come in conjugate pairs: the matrix rotates a plane by arg λ and "
                       "scales it by |λ|, so no real direction stays fixed there.");
                anyComplex = true;
            }
            index++;
        }
        if (anyComplex) note(s, "A complex pair means the matrix rotates a plane");
        if (anyMissing) note(s, "Defective: fewer eigenvectors than eigenvalues");
    }

    // --- Decompositions ----------------------------------------------------
    auto toArray = [](const Matrix3D& m, float* out) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) out[i * 3 + j] = m.m[i][j];
    };
    float vals[9];
    {
        Section& s = section("lu", "LU  ·  PA = LU", false);
        s.tip = "Gaussian elimination as factors: P swaps rows, L (lower triangular) records the elimination "
                "steps, U (upper triangular) is the result.";
        if (is4x4) note(s, "Of the 3×3 linear part");
        LUResult lu = luDecompose(linear);
        toArray(lu.P, vals); addMatrix(s, "P", vals, 3);
        toArray(lu.L, vals); addMatrix(s, "L", vals, 3);
        toArray(lu.U, vals); addMatrix(s, "U", vals, 3);
        if (lu.singular) note(s, "A zero pivot: the matrix is singular");
    }
    {
        Section& s = section("qr", "QR  ·  A = QR", false);
        s.tip = "Gram–Schmidt as factors: Q has orthonormal columns (a rotation, maybe with a mirror) and R is "
                "upper triangular.";
        if (is4x4) note(s, "Of the 3×3 linear part");
        QRResult qr = qrDecompose(linear);
        toArray(qr.Q, vals); addMatrix(s, "Q", vals, 3);
        toArray(qr.R, vals); addMatrix(s, "R", vals, 3);
    }
    {
        Section& s = section("svd", "SVD  ·  A = UΣV^T", false);
        s.tip = "Every matrix is a rotation (V^T), then stretches along the axes by the singular values σ (Σ), "
                "then another rotation (U).";
        if (is4x4) note(s, "Of the 3×3 linear part");
        SVDResult svd = svd3(linear);
        body(s, fmt("σ = %.3f, %.3f, %.3f", svd.S.m[0][0], svd.S.m[1][1], svd.S.m[2][2]));
        toArray(svd.U, vals); addMatrix(s, "U", vals, 3);
        toArray(svd.V, vals); addMatrix(s, "V", vals, 3);
    }
}

float InfoPanel::drawContent(Rectangle area) {
    const ui::Theme& t = ui::theme();
    float x = area.x + ui::px(ui::kPad);
    float w = area.width - 2 * ui::px(ui::kPad);
    float y = area.y + ui::px(8.0f);
    float startY = area.y;

    for (auto& s : m_sections) {
        Rectangle headerRect = {x, y, w, ui::px(28.0f)};
        ui::sectionHeader(headerRect, s.title.c_str(), s.expanded, nullptr, true);
        if (s.tip) ui::tooltip(headerRect, s.tip, s.title.c_str());
        y += headerRect.height + ui::px(2.0f);

        if (s.expanded) {
            for (const auto& line : s.lines) {
                Color c = t.text;
                if (line.tone == Tone::Muted) c = t.textMuted;
                else if (line.tone == Tone::Eigen) c = t.eigen;
                else if (line.tone == Tone::Danger) c = t.danger;
                float lineH = std::round(ui::fontSize(line.role) * 1.45f);
                std::string shown = ui::ellipsize(line.text.c_str(), line.role, w - ui::px(14.0f));
                ui::text(shown.c_str(), x + ui::px(14.0f), y + (lineH - ui::fontSize(line.role)) / 2, line.role, c);
                if (line.tip) ui::tooltip({x, y, w, lineH}, line.tip);
                y += lineH;
            }
            y += ui::px(10.0f);
        }
    }
    return y - startY + ui::px(4.0f);
}

} // namespace linalg
