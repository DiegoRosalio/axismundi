#include "math/Expression.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace linalg {

struct Expression::Node {
    enum class Kind { Number, VarX, VarY, VarT, Negate, Add, Sub, Mul, Div, Pow, Func };

    Kind kind = Kind::Number;
    double value = 0.0;
    double (*func)(double) = nullptr;
    std::unique_ptr<Node> a, b;

    double eval(const ExprVars& v) const {
        switch (kind) {
            case Kind::Number: return value;
            case Kind::VarX:   return v.x;
            case Kind::VarY:   return v.y;
            case Kind::VarT:   return v.t;
            case Kind::Negate: return -a->eval(v);
            case Kind::Add:    return a->eval(v) + b->eval(v);
            case Kind::Sub:    return a->eval(v) - b->eval(v);
            case Kind::Mul:    return a->eval(v) * b->eval(v);
            case Kind::Div:    return a->eval(v) / b->eval(v);
            case Kind::Pow:    return std::pow(a->eval(v), b->eval(v));
            case Kind::Func:   return func(a->eval(v));
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
};

namespace {

using Node = Expression::Node;
using NodePtr = std::unique_ptr<Node>;

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

NodePtr makeNumber(double v) {
    auto n = std::make_unique<Node>();
    n->kind = Node::Kind::Number;
    n->value = v;
    return n;
}

NodePtr makeUnary(Node::Kind k, NodePtr a) {
    auto n = std::make_unique<Node>();
    n->kind = k;
    n->a = std::move(a);
    return n;
}

NodePtr makeBinary(Node::Kind k, NodePtr a, NodePtr b) {
    auto n = std::make_unique<Node>();
    n->kind = k;
    n->a = std::move(a);
    n->b = std::move(b);
    return n;
}

double (*lookupFunction(const std::string& name))(double) {
    struct Entry { const char* name; double (*fn)(double); };
    static const Entry table[] = {
        {"sin", [](double x) { return std::sin(x); }},
        {"cos", [](double x) { return std::cos(x); }},
        {"tan", [](double x) { return std::tan(x); }},
        {"asin", [](double x) { return std::asin(x); }},
        {"acos", [](double x) { return std::acos(x); }},
        {"atan", [](double x) { return std::atan(x); }},
        {"sinh", [](double x) { return std::sinh(x); }},
        {"cosh", [](double x) { return std::cosh(x); }},
        {"tanh", [](double x) { return std::tanh(x); }},
        {"exp", [](double x) { return std::exp(x); }},
        {"log", [](double x) { return std::log(x); }},
        {"ln", [](double x) { return std::log(x); }},
        {"sqrt", [](double x) { return std::sqrt(x); }},
        {"abs", [](double x) { return std::fabs(x); }},
        {"floor", [](double x) { return std::floor(x); }},
        {"ceil", [](double x) { return std::ceil(x); }},
    };
    for (const auto& e : table)
        if (name == e.name) return e.fn;
    return nullptr;
}

class Parser {
public:
    explicit Parser(const std::string& s) : m_s(s) {}

    NodePtr parseAll() {
        NodePtr n = parseExpr();
        skipSpace();
        if (m_pos < m_s.size()) fail(std::string("unexpected '") + m_s[m_pos] + "'");
        return n;
    }

private:
    [[noreturn]] void fail(const std::string& msg) const {
        throw ParseError(msg + " at position " + std::to_string(m_pos + 1));
    }

    void skipSpace() {
        while (m_pos < m_s.size() && std::isspace(static_cast<unsigned char>(m_s[m_pos]))) m_pos++;
    }

    char peek() {
        skipSpace();
        return m_pos < m_s.size() ? m_s[m_pos] : '\0';
    }

    // expr := term (('+' | '-') term)*
    NodePtr parseExpr() {
        NodePtr left = parseTerm();
        for (;;) {
            char c = peek();
            if (c == '+' || c == '-') {
                m_pos++;
                NodePtr right = parseTerm();
                left = makeBinary(c == '+' ? Node::Kind::Add : Node::Kind::Sub,
                                  std::move(left), std::move(right));
            } else {
                return left;
            }
        }
    }

    // term := unary (('*' | '/' | implicit) unary)*
    NodePtr parseTerm() {
        NodePtr left = parseUnary();
        for (;;) {
            char c = peek();
            if (c == '*' || c == '/') {
                m_pos++;
                NodePtr right = parseUnary();
                left = makeBinary(c == '*' ? Node::Kind::Mul : Node::Kind::Div,
                                  std::move(left), std::move(right));
            } else if (c == '(' || std::isalpha(static_cast<unsigned char>(c)) ||
                       std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
                // Implicit multiplication: "2x", "3(x+1)", "x y"
                NodePtr right = parsePower();
                left = makeBinary(Node::Kind::Mul, std::move(left), std::move(right));
            } else {
                return left;
            }
        }
    }

    // unary := ('-' | '+') unary | power
    NodePtr parseUnary() {
        char c = peek();
        if (c == '-') {
            m_pos++;
            return makeUnary(Node::Kind::Negate, parseUnary());
        }
        if (c == '+') {
            m_pos++;
            return parseUnary();
        }
        return parsePower();
    }

    // power := primary ('^' unary)?   (right-associative, so -x^2 = -(x^2))
    NodePtr parsePower() {
        NodePtr base = parsePrimary();
        if (peek() == '^') {
            m_pos++;
            NodePtr exponent = parseUnary();
            return makeBinary(Node::Kind::Pow, std::move(base), std::move(exponent));
        }
        return base;
    }

    NodePtr parsePrimary() {
        char c = peek();
        if (c == '\0') fail("unexpected end of expression");

        if (c == '(') {
            m_pos++;
            NodePtr inner = parseExpr();
            if (peek() != ')') fail("missing ')'");
            m_pos++;
            return inner;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            const char* start = m_s.c_str() + m_pos;
            char* end = nullptr;
            double v = std::strtod(start, &end);
            if (end == start) fail("invalid number");
            m_pos += static_cast<size_t>(end - start);
            return makeNumber(v);
        }

        if (std::isalpha(static_cast<unsigned char>(c))) {
            size_t start = m_pos;
            while (m_pos < m_s.size() && std::isalpha(static_cast<unsigned char>(m_s[m_pos]))) m_pos++;
            std::string name = m_s.substr(start, m_pos - start);

            if (auto fn = lookupFunction(name)) {
                if (peek() != '(') fail("expected '(' after " + name);
                m_pos++;
                NodePtr arg = parseExpr();
                if (peek() != ')') fail("missing ')'");
                m_pos++;
                auto n = makeUnary(Node::Kind::Func, std::move(arg));
                n->func = fn;
                return n;
            }
            if (name == "x") return makeUnary(Node::Kind::VarX, nullptr);
            if (name == "y") return makeUnary(Node::Kind::VarY, nullptr);
            if (name == "t") return makeUnary(Node::Kind::VarT, nullptr);
            if (name == "pi") return makeNumber(3.14159265358979323846);
            if (name == "e") return makeNumber(2.71828182845904523536);
            m_pos = start;
            fail("unknown name '" + name + "'");
        }

        fail(std::string("unexpected '") + c + "'");
    }

    const std::string& m_s;
    size_t m_pos = 0;
};

} // namespace

Expression::Expression() = default;
Expression::~Expression() = default;
Expression::Expression(Expression&&) noexcept = default;
Expression& Expression::operator=(Expression&&) noexcept = default;

bool Expression::parse(const std::string& text) {
    m_text = text;
    m_root.reset();
    m_error.clear();
    try {
        Parser parser(m_text);
        m_root = parser.parseAll();
        return true;
    } catch (const ParseError& e) {
        m_error = e.what();
        return false;
    }
}

double Expression::eval(const ExprVars& vars) const {
    if (!m_root) return std::numeric_limits<double>::quiet_NaN();
    return m_root->eval(vars);
}

} // namespace linalg
