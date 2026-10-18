# 1. C++ through this code

C++ is a compiled language: the compiler (`clang++`) turns the text of `.cpp` files into machine code, and the
linker joins the pieces into one program (`bin/linalg_viz`). This chapter goes through the features you will
meet in this project, each with an example from the code.

## Headers and sources

A C++ program is many `.cpp` files compiled **separately**. To use a function or class defined in another file,
the compiler needs to know it exists and what it looks like. That promise is the **header** (`.hpp`), included
with `#include`. The **source** (`.cpp`) has the actual code.

`src/include/math/GridMath.hpp` (header, the promise):

```cpp
double pixelsPerUnit(double distance, double fovyDegrees, double viewportHeight);
```

`src/math/GridMath.cpp` (source, the code):

```cpp
double pixelsPerUnit(double distance, double fovyDegrees, double viewportHeight) {
    double visible = 2.0 * std::max(distance, 1e-6) * std::tan(fovyDegrees * 3.14159265358979 / 360.0);
    return viewportHeight / visible;
}
```

- `#pragma once` at the top of every header means "include this file only once per compilation".
- `#include "math/GridMath.hpp"` (quotes) is a project header; `#include <cmath>` (angle brackets) is from the
  standard library or raylib.
- If you declare a function in a header but never define it, everything compiles and the **linker** fails with
  "undefined reference". (That really happened in this project once: a file with definitions was deleted by
  mistake.)

## Namespaces

Names are grouped in **namespaces** so two libraries can both have a `Vector3` without clashing. Everything
here lives in `linalg`, with sub-namespaces per area: `linalg::scene`, `linalg::view`, `linalg::expr`,
`linalg::ui`, `linalg::latex`.

```cpp
namespace linalg::grid {
double tickStep(double pixelsPerUnit, double minPixels);
}
```

Outside the namespace you write `grid::tickStep(...)` or `linalg::grid::tickStep(...)`. Inside `.cpp` files
you sometimes see `using namespace draw;` which lets the code say `arrow(...)` instead of `draw::arrow(...)`.
Headers never do that (it would leak into every file that includes them).

An **anonymous namespace** (`namespace { ... }`) makes functions private to one `.cpp` file. The drawing
helpers in `src/view/objects/Calculus.cpp` are written that way: nobody outside the file can call them.

## struct, class and constructors

A `struct` groups data. `src/include/math/GridMath.hpp`:

```cpp
struct Level {
    double spacing = 1.0;
    double fade = 1.0;
    double ratio = 10.0;  // the next family of lines is `ratio` times wider apart
};
```

The `= 1.0` are default values. `Level l;` makes one with those defaults; `l.spacing = 0.1;` changes a member.

A `class` is the same thing with members **private** by default; you choose what is `public:` (usable by
everyone) and what stays `private:` (only the class's own functions may touch it). `OrbitCamera` in
`src/include/math/OrbitCamera.hpp` keeps its state private and offers functions to change it safely (the pitch
can't go past 89°):

```cpp
    void orbit(float dyaw, float dpitch);
...
private:
    Vector3D m_pivot{0, 0, 0};
    float m_distance = 10.0f;
```

The `m_` prefix is a naming habit here: it marks member variables.

A **constructor** runs when an object is created. `TangentObject`'s constructor (in
`src/scene/TypesAdvanced.cpp`) declares its fields:

```cpp
TangentObject::TangentObject() : SceneObject("tangent") {
    setProp("function", true);
```

`: SceneObject("tangent")` calls the base class's constructor first (more on base classes below).

## References and const

`double&` is a **reference**: another name for an existing variable, not a copy. `const` means "I promise not
to change it".

```cpp
const std::string& name() const { return m_name; }
```

- The return type `const std::string&` hands back the object's own string without copying it, read-only.
- The `const` at the end says the function doesn't modify the object, so it may be called on a `const`
  object.

Functions take big things by `const&` (no copy) and return small results by value. Out-parameters (`double&
out`) let a function return a value *and* report success:

```cpp
bool derivative1(const Function& f, double x, double h, double& out);
```

It returns `false` where f is undefined; otherwise it writes the slope into `out`. This pattern is everywhere
in `src/math/`.

## The standard library: string, vector, map

- `std::string`: text that manages its own memory.
- `std::vector<T>`: a growable array. `m_curve.push_back(p)` adds a point; `for (const auto& p : m_curve)`
  visits them all.
- `std::map<K, V>`: a sorted dictionary. The per-type tables are maps from a type name to its functions:
  `std::map<std::string, ObjectView>`.

`auto` lets the compiler figure out a type you'd otherwise have to spell out: `auto it = table.find(typeId);`.

## enum class

A fixed set of named choices, without mixing them up with numbers:

```cpp
enum class CameraMode {
    Orbit,  // Blender-like: drag to turn around a pivot, right-drag to pan, wheel to zoom
    Fly,    // Minecraft-like: Tab captures the mouse, mouse look + WASD/Space/Ctrl
};
```

You write `CameraMode::Orbit`. `static_cast<int>(mode)` converts to a number when really needed (saving to a
file).

## Inheritance and virtual functions

Every object in a scene (vector, matrix, tangent line...) **is a** `SceneObject`. The base class has what all
share (name, fields, value); each subclass adds its own fields and its own way of computing its value.

`src/include/scene/SceneObject.hpp`:

```cpp
class SceneObject {
public:
    explicit SceneObject(std::string typeId);
    virtual ~SceneObject();
```

`virtual` means: when code holding a `SceneObject` calls this function, the **subclass's version** runs.
`Space::evaluate` doesn't know whether it's looking at a vector or a tangent line; it calls `evaluate`, and
each type's `computeValue` does the right thing. In a subclass (`src/include/scene/Types.hpp`):

```cpp
    expr::Value computeValue() const override;
```

`override` asks the compiler to check that a matching virtual function exists in the base class: a typo
becomes an error instead of a silent new function.

## Ownership: std::unique_ptr and RAII

In C++ *someone* must free what is created. This project never writes `delete`; it uses owners that free
automatically.

- `std::unique_ptr<SceneObject>` owns one object; when the pointer goes away, the object is freed. A `Space`
  keeps `std::vector<std::unique_ptr<SceneObject>>`, so removing an entry frees the object.
- `std::make_unique<TangentObject>()` creates one.
- **RAII** ("resource acquisition is initialization"): a class that acquires something (memory, a GPU shader,
  a file) releases it in its **destructor** (`~ClassName()`), which runs automatically. `GridShader` frees its
  GPU shader in `~GridShader()`, and forbids copying, since two copies would both try to free it:

```cpp
    GridShader(const GridShader&) = delete;             // owns a GPU resource: not copyable
```

## Lambdas and std::function

A **lambda** is a small function written inline. `[` `]` lists what it can use from around it (the
"capture"): `[&]` everything by reference, `[=]` by copy, `[]` nothing.

`src/scene/TypesAdvanced.cpp` builds the curve r(u) from three formula fields:

```cpp
integrals::Path curvePath(const Field* fx, const Field* fy, const Field* fz, const expr::Resolver* res, double t) {
    return [=](double u, integrals::D3& o) {
```

`integrals::Path` is a `std::function<bool(double, D3&)>`: "anything callable that takes a double and a point
and returns a bool". The math code (derivatives, integrals, Frenet frames) takes such functions, so it doesn't
need to know about formulas or scenes at all.

## Casts: static_cast and dynamic_cast

- `static_cast<float>(x)` converts between number types; the code uses it instead of C's `(float)x` so
  conversions are easy to find.
- `static_cast<const scene::TangentObject&>(o)`: "I know this `SceneObject` is really a `TangentObject`".
  Used after checking the type name, so it's safe.
- `dynamic_cast<scene::VelocityObject*>(o)` checks at run time and gives `nullptr` if the object is a
  different type. The tutorial checks use it because they can't be sure what the user created.

## Function pointers and member-function pointers

A function pointer stores *which* function to call. The per-type tables are built from them.
`src/include/view/ObjectViews.hpp`:

```cpp
    void (*draw)(const scene::SceneObject& o, const DrawContext& ctx) = nullptr;
```

Read it as "`draw` is a pointer to a function taking (object, context) and returning nothing". The table maps
`"tangent"` to a structure whose `draw` points at the tangent's drawing function, and the drawing loop just
calls `ov->draw(o, ctx)`.

A lambda with an empty capture `[]` converts to a plain function pointer, which is how the table entries are
written inline.

A **member-function pointer** points at a function *of a class*, which needs an object to run on. The
inspector table in `src/include/ui/ScenePanel.hpp`:

```cpp
        void (ScenePanel::*inspect)(InspectorContext&) = nullptr;
```

and the call in `src/ui/ScenePanel.cpp`:

```cpp
    if (const TypeUi* tu = typeUi(type); tu && tu->inspect) (this->*tu->inspect)(ctx);
```

`this->*` says "call it on this ScenePanel".

## static, constexpr, mutable

- `static` inside a function makes a variable that is created once and kept: the tables are
  `static const std::map<...> table = {...};`, built the first time they are needed.
- `constexpr` values are known at compile time: `constexpr double kPi = 3.14159265358979323846;`.
- `mutable` members may change inside `const` functions. `computeValue() const` fills `mutable` caches (a
  tangent's sampled curve) because computing them doesn't change what the object *is*.

## Macros (rarely)

`#define` does text substitution before compiling. The project uses it almost only in the tests
(`tests/TestSupport.hpp`):

```cpp
#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)
```

`#cond` turns the expression into text, so a failing check prints exactly what was tested and where.

## Try it

1. Open `src/include/math/GridMath.hpp` and `src/math/GridMath.cpp` side by side. Find each declared function's
   definition. Remove one definition, run `make test`, and read the linker error. Put it back.
2. In `src/math/GridMath.cpp`, `tickStep` loops over `{1.0, 2.0, 5.0, 10.0}`. First add a check to `testGridMath`
   in `tests/test_calculus.cpp`: `CHECK_NEAR(tickStep(25, 60), 2.5, 1e-12);` and run `make test`: it fails
   (the step is 5). Now add `2.5` to the list before `5.0` and run it again: it passes. That's how a test pins
   down a behavior before you change it.
3. Find all classes that `override computeValue` (search for `computeValue() const override`). Each one is an
   object type you can add from the app's Add menu.
