#pragma once

#include "math/expr/Expr.hpp"
#include "ui/SceneIO.hpp"
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace linalg::scene {

using ObjectId = std::uint32_t;
constexpr ObjectId kNoObject = 0;

enum class SpaceKind { Euclidean3D = 0, Complex = 1 };
constexpr int kSpaceCount = 2;

struct Rgba {
    unsigned char r = 255, g = 255, b = 255, a = 255;
};

// What a field must evaluate to
enum class Expect {
    Any,
    Number,       // real number
    ComplexNum,   // real or complex number
    Vector,
    Matrix,
    Plot,         // real-valued function of x, y, t (checked with sample values)
};

// One editable input of an object, written as an expression ("[1, 0, 0]", "a + b", "cos(theta)")
struct Field {
    std::string key;     // stable id used in files ("value", "m01", "xmin")
    std::string label;   // shown in the inspector
    Expect expect = Expect::Any;
    std::string text;
    expr::Program program;
    std::string error;   // parse or evaluation error ("" when fine)
    expr::Value value;   // result of the last evaluation (not used for Plot fields)
    bool curveVariable = false;  // curve fields: `u` is the curve's own variable
};

// Base of everything that can live in a space: vectors, matrices, plots,
// complex numbers, parameters... Inputs are expression fields plus simple
// properties (flags, sizes); the evaluated result is value(), which other
// objects can use by name. Subclasses only declare fields/properties and
// how to turn field values into their value.
class SceneObject {
public:
    explicit SceneObject(std::string typeId);
    virtual ~SceneObject();

    const std::string& typeId() const { return m_typeId; }
    ObjectId id() const { return m_id; }
    const std::string& name() const { return m_name; }
    bool visible = true;
    bool customColor = false;
    Rgba color;
    int paletteIndex = 0;  // which theme color to use when not custom

    // --- Fields ---------------------------------------------------------------
    std::vector<Field>& fields() { return m_fields; }
    const std::vector<Field>& fields() const { return m_fields; }
    Field* field(const std::string& key);
    const Field* field(const std::string& key) const;
    // Set a field's text and compile it; returns false if it does not parse
    bool setField(const std::string& key, const std::string& text);
    // Fields that currently matter (e.g. matrix cells vs. a single formula)
    virtual bool fieldActive(const Field& f) const;
    // Variables (x, y, t) an object supplies to its own fields, e.g. a parameter's
    // clock t. Return false for none (the default).
    virtual bool provideSlots(expr::Slots& slots) const;

    // Scene time in seconds (set by the scene before each evaluation). Not saved.
    void setTime(double seconds) { m_time = seconds; }
    // True when an active field uses t (the object follows the scene clock)
    bool usesTime() const;
    double time() const { return m_time; }

    // --- Properties (non-expression settings, saved as text) -------------------
    std::string prop(const std::string& key, const std::string& fallback = "") const;
    double propNumber(const std::string& key, double fallback = 0.0) const;
    bool propBool(const std::string& key, bool fallback = false) const;
    void setProp(const std::string& key, const std::string& value);
    void setProp(const std::string& key, double value);
    void setProp(const std::string& key, bool value);
    void removeProp(const std::string& key);
    const std::map<std::string, std::string>& props() const { return m_props; }

    // --- Evaluation --------------------------------------------------------------
    // Names used by active fields
    std::set<std::string> dependencies() const;
    // Evaluate all active fields, then compute value(); `epoch` is the space's
    // evaluation counter, used to tell whether anything changed
    void evaluate(const expr::Resolver& resolver, std::uint64_t epoch, bool inputsChanged);
    // Result visible to other objects (an Error for objects without a value, e.g. plots)
    const expr::Value& value() const { return m_value; }
    // First problem, prefixed with the field label ("Value: unknown name 'q'"), or ""
    const std::string& error() const { return m_error; }
    void setError(const std::string& error) { m_error = error; }
    // Epoch of the last change to fields, properties or the result
    std::uint64_t changedAt() const { return m_changedAt; }
    void touch() { m_dirty = true; }
    bool dirty() const { return m_dirty; }

    // Copy with the same fields and properties (the space assigns id and name)
    std::unique_ptr<SceneObject> clone() const;

protected:
    void addField(const std::string& key, const std::string& label, Expect expect, const std::string& text);
    // Make `u` the curve variable in this field (recompiles it)
    void markCurveVariable(const std::string& key);
    // Turn evaluated fields into the object's value (default: the "value" field)
    virtual expr::Value computeValue() const;
    virtual std::unique_ptr<SceneObject> makeEmpty() const = 0;

private:
    friend class Space;

    std::string m_typeId;
    ObjectId m_id = kNoObject;
    std::string m_name;
    std::vector<Field> m_fields;
    std::map<std::string, std::string> m_props;
    expr::Value m_value;
    std::string m_error;
    std::uint64_t m_changedAt = 0;
    bool m_dirty = true;
    double m_time = 0.0;
    double m_evaluatedTime = -1.0;  // time of the last evaluation (fields using t change with it)
};

// Does a value satisfy an expectation? Returns "" or an error message.
std::string checkExpect(Expect expect, const expr::Value& v);

} // namespace linalg::scene
