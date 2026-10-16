# Linear Algebra 3D Visualizer

An interactive 3D visualization tool for exploring linear algebra concepts including vectors, matrices, and transformations.

## Features

- **3D Coordinate Grid**: Interactive 3D Cartesian coordinate system with labeled axes
- **Matrix Transformations**: Visualize 3x3 and 4x4 matrix transformations in real-time
- **Interactive Input Controls**: Full raygui-powered interface with:
  - Text input boxes for all matrix values (3x3 and 4x4)
  - Text input boxes for vector components (X, Y, Z)
  - Real-time editing and instant visualization updates
  - Toggle between 3x3 and 4x4 matrices
- **Vector Operations**: Display and manipulate vectors with visual representations of:
  - Vector addition
  - Cross product
  - Dot product (future)
- **First-Person Camera**: Full FPS-style camera with:
  - WASD movement in any direction
  - Mouse look (360° rotation)
  - Vertical movement (Space/Ctrl)
  - Sprint mode (Shift)
  - Right-click to lock/unlock camera
- **Preset Transformations**: 5+ built-in transformations:
  - Identity Matrix
  - Rotation (45° around Z-axis)
  - Scaling (2x uniform)
  - Shearing (X by 0.5)
  - Reflection (across X-axis)
- **Professional UI**: Clean raygui-based panels with proper text editing
- **Extensible Architecture**: Built to support future features like:
  - Complex number visualization
  - Function plotting
  - Time-based animations
  - Multivariate functions
  - Integration visualization

## Requirements

- C++20 compatible compiler (g++ recommended)
- Make
- Raylib 5.0+ (automatically included via system package or manual installation)
- Linux operating system

## Installation

### Install Raylib

On Arch Linux:
```bash
sudo pacman -S raylib
```

On Debian/Ubuntu:
```bash
sudo apt install libraylib-dev
```

Or build from source:
```bash
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
```

### Build the Project

```bash
make build
```

## Usage

Run the application:
```bash
make run
```

Or execute directly:
```bash
./bin/linalg_viz
```

### Controls

- **Camera Controls** (First-Person):
  - Right-click: Lock/unlock camera (hide/show cursor)
  - WASD: Move forward/back/left/right
  - Mouse: Look around (when locked)
  - Space/Ctrl: Move up/down
  - Shift: Sprint (2x speed)
  - ESC: Unlock camera

- **UI Panel (Left Side)**:
  - **Matrix Panel**: Toggle between 3x3/4x4, click any cell to edit values
  - **Vector Panel**: Edit X, Y, Z components for vectors A and B
  - **Preset Panel**: Click buttons to apply common transformations instantly
  - All inputs support direct keyboard entry with Tab navigation

- **Visualization**:
  - Red/Green/Blue axes: Original coordinate system
  - Orange/Lime/Skyblue axes: Transformed coordinate system
  - Yellow/Magenta arrows: Input vectors A and B
  - Orange arrow: Vector sum (A + B)
  - Sky blue arrow: Cross product (A × B)

## Project Structure

```
.
├── Makefile              # Build configuration
├── README.md             # This file
├── bin/                  # Compiled binaries
├── include/              # External library headers
└── src/
    ├── main.cpp          # Application entry point
    ├── include/          # Project headers
    │   ├── math/         # Math library headers
    │   ├── rendering/    # Rendering system headers
    │   └── ui/           # UI component headers
    ├── math/             # Vector and Matrix implementations
    ├── rendering/        # Grid, vector, and transform rendering
    └── ui/               # Input panel and controls
```

## Extending the Project

### Adding New Matrix Presets

Edit `src/ui/InputPanel.cpp` in the `drawPresetButtons()` method to add new transformation presets.

### Supporting Complex Numbers

1. Create `src/math/Complex.hpp` and `.cpp`
2. Add complex plane rendering in `src/rendering/`
3. Extend UI to toggle complex mode

### Function Plotting

1. Create `src/math/Function.hpp` for function representation
2. Add `src/rendering/FunctionPlotter.cpp` for mesh generation
3. Integrate with main application loop

### Time Animations

Use Raylib's `GetTime()` function to create time-dependent transformations:
```cpp
float t = GetTime();
Matrix4D anim = Matrix4D::rotationY(t);
transformViz.setTransform4D(anim);
```

## Architecture

The project uses a modular architecture:

- **Math Library** (`src/math/`): Pure C++ math types (Vector3D, Matrix3D, Matrix4D)
- **Rendering** (`src/rendering/`): Raylib-based visualization components
- **UI** (`src/ui/`): raygui-based interface for user input
- **Main Loop**: Coordinates updates and rendering

This separation makes it easy to:
- Unit test math operations independently
- Swap rendering backends if needed
- Add new visualization modes

## Makefile Targets

- `make` or `make run`: Build and run the application
- `make build`: Build the executable only
- `make clean`: Remove all build artifacts
- `make help`: Display available targets
- `make list`: Show source files being compiled

## Future Enhancements

- [ ] Eigenvalue/eigenvector visualization
- [ ] Complex number plane (Argand diagram)
- [ ] Parametric curve plotting
- [ ] Surface plotting for multivariate functions
- [ ] Numerical integration visualization
- [ ] Animation timeline controls
- [ ] Export transformations as code
- [ ] Import matrices from files
- [ ] Determinant and trace display
- [ ] Matrix decompositions (SVD, QR, etc.)

## License

This project is provided as-is for educational purposes.

## Contributing

Feel free to extend this project! Some ideas:
- Add more transformation presets
- Implement additional vector operations
- Create shader-based effects
- Add VR support with Raylib VR
