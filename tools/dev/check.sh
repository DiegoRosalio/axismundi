#!/bin/sh
# Compile every source with the project flags (no linking); prints ALL CLEAN when warning-free.
. "$(dirname "$0")/env.sh"
cd "$REPO" || exit 1
fail=0
# The GTK front end (src/gtk) needs the GTK headers, as system headers so their own warnings stay out
gtk=$(pkg-config --cflags gtkmm-4.0 libadwaita-1 2>/dev/null | sed 's/-I/-isystem /g')
for f in $(find src -name '*.cpp'); do
  extra=; case $f in src/gtk/*) extra=$gtk;; esac
  out=$(clang++ -Wall -Wextra -Wshadow -O2 -std=c++20 -Isrc/include -Iinclude -isystem "$RAYLIB/src" $extra -c "$f" -o /dev/null 2>&1)
  if [ -n "$out" ]; then echo "== $f"; echo "$out" | head -40; fail=1; fi
done
[ $fail = 0 ] && echo "ALL CLEAN"
