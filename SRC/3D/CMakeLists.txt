# 3D module - landscape/terrain, weapon-pack, tile streaming.
# OVERLAY.CPP un-deferred: MFC-context TU (afxwin/afxctl) — needs cstring.h before
# <afxwin.h> (so bob's CString is complete before __AFX_H__ flips it); the MapScr
# screen-state table is bare member-fn-ptrs -> &MapScr:: (guarded perl); the GDI
# GetGlyphOutline/GLYPHMETRICS/MAT2 glyph API is stubbed in compat_wingdi.h
# (returns 0 -> blank overlay text for now); F_BATTLE-only file-enum group.
# 3D/3D.CPP and other unity aggregators excluded.
# NOTE: LSTRASM.ASM (hand x86) is assembled at final-link time, not here.
add_library(bob_3d STATIC
    LANDSCAP.CPP
    LSTREAM.CPP
    TILEMAKE.CPP
    WEAPPAK.CPP
    OVERLAY.CPP
)
target_include_directories(bob_3d PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
