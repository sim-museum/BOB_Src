#!/usr/bin/env python3
"""R3.2 S4 — score a real-GL flight's frame sequence for CLOUD PIXELS OVER THE COCKPIT.

The PO's report is that cloud drifts over the cockpit structure (a draw-order / depth fault).
The cockpit structure is the part of the frame that does NOT change between frames: airframe,
gunsight barrel, instrument panel.  So the mask is built from the sequence itself --

    cockpit mask = pixels whose colour is near-constant across every frame AND dark

-- and the statistic is the count of CLOUD-COLOURED pixels (bright, near-neutral) falling inside
that mask, per frame.  A cloud painting over the cockpit lights those pixels up; chrome on the
gunsight does not move, so it is inside the mask as a CONSTANT baseline that every frame shares.

Reported next to it: the same count above the horizon band (where the clouds legitimately are),
which proves the detector can see a cloud at all.  A detector that reports zero everywhere is
measuring nothing -- the positive control has to be in the same run.

  tools/bob_cloud_score.py <dir-with-f.NNNN.ppm>
"""
import sys, os, glob

def load(p):
    with open(p, 'rb') as f:
        assert f.readline().strip() == b'P6'
        line = f.readline()
        while line.startswith(b'#'):
            line = f.readline()
        w, h = map(int, line.split())
        f.readline()
        return w, h, f.read(w * h * 3)

CLOUD_MIN = 150      # bright
CLOUD_SPREAD = 40    # near-neutral (max-min channel)

def is_cloud(px, i):
    r, g, b = px[i], px[i+1], px[i+2]
    return min(r, g, b) >= CLOUD_MIN and (max(r, g, b) - min(r, g, b)) <= CLOUD_SPREAD

d = sys.argv[1] if len(sys.argv) > 1 else '/tmp/bob_realgl_flight'
files = sorted(glob.glob(os.path.join(d, 'f.*.ppm')))
if not files:
    print("no frames in", d); sys.exit(1)
w, h, first = load(files[0])
frames = [first]
for p in files[1:]:
    w2, h2, px = load(p)
    if (w2, h2) != (w, h):
        print("  ! %s is %dx%d, not %dx%d — skipped" % (os.path.basename(p), w2, h2, w, h)); continue
    frames.append(px)
n = len(frames)
print("%d frames, %dx%d" % (n, w, h))

# cockpit mask: dark and near-constant across the sequence (lower 45% only — the canopy frame
# and panel live there; the upper frame members move against the sky and would be ambiguous)
# ⚠️ THE MASK MUST NOT BE ABLE TO ERASE THE DEFECT.  A rule of "dark in frame 0 AND unchanged in
# EVERY frame" would drop exactly the pixels a passing cloud lit up — the instrument would delete
# its own evidence.  So a pixel is structure if it is dark in a MAJORITY of frames (70%), which
# survives a cloud crossing it in some of them.
MOSTLY = 0.70
mask = bytearray(w * h)
y0 = int(h * 0.55)
nmask = 0
need = int(n * MOSTLY)
for y in range(y0, h):
    for x in range(0, w):
        i = (y * w + x) * 3
        dark = 0
        for f in frames:
            if max(f[i], f[i+1], f[i+2]) <= 110:
                dark += 1
        if dark >= need:
            mask[y * w + x] = 1; nmask += 1
print("cockpit mask: %d px (%.1f%% of the frame)" % (nmask, 100.0 * nmask / (w * h)))
if nmask < 1000:
    print("  ! mask is tiny — the frames may not be a cockpit view; treat the numbers below as void")

sky_rows = range(0, int(h * 0.35))
print("%-14s %10s %10s   %s" % ("frame", "cloud@sky", "cloud@cockpit", "verdict"))
base = None
for p, px in zip(files, frames):
    sky = sum(1 for y in sky_rows for x in range(0, w, 2) if is_cloud(px, (y * w + x) * 3))
    cp  = sum(1 for y in range(y0, h) for x in range(0, w)
              if mask[y * w + x] and is_cloud(px, (y * w + x) * 3))
    if base is None: base = cp
    print("%-14s %10d %10d   %s" % (os.path.basename(p), sky * 2, cp,
          "" if cp <= base + 200 else "*** CLOUD OVER COCKPIT (%+d over the first frame)" % (cp - base)))
