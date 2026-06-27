# Battle of Britain — Linux Port: Turkey Shoot demo + crash root-cause (2026-06-27, S46 spike)

Branch: `linux-port`. Build: 32-bit i386 ELF (`gcc -m32`, `-fpack-struct=1`), SDL2 + OpenGL + OpenAL.
This doc records a live demo session and a **newly root-caused combat-path crash**. No code fix
shipped this round (diagnosis/spike); the fix is scoped for S46. Running log: PORT.md.

## What was demoed

- **In-flight cockpit demo** on real GL `:0` (`BOB_BOOT_FRONTEND=1`) — booted a Quick-Mission
  Spitfire: daylit sky + clouds, terrain, cockpit, OpenAL engine audio, and the user's **Logitech
  Extreme 3D joystick auto-detected + mapped** (aileron/elevator/rudder/throttle).
- **"Turkey Shoot" mission is flyable.** It's Quick-Mission preset **`IDS_QUICK_11` "Turkey Shoot"**
  (`QMISS.CPP:372`, `//COMBAT : turkey`) — a Spitfire-vs-Me109 dogfight at ~10,000 ft over the coast.
  Selected via the boot scaffold's `BOB_QM_INDEX` (the 0-based slot in the live `quickmissions[]`
  table); Turkey Shoot is **index 11**. It boots to interactive flight cleanly
  (`[boot] QM: idx=11 title=2237 … View3d interactive`).

  ```
  cd "/home/m/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
  BOB_RUN_INIT=1 BOB_DRIVE_C="/home/m/sgl/TUE/BattleOfBritain/WP/drive_c" DISPLAY=:0 \
    BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=11 /home/m/bob/build/bob
  ```

## The crash (NEW — found by actually flying combat)

Turkey Shoot boots and flies, but **crashes a few seconds into the flight** — manifesting
non-deterministically as `free(): double free detected in tcache 2` (SIGABRT, exit 134) or a plain
SIGSEGV (exit 139). Varying manifestation = classic heap corruption. It does **not** reproduce under
gdb (perturbed thread timing hides the race) — consistent with the project's R1.3d double-free class.

**Root-caused with ASan** (the project's double-free oracle; `build-asan/`, `-DBOB_ASAN=ON`, i386
`lib32asan8`). With `ASAN_OPTIONS=halt_on_error=0:detect_odr_violation=0`, the combat-path error is:

```
ERROR: AddressSanitizer: heap-use-after-free  3dcode.cpp:1982-1983  ThreeDee::AddLensObject(...)
ERROR: AddressSanitizer: attempting double-free on 0xeed00590 in thread T5:
  #1 animptr::Delete()                         worldinc.h:166      (delete[])
  #2 ItemBase::~ItemBase()                     worldinc.h:708
  #3 ItemOverlayWP::~ItemOverlayWP()           worldinc.h:598
  #4 item::~item()                             worldinc.h:657
  #5 ItemBase::operator delete(void*)          World.cpp:535
  #6 ThreeDee::AddLensObject(...)              3dcode.cpp:1988     <-- frees here
  #7 shape::SunItemAnim(SunAnimData*)          3dcom.cpp:7726
  #8 shape::animate_shape(...)                 3dcom.cpp:1186
  #9 ThreeDee::GetVisibleObjects(...)          3dcode.cpp:4974
 #10 ThreeDee::render3d(...)                   3dcode.cpp:1307
 #11 ThreeDee::render(...)                     3dcode.cpp:3605
 #12 View3d::drawloop(void*)  [render thread T5]  STUB3D.CPP:1556
```

**Diagnosis.** The crash is in the **sun lens-flare object lifecycle** on the render thread —
**not** gun-fire/muzzle-flash as first assumed. `shape::SunItemAnim` animates the sun's lens object
every frame; `ThreeDee::AddLensObject` reads the lens `item` after it was freed (UAF at 1982-1983)
and then frees it again (double-free at 1988, on the same 3-byte region `0xeed00590`). So the lens
`item` is being deleted twice / used after delete across frames. It surfaces in Turkey Shoot because
its time-of-day/view puts the sun lens-flare in frame; the demo QM (index 0) ran too briefly / didn't
frame the sun. (ASan's "freed by" stack truncated on an internal `CHECK failed` — the *free* site
above is the actionable one; the matching prior free is the same `AddLensObject` path on an earlier
frame.)

**Why it's serious:** it's on the render thread during *normal* sun-lens rendering, so it likely
affects any mission/time-of-day that frames the sun lens-flare — not Turkey Shoot alone.

## Also surfaced (latent, separate)

ASan with `halt_on_error=1` stops first on a **startup heap-buffer-overflow** —
`fileman::translatedirlist` (FILEMAN.CPP:460), a 1-byte over-read while parsing the root dir list in
`InitFileSystem` → `loaddirlist` → `makefileblock`. Pre-existing, benign in the normal build (doesn't
crash), but real UB worth fixing. 8× heap-buffer-overflow + 4× heap-use-after-free total reported in
one continue-on-error run; the `AddLensObject` family is the one that crashes the normal build.

## S46 scope (next sprint)

1. **Fix the `AddLensObject` sun-lens double-free / UAF** (`3dcode.cpp:1982-1988`, with
   `SunItemAnim` 3dcom.cpp:7726 and the `item`/`animptr` lifecycle in `worldinc.h`). Root-cause the
   double ownership of the lens `item` (an item-pool / `animptr` ownership issue, likely
   `-fpack-struct`- or compat-threading-exposed, the R1.3d class). Verify with ASan: the
   `AddLensObject` family goes to 0; Turkey Shoot flies a full sortie without crashing.
2. **(Lower) translatedirlist over-read** (FILEMAN.CPP:460) — bound the dir-list parse read.

## Repro (ASan, deterministic)

```
cmake -S . -B build-asan -G Ninja -DBOB_ASAN=ON -DCMAKE_BUILD_TYPE=Release && ninja -C build-asan bob
cd "/home/m/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
ASAN_OPTIONS="halt_on_error=0:detect_leaks=0:detect_odr_violation=0:print_stacktrace=1" \
  BOB_RUN_INIT=1 BOB_DRIVE_C="/home/m/sgl/TUE/BattleOfBritain/WP/drive_c" DISPLAY=:0 \
  BOB_BOOT_FRONTEND=1 BOB_QM_INDEX=11 BOB_AUTOFLY=shoot /home/m/bob/build-asan/bob
```
Expect: the `AddLensObject` heap-use-after-free + double-free (3dcode.cpp:1982-1988) on thread T5.
