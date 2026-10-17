#!/bin/sh
# Compile every standalone .tex in a folder with pdflatex and render PNGs to look at:
#   latex.sh DIR    (e.g. $DEV/shots/1600x900/latex, or a folder filled by LATEX_OUT=DIR make test)
dir=${1:?usage: latex.sh DIR}
cd "$dir" || exit 1
ok=0; bad=0
for f in *.tex; do
  b=${f%.tex}
  if pdflatex -interaction=nonstopmode -halt-on-error "$f" > "$b.log" 2>&1; then
    pdftoppm -png -r 110 -singlefile "$b.pdf" "$b" && ok=$((ok + 1))
  else
    echo "FAIL $f"; grep -m3 -A3 "^!" "$b.log"; bad=$((bad + 1))
  fi
done
echo "$ok compiled, $bad failed (PNGs next to the .tex files)"
