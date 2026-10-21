#pragma once

// The Analysis panel's content: what the selected objects are and do, as sections of lines (dot and cross products,
// determinant, inverse, eigenvalues, LU / QR / SVD, quaternion rotations, complex numbers, integrals, derivatives).
// Computed from the selection, no drawing: each front end shows the sections its own way.

#include "math/Decompositions.hpp"
#include "scene/Space.hpp"
#include "scene/Scene.hpp"
#include <string>
#include <vector>

namespace linalg::scene {

struct AnalysisLine {
    enum class Tone { Normal, Muted, Eigen, Danger };
    enum class Style { Body, Mono, Small };  // text, a matrix row (aligned columns), a note
    std::string text;
    Tone tone = Tone::Normal;
    Style style = Style::Body;
    const char* tip = nullptr;
};

struct AnalysisSection {
    std::string key;    // stable id (titles include object names)
    std::string title;
    bool expanded;      // open at first (the factorizations start closed)
    std::vector<AnalysisLine> lines;
    const char* tip = nullptr;
};

struct Analysis {
    std::vector<AnalysisSection> sections;
    EigenResult eigen{};     // of the selected matrix, when hasMatrix
    bool hasMatrix = false;
};

Analysis analyze(const Space& space, const Selection& selection);

} // namespace linalg::scene
