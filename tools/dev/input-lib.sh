# Helpers for driving the app with real X11 input (xdotool) under Xvfb. Sourced by
# input-test.sh; can also be sourced by hand for exploring:
#   . tools/dev/input-lib.sh; input_start; click 290 83; shot add_menu; input_stop
# Notes: keys must go through XTEST (xdotool key without --window; GLFW ignores synthetic
# window events), the pointer inside the window gives it focus, and screenshots use the
# app's SIGUSR1 hook (writes signal_NNN.png into the working directory).
DEV=${DEV:-${AXISMUNDI_DEV:-$HOME/.cache/axismundi-dev}}
BIN=${BIN:-$DEV/build/linalg_viz}
OUT=${OUT:-$DEV/input}
export DISPLAY=${INPUT_DISPLAY:-:77}
# The GTK app (BIN=.../axismundi-gtk): X11, no accessibility bus (there is no session bus here)
export GDK_BACKEND=x11 GTK_A11Y=none NO_AT_BRIDGE=1

input_start() {  # fresh preferences and working folder, app at 1600x900
    mkdir -p "$OUT"; rm -rf "$OUT/cfg" "$OUT/run" "$OUT"/*.png; mkdir -p "$OUT/cfg" "$OUT/run"
    xdpyinfo >/dev/null 2>&1 || xdotool getmouselocation >/dev/null 2>&1 || {
        Xvfb "$DISPLAY" -screen 0 1700x1000x24 >/dev/null 2>&1 &
        for _ in 1 2 3 4 5 6 7 8 9 10; do xdotool getmouselocation >/dev/null 2>&1 && break; sleep 0.5; done; }
    (cd "$OUT/run" && XDG_CONFIG_HOME="$OUT/cfg" exec "$BIN" --size 1600x900 > "$OUT/app.log" 2>&1) &
    for _ in 1 2 3 4 5 6 7 8 9 10 11 12; do
        W=$(xdotool search --name axismundi 2>/dev/null | head -1); [ -n "$W" ] && break; sleep 0.5; done
    sleep 1
    APP=$(pgrep -n "$(basename "$BIN" | cut -c1-15)")  # BusyBox pgrep: the process name, 15 chars at most
    [ -n "$W" ] || { echo "app window not found"; return 1; }
}
input_stop() { kill "$APP" 2>/dev/null; pkill Xvfb 2>/dev/null; true; }

shot() { rm -f "$OUT"/run/signal_*.png; kill -USR1 "$APP"; sleep 0.8
         f=$(ls -t "$OUT"/run/signal_*.png 2>/dev/null | head -1)
         [ -n "$f" ] && mv "$f" "$OUT/$1.png" && echo "  $1"; }
# click holds the button ~80 ms like a mouse; tap presses and releases at once (touchpad)
click() { xdotool mousemove --window "$W" "$1" "$2" sleep 0.15 mousedown 1 sleep 0.08 mouseup 1; sleep 0.3; }
tap()   { xdotool mousemove --window "$W" "$1" "$2" sleep 0.15 click 1; sleep 0.3; }
dblclick() { xdotool mousemove --window "$W" "$1" "$2" mousedown 1 sleep 0.08 mouseup 1 sleep 0.12 \
             mousedown 1 sleep 0.08 mouseup 1; sleep 0.4; }
drag()  { xdotool mousemove --window "$W" "$1" "$2" sleep 0.1 mousedown 1 sleep 0.1 \
          mousemove --window "$W" $(( ($1+$3)/2 )) $(( ($2+$4)/2 )) sleep 0.1 \
          mousemove --window "$W" "$3" "$4" sleep 0.1 mouseup 1; sleep 0.3; }
rdrag() { xdotool mousemove --window "$W" "$1" "$2" sleep 0.1 mousedown 3 sleep 0.1 \
          mousemove --window "$W" $(( ($1+$3)/2 )) $(( ($2+$4)/2 )) sleep 0.1 \
          mousemove --window "$W" "$3" "$4" sleep 0.1 mouseup 3; sleep 0.3; }
wheel() { xdotool mousemove --window "$W" "$1" "$2" sleep 0.1; for _ in $(seq "$4"); do xdotool click "$3"; sleep 0.05; done; sleep 0.3; }  # 4 up, 5 down
hover() { xdotool mousemove --window "$W" "$1" "$2"; sleep 0.2; }
key()   { xdotool key --delay 80 "$@"; sleep 0.3; }
type_() { xdotool type --delay 40 "$*"; sleep 0.3; }
