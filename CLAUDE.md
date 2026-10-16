# CLAUDE.md — project context for axismundi

Context for Claude Code (and humans) working on this repo. Keep it current when the
architecture, workflow or environment changes.

## What this is

A C++20 / raylib + raygui interactive visualizer with three modes:
- **Matrices**: 3x3/4x4 transforms of the basis, vectors A/B, an analysis panel (det, trace,
  inverse, eigen, LU/QR/SVD), and animations
- **Complex**: Argand diagram, multiplication as rotation + scaling, Euler's formula
- **Plots**: y=f(x), parametric curves, surfaces z=f(x,y), implicit f(x,y)=0, heat map

User docs: `README.md`, `QUICKSTART.md`, `UI_GUIDE.md`. Roadmap: `TODO.md` (High and Medium
items are done; Low priority / Code quality / Research items remain).

## Build and test

```bash
make build   # bin/linalg_viz (needs raylib 5.x dev headers)
make run
make test    # bin/run_tests: math + animation unit tests, no raylib needed
```

- Compiler: `clang++ -Wall -Wextra -O2 -std=c++20`. Keep the build **warning-free**.
- `SRCS` globs every `src/**/*.cpp`, so new files need no Makefile edits.
- The test target builds `src/math/*` + `src/animation/*` + `tests/*.cpp`.
- The bundled `include/raygui.h` is 5.1-dev and **requires raylib 5.x**. Linking against
  raylib 4.5 fails on `TextToFloat`.

### Environment caveats (this machine, Alpine Linux)
- Only the raylib 4.5 runtime is installed, without dev headers (fix: `sudo apk add raylib-dev`).
  There is no display, so the GUI can't be run here.
- Fallback verification: compile every file against raylib 5.5 headers (clone
  `raysan5/raylib` tag 5.5 into the scratchpad) using `-isystem <raylib>/src` so raylib's own
  header warnings stay quiet. Logic that needs raylib types can be tested in scratch programs
  linked against `/usr/lib/libraylib.so.4.5.0` (works for non-window functions like `Fade`).

## Architecture

```
src/
  main.cpp            window, main loop, shortcuts, mode switching, save/load wiring
  math/               NO raylib: Vector3D, Matrix3D/4D (det, trace, inverse),
                      Decompositions (LU, QR, eigen3, svd3), Complex, Expression (parser)
  animation/          NO raylib: Animator (presets, keyframes, playback)
  rendering/          raylib drawing: Grid, VectorRenderer, TransformVisualizer (+eigen lines),
                      ArgandRenderer, PlotRenderer, CameraController, Labels (world→screen text)
  ui/                 raygui panels: SideBar (mode tabs + scroll), InputPanel, ComplexPanel,
                      PlotPanel, ViewPanel, InfoPanel (right), AnimationPanel (bottom),
                      HelpOverlay, Widgets (textField/parseNumber/formatNumber), SceneIO,
                      RayguiImpl.cpp (the only RAYGUI_IMPLEMENTATION, warnings silenced)
  include/<area>/     headers mirror the folders above
tests/test_math.cpp   assert-style runner (CHECK / CHECK_NEAR)
```

Key conventions:
- Namespace `linalg`, `m_` member prefix, `#pragma once`, headers in `src/include/<area>/`.
- **Keep `math/` and `animation/` free of raylib** so they stay unit-testable.
- Side-bar panels use `float draw(float x, float y, float width)` and return the bottom y.
  `SideBar::begin/end` handles clipping and scrolling.
- Text fields: `textField(bounds, buf, size, id, activeId, invalid)` in `ui/Widgets`. Each
  panel keeps its own `m_activeField`, and Tab navigation lives in the panel's `update()`.
  Panels expose `isEditing()`, `finishEditing()` and `cancelEditing()`. `main` uses these to
  block shortcuts while typing, to finish edits on mode switch, and to make Esc cancel an edit
  (Esc quits only when nothing is being edited).
- Numeric fields accept expressions (`parseNumber` uses `Expression`). `formatNumber` prints up
  to 4 decimals.
- `InputPanel` keeps a `State` (text buffers + 3x3/4x4 flag) for what's shown and a committed
  copy. `commit()` pushes undo history (max 100). Getters read committed values only.
- Save/load: each component has `saveState(SceneData&)` / `loadState(const SceneData&)` with
  prefixed keys (`matrix3.`, `view.`, `camera.`, `complex.`, `plot.`, `anim.`). File:
  `scene.txt`, plain `key = value`. Loaders ignore missing or bad keys.
- Coordinates: world is Y-up. Complex mode and 2D plots use the world XY plane. 3D plots use
  math z-up, mapped as world = (x, z, −y).
- The default raylib font has no Greek letters or `×`/`°`: use ASCII in UI text ("lambda", "AxB").

Input model: **camera mode** (cursor hidden: mouse look, WASD/Space/Ctrl movement, GUI locked)
vs **UI mode** (cursor visible, TAB toggles). Shortcuts: H help, G grid, L labels, M info panel,
C reset camera, 1/2/3 modes, F5/F9 save/load, and in Matrices mode I/R/X/F presets, S scale
(UI mode only), Ctrl+Z/Y undo/redo (UI mode only), P play/pause.

## Git workflow

- Work on branch **`claude`** (tracks `origin/claude`, remote `git@gitbro:axismundi.git`).
- Commit in logical groups and **push every commit**. **Never open pull requests unless the
  user explicitly asks.**
- No git identity is configured on this machine. Commit with
  `git -c user.name="Diego Rosalio" -c user.email="diegorosalio19@gmail.com" commit ...`
  and end messages with the `Co-Authored-By` trailer. Don't modify the git config.
- Pushing needs an SSH key in the session (agent forwarding / `ssh-add`). On "Permission
  denied (publickey)", ask the user. It can't be fixed from inside the session.

## History of this branch (2026-10-16)

1. math: matrix ops, decompositions, Complex, Expression + `make test`
2. Linear algebra mode: validated input, shortcuts, undo/redo, labels, InfoPanel, save/load
3. Complex numbers mode
4. Function plotting mode
5. Animations (Animator + AnimationPanel)
6. fix: validate a field when focus jumps straight to another field
7. fix: Esc cancels edits instead of quitting
8. docs: TODO/README/QUICKSTART/UI_GUIDE

Behavior changes vs the original app: WASD only moves the camera in camera mode, presets also
fill the 4x4 matrix, and Esc cancels edits.

## Not yet verified

The GUI has never been run on a real display. The first run with raylib 5 should check layout
and font sizes at 1600×900 and 800×600, every mode, the shortcuts, undo/redo, a save→load round
trip, the animation presets/keyframes, and each plot type.
