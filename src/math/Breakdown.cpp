#include "math/Breakdown.hpp"
#include "math/Decompositions.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace linalg {

namespace {

constexpr float kEps = 1e-3f;
constexpr float kPi = 3.14159265f;

Matrix4D embed(const Matrix3D& m) {
    Matrix4D r = Matrix4D::identity();
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r.m[i][j] = m.m[i][j];
    return r;
}

std::string num(float v) {
    char buf[32];
    if (std::fabs(v - std::round(v)) < 5e-3f) snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::lround(v)));
    else snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}

std::string axisName(const Vector3D& a) {
    const char* names[3] = {"x", "y", "z"};
    float c[3] = {a.x, a.y, a.z};
    for (int i = 0; i < 3; i++) {
        if (std::fabs(std::fabs(c[i]) - 1.0f) < 2e-3f) {
            return std::string(c[i] < 0 ? "−" : "") + names[i] + "-axis";
        }
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "(%.2f, %.2f, %.2f)", a.x, a.y, a.z);
    return buf;
}

BreakdownStep rotationStep(const Matrix3D& r) {
    BreakdownStep s;
    s.kind = BreakdownStep::Kind::Rotation;
    s.title = "Rotate";
    s.rotation = Quaternion::fromMatrix(r);
    float degrees = s.rotation.angle() * 180.0f / kPi;
    std::string axis = axisName(s.rotation.axis());
    // "about the −z-axis" reads better as a negative angle about the z-axis
    if (axis.rfind("−", 0) == 0) {
        degrees = -degrees;
        axis = axis.substr(std::string("−").size());
    }
    char buf[96];
    snprintf(buf, sizeof(buf), "%.1f° about ", degrees);
    if (buf[0] == '-') {
        s.description = "−" + std::string(buf + 1);
    } else {
        s.description = buf;
    }
    s.description += (axis.find("-axis") != std::string::npos ? "the " : "") + axis;
    return s;
}

BreakdownStep scaleStep(const Vector3D& k) {
    BreakdownStep s;
    s.kind = BreakdownStep::Kind::Scale;
    s.scale = k;
    const char* names[3] = {"x", "y", "z"};
    float v[3] = {k.x, k.y, k.z};

    bool uniform = std::fabs(v[0] - v[1]) < kEps && std::fabs(v[1] - v[2]) < kEps;
    if (uniform && v[0] > 0) {
        s.title = v[0] > 1 ? "Enlarge" : "Shrink";
        s.description = "everything by " + num(v[0]);
        return s;
    }

    std::string d;
    bool flips = false, squash = false, stretches = false;
    for (int i = 0; i < 3; i++) {
        float a = std::fabs(v[i]);
        if (std::fabs(a - 1.0f) >= kEps) stretches = true;
        std::string part;
        if (a < kEps) {
            part = std::string("squash ") + names[i] + " to 0";
            squash = true;
        } else if (std::fabs(a - 1.0f) < kEps) {
            part = v[i] < 0 ? std::string("flip ") + names[i] : "";
        } else {
            part = std::string(names[i]) + " by " + num(a);
            if (v[i] < 0) part += " and flip";
        }
        if (v[i] < 0) flips = true;
        if (part.empty()) continue;
        if (!d.empty()) d += ", ";
        d += part;
    }
    s.title = squash ? "Flatten" : (flips && !stretches) ? "Mirror" : flips ? "Stretch and mirror" : "Stretch";
    s.description = d;
    return s;
}

bool nearIdentity(const Matrix3D& r) {
    return Quaternion::fromMatrix(r).angle() < 0.5f * kPi / 180.0f;
}

} // namespace

Matrix4D BreakdownStep::matrix() const {
    return partial(1.0f);
}

Matrix4D BreakdownStep::partial(float f) const {
    f = std::clamp(f, 0.0f, 1.0f);
    switch (kind) {
        case Kind::Rotation:
            return embed(Quaternion::slerp(Quaternion::identity(), rotation, f).toMatrix());
        case Kind::Scale:
            return Matrix4D::scale(1 + (scale.x - 1) * f, 1 + (scale.y - 1) * f, 1 + (scale.z - 1) * f);
        case Kind::Translation:
            return Matrix4D::translation(translation.x * f, translation.y * f, translation.z * f);
    }
    return Matrix4D::identity();
}

Matrix4D TransformBreakdown::after(int k) const {
    Matrix4D m = Matrix4D::identity();
    for (int i = 0; i < std::min(k, static_cast<int>(steps.size())); i++) m = steps[i].matrix() * m;
    return m;
}

Matrix4D TransformBreakdown::at(float t) const {
    float n = static_cast<float>(steps.size());
    t = std::clamp(t, 0.0f, n);
    int k = static_cast<int>(std::floor(t));
    if (k >= static_cast<int>(steps.size())) return after(k);
    return steps[k].partial(t - k) * after(k);
}

TransformBreakdown breakdown(const Matrix4D& m4) {
    TransformBreakdown b;
    Matrix3D a;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) a.m[i][j] = m4.m[i][j];

    SVDResult s = svd3(a);
    Matrix3D u = s.U, v = s.V;
    Vector3D sigma(s.S.m[0][0], s.S.m[1][1], s.S.m[2][2]);

    // Make U and V proper rotations; a mirror becomes a negative stretch
    if (v.determinant() < 0) {
        for (int i = 0; i < 3; i++) v.m[i][2] = -v.m[i][2];
        sigma.z = -sigma.z;
    }
    if (u.determinant() < 0) {
        for (int i = 0; i < 3; i++) u.m[i][2] = -u.m[i][2];
        sigma.z = -sigma.z;
    }

    // The SVD is not unique: permuting the singular values, or flipping the
    // signs of matching columns of U and V, gives the same product. Choose the
    // version with the smallest rotations (both U and V kept proper), so the
    // steps read naturally (e.g. a shear becomes a small turn about z).
    {
        const int perms[6][3] = {{0, 1, 2}, {1, 2, 0}, {2, 0, 1}, {0, 2, 1}, {2, 1, 0}, {1, 0, 2}};
        float sig[3] = {sigma.x, sigma.y, sigma.z};
        float bestCost = 1e9f;
        Matrix3D bestU = u, bestV = v;
        Vector3D bestSigma = sigma;
        for (const auto& p : perms) {
            for (int signs = 0; signs < 8; signs++) {
                Matrix3D pu, pv;
                for (int c = 0; c < 3; c++) {
                    float sign = (signs >> c) & 1 ? -1.0f : 1.0f;
                    for (int r = 0; r < 3; r++) {
                        pu.m[r][c] = sign * u.m[r][p[c]];
                        pv.m[r][c] = sign * v.m[r][p[c]];
                    }
                }
                if (pu.determinant() < 0 || pv.determinant() < 0) continue;
                float cost = Quaternion::fromMatrix(pu).angle() + Quaternion::fromMatrix(pv).angle();
                if (cost < bestCost - 1e-5f) {
                    bestCost = cost;
                    bestU = pu;
                    bestV = pv;
                    bestSigma = Vector3D(sig[p[0]], sig[p[1]], sig[p[2]]);
                }
            }
        }
        u = bestU;
        v = bestV;
        sigma = bestSigma;
    }

    float sx = std::fabs(sigma.x);
    bool equalMagnitude = sx > kEps && std::fabs(sx - std::fabs(sigma.y)) < kEps &&
                          std::fabs(sx - std::fabs(sigma.z)) < kEps;
    if (equalMagnitude) {
        // M = s·Q with Q orthogonal. Prefer one rotation (and one flip) over
        // the two rotations the SVD would give.
        Matrix3D q = a * (1.0f / sx);
        if (q.determinant() > 0) {
            if (!nearIdentity(q)) b.steps.push_back(rotationStep(q));
            if (std::fabs(sx - 1.0f) > kEps) b.steps.push_back(scaleStep(Vector3D(sx, sx, sx)));
        } else {
            // Q = R·F with F a flip of one axis; pick the flip that leaves the smallest rotation
            int best = 0;
            float bestAngle = 10.0f;
            Matrix3D bestR;
            for (int axis = 0; axis < 3; axis++) {
                Matrix3D f = Matrix3D::identity();
                f.m[axis][axis] = -1.0f;
                Matrix3D r = q * f;
                float angle = Quaternion::fromMatrix(r).angle();
                if (angle < bestAngle - 1e-4f) {
                    bestAngle = angle;
                    best = axis;
                    bestR = r;
                }
            }
            Vector3D k(sx, sx, sx);
            if (best == 0) k.x = -sx;
            else if (best == 1) k.y = -sx;
            else k.z = -sx;
            b.steps.push_back(scaleStep(k));
            if (!nearIdentity(bestR)) b.steps.push_back(rotationStep(bestR));
        }
    } else {
        Matrix3D vt = v.transposed();
        if (!nearIdentity(vt)) b.steps.push_back(rotationStep(vt));
        b.steps.push_back(scaleStep(sigma));
        if (!nearIdentity(u)) b.steps.push_back(rotationStep(u));
    }

    Vector3D t(m4.m[0][3], m4.m[1][3], m4.m[2][3]);
    if (t.magnitude() > kEps) {
        BreakdownStep step;
        step.kind = BreakdownStep::Kind::Translation;
        step.title = "Move";
        step.translation = t;
        step.description = "by (" + num(t.x) + ", " + num(t.y) + ", " + num(t.z) + ")";
        b.steps.push_back(step);
    }
    return b;
}

} // namespace linalg
