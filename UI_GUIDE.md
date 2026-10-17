# UI Guide

## Layout

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ ▣ axismundi  [3D space|Complex plane]   ⛶ 📷 │ ▶ ◷ │ ▥ ◈ ≡ ▦ 📖 │ ☀ ? ⚙        │  top bar
├──────────────┬───────────────────────────────────────────────────────────────────┤
│ ▾ OBJECTS  + │                                         ┌──────────────┐          │
│  M  a  b  s  │            3D view                      │ Analysis     │          │
│ ▾ Inspector  │                                         │  ...         │          │
│ ▾ VIEW       │    floating panels: Tutorial, Gallery,  └──────────────┘          │
│              │    Calculation steps, Breakdown, ...                              │
│  side bar    │  ● UI mode · status        ┌── Time ───────────────────┐          │
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

Top bar buttons, from the left: fullscreen and screenshot; **play / pause** the scene clock and show the
**Time panel**; the **Analysis** panel, the transformation breakdown (3D space only), calculation steps, the
example gallery and the tutorial; then theme, help and settings. A highlighted button means its panel is open.

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

## Objects

Each space has its own list of objects. The side bar shows:

- **Objects list**: a color dot, the name, a short value (e.g. `(1, 0, 0)` or `a + b = (1, 1, 0)`), and an
  eye button to show or hide the object. Red text means the object has a problem; hover the row to read it.
  Click to select, Ctrl+click to add to the selection; clicking an object in the 3D view does the same, and
  clicking empty space clears the selection. Undo / redo and **Add** sit in the header.
- **Inspector**: the selected object's type, name (rename it here; formulas that use it are updated), color,
  its fields, and Duplicate / Delete.

### Add menu
**Add** (or N) lists every object type for the current space, grouped by category. When objects are selected,
**From selection** offers ready formulas:
- two vectors: sum, difference, cross product, projection;
- one vector: its unit vector, its opposite;
- a matrix and vectors: M·v;
- a matrix: its eigenvectors, inverse, transpose;
- two matrices: their product;
- complex numbers: product, sum, quotient, conjugate, square.

### Object types

| Type | Space | Inspector |
|------|-------|-----------|
| Vector | 3D | Components (x, y, z) or Formula; Starts at (tail point); length |
| Matrix | 3D | Cells (3×3 / 4×4, presets) or Formula; determinant |
| Eigenvectors | 3D | Matrix formula (e.g. `M`); dashed lines with dots at λv |
| Function / Curve / Surface / Implicit | 3D | Expressions in x, y (u for curves) and t; ranges; Detail per unit; Heat map; Next example |
| Parameter | both | Slider / Animate / Formula; range; repeat style, easing, duration, start delay |
| Complex number | complex | Value; circle, cos/sin projections, helix options; length and angle |
| Product | complex | Two factors; rotation and scaling construction; length and angle breakdown |

### Formulas
Every field is a formula. It can use other objects in the same space by name. Available:
- numbers, `+ - * / ^`, implicit multiplication (`2x`, `3(a + b)`);
- vectors `[x, y, z]`, matrices `[[…], […], […]]` (3×3 or 4×4), `[u, v, w]` with vectors as columns;
- components `v.x`, the constants `pi e i`;
- `sin cos tan asin acos atan sinh cosh tanh exp log ln sqrt abs floor ceil atan2 min max`;
- vectors: `dot cross norm normalize`;
- matrices: `det tr inv transpose rotx roty rotz rot(axis, angle) scale translate eye`;
- complex numbers: `conj re im arg polar`;
- blending: `lerp(a, b, s)`, `keys(s, A, B, …)`; and `mod(a, b)`.

`M*v` applies a matrix to a vector (a 4×4 matrix treats v as a point); `M*N` composes; `M^-1` inverts.
`t` is the **scene time** in seconds, in every formula: `[cos(t), sin(t), 1]`, `rotz(t)`, `exp(i*t)`,
`sin(x - t)`. Objects that use it follow the Time panel (plots redraw while the clock plays). Plots use `x`
and `y` as their variables; curves use `u` for theirs (an object named `u` can't be used inside a curve's
formulas, but works everywhere else). Names must start with a letter and cannot be built-in names.
Problems are shown live: a red outline while typing a broken formula, an error line under the field, and a
red summary in the list for errors such as unknown names, wrong types or circular references.

### Parameters
A parameter is a named number that can be used in any formula. The switch at the top of its inspector picks
how it is driven:
- **Slider**: set it by hand within its range. It is static: time does not move it. Use it to morph things by
  hand, e.g. `lerp(eye(3), M, s)` blends the identity into M as `s` goes from 0 to 1.
- **Animate**: follows the scene clock from min to max.
  - **Once** stops at the end, **Loop** starts again, **Ping-pong** runs back and forth.
  - The **easing** button opens a grid of curves: Linear, Ease in, Ease out, Ease in-out, Sine, Back
    (overshoots), Elastic, Bounce and Steps. Hover one to read what it does.
  - The slider next to it sets the duration of one run; **Start at** delays it, so parameters can move one
    after another.
- **Formula**: the value is a formula of `t`, the scene time in seconds, and may use other objects.
  Examples: `2pi*t/5` (one turn every 5 s), `pi/4*sin(2t)`, `1 + k*cos(t)`, `mod(t, 3)` (restarts every 3 s).

Animate and Formula parameters show the current `t` and a link to the Time panel.

### Time panel
One clock for the whole scene, shared by both spaces:
- **Play / pause** (P), **Restart** (back to t = 0), and a **scrubber**: dragging it shows any moment, because
  time-driven parameters are computed from the time rather than stepped frame by frame.
- **Speed** (0.1× to 4×) and **Loop** (start again at the end, or stop there).
- **Length**: Auto uses the longest Once or Ping-pong animation (start delay included), or 10 s; click Auto
  to set a length by hand.
- The list of everything in the current space that follows the clock (Animate and Formula parameters, and
  any object whose formulas use `t`), with its current value; click one to select it.

The clock is saved with the scene. Undo never moves it.

### Morphing
`lerp(A, B, s)` blends any two values of the same kind (numbers, complex numbers, vectors, matrices); a plain
`0` stands for zero, so `lerp(0, v, s)` grows v from nothing. `keys(s, A, B, C, …)` runs through several values
as s goes from 0 to 1. The Add menu builds these for the selection: **Morph from identity** (a matrix),
**Grow from zero** (a vector or complex number) and **Blend** (two objects), each with a new slider `s`.

### Plot detail
**Detail** is set per unit of the range: samples per unit of x (functions) or u (curves), and grid cells per
unit for surfaces and implicit curves. Doubling the range doubles the samples, so the plot stays as smooth.
The line under the slider shows the actual count. Counts are capped for speed (20 000 samples, 400 × 400
surface cells, 800 × 800 implicit cells), and the line says so when the cap applies.

## Text fields

1. Click a field to edit it.
2. Type a number or a formula.
3. **Enter** or clicking elsewhere confirms, **Tab** / **Shift+Tab** confirms and moves on, **Esc** cancels.
4. Each confirmed edit is one undo step.

## The 3D space

z points up and the floor grid is the xy-plane, the usual math convention, so vectors and surfaces
`z = f(x, y)` agree. The axes are x (red), y (green), z (blue). Matrices are drawn as their moved basis on top
of the faint original basis. To watch a matrix appear, morph it from the identity with a slider (Add →
Morph from identity), or set that slider to Animate.

The floor grid has a line every unit and a stronger one every 5 units, and fades out at its edge. **Grid size**
in the View section sets how far it reaches (±5 to ±100 units).

## The complex plane

The real part runs along the red axis and the imaginary part along the green one, with the unit circle for
reference. Each tab keeps its own camera.

## Learning tools

- **Tutorial** (F1): pick a lesson. Each step explains one idea and usually asks for a small change, which is
  checked automatically. The control it talks about pulses with an outline. Next unlocks when the task is done,
  or press Skip. Finished lessons get a checkmark and are remembered.
- **Example gallery** (E): filter by topic and click a card; it replaces the objects of that space.
- **Calculation steps** (K): pick a topic (A·B, A×B, M·A, det M, M⁻¹, eigenvalues; or z₁·z₂, polar forms, Euler).
  It uses the selected matrix, vectors and complex numbers (or M, a, b, z1, z2), live.
- **Tooltips**: hover almost anything. They can be turned off in Settings.

## View section

- Grid (G), Axes, Labels (L), Axis length, Grid size, Reset camera (C), Save (F5) / Load (F9) of `scene.txt`
- Object colors are set per object: click the color swatch next to the name in the inspector (presets, hex
  value, "Theme color" resets it).

## Settings (gear icon)

Theme, interface size, **Surfaces** (Smooth: colors blend with soft shading by slope; Faceted: flat cells, one
color each, so the sample grid stays visible), tooltips, status overlay, reset panel layout. Preferences are saved in
`$XDG_CONFIG_HOME/axismundi/settings.txt` (usually `~/.config/axismundi/settings.txt`) when the app closes.

## Screenshots and fullscreen

- **F12** or the camera icon saves `axismundi_YYYYMMDD_HHMMSS.png` in the current folder. **Shift+F12** leaves the
  UI out.
- **F11** or the corners icon toggles borderless fullscreen.

## Quick reference

| Action | How |
|--------|-----|
| Camera ⟷ UI mode | Tab |
| Switch space | 1 / 2 or the tabs |
| Select / add to selection | Click / Ctrl+click (list or view) |
| Add / delete / duplicate | N or Add / Del / Ctrl+D |
| Edit a value | Click, type, Enter |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| Tutorial / gallery / steps / breakdown / analysis | F1 / E / K / B / M |
| Theme / interface size | T / Ctrl+= Ctrl+− Ctrl+0 |
| Help | H |
| Save / load / screenshot | F5 / F9 / F12 |
