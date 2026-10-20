#!/bin/sh
# Real X11 input on the GTK app under Xvfb (xdotool): orbit drag, pan, wheel zoom, the space switch.
# Saves a PNG per step to $DEV/input-gtk and checks that each step changed the picture.
DEV=${AXISMUNDI_DEV:-$HOME/.cache/axismundi-dev}
BIN=$DEV/build/axismundi-gtk OUT=$DEV/input-gtk
. "$(dirname "$0")/input-lib.sh"
input_start || exit 1
shot 00_start
drag 800 450 950 500;          shot 01_orbit_drag
rdrag 800 450 700 400;         shot 02_pan_right_drag
wheel 800 450 4 3;             shot 03_wheel_zoom_in
click 839 27;                  shot 04_complex_toggle
input_stop
python3 - "$OUT" <<'PY'
import sys, zlib, hashlib, os
out = sys.argv[1]
names = ["00_start", "01_orbit_drag", "02_pan_right_drag", "03_wheel_zoom_in", "04_complex_toggle"]
def digest(n):
    p = os.path.join(out, n + ".png")
    return hashlib.sha1(open(p, "rb").read()).hexdigest() if os.path.exists(p) else None
d = [digest(n) for n in names]
problems = 0
for i in range(1, len(names)):
    ok = d[i] is not None and d[i] != d[i - 1]
    problems += not ok
    print(("ok      " if ok else "PROBLEM ") + names[i] + ("" if ok else ": missing or unchanged"))
print("%d problems" % problems)
PY
