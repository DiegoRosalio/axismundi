#!/bin/sh
# Run the screenshot tour headlessly: tour.sh WxH [name] [extra args, e.g. --theme light]
# PNGs go to $DEV/shots/<name>; the tour also writes LaTeX figures to $DEV/shots/<name>/latex.
. "$(dirname "$0")/env.sh"
size=${1:-1600x900}; name=${2:-$size}; [ $# -ge 2 ] && shift 2 || shift $#
out=$DEV/shots/$name; rm -rf "$out"; mkdir -p "$out"
w=${size%x*}; h=${size#*x}
# The timeout must be inside xvfb-run, otherwise the app outlives the wrapper
xvfb-run -a -s "-screen 0 $((w+100))x$((h+100))x24" sh -c "timeout ${TOUR_TIMEOUT:-300} $BIN --size $size --tour $out $*" 2>&1 \
  | grep -E "^tour:|WARNING|ERROR|rror" | grep -v "MSAA\|OpenGL" | tail -5
echo "$(ls "$out"/*.png 2>/dev/null | wc -l) screenshots in $out"
