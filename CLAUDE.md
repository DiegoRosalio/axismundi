# CLAUDE.md — project context for axismundi

Context for Claude Code (and humans) working on this repo. Keep it current when the
architecture, workflow or environment changes.

## What this is

A C++20 / raylib 5.5 + raygui interactive visualizer. Users build scenes from any number of
objects (vectors, matrices, eigenvector sets, plots, parameters, complex numbers) whose fields
are named formulas that can reference each other, in two spaces: a 3D space (z up) and the
complex plane. Resolution-independent themed UI with floating panels, plus learning tools
(tutorial, example gallery, worked calculation steps, transformation breakdown, tooltips).

User docs: `README.md`, `QUICKSTART.md`, `UI_GUIDE.md`. Open work: `TODO.md` (starts with a "Next up"
list, then Medium/Low priority, code quality, research, known issues). Done work: `FEATURES.md`.

## Development process (how work is done here)

1. **Plan first** for anything bigger than a small fix: read the code, ask the user about real choices,
   write a plan (context, design, files, commits, verification) and get it approved.
2. **Work in logical commits**, each one building, warning-free and with tests passing; push every commit
   to `claude` (see Git workflow). No pull requests unless asked.
3. **Verify every change** with the scripts in `tools/dev/` (work files go to `~/.cache/axismundi-dev`,
   override with `AXISMUNDI_DEV`):
   - `tools/dev/raylib.sh` once per machine (builds raylib 5.5), then `tools/dev/build.sh`;
   - `make test` (add unit tests for new logic) and `tools/dev/check.sh` (prints ALL CLEAN);
   - UI changes: `tools/dev/tour.sh 800x600`, `1600x900`, `1920x1080 l --theme light`, `2560x1440 x`,
     then look at the PNGs; add tour steps for new UI;
   - tutorial or `App::setMode`/scene changes: `tools/dev/lesson-walk.sh` (0 problems expected);
   - LaTeX export changes: `tools/dev/latex.sh <shots>/latex` or `LATEX_OUT=dir make test` + latex.sh.
4. **Keep the docs in step** with each feature: README, UI_GUIDE, QUICKSTART, this file, gallery/tutorial
   when user-facing, a formula-reference entry for new formula functions. Move finished TODO items to
   `FEATURES.md` (never delete open TODO items).
5. **Report honestly**: what was done, what was checked and how, what is untested (real mouse input,
   real GPU), and bugs found along the way.
6. **Parallel sessions** (e.g. a forked subtask) may edit the same working tree: commit only your own
   files and check `git status` before committing.

## Build and test

```bash
make build   # bin/linalg_viz (needs raylib 5.5 dev headers)
make run
make test    # bin/run_tests: math, expression engine, scene, animation, explanations (no raylib)
make fonts   # regenerate src/ui/generated/fonts.inc from assets/fonts
```

- Compiler: `clang++ -Wall -Wextra -O2 -std=c++20`. Keep the build **warning-free**
  (multi-line string literals inside arrays need parentheses for -Wstring-concatenation).
- `SRCS` globs every `src/**/*.cpp`; the test target builds `src/math/**`, `src/animation/*`,
  `src/scene/*`, `src/export/*`, `src/ui/SceneIO.cpp` and `tests/*`.
- **raylib 5.5 is required**: the bundled raygui 5.1-dev calls `TextToFloat` (new in 5.5), and the UI
  uses `DrawRectangleRoundedLinesEx` and `ToggleBorderlessWindowed`. Alpine's `raylib-dev` is 5.0: too old.

### Visual verification (important for any UI change)

`linalg_viz --tour DIR [--size WxH] [--theme dark|light]` runs `src/app/Tour.cpp`: a scripted walk
through every mode, panel, popover and tooltip that saves PNGs and exits (preferences are not read or
written during the tour). On this machine (Alpine, no display) the workflow is:

The `tools/dev/` scripts automate the steps below (see Development process).

1. raylib 5.5 is built from source as a static lib (`tools/dev/raylib.sh`; clone tag 5.5, compile
   `rcore rshapes rtextures rtext rmodels utils raudio` with `-DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33
   -D_GLFW_X11` plus `rglfw.c` with `-D_GLFW_X11`, then `ar rcs`).
2. Build: `make build BIN_DIR=<dir> INCLUDES="-Isrc/include -Iinclude -isystem <raylib>/src"
   LDLIBS="<raylib>/src/libraylib.a -lGL -lX11 -lm -lpthread -ldl"`.
3. Run headless: `xvfb-run -a -s "-screen 0 <W+100>x<H+100>x24" sh -c "timeout 120 <bin> --size WxH --tour <out>"`
   (put the timeout inside xvfb-run, otherwise the app survives the wrapper). Then view the PNGs.
4. Check 800×600, 1600×900, 1920×1080 (light) and 2560×1440 before committing UI work.

Installed packages that make this possible: xvfb, xvfb-run, mesa-dri-gallium, mesa-dev, libxcursor-dev,
libxrandr-dev, libxinerama-dev, libxi-dev; for LaTeX export checks: texlive, texlive-pictures,
texlive-latexextra, poppler-utils (installed by the user; I cannot use sudo). Frame captures must flush the render batch first
(`rlDrawRenderBatchActive()` before `LoadImageFromScreen()`), or the last draw calls are missing.

## Architecture

```
src/
  main.cpp            option parsing (--size, --theme, --tour)
  app/App             owns everything: Scene, SceneView, main loop, input/shortcuts, picking, panels,
                      files, preferences, screenshots; public API used by edu/ and Tour
  app/Tour            scripted screenshot tour (mouse positions come from ui anchors)
  math/               NO raylib: Vector3D, Matrix3D/4D, Decompositions (LU/QR/eigen3/svd3), Complex,
                      Quaternion, Breakdown (SVD steps), Explain (worked steps), Expression (scalar wrapper)
  math/expr/          NO raylib: typed expression engine — Value (number, complex, vector, 3x3/4x4
                      matrix, error), Program (compile once, names used, eval with Resolver + x/y/t/u slots);
                      Reference (the in-app formula cheat sheet as data: sections, entries, examples)
  scene/              NO raylib: SceneObject (expression fields + props + value), Space (objects, unique
                      names, rename with reference rewrite, dependency-ordered evaluation, cycles),
                      Types (object classes + registry), Scene (2 spaces, selection, undo, files v2 + v1 import)
  export/             NO raylib: LatexExport (space -> black-and-white PGFPlots figure; shared style table)
  view/SceneView      draws a space's objects (per-type), labels, colors, selection halo, plot cache,
                      black-and-white print preview (DrawOptions.print)
  animation/          NO raylib: Playback (Once/Loop/PingPong + 9 easings; phaseAt(seconds) is pure; save/load)
  edu/                Tutorial lessons (setup/check/anchor), Examples (gallery entries), built on App API
  rendering/          Grid (lines every unit, major every 5, fading edge; size = ViewSettings::gridSize ±5..100),
                      VectorRenderer::drawArrow, PlotRenderer (GPU meshes from expressions + resolver),
                      CameraController, Labels, WorldMapping (math z-up ↔ raylib y-up)
  ui/core/            Ui (scale, fonts, text, primitives, interaction, clip, anchors, tooltips), Theme,
                      Layout, Icons, FloatingPanel + PanelManager
  ui/                 ScenePanel (objects list + inspector), AddMenu (types + templates from the
                      selection), Widgets, TopBar, SideBar, ViewPanel, InfoPanel (analysis of the
                      selection), TimePanel (scene clock), ReferencePanel (formula cheat sheet, F2),
                      ExportPanel (LaTeX export, Ctrl+E), FileDialog (open/save .txt scenes, .tex figures), StepsPanel, BreakdownPanel, GalleryPanel, TutorialPanel,
                      SettingsPopover, ColorPopover, HelpOverlay, SceneIO, Preferences, RayguiImpl.cpp
  ui/generated/       fonts.inc (embedded fonts, subset; codepoint list)
  include/<area>/     headers mirror the folders above
tests/test_math.cpp   assert-style runner (CHECK / CHECK_NEAR)
assets/fonts/         subset TTFs + OFL licenses; tools/fonts.py subsets (fonttools) and embeds
docs/screenshots/     images for the docs, produced by --tour
```

### Scene model (the extension point for new math)
- An object = expression **fields** (`Field`: key, label, `Expect` type, text, compiled program, error,
  value) + string **props** (flags, sizes, modes) + a computed **value** (`expr::Value`) that other
  objects use by name. Objects without a value (plots) return the marker `fail("no value")`.
- `Space::evaluate()` runs every frame: dependencies come from the names in active fields, evaluation is
  topological, cycles and missing names become object errors, `changedAt()` tells views what to redraw.
- `Scene::commit()` after each user change pushes an undo snapshot (the whole scene serialized).
- 3D space uses math coordinates with z up; draw with `mathToWorld()` (rendering/WorldMapping.hpp).
- **Time**: `Scene::Timeline` (time, playing, speed, loop, length; 0 = auto from the longest Once/PingPong
  animation) is the only clock. `Scene::advance(dt)` moves it; `Scene::evaluate()` calls
  `SceneObject::setTime()` on every object first. It is saved in files (`time.*`) but left out of undo
  snapshots (`save(data, false)`), so undo never rewinds time. There is no Animator any more.
- **`t` = scene time in every formula**: `SceneObject::provideSlots` fills `t` from `setTime` for all
  fields (plot fields get sample x/y/u plus the real t). Objects whose active fields use t
  (`usesTime()`) count as changed when the time changes, so plots using t rebuild while the clock plays.
  `PlotSpec::time` carries it into `PlotRenderer` (`ExprVars{x, y, t, u}`).
- **Curves use `u`**: the u slot exists only when compiling with `curveVariable` (`Field::curveVariable`,
  set by `markCurveVariable` for curve x/y/z; `Expression::parse(..., curveVariable)` in the renderer).
  Elsewhere `u` is a normal name, so objects named u keep their dependencies. Ranges are `umin/umax`;
  old files (`tmin/tmax`, t in curve fields) are converted in `Scene::restore` and `loadV1`.
- Parameters: `mode` prop slider|animate|formula. Slider is static. Animate is a pure function of the time
  (`animatedValue(t)` from props `anim.repeat/easing/duration/start`), so scrubbing works. Old files: per-
  parameter clocks (`playing`, `clock`, `period`, `speed`, `loop`, `anim.phase`) are dropped in
  `Scene::restore` and a playing parameter starts the timeline; old Animation-panel keyframes become
  `K = keys(s, ...)` (`App::convertOldKeyframes`).
- **Startup**: App clears both spaces and the history (`Scene::clearAll/clearHistory`) except during the tour,
  which keeps the demo scene (`Scene()` still builds it, and tests rely on that). `loadDefaultScene()` is the
  undoable demo; `resetToDefault()` also resets the undo baseline.
- **Matrix presets accumulate**: `MatrixObject::applyPreset(P)` writes P·(current value) into the cells
  (identity starts over; 4×4 keeps its size). App re-evaluates before applying.
- Morphs are formulas: `lerp(a, b, s)` (any value kind, `0` = zero of the other kind), `keys(s, ...)`.
  Add menu templates with `AddChoice::helperParameter` create a slider `s` first and substitute `$s`.
- Plots: `density` prop = samples (cells) per unit; `PlotObject::samples()` gives capped counts, which
  SceneView passes to `PlotRenderer` as `samplesX/Y`. Old `resolution` props are converted after loading
  (`Scene::convertOldPlots`). PlotRenderer uploads meshes once per change (indexed surface bands under
  65 536 vertices, curve tubes, heat map) and frees them in its destructor (CPU-only after CloseWindow).
  `PlotSpec::smooth` picks smooth surfaces (per-vertex color + slope shading) or faceted ones (flat cells,
  plain triangle list); it comes from the Settings preference `UiSettings::smoothSurfaces`
  (`ui.smooth_surfaces`) through `SceneView::setSmoothSurfaces`, and is part of the plot cache key.

### LaTeX export
- `latex::exportLatex(space, selection, options)` writes a `tikzpicture` + PGFPlots `axis`.
  - 2D or 3D: Auto picks 2D for the complex plane or when everything lies in z = 0.
  - `view` comes from the camera via `viewFromDirection`.
  - Plots are sampled with `Expression` (resolver = the space, t = the object's time) at capped counts.
  - Implicit curves use `math/Contour` (marching squares + chaining, shared with `PlotRenderer`).
- **Style table:** `styleFor(index)` / `styleIndexOf` give each exported object a dash pattern and gray. The
  print preview uses the same table (`DrawOptions.print`, `PlotSpec.print/dash/grayShading`), so the preview
  matches the export.
- **PGFPlots quirks handled:**
  - set `height` as well as `width` (otherwise a small default height shrinks equal-aspect and 3D figures);
  - 3D axes with `axis lines=center` are sized by their hidden bounding box, so width and height are
    enlarged by the box/axes projected ratio (a comment in the output explains it);
  - labels at the same point are stacked;
  - surfaces use `axis lines=left`.
- **Adding export support for a new object type:** a branch in `exportLatex` (plus a helper like
  `exportVector`), a decision in the Auto 2D/3D rule, and print-preview drawing in
  `SceneView::drawEuclideanPrint`/`drawComplexPrint`.
- **Verification:** `LATEX_OUT=<dir> make test` writes example standalone figures. The tour writes figures of
  nine gallery scenes to `<shots>/latex/`. Compile them with `pdflatex` (TeX Live + poppler-utils are
  installed) and view them with `pdftoppm -png`.

### Formula reference
`math/expr/Reference.cpp` holds the cheat sheet shown by `ui/ReferencePanel` (F2). `testReference` checks that
every example compiles and evaluates (unless it is marked `usesObjects`) and that every name in the
function table (`functionNames()`) appears in some entry's syntax. **When adding a function or syntax,
add an entry**, or the test fails. The panel's section chips (`kChips` in ReferencePanel.cpp) follow the
section order.

### Adding a new object type (e.g. quaternion, vector field, tensor)
1. Model in `src/scene/Types.{hpp,cpp}`: subclass `SceneObject`; in the constructor `addField(...)` and
   `setProp(...)`; override `computeValue()` (and `fieldActive()` if fields depend on a mode); implement
   `makeEmpty()`. Register it in `objectTypes()` (id, label, category, base name, spaces, description).
2. New value kinds go in `expr::Value` (+ operators/functions in `src/math/expr/Expr.cpp`).
3. Drawing: a branch in `view/SceneView.cpp` (`drawEuclidean` / `drawComplex`, labels, `anchorPoint`
   for picking, `colorOf` palette).
4. Inspector: a branch in `ScenePanel::drawInspector` (reuse `fieldRow`/`editText`; commit with
   `sc.commit()`); optional templates in `AddMenu::templates`; optional analysis in `InfoPanel::update`.
5. Tests in `tests/test_math.cpp` (model + evaluation), tour step + screenshots for the UI.
Saving/loading is automatic (fields and props are serialized generically).

## UI conventions

- **Design units**: layout is written for 1600×900; `ui::px(d)` converts to pixels (scale follows the
  window, clamped to ≥0.75, times the user zoom 0.7–2.0). Never use raw pixel numbers or `DrawText`;
  use `ui::text/textIn/textWrapped` with a `FontRole` and `ui::theme()` colors.
- **Fonts and glyphs**: the subset contains Latin-1, Greek, sub/superscript digits and some math symbols
  (see `tools/fonts.py`). Missing: ∠ ⋅ ⊥ ∈ ⟨⟩ ∇ ∘, subscript letters (ᵢ ⱼ) and ᵀ; the mono font also lacks
  ⁻ and ⁿ. Write `^T`, `C(i,j)`, `angle(A, B)`, and keep `M⁻¹` in Inter text.
- **Panels** are content functions `float drawContent(Rectangle area, ...)` returning their height; App
  wraps them in `ui::FloatingPanel` (`begin(height, content)` / `end()`), registered with the
  `PanelManager`. Side-bar panels use `draw(x, y, width)` and return the bottom y.
- **Inspector fields** use `ScenePanel::editText` (an edit buffer for the active field; commits on
  Enter/Tab/click-away, Esc cancels). Commit lambdas capture the object id, never a pointer.
- **Input ownership**: widgets check `ui::hovered()`, which respects `GuiLock` (camera mode, overlays,
  panels not on top), the clip rectangle (scroll containers) and popovers. raygui is only used for
  text boxes (drawn transparent inside our rounded box) and the color picker.
- **Text fields**: `ui::textField(rect, buf, size, id, activeId, invalid)`. Esc never quits
  (`SetExitKey(KEY_NULL)`): it cancels an edit, closes a popover (add menu, color, settings) or the help,
  or leaves camera mode, in that order.
- **Section headers** uppercase their title; pass `keepCase = true` when it contains object names
  (names are case-sensitive).
- **Tooltips**: `ui::tooltip(rect, body, title)` after drawing a widget; `segmented()` takes per-segment tips.
- **Playback widgets** (Widgets): `repeatControl` (Once/Loop/Ping-pong), `easingButton` + `easingGrid`
  (curve previews; the open/closed state lives in the owning panel), `easingCurve`.
- **Anchors**: `ui::anchor("id", rect)` names a control so tutorial steps can highlight it.
- **Theme colors** are read at draw time; the plot cache in SceneView rebuilds when the theme or an
  object's color changes.
- Scenes (saved through `ui/FileDialog`: Ctrl+S/F5, Ctrl+Shift+S, Ctrl+O/F9; current file and recent files in
  App, recent list and last folder in preferences as `files.*`) hold both spaces' objects (format v2; v1 files are converted), camera,
  view and the timeline; preferences (`~/.config/axismundi/settings.txt`) hold theme, zoom, toggles, panel
  layout, finished lessons, window size. Both use the `key = value` format from `SceneIO`.

## Git workflow

- Work on branch **`claude`** (tracks `origin/claude`, remote `git@gitbro:axismundi.git`).
- Commit in logical groups and **push every commit**. **Never open pull requests unless the
  user explicitly asks.**
- No git identity is configured on this machine. Commit with
  `git -c user.name="Diego Rosalio" -c user.email="diegorosalio19@gmail.com" commit ...`
  and end messages with the `Co-Authored-By` trailer. Don't modify the git config.
- Pushing needs an SSH key in the session; on "Permission denied (publickey)" ask the user.

## Performance notes

Measured under xvfb with Mesa llvmpipe (software rendering, so real GPUs are far faster): a 400×400
surface (161k vertices, indexed) draws in ~57 ms/frame (unindexed it was ~140 ms); rebuilding it takes
~0.2 s, which happens every frame if a time-driven parameter feeds the surface while the clock plays. Wireframe lines are still
drawn per frame (capped at ~60 lines × ~150 segments per direction).

## Tutorials

Lesson steps have `setup`/`check`; App calls `TutorialPanel::update` every frame while the tutorial panel is
visible (this call was lost once, silently disabling every lesson). After changing lessons, walk them: a
temporary tour loop that enters each step, applies a scripted solution through the App API and logs
whether `check` passes (and does not pass before the solution) caught that regression.
`tools/lesson_walk.py src/app/Tour.cpp` inserts that loop (solutions are matched by step title; add one for
a new step), then build, run the tour, grep `WALK`/`SOLVED` in stderr, and restore Tour.cpp.

## Not yet verified

The tour exercises everything visually, but real mouse dragging, typing feel, viewport picking with a
real mouse, fullscreen toggling and the preferences round trip (written on normal exit) have only been
checked in code, not interactively. Plot frame rates on real GPU hardware have not been measured.
