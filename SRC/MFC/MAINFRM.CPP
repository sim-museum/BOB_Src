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

/////////////////////////////////////////////////////////////////////////////
// CMainFrame
//
//	This file manages the toolbars
//

//void CMainFrame::OnBye() 

#include "stdafx.h"
#include "MIG.h"
#undef	HtmlHelp
#undef	HtmlHelpA
#undef	HtmlHelpU
#include	"htmlhelp.h"		//for HTML help


#include "MainFrm.h"
#include <afxpriv.h>
#include "migview.h"
#include "fileman.h"
#include "uiicons.h"   /* S174: IconsUI/IconDescUI for the title-band glyphs */
//TEMPCODE DAW 13/04/00 #include "smack.h"
#include "winmove.h"
#include	"stub3d.h"	//Inst3d
#include	"shapes.h"													//RJS 28Aug00
#include	"miles.h"													//RJS 28Aug00
#include	"rchatter.h"												//RJS 28Aug00
#ifdef _DEBUG
//#define new DEBUG_NEW
#ifndef	THIS_FILE_DEFINED
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif
bool	HELPWASLAUNCHEDRECENTLY=false;

extern CFont* g_AllFonts[MAXFONTS][4];
#ifndef	BOB_DEMO_VER
CRToolBar*	CMainFrame::toolbars[TB_TOTAL]={NULL};
#endif
/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, OnContextHelp)
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_CLOSE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ACTIVATE()
	ON_WM_ACTIVATEAPP()
	ON_WM_DEVMODECHANGE()
	//}}AFX_MSG_MAP
	// Global help commands
//	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
//	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
	ON_MESSAGE( WM_COMMANDHELP,OnCommandHelp)

END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

//DeadCode RJS 03May100 #if !defined (NDEBUG)
#define NOT_TOPMOST
//DeadCode RJS 03May100 #endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
#ifndef	BOB_DEMO_VER
:	m_reportbar(IntellMessages::Make),
	m_commsbar(RadioComms::Make)
#endif
{
	// TODO: add member initialization code here
	m_doIExist=0;
	m_bHideToolbars=TRUE;
/////	FILEMAN.InitFileSystem();
	havedrawn=havesafe=false;
//DeadCode AMM 7Aug00 	m_oldindebrief=(Bool)2;
}

CMainFrame::~CMainFrame()
{
	for (int i=0;i<MAXFONTS;i++)
	{
		for (int j=0;j<4;j++)
		{
			if (g_AllFonts[i][j]!=NULL)
			{
				delete g_AllFonts[i][j];
			}
		}
	}
	if (RDialog::m_pMessageBox)
		delete RDialog::m_pMessageBox;
	if (RDialog::m_pHintBox)
		delete RDialog::m_pHintBox;
	if (RDialog::m_pListBox)
		delete RDialog::m_pListBox;
///	SmackSetSystemRes(SMACKRESRESET);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

//DeadCode JIM 29Oct00 #if !defined(NOT_TOPMOST)
//DeadCode JIM 29Oct00 	cs.dwExStyle|=WS_EX_TOPMOST;
//DeadCode JIM 29Oct00 #endif

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnBye() 
{
#ifndef	BOB_DEMO_VER
	int rv = RDialog::RMessageBox(IDS_QUITGAME, IDS_AREYOUSURE,IDS_SAVE,IDS_YES,IDS_CANCEL);
	if (rv==0)
	{
		MiscToolBar().OpenSaveOnly(true);

///		else
		return;
	}
	else
	if (rv<2)
	{
		if (RFullPanelDial::incomms)
		{
 			_DPlay.ExitDirectPlay();
		}
//DEADCODE AMM 17/05/99 		m_oldindebrief=FALSE;
// About to quit campaign, so...
// ...Kill shapes, stop sounds
//DeadCode RJS 1Sep00 		SHAPESTUFF.UnLoadRequiredShapes();								//RJS 28Aug00
//DeadCode RJS 1Sep00 		_Miles.KillAll();												//RJS 28Aug00
//DeadCode RJS 1Sep00 		_Radio.Enter3D(false);											//RJS 28Aug00
//DeadCode RJS 1Sep00 
//DeadCode RJS 1Sep00 
//DeadCode RJS 1Sep00 		Todays_Packages.WipeAll();

		ExitCampaign();													//RJS 1Sep00

		RDialog::m_pView->LaunchFullPane(&RFullPanelDial::title);	
	}
#endif
}

void	CMainFrame::ExitCampaign()
{
	SHAPESTUFF.UnLoadRequiredShapes();								//RJS 28Aug00
	_Miles.KillAll();												//RJS 28Aug00
	_Radio.Enter3D(false);											//RJS 28Aug00


	Todays_Packages.WipeAll();
}

void CMainFrame::OnGoSmall() 
{
//	ShowWindow(SW_SHOWMINIMIZED);
	// show/hide toolbars instead...
	if (m_bHideToolbars)
		ShowToolbars(false);
	else
		HideToolbars();
#ifndef	BOB_DEMO_VER
	m_wndSystemBox->InDialAncestor()->ShowWindow(SW_SHOW);
#endif
}

void CMainFrame::OnGoBig() 
{
//TEMPCODE DAW 08/05/99 	if (!IsZoomed())
		ModifyStyle(WS_THICKFRAME,NULL); // Bye bye border
		HELPWASLAUNCHEDRECENTLY=false;
//DeadCode JIM 29Oct00 #if !defined(NOT_TOPMOST)
//DeadCode JIM 29Oct00 				SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
//DeadCode JIM 29Oct00 #endif
//DeadCode DAW 17Jun99 		ModifyStyleEx(0,WS_EX_TOPMOST);
//DeadCode MS 16Aug00 		MoveWindow(0,0,1050,800);
	ShowWindow(SW_SHOWMAXIMIZED);
//	m_wndSystemBox.GetDlgItem(ID_GONORMAL)->ShowWindow(SW_SHOW);
//	m_wndSystemBox.GetDlgItem(ID_GOBIG)->ShowWindow(SW_HIDE);
}

void CMainFrame::OnGoNormal() 
{
//TEMPCODE DAW 08/05/99 	if (IsZoomed())
		ModifyStyle(NULL,WS_THICKFRAME); // Come back border!
//DeadCode DAW 17Jun99 	ModifyStyleEx(WS_EX_TOPMOST,0);
	ShowWindow(SW_SHOWNORMAL);
//DeadCode JIM 29Oct00 #if !defined(NOT_TOPMOST)
//DeadCode JIM 29Oct00 				SetWindowPos(&wndBottom,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
//DeadCode JIM 29Oct00 #endif
//	m_wndSystemBox.GetDlgItem(ID_GONORMAL)->ShowWindow(SW_HIDE);
//	m_wndSystemBox.GetDlgItem(ID_GOBIG)->ShowWindow(SW_SHOW);
}

void CMainFrame::Initialise()
{
	CMIGView* view = (CMIGView*)GetActiveView();
		m_doIExist=2;	
		// Init toolbars
//DeadCode JIM 05Jul98 		m_toolbar1.Create(CMainToolbar::IDD,view);
//DeadCode JIM 05Jul98 		CRect rect;
//DeadCode JIM 05Jul98 		m_toolbar1.GetWindowRect(rect);
//DeadCode JIM 05Jul98 		m_toolbar1.Init(this,rect.Width(),rect.Width(),40,AFX_IDW_DOCKBAR_TOP,0);
//DeadCode JIM 05Jul98 		m_toolbar2.Create(CMainToolbar::IDD,view);
//DeadCode JIM 05Jul98 		m_toolbar2.Init(this,rect.Width(),rect.Width(),40,AFX_IDW_DOCKBAR_TOP,0);
//DeadCode JIM 05Jul98 		m_toolbar4.Create(CScaleBar::IDD,view);
//DeadCode JIM 05Jul98 		m_toolbar4.Init(this,200,400,40,AFX_IDW_DOCKBAR_LEFT,4);

		CRect rect;

#ifndef	BOB_DEMO_VER
		m_titlebar.Create(TitleBar::IDD,view);
		m_titlebar.GetWindowRect(rect);
		m_titlebar.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

//		m_toolbar1.Create(CMapFilters::IDD,view);
//		m_toolbar1.GetWindowRect(rect);
//		m_toolbar1.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

		m_toolbar2.Create(CMainToolbar::IDD,view);
		m_toolbar2.GetWindowRect(rect);
		m_toolbar2.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);
//DEADCODE RDH 08/11/99 		m_toolbar5.Create(CDebriefToolbar::IDD,view);
//DEADCODE RDH 08/11/99 		m_toolbar5.GetWindowRect(rect);
//DEADCODE RDH 08/11/99 		m_toolbar5.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0);
		m_reportbar.Create(m_reportbar.IDD,view);
		m_reportbar.GetWindowRect(rect);
		m_reportbar.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

		m_toolbar3.Create(CMiscToolbar::IDD,view);
		m_toolbar3.GetWindowRect(rect);
		m_toolbar3.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);


//		m_commsbar.Create(m_commsbar.IDD,view);
//		m_commsbar.GetWindowRect(rect);
//		m_commsbar.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

//		m_statusbar.Create(m_statusbar.IDD,view);   //m_commsbar
//		m_statusbar.GetWindowRect(rect);
//		m_statusbar.Init(this,rect.Width(),rect.Width(),CRToolBar::TOOBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);

		m_spacebar.Create(m_spacebar.IDD,view);
		m_spacebar.GetWindowRect(rect);
		m_spacebar.Init(this,1000,1000,CRToolBar::TOPBAR_THICKNESS,AFX_IDW_DOCKBAR_BOTTOM,0,CBRS_ALIGN_BOTTOM);
		m_toolbar4.Create(m_toolbar4.IDD,view);
		m_toolbar4.GetWindowRect(rect);
		m_toolbar4.Init(this,500,500,CRToolBar::SIDEBAR_THICKNESS,AFX_IDW_DOCKBAR_RIGHT,4,CBRS_ALIGN_TOP+CBRS_ALIGN_LEFT+CBRS_ALIGN_RIGHT+CBRS_GRIPPER);

		view->m_pScaleBar=&m_toolbar4;
	//{	TB_FILTERS,TB_MAIN,TB_MISC,TB_TITLE,TB_REPORT,TB_COMMS,TB_STATUS,TB_SCALE,TB_SPACE,TB_TOTAL};

		toolbars[TB_FILTERS]=NULL;//&m_toolbar1;
		toolbars[TB_MAIN]=&m_toolbar2;
		toolbars[TB_MISC]=&m_toolbar3;
		toolbars[TB_TITLE]=&m_titlebar;
		toolbars[TB_REPORT]=&m_reportbar;
		toolbars[TB_COMMS]=NULL;//&m_commsbar;
		toolbars[TB_STATUS]=NULL;//&m_statusbar;
		toolbars[TB_SCALE]=&m_toolbar4;
		toolbars[TB_SPACE]=&m_spacebar;


		typedef	HideEdgeControl	EDGE;
		RDialog::MakeTopDialog(CRect(0,0,CRToolBar::SIDEBAR_THICKNESS,CRToolBar::TOOBAR_THICKNESS),
			DialBox(FIL_NULL,m_wndSystemBox=new CSystemBox(this),Edges(EDGE::OCCLUDE_EXACT+EDGE::ALIGN_MIN,EDGE::ALIGN_MIN,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE,EDGE::ALIGN_MAX + EDGE::OCCLUDE_INSIDE)));
		PositionSysBox();

#endif// #ifndef	BOB_DEMO_VER

//		m_wndSystemBox-.Init(CSystemBox::IDD,this);
//		m_wndSystemBox.ShowWindow(SW_SHOW);
		HideToolbars();
#if BOB_LINUX
		// Workstream A: skip the intro Smacker (stubbed, never advances) and go straight to
		// the title menu so the front-end renders. Drop the #if once Smacker/transitions work.
		if (getenv("BOB_FRONTEND"))
			view->LaunchFullPane(&RFullPanelDial::title,SaveData::UIR_FIRST);
		else
#endif
		view->LaunchFullPane(&RFullPanelDial::introsmack,SaveData::UIR_FIRST);			  //JIM 18/01/99
		m_doIExist=1;
}

#if BOB_LINUX
extern "C" void bob_gdi_ensure_window(void);
#endif
void	CMainFrame::InitialiseSafe()
{
	havesafe=true;
#if BOB_LINUX
	// On Windows the front-end bootstraps from the first WM_PAINT (OnPaint sets havedrawn,
	// then calls Initialise). Our compat dispatches no WM_PAINT, so under BOB_FRONTEND we
	// trigger the front-end bring-up here directly. (Opt-in while the RDialog/toolbar
	// window subsystem is brought up; default BOB_RUN_INIT stays at the idle loop.)
	if (getenv("BOB_FRONTEND")) {
		bob_gdi_ensure_window();   // open the SDL window so the front-end has a render target
		havedrawn = true;
	}
#endif
	if (havedrawn)
		Initialise();
}
void CMainFrame::OnPaint() 
{
//	CDC* pDC=GetWindowDC();
//	CRect windrect;
//	GetWindowRect(windrect);
//	pDC->FillSolidRect(0,0,windrect.Width(),windrect.Height(),RGB(0,0,0));
//DeadCode JIM 20Oct00 	CMIGView* view = (CMIGView*)GetActiveView();
	havedrawn=true;
	if (m_doIExist==0)
	{
		if (havesafe)
			Initialise();

	}
	if (m_doIExist==1)
	{
//DEADCODE JIM 14/06/00 		PositionSysBox();
	}
	PAINTSTRUCT ps;
	::BeginPaint(m_hWnd, &ps);
//	pDC->FillSolidRect(0,0,windrect.Width(),windrect.Height(),RGB(0,0,0));
}

void	CMainFrame::PositionSysBox() 
{
#ifndef	BOB_DEMO_VER
	if (!m_doIExist)
		return;
	if (!m_wndSystemBox)
		return;
	CRect rect;
	GetWindowRect(rect);
	CRect systemrect;

	m_wndSystemBox->InDialAncestor()->GetClientRect(systemrect);
	CRect viewrect;
	CMIGView* view = (CMIGView*)GetActiveView();
	view->GetClientRect(viewrect);
	view->ClientToScreen(viewrect);
	int	bs=(viewrect.top-rect.top)%CRToolBar::BARTHICK_MODULO;
	if (viewrect.top-rect.top+10>systemrect.bottom || view->m_currentpage!=0) // check if there is room for the system box 
		m_wndSystemBox->InDialAncestor()->MoveWindow(CRect(rect.right-systemrect.right-bs,rect.top+bs,rect.right-bs,rect.top+systemrect.bottom+bs));
	else 
	{
		CRect	insidenoscroll	(	viewrect.right-systemrect.right,	rect.top+bs,
									viewrect.right,						rect.top+systemrect.bottom+bs),

				insidescroll	(	viewrect.right-systemrect.right-CRToolBar::SCROLLBAR_THICKNESS,	rect.top+bs,
									viewrect.right-CRToolBar::SCROLLBAR_THICKNESS,					rect.top+systemrect.bottom+bs),

				outsidescroll	(	rect.right-bs-systemrect.right,		rect.top+bs,
									rect.right-bs,						rect.top+systemrect.bottom+bs);
		{


		}
		if (m_top.IsEmpty() && m_right.IsEmpty())
			if (view->m_rVertScrollBar.IsWindowVisible())
				m_wndSystemBox->InDialAncestor()->MoveWindow( insidescroll);
			else
				m_wndSystemBox->InDialAncestor()->MoveWindow( insidenoscroll);
		else
			if (!m_right.IsEmpty() || viewrect.top-rect.top>=systemrect.bottom)
				m_wndSystemBox->InDialAncestor()->MoveWindow(outsidescroll);
			else
				m_wndSystemBox->InDialAncestor()->MoveWindow( insidescroll);

	}
#endif
}

void CMainFrame::HideToolbars()
{
	m_bHideToolbars=TRUE;
#ifndef	BOB_DEMO_VER
//	m_titlebar.ShowWindow(SW_HIDE);
//	m_toolbar1.ShowWindow(SW_HIDE);
//	m_toolbar2.ShowWindow(SW_HIDE);
//	m_toolbar3.ShowWindow(SW_HIDE);
//	m_toolbar4.ShowWindow(SW_HIDE);
//DEADCODE RDH 08/11/99 	m_toolbar5.ShowWindow(SW_HIDE);
	m_spacebar.ShowWindow(SW_HIDE);
	m_wndSystemBox->InDialAncestor()->ShowWindow(SW_HIDE);
	UpdateToolbars(true);
#endif #ifndef	BOB_DEMO_VER
//	RecalcLayout();
}

void CMainFrame::ShowToolbars(bool firsttime)
{
#ifndef	BOB_DEMO_VER
	m_bHideToolbars=FALSE;
//DEADCODE RDH 09/11/99 	m_titlebar.ShowWindow(SW_SHOW);
//DEADCODE RDH 09/11/99 	m_toolbar1.ShowWindow(SW_SHOW);
//DEADCODE RDH 09/11/99 
//DEADCODE RDH 09/11/99 	m_toolbar2.ShowWindow(SW_SHOW);
//DEADCODE RDH 09/11/99 	m_toolbar3.ShowWindow(SW_SHOW);
//DEADCODE RDH 09/11/99 	m_toolbar4.ShowWindow(SW_SHOW);
//DEADCODE RDH 08/11/99 	m_toolbar5.ShowWindow(SW_SHOW);
//DEADCODE RDH 09/11/99 	m_spacebar.ShowWindow(SW_SHOW);
	m_wndSystemBox->InDialAncestor()->ShowWindow(SW_SHOW);
	UpdateToolbars(false);
//DEADCODE RDH 09/11/99 	RecalcLayout();
#endif #ifndef	BOB_DEMO_VER
//	if  (firsttime && (RFullPanelDial::incomms && _DPlay.UIPlayerType==PLAYER_GUEST))
//		m_oldindebrief=TRUE;
//DEADCODE JIM 14/05/99  	if (firsttime)
//DEADCODE JIM 14/05/99  	{
//DEADCODE  14/05/99  		//first time in set this flag to opposite of what we want to ensure we
//DEADCODE  14/05/99  		//get a refresh. 
//DEADCODE  14/05/99  		if  (		(RFullPanelDial::incomms)
//DEADCODE  14/05/99  				&&	(_DPlay.UIPlayerType==PLAYER_GUEST)
//DEADCODE  14/05/99  			)
//DEADCODE  14/05/99  			m_oldindebrief=FALSE;
//DEADCODE  14/05/99  		else
//DEADCODE  14/05/99  			if 	(MMC.indebrief)
//DEADCODE  14/05/99  				m_oldindebrief=FALSE;
//DEADCODE  14/05/99  			else
//DEADCODE  14/05/99  				m_oldindebrief=TRUE;
//DEADCODE  14/05/99 	}
//DEADCODE  14/05/99 */
//DEADCODE JIM 30/03/00 	UpdateToolbars(false);
}

void CMainFrame::RePositionLayout(int dx,int dy) 
{
	RePositionLayout(dx,dy,&m_left);
	RePositionLayout(dx,dy,&m_top);
	RePositionLayout(dx,dy,&m_right);
	RePositionLayout(dx,dy,&m_bottom);
	PositionSysBox();

}
void CMainFrame::RePositionLayout(int dx,int dy,CMainFrame::ToolBarList* toollist)
{
	POSITION position;
	CRToolBar* pToolbar;
	position=toollist->GetHeadPosition();
	int	count=toollist->GetCount();
	for (int y=0;y<count;y++)
	{
		pToolbar=toollist->GetNext(position);
		CRect rect;
 		pToolbar->GetWindowRect(rect);
		pToolbar->MoveWindow(rect+CPoint(dx,dy));
	}
}
void CMainFrame::RecalcLayout(BOOL bNotify) 
{

	// TODO: Add your specialized code here and/or call the base class
	CFrameWnd::RecalcLayout(bNotify); // if this isnt called the frame window doesnt appear at all
#ifndef	BOB_DEMO_VER
	if (!m_hWnd)
		return;
	if (m_doIExist==0) return;
	CMIGView* view = (CMIGView*)GetActiveView();
	if (!view) return; // If the view doesnt exist yet return
	CRect rect2;
	GetClientRect(rect2);
	if (m_bHideToolbars) 
	{
		view->MoveWindow(rect2);
		return;
	}
	// draw the toolbars
	m_borderRect=CRect(0,0,0,0); // keeps track of the width of each edge
	int x;
	int y;
	int count;
	int start;
	int row;
	int total;
	int priority;
	CRToolBar* pToolbar;
	CRToolBar* pPriorityToolbar;
	BOOL foundend;
	POSITION position;
	CRect rect;
	view->GetWindowRect(rect);
	CRect systemrect;

	m_wndSystemBox->GetWindowRect(systemrect);

	// first row on top row is a bit shorter because of system box
	int totalwidth=rect2.Width()-systemrect.Width()+1;
	int totalheight=rect2.Height();
	ClientToScreen(rect2);
	// Start with the top

	position=m_top.GetHeadPosition();
	count=m_top.GetCount();
	start=0;
	row=0;
	total=0;
	priority=0;
	foundend=FALSE;
	for (y=0;y<count;y++)
	{
		pToolbar=m_top.GetNext(position);
		pToolbar->toprow=&CRToolBar::toptoprow;
		pToolbar->m_currentlength=pToolbar->m_minlength;
		total+=pToolbar->m_minlength;
		if (total>totalwidth && start!=y)
		{
			y--;
			foundend=TRUE;
			total-=pToolbar->m_minlength;
		}
		else if (pToolbar->m_priority >= priority)
		{
			priority=pToolbar->m_priority;
			pPriorityToolbar=pToolbar;
		}
		// found a row now draw it
		if (foundend || y==count-1)
		{
			pPriorityToolbar->m_currentlength+=totalwidth-total;
			position=m_top.FindIndex(start);
			total=0;
			for (x=start;x<=y;x++)
			{
				pToolbar=m_top.GetNext(position);
				pToolbar->m_index=x+1;
//				pToolbar->MoveWindow(rect2.left+total,rect2.top+(row*pToolbar->m_width),pToolbar->m_currentlength,pToolbar->m_width);
				pToolbar->SetWindowPos(this,rect2.left+total,rect2.top+(row*pToolbar->m_width),pToolbar->m_currentlength,pToolbar->m_width,SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
				pToolbar->m_row=row;
				total+=pToolbar->m_currentlength;
			}
			// initialise the next row if its there
			m_borderRect.top+=pToolbar->m_width; // assumed all of them are the same width for now...
			if (foundend)
			{
				start=y+1;
				row++;
				totalwidth=rect2.Width();
				total=0;
				foundend=FALSE;
				priority=0;
			}
		}
	}
	totalwidth=rect2.Width();
	// repeat for the other edges
	// probably look nicer doing these 4 things as a very complicated
	// loop but whats the point?  its faster and simpler this way
	// plus its easy to have special conditions for a particular edge

	// next the bottom
	//rewritten this to fix 2 bugs. NOTE: These bugs are still in the other versions!!
	//Bug1) Not handling different height tools.
	//			Still a problem here if diff height tools are on same row.
	//Bug2) Not returning useful row number for artwork positioning.
	position=m_bottom.GetHeadPosition();
	count=m_bottom.GetCount();
	start=0;
	row=count-1;
	total=0;
	priority=0;
	foundend=FALSE;
	int	currbottom=0,worstheight=0;

	struct	RowDesc
	{	int	left,right,priority,height,width;	}
	rowdesc[8];
	
	int	rowindex=0;
	rowdesc[rowindex].left=0;
	rowdesc[rowindex].priority=0;
	rowdesc[rowindex].height=0;
	
	for (y=0;y<=count;y++)
	{
		rowdesc[rowindex].width=total;
		if (y<count)
		{
			pToolbar=m_bottom.GetNext(position);
			pToolbar->m_currentlength=pToolbar->m_minlength;
			total+=pToolbar->m_minlength;
		}
		else
		{
			total=totalwidth+1;
		}
		if ((total>totalwidth && y!=rowdesc[rowindex].left))
		{
			rowdesc[rowindex].right=y-1;
			rowdesc[rowindex].height=worstheight;

			//put this row on next line
			rowindex++;
			if (y<count)
			{
				total=pToolbar->m_minlength;
				priority=pToolbar->m_priority;
				rowdesc[rowindex].priority=y;
				rowdesc[rowindex].left=y;
				worstheight=pToolbar->m_width;
				rowdesc[rowindex].width=total;
			}
		}
		else
			if (y<count)
			{
				if (pToolbar->m_priority >= priority)
				{
					priority=pToolbar->m_priority;
					rowdesc[rowindex].priority=y;
				}
				if (pToolbar->m_width>worstheight)
					worstheight=pToolbar->m_width;
			}
	}

	currbottom=0;
	for (row=0,count=rowindex-1;row<rowindex;row++,count--)
	{	// found a row now draw it
		total=0;
		currbottom+=rowdesc[row].height;
		position=m_bottom.FindIndex(rowdesc[row].priority);
		pToolbar=m_bottom.GetNext(position);
		pToolbar->m_currentlength+=totalwidth-rowdesc[row].width;

		for (y=rowdesc[row].left;y<=rowdesc[row].right;y++)
		{
			position=m_bottom.FindIndex(y);
			pToolbar=m_bottom.GetNext(position);
			pToolbar->m_index=y+1;
			pToolbar->SetWindowPos(this,rect2.left+total,rect2.top+(totalheight)-currbottom,pToolbar->m_currentlength,pToolbar->m_width,SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
			pToolbar->m_row=count;
			total+=pToolbar->m_currentlength;
		}
	}
	m_borderRect.bottom+=currbottom;
	totalheight-=(m_borderRect.bottom+m_borderRect.top);
	// Next the left

	position=m_left.GetHeadPosition();
	count=m_left.GetCount();
	start=0;
	row=0;
	total=0;
	priority=0;
	foundend=FALSE;
	for (y=0;y<count;y++)
	{
		pToolbar=m_left.GetNext(position);
		pToolbar->m_currentlength=pToolbar->m_minlength;
		total+=pToolbar->m_minlength;
		if (total>totalheight && start!=y)
		{
			y--;
			foundend=TRUE;
			total-=pToolbar->m_minlength;
		}
		else if (pToolbar->m_priority >= priority)
		{
			priority=pToolbar->m_priority;
			pPriorityToolbar=pToolbar;
		}
		// found a row now draw it
		if (foundend || y==count-1)
		{
			pPriorityToolbar->m_currentlength+=totalheight-total;
			position=m_left.FindIndex(start);
			total=0;
			for (x=start;x<=y;x++)
			{
				pToolbar=m_left.GetNext(position);
				pToolbar->m_index=x+1;
				pToolbar->MoveWindow(rect2.left+(row*pToolbar->m_width),rect2.top+(m_borderRect.top+total),pToolbar->m_width,pToolbar->m_currentlength);
				pToolbar->SetWindowPos(this,rect2.left+(row*pToolbar->m_width),rect2.top+(m_borderRect.top+total),pToolbar->m_width,pToolbar->m_currentlength,SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
				pToolbar->m_row=row;
				total+=pToolbar->m_currentlength;
			}
			// initialise the next row if its there
			m_borderRect.left+=pToolbar->m_width; // assumed all of them are the same width for now...
			if (foundend)
			{
				start=y+1;
				row++;
				total=0;
				foundend=FALSE;
				priority=0;
			}
		}
	}
	// Next the right

	position=m_right.GetHeadPosition();
	count=m_right.GetCount();
	start=0;
	row=0;
	priority=0;
	foundend=FALSE;
	
	int	yfix=0;
	if (m_top.IsEmpty())
		yfix=systemrect.Height();
	total=yfix;
	for (y=0;y<count;y++)
	{
		pToolbar=m_right.GetNext(position);
		pToolbar->m_currentlength=pToolbar->m_minlength;
		total+=pToolbar->m_minlength;
		if (total>totalheight && start!=y)
		{
			y--;
			foundend=TRUE;
			total-=pToolbar->m_minlength;
		}
		else if (pToolbar->m_priority >= priority)
		{
			priority=pToolbar->m_priority;
			pPriorityToolbar=pToolbar;
		}
		// found a row now draw it
		if (foundend || y==count-1)
		{
			pPriorityToolbar->m_currentlength+=totalheight-total;
			position=m_right.FindIndex(start);
			total=yfix;
			yfix=0;
			for (x=start;x<=y;x++)
			{
				pToolbar=m_right.GetNext(position);
				pToolbar->m_index=x+1;
//				pToolbar->MoveWindow(rect2.left+totalwidth-pToolbar->m_width-(row*pToolbar->m_width),rect2.top+m_borderRect.top+total,pToolbar->m_width,pToolbar->m_currentlength);
				pToolbar->SetWindowPos(this,rect2.left+totalwidth-pToolbar->m_width-(row*pToolbar->m_width),rect2.top+m_borderRect.top+total,pToolbar->m_width,pToolbar->m_currentlength,SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
				pToolbar->m_row=row;
				total+=pToolbar->m_currentlength;
			}
			// initialise the next row if its there
			m_borderRect.right+=pToolbar->m_width; // assumed all of them are the same width for now...
			if (foundend)
			{
				start=y+1;
				row++;
				total=0;
				foundend=FALSE;
				priority=0;
			}
		}
	}
//DeadCode MS 21Nov00 	__asm	{	int 3
//DeadCode MS 21Nov00 	};
	// create the border depending on borderrect
	rect-=rect2.TopLeft();
	view->MoveWindow(CRect(rect.left+m_borderRect.left,rect.top+m_borderRect.top,rect.right-m_borderRect.right,rect.bottom-m_borderRect.bottom));
	// move system box to top

	m_wndSystemBox->InDialAncestor()->SetWindowPos(&wndTop,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	PositionSysBox();
#endif
//	Invalidate();
}

//DEADCODE DAW 07/05/99 void CMainFrame::OnNcPaint() 
//DEADCODE DAW 07/05/99 {
//DEADCODE DAW 07/05/99 //	CDC* pDC=GetWindowDC();
//DEADCODE DAW 07/05/99 //	CRect windrect;
//DEADCODE DAW 07/05/99 //	GetWindowRect(windrect);
//DEADCODE DAW 07/05/99 //	pDC->FillSolidRect(0,0,windrect.Width(),windrect.Height(),RGB(0,0,0));
//DEADCODE DAW 07/05/99 	// Do not call CFrameWnd::OnNcPaint() for painting messages
//DEADCODE DAW 07/05/99 }
void	Campaign::GlobalRefreshData()
{
#ifndef	BOB_DEMO_VER

	MainToolBar().GlobalRefreshData();
	MiscToolBar().GlobalRefreshData();
	TitleBarPtr().GlobalRefreshData();
	((CMainFrame*)AfxGetMainWnd())->m_reportbar.GlobalRefreshData();
#endif
}

bool	Campaign::AnyDialogsOpen()
{
#ifndef	BOB_DEMO_VER

	if (MainToolBar().LoggedChild()!=-1) return true;
	if (MiscToolBar().LoggedChild()>MiscToolBar().ZOOMLEVEL) return true;
//DEADCODE RDH 14/04/00 	if (TitleBarPtr().LoggedChild()!=-1) return true;
#endif
	return false;
}
LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
#ifndef NDEBUG
	// TODO: Add your specialized code here and/or call the base class
//DEADCODE DAW 07/06/99 	CRect rect;														  //DAW 07/06/99
//DEADCODE DAW 07/06/99 	AfxGetMainWnd()->GetWindowRect(rect);							  //DAW 07/06/99
//TempCode JIM 11Oct00 static int r=0;
//TempCode JIM 11Oct00 	if ( message!=0x113)	//message!=0x363 &&
//TempCode JIM 11Oct00 	{
//TempCode JIM 11Oct00 		Mono_Text.PrintAt(70,r,(UByte*)"   ");
//TempCode JIM 11Oct00 		r++;
//TempCode JIM 11Oct00 		if (r==25) r=0;
//TempCode JIM 11Oct00 		Mono_Text.PrintAt(70,r,(UByte*)"<--");
//TempCode JIM 11Oct00 		Mono_Text.PrintHexAt(40,r,message,8);
//TempCode JIM 11Oct00 		Mono_Text.PrintHexAt(50,r,wParam,8);
//TempCode JIM 11Oct00 		Mono_Text.PrintHexAt(60,r,lParam,8);
//TempCode JIM 11Oct00 	}
	return CFrameWnd::WindowProc(message, wParam, lParam);
#else
	return 0;
#endif

}

#ifndef	BOB_DEMO_VER

void CMainFrame::UpdateToolbars(bool	hideall)
{
	for (int t=0,i=0;t<TB_TOTAL;t++)
	{
		if (toolbars[t])
			if (hideall)
				toolbars[t]->ReAttach(false,false);
			else
				if (Save_Data.toolbars[ToolBars(t)])
					toolbars[t]->ReAttach(true,false,i++);
				else
					toolbars[t]->ReAttach(false,false);
	}
//DeadCode MS 21Nov00 	__asm	{int 3
//DeadCode MS 21Nov00 	};
	RecalcLayout();
//DeadCode MS 21Nov00 	__asm	{int 3
//DeadCode MS 21Nov00 	};
	MainToolBar().TailorHints();
//DeadCode MS 21Nov00 	__asm	{int 3
//DeadCode MS 21Nov00 	};
	m_toolbar3.m_dialids[m_toolbar3.DIRECTIVESTOGGLE].SetPressed(!MMC.directivespopup);
	m_toolbar3.m_dialids[m_toolbar3.DIRECTIVESTOGGLE].SetHintString(RESSTRING(DIRECTIVES)+": "+RESLIST(OFF,!MMC.directivespopup));

//DEADCODE RDH 08/11/99 #ifndef	BOB_DEMO_VER
//DEADCODE RDH 08/11/99 	if (m_oldindebrief==MMC.indebrief) return;
//DEADCODE RDH 08/11/99 	m_oldindebrief=MMC.indebrief;
//DEADCODE RDH 08/11/99 	if (MMC.indebrief)// && m_toolbar2.IsWindowVisible())
//DEADCODE RDH 08/11/99 	{
//DEADCODE RDH 08/11/99 		m_toolbar5.ShowWindow(SW_SHOW);
//DEADCODE RDH 08/11/99 		m_toolbar2.Replace(&m_toolbar5);
//DEADCODE RDH 08/11/99 	}
//DEADCODE RDH 08/11/99 	else// if (!MMC.indebrief && m_toolbar5.IsWindowVisible())
//DEADCODE RDH 08/11/99 	{
//DEADCODE RDH 08/11/99 		m_toolbar2.ShowWindow(SW_SHOW);
//DEADCODE RDH 08/11/99 		m_toolbar5.Replace(&m_toolbar2);
//DEADCODE RDH 08/11/99 	}
//DEADCODE RDH 08/11/99 #endif
}

#endif

#ifndef	BOB_DEMO_VER

void CMainFrame::CloseToolChildren(RDialog* skipthis)
{
	for (int t=0;t<TB_TOTAL;t++)
	{
		if (toolbars[t])
			toolbars[t]->CloseLoggedChildren(skipthis);
	}
	m_wndSystemBox->CloseLoggedChildren(skipthis);
}

#endif

void CMainFrame::OnHelp() 
{
	// TODO: Add your command handler code here
	if (!Inst3d::InThe3D())
	{
		HELPWASLAUNCHEDRECENTLY=true;
//DEADCODE JIM 21/06/00 		HWND upperwnd=HWND_BOTTOM;  
//DEADCODE JIM 21/06/00 		::SetWindowPos(m_hWnd,upperwnd,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
		CFrameWnd::OnHelp() ;
	}
}

void CMainFrame::OnHelpFinder() 
{
	// TODO: Add your command handler code here
	if (!Inst3d::InThe3D())
		CFrameWnd::OnHelpFinder() ;
	
}

void CMainFrame::OnContextHelp() 
{
	// TODO: Add your command handler code here
	if (!Inst3d::InThe3D())
		CFrameWnd::OnContextHelp() ;
	
}
LRESULT CMainFrame::OnCommandHelp(WPARAM w, LPARAM lParam)
{
	if (!Inst3d::InThe3D())
	{
		CMIGView* view = (CMIGView*)GetActiveView();
		if ((disablehelp&1)==0 || (view && view->m_currentpage==0))
		{
			if (Todays_Packages.localplayer == NAT_RAF)
				AfxGetApp()->WinHelp(HID_BASE_RESOURCE+IDD_RAFCOMMANDBRIEFING);
			else
				AfxGetApp()->WinHelp(HID_BASE_RESOURCE+IDD_RAFCOMMANDBRIEFING);
		}
	}
	return 1;//CFrameWnd::OnCommandHelp(w,lParam);
}

void CMainFrame::WinHelp(DWORD dwData, UINT nCmd)
{
	if (RDialog::m_pHintBox)
		RDialog::m_pHintBox->ShowWindow(false);
	CWinApp* pApp = AfxGetApp();
	ASSERT_VALID(pApp);
	ASSERT(pApp->m_pszHelpFilePath != NULL);

	CWaitCursor wait;
	if (IsFrameWnd())
	{
		// CFrameWnd windows should be allowed to exit help mode first
		CFrameWnd* pFrameWnd = (CFrameWnd*)this;
		pFrameWnd->ExitHelpMode();
	}

	// cancel any tracking modes
	SendMessage(WM_CANCELMODE);
	SendMessageToDescendants(WM_CANCELMODE, 0, 0, TRUE, TRUE);

	// need to use top level parent (for the case where m_hWnd is in DLL)
	CWnd* pWnd = GetTopLevelParent();
	pWnd->SendMessage(WM_CANCELMODE);
	pWnd->SendMessageToDescendants(WM_CANCELMODE, 0, 0, TRUE, TRUE);

	// attempt to cancel capture
	HWND hWndCapture = ::GetCapture();
	if (hWndCapture != NULL)
		::SendMessage(hWndCapture, WM_CANCELMODE, 0, 0);

	TRACE3("WinHelp: pszHelpFile = '%s', dwData: $%lx, fuCommand: %d.\n",
		pApp->m_pszHelpFilePath, dwData, nCmd);

//Command equates are different between WinHelp and HTMLHelp. Why?
	switch (nCmd)
	{
	case	HELP_CONTEXT:
 //DeadCode RJS 03May100			nCmd=HH_DISPLAY_TOPIC;
			nCmd=HH_HELP_CONTEXT;
			break;
	}


	// finally, run the Windows Help engine
//DeadCode RJS 03May100 	if (!HtmlHelp(pWnd->m_hWnd, CString(pApp->m_pszHelpFilePath)+"::/asset.htm", nCmd, 0))
//DEADCODE DAW 15/05/00 	if (!HtmlHelp(NULL, pApp->m_pszHelpFilePath, nCmd, dwData&0xffff))
	if (!HtmlHelp(pWnd->m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData&0xffff))
	{
		dwData=IDD_RAFCOMMANDBRIEFING;
		if (!HtmlHelp(pWnd->m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData&0xffff))
			AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
	}
}


//������������������������������������������������������������������������������
//Procedure		OnSize
//Author		Paul.   
//Date			Mon 25 Jan 1999
//------------------------------------------------------------------------------
void CMainFrame::OnMove(int x, int y) 
{
//DEADCODE DAW 07/05/99 	CRect oldpos;
//DEADCODE DAW 07/05/99 	GetWindowRect(oldpos);
//DEADCODE DAW 07/05/99 	CFrameWnd::OnMove(x, y);
//DEADCODE DAW 07/05/99 	x-=oldpos.left;
//DEADCODE DAW 07/05/99 	y-=oldpos.top;
//DEADCODE DAW 07/05/99 	RePositionLayout(x,y);
//	RecalcLayout();
	// TODO: Add your message handler code here
	
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	if (m_doIExist==1)
		PositionSysBox();
	if (nType==SIZE_MINIMIZED)
	{
		//window has been minimized
		bAppMinimized=true;
	}
	else if (bAppMinimized)
	{
		//window has been restored
		bAppMinimized=false;
	}
	HELPWASLAUNCHEDRECENTLY=false;
	CFrameWnd::OnSize(nType,cx,cy);
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
	CFrameWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainFrame::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
	CFrameWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	return true;//CFrameWnd::OnEraseBkgnd(pDC);
}

void CMainFrame::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	m_doIExist=false;
	Save_Data.ChangeMode(SaveData::UIR_FIRST);

	CFrameWnd::OnClose();
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	// TODO: Add your message handler code here and/or call default
	
	CFrameWnd::OnGetMinMaxInfo(lpMMI);
	lpMMI->ptMinTrackSize=CPoint(300,400);
}

void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
//DeadCode DAW 22Jun99 		TRACE2("Activate %i ==> %08x\n",nState,::GetWindowThreadProcessId(pWndOther->m_hWnd, NULL) );
//DeadCode DAW 22Jun99 	else
//DeadCode DAW 22Jun99 	{
//DeadCode DAW 22Jun99 		TRACE1("Activate %i xxx\n",nState);
		//If in the 3D && state==0 then discard DD here
#ifdef NDEBUG
 	if (!pWndOther)
		if (Inst3d::InThe3D())
		{
			if (!nState)	Inst3d::ReleaseDirectX();
			else
			{
				Inst3d::RestoreDirectX();
				CMIGView* view=(CMIGView*)GetActiveView();
				RFullPanelDial*	dial=GetFullPanel(view);
				dial->LaunchScreen(&RFullPanelDial::quickmissionflight);
			}
		}
#endif
//DeadCode DAW 22Jun99 	}

	CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
	
	// TODO: Add your message handler code here
	
}
static BOOL CALLBACK EnumThreadWndProc(  HWND hwnd,  LPARAM lParam)
{
	*(int*)lParam=(int)hwnd;
	return	FALSE;
}
   

void CMainFrame::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CFrameWnd::OnActivateApp(bActive, hTask);
//DeadCode DAW 22Jun99 	TRACE2("Activate Ap %i ==>%08x\n",bActive,hTask);
	
	// TODO: Add your message handler code here
	//BOOL SetWindowPos( const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags );
	//const CWnd* pWndInsertAfter= wndBottom/wndTop/ wndTopMost/wndNoTopMost
//BOOL SetWindowPos(
//  HWND hWnd,             // handle to window
//  HWND hWndInsertAfter,  // placement-order handle
//  int X,                 // horizontal position
//  int Y,                 // vertical position
//  int cx,                // width
//  int cy,                // height
//  UINT uFlags            // window-positioning flags
//);
		if (!bActive)
		{  
//TEMPCODE JIM 05/04/00 			HWND upperwnd=HWND_BOTTOM;  
//TEMPCODE JIM 05/04/00 			::SetWindowPos(m_hWnd,upperwnd,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
//DEADCODE DAW 16/06/99 			if (hTask)
//DEADCODE DAW 16/06/99 				EnumThreadWindows((int)hTask,EnumThreadWndProc,(int)&upperwnd);
//DEADCODE DAW 16/06/99  			if (upperwnd!=HWND_BOTTOM)
//DEADCODE DAW 16/06/99 			{
//DEADCODE DAW 16/06/99 				::SetWindowPos(m_hWnd,HWND_TOP,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
//DEADCODE DAW 16/06/99 				::SetWindowPos(upperwnd,HWND_TOP,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
//DEADCODE DAW 16/06/99 			}
		}
		else
		{
//TEMPCODE JIM 05/04/00 #if !defined(NOT_TOPMOST)
//TEMPCODE JIM 05/04/00 			if (IsZoomed() && HELPWASLAUNCHEDRECENTLY==false)
//TEMPCODE JIM 05/04/00 				SetWindowPos(&wndTopMost,0,0,0,0,SWP_NOACTIVATE+SWP_NOMOVE+SWP_NOSIZE);
//TEMPCODE JIM 05/04/00 #endif
		}
	
}
/* 
pWndInsertAfter���Identifies the CWnd object that will precede this CWnd object in the Z-order. 
This parameter can be a pointer to a CWnd or a Pointer to one of the following values:

wndBottom���Places the window at the bottom of the Z-order. 
If this CWnd is a topmost window, the window loses its topmost status; 
the system places the window at the bottom of all other windows.

wndTop���Places the window at the top of the Z-order.

wndTopMost���Places the window above all nontopmost windows. 
The window maintains its topmost position even when it is deactivated.

wndNoTopMost���Repositions the window to the top of all nontopmost windows (that is, behind all topmost windows). 
This flag has no effect if the window is already a nontopmost window.


nFlags���Specifies sizing and positioning options. This parameter can be a combination of the following:

SWP_DRAWFRAME���Draws a frame (defined when the window was created) around the window. 

SWP_FRAMECHANGED���Sends a WM_NCCALCSIZE message to the window, even if the window's size is not being changed. If this flag is not specified, WM_NCCALCSIZE is sent only when the window's size is being changed.

SWP_HIDEWINDOW���Hides the window.

SWP_NOACTIVATE���Does not activate the window. If this flag is not set, the window is activated and moved to the top of either the topmost or the nontopmost group (depending on the setting of the pWndInsertAfter parameter).

SWP_NOCOPYBITS���Discards the entire contents of the client area. If this flag is not specified, the valid contents of the client area are saved and copied back into the client area after the window is sized or repositioned.

SWP_NOMOVE���Retains current position (ignores the x and y parameters).

SWP_NOOWNERZORDER���Does not change the owner window's position in the Z-order.

SWP_NOREDRAW���Does not redraw changes. If this flag is set, no repainting of any kind occurs. This applies to the client area, the nonclient area (including the title and scroll bars), and any part of the parent window uncovered as a result of the moved window. When this flag is set, the application must explicitly invalidate or redraw any parts of the window and parent window that must be redrawn.

SWP_NOREPOSITION���Same as SWP_NOOWNERZORDER. 

SWP_NOSENDCHANGING���Prevents the window from receiving the WM_WINDOWPOSCHANGING message.

SWP_NOSIZE���Retains current size (ignores the cx and cy parameters).

SWP_NOZORDER���Retains current ordering (ignores pWndInsertAfter).

SWP_SHOWWINDOW���Displays the window.
*/

void CMainFrame::OnDevModeChange(LPTSTR lpDeviceName) 
{
	CFrameWnd::OnDevModeChange(lpDeviceName);
	
	// TODO: Add your message handler code here
//DEADCODE JIM 04/04/00 	ShowWindow(SW_SHOWMAXIMIZED);

	
}

LRESULT CMainFrame::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	int rv= CFrameWnd::DefWindowProc(message, wParam, lParam);
//DeadCode JIM 11Oct00 	switch (message)
//DeadCode JIM 11Oct00 	{
//DeadCode JIM 11Oct00 	case	WM_NCCREATE:			case	0x036d:	//WM_FLOATSTATUS
//DeadCode JIM 11Oct00 	case	WM_NCCALCSIZE:			case	0x04c8:	//MCIWNDM_NOTIFY:
//DeadCode JIM 11Oct00 	case	WM_CREATE:				case	WM_NCPAINT:
//DeadCode JIM 11Oct00 	case	WM_SIZE:				case	WM_SETCURSOR:
//DeadCode JIM 11Oct00 	case	WM_MOVE:				case	WM_SETFONT:
//DeadCode JIM 11Oct00 	case	WM_INITDIALOG:			case	WM_DISPLAYCHANGE:
//DeadCode JIM 11Oct00 	case	WM_CTLCOLORSTATIC:		case	WM_IDLEUPDATECMDUI:
//DeadCode JIM 11Oct00 	case	WM_GETFONT:				case	WM_ACTIVATE:
//DeadCode JIM 11Oct00 	case	WM_WINDOWPOSCHANGING:	case	WM_NCACTIVATE:
//DeadCode JIM 11Oct00 	case	WM_SHOWWINDOW:			case	WM_PARENTNOTIFY:	case	WM_SYNCPAINT:		
//DeadCode JIM 11Oct00 	case	WM_PAINT:				case	WM_GETTEXT:			case	WM_GETMINMAXINFO:
//DeadCode JIM 11Oct00 	case	WM_WINDOWPOSCHANGED:	case	WM_SETTEXT:			case	WM_KILLFOCUS:
//DeadCode JIM 11Oct00 	case	 WM_NCHITTEST:			case	WM_STYLECHANGING:	case	WM_STYLECHANGED:
//DeadCode JIM 11Oct00 
//DeadCode JIM 11Oct00 
//DeadCode JIM 11Oct00 
//DeadCode JIM 11Oct00 		return rv;
//DeadCode JIM 11Oct00 	}
	return rv;
}

#if BOB_LINUX
/* S89 (campaign Phase 2): paint the strategic-map footer toolbars' hosted CRButton faces onto
   the framebuffer. The MFC docking frame doesn't run WM_PAINT headlessly, so drive each toolbar's
   buttons via bob_ole_draw_toolbar (their OnDraw blits NormalFileNum art). Positions are laid out
   on the footer band; env-gated (BOB_MAP_BUTTONS) while the layout is tuned. */
extern "C" int bob_ole_draw_toolbar(CWnd*, int, int, int);
extern "C" int bob_ole_draw_toolbar_ids(CWnd*, int, int, int, const int*, int);
extern "C" int bob_ole_draw_panel(CWnd*, int, int);   /* S173: the clock panel, drawn whole */
extern "C" void bob_gdi_setdibits_origin(int, int);
/* S183: last-drawn system-box rect, so the click walk mirrors the paint walk exactly. */
static int g_sysboxX = 0, g_sysboxY = 0, g_sysboxW = 0, g_sysboxH = 0;
extern "C" int bob_dlg_dialog_size(int dlgId, int* w, int* h);
extern "C" int bob_ole_draw_panel(CWnd* dialog, int ox, int oy);
extern "C" void bob_map_paint_toolbars(int sw, int sh)
{
	CMainFrame* mf = (CMainFrame*)AfxGetMainWnd();
	{ static int e = 0; if (!e) { e = 1;
		fprintf(stderr, "[sysbox] paint walk entered: mf=%p sysbox=%p\n",
			(void*)mf, (void*)(mf ? mf->m_wndSystemBox : 0)); } }
	if (!mf) return;
	const int band = 128, by = sh - band;   /* == TELETYPE.CPP FOOTER_BAND_H */
	/* misc map-tool row (thumbnail / files / zoom / mapfilters / replay) -- upper row, right of the log */
	if (CMainFrame::toolbars[TB_MISC])
		bob_ole_draw_toolbar((CWnd*)CMainFrame::toolbars[TB_MISC], 468, by + 8, 100);
	/* main OOB/mission row (bases / squadrons / pilots / mission folder / ...) -- lower row (the wide
	   mission-folder strip in the wine reference) */
	if (CMainFrame::toolbars[TB_MAIN])
		bob_ole_draw_toolbar((CWnd*)CMainFrame::toolbars[TB_MAIN], 468, by + 72, 100);
	/* S173 (PO: "time is stopped at 0x"). The clock panel is drawn WHOLE, from its own template.
	   S94 hand-placed three of its buttons as a free-floating row and left the rest to a scaffold
	   that hand-filled a parchment rectangle (bob_map_paint_titlebar). Two things followed. The
	   panel looked nothing like gold; and IDC_CONTROL -- a FOURTH transport button, at DLU x26,
	   sitting between the end cap and Pause -- was never drawn at all, because a hand-written id
	   list can only contain the ids someone thought to put in it.
	   IDDT_TITLEBAR is a complete 209x56 DLU design: end cap, DATE + DATETIME plate, then
	   CONTROL/PAUSE/PLAY/FASTFORWARD in a row at y30, end cap. Drawing it as a panel takes all of
	   that geometry from the resource, so the row cannot drift from the art and no control can be
	   omitted by oversight. It also gives every button a recorded screen rect, which is what
	   bob_ole_click hit-tests -- the reason the map's accel row could not be clicked, and so the
	   reason the campaign clock could never leave ACCEL_PAUSED (LaunchMap starts paused by
	   design and waits for exactly these buttons). */
	if (CMainFrame::toolbars[TB_TITLE]) {
		TitleBar* tb = (TitleBar*)CMainFrame::toolbars[TB_TITLE];
		tb->Redraw(false);        /* the game's own clock update: DATETIME string + TimeColour +
		                             the pressed states that show which accel rate is live */
		/* 1:1 DLU->px, like the other two toolbars. Measured against gold rather than assumed:
		   gold's four buttons sit on a ~34px pitch for a 37-DLU template spacing, i.e. essentially
		   one pixel per DLU. bob_ole_draw_panel's 1.5x config-panel scale made the panel 313px wide
		   against gold's ~250 and inflated the LCD text with it (the text height is derived from the
		   box height, so a stretched box is a stretched font). Passing nids=0 draws ALL eight
		   template controls, which is the point -- the id list is what lost IDC_CONTROL. */
		bob_ole_draw_toolbar_ids((CWnd*)tb, 0, sh - 56, 100, 0, 0);
	}
	/* S173c: the event log, same story as the clock and the same fix. IDDT_TELETYPE is a real
	   317x56 DLU panel (three two-column log lines + end caps) whose controls the game already
	   fills -- TeleType::Refresh -> FillTextLines(IDC_LINE1..3/IDC_ITEM1..3), driven from the map's
	   own update (MAPDLG.CPP). All eight were hosted and NONE were drawn (`dlg=1065:0/8`), because
	   bob_map_paint_teletype hand-rendered the log instead, on the stated belief that "those
	   controls' OnDraw doesn't run headlessly" -- true when it was written, false since the OCX
	   hosting landed. The scaffold even re-walked Node_Data.intel with the same
	   MessageTitleToText/GetTargName calls Refresh uses, so the port carried two implementations of
	   one thing and drew the copy. */
	if (CMainFrame::toolbars[TB_REPORT]) {
		/* Same shape as the clock above: drive the game's own populate pass, then draw. Refresh()
		   is normally called from the map's update (MAPDLG.CPP) on a new message; force it here so
		   the panel is filled on the first paint too, rather than showing three empty lines until
		   the next intel item happens to arrive. */
		((TeleType*)CMainFrame::toolbars[TB_REPORT])->Refresh(true);
		bob_ole_draw_toolbar_ids((CWnd*)CMainFrame::toolbars[TB_REPORT], 6, sh - 128 + 5, 100, 0, 0);
	}
	/* S183 (BOB-PO-15: "no X at upper right to exit campaign"). The system box was never drawn.

	   It is not one of CMainFrame::toolbars[], so every walk in this file skipped it -- and it is
	   the ONLY way out of a campaign: CSystemBox::OnClickedFiles -> CMainFrame::OnBye -> the Quit
	   Game modal -> ExitCampaign + LaunchFullPane(title). With nothing painting it there was no X,
	   and with nothing hit-testing it there was no exit at all; the PO's only way out was to kill
	   the window. CMainFrame::Initialise creates it (MAINFRM.CPP:382) and the game positions it top
	   right (MAINFRM.CPP:461, `rect.right - systemrect.right - bs, rect.top + bs`) through
	   MoveWindow, which is a no-op here -- so the position has to come from the template, as the
	   clock and event log already do.

	   IDDT_SYSTEM is 34x50 DLU: IDC_FILES is the 34x30 button on top (the exit), with THUMBNAIL and
	   ZOOMIN beneath. Drawn as a whole panel so no control can be lost to a hand-written id list,
	   and so every one gets a recorded screen rect for the click walk below. */
	if (mf->m_wndSystemBox) {
		int dw = 34, dh = 50;
		bob_dlg_dialog_size(IDDT_SYSTEM, &dw, &dh);
		int px = dw * 6 / 4, py = dh * 13 / 8;      /* the shared DLU->px conversion */
		int ox = sw - px - 8; if (ox < 0) ox = 0;
		int drawn = bob_ole_draw_panel((CWnd*)mf->m_wndSystemBox, ox, 8);
		g_sysboxX = ox; g_sysboxY = 8; g_sysboxW = px; g_sysboxH = py;
		static int said = 0;
		if (!said) { said = 1;
			fprintf(stderr, "[sysbox] panel %dx%d DLU -> %dx%d px at (%d,8): %d controls drawn\n",
				dw, dh, px, py, ox, drawn); }
	}
	else { static int said0 = 0; if (!said0) { said0 = 1;
		fprintf(stderr, "[sysbox] m_wndSystemBox is NULL -- nothing to draw\n"); } }
}

/* S92: a click at (cx,cy) on the map -> hit-test the footer toolbar buttons (their last-drawn rects)
   and fire the hit button's Clicked event (OnClickedBases/Missionfolder/...) via the OCX eventsink.
   Returns 1 if a button consumed the click. */
extern "C" int bob_ole_click(CWnd*, int, int);
/* S156 (SP.14, from MA note 31 section 3): route a map click into an OPEN OOB dialog.
   MA found its OOB dialogs painted perfectly and ignored every click, and warned us to check.
   We were in the same state: the map dispatch tried the three toolbars and then fell through to
   unit selection, so a click on the Directives grid, Bases or the Mission Folder did nothing at
   all. Everything those dialogs have ever been driven by in this port was a BOB_* scaffold --
   exactly the evidence pattern MA identified ("a capability only ever exercised through
   scaffolding is evidence the real path is missing").

   Shape follows MA's: an open dialog gets FIRST REFUSAL, and a click inside it that hits no
   control is SWALLOWED rather than falling through to the map underneath (otherwise clicking a
   dialog's background selects a unit behind it). Hit-testing needs no separate geometry: the hosts
   record their last-drawn screen rect at paint time, so hit rects cannot drift from drawn rects --
   which is the drift MA had to engineer around by mirroring its paint walk.
   Walks the same set bob_map_paint_oob draws: each toolbar's logged children, plus descendants
   (the panel-wrapper rule, section 8-BoB155 -- the controls live on the CONTAINED dialog). */
extern "C" int bob_ole_click(CWnd*, int, int);
extern "C" int bob_ole_last_click_id;
/* S173d: drawn bounds of a dialog SUBTREE — the union over the same nodes bob_oob_click_tree
   walks, so the swallow region and the hit-test region are built from one traversal of one set of
   paint-recorded rects. (The panel-wrapper rule again: the controls live on the CONTAINED dialog,
   so a parent's own bounds are usually empty and the children carry the geometry.) */
extern "C" int bob_ole_drawn_bounds(CWnd*, int*, int*, int*, int*);
static int bob_oob_drawn_bounds(RDialog* d, int depth, int* x0, int* y0, int* x1, int* y1)
{
	if (!d || depth > 4) return 0;
	int got = bob_ole_drawn_bounds((CWnd*)d, x0, y0, x1, y1);
	for (RDialog* c = d->fchild; c; c = c->sibling) {
		int cx0, cy0, cx1, cy1;
		if (!bob_oob_drawn_bounds(c, depth + 1, &cx0, &cy0, &cx1, &cy1)) continue;
		if (!got) { *x0 = cx0; *y0 = cy0; *x1 = cx1; *y1 = cy1; got = 1; continue; }
		if (cx0 < *x0) *x0 = cx0;
		if (cy0 < *y0) *y0 = cy0;
		if (cx1 > *x1) *x1 = cx1;
		if (cy1 > *y1) *y1 = cy1;
	}
	return got;
}
static int bob_oob_click_tree(RDialog* d, int cx, int cy, int depth)
{
	if (!d || depth > 4) return 0;
	if (bob_ole_click((CWnd*)d, cx, cy)) return 1;
	for (RDialog* c = d->fchild; c; c = c->sibling)
		if (bob_oob_click_tree(c, cx, cy, depth + 1)) return 1;
	return 0;
}
extern "C" int bob_oob_click_title_glyph(int, int);
extern "C" int bob_map_click_oob(int cx, int cy)
{
	if (getenv("BOB_NO_OOB_CLICK")) return 0;
	/* S174: the title band's tick/X are painted OVER a full-width title RButton, so they must be
	   hit-tested before the control walk or that button swallows them -- which is exactly what the
	   PO's session showed (`[oobclick] ... ctrl id=21` for clicks aimed at the check mark). */
	if (bob_oob_click_title_glyph(cx, cy)) return 1;
	for (int tb = 0; tb < TB_TOTAL; tb++) {
		CRToolBar* t = CMainFrame::toolbars[tb];
		if (!t || !t->dialoglinks.loggedchild) continue;
		for (int i = 0; i < t->dialoglinks.MAX_CHILD_DIALS; i++) {
			RDialog* d = t->dialoglinks.loggedchild[i];
			if (!d) continue;
			if (bob_oob_click_tree(d, cx, cy, 0)) {
				fprintf(stderr, "[oobclick] (%d,%d) consumed by toolbar %d child %d ctrl id=%d\n",
					cx, cy, tb, i, bob_ole_last_click_id);
				return 1;
			}
			/* inside the dialog but on no control -> swallow (do not select the map beneath).
			   S173d: the bound comes from what the dialog DREW, not from GetWindowRect. The compat
			   CWnd::GetWindowRect answers with the WHOLE SCREEN for every window (afxwin.h:
			   left=top=0, right/bottom=bob_gdi_screen_size), so this test read
			   `cx>=0 && cx<1024 && cy>=0 && cy<768` -- true for every pixel. One open OOB dialog
			   therefore swallowed EVERY map click: the accel row, both icon rows, the event log and
			   unit selection all went dead the moment anything was open, which is what the PO hit
			   ("clicking red icon at bottom again does not dismiss dialog"). A stub that returns a
			   plausible value and silently reroutes the work -- this port's signature bug.
			   No drawn bounds (nothing hit-testable) -> do NOT swallow: letting the click through
			   is the recoverable failure, eating it is not. */
			int bx0, by0, bx1, by1;
			int subtree = bob_oob_drawn_bounds(d, 0, &bx0, &by0, &bx1, &by1);
			if (subtree && cx >= bx0 && cx < bx1 && cy >= by0 && cy < by1) {
				CRect r(bx0, by0, bx1, by1);
				/* S173d: was gated behind BOB_TRACE_OLE, which is unusable for answering a click
				   question -- it fires per-control-per-frame and once wrote a 70 MB log that
				   starved a run past its timeout. So the single most consequential thing this
				   dispatch does, DISCARDING a click, was the one thing it never reported. A
				   swallow is per-click and rare; print it always. */
				fprintf(stderr, "[oobclick] (%d,%d) inside dialog %d/%d (%d,%d)-(%d,%d), "
					"no control -- SWALLOWED\n", cx, cy, tb, i,
					(int)r.left, (int)r.top, (int)r.right, (int)r.bottom);
				return 1;
			}
		}
	}
	return 0;
}

/* S173d: report WHICH toolbar control consumed a map click. The OOB-dialog walk has had
   `[oobclick]` since S156, but this dispatch -- the one that serves the accel row, the two icon
   rows and now the event log -- had no trace at all, so "did the click land?" could only be
   inferred from a side effect (a dialog count, a clock advancing). That is a weak oracle: it cannot
   distinguish "the click never arrived" from "it arrived and the handler chose to do nothing",
   which are opposite bugs. One line, only when a control takes it. */
static int bob_tbclick(int tb, const char* what, int cx, int cy)
{
	if (!CMainFrame::toolbars[tb]) return 0;
	if (!bob_ole_click((CWnd*)CMainFrame::toolbars[tb], cx, cy)) return 0;
	fprintf(stderr, "[tbclick] (%d,%d) consumed by %s (TB_%d) ctrl id=%d\n",
		cx, cy, what, tb, bob_ole_last_click_id);
	return 1;
}
extern "C" int bob_map_click_toolbars(int cx, int cy)
{
	if (!AfxGetMainWnd()) return 0;
	if (bob_tbclick(TB_TITLE, "accel row", cx, cy)) return 1;
	if (bob_tbclick(TB_MISC,  "map tools", cx, cy)) return 1;
	if (bob_tbclick(TB_MAIN,  "OOB row",   cx, cy)) return 1;
	/* S173d: THE CLICK WALK MUST MIRROR THE PAINT WALK (the port's own rule, booked after the OOB
	   dialogs painted perfectly and ignored every click). S173c added a fourth toolbar to
	   bob_map_paint_toolbars and this list still had three, so the event log's six line controls
	   got drawn rects that nothing ever hit-tested against -- which is precisely the state the
	   whole S173 chain was about. Caught by predicting the toggle test's result from this list and
	   the test agreeing. TB_REPORT's OnClickedLine1..3 open their message's dialog, and close it
	   when clicked again (BOB-PO-11). */
	if (bob_tbclick(TB_REPORT, "event log", cx, cy)) return 1;
	/* S183: the system box, mirroring the paint above -- the rule this file keeps re-learning is
	   that adding a panel to the paint walk and not to the click walk produces something that looks
	   right and cannot be used (S173d). Hit-testing is bob_ole_click against the rects recorded at
	   paint time, so it cannot drift from what was drawn. */
	{
		CMainFrame* mf = (CMainFrame*)AfxGetMainWnd();
		if (mf && mf->m_wndSystemBox && g_sysboxW > 0) {
			if (bob_ole_click((CWnd*)mf->m_wndSystemBox, cx, cy)) {
				fprintf(stderr, "[sysbox] click (%d,%d) consumed\n", cx, cy);
				return 1;
			}
			if (cx >= g_sysboxX && cx < g_sysboxX + g_sysboxW &&
			    cy >= g_sysboxY && cy < g_sysboxY + g_sysboxH) {
				fprintf(stderr, "[sysbox] click (%d,%d) inside panel but hit no control\n", cx, cy);
				return 1;    /* swallowed, like the OOB dialogs -- don't select a unit underneath */
			}
		}
	}
	return 0;
}
#endif

#if BOB_LINUX
/* S99 (scoping spike): probe an open OOB info dialog (Bases/Squadrons/...) -- walk the logged-child
   RDialog tree and report each node's artwork + child structure, to de-risk headless rendering
   (the same DoPaint + bob_ole_draw_panel path the config panels use). Env-gated BOB_PROBE_OOB. */
extern "C" int g_mapPaintActive;
static void bob_probe_walk(RDialog* d, int depth) {
	for (RDialog* n = d; n; n = n->sibling) {
		long off = n->OnGetXYOffset();
		fprintf(stderr, "[oob] %*snode=%p artnum=%d off=(%d,%d) fchild=%p\n", depth*2, "", (void*)n,
			(int)n->artnum, (short)(off&0xFFFF), (short)((off>>16)&0xFFFF), (void*)n->fchild);
		if (n->fchild) bob_probe_walk(n->fchild, depth+1);
	}
}
extern "C" void bob_probe_oob(void) {
	CMainFrame* mf = (CMainFrame*)AfxGetMainWnd();
	if (!mf || !CMainFrame::toolbars[TB_MAIN]) return;
	static int done = 0; if (done) return;
	RDialog* child = CMainFrame::toolbars[TB_MAIN]->LoggedChild(CMainToolbar::BASES);
	if (!child) { child = CMainFrame::toolbars[TB_MAIN]->LoggedChild(CMainToolbar::SQUADRONLIST); }
	if (child) { done = 1; fprintf(stderr, "[oob] BASES/SQUADRON logged child open:\n"); bob_probe_walk(child, 0); }
}

/* S113: render an open OOB info dialog (Bases/Squadrons/…) onto the map. **The S101 "doesn't render"
   conclusion was a mis-diagnosis** (MA note-4 lead): the S101 pixel check sampled the panel's dark
   corner / a closed-dialog tick and read it as "map colours". The DoPaint->SetDIBitsToDevice write DOES
   land in g_gdiFB and survive to present -- proven by an immediate-after-write pixel probe matching the
   post-present dump (0x464680, the dialog art). So the OOB dialogs render via the same DoPaint path as
   the config panels; the tree's own OnGetXYOffset lays the dialog off-screen headlessly, so position it
   at a fixed base offset here. First increment: the panel background (its FIL_D_GROUPS art). */
extern "C" void bob_gdi_setdibits_origin(int, int);
extern "C" int  bob_ole_draw_panel(CWnd*, int, int);
#if BOB_LINUX
static void bob_oob_paint_title_glyphs(RDialog* n, int ox, int oy);   /* S174 */
#else
#define bob_oob_paint_title_glyphs(a,b,c) ((void)0)
#endif
extern "C" void bob_gdi_setdibits_clip(int x0, int y0, int x1, int y1);
/* S184 (generalises S181). A dialog's art is a SHEET, usually larger than the dialog, and on
   Windows the dialog's own DC clips the blit to its client rect. This port paints with a
   screen-wide CDC, so it set an origin and no clip -- and any dialog whose art exceeds its template
   painted straight over its neighbours and off the screen edge. S181 fixed that for the modal
   message box; the OOB tree walks below had the identical shape, which is why a campaign dialog on
   the map came out huge with its text running past x=1024 (captured in doc/img_oob_unclipped.png).

   RDialog::MY_IID is the template id, so the size comes from the same parser the modal uses. When
   the template is unknown the clip is left off, i.e. exactly today's behaviour -- this can only
   ever shrink what a dialog paints, never move it. BOB_NO_OOB_CLIP=1 reverts. */
static int bob_oob_clip_begin(RDialog* n, int ox, int oy)
{
	static int noClip = -1;
	if (noClip < 0) noClip = getenv("BOB_NO_OOB_CLIP") ? 1 : 0;
	if (noClip) return 0;
	int dw = 0, dh = 0;
	if (!bob_dlg_dialog_size(n->MY_IID, &dw, &dh) || dw <= 0 || dh <= 0) return 0;
	int px = dw * 6 / 4, py = dh * 13 / 8;      /* the shared DLU->px conversion */
	{
		static int seen[64]; static int nseen = 0; int known = 0;
		for (int i = 0; i < nseen; i++) if (seen[i] == n->MY_IID) { known = 1; break; }
		if (!known && nseen < 64) { seen[nseen++] = n->MY_IID;
			fprintf(stderr, "[oobclip] dialog iid=%d (%s) -> %dx%d DLU = %dx%d px at (%d,%d)\n",
				n->MY_IID, typeid(*n).name(), dw, dh, px, py, ox, oy); }
	}
	bob_gdi_setdibits_clip(ox, oy, ox + px, oy + py);
	return 1;
}
static void bob_oob_clip_end(int on) { if (on) bob_gdi_setdibits_clip(0, 0, 0, 0); }
static void bob_oob_paint_tree(RDialog* n, int ox, int oy) {
	for (; n; n = n->fchild) {   /* descend containers to the art leaf (the selected tab page) */
		if (n->artnum) {
			CDC dc; dc.m_bobScreen = true; dc.m_hDC = (HDC)1;
			int clipped = bob_oob_clip_begin(n, ox, oy);
			bob_gdi_setdibits_origin(ox, oy);
			n->DoPaint(&dc);                       /* the tab-page background art */
			bob_gdi_setdibits_origin(0, 0);
			bob_oob_clip_end(clipped);
			bob_ole_draw_panel((CWnd*)n, ox, oy);  /* S113b: its hosted OOB controls (squadron RListBox/RStatic) */
			bob_oob_paint_title_glyphs(n, ox, oy);  /* S174: ? / tick / X on the title band */
		}
	}
}
/* S137: full recursive OOB tree walk (fchild + sibling), drawing EVERY node's art + hosted
   controls -- for dense NESTED dialogs like LW/RAF Directives (gold #18), whose allocation grid
   lives in child sub-panels the fchild-only bob_oob_paint_tree never reaches. The nested controls
   carry their own template rects (template-hosted), positioned relative to the same dialog origin
   headlessly (the layout engine computes no sub-panel offset -- cf. S133), so draw each panel at
   the shared (ox,oy). */
static void bob_oob_paint_tree_deep(RDialog* n, int ox, int oy) {
	for (; n; n = n->sibling) {
		if (n->artnum) {
			CDC dc; dc.m_bobScreen = true; dc.m_hDC = (HDC)1;
			int clipped = bob_oob_clip_begin(n, ox, oy);
			bob_gdi_setdibits_origin(ox, oy);
			n->DoPaint(&dc);
			bob_gdi_setdibits_origin(0, 0);
			bob_oob_clip_end(clipped);
		}
		bob_ole_draw_panel((CWnd*)n, ox, oy);      /* hosted controls regardless of artnum */
		bob_oob_paint_title_glyphs(n, ox, oy);     /* S174: ? / tick / X on the title band */
		if (n->fchild) bob_oob_paint_tree_deep(n->fchild, ox, oy);
	}
}
#if BOB_LINUX
/* S174 (BOB-PO-14): the dialog title band's ? / tick / X glyphs.
   PO, on the Directives dialog: "no way to click on check mark to accept directives". Gold draws
   three red glyphs at the right-hand end of every dialog's title band -- help, ACCEPT, close -- and
   this port drew none of them, because the code that made them is commented out in this source drop
   (RDIALOG.CPP ~437: m_tick->IDOK, m_close->IDCANCEL, m_help, with IDB_TICK/IDB_CLOSE/IDB_HELP) and
   CRTitleCtrl::OnDraw is still the ClassWizard stub. The ids the dead code used are the spec.

   Placement comes from the drawn rect of IDJ_TITLE, which is a full-width RButton (0,0,486,17 in
   IDD_LWDIRECTIVES) -- so the glyphs sit at its right end and the band's own height sets their size.
   That same full-width button is why drawing alone is not enough: it covers the glyph positions and
   consumed the PO's clicks (`[oobclick] (767,37) ... ctrl id=21`), so the glyph hit-test has to run
   BEFORE the general control walk. Paint order and hit order have to agree. */
extern "C" int bob_ole_drawn_rect(CWnd*, int, int*, int*, int*, int*);
extern "C" int bob_icon_pagenum(const char*);
struct BobTitleGlyph { RDialog* dlg; int x, y, w, h, cmd; };
static BobTitleGlyph g_titleGlyph[16];
static int g_nTitleGlyph = 0;

static void bob_oob_paint_title_glyphs(RDialog* n, int /*ox*/, int /*oy*/)
{
	int tx, ty, tw, th;
	if (!bob_ole_drawn_rect((CWnd*)n, IDJ_TITLE, &tx, &ty, &tw, &th)) return;
	if (tw <= 0 || th <= 0) return;
	int sz = th > 4 ? th - 2 : th;              /* glyph box, inset in the band */
	if (sz > 20) sz = 20;
	int gap = sz / 4 + 2;
	int cy = ty + (th - sz) / 2;
	/* right-to-left: X (close), tick (accept), ? (help) -- gold's order left-to-right is ? tick X */
	struct { const char* icon; int cmd; } g[3] = {
		{ "ICON_CROSS",         IDCANCEL },
		{ "ICON_TICK",          IDOK     },
		{ "ICON_RED_QUESTION",  0        },   /* help: drawn for fidelity, no handler in this port */
	};
	int cx = tx + tw - gap - sz;
	CDC dc; dc.m_bobScreen = true; dc.m_hDC = (HDC)1;
	for (int i = 0; i < 3; i++) {
		int pv = bob_icon_pagenum(g[i].icon);
		if (pv) {
			IconDescUI ic = IconsUI(pv);
			CPoint pt(cx, cy);
			ic.MaskIcon(&dc, pt);
		}
		if (g_nTitleGlyph < 16 && g[i].cmd) {
			BobTitleGlyph& e = g_titleGlyph[g_nTitleGlyph++];
			e.dlg = n; e.x = cx; e.y = cy; e.w = sz; e.h = sz; e.cmd = g[i].cmd;
		}
		cx -= sz + gap;
	}
}

/* Hit-test the glyphs FIRST (see above: the title button covers them). tick -> IDOK,
   cross -> IDCANCEL, routed through the dialog's own handlers. */
/* S174: close a MODELESS dialog that is a logged child of one of the map toolbars. Called from
   CDialog::EndDialog, which the game reaches via RDialog::OnOK/OnCancel. Returns 1 if it owned and
   closed the dialog, 0 otherwise (so the modal path is untouched). */
extern "C" int bob_close_logged_dialog(void* dlgp)
{
	if (!dlgp || !AfxGetMainWnd()) return 0;
	RDialog* d = (RDialog*)dlgp;
	for (int tb = 0; tb < TB_TOTAL; tb++) {
		CRToolBar* t = CMainFrame::toolbars[tb];
		if (!t || !t->dialoglinks.loggedchild) continue;
		for (int i = 0; i < t->dialoglinks.MAX_CHILD_DIALS; i++) {
			/* S179b: match the dialog OR any DESCENDANT of it. The title glyphs are painted for
			   every node of an OOB dialog tree (the panel-wrapper rule again -- the controls, and
			   hence the title band, live on a CONTAINED dialog), so the node a glyph belongs to is
			   usually not the toolbar's top-level logged child. Matching only the root meant the
			   close silently found nothing and the glyph did nothing at all. */
			RDialog* root = t->dialoglinks.loggedchild[i];
			if (!root) continue;
			int owns = (root == d);
			if (!owns) {
				/* shallow descendant walk, same depth bound as the click walk */
				for (RDialog* c = root->fchild; c && !owns; c = c->sibling) {
					if (c == d) { owns = 1; break; }
					for (RDialog* g = c->fchild; g; g = g->sibling)
						if (g == d) { owns = 1; break; }
				}
			}
			if (!owns) continue;
			fprintf(stderr, "[dlgclose] toolbar %d child %d closed (root=%p, asked=%p)\n",
				tb, i, (void*)root, (void*)d);
			t->CloseLoggedChild(root);
			return 1;
		}
	}
	return 0;
}

extern "C" int bob_oob_click_title_glyph(int cx, int cy)
{
	for (int i = 0; i < g_nTitleGlyph; i++) {
		BobTitleGlyph& e = g_titleGlyph[i];
		if (cx < e.x || cx >= e.x + e.w || cy < e.y || cy >= e.y + e.h) continue;
		fprintf(stderr, "[titleglyph] (%d,%d) -> %s on dialog %p (%s)\n",
			cx, cy, e.cmd == IDOK ? "IDOK (accept)" : "IDCANCEL (close)", (void*)e.dlg,
			typeid(*e.dlg).name());
		/* S174: accept runs the dialog's own OnOK (so it commits whatever it commits), then the
		   dialog is closed the way the rest of this port closes a logged child.
		   NOT via EndDialog: afxwin.h records (S154) that real dialog teardown is deliberately
		   off by default because it once recursed to a stack overflow (S108's per-slot guard) and
		   leaked 181,424 hosted controls -- and routing close through EndDialog would reintroduce
		   exactly that loop, EndDialog -> close -> OnCancel -> EndDialog. CloseLoggedChild is the
		   game's own API and is already what TeleType's line-toggle and CMainFrame use.
		   OnCancel is a documented no-op here, so the cross just closes. */
		/* S179: the tick used to run the dialog's OnOK. That was too strong a verb to hand every
		   dialog: OnOK means "confirm and PROCEED", and what it proceeds to differs per dialog --
		   the PO's session shows two [titleglyph] IDOK clicks immediately followed by LaunchMain,
		   i.e. accepting a campaign dialog left the campaign and returned to the front end, which
		   read to the player as "clicked Fly, it hung". My own regression, from wiring one glyph to
		   a verb I had not checked per dialog.
		   Default now: both glyphs CLOSE, which is the behaviour the player can always undo.
		   BOB_TITLE_TICK_OK=1 restores OnOK for whoever verifies it dialog by dialog. */
		/* S179b: accept is RESTORED as the default. Disabling it made the tick inert, and on the
		   Directives dialog accept is the whole point -- it is how the player commits directives and
		   moves the campaign on, which is what the PO asked for in the first place. The risk it
		   carries is real (an OnOK on some other dialog left the campaign), so the trace above now
		   names the dialog CLASS on every glyph click: the next time it misbehaves we will know
		   which dialog rather than guessing. BOB_TITLE_TICK_OK=0 disables accept. */
		static int tickOk = -1;
		if (tickOk < 0) { const char* e2 = getenv("BOB_TITLE_TICK_OK"); tickOk = e2 ? atoi(e2) : 1; }
		if (e.cmd == IDOK && tickOk) e.dlg->OnOK();
		bob_close_logged_dialog((void*)e.dlg);
		return 1;
	}
	return 0;
}
#endif
/* S186: centre an OOB dialog on its OWN width. The three walks below all used `(sw - 340) / 2` --
   a magic number standing in for "how wide is this dialog", written before the template parser
   could answer that. It is wrong for every dialog that is not 340px: InterceptOffered is 400px
   (267 DLU), so it sat 30px right of centre and further over the Directives panel beside it.
   Falls back to the old constant when the template is unknown, so no dialog can end up worse
   placed than it already was. */
/* The logged child is a WRAPPER -- the panel-wrapper rule (section 8-BoB155) that this file has
   now hit five times: the controls, the title band and the real template all live on a CONTAINED
   dialog. Centring on the wrapper's own template put InterceptOffered at 362 instead of 312,
   because the wrapper measures 200 DLU and the dialog inside it measures 267. Take the WIDEST
   template in the subtree, which is what will actually be painted at this origin -- the same node
   bob_oob_clip_begin clips to. */
static int bob_oob_widest(RDialog* n, int depth)
{
	int best = 0;
	for (; n; n = n->sibling) {
		int dw = 0, dh = 0;
		if (bob_dlg_dialog_size(n->MY_IID, &dw, &dh) && dw > best) best = dw;
		if (n->fchild && depth < 4) {
			int c = bob_oob_widest(n->fchild, depth + 1);
			if (c > best) best = c;
		}
	}
	return best;
}
static int bob_oob_centre_x(RDialog* d, int sw)
{
	int px = 340;
	int dw = d ? bob_oob_widest(d, 0) : 0;
	if (dw > 0) px = dw * 6 / 4;
	int x = (sw - px) / 2;
	return x < 0 ? 0 : x;
}
extern "C" int bob_map_paint_oob(int sw, int sh) {
#if BOB_LINUX
	g_nTitleGlyph = 0;   /* rebuilt every paint, so the hit rects cannot outlive the drawn ones */
#endif
	/* S116: render whichever OOB info dialog is open (any main-toolbar logged child), so a real
	   toolbar-button click (S92) that opened one actually shows it. Renders each open child's tree
	   (background art via DoPaint + hosted controls via bob_ole_draw_panel) at a fixed panel origin. */
	CRToolBar* tb = CMainFrame::toolbars[TB_MAIN];
	int n = 0;
	if (tb)
		for (int slot = 0; slot < CMainToolbar::MAX_ID; slot++) {
			RDialog* child = tb->LoggedChild(slot);
			if (child) { bob_oob_paint_tree(child, bob_oob_centre_x(child, sw), 40); n++; }
		}
	/* S137: the misc toolbar (TB_MISC) also logs OOB dialogs -- notably the LW/RAF Directives
	   (gold #18). Render its open logged children too; the Directives dialog is wide (~740px per
	   gold), so anchor it near the top-left like gold rather than the Bases centre offset. */
	CRToolBar* tbm = CMainFrame::toolbars[TB_MISC];
	if (tbm)
		for (int slot = 0; slot < CMiscToolbar::LISTLEN; slot++) {
			RDialog* child = tbm->LoggedChild(slot);
			if (child) { bob_oob_paint_tree_deep(child, 44, 20); n++; }  /* dense nested grid */
		}
	/* S173d: TB_REPORT logs one too. The event log's OnClickedLine1..3 open the clicked message's
	   dialog via LogChild(0, IntellMessages::Make(...)), and once the click actually reached them
	   (this sprint) that dialog opened and drew NOTHING -- `dialogs=7 ... dlg=1087:0/15` -- because
	   this walk knew about two toolbars and the game logs children on three. The paint walk and the
	   click walk have to agree about the SET as well as the geometry; S173d had to fix the same
	   omission on the click side an hour earlier. Anchored like the TB_MAIN dialogs. */
	CRToolBar* tbr = CMainFrame::toolbars[TB_REPORT];
	if (tbr)
		for (int slot = 0; slot < tbr->dialoglinks.MAX_CHILD_DIALS; slot++) {
			RDialog* child = tbr->LoggedChild(slot);
			if (child) { bob_oob_paint_tree(child, bob_oob_centre_x(child, sw), 40); n++; }
		}
	return n;
}
/* deterministically open Bases (fire OnClickedBases) once the map has settled -- removes the
   click-coordinate/paint-throttle variable from OOB render testing (BOB_MAP_OOB). */
extern "C" int bob_evt_fire(void*, const void*, int, int);
extern "C" void bob_oob_open_bases(void) {
	static int done = 0; if (done) return; done = 1;
	CRToolBar* tb = CMainFrame::toolbars[TB_MAIN];
	if (!tb) return;
	/* BOB_MAP_OOB=2 opens Squadrons (id 1829) instead of Bases (1827), to test the render across
	   dialog structures deterministically (headless clicks are nondeterministic). */
	int id = (getenv("BOB_MAP_OOB") && atoi(getenv("BOB_MAP_OOB")) == 2) ? 1829 : 1827;
	fprintf(stderr,"[oob] firing OnClicked id=%d\n", id);
	bob_evt_fire((void*)tb, &typeid(*tb), id, 1);
}
/* S137: open the LW Directives dialog (gold #18) deterministically once the map has settled.
   CMiscToolbar::OpenDirectivetoggle(NULL) LogChild's LWDirectives::Make(NULL) -- the ctor builds a
   default LWDirectivesResults from MMC.directives.lw.current, so NULL is safe. BOB_MAP_DIRECTIVES. */
extern "C" void bob_oob_open_directives(void) {
	static int done = 0; if (done) return; done = 1;
	fprintf(stderr,"[directives] OpenDirectivetoggle(NULL)\n");
	MiscToolBar().OpenDirectivetoggle(NULL);
}
/* S143 (SP.5, from MA note 29 §2): DISMISS whatever misc-toolbar dialog is up, so a parity
   capture can see the map underneath. The point of the note: a capture scaffold must not route
   through a *toggle*, because the toggle's branch depends on state the scaffold did not create.
   On an active campaign day the GAME opens Directives itself (DIRRSULT.CPP:197), so calling
   OpenDirectivetoggle again is a coin-flip on who logged what; CloseLoggedChildren() is
   unconditional and index-agnostic, which is exactly the property a scaffold needs.
   BOB_MAP_CLOSEDLG=<n>: close after n map paints. Reports what it closed. */
/* S143 (SP.5, the RIGHT lever -- found by instrumenting instead of fighting the close).
   Closing the directives dialogs does not work and cannot work: on an active campaign day the
   game keeps a STACK of two that mutually re-open each other -- `DirectiveResults::OnCancel`
   calls `OpenDirectivetoggle(dr)` (DIRRSULT.CPP:194) and the day-start path re-opens on
   `!MMC.directivespopup` (LWDIRECT.CPP:2050). A close loop just oscillates 6→5→6→5 (measured),
   and every re-open re-creates the dialog's hosted controls, which the state banner caught
   ballooning 184 → 1656.

   The game's own lever is the misc-toolbar DIRECTIVES toggle, whose genuine handler flips
   `MMC.directivespopup`; with it TRUE the day-start path never pops the dialog. Drive the
   genuine handler (MA note 29's rule) rather than poking the flag, so the toolbar button state
   and hint string stay consistent. BOB_MAP_NODIRECTIVES=1. */
extern "C" void bob_oob_suppress_directives(void) {
	static int done = 0; if (done) return; done = 1;
	CMiscToolbar& mtb = MiscToolBar();
	/* `OnClickedDirectivetoggle` is protected (an afx_msg handler), so drive it the way the rest
	   of the port drives toolbar buttons: fire the genuine Clicked event (dispid 1) at
	   IDC_DIRECTIVETOGGLE through the S33 eventsink, exactly as bob_oob_open_bases does. That
	   keeps the toolbar's pressed state and hint string consistent, which poking MMC directly
	   would not. */
	if (!MMC.directivespopup) bob_evt_fire((void*)&mtb, &typeid(mtb), 1007 /*IDC_DIRECTIVETOGGLE*/, 1);
	fprintf(stderr,"[directives] popup suppressed: MMC.directivespopup=%d (open-index=%d)\n",
		(int)MMC.directivespopup, mtb.LoggedChild());
	/* Anything already up still needs dismissing, and the stack unwinds one level per pass:
	   closing DIRECTIVES(6) leaves DIRECTIVERESULTS(5); closing 5 fires its OnCancel, which
	   re-opens 6 ONE more time -- but with the popup now suppressed the day-start path no longer
	   re-arms it, so the loop converges instead of oscillating. Bounded regardless. */
	for (int pass = 0; pass < 8; pass++) {
		int open = mtb.LoggedChild();
		if (open < 0) break;
		mtb.CloseLoggedChildren();
		if (mtb.LoggedChild() == open) { if (mtb.CloseLoggedChild(open)) break; }
		fprintf(stderr,"[directives]   dismiss pass %d: %d -> %d\n", pass, open, mtb.LoggedChild());
	}
	fprintf(stderr,"[directives] after dismiss: open-index=%d\n", mtb.LoggedChild());
	{ extern void bob_shot_arm(const char*); bob_shot_arm("suppress-directives"); }   /* S145 */
}

/* S144 (SP.11, gold #19): ACCEPT the LW orders flow instead of trying to escape it.
   S143 established that Directives <-> DirectiveResults is a closed loop on *cancel* -- each
   one's OnCancel opens the other (LWDIRECT.CPP:1104, DIRRSULT.CPP:194) -- which is faithful: the
   Luftwaffe player must ISSUE orders. The exit is OnOK, and DirectiveResults::OnOK calls
   LWDirectivesResults::MakeLWPackages(dr, true) (DIRRSULT.CPP:207) -- the call that builds the
   day's raid packages. So completing the flow is not just how the map gets uncovered, it is what
   CREATES the raid stacks and route lines gold #19 shows.

   Drive it the genuine way (MA note 29 §1: drive the handler, don't reimplement it): the title
   bar's OK is an event on the dialog itself: ON_EVENT(RDialog, IDJ_TITLE, 3 = OK, OnOK)
   (RDIALOG.CPP:1179) -- so fire dispid 3 at IDJ_TITLE on whatever child is logged, using the
   dialog's RUNTIME type so the DERIVED OnOK runs (LWDirectives::OnOK / DirectiveResults::OnOK),
   not RDialog's. Accepting one opens the next, so loop until nothing is logged. BOB_MAP_ACCEPTDIR. */
extern "C" void bob_oob_accept_directives(void) {
	static int done = 0; if (done) return; done = 1;
	CMiscToolbar& mtb = MiscToolBar();
	for (int pass = 0; pass < 8; pass++) {
		int idx = mtb.LoggedChild();
		if (idx < 0) { fprintf(stderr,"[directives] accept: nothing logged after %d pass(es)\n", pass); break; }
		RDialog* dlg = mtb.LoggedChild(idx);
		if (!dlg) { fprintf(stderr,"[directives] accept: index %d has no dialog\n", idx); break; }
		/* S146: the logged child is a PLACEHOLDER PANEL (traced: rtti=RDEmptyD), not the dialog.
		   LWDirectives::Make returns MakeTopDialog(.., DialBox(art, new LWDirectives(dr))), so the
		   real dialog hangs off the panel as fchild. S144/S145 fired OK at the PANEL, which ran
		   RDialog::OnOK -> EndDialog and closed it without ever reaching LWDirectives::OnOK --
		   while reporting success. Fire at the children first; the panel remains the fallback. */
		int firedChild = 0;
		for (RDialog* c = dlg->fchild; c; c = c->sibling) {
			fprintf(stderr,"[directives]   child %p rtti=%s -- firing OK\n", (void*)c, typeid(*c).name());
			if (bob_evt_fire((void*)c, &typeid(RDialog), 21 /*IDJ_TITLE*/, 3 /*OK*/)) firedChild = 1;
		}
		fprintf(stderr,"[directives] accept pass %d: child-fire=%d\n", pass, firedChild);
		fprintf(stderr,"[directives] accept pass %d: firing title-bar OK on child %d\n", pass, idx);
		/* Fire under typeid(RDialog), NOT typeid(*dlg): bob_evt_fire matches the type
		   EXACTLY (bob_eventsink.cpp:39, *v[i].ti == *dt) with no base-class walk, and the
		   title-bar OK entry is registered on RDialog. NB (S145): this alone does NOT reach a derived handler when the target is a panel --
		   RDialog::OnOK implicitly overrides compat's virtual CDialog::OnOK, so the call
		   virtual-dispatches to LWDirectives::OnOK / DirectiveResults::OnOK. */
		bob_evt_fire((void*)dlg, &typeid(RDialog), 21 /*IDJ_TITLE*/, 3 /*OK*/);
		if (mtb.LoggedChild() == idx) {   /* handler didn't take it down -- don't spin */
			fprintf(stderr,"[directives] accept: child %d still logged after OK; stopping\n", idx);
			break;
		}
	}
	fprintf(stderr,"[directives] accept done: open-index=%d\n", mtb.LoggedChild());
	/* S145 (SP.7): arm the capture RELATIVE to this event, so BOB_SHOT_AFTER=<n> lands n map
	   paints after the orders flow completes rather than at an absolute tick that drifts. */
	{ extern void bob_shot_arm(const char*); bob_shot_arm("accept-directives"); }
}

/* S148 (SP.16): is ANY toolbar holding a logged child? The capture predicate's "map is
   unobstructed" term. Deliberately asks every toolbar: S146 learned the hard way that a dialog can
   be logged on one toolbar while another truthfully reports -1 (the Mission Folder). */
extern "C" int bob_oob_any_dialog_open(void) {
	for (int tb = 0; tb < TB_TOTAL; tb++) {
		CRToolBar* t = CMainFrame::toolbars[tb];
		if (t && t->LoggedChild() >= 0) return 1;
	}
	return 0;
}

extern "C" void bob_oob_close_dialogs(void) {
	/* S147: deliberately NOT one-shot. It was, and that is MA note 29 section 3's trap in my own
	   scaffold: a drive hook guarded by a function-local `static done` fires once per PROCESS, so
	   every dialog the game opened LATER sailed past it -- the Mission Folder, then the "Take over?"
	   intercept prompt -- and I was writing that up as "the game keeps re-opening dialogs". Part of
	   that was real (the Directives popup genuinely re-arms; measured as an oscillation), but part
	   was this. A dismiss-for-capture hook must re-run: on a live raid day the campaign is a
	   sequence of prompts, so whatever pops between arming and the shot has to be cleared too.
	   Cheap: every call early-outs per toolbar when nothing is logged. */
	/* S146: dialogs are logged per TOOLBAR, and a scaffold that walks only the misc toolbar leaves
	   the others up -- the post-orders Mission Folder is logged on a different one, so
	   MiscToolBar().LoggedChild() correctly said -1 while the folder still covered half the map.
	   Sweep every toolbar; same "a scaffold must not assume a single dialog" rule as the stack. */
	for (int tb = 0; tb < TB_TOTAL; tb++) {
		CRToolBar* t = CMainFrame::toolbars[tb];
		if (!t) continue;
		int had = t->LoggedChild();          /* S148: remember, so we can log only on a transition */
		for (int pass = 0; pass < 4 && t->LoggedChild() >= 0; pass++) {
			int open = t->LoggedChild();
			t->CloseLoggedChildren();
			if (t->LoggedChild() == open) { if (t->CloseLoggedChild(open)) break; }
		}
		/* S148: report only when this pass actually CLOSED something. Making the hook repeatable
		   (S147) without making its logging conditional turned it into per-paint spam -- one run
		   produced a 3.5-million-line log, which is both noise and real stderr I/O drag on a capture
		   that has to reach a particular game state. "Filter, don't cap" applies to scaffold logging
		   too: print the transition, not the poll. */
		if (had >= 0)
			fprintf(stderr,"[directives] toolbar %d: %d -> %d\n", tb, had, t->LoggedChild());
	}
	/* S150 (SP.18): the misc-toolbar-only loop that used to live here is GONE, not silenced.
	   It dated from S143 when only that toolbar was swept; S146's all-toolbar loop above
	   superseded it, and leaving it behind meant the stack was walked twice AND it printed
	   ~2 lines per call unconditionally -- the residue of the same per-paint spam S148 fixed
	   in the loop above but not here. Deleting redundant code beats making it quieter:
	   the duplicate was also a second place for the dismiss policy to drift. */
}
#endif
