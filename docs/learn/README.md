# Learning the axismundi code

This guide walks through the codebase for someone who is **new to C++**. Every idea is shown with real code
from this project, so you learn the language and the program at the same time. Each chapter ends with
**Try it** exercises: small changes you can make, build and see.

## Reading order

| # | Chapter | What you learn |
|---|---------|----------------|
| 1 | [C++ through this code](01-cpp-through-this-code.md) | The language features the project uses, one by one |
| 2 | [The big picture](02-big-picture.md) | The layers, the folders, and how the program starts |
| 3 | [One frame](03-one-frame.md) | What happens 60 times a second: input, update, draw |
| 4 | [Formulas](04-formulas.md) | How `2a + b` becomes a value: the expression engine |
| 5 | [The scene model](05-scene-model.md) | Objects, fields, dependencies, undo, files, time |
| 6 | [Drawing](06-drawing.md) | raylib, the 3D view, the per-type table, the grid shader |
| 7 | [The UI toolkit](07-ui-toolkit.md) | Panels, widgets, the inspector, the text editor |
| 8 | [Walkthrough: a new object type](08-walkthrough-new-object.md) | Every place a type touches, using the tangent line |
| 9 | [Tests and tools](09-tests-and-tools.md) | Unit tests, the screenshot tour, the input test |
| 10 | [The GTK front end](10-gtk-front-end.md) | The GTK 4 app in progress: retained widgets, signals, the inspector, the tutorial |
| – | [Glossary](glossary.md) | Words used in the code and the math |

Chapters 1–3 are the foundation; after them you can read in any order. Chapter 8 ties everything together.

## Build and run

```bash
make build        # compiles bin/linalg_viz (needs raylib 5.5 installed)
make run          # builds and starts the app
make test         # builds and runs the unit tests (no window needed)
```

On the development machine without a display, the scripts in `tools/dev/` do the same with a raylib built
from source (see [chapter 9](09-tests-and-tools.md)).

## How to read code here

- File paths are written like `src/app/App.cpp`; open them next to the guide.
- When the guide names a function, search for it in the file (for example `App::run`), so the guide stays
  right even when lines move.
- `src/include/<area>/X.hpp` is the **header** (what a piece of code offers) and `src/<area>/X.cpp` the
  **source** (how it does it). Chapter 1 explains why C++ splits them.
- [`docs/architecture.md`](../architecture.md) is a one-page summary; `CLAUDE.md` at the top of the repository
  is the developers' detailed map.
