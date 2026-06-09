/* FreeFalcon Linux Port - MFC unity prelude, PART 2 (include AFTER stdafx.h + _mfc.h,
 * before the unity's .cpp fragments).
 *
 * bob's UI base classes / data enums, included early so every fragment sees their
 * full definitions (the dialog headers that use them, e.g. MapDlg.h/FullPane.h,
 * don't self-include them; on Windows the PCH/build order covered this). Leaf
 * ActiveX-control wrappers first, composites after. Idempotent (include guards),
 * so safe to include from every MFC unity (_MFC/_AFX/_TOOL/_FULL/_SA/_LW/_RAF).
 */
#if defined(BOB_LINUX)
#include "uniqueid.h"
#include "resource.h"		/* dialog IDD/IDC/IDS symbols (control headers' enum{IDD=...}) */
/* leaf ActiveX-control wrappers FIRST (composites below derive from these) */
#include "rbutton.h"
#include "rstatic.h"
#include "rcombo.h"
#include "rspinbut.h"
#include "rlistbox.h"
#include "rtabs.h"
#include "rspltbar.h"
#include "rscrlbar.h"
/* composites / dialogs / bars that use the leaf controls */
#include "rcombox.h"
#include "rdialog.h"
#include "rmdldlg.h"
#include "listbx.h"
#include "hintbox.h"
#include "maintbar.h"
#include "titlebar.h"
#include "sysbox.h"
#include "rradio.h"
#include "rtestsh1.h"
#include "squick1.h"
#include "thumnail.h"
#include "filing.h"
#include "movement.h"
#include "savegame.h"		/* MapFilters enum incl. MAPFILTERSMAX */
#include "mapfltrs.h"
#include "intelmsg.h"
#include "radcomms.h"
/* frame / view / panel headers (MainToolBar()/TitleBarPtr()/CMIGView/
   RFullPanelDial used across the campaign-UI unities) */
#include "redit.h"
#include "fullpane.h"
#include "MainFrm.h"
#include "MIGView.h"
/* info_airgrp/info_waypoint full defs (forward-declared in package.h; the
   RAF directives fragments dereference them) */
#include "infoitem.h"
/* cross-unity dialog classes referenced from sibling fragments before their
   own .cpp is included in this unity (RAF refs LW dialogs; RAFDiary refs
   RAFDiaryDetails defined later in the same unity) */
#include "LWRouteM.h"
#include "LWRevAc.h"
#include "RAFDryD.h"
#endif
