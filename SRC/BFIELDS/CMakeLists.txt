# BFIELDS module - bit-field / persons / save-binary / global-refs serialization.
# _BFIE.CPP is the unity that #includes Globrefs/Mapcode/Messages/Persons2/3/4/7/
# qmiss/Savebin/Tank — the Persons2/3/4/EventVal serialization layer.
#
# Linux-port fixes that made it build:
#  - bfrefs.g (MAKEBF-generated, was corrupt): removed 2 spurious `NULL};` that
#    split the single BFieldGlobalTable[] + restored 2 truncated entries; mapped
#    the bogus GR_Pack_TakeTime_W?G? names to the declared GR_Pack_TakeTime[w][g];
#    GR_Pack_TakeTime un-deadcoded so it links (zero-filled, runtime-harmless).
#    NOTE: table order/content drives save/replay IDs — verify vs an original.
#  - array-typed GR_ globals (globrefs.h no-`pass` branch) extern-declared ahead
#    of bfrefs.g (this TU only includes globrefs.h with pass 1/2).
#  - SAVEBIN `struct _iobuf*` -> FILE*; batnode.h made self-contained (enumbits.m
#    for BITABLE); implicit-int (BAD_RV, static go); bool++ -> =true; EventVal&
#    temp-bind via a named local; for-scope hoists.
add_library(bob_bfields STATIC
    _BFIE.CPP
)
target_include_directories(bob_bfields PUBLIC ${BOB_SRC}/compat ${BOB_SRC}/H)
