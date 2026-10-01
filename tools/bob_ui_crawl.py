#!/usr/bin/env python3
"""BoB UI crawler (FUNC-SWEEP-BOB): click every menu item and hosted control reachable from the main menu,
each in a FRESH headless launch, and record crash / quit / which screen it led to.

Targets come from the game itself: BOB_DUMP_HITTARGETS=1 prints each screen's menu rects and hosted
controls when the screen launches; the LAST dump in a log is where the clicks led. Clicks are id-based
BOB_AUTOCLICK steps ("N" = menu item N, "#ID" = hosted control), one step per 120 ticks, so no pixel is
guessed. Breadth-first, each distinct screen expanded once.

Runs on a DISPOSABLE drive_c (default ~/bob-test/drive_c). Usage: tools/bob_ui_crawl.py [--max-runs N]
"""
import argparse, os, re, subprocess

ap = argparse.ArgumentParser()
ap.add_argument('--dc', default=os.path.expanduser('~/bob-test/drive_c'))
ap.add_argument('--bin', default=os.path.expanduser('~/bob/build/bob'))
ap.add_argument('--out', default=os.path.expanduser('~/bob-gates/crawl'))
ap.add_argument('--max-runs', type=int, default=500)
ap.add_argument('--depth', type=int, default=3)
A = ap.parse_args()
GD = A.dc + '/Program Files/Rowan Software/Battle Of Britain'
os.makedirs(A.out + '/runs', exist_ok=True)
nruns = 0

def parse_last(txt):
    i = txt.rfind('[hittargets] menu rects:')
    if i < 0: return None
    out = []
    for ln in txt[i:].split('\n'):
        if not ln.startswith('[hittargets]'):
            if out or 'menu rects' not in ln: break
            continue
        m = re.match(r'\[hittargets\]\s+menu\[(\d+)\] = \((-?\d+),(-?\d+) (\d+)x(\d+)\)', ln)
        if m and int(m.group(4)) > 0: out.append(('menu%s' % m.group(1), m.group(1))); continue
        m = re.match(r'\[hittargets\]\s+id=(-?\d+)\s+rect=\((-?\d+),(-?\d+) (\d+)x(\d+)\)', ln)
        if m and int(m.group(4)) > 0 and int(m.group(5)) > 0 and int(m.group(1)) > 0:
            out.append(('#%s' % m.group(1), '#%s' % m.group(1)))
    return out

def run(steps, tag):
    global nruns
    nruns += 1
    secs = 14 + 3 * len(steps) + 6
    log = '%s/runs/%s.log' % (A.out, tag)
    env = dict(os.environ, BOB_RUN_INIT='1', BOB_FRONTEND='1', BOB_OLE_DRAW='1',
               BOB_DRIVE_C=A.dc, BOB_DUMP_HITTARGETS='1')
    if steps: env['BOB_AUTOCLICK'] = ','.join(steps)
    with open(log, 'wb') as f:
        p = subprocess.run(['timeout', '-k', '5', '-s', 'INT', str(secs), A.bin], cwd=GD, env=env, stdout=f, stderr=subprocess.STDOUT)
    txt = open(log, 'rb').read().decode('latin-1')
    crash = txt.count('=== CRASH') + txt.count('Segmentation fault') + txt.count('Aborted')
    fired = txt.count('autoclick step') + txt.count('-> menu item')
    state = 'CRASH' if crash else ('alive' if p.returncode in (124, 130, -2, 137) else 'exited(%d)' % p.returncode)
    return state, parse_last(txt), fired, log

tsv = open(A.out + '/crawl.tsv', 'a')
state, top, _, _ = run([], 'main')
print('main targets:', [n for n, _ in top or []], flush=True)
frontier = [([], top, 'main')]
seen = {tuple(sorted(n for n, _ in top or []))}
for depth in range(A.depth):
    nxt = []
    for steps, targets, pdesc in frontier:
        for name, step in targets or []:
            if nruns >= A.max_runs: break
            path = steps + [step]
            tag = re.sub(r'[^A-Za-z0-9]+', '_', '%s_%s' % (pdesc, name))[-120:]
            st, after, fired, lg = run(path, tag)
            key = tuple(sorted(n for n, _ in after)) if after else ()
            new = bool(after) and key not in seen
            tsv.write('\t'.join(['%s > %s' % (pdesc, name), st, 'newscreen' if new else 'same', 'fired=%d/%d' % (fired, len(path)), os.path.basename(lg)]) + '\n'); tsv.flush()
            print('%-60s %-10s %-9s fired=%d/%d' % (('%s > %s' % (pdesc, name))[:60], st, 'newscreen' if new else 'same', fired, len(path)), flush=True)
            if st == 'alive' and new:
                seen.add(key); nxt.append((path, after, '%s > %s' % (pdesc, name)))
    frontier = nxt
print('=== CRAWL COMPLETE runs=%d screens=%d ===' % (nruns, len(seen)), flush=True)
