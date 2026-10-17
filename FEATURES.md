# Features

Everything that is done, grouped by area. Open work is in [TODO.md](TODO.md); how to use the features is in
[README.md](README.md), [QUICKSTART.md](QUICKSTART.md) and [UI_GUIDE.md](UI_GUIDE.md).

## Objects and formulas
- [x] Any number of objects at runtime (vectors, matrices, eigenvectors, plots, parameters, complex numbers)
- [x] Every field is a formula that can use other objects by name (typed expression engine, dependency
      order, cycle and error reporting, rename updates formulas)
- [x] Two spaces: 3D space (z up) and the complex plane, each with its own objects and camera
- [x] Add menu with templates built from the selection; undo/redo for every change
- [x] Formula functions for blending: `lerp`, `keys`, `mod`
- [x] Scene object model: any number of objects, named formulas, parameters (src/scene, src/math/expr)

## Scene time `t` in every formula
- [x] `t` (the scene clock, seconds) usable in every formula: vectors, matrices, complex numbers and all
      plots, e.g. `[cos(t), sin(t), 1]`, `rotz(t)`, `exp(i*t)`, `sin(x - t)`
- [x] Curves use `u` for their own variable (`u from / to`); old scene files are converted
- [x] Functions, surfaces and implicit curves: `t` is the scene time (it was silently 0)
- [x] Plots that use `t` rebuild while the clock plays

## Animations and parameters
- [x] Add play/pause button
- [x] Implement timeline scrubber
- [x] Create animation presets (rotation, scaling, morph)
- [x] Add animation speed control
- [x] Support keyframe-based animations (now `keys(s, ...)` formulas)
- [x] One scene clock (Time panel) for all time-driven parameters; morphs as `lerp` formulas
- [x] Repeat styles (once, loop, ping-pong) and nine easing curves with previews
- [x] Parameters as slider, animation (with start delay) or formula of time
- [x] Scrubbing shows any moment (time-driven values are computed from the time)

## Integrals
- [x] Volume under z = f(x, y): Riemann boxes (left/middle/right, n per side as a formula) next to the precise value
      and the error; boxes below the floor in another color
- [x] Work ∫ F·dr along a curve: path colored by F·T, F arrows, a marker with the work so far
- [x] Templates, gallery, tutorial lesson, Analysis panel, LaTeX export, Paper style

## Quaternions
- [x] Quaternion value kind in formulas: `qaxis(axis, angle)`, `quat(w, x, y, z)`, `quat(M)`, Hamilton product,
      `rotate`, `mat`, `axis`, `angle`, `slerp`, `conj`/`inv`/`abs`/`unit`, components; lerp/keys slerp
- [x] Quaternion object: axis and angle, rotated basis, acting on a vector; non-unit quaternions explained
- [x] Analysis panel (axis/angle, matrix, yaw/pitch/roll; two quaternions: both products, angle between), Add menu
      templates, gallery examples, tutorial lesson, LaTeX export and Paper style

## Vector fields
- [x] Vector field object F(x, y, z) (`z` is a variable in field formulas), on the floor (Plane) or in a box
      (Volume), with ranges and arrows per unit
- [x] Arrows scaled by |F| (the longest fits a grid cell) and colored by strength; a same-length option
- [x] Flow lines (RK4 streamlines, kept evenly apart, arrowhead halfway); lines per axis
- [x] Particles drifting along the flow lines as the clock plays (a pure function of t, so scrubbing works)
- [x] Inspector with examples, gallery (rotation, source and sink, flow with particles, 3D swirl), Paper style
- [x] LaTeX export as a PGFPlots quiver plot with flow lines and particles

## Matrices
- [x] Display determinant value
- [x] Show matrix trace
- [x] Visualize eigenvalues and eigenvectors
- [x] Implement matrix inverse (if exists)
- [x] Add matrix decompositions (QR, SVD, LU)

## Complex numbers
- [x] Create Complex class
- [x] Implement Argand diagram renderer
- [x] Visualize complex multiplication as rotation + scaling
- [x] Show Euler's formula visualization (e^(iθ) = cos(θ) + i·sin(θ))
- [x] Add polar form display

## Function plotting
- [x] Implement 2D function plotter (y = f(x))
- [x] Add parametric curve support (x(t), y(t), z(t))
- [x] Implement 3D surface plotter (z = f(x, y))
- [x] Add implicit function plotting
- [x] Color-code function values (heat map)
- [x] Detail set per unit of the range (samples or cells per unit), with caps and the effective count shown
- [x] Surfaces and curves uploaded once as GPU meshes; smooth (shaded) or faceted surfaces (Settings)

## Interaction and input
- [x] Add text input for matrix values (implement text input boxes or integrate raygui)
- [x] Add text input for vector values
- [x] Implement keyboard shortcuts (I=Identity, R=Rotate, S=Scale, etc.)
- [x] Add undo/redo for transformations
- [x] Matrix presets accumulate: applying Rotate and then Scale should give scale·rotate (compose with the
      current matrix), not replace it with the last preset. Keep a way to start over (Identity).

## Visualization
- [x] Add 3D labels for vectors (billboard text)
- [x] Show vector magnitudes as floating text
- [x] Display dot product result
- [x] Add grid toggling (on/off button)
- [x] Add axes length customization
- [x] Larger floor grid (±5 to ±100) with major lines and a soft edge

## Interface
- [x] Add help overlay (press H for help)
- [x] Show current camera position
- [x] Add reset camera button
- [x] Add color picker for vectors
- [x] Implement draggable UI panels
- [x] Add fullscreen mode
- [x] Create dark/light theme toggle
- [x] Add screenshot capture (F12)
- [x] Resolution-independent interface with a size setting, embedded fonts, floating panels
- [x] Top bar buttons for every panel (Time, Analysis, Formula reference, ...)
- [x] Extract UI components into separate files

## Real mouse and keyboard
Tried with real X11 input (xdotool under Xvfb, `tools/dev/input-test.sh`) and fixed what was awkward:
- [x] Long formulas wrap onto up to four lines in the inspector (own text editor: cursor, selection,
      clipboard, Up/Down across lines, click to place the cursor, drag to select)
- [x] Big formula editor (⤢ button or Ctrl+Enter) with the result or error computed live
- [x] Collapsible side bar (Ctrl+B, top bar button, strip with expand and Add)
- [x] Touchpad taps shorter than a frame are no longer lost (mouse presses are latched)
- [x] Short fields (cells, names) and the Save dialog's suggested name select their text on focus, so typing
      replaces it (typing 7 in a cell holding 1 gave 17)
- [x] Enter in the big editor doesn't apply a formula with a syntax error
- [x] Screenshots on SIGUSR1 for scripted input tests

## View styles and visibility
- [x] Style setting in the View section: Colorful, Paper (the B/W preview, matching the LaTeX export), High
      contrast (thick lines, big labels, strong colors) and Chalkboard (green board, pastel colors)
- [x] One style definition (`view/ViewStyle`): a new style is data, not a new drawing path
- [x] The style is saved in the scene file; new scenes use the last style picked
- [x] The export panel's "B/W preview" switches the view to Paper and back
- [x] Smooth/Faceted surfaces default per style; Settings can override
- [x] Selection highlight: halo, label ring; other objects fade while something is selected (Settings switch)
- [x] 3D labels never overlap (greedy placement, leader lines)
- [x] Show only the selection (O, objects header button) and Show all

## Files and defaults
- [x] Implement save/load scene state
- [x] Sensible defaults: new vectors start with length 1 (e.g. along an axis), and the app opens with an
      empty scene the first time (no default objects); the current demo scene stays available (gallery /
      reset)
- [x] Better file loading and saving: choose the file name and folder (save as, open, recent files)
      instead of the fixed `scene.txt` in the current folder

## Learning tools
- [x] Step-by-step transformation breakdown
- [x] Show intermediate calculation steps
- [x] Add explanatory tooltips
- [x] Implement tutorial mode
- [x] Add example gallery
- [x] In-app formula reference (F2): cheat sheet with live examples and a box to try formulas

## LaTeX export (TikZ / PGFPlots, black and white)
Build a graph in the app, then get LaTeX code that drops into a normal document and looks like a textbook
figure: black and white, thin lines, math labels, the document's own fonts.

Output
- [x] A `tikzpicture` with a PGFPlots `axis` (2D) or 3D axis, copied to the clipboard and/or saved as `.tex`
- [x] Needed preamble listed in a comment (`\usepackage{pgfplots}`, `\pgfplotsset{compat=1.18}`,
      `\usetikzlibrary{arrows.meta}`); no font commands, so the figure uses the document's fonts
- [x] Width as a fraction of `\linewidth` (option), so it fits columns; optional complete `standalone`
      document to compile on its own
- [x] Plot data as sampled coordinates (faithful for parameters, objects and complex values; PGF math lacks
      many functions). Export detail is separate from screen detail and capped (e.g. 200 points per curve,
      30 × 30 surface) so LaTeX compiles quickly

Black-and-white style
- [x] Objects told apart by line style instead of color: solid, dashed, dotted, dash-dotted; gray levels
      as a second cue; a legend with object names in math (`$\theta$`, `$z_1$`, `$\vec a$`)
- [x] Vectors as arrows (`-{Stealth}`), labelled at the tip; eigenvectors as dashed lines
- [x] Matrices as the moved basis plus the image of the unit square/cube (light gray), with the
      original basis dotted
- [x] Functions and curves as thin black lines; implicit curves as contours
- [x] Surfaces as a black mesh on white (`mesh`/`surf` with `faceted color=black`, white or light-gray
      fill, `z buffer=sort`), optionally gray shading by height
- [x] Complex plane: unit circle, points and arrows for complex numbers, labels `$z_1$`, angle arcs for
      products
- [x] Axes in the usual style: thin, arrow tips, `$x$ $y$ $z$` labels, few ticks; grid off by default

Scope and view
- [x] Export the visible objects of the current space, or only the selection
- [x] 3D view taken from the current camera (`view={azimuth}{elevation}`); 2D axis when only floor
      objects (functions, implicit curves) are shown or for the complex plane
- [x] A snapshot at the current scene time `t`
- [x] Names to math: Greek names (`theta` → `\theta`), trailing digits as subscripts (`z1` → `z_1`)

In the app
- [x] Export panel (top bar, Ctrl+E): scope, width, 2D/3D, axes, legend, grid, gray surfaces, standalone
- [x] Print preview: draw the scene in the same black-and-white styles inside the app, so you design the
      figure while looking at it
- [x] Export to LaTeX (PGFPlots)

Direct PDF
- [x] "Save PDF…" next to "Save .tex…", enabled when `pdflatex` (or `lualatex`) is found; otherwise the tooltip
      says how to install TeX Live. The standalone version is compiled in a temporary folder, the PDF copied to
      the chosen path, the temporary files removed
- [x] Compiled in the background ("Compiling…") with a 30 s timeout; on failure LaTeX's first error lines show
      in the panel
- [x] `pdflatex` started with an argument list (no shell), so names with spaces or odd characters are safe

Code and tests
- [x] Exporter as a graphics-free module (scene → text) with unit tests on the generated code
- [x] Compile check of exported examples with pdflatex (TeX Live on the dev machine; tour writes figures)

## Performance
- [x] Optimize mesh generation (indexed GPU meshes built once per change)

## Testing
- [x] Add unit tests for Vector3D
- [x] Add unit tests for Matrix3D/4D
- [x] Unit tests for the expression engine, scene model, playback, file conversion and the formula reference
- [x] Scripted screenshot tour (`--tour`) for visual checks at several window sizes
