#pragma once

#include <memory>
#include <string>

namespace linalg {

// Variables an expression can refer to.
struct ExprVars {
    double x = 0.0;
    double y = 0.0;
    double t = 0.0;
};

// A parsed math expression, e.g. "sin(x) * exp(-y^2/2)".
//
// Supports + - * / ^ (right-associative), unary minus, parentheses,
// implicit multiplication like "2x" or "3(x+1)", the variables x y t,
// the constants pi and e, and the functions
// sin cos tan asin acos atan sinh cosh tanh exp log ln sqrt abs floor ceil.
class Expression {
public:
    struct Node;

    Expression();
    ~Expression();
    Expression(Expression&&) noexcept;
    Expression& operator=(Expression&&) noexcept;

    // Parse text. On failure returns false and error() describes the problem;
    // the previous expression is discarded either way.
    bool parse(const std::string& text);

    bool valid() const { return m_root != nullptr; }
    const std::string& error() const { return m_error; }
    const std::string& text() const { return m_text; }

    // Evaluate; returns NaN if the expression is invalid.
    double eval(const ExprVars& vars) const;
    double eval(double x, double y = 0.0, double t = 0.0) const { return eval(ExprVars{x, y, t}); }

private:
    std::unique_ptr<Node> m_root;
    std::string m_error;
    std::string m_text;
};

} // namespace linalg
