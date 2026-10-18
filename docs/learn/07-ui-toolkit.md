# 7. The UI toolkit

The panels, buttons and fields are drawn by a small toolkit in `src/ui/core/`, built on raylib's 2D drawing.
Chapter 3 introduced the idea (immediate mode: a widget is a function call that draws and returns whether it was
used). This chapter shows the pieces.

## Design units: ui::px

The layout is written for a 1600×900 window. Every size goes through `ui::px`, which scales it to the real window
(and the user's zoom from Settings). `src/include/ui/core/Ui.hpp`:

```cpp
inline float px(float design) { return design * scale(); }
```

So code says `ui::px(8.0f)` for "8 design pixels", never a raw pixel number, and the UI looks the same at 800×600
and 2560×1440. Text never uses raylib's `DrawText` directly: it goes through `ui::text` / `ui::textIn` with a
`FontRole` (Body, Small, Mono, Title...), so fonts scale too.

## Colors: the theme

`src/include/ui/core/Theme.hpp` defines the colors (panel backgrounds, text, accent, danger...) for the dark and
light themes. Widgets read `ui::theme()` when drawing, so switching the theme changes everything on the next frame.
The 3D view uses view styles instead (chapter 6).

## Layout: a Column of rows

Most panels stack rows top to bottom with `ui::Column` (`src/include/ui/core/Layout.hpp`):

```cpp
    // Next row of the given height (design units), followed by the gap
    Rectangle row(float heightDesign) {
        Rectangle r = {m_x, m_y, m_width, px(heightDesign)};
        m_y += r.height + m_gap;
        return r;
    }
```

`col.row(30)` returns the next 30-design-pixel rectangle and moves down. Helpers split a row into cells
(`ui::splitRow(row, {1, 1, 1})` gives three equal parts) or take a piece from one side (`ui::takeLeft`).

## Widgets

Widgets live in `src/ui/Widgets.cpp`. Each takes a rectangle and returns whether the user changed something:

```cpp
    ui::toggleRow(col.row(30), "Grid", m_settings.showGrid, "G");
```

draws a labeled switch bound to a `bool&` (a reference, chapter 1): flipping the switch writes straight into
`m_settings.showGrid`. Others: `ui::button`, `ui::segmented` (one of several choices), `ui::slider`.

After drawing a widget, two calls add help and a name:

```cpp
void tooltip(Rectangle r, const char* body, const char* title = nullptr);
```

```cpp
void anchor(const char* id, Rectangle r);
```

A tooltip appears when the mouse rests on the rectangle. An **anchor** gives a control a name ("view.gridspacing")
so a tutorial step can highlight it, and the tour can find it to hover or click.

## Who gets the mouse

Many things can be under the mouse at once: a panel over the 3D view, a popup over a panel. Widgets ask
`ui::hovered(r)` instead of checking the mouse position themselves:

```cpp
bool hovered(Rectangle r);        // mouse inside and interaction allowed
```

It's false while the GUI is locked (fly-camera mode, an overlay is open, the panel isn't on top) or when the
rectangle is scrolled out of view. Mouse buttons go through `ui::mousePressed`, not raylib's function, because a
quick touchpad tap can press and release within one frame, and the toolkit "latches" such clicks so they aren't lost
(`ui::installClickLatch`).

## Floating panels

Analysis, Time, Tutorial, Export... are floating panels (`src/ui/core/FloatingPanel.cpp`): they can be dragged,
collapsed and closed, remember their place, and scroll when the content is taller than the window. A panel's
content is just a function that draws into a rectangle and returns how tall it was. In `src/app/AppDraw.cpp`:

```cpp
            if (p->begin(m_analysisHeight, content)) {
                m_analysisHeight = m_infoPanel.drawContent(content);
                p->end();
            }
```

`begin` draws the frame and gives the content rectangle (and returns false if the panel is hidden); the content
function draws; `end` finishes scrolling and clipping. Next frame the panel uses the returned height.

## The inspector

The side bar's inspector (`src/ui/ScenePanel.cpp`) shows the selected object. The common part (name, color,
duplicate/delete) is the same for all objects; the type-specific part comes from the type table (chapter 1's
member-function pointers): `ScenePanel::typeUi` maps `"tangent"` to `inspectTangent`, which is in
`src/ui/ScenePanelInspectors.cpp`. An inspector is a list of rows:

```cpp
    col.setY(fieldRow(col.x(), col.y(), col.width(), tg, "x0", sc, 64.0f));
```

`fieldRow` draws a field's label and its formula editor and handles commit and errors. When a change is done, the
inspector calls `sc.commit()` (an undo snapshot, chapter 5).

## The text editor

Formula fields use the project's own editor (`src/ui/core/TextEditor.cpp`), because raylib's text box is
single-line. It wraps long formulas, moves the cursor by click and keys, selects, copies and pastes. It reports
events instead of acting on them:

```cpp
    Commit,     // Enter, or a click outside
    Expand,     // Ctrl+Enter: open the big editor
```

The inspector decides what Commit means (compile the formula, take an undo snapshot) and what Expand means (open
the big formula editor).

## Try it

1. In `src/ui/ViewPanel.cpp`, add a new `ui::toggleRow` for `m_settings.soloSelection` with the text
   "Only the selection". Build; the switch does the same as the O key.
2. Give it a tooltip with `ui::tooltip(rowRect, "...")` (store the row in a `Rectangle` first).
3. Open the app with `--size 800x600` and with `--size 2560x1440`: the same layout scales because everything goes
   through `ui::px`.
