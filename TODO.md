# TODO: Future Enhancements

## High Priority (Core Features)

### Scene time `t` in every formula
- [x] `t` (the scene clock, seconds) usable in every formula: vectors, matrices, complex numbers and all
      plots, e.g. `[cos(t), sin(t), 1]`, `rotz(t)`, `exp(i*t)`, `sin(x - t)`
- [x] Curves use `u` for their own variable (`u from / to`); old scene files are converted
- [x] Functions, surfaces and implicit curves: `t` is the scene time (it was silently 0)
- [x] Plots that use `t` rebuild while the clock plays

### Objects and formulas
- [x] Any number of objects at runtime (vectors, matrices, eigenvectors, plots, parameters, complex numbers)
- [x] Every field is a formula that can use other objects by name (typed expression engine, dependency
      order, cycle and error reporting, rename updates formulas)
- [x] Two spaces: 3D space (z up) and the complex plane, each with its own objects and camera
- [x] Add menu with templates built from the selection; undo/redo for every change
- [x] Formula functions for blending: `lerp`, `keys`, `mod`

### Interactive Input
- [x] Add text input for matrix values (implement text input boxes or integrate raygui)
- [x] Add text input for vector values
- [x] Implement keyboard shortcuts (I=Identity, R=Rotate, S=Scale, etc.)
- [x] Add undo/redo for transformations

### Enhanced Visualization
- [x] Add 3D labels for vectors (billboard text)
- [x] Show vector magnitudes as floating text
- [x] Display dot product result
- [x] Add grid toggling (on/off button)
- [x] Add axes length customization

### User Experience
- [x] Add help overlay (press H for help)
- [x] Show current camera position
- [x] Add reset camera button
- [x] Implement save/load scene state

## Roadmap after the usability upgrades

In this order (each item below has its own section):
1. ~~LaTeX export of graphs~~ (done; follow-ups in "LaTeX export" under Medium Priority)
2. View styles: design to refine first, see "View styles" under Medium Priority
3. Vector fields: the first advanced-math object type (tests the "add an object type" recipe)
4. Quaternions: a new value kind in the expression engine
5. CI: `make test` and the headless tour on every push
6. Recording the Time panel to a PNG sequence or video
7. Later: the remaining Advanced Math items, Windows/macOS builds, performance

Before stacking new features, try the usability work with a real mouse and keyboard (dragging, typing,
picking) and fix what feels awkward.

## Medium Priority (Extended Features)

### Usability upgrades
- [x] Matrix presets accumulate: applying Rotate and then Scale should give scale·rotate (compose with the
      current matrix), not replace it with the last preset. Keep a way to start over (Identity).
- [x] Sensible defaults: new vectors start with length 1 (e.g. along an axis), and the app opens with an
      empty scene the first time (no default objects); the current demo scene stays available (gallery /
      reset)
- [x] Better file loading and saving: choose the file name and folder (save as, open, recent files)
      instead of the fixed `scene.txt` in the current folder
- [ ] Better visibility of objects: make vectors, matrices, plots and labels easier to see and tell apart
      (to define: thickness, contrast, labels, highlight of the selection, show/hide controls)

### LaTeX export (TikZ / PGFPlots, black and white)
Goal: build a graph in the app, then get LaTeX code that drops into a normal document and looks like a
textbook figure: black and white, thin lines, math labels, the document's own fonts. Not a screenshot.

Output
- [x] A `tikzpicture` with a PGFPlots `axis` (2D) or 3D axis, copied to the clipboard and/or saved as `.tex`
- [x] Needed preamble listed in a comment (`\usepackage{pgfplots}`, `\pgfplotsset{compat=1.18}`,
      `\usetikzlibrary{arrows.meta}`); no font commands, so the figure uses the document's fonts
- [x] Width as a fraction of `\linewidth` (option), so it fits columns; optional complete `standalone`
      document to compile on its own
- [x] Plot data as sampled coordinates (faithful for parameters, objects and complex values; PGF math lacks
      many functions); optional symbolic `\addplot {sin(deg(x))}` for simple functions. Export detail is
      separate from screen detail and capped (e.g. 200 points per curve, 30 × 30 surface) so LaTeX compiles
      quickly

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
- [ ] A frame series for the `animate` package
- [x] Names to math: Greek names (`theta` → `\theta`), trailing digits as subscripts (`z1` → `z_1`)

In the app
- [x] Export panel (top bar, Ctrl+E): scope, width, 2D/3D, axes, legend, grid, gray surfaces, standalone
- [ ] Option for symbolic plots (`\addplot {sin(deg(x))}`) instead of sampled coordinates
- [x] Print preview: draw the scene in the same black-and-white styles inside the app, so you design the
      figure while looking at it

Code and tests
- [x] Exporter as a graphics-free module (scene → text) with unit tests on the generated code
- [x] Compile check of exported examples with pdflatex (TeX Live on the dev machine; tour writes figures)
- [ ] The same compile check in CI

Direct PDF (and image) export
- [ ] "Save PDF…" next to "Save .tex…", enabled when `pdflatex` is found (fallback `lualatex`; otherwise the
      tooltip says how to install TeX Live): write the standalone version to a temporary folder, compile,
      copy the PDF to the chosen path, remove the temporary files
- [ ] Compile in the background with a "Compiling…" message and a timeout (~30 s); on failure show LaTeX's
      first error lines
- [ ] Start `pdflatex` with an argument list (no shell), so file names with spaces or odd characters are safe
- [ ] Note: the PDF uses LaTeX's default font (Computer Modern); later an option for a font package
      (`lmodern`, `mathpazo`, ...). For documents the `.tex` stays the better choice (document fonts)
- [ ] Later: PNG via `pdftoppm` (chosen resolution) and SVG via `dvisvgm`

### View styles (needs more iteration before implementing)
A Style setting in the View section chooses how the 3D view is drawn; the B/W preview becomes one style.
First choices (from the discussion, to be refined):
- [ ] Colorful: today's look (theme colors, heat maps, shaded surfaces)
- [ ] Paper: today's B/W preview (white, line styles and grays, black mesh), matching the LaTeX export
- [ ] Colorblind-safe: Okabe–Ito palette for objects, viridis scale for heat maps
- [ ] Chalkboard: dark green board, chalk-white lines, pastel colors
- [ ] High contrast: thicker lines and arrows, bigger labels, strong colors (projectors, slides)
- [ ] The style is saved in the scene file; new scenes use the last style picked
- [ ] The export panel's "B/W preview" becomes a shortcut: switch the view to Paper and back
- [ ] One style definition (background, ink, palette, line thickness, label colors, surface mode) so a new
      style is data, not a new drawing path; the Smooth/Faceted setting becomes a per-style default that
      Settings can override

Open questions to iterate on: the exact look of each style (palettes, thickness, labels); light/dark
theme interplay; whether objects outside the export scope stay faint in Paper; whether Blueprint or other
styles are worth adding.

### Complex Numbers
- [x] Create Complex class
- [x] Implement Argand diagram renderer
- [x] Visualize complex multiplication as rotation + scaling
- [x] Show Euler's formula visualization (e^(iθ) = cos(θ) + i·sin(θ))
- [x] Add polar form display

### Function Plotting
- [x] Implement 2D function plotter (y = f(x))
- [x] Add parametric curve support (x(t), y(t), z(t))
- [x] Implement 3D surface plotter (z = f(x, y))
- [x] Add implicit function plotting
- [x] Color-code function values (heat map)
- [x] Detail set per unit of the range (samples or cells per unit), with caps and the effective count shown
- [x] Surfaces and curves uploaded once as GPU meshes; smooth (shaded) or faceted surfaces (Settings)

### Animations
- [x] Add play/pause button
- [x] Implement timeline scrubber
- [x] Create animation presets (rotation, scaling, morph)
- [x] Add animation speed control
- [x] Support keyframe-based animations (now `keys(s, ...)` formulas)
- [x] One scene clock (Time panel) for all time-driven parameters; morphs as `lerp` formulas
- [x] Repeat styles (once, loop, ping-pong) and nine easing curves with previews
- [x] Parameters as slider, animation (with start delay) or formula of time
- [x] Scrubbing shows any moment (time-driven values are computed from the time)

### Matrix Operations
- [x] Display determinant value
- [x] Show matrix trace
- [x] Visualize eigenvalues and eigenvectors
- [x] Implement matrix inverse (if exists)
- [x] Add matrix decompositions (QR, SVD, LU)

## Low Priority (Nice to Have)

### UI Improvements
- [x] Add color picker for vectors
- [x] Implement draggable UI panels
- [x] Add fullscreen mode
- [x] Create dark/light theme toggle
- [x] Add screenshot capture (F12)
- [x] Resolution-independent interface with a size setting, embedded fonts, floating panels
- [x] Top bar buttons for every panel (Time, Analysis, Formula reference, ...)
- [x] Larger floor grid (±5 to ±100) with major lines and a soft edge

### Educational Features
- [x] Step-by-step transformation breakdown
- [x] Show intermediate calculation steps
- [x] Add explanatory tooltips
- [x] Implement tutorial mode
- [x] Add example gallery
- [x] In-app formula reference (F2): cheat sheet with live examples and a box to try formulas

### Advanced Math
- [ ] Quaternion visualization (roadmap step 3: a quaternion value kind, axis-angle view, slerp)
- [ ] Tensor visualization
- [ ] Differential geometry tools
- [ ] Vector field visualization (roadmap step 2: F(x, y, z) as arrows, flow lines, divergence and curl)
- [ ] Integration volume visualization
- [ ] Fourier transform visualization

### Performance
- [ ] Implement level-of-detail for large datasets
- [ ] Add frustum culling
- [x] Optimize mesh generation (indexed GPU meshes built once per change)
- [ ] Implement object pooling for vectors
- [ ] Add multi-threading for heavy computations

### Export/Import
- [ ] Save current transformation as JSON
- [ ] Load transformation from JSON
- [ ] Export scene as image (PNG, JPG) (PNG screenshots exist: F12; no JPG or size options yet)
- [ ] Export animation as video (roadmap step 5: record the Time panel to PNGs, then video)
- [ ] Import matrices from CSV
- [x] Export to LaTeX (PGFPlots): see "LaTeX export" under Medium Priority

### Platform Support
- [ ] Test on other Linux distributions
- [ ] Add macOS support
- [ ] Add Windows support
- [ ] Create installation script
- [ ] Package as AppImage/Flatpak

## Code Quality

### Testing
- [x] Add unit tests for Vector3D
- [x] Add unit tests for Matrix3D/4D
- [x] Unit tests for the expression engine, scene model, playback, file conversion and the formula reference
- [x] Scripted screenshot tour (`--tour`) for visual checks at several window sizes
- [ ] Add integration tests
- [ ] Implement benchmark suite
- [ ] Add CI/CD pipeline (roadmap step 4: `make test` + headless tour on every push)

### Documentation
- [ ] Add Doxygen comments to all headers
- [ ] Generate API documentation
- [ ] Create video tutorials
- [ ] Write detailed math explanations (the formula reference and calculation steps cover part of this)
- [ ] Add architecture diagrams

### Refactoring
- [x] Extract UI components into separate files
- [x] Scene object model: any number of objects, named formulas, parameters (src/scene, src/math/expr)
- [ ] Create preset system (JSON-based)
- [ ] Implement event system for better decoupling
- [ ] Add configuration file support
- [ ] Improve error handling

## Research Ideas

### Experimental Features
- [ ] VR support using Raylib VR
- [ ] AR support for mobile
- [ ] Web version using Emscripten
- [ ] GPU-accelerated computations
- [ ] Machine learning visualization (gradient descent, etc.)
- [ ] Quantum computing gate visualization

### Collaboration Features
- [ ] Multi-user mode (network sync)
- [ ] Share transformations via URL
- [ ] Collaborative editing
- [ ] Live presentation mode

## Known Issues

- The bundled `include/raygui.h` (5.1-dev) needs **raylib 5.5** (it calls
  `TextToFloat`, added in 5.5). Distribution packages of raylib 5.0 or older
  fail to link.
- Stock raylib builds also save `screenshotNNN.png` on F12 (raylib's own
  `SUPPORT_SCREEN_CAPTURE`), next to axismundi's timestamped screenshot.
- Dense surfaces that use `t` (or a playing parameter) are rebuilt every frame while the clock plays;
  at 400 × 400 cells that takes about 0.2 s per rebuild.
- Real mouse dragging, typing feel and picking in the viewport have only been checked through the
  scripted tour, not interactively.

## Getting Started with Development

To work on any of these features:

1. Fork/clone the repository
2. Create a feature branch
3. Read CLAUDE.md (architecture, conventions, how to add object types and formula functions)
4. Test thoroughly
5. Update documentation

## Contributing Guidelines

When implementing new features:

- Maintain the existing code style
- Keep math library independent of graphics
- Add a gallery example or tutorial step for user-facing features, and a formula reference entry for new
  formula functions
- Update README.md if adding major features
- Ensure no compiler warnings
- Test on at least one Linux distribution

---

**Priority Legend:**
- High: Essential for core functionality
- Medium: Valuable additions
- Low: Nice improvements

**Difficulty Estimate:**
- Easy: < 1 hour
- Medium: 1-4 hours
- Hard: 4+ hours

All High priority items are done; the Medium priority usability upgrades are open.
Most items in "Advanced Math" are hard difficulty.
