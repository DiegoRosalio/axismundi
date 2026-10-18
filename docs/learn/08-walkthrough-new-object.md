# 8. Walkthrough: a new object type

This chapter follows one object type, the **tangent line on y = f(x)**, through every file it touches. Then it
gives you a guided exercise to build your own. It ties together everything from chapters 4–7.

## 1. The math (src/math/)

The tangent's slope is a derivative. The math doesn't know about objects or formulas; it takes a C++ function
(chapter 1's `std::function`). `src/include/math/Derivatives.hpp`:

```cpp
bool derivative1(const Function& f, double x, double h, double& out);
```

and it's tested on its own in `tests/test_calculus.cpp` (`testDerivatives`: sin' = cos, and so on). Keeping math
separate means it can be tested exactly, and reused (the velocity object uses the same file).

## 2. The model (src/scene/)

**The class** in `src/include/scene/Types.hpp`:

```cpp
class TangentObject : public SceneObject {
```

with accessors the view and the inspector need (`x0()`, `slope()`, `function()`...), `mutable` caches of the sampled
curves, and an override of `computeValue`.

**The constructor and `computeValue`** in `src/scene/TypesAdvanced.cpp`. The constructor adds fields (`f`, `x0`,
`h`, the x range) and props (`function`, `derivative`, `triangle`). `computeValue` evaluates f through the formula
engine, calls `derivatives::derivative1`, samples f and f' for drawing, and returns the slope as the value:

```cpp
    return expr::Value::scalar(m_slope);
```

**The registry** in `src/scene/Types.cpp` makes the type exist for the Add menu, files and the tests:

```cpp
        {"tangent", "Tangent line on y = f(x)", "Calculus", "T", spaceBit(SpaceKind::Euclidean3D),
```

(id stored in files, label, menu category, base name, which space). Saving and loading need nothing more.

## 3. Drawing (src/view/objects/Calculus.cpp)

One `ObjectView` entry (chapter 6): the palette, a `draw` function (`drawTangent` draws f, f', the tangent line,
the slope triangle and the secant, in normal and Paper looks), an `anchor` (the point), a `label`
("T  f'(1) = 0.54") and a `pick` handler.

## 4. The inspector (src/ui/)

- `ScenePanel::inspectTangent` in `src/ui/ScenePanelInspectors.cpp`: field rows, switches (`toggleProp`), and
  notes (the slope, the line's equation, f'').
- Its declaration in `src/include/ui/ScenePanel.hpp`, and its row in the table `ScenePanel::typeUi`
  (`src/ui/ScenePanel.cpp`), which also gives the objects list's summary ("slope 0.54 at x = 1").

## 5. The extras

| What | Where |
|------|-------|
| Add-menu template (select a function → "Tangent line on f") | `AddMenu::templates`, `src/ui/AddMenu.cpp` |
| Analysis panel section | `InfoPanel::update`, `src/ui/InfoPanel.cpp` |
| LaTeX export | `exportTangent` and its table row, `src/export/LatexObjects.cpp` |
| Tests of the model | `tests/test_scene.cpp` (search "Derivative objects") |
| Tests of the export | `tests/test_export.cpp` (search "tangent") |
| Gallery examples | `src/edu/Examples.cpp` ("Secant becomes tangent") |
| Tutorial lesson | `src/edu/Tutorial.cpp` ("Derivatives: slopes and tangents"), solutions in `tools/lesson_walk.py` |
| Screenshot tour steps | `src/app/Tour.cpp` (`16da_secant_tangent`) |
| User docs | `README.md`, `UI_GUIDE.md`, `FEATURES.md` |

Only steps 1–4 are needed for a working type; the extras make it discoverable and checked.

## Exercise: build a "midpoint" object

Build an object that takes two vectors and shows their **midpoint** as a point, with a dashed line between them.

**1. The class.** In `src/include/scene/Types.hpp`, next to `TangentObject`:

```c++
// The midpoint of two vectors' tips, (a + b) / 2; the value is the midpoint
class MidpointObject : public SceneObject {
public:
    MidpointObject();

protected:
    expr::Value computeValue() const override;
    std::unique_ptr<SceneObject> makeEmpty() const override { return std::make_unique<MidpointObject>(); }
};
```

**2. Fields and value.** In `src/scene/TypesAdvanced.cpp`:

```c++
MidpointObject::MidpointObject() : SceneObject("midpoint") {
    addField("a", "From", Expect::Vector, "[1, 0, 0]");
    addField("b", "To", Expect::Vector, "[0, 2, 1]");
}

expr::Value MidpointObject::computeValue() const {
    Vector3D a = field("a")->value.vector, b = field("b")->value.vector;
    return expr::Value::vec((a + b) * 0.5f);
}
```

(If a field has an error, the base class never calls `computeValue`: the error shows in the inspector instead.)

**3. Register it** in `objectTypes()` in `src/scene/Types.cpp`, right after the `"vector"` entry (the Add menu
groups consecutive entries of the same category, so another "Basics" entry elsewhere would make a second group):

```c++
        {"midpoint", "Midpoint of two points", "Basics", "m", spaceBit(SpaceKind::Euclidean3D),
         "Halfway between two points: (a + b) / 2",
         [] { return std::make_unique<MidpointObject>(); }},
```

Build and run `make test`: the test that checks every registered type (`testAllObjectTypes`) now also creates,
evaluates, saves and loads your type. Start the app: it's in the Add menu, and other objects can use it by name
(a vector with formula `m`). Nothing is drawn yet.

**4. Draw it.** In `src/view/objects/Basics.cpp`, inside `registerBasicViews`:

```c++
    ObjectView midpoint;
    midpoint.palette = Palette::Vectors;
    midpoint.draw = [](const SceneObject& o, const DrawContext& c) {
        if (o.value().type != expr::Type::Vector) return;
        Vector3D a = o.field("a")->value.vector, b = o.field("b")->value.vector;
        dashed(mathToWorld(a), mathToWorld(b), c.color);
        DrawSphere(mathToWorld(o.value().vector), c.selected ? 0.1f : 0.07f, c.color);
    };
    midpoint.anchor = [](const SceneObject& o, const DrawOptions&, Vector3& out) {
        if (o.value().type != expr::Type::Vector) return false;
        out = mathToWorld(o.value().vector);
        return true;
    };
    midpoint.pick = [](const SceneObject& o, const DrawOptions& options, std::vector<Segment>& out) {
        Vector3 p;
        if (SceneView::anchorPoint(o, options, p)) out.push_back({p, p});
    };
    t["midpoint"] = midpoint;
```

Now it's drawn, labeled and clickable. The inspector shows the name and color but not the fields yet.

**5. The inspector.** Add `void inspectMidpoint(InspectorContext& c);` in `ScenePanel.hpp`, a row
`{"midpoint", {&ScenePanel::inspectMidpoint, nullptr}},` in `ScenePanel::typeUi`, and in
`ScenePanelInspectors.cpp`:

```c++
void ScenePanel::inspectMidpoint(InspectorContext& c) {
    for (const char* k : {"a", "b"})
        c.col.setY(fieldRow(c.col.x(), c.col.y(), c.col.width(), c.o, k, c.sc, 64.0f));
}
```

**6. A test.** In `tests/test_scene.cpp`, inside `testScene`, create a scene, add a midpoint with `a = [2, 0, 0]`
and `b = [0, 2, 0]`, evaluate, and `CHECK` that the value is (1, 1, 0).

That's a complete object type. Compare your result with the tangent line's files to see what the "extras" add.

## Try it

1. Do the exercise above.
2. Extend it: a prop `"line"` (`setProp("line", true)` in the constructor, `toggleProp(c, "Line", "line", ...)` in
   the inspector) that turns the dashed line on and off.
3. Give it LaTeX export: a function in `src/export/LatexObjects.cpp` (copy `exportVector`), a row in the table with
   a `flat` test (true when both points have z = 0), and a check in `tests/test_export.cpp`.
