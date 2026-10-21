#!/bin/sh
# Real X11 input on the GTK app under Xvfb (xdotool): the camera (orbit drag, pan, wheel), the space switch, the
# Add menu, the objects list (click, Ctrl+click), picking in the view, undo, saving and opening a scene through the
# file dialog, the inspector (typing, Esc, Delete inside a field, modes, rename, a slider), Delete in the list and
# the view. Saves a PNG per step to $DEV/input-gtk and checks that each step changed the picture.
DEV=${AXISMUNDI_DEV:-$HOME/.cache/axismundi-dev}
BIN=$DEV/build/axismundi-gtk OUT=$DEV/input-gtk
. "$(dirname "$0")/input-lib.sh"
input_start || exit 1
shot 00_start
# Camera and space switch (on the empty view)
drag 900 450 1050 500;         shot 01_orbit_drag
rdrag 900 450 800 400;         shot 02_pan_right_drag
wheel 900 450 4 3;             shot 03_wheel_zoom_in
click 1014 27;                 shot 04_complex_toggle
click 905 27;                  shot 05_back_to_3d
# Add a vector and a matrix through the + popover (sidebar header)
click 28 27;                   shot 06_add_menu
click 54 142;                  shot 07_vector_added
# (with something selected the popover lists the "From selection" templates first: deselect before adding)
click 1450 820; click 28 27; click 54 176
                               shot 08_matrix_added
# The list: select the vector, Ctrl+click the matrix too; the popover then offers templates (M·v)
click 160 78;                  shot 09_list_select
xdotool mousemove --window "$W" 160 122 keydown ctrl click 1 keyup ctrl; sleep 0.4
                               shot 10_list_ctrl_click
click 28 27;                   shot 11_templates
key Escape
# A click on empty space in the view clears the selection
click 1450 820;                shot 12_pick_empty
key ctrl+z;                    shot 13_undo
# Save through the file dialog (it starts in the working folder), then New, then Open by double-clicking the file
key ctrl+s; sleep 1;           shot 14_save_dialog
key ctrl+a; type_ "gtkscene.txt"; key Return; sleep 1
                               shot 15_saved
key ctrl+n;                    shot 16_new
key ctrl+o; sleep 1;           shot 17_open_dialog
# (no window manager here: the dialog opens at the screen's top left, the saved file is its first row)
dblclick 210 90; sleep 1
                               shot 18_opened
# A click on the vector's arrow (the camera saved in the file puts its tip there) selects it
click 905 429;                 shot 19_pick_vector
# The inspector: type a component (Enter commits), Delete edits the text (it doesn't delete the object), Esc reverts
click 84 527; key ctrl+a; type_ "2"; key Return
                               shot 20_type_component
click 195 527; key Home; key Delete
                               shot 21_delete_key_in_field
key Escape;                    shot 22_escape_reverts
# Formula mode, a formula, a broken one (shown in red), undo, rename, save
click 290 416;                 shot 23_formula_mode
click 150 527; key ctrl+a; type_ "[1, 2, 2]"; key Return
                               shot 24_formula_typed
key ctrl+a; type_ "q*2"; key Return
                               shot 25_bad_formula
click 448 27;                  shot 26_undo_button
click 150 362; key ctrl+a; type_ "w"; key Return
                               shot 27_rename
key ctrl+s;                    shot 28_saved
# A parameter's slider
click 1450 820; click 28 27; click 54 108
                               shot 29_parameter_added
drag 172 510 260 510;          shot 30_slider_drag
# Delete still works where it belongs: in the list and in the view
click 160 122; key Delete;     shot 31_list_delete
click 874 296;                 shot 32_view_pick
key Delete;                    shot 33_view_delete
input_stop
python3 - "$OUT" <<'PY'
import sys, hashlib, os
out = sys.argv[1]
names = sorted(f[:-4] for f in os.listdir(out) if f.endswith(".png") and f[:2].isdigit())
want = ["00_start", "01_orbit_drag", "02_pan_right_drag", "03_wheel_zoom_in", "04_complex_toggle", "05_back_to_3d",
        "06_add_menu", "07_vector_added", "08_matrix_added", "09_list_select", "10_list_ctrl_click", "11_templates",
        "12_pick_empty", "13_undo", "14_save_dialog", "15_saved", "16_new", "17_open_dialog", "18_opened", "19_pick_vector",
        "20_type_component", "21_delete_key_in_field", "22_escape_reverts", "23_formula_mode", "24_formula_typed",
        "25_bad_formula", "26_undo_button", "27_rename", "28_saved", "29_parameter_added", "30_slider_drag",
        "31_list_delete", "32_view_pick", "33_view_delete"]
def digest(n):
    p = os.path.join(out, n + ".png")
    return hashlib.sha1(open(p, "rb").read()).hexdigest() if os.path.exists(p) else None
problems = 0
for i in range(1, len(want)):
    a, b = digest(want[i - 1]), digest(want[i])
    ok = b is not None and a != b
    problems += not ok
    print(("ok      " if ok else "PROBLEM ") + want[i] + ("" if ok else ": missing or unchanged"))
saved = os.path.join(out, "run", "gtkscene.txt")
text = open(saved).read() if os.path.exists(saved) else ""
ok = "type = vector" in text and "type = matrix" not in text
problems += not ok
print(("ok      " if ok else "PROBLEM ") + "the saved file holds the vector, not the undone matrix")
# The inspector's edits reached the file saved at step 28
for line in ["object.0.name = w", "object.0.prop.mode = formula", "object.0.field.value = [1, 2, 2]",
             "object.0.field.x = 2"]:
    ok = line in text
    problems += not ok
    print(("ok      " if ok else "PROBLEM ") + "the saved file has " + line)
print("%d problems" % problems)
PY
