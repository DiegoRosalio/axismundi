# 10. The GTK front end

axismundi is moving from its own raylib-drawn UI to **GTK 4** with **libadwaita**, the toolkit of GNOME apps. During
the move both apps exist: `make build` gives the raylib app (`bin/linalg_viz`) and `make gtk` the GTK one
(`bin/axismundi-gtk`). The plan and progress are in `docs/gtk-port.md`. This chapter shows how the GTK app is put
together and what is different from the raylib UI of chapters 3 and 7.

## What both apps share

Everything below the UI is the same code, compiled once for both binaries:

- the math, the formula engine, the scene model and the LaTeX export (chapters 4 and 5);
- `Session` (`src/include/app/Session.hpp`): the scene, the mode, the camera, the view settings and the drawing
  steps of a frame, without any window or panel;
- `AppApi` (`src/include/app/AppApi.hpp`): what the gallery examples and the tutorial lessons may do to an app.
  Scene work goes to the Session; the few UI things (open a panel, show a toast) are virtual functions, and each
  app implements them: `App` in the raylib app, `WindowApi` in the GTK one;
- the 3D drawing (chapter 6). The GTK app draws the view with raylib's GL layer too, inside a GTK widget (below).

So a lesson's check like "is b equal to (1, 1, 0)?" or an example's setup runs unchanged in both apps.

## Two ways to build a UI

The raylib UI is **immediate mode** (chapter 3): every frame, `drawUi` calls a function per widget, and the call
both draws the widget and returns whether it was clicked. Nothing is kept between frames.

GTK is **retained mode**: you create widget objects once, put them into containers, and GTK keeps them, draws
them when needed and calls your code back when something happens (a **signal**: "clicked", "changed",
"notify::active"). Your code then changes the scene, and something must update the widgets that show it. In this
app that is `refresh()`: the window calls it after each change and every 150 ms (values move with the clock).

| | raylib UI | GTK |
|---|---|---|
| A button | `if (ui::button(rect, "Delete")) ...` every frame | `gtk_button_new_with_label("Delete")` once, plus a "clicked" callback |
| Layout | rectangles from `ui::Column` | containers (boxes, grids, lists) that measure their children |
| Updating | free: everything is redrawn from the scene each frame | `refresh()` sets the new values into existing widgets |
| Who owns a widget | nobody: it's a function call | its parent container |

## gtkmm and the C API

GTK is written in C. **gtkmm** wraps it in C++ classes (`Gtk::Button`, `Gtk::Box`), and the app uses it for its own
widgets: `ObjectsList`, `Inspector`, `SceneArea`, `LessonCard` are classes derived from gtkmm widgets. libadwaita
has no C++ wrapper, so its widgets (`AdwPreferencesGroup`, `AdwToggleGroup`, `AdwDialog`...) are made through
the C functions, and `gobj()` gives the C pointer of a gtkmm widget when the two meet.

Widgets belong to their parent. `Gtk::make_managed` creates a gtkmm widget that the container will delete:

```cpp
    m_area = Gtk::make_managed<SceneArea>(m_session);
```

Data that a C callback needs is often tied to a widget so that it is freed with it. `g_object_set_data_full`
stores a pointer on the object together with a function that deletes it (`src/gtk/FormulaDialog.cpp`):

```cpp
    g_object_set_data_full(G_OBJECT(dialog), "axismundi-state", st, [](gpointer p) { delete static_cast<FormulaState*>(p); });
```

## Signals and lambdas

A C signal callback is a plain function pointer. A lambda **without captures** converts to one; the unary `+`
asks for that conversion explicitly:

```cpp
    g_signal_connect_swapped(st->applyButton, "clicked", G_CALLBACK(+[](FormulaState* s) { s->commit(); }), st);
```

"Swapped" means GTK passes the last argument (`st`) first, so the lambda gets the state directly. When a callback
needs captures, `onSignal` in `src/gtk/Inspector.cpp` keeps a `std::function` alive on the heap and deletes it when
the signal is disconnected:

```cpp
void onSignal(gpointer object, const char* signal, std::function<void()> callback) {
```

A trap: `G_CALLBACK(...)` is a macro, and the preprocessor splits macro arguments at commas it doesn't see inside
parentheses. A lambda containing `std::pair<int, int>` or `{a, b}` breaks into several "arguments" and gives a
confusing error. Define the lambda first, then pass its name.

## The inspector: describing rows

The raylib inspector has one function per object type that draws its rows every frame (chapter 7). The GTK
inspector also has one function per type (`src/gtk/InspectorsBasic.cpp`, `InspectorsCalculus.cpp`,
`InspectorsMore.cpp`), but it runs only when the rows must be built. It *describes* the rows to an
`InspectorBuilder`, which makes the widgets and remembers how to update each one. From the Taylor polynomial's
inspector:

```cpp
    b.formula("a");
    b.anchor("taylor.a");
    b.formula("N");
    b.anchor("taylor.N");
```

`formula` makes a text row bound to the field, with its problem shown underneath; `anchor` names the row so a lesson
can point at it (below). Other row kinds: `cells` (fields side by side), `toggle`, `choice`, `combo`, `slider`,
`value`, `button`, `buttonGrid`, `widget` (a chart drawn by hand).

**When to rebuild.** Typing in a field must not rebuild the rows (the field would lose the keyboard focus). So
`Inspector::refresh` compares a small text, `layoutKey()`, made of what decides which rows exist: the object, its
type, its active fields and a few properties per type (a mode, a matrix size). Only when that text changes are the
rows rebuilt; otherwise each row's update function sets the new values.

**Ids, not pointers.** Undo replaces every object with a fresh copy (chapter 5), so a row must never keep a pointer
to its object. Rows keep the object's id and find it again each time (`Inspector::current`).

**Editing text.** `EditBinding` (`src/gtk/EditBinding.cpp`) attaches to any GTK text field: Enter or leaving the
field commits (one undo step), Esc puts the old text back, and while you type the field turns red if the formula
doesn't parse.

## The 3D view inside GTK

`SceneArea` is a `Gtk::GLArea`: GTK makes an OpenGL context for it and calls `on_render` whenever it needs a new
picture. `SceneRenderer` (`src/gtk/SceneRenderer.cpp`) starts raylib's GL layer, rlgl, inside that context instead
of raylib's own window, then draws with the Session exactly like the raylib app. The GLArea's framebuffer is bound
by GTK before `on_render`:

```cpp
    GLint target = 0;  // the GLArea's framebuffer, bound by GTK before this call
```

Mouse input comes from GTK gesture objects (drag, scroll, click) and goes to the same orbit camera. Because the
drawing code only talks to `gfx::Canvas3D` (chapter 6), a later version can replace rlgl with plain OpenGL without
touching it.

## The tutorial in GTK

The lessons (`src/edu/Tutorial.cpp`) are data: each step has a setup, a text, a task with a check and the name of the
control it is about. Where the user is and whether the task is done is kept by `TutorialRunner`, shared by both apps.
Its `update` runs the setup of a new step, then the check:

```cpp
    if (s.check && !m_done && s.check(app, m_context)) m_done = true;
```

The raylib app draws the runner's state in a floating panel and highlights a rectangle. The GTK app shows it in
`LessonCard` (`src/gtk/LessonCard.cpp`), a card over the 3D view, and finds the control as a widget:
`MainWindow::anchorWidget("objects.add")` returns the + button, and inspector rows come from the builder's
`anchor` names. The card outlines that widget with a CSS class and points at it with a popover until the task is
done.

## Tools for the GTK app

The same checks exist as for the raylib app (chapter 9):

- `tools/dev/build-gtk.sh` builds it;
- `tools/dev/tour-gtk.sh 1600x900` runs its screenshot tour (`--tour`) into `~/.cache/axismundi-dev/shots-gtk/`;
- `tools/dev/input-test-gtk.sh` clicks and types into it with xdotool;
- `tools/dev/lesson-walk-gtk.sh` enters every lesson step, does its task the scripted way
  (`src/edu/TutorialSolutions.cpp`) and checks that the lesson card sees it done.

## Try it

1. In `src/gtk/InspectorsBasic.cpp`, give the complex number inspector a `b.value(...)` row that shows the number's
   length, following the Determinant row of the matrix inspector. Build with `make gtk` and select z1 in the complex
   plane.
2. Find where the main menu is built in `src/gtk/MainWindow.cpp` (`Gio::Menu`) and follow the "win.lessons" action
   from the menu item to the code that opens the lesson list.
3. Run `tools/dev/tour-gtk.sh 1280x800 --theme light` and compare a screenshot with the raylib app's one of the
   same scene.
