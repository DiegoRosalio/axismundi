# Quick Start Guide

## Prerequisites

Install raylib 5.x with its development files, e.g. on Arch Linux:
```bash
sudo pacman -S raylib
```
(Debian/Ubuntu: `libraylib-dev`, Alpine: `raylib-dev`.)

## Build and Run

```bash
make run
```

## First Steps

1. The app starts in **camera mode**: move the mouse to look around, and use WASD to move.
2. Press **TAB** to get the cursor back (**UI mode**) and use the panels on the left.
3. Press **H** at any time to see every shortcut.

## Matrices Mode (1)

- Click a matrix cell, type a value (or an expression like `cos(pi/6)`), press **Enter**.
  **Tab** confirms and jumps to the next cell.
- Try the presets: **I** identity, **R** rotate, **S** scale, **X** shear, **F** reflect.
- **Ctrl+Z / Ctrl+Y** undo and redo.
- The right panel shows the determinant, inverse, eigenvalues and LU/QR/SVD. Click a section
  header to open or close it. **M** hides the panel.
- Press **P** to animate from the identity to your matrix. The bottom bar has more presets,
  a timeline and keyframes: set a matrix and press "+ Key", then set another and press it again.

## Complex Mode (2)

- Enter z₁ and z₂. The orange arrow is z₁·z₂. The faint arrow and the dashed segment show it
  as a rotation followed by a scaling.
- Turn on "Animate" under Euler's Formula to watch e^(it) go around the circle.

## Plots Mode (3)

- Choose a plot type, type an expression, and press **Enter**.
- Press **Example** to cycle through samples. Use **Detail** to change the resolution.
- Surfaces and 3D curves use z as up.

## Saving

**F5** saves everything (matrices, vectors, view, camera, complex and plot inputs, keyframes)
to `scene.txt`. **F9** loads it back.

## Next Steps

See [UI_GUIDE.md](UI_GUIDE.md) for details and [README.md](README.md) for the architecture.
