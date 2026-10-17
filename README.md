# axismundi: Linear Algebra 3D Visualizer

An interactive 3D tool for exploring linear algebra, complex numbers and functions, with guided lessons
and worked calculations.

![Objects in the 3D space](docs/screenshots/matrices.png)

## Features

### Objects, formulas and parameters
- Build a scene from **any number of objects**: vectors, matrices, eigenvector sets, plots, parameters and
  complex numbers. Add them with **Add** (or N), select them in the list or by clicking them in the view
  (Ctrl+click for several), edit them in the inspector, hide, duplicate (Ctrl+D) or delete (Del) them.
- **Every field is a formula** that can use other objects by name: `s = a + b`, `c = cross(a, b)`,
  `Ma = M*a`, a matrix cell `cos(theta)`, a whole matrix `rotz(theta)` or `[u, v, w]` (vectors as columns).
  When an input changes, everything that depends on it updates. Circular references and missing names are
  reported on the objects involved.
- **Parameters** are named numbers (θ, k, …) usable in any formula, so they can turn a rotation matrix,
  reshape a surface or move e^(iθ) around the circle. Each one is driven in one of three ways:
  - a **slider**, static, for morphing by hand (e.g. `lerp(eye(3), M, s)`);
  - an **animation** that follows the scene clock from min to max: once, loop or ping-pong, with an easing
    curve (ease in/out, sine, back, elastic, bounce, steps), a duration and a start delay;
  - a **formula of time** `t`, such as `pi/4*cos(2t)`, which may use other objects.
- **`t` is the scene time in every formula**: `[cos(t), sin(t), 1]`, `rotz(t)`, `exp(i*t)`, `sin(x - t)`.
- One **Time panel** plays, pauses and scrubs the scene clock for everything at once.
- The **Add menu** offers templates for the current selection: sum, difference, cross product and projection
  of two vectors, M·v, the eigenvectors, inverse or transpose of a matrix, A·B, z₁·z₂, conjugates…
- Undo/redo covers every change (Ctrl+Z / Ctrl+Y).

### Two spaces
- **3D space** (key 1), with the math convention that z is up and the floor is the xy-plane. It holds:
  - vectors (by components or by formula, with an optional start point);
  - 3×3 / 4×4 matrices (by cells with presets, or by formula) drawn as the moved basis;
  - eigenvector sets;
  - plots: `y = f(x)`, curves `(x(u), y(u), z(u))`, surfaces `z = f(x, y)` and implicit curves
    `f(x, y) = 0`, with heat maps. Detail is set **per unit** (e.g. 30 samples per unit), so a wider range
    stays just as smooth. Surfaces are shaded GPU meshes.
  - a floor grid from ±5 to ±100 units, with major lines every 5 units.
- **Complex plane** (key 2): complex numbers (e.g. `2 + i`, `z1*z2`, `exp(i*theta)`), with optional circle,
  cos/sin projections and the helix of e^(it), and products drawn as a rotation plus a scaling.

### Analysis and learning tools
- **Analysis panel** (M): follows the selection.
  - One vector: its length and unit vector. Two vectors: dot product, angle and cross product.
  - A matrix: determinant, trace, inverse, eigenvalues, LU, QR and SVD.
  - Complex numbers: polar form, product and quotient.
- **Transformation breakdown** (B): the selected matrix as *rotate → stretch → rotate (→ move)*, played step
  by step.
- **Calculation steps** (K): worked calculations with the selected objects' numbers.
- **Tutorial** (F1): 11 lessons with tasks that are checked as you work, including adding objects, formulas
  and parameters.
- **Example gallery** (E): about 20 ready-made scenes.
- **Tooltips** everywhere.

### Interface
- Resolution independent (design reference 1600×900) with a user size setting (Ctrl+= / Ctrl+− / Ctrl+0),
  embedded fonts and real math symbols.
- Dark and light themes (T), floating panels you can drag, collapse and close, fullscreen (F11),
  screenshots (F12).
- Preferences in `~/.config/axismundi/settings.txt`; scenes saved and loaded with F5 / F9 (`scene.txt`,
  older scene files are converted automatically).

| | |
|---|---|
| ![Objects and formulas](docs/screenshots/matrices.png) | ![Add menu with templates](docs/screenshots/add-menu.png) |
| ![A parameter driving rotz(theta)](docs/screenshots/parameters.png) | ![Roots of unity](docs/screenshots/complex-roots.png) |
| ![Surface with a slider](docs/screenshots/plot-surface.png) | ![Light theme](docs/screenshots/matrices-light.png) |
| ![Breakdown](docs/screenshots/breakdown.png) | ![Tutorial](docs/screenshots/tutorial.png) |

### Formulas

Fields accept:
- numbers and the operators `+ - * / ^`, with implicit multiplication (`2x`, `3(a + b)`);
- vectors `[x, y, z]`, matrices `[[…], […], […]]` (3×3 or 4×4), and `[u, v, w]` to use vectors as columns;
- components `v.x`, the constants `pi`, `e` and `i`;
- functions:
  - `sin cos tan exp log sqrt abs …` (complex-aware);
  - vectors: `dot cross norm normalize`;
  - matrices: `det tr inv transpose rotx roty rotz rot(axis, angle) scale translate eye`;
  - complex numbers: `conj re im arg polar`;
  - blending: `lerp(a, b, s)` (any value kind), `keys(s, A, B, …)`, and `mod(a, b)`.

`M*v` applies a matrix to a vector (4×4 matrices treat v as a point), and `M*N` composes matrices.
`t` is the scene time (seconds) everywhere; plots use `x` and `y`, and curves use `u` for their own variable.

## Requirements

- C++20 compiler (the Makefile uses `clang++`)
- GNU Make
- **raylib 5.5** with development headers. The bundled `include/raygui.h` (5.1-dev) uses `TextToFloat`, which
  first appeared in raylib 5.5, so older distribution packages (5.0 and below) will not link.
- Linux

## Installation

### raylib 5.5

If your distribution ships raylib 5.5 or newer, install its development package (Arch: `raylib`,
Debian/Ubuntu: `libraylib-dev`). Otherwise build it from source:

```bash
git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```

Building raylib needs the X11 and OpenGL development headers (for example on Alpine:
`mesa-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev`).

### Build

```bash
make build
```

## Usage

```bash
make run                       # build and run
./bin/linalg_viz --theme light # start with the light theme
./bin/linalg_viz --size 1280x720
make test                      # math, expressions, scene, playback and explanation tests (no raylib needed)
```

### Controls

The app has two input states. In **camera mode** (cursor hidden) the mouse looks around, WASD move,
Space/Ctrl go up and down, and Shift moves faster. In **UI mode** (cursor visible) you click objects, fields,
buttons and panels. Press **Tab** to switch; **Esc** always gives the mouse back. Esc never quits: close the
window to exit.

| Key | Action |
|-----|--------|
| Tab | Camera mode ⟷ UI mode |
| Esc | Leave camera mode, close a popup or the help, cancel an edit |
| 1 / 2 | 3D space / complex plane |
| Click / Ctrl+click | Select an object / add it to the selection |
| N | Add an object |
| Del / Ctrl+D | Delete / duplicate the selection |
| Ctrl+Z / Ctrl+Y | Undo / redo (UI mode) |
| I / R / S / X / F | Presets for the selected matrix (S in UI mode) |
| F1 / E / K / B / M | Tutorial / gallery / calculation steps / breakdown / analysis |
| P | Play / pause the scene clock (Time panel) |
| H | Help with all shortcuts |
| T | Dark / light theme |
| Ctrl+= / Ctrl+− / Ctrl+0 | Bigger / smaller / default UI size |
| F11 / F12 | Fullscreen / screenshot (Shift+F12: 3D view only) |
| C / G / L | Reset camera / grid / labels |
| F5 / F9 | Save / load `scene.txt` |
| Enter / Tab | Confirm field / next field |

Shortcuts are ignored while a text field is being edited.

## Project Structure

```
.
├── Makefile
├── assets/fonts/             # subset Inter and JetBrains Mono (SIL OFL) + licenses
├── docs/screenshots/         # images used by the docs (made with --tour)
├── include/raygui.h          # raygui (single header)
├── tests/test_math.cpp       # unit tests
├── tools/fonts.py            # font subsetting and embedding
└── src/
    ├── main.cpp              # command-line options
    ├── app/                  # App (main loop, shortcuts, files, preferences) and Tour
    ├── math/                 # vectors, matrices, decompositions, complex, quaternions,
    │   └── expr/             #   breakdown, worked explanations; typed expression engine
    ├── scene/                # object model: objects, spaces, dependencies, undo, files
    ├── view/                 # SceneView: draws the objects of a space
    ├── animation/            # Playback (repeat styles, easing)
    ├── edu/                  # tutorial lessons and gallery examples
    ├── rendering/            # grid, arrows, plots, camera, labels
    ├── ui/                   # side bar (objects + inspector), panels, popovers, widgets
    │   ├── core/             # scaling, fonts, theme, layout, icons, floating panels
    │   └── generated/        # fonts.inc (embedded font data)
    └── include/              # headers, mirroring the folders above
```

`src/math`, `src/scene` and `src/animation` have no raylib dependency, which keeps them unit-testable.

## Development

- `make test` runs the unit tests.
- `make fonts` regenerates `src/ui/generated/fonts.inc` after changing `assets/fonts` (see `tools/fonts.py`).
- `./bin/linalg_viz --tour DIR [--size WxH] [--theme dark|light]` walks through every mode and panel, saves a
  PNG per step into `DIR` and exits. It is used to check the UI at different sizes, also headlessly with
  `xvfb-run`.

## License

The code is provided as-is for educational purposes. The embedded fonts are under the SIL Open Font License 1.1
(see `assets/fonts/`).
