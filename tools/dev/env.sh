# Shared settings for the dev scripts (sourced, not run).
# Work files (raylib build, binary, screenshots) live outside the repo, in a folder that
# survives reboots. Override with AXISMUNDI_DEV=/some/dir.
REPO=$(cd "$(dirname "$0")/../.." && pwd)
DEV=${AXISMUNDI_DEV:-$HOME/.cache/axismundi-dev}
RAYLIB=$DEV/raylib
BIN=$DEV/build/linalg_viz
mkdir -p "$DEV"
