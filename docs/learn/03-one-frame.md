# 3. One frame

An interactive program is a loop: read what the user did, update the state, draw the picture, repeat. Each pass
is a **frame**; at 60 frames a second each one has about 16 milliseconds.

## The main loop

`App::run` in `src/app/App.cpp` (slightly shortened):

```cpp
    while (!WindowShouldClose()) {
        float deltaTime = ui::frameStep(GetFrameTime());
        ...
        ui::beginFrame();
        m_panels.beginFrame();
        handleInput(deltaTime);
        update(deltaTime);

        BeginDrawing();
            ClearBackground(view::activeStyle().background);
            drawScene();
            drawSceneLabels();
            if (m_screenshotRequest != 2) drawUi(deltaTime);
            ...
        EndDrawing();
    }
```

- `deltaTime` is how long the last frame took, in seconds. Anything that moves with time multiplies by it, so
  the speed doesn't depend on how fast the computer is. (`ui::frameStep` replaces it with exactly 1/60 s during
  the screenshot tour, so screenshots repeat exactly; see chapter 9.)
- `BeginDrawing` ... `EndDrawing` is raylib's frame: everything drawn in between appears at once when
  `EndDrawing` shows the finished picture.

## 1. Input: `App::handleInput` (src/app/AppInput.cpp)

- **Escape** steps back one level: cancel an edit, close a popup, leave camera mode. It never quits.
- **Shortcuts** (`App::handleShortcuts`): keys like G (grid), C (reset camera), N (add menu).
- **The 3D view's mouse** (`App::handleViewMouse`): a press on the empty view starts a drag. Moving more than
  4 pixels makes it an orbit or pan (`CameraController::applyOrbit`); releasing before that is a click, which
  **picks** the object under the cursor (`App::pickAt`).

Picking asks every object type for screen segments: "a click near this arrow selects me". Each type's segments
come from its entry in the per-type table (chapter 6).

## 2. Update: `App::update` (src/app/App.cpp)

```cpp
    m_scene.advance(deltaTime);
    m_scene.evaluate();
```

- `advance` moves the scene clock `t` forward (if playing).
- `evaluate` recomputes every object's value in dependency order: if `w = 2a + b`, then `a` and `b` are
  computed before `w` (chapter 5).
- Then the panels that show results catch up: the Analysis panel, the breakdown, the tutorial's task checks.

## 3. Draw the 3D scene: `App::drawScene` (src/app/AppDraw.cpp)

```cpp
void App::drawScene() {
    BeginMode3D(m_camera.camera());
```

Inside `BeginMode3D`, coordinates are 3D world positions; raylib projects them with the camera. The grid comes
first (`App::drawGrid`), then `SceneView::draw3D` draws the axes and every object (chapter 6).

`drawSceneLabels` then draws 2D text on top: the axis numbers and the object labels, placed so they don't
overlap.

## 4. Draw the UI: `App::drawUi` (src/app/AppDraw.cpp)

The top bar, the side bar (objects list and inspector), floating panels (Analysis, Time, Tutorial...), popovers
and the status line, all in 2D on top of the scene.

## Immediate-mode UI

This project's UI is **immediate mode**: there are no button objects that live between frames. Each frame,
the code that draws a button also asks whether it was clicked:

```cpp
    if (ui::button(reset, "Reset camera", ui::ButtonKind::Secondary, true, ui::Icon::Reset, "C")) {
        m_resetCamera = true;
    }
```

`ui::button` draws the button and returns `true` in the frame it is clicked. The layout is simply the order of
the calls, and hiding a control means not calling it. The advantages: the UI always shows the current state
(there's nothing to keep in sync), and a panel is just a function. The cost: everything is redrawn every frame,
which is cheap for a few hundred rectangles and texts. Chapter 7 shows the toolkit (`src/ui/core/`) in detail.

## Where time goes

Most frames are cheap: evaluating formulas, drawing a few arrows. The expensive parts are cached and rebuilt only
when something changes:

- **plot meshes**: a surface with 160,000 vertices is turned into GPU buffers once, then drawn every frame
  (`SceneView::updatePlots` checks `changedAt()` to see if the object changed);
- **cached object values**: integrals, Fourier series and epicycles recompute only when their inputs or the time
  change (`cachesValue()`, chapter 5).

The status line shows the frames per second (FPS).

## Try it

1. In `App::update`, the scene is evaluated every frame. Add `std::printf("t = %f\n", m_scene.timeline().time);`
   after `m_scene.evaluate();`, run the app from a terminal, press P, and watch the clock advance. Remove it
   again.
2. If you added `--demo` in chapter 2: App's constructor (`App::App` in `src/app/App.cpp`) empties the scene at
   startup with `m_scene.clearAll()`. Right after that, call `resetScene()` when `options.demo` is set, so
   `linalg_viz --demo` starts with the demo scene.
3. Find `ui::button(` calls in `src/ui/ViewPanel.cpp`. Change the text of one button and see it in the app.
