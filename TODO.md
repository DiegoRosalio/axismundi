# TODO: Future Enhancements

## High Priority (Core Features)

### Scene time `t` in every formula
- [x] `t` (the scene clock, seconds) usable in every formula: vectors, matrices, complex numbers and all
      plots, e.g. `[cos(t), sin(t), 1]`, `rotz(t)`, `exp(i*t)`, `sin(x - t)`
- [x] Curves use `u` for their own variable (`u from / to`); old scene files are converted
- [x] Functions, surfaces and implicit curves: `t` is the scene time (it was silently 0)
- [x] Plots that use `t` rebuild while the clock plays

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

## Medium Priority (Extended Features)

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

### Animations
- [x] Add play/pause button
- [x] Implement timeline scrubber
- [x] Create animation presets (rotation, scaling, morph)
- [x] Add animation speed control
- [x] Support keyframe-based animations (now `keys(s, ...)` formulas)
- [x] One scene clock (Time panel) for all time-driven parameters; morphs as `lerp` formulas

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

### Educational Features
- [x] Step-by-step transformation breakdown
- [x] Show intermediate calculation steps
- [x] Add explanatory tooltips
- [x] Implement tutorial mode
- [x] Add example gallery

### Advanced Math
- [ ] Quaternion visualization
- [ ] Tensor visualization
- [ ] Differential geometry tools
- [ ] Vector field visualization
- [ ] Integration volume visualization
- [ ] Fourier transform visualization

### Performance
- [ ] Implement level-of-detail for large datasets
- [ ] Add frustum culling
- [ ] Optimize mesh generation
- [ ] Implement object pooling for vectors
- [ ] Add multi-threading for heavy computations

### Export/Import
- [ ] Save current transformation as JSON
- [ ] Load transformation from JSON
- [ ] Export scene as image (PNG, JPG)
- [ ] Export animation as video
- [ ] Import matrices from CSV
- [ ] Export to LaTeX (TikZ)

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
- [ ] Add integration tests
- [ ] Implement benchmark suite
- [ ] Add CI/CD pipeline

### Documentation
- [ ] Add Doxygen comments to all headers
- [ ] Generate API documentation
- [ ] Create video tutorials
- [ ] Write detailed math explanations
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

## Getting Started with Development

To work on any of these features:

1. Fork/clone the repository
2. Create a feature branch
3. See EXAMPLES.md for code patterns
4. Test thoroughly
5. Update documentation

## Contributing Guidelines

When implementing new features:

- Maintain the existing code style
- Keep math library independent of graphics
- Add examples to EXAMPLES.md
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

Most items in "High Priority" are medium difficulty.
Most items in "Advanced Math" are hard difficulty.
