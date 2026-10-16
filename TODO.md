# TODO: Future Enhancements

## High Priority (Core Features)

### Interactive Input
- [ ] Add text input for matrix values (implement text input boxes or integrate raygui)
- [ ] Add text input for vector values
- [ ] Implement keyboard shortcuts (I=Identity, R=Rotate, S=Scale, etc.)
- [ ] Add undo/redo for transformations

### Enhanced Visualization
- [ ] Add 3D labels for vectors (billboard text)
- [ ] Show vector magnitudes as floating text
- [ ] Display dot product result
- [ ] Add grid toggling (on/off button)
- [ ] Add axes length customization

### User Experience
- [ ] Add help overlay (press H for help)
- [ ] Show current camera position
- [ ] Add reset camera button
- [ ] Implement save/load scene state

## Medium Priority (Extended Features)

### Complex Numbers
- [ ] Create Complex class
- [ ] Implement Argand diagram renderer
- [ ] Visualize complex multiplication as rotation + scaling
- [ ] Show Euler's formula visualization (e^(iθ) = cos(θ) + i·sin(θ))
- [ ] Add polar form display

### Function Plotting
- [ ] Implement 2D function plotter (y = f(x))
- [ ] Add parametric curve support (x(t), y(t), z(t))
- [ ] Implement 3D surface plotter (z = f(x, y))
- [ ] Add implicit function plotting
- [ ] Color-code function values (heat map)

### Animations
- [ ] Add play/pause button
- [ ] Implement timeline scrubber
- [ ] Create animation presets (rotation, scaling, morph)
- [ ] Add animation speed control
- [ ] Support keyframe-based animations

### Matrix Operations
- [ ] Display determinant value
- [ ] Show matrix trace
- [ ] Visualize eigenvalues and eigenvectors
- [ ] Implement matrix inverse (if exists)
- [ ] Add matrix decompositions (QR, SVD, LU)

## Low Priority (Nice to Have)

### UI Improvements
- [ ] Add color picker for vectors
- [ ] Implement draggable UI panels
- [ ] Add fullscreen mode
- [ ] Create dark/light theme toggle
- [ ] Add screenshot capture (F12)

### Educational Features
- [ ] Step-by-step transformation breakdown
- [ ] Show intermediate calculation steps
- [ ] Add explanatory tooltips
- [ ] Implement tutorial mode
- [ ] Add example gallery

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
- [ ] Add unit tests for Vector3D
- [ ] Add unit tests for Matrix3D/4D
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
- [ ] Extract UI components into separate files
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

Currently none! The application builds and runs without warnings or errors.

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
