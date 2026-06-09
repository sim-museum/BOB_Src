# MISSMAN module - mission/campaign management.
# 9 standalone TUs compile; UIMSG still deferred.
# PACKAGES/SAVEGAME un-deferred via the same recipe: the BOB_LINUX prelude
# reproduces the MSVC PCH include-ordering so macro-gated declaration blocks are
# enabled (PACKAGE_INCLUDED gates Campaign/MissMan in missman2.h + EventVal
# methods in bfnumber.h; RANGES_Included gates Profile methods; CSTRING_Included
# gates SaveData::lastsavegame; BFNUMBER_Included gates info_itemS) + for-scope.
# Unity aggregators (_MISS.CPP, MISSP.CPP) and zero-include fragments excluded.
add_library(bob_missman STATIC
    DEBRIEF.CPP
    INTRMISS.CPP
    NODEBOB.CPP
    NODEKILL.CPP
    ONEMISS.CPP
    PEACMISS.CPP
    SO51MISS.CPP
    PACKAGES.CPP
    SAVEGAME.CPP
)
target_include_directories(bob_missman PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
