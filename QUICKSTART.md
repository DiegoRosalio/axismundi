# Quick Start Guide

## Build and run

You need raylib **5.5** with its development files (see the README for building it from source), then:

```bash
make run
```

## First minute

1. The app starts with an **empty scene** and the cursor free (**UI mode**). Click **Add** (or press N) to create
   an object, **Demo scene** for a ready-made set, or **Open…** (F9) for a saved scene.
2. **Tab** switches to **camera mode**: move the mouse to look around, W A S D to move. **Esc** (or Tab) gives
   the cursor back. Esc never closes the app.
3. Press **F1** (or the book icon in the top bar) and pick **Getting around**. The tutorial walks you through
   the rest, one small task at a time.
4. Press **E** for the example gallery and click any card.
5. Press **H** at any time for every shortcut.

## Objects

- The side bar lists the objects of the current space: in the 3D space, the matrix `M`, the vectors `a` and
  `b`, and vectors computed from them (`s = a + b`, `c = cross(a, b)`, `Ma = M*a`, …).
- Click an object (in the list or in the view) to select it. The **inspector** below the list edits it.
- **Add** (or N) creates new objects. Select objects first to get templates: with `a` and `b` selected you
  can add `a + b`, `cross(a, b)` or the projection of `a` on `b` in one click.
- Fields are formulas: type `2a - b`, `normalize(a)`, `cos(theta)`, `rotz(theta)` … and the object follows
  its inputs. A red outline or a red badge in the list means a formula has a problem; hover it to see why.
- **Del** deletes, **Ctrl+D** duplicates, **Ctrl+Z / Ctrl+Y** undo and redo.

## Parameters

Add a **Parameter**, give it a name like `theta`, and use it in any formula, for example a matrix
`rotz(theta)` or a surface `sin(theta*x)`. A parameter can be:
- a **Slider** (static: it stays where you put it);
- **Animate**: follows the scene clock, once, loop or ping-pong, with an easing curve;
- a **Formula** of time `t`, such as `pi/4*cos(2t)`.

`t` (the scene time) also works directly in any formula: `rotz(t)`, `[cos(t), sin(t), 1]`, `sin(x - t)`.
Curves use `u` for their own variable.

The **Time panel** (P to play or pause) runs the clock for everything at once and can be scrubbed. To morph
something by hand, select it and use Add → Morph from identity / Grow from zero: you get a slider `s` and a
`lerp` formula.

Plot **Detail** is per unit of the range, so wide plots stay smooth. **Grid size** (View section) makes the
floor grid larger.

## The two spaces

- **3D space** (key 1): z points up and the floor is the xy-plane. It holds vectors, matrices, eigenvectors
  and plots (`y = f(x)`, curves, surfaces `z = f(x, y)`, implicit curves).
- **Complex plane** (key 2): complex numbers (`2 + i`, `z1*z2`, `exp(i*theta)`) and products drawn as a
  rotation plus a scaling.

## Tools

- **M** Analysis: results for the selection (lengths, angles, det, inverse, eigenvalues, LU/QR/SVD, polar form).
- **B** Breakdown: the selected matrix as rotate → stretch → rotate, step by step.
- **K** Calculation steps: worked calculations with your numbers.
- **F2** Formula reference: everything you can type in a formula, with live examples and a box to try your own.
- **P** plays or pauses the scene clock (Time panel).

## Make it yours

- **T** switches between the dark and light theme; **Ctrl+=** / **Ctrl+−** change the interface size.
- Drag panels by their title bar; collapse or close them with the buttons on the right.
- **Ctrl+S** / **F5** saves the scene (the first time it asks for a name), **Ctrl+Shift+S** saves under a new
  name, **Ctrl+O** / **F9** opens a scene file (with recent files), **New** clears both spaces. Opening and
  New can be undone. **F12** saves a screenshot.
- Matrix presets add up: Rotate 45° then Scale 2× gives scale·rotate. **Identity** starts over.

See [UI_GUIDE.md](UI_GUIDE.md) for details.
