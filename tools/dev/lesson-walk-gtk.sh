#!/bin/bash
# Check every tutorial step in the GTK app (its --lesson-walk mode): enter the step, do the task the scripted way
# (edu/TutorialSolutions.cpp), let the lesson card check it. PNGs per step go to $DEV/shots-gtk/walk.
# A line ends with "<-- PROBLEM" when a scripted task is not solved or a task is done before acting.
. "$(dirname "$0")/env.sh"
out=$DEV/shots-gtk/walk; rm -rf "$out"; mkdir -p "$out"
export GDK_BACKEND=x11 GTK_A11Y=none NO_AT_BRIDGE=1
log=$(xvfb-run -a -s "-screen 0 1700x1000x24" sh -c "timeout ${WALK_TIMEOUT:-1500} $GTK_BIN --size 1600x900 --lesson-walk $out" 2>&1 \
  | grep -o "WALK.*\|SOLVED.*")
paste -d' ' <(grep WALK <<<"$log") <(grep SOLVED <<<"$log" | awk '{print "solved="$3}') \
  | awk '{bad = ($3=="task=1" && $5=="scripted=1" && $NF!="solved=1") || $4=="early=1"; print $0 (bad ? "   <-- PROBLEM" : "")}'
