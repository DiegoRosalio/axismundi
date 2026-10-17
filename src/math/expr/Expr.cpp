#include "math/expr/Expr.hpp"
#include "math/Quaternion.hpp"
#include "math/expr/Reference.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace linalg::expr {

// ---------------------------------------------------------------------------
// AST
// ---------------------------------------------------------------------------

struct Node {
    enum class Kind { Number, Imaginary, Name, Negate, Binary, Call, Member, List, Rows };
    Kind kind = Kind::Number;
    double number = 0.0;
    char op = 0;                 // Binary: + - * / ^
    std::string name;            // Name, Call, Member
    int slot = -1;               // Name: 0 = x, 1 = y, 2 = t, 3 = u (curves)
    std::vector<std::unique_ptr<Node>> children;  // operands, arguments, list items
    std::vector<int> rowSizes;   // Rows: number of items in each row
};

namespace {

using NodePtr = std::unique_ptr<Node>;
using cplx = std::complex<double>;

constexpr double kPi = 3.14159265358979323846;

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Built-in functions and how many arguments they accept (min, max)
struct FunctionInfo {
    const char* name;
    int minArgs, maxArgs;
    // Common words added later (angle, axis...): an object may also have this name; the
    // function is meant only when '(' follows
    bool nameable = false;
};

const FunctionInfo kFunctions[] = {
    {"sin", 1, 1}, {"cos", 1, 1}, {"tan", 1, 1}, {"asin", 1, 1}, {"acos", 1, 1}, {"atan", 1, 1},
    {"sinh", 1, 1}, {"cosh", 1, 1}, {"tanh", 1, 1}, {"exp", 1, 1}, {"log", 1, 1}, {"ln", 1, 1},
    {"sqrt", 1, 1}, {"abs", 1, 1}, {"floor", 1, 1}, {"ceil", 1, 1}, {"atan2", 2, 2}, {"min", 2, 2},
    {"max", 2, 2}, {"mod", 2, 2},
    // blending (any value kind)
    {"lerp", 3, 3}, {"keys", 3, 33},
    // vectors
    {"dot", 2, 2}, {"cross", 2, 2}, {"norm", 1, 1}, {"normalize", 1, 1}, {"unit", 1, 1},
    // matrices
    {"det", 1, 1}, {"tr", 1, 1}, {"trace", 1, 1}, {"inv", 1, 1}, {"transpose", 1, 1}, {"rotx", 1, 1},
    {"roty", 1, 1}, {"rotz", 1, 1}, {"rot", 2, 2}, {"scale", 1, 3}, {"translate", 1, 3}, {"eye", 0, 1},
    // complex
    {"conj", 1, 1}, {"re", 1, 1}, {"im", 1, 1}, {"arg", 1, 1}, {"polar", 2, 2},
    // quaternions (rotations)
    {"quat", 1, 4}, {"qaxis", 2, 2}, {"rotate", 2, 2, true}, {"mat", 1, 1, true}, {"axis", 1, 1, true},
    {"angle", 1, 1, true}, {"slerp", 3, 3},
};

} // namespace

std::vector<std::string> functionNames() {
    std::vector<std::string> names;
    for (const auto& f : kFunctions) names.push_back(f.name);
    return names;
}

namespace {

const FunctionInfo* findFunction(const std::string& name) {
    for (const auto& f : kFunctions)
        if (name == f.name) return &f;
    return nullptr;
}

NodePtr make(Node::Kind kind) {
    auto n = std::make_unique<Node>();
    n->kind = kind;
    return n;
}

bool isNameStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isNameChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

class Parser {
public:
    explicit Parser(const std::string& s, unsigned variables = 0)
        : m_s(s), m_curve((variables & kVarU) != 0), m_fieldZ((variables & kVarZ) != 0) {}

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

    void expect(char c, const char* what) {
        if (peek() != c) fail(std::string("missing ") + what);
        m_pos++;
    }

    NodePtr binary(char op, NodePtr a, NodePtr b) {
        auto n = make(Node::Kind::Binary);
        n->op = op;
        n->children.push_back(std::move(a));
        n->children.push_back(std::move(b));
        return n;
    }

    NodePtr parseExpr() {
        NodePtr left = parseTerm();
        for (;;) {
            char c = peek();
            if (c == '+' || c == '-') {
                m_pos++;
                left = binary(c, std::move(left), parseTerm());
            } else {
                return left;
            }
        }
    }

    NodePtr parseTerm() {
        NodePtr left = parseUnary();
        for (;;) {
            char c = peek();
            if (c == '*' || c == '/') {
                m_pos++;
                left = binary(c, std::move(left), parseUnary());
            } else if (c == '(' || c == '[' || isNameStart(c) || std::isdigit(static_cast<unsigned char>(c)) ||
                       c == '.') {
                // Implicit multiplication: "2x", "3(x+1)", "x y", "2[1, 0, 0]"
                left = binary('*', std::move(left), parsePower());
            } else {
                return left;
            }
        }
    }

    NodePtr parseUnary() {
        char c = peek();
        if (c == '-') {
            m_pos++;
            auto n = make(Node::Kind::Negate);
            n->children.push_back(parseUnary());
            return n;
        }
        if (c == '+') {
            m_pos++;
            return parseUnary();
        }
        return parsePower();
    }

    NodePtr parsePower() {
        NodePtr base = parsePostfix();
        if (peek() == '^') {
            m_pos++;
            return binary('^', std::move(base), parseUnary());
        }
        return base;
    }

    NodePtr parsePostfix() {
        NodePtr n = parsePrimary();
        // Member access: v.x — only when a name follows the dot
        while (peek() == '.' && m_pos + 1 < m_s.size() && isNameStart(m_s[m_pos + 1])) {
            m_pos++;
            size_t start = m_pos;
            while (m_pos < m_s.size() && isNameChar(m_s[m_pos])) m_pos++;
            auto member = make(Node::Kind::Member);
            member->name = m_s.substr(start, m_pos - start);
            if (member->name != "x" && member->name != "y" && member->name != "z" && member->name != "w") {
                m_pos = start;
                fail("unknown component '." + member->name + "' (use .x, .y, .z, or .w for quaternions)");
            }
            member->children.push_back(std::move(n));
            n = std::move(member);
        }
        return n;
    }

    NodePtr parseList() {
        // '[' already consumed
        if (peek() == '[') {
            // Matrix given by rows: [[a, b, c], [d, e, f], ...]
            auto rows = make(Node::Kind::Rows);
            for (;;) {
                expect('[', "'['");
                int count = 0;
                for (;;) {
                    rows->children.push_back(parseExpr());
                    count++;
                    if (peek() == ',') {
                        m_pos++;
                        continue;
                    }
                    break;
                }
                expect(']', "']'");
                rows->rowSizes.push_back(count);
                if (peek() == ',') {
                    m_pos++;
                    continue;
                }
                break;
            }
            expect(']', "']'");
            return rows;
        }
        auto list = make(Node::Kind::List);
        if (peek() == ']') fail("empty list");
        for (;;) {
            list->children.push_back(parseExpr());
            if (peek() == ',') {
                m_pos++;
                continue;
            }
            break;
        }
        expect(']', "']'");
        return list;
    }

    NodePtr parsePrimary() {
        char c = peek();
        if (c == '\0') fail("unexpected end of expression");

        if (c == '(') {
            m_pos++;
            NodePtr inner = parseExpr();
            expect(')', "')'");
            return inner;
        }
        if (c == '[') {
            m_pos++;
            return parseList();
        }
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            const char* start = m_s.c_str() + m_pos;
            char* end = nullptr;
            double v = std::strtod(start, &end);
            if (end == start) fail("invalid number");
            m_pos += static_cast<size_t>(end - start);
            auto n = make(Node::Kind::Number);
            n->number = v;
            return n;
        }
        if (isNameStart(c)) {
            size_t start = m_pos;
            while (m_pos < m_s.size() && isNameChar(m_s[m_pos])) m_pos++;
            std::string name = m_s.substr(start, m_pos - start);

            const FunctionInfo* f = findFunction(name);
            if (f && f->nameable && peek() != '(') f = nullptr;  // an object with that name
            if (f) {
                if (peek() != '(') fail("expected '(' after " + name);
                m_pos++;
                auto call = make(Node::Kind::Call);
                call->name = name;
                if (peek() != ')') {
                    for (;;) {
                        call->children.push_back(parseExpr());
                        if (peek() == ',') {
                            m_pos++;
                            continue;
                        }
                        break;
                    }
                }
                expect(')', "')'");
                int n = static_cast<int>(call->children.size());
                if (n < f->minArgs || n > f->maxArgs) {
                    std::string count = f->minArgs == f->maxArgs ? std::to_string(f->minArgs)
                                                                  : std::to_string(f->minArgs) + " to " +
                                                                        std::to_string(f->maxArgs);
                    fail(name + " takes " + count + " argument" + (f->maxArgs == 1 ? "" : "s"));
                }
                return call;
            }
            if (name == "pi") {
                auto n = make(Node::Kind::Number);
                n->number = kPi;
                return n;
            }
            if (name == "e") {
                auto n = make(Node::Kind::Number);
                n->number = 2.71828182845904523536;
                return n;
            }
            if (name == "i") return make(Node::Kind::Imaginary);

            auto n = make(Node::Kind::Name);
            n->name = name;
            if (name == "x") n->slot = 0;
            else if (name == "y") n->slot = 1;
            else if (name == "t") n->slot = 2;
            else if (name == "u" && m_curve) n->slot = 3;
            else if (name == "z" && m_fieldZ) n->slot = 4;
            return n;
        }
        fail(std::string("unexpected '") + c + "'");
    }

    const std::string& m_s;
    bool m_curve = false;
    bool m_fieldZ = false;
    size_t m_pos = 0;
};

void collectNames(const Node& n, std::set<std::string>& names, bool& x, bool& y, bool& t, bool& u, bool& z) {
    if (n.kind == Node::Kind::Name) {
        if (n.slot == 0) x = true;
        else if (n.slot == 1) y = true;
        else if (n.slot == 2) t = true;
        else if (n.slot == 3) u = true;
        else if (n.slot == 4) z = true;
        else names.insert(n.name);
    }
    for (const auto& c : n.children) collectNames(*c, names, x, y, t, u, z);
}

// ---------------------------------------------------------------------------
// Evaluation
// ---------------------------------------------------------------------------

cplx toComplex(const Value& v) { return {v.re, v.type == Type::Complex ? v.im : 0.0}; }

Value fromComplex(cplx z) {
    return Value::complex(z.real(), z.imag());
}

// Keep real results real: complex arithmetic only when a complex is involved
Value number(cplx z, bool anyComplex) {
    if (!anyComplex) return Value::scalar(z.real());
    return fromComplex(z);
}

std::string describe(const Value& v) {
    if (v.type == Type::Matrix) return v.size == 4 ? "4×4 matrix" : "3×3 matrix";
    return "a " + typeName(v.type);
}

Value typeError(char op, const Value& a, const Value& b) {
    std::string sym(1, op);
    if (op == '*' && a.type == Type::Vector && b.type == Type::Vector)
        return Value::fail("vector × vector: use dot(a, b) or cross(a, b)");
    return Value::fail("cannot compute " + describe(a) + " " + sym + " " + describe(b));
}

Matrix4D promote(const Value& m) { return m.matrix4(); }

Value matrixResult(const Matrix4D& m, int size) {
    if (size == 4) return Value::mat4(m);
    Matrix3D r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r.m[i][j] = m.m[i][j];
    return Value::mat3(r);
}

Value scaleMatrix(const Value& m, double s) {
    Matrix4D r = m.matrix;
    int n = m.size;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) r.m[i][j] = static_cast<float>(r.m[i][j] * s);
    Value v = Value::mat4(r);
    v.size = n;
    return v;
}

// --- Quaternions (w + xi + yj + zk), in double precision ---------------------------
struct Quat {
    double w, x, y, z;
};
Quat toQuat(const Value& v) { return {v.re, v.qi, v.qj, v.qk}; }
Value fromQuat(const Quat& q) { return Value::quat(q.w, q.x, q.y, q.z); }
Quat qmul(const Quat& a, const Quat& b) {
    // Hamilton product: i·j = k, j·k = i, k·i = j, i² = j² = k² = −1
    return {a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x, a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w};
}
double qnorm(const Quat& q) { return std::sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z); }
Quat qconj(const Quat& q) { return {q.w, -q.x, -q.y, -q.z}; }
Quat qscale(const Quat& q, double s) { return {q.w * s, q.x * s, q.y * s, q.z * s}; }
bool qinverse(const Quat& q, Quat& out) {
    double n2 = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
    if (n2 < 1e-24) return false;
    out = qscale(qconj(q), 1.0 / n2);
    return true;
}
// Rotation matrix of q/|q|
Matrix3D qmatrix(const Quat& q0) {
    double n = qnorm(q0);
    Quat q = qscale(q0, 1.0 / n);
    double w = q.w, x = q.x, y = q.y, z = q.z;
    auto f = [](double v) { return static_cast<float>(v); };
    return Matrix3D({{
        {{f(1 - 2 * (y * y + z * z)), f(2 * (x * y - w * z)), f(2 * (x * z + w * y))}},
        {{f(2 * (x * y + w * z)), f(1 - 2 * (x * x + z * z)), f(2 * (y * z - w * x))}},
        {{f(2 * (x * z - w * y)), f(2 * (y * z + w * x)), f(1 - 2 * (x * x + y * y))}},
    }});
}
Quat qslerp(Quat a, Quat b, double s) {
    a = qscale(a, 1.0 / qnorm(a));
    b = qscale(b, 1.0 / qnorm(b));
    double d = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
    if (d < 0) {  // the shorter way round (q and −q are the same rotation)
        b = qscale(b, -1.0);
        d = -d;
    }
    double ka, kb;
    if (d > 0.9995) {
        ka = 1 - s;
        kb = s;
    } else {
        double th = std::acos(std::min(1.0, d)), sn = std::sin(th);
        ka = std::sin((1 - s) * th) / sn;
        kb = std::sin(s * th) / sn;
    }
    Quat r = {ka * a.w + kb * b.w, ka * a.x + kb * b.x, ka * a.y + kb * b.y, ka * a.z + kb * b.z};
    return qscale(r, 1.0 / qnorm(r));
}

Value quaternionOp(char op, const Value& a, const Value& b) {
    bool qa = a.type == Type::Quaternion, qb = b.type == Type::Quaternion;
    // A real number is the quaternion w + 0i + 0j + 0k
    Quat x = qa ? toQuat(a) : Quat{a.re, 0, 0, 0};
    Quat y = qb ? toQuat(b) : Quat{b.re, 0, 0, 0};
    switch (op) {
        case '+': return fromQuat({x.w + y.w, x.x + y.x, x.y + y.y, x.z + y.z});
        case '-': return fromQuat({x.w - y.w, x.x - y.x, x.y - y.y, x.z - y.z});
        case '*': return fromQuat(qmul(x, y));
        case '/': {
            Quat inv;
            if (!qinverse(y, inv)) return Value::fail("division by the zero quaternion");
            return fromQuat(qmul(x, inv));
        }
        case '^': {
            if (!qa || qb || std::fabs(b.re - std::round(b.re)) > 1e-9 || std::fabs(b.re) > 64)
                return Value::fail("quaternion powers must be whole numbers (q^2, q^-1)");
            int n = static_cast<int>(std::lround(b.re));
            Quat base = x;
            if (n < 0) {
                if (!qinverse(x, base)) return Value::fail("the zero quaternion has no inverse");
                n = -n;
            }
            Quat r = {1, 0, 0, 0};
            for (int k = 0; k < n; k++) r = qmul(r, base);
            return fromQuat(r);
        }
    }
    return Value::fail("cannot compute that with quaternions");
}

Value binaryOp(char op, const Value& a, const Value& b) {
    if (!a.ok()) return a;
    if (!b.ok()) return b;

    // Quaternions, with real numbers
    if (a.type == Type::Quaternion || b.type == Type::Quaternion) {
        const Value& other = a.type == Type::Quaternion ? b : a;
        if (other.type == Type::Quaternion || other.isRealNumber()) return quaternionOp(op, a, b);
        if (other.type == Type::Vector) return Value::fail("quaternion and vector: use rotate(q, v) to turn a vector");
        if (other.type == Type::Matrix) return Value::fail("quaternion and matrix: use mat(q) to get its matrix");
        return Value::fail("quaternions and complex numbers don't mix: write quat(re, im, 0, 0)");
    }

    // Numbers (real or complex)
    if (a.isNumber() && b.isNumber()) {
        bool anyComplex = a.type == Type::Complex || b.type == Type::Complex;
        if (!anyComplex) {
            double x = a.re, y = b.re;
            switch (op) {
                case '+': return Value::scalar(x + y);
                case '-': return Value::scalar(x - y);
                case '*': return Value::scalar(x * y);
                case '/': return Value::scalar(x / y);
                case '^': return Value::scalar(std::pow(x, y));
            }
        }
        cplx x = toComplex(a), y = toComplex(b);
        switch (op) {
            case '+': return number(x + y, true);
            case '-': return number(x - y, true);
            case '*': return number(x * y, true);
            case '/': return number(x / y, true);
            case '^':
                // Integer powers stay exact (i^2 = -1)
                if (b.isRealNumber() && std::fabs(b.re - std::round(b.re)) < 1e-12 && std::fabs(b.re) <= 64) {
                    cplx r = 1.0;
                    int n = static_cast<int>(std::lround(b.re));
                    for (int k = 0; k < std::abs(n); k++) r *= x;
                    return number(n < 0 ? 1.0 / r : r, true);
                }
                return number(std::pow(x, y), true);
        }
    }

    // Vectors
    if (a.type == Type::Vector && b.type == Type::Vector) {
        if (op == '+') return Value::vec(a.vector + b.vector);
        if (op == '-') return Value::vec(a.vector - b.vector);
        return typeError(op, a, b);
    }
    if (a.type == Type::Vector && b.isRealNumber()) {
        if (op == '*') return Value::vec(a.vector * static_cast<float>(b.re));
        if (op == '/') return Value::vec(a.vector / static_cast<float>(b.re));
        return typeError(op, a, b);
    }
    if (a.isRealNumber() && b.type == Type::Vector && op == '*') {
        return Value::vec(b.vector * static_cast<float>(a.re));
    }

    // Matrices
    if (a.type == Type::Matrix && b.type == Type::Matrix) {
        int n = std::max(a.size, b.size);
        Matrix4D x = promote(a), y = promote(b);
        if (op == '+' || op == '-') {
            if (a.size != b.size) return Value::fail("matrix sizes differ (3×3 and 4×4)");
            Matrix4D r = op == '+' ? x + y : x - y;
            return matrixResult(r, n);
        }
        if (op == '*') return matrixResult(x * y, n);
        return typeError(op, a, b);
    }
    if (a.type == Type::Matrix && b.isRealNumber()) {
        if (op == '*') return scaleMatrix(a, b.re);
        if (op == '/') return scaleMatrix(a, 1.0 / b.re);
        if (op == '^') {
            if (std::fabs(b.re - std::round(b.re)) > 1e-9) return Value::fail("matrix powers must be whole numbers");
            int n = static_cast<int>(std::lround(b.re));
            Matrix4D base = promote(a);
            if (n < 0) {
                auto inv = base.inverse();
                if (!inv) return Value::fail("matrix is not invertible (det = 0)");
                base = *inv;
                n = -n;
            }
            Matrix4D r = Matrix4D::identity();
            for (int k = 0; k < std::min(n, 64); k++) r = r * base;
            return matrixResult(r, a.size);
        }
        return typeError(op, a, b);
    }
    if (a.isRealNumber() && b.type == Type::Matrix && op == '*') return scaleMatrix(b, a.re);
    if (a.type == Type::Matrix && b.type == Type::Vector && op == '*') {
        if (a.size == 4) return Value::vec(a.matrix.transform(b.vector));  // point with w = 1
        return Value::vec(a.matrix3().transform(b.vector));
    }
    return typeError(op, a, b);
}

Value needNumber(const Value& v, const char* fn) {
    if (!v.ok()) return v;
    if (!v.isNumber()) return Value::fail(std::string(fn) + " needs a number, not " + describe(v));
    return v;
}

Value callFunction(const std::string& name, const std::vector<Value>& args) {
    for (const auto& a : args)
        if (!a.ok()) return a;

    auto arg = [&](size_t i) -> const Value& { return args[i]; };
    auto realArg = [&](size_t i, double& out) {
        if (!arg(i).isRealNumber()) return false;
        out = arg(i).re;
        return true;
    };

    // Elementary functions on numbers (complex-aware)
    struct Elementary {
        const char* name;
        double (*real)(double);
        cplx (*complexFn)(const cplx&);
    };
    static const Elementary elementary[] = {
        {"sin", [](double x) { return std::sin(x); }, [](const cplx& z) { return std::sin(z); }},
        {"cos", [](double x) { return std::cos(x); }, [](const cplx& z) { return std::cos(z); }},
        {"tan", [](double x) { return std::tan(x); }, [](const cplx& z) { return std::tan(z); }},
        {"asin", [](double x) { return std::asin(x); }, [](const cplx& z) { return std::asin(z); }},
        {"acos", [](double x) { return std::acos(x); }, [](const cplx& z) { return std::acos(z); }},
        {"atan", [](double x) { return std::atan(x); }, [](const cplx& z) { return std::atan(z); }},
        {"sinh", [](double x) { return std::sinh(x); }, [](const cplx& z) { return std::sinh(z); }},
        {"cosh", [](double x) { return std::cosh(x); }, [](const cplx& z) { return std::cosh(z); }},
        {"tanh", [](double x) { return std::tanh(x); }, [](const cplx& z) { return std::tanh(z); }},
        {"exp", [](double x) { return std::exp(x); }, [](const cplx& z) { return std::exp(z); }},
        {"log", [](double x) { return std::log(x); }, [](const cplx& z) { return std::log(z); }},
        {"ln", [](double x) { return std::log(x); }, [](const cplx& z) { return std::log(z); }},
        {"sqrt", [](double x) { return std::sqrt(x); }, [](const cplx& z) { return std::sqrt(z); }},
        {"floor", [](double x) { return std::floor(x); }, nullptr},
        {"ceil", [](double x) { return std::ceil(x); }, nullptr},
    };
    for (const auto& e : elementary) {
        if (name != e.name) continue;
        Value v = needNumber(arg(0), e.name);
        if (!v.ok()) return v;
        if (v.type == Type::Complex) {
            if (!e.complexFn) return Value::fail(name + " needs a real number");
            return fromComplex(e.complexFn(toComplex(v)));
        }
        return Value::scalar(e.real(v.re));
    }

    double x = 0, y = 0;
    if (name == "abs") {
        const Value& v = arg(0);
        if (v.type == Type::Scalar) return Value::scalar(std::fabs(v.re));
        if (v.type == Type::Complex) return Value::scalar(std::abs(toComplex(v)));
        if (v.type == Type::Vector) return Value::scalar(v.vector.magnitude());
        if (v.type == Type::Quaternion) return Value::scalar(qnorm(toQuat(v)));
        return Value::fail("abs needs a number, a vector or a quaternion");
    }
    if (name == "mod") {
        if (!realArg(0, x) || !realArg(1, y)) return Value::fail("mod needs two real numbers");
        if (y == 0.0) return Value::fail("mod: division by zero");
        return Value::scalar(x - y * std::floor(x / y));
    }
    if (name == "lerp" || name == "keys") {
        double s = 0;
        if (!realArg(0 + (name == "lerp" ? 2 : 0), s)) return Value::fail(name + ": the blend amount must be a real number");
        // lerp(a, b, s) = a + (b - a)·s; a plain 0 stands for "zero" of the other value's kind
        auto blend = [&](const Value& a, const Value& b, double u) -> Value {
            if (!a.ok()) return a;
            if (!b.ok()) return b;
            bool aZero = a.type == Type::Scalar && a.re == 0.0, bZero = b.type == Type::Scalar && b.re == 0.0;
            if (aZero && b.type != Type::Scalar) return binaryOp('*', Value::scalar(u), b);
            if (bZero && a.type != Type::Scalar) return binaryOp('*', Value::scalar(1.0 - u), a);
            if (a.type != b.type && !(a.isNumber() && b.isNumber()))
                return Value::fail(name + " needs values of the same kind (got " + a.typeName() + " and " +
                                   b.typeName() + ")");
            Value diff = binaryOp('-', b, a);
            if (!diff.ok()) return diff;
            Value step = binaryOp('*', Value::scalar(u), diff);
            if (!step.ok()) return step;
            return binaryOp('+', a, step);
        };
        if (name == "lerp") {
            // Rotations blend along the shortest arc (slerp)
            if (arg(0).type == Type::Quaternion && arg(1).type == Type::Quaternion)
                return fromQuat(qslerp(toQuat(arg(0)), toQuat(arg(1)), s));
            return blend(arg(0), arg(1), s);
        }
        // keys(s, A, B, C, ...): s from 0 to 1 runs through the values in order
        size_t n = args.size() - 1;
        double pos = std::clamp(s, 0.0, 1.0) * static_cast<double>(n - 1);
        size_t i = std::min(static_cast<size_t>(pos), n - 2);
        if (arg(1 + i).type == Type::Quaternion && arg(2 + i).type == Type::Quaternion)
            return fromQuat(qslerp(toQuat(arg(1 + i)), toQuat(arg(2 + i)), pos - static_cast<double>(i)));
        return blend(arg(1 + i), arg(2 + i), pos - static_cast<double>(i));
    }
    if (name == "atan2" || name == "min" || name == "max") {
        if (!realArg(0, x) || !realArg(1, y)) return Value::fail(name + " needs two real numbers");
        if (name == "atan2") return Value::scalar(std::atan2(x, y));
        return Value::scalar(name == "min" ? std::min(x, y) : std::max(x, y));
    }

    // Vectors
    if (name == "dot" || name == "cross") {
        if (arg(0).type != Type::Vector || arg(1).type != Type::Vector) return Value::fail(name + " needs two vectors");
        if (name == "dot") return Value::scalar(arg(0).vector.dot(arg(1).vector));
        return Value::vec(arg(0).vector.cross(arg(1).vector));
    }
    if (name == "norm") {
        if (arg(0).type == Type::Vector) return Value::scalar(arg(0).vector.magnitude());
        if (arg(0).isNumber()) return Value::scalar(std::abs(toComplex(arg(0))));
        if (arg(0).type == Type::Quaternion) return Value::scalar(qnorm(toQuat(arg(0))));
        return Value::fail("norm needs a vector");
    }
    if (name == "normalize" || name == "unit") {
        if (arg(0).type == Type::Quaternion) {
            Quat q = toQuat(arg(0));
            double n = qnorm(q);
            if (n < 1e-12) return Value::fail("the zero quaternion has no direction");
            return fromQuat(qscale(q, 1.0 / n));
        }
        if (arg(0).type != Type::Vector) return Value::fail(name + " needs a vector");
        if (arg(0).vector.magnitude() < 1e-12f) return Value::fail("the zero vector has no direction");
        return Value::vec(arg(0).vector.normalized());
    }

    // Matrices
    if (name == "inv" && arg(0).type == Type::Quaternion) {
        Quat r;
        if (!qinverse(toQuat(arg(0)), r)) return Value::fail("the zero quaternion has no inverse");
        return fromQuat(r);
    }
    if (name == "det" || name == "tr" || name == "trace" || name == "inv" || name == "transpose") {
        const Value& m = arg(0);
        if (m.type != Type::Matrix) return Value::fail(name + " needs a matrix");
        if (name == "det") return Value::scalar(m.size == 4 ? m.matrix.determinant() : m.matrix3().determinant());
        if (name == "tr" || name == "trace") return Value::scalar(m.size == 4 ? m.matrix.trace() : m.matrix3().trace());
        if (name == "transpose") return matrixResult(m.matrix4().transposed(), m.size);
        if (m.size == 4) {
            auto inv = m.matrix.inverse();
            if (!inv) return Value::fail("matrix is not invertible (det = 0)");
            return Value::mat4(*inv);
        }
        auto inv = m.matrix3().inverse();
        if (!inv) return Value::fail("matrix is not invertible (det = 0)");
        return Value::mat3(*inv);
    }
    if (name == "rotx" || name == "roty" || name == "rotz") {
        if (!realArg(0, x)) return Value::fail(name + " needs an angle in radians");
        Matrix4D r = name == "rotx" ? Matrix4D::rotationX(static_cast<float>(x))
                   : name == "roty" ? Matrix4D::rotationY(static_cast<float>(x))
                                    : Matrix4D::rotationZ(static_cast<float>(x));
        return matrixResult(r, 3);
    }
    if (name == "rot") {
        if (arg(0).type != Type::Vector || !realArg(1, x)) return Value::fail("rot needs an axis vector and an angle");
        Vector3D a = arg(0).vector;
        if (a.magnitude() < 1e-12f) return Value::fail("rot needs a non-zero axis");
        a = a.normalized();
        // Rodrigues' formula
        float c = static_cast<float>(std::cos(x)), s = static_cast<float>(std::sin(x)), k = 1 - c;
        Matrix3D r({{
            {{c + a.x * a.x * k, a.x * a.y * k - a.z * s, a.x * a.z * k + a.y * s}},
            {{a.y * a.x * k + a.z * s, c + a.y * a.y * k, a.y * a.z * k - a.x * s}},
            {{a.z * a.x * k - a.y * s, a.z * a.y * k + a.x * s, c + a.z * a.z * k}},
        }});
        return Value::mat3(r);
    }
    if (name == "scale") {
        double s[3];
        if (args.size() == 1 && arg(0).isRealNumber()) {
            s[0] = s[1] = s[2] = arg(0).re;
        } else if (args.size() == 1 && arg(0).type == Type::Vector) {
            s[0] = arg(0).vector.x;
            s[1] = arg(0).vector.y;
            s[2] = arg(0).vector.z;
        } else if (args.size() == 3 && realArg(0, s[0]) && realArg(1, s[1]) && realArg(2, s[2])) {
        } else {
            return Value::fail("scale needs a factor, a vector or three factors");
        }
        return matrixResult(Matrix4D::scale(static_cast<float>(s[0]), static_cast<float>(s[1]), static_cast<float>(s[2])), 3);
    }
    if (name == "translate") {
        Vector3D t;
        double a, b, c;
        if (args.size() == 1 && arg(0).type == Type::Vector) t = arg(0).vector;
        else if (args.size() == 3 && realArg(0, a) && realArg(1, b) && realArg(2, c))
            t = Vector3D(static_cast<float>(a), static_cast<float>(b), static_cast<float>(c));
        else return Value::fail("translate needs a vector or three numbers");
        return Value::mat4(Matrix4D::translation(t.x, t.y, t.z));
    }
    if (name == "eye") {
        double n = 3;
        if (!args.empty() && !realArg(0, n)) return Value::fail("eye needs 3 or 4");
        if (std::lround(n) == 4) return Value::mat4(Matrix4D::identity());
        if (std::lround(n) == 3) return Value::mat3(Matrix3D::identity());
        return Value::fail("eye needs 3 or 4");
    }

    // Quaternions
    if (name == "conj" && arg(0).type == Type::Quaternion) return fromQuat(qconj(toQuat(arg(0))));
    if (name == "quat") {
        if (args.size() == 4) {
            double c[4];
            for (int k = 0; k < 4; k++)
                if (!realArg(k, c[k])) return Value::fail("quat(w, x, y, z) needs four real numbers");
            return Value::quat(c[0], c[1], c[2], c[3]);
        }
        if (args.size() == 1 && arg(0).type == Type::Matrix) {
            Matrix3D m = arg(0).matrix3();
            // Only rotation matrices have a quaternion: columns of length 1, at right angles, det = +1
            Matrix3D mtm = m.transposed() * m;
            double off = 0;
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++) off = std::max(off, std::fabs(mtm.m[i][j] - (i == j ? 1.0 : 0.0)));
            if (off > 1e-3 || m.determinant() < 0)
                return Value::fail("quat(M) needs a rotation matrix (columns of length 1 at right angles, det = 1)");
            Quaternion q = Quaternion::fromMatrix(m).normalized();
            if (q.w < 0) q = {-q.w, -q.x, -q.y, -q.z};  // the one with w ≥ 0
            return Value::quat(q.w, q.x, q.y, q.z);
        }
        if (args.size() == 1 && arg(0).isRealNumber()) return Value::quat(arg(0).re, 0, 0, 0);
        return Value::fail("quat needs (w, x, y, z) or a rotation matrix");
    }
    if (name == "qaxis") {
        if (arg(0).type != Type::Vector || !realArg(1, x)) return Value::fail("qaxis needs an axis vector and an angle");
        Vector3D a = arg(0).vector;
        double len = a.magnitude();
        if (len < 1e-12) return Value::fail("qaxis needs a non-zero axis");
        double s = std::sin(x / 2) / len;
        return Value::quat(std::cos(x / 2), a.x * s, a.y * s, a.z * s);
    }
    if (name == "rotate" || name == "mat" || name == "axis" || name == "angle") {
        if (arg(0).type != Type::Quaternion) return Value::fail(name + " needs a quaternion first, e.g. qaxis([0, 0, 1], pi/2)");
        Quat q = toQuat(arg(0));
        double n = qnorm(q);
        if (n < 1e-12) return Value::fail("the zero quaternion is not a rotation");
        q = qscale(q, 1.0 / n);
        if (name == "mat") return Value::mat3(qmatrix(q));
        if (name == "rotate") {
            if (args.size() < 2 || arg(1).type != Type::Vector) return Value::fail("rotate needs a quaternion and a vector");
            return Value::vec(qmatrix(q).transform(arg(1).vector));  // q v q⁻¹
        }
        // Angle in [0, π]; q and −q are the same rotation, so use the half with w ≥ 0
        if (q.w < 0) q = qscale(q, -1.0);
        double s = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        if (name == "angle") return Value::scalar(2 * std::atan2(s, q.w));
        if (s < 1e-9) return Value::fail("no rotation (angle 0): every axis works");
        return Value::vec(Vector3D(static_cast<float>(q.x / s), static_cast<float>(q.y / s), static_cast<float>(q.z / s)));
    }
    if (name == "slerp") {
        if (arg(0).type != Type::Quaternion || arg(1).type != Type::Quaternion || !realArg(2, x))
            return Value::fail("slerp needs two quaternions and an amount from 0 to 1");
        if (qnorm(toQuat(arg(0))) < 1e-12 || qnorm(toQuat(arg(1))) < 1e-12)
            return Value::fail("slerp: the zero quaternion is not a rotation");
        return fromQuat(qslerp(toQuat(arg(0)), toQuat(arg(1)), x));
    }

    // Complex
    if (name == "conj" || name == "re" || name == "im" || name == "arg") {
        Value v = needNumber(arg(0), name.c_str());
        if (!v.ok()) return v;
        cplx z = toComplex(v);
        if (name == "conj") return number(std::conj(z), v.type == Type::Complex);
        if (name == "re") return Value::scalar(z.real());
        if (name == "im") return Value::scalar(z.imag());
        return Value::scalar(std::arg(z));
    }
    if (name == "polar") {
        if (!realArg(0, x) || !realArg(1, y)) return Value::fail("polar needs a length and an angle");
        return fromComplex(std::polar(x, y));
    }
    return Value::fail("unknown function " + name);
}

Value evalNode(const Node& n, const Resolver* resolver, const Slots* slots) {
    switch (n.kind) {
        case Node::Kind::Number:
            return Value::scalar(n.number);
        case Node::Kind::Imaginary:
            return Value::complex(0.0, 1.0);
        case Node::Kind::Name: {
            if (n.slot >= 0) {
                if (slots) {
                    if (n.slot == 0 && slots->hasX) return Value::scalar(slots->x);
                    if (n.slot == 1 && slots->hasY) return Value::scalar(slots->y);
                    if (n.slot == 2 && slots->hasT) return Value::scalar(slots->t);
                    if (n.slot == 3 && slots->hasU) return Value::scalar(slots->u);
                    if (n.slot == 4 && slots->hasZ) return Value::scalar(slots->z);
                }
            }
            Value v;
            if (resolver && resolver->lookup(n.name, v)) return v;
            if (n.slot == 2) return Value::fail("t (the scene time) is not available here");
            if (n.slot == 3) return Value::fail("u is only available in curves");
            if (n.slot == 4) return Value::fail("z is only available in vector fields");
            if (n.slot >= 0) return Value::fail(n.name + " is only available in plots");
            return Value::fail("unknown name '" + n.name + "'");
        }
        case Node::Kind::Negate: {
            Value v = evalNode(*n.children[0], resolver, slots);
            if (!v.ok()) return v;
            return binaryOp('*', Value::scalar(-1.0), v);
        }
        case Node::Kind::Binary: {
            Value a = evalNode(*n.children[0], resolver, slots);
            if (!a.ok()) return a;
            Value b = evalNode(*n.children[1], resolver, slots);
            return binaryOp(n.op, a, b);
        }
        case Node::Kind::Call: {
            std::vector<Value> args;
            args.reserve(n.children.size());
            for (const auto& c : n.children) args.push_back(evalNode(*c, resolver, slots));
            return callFunction(n.name, args);
        }
        case Node::Kind::Member: {
            Value v = evalNode(*n.children[0], resolver, slots);
            if (!v.ok()) return v;
            if (v.type == Type::Quaternion) {
                return Value::scalar(n.name == "w" ? v.re : n.name == "x" ? v.qi : n.name == "y" ? v.qj : v.qk);
            }
            if (v.type != Type::Vector) return Value::fail("." + n.name + " needs a vector or a quaternion");
            if (n.name == "w") return Value::fail(".w is for quaternions; vectors have .x, .y and .z");
            return Value::scalar(n.name == "x" ? v.vector.x : n.name == "y" ? v.vector.y : v.vector.z);
        }
        case Node::Kind::List: {
            std::vector<Value> items;
            for (const auto& c : n.children) {
                items.push_back(evalNode(*c, resolver, slots));
                if (!items.back().ok()) return items.back();
            }
            if (items.size() != 3) return Value::fail("a vector needs exactly 3 components");
            bool allReal = std::all_of(items.begin(), items.end(), [](const Value& v) { return v.isRealNumber(); });
            if (allReal) {
                return Value::vec(Vector3D(static_cast<float>(items[0].re), static_cast<float>(items[1].re),
                                           static_cast<float>(items[2].re)));
            }
            bool allVectors = std::all_of(items.begin(), items.end(), [](const Value& v) { return v.type == Type::Vector; });
            if (allVectors) {
                // [u, v, w]: the vectors become the columns of a matrix
                Matrix3D m;
                for (int j = 0; j < 3; j++) {
                    m.m[0][j] = items[j].vector.x;
                    m.m[1][j] = items[j].vector.y;
                    m.m[2][j] = items[j].vector.z;
                }
                return Value::mat3(m);
            }
            return Value::fail("list items must all be real numbers or all vectors");
        }
        case Node::Kind::Rows: {
            size_t rows = n.rowSizes.size();
            for (int s : n.rowSizes)
                if (static_cast<size_t>(s) != rows) return Value::fail("a matrix must be square: 3×3 or 4×4");
            if (rows != 3 && rows != 4) return Value::fail("a matrix must be 3×3 or 4×4");
            Matrix4D m = Matrix4D::identity();
            size_t k = 0;
            for (size_t i = 0; i < rows; i++) {
                for (size_t j = 0; j < rows; j++) {
                    Value v = evalNode(*n.children[k++], resolver, slots);
                    if (!v.ok()) return v;
                    if (!v.isRealNumber()) return Value::fail("matrix entries must be real numbers");
                    m.m[i][j] = static_cast<float>(v.re);
                }
            }
            return matrixResult(m, static_cast<int>(rows));
        }
    }
    return Value::fail("internal error");
}

} // namespace

// ---------------------------------------------------------------------------
// Program
// ---------------------------------------------------------------------------

Program::Program() = default;
Program::~Program() = default;
Program::Program(Program&&) noexcept = default;
Program& Program::operator=(Program&&) noexcept = default;

bool Program::compile(const std::string& text, unsigned variables) {
    m_text = text;
    m_root.reset();
    m_error.clear();
    m_names.clear();
    m_usesX = m_usesY = m_usesT = m_usesU = m_usesZ = false;
    try {
        Parser parser(m_text, variables);
        m_root = parser.parseAll();
        collectNames(*m_root, m_names, m_usesX, m_usesY, m_usesT, m_usesU, m_usesZ);
        return true;
    } catch (const ParseError& e) {
        m_error = e.what();
        return false;
    }
}

Value Program::eval(const Resolver* resolver, const Slots* slots) const {
    if (!m_root) return Value::fail(m_error.empty() ? "empty expression" : m_error);
    return evalNode(*m_root, resolver, slots);
}

bool isReservedName(const std::string& name) {
    const FunctionInfo* f = findFunction(name);
    return (f != nullptr && !f->nameable) || name == "pi" || name == "e" || name == "i" || name == "x" ||
           name == "y" || name == "t";
}

bool isValidName(const std::string& name) {
    if (name.empty() || name.size() > 24 || !isNameStart(name[0])) return false;
    for (char c : name)
        if (!isNameChar(c)) return false;
    return !isReservedName(name);
}

} // namespace linalg::expr
