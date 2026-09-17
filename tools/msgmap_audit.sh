#!/usr/bin/env bash
# MOUSEMOVE-BOB-1: which Windows messages does the game register that this port never delivers?
#
# WHY. SRC/compat/afxwin.h #defines the whole ON_WM_*() family to NOTHING. Every registration in the
# game's message maps therefore vanishes at compile time -- silently: no warning, no unresolved
# symbol, and the game source still reads as though the handler is wired. The port compensates by
# calling specific entry points directly from its own layer (bob_ole_draw_*, bob_frontend_tick, ...),
# which works, but it means any behaviour nobody re-implemented is inert and LOOKS implemented.
#
# That has now cost real work twice:
#   ON_WM_ERASEBKGND  the campaign map's header band art (found via a gold comparison, S10 + MA S25)
#   ON_WM_MOUSEMOVE   every hover affordance, incl. side-select highlighting  (found via MA/BoB S13-14)
#
# Both were found by chasing a pixel difference for several sprints. This script finds the rest in
# one run. It does NOT say "defect": the port legitimately replaces some of these (painting is done
# by its own draw layer, not WM_PAINT). It says "registered by the game, never delivered here" --
# which is the list a human should triage.
#
#   bash tools/msgmap_audit.sh
set -u
ROOT="${ROOT:-/home/admin/bob}"
cd "$ROOT"
GAME="SRC/MFC SRC/RBUTTON SRC/RCOMBO SRC/RLISTBOX SRC/RSTATIC SRC/RSCRLBAR SRC/REDTBT SRC/REDIT SRC/RRADIO SRC/RSPINBUT SRC/MSCTLBR"
GAME=$(for d in $GAME; do [ -d "$d" ] && printf '%s ' "$d"; done)

printf '  %-18s %4s %5s %s\n' message reg impl delivery
undelivered=0; total=0
for m in $(grep -a -o '#define ON_WM_[A-Z_]*()' SRC/compat/afxwin.h | sed 's/#define ON_WM_//; s/()//' | sort -u); do
    bare=$(echo "$m" | tr -d '_')
    reg=$(grep -a -r -o "ON_WM_${m}()" $GAME 2>/dev/null | wc -l)
    imp=$(grep -a -r -o -i "::On${bare}[[:space:]]*(" $GAME 2>/dev/null | wc -l)
    [ "$reg" -eq 0 ] && [ "$imp" -eq 0 ] && continue
    total=$((total+1))
    # DELIVERY = PORT code calls the handler through an object.
    #
    # S2 CORRECTION. S1's test was "anyone calls On<X> through an object anywhere", and it reported
    # four delivered messages. Three were false positives, checked one by one:
    #   LBUTTONUP  CREDITS.CPP:212  FullPanel()->OnLButtonUp(...)   game forwarding to game
    #   MOVE       MEMAIN2.CPP      VS_Scroll.OnMove(Y)             a DIFFERENT class's OnMove
    #   SIZE       RDIALOG.CPP:469  //  dial->OnSize(...)           COMMENTED OUT
    # Only ON_WM_TIMER is really delivered, by bob_ole.cpp:1420. So: require the call site to be
    # PORT code (a bob_*/ma_* file, or SRC/compat), and drop commented-out lines. Game code calling
    # game code is the app talking to itself -- it cannot substitute for a message the port never
    # routes, because nothing invokes the caller either.
    deliv=$(grep -a -r -n -i "[->.]On${bare}[[:space:]]*(" SRC/ --include=*.cpp --include=*.CPP --include=*.h 2>/dev/null \
            | grep -av -i "::On${bare}" | grep -av -i "this->On${bare}" \
            | grep -av ':[^:]*:[[:space:]]*//' \
            | grep -aE '(^|/)(bob_|ma_)[^/]*:|^SRC/compat/' | wc -l)
    if [ "$deliv" -eq 0 ]; then
        printf '  %-18s %4d %5d  NEVER DELIVERED\n' "$m" "$reg" "$imp"
        undelivered=$((undelivered+1))
    else
        printf '  %-18s %4d %5d  delivered (%d site(s))\n' "$m" "$reg" "$imp" "$deliv"
    fi
done
echo "  ----------------------------------------"
echo "  $undelivered of $total registered messages are never delivered by the port."
echo "  Not all are defects -- the port replaces some (painting) in its own layer."
echo "  Triage against what the port re-implements before filing any of them."
