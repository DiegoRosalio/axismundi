#include "ui/InfoPanel.hpp"
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

void InfoPanel::update(const Matrix4D& matrix, bool is4x4, const Vector3D& a, const Vector3D& b) {
    // Rebuild sections in a fixed order, keeping their expanded state
    std::vector<Section> previous = std::move(m_sections);
    m_sections.clear();
    auto section = [&](const std::string& title, bool def) -> Section& {
        bool expanded = def;
        for (const auto& p : previous)
            if (p.title == title) expanded = p.expanded;
        m_sections.push_back({title, expanded, {}});
        return m_sections.back();
    };
    auto body = [](Section& s, const std::string& text, Tone tone = Tone::Normal) {
        s.lines.push_back({text, tone, ui::FontRole::Body});
    };
    auto note = [](Section& s, const std::string& text) {
        s.lines.push_back({text, Tone::Muted, ui::FontRole::Small});
    };

    // --- Vectors -----------------------------------------------------------
    {
        Section& s = section("Vectors", true);
        float dot = a.dot(b);
        float la = a.magnitude(), lb = b.magnitude();
        Vector3D cross = a.cross(b);
        body(s, fmt("A · B = %.3f", clean(dot)));
        body(s, fmt("|A| = %.3f     |B| = %.3f", la, lb));
        if (la > 1e-6f && lb > 1e-6f) {
            float c = std::clamp(dot / (la * lb), -1.0f, 1.0f);
            body(s, fmt("angle(A, B) = %.1f°", std::acos(c) * RAD2DEG));
            if (std::fabs(dot) < 1e-5f) note(s, "A and B are perpendicular");
        }
        body(s, "A × B = " + vecStr(cross));
        note(s, fmt("|A × B| = %.3f, the area of the parallelogram", cross.magnitude()));
    }

    // --- Matrix properties -------------------------------------------------
    Matrix3D linear;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) linear.m[i][j] = matrix.m[i][j];

    {
        Section& s = section("Matrix", true);
        float det = is4x4 ? matrix.determinant() : linear.determinant();
        float trace = is4x4 ? matrix.trace() : linear.trace();
        body(s, fmt("det M = %.4f", clean(det)));
        if (std::fabs(det) < 1e-6f) note(s, "Space is squashed flat: M is singular");
        else if (det < 0.0f) note(s, fmt("Orientation flips; volumes scale by %.3f", std::fabs(det)));
        else note(s, fmt("Volumes scale by %.3f", det));
        body(s, fmt("tr M = %.4f", clean(trace)));

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
        if (invertible) addMatrix(s, "M⁻¹", vals, n);
        else body(s, "M⁻¹ does not exist (det = 0)", Tone::Danger);
    }

    // --- Eigen -------------------------------------------------------------
    m_eigen = eigen3(linear);
    {
        Section& s = section("Eigenvalues", true);
        if (is4x4) note(s, "Of the upper-left 3×3 (linear) part");
        int index = 1;
        bool anyComplex = false, anyMissing = false;
        for (const auto& ev : m_eigen.values) {
            if (ev.isReal()) {
                body(s, fmt("λ%s = %.3f", subscript(index), clean(ev.re)));
                if (ev.hasVector) {
                    s.lines.push_back({fmt("v%s = ", subscript(index)) + vecStr(ev.vector), Tone::Eigen,
                                       ui::FontRole::Body});
                } else {
                    anyMissing = true;
                }
            } else {
                body(s, fmt("λ%s = %.3f %c %.3fi", subscript(index), clean(ev.re), ev.im < 0 ? '-' : '+',
                            std::fabs(ev.im)));
                anyComplex = true;
            }
            index++;
        }
        if (anyComplex) note(s, "A complex pair means M rotates a plane");
        if (anyMissing) note(s, "Defective: fewer eigenvectors than eigenvalues");
    }

    // --- Decompositions ----------------------------------------------------
    auto toArray = [](const Matrix3D& m, float* out) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) out[i * 3 + j] = m.m[i][j];
    };
    float vals[9];
    {
        Section& s = section("LU  ·  PA = LU", false);
        if (is4x4) note(s, "Of the 3×3 linear part");
        LUResult lu = luDecompose(linear);
        toArray(lu.P, vals); addMatrix(s, "P", vals, 3);
        toArray(lu.L, vals); addMatrix(s, "L", vals, 3);
        toArray(lu.U, vals); addMatrix(s, "U", vals, 3);
        if (lu.singular) note(s, "A zero pivot: the matrix is singular");
    }
    {
        Section& s = section("QR  ·  A = QR", false);
        if (is4x4) note(s, "Of the 3×3 linear part");
        QRResult qr = qrDecompose(linear);
        toArray(qr.Q, vals); addMatrix(s, "Q", vals, 3);
        toArray(qr.R, vals); addMatrix(s, "R", vals, 3);
    }
    {
        Section& s = section("SVD  ·  A = UΣV^T", false);
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
        ui::sectionHeader(headerRect, s.title.c_str(), s.expanded);
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
                y += lineH;
            }
            y += ui::px(10.0f);
        }
    }
    return y - startY + ui::px(4.0f);
}

} // namespace linalg
