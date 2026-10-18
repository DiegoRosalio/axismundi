#pragma once

#include "animation/Playback.hpp"
#include "math/CurveGeometry.hpp"
#include "math/Derivatives.hpp"
#include "math/Taylor.hpp"
#include "math/Fourier.hpp"
#include "math/Integrals.hpp"
#include "scene/SceneObject.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace linalg::scene {

// --- Concrete object types ------------------------------------------------------

// A named number, driven three ways:
//  - Slider:  set by hand; static (the way to morph things by dragging)
//  - Animate: follows the scene clock from min to max with a repeat style,
//             easing, duration and start delay
//  - Formula: an expression of t, the scene time in seconds
//             (e.g. 2pi*t/5, sin(t), 3 + cos(2t)*k)
class ParameterObject : public SceneObject {
public:
    enum class Mode { Slider, Animate, Formula };

    ParameterObject();
    Mode mode() const;
    void setMode(Mode mode);
    bool timeDriven() const { return mode() != Mode::Slider; }
    double number() const;          // current value
    void setNumber(double v);       // Slider: set by hand (clamped and snapped to the step)
    double minimum() const { return propNumber("min", 0.0); }
    double maximum() const { return propNumber("max", 1.0); }
    Playback playback() const;      // repeat, easing and duration from the properties
    void setPlayback(const Playback& p);
    double startDelay() const { return propNumber("anim.start", 0.0); }
    // Animate mode: the value at a given scene time (pure)
    double animatedValue(double seconds) const;

    bool fieldActive(const Field& f) const override;

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<ParameterObject>(); }
};

// Arrow: by components (x, y, z) or by a formula ("a + b", "M*a"), drawn from `tail`
class VectorObject : public SceneObject {
public:
    VectorObject();
    bool byFormula() const { return prop("mode") == "formula"; }
    void setByFormula(bool formula);
    bool fieldActive(const Field& f) const override;
    Vector3D tail() const;

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<VectorObject>(); }
};

// 3x3 or 4x4 transformation: by cells or by a formula ("rotz(theta)", "A*B")
class MatrixObject : public SceneObject {
public:
    MatrixObject();
    int size() const { return static_cast<int>(propNumber("size", 3)); }
    void setSize(int n);
    bool byFormula() const { return prop("mode") == "formula"; }
    void setByFormula(bool formula);
    bool fieldActive(const Field& f) const override;
    static std::string cellKey(int row, int col);
    // Apply a preset on top of the current matrix (P·M), so presets add up:
    // rotate, then scale gives scale·rotate. The identity starts over. The
    // result is written as cells (a formula matrix becomes cells).
    void applyPreset(const Matrix3D& preset);
    // Write a matrix into the cells (size 3, or 4 with the full 4x4)
    void setCells(const Matrix4D& m, int size);

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<MatrixObject>(); }
};

// Eigenvector directions of a matrix
class EigenObject : public SceneObject {
public:
    EigenObject();

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<EigenObject>(); }
};

// Plots: y = f(x), curve (x(t), y(t), z(t)), surface z = f(x, y), implicit f(x, y) = 0
class PlotObject : public SceneObject {
public:
    enum class Kind { Function, Curve, Surface, Implicit };
    explicit PlotObject(Kind kind);
    Kind kind() const { return m_kind; }
    bool heatmap() const { return propBool("heatmap", true); }
    // Range values after evaluation (fields can be expressions such as -pi)
    double rangeValue(const char* key, double fallback) const;

    // Detail = samples per unit of the plotted range (cells per unit for surfaces
    // and implicit curves), so a wider range keeps the same smoothness.
    struct DensityRange {
        double min, max, defaultValue;
        int cap;  // most samples along one axis, for speed
    };
    static DensityRange densityRange(Kind kind);
    double density() const;
    void setDensity(double perUnit);
    struct Samples {
        int nx = 0, ny = 0;          // samples (cells) along x (or t) and y; ny = 0 for 1D plots
        int wantedX = 0, wantedY = 0;  // before the cap
        bool capped = false;
    };
    Samples samples() const;  // uses the evaluated ranges
    // Files from before per-unit detail stored a total "resolution"; convert it
    // to a density for the current ranges (call after evaluation).
    void convertOldResolution();

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<PlotObject>(m_kind); }

private:
    Kind m_kind;
};

// A point/arrow in the complex plane, with optional Euler-style extras
class ComplexObject : public SceneObject {
public:
    ComplexObject();

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<ComplexObject>(); }
};

// z = a·b shown as rotation + scaling (angles add, lengths multiply)
class ProductObject : public SceneObject {
public:
    ProductObject();

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<ProductObject>(); }
};

// A quaternion: a rotation in four numbers. Drawn as its axis and angle, the rotated basis, and
// optionally a vector before and after the rotation. Its value is the quaternion.
class QuaternionObject : public SceneObject {
public:
    QuaternionObject();
    bool showAxis() const { return propBool("axis", true); }
    bool showBasis() const { return propBool("basis", true); }
    bool actsOnVector() const { return propBool("act", false); }

protected:
    bool fieldActive(const Field& f) const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<QuaternionObject>(); }
};

// A vector field F(x, y, z): arrows on a grid, optional flow lines and particles. Plane fields
// live on the floor z = 0 (F's x and y are used); volume fields fill a box.
class VectorFieldObject : public SceneObject {
public:
    VectorFieldObject();
    bool volume() const { return prop("domain", "plane") == "volume"; }
    bool heatmap() const { return propBool("heatmap", true); }
    bool sameLength() const { return propBool("same_length", false); }
    bool lines() const { return propBool("lines", false); }
    bool particles() const { return propBool("particles", false); }
    int seeds() const;            // flow lines per axis
    double speed() const;         // particle speed (flow time per second)
    double rangeValue(const char* key, double fallback) const;

    // Arrows per unit of the range, capped for speed
    static constexpr double kDensityMin = 0.25, kDensityMax = 4.0, kDensityDefault = 1.0;
    static constexpr int kPlaneCap = 40, kVolumeCap = 12;  // most arrows along one axis
    double density() const;
    struct Counts {
        int nx = 1, ny = 1, nz = 1;
        bool capped = false;
    };
    Counts counts() const;
    bool followsClock() const override { return usesTime() || particles(); }

protected:
    bool fieldActive(const Field& f) const override;
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<VectorFieldObject>(); }
};

// ∬ f dA over a rectangle: Riemann boxes (n per side, a sampling rule) next to the precise value.
// The value is the integral; the boxes and a sampled surface are kept for drawing.
class DoubleIntegralObject : public SceneObject {
public:
    DoubleIntegralObject();
    integrals::Rect rect() const;
    int boxesPerSide() const;  // the n field, rounded and clamped to 1..60
    integrals::Rule rule() const;
    bool showBoxes() const { return propBool("boxes", true); }
    bool showSurface() const { return propBool("surface", true); }
    // From the last evaluation
    const integrals::RiemannSum& riemann() const { return m_riemann; }
    double exact() const { return m_exact; }
    const std::vector<std::vector<integrals::D3>>& surfaceLines() const { return m_lines; }
    std::uint64_t geometryVersion() const { return m_version; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<DoubleIntegralObject>(); }

private:
    mutable integrals::RiemannSum m_riemann;
    mutable double m_exact = 0.0;
    mutable std::vector<std::vector<integrals::D3>> m_lines;  // surface grid lines (x and y directions)
    mutable std::uint64_t m_version = 0;
};

// ∫ F · dr along a curve r(u): the work of a vector field. The value is the work up to `upto`
// (a fraction of the path); the path, F·T along it and a few F arrows are kept for drawing.
class LineIntegralObject : public SceneObject {
public:
    LineIntegralObject();
    struct Sample {
        integrals::D3 p;
        double along;  // F · T (positive: F helps the motion)
    };
    struct FieldArrow {
        integrals::D3 p, f;
    };
    const std::vector<Sample>& path() const { return m_path; }
    const std::vector<FieldArrow>& arrows() const { return m_arrows; }
    double total() const { return m_total; }        // the whole path
    double upto() const { return m_upto; }          // 0..1
    integrals::D3 marker() const { return m_marker; }  // where the work so far is measured
    double maxAlong() const { return m_maxAlong; }   // |F · T| largest, for coloring
    std::uint64_t geometryVersion() const { return m_version; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<LineIntegralObject>(); }

private:
    mutable std::vector<Sample> m_path;
    mutable std::vector<FieldArrow> m_arrows;
    mutable double m_total = 0.0, m_upto = 1.0, m_maxAlong = 0.0;
    mutable integrals::D3 m_marker;
    mutable std::uint64_t m_version = 0;
};

// The moving (Frenet) frame on a curve r(u) at u = u0: tangent, normal, binormal, curvature, torsion,
// the osculating circle and a curvature comb. The value is the point r(u0).
class FrameObject : public SceneObject {
public:
    FrameObject();
    bool showFrame() const { return propBool("frame", true); }
    bool showCircle() const { return propBool("circle", true); }
    bool showComb() const { return propBool("comb", false); }
    bool showCurve() const { return propBool("curve", true); }
    const std::vector<integrals::D3>& curve() const { return m_curve; }
    // Comb: base points and the tips (base − κ·N·scale), along the curve
    const std::vector<std::pair<integrals::D3, integrals::D3>>& comb() const { return m_comb; }
    const geometry::Frame& frame() const { return m_frame; }
    double u0() const { return m_u0; }
    std::uint64_t geometryVersion() const { return m_version; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<FrameObject>(); }

private:
    mutable std::vector<integrals::D3> m_curve;
    mutable std::vector<std::pair<integrals::D3, integrals::D3>> m_comb;
    mutable geometry::Frame m_frame;
    mutable double m_u0 = 0.0;
    mutable std::uint64_t m_version = 0;
};

// y = f(x) on the floor with its tangent line at x0: the slope f'(x0) is the value. Optional: the graph
// of f', a run-1/rise-f' triangle, and a secant through x0 and x0 + h that becomes the tangent as h → 0.
class TangentObject : public SceneObject {
public:
    using Samples = std::vector<std::pair<double, double>>;  // (x, y); y NaN where undefined
    TangentObject();
    bool showFunction() const { return propBool("function", true); }
    bool showDerivative() const { return propBool("derivative", true); }
    bool showTriangle() const { return propBool("triangle", true); }
    const Samples& function() const { return m_function; }
    const Samples& derivative() const { return m_derivative; }
    double x0() const { return m_x0; }
    double y0() const { return m_y0; }  // f(x0)
    double slope() const { return m_slope; }
    bool hasSecond() const { return m_hasSecond; }
    double second() const { return m_second; }  // f''(x0)
    bool hasSecant() const { return m_hasSecant; }
    double secantH() const { return m_h; }
    double secantSlope() const { return m_secantSlope; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<TangentObject>(); }

private:
    mutable Samples m_function, m_derivative;
    mutable double m_x0 = 0, m_y0 = 0, m_slope = 0, m_second = 0, m_h = 0, m_secantSlope = 0;
    mutable bool m_hasSecond = false, m_hasSecant = false;
};

// z = f(x, y) with its tangent plane at (x0, y0): the slopes along x and y, the gradient (the value, as
// [fx, fy, 0]: the steepest way up on the floor) and the normal.
class TangentPlaneObject : public SceneObject {
public:
    TangentPlaneObject();
    bool showSurface() const { return propBool("surface", true); }
    bool showPlane() const { return propBool("plane", true); }
    bool showSlopes() const { return propBool("slopes", true); }
    bool showGradient() const { return propBool("gradient", true); }
    bool showNormal() const { return propBool("normal", false); }
    // Wire lines of the surface (constant x, then constant y)
    const std::vector<std::vector<integrals::D3>>& surfaceLines() const { return m_lines; }
    integrals::D3 point() const { return {m_x0, m_y0, m_z0}; }
    double fx() const { return m_fx; }
    double fy() const { return m_fy; }
    static constexpr double kPatch = 1.5;  // the plane patch reaches ±1.5 around the point

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<TangentPlaneObject>(); }

private:
    mutable std::vector<std::vector<integrals::D3>> m_lines;
    mutable double m_x0 = 0, m_y0 = 0, m_z0 = 0, m_fx = 0, m_fy = 0;
};

// A point moving along a curve r(u) with its velocity r'(u0) (the value) and acceleration r''(u0); the
// acceleration can be split into the part along the motion (speeding up) and across it (turning).
class VelocityObject : public SceneObject {
public:
    VelocityObject();
    bool showCurve() const { return propBool("curve", true); }
    bool showVelocity() const { return propBool("velocity", true); }
    bool showAcceleration() const { return propBool("acceleration", true); }
    bool splitAcceleration() const { return propBool("split", false); }
    double arrowScale() const;  // prop "scale", 0.1..2 (default 0.7)
    const std::vector<integrals::D3>& curve() const { return m_curve; }
    integrals::D3 point() const { return m_p; }
    integrals::D3 velocity() const { return m_v; }
    integrals::D3 acceleration() const { return m_a; }
    integrals::D3 tangential() const { return m_at; }  // a's part along v
    integrals::D3 normal() const { return m_an; }      // a's part across v
    double u0() const { return m_u0; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<VelocityObject>(); }

private:
    mutable std::vector<integrals::D3> m_curve;
    mutable integrals::D3 m_p, m_v, m_a, m_at, m_an;
    mutable double m_u0 = 0;
};

// y = f(x) and its Taylor polynomial of degree N about a center a, on the floor. Optional: the previous degree
// (faint, to see what the newest term changed) and the separate terms. Drawn only (no value).
class TaylorObject : public SceneObject {
public:
    using Samples = std::vector<std::pair<double, double>>;  // (x, y); y NaN where undefined
    TaylorObject();
    int degree() const;  // N, rounded and clamped to 0..taylor::kMaxDegree
    bool showFunction() const { return propBool("function", true); }
    bool showPrevious() const { return propBool("previous", true); }
    bool showTerms() const { return propBool("terms", false); }
    const taylor::Series& series() const { return m_series; }
    double fCenter() const { return m_fa; }
    double maxError() const { return m_maxError; }  // largest |f − T_N| on the range (NaN: none)
    const Samples& function() const { return m_function; }
    const Samples& polynomial() const { return m_polynomial; }
    const Samples& previous() const { return m_previous; }
    const std::vector<Samples>& terms() const { return m_terms; }  // c_k (x − a)^k, the first 6

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<TaylorObject>(); }

private:
    mutable taylor::Series m_series;
    mutable double m_fa = 0, m_maxError = 0;
    mutable Samples m_function, m_polynomial, m_previous;
    mutable std::vector<Samples> m_terms;
};

// Newton's method on y = f(x): from x0, each step follows the tangent at (x_k, f(x_k)) down to the x axis.
// The value is the last iterate (the root estimate).
class NewtonObject : public SceneObject {
public:
    using Samples = std::vector<std::pair<double, double>>;
    NewtonObject();
    int stepsAsked() const;  // rounded and clamped to 0..30
    bool showFunction() const { return propBool("function", true); }
    const taylor::NewtonRun& run() const { return m_run; }
    const Samples& function() const { return m_function; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<NewtonObject>(); }

private:
    mutable taylor::NewtonRun m_run;
    mutable Samples m_function;
};

// A closed curve z(u) in the complex plane drawn by spinning circles (Fourier epicycles). The value is
// the tip at the current angle 2π·speed·t; the coefficients are kept until the curve changes.
class EpicyclesObject : public SceneObject {
public:
    EpicyclesObject();
    int circles() const;      // N, rounded and clamped to 1..100
    double angle() const { return m_angle; }  // u = 2π·speed·t
    const std::vector<fourier::Term>& terms() const { return m_terms; }
    const std::vector<fourier::cplx>& target() const { return m_target; }  // the curve, sampled
    bool followsClock() const override { return true; }
    static constexpr int kMaxK = 64;  // frequencies −64..64 are computed

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<EpicyclesObject>(); }

private:
    mutable std::vector<fourier::Term> m_terms;
    mutable std::vector<fourier::cplx> m_target;
    mutable double m_angle = 0.0;
};

// The Fourier series of f(x) on [a, b]: the partial sum with N terms next to f (on the floor, like a
// function plot), the separate harmonics and the spectrum. Drawn only (no value).
class FourierSeriesObject : public SceneObject {
public:
    FourierSeriesObject();
    int terms() const;  // N, rounded and clamped to 0..200
    bool showOriginal() const { return propBool("original", true); }
    bool showHarmonics() const { return propBool("harmonics", false); }
    bool extended() const { return propBool("extend", true); }
    const fourier::Series& series() const { return m_series; }
    // Sampled for drawing: f on [a, b], the partial sum (over three periods when extended), harmonics
    const std::vector<std::pair<double, double>>& original() const { return m_original; }
    const std::vector<std::pair<double, double>>& sum() const { return m_sum; }
    const std::vector<std::vector<std::pair<double, double>>>& harmonics() const { return m_harmonics; }
    int undefinedPoints() const { return m_undefined; }

protected:
    bool cachesValue() const override { return true; }
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<FourierSeriesObject>(); }

private:
    mutable fourier::Series m_series;
    mutable std::vector<std::pair<double, double>> m_original, m_sum;
    mutable std::vector<std::vector<std::pair<double, double>>> m_harmonics;
    mutable int m_undefined = 0;
};

// --- Registry ----------------------------------------------------------------------

struct ObjectTypeInfo {
    const char* id;          // stored in files
    const char* label;       // "Vector", "Surface z = f(x, y)", ...
    const char* category;    // "Basics", "Plots", ...
    const char* baseName;    // default name ("v", "M", "f", "z")
    unsigned spaces;         // bit mask of SpaceKind values
    const char* description; // one line for the Add menu
    std::function<std::unique_ptr<SceneObject>()> create;
};

inline unsigned spaceBit(SpaceKind k) { return 1u << static_cast<int>(k); }

const std::vector<ObjectTypeInfo>& objectTypes();
const ObjectTypeInfo* findType(const std::string& id);
std::unique_ptr<SceneObject> createObject(const std::string& id);

} // namespace linalg::scene
