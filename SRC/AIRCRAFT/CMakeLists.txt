# AIRCRAFT module - per-type flight-model / collision data tables.
# _AIRC.CPP is a unity aggregator (#includes the Dt_*/Cd_BOB files); excluded.
add_library(bob_aircraft STATIC
    AIRCRAFT.CPP
    CD_BOB.CPP
    DT_BF109.CPP
    DT_BF110.CPP
    DT_HURRI.CPP
    DT_JU87B.CPP
    DT_OTHER.CPP
    DT_SPITF.CPP
)
target_include_directories(bob_aircraft PUBLIC
    ${BOB_SRC}/compat
    ${BOB_SRC}/H
)
