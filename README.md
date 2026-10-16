# Linear Algebra 3D Visualizer

An interactive 3D visualization tool for exploring linear algebra, complex numbers and functions.

## Features

The app has three modes, selected with the tabs at the top of the left panel (or keys **1**, **2**, **3**).

### Matrices (linear algebra)
- **3x3 and 4x4 matrix transformations** applied live to the coordinate basis
- **Editable fields** for every matrix cell and for vectors A and B. Fields accept numbers or
  expressions such as `cos(pi/4)` or `sqrt(2)/2`. Invalid input gets a red outline and is rejected.
- **Presets**: identity, 45° rotation, 2x scale, shear, reflection, with keyboard shortcuts
- **Undo / redo** for every change (buttons or Ctrl+Z / Ctrl+Y)
- **Vectors**: A, B, A+B, A×B and the transformed vectors MA, MB, each labeled with its magnitude
- **Analysis panel** (right side, toggle with **M**):
  - dot product, angle between A and B, cross product and the parallelogram area
  - determinant (with what it means geometrically), trace, inverse
  - eigenvalues (including complex pairs) and eigenvectors, which are also drawn in the scene
  - LU (with pivoting), QR and SVD factorizations
- **Animations** (bottom bar): rotation, scaling and morph (identity → your matrix), or
  keyframes you capture yourself. Play/pause, loop, timeline scrubber and speed control.

### Complex numbers
- Argand diagram with unit circle and axes
- z₁, z₂ and z₁·z₂ in rectangular and polar form
- Multiplication shown as **rotation + scaling**, with the angle arcs adding up
- **Euler's formula**: e^(it) on the unit circle with its cos/sin projections, plus the helix
  e^(it) rising along Z. The angle t can be dragged or animated.

### Plots
- `y = f(x)` curves (asymptotes are not joined)
- Parametric curves `(x(t), y(t), z(t))`
- Surfaces `z = f(x, y)` with wireframe
- Implicit curves `f(x, y) = 0` (marching squares)
- Heat-map coloring by value, adjustable ranges and detail, and built-in examples

### General
- First-person camera (WASD + mouse) with a reset per mode
- Grid and axes toggles, adjustable axis length, labels on/off
- Help overlay (**H**) listing every shortcut
- **Save / load** the whole scene to `scene.txt` (F5 / F9)

## Requirements

- C++20 compiler (the Makefile uses `clang++`)
- GNU Make
- Raylib **5.x** with development headers (the bundled `include/raygui.h` is 5.1-dev and needs raylib 5)
- Linux

## Installation

### Install Raylib

On Arch Linux:
```bash
sudo pacman -S raylib
```

On Debian/Ubuntu:
```bash
sudo apt install libraylib-dev
```

On Alpine:
```bash
sudo apk add raylib-dev
```

Or build from source:
```bash
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```

### Build the Project

```bash
make build
```

## Usage

```bash
make run          # build and run
./bin/linalg_viz  # or run directly
make test         # run the math unit tests (no raylib needed)
```

### Controls

The app has two input states:

- **Camera mode** (cursor hidden): mouse looks around, WASD moves, Space/Ctrl go up/down, Shift moves faster.
- **UI mode** (cursor visible): click fields and buttons. Letter keys are free for typing and shortcuts.

Press **TAB** to switch between them.

| Key | Action |
|-----|--------|
| TAB | Camera mode ⟷ UI mode |
| 1 / 2 / 3 | Matrices / Complex / Plots mode |
| H | Help overlay |
| C | Reset camera |
| G / L / M | Toggle grid / labels / analysis panel |
| I / R / X / F | Identity / Rotate 45° / Shear / Reflect presets |
| S | Scale 2x preset (UI mode only; S moves the camera in camera mode) |
| Ctrl+Z / Ctrl+Y | Undo / redo (UI mode) |
| P | Play / pause animation |
| F5 / F9 | Save / load `scene.txt` |
| Enter / Tab / Shift+Tab | Confirm field / next field / previous field |
| Esc | Cancel the field edit (quits when not editing) |

Shortcuts are ignored while a text field is being edited.

### Colors

- Red/Green/Blue axes: original coordinate system
- Orange/Lime/Sky blue axes: transformed basis
- Yellow/Magenta: vectors A and B; Orange: A+B; Sky blue: A×B
- Gold/Pink: transformed vectors MA and MB
- Purple dashed lines: eigenvector directions, with a dot at λv

## Project Structure

```
.
├── Makefile
├── include/raygui.h          # raygui (single header)
├── tests/test_math.cpp       # unit tests for math + animator
└── src/
    ├── main.cpp              # window, main loop, shortcuts, save/load wiring
    ├── include/              # headers, mirroring the folders below
    ├── math/                 # Vector3D, Matrix3D/4D, Decompositions, Complex, Expression
    ├── animation/            # Animator (matrix animation state)
    ├── rendering/            # Grid, vectors, transforms, Argand, plots, camera, labels
    └── ui/                   # side bar, panels, info panel, help, scene file I/O
```

`src/math` and `src/animation` have no raylib dependency, which keeps them unit-testable.

## Extending the Project

- **New matrix preset**: add a value to `MatrixPreset` and a case in `InputPanel::applyPreset`
  (`src/ui/InputPanel.cpp`), then a button in `drawPresetGroup`.
- **New animation**: add an `AnimationPreset` and its case in `Animator::matrixAt`.
- **New plot example**: add an entry to the example tables at the top of `src/ui/PlotPanel.cpp`.
- **New expression function**: add it to the table in `lookupFunction` (`src/math/Expression.cpp`).
- **Saving more state**: give the component `saveState(SceneData&)` / `loadState(const SceneData&)`
  and call them from `saveScene` / `loadScene` in `main.cpp`.

## Makefile Targets

- `make` or `make run`: build and run the application
- `make build`: build the executable only
- `make test`: build and run the unit tests
- `make clean`: remove build artifacts
- `make help`: list targets

## Future Enhancements

See [TODO.md](TODO.md). The high- and medium-priority items are done.

## License

This project is provided as-is for educational purposes.
