# 2. The big picture

axismundi is about 26,000 lines of C++. That is a lot to read, but it is organized in **layers**, and each layer
only uses the ones below it.

## The layers

```
  app/        App: the window, the main loop, panels, files          (uses everything)
  ui/  edu/   panels, widgets, inspector, tutorial, gallery           (uses view, scene, math)
  view/       SceneView: draws a space's objects; the per-type table  (uses rendering, scene, math)
  rendering/  raylib helpers: camera, grid, arrows, plot meshes       (uses math)
  ─────────────────────────── raylib (graphics) above this line, none below ───────────────────────────
  export/     LaTeX figures                                           (uses scene, math)
  scene/      objects, spaces, dependencies, undo, files, time        (uses math)
  math/       vectors, matrices, the formula engine, calculus         (uses only the C++ standard library)
```

The line in the middle is the most important rule of the project: **nothing below it includes raylib**. So the
math, the formula engine, the scene model and the LaTeX export can be tested by a plain program without a window
or a graphics card (`make test`, chapter 9). If you ever see `#include <raylib.h>` under `src/math/` or
`src/scene/`, that is a bug.

## The folders

| Folder | What lives there |
|--------|-----------------|
| `src/main.cpp` | Reads the command line, creates the App, runs it |
| `src/app/` | `App` (one class in four files) and `Tour` (the scripted screenshot tour) |
| `src/math/` | `Vector3D`, `Matrix3D/4D`, decompositions, complex numbers, quaternions, derivatives, integrals, Fourier, curve geometry, grid math, the orbit camera math |
| `src/math/expr/` | The formula engine: parsing, values, functions, the in-app formula reference |
| `src/scene/` | `SceneObject`, `Space`, `Scene`, and every object type (`Types.cpp`, `TypesAdvanced.cpp`) |
| `src/export/` | The LaTeX export and PDF compilation |
| `src/text/` | Text wrapping and label placement (no graphics) |
| `src/animation/` | Playback (repeat modes, easing curves) |
| `src/rendering/` | Camera, grid (and its shader), arrows, plot and field meshes |
| `src/view/` | `SceneView`, view styles, the per-type drawing table (`view/objects/`), axis numbers |
| `src/ui/` | Every panel, the inspector, the add menu, file dialogs; `ui/core/` is the small UI toolkit |
| `src/edu/` | Tutorial lessons and gallery examples, written against the App API |
| `src/include/` | All headers, in the same folder structure |
| `tests/` | Unit tests |
| `tools/dev/` | Build, test, screenshot and input-test scripts |

## How the program starts

`src/main.cpp` is short: it reads options like `--size 1280x720` into an `AppOptions` struct, then:

```cpp
    App app(options);
    app.run();
```

1. `App app(options);` runs App's **constructor** (`App::App` in `src/app/App.cpp`): it opens the window
   (raylib's `InitWindow`), loads fonts and preferences, and sets up the panels. The scene starts empty.
2. `app.run();` is the **main loop**: it runs until the window closes (chapter 3).
3. When `main` returns, `app` goes out of scope and its destructor frees everything (RAII, chapter 1).

## Who owns what

`App` (in `src/include/app/App.hpp`) holds almost everything as members, so their lifetime is the program's:

```cpp
    scene::Scene m_scene;
    view::SceneView m_sceneView;
    CameraController m_camera;
```

- `m_scene` is the **model**: the two spaces (3D and the complex plane), their objects, the selection, the
  clock, the undo history.
- `m_sceneView` **draws** the model; it keeps GPU caches (plot meshes) but no math state.
- `m_camera` is where you look from.
- The panels (`m_scenePanel`, `m_viewPanel`, ...) **edit** the model through its public functions.

This split (model / view / controls) means the model never knows how it is drawn: the same `Scene` is drawn by
`SceneView`, written to a file by `Scene::save`, turned into LaTeX by `latex::exportLatex`, and checked by the
tests.

## The App API

Tutorial lessons, gallery examples and the screenshot tour drive the app through a small public API on `App`:
`addObject`, `select`, `setCameraView`, `resetCamera`, `setMode`, ... A gallery example
(`src/edu/Examples.cpp`) is just a function that calls these:

```cpp
                        app.addObject(AppMode::Space3D, "frame", "P", {{"u0", "mod(t, 4pi)"}});
```

Reading a few examples is a quick way to learn what the program can do.

## Try it

1. Run `grep -rn "raylib.h" src/math src/scene src/export` from the repository root. It should print nothing.
   That is the layer rule, kept.
2. Open `src/edu/Examples.cpp`, copy one example, change its title and one formula, build, and find your
   example in the gallery (E key).
3. In `src/main.cpp`, add an option `--demo` that sets a new `bool demo` in `AppOptions`. (You'll use it in the
   next chapter's exercise.)
