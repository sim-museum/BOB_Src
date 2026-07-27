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
static const CLSID CLSID_RButton  = { 0x78918646, 0xa917, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
static const CLSID CLSID_REdit    = { 0x499e2be6, 0xac32, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

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
    else if (memcmp(clsid, &CLSID_RButton,  sizeof(CLSID)) == 0) { h = bob_make_rbutton(parent);  what = "CRButtonCtrl"; }
    else if (memcmp(clsid, &CLSID_REdit,    sizeof(CLSID)) == 0) { h = bob_make_redit(parent);    what = "CREditCtrl"; }
    if (!h) return FALSE;              /* other R* controls not hosted yet -> wrapper no-ops */
    h->ctrlId = (int)id; h->parentDlg = parent; h->dlgId = g_bobDlgIDD;
    h->applyDesignProps();
    hosts()[self] = h;
    if (bob_ole_trace()) fprintf(stderr, "[ole] created %s for wrapper %p id=%u (parent %p)\n", what, (void*)self, id, (void*)parent);
    return TRUE;
}

/* S124: host the dialog TEMPLATE's label statics that no DDX_Control bound.
   On Windows the dialog manager creates EVERY template item; our DDX-driven
   creation only instantiates the members a dialog binds — e.g. SMissionConfigure
   binds its 8 combos but none of its 6 RStatic labels, so the Sim-Config Mission
   tab rendered label-less. The ids come from the installed build's PE DIALOG
   templates (bob_dlg_enum_statics; empty under BOB_NO_PE_RSRC → feature off).
   Called from CDialog::Create between DoDataExchange and OnInitDialog, so
   g_bobDlgIDD is the owning dialog and OnInitDialog can already GetDlgItem them.
   The synthetic wrapper CWnds follow the existing host-lifetime pattern (hosts
   are never erased; draw/click filter by parentDlg). */
extern "C" int bob_dlg_enum_statics(int dlgId, int* ids, int maxn);
void bob_ole_host_template_statics(CWnd* dlg, int dlgId) {
    if (!dlg || dlgId <= 0) return;
    int ids[96];
    int n = bob_dlg_enum_statics(dlgId, ids, 96);
    for (int i = 0; i < n; i++) {
        if (bob_ole_find_wrapper(dlg, ids[i])) continue;     /* DDX-bound already */
        CWnd* w = new CWnd;                                   /* synthetic wrapper (GetDlgItem/ShowWindow reach it via hosts()) */
        if (!bob_ole_create_control(w, (const GUID*)&CLSID_RStatic, dlg, (UINT)ids[i])) { delete w; continue; }
        if (bob_ole_trace()) fprintf(stderr, "[ole] template static id=%d hosted for dlg IDD=%d\n", ids[i], dlgId);
    }
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
extern "C" int bob_dlg_lookup_in(int dlgId, int ctrlId, int* x, int* y, int* w, int* h);
extern "C" int bob_dlg_in_template(int dlgId, int ctrlId);   /* S124: 1 in / 0 absent / -1 unknown dialog */
static bool lookupDlu(int id, DluRect& out) {
    /* prefer the runtime .rc parser (covers every screen); fall back to the small
       hardcoded SDETAIL table if the source .rc isn't reachable. */
    if (bob_dlg_lookup(id, &out.x, &out.y, &out.w, &out.h)) { out.id = id; return true; }
    for (const DluRect& r : SDETAIL_LAYOUT) if (r.id == id) { out = r; return true; }
    return false;
}
/* dialog-scoped rect (disambiguates shared control ids like the toolbars' IDC_PAUSE) */
static bool lookupDluIn(int dlgId, int id, DluRect& out) {
    if (dlgId > 0 && bob_dlg_lookup_in(dlgId, id, &out.x, &out.y, &out.w, &out.h)) { out.id = id; return true; }
    return lookupDlu(id, out);
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
        /* SP.2 (S123): honor the game's runtime ShowWindow state -- a hidden control isn't
           drawn and can't be clicked (zeroed hit rect). E.g. CSQuick1 hides IDC_DISABLEDEMO
           ("This is disabled in the demo") on the full game. */
        if (!host->visible) { host->sw = host->sh = 0; continue; }
        /* S124: a control absent from the installed build's template for this dialog would
           never be created by the Windows dialog manager -- don't draw it (e.g. CSSound's
           source-only music combos over the BDG IDD_SSOUND layout). Dialogs the PE doesn't
           cover return -1 (no filtering). Zeroed hit rect keeps clicks consistent. */
        if (bob_dlg_in_template(host->dlgId, host->ctrlId) == 0) { host->sw = host->sh = 0; continue; }
        DluRect r;
        /* SP.2 (S123): look the rect up SCOPED to the control's own dialog template first.
           The unscoped by-id lookup returned the first id match across ALL parsed dialogs;
           combo ids are unique, but STATIC-label ids repeat across dialog templates, so config
           labels took rects from other screens' templates -- scrambled/overlapping label
           layout on the GFX/Sound/Controls/Views forms (vs the Wine gold shots). lookupDluIn
           falls back to the unscoped search when the (dlg,id) pair isn't found. */
        if (!lookupDluIn(host->dlgId, host->ctrlId, r)) continue;
        /* S126 (#16): settled-state emulation of the Windows dirty-region repaint.
           On Windows a WS_VISIBLE static under an interactive listbox paints once;
           the listbox's next repaint re-blits the panel background over it, and the
           static is never re-invalidated — so it is absent from the settled screen
           (gold #16: the date heading RStatic 1227 under CSCampaign's tab-row
           listbox). Our panel model redraws every control every frame; emulate the
           settled state by skipping a static whose template rect is >=90% covered
           by a sibling hosted listbox's rect. BOB_NO_COVER_ERASE reverts. */
        static int noCover = -1;
        if (noCover < 0) noCover = getenv("BOB_NO_COVER_ERASE") ? 1 : 0;
        if (!noCover && bob_dlg_kind(host->dlgId, host->ctrlId) == 1 /*K_RSTATIC*/) {
            int covered = 0;
            for (auto& kv2 : hosts()) {
                OleHost* lb = kv2.second;
                if (lb == host || lb->parentDlg != dialog || !lb->visible) continue;
                if (bob_dlg_kind(lb->dlgId, lb->ctrlId) != 3 /*K_RLISTBOX*/) continue;
                DluRect lr;
                if (!lookupDluIn(lb->dlgId, lb->ctrlId, lr)) continue;
                int ix = r.x > lr.x ? r.x : lr.x;
                int iy = r.y > lr.y ? r.y : lr.y;
                int ix2 = (r.x + r.w) < (lr.x + lr.w) ? (r.x + r.w) : (lr.x + lr.w);
                int iy2 = (r.y + r.h) < (lr.y + lr.h) ? (r.y + r.h) : (lr.y + lr.h);
                if (ix2 <= ix || iy2 <= iy) continue;
                long inter = (long)(ix2 - ix) * (iy2 - iy), area = (long)r.w * r.h;
                if (area > 0 && inter * 10 >= area * 9) { covered = 1; break; }
            }
            if (covered) {
                host->sw = host->sh = 0;
                if (bob_ole_trace()) fprintf(stderr,
                    "[ole] static id=%d covered by listbox re-blit -> erased (settled state)\n",
                    host->ctrlId);
                continue;
            }
        }
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

/* S89: draw a docked TOOLBAR's hosted buttons at their template rects, scaled px-per-100-DLU
   (the config-panel dluX/dluY 1.5x is too big for the 50-DLU toolbar buttons). Each CRButtonCtrl
   OnDraw blits its NormalFileNum art (SetDIBitsToDevice) at the button's screen origin -- the art
   is native-BMP-sized, so scale here just sets the button spacing/layout. */
extern "C" void bob_gdi_setdibits_origin(int, int);
/* ids/nids: if nids>0, draw only controls whose id is in ids[] (used to draw just the accel
   buttons off the TitleBar, whose DATETIME/DATE we render separately). nids==0 -> draw all. */
extern "C" int bob_ole_draw_toolbar_ids(CWnd* dialog, int ox, int oy, int pxPer100, const int* ids, int nids) {
    int n = 0;
    for (auto& kv : hosts()) {
        OleHost* host = kv.second;
        if (host->parentDlg != dialog) continue;
        if (nids > 0) { int hit = 0; for (int k = 0; k < nids; k++) if (ids[k] == host->ctrlId) { hit = 1; break; } if (!hit) continue; }
        DluRect r;
        if (!lookupDluIn(host->dlgId, host->ctrlId, r)) continue;
        int sx = ox + r.x * pxPer100 / 100, sy = oy + r.y * pxPer100 / 100;
        int w = r.w * pxPer100 / 100,       h = r.h * pxPer100 / 100;
        CDC dc; dc.m_hDC = (HDC)1; dc.m_bobScreen = true;
        dc.m_bobVpX = sx; dc.m_bobVpY = sy; dc.m_bobTextH = 12;
        bob_gdi_setdibits_origin(sx, sy);      /* button art (SetDIBitsToDevice) -> screen pos */
        host->draw(&dc, w, h);
        bob_gdi_setdibits_origin(0, 0);
        host->sx = sx; host->sy = sy; host->sw = w; host->sh = h;
        n++;
    }
    if (bob_ole_trace()) fprintf(stderr, "[ole] draw_toolbar dialog=%p off=(%d,%d) drew=%d\n", (void*)dialog, ox, oy, n);
    return n;
}
extern "C" int bob_ole_draw_toolbar(CWnd* dialog, int ox, int oy, int pxPer100) {
    return bob_ole_draw_toolbar_ids(dialog, ox, oy, pxPer100, 0, 0);
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
        if (h->parentDlg != dialog) continue;
        if (bob_ole_trace()) fprintf(stderr, "[ole]   hit? id=%d rect=(%d,%d,%d,%d) click=(%d,%d)\n", h->ctrlId, h->sx, h->sy, h->sw, h->sh, x, y);
        if (h->sw <= 0 || h->sh <= 0) continue;
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

/* SP.2 (S123): runtime visibility from CWnd::ShowWindow (afxwin.h forwards here).
   Only hosted OLE controls track state; other CWnds no-op as before. SW_HIDE==0. */
extern "C++" void bob_ole_show_window(CWnd* w, int nCmdShow) {
    OleHost* h = findHost(w);
    if (!h) return;
    int vis = (nCmdShow != 0);
    if (h->visible != vis && bob_ole_trace())
        fprintf(stderr, "[ole] ShowWindow id=%d -> %s\n", h->ctrlId, vis ? "SHOW" : "HIDE");
    h->visible = vis;
}
