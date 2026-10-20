#!/bin/sh
# Run the GTK app's screenshot tour headlessly: tour-gtk.sh WxH [name] [extra args, e.g. --theme light]
# PNGs go to $DEV/shots-gtk/<name>. GTK draws through X11 here (Xvfb, Mesa's software OpenGL).
. "$(dirname "$0")/env.sh"
size=${1:-1280x800}; name=${2:-$size}; [ $# -ge 2 ] && shift 2 || shift $#
out=$DEV/shots-gtk/$name; rm -rf "$out"; mkdir -p "$out"
w=${size%x*}; h=${size#*x}
# No session bus here: keep GTK from trying portals and accessibility (only warnings otherwise)
export GDK_BACKEND=x11 GTK_A11Y=none GDK_DEBUG=${GDK_DEBUG:-} NO_AT_BRIDGE=1
xvfb-run -a -s "-screen 0 $((w+100))x$((h+100))x24" sh -c "timeout ${TOUR_TIMEOUT:-120} $GTK_BIN --size $size --tour $out $*" 2>&1 \
  | grep -v "^$" | tail -15
echo "$(ls "$out"/*.png 2>/dev/null | wc -l) screenshots in $out"
