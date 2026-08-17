/*
	 Battle of Britain
	 Copyright (C) 2000, 2001 Empire Interactive (Europe) Ltd,
	 677 High Road, North Finchley, London N12 0DA

	 Please see the document licence.doc for the full licence agreement

2. LICENCE
 2.1 	
 	Subject to the provisions of this Agreement we now grant to you the 
 	following rights in respect of the Source Code:
  2.1.1 
  	the non-exclusive right to Exploit  the Source Code and Executable 
  	Code on any medium; and 
  2.1.2 
  	the non-exclusive right to create and distribute Derivative Works.
 2.2 	
 	Subject to the provisions of this Agreement we now grant you the
	following rights in respect of the Object Code:
  2.2.1 
	the non-exclusive right to Exploit the Object Code on the same
	terms and conditions set out in clause 3, provided that any
	distribution is done so on the terms of this Agreement and is
	accompanied by the Source Code and Executable Code (as
	applicable).

3. GENERAL OBLIGATIONS
 3.1 
 	In consideration of the licence granted in clause 2.1 you now agree:
  3.1.1 
	that when you distribute the Source Code or Executable Code or
	any Derivative Works to Recipients you will also include the
	terms of this Agreement;
  3.1.2 
	that when you make the Source Code, Executable Code or any
	Derivative Works ("Materials") available to download, you will
	ensure that Recipients must accept the terms of this Agreement
	before being allowed to download such Materials;
  3.1.3 
	that by Exploiting the Source Code or Executable Code you may
	not impose any further restrictions on a Recipient's subsequent
	Exploitation of the Source Code or Executable Code other than
	those contained in the terms and conditions of this Agreement;
  3.1.4 
	not (and not to allow any third party) to profit or make any
	charge for the Source Code, or Executable Code, any
	Exploitation of the Source Code or Executable Code, or for any
	Derivative Works;
  3.1.5 
	not to place any restrictions on the operability of the Source 
	Code;
  3.1.6 
	to attach prominent notices to any Derivative Works stating
	that you have changed the Source Code or Executable Code and to
	include the details anddate of such change; and
  3.1.7 
  	not to Exploit the Source Code or Executable Code otherwise than
	as expressly permitted by  this Agreement.

questions about this file may be asked at bob@rowansoftware.com a
better place to ask is http://www.simhq.com/ or even :-
http://www.simhq.com/cgi-bin/boards/cgi-bin/forumdisplay.cgi?action=topics&forum=Battle+of+Britain&number=40&DaysPrune=20&LastLogin=
*/

//////////////////////////////////////////////////////////////////////
//
// Module:      FullPSys.cpp
//
// Created:     03/12/99 by JIM
//
// Description: The system part of FULLPANE dialogs
//
//////////////////////////////////////////////////////////////////////
#define F_SOUNDS
#define F_BATTLE
#define F_COMMON
#include "stdafx.h"
#include "mig.h"
#include "stdlib.h"
#include "FullPane.h"
#include "Rtestsh1.h"
#include "messages.h"
#include "MIGView.h"
#include "replay.h"
#include "package.h"   /* R4.3 (BOB_CAMPAIGN_FLY): Todays_Packages / Profile for the map->frag seam */
#include "misssub.h"   /* R4.3 (BOB_CAMPAIGN_FLY): Directives / RAFDirectivesResults::RAFInterceptType */
#include "filing.h"    /* R4.4 (BOB_CAMPAIGN_SAVE): CFiling::SaveGame */
#include "bstream.h"   /* R4.5 (S44): BOStream for the pre-fly Package.dat write (mirrors OnClickedFrag2) */
#include "winmove.h"
#include "aggrgtor.h"
#include "stub3d.h"
#include "vfw.h"
#include	"miles.h"													//RJS 18Oct00

//DeadCode JON 10Nov00 extern HINSTANCE	g_hinst;
//DeadCode JON 10Nov00 extern HWND		g_hwndMCIWnd;

#define INITGUID
#include <streams.h>
#define WM_GRAPHNOTIFY  WM_USER+13										//JON 10Nov00
#define HELPER_RELEASE(x) { if (x) x->Release(); x = NULL; }			//JON 10Nov00

IGraphBuilder *pigb  = NULL;											//JON 10Nov00
IMediaControl *pimc  = NULL;											//JON 10Nov00
IMediaEventEx *pimex = NULL;											//JON 10Nov00
IVideoWindow  *pivw  = NULL;											//JON 10Nov00
IBasicAudio   *videoAudio = NULL;										//JON 10Nov00

const SLong videoWidth = 384;											//JON 12Nov00
const SLong videoHeight = 288;											//JON 12Nov00

int RFullPanelDial::resolutions[]={800,800,1024};//,1280};



RFullPanelDial::RFullPanelDial(CWnd* pParent /*=NULL*/)
	: RowanDialog(RFullPanelDial::IDD, pParent)
{
	//{{AFX_DATA_INIT(RFullPanelDial)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_currentscreen=NULL;
	whichcamp=0;
	pdial[0]=NULL;
	pdial[1]=NULL;
	pdial[2]=NULL;
	m_timerID=NULL;
	m_currentres=-1;
}

RFullPanelDial::~RFullPanelDial()
{
	m_pView->m_pfullpane=NULL;

}



void RFullPanelDial::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RFullPanelDial)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX,IDC_RLISTBOX,m_IDC_RLISTBOX);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(RFullPanelDial, CDialog)
	//{{AFX_MSG_MAP(RFullPanelDial)
	ON_WM_TIMER()
	ON_WM_LBUTTONUP()
	ON_WM_CREATE()
	ON_WM_ENABLE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_MESSAGE_CLASS( WM_GETGLOBALFONT,RFullPanelDial,OnGetGlobalFont)
	ON_MESSAGE_CLASS( WM_GETARTWORK,RFullPanelDial, OnGetArt)
	ON_MESSAGE_CLASS( WM_SELECTTAB,RFullPanelDial, OnMciDidQuit)
//DeadCode JON 10Nov00 	ON_MESSAGE_CLASS( 0x04c8, RFullPanelDial, OnMciNotify)
	ON_MESSAGE_CLASS( WM_GRAPHNOTIFY, RFullPanelDial, OnMciNotify)				//JON 10Nov00
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// RFullPanelDial message handlers


#if BOB_LINUX
extern "C" void bob_gdi_present(void);                              /* present (bob_video.cpp) */
extern "C" int  bob_gdi_text(int,int,const char*,int,unsigned);    /* TTF text (bob_gdi_font.cpp) */
extern "C" int  bob_gdi_text_width(const char*,int);
extern "C" void bob_gdi_set_face(int);                             /* S131: current text face (bob_gdi_font.cpp) */
extern "C" int  bob_gdi_get_click(int*,int*);                      /* pending left-click (bob_video.cpp) */
/* Linux port: drive the genuine CRListBoxCtrl::OnDraw (BOB_OLE_DRAW) instead of the
   hand-drawn menu text, to render the hosted ActiveX control's real layout. */
extern "C" int bob_ole_draw_listbox(CWnd* wrapper, int x, int y, int w, int h, int textH);
extern "C" int bob_ole_draw_panel(CWnd* dialog, int ox, int oy);
extern "C" int bob_ole_click(CWnd* dialog, int x, int y);
extern "C" int bob_ole_ctrl_point(CWnd* dialog, int id, int col, int* px, int* py);  /* S141 */
extern "C" void bob_ole_dump_template_membership(int dlgId);  /* S146 (SP.8 probe) */
extern "C" void bob_ole_dump_drawn_rects(CWnd* dialog);       /* S160: hit-test target dump */
extern "C" void bob_gdi_restore_mode(void);                   /* S162: back to the front-end mode */
extern "C" int  bob_ole_col_rect(CWnd* ctrl, int col, int* x, int* y, int* w, int* h);  /* S160 */
static int g_bobOleDraw = -1;
static int bobOleDraw() { if (g_bobOleDraw < 0) g_bobOleDraw = getenv("BOB_OLE_DRAW") ? 1 : 0; return g_bobOleDraw; }

/* The active front-end + its drawn menu-item rects, so a click can be mapped to an item. */
static RFullPanelDial* g_bobActiveFP = NULL;
static struct { int x,y,w,h; } g_bobMenuRect[10];
static int g_bobMenuN = 0;
long g_bob_shot_t = 0;   /* SP.1 (BOB_SHOT): one-shot parity-capture tick counter (map + front-end) */
/* S145 (SP.7, from MA note 29 section 3): BOB_SHOT counts ABSOLUTE ticks, so it cannot be
   aimed at a state whose arrival time varies -- the capture lands wherever the run happens to
   be. That is not theoretical: S144's map capture drifted THREE campaign days past the state
   its verdict would have claimed (caught only by the S143 state banner). BOB_SHOT_AFTER=<n>
   instead captures n paints after a drive scaffold calls bob_shot_arm() -- i.e. relative to
   the EVENT the recipe cares about, so queueing, lock waits and frame-rate cannot move it. */
long g_bob_shot_armed = -1;   /* -1 = not armed; >=0 = paints since arming */
extern "C" void bob_shot_arm(const char* why) {
	if (g_bob_shot_armed >= 0) return;   /* first arm wins; re-arming would reintroduce drift */
	g_bob_shot_armed = 0;
	fprintf(stderr, "[shot] ARMED by %s (BOB_SHOT_AFTER=%s)\n",
		why ? why : "?", getenv("BOB_SHOT_AFTER") ? getenv("BOB_SHOT_AFTER") : "(unset)");
	fflush(stderr);
}
/* S148 (SP.16): capture on a GAME-STATE PREDICATE rather than a count.
   S145's BOB_SHOT_AFTER fixed drift caused by QUEUEING (arm from the drive, not an absolute tick).
   S147 found the next layer and it defeats every counter: the dismiss scaffold suppresses the
   prompts the campaign would otherwise block on, so the sim runs FASTER -- the same recipe landed
   on 12 Aug in S146 and on 13 Aug twice in S147, at two different paint counts. The harness
   changed the quantity the timing depended on. The only stable answer is to describe the STATE we
   want and shoot when it arrives:
     BOB_SHOT_WHEN=clear    -- no logged child on ANY toolbar (nothing covering the map)
     BOB_SHOT_DATE=<n>      -- and MMC.currdate == n            (optional extra term)
     BOB_SHOT_TIME_LT=<n>   -- and MMC.currtime <  n            (optional extra term)
   Every term is something the frame itself displays, so the verdict stays checkable against the
   pixels rather than resting on the recipe's word. */
extern "C" int bob_oob_any_dialog_open(void);
static bool bob_shot_state_ok(void) {
	const char* when = getenv("BOB_SHOT_WHEN");
	if (!when || !*when) return true;
	if (bob_oob_any_dialog_open()) return false;          /* "clear": map unobstructed */
	const char* wd = getenv("BOB_SHOT_DATE");
	if (wd && *wd && (long)MMC.currdate != atol(wd)) return false;
	/* S148: time-of-day term. Gold #19 is 12 Aug *06:31* -- raids OUTBOUND, route lines and raid
	   stacks on the map. The first CLEAR paint on that day came at 20:58, by which time the footer
	   reads "Geschwader Landed" and the routes are gone: the map is obstructed precisely WHILE raids
	   fly (that is when the game prompts) and clear precisely when they do not. So the state we want
	   is a morning one, and it has to be said out loud rather than hoped for.
	   MMC.currtime is seconds-of-day (06:31 = 23460; measured 28340 at 07:52, 75520 at 20:58). */
	const char* wt = getenv("BOB_SHOT_TIME_LT");
	if (wt && *wt && (long)MMC.currtime >= atol(wt)) return false;
	return true;
}

/* Shared by both capture sites: has the shot condition been met this paint? */
static bool bob_shot_due(void) {
	if (!bob_shot_state_ok()) return false;   /* S148: never shoot in the wrong state */
	const char* after = getenv("BOB_SHOT_AFTER");
	if (after && *after) {
		if (g_bob_shot_armed < 0) return false;          /* drive has not fired yet */
		return ++g_bob_shot_armed >= atol(after);
	}
	const char* abs_ = getenv("BOB_SHOT");
	if (abs_ && *abs_) return ++g_bob_shot_t >= atol(abs_);
	/* BOB_SHOT_WHEN on its own: the state IS the trigger -- shoot the first paint it holds. */
	return getenv("BOB_SHOT_WHEN") != NULL;
}
/* S143 (SP.9, adopted from FreeFalcon note 15): "a parity capture must record the state it
   claims to be capturing." Without this, a mis-selected screen and a genuine render bug are
   indistinguishable in the output — FF spent a sprint on a deviation whose recorded mechanism
   was invented because nothing logged which view was actually in force, and BoB's own S142
   Directives verdict was corroborated only by luck (the phase happened to be rendered into the
   frame). Always-on, one line, emitted right before every BOB_SHOT dump. */
int g_bob_autoclick_seq = 0;      /* how many BOB_AUTOCLICK steps have actually FIRED */
extern "C" int bob_ole_state_summary(char* out, int outsz);
static void bob_shot_state_banner(const char* where)
{
	char panels[256]; panels[0] = 0;
	int ndlg = bob_ole_state_summary(panels, (int)sizeof(panels));
	/* S146 (SP.8): with BOB_TRACE_DIR=<dlgId>, dump that dialog's template membership once. */
	{ const char* td = getenv("BOB_TRACE_DIR");
	  if (td && atoi(td) > 1) bob_ole_dump_template_membership(atoi(td)); }
	/* S160: dump EVERY hit-test target on this screen -- the menu rects and each panel's hosted
	   controls. User-reported (2026-08-09): the Fly/Back buttons on Quick Mission and Begin/Back in
	   campaign do nothing, and their clicks produce NO trace line at all, while combos on the same
	   screen log "combo cycled". A click that matches nothing is the S156 class (never arrives),
	   not the §8z class (arrives, no handler) -- opposite fixes, so establish which before touching
	   anything. If no target covers the button's pixels, that is the answer. BOB_DUMP_HITTARGETS=1 */
	if (getenv("BOB_DUMP_HITTARGETS")) {
		fprintf(stderr, "[hittargets] menu rects: n=%d\n", g_bobMenuN);
		for (int i = 0; i < g_bobMenuN; i++)
			fprintf(stderr, "[hittargets]   menu[%d] = (%d,%d %dx%d)\n",
				i, g_bobMenuRect[i].x, g_bobMenuRect[i].y, g_bobMenuRect[i].w, g_bobMenuRect[i].h);
		if (g_bobActiveFP)
			for (int d = 0; d < 3; d++)
				if (g_bobActiveFP->pdial[d]) {
					fprintf(stderr, "[hittargets] panel %d hosted controls:\n", d);
					bob_ole_dump_drawn_rects((CWnd*)g_bobActiveFP->pdial[d]);
				}
	}
	const char* ac = getenv("BOB_AUTOCLICK");
	int acTotal = 0; if (ac && *ac) { acTotal = 1; for (const char* p = ac; *p; p++) if (*p == ',') acTotal++; }
	long art = -1; int resw = 0, phase = -1;
	if (g_bobActiveFP) {
		phase = g_bobActiveFP->whichcamp;
		if (g_bobActiveFP->m_currentscreen) {
			art  = (long)g_bobActiveFP->m_currentscreen->resolutions[g_bobActiveFP->m_currentres].artwork;
			resw = RFullPanelDial::resolutions[g_bobActiveFP->m_currentres];
		}
	}
	/* NB `fe_art` is the FRONT-END screen's resolved art FileNum; on the map path it is the last
	   front-end screen, not the map's own art. Named so the banner can't imply more than it knows —
	   a state line that overstates is the failure mode it exists to prevent (cf. BoB S101: a
	   diagnostic that lies is worse than no diagnostic). */
	fprintf(stderr,
		"[shot-state] where=%s tick=%ld fe_art=%ld resw=%d phase=%d date=%d time=%d "
		"autoclick=%d/%d dialogs=%d %s\n",
		where, g_bob_shot_t, art, resw, phase,
		(int)MMC.currdate, (int)MMC.currtime,
		g_bob_autoclick_seq, acTotal, ndlg, panels);
	fflush(stderr);
}

/* R4.1: the Campaigns side-select screen (RFullPanelDial::singleplayer -> SinglePlayerInit ->
   SideSelect dialog). Its choices are not text menu items but POLYGON hit-areas over the
   RAF/Luftwaffe art -- the faithful regions are SideSelect::SideSelectOutlines (SIDESEL.CPP),
   authored at 1024x768; a click in a polygon fires OnSelectRlistbox(index) (== the real
   OnClickedRaf/Luftwaffe/Cancel -> FullPanel()->OnSelectRlistbox path). The text-menu loop in
   bob_draw_menu breaks on the screen's NULL captions, so without this the screen is unnavigable.
   We replicate the polygons here (1024x768, scaled to the live res) + the standard ray-cast test. */
struct BobPoly { int n; int pt[6][2]; int item; };
static const BobPoly g_sideSelectPolys[] = {
	{ 4, {{0,0},{700,0},{300,650},{0,650}},                 0 },  /* RAF    -> item 0 */
	{ 5, {{1024,0},{1024,768},{300,768},{300,650},{700,0}}, 1 },  /* LW     -> item 1 */
	{ 4, {{0,650},{300,650},{300,768},{0,768}},             2 },  /* Cancel -> item 2 */
};
static bool bob_pt_in_poly(int x, int y, const int pt[][2], int n) {
	bool in=false;
	for (int i=0,j=n-1; i<n; j=i++) {
		int xi=pt[i][0], yi=pt[i][1], xj=pt[j][0], yj=pt[j][1];
		if (((yi>y)!=(yj>y)) && (x < (long)(xj-xi)*(y-yi)/(double)(yj-yi)+xi)) in=!in;
	}
	return in;
}
/* Hit-test a click (live framebuffer coords) against the side-select art polygons, scaled from
   1024x768 to the current resolution (4:3). Returns the OnSelectRlistbox item index, or -1. */
static int bob_sideselect_hit(int cx, int cy, int resW) {
	double sx = resW/1024.0, sy = (resW*3/4)/768.0;
	for (unsigned k=0; k<sizeof(g_sideSelectPolys)/sizeof(g_sideSelectPolys[0]); k++) {
		int scaled[6][2]; const BobPoly& P=g_sideSelectPolys[k];
		for (int i=0;i<P.n;i++){ scaled[i][0]=(int)(P.pt[i][0]*sx); scaled[i][1]=(int)(P.pt[i][1]*sy); }
		if (bob_pt_in_poly(cx,cy,scaled,P.n)) return P.item;
	}
	return -1;
}
/* Centroid of side-select polygon `item` (live res) -- the synthetic AUTOCLICK target so the
   headless campaign-nav test reaches RAF/LW/Cancel without a display. */
static bool bob_sideselect_centroid(int item, int resW, int* ox, int* oy) {
	double sx = resW/1024.0, sy = (resW*3/4)/768.0;
	for (unsigned k=0; k<sizeof(g_sideSelectPolys)/sizeof(g_sideSelectPolys[0]); k++) {
		const BobPoly& P=g_sideSelectPolys[k];
		if (P.item!=item) continue;
		long mx=0,my=0; for(int i=0;i<P.n;i++){ mx+=P.pt[i][0]; my+=P.pt[i][1]; }
		*ox=(int)(mx/P.n*sx); *oy=(int)(my/P.n*sy); return true;
	}
	return false;
}
static bool bob_on_sideselect() {
	return g_bobActiveFP && g_bobActiveFP->m_currentscreen == &RFullPanelDial::singleplayer;
}
/* Draw the screen's menu items (textlists) with the game's font, centred in the left column
   to match the wine reference. Records each item's rect for click hit-testing. Bring-up:
   bypasses the OLE RListBox layout/highlight. */
static void bob_draw_menu(RFullPanelDial* self, FullScreen* scr, int resW)
{
	g_bobActiveFP = self; g_bobMenuN = 0;
	if (!scr) return;
	bob_gdi_set_face(0);   /* S131: the menu / Back-Begin-Fly rows draw directly in the ART
	                          (Rowan stencil) face -- reset in case a prior control draw left
	                          the shared face non-ART. The hosted listbox draw sets its own. */
	bool horiz = (scr->listalign & FullScreen::HORIZ) != 0;
	int fh = resW * 36 / 1000; if (fh < 16) fh = 16;        /* ~37px at 1024 wide (matches wine) */
	/* SP.2 (S123): anchor the menu list at the screen's own per-resolution ListX/ListY --
	   the layout data the Windows build positions its RListBox with (FullScreen::Resolutions).
	   E.g. campaignselect@1024 = (35,710): Back/Begin bottom-left (matches the gold shots,
	   which the old synthetic top-centre/left-column-quarter anchors did not); title = (210,220);
	   config tab rows = (10,10). Degenerate values fall back to the old synthetic anchors;
	   BOB_NO_LISTXY reverts wholesale. */
	int lx = scr->resolutions[self->m_currentres].ListX;
	int ly = scr->resolutions[self->m_currentres].ListY;
	bool useXY = !getenv("BOB_NO_LISTXY")
	             && (lx > 0 || ly > 0) && lx >= 0 && ly >= 0 && lx < resW && ly < resW * 3 / 4;
	if (horiz) {
		/* horizontal row (config tab rows at the top; Back/Begin/Fly rows at the bottom) */
		int gap = resW * 14 / 1000, y = useXY ? ly : resW * 6 / 1000;
		/* The OLE listbox draws the row at oleH (the list font), so measure widths/centring/
		   hit-rects at oleH too -- NOT fh. (First-pilot bug: rects measured at fh, ~38% wider
		   than the drawn text, drifted each item right -> the last item ("Fly") landed ~1cm
		   right of its text, so clicks missed. Use one font height for draw + hit-test.) */
		int oleH = resW * 26 / 1000; if (oleH < 16) oleH = 16;   /* resolution-scaled list font height */
		/* measure the items */
		int wids[10]; int sumw = 0, n = 0;
		for (; n < 10 && scr->textlists[n].text; n++) {
			CString s; s.LoadString(scr->textlists[n].text);
			wids[n] = bob_gdi_text_width((const char*)(LPCSTR)s, oleH);
			sumw += wids[n];
		}
		/* NB: the gold shots' config tab bars are additionally SPREAD across the full width
		   (PositionRListBox column spacing); not reproduced here because the hosted control
		   draws its own tight-packed columns -- widening only the hit-rects would desync them
		   from the drawn text (the first-pilot hit-box bug class). Documented deviation. */
		int total = sumw + gap * n;   /* keep old width semantics for the OLE box */
		int x = useXY ? lx : (resW - total) / 2; if (x < 8) x = 8;
		/* S139: the CRListBoxCtrl lays its columns at its OWN colwidths (AddColumn/ResizeToFit in
		   PositionRListBox) but ExtTextOut-clips each to the passed rcBounds width; our tight
		   `total` (sum of re-measured text widths) is narrower than the listbox's internal column
		   spread (per-column m_horzSeperation), so a 3rd column clips off -- e.g. bobfrag's
		   Back/Sim Config/**Fly** footer showed only the first two (gold #3). The clip width only
		   gates visibility -- column POSITIONS are internal to the listbox and hit-rects come from
		   wids[] below -- so widen the clip to the remaining screen width: previously-clipped
		   columns appear, nothing moves. BOB_NO_FOOTER_CLIP reverts to the tight `total`. */
		int clipw = total;
		if (!getenv("BOB_NO_FOOTER_CLIP")) { clipw = resW - x - 8; if (clipw < total) clipw = total; }
		/* Real ActiveX control render: draw the hosted CRListBoxCtrl (multi-column
		   tab row) and keep click rects from the same oleH-measured widths. */
		if (bobOleDraw() && bob_ole_draw_listbox((CWnd*)&self->m_IDC_RLISTBOX, x, y, clipw, fh + fh/2, oleH)) {
			/* S160 (user-reported): take each item's click rect from the column the listbox
			   ACTUALLY DREW, not from a re-measurement. The hosted CRListBoxCtrl lays its columns
			   out internally (PositionRListBox / m_horzSeperation) while wids[] below is
			   bob_gdi_text_width() packed with a fixed `gap` — two independent layouts that drift.
			   Measured on Quick Mission before this fix: "Fly" painted x=132..153 vs click rect
			   x=83..108 (NO overlap — the button could not be clicked at all), "Back" painted
			   56..97 vs rect 25..69 (only its leftmost 14px responded). Same class as S156's
			   OOB hit-testing and MA §8-MA84 trap #1: store what paint did, never re-derive it.
			   Falls back to the old computation per item if the column is not drawn.
			   BOB_NO_MENU_DRAWNRECT reverts. */
			int rx = x;
			int useDrawn = !getenv("BOB_NO_MENU_DRAWNRECT");
			for (int i = 0; i < n; i++) {
				int cx, cy, cw, ch;
				if (useDrawn && bob_ole_col_rect((CWnd*)&self->m_IDC_RLISTBOX, i, &cx, &cy, &cw, &ch)) {
					g_bobMenuRect[i].x=cx; g_bobMenuRect[i].y=cy; g_bobMenuRect[i].w=cw; g_bobMenuRect[i].h=ch;
				} else {
					g_bobMenuRect[i].x=rx; g_bobMenuRect[i].y=y; g_bobMenuRect[i].w=wids[i]; g_bobMenuRect[i].h=fh+fh/2;
				}
				rx += wids[i] + gap;
			}
			g_bobMenuN = n;
			return;
		}
		for (int i = 0; i < n; i++) {
			CString s; s.LoadString(scr->textlists[i].text);
			const char* t = (const char*)(LPCSTR)s;
			int w = bob_gdi_text_width(t, fh);
			bob_gdi_text(x + 2, y + 2, t, fh, 0x301800);
			bob_gdi_text(x,     y,     t, fh, 0xF0C030);
			g_bobMenuRect[i].x=x; g_bobMenuRect[i].y=y; g_bobMenuRect[i].w=w; g_bobMenuRect[i].h=fh+fh/2;
			x += w + gap;
		}
		g_bobMenuN = n;
		return;
	}
	int cx = useXY ? lx : resW / 4;                         /* column centre (title: 210 @1024) */
	int my = useXY ? ly : resW * 23 / 100;
	int step = resW * 35 / 1000;                            /* ~36px pitch */
	for (int i = 0; i < 10; i++) {
		if (scr->textlists[i].text == NULL) break;
		CString s; s.LoadString(scr->textlists[i].text);
		const char* t = (const char*)(LPCSTR)s;
		int w = bob_gdi_text_width(t, fh);
		int x = cx - w/2, y = my + i*step;
		bob_gdi_text(x + 2, y + 2, t, fh, 0x301800);    /* shadow */
		bob_gdi_text(x,     y,     t, fh, 0xF0C030);    /* gold   */
		g_bobMenuRect[i].x=x; g_bobMenuRect[i].y=y; g_bobMenuRect[i].w=w; g_bobMenuRect[i].h=step;
		g_bobMenuN = i+1;
	}
}
extern "C" void bob_gdi_setdibits_origin(int, int);
/* S133 (probe, BOB_TRACE_OOBTREE): dump the nested RDialog child tree of a panel so we can
   see the geometry model (OnGetXYOffset vs viewsize vs GetWindowRect) before wiring the real
   nested draw. The QS order-of-battle panel (QuickMissionBlue/Red -> QuickMissionPanel with a
   DialList of CSQuickLine rows) nests controls whose parentDlg != pdial[d], so the standard
   bob_ole_draw_panel(pdial[d]) never draws them. */
static void bob_oobtree_probe(RDialog* n, int depth)
{
	for (; n; n = n->sibling) {
		long off = n->OnGetXYOffset();
		CRect wr(0,0,0,0); n->GetWindowRect(&wr);
		fprintf(stderr,
			"[oobtree] %*snode=%p art=%d xyoff=(%d,%d) view=(%d,%d,%d,%d) home=(%d,%d,%d,%d) wr=(%d,%d,%d,%d) fchild=%p\n",
			depth*2, "", (void*)n, (int)n->artnum,
			(short)(off&0xFFFF), (short)((off>>16)&0xFFFF),
			n->viewsize.left, n->viewsize.top, n->viewsize.right, n->viewsize.bottom,
			n->homesize.left, n->homesize.top, n->homesize.right, n->homesize.bottom,
			wr.left, wr.top, wr.right, wr.bottom, (void*)n->fchild);
		if (n->fchild) bob_oobtree_probe(n->fchild, depth+1);
	}
}

/* S133: draw a panel's NESTED child dialogs' hosted controls. bob_ole_draw_panel only draws
   controls whose parentDlg == the passed dialog; a DialList screen (QS order-of-battle:
   QuickMissionPanel -> DialList of CSQuickLine flight rows) nests each row as its own RDialog
   with its own hosted controls, so the standard per-pdial draw never reaches them. The game's
   own layout engine (MoveWindow/OnSize/GetWindowRect) computes no real rects headlessly (probe:
   every nested node has viewsize height 0 + full-screen GetWindowRect), so we cannot read the
   row positions from the tree. Instead SYNTHESIZE the vertical stacking: the DialList rows are
   identical CSQuickLine panels, so draw each successive content-bearing child one row-step lower,
   reusing bob_ole_draw_panel's own template-rect positioning for the within-row column layout.
   Default-on; BOB_NO_QS_NESTED reverts to the empty-panel (pre-S133) behaviour for A/B. */
static void bob_nested_walk(RDialog* n, int ox, int oyBase, int rowStep, int* rowIdx)
{
	for (; n; n = n->sibling) {
		int oy = oyBase + (*rowIdx) * rowStep;
		int drew = bob_ole_draw_panel((CWnd*)n, ox, oy);
		if (drew > 0) (*rowIdx)++;   /* only content rows advance the vertical cursor */
		if (n->fchild) bob_nested_walk(n->fchild, ox, oyBase, rowStep, rowIdx);
	}
}
static void bob_fp_draw_nested(RDialog* top, int ox, int oy)
{
	static int off = -1; if (off < 0) off = getenv("BOB_NO_QS_NESTED") ? 1 : 0;
	if (off || !top || !top->fchild) return;
	int rowStep = 40;
	if (getenv("BOB_QS_ROWSTEP")) rowStep = atoi(getenv("BOB_QS_ROWSTEP"));
	int row = 0;
	bob_nested_walk(top->fchild, ox, oy, rowStep, &row);
}

/* Repaint the current screen in place (background + dial panels + hosted controls + menu),
   without re-launching it (which would reset combo values). Used after a combo click cycles
   its value, so the new value shows. Mirrors LaunchScreen's BOB_LINUX paint block. */
static void bob_fp_repaint(RFullPanelDial* fp)
{
	if (getenv("BOB_TRACE_OOBTREE"))
		for (int d=0; d<3; d++) if (fp->pdial[d]) {
			fprintf(stderr, "[oobtree] === pdial[%d]=%p ===\n", d, (void*)fp->pdial[d]);
			bob_oobtree_probe(fp->pdial[d], 0);
		}
	fp->DoPaint(fp->GetDC());
	for (int d=0; d<3; d++) if (fp->pdial[d]) {
		fp->pdial[d]->DoPaint(fp->GetDC());
		if (bobOleDraw()) {
			int ox = fp->m_currentscreen->resolutions[fp->m_currentres].dials[d].X;
			int oy = fp->m_currentscreen->resolutions[fp->m_currentres].dials[d].Y;
			bob_ole_draw_panel((CWnd*)fp->pdial[d], ox, oy);
			bob_fp_draw_nested(fp->pdial[d], ox, oy);   /* S133: nested DialList rows (QS OOB) */
		}
	}
	bob_draw_menu(fp, fp->m_currentscreen, RFullPanelDial::resolutions[fp->m_currentres]);
	bob_gdi_present();
}
/* R1.1b (Scrum Sprint 4): the QM pre-flight world setup that the game normally runs before
   StartFlying (factory init + node/squad tables + scramble campaign + the player's quick-
   mission squadron). Mirrors the BOB_BOOT_FRONTEND scaffold's pre-flight (MIG.CPP), but here
   we hand the actual Inst3d/View3d construction to the game's OWN path (Rtestsh1::Launch3d)
   instead of synthesizing it -- this is the faithful menu->flight transition. */
static void bob_startflying_preflight(bool clickMode, bool firstTime)
{
	/* One-time device/factory init (display/sound config that persists across missions) -- only
	   on the first flight; re-flying (R2.4) skips it. The per-mission world setup below runs
	   every flight (as the game's SetUpHotShot does per QM). */
	if (firstTime) {
		Save_Data.SetUnits();                              /* unit-conversion factors (else SIGFPE) */
		if (!getenv("BOB_NOINITPREFS")) {
			Save_Data.InitPreferences((int)Master_3d.winst);  /* R1.4: real factory defaults */
			Save_Data.filtering = 1;                          /* R1.3c: pin BILINEAR (trilinear NULL-derefs) */
		}
		if (!getenv("BOB_NOCLOUDS")) {                     /* faithful sky (default-on, cf. scaffold) */
			Save_Data.hardwareconfig |= HW_FLUFFYCLOUDS;
			Save_Data.hardwareconfig |= HW_WEATHEREFFECTS;
		}
		if (getenv("BOB_NOSOUND")) {
			Save_Data.vol.sfx = Save_Data.vol.engine = Save_Data.vol.anim = 0;
			Save_Data.vol.music = Save_Data.vol.rchat = Save_Data.vol.uisfx = Save_Data.vol.uiambient = 0;
		}
	}
	/* Per-mission world setup. Sprint 7: the REAL click flow provides this itself
	   (SetQuickState sets the campaign; BoBFrag::OnInitDialog sets playersquadron + node
	   tables), so click mode leaves it to the game (verified: a real title->QM->Fly->bobfrag->
	   Fly click-through flies with this skipped). The forced path (=1) doesn't pass through
	   bobfrag, so it still needs the setup. BOB_PREMISSION forces it on for click mode too. */
	if (!clickMode || getenv("BOB_PREMISSION")) {
		Node_Data.BuildTargetTable();                      /* node/squad tables first (cold start) */
		Node_Data.LoadCleanNodeTree();
		Miss_Man.currcampaignnum = MissMan::SCRAMBLECAMPAIGN;
		Miss_Man.camp = Miss_Man.campaigntable[MissMan::SCRAMBLECAMPAIGN];
		/* R3.1: SetQuickState sets an afternoon time + clear sky for QM; the campaign default is
		   PRE-DAWN (curTime 23400 < dawn 25200) -> dark twilight lighting. The forced path (=1)
		   skips SetQuickState, so set them here too -- otherwise headless flight is wrongly dark. */
		Miss_Man.camp.currtime = HR14;                     /* 2pm (matches SetQuickState) */
		Miss_Man.camp.Sky.Conditions = 0;                  /* clear */
	}
	/* R2.2: gamestate=HOT is the QM state (SetUpHotShot sets it). OnFlyingClosed routes an
	   IDOK exit (EXITKEY/Alt+X) to quickmissiondebrief only when gamestate is HOT or QUICK;
	   without it the close falls through to the campaign-map branch. */
	RFullPanelDial::gamestate = RFullPanelDial::HOT;
	if (clickMode) {
		/* 4.2: navigate the REAL QM config flow. Leave the quick-mission selection to the game's
		   own CSQuick1 ctor, which (when currquickmiss==-1, the fresh-entry sentinel) sets
		   currquickmiss=0, currquickfamily=0, quickdef=quickmissions[0]. Pre-seeding currquickmiss
		   here would DEFEAT that init -> currquickfamily stays at the stale -1 -> SetIndex(-1)
		   NULL-derefs the family combo. So reset to the sentinel and let the screen initialise. */
		CSQuick1::currquickmiss = -1;
		CSQuick1::currquickfamily = -1;
		fprintf(stderr,"[startfly] pre-flight(click): world init done; QM left to CSQuick1 (currquickmiss=-1)\n");
	} else {
		/* 4.1: forced flight, no config screen -- seed the quick-mission + player squadron directly. */
		const char* qix = getenv("BOB_QM_INDEX");
		int qi = qix ? atoi(qix) : 0;
		CSQuick1::currquickmiss = qi;
		CSQuick1::quickdef = CSQuick1::quickmissions[qi];
		QuickDef& q = CSQuick1::quickdef;
		MMC.playersquadron = q.line[q.plside][q.plwave][q.plgrp].actype;
		MMC.playeracnum = q.plac;
		fprintf(stderr,"[startfly] pre-flight(forced): texQ=%d filtering=%d vol.sfx=%d QM=%d playersquadron=%d\n",
			(int)Save_Data.textureQuality,(int)Save_Data.filtering,(int)Save_Data.vol.sfx,qi,(int)MMC.playersquadron);
	}
}

/* R1.1b Increment 4.3: the return path (flight -> menu, one process). In flight, pressing
   KEY_CONFIGMENU (F12) makes View3d::CloseWindow(IDCANCEL) post WM_COMMAND to the Rtestsh1
   dialog -- which our compat swallows (no message dispatch). bob_capture_wm_command (called
   from CWnd::PostMessage) records that close request while flight is live; the main-thread Run
   loop drains it via bob_process_flight_close, running the game's OWN public teardown
   (Rtestsh1::OnCancel/OnOK -> delete View3d/Inst3d) then OnFlyingClosed -> LaunchScreen(the
   exit screen). Faithful, just delivering the message the dead Win32 hop would have. */
int g_campfly_shot = 0;   /* R4.3 (BOB_CAMPFLY_SHOT): frames-until-bobfrag-capture countdown */
int g_campfly_go = 0;     /* R4.3 (BOB_CAMPFLY_GO): frames-until-Fly (bobfrag -> campaign flight) */
int g_campfly_flown = 0;  /* R4.3: a campaign mission has been launched -> stop the post-mission sim FF */
extern "C" int bob_gdi_dump_to(const char*);   /* R4.3: one-shot framebuffer dump (bob_video.cpp) */
extern "C" int bob_ole_host_total(void);       /* S166: size of the hosted-control table */
static int g_bobFlightCloseId = -1;
void bob_capture_wm_command(unsigned int /*msg*/, unsigned int id)
{
	if (Rtestsh1::THISTHIS && Inst3d::InThe3D())
		g_bobFlightCloseId = (int)id;
}
int bob_process_flight_close(void)
{
	if (g_bobFlightCloseId < 0) return 0;
	int id = g_bobFlightCloseId; g_bobFlightCloseId = -1;
	fprintf(stderr,"[startfly] flight close (id=%d) -> %s + OnFlyingClosed\n",
		id, id==IDOK ? "OnOK" : "OnCancel");
	if (Rtestsh1::THISTHIS) {                 /* game's own teardown: Paused, delete view/inst */
		if (id==IDOK) Rtestsh1::THISTHIS->OnOK();
		else          Rtestsh1::THISTHIS->OnCancel();
	}
	/* S162: arm the one-shot capture on flight CLOSE, so a post-flight screen (the debrief) can be
	   captured by STATE rather than by counting ticks. Front-end ticks pause while flying, so no
	   tick number reliably lands after the return — three attempts at QS-3's repro missed for
	   exactly that reason. S148's rule: no counter survives the harness changing the sim rate;
	   only a state description does. Pairs with BOB_SHOT_AFTER=<n>. */
	bob_shot_arm("flight-close");
	bob_gdi_restore_mode();   /* S162: 3D ran 800x600; the front end lays out at 1024x768 */
	if (g_bobActiveFP) {                       /* navigate back to a front-end screen */
		g_bobActiveFP->OnFlyingClosed(NULL, id);
		fprintf(stderr,"[startfly] back in front-end (InThe3D=%d)\n",(int)Inst3d::InThe3D());
	}
	{ extern int g_bob_flight_active; g_bob_flight_active = 0; }   /* R2.4: flight ended -> re-arm auto-quit */
	return 1;
}

/* Called each idle from CMIGApp::OnIdle (under BOB_FRONTEND): if a menu item was clicked,
   fire the existing OnSelectRlistbox(i) which navigates to that item's screen (+ repaints).
   A click that misses the menu but lands on a hosted combo cycles that combo's value. */
extern "C" int g_bob_map_active = 0;            /* R4.2: strategic-map mode (set by LaunchMap) */
extern "C" void bob_map_paint_begin(void);
extern "C" void bob_map_paint_end(void);
extern "C" void bob_gdi_present(void);
extern "C" void bob_gdi_screen_size(int*, int*);
extern "C" void bob_map_paint_scalebar(CMIGView*, int, int);  /* right-edge ruler (SCALEBAR.CPP) */
extern "C" void bob_map_paint_titlebar(int, int);             /* footer date/time band (TITLEBAR.CPP) */
extern "C" void bob_map_paint_teletype(int, int);             /* footer event-log pane (TELETYPE.CPP) */
extern "C" void bob_map_paint_toolbars(int, int);             /* footer button rows (MAINFRM.CPP, S89) */
extern "C" int  bob_map_click_toolbars(int, int);             /* S92: map click -> button handler */
extern "C" int  bob_map_click_oob(int, int);                  /* S156: open OOB dialog gets first refusal */
extern "C" int  bob_sdl_push_click(int, int);                 /* S157: queue a REAL SDL mouse event */
extern "C" int  bob_gdi_get_click(int*, int*);
extern "C" int  bob_gdi_get_mapnav(int*, int*, int*);         /* S96: pan/zoom from the SDL layer */
extern "C" void bob_map_nav(int, int, int);                   /* S96: apply pan/zoom (MIGVIEW.CPP) */
extern "C" int  bob_map_select(int, int);                     /* S97: click -> unit select (MAPDLG.CPP) */

extern "C" void bob_frontend_tick(void)
{
	/* R4.2: paint the strategic map each idle. CMIGView::LaunchMap destroyed the front-end
	   panel and switched to the map view (m_mapdlg); drive its terrain render
	   (CMIGView::UpdateBitmaps) onto the framebuffer with a screen DC, then present. Map coords
	   are full-window, so the DC viewport is (0,0). Keeps the window live (no idle->SDL_QUIT). */
	/* S178 (PO: clicked FLY -> "dead parrot"). The map paint was gated on our OWN flag,
	   g_bob_map_active, which CMIGView::LaunchMap sets and nothing clears. LaunchFullPane -- what
	   Fly, the mission folder and the day review all go through -- sets the GAME's page state
	   m_currentpage=1 without touching that flag, so the port kept painting the strategic map while
	   the game had moved to a full-screen page. The player saw an unchanged map that ignored every
	   transport button and whose clock had stopped, because the clock and the accel handlers are
	   correctly gated on m_currentpage. Measured: `[clock] STUCK for 2400 paints: m_currentpage=1
	   ... Master_3d.currinst=(nil)` -- a page was up, and no flight had started.
	   Follow the game's own notion of which page is showing, so the render and the state agree.
	   BOB_NO_PAGE_GATE=1 reverts. */
	static int noPageGate = -1;
	if (noPageGate < 0) noPageGate = getenv("BOB_NO_PAGE_GATE") ? 1 : 0;
	int onMapPage = noPageGate || !RDialog::m_pView || RDialog::m_pView->m_currentpage == 0;
	/* S179: report when the gate SUPPRESSES the map. Adding the gate also silenced the clock
	   diagnostic, because the clock is driven from inside this block -- so the session after it went
	   quiet exactly when something interesting happened, and "no STUCK line" looked like health
	   rather than like the instrument being switched off. A suppressed map is itself the state worth
	   knowing: the game is on a full-screen page the port may or may not be rendering. */
	if (g_bob_map_active && !onMapPage) {
		static long supp = 0;
		if ((supp++ % 600) == 0)
			fprintf(stderr, "[pagegate] map paint suppressed: m_currentpage=%d (frame %ld) -- game is "
				"on a full-screen page; clock drive is not running\n",
				(int)RDialog::m_pView->m_currentpage, supp);
	}
	if (g_bob_map_active && onMapPage && RDialog::m_pView && !Inst3d::InThe3D()) {
		int sw = 1024, sh = 768; bob_gdi_screen_size(&sw, &sh);
		CDC dc; dc.m_bobScreen = true; dc.m_hDC = (HDC)1; dc.m_bobVpX = 0; dc.m_bobVpY = 0;
		CRect rc(0, 0, sw, sh);
		/* S92: a click on the map -> fire the hit footer-toolbar button's handler. Done before the
		   paint so a handler that changes mode (e.g. mission folder -> LaunchFullPane) takes effect;
		   if it left map mode, bail to the front-end paint path next tick. */
		{ int cx, cy; int haveClick = 0;
		  if (!getenv("BOB_NO_BUTTONS") && bob_gdi_get_click(&cx, &cy)) haveClick = 1;
		  /* BOB_MAP_CLICK="x,y" (diag): inject one click at (x,y) once the map has settled, to test
		     the button-click -> handler path deterministically (no tick-counting). */
		  /* S187: the settle count is now a THIRD field, "x,y[,afterPaints]" (default 6). Six paints
		     reaches the toolbars, which is all this hook was built for -- but the dialogs worth
		     clicking now (an intercept offer, a raid warning) only exist once the campaign sim has
		     run a raid out, several hundred paints in. Without this the harness could only ever
		     click things that are present immediately, which quietly limited what could be tested
		     to the subset that was never in doubt. */
		  static int mc_paints = 0, mc_done = 0; const char* mcxy = getenv("BOB_MAP_CLICK");
		  if (mcxy && !mc_done) { int mx, my, after = 6;
			int nf = sscanf(mcxy, "%d,%d,%d", &mx, &my, &after);
			if (nf >= 2 && ++mc_paints > after) { cx=mx; cy=my; haveClick=1; mc_done=1;
				fprintf(stderr,"[mapclick] injecting (%d,%d) after %d paints\n", cx, cy, after); } }
		  /* S157 (SP.24): BOB_MAP_SDLCLICK="x,y" -- same settling rule, but pushes a REAL
		     SDL_MOUSEBUTTONDOWN instead of assigning cx/cy here. That makes the SDL event handler,
		     its logical->drawable scaling and bob_gdi_get_click part of what the test proves,
		     instead of the three layers every existing click driver skips. The click arrives via
		     bob_gdi_get_click on a subsequent paint, not this one. */
		  static int sc_paints = 0, sc_done = 0; const char* scxy = getenv("BOB_MAP_SDLCLICK");
		  if (scxy && !sc_done && ++sc_paints > 6) { int mx, my;
			if (sscanf(scxy, "%d,%d", &mx, &my) == 2) { bob_sdl_push_click(mx, my); sc_done = 1; } }
		  if (haveClick) {
			/* S156 (SP.14): an OPEN OOB dialog gets first refusal -- before S156 the map routed
			   clicks to the toolbars and then to unit selection, so every OOB dialog was
			   render-only (MA note 31 section 3 warned us to check; we were in the same state). */
			if (bob_map_click_oob(cx, cy)) { if (!g_bob_map_active) return; }
			else if (bob_map_click_toolbars(cx, cy)) { if (!g_bob_map_active) return; }
			else bob_map_select(cx, cy);   /* S97: missed everything -> select the unit under the click */
		  }
		}
		/* S96: strategic-map pan (arrows) + zoom (wheel / +/-), from the SDL layer. BOB_MAP_NAV="dx,dy,z"
		   injects one nav step (once settled) for deterministic testing. */
		{ int px, py, zs; int haveNav = bob_gdi_get_mapnav(&px, &py, &zs);
		  static int nv_paints = 0, nv_done = 0; const char* nvv = getenv("BOB_MAP_NAV");
		  if (nvv && !nv_done && ++nv_paints > 6) { if (sscanf(nvv, "%d,%d,%d", &px, &py, &zs) == 3) {
			haveNav = 1; nv_done = 1; fprintf(stderr,"[mapnav] injecting pan=(%d,%d) zoom=%d\n", px, py, zs); } }
		  if (haveNav) bob_map_nav(px, py, zs);
		}
		/* R4.5 (BOB_MAP_TIMER): drive the campaign clock -- the MFC WM_TIMER doesn't fire on Linux,
		   so the day never starts. CMapDlg::OnTimer advances the clock + runs the sim. (The
		   raid-generator heap bugs that used to SIGSEGV the first PerformMoveCycle are fixed --
		   ReorderPackage new[]/delete + RecostRaidList terminator off-by-one.) Default OFF while the
		   dynamic-icon / mission-flow bring-up continues. */
		/* R4.3: once a campaign mission has been flown + we've returned to the map, STOP the
		   headless fast-forward. PerformMoveCycle over the post-mission world (a returned
		   interceptor / consumed raid) SIGSEGVs in the SAG-movement AI (GetCruiseAt ->
		   Plane_Type_Translate[bad ptype]) -- the R4.5 uninit-SAG grind, a distinct onion layer.
		   In real play the returned map is PAUSED (the player reviews, then advances), so not
		   driving it is faithful; the post-mission sim fix is deferred to R4.5. */
		extern int g_campfly_flown;
		if (getenv("BOB_MAP_TIMER") && (!g_campfly_flown || getenv("BOB_POSTMISSION_FF"))) {
			/* BOB_POSTMISSION_FF (R4.5 diagnostic, default-off): override the post-mission/post-fly
			   g_campfly_flown guard so BOB_MAP_TIMER advances the world AFTER a flown mission returns
			   to the map (the map tick only runs when !InThe3D, so this is dormant during flight) --
			   the repro for ASan'ing the post-mission SAG sim (GetCruiseAt/Plane_Type_Translate). */
			/* The campaign map starts PAUSED (curracceltype=ACCEL_PAUSED, rate 0) -- the player
			   normally clicks a UI accel button to run time. Headlessly, un-pause it so the day
			   advances and raids reach their launch times. */
			MMC.curracceltype = MMC.ACCEL_RAIDSPD;
			MMC.curraccelrate = MMC.accelrates[MMC.ACCEL_RAIDSPD] > 0 ? MMC.accelrates[MMC.ACCEL_RAIDSPD] : 20;
			/* fast-forward factor: BOB_MAP_TIMER=N drives the campaign clock N times per paint so the
			   day reaches raid-launch times quickly headlessly. */
			int ff = atoi(getenv("BOB_MAP_TIMER")); if (ff < 1) ff = 1; if (ff > 64) ff = 64;
			if (getenv("BOB_TRACE_DAYEND")) { static int tp=0; if ((tp++%200)==0)
				fprintf(stderr,"[mapdrive] m_currentpage=%d curracceltype=%d currtime=%d\n",
					(int)RDialog::m_pView->m_currentpage, (int)MMC.curracceltype, (int)MMC.currtime); }
			for (int z = 0; z < ff; z++) RDialog::m_pView->m_mapdlg.bob_drive_timer();
		}
		/* S94: live accel controls -- when the player has clicked Play/Fastforward (so the campaign
		   is not ACCEL_PAUSED), drive the clock each paint at the chosen rate, exactly as the map's
		   WM_TIMER would. This is the faithful map behaviour (the day starts running on Play). Same
		   post-mission safety guard as BOB_MAP_TIMER (the post-fly SAG grind is the R4.5 layer).
		   BOB_MAP_TIMER (headless) takes precedence; skip if it already ran this paint. */
		else if (MMC.curracceltype != MMC.ACCEL_PAUSED && (!g_campfly_flown || getenv("BOB_POSTMISSION_FF"))) {
			int reps = MMC.curracceltype >= MMC.ACCEL_RAIDSPD ? 8 : 1;   /* fast-forward vs normal */
			/* S175 (PO: "clock is not running, even though time is supposed to be 1x", and the accel
			   buttons do nothing). The clock is driven from here, but the advance itself lives inside
			   the map's OnTimer behind THREE gates that this loop cannot see:
			       if (m_currentpage == 0)                      <- a full-screen page freezes it
			       if (!RDialog::messageboxopen)                <- a modal is up
			       if (!Master_3d.currinst)                     <- a 3D instance exists
			   All three also disable every accel button (TitleBar::OnClickedPause/Play/Fastforward
			   each start with the same m_currentpage guard), which is why the buttons take the click
			   -- the log shows them consumed -- and nothing happens. So "drive ran, clock did not
			   move" is the observation, and WHICH gate is shut is the only useful question. Report it
			   rather than infer it: compare currtime across the drive and name the closed gate.
			   BOB_TRACE_CLOCK=1, default off, one line when it is actually stuck. */
			static int traceClock = -1;
			if (traceClock < 0) traceClock = getenv("BOB_TRACE_CLOCK") ? 1 : 0;
			long ct_before = traceClock ? (long)MMC.currtime : 0;
			static int liveOnce = 0; if (getenv("BOB_TRACE_MAP") && !liveOnce) { liveOnce = 1;
				fprintf(stderr,"[accel] live clock drive ON (curracceltype=%d rate=%d currtime=%d)\n",
					(int)MMC.curracceltype, (int)MMC.curraccelrate, (int)MMC.currtime); }
			for (int z = 0; z < reps; z++) RDialog::m_pView->m_mapdlg.bob_drive_timer();
			/* Only report a clock that is REALLY stuck. At rate 1 most paints legitimately do not
			   advance (the accumulator wraps), so "did not move this paint" is not a fault -- my
			   first cut fired on healthy play and would have sent the next session chasing noise.
			   Count consecutive non-advancing paints instead, and report once past a threshold no
			   normal rate can reach. */
			if (traceClock) {
				static long same = 0; static long lastReport = -1;
				if ((long)MMC.currtime == ct_before) same++; else { same = 0; lastReport = -1; }
				if (same == 600 || (same > 600 && same - lastReport >= 600)) {
					lastReport = same;
					fprintf(stderr, "[clock] STUCK for %ld paints: currtime=%ld rate=%d type=%d | "
						"m_currentpage=%d messageboxopen=%d Master_3d.currinst=%p campfly=%d\n",
						same, (long)MMC.currtime, (int)MMC.curraccelrate, (int)MMC.curracceltype,
						(int)RDialog::m_pView->m_currentpage, (int)RDialog::messageboxopen,
						(void*)Master_3d.currinst, g_campfly_flown);
				}
			}
		}
		/* S107/S108/S110 (BOB_DAYLOOP): headless multi-day continuity, after EITHER clock-drive path
		   (BOB_MAP_TIMER or the live accel controls). At dusk the sim's EndOfDay rolls the date/clock
		   (currdate++, currtime=MORNINGPERIODSTART) + WipeAll's the raids, then goes to the EndDayReview
		   front-end screen -- the next day's world rebuild happens only on the way back via
		   LaunchMapFirstTime, which the headless map scaffold doesn't navigate. So when the day has just
		   rolled, run the rebuild directly. The three cracked layers (see PORT.md S104-S109): StartOfDay
		   alone leaves the world empty -> need BuildTargetTable + StartUpMapWorld (S107); the rebuild
		   leaves m_currentpage=1 which freezes OnTimer's whole sim-advance -> restore it (S108); the
		   production-overflow + directive-recursion fixes (S108/S109) are unconditional in their sources.
		   Validated: 9 days cycle ASan-clean. Fires once per date. */
		if (getenv("BOB_DAYLOOP") && !getenv("BOB_DAYADV")) {
			static long lastDay = -1;
			if (MMC.currtime <= MORNINGPERIODSTART + 40 && (long)MMC.currdate != lastDay) {
				lastDay = (long)MMC.currdate;
				fprintf(stderr,"[dayloop] new day (date=%ld) -> full rebuild (StartUpMapWorld)\n", (long)MMC.currdate);
				long ct0 = (long)MMC.currtime;
				Node_Data.BuildTargetTable();
				Persons4::StartUpMapWorld();          /* rebuilds the raid world (StartOfDay alone doesn't) */
				Node_Data.StartOfDay();
				if ((long)MMC.currtime != ct0) MMC.currtime = ct0;   /* restore morning currtime */
				RDialog::m_pView->m_currentpage = 0;  /* un-freeze OnTimer (S108) */
			}
		}
		/* S120 (BOB_DAYADV): the FAITHFUL day-advance. At dusk, EndOfDay -> GoToEndDayRouting ->
		   LaunchFullPane(enddayreview) sets m_currentpage=1 (the day-review FullScreen). The scaffold
		   doesn't navigate that screen, so instead of BOB_DAYLOOP's manual rebuild, drive the game's OWN
		   review->map return -- RFullPanelDial::ReturnToMapAfterReview (StartUpMapWorld + LaunchMap, which
		   sets m_currentpage=0). This uses the exact path the CONTINUE button would, not a scaffold hack.
		   Trigger: the map was running (m_currentpage flipped 0->1 by the enddayreview launch). */
		if (getenv("BOB_DAYADV") && g_bob_map_active && g_bobActiveFP) {
			static int wasPage = 0;
			int pg = (int)RDialog::m_pView->m_currentpage;
			if (pg == 1 && wasPage == 0) {   /* enddayreview just launched */
				fprintf(stderr,"[dayadv] enddayreview launched (currdate=%ld) -> ReturnToMapAfterReview\n", (long)MMC.currdate);
				FullScreen* fs = NULL;
				g_bobActiveFP->ReturnToMapAfterReview(fs);   /* the game's faithful review->map return */
			}
			wasPage = (int)RDialog::m_pView->m_currentpage;
		}
		/* R4.4 (BOB_CAMPAIGN_SAVE, default-off): produce a campaign save from the running map.
		   CFiling::SaveGame serialises Miss_Man (the campaign state) -- the same call the autosave
		   + OnClickedFrag2 make. The fakefile savegame path was corrupted on Linux (fixed here +
		   in LoadGame), so this is the first save that actually lands on disk. BOB_CAMPAIGN_SAVE=<n>
		   = save after n paints (let the sim populate the campaign first). */
		if (getenv("BOB_CAMPAIGN_SAVE")) {
			static int sv_t = 0; static bool sv_done = false;
			if (!sv_done && ++sv_t >= atoi(getenv("BOB_CAMPAIGN_SAVE"))) {
				sv_done = true;
				const char* nm = getenv("BOB_SAVE_NAME"); if (!nm || !*nm) nm = "Auto Save.BSR";
				bool ok = CFiling::SaveGame(nm);
				fprintf(stderr,"[campsave] CFiling::SaveGame(\"%s\") -> %s (currtime=%d localplayer=%d)\n",
					nm, ok?"OK":"FAIL", (int)MMC.currtime, (int)Todays_Packages.localplayer);
			}
		}
		/* R4.4 (BOB_CAMPAIGN_LOAD, default-off): load a campaign save into the running map.
		   CFiling::LoadGame deserialises Miss_Man (bis >> Miss_Man) + restores the map view. Proof
		   of the save/load round-trip: a freshly-started campaign is at day-start (currtime ~23400);
		   after loading a save taken later, currtime jumps to the saved value. BOB_CAMPAIGN_LOAD=<n>
		   = load after n paints; BOB_SAVE_NAME picks the file (default "Auto Save.BSR"). */
		if (getenv("BOB_CAMPAIGN_LOAD")) {
			static int ld_t = 0; static bool ld_done = false;
			if (!ld_done && ++ld_t >= atoi(getenv("BOB_CAMPAIGN_LOAD"))) {
				ld_done = true;
				const char* nm = getenv("BOB_SAVE_NAME"); if (!nm || !*nm) nm = "Auto Save.BSR";
				int before = (int)MMC.currtime;
				bool ok = CFiling::LoadGame(nm);
				fprintf(stderr,"[campload] CFiling::LoadGame(\"%s\") -> %s (currtime %d -> %d, localplayer=%d gamestate=%d)\n",
					nm, ok?"OK":"FAIL", before, (int)MMC.currtime,
					(int)Todays_Packages.localplayer, (int)RFullPanelDial::gamestate);
				/* LoadGame restores Miss_Man but does NOT rebuild the world (the real flow does that
				   via LaunchMap->StartUpMapWorld next); fast-forwarding the sim over the loaded-but-
				   un-rebuilt world then crashes in the SAG AI (R4.5 family). Stop the headless
				   fast-forward after a load, exactly as after a flown mission. */
				if (ok) g_campfly_flown = 1;
			}
		}
		/* R4.3 (BOB_CAMPAIGN_FLY, default-off): drive the campaign's OWN map->mission seam. Once
		   the sim has run a while (raids generated; the RAF has standing patrols/intercepts),
		   find a flyable RAF package (attackmethod PATROL/INTERCEPT, i.e. < AM_FIELD_MIN; not
		   SPARE; has squadrons), highlight it (hipack/hisquad -- exactly what a map squadron-click
		   + the RAF mission folder set), and call the game's own
		   CMIGView::LaunchFullPane(&bobfrag, UIR_FRAG) -- the precise seam OnClickedFrag2 uses to
		   raise the campaign mission briefing. Bypasses the CRToolBar/mission-folder UI (a
		   separate ~13pt subsystem) with the faithful public entry. BOB_CAMPAIGN_FLY=<paints-to-wait>. */
		if (getenv("BOB_CAMPAIGN_FLY")) {
			static int cf_t = 0; static bool cf_done = false;
			if (!cf_done) {
				int wait = atoi(getenv("BOB_CAMPAIGN_FLY")); if (wait < 1) wait = 1;
				/* Past the initial wait, RE-SCAN every paint until a flyable mission appears --
				   enemy raids spawn airborne only at/after their takeoff time, so the first scan
				   may find none; keep trying. cf_done is set only once we actually launch bobfrag. */
				if (++cf_t >= wait) {
					static int cf_log = 0; bool logScan = (cf_log++ % 60 == 0);  /* throttle the retry log */
					/* Player side decides which packages are flyable: the RAF player flies its own
					   PATROL/INTERCEPT squadrons (attackmethod < AM_FIELD_MIN); the LW player flies
					   its raid packages (attackmethod >= AM_LWPACKS). In a fresh campaign the RAF has
					   no packages yet (squadrons sit at airfields until scrambled via the mission
					   folder) -- so the LW side has flyable packages first. */
					bool lwPlayer = ((int)Todays_Packages.localplayer == (int)NAT_LUF);
					int found = -1, nNonSpare = 0, nMine = 0;
					for (int p = 0; p < Profile::MAX_PACKS; p++) {
						Profile& pk = Todays_Packages.pack[p];
						if (pk.packagestatus == Profile::PS_SPARE) continue;
						nNonSpare++;
						bool mine = lwPlayer ? ((int)pk.attackmethod >= (int)Profile::AM_LWPACKS)
						                     : ((int)pk.attackmethod <  (int)Profile::AM_FIELD_MIN);
						if (mine) nMine++;
						if (getenv("BOB_TRACE_CAMPFLY") && logScan)
							fprintf(stderr,"[campfly] pack %d: method=%d status=%d squads=%d%s\n",
								p,(int)pk.attackmethod,(int)pk.packagestatus,(int)(ULong)pk.squadlist, mine?" MINE":"");
						if (found < 0 && mine && (int)(ULong)pk.squadlist >= 1) found = p;
					}
					/* If the player has no own package yet (the usual RAF case), AUTHORISE AN
					   INTERCEPTION against the first enemy raid -- the exact action
					   HostilesList::OnClickedRbuttonauthorise performs: NewPackage(raid-squadron
					   instance, the directives' RAF intercept profile) scrambles a real RAF
					   interceptor, then we highlight + brief it. This is the genuine campaign
					   player action, just reached without the hostiles-list OCX UI. */
					/* Find an enemy raid whose squadron has actually SPAWNED in the world
					   (instance != 0). Raids appear in pItem only at/after their takeoff time, so
					   the squadron UID is 0 for not-yet-launched raids; NewPackage on a 0 instance
					   SIGSEGVs. Pick the first LW squadron with a live instance (preferring one the
					   player can see, as OnClickedRbuttonauthorise does). */
					int enemy = -1, enemySq = -1;
					if (found < 0 && !lwPlayer) {
						for (int p = 0; p < Profile::MAX_PACKS && enemy < 0; p++) {
							Profile& pk = Todays_Packages.pack[p];
							if (pk.packagestatus == Profile::PS_SPARE) continue;
							if ((int)pk.attackmethod < (int)Profile::AM_LWPACKS) continue;
							int ns = (int)(ULong)pk.squadlist;
							for (int s = 0; s < ns; s++) {
								if ((unsigned)pk[s].instance == 0) continue;
								if (enemySq < 0) { enemy = p; enemySq = s; }       /* first live one */
								if (pk[s].playercansee) { enemy = p; enemySq = s; break; }
							}
						}
					}
					if (logScan)
						fprintf(stderr,"[campfly] scan: localplayer=%d(%s) nonspare=%d mine=%d own=%d enemyraid=%d sq=%d (currtime=%d)\n",
							(int)Todays_Packages.localplayer, lwPlayer?"LW":"RAF", nNonSpare, nMine, found, enemy, enemySq, (int)MMC.currtime);

					int hipack = found;
					if (hipack < 0 && enemy >= 0) {
						Profile& pk = Todays_Packages.pack[enemy];
						int sq = enemySq;
						UniqueID inst = pk[sq].instance;
						Directives::RAF::MissTypes mt = RAFDirectivesResults::RAFInterceptType(enemy, sq);
						fprintf(stderr,"[campfly] authorise intercept: raid pack=%d sq=%d inst=%u misstype=%d\n",
							enemy, sq, (unsigned)inst, (int)mt);
						int packnum = Todays_Packages_NewPackage(inst, MMC.directives.raf.userprofiles[mt]);
						fprintf(stderr,"[campfly] NewPackage -> packnum=%d\n", packnum);
						if (packnum >= 0) {
							Todays_Packages[packnum].CalcRoutePositionsAndTime();
							Todays_Packages.InvalidateRoute(&Todays_Packages[packnum], true, true);
							hipack = packnum;
						}
					}

					if (hipack >= 0) {
						cf_done = true;                       /* committed -- stop scanning */
						Todays_Packages.hipack = (char)hipack;
						Todays_Packages.hisquad = 0;
						/* R4.5 (S42) diagnostic, default-off: intercept but DON'T fly -- stay in map mode so
						   BOB_MAP_TIMER keeps ticking the freshly-scrambled interception SAGs. If the S39
						   GetCruiseAt clamp (BOB_TRACE_SAG) fires on them PRE-flight, the garbage `type` is a
						   CREATION bug (NewPackage); if it only fires post-mission, it's the rebuild. */
						if (getenv("BOB_CAMPFLY_NOFLY")) {
							fprintf(stderr,"[campfly] NOFLY: intercepted pack=%d, staying in map sim (no bobfrag)\n", hipack);
							return;
						}
						/* R4.5 (S44): write Package.dat with the LIVE campaign state before flying, exactly
						   as the real CMainToolbar::OnClickedFrag2 does (MAINTBAR.CPP). This scaffold drives
						   the map->mission seam directly (the mission-folder OCX isn't built), bypassing
						   OnClickedFrag2's save -- so without this, the post-mission StartUpMapWorld(true)
						   reloads a STALE shipped Package.dat (out-of-range squadnums -> the S38-S43 crash).
						   Saving here makes the post-mission restore the just-flown state (the S43 funnel
						   guard remains as defense against any mismatched file). */
						{
							char pdbuf[fileman::pathnamesize];
							BOStream pdbos(File_Man.namenumberedfile(
								File_Man.fakefile(FIL_SAVEGAMEDIR,"Package.dat"),pdbuf));
							if (pdbos.is_open()) { Todays_Packages.SaveGame(pdbos);
								fprintf(stderr,"[campfly] wrote live Package.dat (pre-fly, mirrors OnClickedFrag2)\n"); }
						}
						fprintf(stderr,"[campfly] LaunchFullPane(bobfrag, UIR_FRAG) hipack=%d hisquad=0\n", hipack);
						/* S180: the manual `g_bob_map_active = 0` that used to sit here is GONE.
						   LaunchFullPane now does it for every caller. Keeping it would leave this
						   scaffold patching around the production hook -- which is exactly why the
						   scaffold reached the briefing for 40 sprints while the PO's real clicks
						   hung on the same seam. With it removed, this harness exercises the real
						   handoff, so a regression here fails the gate instead of hiding. */
						RDialog::m_pView->LaunchFullPane(&RFullPanelDial::bobfrag, SaveData::UIR_FRAG);
						if (getenv("BOB_CAMPFLY_SHOT")) g_campfly_shot = 12;  /* dump bobfrag after it settles */
						if (getenv("BOB_CAMPFLY_GO"))   g_campfly_go   = 12;  /* then Fly -> campaign flight */
						return;
					}
					if (logScan)
						fprintf(stderr,"[campfly] no live mission yet (currtime=%d) -- waiting for a raid to launch\n",(int)MMC.currtime);
				}
			}
		}
		bob_map_paint_begin();
		RDialog::m_pView->UpdateBitmaps(&dc, rc);
		/* S83 (Phase 2): overlay the right-edge scale/ruler bar (the docking frame's
		   CScaleBar) -- its OnPaint doesn't run headlessly, so drive its real tick-drawing
		   into the right strip. Inside the paint-active window so its backdrop FillRect lands
		   (bob_gdi_fillrect is gated on g_mapPaintActive). BOB_NO_SCALEBAR reverts to bare map. */
		if (!getenv("BOB_NO_SCALEBAR"))
			bob_map_paint_scalebar(RDialog::m_pView, sw, sh);
		/* S84 (Phase 2): the map footer band + date/time/accel readout (the docking frame's
		   TitleBar). BOB_NO_TITLEBAR reverts. */
		if (!getenv("BOB_NO_TITLEBAR")) {
			bob_map_paint_titlebar(sw, sh);
			/* S173c: the log is drawn by bob_map_paint_toolbars now, from the real IDDT_TELETYPE
			   controls. BOB_TELETYPE_SCAFFOLD=1 restores the hand-rendered pane for A/B. */
			if (getenv("BOB_TELETYPE_SCAFFOLD"))
				bob_map_paint_teletype(sw, sh);
		}
		if (!getenv("BOB_NO_BUTTONS"))        /* S89-91: footer toolbar button rows (now default-on) */
			bob_map_paint_toolbars(sw, sh);
		if (getenv("BOB_PROBE_OOB")) { extern void bob_probe_oob(void); bob_probe_oob(); }  /* S99 spike */
		/* S116: render any open OOB info dialog (opened by a real toolbar-button click, S92) over the
		   map -- default-on (renders nothing if no dialog is open). BOB_MAP_OOB additionally auto-opens
		   Bases for headless testing; BOB_NO_OOB reverts. */
		if (getenv("BOB_MAP_OOB")) { extern void bob_oob_open_bases(void);
			static int t=0; if (++t > 3) bob_oob_open_bases(); }
		if (getenv("BOB_MAP_DIRECTIVES")) { extern void bob_oob_open_directives(void);  /* S137: gold #18 */
			static int t=0; if (++t > 3) bob_oob_open_directives(); }
		/* S143 (SP.5): dismiss any open misc-toolbar dialog after N map paints, so the map
		   underneath can be captured (gold #19's raid stacks). Default-off. NB on an ACTIVE
		   campaign day this alone is not enough — the two directives dialogs mutually re-open
		   (see bob_oob_suppress_directives); use BOB_MAP_NODIRECTIVES there. */
		if (getenv("BOB_MAP_CLOSEDLG")) { extern void bob_oob_close_dialogs(void);
			static int t=0; if (++t > atoi(getenv("BOB_MAP_CLOSEDLG"))) bob_oob_close_dialogs(); }
		/* S143 (SP.5): suppress the day-start directives popup via the game's own toolbar
		   toggle, so the strategic map is capturable on an active day (gold #19). Default-off. */
		if (getenv("BOB_MAP_NODIRECTIVES")) { extern void bob_oob_suppress_directives(void);
			static int t=0; if (++t > atoi(getenv("BOB_MAP_NODIRECTIVES"))) bob_oob_suppress_directives(); }
		/* S144 (SP.11): ACCEPT the orders flow (title-bar OK on each logged dialog) -- the faithful
		   exit, and the call chain that creates the day's raid packages (gold #19). Default-off. */
		if (getenv("BOB_MAP_ACCEPTDIR")) { extern void bob_oob_accept_directives(void);
			static int t=0; if (++t > atoi(getenv("BOB_MAP_ACCEPTDIR"))) bob_oob_accept_directives(); }
		if (!getenv("BOB_NO_OOB")) { extern int bob_map_paint_oob(int,int); bob_map_paint_oob(sw, sh); }
		bob_map_paint_end();
		/* SP.1 (BOB_SHOT, default-off): deterministic one-shot screen capture for the parity
		   sweep -- after N map paints dump the GDI framebuffer to BOB_SHOT_PATH (private target;
		   /tmp is shared with the sibling ports) and exit. Same counter serves the front-end
		   branch below; whichever paint path is active when the count is reached dumps. */
		if (getenv("BOB_SHOT") || getenv("BOB_SHOT_AFTER") || getenv("BOB_SHOT_WHEN")) {
			if (bob_shot_due()) {
				const char* sp = getenv("BOB_SHOT_PATH"); if (!sp || !*sp) sp = "/tmp/bobshot.ppm";
				bob_shot_state_banner("map");   /* S143/SP.9: record the state, not just the pixels */
				bob_gdi_present();
				int nz = bob_gdi_dump_to(sp);
				fprintf(stderr, "[shot] map paint %ld -> %s (nonblack=%d) -- exiting\n",
					g_bob_shot_t, sp, nz); fflush(stderr);
				_exit(0);
			}
		}
		static int mp = 0; if (getenv("BOB_TRACE_MAP") && (mp++ % 200) == 0) {
			int nItems=0; for (int k=0;k<WorldStuff::PITEMTABLESIZE;k++) if (WorldStuff::pItem[k]) nItems++;
			fprintf(stderr, "[map] paint#%d zoom=%.2f currtime=%d worlditems=%d\n",
				mp, (double)RDialog::m_pView->m_zoom, (int)MMC.currtime, nItems);
		}
		bob_gdi_present();
		return;
	}

	if (!g_bobActiveFP) return;

	/* S165 (BOB_TEST_MODAL=<tick>, default-off): fire the game's OWN confirmation box once, from
	   the idle loop, and report the answer it returns.
	   The modal loop this exercises had no reachable trigger in any headless recipe -- OnBye needs
	   the system box, and the bad-weather prompt needs a campaign day whose weather says so. A
	   defect you cannot drive from a script cannot have a gate (MA S121), so this drives it. The
	   answer is printed, because the ANSWER is what was broken: DoModal returned -1 and
	   CMainFrame::OnBye reads `rv<2` as "quit". */
	/* S166 (BOB_TRACE_OLETOTAL=<every-n-ticks>): how many hosted R* controls exist right now.
	   The S154 dialog teardown is gated "default OFF until measured on the campaign paths"; this
	   is the measurement. With teardown off the table only ever grows -- every re-open of a logged
	   dialog allocates a fresh dialog plus its controls and retains both. */
	if (const char* ot = getenv("BOB_TRACE_OLETOTAL")) {
		static long t = 0; long every = atol(ot); if (every < 1) every = 100;
		if ((++t % every) == 0) {
			fprintf(stderr, "[oletotal] tick %ld: %d hosted controls\n", t, bob_ole_host_total());
			fflush(stderr);
		}
	}

	if (const char* tm = getenv("BOB_TEST_MODAL")) {
		static int fired = 0; static long ticks = 0;
		/* own counter: g_bob_shot_t only advances when a BOB_SHOT* variable is set */
		if (!fired && ++ticks >= atol(tm)) {
			fired = 1;
			fprintf(stderr, "[testmodal] calling RMessageBox(QUITGAME, AREYOUSURE, SAVE, YES, CANCEL)\n");
			fflush(stderr);
			int rv = RDialog::RMessageBox(IDS_QUITGAME, IDS_AREYOUSURE, IDS_SAVE, IDS_YES, IDS_CANCEL);
			fprintf(stderr, "[testmodal] RMessageBox returned %d  (%s)\n", rv,
			        rv == 0 ? "Save" : rv == 1 ? "Yes/quit" : rv == 2 ? "Cancel/stay" : "STUB -- no dialog ran");
			fflush(stderr);
			if (getenv("BOB_TEST_MODAL_EXIT")) _exit(0);
		}
	}

	/* SP.1 (BOB_SHOT, default-off): front-end half of the one-shot parity capture (see the map
	   branch above). Counts front-end ticks; dumps the settled screen then exits. */
	if (getenv("BOB_SHOT") || getenv("BOB_SHOT_AFTER") || getenv("BOB_SHOT_WHEN")) {
		if (bob_shot_due()) {
			const char* sp = getenv("BOB_SHOT_PATH"); if (!sp || !*sp) sp = "/tmp/bobshot.ppm";
			bob_shot_state_banner("frontend");   /* S143/SP.9: record the state, not just the pixels */
			int nz = bob_gdi_dump_to(sp);
			fprintf(stderr, "[shot] frontend tick %ld -> %s (nonblack=%d) -- exiting\n",
				g_bob_shot_t, sp, nz); fflush(stderr);
			_exit(0);
		}
	}

	/* R4.3 (BOB_CAMPFLY_SHOT): deterministic one-shot capture of the campaign briefing. After
	   BOB_CAMPAIGN_FLY launches bobfrag, wait a few front-end paints for it to populate, then dump
	   the framebuffer to a dedicated PPM and exit -- avoids the per-frame BOB_DUMP_GDI race. */
	if (g_campfly_shot > 0) {
		if (--g_campfly_shot == 0) {
			int nz = bob_gdi_dump_to("/tmp/bobfrag_shot.ppm");
			fprintf(stderr,"[campfly] one-shot bobfrag capture (nonblack=%d) -- exiting\n", nz);
			fflush(stderr);
			_exit(0);
		}
	}
	/* R4.3 (BOB_CAMPFLY_GO): from the settled briefing, click "Fly" to launch the campaign
	   mission flight. The bobfrag screen's Fly item (last menu entry) navigates to
	   quickmissionflight via FragFly2 (which requires MMC.playersquadron, set by the briefing's
	   SetPlayersPositionCamp), whose InitProc is StartFlying -> creates Rtestsh1 -> the always-on
	   Launch3d bridge above fires -> campaign flight (the SAME path QM uses). */
	if (g_campfly_go > 0) {
		if (--g_campfly_go == 0) {
			/* Ensure the scrambled interceptor is the player's squadron. bobfrag's OnInitDialog
			   ran SetPlayersPositionCamp, which only selects squadrons already in the flyable
			   status window (PS_ACTIVE_MIN..PS_REFUELLING) and resets playersquadron to -1
			   otherwise -- and a just-scrambled squadron is still PS_PLANNING/TAKINGOFF. The
			   player explicitly chose to fly this interception, so set it from the highlighted
			   package's first squadron (post-init, so it survives into FragFly2's gate). */
			if (MMC.playersquadron == -1) {
				int hp = Todays_Packages.hipack;
				if (hp >= 0 && (int)(ULong)Todays_Packages[hp].squadlist >= 1) {
					MMC.playersquadron = Todays_Packages[hp][0].squadnum;
					MMC.playeracnum = 0;
					fprintf(stderr,"[campfly] set player squadron=%d (pack %d sq0 status=%d)\n",
						(int)MMC.playersquadron, hp, (int)Todays_Packages[hp][0].status);
				}
			}
			int flyIdx = g_bobMenuN > 0 ? g_bobMenuN - 1 : 2;   /* Fly is the last menu item */
			fprintf(stderr,"[campfly] Fly: playersquadron=%d gamestate=%d menuN=%d -> OnSelectRlistbox(%d)\n",
				(int)MMC.playersquadron, (int)RFullPanelDial::gamestate, g_bobMenuN, flyIdx);
			g_campfly_flown = 1;   /* after this mission, don't headlessly fast-forward the map sim */
			g_bobActiveFP->OnSelectRlistbox(flyIdx, 0);
			return;
		}
	}

	/* R5.3 (BOB_CONFIGSCREEN, default-off): jump straight to a config screen for a headless
	   render test, instead of walking the whole menu tree. controls -> the SController controls
	   form (device/axis assignment combos); also gfx/gfx2/sound/2d/sim. Forces LaunchScreen(scr)
	   once the front-end is up + settled (the same public entry the title-bar tab click uses). */
	if (getenv("BOB_CONFIGSCREEN")) {
		static int cs_t=0; static bool cs_done=false;
		if (!cs_done) {
			if (++cs_t < 30) return;   /* let the front-end settle + paint */
			const char* w=getenv("BOB_CONFIGSCREEN");
			FullScreen* scr=&RFullPanelDial::options3d;
			if      (strstr(w,"control")) scr=&RFullPanelDial::controloptions;
			else if (strstr(w,"sound"))   scr=&RFullPanelDial::soundoptions;
			else if (strstr(w,"2d"))      scr=&RFullPanelDial::twodoptions;
			else if (strstr(w,"sim")||strstr(w,"flight")) scr=&RFullPanelDial::flightoptions;
			else if (strstr(w,"game"))    scr=&RFullPanelDial::gameoptions;    /* SP.1: Sim Config "Game" tab */
			else if (strstr(w,"mission")) scr=&RFullPanelDial::missionoptions; /* SP.1: Sim Config "Mission" tab */
			else if (strstr(w,"view"))    scr=&RFullPanelDial::vieweroptions;  /* SP.1: Sim Config "Views" tab */
			else if (strstr(w,"quick"))   scr=&RFullPanelDial::quickview;      /* SP.1: Quick Shots select screen */
			else if (strstr(w,"gfx2"))    scr=&RFullPanelDial::options3d2;
			else if (strstr(w,"load"))    scr=&RFullPanelDial::loadgame;   /* R6.5: CLoad save-file screen */
			/* R4.4 (BOB_LOAD_GO): pre-seed the selected save BEFORE the load screen launches, so
			   CLoad::MakeFileList finds it as the default and PRE-SELECTS it (found>=0). Setting it
			   after launch is wiped because re-init resets selectedfile="" when the default isn't in
			   the list. With it pre-selected, the "Load" menu -> DoLoadGame reads it. */
			if (getenv("BOB_LOAD_GO") && scr==&RFullPanelDial::loadgame) {
				const char* nm=getenv("BOB_SAVE_NAME"); if(!nm||!*nm) nm="Auto Save.bsr";
				CFiling::selectedfile = nm;
			}
			fprintf(stderr,"[configscreen] LaunchScreen(%s)\n", w);
			g_bobActiveFP->LaunchScreen(scr);
			cs_done=true;
			return;
		}
		/* launched -> fall through to the normal tick so click injection / combo hosting run. */
	}

	/* R4.4 (BOB_LOAD_GO, default-off): drive the loadgame screen's "Load" end-to-end. After the
	   load screen settles, set the selected file and fire the "Load" menu item (the last entry,
	   RAF/Luftwaffe/Back/Load) -> RFullPanelDial::DoLoadGame -> CFiling::LoadGame(selectedfile)
	   (the working S29 deserialise) -> LaunchMap (enter the campaign map on the loaded state).
	   This is the file-selection the OCX eventsink would set on a row click; the Load nav itself
	   already rides the menu-click path. BOB_LOAD_GO=<paints-to-wait>; BOB_SAVE_NAME picks the file. */
	if (getenv("BOB_LOAD_GO") && g_bobActiveFP) {
		static int lg_t=0; static bool lg_done=false;
		if (!lg_done && g_bobMenuN>0) {
			if (++lg_t >= atoi(getenv("BOB_LOAD_GO"))) {
				lg_done=true;
				/* Set the selected file (what clicking the "Auto Save" row -> OnSelectRlistboxfile
				   sets -- the OCX file-row click is the one piece the no-op eventsink can't deliver,
				   so the scaffold sets it directly), then run the game's own DoLoadGame body:
				   CFiling::LoadGame (the working S29 deserialise) -> LaunchMap (enter the campaign
				   map on the loaded state). Driving the load directly avoids OnSelectRlistbox's
				   screen re-init, which wipes selectedfile back to the player name. */
				const char* nm=getenv("BOB_SAVE_NAME"); if(!nm||!*nm) nm="Auto Save.bsr";
				CFiling::selectedfile = nm;
				/* Normally stop the post-load fast-forward (R4.5 SAG-AI crash). BOB_POSTLOAD_FF (R4.5
				   diagnostic, default-off) leaves it running so BOB_MAP_TIMER advances the loaded+
				   LaunchMap-rebuilt world -- the repro for ASan'ing the post-load SAG crash. */
				if (!getenv("BOB_POSTLOAD_FF")) g_campfly_flown = 1;
				bool ok = CFiling::LoadGame(CFiling::selectedfile);
				fprintf(stderr,"[loadgo] selectedfile='%s' -> LoadGame=%d (currtime=%d) -> LaunchMap\n",
					nm, (int)ok, (int)MMC.currtime);
				if (ok) {
					Save_Data.lastsavegame = CFiling::selectedfile;
					FullScreen* fs = &RFullPanelDial::loadgame;
					g_bobActiveFP->LaunchMap(fs, false);
				}
				return;
			}
		}
	}

	/* Sprint 7 (env-var-free): ALWAYS-ON Launch3d bridge for a real menu click-through (no
	   BOB_STARTFLYING harness). When the game's own flow reaches quickmissionflight and StartFlying
	   creates a fresh Rtestsh1 (S3D_STARTSETUP set by its ctor, no Inst3d yet), deliver the
	   WM_GETSTRING the compat swallows by calling the game's own Rtestsh1::Launch3d -- exactly what
	   OnGetString does. Driving Start3d to 7 sets S3D_GOING (clears STARTSETUP) so it fires once per
	   StartFlying, not on the stale post-flight dialog. The BOB_STARTFLYING harness has its own copy
	   of this (with chaining/trace), so skip here when the harness is active to avoid double-firing. */
	if (!getenv("BOB_STARTFLYING") && Rtestsh1::THISTHIS && !Rtestsh1::tmpinst
	    && (Rtestsh1::setup3dstatus & Rtestsh1::S3D_STARTSETUP)) {
		Rtestsh1::Start3d(Rtestsh1::S3D_DONEBACK);
		Rtestsh1::Start3d(Rtestsh1::S3D_DONESHEET);     /* -> S3D_GOING */
		fprintf(stderr,"[frontend] (bridge) StartFlying -> Launch3d(wasrunning=%d)\n",(int)Rtestsh1::THISTHIS->wasrunning);
		Rtestsh1::THISTHIS->Launch3d(Rtestsh1::THISTHIS->wasrunning);
		fprintf(stderr,"[frontend] (bridge) Launch3d done; tmpinst=%p tmpview=%p InThe3D=%d\n",
			(void*)Rtestsh1::tmpinst,(void*)Rtestsh1::tmpview,(int)Inst3d::InThe3D());
		extern int g_bob_flight_active; g_bob_flight_active = 1;
		return;
	}

	/* S135 (BOB_BOBFRAG, default-off): reach the mission BRIEFING screen (IDD_BOBFRAG, gold #3)
	   headlessly. The real flow is title->QM->Fly->bobfrag->Fly (see bob_startflying_preflight's
	   note); bobfrag's roster comes from CSQuick1::quickdef. So: run the QM click-mode pre-flight
	   (resets currquickmiss to the -1 sentinel + gamestate=HOT), launch `quickmission` so CSQuick1's
	   ctor populates quickdef=quickmissions[0], then LaunchScreen(&bobfrag) -> BoBFragInit -> the
	   BoBFrag roster dialog. We STOP at the briefing (never click its Fly), so no Rtestsh1 is created
	   and BOB_SHOT can capture it. Models BOB_STARTFLYING but targets bobfrag, not the flight. */
	if (getenv("BOB_BOBFRAG") && g_bobActiveFP) {
		static int bf_t = 0; static int bf_phase = 0;
		if (bf_phase == 0) {
			if (++bf_t < 30) return;                       /* let the title screen settle/paint */
			fprintf(stderr,"[bobfrag] pre-flight + LaunchScreen(quickmission)\n");
			bob_startflying_preflight(true, true);         /* click-mode: resets QS sentinel, gamestate=HOT */
			g_bobActiveFP->LaunchScreen(&RFullPanelDial::quickmission);  /* CSQuick1 ctor -> quickdef */
			bf_phase = 1; bf_t = 0; return;
		}
		if (bf_phase == 1) {
			if (++bf_t < 20) return;                       /* let CSQuick1 settle (quickdef ready) */
			fprintf(stderr,"[bobfrag] LaunchScreen(bobfrag)\n");
			g_bobActiveFP->LaunchScreen(&RFullPanelDial::bobfrag);
			bf_phase = 2; return;
		}
		return;   /* hold on the briefing; BOB_SHOT captures it */
	}

	/* R1.1b Increment 4.1 (BOB_STARTFLYING, default-off): drive the game's own
	   menu->flight path in one process. Once the front-end is up + has painted a few idles,
	   run the QM pre-flight then LaunchScreen(quickmissionflight) -- whose InitProc is
	   RFullPanelDial::StartFlying() -> flybox dialog hosting Rtestsh1. Rtestsh1 reaches flight
	   when Start3d accumulates STARTSETUP|DONESHEET|DONEBACK(=7): the ctor sets STARTSETUP, and
	   the two paints set the other bits. Our compat dispatches no WM_PAINT, so feed those two
	   Start3d bits here; that posts WM_GETSTRING -> (Run-loop pump) -> OnGetString -> Launch3d
	   -> new Inst3d/View3d/MakeInteractive. Proves the real transition (not the scaffold). */
	if (getenv("BOB_STARTFLYING")) {
		const char* mode = getenv("BOB_STARTFLYING");
		bool clickMode = mode && strcmp(mode,"click")==0;   /* 4.2: reach Fly by real menu clicks */
		int totalFlights = 1 + (getenv("BOB_REFLY") ? atoi(getenv("BOB_REFLY")) : 0);  /* R2.4 chaining */
		extern int g_bob_flight_active;                     /* per-flight auto-quit re-arm (bob_video) */
		static int sf_t = 0;
		static bool sf_preflighted = false;
		static bool sf_bridged = false;
		static int  sf_flights = 0;                         /* flights started so far */

		/* R2.4 (campaign continuity): a flight we bridged has ended (Rtestsh1 torn down + out of
		   3D, i.e. OnFlyingClosed reached the debrief/menu). If more missions remain, re-arm and
		   fly the next one -- proving the game's debrief->next-mission loop runs in one process. */
		/* "flight ended" = out of 3D + the view/inst the bridge built are gone (OnOK/OnCancel
		   delete tmpinst/tmpview; compat's CDialog::OnOK doesn't destruct the dialog so THISTHIS
		   can linger -- tmpinst==NULL is the reliable signal). The 30-tick pre-flight delay below
		   keeps the bridge from re-firing on the stale dialog before the next LaunchScreen. */
		if (sf_bridged && !Inst3d::InThe3D() && Rtestsh1::tmpinst==NULL) {
			if (sf_flights < totalFlights) {
				fprintf(stderr,"[startfly] === mission %d ended; chaining next (#%d/%d) ===\n",
					sf_flights, sf_flights+1, totalFlights);
				sf_bridged=false; sf_preflighted=false; sf_t=0;   /* re-arm for the next mission */
			} else {
				return;   /* all missions flown; stay in the front-end (final debrief) */
			}
		}
		if (sf_bridged) return;   /* currently flying -- the Run loop owns it */

		/* Pre-flight: one-time device init (firstTime) + per-mission world setup. Inc 4.1 (=1)
		   force-launches the flight screen; inc 4.2 (=click) seeds the state and lets real menu
		   clicks drive OnSelectRlistbox -> FragFly2 -> LaunchScreen(quickmissionflight). */
		if (!sf_preflighted) {
			if (++sf_t < 30) return;                        /* let the prev screen settle/paint */
			fprintf(stderr,"[startfly] BOB_STARTFLYING=%s: pre-flight (mission %d/%d)\n", mode, sf_flights+1, totalFlights);
			bob_startflying_preflight(clickMode, sf_flights==0);
			sf_preflighted = true;
			if (!clickMode) {
				fprintf(stderr,"[startfly] forced LaunchScreen(quickmissionflight)\n");
				g_bobActiveFP->LaunchScreen(&RFullPanelDial::quickmissionflight);  /* fires StartFlying */
			} else {
				fprintf(stderr,"[startfly] navigate to Fly by clicks (use BOB_AUTOCLICK=0,1,2)\n");
			}
			if (!clickMode) return;
		}

		/* Trigger-agnostic Launch3d bridge: the moment StartFlying has created the Rtestsh1
		   dialog (by force OR by a real click), drive Start3d to S3D_GOING and call the game's
		   own public Rtestsh1::Launch3d(wasrunning) -- exactly what OnGetString does (compat has
		   no message-map dispatch, so the posted WM_GETSTRING is swallowed; we deliver it). */
		if (!sf_bridged && Rtestsh1::THISTHIS && !Rtestsh1::tmpinst) {
			Rtestsh1::Start3d(Rtestsh1::S3D_DONEBACK);
			Rtestsh1::Start3d(Rtestsh1::S3D_DONESHEET);
			fprintf(stderr,"[startfly] Rtestsh1 up -> Launch3d(wasrunning=%d)\n",(int)Rtestsh1::THISTHIS->wasrunning);
			Rtestsh1::THISTHIS->Launch3d(Rtestsh1::THISTHIS->wasrunning);
			fprintf(stderr,"[startfly] Launch3d returned; tmpinst=%p tmpview=%p\n",
				(void*)Rtestsh1::tmpinst,(void*)Rtestsh1::tmpview);
			sf_bridged = true; sf_flights++; g_bob_flight_active = 1;   /* flight #sf_flights live */
			return;
		}
		if (sf_bridged) return;   /* flight is up; the Run-loop / Inst3d path takes over */
		/* clickMode, not yet flying: fall through to the click dispatch so BOB_AUTOCLICK /
		   real clicks navigate the menu toward Fly. */
	}
	int cx, cy; bool got = bob_gdi_get_click(&cx, &cy);
	/* BOB_AUTOCLICK=N: headless test -- after a delay synthesize a click on item N's centre
	   so the real hit-test path below runs (verifies click -> navigation without a display). */
	/* BOB_AUTOCLICK supports a comma-separated SEQUENCE (e.g. "5,3" = open PC Config,
	   then click the Sound tab) -- one click every 120 ticks, to walk into sub-screens. */
	const char* ac = getenv("BOB_AUTOCLICK");
	if (!got && ac) { static int t=0; int& seq = g_bob_autoclick_seq;   /* S143: observable by the shot banner */
		bool sideSel = bob_on_sideselect();   /* R4.1: art-polygon screen has no text menu */
		if (++t % 120 == 0 && (g_bobMenuN>0 || sideSel)) {
			const char* p = ac; for (int k=0; k<seq && p; k++) { p=strchr(p,','); if(p) p++; }
			/* S141: a "#ID[:COL]" step clicks a hosted PANEL control by its dialog id
			   (optionally a listbox column) instead of a menu-item index -- the point is
			   resolved from the control's own drawn rect + column metrics. This is how the
			   campaign PHASE is chosen headlessly (#1000:1 = IDC_RLIST_CAMPAIGNS, Eagle
			   Attack), and it follows MA S62/S63's rule that a drive recipe must never
			   encode fixed pixels. */
			if (p && *p=='#') {
				int cid = atoi(p+1), ccol = 0;
				const char* c = strchr(p+1, ':');
				if (c && (!strchr(p,',') || c < strchr(p,','))) ccol = atoi(c+1);
				for (int d=0; d<3 && !got; d++) {
					RDialog* pd = g_bobActiveFP->pdial[d];
					if (pd && bob_ole_ctrl_point((CWnd*)pd, cid, ccol, &cx, &cy)) got=true;
				}
				if (got) { seq++; fprintf(stderr,"[frontend] autoclick step #%d:%d -> (%d,%d)\n", cid, ccol, cx, cy); }
				else fprintf(stderr,"[frontend] autoclick step #%d:%d -- control not drawn yet, retrying\n", cid, ccol);
			}
			else if (p && *p) {
				int n=atoi(p);
				int resW = RFullPanelDial::resolutions[g_bobActiveFP->m_currentres];
				if (sideSel && bob_sideselect_centroid(n, resW, &cx, &cy)) { got=true; seq++; }
				else if (g_bobMenuN>0) {
					if(n<0||n>=g_bobMenuN) n=0;
					cx=g_bobMenuRect[n].x+g_bobMenuRect[n].w/2; cy=g_bobMenuRect[n].y+g_bobMenuRect[n].h/2; got=true;
					seq++;
				}
			}
		}
	}
	if (got) {
		bool consumed = false;
		for (int i=0; i<g_bobMenuN; i++)
			if (cx>=g_bobMenuRect[i].x && cx<g_bobMenuRect[i].x+g_bobMenuRect[i].w &&
			    cy>=g_bobMenuRect[i].y && cy<g_bobMenuRect[i].y+g_bobMenuRect[i].h) {
				fprintf(stderr,"[frontend] click (%d,%d) -> menu item %d\n", cx, cy, i);
				g_bobActiveFP->OnSelectRlistbox(i, 0);
				consumed = true;
				break;
			}
		/* R4.1: Campaigns side-select -- hit-test the RAF/LW/Cancel art polygons (faithful
		   SideSelectOutlines) and fire the real OnSelectRlistbox navigation. */
		if (!consumed && bob_on_sideselect()) {
			int resW = RFullPanelDial::resolutions[g_bobActiveFP->m_currentres];
			int hit = bob_sideselect_hit(cx, cy, resW);
			if (hit >= 0) {
				fprintf(stderr,"[frontend] click (%d,%d) -> side-select item %d (%s)\n", cx, cy, hit,
					hit==0?"RAF":hit==1?"Luftwaffe":"Cancel");
				g_bobActiveFP->OnSelectRlistbox(hit, hit);
				consumed = true;
			}
		}
		/* missed the menu -> try the active panels' hosted combos (cycle on click) */
		if (!consumed && bobOleDraw())
			for (int d=0; d<3; d++) {
				RDialog* pd = g_bobActiveFP->pdial[d];
				if (pd && bob_ole_click((CWnd*)pd, cx, cy)) {
					fprintf(stderr,"[frontend] click (%d,%d) -> combo cycled (panel %d)\n", cx, cy, d);
					bob_fp_repaint(g_bobActiveFP);
					break;
				}
			}
	}
}
#endif
void RFullPanelDial::LaunchMain(FullScreen* startscreen)
{ // launches the main dialog window
	CRect rect;
//DeadCode DAW 18Jun99 	if (GetCurrentRes()==0)
//DeadCode DAW 18Jun99 		MessageBox("Resolution is too small to run Mig Alley - this message to be removed","Warning",MB_OK);
//DeadCode DAW 18Jun99 	else
	{

	m_pView->GetClientRect(rect);
	RDialog* dial=RDialog::MakeTopDialog(rect,
								DialBox(FIL_NULL,this,EDGES_NOSCROLLBARS_NODRAGGING)
							);
// launch the first sheet...
//	CWnd* parent=GetParent()->GetParent();
//	ASSERT(parent==m_pView);
	GetParent()->BringWindowToTop();
	m_pStartScreen=startscreen;
	LaunchScreen(startscreen);
	UpdateSize();
#if BOB_LINUX
	// Initial launch: UpdateSize() has now selected the resolution + correct artnum, so paint
	// the screen (LaunchScreen ran before res selection). No WM_PAINT dispatch on Linux.
	if (getenv("BOB_FRONTEND")) {
		DoPaint(GetDC());
		bob_draw_menu(this, m_currentscreen, resolutions[m_currentres]);
		bob_gdi_present();
		fprintf(stderr, "[frontend] LaunchMain painted artnum=%d res=%d + menu + presented\n",
			(int)artnum, (int)resolutions[m_currentres]);
	}
#endif
	}
}

void RFullPanelDial::LaunchDial(RDialog* dial, UINT dialID,FileNum art) // typically pass new CSoundDialog for example
{
	if (pdial[dialID]) 
	{
		pdial[dialID]->PreDestroyPanel();
		pdial[dialID]->DestroyPanel();
	}
//	dial->Create(dial->MY_IID,this);
	// scale the dialog if necessary due to font size change
//	dial->m_scalingfactor=(float)fontdpi/96;
//	dial->ScaleDialog(1);
	if (art==FIL_NULL)
		art=m_currentscreen->resolutions[m_currentres].artwork;
	pdial[dialID]=AddPanel(dial,
		m_currentscreen->resolutions[m_currentres].dials[dialID].flags & FullScreen::Dial::X2, 
		CRect(	m_currentscreen->resolutions[m_currentres].dials[dialID].X,
				m_currentscreen->resolutions[m_currentres].dials[dialID].Y,
				0,
				m_currentscreen->resolutions[m_currentres].dials[dialID].flags  & FullScreen::Dial::CENTRE
			),
		art,
		EDGES_NOSCROLLBARS_NODRAGGING);
	pdial[dialID]->m_bDrawBackground=pdial[dialID]->m_bDrawBackground||enablepanelselfdrawstate;
//	dial->ShowWindow(SW_SHOW);
}
void RFullPanelDial::LaunchDial(UINT dialID,const DialBox* d) // typically pass new CSoundDialog for example
{
	if (pdial[dialID]) 
	{
		pdial[dialID]->PreDestroyPanel();
		pdial[dialID]->DestroyPanel();
	}
//	dial->Create(dial->MY_IID,this);
	// scale the dialog if necessary due to font size change
//	dial->m_scalingfactor=(float)fontdpi/96;
	pdial[dialID]=AddPanel(d,
				m_currentscreen->resolutions[m_currentres].dials[dialID].flags & FullScreen::Dial::X2, 
				CRect(	m_currentscreen->resolutions[m_currentres].dials[dialID].X,
						m_currentscreen->resolutions[m_currentres].dials[dialID].Y,
						0,
						m_currentscreen->resolutions[m_currentres].dials[dialID].flags  & FullScreen::Dial::CENTRE
					)
				);
	pdial[dialID]->m_bDrawBackground=pdial[dialID]->m_bDrawBackground||enablepanelselfdrawstate;
//	dial->ShowWindow(SW_SHOW);
}


//extern	Smack*	OpenSmack(FileNum filenumber,int win,int X,int Y,int w,int h);
//extern	cdecl	UWord	DoSmack(int poo);

FileNum	lastsmacker;

void RFullPanelDial::LaunchSmacker(int dialID,FileNum smackerID)
{
	int	X=	m_currentscreen->resolutions[m_currentres].dials[dialID].X;
	int	Y=	m_currentscreen->resolutions[m_currentres].dials[dialID].Y;
//	if (lastsmacker!=smackerID)	//new video
//	{
	pleasestartthissmacker=smackerID;
		if (smackerID)	//If we have a video to play
		{
			File_Man.MakeRoomFor(10000000);	//force 10MB to be dumped...
			File_Man.MakeRoomFor(-10000000);
			File_Man.DiscardDirList(smackerID);
			Bool exist=File_Man.ExistNumberedFile(smackerID);
#ifdef	BOB_DEMO_VER
			if (!exist)		  //demo can live without videos
			{
				pleasestartthissmacker=FIL_NULL;
				PostMessage(WM_SELECTTAB,0,0);
			}
			else
#endif
			{

//DeadCode JIM 19Oct00 MCIWNDF_NOAUTOSIZEWINDOW	Will not change the dimensions of an MCIWnd window when the image size changes. 
//DeadCode JIM 19Oct00 MCIWNDF_NOAUTOSIZEMOVIE	Will not change the dimensions of the destination rectangle when an MCIWnd window size changes.  
//DeadCode JIM 19Oct00 MCIWNDF_NOERRORDLG		Inhibits display of MCI errors to users. 
//DeadCode JIM 19Oct00 MCIWNDF_NOMENU			Hides the Menu button from view on the toolbar and prohibits users from accessing its pop-up menu. 
//DeadCode JIM 19Oct00 MCIWNDF_NOOPEN			Hides the open and close commands from the MCIWnd menu and prohibits users from accessing these choices in the pop-up menu. 
//DeadCode JIM 19Oct00 MCIWNDF_NOPLAYBAR		Hides the toolbar from view and prohibits users from accessing it. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYANSI		Causes MCIWnd to use an ANSI string instead of a Unicode string when notifying the parent window of device mode changes. This flag is used in combination with MCIWNDF_NOTIFYMODE and is exclusive to Windows NT/Windows 2000. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYMODE		Causes MCIWnd to notify the parent window with an MCIWNDM_NOTIFYMODE message whenever the device changes operating modes. The lParam parameter of this message identifies the new mode, such as MCI_MODE_STOP. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYPOS		Causes MCIWnd to notify the parent window with an MCIWNDM_NOTIFYPOS message whenever a change in the playback or record position within the content occurs. The lParam parameter of this message contains the new position in the content. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYMEDIA		Causes MCIWnd to notify the parent window with an MCIWNDM_NOTIFYMEDIA message whenever a new device is used or a data file is opened or closed. The lParam parameter of this message contains a pointer to the new filename. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYSIZE		Causes MCIWnd to notify the parent window when the MCIWnd window size changes. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYERROR		Causes MCIWnd to notify the parent window when an MCI error occurs. 
//DeadCode JIM 19Oct00 MCIWNDF_NOTIFYALL		Causes all MCIWNDF window notification styles to be used. 
//DeadCode JIM 19Oct00 MCIWNDF_RECORD			Adds a Record button to the toolbar and adds a new file command to the menu if the MCI device has recording capability.  
//DeadCode JIM 19Oct00 MCIWNDF_SHOWALL			Causes all MCIWNDF_SHOW styles to be used. 
//DeadCode JIM 19Oct00 MCIWNDF_SHOWMODE			Displays the current mode of the MCI device in the window title bar. For a list of device modes, see the MCIWndGetMode macro. 
//DeadCode JIM 19Oct00 MCIWNDF_SHOWNAME			Displays the name of the open MCI device or data file in the MCIWnd window title bar. 
//DeadCode JIM 19Oct00 MCIWNDF_SHOWPOS			Displays the current position within the content of the MCI device in the window title bar. 

//DeadCode JON 10Nov00 #pragma warnmsg ( "remove the g_hwndMCIWnd" )
//DeadCode JON 10Nov00 				g_hwndMCIWnd=MCIWndCreate(m_hWnd,g_hinst,WS_CHILD|WS_VISIBLE|MCIWNDF_NOTIFYMODE|MCIWNDF_NOPLAYBAR|MCIWNDF_NOMENU,FILEMAN.namenumberedfile(smackerID));
//DeadCode JON 10Nov00 				::SetWindowPos(g_hwndMCIWnd,NULL,X,Y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
//			MCIWndPlay(g_hwndMCIWnd);				//MCI did not run on all our machines. Switched to DirectMedia

			WCHAR wFile[MAX_PATH];
		    MultiByteToWideChar( CP_ACP, 0, FILEMAN.namenumberedfile(smackerID), -1, wFile, MAX_PATH );

			HRESULT hr =
				CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pigb);

			if (SUCCEEDED(hr))
			{ // Graphbuilder instance

				// QueryInterface for some basic interfaces
				pigb->QueryInterface(IID_IMediaControl, (void **)&pimc);
				pigb->QueryInterface(IID_IMediaEventEx, (void **)&pimex);
				pigb->QueryInterface(IID_IVideoWindow, (void **)&pivw);
				pigb->QueryInterface(IID_IBasicAudio, (void **)&videoAudio);

				// Have the graph construct its the appropriate graph automatically
				hr = pigb->RenderFile(wFile, NULL);

				pivw->put_Owner((OAHWND)m_hWnd);
				pivw->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);

				// Have the graph signal event via window callbacks for performance
				pimex->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0);

				if ( smackerID == FIL_VID_INTRO )
				{ // don't bother scaling..
					pivw->put_Left( X );
					pivw->put_Top( Y );
				} else
				{	
//DeadCode JON 12Nov00 					RECT grc;
//DeadCode JON 12Nov00 					grc.left = X;
//DeadCode JON 12Nov00 					grc.top = Y;
//DeadCode JON 12Nov00 					grc.right = X+videoWidth;
//DeadCode JON 12Nov00 					grc.bottom = Y+videoHeight;
					pivw->SetWindowPosition(X, Y, videoWidth, videoHeight);	
				}
//DeadCode JON 10Nov00 				GetClientRect(ghApp, &grc);
//DeadCode JON 10Nov00 				pivw->SetWindowPosition(grc.left, grc.top, grc.right, grc.bottom);

				// Run the graph if RenderFile succeeded
				if (!SUCCEEDED(hr))
					CloseVideo();
			//	  pimc->Run();

			} // Graphbuilder instance


				lastsmacker=smackerID;
				if (m_currentscreen->textlists[0].text==NULL)
					SetCapture();
			}
		}
		else
		{			//stop video
//DeadCode JON 10Nov00 			if (g_hwndMCIWnd)
//DeadCode JON 10Nov00 			{
//DeadCode JON 10Nov00 				MCIWndClose(g_hwndMCIWnd);
				CloseVideo();
				lastsmacker=(FileNum)-1;
//DeadCode JON 10Nov00 			}
		}
//	}else{
//	}
}
void RFullPanelDial::ChangeSmacker(int dialID,FileNum aviID)
{
//DeadCode JON 10Nov00 	MCIWndClose(g_hwndMCIWnd);
	CloseVideo();
	LaunchSmacker(dialID,aviID);
}


void RFullPanelDial::OnTimer(UINT nIDEvent) 
{
	// Nick the Enum loop thing
	if (nIDEvent==m_timerID)
	{
//DeadCode DAW 25Feb100 		if (!DoSmack((int)m_hWnd))
//DeadCode DAW 25Feb100 		{
//DeadCode DAW 25Feb100 //			Master_3d.BigWin();
//DeadCode DAW 25Feb100 //			Invalidate();
//DeadCode DAW 25Feb100 			CloseSmack();
//DeadCode DAW 25Feb100 			OnLButtonUp(0, CPoint (0,0)) ;
//DeadCode DAW 25Feb100 			//KillTimer(m_timerID);
//DeadCode DAW 25Feb100 			//m_timerID=0;
//DeadCode DAW 25Feb100 		}
	}
}

void RFullPanelDial::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_currentscreen && m_currentscreen->textlists[0].text==NULL) 
	{
		OnSelectRlistbox(0,0);
//DEADCODE DAW 15/06/99 		UpdateSize();
//DEADCODE DAW 15/06/99 
//DEADCODE DAW 15/06/99 		_Miles.InitSoundFonts();		//RJS 24Nov98
	}
//	{
//		LaunchScreen();
//		UpdateSize();
//	}

}

int RFullPanelDial::GetCurrentRes()
{ // the screen height is assumed to be 3/4 the screen width...
	CRect rect;
//	m_pView->GetClientRect(rect);
	AfxGetMainWnd()->GetWindowRect(rect); // a slightly more accurate representation
	int bestresX=0;
	int res;
	for (res=0;res<6;res++)
	{
		if (resolutions[res] &&
			rect.Width()>=resolutions[res] && 
			rect.Width()-resolutions[res]<=rect.Width()-resolutions[bestresX])
			bestresX=res;
	}
	int bestresY=0;
	for (res=0;res<6;res++)
	{
		if (resolutions[res] &&
			rect.Height()*4>=resolutions[res]*3 && 
			rect.Height()-resolutions[res]<=rect.Width()-resolutions[bestresY])
			bestresY=res;
	}
//	if (resolutions[bestresX]<=resolutions[bestresY]) return bestresX;
//	return bestresY;
	if (resolutions[bestresX]<=resolutions[bestresY]) res = bestresX;
	else res=bestresY;
	// now check if the artwork for this resolution is available, if not
	// search downwards until you find one that is there (Note there must be
	// one at the lowest resolution...
//	if (!m_currentscreen->resolutions[res].artwork)
//		return res;
	if (m_currentscreen)
		if (!m_currentscreen->resolutions[res].artwork)
			return res;
		else{}
	else
		return res;


	if (m_currentscreen)
	{
		if (!m_currentscreen->resolutions[res].artwork) return res;
		while (!m_currentscreen->resolutions[res].artwork || !FILEMAN.existnumberedfile(m_currentscreen->resolutions[res].artwork))
		{
			// find the next lowest resolution...
			int currentres=resolutions[res];
			int closestres=0;
			for (int newres=0;newres<6;newres++)
			{
				if (resolutions[newres]<currentres && resolutions[newres]>closestres)
				{
					closestres=resolutions[newres];
					res=newres;
				}
			}
			if (closestres==0)
			{
//				ASSERT(FALSE); // cannot find artwork for this sheet make sure there
								// is one at the lowest resolution...
				return 0;
			}
		}
	}
	return res;
}


void RFullPanelDial::LaunchScreen(FullScreen * pfullscreen)
{
	pleasestartthissmacker=FIL_NULL;
	bool	closedsmack=false;
//DeadCode DAW 25Feb100 	if (m_timerID) 
//DeadCode DAW 25Feb100 	{
//DeadCode DAW 25Feb100 		CloseSmack();
//DeadCode DAW 25Feb100 		Master_3d.BigWin();
//DeadCode DAW 25Feb100 		KillTimer(m_timerID);
//DeadCode DAW 25Feb100 		m_txosedsmack=true;
//DeadCode DAW 25Feb100 	}
//DeadCode JON 10Nov00 	if (g_hwndMCIWnd)
	if ( pivw )
	{
		CloseVideo();
//DEADCODE RDH 18/04/00 		OnMciNotify(0,MCI_MODE_STOP);
//DEADCODE RDH 18/05/00 		Save_Data.ChangeMode(SaveData::UIR_SCRAMBLE);
		closedsmack=true;
//DEADCODE RDH 18/05/00 		ReleaseCapture();
	}
	polylist.Clear();
	pdial[0]=pdial[1]=pdial[2]=NULL;
	Invalidate();
	m_currentscreen=pfullscreen;
#if defined(BOB_LINUX)
	// S57: FullScreen::resolutions is [6] (FULLPANE.H:35) and m_currentres starts -1
	// (constructor). LaunchScreen reads resolutions[m_currentres] but -- unlike the
	// lazy-init guard the engine already has at fullpsys.cpp:1442 -- doesn't ensure it's
	// set, so the first (startup) LaunchScreen indexes resolutions[-1]/out-of-range
	// (ASan global-buffer-overflow READ at :1222, via Initialise->LaunchMain->LaunchScreen).
	// Apply the same guard here, and bound it to [0,5] against any stale value. See PORT.md (S57).
	if (m_currentres < 0 || m_currentres >= 6)
		m_currentres = GetCurrentRes();
	if (m_currentres < 0 || m_currentres >= 6)
		m_currentres = 0;
#endif
	artnum=pfullscreen->resolutions[m_currentres].artwork;
	PositionRListBox();
	localnote=NULL;
	enablepanelselfdrawstate=false;
	if (m_currentscreen->InitProc) (this->*m_currentscreen->InitProc)();
	enablepanelselfdrawstate=false;
#if BOB_LINUX
	// Workstream A: no WM_PAINT dispatch on Linux, so drive the front-end paint here.
	// DoPaint draws this screen's background artwork (artnum) into the GDI framebuffer
	// (SetDIBitsToDevice -> bob_gdi_setdibits); present uploads + swaps it to the window.
	if (getenv("BOB_FRONTEND")) {
		DoPaint(GetDC());                                   /* screen background */
		for (int d=0; d<3; d++) if (pdial[d]) {             /* dial panels (config widgets) */
			pdial[d]->DoPaint(GetDC());
			if (bobOleDraw()) {                             /* draw the panel's hosted R* controls
			       at the panel's screen origin (FullScreen dial X/Y) + template offsets */
				int ox = m_currentscreen->resolutions[m_currentres].dials[d].X;
				int oy = m_currentscreen->resolutions[m_currentres].dials[d].Y;
				bob_ole_draw_panel((CWnd*)pdial[d], ox, oy);
				bob_fp_draw_nested(pdial[d], ox, oy);   /* S133: nested DialList rows (QS OOB) */
			}
			fprintf(stderr, "[frontend]   dial[%d] painted\n", d);
		}
		bob_draw_menu(this, m_currentscreen, resolutions[m_currentres]);  /* tabs / menu items */
		bob_gdi_present();
		fprintf(stderr, "[frontend] painted screen artnum=%d + dials + menu + presented\n", (int)artnum);
	}
#endif
}

BEGIN_EVENTSINK_MAP(RFullPanelDial, CDialog)
    //{{AFX_EVENTSINK_MAP(RFullPanelDial)
	ON_EVENT(RFullPanelDial, IDC_RLISTBOX, 1 /* Select */, OnSelectRlistbox, VTS_I4 VTS_I4)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void RFullPanelDial::OnSelectRlistbox(long row, long column) 
{
	int x=max(row,column);
	Bool retval=TRUE;

	//Nuke any running smacker
//	CloseSmack();
//	OnMciNotify(0,MCI_MODE_STOP);

	FullScreen* nextscreen=m_currentscreen->textlists[x].nextscreen;
	if (m_currentscreen->textlists[x].onselect) 
		retval=(this->*m_currentscreen->textlists[x].onselect)(nextscreen);
	if (retval && nextscreen)
	{
		// Nuke all panels then launch relevant sheet.
		RDialog* nextchild;

		for (int dialID=0;dialID<3;dialID++)
			if (pdial[dialID])
			{
				pdial[dialID]->PreDestroyPanel();
				pdial[dialID]->DestroyPanel();
				pdial[dialID]=NULL;
			}
		fchild=NULL;
		LaunchScreen(nextscreen);
	}
}

void RFullPanelDial::UpdateSize()
{
	// first move this so that it fits one of the resolutions
	int m_prevresart=m_currentscreen->resolutions[m_currentres].artwork;
	m_currentres=GetCurrentRes();
	int width=resolutions[m_currentres];
	int height=(resolutions[m_currentres]*3)/4;
	if (resolutions[m_currentres]==1280)
		height=1024;
	artnum=m_currentscreen->resolutions[m_currentres].artwork;
	CRect rect;
	AfxGetMainWnd()->GetClientRect(rect);
	if (m_currentscreen->textlists[0].text==NULL && height!=600)
		NOP;
//DEADCODE RDH 18/05/00 	{
//DEADCODE RDH 18/05/00 		parent->MoveWindow(rect);
//DEADCODE RDH 18/05/00 		MoveWindow(0,0,rect.Width(),rect.Height());
//DEADCODE RDH 18/05/00 	}
//DEADCODE RDH 18/05/00 	else
	{
		parent->MoveWindow((rect.Width()-width)/2,(rect.Height()-height)/2,
				width,height);
		MoveWindow(0,0,width,height);
	}
	PositionRListBox();
	for (int x=0;x<3;x++)
	{
		if (pdial[x])
		{
			if (pdial[x]->artnum==m_prevresart)
				pdial[x]->artnum=artnum;
			pdial[x]->MoveWindow(GetDialPosition(pdial[x],x));
			if (m_currentscreen->resolutions[m_currentres].dials[x].flags & FullScreen::Resolutions::Dial::X2) pdial[x]->ScaleDialog(2);
			else pdial[x]->ScaleDialog(1);
		}
	}
}

CRect RFullPanelDial::GetDialPosition(RDialog * dial, UINT dialID)
{
	CRect rect;
	dial->GetClientRect(rect);
	CPoint bottomright=rect.BottomRight();
	rect+=CPoint(m_currentscreen->resolutions[m_currentres].dials[dialID].X,
					 m_currentscreen->resolutions[m_currentres].dials[dialID].Y);
	if (m_currentscreen->resolutions[m_currentres].dials[dialID].flags & FullScreen::Resolutions::Dial::CENTRE)
	{
		bottomright.x/=2;
		bottomright.y/=2;
		rect-=bottomright;
	}
	return rect;
}

void RFullPanelDial::PositionRListBox()
{
	CRListBox* prlistbox;
	prlistbox=GETDLGITEM(IDC_RLISTBOX);
	prlistbox->SetBorder(FALSE);
	prlistbox->Clear();
	int seperation=100;

	if (m_currentscreen->listalign & FullScreen::Align::HORIZ)
	{	// a horizontal listbox...
		// spacing depends on resolution...
		for (int x=0;x<10;x++)
		{
			if (m_currentscreen->textlists[x].text==NULL) break;
			prlistbox->AddColumn(seperation);
			CString string;
			string.LoadString(m_currentscreen->textlists[x].text);
			prlistbox->AddString(string,x);
		}
	}
	else // a vertical one
	{
		prlistbox->AddColumn(100);
		for (int x=0;x<10;x++)
		{
			if (m_currentscreen->textlists[x].text==NULL) break;
			CString string;
			string.LoadString(m_currentscreen->textlists[x].text);
			prlistbox->AddString(string,0);
		}
	}
	// move to correct position.
	prlistbox->Shrink();
	prlistbox->ResizeToFit();
	CRect rect;
	prlistbox->GetClientRect(rect);
	CPoint bottomright=rect.BottomRight();
	CPoint topleft(m_currentscreen->resolutions[m_currentres].ListX,m_currentscreen->resolutions[m_currentres].ListY);
	switch (m_currentscreen->listalign&0xF8)
	{
		case FullScreen::Align::TOP:
			break;
		case FullScreen::Align::BOT:
			bottomright.x/=2;
			bottomright.y/=2;
			rect-=bottomright;
			break;
		case FullScreen::Align::MID:
			topleft.x-=bottomright.x/2;
			rect-=bottomright;
			break;
	}
	switch (m_currentscreen->listalign&0x3)
	{
		case FullScreen::Align::LEFT:
			prlistbox->SetCentred(FALSE);
			break;
		case FullScreen::Align::RIGHT:
			prlistbox->SetCentred(FALSE);
			break;
		case FullScreen::Align::CENTRE:
			prlistbox->SetCentred(TRUE);
			break;
		case FullScreen::Align::ONSPACE:
			prlistbox->SetCentred(FALSE);
			break;
	}
	prlistbox->MoveWindow(topleft.x,topleft.y,bottomright.x,bottomright.y);
}

// some utility procedures for launching map or 3d
#ifndef	BOB_DEMO_VER

extern "C" int g_bob_map_active;   /* R4.2: front-end is showing the strategic map (paint via the tick) */
Bool RFullPanelDial::LaunchMap(FullScreen*&fs, bool firsttime)
{
//DeadCode DAW 25Feb100 	CloseSmack();
	CloseVideo();
//DEADCODE RDH 18/04/00 	OnMciNotify(0,MCI_MODE_STOP);
	Save_Data.ChangeMode(SaveData::UIR_MAP);
//	Persons4::StartUpMapWorld();
	Node_Data.SetPlayerInfoLevel();
	m_pView->LaunchMap(firsttime);
#if BOB_LINUX
	g_bob_map_active = 1;   /* the map tick (bob_frontend_tick) now paints CMIGView::UpdateBitmaps */
	/* S94: the map should start PAUSED (faithful — the real game freezes until the player clicks an
	   accel button; the boot scaffold otherwise leaves curracceltype at RAIDSPD, auto-running the
	   sim). Start paused so the default map is frozen + safe; the accel buttons (Play/FF) run it. */
	MMC.curracceltype = MMC.ACCEL_PAUSED;
	MMC.curraccelrate = MMC.accelrates[MMC.ACCEL_PAUSED];
	fprintf(stderr,"[map] LaunchMap done -> strategic map active (firsttime=%d, paused)\n", (int)firsttime);
#endif
	return FALSE;
}
#endif

Bool RFullPanelDial::Launch3d(FullScreen*&fs)
{
	CloseVideo();
//DEADCODE RDH 18/04/00 	OnMciNotify(0,MCI_MODE_STOP);
//	closedsmack=true;
//DeadCode DAW 25Feb100 	CloseSmack();
	return TRUE;
}

CFont* RFullPanelDial::OnGetGlobalFont(int fontnum)
{
	if (m_currentres==-1) m_currentres=GetCurrentRes();
	if (g_AllFonts[fontnum][1])
	{
//DeadCode JIM 20Oct00 		int currentres=resolutions[m_currentres];
		if (m_currentres>=3)
			return g_AllFonts[fontnum][3];
		else
			return g_AllFonts[fontnum][m_currentres];
//DEADCODE MS 01/02/00 		if (currentres>=1280) return g_AllFonts[fontnum][3];
//DEADCODE MS 01/02/00 		else if (currentres>=1024) return g_AllFonts[fontnum][2];
//DEADCODE MS 01/02/00 		else if (currentres>=800) return g_AllFonts[fontnum][1];
//DEADCODE MS 01/02/00 		else return g_AllFonts[fontnum][0];	
	}
	else if (m_scalingfactor==2) return g_AllFonts[fontnum][3];
	else return g_AllFonts[fontnum][0];
}

void	RFullPanelDial::SetTitleText(bool clear)
{	
	if (clear)
		AfxGetMainWnd()->SetWindowText(RESSTRING(BOB));
	else
	if (Miss_Man.currcampaignnum!=Miss_Man.SCRAMBLECAMPAIGN)
		AfxGetMainWnd()->SetWindowText(RESSTRING(BOB)+": "+LoadResString(IDS_L_SCAMPAIGNSELECT1+Miss_Man.currcampaignnum));
	else
		if (CSQuick1::currquickmiss>=0)
		AfxGetMainWnd()->SetWindowText(RESSTRING(BOB)+": "+LoadResString(CSQuick1::quickmissions[CSQuick1::currquickmiss].missionname));
}

LRESULT RFullPanelDial::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::WindowProc(message, wParam, lParam);
}

BOOL RFullPanelDial::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::PreCreateWindow(cs);
}

int RFullPanelDial::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}

BOOL RFullPanelDial::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void RFullPanelDial::OnEnable(BOOL bEnable) 
{
	CDialog::OnEnable(bEnable);
	
	// TODO: Add your message handler code here
	
}


void RFullPanelDial::OnPaint() 
{
	if (artnum)
		CDialog::OnPaint();
	else
	{
		PAINTSTRUCT paintstruct;
		CDC* pDC;
		pDC=BeginPaint(&paintstruct);
		Rtestsh1::Setup3dStatuses state=Rtestsh1::Start3d(Rtestsh1::S3D_DONEBACK);
		if (state==Rtestsh1::S3D_STOPPED)
		{
			paintstruct.fErase=TRUE;
			CRect rect;
			GetClientRect(rect);
			pDC->FillSolidRect(rect,RGB(0,0,0));
		}
		else
		if (state==Rtestsh1::S3D_STARTSETUP)
		{
			paintstruct.fErase=TRUE;
			CRect rect;
			GetClientRect(rect);
			pDC->FillSolidRect(rect,RGB(0,0,0));	//RGB(160,184,208));
		}
		EndPaint(&paintstruct);
		
	}
//DeadCode JON 10Nov00 	if (pleasestartthissmacker && g_hwndMCIWnd)
	if (pleasestartthissmacker && pimc)
	{
		pimc->Run();
		videoAudio->put_Volume( _Miles.SmackerOn(0) );
//DeadCode JON 10Nov00 _Miles.SmackerOn(int(g_hwndMCIWnd));									//RJS 18Oct00
//DeadCode JON 10Nov00 		MCIWndPlay(g_hwndMCIWnd);
	}

	pleasestartthissmacker=FIL_NULL;
}

////////////////////////////////////////////////////////////////////////
//
// Function:    OnGetArt
// Date:        18/01/99
// Author:      JIM
//
// Description: 
//
////////////////////////////////////////////////////////////////////////
FileNum RFullPanelDial::OnGetArt()
{
	return artnum;
}


void	RFullPanelDial::ScaleHitAreas(int polygonscale)	//1024 default
{
	polylist.scale=polygonscale/resolutions[m_currentres];
}

// this closes the video
void RFullPanelDial::CloseVideo()
{
    if (pivw)
	{ // Relinquish ownership (IMPORTANT!) after hiding
		pivw->put_Visible(OAFALSE);
		pivw->put_Owner(NULL);
		HELPER_RELEASE(pivw)
		ReleaseCapture();;
		_Miles.SmackerOff();											//RJS 18Oct00
	} // Relinquish ownership (IMPORTANT!) after hiding

    HELPER_RELEASE(pigb);
    HELPER_RELEASE(pimc);
    HELPER_RELEASE(pimex);
	HELPER_RELEASE(videoAudio);

//DeadCode JON 10Nov00 	if (g_hwndMCIWnd)
//DeadCode JON 10Nov00 	{
//DeadCode JON 10Nov00 
//DeadCode JON 10Nov00 		MCIWndClose(g_hwndMCIWnd);
//DeadCode JON 10Nov00 		MCIWndDestroy(g_hwndMCIWnd);
//DeadCode JON 10Nov00 		g_hwndMCIWnd=NULL;
//DeadCode JON 10Nov00 		ReleaseCapture();
//DeadCode JON 10Nov00 		_Miles.SmackerOff();											//RJS 18Oct00
//DeadCode JON 10Nov00 	}																	//JIM 23Oct00
}

void RFullPanelDial::OnMciDidQuit(int wParam,int lParam)
{
	OnLButtonUp(0, CPoint (0,0)); //terminate the sheet also


}

// this function is called when a message is recieved from the smacker...	
void RFullPanelDial::OnMciNotify(int wParam,int lParam)
{
	LONG		evCode;
	LONG		evParam1;
	LONG		evParam2;
	HRESULT		hr;

	if ( pimex ) // check we've not cloded it down already
		while (SUCCEEDED(pimex->GetEvent(&evCode, &evParam1, &evParam2, 0)))
		{ // Spin through the events

			hr = pimex->FreeEventParams(evCode, evParam1, evParam2);

			if (EC_COMPLETE == evCode)
			{ // Finished
				CloseVideo();
				PostMessage(WM_SELECTTAB,0,0);
				break;
			} // Finished
		} // Spin through the events

//DeadCode JON 10Nov00 	switch (lParam)
//DeadCode JON 10Nov00 	{
//DeadCode JON 10Nov00 	case	MCI_MODE_STOP:
//DeadCode JON 10Nov00 		CloseVideo();
//DeadCode JON 10Nov00 		PostMessage(WM_SELECTTAB,0,0);
//DeadCode JON 10Nov00 //DeadCode DAW 2Aug00 		TRACE0("MCI STOP");
//DeadCode JON 10Nov00 //DeadCode JIM 2Aug00 		ReleaseCapture();
//DeadCode JON 10Nov00 //DEADCODE RDH 18/04/00 				MCIWndClose(g_hwndMCIWnd);
//DeadCode JON 10Nov00 //DEADCODE RDH 18/04/00 				MCIWndDestroy(g_hwndMCIWnd);
//DeadCode JON 10Nov00 //DEADCODE RDH 18/04/00 				g_hwndMCIWnd=NULL;
//DeadCode JON 10Nov00 //DeadCode DAW 2Aug00 				OnLButtonUp(0, CPoint (0,0)); //terminate the sheet also
//DeadCode JON 10Nov00 	break;
//DeadCode JON 10Nov00 	default:
//DeadCode JON 10Nov00 //		wParam=lParam;
//DeadCode JON 10Nov00 	break;
//DeadCode JON 10Nov00 	}

}

BOOL RFullPanelDial::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	return CDialog::OnCommand(wParam, lParam);
}

//DEADCODE RDH 18/05/00 LRESULT RFullPanelDial::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
//DEADCODE RDH 18/05/00 {
//DEADCODE RDH 18/05/00 	// TODO: Add your specialized code here and/or call the base class
//DEADCODE RDH 18/05/00 	int rv= CDialog::DefWindowProc(message, wParam, lParam);
//DEADCODE RDH 18/05/00 	switch (message)
//DEADCODE RDH 18/05/00 	{
//DEADCODE RDH 18/05/00 	case	WM_NCCREATE:			case	WM_PARENTNOTIFY:
//DEADCODE RDH 18/05/00 	case	WM_NCCALCSIZE:			case	0x04c8:	//MCIWNDM_NOTIFY:
//DEADCODE RDH 18/05/00 	case	WM_CREATE:				case	WM_NCPAINT:
//DEADCODE RDH 18/05/00 	case	WM_SIZE:				case	WM_SETCURSOR:
//DEADCODE RDH 18/05/00 	case	WM_MOVE:				case	WM_SETFONT:
//DEADCODE RDH 18/05/00 	case	WM_INITDIALOG:			case	WM_DISPLAYCHANGE:
//DEADCODE RDH 18/05/00 	case	WM_CTLCOLORSTATIC:		case	WM_IDLEUPDATECMDUI: //363
//DEADCODE RDH 18/05/00 	case	WM_GETFONT:				//
//DEADCODE RDH 18/05/00 	case	WM_WINDOWPOSCHANGING:	//
//DEADCODE RDH 18/05/00 	case	WM_SHOWWINDOW:			//
//DEADCODE RDH 18/05/00 	case	WM_PAINT:				//
//DEADCODE RDH 18/05/00 	case	WM_WINDOWPOSCHANGED:	//
//DEADCODE RDH 18/05/00 	case	 WM_NCHITTEST:			//
//DEADCODE RDH 18/05/00 		return rv;
//DEADCODE RDH 18/05/00 	}
//DEADCODE RDH 18/05/00 	return rv;
//DEADCODE RDH 18/05/00 }

BOOL RFullPanelDial::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL RFullPanelDial::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::OnNotify(wParam, lParam, pResult);
}

BOOL RFullPanelDial::PreTranslateMessage(MSG* pMsg) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::PreTranslateMessage(pMsg);
}
