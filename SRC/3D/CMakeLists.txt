# 3D module - landscape/terrain, weapon-pack, tile streaming.
# OVERLAY.CPP deferred (needs the MFC CDC/CFont + GDI GetGlyphOutline buildout).
# 3D/3D.CPP and other unity aggregators excluded.
# NOTE: LSTRASM.ASM (hand x86) is assembled at final-link time, not here.
add_library(bob_3d STATIC
    LANDSCAP.CPP
    LSTREAM.CPP
    TILEMAKE.CPP
    WEAPPAK.CPP
)
target_include_directories(bob_3d PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
