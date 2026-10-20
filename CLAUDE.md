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
   - LaTeX export changes: `tools/dev/latex.sh <shots>/latex` or `LATEX_OUT=dir make test` + latex.sh;
   - input handling (clicks, typing, editors, dialogs): `tools/dev/input-test.sh` drives the real app with
     xdotool under Xvfb and saves a PNG per step to `$DEV/input` (helpers in `tools/dev/input-lib.sh`,
     which can also be sourced by hand to explore).
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

- Compiler: `clang++ -Wall -Wextra -Wshadow -O2 -std=c++20`. Keep the build **warning-free**
  (multi-line string literals inside arrays need parentheses for -Wstring-concatenation).
- `SRCS` globs every `src/**/*.cpp`; the test target builds `src/math/**`, `src/text/*`, `src/animation/*`,
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

Screenshots on demand: the running app saves `signal_NNN.png` in its working folder when it receives
SIGUSR1 (used by `input-lib.sh`). Real-input notes: GLFW ignores xdotool's `--window` key events, so keys go
through XTEST (`xdotool key` with the pointer inside the window); BusyBox `pgrep -x` doesn't match, use plain
`pgrep linalg_viz`.

Installed packages that make this possible: xdotool, xvfb, xvfb-run, mesa-dri-gallium, mesa-dev, libxcursor-dev,
libxrandr-dev, libxinerama-dev, libxi-dev; for LaTeX export checks: texlive, texlive-pictures,
texlive-latexextra, poppler-utils (installed by the user; I cannot use sudo). Frame captures must flush the render batch first
(`rlDrawRenderBatchActive()` before `LoadImageFromScreen()`), or the last draw calls are missing.

## Architecture

```
src/
  main.cpp            option parsing (--size, --theme, --tour)
  app/App             owns everything: Scene, SceneView, main loop, panels, screenshots; public API used by
                      edu/ and Tour. One class in four files: App.cpp (construction, scripting API, run,
                      update), AppInput.cpp (keys, shortcuts, picking), AppDraw.cpp (scene, UI, panels, HUD),
                      AppFiles.cpp (scene/LaTeX files, preferences)
  app/Tour            scripted screenshot tour (mouse positions come from ui anchors)
  math/               NO raylib: Vector3D, Matrix3D/4D, Decompositions (LU/QR/eigen3/svd3), Complex,
                      Quaternion, Breakdown (SVD steps), Explain (worked steps), Expression (scalar wrapper),
                      VectorField (arrows on a grid, RK4 flow lines kept apart, particles as a function of t),
                      Integrals (Riemann boxes, Simpson, line integrals), CurveGeometry (Frenet frame, κ, τ),
                      Fourier (curve coefficients for epicycles, series a_k/b_k), Derivatives (five-point f',
                      f'', partials, r'/r''), OrbitCamera (pivot/distance/yaw/pitch
                      in world y-up axes)
  math/expr/          NO raylib: typed expression engine — Value (number, complex, vector, 3x3/4x4
                      matrix, quaternion, error; rotationOf gives a quaternion's axis/angle/matrix), Program (compile once, names used, eval with Resolver + x/y/t/u slots);
                      Reference (the in-app formula cheat sheet as data: sections, entries, examples)
  scene/              NO raylib: SceneObject (expression fields + props + value), Space (objects, unique
                      names, rename with reference rewrite, dependency-ordered evaluation, cycles),
                      Types (basic object classes + registry), TypesAdvanced (quaternions, fields, integrals,
                      frames, derivatives, Fourier), Scene (2 spaces, selection, undo, files v2 + v1 import)
  text/               NO raylib: TextLayout (wrapping formulas into lines, cursor ↔ line/column, UTF-8 steps),
                      LabelLayout (placing screen labels without overlaps)
  export/             NO raylib: LatexExport (space -> black-and-white PGFPlots figure; shared style table; the
                      axis), LatexObjects (one export function per type + the type table), LatexFigure (pieces)
  view/SceneView      draws a space: axes, the plot/field caches, labels (laid out, no overlaps), selection
                      halo + fading, solo filter (`shown`), Paper = the print path (DrawOptions.print); each object
                      through its type's ObjectView (view/ObjectViews.hpp: draw, anchor, label, pick, palette),
                      registered per family in view/objects/{Basics,Complex,Quaternion,Calculus,Fourier}.cpp;
                      SceneViewDraw.{hpp,cpp} the shared drawing helpers
  view/AxisNumbers    tick numbers along the axes (step = GridMath::tickStep of the zoom; App::pixelsPerUnit from
                      CameraController::viewDistance)
  view/ViewStyle      view styles as data (Colorful, Paper, High contrast, Chalkboard): `styleFor(id, theme)`,
                      `activeStyle()` (set by App each frame, read by SceneView, Grid colors, the objects list)
  animation/          NO raylib: Playback (Once/Loop/PingPong + 9 easings; phaseAt(seconds) is pure; save/load)
  edu/                Tutorial lessons (setup/check/anchor), Examples (gallery entries), built on App API
  rendering/          CameraController (Orbit = math/OrbitCamera around a pivot, or Fly = mouse look + WASD;
                      `camera.mode` preference; target = the real pivot, files with a 1-unit target get one
                      from `pivotAlong`), GridShader (the endless adaptive grid: one quad per
                      plane, GLSL 330 fragment shader with fwidth lines, families from math/GridMath, polar mode;
                      the project's only shader: strings in the .cpp, LoadShaderFromMemory, valid() false ->
                      fallback; AXISMUNDI_NO_SHADER=1 forces the fallback), Grid (the fallback line grid),
                      VectorRenderer (drawVectorArrow: the vector look, ~2 px shaft, head 30% of the length up to
                      ~16 px, outline when selected; drawArrow clamps given sizes in pixels; pixelsAt(point, px) from
                      setScreenScale, set each frame from DrawOptions.cameraPosition/worldPerPixel), PlotRenderer (GPU meshes from expressions + resolver),
                      FieldRenderer (vector field arrows/lines/particles), MeshBuilder (MeshData, tubes, cones,
                      MeshSet: shared by plots and fields), CameraController, Labels, WorldMapping (math z-up ↔ raylib y-up)
  ui/core/            Ui (scale, fonts, text, primitives, interaction, clip, anchors, tooltips, click
                      latch), TextEditor (wrapping multi-line editor), Theme, Layout, Icons,
                      FloatingPanel + PanelManager
  ui/                 ScenePanel (objects list + inspector; ScenePanelInspectors.cpp has one inspect<Type>
                      method per object type, with rangeRow/toggleProp helpers), AddMenu (types + templates from the
                      selection), Widgets, TopBar, SideBar, ViewPanel, InfoPanel (analysis of the
                      selection), TimePanel (scene clock), ReferencePanel (formula cheat sheet, F2),
                      ExportPanel (LaTeX export, Ctrl+E), FormulaEditor (big
                      formula editor with live result, Ctrl+Enter), FileDialog (open/save .txt scenes, .tex figures), StepsPanel, BreakdownPanel, GalleryPanel, TutorialPanel,
                      SettingsPopover, ColorPopover, HelpOverlay, SceneIO, Preferences, RayguiImpl.cpp
  ui/generated/       fonts.inc (embedded fonts, subset; codepoint list)
  include/<area>/     headers mirror the folders above
tests/                main.cpp (runner + list), TestSupport.hpp (CHECK / CHECK_NEAR, helpers), test_{linear,expr,
                      calculus,scene,export,text}.cpp; test_scene includes a check over every registered type
docs/architecture.md  how the pieces fit together (data flow per frame, where to add things)
docs/learn/           a guide to the code for C++ beginners (9 chapters + glossary). Its ```cpp snippets are
                      real code: when renaming or moving what a snippet shows, update the guide too
assets/fonts/         subset TTFs + OFL licenses; tools/fonts.py subsets (fonttools) and embeds
data/                 desktop integration, app ID com.bisonanon.axismundi: the .desktop file and icons/hicolor
                      (scalable SVG, PNG sizes, symbolic SVG), drawn by tools/icon.py (`make icon`, GNOME icon
                      style: light tile with a darker bottom edge, GNOME palette), which also writes
                      src/ui/generated/icon.inc (ui/AppIcon: window icons + the top bar logo). `make install`
                      copies them under PREFIX (~/.local). The window title "axismundi" is also GLFW's X11
                      WM_CLASS, matched by StartupWMClass in the desktop file: keep them equal
packaging/arch/       PKGBUILD + .SRCINFO for axismundi-git (AUR style; source = this clone via git+file, branch
                      main; honors makepkg flags; check() = make test; package() = make install DESTDIR PREFIX=/usr,
                      which skips the desktop/icon caches when DESTDIR is set). Not testable here (no makepkg):
                      the user builds it on Arch
LICENSE               GPL-3.0-or-later (official text, unmodified; the copyright line is in README's License)
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
- **Expensive values** (integrals, series, epicycles): override `cachesValue()` so `computeValue` runs only
  when a field, property, input or the time (formulas of t, or `followsClock()`) changed; `evaluationResolver()`
  gives it the other objects; `inputsChangedNow()` tells "only the time moved" (epicycles keep their
  coefficients and just move the tip). Drawn-only geometry is kept in `mutable` members filled by computeValue
  and read by SceneView and the export. Fields with the curve variable get sample slots even when complex.
- **Vector fields use `z`**: `Program::compile(text, variables)` takes `expr::kVarU` (curves) and `expr::kVarZ`
  (fields; `true` still means kVarU). `Field::zVariable` (`markFieldVariable`) and `Expect::Field` (a vector
  checked with sample x/y/z/t). Elsewhere z is a normal name. `VectorFieldObject` ("field") has no value; it
  overrides `followsClock()` (particles) so the Time panel lists it without rebuilding every frame.
  `math/VectorField::fieldFlowLines` holds the flow-line settings shared by FieldRenderer and the export.
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

### View styles
- A style changes only the 3D view (panels keep the theme). `ViewSettings::style` is saved in scene files
  (`view.style`; files without it keep the current style) and as the preference `view.style` (startup, New).
- **Adding a style:** a `StyleId`, a row in `kStyles` (key, name, description) and a `case` in `styleFor`
  that adjusts the Colorful values (palettes, background, grid, `lineScale`, `labelLarge`, `heatSoften`,
  `smoothDefault`). Drawing code reads only the style, never `ui::theme()` for scene colors. The View section
  shows 2×2 cards (`ViewPanel::styleCard`); more styles need another card row.
- The export panel's B/W preview is synced with Paper in `App::update` (`m_previewShown`, `m_styleBeforePaper`).
- Sizes on screen: arrows, axes and point markers (`draw::dot`) are sized in pixels through
  `VectorRenderer::pixelsAt`, so zooming in never makes them fat; plots, fields and surfaces keep world sizes.
- Selection: `DrawOptions.dimUnselected` (Settings `ui.dim_unselected`, off during lessons and the breakdown)
  fades the others by blending toward the background (not alpha: keeps depth right; plots use `draw(fade)`);
  `DrawOptions.solo` (`ViewSettings::soloSelection`, key O) draws and picks only the selection.

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
- **PDF:** `export/PdfCompile` (no raylib): `latexCompiler()` (pdflatex, else lualatex, via PATH), `compilePdf`
  (mkdtemp, fork + execl with an argument list, nonstop mode, timeout kill, copy, cleanup; errors from
  `firstLatexErrors`). ExportPanel runs it on a `std::thread` (atomic running flag, mutex-guarded result,
  joined in the destructor); App polls `takePdfResult` in `update` and toasts. `PDF_TEST=1 make test` compiles.
- **Adding export support for a new object type:** an `export<Type>` function and its row in the table in
  `export/LatexObjects.cpp` (`detail::objectExport`: the writer plus a `flat` test for the Auto 2D rule; nullptr
  = needs 3D), and Paper drawing in its `ObjectView::draw` (`ctx.print`). Shared pieces (Figure, arrow, line,
  labelNode, num, Sampler) are in `export/LatexFigure.hpp`; `LatexExport.cpp` builds the axis around them.
- **Verification:** `LATEX_OUT=<dir> make test` writes example standalone figures. The tour writes figures of
  nine gallery scenes to `<shots>/latex/`. Compile them with `pdflatex` (TeX Live + poppler-utils are
  installed) and view them with `pdftoppm -png`.

### Formula functions and names
Function names are reserved as object names, except entries marked `nameable` in `kFunctions` (common words
added later: `rotate`, `mat`, `axis`, `angle`): followed by `(` they are the function, otherwise a name, so old
scenes with such objects keep working. Quaternions live in `Value` as w = `re` plus `qi qj qk` (doubles); the
Hamilton product and slerp helpers are in Expr.cpp.

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
3. Drawing: one `ObjectView` entry in `view/objects/<Family>.cpp` (its `register…Views` function): `palette`,
   `draw` (normal and Paper via `ctx.print`), `anchor` (label + picking handle), `label` text, `pick`
   segments. SceneView and App's picking loop over the objects and call these; no if-chains to extend.
4. Inspector: an `inspect<Type>` method in `ScenePanelInspectors.cpp` plus its row in `ScenePanel::typeUi`
   (inspector + list summary; reuse `fieldRow`, `rangeRow`, `toggleProp`; commit with `sc.commit()`);
   optional templates in `AddMenu::templates`; optional analysis in `InfoPanel::update`.
5. Tests in the topic file under `tests/` (model + evaluation; add the function to the list in `tests/main.cpp`),
   tour step + screenshots for the UI.
6. LaTeX export: an `export<Type>` + its row in `export/LatexObjects.cpp` (with the flat test) + Paper drawing
   (see LaTeX export).
Saving/loading is automatic (fields and props are serialized generically).
Notes from adding vector fields: give drawn-only types a cache in SceneView like `PlotCache` (keyed on
`changedAt`, style, theme, color); per-frame animation that only depends on the time (particles) belongs in
`draw(time)`, not in the cached geometry; new formula variables need a slot + flag (see `z`); a type that moves
with the clock without using t overrides `followsClock()`. `setProp(key, "text")` is safe (a `const char*`
overload; before it, a literal picked the bool overload).

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
- **Inspector and dialog fields** use `ui::textEditor` (TextEditor.hpp: our own editor, since raygui's box is
  single-line): wraps by width, cursor/selection/clipboard, Enter = Commit, Ctrl+Enter = Expand, a click
  outside = Commit. `selectAllOnFocus` for short fields (cells, names), cursor-at-click for formulas.
  One `EditorState` is shared by all inline fields: only the active field may touch it. The inspector's
  `fieldRow` sizes the field with `editorHeight(text, width, 4)` and adds the ⤢ button (FormulaEditor).
- **The 3D view's mouse** (`App::handleViewMouse`): a press on the empty view (`viewUnderMouse`: not the bars,
  panels or popovers) starts a drag; moving more than 4 px makes it an orbit (left/middle) or pan (right or
  Shift) with the orbit camera, releasing before that is a click that picks (`pickAt`). The wheel zooms only
  over the view. Panels that scroll must keep checking their own bounds.
- **Mouse buttons**: use `ui::mousePressed/mouseReleased`, never raylib's `IsMouseButtonPressed/Released`.
  A GLFW mouse-button callback (installed by `ui::installClickLatch()` after InitWindow, chaining raylib's)
  latches presses, so a touchpad tap that presses and releases within one frame is not lost. It needs the
  GLFW header from raylib's source tree (`__has_include`); with a system raylib the latch is off.
- **Side bar collapse**: `SideBar::collapsed()/setCollapsed()`; `width()` follows, so panels, picking and
  the HUD adapt. Ctrl+B, the top bar button and the strip's chevron toggle it (`ui.sidebar_collapsed`).
- **Text fields** (raygui, older panels): `ui::textField(rect, buf, size, id, activeId, invalid)`. Esc never quits
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

- Work on branch **`feature/gtkui`** (the GTK4 port, see `docs/gtk-port.md`; remote `git@gitbro:axismundi.git`).
  `main` holds releases; the old `claude` branch was merged for v0.1.0 and deleted.
- Commit in logical groups and **push every commit**. **Never open pull requests unless the
  user explicitly asks.**
- No git identity is configured on this machine. Commit with
  `git -c user.name="Diego Rosalio" -c user.email="diegorosalio19@gmail.com" commit ...`
  and end messages with the `Co-Authored-By` trailer. Don't modify the git config.
- Pushing needs an SSH key in the session; on "Permission denied (publickey)" ask the user.

## Releases

Semantic versions (`vMAJOR.MINOR.PATCH`). To release: bump `kVersion` in `src/include/app/Version.hpp` (shown by
`--version` and the keyboard help) and `pkgver` in `packaging/arch/PKGBUILD` + `.SRCINFO` on the work branch, merge
it into `main` with `git merge --no-ff`, put an annotated tag `vX.Y.Z` on the merge commit, push `main` and
the tag. The PKGBUILD's `pkgver()` derives the package version from the latest tag.

## Performance notes

Measured under xvfb with Mesa llvmpipe (software rendering, so real GPUs are far faster): a 400×400
surface (161k vertices, indexed) draws in ~57 ms/frame (unindexed it was ~140 ms); rebuilding it takes
~0.2 s, which happens every frame if a time-driven parameter feeds the surface while the clock plays. Wireframe lines are still
drawn per frame (capped at ~60 lines × ~150 segments per direction).

## Tutorials

Finished lessons are saved as a bitmask by position (`tutorial.completed`): add new lessons at the end of
`lessons()` (or convert the preference) so existing progress keeps pointing at the right lessons.

Lesson steps have `setup`/`check`; App calls `TutorialPanel::update` every frame while the tutorial panel is
visible (this call was lost once, silently disabling every lesson). After changing lessons, walk them: a
temporary tour loop that enters each step, applies a scripted solution through the App API and logs
whether `check` passes (and does not pass before the solution) caught that regression.
`tools/lesson_walk.py src/app/Tour.cpp` inserts that loop (solutions are matched by step title; add one for
a new step), then build, run the tour, grep `WALK`/`SOLVED` in stderr, and restore Tour.cpp.

## Not yet verified

The tour exercises everything visually, and `tools/dev/input-test.sh` checks clicks, taps, drags, typing,
picking in the view, the file dialog, the orbit camera (drag, right-drag, wheel, .) and fly camera mode with real X11 events under Xvfb. Not checked: the feel
on real hardware (a physical mouse, touchpad, keyboard layouts, HiDPI), fullscreen toggling and the
preferences round trip (written on normal exit). Plot frame rates on real GPU hardware have not been measured.
