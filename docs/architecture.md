# Architecture

How axismundi fits together: what happens each frame, where the data lives, and where to add things.
The folder-by-folder map is in [CLAUDE.md](../CLAUDE.md#architecture).

## Layers

```
                 ┌──────────────────────────── app/ ─────────────────────────────┐
  input ───────► │ App (one class, four files): loop, input, drawing, files      │ ◄── edu/ (gallery,
  (raylib)       │   owns Scene, SceneView, panels, camera, preferences          │      lessons), Tour
                 └───────┬──────────────────────┬───────────────────────┬────────┘
                         │ edits / evaluates    │ draws                 │ shows
                         ▼                      ▼                       ▼
        ┌──────── scene/ (no raylib) ─┐  ┌──── view/ + rendering/ ─┐  ┌──────── ui/ ─────────────┐
        │ Scene: 2 Spaces, selection, │  │ SceneView: per-type     │  │ ScenePanel (list +       │
        │   undo, timeline, files     │  │   drawing, labels,      │  │   inspectors), panels,   │
        │ Space: objects by name,     │  │   styles, plot/field    │  │   popovers, dialogs,     │
        │   dependency order          │  │   caches                │  │   editors (ui/core:      │
        │ SceneObject: fields (expr), │  │ PlotRenderer,           │  │   scale, fonts, widgets, │
        │   props, value              │  │   FieldRenderer,        │  │   text editor)           │
        │ Types, TypesAdvanced        │  │   MeshBuilder, Grid     │  └──────────────────────────┘
        └──────────────┬──────────────┘  └────────────┬────────────┘
                       │ formulas                     │ math
                       ▼                              ▼
        ┌──────── math/ (no raylib) ─────────────────────────────────────────────┐
        │ expr/: Program (parse once), Value (number … quaternion), functions,    │
        │   Reference (cheat sheet)                                              │
        │ Vector3D, Matrix3D/4D, Decompositions, Quaternion, VectorField,         │
        │ Integrals, CurveGeometry, Fourier, Contour …                           │
        └────────────────────────────────────────────────────────────────────────┘
        export/ (no raylib): LatexExport (Space → PGFPlots), PdfCompile (pdflatex)
        text/ (no raylib): TextLayout (wrapping), LabelLayout (non-overlapping labels)
```

Everything without raylib is unit-tested in `tests/` (`make test`): `main.cpp` runs the list, `TestSupport.hpp` has CHECK, and one file per topic (linear, expr, calculus, scene, export, text).

## One frame

1. **Input** (`AppInput.cpp`): shortcuts; `handleViewMouse` turns presses on the empty view into orbit/pan
   drags and wheel zoom (`CameraController::applyOrbit`, math in `math/OrbitCamera`) or, released without
   moving, into picks (`pickAt`, using each type's screen segments); with the fly camera Tab switches
   camera/UI mode.
2. **Update** (`App::update`): the scene clock advances (`Scene::advance`), then `Scene::evaluate`:
   - every object gets the time (`setTime`);
   - each `Space` evaluates its objects in dependency order: each active field's `Program` runs with a
     resolver (other objects by name) and slots (t, and x/y/u/z where a field allows them);
   - `computeValue` turns fields into the object's value. Expensive types cache it (`cachesValue`) and
     redo it only when something changed;
   - an object counts as changed when its fields, props, inputs or value changed (`changedAt`), so views
     know what to rebuild.
3. **Draw** (`AppDraw.cpp`): the background and grid from the active view style, then `SceneView::draw3D`
   (per-type drawing; plots and fields from GPU-mesh caches keyed on `changedAt`, style and color), then the
   labels (laid out without overlaps), then the UI: side bar (list, inspector, view settings), floating panels,
   popovers, dialogs, HUD, toasts, tooltips.

## State and files

- **Scene** (saved in `.txt` scene files, `key = value`): both spaces' objects (fields as text, props), camera,
  view settings (including the style) and the timeline. Undo snapshots are the same data without the time.
- **Preferences** (`~/.config/axismundi/settings.txt`): theme, size, panel layout, finished lessons, last style,
  recent files.
- Objects never hold pointers to each other: they refer by name through formulas; renaming rewrites them.

## Where to add things

| To add | Touch |
|--------|-------|
| A formula function | `math/expr/Expr.cpp` (table + `callFunction`), a `Reference.cpp` entry (the test insists) |
| A value kind | `math/expr/Value`, operators in `binaryOp`, `checkExpect`, `sameValue`, summaries |
| An object type | `scene/Types*` (class + registry), one `ObjectView` entry in `view/objects/<Family>.cpp` (drawing, Paper, label, anchor, picking), an `inspect<Type>` method + its `ScenePanel::typeUi` row, optional templates/analysis/gallery/lesson, a `LatexExport` branch + the Auto 2D rule, tests and tour steps |
| A view style | `view/ViewStyle.cpp` (`kStyles` + a `case` in `styleFor`) |
| A panel | a content function in `ui/`, registered in `App` with the `PanelManager`, a top bar button |
| A lesson | append to `edu/Tutorial.cpp` (finished lessons are a bitmask by position) and add walk solutions to `tools/lesson_walk.py` |
