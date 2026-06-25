#!/usr/bin/env bash
# Cross-port sync guard for the shared Rowan-engine lessons doc.
#
# BoB keeps the engine notes at  doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md  and the sister
# MiG Alley port keeps a hand-synced copy at  ~/ma/port/BOB_PORT_LESSONS.md . The two
# are meant to be BYTE-IDENTICAL, but being hand-synced they drift (it has slipped three
# times). This guard catches that: it fails loudly when the copies diverge so a stale
# copy is caught at build time, not by a manual `diff` weeks later.
#
# Behaviour (a loud nudge, NOT a wall — matched to MA's rebuild.sh guard, which is
# deliberately non-fatal: a doc drift should never wedge a code build):
#   - copies identical          -> exit 0, quiet
#   - MA copy absent (no ~/ma)   -> exit 0, one-line note (don't break BoB-only checkouts)
#   - copies DIFFER              -> exit 0 with a LOUD banner + resync hint (warn, don't fail);
#                                   BOB_NOTES_SYNC_STRICT=1 escalates to exit 1 for CI/pre-commit
#
# Paths are overridable:  BOB_LESSONS=... MA_LESSONS=...  (CI / non-default layouts).
set -u

HERE="$(cd "$(dirname "$0")/.." && pwd)"
BOB_LESSONS="${BOB_LESSONS:-$HERE/doc/ROWAN_ENGINE_LINUX_PORT_NOTES.md}"
MA_LESSONS="${MA_LESSONS:-$HOME/ma/port/BOB_PORT_LESSONS.md}"

if [ ! -f "$BOB_LESSONS" ]; then
    echo "notes-sync: WARN: BoB lessons doc not found: $BOB_LESSONS" >&2
    exit 0
fi
if [ ! -f "$MA_LESSONS" ]; then
    echo "notes-sync: MiG Alley copy not present ($MA_LESSONS) — skipping cross-port check."
    exit 0
fi

if cmp -s "$BOB_LESSONS" "$MA_LESSONS"; then
    echo "notes-sync: shared lessons doc in sync with MiG Alley ✓"
    exit 0
fi

# Divergent — loud banner, but non-fatal by default (mirrors MA's guard).
echo "notes-sync: ============================================================" >&2
echo "notes-sync: WARNING: shared lessons doc has DRIFTED between the two ports." >&2
echo "  BoB: $BOB_LESSONS" >&2
echo "  MA : $MA_LESSONS" >&2
echo "  $(wc -l <"$BOB_LESSONS") lines (BoB) vs $(wc -l <"$MA_LESSONS") lines (MA)" >&2
echo "  diff (first 20 lines):" >&2
diff -u "$MA_LESSONS" "$BOB_LESSONS" 2>/dev/null | sed -n '1,20p' | sed 's/^/    /' >&2
echo "  Resync: cp the newer copy over the older (the two must be byte-identical)." >&2
echo "          e.g.  cp '$BOB_LESSONS' '$MA_LESSONS'" >&2
echo "notes-sync: ============================================================" >&2

# Non-fatal by default (a doc drift shouldn't block a code build). CI / pre-commit
# can escalate to a hard failure with BOB_NOTES_SYNC_STRICT=1.
if [ "${BOB_NOTES_SYNC_STRICT:-}" = "1" ]; then
    echo "notes-sync: BOB_NOTES_SYNC_STRICT=1 set — failing (exit 1)." >&2
    exit 1
fi
exit 0
