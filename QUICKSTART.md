# Quick Start Guide

## Build and run

You need raylib **5.5** with its development files (see the README for building it from source), then:

```bash
make run
```

## First minute

1. The app starts in **camera mode**: move the mouse to look around, W A S D to move.
2. Press **Esc** (or **Tab**) to get the cursor back (**UI mode**). Esc never closes the app.
3. Press **F1** (or the book icon in the top bar) and pick **Getting around**: the tutorial walks you through
   the rest, one small task at a time.
4. Press **E** for the example gallery and click any card.
5. Press **H** at any time for every shortcut.

## Matrices (1)

- Click a matrix cell, type a value or an expression like `cos(pi/6)`, press **Enter** (**Tab** moves to the
  next cell, **Esc** cancels).
- Presets: **I** identity, **R** rotate, **S** scale, **X** shear, **F** reflect. **Ctrl+Z / Ctrl+Y** undo and redo.
- The **Analysis** panel shows det, trace, inverse, eigenvalues and factorizations. Hover a line to see what it
  means.
- **B** opens the transformation breakdown: your matrix as rotate → stretch → rotate, played step by step.
- **K** opens the calculation steps: the determinant, inverse or eigenvalues worked out with your numbers.
- **P** animates from the identity to your matrix. The Animation panel also has presets and keyframes.

## Complex (2)

- Enter z₁ and z₂. The orange arrow is z₁z₂: z₁ turned by the angle of z₂ and stretched by its length.
- Turn on "Animate θ" under Euler's formula to watch e^(iθ) go around the circle.

## Plots (3)

- Choose a plot type, type an expression, press **Enter**. **Next example** cycles through samples.
- Surfaces and 3D curves use z as the up axis.

## Make it yours

- **T** switches between the dark and light theme.
- **Ctrl+=** / **Ctrl+−** make the whole interface bigger or smaller.
- Drag panels by their title bar; collapse or close them with the buttons on the right.
- The gear icon opens the settings: theme, interface size, tooltips, and a reset for the panel layout.
- **F5** saves the scene to `scene.txt`, **F9** loads it. **F12** saves a screenshot.

See [UI_GUIDE.md](UI_GUIDE.md) for details.
