# 4. Formulas: the expression engine

Every input in the app is a formula: a vector's value can be `[1, 0, 0]`, `2a - b` or `rotz(t)*[1, 0, 0]`; a surface
is `sin(x)*cos(y)`. The **expression engine** in `src/math/expr/` turns that text into values. It is plain C++
(no graphics) and heavily unit-tested.

## Three steps: parse, compile, evaluate

1. **Parse**: read the text and build a **tree** that shows the structure. `2a + b` becomes "add(multiply(2, a),
   b)".
2. **Compile**: keep that tree in a `Program`, and remember which names it uses (`a`, `b`) and which variables
   (`x`, `t`...).
3. **Evaluate**: walk the tree and compute, asking a **resolver** for the value of each name.

Parsing happens once, when you type. Evaluation happens every frame (or, for plots, thousands of times per change,
once per sample), so it must be fast: no text processing at that point.

## The Program class

`src/include/math/expr/Expr.hpp`:

```cpp
class Program {
public:
    bool compile(const std::string& text, unsigned variables = 0);
    Value eval(const Resolver* resolver = nullptr, const Slots* slots = nullptr) const;
```

- `compile` returns `false` on a syntax error; `error()` then says why ("missing ']'").
- `names()` lists the names it uses. The scene uses this to know that `w = 2a + b` depends on `a` and `b`
  (chapter 5).
- `eval` computes a `Value` (below). Names come from the `Resolver`; variables (`x`, `y`, `t`, `u`, `z`) come
  from `Slots`.

## The tree

Inside `src/math/expr/Expr.cpp`, each part of a formula is a `Node`:

```cpp
struct Node {
    enum class Kind { Number, Imaginary, Name, Negate, Binary, Call, Member, List, Rows };
```

A number is a leaf; `a + b` is a `Binary` node with `op = '+'` and two children; `cross(a, b)` is a `Call` with two
children. Children are `std::unique_ptr<Node>`, so freeing the root frees the whole tree.

## The parser: one function per level of precedence

In `2 + 3*4` the multiplication binds tighter. The parser handles this with one function per level, each calling
the next tighter one (a **recursive descent** parser). The grammar is written at the top of `Expr.hpp`:

```cpp
//   expr    := term (('+' | '-') term)*
//   term    := unary (('*' | '/' | implicit) unary)*      implicit: "2x", "3(x+1)"
//   unary   := ('-' | '+') unary | power
```

and each line is a function in `Expr.cpp`: `parseExpr` reads terms separated by `+`/`-`, `parseTerm` reads
factors separated by `*`/`/` (and implicit multiplication like `2x`), `parseUnary` handles a leading minus,
`parsePower` the `^`. Because `parseExpr` calls `parseTerm`, which calls `parseUnary`, products are grouped before
sums automatically.

## Values

The result of a formula can be several kinds of thing. `Value` (`src/include/math/expr/Value.hpp`) holds any of
them:

```cpp
enum class Type { Scalar, Complex, Vector, Matrix, Quaternion, Error };
```

```cpp
struct Value {
    Type type = Type::Error;
    double re = 0.0;            // Scalar, or real part of Complex
```

Errors are values too: `Value::fail("cross needs two vectors")`. An error flows through the rest of the
calculation and ends up in the inspector next to the field, instead of crashing the program.

## Functions

The built-in functions are listed in a table in `Expr.cpp`, with how many arguments each takes:

```cpp
    {"max", 2, 2}, {"mod", 2, 2}, {"sign", 1, 1},
```

and implemented in `callFunction`, one `if` per name:

```cpp
    if (name == "sign") {
        if (!realArg(0, x)) return Value::fail("sign needs a real number");
        return Value::scalar(x > 0 ? 1.0 : x < 0 ? -1.0 : 0.0);
    }
```

## Names and slots

- **Names** are other objects (`a`, `M`, `theta`). The engine doesn't know about scenes; it asks a `Resolver`:

```cpp
class Resolver {
public:
    virtual ~Resolver() = default;
    // Returns false when the name is unknown
    virtual bool lookup(const std::string& name, Value& out) const = 0;
};
```

  `= 0` makes it **pure virtual**: Resolver is an interface, and whoever evaluates provides an implementation.
  `Space` (the list of objects) is one; the tests use a tiny `MapResolver` with a `std::map`.

- **Slots** are variables that change while a formula is evaluated many times: `x` and `y` for plots, `u` for
  curves, `z` for vector fields, `t` (the scene time) everywhere.

```cpp
struct Slots {
    double x = 0.0, y = 0.0, t = 0.0, u = 0.0, z = 0.0;
```

  `u` and `z` are only variables where they make sense (curves, fields); elsewhere they are ordinary names, so an
  object may be called `z`. That's the `variables` argument of `compile` (`kVarU`, `kVarZ`).

## The formula reference

The in-app cheat sheet (F2) is data in `src/math/expr/Reference.cpp`: sections of entries with a syntax line, an
explanation and a live example:

```cpp
                     {"cross(a, b)", "Cross product: perpendicular to both, by the right-hand rule.",
                      "cross([1, 0, 0], [0, 1, 0])"},
```

A unit test (`testReference`) checks two things: every example compiles and evaluates, and **every function in the
table appears in the reference**. So if you add a function and forget to document it, `make test` fails.

## Try it

1. Add a function `sq(x)` = x². Put `{"sq", 1, 1}` in the function table, an `if (name == "sq")` branch in
   `callFunction` (copy `sign`), run `make test`, and read the failure: `testReference` wants an entry. Add one
   to `Reference.cpp` (next to `sqrt`), and the tests pass. Now `sq(3)` works in any field of the app.
2. In the app, type `[1, 2` into a vector's formula and read the error ("missing ']'"). Find where that message
   is produced in `Expr.cpp` (search for `missing `).
3. Read `testExprEngine` in `tests/test_expr.cpp`. Pick a check, predict the result, and change the expected value
   to see the failure message.
