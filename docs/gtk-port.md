# The GTK4 port (branch `feature/gtkui`)

The progress file of the port: the plan, what is done, what's next. Updated at the end of every run, so work can
resume from here after a break.

## Goal

A GTK4 + libadwaita front end (gtkmm-4.0 for the C++ side, libadwaita's C API for its widgets), so axismundi looks
and behaves like a GNOME app.

- **The raylib front end keeps working** until the GTK one does everything; both are built from the same core.
- **The 3D view is still drawn with raylib's GL layer** (rlgl) inside a `Gtk::GLArea`.
- **A later switch away from raylib stays possible:** drawing goes through a small interface (`gfx::Canvas3D`), so a
  plain-OpenGL implementation can replace raylib without touching the drawing code.

### Dependencies

- **Arch:** `sudo pacman -S --needed gtkmm-4.0 libadwaita adwaita-icon-theme pkgconf` (GNOME already has the
  icon theme).
- **Alpine (the dev box):** `gtk4.0-dev gtkmm4-dev libadwaita-dev libepoxy-dev adwaita-fonts font-dejavu
  adwaita-icon-theme` (all installed).

### Layout

- **Shared core:** `src/math`, `src/scene`, `src/export`, `src/text`, `src/animation`, `src/view`, `src/rendering`.
- **Only in the raylib app:** `src/ui`, `src/app`, `src/main.cpp`.
- **Only in the GTK app:** `src/gtk` (built by `make gtk` → `bin/axismundi-gtk`).

## Runs

Each run ends with pushed commits that build without warnings and pass the tests. Runs marked ⛔ wait for the user.

| Run | Content | State |
|-----|---------|-------|
| 0 | dependencies installed | done |
| 1 | skeleton (`make gtk`, Adw window, screenshot mode) + spike: the scene drawn by rlgl in a GLArea, orbit/pan/zoom (⛔ if rlgl can't run in GTK) | done |
| 2 | `gfx::Canvas3D` + RaylibCanvas; labels as a list; the raylib tour pixel-identical | done |
| 3 | `Session` + `AppApi` shared by both front ends; edu/ on AppApi | done |
| 4 | GTK main window: split view, objects list, Add menu, selection, picking, undo, toasts, files | done |
| 5 | GTK inspectors I: framework + basic types | done |
| 6 | GTK inspectors II: the remaining types | done |
| 7 | GTK panels: Analysis, Time, Settings, Export/PDF, Gallery, Reference, Steps, Breakdown, Help | done |
| 8 | tutorial with popovers, GTK tour, input test, lesson walk, docs/learn chapter | done |
| 9 | parity checklist; the user tries it on Arch + GNOME (⛔) | next |
| 10 | GTK becomes `axismundi`; the raylib UI is removed | |
| later | own OpenGL renderer replaces raylib (⛔ separate decision) | |

## Log

(Newest last: per run, the commits, what was checked, open problems.)

### Run 1: skeleton + spike (2026-10-20), done

**Result: rlgl runs inside GTK's GL context**, so the 3D drawing code stays (the ⛔ stop wasn't needed).

**What exists:**
- `make gtk` → `bin/axismundi-gtk`: an AdwApplicationWindow with a header bar (the 3D space / complex plane
  switch, an AdwToggleGroup) over the 3D view.
- The 3D view (`gtk/SceneArea`, a Gtk::GLArea + `gtk/SceneRenderer`, raylib only) draws the demo scene: grid
  shader, objects, labels, axis numbers. Drag orbits, right-drag or Shift+drag pans, the wheel zooms.
- It follows the system's dark/light setting (AdwStyleManager), or `--theme`.
- `--tour DIR` saves 5 screenshots; SIGUSR1 saves `signal_NNN.png`.

**Shared-code changes**, with the raylib app pixel-identical (all 111 tour shots; only the HUD frame-rate
text differs):
- `ui::setScreenSize` (and `screenWidth/Height`) for a host that isn't raylib's window. Labels and axis numbers
  use `GetWorldToScreenEx` with that size.
- `gpuContextReady()` (`rendering/GpuContext`) replaces `IsWindowReady()` in the GPU cleanup.
- Fonts upload their own atlas: raylib 5.5 only uploads textures when it opened the window (its internal
  `isGpuReady`), and otherwise falls back to a default font that doesn't exist, which crashed.
- `ui::now` and the font-rebuild delay use a steady clock without a raylib window.

**GL details found:**
- GL functions are loaded through `eglGetProcAddress` when the current context is EGL (GTK's default, also on
  Wayland), else `glXGetProcAddressARB`. Both were checked under Xvfb (`GDK_DISABLE=egl` forces GLX).
- raylib's shapes texture is set to rlgl's real white texture, because GTK's contexts share texture names.

**Checks:**
- `make test` (4,408), `check.sh` ALL CLEAN (it now builds `src/gtk` with the GTK flags and `-Wshadow`).
- `tour-gtk.sh` at 800×600, 1280×800, 1600×900 dark.
- `input-test-gtk.sh`: drag, right-drag, wheel and the space switch with real X11 events, 0 problems.

**Open problems (for later runs):**
- ~~No antialiasing yet~~ (done in run 2).
- ~~The demo scene is fixed; no objects list, files, picking~~ (run 4). No inspector yet (run 5).
- ~~The grid and label code is duplicated from App~~ (shared through Session since run 3).
- Not tried on a real desktop: Wayland, HiDPI (the view renders at device pixels, so labels follow the scale
  automatically), touchpad gestures.

### Run 2: Canvas3D, labels as a list (2026-10-21), done

**Result: `view/` and `rendering/` call no raylib function any more** (only its value types and the header-only
raymath). A plain-OpenGL `Canvas3D` can now replace raylib by changing `src/gfx` alone, plus the label painting.

**Commits:**
- **Antialiasing in the GTK view:** a 4× multisampled framebuffer, blitted into the GLArea's (libepoxy for the
  GL calls).
- **`gfx::Canvas3D` + `RaylibCanvas`:** lines, tubes, spheres, triangles, boxes, immediate vertices, state, GPU
  meshes, shaders. `gfx::Pure.hpp` holds raylib's fade/colorToInt/overlaps/worldToScreen, bit-identical.
  MeshSet and GridShader keep canvas ids.
- **Labels as a list:**
  - `SceneView::labels` and `view::axisNumbers` fill a `gfx::Overlay` (pills, texts, leader lines) using a
    `gfx::TextMeasure`;
  - `ui::paintOverlay` paints it (both apps for now);
  - the dead `PlotRenderer/VectorRenderer::drawLabels` are removed.
- **The fly camera's keys and mouse** come in as `FlyInput` from the front end; `GpuContext` moved into `gfx/`.

**Checks:**
- The raylib tour is pixel-identical to the baseline after each step (111 shots; only the HUD frame-rate text
  differs).
- `make test`, `check.sh`, the raylib input test (fly mode included), the GTK tour and the GTK input test.

**Next (run 3):**
- `Session`: the state both front ends share (scene, view settings, camera, clock, files), plus the drawing
  sequence that `App::drawScene/drawGrid/drawSceneLabels` and `gtk::SceneRenderer` now duplicate.
- `AppApi` for edu/ and the tour.

### Run 3: Session and AppApi (2026-10-21), done

**Result:** the state and the drawing steps both front ends need live in `app/Session`; the lessons and the gallery
(`edu/`) only know `app/AppApi`, so the GTK app can run them once it implements the UI hooks.

**Commits:**
- **`scene/Templates`:** AddChoice, `addTemplates` (was `AddMenu::templates`) and `applyAddChoice` leave
  ui/AddMenu (scene logic, now unit-tested).
- **`Session`:**
  - it holds the scene, the mode (one saved camera per space), ViewSettings, the camera, SceneView, the grid,
    the drawing steps and the scripting functions;
  - App forwards to it, and ViewPanel edits its ViewSettings by reference;
  - the GTK SceneRenderer keeps only the GL setup;
  - `formatNumber` moves to `text/Numbers`.
- **`AppApi`:** App implements it, and `edu/` uses it instead of `App`. The panel and breakdown access in the
  lessons became `showPanel`, `panelVisible`, `restartBreakdown` and `breakdownFinished`.

**Checks:**
- The raylib tour is pixel-identical (111 shots).
- The lesson walk has 0 problems in 62 steps.
- `make test` (4,418) and `check.sh` pass, as do the raylib and GTK input tests and the GTK tour.

**Notes:**
- The raylib tour (`app/Tour`) still drives `App` directly. It's specific to that front end, and the GTK app has
  its own tour.
- Scene files (`App::writeScene/readScene`) still live in App, because they also save panel state. Run 4 moves
  the scene part into Session for GTK's Open/Save.

**Next (run 4):** the GTK main window:
- the split view: an objects list in the sidebar next to the 3D view;
- the Add menu (types + templates);
- selection and picking in the view;
- undo/redo, toasts, files (Gtk::FileDialog).

### Run 4: the GTK main window (2026-10-21), done

**What exists:**
- **The window:** an AdwOverlaySplitView with the objects list in the sidebar and the 3D view; below 700 px the
  sidebar floats over the view (AdwBreakpoint).
- **The header bar:** the sidebar toggle, undo/redo, the space switch, and the main menu (New, Open…, Save,
  Save As…, About with AdwAboutDialog).
- **The objects list:** a color dot, the name and type, a summary that updates with the values, and a visibility
  eye per row. Click selects; Ctrl+click and Shift+click select more.
- **The Add button:** a popover with the "From selection" templates first, then every type by category.
- **Picking:** a click on the view selects (Ctrl+click adds); a click on empty space clears the selection.
- **Toasts** (AdwToastOverlay) report adds, files and errors.
- **Keyboard:** Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y, Delete, Ctrl+D, Ctrl+N/O/S/Shift+S, Ctrl+B (sidebar),
  Alt+1/Alt+2 (spaces), Ctrl+Shift+A (Add menu), F10 (main menu).
- **Startup:** the app starts with an empty scene (the tour keeps the demo); `axismundi-gtk scene.txt` opens a
  file.

**Shared code moved out of the raylib UI:**
- `Session::pickAt`, `saveTo/loadFrom` (with the old-keyframe conversion), `deleteSelection/duplicateSelection`;
- `view/ViewSettings` (the struct and its file keys, out of ui/ViewPanel);
- `scene/Summaries` (the list summaries, out of ScenePanel::typeUi).

**Checks:**
- The raylib tour is pixel-identical, the lesson walk has 0 problems, and the raylib input test (picking, file
  dialog) passes.
- `input-test-gtk.sh`, with real X11 clicks and keys, has 0 problems in 20 checks: camera, space switch, Add menu,
  list click and Ctrl+click, templates, a click on empty space, undo, Save in the file dialog, New, Open
  (double-click), and picking the vector by its arrow. The saved file holds the vector and not the undone matrix.
- The GTK tour has 10 steps: list, selection, template, undo, Add menu, camera, hidden sidebar, complex plane,
  light theme.
- Screenshots now come from the X11 screen (`gtk/Capture`), so popovers and toasts are in them.

**Open problems:**
- **Icons:** this machine lacks the Adwaita icon theme, so the sidebar/undo/redo icons show as placeholders in
  the screenshots. The user is asked to install `adwaita-icon-theme` (GNOME on Arch has it).
- **Toasts** show only once the window has focus: the scripted tour doesn't see them, the input test does.
- ~~**Delete is a window-wide shortcut.**~~ Moved to the list and the view in run 5.
- **No window manager under Xvfb:** dialogs open at the screen's top left (the input test clicks there).

### Run 5: inspectors I (2026-10-21), done

**Before the run:** Arch ships raylib 6.2, where `LoadFontData` takes an extra glyph-count argument, so the build
failed there. `ui::loadTtf` now checks `RAYLIB_VERSION_MAJOR`; both apps were built and toured against a raylib
6.2 build (same pictures apart from antialiasing).

**What exists:**
- **The inspector** under the objects list (the divider can be dragged): the type and color in the title, the name,
  then the type's rows, then Duplicate and Delete. Nothing selected: "Select an object to edit it here."
- **Text fields** (formulas, cells, ranges, names, numbers): Enter or leaving the field commits one undo step, Esc
  goes back, a formula turns red while it doesn't parse, and a saved formula's problem ("unknown name 'q'") is
  written underneath.
- **The basic types** have their full inspectors:
  - vector: components or formula, starting point, length;
  - matrix: cells (3×3 or 4×4) with the five presets, or a formula; the determinant;
  - eigenvectors, the four plots (formulas, ranges, detail slider, samples, heat map, value range, next example);
  - parameter: slider, animate (repeat, easing, duration, start delay) or formula, the scene clock;
  - complex number (polar form, three switches) and complex product (lengths and angles).
- **The other types** list their formula fields until run 6.
- **Delete** works in the list and the view, and edits text inside fields (before, it deleted the object).
- The list scrolls to a newly selected object.

**For run 6 (the agents):** an inspector is a function `void inspectX(InspectorBuilder& b)` registered in
`inspectorFor` (gtk/InspectorsBasic.cpp); copy the raylib version in ui/ScenePanelInspectors.cpp row by row:
`fieldRow` → `formula`, `rangeRow` → `range`, `toggleProp` → `toggle`, `segmented` → `choice`, `slider` → `slider`,
`note` with live numbers → `value` or `line`, static hints → the group's description. The example lists are in
scene/Templates. Charts (the Fourier spectrum) need a small Gtk::DrawingArea.

**Checks:**
- `make test` (4,449, with a new test that every example formula works), `check.sh` ALL CLEAN.
- The GTK tour has 19 steps (vector, matrix cells and formula, parameter slider and animate, surface, a field
  problem, complex number, light theme), checked at 800×600, 1280×800 and 1600×900 light.
- `input-test-gtk.sh`: 0 problems in 38 checks. New: typing a component, Delete inside a field, Esc, the formula
  mode, a broken formula, undo, rename, Save (the file has the edits), adding a parameter and dragging its slider,
  Delete in the list and in the view.
- The raylib app: only the example lists moved (tests, build); its UI code is unchanged otherwise.

**Open problems:**
- The big formula editor (⤢, Ctrl+Enter) and the color "reset to the theme color" aren't in GTK yet (run 7, with
  the panels).
- Ctrl+Z inside a field undoes the scene (the typed text first becomes its own step), not the text.
- At small window heights the inspector gets little room under the list; the divider can be dragged.

### Run 6: inspectors II (2026-10-21), done

**Done without agents:** the plan allowed parallel agents here, but they cost extra usage, so I wrote the twelve
inspectors myself.

**What exists:** every object type has its full GTK inspector.
- `InspectorsCalculus.cpp`: double and line integrals, the Frenet frame, tangent, tangent plane, velocity, Taylor,
  Newton (its steps as a monospace table).
- `InspectorsMore.cpp`: quaternion (the vector it acts on appears with its switch), vector field (plane or volume;
  flow-line and particle sliders appear with their switches), epicycles, Fourier series (the spectrum as a small
  chart, the first coefficients).
- `Inspector.cpp` holds the one table: type → inspector function + the properties that change its rows.

**Shared code:** the quaternion and vector-field examples moved to scene/Templates, the Taylor polynomial text to
scene/Summaries (tests for both). The raylib tour stayed pixel-identical.

**Fixes found on the way:**
- After a mode switch rebuilt the rows, the keyboard focus jumped into the name field with its text selected (the
  next key would rename the object). It now goes back to the same place.
- Choices didn't follow undo; they do now.
- A new object's inspector opened at the previous object's scroll position; it starts at the top now.

**Checks:**
- `make test` (4,465), `check.sh` ALL CLEAN.
- The GTK tour has 35 steps (each type alone, the long inspectors scrolled to their end), at 800×600, 1280×800 and
  1600×900 light. It may take up to 300 s here (software rendering).
- `input-test-gtk.sh`: 0 problems in 44 checks. New: a parameter switched to Animate, Ping-pong picked in the
  Repeat drop-down, and both found in the saved file.

**Next (run 7):** the panels: Analysis, Time (the scene clock), Settings (AdwPreferencesDialog), Export + PDF,
Gallery, Reference, Steps, Breakdown, Help (shortcuts); the big formula editor and the color reset.

### Run 7: the panels (2026-10-21), done

**Fixes for the user's Arch build first:**
- raylib 6.2: `LoadFontData`'s new argument (before run 5).
- The link: `gtk/Capture` calls Xlib, and only the dev scripts passed `-lX11`. `GTK_LIBS` now adds `x11` from pkg-config.

**What exists (all in the GTK app):**
- **The pane on the right** (F9, or the header button): view-switcher pages.
  - Analysis: the sections as expander rows.
  - Steps: topic chips and the worked calculation.
  - Breakdown: the steps, controls and a slider. The view draws the matrix part-way through the renderer's frame hooks;
    picking uses the same options.
- **The scene clock** under the view: play/pause (also Space in the view), back to 0, a scrubber, the readout, loop,
  and the speed and length in a popover.
- **The View menu** (eye button): grid, axes, labels, numbers, grid spacing and planes (polar grid in the complex
  plane), the four view styles, the axis length, the camera views.
- **The main menu:**
  - Examples (also the grid button in the sidebar): the gallery, run through the window's `AppApi` (`gtk/WindowApi`).
  - Export to LaTeX (Ctrl+E): the options, the code as a live preview, Copy, Save .tex, Save PDF (compiled in a
    background thread), and a black-and-white preview.
  - Keyboard Shortcuts (Ctrl+?, AdwShortcutsDialog), Formula Reference (F1: search, try a formula, live examples).
  - Preferences (Ctrl+comma: theme, dimming, surfaces; saved in `~/.config/axismundi/gtk.txt`, not read in the tour).
- **The inspector:** ⤢ on every formula row opens the big formula editor (live result, Ctrl+Enter applies). A button next
  to a hand-picked color goes back to the view style's color.

**Shared code** (the raylib app stayed pixel-identical, and the lesson walk has 0 problems):
- scene/Analysis;
- `BreakdownPlayer` (math/Breakdown), and `StepsInput` / `stepTopics` / `explainTopic` (math/Explain);
- `compactValue` / `evaluateExample` (math/Expr/Reference);
- `fieldPreview` (scene/Summaries);
- `Scene::togglePlay`;
- `Session::stepsInput` and `applyBreakdown`;
- Preferences take a file name.

**Checks:**
- `make test` (4,506), `check.sh` ALL CLEAN.
- The GTK tour has 52 steps.
- The GTK input test still has 0 problems. Its pick points moved: the clock bar makes the view shorter.

**Open problems:**
- **On the Breakdown page,** tiny dots show under the controls in the Xvfb screenshots. Not explained yet; check on a real
  desktop.
- **"M⁻¹" in the Analysis page** shows its superscript minus faintly or not at all with the fonts here. Arch's fonts are
  probably fine.
- **Gallery examples** record one undo step per object they add (as in the raylib app).
- **Not there yet:** the HUD, fly camera mode, recent files.

**Next (run 8):** the tutorial (lessons through `WindowApi`, steps pointing at widgets with popovers), the GTK tour
at four sizes, more input tests, the lesson walk on GTK, and a docs/learn chapter.

### Run 8: the tutorial and the checks (2026-10-21), done

**What exists:**
- **Lessons** (the ? button above the objects list, or Lessons in the main menu): the 17 lessons in a dialog, finished
  ones ticked (remembered in `gtk.txt` as `tutorial.completed`, the raylib app's key).
- **The lesson card** floats over the top right of the 3D view: the lesson, "Step 2 of 4" with progress bars, the
  step's text, its task (ticked green when the scene shows it done), Back, Skip, Next / Finish, and a close button.
  It narrows with the view (at 800×600 with the pane open the view is about 350 px wide).
- **The pointer:** the step's control is outlined, and while the task is open a popover points at it with the task. Inspector
  rows scroll into sight first. Without a window manager (Xvfb) popovers draw as plain boxes without their arrow;
  on GNOME they look normal.
- **GTK wording** for the 15 steps whose text names the raylib app's controls ("press E", "the layers icon in the top
  bar"): gtk/LessonTexts.cpp, by step title.

**Shared code:**
- `TutorialRunner` (edu/Tutorial): the lesson, the step, the setup and the live check, taken out of the raylib
  TutorialPanel. A check now waits for the update after the setup: in the GTK app it ran before the scene was evaluated,
  so a new object's default value could pass it (the Newton lesson's "Take one step" showed done at once).
- `solveTutorialStep` (edu/TutorialSolutions.cpp): the scripted solutions both lesson walks use (they were inside
  tools/lesson_walk.py).
- `InspectorBuilder::anchor` names rows for the lessons (24 anchors, as in the raylib inspectors).

**Checks:**
- The raylib tour is pixel-identical (111 shots) and its lesson walk has 0 problems in 62 steps.
- **GTK lesson walk** (`tools/dev/lesson-walk-gtk.sh`, the app's `--lesson-walk`): 0 problems in 62 steps. Every scripted
  task is seen as done by the card itself, and none before acting. A PNG per step (card and pointer) goes to
  `$DEV/shots-gtk/walk`.
- **GTK input test:** 0 problems in 52 checks. New: open the lesson list, start "Objects", Next, add a vector through
  the + menu, type (0, 0, 2) into its cells, Finish; the preferences file then has the finished lesson.
- **The GTK tour** has 59 steps (new: the lesson list, the card pointing at the + button, the matrix cells, a done task, the
  parameter's mode switch, the pane's Breakdown button), at 800×600, 1600×900, 1920×1080 light and 2560×1440.
- `make test`, `check.sh` ALL CLEAN; docs/learn chapter 10 (the GTK front end), its snippets checked against the code.

**Open problems:**
- The "Move the camera" step counts a camera move in the last 300 ms, checked every 100 ms; fine here, untested on a
  real touchpad.
- Lessons that open the Analysis pane cover part of the view at small widths (below 700 px the pane floats over it).
- Not there yet in GTK: the HUD, fly camera mode, recent files.

**Next (run 9, ⛔):** a side-by-side parity checklist against the raylib app, then the user tries the GTK app on
Arch + GNOME (Wayland, HiDPI, touchpad) and decides on the switch.

## Parity checklist (run 9)

The raylib app's features (UI_GUIDE.md) next to the GTK app. ✅ there, ≈ there in a GNOME way, ❌ missing.

| Feature | raylib app | GTK app |
|---|---|---|
| Objects list, select, Ctrl+click, visibility | side bar | ✅ sidebar list |
| Add menu: types + templates from the selection | N, Add | ✅ + button, Ctrl+Shift+A |
| Inspector for every type, ⤢ formula editor | ✅ | ✅ |
| Ctrl+Enter in a field opens the big editor | ✅ | ❌ only the ⤢ button |
| Delete / duplicate / undo / redo | Del, Ctrl+D, Ctrl+Z/Y | ✅ |
| Object colors | swatch: presets, hex, theme color | ≈ GTK color dialog + "back to the style's color" |
| Show only the selection (O, Show all) | ✅ | ❌ |
| Orbit camera: drag, right/Shift-drag, wheel | ✅ | ✅ |
| Camera views Front/Right/Top/Focus, reset | keypad 1/3/7, ., C, View section | ≈ View menu only, no keys |
| Fly camera (Tab, WASD) | ✅ | ❌ |
| Status overlay (HUD) | ✅ | ❌ |
| View: grid, axes, labels, numbers, spacing, planes, polar, axis length, styles | ✅ (G, L keys) | ✅ View menu (no keys) |
| Scene clock: play, scrub, loop, speed, length | Time panel, P | ✅ bar under the view, Space in the view |
| Analysis / Steps / Breakdown | floating panels, M / K / B | ✅ pane on the right, F9 (no per-page keys) |
| Example gallery | E, topic filter | ≈ dialog grouped by topic, no key |
| Tutorial | F1, panel, pulsing outline | ✅ lesson list, card, outline + popover, no key |
| Formula reference | F2 | ✅ F1 |
| Export to LaTeX / PDF, B/W preview | Ctrl+E | ✅ |
| Files: New, Open, Save, Save as | ✅ own dialog | ✅ GTK file dialog (asks before replacing) |
| Recent files | ✅ | ❌ |
| Preferences: theme, surfaces, fade unselected | ✅ | ✅ |
| Interface size, tooltips on/off, reset panel layout | ✅ | ≈ not needed: GNOME's text scaling, GTK tooltips, no floating panels |
| Keyboard help | H | ✅ Ctrl+? shortcuts dialog |
| Screenshot (F12), fullscreen (F11) | ✅ | ❌ (GNOME's own screenshot tool works) |
| Collapsed side bar strip | Ctrl+B | ≈ Ctrl+B hides the sidebar |
| Command line: --size, --theme, a scene file | ✅ | ✅ (+ --tour, --lesson-walk) |
| Desktop file, icons, `make install`, PKGBUILD | ✅ | ❌ until the switch (run 10) |

**To do before the switch (proposed):** solo selection, recent files, single-key shortcuts that don't fight text fields
(local to the view, like Space and Delete), Ctrl+Enter in formula fields, F11 fullscreen. The fly camera and the HUD
could also stay out: the orbit camera covers the lessons and examples.

