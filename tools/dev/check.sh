#!/bin/sh
# Compile every source with the project flags (no linking); prints ALL CLEAN when warning-free.
. "$(dirname "$0")/env.sh"
cd "$REPO" || exit 1
fail=0
for f in $(find src -name '*.cpp'); do
  out=$(clang++ -Wall -Wextra -O2 -std=c++20 -Isrc/include -Iinclude -isystem "$RAYLIB/src" -c "$f" -o /dev/null 2>&1)
  if [ -n "$out" ]; then echo "== $f"; echo "$out" | head -40; fail=1; fi
done
[ $fail = 0 ] && echo "ALL CLEAN"
