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

- **Arch:** `sudo pacman -S --needed gtkmm-4.0 libadwaita pkgconf`.
- **Alpine (the dev box):** `gtk4.0-dev gtkmm4-dev libadwaita-dev libepoxy-dev adwaita-fonts font-dejavu`.

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
| 4 | GTK main window: split view, objects list, Add menu, selection, picking, undo, toasts, files | next |
| 5 | GTK inspectors I: framework + basic types | |
| 6 | GTK inspectors II: the remaining types (parallel agents) | |
| 7 | GTK panels: Analysis, Time, Settings, Export/PDF, Gallery, Reference, Steps, Breakdown, Help | |
| 8 | tutorial with popovers, GTK tour, input test, lesson walk, docs/learn chapter | |
| 9 | parity checklist; the user tries it on Arch + GNOME (⛔) | |
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
- The demo scene is fixed. No objects list, inspector or files yet (runs 4–5).
- A click on the view doesn't pick yet (run 4).
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
