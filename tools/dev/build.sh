#!/bin/sh
# Build the app against the local raylib 5.5 into $DEV/build (warnings are shown).
. "$(dirname "$0")/env.sh"
[ -f "$RAYLIB/src/libraylib.a" ] || { echo "run tools/dev/raylib.sh first"; exit 1; }
cd "$REPO" || exit 1
make build BIN_DIR="$DEV/build" INCLUDES="-Isrc/include -Iinclude -isystem $RAYLIB/src" \
  LDLIBS="$RAYLIB/src/libraylib.a -lGL -lX11 -lm -lpthread -ldl" 2>&1 | grep -v "^compiling\|^linking" || true
