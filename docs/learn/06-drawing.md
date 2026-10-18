# 6. Drawing

Drawing uses **raylib**, a small C library for windows, input and graphics. This chapter covers how the scene
becomes a picture: coordinates, the 3D view, the per-type drawing table, caches, styles, labels, picking, and the
grid shader.

## raylib in five calls

```cpp
    BeginMode3D(m_camera.camera());
```

Between `BeginMode3D(camera)` and `EndMode3D()`, you give 3D world coordinates and raylib projects them through the
camera. The drawing calls are functions like `DrawLine3D(a, b, color)`, `DrawSphere(center, radius, color)` and
`DrawCylinderEx(a, b, r1, r2, sides, color)` (an arrow's shaft is a cylinder, its head a cone: a cylinder whose end
radius is 0). After `EndMode3D` you draw 2D: text, panels.

raylib batches these calls and sends them to the GPU together; that's why drawing a thousand lines per frame is
fine.

## Two coordinate systems

Math uses **z up** (the floor is the xy-plane). raylib's world uses **y up**. One small header converts, so the rest
of the code thinks in math coordinates. `src/include/rendering/WorldMapping.hpp`:

```cpp
inline Vector3 mathToWorld(float x, float y, float z) { return {x, z, -y}; }
```

Math (x, y, z) is drawn at world (x, z, −y); the minus keeps the axes right-handed. Whenever you see
`mathToWorld(...)` in drawing code, a math point is being placed in the world.

## SceneView and the per-type table

`SceneView` (`src/view/SceneView.cpp`) draws one space. It draws the axes, keeps the GPU caches, and then draws each
object, without knowing what the object is. It looks up the object's type in a table of handlers,
`src/include/view/ObjectViews.hpp`:

```cpp
struct ObjectView {
    Palette palette = Palette::Accent;
    // Draw the object (inside BeginMode3D); ctx.print selects the Paper look
    void (*draw)(const scene::SceneObject& o, const DrawContext& ctx) = nullptr;
```

An `ObjectView` holds pointers to the functions that **draw** the type, give its **anchor** (where its label goes
and what a click aims at), its **label** text and its **pick** segments, plus which palette colors it. The drawing
loop then just looks up and calls:

```cpp
        const ObjectView* ov = objectView(o.typeId());
        if (!ov || !ov->draw) continue;
```

Each family of types registers its handlers in one file under `src/view/objects/` (`Basics.cpp`, `Complex.cpp`,
`Quaternion.cpp`, `Calculus.cpp`, `Fourier.cpp`). For example, the tangent line's entry in `Calculus.cpp`:

```cpp
    ObjectView tangent;
    tangent.palette = Palette::Plots;
```

```cpp
    t["tangent"] = tangent;
```

This design is called a **dispatch table**. Before it, every place that drew, labeled or picked objects had an
`if (type == "vector") ... else if (type == "frame") ...` chain, and adding a type meant finding and extending all
of them. Now a type is one entry.

`DrawContext` gives the draw function what it needs for this object: the color (already faded if another object is
selected), whether it's selected, whether this is the Paper look, and access to the caches.

## Caches: plots and fields

A surface is thousands of triangles. Computing them means evaluating the formula at every grid point, too slow to
do every frame. `SceneView::updatePlots` builds GPU meshes (`PlotRenderer`) only when the object changed, the theme
changed, or its color changed; every other frame just draws the stored meshes. The check uses the object's
`changedAt()` (the evaluation pass in which its value last changed).

## View styles and Paper

Colors don't come from the code that draws; they come from the active **view style**
(`src/view/ViewStyle.cpp`): Colorful, Paper, High contrast, Chalkboard. A style is data (palettes, background, line
thickness). Drawing code asks `activeStyle()` for colors.

**Paper** is the print preview: black, grays and dash patterns, matching what the LaTeX export writes. Every draw
function handles both looks through `ctx.print`.

## Labels

Labels are 2D text placed next to 3D points. `SceneView::drawLabels` collects them all (each type's `label` text at
its `anchor`), projects them to the screen, and lays them out so they don't overlap (`src/text/LabelLayout.cpp`,
which has its own unit tests). A label pushed away from its point gets a thin leader line back to it.

## Picking

To select by clicking, App asks each type for **segments** near which a click should select it (an arrow's shaft,
a curve's pieces) through `ObjectView::pick`. `App::pickAt` (`src/app/AppInput.cpp`) projects them to the screen
and selects the closest one within 12 pixels of the cursor.

## The grid: a shader

The grid is drawn by the project's only **shader**: a small program, written in GLSL, that runs on the GPU **once
per pixel**. The C++ side (`src/rendering/GridShader.cpp`) draws one large square on the floor; the shader decides,
for each pixel of that square, how much grid line passes through it.

```cpp
float lines2(vec2 p, float s) {
    vec2 u = p / s;
    vec2 w = max(fwidth(u), vec2(1e-6));
```

`p / s` measures the position in grid cells; `fract` keeps the part after the decimal point, which is near 0 or 1
on a line; `fwidth` is how much that changes from one pixel to the next. Dividing the distance to the line by it
gives the distance **in pixels**, so lines are one pixel wide and smooth at any zoom or angle.

The spacing comes from the zoom (`src/math/GridMath.cpp`, unit-tested): the finest lines that are at least 14 pixels
apart, fading in and out as you zoom so there are no jumps. If a computer can't compile the shader, `valid()` is
false and the older line grid (`rendering/Grid`) is drawn instead.

## Try it

1. In `src/view/objects/Basics.cpp`, find `drawVector`. Change `arrow(...)` to draw only a `DrawLine3D` between the
   same two points, build, and see vectors become thin lines. Undo it.
2. Change a palette color of the Colorful style in `src/view/ViewStyle.cpp` and watch every vector change.
3. In the grid shader's `main`, the line `a *= 1.0 - smoothstep(0.55 * radius, radius, length(p - c));` fades the
   grid toward its edge. Change `0.55` to `0.95` to see a sharper edge.
