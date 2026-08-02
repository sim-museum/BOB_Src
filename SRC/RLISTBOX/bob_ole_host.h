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
    int         visible = 1;     /* SP.2 (S123): runtime ShowWindow state -- the game hides
                                    off-page/disabled controls (e.g. CSQuick1's IDC_DISABLEDEMO)
                                    via CWnd::ShowWindow(SW_HIDE); hidden hosts aren't drawn. */
    virtual ~OleHost() {}
    virtual void dispatch(DISPID id, VARTYPE vtRet, void* pvRet, va_list ap) = 0;
    virtual void setprop(DISPID id, va_list ap) = 0;
    virtual void getprop(DISPID id, void* pvRet) = 0;
    virtual void draw(class CDC* pdc, int w, int h) = 0;
    virtual void applyDesignProps() {}   /* set design-time props (e.g. RStatic label caption) once ids are known */
    virtual int  onClick() { return 0; } /* interactive controls (RCombo) cycle on click; return 1 if state changed */
    virtual int  rowAtY(int /*localY*/) { return -1; } /* list controls: the row under a click (local Y), or -1 */
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

bool bob_ole_trace();   /* BOB_TRACE_OLE gate, shared */

/* standard OLE stock-property dispids (negative). */
enum { DISPID_FORECOLOR_ = -512, DISPID_BACKCOLOR_ = -501, DISPID_ENABLED_ = -514,
       DISPID_CAPTION_ = -518, DISPID_TEXT_ = -517 };

#endif
