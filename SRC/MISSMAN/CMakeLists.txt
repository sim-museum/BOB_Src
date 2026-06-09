# MISSMAN module - mission/campaign management.
# 8 standalone TUs compile; SAVEGAME/UIMSG still deferred.
# PACKAGES un-deferred: needed ranges.h->package.h->bfnumber.h include ordering
# (RANGES_Included gates Profile::AddAttackWP/InsertWpBetween/AddNumerousEscorts;
# PACKAGE_INCLUDED gates EventVal::MakeIcptGRExpr) + mytime/missman2 + for-scope.
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
)
target_include_directories(bob_missman PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
