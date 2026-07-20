#!/usr/bin/env bash
# M0 validation harness for the BOB Linux port.
# Parks the spectator camera (BOB_CAM_*) at a fixed world pose, renders the 3D
# scene, captures one framebuffer to PNG, and reports objective stats so we judge
# rendering by pixels rather than impressions.
#
# Usage:
#   tools/bob_validate.sh <tag> [landfix 0|1] [frame N]
# Camera pose comes from env (with land defaults):
#   CAM_X CAM_Y CAM_Z CAM_PITCH CAM_HDG CAM_ROLL  QM_INDEX
#
# Output: /tmp/bobval/<tag>.png  + printed stats (size, distinct colours, samples)
set -u

TAG="${1:-shot}"
LANDFIX="${2:-0}"
FRAME="${3:-120}"

# Paths derive from this script's location / $HOME so the repo works from any
# checkout; override with BOB_DRIVE_C / BOB_BIN.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVE_C="${BOB_DRIVE_C:-$HOME/sgl/TUE/BattleOfBritain/WP/drive_c}"
GAME_DIR="$DRIVE_C/Program Files/Rowan Software/Battle Of Britain"
BOB="${BOB_BIN:-$SCRIPT_DIR/../build/bob}"
OUT=/tmp/bobval
mkdir -p "$OUT"

# Land defaults: aim at the highest map item (a hill) from above, pitched down.
CAM_X="${CAM_X:-25677824}"
CAM_Y="${CAM_Y:-300000}"
CAM_Z="${CAM_Z:-35008512}"
CAM_PITCH="${CAM_PITCH:--6000}"   # ANGLES; 0x2000=45deg. negative ~ nose-down (verify empirically)
CAM_HDG="${CAM_HDG:-0}"
CAM_ROLL="${CAM_ROLL:-0}"
QM_INDEX="${QM_INDEX:-0}"

rm -f /tmp/bobframe.ppm
( cd "$GAME_DIR" && timeout 25 env \
    DISPLAY=:0 \
    BOB_DRIVE_C="$DRIVE_C" \
    BOB_RUN_INIT=1 BOB_BOOT_FRONTEND=1 \
    BOB_QM_INDEX="$QM_INDEX" \
    BOB_LANDFIX="$LANDFIX" \
    BOB_CAM_X="$CAM_X" BOB_CAM_Y="$CAM_Y" BOB_CAM_Z="$CAM_Z" \
    BOB_CAM_PITCH="$CAM_PITCH" BOB_CAM_HDG="$CAM_HDG" BOB_CAM_ROLL="$CAM_ROLL" \
    BOB_DUMP_FRAME="$FRAME" BOB_EXIT_AFTER_DUMP=1 \
    "$BOB" >/tmp/bobval/$TAG.log 2>&1 )

echo "=== $TAG (landfix=$LANDFIX frame=$FRAME cam=($CAM_X,$CAM_Y,$CAM_Z) pitch=$CAM_PITCH hdg=$CAM_HDG) ==="
grep -aE '\[present\] dumped|FATAL|assert' /tmp/bobval/$TAG.log | head -3

if [ ! -s /tmp/bobframe.ppm ]; then
    echo "  NO FRAME captured (see /tmp/bobval/$TAG.log)"
    exit 1
fi

python3 - "$OUT/$TAG.png" <<'PY'
import sys
from PIL import Image
im = Image.open('/tmp/bobframe.ppm').convert('RGB')
im.save(sys.argv[1])
w,h = im.size
px = list(im.getdata())
colors = set(px)
print(f"  size={w}x{h}  distinct_colours={len(colors)}")
# coarse top/middle/bottom band sampling — terrain should differ from sky
def band(y0,y1):
    s=[px[y*w+x] for y in range(y0,y1) for x in range(0,w,17)]
    cs=set(s)
    avg=tuple(sum(c[i] for c in s)//len(s) for i in range(3))
    return avg,len(cs)
for name,(y0,y1) in [("top",(0,h//3)),("mid",(h//3,2*h//3)),("bot",(2*h//3,h))]:
    avg,nc = band(y0,y1)
    print(f"  {name}: avg_rgb={avg} distinct={nc}")
print(f"  saved {sys.argv[1]}")
PY