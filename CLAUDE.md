# CLAUDE.md — project context for axismundi

Context for Claude Code (and humans) working on this repo. Keep it current when the
architecture, workflow or environment changes.

## What this is

A C++20 / raylib 5.5 + raygui interactive visualizer with three modes (Matrices, Complex, Plots),
a resolution-independent themed UI with floating panels, and learning tools (tutorial, example
gallery, worked calculation steps, transformation breakdown, tooltips).

User docs: `README.md`, `QUICKSTART.md`, `UI_GUIDE.md`. Roadmap: `TODO.md` (High, Medium, UI
Improvements and Educational Features are done; Advanced Math, Performance, Export/Import,
Platform, Code quality and Research items remain).

## Build and test

```bash
make build   # bin/linalg_viz (needs raylib 5.5 dev headers)
make run
make test    # bin/run_tests: math + animation + explanation tests, no raylib needed
make fonts   # regenerate src/ui/generated/fonts.inc from assets/fonts
```

- Compiler: `clang++ -Wall -Wextra -O2 -std=c++20`. Keep the build **warning-free**
  (multi-line string literals inside arrays need parentheses for -Wstring-concatenation).
- `SRCS` globs every `src/**/*.cpp`; the test target builds `src/math/*` + `src/animation/*` + `tests/*`.
- **raylib 5.5 is required**: the bundled raygui 5.1-dev calls `TextToFloat` (new in 5.5), and the UI
  uses `DrawRectangleRoundedLinesEx` and `ToggleBorderlessWindowed`. Alpine's `raylib-dev` is 5.0: too old.

### Visual verification (important for any UI change)

`linalg_viz --tour DIR [--size WxH] [--theme dark|light]` runs `src/app/Tour.cpp`: a scripted walk
through every mode, panel, popover and tooltip that saves PNGs and exits (preferences are not read or
written during the tour). On this machine (Alpine, no display) the workflow is:

1. raylib 5.5 is built from source as a static lib in the session scratchpad (clone tag 5.5, compile
   `rcore rshapes rtextures rtext rmodels utils raudio` with `-DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33
   -D_GLFW_X11` plus `rglfw.c` with `-D_GLFW_X11`, then `ar rcs`).
2. Build: `make build BIN_DIR=<dir> INCLUDES="-Isrc/include -Iinclude -isystem <raylib>/src"
   LDLIBS="<raylib>/src/libraylib.a -lGL -lX11 -lm -lpthread -ldl"`.
3. Run headless: `xvfb-run -a -s "-screen 0 <W+100>x<H+100>x24" sh -c "timeout 120 <bin> --size WxH --tour <out>"`
   (put the timeout inside xvfb-run, otherwise the app survives the wrapper). Then view the PNGs.
4. Check 800×600, 1600×900, 1920×1080 (light) and 2560×1440 before committing UI work.

Installed packages that make this possible: xvfb, xvfb-run, mesa-dri-gallium, mesa-dev, libxcursor-dev,
libxrandr-dev, libxinerama-dev, libxi-dev. Frame captures must flush the render batch first
(`rlDrawRenderBatchActive()` before `LoadImageFromScreen()`), or the last draw calls are missing.

## Architecture

```
src/
  main.cpp            option parsing (--size, --theme, --tour)
  app/App             owns everything: main loop, input/shortcuts, update, scene drawing, UI drawing,
                      scene save/load, preferences, screenshots; public API used by edu/ and Tour
  app/Tour            scripted screenshot tour
  math/               NO raylib: Vector3D, Matrix3D/4D, Decompositions (LU/QR/eigen3/svd3), Complex,
                      Expression parser, Quaternion, Breakdown (SVD steps), Explain (worked steps)
  animation/          NO raylib: Animator
  edu/                Tutorial lessons (steps with setup/check/anchor), Examples (gallery entries)
  rendering/          Grid, VectorRenderer, TransformVisualizer (+eigen lines, ghost bases), ArgandRenderer,
                      PlotRenderer, CameraController, Labels (world-anchored text pills)
  ui/core/            Ui (scale, fonts, text, primitives, interaction, clip, anchors, tooltips),
                      Theme (dark/light tokens), Layout (Column, splitRow, takeLeft/Right), Icons (vector),
                      FloatingPanel + PanelManager (drag, collapse, z-order, input ownership)
  ui/                 Widgets (button, iconButton, segmented, toggleRow, slider, sectionHeader, textField,
                      card, keycap, swatch), TopBar, SideBar, InputPanel, ComplexPanel, PlotPanel, ViewPanel,
                      InfoPanel, AnimationPanel, StepsPanel, BreakdownPanel, GalleryPanel, TutorialPanel,
                      SettingsPopover, ColorPopover, HelpOverlay, SceneIO, Preferences, RayguiImpl.cpp
  ui/generated/       fonts.inc (embedded Inter Regular/SemiBold + JetBrains Mono, subset; codepoint list)
  include/<area>/     headers mirror the folders above
tests/test_math.cpp   assert-style runner (CHECK / CHECK_NEAR)
assets/fonts/         subset TTFs + OFL licenses; tools/fonts.py subsets (fonttools) and embeds
docs/screenshots/     images for the docs, produced by --tour
```

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
- **Input ownership**: widgets check `ui::hovered()`, which respects `GuiLock` (camera mode, overlays,
  panels not on top), the clip rectangle (scroll containers) and popovers. raygui is only used for
  text boxes (drawn transparent inside our rounded box) and the color picker.
- **Text fields**: `ui::textField(rect, buf, size, id, activeId, invalid)`; panels expose
  `isEditing/finishEditing/cancelEditing`; Esc cancels an edit (quits only when nothing is edited).
- **Tooltips**: `ui::tooltip(rect, body, title)` after drawing a widget; `segmented()` takes per-segment tips.
- **Anchors**: `ui::anchor("id", rect)` names a control so tutorial steps can highlight it.
- **Theme colors** are read at draw time; plot geometry bakes colors, so theme changes call
  `PlotPanel::markChanged()`.
- Scenes (`scene.txt`, F5/F9) hold matrices/vectors/plots/camera/keyframes; preferences
  (`~/.config/axismundi/settings.txt`) hold theme, zoom, toggles, panel layout, vector colors,
  finished lessons, window size. Both use the `key = value` format from `SceneIO`.

## Git workflow

- Work on branch **`claude`** (tracks `origin/claude`, remote `git@gitbro:axismundi.git`).
- Commit in logical groups and **push every commit**. **Never open pull requests unless the
  user explicitly asks.**
- No git identity is configured on this machine. Commit with
  `git -c user.name="Diego Rosalio" -c user.email="diegorosalio19@gmail.com" commit ...`
  and end messages with the `Co-Authored-By` trailer. Don't modify the git config.
- Pushing needs an SSH key in the session; on "Permission denied (publickey)" ask the user.

## Not yet verified

The tour exercises everything visually, but real mouse dragging, typing feel, fullscreen toggling and
the preferences round trip (written on normal exit) have only been checked in code, not interactively.
