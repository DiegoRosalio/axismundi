# TODO

Open work, roughly in the order it makes sense to do it. Everything already done is listed in
[FEATURES.md](FEATURES.md).

## Next up (in this order)

1. Vector fields: the first advanced-math object type (tests the "add an object type" recipe)
2. Quaternions: a new value kind in the expression engine
3. CI: `make test` and the headless tour on every push
4. Recording the Time panel to a PNG sequence or video
5. Later: the remaining Advanced Math items, Windows/macOS builds, performance

## Medium priority

### Look and usability
- [ ] Try the feel on real hardware: a physical mouse and touchpad, other keyboard layouts (typing `^`, `[`
      with dead keys), HiDPI screens. `tools/dev/input-test.sh` covers X11 events under Xvfb only
#### View styles: follow-ups
- [ ] Colorblind-safe style: Okabe–Ito palette for objects, viridis scale for heat maps
- [ ] Blueprint or other styles, if wanted
- [ ] Whether objects outside the export scope stay faint in Paper (today: yes)
- [ ] Selected plots: an outline on the curve or surface itself (today only the others fade)

### LaTeX export follow-ups

Direct PDF (and image) export
- [ ] Note: the PDF uses LaTeX's default font (Computer Modern); later an option for a font package
      (`lmodern`, `mathpazo`, ...). For documents the `.tex` stays the better choice (document fonts)
- [ ] Later: PNG via `pdftoppm` (chosen resolution) and SVG via `dvisvgm`

More export options
- [ ] Option for symbolic plots (`\addplot {sin(deg(x))}`) instead of sampled coordinates
- [ ] A frame series for the `animate` package

### Advanced math
Vector fields and quaternions first (roadmap steps 4 and 5); the rest later.
- [ ] Vector field visualization (roadmap step 4: F(x, y, z) as arrows, flow lines, divergence and curl)
- [ ] Quaternion visualization (roadmap step 5: a quaternion value kind, axis-angle view, slerp)
- [ ] Fourier transform visualization
- [ ] Integration volume visualization
- [ ] Differential geometry tools
- [ ] Tensor visualization

### Automation (CI)
- [ ] Add CI/CD pipeline (roadmap step 6: `make test` + headless tour on every push)
- [ ] The same compile check in CI (LaTeX export figures compiled with pdflatex)

## Low priority

### Export/Import
- [ ] Export animation as video (roadmap step 7: record the Time panel to PNGs, then video)
- [ ] Export scene as image (PNG, JPG) (PNG screenshots exist: F12; no JPG or size options yet)
- [ ] Save current transformation as JSON
- [ ] Load transformation from JSON
- [ ] Import matrices from CSV

### Performance
- [ ] Add multi-threading for heavy computations (e.g. rebuilding dense surfaces that use `t`)
- [ ] Implement level-of-detail for large datasets
- [ ] Add frustum culling
- [ ] Implement object pooling for vectors

### Platform Support
- [ ] Test on other Linux distributions
- [ ] Add Windows support
- [ ] Add macOS support
- [ ] Create installation script
- [ ] Package as AppImage/Flatpak

## Code quality

### Testing
- [ ] Add integration tests
- [ ] Implement benchmark suite

### Refactoring
- [ ] Improve error handling
- [ ] Add configuration file support
- [ ] Implement event system for better decoupling
- [ ] Create preset system (JSON-based)

### Documentation
- [ ] Write detailed math explanations (the formula reference and calculation steps cover part of this)
- [ ] Add architecture diagrams
- [ ] Add Doxygen comments to all headers
- [ ] Generate API documentation
- [ ] Create video tutorials

## Research ideas

### Experimental Features
- [ ] Web version using Emscripten
- [ ] Machine learning visualization (gradient descent, etc.)
- [ ] Quantum computing gate visualization
- [ ] GPU-accelerated computations
- [ ] VR support using Raylib VR
- [ ] AR support for mobile

### Collaboration Features
- [ ] Live presentation mode
- [ ] Share transformations via URL
- [ ] Multi-user mode (network sync)
- [ ] Collaborative editing

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
- Plots (functions, curves, surfaces, implicit curves) and eigenvector lines cannot be picked in the view
  yet; select them in the objects list.

## Getting Started with Development

To work on any of these features:

1. Fork/clone the repository
2. Create a feature branch
3. Read CLAUDE.md (architecture, conventions, how to add object types and formula functions)
4. Test thoroughly
5. Update documentation (move finished items from TODO.md to FEATURES.md)

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

**Priority:** Next up = the current plan; Medium = valuable additions; Low = nice improvements;
Research = ideas to explore.

**Difficulty:** Easy < 1 hour, Medium 1–4 hours, Hard 4+ hours. Most Advanced Math items are hard.
