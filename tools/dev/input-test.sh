#!/bin/sh
# Real-input walkthrough: starts the app normally (empty preferences) under Xvfb, clicks, drags
# and types like a user with xdotool, and saves a screenshot after each step. Look at the PNGs
# in $DEV/input afterwards (each name says what it should show). Needs xdotool, Xvfb and a
# built app (build.sh). Coordinates are window pixels at 1600x900; if the layout changes, fix
# them here (the helpers are in input-lib.sh).
cd "$(dirname "$0")/../.." || exit 1
. tools/dev/input-lib.sh
command -v xdotool >/dev/null || { echo "xdotool is not installed"; exit 1; }
input_start || { input_stop; exit 1; }
echo "screenshots in $OUT:"

shot 00_start
tap 290 83;                       shot 01_add_menu_by_tap        # touchpad-style click opens it
click 56 203;                     shot 02_vector_added
click 248 263;                    shot 03_formula_mode
click 150 302; key ctrl+a
type_ "[2cos(t) + 0.5*sin(3t), 2sin(t) - 0.25*cos(5t), 1 + 0.5*sin(2t)*cos(t/2)] * (1 + 0.1*t)"
                                  shot 04_long_formula_typing    # wraps onto 3 lines
key Return;                       shot 05_committed
click 150 302; key ctrl+End; key ctrl+Return
                                  shot 06_big_editor             # Ctrl+Enter, same text
type_ " + q";                     shot 07_big_editor_error       # live error: unknown name q
key Escape;                       shot 08_cancelled              # formula unchanged
click 1000 250;                   shot 09_deselected             # click on empty space
click 835 470;                    shot 10_picked                 # click the arrow selects v
drag 760 736 760 420;             shot 11_panel_dragged          # Time panel moved up
key ctrl+b;                       shot 12_sidebar_collapsed
key ctrl+b
click 1000 250                                                   # deselect: no templates in Add
click 290 83; click 56 251;       shot 13_matrix_added           # M below v
click 65 381; type_ "7"; key Return
                                  shot 14_cell_replaced          # 7, not 17
click 180 262; type_ "A"; key Tab; type_ "2"; key Return
                                  shot 15_renamed_and_tab        # name A, first cell 2
hover 800 300; key ctrl+s;        shot 16_save_dialog            # suggested name selected
type_ "trial"; key Return;        shot 17_saved                  # trial.txt
key ctrl+o;                       shot 18_open_dialog
dblclick 615 295;                 shot 19_opened
key Tab; hover 900 450; xdotool mousemove_relative 60 0 sleep 0.2 mousemove_relative 60 -20
                                  shot 20_camera_mode
key Escape;                       shot 21_back_to_ui
echo "files saved by the app:"; ls "$OUT/run"
input_stop
