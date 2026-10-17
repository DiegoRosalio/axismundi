#pragma once

#include "math/expr/Value.hpp"
#include <memory>
#include <set>
#include <string>

namespace linalg::expr {

// Looks up names used in an expression (other objects, parameters, ...)
class Resolver {
public:
    virtual ~Resolver() = default;
    // Returns false when the name is unknown
    virtual bool lookup(const std::string& name, Value& out) const = 0;
};

// Variables supplied at evaluation: x, y (plots), t (scene time), u (curve variable),
// z (vector fields)
struct Slots {
    double x = 0.0, y = 0.0, t = 0.0, u = 0.0, z = 0.0;
    bool hasX = false, hasY = false, hasT = false, hasU = false, hasZ = false;
};

// Extra variables a formula may use (Program::compile): u in curves, z in vector fields.
// Elsewhere u and z are ordinary names (objects may be called u or z).
constexpr unsigned kVarU = 1;
constexpr unsigned kVarZ = 2;

struct Node;

// A compiled expression. Grammar (lowest to highest precedence):
//   expr    := term (('+' | '-') term)*
//   term    := unary (('*' | '/' | implicit) unary)*      implicit: "2x", "3(x+1)"
//   unary   := ('-' | '+') unary | power
//   power   := postfix ('^' unary)?                       right-associative
//   postfix := primary ('.' name)*                        v.x, v.y, v.z
//   primary := number | name | name '(' args ')' | '(' expr ')' | '[' list ']'
// Lists: [a, b, c] is a vector (or a matrix whose columns are vectors a, b, c);
// [[...], [...], [...]] is a matrix given by rows (3x3 or 4x4).
// Constants: pi, e, i (imaginary unit).
class Program {
public:
    Program();
    ~Program();
    Program(Program&&) noexcept;
    Program& operator=(Program&&) noexcept;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    // Parse. On failure returns false and error() says why. `variables` (kVarU, kVarZ) makes u
    // and/or z variables (slots) instead of names; `true` means kVarU (curves).
    bool compile(const std::string& text, unsigned variables = 0);
    bool valid() const { return m_root != nullptr; }
    const std::string& error() const { return m_error; }
    const std::string& text() const { return m_text; }

    // Names the expression refers to (excluding constants, functions, x/y/t and the curve u)
    const std::set<std::string>& names() const { return m_names; }
    // Whether the plot variables appear
    bool usesX() const { return m_usesX; }
    bool usesY() const { return m_usesY; }
    bool usesT() const { return m_usesT; }
    bool usesU() const { return m_usesU; }
    bool usesZ() const { return m_usesZ; }

    Value eval(const Resolver* resolver = nullptr, const Slots* slots = nullptr) const;

private:
    std::unique_ptr<Node> m_root;
    std::string m_text;
    std::string m_error;
    std::set<std::string> m_names;
    bool m_usesX = false, m_usesY = false, m_usesT = false, m_usesU = false, m_usesZ = false;
};

// True for built-in names that objects may not use: constants, functions, x, y, t
bool isReservedName(const std::string& name);
// A valid object name: letter followed by letters, digits or '_', not reserved
bool isValidName(const std::string& name);

} // namespace linalg::expr
