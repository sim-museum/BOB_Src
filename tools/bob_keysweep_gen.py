#!/usr/bin/env python3
"""FUNC-SWEEP-BOB: a BOB_DIK_MS list that taps every REACHABLE live binding once (states 0/Alt/Ctrl/Shift).
Held back (leave the 3-D, end or pause the flight; test separately): EXITKEY, EJECTPILOT, PAUSEKEY,
KEY_CONFIGMENU, GOTOMAPKEY, ACCELKEY, ACCELKEY2, SUICIDE. Controller codes (>=0x100) and modifier rows skipped.
Usage: bob_keysweep_gen.py live_keytable.txt KEYMAPS.H [start_ms] [step_ms] > dik.txt ; writes dik.txt.map"""
import re, sys
HOLD = {'EXITKEY','EJECTPILOT','PAUSEKEY','KEY_CONFIGMENU','GOTOMAPKEY','ACCELKEY','ACCELKEY2','SUICIDE'}
MOD = {0: 0, 2: 0x38, 3: 0x1D, 4: 0x2A}
idx = {}
for ln in open(sys.argv[2], encoding='latin-1'):
    m = re.match(r'^(?:/\*\*/)?\s*KeyName\(\s*(\d+)\s*,\s*(\w+)', ln)
    if m and int(m.group(1)) not in idx: idx[int(m.group(1))] = m.group(2)
t = int(sys.argv[3]) if len(sys.argv) > 3 else 15000
step = int(sys.argv[4]) if len(sys.argv) > 4 else 600
out, mp = [], []
for ln in open(sys.argv[1]):
    m = re.match(r'^0x([0-9A-Fa-f]+),(\d+),(\d+)', ln)
    if not m: continue
    sc, st, val = int(m.group(1), 16), int(m.group(2)), int(m.group(3))
    name = idx.get(val >> 1, 'value_%d' % val)
    if st not in MOD or sc >= 0x100 or name.startswith('KeySrc_') or name in HOLD: continue
    out.append('%d,0x%02x,0x%02x' % (t, sc, MOD[st])); mp.append('%d\t0x%02x\tstate%d\t%s' % (t, sc, st, name)); t += step
print(';'.join(out))
open(sys.argv[5] if len(sys.argv) > 5 else '/dev/stderr', 'w').write('\n'.join(mp) + '\n')
