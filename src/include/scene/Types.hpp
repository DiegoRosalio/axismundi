#pragma once

#include "animation/Playback.hpp"
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
