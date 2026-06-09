/* FreeFalcon Linux Port - MFC unity prelude, PART 1 (include BEFORE stdafx.h).
 *
 * Two things must happen before the MFC unity's stdafx.h:
 *  1. Select the file-enum groups: defining F_BATTLE makes files.g pull in
 *     F_COMMON.G (FIL_MAP_TABLE) + F_GRAFIX.G (map tiles) + F_SOUNDS.G and lock
 *     the FileNum enum, matching RDIALOG.H's intent for the dialog/map UI.
 *  2. Make bob's own CString complete: stdafx->afxwin.h defines __AFX_H__, which
 *     flips bob headers (savegame.h etc.) into their "MFC present" branch that
 *     declares CString members — and CString is bob's own (cstring.h), not MFC's.
 */
#if defined(BOB_LINUX) && !defined(F_BATTLE)
#define F_BATTLE
#include "dosdefs.h"
#include "files.g"
#include "cstring.h"
/* Define BFNUMBER_Included's bit-field types BEFORE the _mfc.h chain first
   reaches infoitem.h (via persons2.h): info_airgrp/info_waypoint are gated on
   #ifdef BFNUMBER_Included and use EventVal bit-field members. Without this the
   structs are forward-decl only and dereferencing them (RAF directives) fails.
   bfnumber.h pulls only bfenum.h, not the deferred (corrupt) bfrefs.g. */
#include "uniqueid.h"		/* UniqueID, used in bfnumber.h signatures */
#include "bfnumber.h"
#endif
