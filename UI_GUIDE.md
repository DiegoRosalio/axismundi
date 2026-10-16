# UI Guide

## Layout

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ ▣ axismundi  [Matrices|Complex|Plots]        ⛶ 📷 │ ≡ ▦ 📖 ⧉ │ ☀ ? ⚙           │  top bar
├──────────────┬───────────────────────────────────────────────────────────────────┤
│ ▾ MATRIX     │                                         ┌──────────────┐          │
│ ▾ VECTORS    │            3D view                      │ Analysis     │          │
│ ▾ PRESETS    │                                         │  ...         │          │
│ ▾ VIEW       │    floating panels: Tutorial, Gallery,  └──────────────┘          │
│              │    Calculation steps, Breakdown, ...                              │
│  side bar    │  ● UI mode · status        ┌── Animation ──────────────┐          │
│  (scrolls)   │                            └───────────────────────────┘          │
└──────────────┴───────────────────────────────────────────────────────────────────┘
```

- **Top bar**: the mode tabs, then fullscreen, screenshot, breakdown (Matrices only), calculation steps, gallery
  and tutorial, then theme, help and settings. Every button has a tooltip.
- **Side bar**: inputs for the current mode plus the View section. Section headers collapse when clicked, and the
  side bar scrolls with the mouse wheel.
- **Floating panels**: drag them by the title bar, collapse them with the chevron, close them with ×. Clicking
  a panel brings it to the front, and only the top panel under the mouse reacts to clicks. Positions are kept
  as a fraction of the free space, so they stay in place when the window is resized. Settings → "Reset panel
  layout" puts everything back.

## Camera mode and UI mode

- **Camera mode** (cursor hidden, green status dot): mouse look, WASD move, Space/Ctrl up/down, Shift faster.
  Panels ignore the mouse.
- **UI mode** (cursor visible, blue status dot): click fields, buttons and panels; letter keys are shortcuts.
- **Tab** switches between them (while no field is being edited).
- **Esc** steps back one level: it cancels an edit, closes a popover or the help, or leaves camera mode.
  It never quits the app; close the window to exit.

## Resolution independence

Layout is written for a 1600×900 reference and scaled to the window, so the interface keeps its proportions from
800×600 up to 4K. On top of that, the interface size setting (Settings, or Ctrl+= / Ctrl+− / Ctrl+0) scales
everything from 70% to 200%. Fonts are rasterized at the exact pixel size they are drawn at and rebuilt when the
size changes, so text stays sharp. Small text never drops below a readable minimum.

## Text fields

1. Click a field to edit it.
2. Type a number or an expression: `2`, `-0.5`, `1e-3`, `sqrt(2)/2`, `cos(pi/4)`, `2pi/3`.
3. **Enter** or clicking elsewhere confirms, **Tab** / **Shift+Tab** confirms and moves on, **Esc** cancels.
4. A red outline means the text can't be read. Invalid numbers are rejected. In Plots mode the expression is
   kept and the error appears below.

Expressions support `+ - * / ^`, parentheses, implicit multiplication (`2x`, `3(x+1)`), the variables
`x y t`, the constants `pi e`, and `sin cos tan asin acos atan sinh cosh tanh exp log ln sqrt abs floor ceil`.

## Matrices mode

- **Matrix**: 3×3 or 4×4 (the 4th column is a translation), undo/redo. Hover a cell to see what its column
  means.
- **Vectors**: A and B. Their colors are set in View → Vector colors.
- **Presets**: each button shows its shortcut, and its tooltip gives the formula and determinant.
- **Analysis** (M): vectors, matrix properties, eigenvalues, LU/QR/SVD. Hover any line for an explanation.
- **Animation**: play/pause (P), stop, loop, timeline, Rotate/Scale/Morph presets, keyframes, speed.
- **Transformation breakdown** (B): the matrix as numbered steps (rotate, stretch, rotate, move). Use
  restart / previous / play / next or click a step. While the panel is open, the 3D view shows the matrix part
  way through, with faded copies of the finished steps.

## Complex mode

- z₁ and z₂ with their rectangular and polar forms and the product z₁z₂.
- "Show product and angles" draws the rotation step (faint arrow), the scaling step (dashed) and the angle arcs.
- Euler's formula: show or animate e^(iθ), or drag θ.

## Plots mode

- Tabs: y = f(x), Curve (parametric), Surface (z = f(x, y)), Implicit (f(x, y) = 0).
- Ranges, Detail (sampling), Heat map, Next example. The value range or the parse error is shown below.

## Learning tools

- **Tutorial** (F1): pick a lesson. Each step explains one idea and usually asks for a small change, which is
  checked automatically. The control it talks about pulses with an outline. Next unlocks when the task is done,
  or press Skip. Finished lessons get a checkmark and are remembered.
- **Example gallery** (E): filter by mode and click a card to load it.
- **Calculation steps** (K): pick a topic (A·B, A×B, M·A, det M, M⁻¹, eigenvalues; or z₁·z₂, polar forms, Euler).
  The formula, the numbers filled in and the result follow your inputs live.
- **Tooltips**: hover almost anything. They can be turned off in Settings.

## View section

- Grid (G), Axes, Labels (L), Eigenvectors and Transformed A, B (Matrices only)
- Vector colors: click a swatch for the color picker (presets, hex value, "Theme color" resets it)
- Axis length, Reset camera (C), Save (F5) / Load (F9) of `scene.txt`

## Settings (gear icon)

Theme, interface size, tooltips, status overlay, reset panel layout. Preferences are saved in
`$XDG_CONFIG_HOME/axismundi/settings.txt` (usually `~/.config/axismundi/settings.txt`) when the app closes.

## Screenshots and fullscreen

- **F12** or the camera icon saves `axismundi_YYYYMMDD_HHMMSS.png` in the current folder. **Shift+F12** leaves the
  UI out.
- **F11** or the corners icon toggles borderless fullscreen.

## Quick reference

| Action | How |
|--------|-----|
| Camera ⟷ UI mode | Tab |
| Switch mode | 1 / 2 / 3 or the tabs |
| Edit a value | Click, type, Enter |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| Tutorial / gallery / steps / breakdown / analysis | F1 / E / K / B / M |
| Theme / interface size | T / Ctrl+= Ctrl+− Ctrl+0 |
| Help | H |
| Save / load / screenshot | F5 / F9 / F12 |
