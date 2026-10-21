# Glossary

Words used in the code and in this guide.

## Code

| Word | Meaning |
|------|---------|
| **anchor** (view) | The world point of an object's handle: its label sits there and clicks aim at it (`ObjectView::anchor`) |
| **anchor** (UI) | A name given to a control (`ui::anchor("view.gridspacing", rect)`) so tutorials can highlight it; in the GTK app a widget (`MainWindow::anchorWidget`) |
| **AppApi** | The interface lessons and gallery examples use to drive an app (`addObject`, `select`, ...); `App` and the GTK app's `WindowApi` implement it |
| **retained mode** | A UI whose widgets are objects kept between frames (GTK), unlike immediate mode (the raylib UI, chapter 3) |
| **signal** | A GTK event a callback can be connected to ("clicked", "changed"), chapter 10 |
| **Session** | What both front ends share: the scene, camera, 3D view, grid and the scripting functions (`app/Session`) |
| **cache** | Stored results reused until their inputs change: plot meshes, cached object values |
| **changedAt** | The evaluation pass in which an object's value last changed; caches compare it |
| **commit** | Taking an undo snapshot after a user change (`Scene::commit`) |
| **design units** | Sizes for a 1600×900 window, scaled by `ui::px` |
| **dispatch table** | A map from a type name to the functions that handle it (`objectView`, `ScenePanel::typeUi`, `objectExport`) |
| **DrawContext** | What a type's draw function gets: color, selected, Paper, the caches |
| **field** | One formula input of an object (key, label, text, compiled program, value) |
| **header / source** | `.hpp` declares, `.cpp` defines (chapter 1) |
| **immediate mode** | UI drawn from scratch every frame; widgets are function calls that return "used" |
| **ObjectView** | One type's drawing, label, anchor and picking functions |
| **Paper** | The black-and-white print preview, matching the LaTeX export |
| **pick** | Selecting an object by clicking near it in the 3D view |
| **prop** | A simple setting of an object stored as text (switches, sizes, modes) |
| **RAII** | Resources are released by destructors automatically (chapter 1) |
| **registry** | The list of object types in `objectTypes()` (`src/scene/Types.cpp`) |
| **resolver** | What the formula engine asks for the value of a name (the space) |
| **shader** | A small GPU program, here the grid's per-pixel line drawing (GLSL) |
| **slot** | A variable supplied while evaluating: x, y (plots), u (curves), z (fields), t (time) |
| **space** | The 3D space or the complex plane: a list of objects with unique names |
| **tour** | The scripted screenshot run (`--tour`) |
| **value** | An object's computed result, usable by other objects by name (`expr::Value`) |
| **view style** | Colors and line weights of the 3D view (Colorful, Paper, High contrast, Chalkboard) |

## Math

| Word | Meaning |
|------|---------|
| **curvature κ** | How sharply a curve turns: 1 / radius of the best-fitting circle |
| **derivative f'(x)** | The slope of f at x: the limit of secant slopes (f(x + h) − f(x)) / h as h → 0 |
| **eigenvector** | A direction a matrix only stretches (by its eigenvalue λ) |
| **epicycles** | Spinning circles whose sum draws a closed curve (its Fourier series) |
| **Frenet frame** | T (direction of motion), N (toward the center of the turn), B = T × N, along a curve |
| **gradient ∇f** | For z = f(x, y): (∂f/∂x, ∂f/∂y), pointing the way f climbs fastest |
| **line integral** | ∫ F · dr: the work a vector field does along a path |
| **partial derivative** | The slope in one direction with the other variables held fixed |
| **quaternion** | Four numbers that describe a rotation (an axis and an angle) |
| **Riemann sum** | Boxes approximating the volume under a surface |
| **secant** | The line through two points of a graph |
| **tangent plane** | The plane that touches a surface at a point |
| **torsion τ** | How fast a curve twists out of the plane it turns in |
