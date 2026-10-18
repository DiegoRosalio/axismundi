#!/bin/sh
# Build raylib 5.5 as a static library in $DEV/raylib (the distribution package is too old).
# Needs git, a C compiler and the X11/GL development packages (see CLAUDE.md).
set -e
. "$(dirname "$0")/env.sh"
if [ -f "$RAYLIB/src/libraylib.a" ]; then echo "raylib already built: $RAYLIB/src/libraylib.a"; exit 0; fi
[ -d "$RAYLIB" ] || git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git "$RAYLIB"
cd "$RAYLIB/src"
for f in rcore rshapes rtextures rtext rmodels utils raudio; do
  cc -c -O2 -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -D_GLFW_X11 -Iexternal/glfw/include "$f.c" -o "$f.o"
done
cc -c -O2 -D_GLFW_X11 -Iexternal/glfw/include rglfw.c -o rglfw.o
ar rcs libraylib.a rcore.o rshapes.o rtextures.o rtext.o rmodels.o utils.o raudio.o rglfw.o
echo "built $RAYLIB/src/libraylib.a"
