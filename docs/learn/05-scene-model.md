# 5. The scene model

The **model** is what the scene *is*, independent of how it looks: which objects exist, their formulas, their
values, the clock and the undo history. It lives in `src/scene/` and has no graphics, so it is tested directly.

```
Scene ── two Spaces (3D space, complex plane) ── objects (SceneObject subclasses)
      ├─ selection per space
      ├─ timeline (the clock t)
      └─ undo / redo history
```

## SceneObject: fields, props, value

Every object is a `SceneObject` (`src/include/scene/SceneObject.hpp`). It has three kinds of data:

- **Fields**: the formulas you edit. Each `Field` has a key (used in files), a label (shown in the inspector), the
  text, the compiled `Program`, an error, and the last value:

```cpp
struct Field {
    std::string key;     // stable id used in files ("value", "m01", "xmin")
    std::string label;   // shown in the inspector
```

- **Props**: simple settings stored as text: switches (`"comb"`), sizes, modes. `propBool("comb", false)` reads
  one; `setProp("comb", true)` writes one.
- **Value**: the result (`value()`), which other objects use by name. A vector's value is a vector, a tangent
  line's value is its slope, a plot has no value (it's only drawn).

A type is a subclass that adds its fields and props in its constructor and computes its value. The whole tangent
line type is about 50 lines in `src/scene/TypesAdvanced.cpp`:

```cpp
TangentObject::TangentObject() : SceneObject("tangent") {
    setProp("function", true);
    setProp("derivative", true);
    setProp("triangle", true);
    addField("f", "f(x) =", Expect::Plot, "sin(x)");
    addField("x0", "At x =", Expect::Number, "1");
```

`Expect::Plot` says "a function of x, check it with a sample x"; `Expect::Number` says "this must be a real
number" (otherwise the field shows an error).

## Space: names and evaluation order

A `Space` (`src/scene/Space.cpp`) holds the objects of one space and gives them unique names. Its most important
job is `Space::evaluate`: computing every value in the right order.

If `w = 2a + b`, then `w` depends on `a` and `b`. Each object reports the names it uses:

```cpp
std::set<std::string> SceneObject::dependencies() const {
```

(collected from its fields' `Program::names()`). `Space::evaluate` sorts the objects so that every object comes
after the ones it depends on, using **Kahn's algorithm**:

```cpp
    // Kahn's algorithm over "depends on" edges between existing objects
```

1. count, for each object, how many objects it depends on;
2. start with the ones that depend on nothing;
3. each time one is evaluated, its dependents have one less to wait for; when that reaches zero they're ready.

Objects that never become ready are in a **cycle** (`a = b`, `b = a`): they get an error instead of looping
forever. `Space` is also the `Resolver` from chapter 4, so a formula asking for `a` gets `a`'s value.

## Evaluating one object

`SceneObject::evaluate` (in `src/scene/SceneObject.cpp`) evaluates each active field, checks that it has the
expected kind (number, vector, ...), and then calls the type's `computeValue()`. If a field has an error, the
object's value becomes that error and the inspector shows it next to the field.

Some values are expensive: an integral samples a surface thousands of times. Such types override
`cachesValue()` to return `true`, and the object only recomputes when a field, a prop, an input or the time changed:

```cpp
    } else if (cachesValue() && m_computed && !m_dirty && !inputsChanged && !timeMoved) {
        next = m_value;  // nothing it depends on changed
```

## Time

The scene has one clock, `t`, in `Scene::Timeline` (time, playing, speed, loop). `Scene::advance(deltaTime)` moves
it, and every formula can use `t`: `rotz(t)*[1, 0, 0]` spins. Objects that use `t` count as changed whenever the
time moves, so plots that use `t` are rebuilt while the clock plays. Parameters set to *Animate* compute their
value from `t` too, which is why you can scrub the time slider backwards.

## Undo: snapshots

Undo is simple: after every user change, the whole scene is saved into a `SceneData` (a map of `key = value`
strings) and kept if it differs from the last snapshot:

```cpp
void Scene::commit() {
    SceneData now;
    save(now, false);
    if (now == m_committed) return;
    m_undo.push_back(m_committed);
```

Undo restores the previous snapshot. Saving the *whole* scene each time sounds wasteful, but scenes are small
(a few kilobytes), and it means undo can never get out of sync with the model: there is no per-action undo code.

## Files

Scene files use the same `key = value` text (`src/ui/SceneIO.cpp`). Fields and props are saved generically, so
**a new object type is saved and loaded with no extra code**. Older file formats are converted when loading (see
`Scene::restore`), so old scenes keep opening.

## Try it

1. Read `TangentObject::computeValue` in `src/scene/TypesAdvanced.cpp`. Change the default `"sin(x)"` to
   `"x^2/4"`, build, and add a tangent line: the new default appears.
2. In the app, make `a = b` and `b = a` (formula mode). Read the error ("circular reference through b"), then
   find where it's produced in `src/scene/Space.cpp` (search for `circular`).
3. Save a scene (Ctrl+S) and open the `.txt` file in an editor. Find a field (`space.3d.object.0.field.value =
   ...`) and a prop (`...prop.<name> = ...`). Change a number there and open the file again in the app.
