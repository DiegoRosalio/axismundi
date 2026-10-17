#pragma once

#include "animation/Playback.hpp"
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
