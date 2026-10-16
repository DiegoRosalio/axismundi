#pragma once

#include "math/Decompositions.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

// Right-hand panel with computed results: dot/cross products, determinant,
// trace, inverse, eigenvalues and LU / QR / SVD factorizations.
// Sections can be collapsed by clicking their headers.
class InfoPanel {
public:
    InfoPanel();

    // Recompute everything (call when the inputs change)
    void update(const Matrix4D& matrix, bool is4x4, const Vector3D& a, const Vector3D& b);

    void draw();

    void toggleVisible() { m_visible = !m_visible; }
    bool visible() const { return m_visible; }
    float width() const { return m_visible ? kWidth + kMargin : 0.0f; }
    bool containsMouse() const;

    // Eigen-decomposition of the linear (3x3) part, for drawing eigenvectors
    const EigenResult& eigen() const { return m_eigen; }

    static constexpr float kWidth = 440.0f;
    static constexpr float kMargin = 10.0f;

private:
    struct Line {
        std::string text;
        Color color;
    };
    struct Section {
        std::string title;
        bool expanded;
        std::vector<Line> lines;
    };

    void addMatrix(Section& s, const char* name, const float* values, int n);

    std::vector<Section> m_sections;
    EigenResult m_eigen;
    Rectangle m_bounds;
    float m_scroll;
    float m_contentHeight;
    bool m_visible;
};

} // namespace linalg
