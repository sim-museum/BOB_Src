# Tacview: installing it, and opening a sortie recorded by this port

This port records flights as **ACMI** files, which Tacview reads. Nothing in the repo said how to
get Tacview onto a Linux box or where the exports land, so this is that (R15). MiG Alley has the
twin of this file (`port/TACVIEW.md`, its PO-87); the installation is identical and **the export
paths are not** — see below, because this port archives sorties and MiG Alley does not.

## What this port writes, and where

Recording is **on by default**; `BOB_ACMI=0` turns it off (`bob_acmi.cpp:39`). Two files matter,
and the difference between them is the thing to get right:

**1. The live export — one fixed name, overwritten every sortie:**

    …/Rowan Software/Battle Of Britain/acmi_current.txt

**2. The archive — one file per saved replay, kept:**

    …/Rowan Software/Battle Of Britain/VIDEOS/<stem>.acmi

When the game saves a replay, `bob_acmi_save_as()` copies the live export beside the `.cam` the
game just wrote and **gives it the same stem** — save `260825test6.cam` and you get
`VIDEOS/260825test6.acmi`. The live recording stays open while it copies, so saving mid-flight does
not stop the tee.

**This is the real difference from MiG Alley**, which has only the fixed-name live file and
therefore loses each sortie to the next one. Here, anything you *save as a replay* is kept
automatically; anything you merely fly is not.

⚠️ **The `VIDEOS/*.acmi` archive is not a parity oracle.** Every file currently in it **predates
S276**, the fix to the yaw convention, so those recordings carry the old orientation. The
orientation gate deliberately reads **only** `acmi_current.txt` for this reason
(`tools/bob_gates.sh:238`). Do not judge the exporter by an archived file.

## Installing Tacview

Tacview is a Windows application; on this box it runs under Wine in **its own prefix**, kept apart
from the game prefixes so a Wine change for one cannot disturb the other.

**The maintained way — a self-contained launcher that installs on first run:**

    /home/admin/sgl/SAT/tacview/tacview.sh

It sets `WINEPREFIX=$PWD/WP` and `WINEARCH=win64`, installs **Wine Mono** if absent (Tacview's
addons need .NET; without it the install appears to succeed and then misbehaves), runs the newest
`INSTALL/Tacview*Setup*.exe` it finds (`sort -V`), and afterwards just launches — inside
`wine explorer /desktop=Tacview,1280x800`, so it keeps its own virtual desktop instead of fighting
the host window manager.

Installers already on this machine, newest last:

    /home/admin/sgl/SAT/INSTALL/Tacview176Setup.exe
    /home/admin/sgl/SAT/tacview/INSTALL/Tacview187Setup.exe
    /home/admin/Downloads/Tacview195Setup.exe            (1.9.5)
    /home/admin/Documents/260825/Tacview195Setup.exe

A second, independent 1.9.5 install sits in `/home/admin/Documents/260825/WP` behind
`~/Desktop/Tacview.desktop`. Either works; they share no preferences.

## Opening a recording

1. Fly a sortie. If you want it kept, **save the replay in-game** — that is what produces the
   named `VIDEOS/<stem>.acmi`.
2. Otherwise copy the live file out before flying again:

       cp "…/Battle Of Britain/acmi_current.txt" ~/Documents/Tacview/sortie.acmi

3. Start Tacview and use **File > Open**. Wine maps `Z:` to the Linux root, so `~/Documents/…`
   appears under `Z:\home\admin\Documents\…`.

The extension of the live file is `.txt`; Tacview identifies the format by content and opens it
regardless, but renaming a copy to `.acmi` makes it obvious what it is.

## Two things to know before you read anything into a replay

**1. Tacview orients the model from `Yaw` ALONE, and in the OPPOSITE rotational sense from compass
heading** (S276/S277). The transform is `T=Lon|Lat|Alt|Roll|Pitch|Yaw|U|V|Heading`; Tacview draws
the aircraft from `Yaw`, and the trailing `Heading` field does **not** orient it. Both ports lost
time to this — a confident, wrong "the aircraft is flying backwards" reading — before a
four-variant synthetic control settled it. If aircraft point away from their own track, suspect
this convention before the flight model.

**2. A sortie that stops at 20.48 s is R10, and it is fixed.** The exporter used to truncate every
recording there — a block-wrap in the frame counter — taking a 232 s sortie down to 20.4 s. If you
open an old archived file and it ends abruptly, check its date against that fix before treating it
as evidence.

## Checking an export without opening Tacview

    tools/bob_acmi_orientation.sh <file.acmi>

asserts the yaw convention offline and displaylessly. It returns **INCONCLUSIVE**, not PASS, when a
sortie contains no east-west leg to judge by — a distinction worth keeping, since a clean result on
a recording that never tested the thing is not a pass. It runs as `GATE R11` in `tools/bob_gates.sh`.
