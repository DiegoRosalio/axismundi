#pragma once
// Shared by every test file: the CHECK macros, the counters, and small helpers.
// The tests need no window and no raylib: they check the math, the formula engine, the scene model and the
// export directly. Run them with `make test`.
#include "animation/Playback.hpp"
#include "math/expr/Reference.hpp"
#include "math/Contour.hpp"
#include "export/LatexExport.hpp"
#include "export/PdfCompile.hpp"
#include "math/VectorField.hpp"
#include "math/Integrals.hpp"
#include "math/CurveGeometry.hpp"
#include "math/Fourier.hpp"
#include "math/OrbitCamera.hpp"
#include "math/Derivatives.hpp"
#include "text/LabelLayout.hpp"
#include "text/TextLayout.hpp"
#include <fstream>
#include "math/Complex.hpp"
#include "math/Decompositions.hpp"
#include "math/Expression.hpp"
#include "math/Explain.hpp"
#include "math/expr/Expr.hpp"
#include "scene/Scene.hpp"
#include <map>
#include <memory>
#include "math/Breakdown.hpp"
#include "math/Quaternion.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

// Test files use the project's names without the linalg:: prefix. (Fine in a test header; library headers
// never do this, because it would leak into every file that includes them.)
using namespace linalg;

// Counted by check(); main.cpp prints the totals
extern int g_failures;
extern int g_checks;
void check(bool ok, const char* expr, const char* file, int line);

// CHECK(x) records whether x is true; on failure it prints the file, line and expression.
// #cond turns the expression into text, and __FILE__/__LINE__ say where the check is.
#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) check(std::fabs((a) - (b)) <= (tol), #a " ~= " #b, __FILE__, __LINE__)

inline bool near(const Matrix3D& a, const Matrix3D& b, float tol = 1e-4f) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (std::fabs(a.m[i][j] - b.m[i][j]) > tol) return false;
    return true;
}

inline bool near(const Matrix4D& a, const Matrix4D& b, float tol = 1e-4f) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (std::fabs(a.m[i][j] - b.m[i][j]) > tol) return false;
    return true;
}

inline bool isUpperTriangular(const Matrix3D& m, float tol = 1e-5f) {
    return std::fabs(m.m[1][0]) < tol && std::fabs(m.m[2][0]) < tol && std::fabs(m.m[2][1]) < tol;
}

inline const std::vector<Matrix3D>& sampleMatrices() {
    static const std::vector<Matrix3D> mats = {
        Matrix3D::identity(),
        Matrix3D({{{{2, 1, 0}}, {{1, 3, 1}}, {{0, 1, 4}}}}),       // symmetric
        Matrix3D({{{{0, 2, 1}}, {{1, 0, 3}}, {{4, 1, 0}}}}),       // needs pivoting
        Matrix3D::rotation(0.7f),                                 // complex eigenvalues
        Matrix3D({{{{1, 2, 3}}, {{2, 4, 6}}, {{1, 1, 1}}}}),       // singular (rank 2)
        Matrix3D({{{{1, 2, 3}}, {{2, 4, 6}}, {{3, 6, 9}}}}),       // rank 1
        Matrix3D({{{{2, 0, 0}}, {{0, 2, 0}}, {{0, 0, 3}}}}),       // repeated eigenvalue
        Matrix3D::shear(0.5f, 0.0f),                              // defective
        Matrix3D({{{{-1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}}}),      // reflection
    };
    return mats;
}

struct MapResolver : expr::Resolver {
    std::map<std::string, expr::Value> values;
    bool lookup(const std::string& name, expr::Value& out) const override {
        auto it = values.find(name);
        if (it == values.end()) return false;
        out = it->second;
        return true;
    }
};

inline expr::Value evalText(const std::string& text, const expr::Resolver* r = nullptr) {
    expr::Program p;
    if (!p.compile(text)) return expr::Value::fail(p.error());
    return p.eval(r);
}
