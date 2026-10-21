#pragma once

#include "math/Decompositions.hpp"
#include "scene/Analysis.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "scene/Scene.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

// Content of the "Analysis" panel: the sections computed by scene::analyze (dot/cross products, determinant,
// trace, inverse, eigenvalues, LU / QR / SVD...), drawn here. Sections collapse when their headers are clicked.
class InfoPanel {
public:
    InfoPanel();

    // Recompute from the selected objects of a space
    void update(const scene::Space& space, const scene::Selection& selection);

    // Draw the sections into `area` (its y already includes scrolling).
    // Returns the content height in pixels.
    float drawContent(Rectangle area);

    // Eigen-decomposition of the selected matrix (valid when hasMatrix())
    const EigenResult& eigen() const { return m_eigen; }
    bool hasMatrix() const { return m_hasMatrix; }

private:
    struct Section {
        scene::AnalysisSection content;  // computed by scene::analyze
        bool expanded;                   // clicked open or closed
    };

    std::vector<Section> m_sections;
    EigenResult m_eigen;
    bool m_hasMatrix = false;
};

} // namespace linalg
