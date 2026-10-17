#include "math/Expression.hpp"
#include <limits>

namespace linalg {

bool Expression::parse(const std::string& text, bool allowNames) {
    m_valid = false;
    m_error.clear();
    if (!m_program.compile(text)) {
        m_error = m_program.error();
        return false;
    }
    if (!allowNames && !m_program.names().empty()) {
        m_error = "unknown name '" + *m_program.names().begin() + "'";
        return false;
    }
    m_valid = true;
    return true;
}

double Expression::eval(const ExprVars& vars, const expr::Resolver* resolver) const {
    if (!m_valid) return std::numeric_limits<double>::quiet_NaN();
    expr::Slots slots;
    slots.x = vars.x;
    slots.y = vars.y;
    slots.t = vars.t;
    slots.hasX = slots.hasY = slots.hasT = true;
    expr::Value v = m_program.eval(resolver, &slots);
    if (!v.isRealNumber()) return std::numeric_limits<double>::quiet_NaN();
    return v.re;
}

} // namespace linalg
