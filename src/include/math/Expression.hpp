#pragma once

#include "math/expr/Expr.hpp"
#include <string>

namespace linalg {

// Variables an expression can refer to.
struct ExprVars {
    double x = 0.0;
    double y = 0.0;
    double t = 0.0;
};

// A scalar math expression in x, y and t, e.g. "sin(x) * exp(-y^2/2)".
// Thin wrapper over the typed engine (math/expr) for plots and numeric fields:
// other names are rejected at parse time, and non-real results evaluate to NaN.
// Optionally a resolver supplies extra names (parameters) for evaluation.
class Expression {
public:
    // Parse text. On failure returns false and error() describes the problem.
    // With allowNames, names other than x/y/t are accepted (resolved at eval time).
    bool parse(const std::string& text, bool allowNames = false);

    bool valid() const { return m_valid; }
    const std::string& error() const { return m_error; }
    const std::string& text() const { return m_program.text(); }
    const expr::Program& program() const { return m_program; }

    // Evaluate; returns NaN if the expression is invalid or not a real number.
    double eval(const ExprVars& vars, const expr::Resolver* resolver = nullptr) const;
    double eval(double x, double y = 0.0, double t = 0.0) const { return eval(ExprVars{x, y, t}); }

private:
    expr::Program m_program;
    std::string m_error;
    bool m_valid = false;
};

} // namespace linalg
