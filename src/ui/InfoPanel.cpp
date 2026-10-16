#include "ui/InfoPanel.hpp"
#include <raygui.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace linalg {

namespace {

constexpr int kFont = 20;
constexpr int kHeaderFont = 22;
constexpr float kLineHeight = 24.0f;
constexpr float kHeaderHeight = 32.0f;

const Color kTextColor = {40, 40, 40, 255};
const Color kNoteColor = {110, 110, 110, 255};

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

} // namespace

InfoPanel::InfoPanel()
    : m_eigen{}, m_bounds{0, 0, 0, 0}, m_scroll(0.0f), m_contentHeight(0.0f), m_visible(true) {}

void InfoPanel::addMatrix(Section& s, const char* name, const float* values, int n) {
    for (int i = 0; i < n; i++) {
        std::string row = (i == n / 2) ? fmt("%-3s", name) : "   ";
        row += "[";
        for (int j = 0; j < n; j++) row += fmt("%7.3f", clean(values[i * n + j]));
        row += " ]";
        s.lines.push_back({row, kTextColor});
    }
}

void InfoPanel::update(const Matrix4D& matrix, bool is4x4, const Vector3D& a, const Vector3D& b) {
    // Rebuild sections in a fixed order, keeping their expanded state
    std::vector<Section> previous = std::move(m_sections);
    m_sections.clear();
    auto restore = [&](const std::string& title, bool def) -> Section& {
        bool expanded = def;
        for (const auto& p : previous)
            if (p.title == title) expanded = p.expanded;
        m_sections.push_back({title, expanded, {}});
        return m_sections.back();
    };

    // --- Vectors -----------------------------------------------------------
    {
        Section& s = restore("Vectors", true);
        float dot = a.dot(b);
        float la = a.magnitude(), lb = b.magnitude();
        Vector3D cross = a.cross(b);
        s.lines.push_back({fmt("A . B = %.3f", clean(dot)), kTextColor});
        s.lines.push_back({fmt("|A| = %.3f    |B| = %.3f", la, lb), kTextColor});
        if (la > 1e-6f && lb > 1e-6f) {
            float c = std::clamp(dot / (la * lb), -1.0f, 1.0f);
            s.lines.push_back({fmt("angle(A, B) = %.1f deg", std::acos(c) * RAD2DEG), kTextColor});
            if (std::fabs(dot) < 1e-5f)
                s.lines.push_back({"A and B are perpendicular", kNoteColor});
        }
        s.lines.push_back({"A x B = " + vecStr(cross), kTextColor});
        s.lines.push_back({fmt("|A x B| = %.3f (parallelogram area)", cross.magnitude()), kNoteColor});
    }

    // --- Matrix properties -------------------------------------------------
    Matrix3D linear;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            linear.m[i][j] = matrix.m[i][j];

    {
        Section& s = restore("Matrix", true);
        float det = is4x4 ? matrix.determinant() : linear.determinant();
        float trace = is4x4 ? matrix.trace() : linear.trace();
        s.lines.push_back({fmt("det = %.4f", clean(det)), kTextColor});
        if (std::fabs(det) < 1e-6f)
            s.lines.push_back({"  space is squashed flat (singular)", kNoteColor});
        else if (det < 0.0f)
            s.lines.push_back({"  orientation is flipped", kNoteColor});
        else
            s.lines.push_back({fmt("  volumes scale by %.3f", det), kNoteColor});
        s.lines.push_back({fmt("trace = %.4f", clean(trace)), kTextColor});

        if (is4x4) {
            auto inv = matrix.inverse();
            if (inv) {
                s.lines.push_back({"inverse:", kTextColor});
                float vals[16];
                for (int i = 0; i < 4; i++)
                    for (int j = 0; j < 4; j++) vals[i * 4 + j] = inv->m[i][j];
                addMatrix(s, "M^-1", vals, 4);
            } else {
                s.lines.push_back({"inverse: none (singular)", MAROON});
            }
        } else {
            auto inv = linear.inverse();
            if (inv) {
                s.lines.push_back({"inverse:", kTextColor});
                float vals[9];
                for (int i = 0; i < 3; i++)
                    for (int j = 0; j < 3; j++) vals[i * 3 + j] = inv->m[i][j];
                addMatrix(s, "M^-1", vals, 3);
            } else {
                s.lines.push_back({"inverse: none (singular)", MAROON});
            }
        }
    }

    // --- Eigen -------------------------------------------------------------
    m_eigen = eigen3(linear);
    {
        Section& s = restore("Eigenvalues", true);
        if (is4x4) s.lines.push_back({"(of the upper-left 3x3 linear part)", kNoteColor});
        int index = 1;
        bool anyComplex = false;
        bool anyMissing = false;
        for (const auto& ev : m_eigen.values) {
            if (ev.isReal()) {
                s.lines.push_back({fmt("lambda%d = %.3f", index, clean(ev.re)), kTextColor});
                if (ev.hasVector)
                    s.lines.push_back({"    v = " + vecStr(ev.vector), PURPLE});
                else
                    anyMissing = true;
            } else {
                s.lines.push_back({fmt("lambda%d = %.3f %c %.3fi", index, clean(ev.re),
                                       ev.im < 0 ? '-' : '+', std::fabs(ev.im)), kTextColor});
                anyComplex = true;
            }
            index++;
        }
        if (anyComplex)
            s.lines.push_back({"complex pair: the map rotates a plane", kNoteColor});
        if (anyMissing)
            s.lines.push_back({"defective: fewer eigenvectors than roots", kNoteColor});
    }

    // --- Decompositions ----------------------------------------------------
    auto toArray = [](const Matrix3D& m, float* out) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) out[i * 3 + j] = m.m[i][j];
    };
    float vals[9];

    {
        Section& s = restore("LU  (PA = LU)", false);
        if (is4x4) s.lines.push_back({"(of the 3x3 linear part)", kNoteColor});
        LUResult lu = luDecompose(linear);
        toArray(lu.P, vals); addMatrix(s, "P", vals, 3);
        toArray(lu.L, vals); addMatrix(s, "L", vals, 3);
        toArray(lu.U, vals); addMatrix(s, "U", vals, 3);
        if (lu.singular) s.lines.push_back({"zero pivot: matrix is singular", kNoteColor});
    }
    {
        Section& s = restore("QR  (A = QR)", false);
        if (is4x4) s.lines.push_back({"(of the 3x3 linear part)", kNoteColor});
        QRResult qr = qrDecompose(linear);
        toArray(qr.Q, vals); addMatrix(s, "Q", vals, 3);
        toArray(qr.R, vals); addMatrix(s, "R", vals, 3);
    }
    {
        Section& s = restore("SVD  (A = U S V^T)", false);
        if (is4x4) s.lines.push_back({"(of the 3x3 linear part)", kNoteColor});
        SVDResult svd = svd3(linear);
        s.lines.push_back({fmt("singular values: %.3f, %.3f, %.3f", svd.S.m[0][0], svd.S.m[1][1],
                               svd.S.m[2][2]), kTextColor});
        toArray(svd.U, vals); addMatrix(s, "U", vals, 3);
        toArray(svd.V, vals); addMatrix(s, "V", vals, 3);
    }
}

bool InfoPanel::containsMouse() const {
    return m_visible && CheckCollisionPointRec(GetMousePosition(), m_bounds);
}

void InfoPanel::draw() {
    if (!m_visible) return;

    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    float height = std::min(m_contentHeight + 20.0f, screenH - 2 * kMargin);
    m_bounds = {screenW - kWidth - kMargin, kMargin, kWidth, std::max(height, 60.0f)};

    DrawRectangleRec(m_bounds, Fade(RAYWHITE, 0.92f));
    DrawRectangleLinesEx(m_bounds, 1.0f, GRAY);

    // Scrolling
    float maxScroll = std::max(0.0f, m_contentHeight + 20.0f - m_bounds.height);
    if (containsMouse() && !GuiIsLocked()) m_scroll -= GetMouseWheelMove() * 40.0f;
    m_scroll = std::clamp(m_scroll, 0.0f, maxScroll);

    BeginScissorMode(static_cast<int>(m_bounds.x), static_cast<int>(m_bounds.y),
                     static_cast<int>(m_bounds.width), static_cast<int>(m_bounds.height));

    float x = m_bounds.x + 12.0f;
    float y = m_bounds.y + 10.0f - m_scroll;
    float startY = y;
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !GuiIsLocked();
    Vector2 mouse = GetMousePosition();

    for (auto& s : m_sections) {
        Rectangle header = {m_bounds.x + 4, y, kWidth - 8, kHeaderHeight - 4};
        bool hover = CheckCollisionPointRec(mouse, header) && CheckCollisionPointRec(mouse, m_bounds);
        DrawRectangleRec(header, hover ? Fade(SKYBLUE, 0.35f) : Fade(LIGHTGRAY, 0.5f));
        std::string title = (s.expanded ? "- " : "+ ") + s.title;
        DrawText(title.c_str(), static_cast<int>(x), static_cast<int>(y + 3), kHeaderFont, DARKBLUE);
        if (hover && clicked) s.expanded = !s.expanded;
        y += kHeaderHeight;

        if (s.expanded) {
            for (const auto& line : s.lines) {
                DrawText(line.text.c_str(), static_cast<int>(x + 6), static_cast<int>(y), kFont, line.color);
                y += kLineHeight;
            }
            y += 6.0f;
        }
    }
    m_contentHeight = y - startY;

    EndScissorMode();
}

} // namespace linalg
