#pragma once

#include "math/Decompositions.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

// Right-hand "Analysis" card with computed results: dot/cross products,
// determinant, trace, inverse, eigenvalues and LU / QR / SVD factorizations.
// Sections collapse when their headers are clicked.
class InfoPanel {
public:
    InfoPanel();

    // Recompute everything (call when the inputs change)
    void update(const Matrix4D& matrix, bool is4x4, const Vector3D& a, const Vector3D& b);

    void draw();

    void toggleVisible() { m_visible = !m_visible; }
    void setVisible(bool v) { m_visible = v; }
    bool visible() const { return m_visible; }
    float width() const;            // pixels, including the margin (0 when hidden)
    bool containsMouse() const;
    Rectangle bounds() const { return m_bounds; }

    // Eigen-decomposition of the linear (3x3) part, for drawing eigenvectors
    const EigenResult& eigen() const { return m_eigen; }

private:
    enum class Tone { Normal, Muted, Eigen, Danger };
    struct Line {
        std::string text;
        Tone tone;
        ui::FontRole role;
    };
    struct Section {
        std::string title;
        bool expanded;
        std::vector<Line> lines;
    };

    static void addMatrix(Section& s, const char* name, const float* values, int n);

    std::vector<Section> m_sections;
    EigenResult m_eigen;
    Rectangle m_bounds;
    float m_scroll;
    float m_contentHeight;
    bool m_visible;
};

} // namespace linalg
