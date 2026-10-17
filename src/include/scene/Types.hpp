#pragma once

#include "scene/SceneObject.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace linalg::scene {

// --- Concrete object types ------------------------------------------------------

// A named number with a slider range; can play (animate) through its range
class ParameterObject : public SceneObject {
public:
    ParameterObject();
    double number() const { return propNumber("value", 0.0); }
    void setNumber(double v);
    double minimum() const { return propNumber("min", 0.0); }
    double maximum() const { return propNumber("max", 1.0); }
    bool playing() const { return propBool("playing"); }
    // Advance while playing; speed is in range-widths per second
    void advance(double deltaSeconds);

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
    int resolution() const { return static_cast<int>(propNumber("resolution", 60)); }
    bool heatmap() const { return propBool("heatmap", true); }
    // Range values after evaluation (fields can be expressions such as -pi)
    double rangeValue(const char* key, double fallback) const;

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
