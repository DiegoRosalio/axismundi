# Quick Start Guide

## Prerequisites

Install Raylib on Arch Linux:
```bash
sudo pacman -S raylib
```

## Build

```bash
make build
```

## Run

```bash
make run
```

Or directly:
```bash
./bin/linalg_viz
```

## First Steps

When you run the application, you'll see:

1. **3D Visualization** (main area):
   - Red/Green/Blue coordinate axes
   - Grid on the XZ plane
   - Original coordinate system (semi-transparent)
   - Transformed coordinate system (bright colors)
   - Vector arrows

2. **Control Panel** (left side):
   - Matrix display showing current transformation
   - Preset buttons for common transformations
   - Vector information

## Try These Presets

Click the preset buttons to see different transformations:

- **Identity**: Reset to no transformation
- **Rotate 45**: Rotate the coordinate system 45 degrees
- **Scale 2x**: Scale everything by 2x

## Camera Controls

- **Right-click + Drag**: Orbit camera around the origin
- **Mouse Wheel**: Zoom in/out

## Understanding the Visualization

### Axes Colors

Original coordinate system (faded):
- Red: X-axis
- Green: Y-axis
- Blue: Z-axis

Transformed coordinate system (bright):
- Orange: Transformed X-axis
- Lime: Transformed Y-axis
- Sky Blue: Transformed Z-axis

### Vectors

- Yellow: Vector A
- Magenta: Vector B
- Orange: Sum (A + B)
- Sky Blue: Cross product (A × B)

## Extending the Application

The code is structured to be easily extensible. See README.md for architecture details and extension points.

## Next Steps

- Modify matrix values by editing `src/ui/InputPanel.cpp`
- Add your own transformation presets
- Implement keyboard shortcuts for quick transformations
- Add text input for interactive matrix editing (requires raygui or custom implementation)
