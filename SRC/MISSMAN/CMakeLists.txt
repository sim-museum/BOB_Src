# MISSMAN module - mission/campaign management.
# 7 standalone TUs compile; PACKAGES/SAVEGAME/UIMSG deferred (per-file issues:
# non-self-contained rchatter.h, incomplete MissMan/CString, more for-scope).
# Unity aggregators (_MISS.CPP, MISSP.CPP) and zero-include fragments excluded.
add_library(bob_missman STATIC
    DEBRIEF.CPP
    INTRMISS.CPP
    NODEBOB.CPP
    NODEKILL.CPP
    ONEMISS.CPP
    PEACMISS.CPP
    SO51MISS.CPP
)
target_include_directories(bob_missman PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
