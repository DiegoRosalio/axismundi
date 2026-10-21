#include "math/Explain.hpp"
#include "math/Decompositions.hpp"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace linalg {

namespace {

using K = ExplainLine::Kind;

std::string fmt(const char* format, ...) __attribute__((format(printf, 1, 2)));

std::string fmt(const char* format, ...) {
    char buf[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return buf;
}

const char* kMinus = "−";

// Number for use inside a product: negative values get parentheses
std::string term(float v) {
    std::string s = explainNumber(v);
    return v < 0 && s != "0" ? "(" + s + ")" : s;
}

// "a + b" / "a − b" with the sign of b folded into the operator
std::string plus(const std::string& a, float b) {
    std::string n = explainNumber(std::fabs(b));
    return a + (b < 0 && n != "0" ? std::string(" ") + kMinus + " " : " + ") + n;
}

std::string vec(const Vector3D& v) {
    return "(" + explainNumber(v.x) + ", " + explainNumber(v.y) + ", " + explainNumber(v.z) + ")";
}

const char* sub(int i) {
    static const char* digits[] = {"₀", "₁", "₂", "₃", "₄"};
    return digits[std::clamp(i, 0, 4)];
}

std::string matrixRow(const float* row, int n) {
    std::string s = "[";
    for (int j = 0; j < n; j++) {
        std::string v = explainNumber(row[j]);
        s += std::string(std::max(0, 8 - static_cast<int>(v.size())), ' ');
        // The minus sign is one character wide but three bytes long
        if (v.rfind(kMinus, 0) == 0) s += "  ";
        s += v;
    }
    return s + " ]";
}

void addMatrix(Explanation& e, const char* label, const Matrix3D& m) {
    e.push_back({K::Note, label});
    for (int i = 0; i < 3; i++) e.push_back({K::Step, matrixRow(m.m[i].data(), 3)});
}

float det2(float a, float b, float c, float d) { return a * d - b * c; }

Matrix3D linearPart(const Matrix4D& m) {
    Matrix3D r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r.m[i][j] = m.m[i][j];
    return r;
}

} // namespace

std::string explainNumber(float v) {
    if (!std::isfinite(v)) return "undefined";
    if (std::fabs(v) < 5e-4f) return "0";
    std::string s;
    if (std::fabs(v - std::round(v)) < 5e-4f) {
        s = fmt("%d", static_cast<int>(std::lround(v)));
    } else {
        s = fmt("%.3f", v);
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    if (!s.empty() && s[0] == '-') s = kMinus + s.substr(1);
    return s;
}

Explanation explainDot(const Vector3D& a, const Vector3D& b) {
    Explanation e;
    e.push_back({K::Heading, "Dot product A · B"});
    e.push_back({K::Formula, "A · B = AxBx + AyBy + AzBz"});
    e.push_back({K::Step, "      = " + term(a.x) + "·" + term(b.x) + " + " + term(a.y) + "·" + term(b.y) + " + " +
                              term(a.z) + "·" + term(b.z)});
    e.push_back({K::Step, "      = " + plus(plus(explainNumber(a.x * b.x), a.y * b.y), a.z * b.z)});
    float dot = a.dot(b);
    e.push_back({K::Result, "A · B = " + explainNumber(dot)});

    float la = a.magnitude(), lb = b.magnitude();
    if (la > 1e-6f && lb > 1e-6f) {
        float c = std::clamp(dot / (la * lb), -1.0f, 1.0f);
        e.push_back({K::Heading, "Angle between A and B"});
        e.push_back({K::Formula, "cos θ = A·B / (|A| |B|)"});
        e.push_back({K::Step, "      = " + explainNumber(dot) + " / (" + explainNumber(la) + " · " +
                                  explainNumber(lb) + ") = " + explainNumber(c)});
        e.push_back({K::Result, "θ = " + explainNumber(std::acos(c) * 180.0f / 3.14159265f) + "°"});
        if (std::fabs(dot) < 1e-5f) e.push_back({K::Note, "A · B = 0, so A and B are perpendicular."});
    }
    return e;
}

Explanation explainCross(const Vector3D& a, const Vector3D& b) {
    Explanation e;
    Vector3D c = a.cross(b);
    e.push_back({K::Heading, "Cross product A × B"});
    e.push_back({K::Formula, "A × B = (AyBz − AzBy, AzBx − AxBz, AxBy − AyBx)"});
    e.push_back({K::Step, "x: " + term(a.y) + "·" + term(b.z) + " − " + term(a.z) + "·" + term(b.y) + " = " +
                              explainNumber(c.x)});
    e.push_back({K::Step, "y: " + term(a.z) + "·" + term(b.x) + " − " + term(a.x) + "·" + term(b.z) + " = " +
                              explainNumber(c.y)});
    e.push_back({K::Step, "z: " + term(a.x) + "·" + term(b.y) + " − " + term(a.y) + "·" + term(b.x) + " = " +
                              explainNumber(c.z)});
    e.push_back({K::Result, "A × B = " + vec(c)});
    e.push_back({K::Note, "Its length " + explainNumber(c.magnitude()) +
                              " is the area of the parallelogram spanned by A and B."});
    return e;
}

Explanation explainMatVec(const Matrix4D& m, bool is4x4, const Vector3D& v) {
    Explanation e;
    e.push_back({K::Heading, "Matrix times vector: M · A"});
    if (is4x4) {
        e.push_back({K::Note, "A point gets a 4th coordinate w = 1, so the last column adds a translation."});
        e.push_back({K::Formula, "x' = m₁₁x + m₁₂y + m₁₃z + m₁₄·1"});
    } else {
        e.push_back({K::Note, "Each row of M is multiplied with A (a dot product)."});
        e.push_back({K::Formula, "x' = m₁₁x + m₁₂y + m₁₃z"});
    }
    const char* names[3] = {"x'", "y'", "z'"};
    float out[4];
    int n = is4x4 ? 4 : 3;
    float in[4] = {v.x, v.y, v.z, 1.0f};
    for (int i = 0; i < n; i++) {
        std::string s;
        float sum = 0.0f;
        for (int j = 0; j < n; j++) {
            if (j) s += " + ";
            s += term(m.m[i][j]) + "·" + term(in[j]);
            sum += m.m[i][j] * in[j];
        }
        out[i] = sum;
        std::string name = i < 3 ? names[i] : "w'";
        e.push_back({K::Step, name + " = " + s + " = " + explainNumber(sum)});
    }
    Vector3D r(out[0], out[1], out[2]);
    if (is4x4 && std::fabs(out[3] - 1.0f) > 1e-6f && std::fabs(out[3]) > 1e-6f) {
        e.push_back({K::Note, "w' ≠ 1: divide by w' to get the point (perspective division)."});
        r = r * (1.0f / out[3]);
    }
    e.push_back({K::Result, "M · A = " + vec(r)});
    return e;
}

Explanation explainDeterminant(const Matrix4D& m4, bool is4x4) {
    Explanation e;
    if (!is4x4) {
        Matrix3D m = linearPart(m4);
        float a = m.m[0][0], b = m.m[0][1], c = m.m[0][2];
        float d = m.m[1][0], f = m.m[1][1], g = m.m[1][2];
        float h = m.m[2][0], i = m.m[2][1], k = m.m[2][2];
        float m1 = det2(f, g, i, k), m2 = det2(d, g, h, k), m3 = det2(d, f, h, i);
        e.push_back({K::Heading, "Determinant (expand along row 1)"});
        e.push_back({K::Formula, "det M = a(ei − fh) − b(di − fg) + c(dh − eg)"});
        e.push_back({K::Note, "a, b, c is the first row; each is multiplied by the 2×2 determinant left "
                              "after crossing out its row and column."});
        e.push_back({K::Step, "= " + term(a) + "·(" + term(f) + "·" + term(k) + " − " + term(g) + "·" + term(i) +
                                  ")"});
        e.push_back({K::Step, "  − " + term(b) + "·(" + term(d) + "·" + term(k) + " − " + term(g) + "·" + term(h) +
                                  ")"});
        e.push_back({K::Step, "  + " + term(c) + "·(" + term(d) + "·" + term(i) + " − " + term(f) + "·" + term(h) +
                                  ")"});
        e.push_back({K::Step, "= " + term(a) + "·" + term(m1) + " − " + term(b) + "·" + term(m2) + " + " + term(c) +
                                  "·" + term(m3)});
        float det = a * m1 - b * m2 + c * m3;
        e.push_back({K::Result, "det M = " + explainNumber(det)});
        if (std::fabs(det) < 1e-6f) e.push_back({K::Note, "Zero: M flattens space, so it has no inverse."});
        else if (det < 0) e.push_back({K::Note, "Negative: M flips orientation (like a mirror)."});
        else e.push_back({K::Note, "M scales every volume by " + explainNumber(det) + "."});
        return e;
    }

    e.push_back({K::Heading, "Determinant of a 4×4 (expand along row 1)"});
    e.push_back({K::Formula, "det M = Σ (−1)^(1+j) · m(1,j) · det(minor(1,j))"});
    float total = 0.0f;
    std::string sum;
    for (int j = 0; j < 4; j++) {
        Matrix3D minor;
        for (int r = 1; r < 4; r++) {
            int cc = 0;
            for (int c = 0; c < 4; c++) {
                if (c == j) continue;
                minor.m[r - 1][cc++] = m4.m[r][c];
            }
        }
        float md = minor.determinant();
        float sign = (j % 2 == 0) ? 1.0f : -1.0f;
        total += sign * m4.m[0][j] * md;
        e.push_back({K::Step, fmt("j = %d:  %s", j + 1, (sign > 0 ? "+" : kMinus)) + " " + term(m4.m[0][j]) +
                                  " · " + term(md)});
        if (j) sum += sign > 0 ? " + " : std::string(" ") + kMinus + " ";
        sum += term(m4.m[0][j]) + "·" + term(md);
    }
    e.push_back({K::Step, "= " + sum});
    e.push_back({K::Result, "det M = " + explainNumber(total)});
    e.push_back({K::Note, "Each minor is the 3×3 determinant left after crossing out row 1 and column j."});
    return e;
}

Explanation explainInverse(const Matrix4D& m4, bool is4x4) {
    Explanation e;
    if (is4x4) {
        e.push_back({K::Heading, "Inverse of a 4×4"});
        float det = m4.determinant();
        e.push_back({K::Step, "det M = " + explainNumber(det)});
        auto inv = m4.inverse();
        if (!inv) {
            e.push_back({K::Result, "det M = 0, so M has no inverse"});
            return e;
        }
        e.push_back({K::Note, "Computed by Gauss–Jordan elimination on [M | I]: row operations turn M into I, "
                              "and the same operations turn I into M⁻¹."});
        e.push_back({K::Note, "M⁻¹ ="});
        for (int i = 0; i < 4; i++) e.push_back({K::Step, matrixRow(inv->m[i].data(), 4)});
        e.push_back({K::Result, "M⁻¹ · M = I"});
        return e;
    }

    Matrix3D m = linearPart(m4);
    float det = m.determinant();
    e.push_back({K::Heading, "Inverse with the adjugate"});
    e.push_back({K::Formula, "M⁻¹ = adj(M) / det M"});
    e.push_back({K::Step, "det M = " + explainNumber(det)});
    if (std::fabs(det) < 1e-6f) {
        e.push_back({K::Result, "det M = 0, so M has no inverse"});
        e.push_back({K::Note, "M squashes space flat; information is lost and cannot be recovered."});
        return e;
    }
    // Cofactors C_ij = (−1)^(i+j) · det(minor_ij)
    Matrix3D cof;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float minor[4];
            int k = 0;
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 3; c++)
                    if (r != i && c != j) minor[k++] = m.m[r][c];
            float sign = ((i + j) % 2 == 0) ? 1.0f : -1.0f;
            cof.m[i][j] = sign * det2(minor[0], minor[1], minor[2], minor[3]);
        }
    }
    addMatrix(e, "Cofactors C(i,j) = (−1)^(i+j) · minor(i,j):", cof);
    Matrix3D adj = cof.transposed();
    addMatrix(e, "Adjugate = transpose of the cofactors:", adj);
    Matrix3D inv = adj * (1.0f / det);
    addMatrix(e, ("Divide every entry by det M = " + explainNumber(det) + ":").c_str(), inv);
    e.push_back({K::Result, "M⁻¹ · M = I"});
    return e;
}

Explanation explainEigen(const Matrix3D& m) {
    Explanation e;
    float tr = m.trace();
    float c1 = det2(m.m[0][0], m.m[0][1], m.m[1][0], m.m[1][1]) + det2(m.m[0][0], m.m[0][2], m.m[2][0], m.m[2][2]) +
               det2(m.m[1][1], m.m[1][2], m.m[2][1], m.m[2][2]);
    float det = m.determinant();

    e.push_back({K::Heading, "Eigenvalues: solve det(M − λI) = 0"});
    e.push_back({K::Formula, "λ³ − (tr M)λ² + c λ − det M = 0"});
    e.push_back({K::Note, "c is the sum of the three 2×2 determinants on the diagonal (principal minors)."});
    e.push_back({K::Step, "tr M = " + explainNumber(tr)});
    e.push_back({K::Step, "c = " + explainNumber(c1)});
    e.push_back({K::Step, "det M = " + explainNumber(det)});
    std::string poly = "λ³";
    poly = plus(poly, -tr) + "λ²";
    poly = plus(poly, c1) + "λ";
    poly = plus(poly, -det) + " = 0";
    e.push_back({K::Step, poly});

    EigenResult r = eigen3(m);
    for (int i = 0; i < 3; i++) {
        const auto& ev = r.values[i];
        std::string v = explainNumber(ev.re);
        if (!ev.isReal()) v += (ev.im < 0 ? std::string(" ") + kMinus + " " : " + ") + explainNumber(std::fabs(ev.im)) + "i";
        e.push_back({K::Result, std::string("λ") + sub(i + 1) + " = " + v});
    }
    float sumRe = r.values[0].re + r.values[1].re + r.values[2].re;
    e.push_back({K::Note, "Check: λ₁ + λ₂ + λ₃ = " + explainNumber(sumRe) + " = tr M, and λ₁λ₂λ₃ = det M."});
    for (int i = 0; i < 3; i++) {
        const auto& ev = r.values[i];
        if (ev.isReal() && ev.hasVector) {
            e.push_back({K::Step, std::string("v") + sub(i + 1) + " = " + vec(ev.vector)});
        }
    }
    e.push_back({K::Note, "Each eigenvector v solves (M − λI)v = 0: M only stretches it by λ."});
    return e;
}

Explanation explainComplexProduct(const Complex& z1, const Complex& z2) {
    Explanation e;
    Complex p = z1 * z2;
    e.push_back({K::Heading, "Multiply z₁ · z₂"});
    e.push_back({K::Formula, "(a + bi)(c + di) = ac + adi + bci + bd·i²"});
    e.push_back({K::Formula, "                 = (ac − bd) + (ad + bc)i"});
    e.push_back({K::Note, "using i² = −1."});
    e.push_back({K::Step, "real: " + term(z1.re) + "·" + term(z2.re) + " − " + term(z1.im) + "·" + term(z2.im) + " = " +
                              explainNumber(p.re)});
    e.push_back({K::Step, "imag: " + term(z1.re) + "·" + term(z2.im) + " + " + term(z1.im) + "·" + term(z2.re) + " = " +
                              explainNumber(p.im)});
    e.push_back({K::Result, "z₁z₂ = " + plus(explainNumber(p.re), p.im) + "i"});
    e.push_back({K::Heading, "Same product in polar form"});
    e.push_back({K::Formula, "|z₁z₂| = |z₁| |z₂|,  arg(z₁z₂) = arg z₁ + arg z₂"});
    e.push_back({K::Step, "|z₁z₂| = " + explainNumber(z1.abs()) + " · " + explainNumber(z2.abs()) + " = " +
                              explainNumber(z1.abs() * z2.abs())});
    float d1 = z1.arg() * 180.0f / 3.14159265f, d2 = z2.arg() * 180.0f / 3.14159265f;
    e.push_back({K::Step, "arg = " + explainNumber(d1) + "° + " + explainNumber(d2) + "° = " + explainNumber(d1 + d2) +
                              "°"});
    e.push_back({K::Note, "Multiplying by z₂ rotates by arg z₂ and scales by |z₂|."});
    return e;
}

Explanation explainPolar(const Complex& z, const char* name) {
    Explanation e;
    float r = z.abs();
    float deg = z.arg() * 180.0f / 3.14159265f;
    e.push_back({K::Heading, std::string("Polar form of ") + name});
    e.push_back({K::Formula, "|z| = √(a² + b²),  arg z = atan2(b, a)"});
    e.push_back({K::Step, "|z| = √(" + term(z.re) + "² + " + term(z.im) + "²) = " + explainNumber(r)});
    e.push_back({K::Step, "arg z = atan2(" + explainNumber(z.im) + ", " + explainNumber(z.re) + ") = " +
                              explainNumber(deg) + "°"});
    e.push_back({K::Result, std::string(name) + " = " + explainNumber(r) + " (cos " + explainNumber(deg) + "° + i sin " +
                                explainNumber(deg) + "°)"});
    e.push_back({K::Result, std::string("   = ") + explainNumber(r) + " e^(i·" + explainNumber(z.arg()) + ")"});
    return e;
}

Explanation explainEuler(float theta) {
    Explanation e;
    float c = std::cos(theta), s = std::sin(theta);
    e.push_back({K::Heading, "Euler's formula"});
    e.push_back({K::Formula, "e^(iθ) = cos θ + i sin θ"});
    e.push_back({K::Step, "θ = " + explainNumber(theta) + " rad = " + explainNumber(theta * 180.0f / 3.14159265f) + "°"});
    e.push_back({K::Step, "cos θ = " + explainNumber(c) + ",  sin θ = " + explainNumber(s)});
    e.push_back({K::Result, "e^(iθ) = " + plus(explainNumber(c), s) + "i"});
    e.push_back({K::Step, "|e^(iθ)| = √(cos²θ + sin²θ) = 1"});
    e.push_back({K::Note, "So e^(iθ) always lies on the unit circle, at angle θ. With θ = π it gives −1."});
    return e;
}

const std::vector<const char*>& stepTopics(bool complexPlane) {
    static const std::vector<const char*> space = {"A · B", "A × B", "M · A", "det M", "M⁻¹", "Eigenvalues"};
    static const std::vector<const char*> plane = {"z₁ · z₂", "Polar z₁", "Polar z₂", "Euler"};
    return complexPlane ? plane : space;
}

Explanation explainTopic(const StepsInput& in, int topic) {
    Explanation e;
    if (!in.complexPlane) {
        Matrix3D linear;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) linear.m[i][j] = in.matrix.m[i][j];
        switch (topic) {
            case 0: e = explainDot(in.a, in.b); break;
            case 1: e = explainCross(in.a, in.b); break;
            case 2: e = explainMatVec(in.matrix, in.is4x4, in.a); break;
            case 3: e = explainDeterminant(in.matrix, in.is4x4); break;
            case 4: e = explainInverse(in.matrix, in.is4x4); break;
            default: e = explainEigen(linear); break;
        }
        if (in.is4x4 && topic == 5) e.insert(e.begin(), {ExplainLine::Kind::Note, "Using the upper-left 3×3 (linear) part."});
    } else {
        switch (topic) {
            case 0: e = explainComplexProduct(in.z1, in.z2); break;
            case 1: e = explainPolar(in.z1, "z₁"); break;
            case 2: e = explainPolar(in.z2, "z₂"); break;
            default: e = explainEuler(in.theta); break;
        }
    }
    return e;
}

} // namespace linalg

