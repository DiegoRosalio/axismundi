# UI Guide - Linear Algebra Visualizer

## Overview

The screen has a scrollable side bar on the left, the 3D view in the middle and, in Matrices mode,
an analysis panel on the right plus an animation bar at the bottom.

```
┌──────────────┬──────────────────────────────────┬──────────────┐
│ [Matrices][Complex][Plots]                       │  Analysis    │
│              │  HUD: mode hint, camera position │  - Vectors   │
│  Mode panel  │                                  │  - Matrix    │
│  (inputs)    │            3D view               │  - Eigen...  │
│              │                                  │  + LU        │
│  View panel  │                                  │  + QR        │
│              ├──────────────────────────────────┤  + SVD       │
│              │  Animation bar (Matrices mode)   │              │
└──────────────┴──────────────────────────────────┴──────────────┘
```

## Camera Mode and UI Mode

- **Camera mode** (cursor hidden): mouse look, WASD move, Space/Ctrl up/down, Shift for speed.
  The panels ignore the mouse in this mode.
- **UI mode** (cursor visible): click fields and buttons. The camera stays still, so letter keys
  can be used for typing and shortcuts.
- **TAB** switches modes (when no field is being edited).

## Text Fields

1. Click a field to edit it.
2. Type a number or an expression: `2`, `-0.5`, `1e-3`, `sqrt(2)/2`, `cos(pi/4)`, `2pi/3`.
3. Confirm:
   - **Enter** or clicking elsewhere applies the value
   - **Tab** / **Shift+Tab** applies it and moves to the next / previous field
   - **Esc** cancels and restores the previous value
4. A red outline means the text can't be parsed. Invalid numbers are rejected and the previous
   value comes back. In Plots mode the expression is kept and the error is shown below it.

Available in expressions: `+ - * / ^`, parentheses, implicit multiplication (`2x`, `3(x+1)`),
variables `x y t`, constants `pi e`, and functions
`sin cos tan asin acos atan sinh cosh tanh exp log ln sqrt abs floor ceil`.

## Matrices Mode

### Matrix Transformation
- Toggle **3x3 / 4x4**. Presets fill both, so they work in either size.
- **Undo / Redo** buttons (also Ctrl+Z / Ctrl+Y in UI mode). The history keeps 100 steps and
  covers matrix and vector edits, presets, the size toggle and scene loads.

### Vectors
One row per vector (A, B) with x, y, z fields.

### Preset Transformations

| Button | Key | Matrix |
|--------|-----|--------|
| Identity | I | diagonal 1s |
| Rot 45 | R | rotation by 45° about Z |
| Scale 2 | S | diag(2, 2, 1) |
| Shear | X | top row [1, 0.5, 0] |
| Reflect | F | diag(-1, 1, 1) |

### Analysis Panel (right, toggle with M)
Click a section header to collapse or expand it, and scroll with the mouse wheel over the panel.
- **Vectors**: A·B, |A|, |B|, angle, A×B and its length (parallelogram area)
- **Matrix**: determinant (and whether volume is scaled, flipped or squashed), trace, inverse
- **Eigenvalues**: real and complex eigenvalues with eigenvectors. Defective matrices are flagged.
- **LU / QR / SVD**: factor matrices (collapsed by default)

In 4x4 mode the determinant, trace and inverse use the full matrix. The eigenvalues and
factorizations use the upper-left 3x3 part.

### Animation Bar
- **Play/Pause (P)**, **Stop**, **Loop**, and a timeline slider (drag it to scrub)
- **Rotate**: a full turn about Y. **Scale**: 1x → 2x. **Morph**: identity → your matrix.
- **+ Key** captures the current matrix as a keyframe. With two or more keys the animation
  plays through them. **Clear** removes them.
- Speed slider from 0.1x to 4x

## Complex Mode
- Fields for z₁ and z₂ (real and imaginary parts)
- Results in rectangular and polar form, the product's modulus as |z₁|·|z₂|, and its argument
  as the sum of arguments
- "Show z1 z2 and angles" draws the product, the rotation and scaling steps, and the angle arcs
- **Euler's Formula**: show/animate e^(it), drag t from 0 to 2π

## Plots Mode
- Tabs: **y=f(x)**, **Curve** (parametric), **Surface** (z = f(x, y)), **Implicit** (f(x, y) = 0)
- Range fields for x, y or t, depending on the type
- **Detail** sets the sampling resolution
- **Heat map** colors by value. For implicit plots it shades the sign of f behind the curve.
- **Example** cycles through sample functions for the current type
- The value range or the parse error is shown under the controls

## View Panel (all modes)
- Grid (G), Axes, Labels (L), Eigenvectors, Transformed A/B toggles
- Axis length slider
- **Reset Camera (C)** returns to the default view for the current mode
- **Save (F5) / Load (F9)** write and read `scene.txt` in the working directory. The file is
  plain `key = value` text and can be edited by hand.

## Window Size
- Default 1600×900, minimum 800×600
- The side bar scrolls with the mouse wheel when its content is taller than the window
- On narrow windows the animation bar extends under the analysis panel. Press M to hide the panel.

## Quick Reference

| Action | Method |
|--------|--------|
| Switch camera / UI mode | TAB |
| Switch app mode | 1 / 2 / 3 or the tabs |
| Edit a value | Click, type, Enter |
| Next field | Tab |
| Cancel edit | Esc |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| Help | H |
| Save / load | F5 / F9 |
