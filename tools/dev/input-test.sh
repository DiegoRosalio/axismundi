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
click 250 423;                    shot 09b_style_chalkboard      # View section: Style card
click 90 385                                                     # back to Colorful
click 835 470;                    shot 10_picked                 # click the arrow selects v
hover 1000 250; key o;            shot 10b_solo                  # O: only the selection, chip on top
key o
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
drag 1000 250 1150 320;           shot 20_orbit_drag             # empty view: the camera turns
key Tab;                          shot 21_tab_hint               # toast: the fly camera is in Settings
key ctrl+e;                       shot 22_export_panel
click 1352 660;                   shot 23_pdf_dialog             # Save PDF… (needs pdflatex)
type_ "my figure"; key Return; sleep 8
                                  shot 24_pdf_saved              # toast "Saved my figure.pdf"
f="$OUT/run/my figure.pdf"
if [ -f "$f" ] && [ "$(head -c 4 "$f")" = "%PDF" ]; then echo "  PDF ok: $f"; else echo "  PDF MISSING: $f"; fi
echo "files saved by the app:"; ls "$OUT/run"
input_stop

# Second session: a vector field from the Add menu (fresh start, so the menu has no templates)
OUT=$DEV/input/field; input_start || { input_stop; exit 1; }
click 290 83; click 97 671;        shot 30_field_added            # Add > Fields > Vector field
click 205 264; key ctrl+a; type_ "[-y - x/4, x - y/4, 0]"; key Return
click 305 589;                    shot 31_field_particles_on     # Particles switch
hover 1000 250; key p; sleep 1.5; shot 32_particles_moving       # P plays the clock
key p
input_stop

# Third session: a quaternion from the Add menu, a new formula, acting on a vector
OUT=$DEV/input/quaternion; input_start || { input_stop; exit 1; }
click 290 83; click 99 375;        shot 40_quaternion_added       # Add > Linear algebra > Quaternion
click 175 264; key ctrl+a; type_ "qaxis([1, 1, 0], 2pi/3)"; key Return
                                  shot 41_formula                # 120° about (0.71, 0.71, 0)
click 305 447;                    shot 42_acts_on_vector         # the switch shows v and q v q⁻¹
input_stop

# Fourth session: the calculus and Fourier objects from the Add menu (the menu scrolls with the wheel)
OUT=$DEV/input/calculus; input_start || { input_stop; exit 1; }
click 290 83; click 110 747;       shot 50_volume_added           # Add > Calculus > Volume under z = f(x, y)
hover 1000 250; click 1000 250
click 290 83; click 110 843;       shot 51_frame_added            # Add > Calculus > Moving frame
click 1000 250
click 290 83; xdotool mousemove --window $W 150 600 click --repeat 5 5; sleep 0.4
click 110 701;                     shot 52a_tangent_added         # (scrolled) Add > Calculus > Tangent line
click 1000 250
click 290 83; xdotool mousemove --window $W 150 600 click --repeat 5 5; sleep 0.4
click 110 749;                     shot 52b_tangent_plane_added   # (scrolled) Add > Calculus > Tangent plane
click 1000 250
click 290 83; xdotool mousemove --window $W 150 600 click --repeat 5 5; sleep 0.4
click 110 797;                     shot 52c_velocity_added        # (scrolled) Add > Calculus > Velocity
click 1000 250
click 290 83; xdotool mousemove --window $W 150 600 click --repeat 5 5; sleep 0.4
click 110 845;                     shot 52d_taylor_added          # (scrolled) Add > Calculus > Taylor polynomial
click 1000 250
click 290 83; xdotool mousemove --window $W 150 600 click --repeat 10 5; sleep 0.4
click 110 731;                     shot 52e_newton_added          # (scrolled to the end) Add > Calculus > Newton
click 1000 250
click 290 83; xdotool mousemove --window $W 150 600 click --repeat 10 5; sleep 0.4
click 110 807;                     shot 52_series_added           # (scrolled to the end) Add > Fourier > Fourier series
key 2; click 290 83;               shot 53_complex_menu           # complex plane: Add menu
click 87 355;                      shot 54_epicycles_added        # Add > Fourier > Fourier epicycles
hover 1000 250; key p; sleep 2;    shot 55_epicycles_drawing      # P: the circles start drawing
key p
input_stop

# Fifth session: the orbit camera (the default) and switching to the fly camera in Settings
OUT=$DEV/input/camera; input_start || { input_stop; exit 1; }
click 290 83; click 56 203;        shot 60_vector_selected
drag 1000 300 1180 380;           shot 61_orbit_keeps_selection  # turned; v still selected
rdrag 1000 300 900 250;           shot 62_panned                 # right-drag slides the view
wheel 1000 300 4 4;               shot 63_zoomed_in              # wheel up: closer
wheel 1000 300 5 10;              shot 63b_zoomed_out            # wheel down: farther; grid lines and numbers spread out
wheel 1000 300 4 10
key period;                       shot 64_focus_on_v             # . : v's tip is the orbit center
click 1566 25; click 1499 283;    shot 65_settings_fly           # Settings > Camera > Fly
key Escape; key Tab; hover 900 450; xdotool mousemove_relative 60 0 sleep 0.2 mousemove_relative 60 -20
                                  shot 66_fly_camera_mode        # Tab captures the mouse again
key Escape;                       shot 67_back_to_ui
drag 1000 300 1180 380;           shot 68_fly_drag_does_nothing  # no orbit with the fly camera
input_stop
