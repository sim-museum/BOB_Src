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
#include "AcUnit.h"		/* AircraftUnit (+TypesToList enum) */
#include "WPDialog.h"		/* WayPointDialog */
#include "RAFRevAs.h"		/* RAFReviewAsset */
#include "RAFRevAc.h"		/* RAFReviewAircraft */
#include "LWTaskSm.h"		/* LWTaskSummary */
#include "LWDiaryD.h"		/* LWDiaryDetails */
#include "LWDiary.h"		/* LWDiary */
#include "SquadDtl.h"		/* SquadronDetails */
#include "GrpGesch.h"		/* GroupGeschwader */
#include "AfDetl.h"		/* AirfieldDetails */
#include "Load.h"		/* LSD_State enum (LSD_LOAD/LSD_SAVE/...) */
#include "MapFltLw.h"		/* MapFiltersRaidsLW */
/* _TOOL is the top-level toolbar/navigator: it instantiates ~all campaign
   dialogs, so it needs every dialog class declared. */
#include "AcUnitRF.h"
#include "acalloc.h"
#include "AfDossr.h"
#include "BasesLft.h"
#include "AUTHOR.H"
#include "clock.h"
#include "CntrlTop.h"
#include "SUPPLY.H"
#include "WEATHER.H"
#include "DirRsult.h"
#include "DirNoRes.h"
#include "Dossierb.h"
#include "GWadlist.h"
#include "Hostiles.h"
#include "IntOff.h"
#include "LuftFlot.h"
#include "LWDirect.h"
#include "LWMssFr.h"
#include "LWRevTp.h"
#include "LWRouteT.h"
#include "LWTaskTp.h"
#include "MFTop.h"
#include "PltLogBk.h"
#include "RAFCombt.h"
#include "RAFDiary.h"
#include "RAFDrRes.h"
#include "RAFDir.h"
#include "RAFMssFr.h"
#include "RAFRevTp.h"
#include "RAFRoutT.h"
#include "RAFSqLst.h"
#include "RAFTaskT.h"
#include "SelTrg.h"
#include "TOOff.h"
#include "TlBrCntl.h"
#include "ToteSect.h"
#include "ZoomLevl.h"
/* _FULL (full-screen panel navigator) dialog classes */
#include "SQUICKUN.H"		/* CSQuickLine */
#include "redtbt.h"		/* CREdtBt */
#include "VwQList.h"		/* CViewQList */
#include "MTChild.h"		/* EmptyChildWindow */
#include "QuickPar.h"		/* QuickParameters */
#include "sQuickP.h"		/* QuickMissionPanel */
#include "CommChat.h"		/* CommsChat */
#include "Locker.h"		/* CLockerRoom */
#include "CurrEmbl.h"		/* CCurrEmblem */
#include "CampName.h"		/* CampaignEnterName */
#include "BoBFrag.h"		/* BoBFrag */
#include "APILOT.H"		/* CAutoPilot */
#include "CommsAc.h"		/* CCommsDeathMatchAc */
#include "CommsPnt.h"		/* CCommsPaint */
#include "CntrlFly.h"		/* ControlFly */
#include "SQView.h"		/* CQuickView */
#include "Radio.h"		/* CRadio */
#include "Ready.h"		/* CReadyRoom */
#include "SCAMP.H"		/* CSCampaign */
#include "SDETAIL.H"		/* CSDetail */
#include "SFLIGHT.H"		/* CSFlight */
#include "SGAME.H"		/* CSGame */
#include "service.h"		/* CSelectService */
#include "session.h"		/* CSelectSession */
#include "SSOUND.H"		/* CSSound */
#include "SVIEWER.H"		/* CSViewer */
#include "Visitors.h"		/* CVisitorsBook */
#include "EndDayRL.h"		/* EndOfDayReviewList */
#include "GameSelt.h"		/* GameSelect */
#include "SControl.h"		/* SController */
#include "SMission.h"		/* SMissionConfigure */
#include "TwoDPref.h"		/* TwoDPref */
#include "SideSel.h"		/* SideSelect */
#include "PhsDscr.h"		/* PhaseDescription */
#include "EndDyBmp.h"		/* EndofDayReviewBmp */
#include "EndDayRv.h"		/* EndOfDayReviewText */
#endif
