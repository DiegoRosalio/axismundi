#pragma once

#include "math/Decompositions.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include "scene/Scene.hpp"
#include "ui/core/Ui.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace linalg {

// Content of the "Analysis" panel: computed results: dot/cross products,
// determinant, trace, inverse, eigenvalues and LU / QR / SVD factorizations.
// Sections collapse when their headers are clicked.
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
    enum class Tone { Normal, Muted, Eigen, Danger };
    struct Line {
        std::string text;
        Tone tone;
        ui::FontRole role;
        const char* tip = nullptr;
    };
    struct Section {
        std::string key;    // stable id (titles include object names)
        std::string title;
        bool expanded;
        std::vector<Line> lines;
        const char* tip = nullptr;
    };

    static void addMatrix(Section& s, const char* name, const float* values, int n);

    std::vector<Section> m_sections;
    EigenResult m_eigen;
    bool m_hasMatrix = false;
};

} // namespace linalg
