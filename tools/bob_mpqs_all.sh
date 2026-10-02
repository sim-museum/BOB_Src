#!/bin/bash
# MA-MPQS-1 (BoB): two-player co-op runs of the lone Quick Missions (joiner = host's wingman, Frag slot 1).
# PVP=1: the joiner picks Luftwaffe in the Locker Room and takes the lone Bf 109 (Turkey Shoot 11, One on One 12).
# Disposable tree only (~/bob-test). MISSIONS="0 1 7 11 12" by default; BOB_NO_MPQS_LONE=1 reverts the feature.
cd "$(dirname "$0")/.." || exit 1
MISSIONS="${MISSIONS:-0 1 7 11 12}"
if [ "${PVP:-0}" = 1 ]; then tag=pvp; CC="2,2,1,#2129:0.1,1,1,#2200,1"; else tag=coop; CC="2,2,1,1,1,#2201,1"; fi
export BOB_TRACE_QMLIST=1
for m in $MISSIONS; do
  o="$HOME/bob-gates/mpqs/$tag$m"
  r=$(BOB_QM_INDEX=$m GD="$HOME/bob-test/drive_c/Program Files/Rowan Software/Battle Of Britain" \
      BOB_DRIVE_C="$HOME/bob-test/drive_c" OUT="$o" HOST_ENV="BOB_TRACE_AGG=1" CLIENT_ENV="BOB_TRACE_AGG=1" \
      HOST_CLICKS="2,1,#2128:0.2,1,1,#2200" HOST_FLY_MS="90000,121,747;125000,152,747" \
      CLIENT_CLICKS="$CC" SECS=300 timeout 460 bash tools/bob_mp_two_instance.sh 2>&1 | grep -a 'TWO-INSTANCE:')
  name=$(grep -ah "\[qmtable\] $m " "$o/host.log" | head -n1 | cut -d'"' -f2)
  lone=$(grep -ahE '\[mpqs\] (lone|  player 1)' "$o/host.log" | sort -u | tr '\n' ' ')
  crash=$(grep -acE 'SIGSEGV|crash handler|Segmentation' "$o"/*.log | awk -F: '{s+=$NF} END {print s+0}')
  printf "$tag "; printf 'mission %2d %-22s %s crash=%s | %s\n' "$m" "\"$name\"" "${r##*: }" "$crash" "$lone"
done
