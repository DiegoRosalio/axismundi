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
  side bar scrolls with the mouse wheel. **Ctrl+B** (or the side bar button left of the logo) collapses it to
  a thin strip with an expand chevron and an Add button; clicking an object in the view while it's collapsed
  selects it, and a hint says Ctrl+B shows the inspector. The state is remembered.
- **Floating panels**: drag them by the title bar, collapse them with the chevron, close them with ×. Clicking
  a panel brings it to the front, and only the top panel under the mouse reacts to clicks. Positions are kept
  as a fraction of the free space, so they stay in place when the window is resized. Settings → "Reset panel
  layout" puts everything back.

Top bar buttons, from the left: fullscreen and screenshot; **play / pause** the scene clock and show the
**Time panel**; the **Analysis** panel, the transformation breakdown (3D space only), calculation steps, the
example gallery and the tutorial; then theme, help and settings. A highlighted button means its panel is open.

## The camera: Orbit or Fly

**Settings → Camera** picks how the view moves (saved as a preference).

- **Orbit** (default, like Blender): on the empty 3D view, **drag** turns around the center point, **right-drag**
  (or Shift+drag, or the middle button) slides the view, the **wheel** zooms. A click that doesn't move selects
  as usual, and a drag that starts on a panel never moves the camera. **.** (or **Focus** under View) makes the
  selected object's point the center; **keypad 1 / 3 / 7** (or **Front / Right / Top** under View) look along
  y, along x, or straight down with x to the right. The complex plane stays face-on: a drag pans.
- **Fly** (like a game): Tab enters camera mode, below.

## Camera mode and UI mode (fly camera)

- **Camera mode** (cursor hidden, green status dot): mouse look, WASD move, Space/Ctrl up/down, Shift faster.
  Panels ignore the mouse. With the orbit camera Tab only shows a hint.
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
  - In the view, selected objects get a halo and an accent ring around their label, and the others fade (Settings
    can turn the fading off). Labels never overlap: a label that had to move gets a thin line back to its point.
  - **Only the selection** (O, or the target button in the header) hides everything else in the view (the list
    keeps them, faded); a chip at the top of the view says it's on. **Show all** shows hidden objects again
    (one undo step) and turns it off.
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
- complex numbers: product, sum, quotient, conjugate, square;
- two quaternions: compose a·b or b·a, turn from a to b (slerp with a slider); a quaternion and vectors: rotate
  them; one quaternion: its matrix, its inverse, a turn from no rotation; a matrix: its quaternion.

### Object types

| Type | Space | Inspector |
|------|-------|-----------|
| Vector | 3D | Components (x, y, z) or Formula; Starts at (tail point); length |
| Matrix | 3D | Cells (3×3 / 4×4, presets that add up; Identity starts over) or Formula; determinant |
| Eigenvectors | 3D | Matrix formula (e.g. `M`); dashed lines with dots at λv |
| Function / Curve / Surface / Implicit | 3D | Expressions in x, y (u for curves) and t; ranges; Detail per unit; Heat map; Next example |
| Fourier series of f(x) | 3D | f, [a, b], Terms (a formula); Show f, Harmonics, Repeat outside; spectrum chart and coefficients |
| Fourier epicycles | complex | z(u) for one lap, Circles, Laps per second; the biggest circles; Next example |
| Moving frame on a curve | 3D | Curve x(u) y(u) z(u), u range, At u (a formula); Frame T/N/B, Osculating circle, Curvature comb, Curve; κ, τ, speed |
| Tangent line on y = f(x) | 3D | f, At x (a formula), Secant h (0 = none), x range; Graph of f, Graph of f', Slope triangle; f'(x0), the line, f'' |
| Tangent plane and gradient | 3D | z = f(x, y), At x / At y, ranges; Surface, Tangent plane, Slopes along x and y, Gradient, Normal; ∂f/∂x, ∂f/∂y |
| Velocity on a curve | 3D | Curve x(u) y(u) z(u), u range, At u; Velocity, Acceleration, Split acceleration, Curve, Arrow scale; speed |
| Taylor polynomial of f(x) | 3D | f, About x (a formula), Degree (a formula, 0..20), x range; Graph of f, Previous degree, Separate terms; the polynomial, its largest gap to f |
| Newton's method on f(x) | 3D | f, Start x0, Steps (a formula, 0..30), x range; Graph of f; the table of x_k and f(x_k), why it stopped |
| Volume under z = f(x, y) | 3D | f, x/y ranges, boxes per side (a formula), Left/Middle/Right rule, Boxes, Surface; boxes' sum, exact value, error |
| Work ∫ F·dr | 3D | F(x, y, z), curve x(u) y(u) z(u), u range, Up to (0..1); work so far and over the whole path |
| Quaternion | 3D | q = formula (e.g. `qaxis([0, 0, 1], pi/2)`); the four numbers, angle and axis, \|q\|; Axis and angle, Rotated basis, Acts on a vector; Next example |
| Vector field | 3D | F(x, y, z) (a vector formula); Plane / Volume; ranges; arrows per unit; color by strength, same length, flow lines, particles (+ speed); Next example |
| Parameter | both | Slider / Animate / Formula; range; repeat style, easing, duration, start delay |
| Complex number | complex | Value; circle, cos/sin projections, helix options; length and angle |
| Product | complex | Two factors; rotation and scaling construction; length and angle breakdown |

### Formulas
The **Formula reference** (F2) has all of this with live examples. Every field is a formula. It can use other objects in the same space by name. Available:
- numbers, `+ - * / ^`, implicit multiplication (`2x`, `3(a + b)`);
- vectors `[x, y, z]`, matrices `[[…], […], […]]` (3×3 or 4×4), `[u, v, w]` with vectors as columns;
- components `v.x`, the constants `pi e i`;
- `sin cos tan asin acos atan sinh cosh tanh exp log ln sqrt abs floor ceil atan2 min max mod sign`;
- vectors: `dot cross norm normalize`;
- matrices: `det tr inv transpose rotx roty rotz rot(axis, angle) scale translate eye`;
- complex numbers: `conj re im arg polar`;
- quaternions: `qaxis(axis, angle) quat(w, x, y, z) quat(M) rotate(q, v) mat(q) axis(q) angle(q) slerp(a, b, s)`,
  `q1*q2` (q2 first), `conj inv abs unit`, `q.w q.x q.y q.z`; `lerp`/`keys` slerp between quaternions;
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

### Fourier
- **Fourier series of f(x)** (3D space, Add → Fourier): f on [a, b] rebuilt as a₀/2 + Σ aₖ cos(kωx) + bₖ sin(kωx)
  with ω = 2π/(b − a). The partial sum (colored) is drawn next to f (gray), over three periods when **Repeat
  outside** is on; **Harmonics** shows the first 8 waves. The inspector has a **spectrum** (the strength
  √(aₖ² + bₖ²) of each wave) and the first coefficients. Terms is a formula, so a slider can add waves. Near a
  jump the sums overshoot by about 9% (Gibbs).
- **Fourier epicycles** (complex plane): a closed curve z(u), u from 0 to 2π (e.g. a heart or a square), drawn by
  spinning circles, biggest first; circle k turns k times per lap. **Circles** sets how many; **Laps per second**
  the speed (press P). The last lap is traced, and the value is the tip.
- Gallery: square wave from sines, Gibbs overshoot, epicycles draw a heart / a square. Tutorial: "Fourier: sums
  of waves". LaTeX export draws f and the sum, or the circles and the traced path.

### Moving frame on a curve
Add → Calculus → **Moving frame on a curve** (or select a curve and use the template). At the point u = u0:
- **T** (red) is the direction of motion, **N** (green) points towards the center of the turn, **B** = T × N (blue).
- **κ** (curvature) = 1/radius of the **osculating circle**, the circle that hugs the curve best there (gray).
  **τ** (torsion) measures how fast the curve leaves the plane it turns in: 0 for flat curves.
- **At u** is a formula: a slider, or `mod(t, 4pi)` to ride along with the clock.
- The **curvature comb** draws κ along the whole curve (long teeth where it bends most).
- The value is the point r(u0), usable in other formulas. Gallery: helix, ellipse, parabola. Tutorial: "Curvature
  and the moving frame". LaTeX export draws the curve, the frame and the circle.

### Derivatives
Add → Calculus (or select a function, surface or curve and use its template).
- **Tangent line on y = f(x)**: the point (x0, f(x0)) with its tangent line; the value is the slope f'(x0). The
  **slope triangle** shows one step right climbing f'(x0); **Graph of f'** draws the slope at every x as a second
  curve (it crosses 0 at peaks and dips). **Secant h** draws the line through x0 and x0 + h: shrink h (or animate
  it) to watch the secant turn into the tangent. The inspector gives the line's equation and f'' (bending up or
  down).
- **Tangent plane and gradient**: on z = f(x, y), the plane touching the surface at (x0, y0), the slopes along x
  (red) and y (green), and the gradient (∂f/∂x, ∂f/∂y) drawn on the floor: the steepest way up. The value is the
  gradient as a vector.
- **Velocity on a curve**: r'(u0) (the value) and r''(u0) at a point of a curve; **Split acceleration** shows the
  part along the motion (speeding up or slowing down) and the part across it (turning). **Arrow scale** only
  changes how long the arrows are drawn.
- At x / At y / At u are formulas: a slider or the clock moves the point. Gallery: secant becomes tangent, f and
  f', gradient points uphill, velocity on a helix, speeding up vs turning. Tutorial: "Derivatives: slopes and
  tangents". LaTeX export draws each (the tangent as a 2D figure).

### Taylor polynomials and Newton's method
Add → Calculus (or select a function and use its template).
- **Taylor polynomial of f(x)**: the polynomial of degree N that copies f at the center a: the value, the slope,
  the bending... (degree 1 is the tangent line). **Degree** and **About x** are formulas, so `min(floor(t), 15)`
  adds a term every second and a slider moves the center. **Previous degree** draws the polynomial one degree
  lower (faint), so you see what the newest term changes; **Separate terms** draws each term on its own. The
  coefficients come from f on a small circle in the complex plane (accurate to degree 20); formulas that can't take
  complex values (`floor`, `sign`, `mod`) or aren't smooth (`abs`) get degrees up to 4 from nearby values, and the
  inspector says so. Example: 1/(1 + x²) only fits for |x| < 1, because 1 + x² = 0 at x = ±i.
- **Newton's method on f(x)**: from **Start x0**, each step follows the tangent at (x, f(x)) down to the x axis.
  **Steps** is a formula (the clock can reveal one step at a time). The value is the last x (a root estimate, usable
  by name); the inspector lists every step and says why it stopped (converged, flat tangent, diverging).
- Gallery: sin(x) term by term, e^x around a moving center, where the series stops working, Newton finds √2,
  Newton overshoots. Tutorial: "Taylor polynomials and Newton's method". LaTeX export draws both (2D).

### Integrals
Add → Calculus.
- **Volume under z = f(x, y)**: the double integral over a rectangle. Boxes stand on a grid of n × n squares, each as
  tall as f at its sample point (left corner, middle or right corner); their sum is the Riemann sum, shown next to
  the precise value and the error. Volume below the floor counts negative (other color). n is a formula, so
  `round(2 + 18*s)` with an animated s shows the boxes refining. The value (the precise integral) can be used in
  other formulas.
- **Work ∫ F·dr**: a vector field F along a curve. The path is green where F pushes along the motion and red where
  it pushes against it, with F drawn at a few points. **Up to** (0..1) moves a marker and gives the work so far,
  e.g. `mod(t/8, 1)` to travel the path with the clock.
- Templates: a selected surface → its volume; a selected curve and vector field → the work. Gallery: volume under a
  paraboloid, refining boxes, work around a circle, path independence. Tutorial: "Integrals as volumes". LaTeX
  export draws the boxes and surface, or the path with F.

### Quaternions
A quaternion w + xi + yj + zk is a rotation in four numbers: turning by an angle about an axis gives
w = cos(angle/2) and (x, y, z) = the axis × sin(angle/2). Add → Linear algebra → **Quaternion**.
- Write them with functions: `qaxis([0, 0, 1], pi/2)` (a quarter turn about z), `quat(w, x, y, z)`, `quat(M)`
  from a rotation matrix. Results show as `0.707 + 0.707k`. There are no i/j/k units to type: `i` is the complex
  unit and j, k stay free as names.
- **q1*q2** is "q2 first, then q1"; the order matters. `rotate(q, v)` turns a vector, `mat(q)` gives the matrix,
  `axis(q)` and `angle(q)` read it back, `slerp(a, b, s)` turns from a to b at constant speed (lerp and keys do the
  same with quaternions).
- The view shows the **axis** (dashed line, arrow, label `q  90° about (0, 0, 1)`), the **angle** as an arc with
  an arrowhead (right-hand rule), the **rotated basis**, and with **Acts on a vector** a vector v (gray), its
  image q v q⁻¹ and the circle its tip follows.
- Any quaternion is allowed; one whose length isn't 1 is drawn as the rotation of q/|q| and the inspector says so.
- The names `rotate`, `mat`, `axis` and `angle` can still be object names: followed by `(` they are the functions.
- Analysis, templates, LaTeX export and the Paper style support them. Gallery: quarter turn, order matters,
  slerp, quaternion from a matrix. Tutorial: "Quaternions".

### Vector fields
Add → Fields → **Vector field**. **F(x, y, z)** is a vector formula of the point, such as `[-y, x, 0]`
(a rotation), `[x, y, 0]` (a source) or `k*[-y, x, z]` with a parameter k; `t` makes it change with the clock.
`z` is a variable only here (elsewhere it is an ordinary name).
- **Plane** draws F on the floor z = 0 using its x and y components; **Volume** fills a box (z range too).
- Arrows start at grid points. Their length is proportional to |F|, with the longest filling one grid cell, and
  **Color by strength** colors them with the heat map (blue weak, red strong). **Same length** shows only the
  direction. **Detail** sets arrows per unit (capped at 40 per axis on a plane, 12 in a volume).
- **Flow lines** follow F everywhere (streamlines, traced with RK4) and carry an arrowhead halfway. A line stops
  where it comes too close to another one, so spirals and sinks stay readable. **Lines per axis** sets how many
  start (2–16 on a plane, 2–4 in a volume).
- **Particles** ride the flow lines at the field's speed (× **Speed**) while the clock plays (P). They are a
  function of the time, so scrubbing works. For a field that changes with t they follow the current flow
  lines, an approximation of the true paths.
- The Time panel lists fields with particles. The gallery has a rotation field, a source and sink, a flow with
  particles and a 3D swirl.
- LaTeX export: arrows as a quiver plot, flow lines and particles (at the current time) in the object's line
  style; a plane field gives a 2D figure.

### Plot detail
**Detail** is set per unit of the range: samples per unit of x (functions) or u (curves), and grid cells per
unit for surfaces and implicit curves. Doubling the range doubles the samples, so the plot stays as smooth.
The line under the slider shows the actual count. Counts are capped for speed (20 000 samples, 400 × 400
surface cells, 800 × 800 implicit cells), and the line says so when the cap applies.

## Text fields

1. Click a field to edit it. Short fields (numbers, cells, names, ranges) select their text, so typing
   replaces it; formula fields place the cursor where you click (drag to select).
2. Type a number or a formula. Long formulas wrap onto up to four lines; ← → ↑ ↓, Home/End, Shift to select,
   Ctrl+A, Ctrl+C / Ctrl+X / Ctrl+V work as usual.
3. **Enter** or clicking elsewhere confirms, **Tab** / **Shift+Tab** confirms and moves on, **Esc** cancels.
4. Each confirmed edit is one undo step.

**Big formula editor**: the ⤢ button at the right of a formula field, or **Ctrl+Enter** while typing in it,
opens a wide editor over the view with the text so far. Under it the result is computed live (`= (2, 0, 1)
(vector)`) or the error is shown in red. **Enter** applies (not while the formula has a syntax error), **Esc**
cancels, and the Formula reference (F2) is one click away.

## The 3D space

z points up and the floor grid is the xy-plane, the usual math convention, so vectors and surfaces
`z = f(x, y)` agree. The axes are x (red), y (green), z (blue). Matrices are drawn as their moved basis on top
of the faint original basis. To watch a matrix appear, morph it from the identity with a slider (Add →
Morph from identity), or set that slider to Animate.

The grid reaches the horizon wherever you look and adapts to the zoom: zoom in and its lines come every 0.1,
0.01…; zoom out and they come every 10, 100…, the finest lines fading in and out smoothly. **Numbers on axes**
label the axes at a step that follows the zoom too (x and y on both sides along the floor, z upward). In the
View section:
- **Grid spacing**: Auto (follows the zoom) or 1 unit (a line every unit, stronger every 5, for counting).
- 3D: **Floor xy**, **Wall xz**, **Wall yz** turn the grid on each plane through the origin on or off.
- Complex plane: **Square grid** or **Polar grid** (circles |z| = r and rays every 15°, stronger every 45°).
The grid is drawn by a GPU shader; if a graphics card can't run it, the older line grid (floor only, lines every
unit) is used instead.

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
- **Formula reference** (F2, the Σ button in the top bar or in the inspector header): the formula language as
  a cheat sheet. **Try a formula** evaluates anything you type against the objects of the current space (with
  `t` = the scene time, `x` = 1, `y` = 2; matrices are shown in full). Below it, search or pick a section
  (basics, names, time, vectors, matrices, complex numbers, functions, blending, errors). Every entry shows its
  syntax, what it does and an example with its live result; click an example to copy it into the box.
- **Tooltips**: hover almost anything. They can be turned off in Settings.

## View section

- Grid (G), Axes, Labels (L), grid spacing, grid planes (3D) or square/polar (complex plane), Numbers on axes,
  Axis length, Reset camera (C), Front / Right / Top / Focus
- **Style**: how the 3D view is drawn. The panels keep the dark/light theme.
  - **Colorful**: theme colors, heat maps, shaded surfaces.
  - **Paper**: black and white on white, in the LaTeX export's line styles and grays (the export panel's B/W
    preview switches to it and back).
  - **High contrast**: thick lines and arrows, big labels, strong colors on black (or white in the light theme),
    for projectors and slides.
  - **Chalkboard**: a green board, chalk-white axes and grid, pastel objects and softened heat maps.

  The style is saved in the scene file; the last style picked is used at startup and for new scenes.
- **New** (clear both spaces), **Open…** (Ctrl+O / F9), **Save** (Ctrl+S / F5; asks for a name the first time),
  **Save as…** (Ctrl+Shift+S), and the current file name. The file dialog browses folders (scene files are
  `.txt`), remembers recent files, and asks before replacing a file. Opening and New can be undone.
- Object colors are set per object: click the color swatch next to the name in the inspector (presets, hex
  value, "Theme color" resets it).

## Settings (gear icon)

Theme, interface size, **Surfaces** (By style: each view style picks, smooth for Colorful and High contrast,
faceted for Paper and Chalkboard; Smooth: colors blend with soft shading by slope; Faceted: flat cells, one color
each, so the sample grid stays visible), **Fade unselected objects** (on: while something is selected the other
objects fade toward the background), tooltips, status overlay, reset panel layout. Preferences are saved in
`$XDG_CONFIG_HOME/axismundi/settings.txt` (usually `~/.config/axismundi/settings.txt`) when the app closes.

## Export to LaTeX

**Ctrl+E**, or the export button (a page with an arrow) in the top bar, opens the export panel. The panel isn't
modal: you can keep adjusting the scene while it's open, and the code updates live.

- **Visible objects / Selection**: what goes into the figure. Parameters are numbers and are not drawn;
  objects with errors are left out. The panel says what was left out and why.
- **Auto / 2D / 3D**: Auto makes a 2D figure when everything lies in the floor plane (functions, implicit
  curves, flat vectors and matrices) or for the complex plane. Otherwise the figure is 3D, seen from the
  current camera angle (rotate the view to choose it).
- **Width**: a fraction of `\linewidth`, so the figure fits your columns.
- **Axes, Legend, Grid, Gray surfaces, Standalone**:
  - Plots get legend entries; vectors and points are labelled at their tips.
  - Surfaces are white with a black mesh, or shaded in grays by height.
  - Standalone gives a complete document to compile on its own.
- **B/W preview** switches the view to the Paper style (and back when turned off): the export's line styles,
  grays and labels, so what you see is what you get. Objects outside the export are drawn faint.
- **Copy LaTeX** puts the code on the clipboard; **Save .tex…** writes a file to `\input`.
- **Save PDF…** compiles the figure as a standalone document with `pdflatex` (or `lualatex`) in the background
  and saves the PDF (a figure takes a few seconds; it is stopped after 30 s). If LaTeX fails, its first error lines
  appear in the panel. The button is disabled, with a hint, when no LaTeX is installed. The PDF uses LaTeX's
  default font; inside a document, the `.tex` picks up the document's fonts.

**Style.**
- Objects are told apart by line style: solid, dashed, dotted, dash-dotted, loosely dashed. After five
  objects the gray level changes as well.
- A matrix is drawn as its moved basis plus the light-gray image of the unit square or cube, with the
  original basis dotted. Eigenvectors are dashed lines with their λ.
- The complex plane gets a unit circle, arrows, cos/sin projections and angle arcs when those options are on
  for an object.
- Plots are sampled at a capped resolution (200 points per curve, 30 × 30 for surfaces), so documents compile
  quickly.

To use the figure, add to your preamble (the first comment of the code lists these too):

```latex
\usepackage{pgfplots}
\pgfplotsset{compat=1.18}
\usetikzlibrary{arrows.meta}
```

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
| Show only the selection | O, or the target button in the objects header; Show all brings everything back |
| Add / delete / duplicate | N or Add / Del / Ctrl+D |
| Edit a value | Click, type, Enter |
| Undo / redo | Ctrl+Z / Ctrl+Y |
| Tutorial / gallery / steps / breakdown / analysis | F1 / E / K / B / M |
| Theme / interface size | T / Ctrl+= Ctrl+− Ctrl+0 |
| Help | H |
| Formula reference / export to LaTeX | F2 / Ctrl+E |
| Big formula editor / hide the side bar | ⤢ or Ctrl+Enter / Ctrl+B |
| Save / save as / open / screenshot | Ctrl+S (F5) / Ctrl+Shift+S / Ctrl+O (F9) / F12 |
