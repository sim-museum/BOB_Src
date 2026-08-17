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

// Not A RowanDialog: // RMdlDlg.cpp : implementation file
//
// a greatly reduced rdialog used for message boxes
// does not support dialog nesting or resizing
// does not necessarily have to be modal though

#if defined(BOB_LINUX)
#include "bob_mfc_pre.h"	/* F_BATTLE + cstring before stdafx/afxwin */
#endif
#include "stdafx.h"
#include "mig.h"
#if defined(BOB_LINUX)
/* worldinc (item base + ITEMSIZE/AIRSTRUCSIZE/etc.) must precede post.h, whose
   infoitem.h info_itemS() ctor does Status.size=ITEMSIZE */
class ViewPoint;
#include "worldinc.h"
#include "ranges.h"
#include "package.h"
#include "missman2.h"
#include "nodebob.h"
#include "bob_mfc_post.h"	/* control wrappers + dialog ecosystem */
#endif
#include "RMdlDlg.h"
#include "messages.h"
#include "fileman.h"
#include "MIGView.h"
#include "uiicons.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#ifndef THIS_FILE_DEFINED
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif
extern CFont* (g_AllFonts[][4]);
extern CDC g_OffScreenDC;

/////////////////////////////////////////////////////////////////////////////
// RMdlDlg dialog




RMdlDlg::RMdlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(RMdlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(RMdlDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pfileblock=NULL;
	m_lbuttondown=FALSE;
}


void RMdlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RMdlDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX,IDC_OK,m_IDC_OK);
	DDX_Control(pDX,IDC_CANCEL,m_IDC_CANCEL);
	DDX_Control(pDX,IDJ_TITLE,m_IDJ_TITLE);

	DDX_Control(pDX,IDC_RETRY,m_IDC_RETRY);
	DDX_Control(pDX,IDC_MESSAGE_TEXT,m_IDC_MESSAGE_TEXT);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(RMdlDlg, CDialog)
	//{{AFX_MSG_MAP(RMdlDlg)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_MESSAGE( WM_GETARTWORK, OnGetArt)
	ON_MESSAGE( WM_GETXYOFFSET, OnGetXYOffset)
	ON_MESSAGE( WM_GETGLOBALFONT, OnGetGlobalFont)
	ON_MESSAGE( WM_GETFILE, OnGetFile)
	ON_MESSAGE( WM_RELEASELASTFILE, OnReleaseLastFile)
	ON_MESSAGE( WM_GETX2FLAG, OnGetX2Flag)
	ON_MESSAGE( WM_GETOFFSCREENDC, OnGetOffScreenDC)
	ON_MESSAGE( WM_GETHINTBOX, OnGetHintBox)
	ON_MESSAGE( WM_PLAYSOUND, OnPlaySound)
	ON_MESSAGE( WM_GETCOMBODIALOG, OnGetComboDialog)
	ON_MESSAGE( WM_GETCOMBOLISTBOX, OnGetComboListbox)
	ON_MESSAGE( WM_GETSTRING, OnGetString)
	ON_MESSAGE( WM_ACTIVEXSCROLL, OnActiveXScroll)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#if defined(BOB_LINUX)
/* OnCommandHelp is declared by the rdialmsg.h mixin (in the class body) but
   rdialmsg.cpp only defines the OTHER mixin handlers — each dialog provides its
   own OnCommandHelp (RDialog does in RDialog.cpp). For this modal dialog, no
   context-help routing is needed. */
LRESULT RMdlDlg::OnCommandHelp(WPARAM, LPARAM) { return 0; }
#endif

/////////////////////////////////////////////////////////////////////////////
// RMdlDlg message handlers


#define	 DIALCLASS	RMdlDlg
#pragma warnmsg("Windows message functions:")
#include	"rdialmsg.cpp"
#undef	 DIALCLASS


FileNum RMdlDlg::OnGetArt()
{
	return m_artnum;
}

long RMdlDlg::OnGetXYOffset()
{
	return 0;
}

void RMdlDlg::OnActiveXScroll(int,int)
{
}

BEGIN_EVENTSINK_MAP(RMdlDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(RMdlDlg)
	ON_EVENT(RMdlDlg, IDC_OK, 1 /* Clicked */, OnClickedOk, VTS_NONE)
	ON_EVENT(RMdlDlg, IDC_RETRY, 1 /* Clicked */, OnClickedRetry, VTS_NONE)
	ON_EVENT(RMdlDlg, IDC_CANCEL, 1 /* Clicked */, OnClickedCancel, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void RMdlDlg::OnClickedOk() 
{
	EndDialog(0);
}

void RMdlDlg::OnClickedRetry() 
{
	EndDialog(2);
}

void RMdlDlg::OnClickedCancel() 
{
	EndDialog(1);
}

BOOL RMdlDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// scale due to font size change
	// xfactor seems to be slightly smaller than yfactor
	// for some reason
	float yfactor=96.0f/RDialog::fontdpi;
	float xfactor=yfactor;
	if (xfactor!=1.0f)
		xfactor*=0.94f; // fiddle factor
	CRect rect;
	CRect parentrect;
	CWnd* pControl;
	pControl=GetTopWindow();
	GetClientRect(parentrect);
	ClientToScreen(parentrect);
	while (pControl)
	{
		// find coords relative to parent window...
		// why they dont have a ClientToParent function I dont know
		pControl->GetWindowRect(rect);
		rect-=parentrect.TopLeft();
		rect.right*=xfactor;
		rect.bottom*=yfactor;
		rect.left*=xfactor;
		rect.top*=yfactor;
		pControl->MoveWindow(rect);
		pControl=pControl->GetNextWindow();
	}
	// size the actual dialog
	CRect rect2;
	GetClientRect(rect2);
	rect2.right*=xfactor;
	rect2.bottom*=yfactor;

	CRButton* rbutton;
	CRStatic* rstatic;
	CString string;
	rbutton=GETDLGITEM(IDJ_TITLE);
//DeadCode JIM 19Sep00 	string.LoadString(m_TitleID);
	rbutton->SetString(m_TitleID);
	rstatic=GETDLGITEM(IDC_MESSAGE_TEXT);
//DeadCode JIM 19Sep00 	string.LoadString(m_MessageID);
	rstatic->SetString(m_MessageID);
	rbutton=GETDLGITEM(IDC_OK);
	if (m_Button0ID)
	{
		string.LoadString(m_Button0ID);
		rbutton->SetString(string);
	}
	else
		rbutton->ShowWindow(SW_HIDE);
	rbutton=GETDLGITEM(IDC_CANCEL);
	if (m_Button1ID)
	{
		string.LoadString(m_Button1ID);
		rbutton->SetString(string);
	}
	else
		rbutton->ShowWindow(SW_HIDE);
	rbutton=GETDLGITEM(IDC_RETRY);
	if (m_Button2ID)
	{
		string.LoadString(m_Button2ID);
		rbutton->SetString(string);
	}
	else
		rbutton->ShowWindow(SW_HIDE);
	// centre the dialog on the screen
	GetDesktopWindow()->GetWindowRect(rect);
	MoveWindow((rect.Width()-rect2.Width())/2,(rect.Height()-rect2.Height())/2,rect2.Width(),rect2.Height());
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void RMdlDlg::OnPaint() 
{
	PAINTSTRUCT paintstruct;
	CDC* pDC;
	pDC=this->BeginPaint(&paintstruct);
	paintstruct.fErase=TRUE;
	BYTE* pData;
	if (m_artnum)
	{
		fileblock picture(m_artnum);
		pData=(BYTE*)getdata(picture);
		if (pData[0]=='B' && pData[1]=='M') // checks if its a bitmap file
		{
			// now render it...
			BITMAPFILEHEADER* pFile=(BITMAPFILEHEADER*)pData;
			BITMAPINFO* pInfo=(BITMAPINFO*)(pData+sizeof(BITMAPFILEHEADER));
			pData+=pFile->bfOffBits;
			// check if any of the parents have the same artwork then calculate an offset
			long offsets=OnGetXYOffset();
			SetDIBitsToDevice(pDC->m_hDC,short(offsets & 0x0000FFFF),short((offsets & 0xFFFF0000)>>16),pInfo->bmiHeader.biWidth, pInfo->bmiHeader.biHeight,
				0,0,0,pInfo->bmiHeader.biHeight,pData,pInfo,DIB_RGB_COLORS);
		}
	}
	this->EndPaint(&paintstruct);
}

void RMdlDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	m_point=point;	
	m_lbuttondown=TRUE;
	SetCapture();
	CDialog::OnLButtonDown(nFlags, point);
}

void RMdlDlg::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_lbuttondown)
	{
		CRect rect;
		GetWindowRect(rect);
		rect+=point-m_point;
		MoveWindow(rect);
	}
	CDialog::OnMouseMove(nFlags, point);
}

void RMdlDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	m_lbuttondown=FALSE;
	ReleaseCapture();
	CDialog::OnLButtonUp(nFlags, point);
}

BOOL RMdlDlg::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

#if defined(BOB_LINUX)
/* ---------------------------------------------------------------------------------------------
   S165 (cross-port from MA S138): a real modal loop for the game's message box.

   `CDialog::DoModal` was `{ return -1; }` and `EndDialog` a no-op, so RDialog::RMessageBox always
   returned -1 and none of its callers ever saw a human answer:

     CMainFrame::OnBye         `if (rv==0) save; else if (rv<2) quit;`   -1 < 2  -> QUIT, unasked
     LWDIRECT bad weather      `if (RMessageBox(...)==1) badweather=false;`      -> never flies
     LWDIRECT aircraft alloc   branches on rv                                    -> stale branch

   The quit path is the dangerous one: the game threw the player out of a campaign without ever
   showing the Save / Yes / Cancel it was written to show.

   This runs the loop the port never had: pump input, let the dialog paint its own art, draw its
   hosted controls, present, until one of the buttons calls EndDialog (0 = OK/Save, 1 = Cancel,
   2 = Retry -- RMDLDLG.CPP:198-211). Clicks are routed to this dialog alone while it runs, which
   is what "modal" means; RMessageBox has already disabled every toolbar around the call.

   Scoped to RMdlDlg deliberately: the other DoModal call sites are RDialog's and CRToolBar's
   forwarding overrides, and a nested loop under a dialog the port drives differently would hang
   rather than fail visibly.
   --------------------------------------------------------------------------------------------- */
extern "C" {
void  bob_modal_set_at(void* dlg, int ox, int oy, int w, int h);
void  bob_modal_pump(void);
void  bob_gdi_present(void);
void  bob_gdi_screen_size(int* w, int* h);
int   bob_ole_draw_panel(CWnd*, int, int);
int   bob_gdi_dump_to(const char*);
void  bob_gdi_setdibits_origin(int x, int y);
void  bob_gdi_get_setdibits_origin(int* x, int* y);
void  bob_gdi_setdibits_clip(int x0, int y0, int x1, int y1);
void  bob_gdi_get_setdibits_clip(int* x0, int* y0, int* x1, int* y1);
int   bob_dlg_dialog_size(int dlgId, int* w, int* h);
}

int RMdlDlg::DoModal()
{
	Create(RMdlDlg::IDD, m_pBobParent);    /* template + OnInitDialog + host the controls */

	int sw = 800, sh = 600;
	bob_gdi_screen_size(&sw, &sh);
	/* S181 (PO: "Caught Naping dialog too big"). The size is the TEMPLATE's, not a guess.

	   OnPaint blits the whole background bitmap -- ~780x585 for FIL_D_BLANK -- at OnGetXYOffset(),
	   which is 0. On Windows that lands in the dialog's own DC and is clipped to its client rect,
	   so the player sees a 272x104-DLU panel showing the top-left corner of a larger shared art
	   sheet. Our CDC is the entire framebuffer with no origin and no clip, so the bitmap covered
	   most of the screen from (0,0) while the title, message and buttons were drawn centred --
	   captured in doc/img_modal_oversized.png: art at the top-left, controls floating mid-screen.

	   The old `const int dw = 400, dh = 200` was a nominal guess that happened to be close to the
	   real 272x104 DLU (= 408x169 px) but had no relationship to it, and nothing clipped to it
	   anyway. Take the template size and enforce it, which is what the missing window did.
	   BOB_NO_MODAL_CLIP=1 reverts. */
	int dluW = 272, dluH = 104;                    /* IDDS_MODAL_DIALOG's template, as a fallback */
	bob_dlg_dialog_size(RMdlDlg::IDD, &dluW, &dluH);
	const int dw = dluW * 6 / 4;                   /* MS Sans Serif 8pt base units ~ (6,13) */
	const int dh = dluH * 13 / 8;                  /* -- the same conversion bob_ole.cpp uses */
	int ox = (sw - dw) / 2; if (ox < 0) ox = 0;
	int oy = (sh - dh) / 2; if (oy < 0) oy = 0;
	static int noClip = -1;
	if (noClip < 0) noClip = getenv("BOB_NO_MODAL_CLIP") ? 1 : 0;
	fprintf(stderr, "[modal] template %dx%d DLU -> %dx%d px at (%d,%d)%s\n",
		dluW, dluH, dw, dh, ox, oy, noClip ? " (clip DISABLED)" : "");

	m_bobModalDone = 0;
	/* No answer must never mean "quit" or "act". 2 is Retry/Cancel for the three-button form and
	   "neither button" for the two-button ones, so every caller takes its do-nothing branch:
	   OnBye stays in the game, and the weather prompt leaves badweather set. */
	m_bobModalResult = 2;
	bob_modal_set_at(this, ox, oy, dw, dh);

	/* Bounded: a modal that can never be dismissed (no hosted buttons, say) must not hang the
	   game. Falling out returns the same safe answer. */
	int guard = 0;
	const int BOB_MODAL_MAX_ITERS = 200000;
	while (!m_bobModalDone && ++guard < BOB_MODAL_MAX_ITERS)
	{
		bob_modal_pump();
		/* Stand in for the window the dialog does not have: shift its DC to the dialog origin and
		   clip to the dialog rect, so OnPaint's full-sheet blit shows only this dialog's part of
		   it. Restored immediately -- the origin/clip are global to the GDI layer and leaving them
		   set would misplace whatever paints next (the S173 lesson). */
		int sox = 0, soy = 0, cx0 = 0, cy0 = 0, cx1 = 0, cy1 = 0;
		if (!noClip) {
			bob_gdi_get_setdibits_origin(&sox, &soy);
			bob_gdi_get_setdibits_clip(&cx0, &cy0, &cx1, &cy1);
			bob_gdi_setdibits_origin(ox, oy);
			bob_gdi_setdibits_clip(ox, oy, ox + dw, oy + dh);
		}
		OnPaint();                              /* the dialog's own background art */
		if (!noClip) {
			bob_gdi_setdibits_origin(sox, soy);
			bob_gdi_setdibits_clip(cx0, cy0, cx1, cy1);
		}
		bob_ole_draw_panel(this, ox, oy);       /* its buttons and message text */
		bob_gdi_present();
		/* BOB_MODAL_SHOT=<path>: capture the modal once it has settled. BOB_SHOT counts ticks
		   of the idle loop, and the idle loop is exactly what a modal suspends -- so the standard
		   capture hook can never photograph a modal dialog (MA S138 hit the same wall). */
		{ const char* ms = getenv("BOB_MODAL_SHOT");
		  if (ms && guard == 20) bob_gdi_dump_to(ms); }
	}
	if (guard >= BOB_MODAL_MAX_ITERS)
		fprintf(stderr, "[modal] dismissal never arrived after %d iterations -> Cancel\n", guard);

	bob_modal_set_at(0, 0, 0, 0, 0);
	return m_bobModalResult;
}
#endif
