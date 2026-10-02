/* Shared interface for the R* ActiveX control hosts. Each genuine control lives in
   its own TU (bob_ole_rlistbox.cpp, bob_ole_rcombo.cpp) so their conflicting
   RCOMBO/RLISTBOX "resource.h" guards don't collide; bob_ole.cpp owns the side-table
   and entry points and only sees this type-agnostic interface.
   NOTE: include AFTER the MFC/windows types (DISPID/VARTYPE/CDC/CWnd) are available. */
#ifndef BOB_OLE_HOST_H
#define BOB_OLE_HOST_H
#include <cstdarg>

struct OleHost {
    int         ctrlId = 0;      /* the dialog control id (DDX_Control) -> template lookup */
    int         dlgId  = 0;      /* the owning dialog's IDD -> (dialog,control) DLGINIT caption */
    class CWnd* parentDlg = NULL;/* owning dialog (for per-panel draw) */
    int         sx=0, sy=0, sw=0, sh=0;  /* last-drawn screen rect, for click hit-testing */
    /* S207 (§8-MA137): the height this control's paint actually COVERS, which for a list is not
       its rect. Separate from sh on purpose: sh also builds the dialog's swallow region
       (bob_ole_drawn_bounds), and widening THAT would change which clicks the map stops seeing.
       This one is read by the per-control hit test alone. <=0 -> fall back to sh. */
    int         hitH = 0;
    int         visible = 1;     /* SP.2 (S123): runtime ShowWindow state -- the game hides
                                    off-page/disabled controls (e.g. CSQuick1's IDC_DISABLEDEMO)
                                    via CWnd::ShowWindow(SW_HIDE); hidden hosts aren't drawn. */
    /* TABHEAD-1 / TOTE-1: a rect set by the game's own MoveWindow, in pixels relative to the
       dialog origin. Only honoured for hosts whose geometry is "live" (see liveGeometry): the
       rest of the port lays controls out from their templates and its GetWindowRect answers the
       whole screen, so honouring every MoveWindow the game issues would move controls with
       coordinates computed from that degenerate answer. */
    int         mvSet = 0, mvX = 0, mvY = 0, mvW = 0, mvH = 0;
    virtual int liveGeometry() { return 0; }
    virtual int isTabStrip() { return 0; }
    virtual int tabStripHeight(int /*w*/) { return 0; }
    virtual ~OleHost() {}
    virtual void dispatch(DISPID id, VARTYPE vtRet, void* pvRet, va_list ap) = 0;
    virtual void setprop(DISPID id, va_list ap) = 0;
    virtual void getprop(DISPID id, void* pvRet) = 0;
    virtual void draw(class CDC* pdc, int w, int h) = 0;
    virtual void applyDesignProps() {}   /* set design-time props (e.g. RStatic label caption) once ids are known */
    /* PO 2026-09-05: multiplayer chat -- "can't type anything in". The chat box is a genuine
       CREdit (COMMCHAT.CPP:134 GETDLGITEM(IDC_PLAYERCHAT)) and IS hosted, so it draws its caret --
       but HostREdit implemented boot/draw/setprop/getprop/dispatch and NO keyboard path at all, so
       a keystroke had nowhere to go. Note P8 was closed on the premise "BoB hosts no edit controls,
       so nothing fires it today"; this screen is the counter-example.
       wantsKeys() marks a control that should take focus on click; onKey() delivers one keystroke,
       returning 1 if it consumed it. */
    virtual int  wantsKeys() { return 0; }
    virtual int  onKey(int /*ch*/, int /*isText*/) { return 0; }
    virtual const char* keyText() { return 0; }   /* current text, for a submit event's argument */
    virtual void onFocus() {}   /* the control just gained the keyboard: run its focus-time setup */
    virtual int  onClick() { return 0; } /* interactive controls (RCombo) cycle on click; return 1 if state changed */
    /* S197: some controls need WHERE inside themselves they were clicked -- a spin button's arrows
       are the right ~15px and its up/down halves are decided by Y. onClick() has no coordinates and
       onButtonClick() has only X, so neither could serve. Offered before onClick(); return 1 if the
       control's state changed. */
    virtual int  onClickXY(int /*localX*/, int /*localY*/) { return 0; }
    /* LBSCROLL-1: a list box takes clicks on its own (runtime-created) scrollbars here, BEFORE the
       row / event paths -- a bar click must not fire Select. Returns 1 if a bar took it. */
    virtual int  onScrollbarClick(int /*localX*/, int /*localY*/) { return 0; }
    /* LBSCROLL-1: 1 while the control shows a vertical scrollbar -> its rows are clipped to the box
       and the hit area is the box (not contentH). */
    virtual int  clipsRows() { return 0; }
    /* MA-MPQS-1: a VERTICAL radio answers the local Y of button ROW, so an autoclick `#ID:0.ROW` can name it
       (the column resolver alone lands every vertical radio on its middle button). Returns 1 if handled. */
    virtual int  rowPoint(int /*row*/, int* /*localY*/) { return 0; }
    virtual int  curIndex() { return -1; } /* S161: current selection, for the event's index argument */
    virtual int  rowAtY(int /*localY*/) { return -1; } /* list controls: the row under a click (local Y), or -1 */
    /* S207, answering MA's §8-MA137: how tall is the content this control would LAY OUT, as
       opposed to the rect it is hosted in? MA found its title menu drawing 199px of rows inside a
       100px listbox while every hit test bounded clicks by the rect, so the lower rows -- one of
       them Replay -- were painted and unclickable by any route. We host the same R* listboxes.
       -1 = the control has no notion of content height (the honest answer for a button), which is
       NOT the same as 0 and must not be compared against a rect. */
    virtual int  contentH() { return -1; }
    virtual int  colAtX(int /*localX*/) { return 0; }  /* S141: list controls: the COLUMN under a click (local X) */
    virtual int  buttonCount() { return 0; }      /* QMSIDE-1: multi-button controls: how many equal-width buttons (recipe `#ID:COL` on a tab row) */
    virtual int  onButtonClick(int /*localX*/) { return -1; } /* multi-button controls (RRadio tabs): select the button at local X + return its index, or -1 */
};

extern "C" int bob_dlg_caption(int dlgId, int ctrlId, char* out, int outsz);   /* DLGINIT caption (bob_dlgtemplate.cpp) */
extern "C" int bob_dlg_artname(int dlgId, int ctrlId, char* out, int outsz);   /* DLGINIT "FIL_*" NormalFileNumString (buttons) */
extern "C" int bob_dlg_resnum(int dlgId, int ctrlId, unsigned* rn);            /* S125: persisted ResourceNumber (RButton alignment in bits 24..31) */
extern "C" int bob_dlg_columns(int dlgId, int ctrlId, short w[9], int a[9]);   /* S125: persisted RListBox column widths + align/icon codes */
extern "C" int bob_dlg_propbag(int dlgId, int ctrlId, const unsigned char** p, int* n);  /* S126: raw persisted property stream (0 under BOB_NO_PROP_STREAM) */
extern "C" int bob_dlg_kind(int dlgId, int ctrlId);                            /* S126: template control class (1=RStatic 3=RListBox ...) */

/* per-control factories (one per TU) */
OleHost* bob_make_rlistbox(class CWnd* parent);
OleHost* bob_make_rcombo(class CWnd* parent);
OleHost* bob_make_rstatic(class CWnd* parent);
OleHost* bob_make_rbutton(class CWnd* parent);
OleHost* bob_make_redit(class CWnd* parent);
OleHost* bob_make_rradio(class CWnd* parent);
OleHost* bob_make_redtbt(class CWnd* parent);
OleHost* bob_make_rscrlbar(class CWnd* parent);   /* LBSCROLL-1: list box scrollbars */
extern "C" OleHost* bob_ole_host_of(class CWnd* wrapper);
extern "C" int bob_scrlbar_rect(OleHost* h, int* x, int* y, int* w, int* hh);
extern "C" int bob_scrlbar_click(OleHost* h, int lx, int ly);
extern "C" void bob_scrlbar_set_parent(OleHost* h, class CWnd* dlg);
OleHost* bob_make_rspinbut(class CWnd* parent);   /* S142: 8th (last) R* type */
OleHost* bob_make_rtabs(class CWnd* parent);      /* TABHEAD-1: the HTabBox tab strip */

bool bob_ole_trace();   /* BOB_TRACE_OLE gate, shared */

/* standard OLE stock-property dispids (negative). */
enum { DISPID_FORECOLOR_ = -512, DISPID_BACKCOLOR_ = -501, DISPID_ENABLED_ = -514,
       DISPID_CAPTION_ = -518, DISPID_TEXT_ = -517 };

#endif
