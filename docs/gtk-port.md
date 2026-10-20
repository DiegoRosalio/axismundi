# The GTK4 port (branch `feature/gtkui`)

The progress file of the port: the plan, what is done, what's next. Updated at the end of every run, so work can
resume from here after a break.

## Goal

A GTK4 + libadwaita front end (gtkmm-4.0 for the C++ side, libadwaita's C API for its widgets), so axismundi looks
and behaves like a GNOME app.

- **The raylib front end keeps working** until the GTK one does everything; both are built from the same core.
- **The 3D view is still drawn with raylib's GL layer** (rlgl) inside a `Gtk::GLArea`.
- **A later switch away from raylib stays possible:** drawing goes through a small interface (`gfx::Canvas3D`), so a
  plain-OpenGL implementation can replace raylib without touching the drawing code.

### Dependencies

- **Arch:** `sudo pacman -S --needed gtkmm-4.0 libadwaita pkgconf`.
- **Alpine (the dev box):** `gtk4.0-dev gtkmm4-dev libadwaita-dev libepoxy-dev adwaita-fonts font-dejavu`.

### Layout

- **Shared core:** `src/math`, `src/scene`, `src/export`, `src/text`, `src/animation`, `src/view`, `src/rendering`.
- **Only in the raylib app:** `src/ui`, `src/app`, `src/main.cpp`.
- **Only in the GTK app:** `src/gtk` (built by `make gtk` → `bin/axismundi-gtk`).

## Runs

Each run ends with pushed commits that build without warnings and pass the tests. Runs marked ⛔ wait for the user.

| Run | Content | State |
|-----|---------|-------|
| 0 | dependencies installed | done |
| 1 | skeleton (`make gtk`, Adw window, screenshot mode) + spike: the scene drawn by rlgl in a GLArea, orbit/pan/zoom (⛔ if rlgl can't run in GTK) | in progress |
| 2 | `gfx::Canvas3D` + RaylibCanvas; labels as a list; the raylib tour pixel-identical | |
| 3 | `Session` + `AppApi` shared by both front ends; edu/ and the tour on AppApi | |
| 4 | GTK main window: split view, objects list, Add menu, selection, picking, undo, toasts, files | |
| 5 | GTK inspectors I: framework + basic types | |
| 6 | GTK inspectors II: the remaining types (parallel agents) | |
| 7 | GTK panels: Analysis, Time, Settings, Export/PDF, Gallery, Reference, Steps, Breakdown, Help | |
| 8 | tutorial with popovers, GTK tour, input test, lesson walk, docs/learn chapter | |
| 9 | parity checklist; the user tries it on Arch + GNOME (⛔) | |
| 10 | GTK becomes `axismundi`; the raylib UI is removed | |
| later | own OpenGL renderer replaces raylib (⛔ separate decision) | |

## Log

(Newest last: per run, the commits, what was checked, open problems.)
