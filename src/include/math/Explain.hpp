#pragma once

#include "math/Complex.hpp"
#include "math/Matrix3D.hpp"
#include "math/Matrix4D.hpp"
#include "math/Vector3D.hpp"
#include <string>
#include <vector>

namespace linalg {

// Worked, step-by-step calculations as text lines (no graphics), shown by
// the "Calculation steps" panel.
struct ExplainLine {
    enum class Kind {
        Heading,  // what is being computed
        Formula,  // the general rule
        Step,     // the rule with numbers filled in, intermediate values
        Result,   // the final value
        Note,     // a short remark in words
    };
    Kind kind;
    std::string text;
};

using Explanation = std::vector<ExplainLine>;

Explanation explainDot(const Vector3D& a, const Vector3D& b);
Explanation explainCross(const Vector3D& a, const Vector3D& b);
Explanation explainMatVec(const Matrix4D& m, bool is4x4, const Vector3D& v);
Explanation explainDeterminant(const Matrix4D& m, bool is4x4);
Explanation explainInverse(const Matrix4D& m, bool is4x4);
Explanation explainEigen(const Matrix3D& m);
Explanation explainComplexProduct(const Complex& z1, const Complex& z2);
Explanation explainPolar(const Complex& z, const char* name);
Explanation explainEuler(float theta);

// The Steps panel: what the worked calculations work on (the selected objects, else a, b, M, z1, z2, theta),
// its topics and the calculation for a topic. Shared by both front ends.
struct StepsInput {
    bool complexPlane = false;  // the complex plane's topics, else the 3D space's
    Matrix4D matrix = Matrix4D::identity();
    bool is4x4 = false;
    Vector3D a, b;
    Complex z1, z2;
    float theta = 0.0f;
};
const std::vector<const char*>& stepTopics(bool complexPlane);  // "A · B", "det M"... / "z₁ · z₂", "Euler"...
Explanation explainTopic(const StepsInput& input, int topic);
constexpr int kDefaultMatrixTopic = 3;  // det M

// Number formatting used in explanations: "2", "0.707", "−1.5" (real minus sign)
std::string explainNumber(float v);

} // namespace linalg
