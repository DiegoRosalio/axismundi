# 9. Tests and tools

Two kinds of checking keep this project working: **unit tests** for everything without graphics, and **scripts**
that run the real app under a virtual display and take screenshots.

## Unit tests (make test)

`make test` builds `bin/run_tests` from the code below the raylib line (chapter 2) plus `tests/`, and runs it:

```
Vector3D     ok
...
Grid math    ok
4287 checks, 0 failures
```

The test files:

| File | Covers |
|------|--------|
| `tests/main.cpp` | The runner and the list of tests |
| `tests/TestSupport.hpp` | `CHECK`, `CHECK_NEAR`, shared helpers |
| `tests/test_linear.cpp` | Vectors, matrices, decompositions, complex numbers, quaternions, explanations |
| `tests/test_expr.cpp` | The formula engine and the formula reference |
| `tests/test_calculus.cpp` | Fields, integrals, curves, derivatives, Fourier, grid math, camera math |
| `tests/test_scene.cpp` | Objects, files, undo, time, every object type |
| `tests/test_export.cpp` | LaTeX export, PDF compilation |
| `tests/test_text.cpp` | Text wrapping, label layout |

A test is a function full of checks:

```cpp
    CHECK_NEAR(autoLevel(120, 12).spacing, 0.1, 1e-12);  // zoomed in 10x: 0.1
```

`CHECK_NEAR(a, b, tol)` passes when a and b differ by at most tol. Floating-point math is rarely exact, so numeric
checks use a tolerance. When a check fails, it prints the file, line and expression, and the other checks still run.

To add a test: write `void testSomething() { ... }` in the right topic file, declare it in `tests/main.cpp`, and add
it to the list there.

## The development scripts (tools/dev/)

The development machine has no screen, so the scripts run the app inside **Xvfb**, a virtual display.

| Script | What it does | What it catches |
|--------|-------------|-----------------|
| `build.sh` | Builds the app with raylib 5.5 built from source | Compile and link errors, warnings |
| `check.sh` | Compiles every source file with the warning flags; prints ALL CLEAN | Warnings anywhere in `src/` |
| `tour.sh 1600x900` | Runs the screenshot tour: about 100 scripted steps, one PNG each | Anything that *looks* wrong |
| `input-test.sh` | Drives the real app with simulated mouse and keyboard (xdotool) | Clicks, drags, typing, dialogs, picking |
| `lesson-walk.sh` | Solves every tutorial step by script and checks it | A lesson whose task can't be done |
| `latex.sh` | Compiles exported LaTeX figures with pdflatex | Export output that doesn't compile |

### The screenshot tour

`linalg_viz --tour DIR` runs `src/app/Tour.cpp`: a list of steps, each changing something (open a panel, load an
example, change the theme) and saving a screenshot. Looking through the PNGs is how UI changes are reviewed. During
the tour the clock advances exactly 1/60 s per frame (`ui::setFixedStep`), so **two tours give identical pictures**:
comparing old and new screenshots pixel by pixel shows exactly what a change affected. The refactoring of this
codebase was checked that way.

### The input test

`tools/dev/input-test.sh` moves a simulated mouse and presses keys (`tools/dev/input-lib.sh` has helpers like
`click X Y`, `drag`, `wheel`, `key`), then asks the app for a screenshot after each step. It catches things the tour
can't: a click that doesn't register, a drag that starts on the wrong thing, a field that loses typed text.

## Reading a failure

- **Compiler error**: the first error is the real one; later ones are often consequences. The message names the file
  and line.
- **Linker error** ("undefined reference to ..."): something is declared but never defined, often a function in a
  `.hpp` without a body in any `.cpp`, or a `.cpp` file that isn't compiled.
- **Test failure**: `FAIL tests/test_calculus.cpp:391: formatTick(-1e-12, 0.5) == "0"` gives you the file, line and
  the exact expression. Print the actual value (`std::printf`) to see what it was.

## Try it

1. Break something on purpose: in `src/math/Vector3D.cpp` make `dot` return `0`. Run `make test` and count how many
   tests fail; that shows how much depends on one function. Undo it.
2. Run `tools/dev/tour.sh 800x600` and look at a few PNGs in `~/.cache/axismundi-dev/shots/800x600/`.
3. Add a test for the midpoint object from chapter 8, following the steps above.
