#!/bin/sh
#
# port-diff.sh - show every change the port made to the legacy sources.
#
# The ported sources in src/ (and tools/) are compared with the canonical
# originals in Kk/.  Line endings (CRLF in the originals) and white space are
# ignored so that only real changes appear.
#
#   tools/port-diff.sh            # unified diff of all files
#   tools/port-diff.sh --stat     # changed-line counts per file
#
cd "$(dirname "$0")/.." || exit 1

if [ ! -d Kk ]; then
    echo "port-diff.sh: the original sources (Kk/) are not here." >&2
    echo "Copy the Kk folder from the reference package next to src/ and run again." >&2
    exit 1
fi

stat=0
[ "$1" = "--stat" ] && stat=1

for ported in src/*.c src/*.h tools/makecfg.c tools/driver.c; do
    base=$(basename "$ported")
    orig=$(ls Kk/ | grep -i "^$base$" | head -1)
    [ -z "$orig" ] && continue
    if [ $stat -eq 1 ]; then
        n=$(diff -w --strip-trailing-cr "Kk/$orig" "$ported" | grep -c '^[<>]')
        [ "$n" -gt 0 ] && printf '%-12s %4d changed lines\n' "$base" "$n"
    else
        diff -u -w --strip-trailing-cr "Kk/$orig" "$ported"
    fi
done
