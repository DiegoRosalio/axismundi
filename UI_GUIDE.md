# UI Guide - Linear Algebra Visualizer

## Overview

The application now features a fully responsive, professional UI built with raygui.

## Panel Layout

```
┌─────────────────────────────────────────────────┐
│  Matrix Transformation                          │
│  ┌──────────┐                                   │
│  │ 3x3 ⟷ 4x4│  Toggle Button                    │
│  └──────────┘                                   │
│                                                  │
│  3x3 Matrix  (or 4x4 when toggled)             │
│  ┌────┬────┬────┐                               │
│  │1.00│0.00│0.00│ ← Click any cell to edit      │
│  ├────┼────┼────┤                               │
│  │0.00│1.00│0.00│                               │
│  ├────┼────┼────┤                               │
│  │0.00│0.00│1.00│                               │
│  └────┴────┴────┘                               │
└─────────────────────────────────────────────────┘
                   ↕ (Resizes with window)
┌─────────────────────────────────────────────────┐
│  Vectors                                        │
│                                                  │
│  Vector A                                       │
│  X: [1.00]  Y: [0.00]  Z: [0.00]               │
│                                                  │
│  Vector B                                       │
│  X: [0.00]  Y: [1.00]  Z: [0.00]               │
└─────────────────────────────────────────────────┘
                   ↕ (Resizes with window)
┌─────────────────────────────────────────────────┐
│  Preset Transformations                         │
│                                                  │
│  [Identity Matrix]     [Shear X by 0.5]        │
│  [Rotate 45° (Z)]      [Reflect X-axis]        │
│  [Scale 2x]                                     │
└─────────────────────────────────────────────────┘
```

## Responsive Behavior

### Window Sizing

**Large Window (> 900px height)**
- All panels visible without scrolling
- Panels expand proportionally
- Matrix panel: 35% of available height
- Vector panel: 30% of available height
- Preset panel: 30% of available height

**Medium Window (600-900px height)**
- Panels reach minimum size
- May require scrolling
- Scroll with mouse wheel over panel area
- Scrollbar appears on right edge

**Small Window (< 600px height)**
- Minimum window size enforced (800×600)
- Scrolling active
- All features remain accessible

### Panel Minimum Sizes

- **Matrix Panel**: 250px (fits 3×3 or 4×4 grid comfortably)
- **Vector Panel**: 230px (both vectors fully visible)
- **Preset Panel**: 150px (all buttons accessible)

## Interaction Guide

### Editing Matrix Values

1. **Click to Select**
   - Click any cell in the matrix grid
   - Cell highlights to show it's active
   - Previous value is selected

2. **Type New Value**
   - Enter any floating-point number
   - Supports:
     - Integers: `1`, `2`, `-5`
     - Decimals: `0.5`, `3.14`, `-2.718`
     - Scientific: `1e-3`, `2.5e2`

3. **Confirm Changes**
   - Press **Enter**: Apply and deactivate
   - Press **Tab**: Apply and move to next cell
   - Click elsewhere: Apply and select new cell
   - Press **Escape**: Cancel and deactivate

4. **See Results**
   - Transformation updates immediately
   - 3D visualization reflects new matrix
   - Basis vectors transform in real-time

### Editing Vector Values

Same process as matrix values:
1. Click X, Y, or Z field
2. Type new value
3. Press Enter or Tab
4. Vector updates in 3D instantly

### Using Preset Buttons

**Click any button to:**
1. Populate matrix with preset values
2. See transformation apply immediately
3. Observe effect on coordinate system
4. Experiment by editing values further

**Available Presets:**

| Button | Effect | Matrix Values |
|--------|--------|---------------|
| Identity Matrix | Reset to no transformation | Diagonal 1's, rest 0's |
| Rotate 45° (Z) | Rotate around Z-axis | Rotation matrix (cos/sin) |
| Scale 2x | Uniform 2x scaling | Diagonal 2's |
| Shear X by 0.5 | Horizontal shear | [1, 0.5, 0] top row |
| Reflect X-axis | Mirror across X | Diagonal [-1, 1, 1] |

### Scrolling Panels

**When to Scroll:**
- Window height < total panel height
- Scrollbar appears automatically

**How to Scroll:**
1. Move mouse over panel area (left side)
2. Use **mouse wheel**:
   - Scroll up: Move panels down (see higher content)
   - Scroll down: Move panels up (see lower content)
3. Watch scrollbar indicator on right edge

**Scroll Tips:**
- Scroll speed: 20 pixels per tick
- Can't over-scroll (clamped)
- Camera controls still work in 3D area
- Scroll resets when window is resized larger

## Keyboard Shortcuts

### Text Input Navigation
- **Tab**: Move to next input field
- **Shift+Tab**: Move to previous field
- **Enter**: Apply changes and deactivate
- **Escape**: Cancel editing

### Future Shortcuts (Planned)
- **I**: Identity matrix
- **R**: Rotate preset
- **S**: Scale preset
- **Space**: Toggle animation
- **H**: Help overlay

## Tips & Tricks

### Efficient Editing
1. **Use Tab** to quickly move through matrix cells
2. **Copy values** by typing them in multiple cells
3. **Preset then tweak** for faster custom transformations
4. **Watch the 3D view** while editing for instant feedback

### Window Management
1. **Start large** (1600×900) for best experience
2. **Resize vertically** to see scroll behavior
3. **Minimum 800×600** enforced for usability
4. **Fullscreen** works great (F11 in most systems)

### Understanding Transformations
1. **Start with Identity** to reset
2. **Apply one preset** at a time
3. **Edit values gradually** to see effects
4. **Watch basis vectors** (colored axes) transform

### Troubleshooting

**Can't click matrix cells?**
- Ensure you're clicking inside the cell boundaries
- Try toggling 3×3 ⟷ 4×4 to refresh

**Panels cut off?**
- Resize window larger, or
- Use mouse wheel to scroll over panel area

**Values not updating?**
- Press Enter or click elsewhere after typing
- Check for valid number format

**Camera won't move?**
- Ensure mouse is in 3D area (right of panels)
- Right-click + drag for orbit
- Mouse wheel when not over panels for zoom

## Accessibility

### Visual
- Large text (16pt default)
- Clear panel borders
- High contrast colors
- Visual feedback on active cells

### Input
- Full keyboard navigation
- Mouse-free editing possible
- Clear focus indicators
- Undo/redo (planned)

### Layout
- Logical top-to-bottom flow
- Grouped related controls
- Consistent spacing
- Responsive to different screen sizes

---

## Quick Reference

| Action | Method |
|--------|--------|
| Edit matrix | Click cell, type, Enter |
| Edit vector | Click field, type, Enter |
| Toggle 3×3/4×4 | Click toggle button |
| Apply preset | Click preset button |
| Scroll panels | Mouse wheel over panels |
| Orbit camera | Right-click + drag (3D area) |
| Zoom camera | Mouse wheel (3D area) |

**Remember**: The UI is fully responsive—resize the window to see it adapt!
