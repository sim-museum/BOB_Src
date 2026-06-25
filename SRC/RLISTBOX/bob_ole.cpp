/* ==========================================================================
 *  bob_ole.cpp — OLE/ActiveX control hosting for the R* controls (side-table +
 *  entry points). The genuine controls live in per-control TUs (bob_ole_rlistbox.cpp,
 *  bob_ole_rcombo.cpp) behind the OleHost interface; this file is control-agnostic.
 *
 *  On Windows the front-end's config/loadout/map widgets are ActiveX controls
 *  (CRListBoxCtrl/CRComboCtrl : COleControl) hosted in dialogs; the game talks to
 *  them through thin CWnd wrappers that forward every call as OLE automation
 *  (CRListBox::AddString -> CWnd::InvokeHelper(0x38, ...)). The Linux compat used to
 *  no-op InvokeHelper. CWnd::CreateControl(clsid) (driven by DDX_Control) now routes
 *  here; we instantiate the GENUINE control and route each InvokeHelper/Get/SetProperty
 *  dispid to its real (protected) method. Game sources stay unedited.
 * ======================================================================== */
#include "stdafx.h"
#include "bob_ole_host.h"
#include <unordered_map>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>

/* Live R* list font pixel height (see afxwin.h). Shared between the stub GetDC CDC
   (Shrink/measure at populate time) and the screen CDC (OnDraw), so column widths
   and rendered glyph height agree. */
int g_bobListFontH = 18;
int g_bobDlgIDD = 0;

static int g_traceOle = -1;
bool bob_ole_trace() { if (g_traceOle < 0) g_traceOle = getenv("BOB_TRACE_OLE") ? 1 : 0; return g_traceOle; }

/* coclass CLSIDs (the wrappers' GetClsid). */
static const CLSID CLSID_RListBox = { 0x48814009, 0x65ae, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
static const CLSID CLSID_RCombo   = { 0x737cb0c9, 0xb42b, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
static const CLSID CLSID_RStatic  = { 0xc42bac3d, 0xca3c, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

/* wrapper CWnd*  ->  hosted control (type-agnostic via OleHost). */
static std::unordered_map<CWnd*, OleHost*>& hosts() {
    static std::unordered_map<CWnd*, OleHost*> m; return m;
}
static OleHost* findHost(CWnd* w) { auto& m = hosts(); auto it = m.find(w); return it == m.end() ? NULL : it->second; }

/* Reverse lookup: the wrapper CWnd* for (dialog, control id). Lets CWnd::GetDlgItem(id)
   reach the hosted control -- the config's SG2C_DISPLAY populate pass does
   GetDlgItem(IDC_CBO_x)->SetList/SetIndex to fill each combo. */
extern "C" CWnd* bob_ole_find_wrapper(CWnd* dlg, int id) {
    for (auto& kv : hosts())
        if (kv.second->parentDlg == dlg && kv.second->ctrlId == id) return kv.first;
    return NULL;
}

extern "C" BOOL bob_ole_create_control(CWnd* self, const GUID* clsid, CWnd* parent, UINT id) {
    if (!self || !clsid) return FALSE;
    if (findHost(self)) return TRUE;   /* already bound */
    OleHost* h = NULL;
    const char* what = NULL;
    if      (memcmp(clsid, &CLSID_RListBox, sizeof(CLSID)) == 0) { h = bob_make_rlistbox(parent); what = "CRListBoxCtrl"; }
    else if (memcmp(clsid, &CLSID_RCombo,   sizeof(CLSID)) == 0) { h = bob_make_rcombo(parent);   what = "CRComboCtrl"; }
    else if (memcmp(clsid, &CLSID_RStatic,  sizeof(CLSID)) == 0) { h = bob_make_rstatic(parent);  what = "CRStaticCtrl"; }
    if (!h) return FALSE;              /* other R* controls not hosted yet -> wrapper no-ops */
    h->ctrlId = (int)id; h->parentDlg = parent; h->dlgId = g_bobDlgIDD;
    h->applyDesignProps();
    hosts()[self] = h;
    if (bob_ole_trace()) fprintf(stderr, "[ole] created %s for wrapper %p id=%u (parent %p)\n", what, (void*)self, id, (void*)parent);
    return TRUE;
}

/* ---- dialog-template positioning ------------------------------------------
   The R* controls' on-screen rects come from the dialog resource template (DLU).
   Stage 1: a table for IDD_SDETAIL (the GFX-config sub-panel) from ENGLISH/MIG.RC;
   to be replaced by a runtime .rc parser. DLU rect (x,y,w,h) per control id. */
struct DluRect { int id, x, y, w, h; };
static const DluRect SDETAIL_LAYOUT[] = {
    { 1066,   1, 28, 253, 16 },  /* IDC_CBO_DISPLAYDRIVERS  */
    { 1069, 164, 56,  90, 16 },  /* IDC_CBO_RESOLUTIONS     */
    { 1070, 164, 88,  90, 16 },  /* IDC_CBO_GAMMACORRECTION */
    { 1072, 164,184,  90, 16 },  /* IDC_CBO_GROUNDSHADING   */
    { 1073, 164,216,  90, 16 },  /* IDC_CBO_ITEMSHADING     */
    { 1975, 164,152,  90, 16 },  /* IDC_CBO_AUTOFRAMERATE   */
    { 1979, 164,120,  90, 16 },  /* IDC_CBO_LOWESTFRAMERATE */
    { 1999, 164,248,  90, 16 },  /* IDC_CBO_REFLECTIONS     */
    { 2002, 164,279,  90, 16 },  /* IDC_CBO_WEATHER         */
};
extern "C" int bob_dlg_lookup(int ctrlId, int* x, int* y, int* w, int* h);
static bool lookupDlu(int id, DluRect& out) {
    /* prefer the runtime .rc parser (covers every screen); fall back to the small
       hardcoded SDETAIL table if the source .rc isn't reachable. */
    if (bob_dlg_lookup(id, &out.x, &out.y, &out.w, &out.h)) { out.id = id; return true; }
    for (const DluRect& r : SDETAIL_LAYOUT) if (r.id == id) { out = r; return true; }
    return false;
}
/* MS Sans Serif 8pt dialog base units ~ (6,13): px = DLU*base/(4 horiz, 8 vert). */
static int dluX(int d) { return d * 6 / 4; }
static int dluY(int d) { return d * 13 / 8; }

/* Draw every hosted control owned by `dialog` at its template position, offset by
   the panel's screen origin (ox,oy). Returns the count drawn. */
extern "C" int bob_ole_draw_panel(CWnd* dialog, int ox, int oy) {
    int n = 0;
    if (bob_ole_trace()) {
        fprintf(stderr, "[ole] draw_panel dialog=%p off=(%d,%d) hosts=%zu\n", (void*)dialog, ox, oy, hosts().size());
        for (auto& kv : hosts()) fprintf(stderr, "[ole]     host id=%d parentDlg=%p%s\n",
            kv.second->ctrlId, (void*)kv.second->parentDlg, kv.second->parentDlg==dialog?" <==MATCH":"");
    }
    for (auto& kv : hosts()) {
        OleHost* host = kv.second;
        if (host->parentDlg != dialog) continue;
        DluRect r;
        if (!lookupDlu(host->ctrlId, r)) continue;
        int sx = ox + dluX(r.x), sy = oy + dluY(r.y);
        int hpx = dluY(r.h);
        CDC dc; dc.m_hDC = (HDC)1; dc.m_bobScreen = true;
        dc.m_bobVpX = sx; dc.m_bobVpY = sy;
        /* R6.2: the text/font height tracked the control's BOX height (hpx-4). That's right for a
           single-line label/combo (the standard control is 16 DLU tall), but a TALL multi-line text
           control -- e.g. the campaign PhaseDescription RStatic -- then drew its font at the full
           box height -> giant overlapping text. The dialog font is one text line regardless of how
           tall the text box is: cap the font at the single-line height for clearly multi-line boxes;
           single-line controls keep their current size (no regression). */
        int oneLineBox = dluY(16);                 /* standard single-line control box (~26px) */
        int textH = hpx > 4 ? hpx - 4 : hpx;
        if (hpx > oneLineBox * 9 / 5) textH = oneLineBox - 4;   /* multi-line area -> one-line font */
        dc.m_bobTextH = textH;
        host->draw(&dc, dluX(r.w), hpx);
        host->sx = sx; host->sy = sy; host->sw = dluX(r.w); host->sh = hpx;  /* for click hit-test */
        if (bob_ole_trace()) fprintf(stderr, "[ole] draw panel ctrl id=%d at (%d,%d) %dx%d\n", host->ctrlId, sx, sy, dluX(r.w), hpx);
        n++;
    }
    return n;
}

/* A click landed at screen (x,y) over `dialog`'s panel: hit-test its hosted controls' last-drawn
   rects; an interactive control (RCombo) cycles its value (onClick). Returns 1 if a control
   consumed the click (the caller then repaints). Mirrors the MiG Alley port's ma_ole_click. */
#include <typeinfo>
extern "C" int bob_evt_fire(void* dlg, const void* tinfo, int id, int dispid);  /* S33 general OCX eventsink */
extern "C" { extern long bob_evtA0, bob_evtA1; }

extern "C" int bob_ole_click(CWnd* dialog, int x, int y) {
    if (bob_ole_trace()) {
        int match=0; for (auto& kv : hosts()) if (kv.second->parentDlg==dialog) match++;
        fprintf(stderr, "[ole] click test dialog=%p (%d,%d) hostsForDialog=%d\n", (void*)dialog, x, y, match);
    }
    for (auto& kv : hosts()) {
        OleHost* h = kv.second;
        if (h->parentDlg != dialog || h->sw <= 0 || h->sh <= 0) continue;
        if (x >= h->sx && x < h->sx + h->sw && y >= h->sy && y < h->sy + h->sh) {
            if (h->onClick()) {
                /* S33: the value already cycled (onClick); now fire the combo's TextChanged event
                   (dispid 1) on the dialog's RUNTIME type via the general eventsink so the genuine
                   handler runs (e.g. SController::OnTextChanged* applies the device/axis rebind —
                   it reads the combo's new GetIndex). Dialogs with no registered handler for this
                   (id,dispid) no-op faithfully (they persist via writeback-on-destroy). */
                bob_evt_fire((void*)dialog, &typeid(*dialog), h->ctrlId, 1);
                if (bob_ole_trace()) fprintf(stderr, "[ole] click (%d,%d) -> ctrl id=%d cycled (evt fired)\n", x, y, h->ctrlId);
                return 1;
            }
            /* S33: a click on a hosted LIST control (e.g. the load screen's file list) selects the
               row under the cursor — fire the listbox Select event (dispid 1, args row/col) via the
               general eventsink so the genuine handler runs (e.g. CLoad::OnSelectRlistboxfile sets
               selectedfile). id==0 is the FullPanelDial menu listbox (handled elsewhere) — skip. */
            int row = h->rowAtY(y - h->sy);
            if (row >= 0 && h->ctrlId) {
                bob_evtA0 = row; bob_evtA1 = 0;
                if (bob_evt_fire((void*)dialog, &typeid(*dialog), h->ctrlId, 1)) {
                    if (bob_ole_trace()) fprintf(stderr, "[ole] click (%d,%d) -> list id=%d row=%d selected (evt fired)\n", x, y, h->ctrlId, row);
                    return 1;
                }
            }
        }
    }
    return 0;
}

extern "C" void bob_ole_invoke(CWnd* self, DISPID id, WORD /*flags*/, VARTYPE vtRet, void* pvRet, const BYTE* /*pInfo*/, va_list ap) {
    OleHost* h = findHost(self); if (h) h->dispatch(id, vtRet, pvRet, ap);
}
extern "C" void bob_ole_setprop(CWnd* self, DISPID id, VARTYPE /*vt*/, va_list ap) {
    OleHost* h = findHost(self); if (h) h->setprop(id, ap);
}
extern "C" void bob_ole_getprop(CWnd* self, DISPID id, VARTYPE /*vt*/, void* pvRet) {
    OleHost* h = findHost(self); if (h) h->getprop(id, pvRet);
}

/* Drive the genuine control's OnDraw onto the front-end framebuffer at screen (x,y),
   size (w,h). Returns 1 if a control was found & drawn. */
extern "C" int bob_ole_draw_listbox(CWnd* wrapper, int x, int y, int w, int h, int textH) {
    OleHost* host = findHost(wrapper); if (!host) return 0;
    CDC dc;
    dc.m_hDC = (HDC)1;
    dc.m_bobScreen = true;
    dc.m_bobVpX = x; dc.m_bobVpY = y;
    dc.m_bobTextH = textH > 0 ? textH : 14;
    host->draw(&dc, w, h);
    if (bob_ole_trace()) fprintf(stderr, "[ole] OnDraw %p at (%d,%d) %dx%d h=%d\n", (void*)wrapper, x, y, w, h, textH);
    return 1;
}
