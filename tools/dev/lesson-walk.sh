#!/bin/bash
# Check every tutorial step: patch the tour with tools/lesson_walk.py, build, run, report, restore.
# A line ends with "<-- PROBLEM" when a scripted task is not solved or a task is done before acting.
. "$(dirname "$0")/env.sh"
cd "$REPO" || exit 1
cp src/app/Tour.cpp "$DEV/Tour.cpp.bak"
python3 tools/lesson_walk.py src/app/Tour.cpp
"$(dirname "$0")/build.sh" | grep -E "error" -A3
mkdir -p "$DEV/shots/walk"
xvfb-run -a -s "-screen 0 1700x1000x24" sh -c "timeout ${WALK_TIMEOUT:-600} $BIN --size 1600x900 --tour $DEV/shots/walk" 2>&1 \
  | grep -o "WALK.*\|SOLVED.*" > "$DEV/walk.log"
cp "$DEV/Tour.cpp.bak" src/app/Tour.cpp
"$(dirname "$0")/build.sh" > /dev/null
paste -d' ' <(grep WALK "$DEV/walk.log") <(grep SOLVED "$DEV/walk.log" | awk '{print "solved="$3}') \
  | awk '{bad = ($3=="task=1" && $5=="scripted=1" && $NF!="solved=1") || $4=="early=1"; print $0 (bad ? "   <-- PROBLEM" : "")}'
