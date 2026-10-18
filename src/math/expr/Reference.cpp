#include "math/expr/Reference.hpp"

namespace linalg::expr {

namespace {

std::vector<ReferenceSection> build() {
    std::vector<ReferenceSection> s;

    s.push_back({"Numbers and operators",
                 "Every field is a formula. Type numbers, operators and functions; the result updates as you type, "
                 "and a red outline means the formula has a problem.",
                 {
                     {"+  −  *  /", "Add, subtract, multiply, divide. * and / bind tighter than + and −.", "1 + 2*3"},
                     {"a ^ b", "Power. It binds tightest and groups from the right: 2^3^2 = 2^9.", "2^3^2"},
                     {"( … )", "Parentheses group.", "(1 + 2)*3"},
                     {"2x   3(a + b)   2pi", "Implicit multiplication: a number or ) followed by a name, ( or [ "
                                         "multiplies.", "2pi"},
                     {"-x", "Minus sign. -2^2 is -(2^2) = -4.", "-2^2"},
                     {"pi   e   i", "Constants: π, Euler's number and the imaginary unit.", "e^(i*pi)"},
                 }});

    s.push_back({"Objects and names",
                 "Any object can be used by its name in the formulas of other objects in the same space. When you "
                 "rename an object, formulas that use it are updated. Names start with a letter and may not be a "
                 "built-in name (sin, pi, t, ...). An object cannot depend on itself, even through others.",
                 {
                     {"a   M   theta", "The value of the object with that name: a number, complex number, vector or "
                                     "matrix. Plots have no value.", "2a - b", true},
                     {"v.x   v.y   v.z", "One component of a vector.", "[1, 2, 3].y"},
                     {"[a, b, c]", "A matrix whose columns are the vectors a, b and c.", "[a, b, c]", true},
                 }});

    s.push_back({"Time and plot variables",
                 "t is the scene time in seconds, run by the Time panel; everything that uses it moves when the "
                 "clock plays and can be scrubbed. Plots and vector fields also have their own variables.",
                 {
                     {"t", "Scene time in seconds, in every formula.", "[cos(t), sin(t), 1]"},
                     {"x   y", "Plot variables: y = f(x), z = f(x, y), f(x, y) = 0. Examples here use x = 1, y = 2.",
                      "x^2 + y^2"},
                     {"u", "The curve variable in x(u), y(u), z(u); it runs over the curve's range. Examples here use "
                           "u = 0.5.", "2cos(u)"},
                     {"x   y   z", "Vector field variables: F(x, y, z) gives the arrow at the point (x, y, z), e.g. a "
                                   "rotation [-y, x, 0] or a source [x, y, z]. Only fields have z; elsewhere z is an "
                                   "ordinary name. Examples here use z = 3.", "[-y, x, z/3]"},
                 }});

    s.push_back({"Vectors",
                 "Vectors have three components and are drawn as arrows in the 3D space (z points up).",
                 {
                     {"[x, y, z]", "A vector.", "[1, 2, 2]"},
                     {"v + w   v − w   k*v   v / k", "Add and subtract vectors, scale by a number.", "2[1, 0, 0] + [0, 1, 0]"},
                     {"dot(a, b)", "Dot product: zero when a and b are perpendicular.", "dot([1, 0, 0], [1, 1, 0])"},
                     {"cross(a, b)", "Cross product: perpendicular to both, by the right-hand rule.",
                      "cross([1, 0, 0], [0, 1, 0])"},
                     {"norm(v)   abs(v)", "Length.", "norm([3, 4, 0])"},
                     {"normalize(v)   unit(v)", "The vector with the same direction and length 1.", "normalize([0, 3, 4])"},
                 }});

    s.push_back({"Matrices",
                 "3×3 matrices move space; 4×4 matrices also move the origin (homogeneous coordinates). Matrices "
                 "are drawn as the moved basis.",
                 {
                     {"[[a, b, c], [d, e, f], [g, h, i]]", "A 3×3 matrix by rows (4×4 with four rows of four).",
                      "[[1, 2, 0], [0, 1, 0], [0, 0, 1]]"},
                     {"M*v", "Apply M to a vector. A 4×4 matrix treats v as a point.", "rotz(pi/2)*[1, 0, 0]"},
                     {"M*N   k*M   M + N", "Compose (first N, then M), scale, add.", "scale(2)*rotz(pi/2)"},
                     {"M^n   M^-1", "Whole powers; negative powers use the inverse.", "[[2, 0, 0], [0, 4, 0], [0, 0, 1]]^-1"},
                     {"det(M)", "Determinant: how volumes scale (negative: mirrored).", "det([[2, 1, 0], [1, 2, 0], [0, 0, 1]])"},
                     {"tr(M)   trace(M)", "Trace: the sum of the diagonal.", "tr(scale(2))"},
                     {"inv(M)", "Inverse (an error when det = 0).", "inv(scale(2))"},
                     {"transpose(M)", "Rows become columns.", "transpose([[1, 2, 0], [0, 1, 0], [0, 0, 1]])"},
                     {"rotx(a)   roty(a)   rotz(a)", "Rotation by angle a (radians) about an axis.", "rotz(pi/4)"},
                     {"rot(axis, a)", "Rotation by angle a about any axis vector.", "rot([1, 1, 1], 2pi/3)"},
                     {"scale(k)   scale(sx, sy, sz)", "Uniform or per-axis scaling.", "scale(2, 1, 0.5)"},
                     {"translate(x, y, z)   translate(v)", "A 4×4 matrix that moves the origin.", "translate(1, 0, 2)"},
                     {"eye()   eye(4)", "The identity, 3×3 or 4×4.", "eye(4)"},
                 }});

    s.push_back({"Quaternions",
                 "A quaternion w + xi + yj + zk is a rotation in four numbers: turning by an angle about an axis "
                 "gives w = cos(angle/2) and (x, y, z) = the axis times sin(angle/2). Multiplying two of them does "
                 "one rotation after the other.",
                 {
                     {"qaxis(axis, angle)", "The rotation by angle (radians) about the axis: a unit quaternion.",
                      "qaxis([0, 0, 1], pi/2)"},
                     {"quat(w, x, y, z)   quat(M)", "A quaternion from its four numbers, or from a rotation matrix.",
                      "quat(rotz(pi/2))"},
                     {"q1 * q2", "First q2, then q1 (like matrices). The order matters: qx·qz ≠ qz·qx.",
                      "qaxis([1, 0, 0], pi/2) * qaxis([0, 0, 1], pi/2)"},
                     {"rotate(q, v)", "The vector v turned by q (q v q⁻¹).", "rotate(qaxis([0, 0, 1], pi/2), [1, 0, 0])"},
                     {"mat(q)", "The rotation matrix of q.", "mat(qaxis([0, 0, 1], pi/2))"},
                     {"axis(q)   angle(q)", "The rotation axis (length 1) and angle (0 to π) of q.",
                      "angle(qaxis([0, 1, 0], 2))"},
                     {"slerp(a, b, s)", "Turns from rotation a to rotation b as s goes from 0 to 1, along the "
                                        "shortest way and at constant speed. lerp and keys do the same with "
                                        "quaternions.",
                      "slerp(quat(1, 0, 0, 0), qaxis([0, 0, 1], pi), 0.5)"},
                     {"conj(q)   inv(q)   abs(q)   unit(q)   q.w q.x q.y q.z",
                      "Conjugate (the opposite rotation for |q| = 1), inverse, length, q scaled to length 1, and "
                      "the four numbers.", "abs(quat(1, 1, 1, 1))"},
                 }});

    s.push_back({"Complex numbers",
                 "Used in the complex plane: the real part runs along the red axis, the imaginary part along the "
                 "green one. Most functions accept complex numbers.",
                 {
                     {"2 + 3i", "A complex number.", "(1 + i)*(1 - i)"},
                     {"conj(z)", "Conjugate: mirror across the real axis.", "conj(2 + 3i)"},
                     {"re(z)   im(z)", "Real and imaginary parts.", "im(2 + 3i)"},
                     {"abs(z)   arg(z)", "Length and angle (radians).", "arg(i)"},
                     {"polar(r, a)", "The complex number with length r and angle a.", "polar(2, pi/2)"},
                     {"exp(i*a)", "The point at angle a on the unit circle (Euler's formula).", "exp(i*pi/3)"},
                 }});

    s.push_back({"Functions",
                 "Angles are in radians (pi = 180°). Functions marked with z also work on complex numbers.",
                 {
                     {"sin  cos  tan", "Trigonometry (z).", "sin(pi/6)"},
                     {"asin  acos  atan", "Inverse trigonometry (z).", "acos(0)"},
                     {"atan2(y, x)", "The angle of the point (x, y), in (−π, π].", "atan2(1, -1)"},
                     {"sinh  cosh  tanh", "Hyperbolic functions (z).", "cosh(0)"},
                     {"exp  log  ln", "Exponential and natural logarithm; log and ln are the same (z).", "ln(e^2)"},
                     {"sqrt(x)", "Square root; sqrt(-1) needs a complex input: sqrt(-1 + 0i).", "sqrt(2)"},
                     {"abs(x)", "Absolute value (also the length of vectors and complex numbers).", "abs(-3)"},
                     {"floor(x)   ceil(x)", "Round down / up.", "floor(2.7)"},
                     {"min(a, b)   max(a, b)", "Smaller / larger of two numbers.", "max(2, 5)"},
                     {"mod(a, b)", "Remainder, never negative for positive b: mod(t, 3) restarts every 3 s.", "mod(7, 3)"},
                     {"sign(x)", "−1, 0 or 1: the sign of x. sign(x) is a square wave on [−π, π].", "sign(-2.5)"},
                 }});

    s.push_back({"Blending and morphing",
                 "Blend any two values of the same kind with a number s from 0 to 1, usually a slider parameter. "
                 "The Add menu builds these for the selection (Morph from identity, Grow from zero, Blend).",
                 {
                     {"lerp(a, b, s)", "From a (s = 0) to b (s = 1). Works for numbers, complex numbers, vectors and "
                                       "matrices; a plain 0 stands for zero of the other kind.",
                      "lerp(eye(), rotz(pi/2), 0.5)"},
                     {"keys(s, A, B, C, …)", "Runs through the values in order as s goes from 0 to 1.",
                      "keys(0.25, 0, 10, 0)"},
                 }});

    s.push_back({"When something is red",
                 "Hover a red row in the objects list or the red line under a field to read the problem.",
                 {
                     {"unknown name 'q'", "No object is called q in this space (names are case-sensitive).", ""},
                     {"circular reference", "Objects depend on each other in a loop; break it by changing one formula.", ""},
                     {"use dot(a, b) or cross(a, b)", "Vectors cannot be multiplied with *: pick the product you mean.", ""},
                     {"x is only available in plots", "x and y are plot variables; in other objects use numbers, "
                                                      "names or t.", ""},
                 }});
    return s;
}

} // namespace

const std::vector<ReferenceSection>& reference() {
    static const std::vector<ReferenceSection> sections = build();
    return sections;
}

} // namespace linalg::expr
