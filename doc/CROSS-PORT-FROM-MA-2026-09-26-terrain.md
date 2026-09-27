# Cross-port from MiG Alley, 2026-09-26: TERRAIN-1 / TERRAIN-2

| MA | BoB | status |
|---|---|---|
| e2905a0 TERRAIN-1: GL exec path drew TLVERTEX with rhw ignored -> affine texturing; submit (x·w, y·w, z·w, w) | `draw_fvf` + `DEV_DrawIndexedPrimitiveVB` dropped rhw identically; same fix. `BOB_NO_PERSP`, `BOB_PERSP_FLIP_EVERY`, `BOB_TRACE_PERSP` | ✅ ported |
| e2905a0 hooks: `BOB_DUMP_FRAME_COUNT` (sequence dump) | same name in BoB (the shared `BOB_DUMP_*` family); works with `BOB_DUMP_FRAME` / `_AFTER_KEY` / `_ON_FIRE` | ✅ ported |
| 1faeee4 TERRAIN-2a: opaque 8-bit land never mip-mapped (port rule S154) | cause absent (BoB land is RGB565, upload follows the game chain) but the game builds no chain for land in QM: land now gets a GL chain, `BOB_NO_LANDMIP` | ✅ analogue |
| 1faeee4 TERRAIN-2b: honour D3DRENDERSTATE_TEXTUREADDRESS (land CLAMP) | already done by R3.6 (`D3DTSS_ADDRESS` -> GL wrap at draw) | ✅ pre-existing |
| MA swim instrument: unbounded lattice walk | hung BoB's draw thread on tiled textures; bounded in both ports | ✅ both |

Measured effect in BoB is 10-50x smaller than in MA (finer ground mesh) -- see scrum.md TERRAIN-1-BOB.
⚠ BoB-only finding for MA to check: BoB's `ApplyStateBlock` is a no-op (sticky stage state). MA's DX5 path
has no state blocks (execute buffers carry every state), so it should not be affected.
