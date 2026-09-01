/* ==========================================================================
 *  bob_ole.cpp ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ OLE/ActiveX control hosting for the R* controls (side-table +
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
#include <set>
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
static const CLSID CLSID_RRadio   = { 0x5363ba22, 0xd90a, 0x11d6, { 0xa1,0xf0,0x00,0x80,0xc8,0x58,0x2d,0xe4 } };
static const CLSID CLSID_REdtBt   = { 0x461a1fe3, 0xb81b, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };  /* S140: CREdtBt pilot slots (BoBFrag) */
static const CLSID CLSID_RSpinBut = { 0xc3270e66, 0x6d6b, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };  /* S142: CRSpinBut ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ the LW Directives allocation grid (gold #18); 8th and LAST R* type */

/* wrapper CWnd*  ->  hosted control (type-agnostic via OleHost). */
static std::unordered_map<CWnd*, OleHost*>& hosts() {
    static std::unordered_map<CWnd*, OleHost*> m; return m;
}
static OleHost* findHost(CWnd* w) { auto& m = hosts(); auto it = m.find(w); return it == m.end() ? NULL : it->second; }
/* R13 (2026-08-29): expose the control id the REGISTRY knows. RCOMBO's GetIndex trace tried
   GetDlgCtrlID() and got 0 on every line -- these are OLE-hosted controls with no dialog child id
   at that point, so the trace stayed anonymous and a red gate could not be diagnosed from its own
   log. The host record has carried `ctrlId` since creation (:105); this just lets the wrapper ask.
   Returns -1 when the wrapper is not hosted, which is itself worth seeing. */
extern "C" int bob_ole_ctrlid(void* wrapper) {
    OleHost* h = findHost((CWnd*)wrapper);
    return h ? h->ctrlId : -1;
}

/* Reverse lookup: the wrapper CWnd* for (dialog, control id). Lets CWnd::GetDlgItem(id)
   reach the hosted control -- the config's SG2C_DISPLAY populate pass does
   GetDlgItem(IDC_CBO_x)->SetList/SetIndex to fill each combo. */
extern "C" CWnd* bob_ole_find_wrapper(CWnd* dlg, int id) {
    /* S199 (answering MA note MA117): hosts are never erased -- see the lifetime comment
       below -- so a dialog that is destroyed and reopened leaves its old entries behind with
       a dangling parentDlg. That is harmless while the new dialog gets a NEW address, and a
       collision the moment the allocator RECYCLES the old one: two hosts would then match the
       same (dlg,id) and this function would return whichever the unordered_map happens to
       iterate first -- i.e. by hash order. MiG Alley hit exactly this (its S171): two live
       copies of one dialog, and the id resolver picked the DEAD one, so two clicks naming one
       control reached two different controls.
       Whether it happens here is a MEASUREMENT, not a theory -- so measure it. Counting is
       cheap next to the draw walk, and a silent wrong-control is exactly the failure this
       port keeps rediscovering. */
    CWnd* found = NULL;
    int n = 0;
    for (auto& kv : hosts())
        if (kv.second->parentDlg == dlg && kv.second->ctrlId == id) { if (!n) found = kv.first; n++; }
    if (n > 1) {
        static int warned = 0;
        if (warned < 20) {
            warned++;
            fprintf(stderr, "[ole] WARNING dlg=%p id=%d matches %d hosts -- a reopened dialog "
                            "reused a dead one's address; the wrapper returned is hash-order "
                            "luck (see MA117)\n", (void*)dlg, id, n);
        }
    }
    return found;
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
    else if (memcmp(clsid, &CLSID_RRadio,   sizeof(CLSID)) == 0) { h = bob_make_rradio(parent);   what = "CRRadioCtrl"; }
    else if (memcmp(clsid, &CLSID_REdtBt,   sizeof(CLSID)) == 0) { h = bob_make_redtbt(parent);   what = "CREdtBtCtrl"; }
    else if (memcmp(clsid, &CLSID_RSpinBut, sizeof(CLSID)) == 0) { h = bob_make_rspinbut(parent); what = "CRSpinButCtrl"; }
    if (!h) {
        /* S170 (MA's census method, S136/S140): an UNHOSTED control type is silent -- the wrapper
           becomes a no-op and the dialog simply lacks that widget, which reads as "the game does
           not have one" rather than "the port did not build it". MA found two whole types this way
           (RRadio, RScrlBar), each of which had been quietly discarding every call the game made.
           Report each distinct CLSID once, uncapped: a per-run budget would be spent by whatever
           screen loads first. */
        static std::set<unsigned long> seen;
        unsigned long d1 = clsid ? (unsigned long)clsid->Data1 : 0;
        if (seen.insert(d1).second)
            fprintf(stderr, "[ole] UNHOSTED control clsid.Data1=%08lx (id=%u, parent=%p) -- wrapper is a no-op\n",
                    d1, id, (void*)parent);
        return FALSE;
    }
    h->ctrlId = (int)id; h->parentDlg = parent; h->dlgId = g_bobDlgIDD;
    h->applyDesignProps();
    hosts()[self] = h;
    if (bob_ole_trace()) fprintf(stderr, "[ole] created %s for wrapper %p id=%u (parent %p)\n", what, (void*)self, id, (void*)parent);
    return TRUE;
}

/* S124: host the dialog TEMPLATE's label statics that no DDX_Control bound.
   On Windows the dialog manager creates EVERY template item; our DDX-driven
   creation only instantiates the members a dialog binds ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ e.g. SMissionConfigure
   binds its 8 combos but none of its 6 RStatic labels, so the Sim-Config Mission
   tab rendered label-less. The ids come from the installed build's PE DIALOG
   templates (bob_dlg_enum_statics; empty under BOB_NO_PE_RSRC ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ feature off).
   Called from CDialog::Create between DoDataExchange and OnInitDialog, so
   g_bobDlgIDD is the owning dialog and OnInitDialog can already GetDlgItem them.
   The synthetic wrapper CWnds follow the existing host-lifetime pattern (hosts
   are never erased; draw/click filter by parentDlg). */
extern "C" int bob_dlg_enum_statics(int dlgId, int* ids, int maxn);
extern "C" int bob_dlg_enum_buttons(int dlgId, int* ids, int maxn);
extern "C" int bob_dlg_enum_combos(int dlgId, int* ids, int maxn);   /* S176 */
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
    /* S136: same for template BUTTONS the dialog never DDX-binds (e.g. IDC_RETURNTOPLAYER on
       IDD_BOBFRAG, gold #3). bob_make_rbutton already renders a hosted RButton's art+caption
       (verified by the DDX-bound tickbox), so a template-created one draws the same way.
       BOB_NO_TEMPLATE_BUTTONS reverts. */
    static int noBtn = -1; if (noBtn < 0) noBtn = getenv("BOB_NO_TEMPLATE_BUTTONS") ? 1 : 0;
    if (noBtn) return;
    n = bob_dlg_enum_buttons(dlgId, ids, 96);
    for (int i = 0; i < n; i++) {
        if (bob_ole_find_wrapper(dlg, ids[i])) continue;     /* DDX-bound already */
        CWnd* w = new CWnd;
        if (!bob_ole_create_control(w, (const GUID*)&CLSID_RButton, dlg, (UINT)ids[i])) { delete w; continue; }
        if (bob_ole_trace()) fprintf(stderr, "[ole] template button id=%d hosted for dlg IDD=%d\n", ids[i], dlgId);
    }
    /* S176: and the template COMBOS the game never binds -- e.g. the GFX screen's Campaign
       Resolution, which is in the installed template but absent from this source drop's .rc and
       from the dialog class, so nothing created it and the row rendered as a label with no
       dropdown. BOB_NO_TEMPLATE_COMBOS reverts. */
    static int noCbo = -1; if (noCbo < 0) noCbo = getenv("BOB_NO_TEMPLATE_COMBOS") ? 1 : 0;
    if (!noCbo) {
        n = bob_dlg_enum_combos(dlgId, ids, 96);
        for (int i = 0; i < n; i++) {
            if (bob_ole_find_wrapper(dlg, ids[i])) continue;     /* DDX-bound already */
            CWnd* w = new CWnd;
            if (!bob_ole_create_control(w, (const GUID*)&CLSID_RCombo, dlg, (UINT)ids[i])) { delete w; continue; }
            if (bob_ole_trace()) fprintf(stderr, "[ole] template combo id=%d hosted for dlg IDD=%d\n", ids[i], dlgId);
        }
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
/* R3.8 (S344): NAME the skipped controls, do not only count them. The R12 line reports totals
   per dialog -- "DREW=156 ... not-in-template=17" -- which proves controls are being dropped but
   not WHICH, and R3.8's literal ask is for the ctrlIds. Counting told us there is a problem;
   only the ids can tell us whether it matters (a decorative row versus a button the player needs).
   One line per (dialog, ctrl, reason), printed once each, so a per-frame draw path cannot flood
   the log. */
static void bob_skip_name(int dlgId, int ctrlId, const char* why)
{
    if (!getenv("BOB_TRACE_SKIP")) return;
    struct Seen { int d, c; const char* w; };
    static Seen seen[512]; static int nSeen = 0;
    for (int i = 0; i < nSeen; i++)
        if (seen[i].d == dlgId && seen[i].c == ctrlId && seen[i].w == why) return;
    if (nSeen < 512) { seen[nSeen].d = dlgId; seen[nSeen].c = ctrlId; seen[nSeen].w = why; nSeen++; }
    fprintf(stderr, "[skipid] dlgId=%d ctrl=%d %s\n", dlgId, ctrlId, why);
    fflush(stderr);
}
static int dluY(int d) { return d * 13 / 8; }

/* Draw every hosted control owned by `dialog` at its template position, offset by
   the panel's screen origin (ox,oy). Returns the count drawn. */
extern "C" void bob_gdi_setdibits_clip(int, int, int, int);
extern "C" void bob_gdi_get_setdibits_clip(int*, int*, int*, int*);
/* R21 (S395): the TEXT clip, which the art clip above does not cover. See bob_gdi_font.cpp. */
extern "C" void bob_gdi_text_clip(int, int, int, int);
extern "C" void bob_gdi_get_text_clip(int*, int*, int*, int*);
int g_bobFontHCtrl = 0, g_bobFontHDlg = 0;   /* S185 measurement only */
/* S191 (BOB_TRACE_CENSUS, default-off): after a dialog is built, list the template controls that
   got NO host, by control type.

   The port has found missing template control kinds three times, each after a player noticed the
   consequence: S124 (statics), S136 (buttons), S176 (combos -- the PO's "the campaign resolution
   dropdown is missing"). Each fix added another `bob_ole_host_template_<kind>` and left the next
   kind to be discovered the same way. The templates use NINE R* types; nothing enumerated the
   other six, and nothing ever said so, because an unhosted control is silent by construction --
   it simply is not drawn and cannot be clicked, which looks like a design with fewer widgets.

   This asks the question directly and for every kind at once, so the remaining gaps come from a
   census rather than from a bug report. */
extern "C" int bob_dlg_enum_all(int dlgId, int* ids, int* kinds, int maxn);
extern "C" const char* bob_dlg_kind_name(int k);
extern "C" void bob_ole_census(int dlgId)
{
    if (!getenv("BOB_TRACE_CENSUS")) return;
    static int seen[256]; static int nseen = 0;
    for (int i = 0; i < nseen; i++) if (seen[i] == dlgId) return;   /* one report per dialog */
    if (nseen < 256) seen[nseen++] = dlgId;

    int ids[256], kinds[256];
    int n = bob_dlg_enum_all(dlgId, ids, kinds, 256);
    if (n <= 0) return;
    int missByKind[16]; for (int k = 0; k < 16; k++) missByKind[k] = 0;
    int hosted = 0, missing = 0;
    for (int i = 0; i < n; i++) {
        bool have = false;
        for (auto& kv : hosts())
            if (kv.second->ctrlId == ids[i] && kv.second->dlgId == dlgId) { have = true; break; }
        if (have) hosted++;
        else { missing++; int k = kinds[i]; if (k < 0 || k > 15) k = 0; missByKind[k]++; }
    }
    fprintf(stderr, "[census] dlg IDD=%d: %d template controls, %d hosted, %d MISSING",
            dlgId, n, hosted, missing);
    if (missing) {
        fprintf(stderr, " {");
        for (int k = 0; k < 16; k++) if (missByKind[k])
            fprintf(stderr, " %s=%d", bob_dlg_kind_name(k), missByKind[k]);
        fprintf(stderr, " }");
    }
    fprintf(stderr, "\n");
}

extern "C" int bob_ole_draw_panel(CWnd* dialog, int ox, int oy) {
    int n = 0;
    /* R12 (cross-port from MA S329-S2, 2026-08-29). In MA the PO's "empty variants screen" was a
       control type that was created, hosted, classified and populated -- and then had NO BRANCH in
       the main draw dispatcher, so it was never drawn and NOTHING SAID SO. The generalisable lesson
       is that a control skipped SILENTLY is indistinguishable from one that does not exist, and
       this function has several distinct skip paths.
       BOB_TRACE_SKIP=1 reports, once per dialog, how many hosted controls each path dropped. A
       screen that "shows nothing" can then be read directly rather than narrowed by elimination
       over several sprints, which is what R3.8 has cost so far. */
    int skipVis = 0, skipDead = 0, skipTmpl = 0, skipDlu = 0, drawn = 0;
    /* R3.8: "zero hosted controls" and "hosted under a DIFFERENT parent" look identical from inside
       the match loop below, because it only ever prints hosts that already matched. That ambiguity
       is the whole question for the briefing pane (never populated vs populated and not drawn), so
       report the registry TOTAL alongside the match count, once per dialog. */
    if (getenv("BOB_TRACE_SYSBOX")) {
        static const void* seenDlg[64]; static int nSeen = 0; int already = 0;
        for (int k = 0; k < nSeen; k++) if (seenDlg[k] == (void*)dialog) { already = 1; break; }
        if (!already && nSeen < 64) {
            seenDlg[nSeen++] = (void*)dialog;
            int match = 0, firstDlgId = -1;
            for (auto& kv : hosts()) if (kv.second->parentDlg == dialog) {
                match++; if (firstDlgId < 0) firstDlgId = kv.second->dlgId; }
            fprintf(stderr, "[panel] dialog=%p dlgId=%d registry=%zu matched=%d\n",
                    (void*)dialog, firstDlgId, hosts().size(), match);
        }
    }
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
        if (!host->visible) { host->sw = host->sh = 0; skipVis++; bob_skip_name(host->dlgId, host->ctrlId, "not-visible"); continue; }
        /* S124: a control absent from the installed build's template for this dialog would
           never be created by the Windows dialog manager -- don't draw it (e.g. CSSound's
           source-only music combos over the BDG IDD_SSOUND layout). Dialogs the PE doesn't
           cover return -1 (no filtering). Zeroed hit rect keeps clicks consistent. */
        /* S150 (SP.8, gold #18): the LW Directives grid draws a SEVENTH target row that gold shows
           nowhere, overprinting the "Ground Attack Gruppen"/"Escort Gruppen" headers. Measured, not
           guessed: a per-control dump (BOB_TRACE_DIR=<dlg>) put the six real rows at y=317..447 with
           12 controls each and an extra 11-control row at y=229 whose ids are the ...7 members of the
           same families -- i.e. index 7 of FillTargetLists' `for (i=1;i<8;i++)`, which is
           SWEEPSNDECOYS. The game marks that branch DEAD (`INT3; //This should not happen. Patrols
           removed.` LWDIRECT.CPP:1626): the feature was cut, its logic is unreachable, but its
           controls stayed in the BDG template so our DDX/template hosting still draws them. Skip
           them -- the shipped game has no fighter-sweep line, which is exactly what gold shows.
           BOB_NO_SWEEPROW_SKIP reverts. (S146's IDC_FIGHTERSWEEP* guess was wrong; these are the
           real ids.) */
        static int noSweepSkip = -1;
        if (noSweepSkip < 0) noSweepSkip = getenv("BOB_NO_SWEEPROW_SKIP") ? 1 : 0;
        if (!noSweepSkip && host->dlgId == 1032) {
            static const int deadSweepRow[] = { 1468, 2227, 2247, 2267, 2277, 2287,
                                                2297, 2307, 2327, 2337, 2377 };
            bool dead = false;
            for (unsigned k = 0; k < sizeof(deadSweepRow)/sizeof(deadSweepRow[0]); k++)
                if (host->ctrlId == deadSweepRow[k]) { dead = true; break; }
            if (dead) { host->sw = host->sh = 0; skipDead++; bob_skip_name(host->dlgId, host->ctrlId, "dead-sweep-row"); continue; }
        }
        if (bob_dlg_in_template(host->dlgId, host->ctrlId) == 0) { host->sw = host->sh = 0; skipTmpl++; bob_skip_name(host->dlgId, host->ctrlId, "not-in-template"); continue; }
        /* R3.6: name the controls the sysbox actually hosts. BOB_TRACE_OLE dumps every control of
           every dialog per frame (85 spinners once wrote 70 MB and starved a run), so it cannot be
           used to answer a question about ONE panel. This prints one line per control for one
           dialog. The PO reports two CAMPAIGN icons where the "X" exit icon belongs, with the
           control's position and behaviour both correct -- so the question is which id is drawing
           what, not where anything sits. */
        if (getenv("BOB_TRACE_SYSBOX")) {
            /* R3.8: the dedup key MUST include dlgId, and the cap must not silently swallow.
               The first cut keyed on ctrlId alone with a 64-entry table and simply stopped
               recording once full -- the directives grid contributes 49 ids by itself, so later
               dialogs printed NOTHING and the briefing pane looked like it hosted zero controls.
               It hosts 22. A trace that goes quiet when it runs out of room reports "absent" for
               "I stopped looking". */
            static long seen[1024]; static int nseen = 0; static int warned = 0;
            long key = (long)host->dlgId * 100000L + host->ctrlId;
            int already = 0;
            for (int k = 0; k < nseen; k++) if (seen[k] == key) { already = 1; break; }
            if (!already && nseen >= 1024 && !warned) {
                warned = 1; fprintf(stderr, "[sysbox-ctl] TABLE FULL -- further controls NOT listed\n"); }
            if (!already && nseen < 1024) { seen[nseen++] = key;
                DluRect rr;
                int haveR = lookupDluIn(host->dlgId, host->ctrlId, rr);
                fprintf(stderr, "[sysbox-ctl] dlgId=%d ctrlId=%d visible=%d dlu=(%d,%d,%d,%d)%s\n",
                        host->dlgId, host->ctrlId, host->visible ? 1 : 0,
                        haveR ? rr.x : -1, haveR ? rr.y : -1, haveR ? rr.w : -1, haveR ? rr.h : -1,
                        haveR ? "" : " NO-RECT"); }
        }
        DluRect r;
        /* SP.2 (S123): look the rect up SCOPED to the control's own dialog template first.
           The unscoped by-id lookup returned the first id match across ALL parsed dialogs;
           combo ids are unique, but STATIC-label ids repeat across dialog templates, so config
           labels took rects from other screens' templates -- scrambled/overlapping label
           layout on the GFX/Sound/Controls/Views forms (vs the Wine gold shots). lookupDluIn
           falls back to the unscoped search when the (dlg,id) pair isn't found. */
        if (!lookupDluIn(host->dlgId, host->ctrlId, r)) { skipDlu++; bob_skip_name(host->dlgId, host->ctrlId, "no-DLU-rect"); continue; }
        /* S126 (#16): settled-state emulation of the Windows dirty-region repaint.
           On Windows a WS_VISIBLE static under an interactive listbox paints once;
           the listbox's next repaint re-blits the panel background over it, and the
           static is never re-invalidated ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ so it is absent from the settled screen
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
        g_bobFontHCtrl = host->ctrlId; g_bobFontHDlg = host->dlgId;   /* S185: name the control in [fonth] */
        /* S173: clip this control's art blit to its own rect.
           RBUTTONC.CPP OnDraw blits the shared PANEL artwork into the control's DC offset by
           (parentrect.left-rect.left) and lets the control's WINDOW clip it, so each control shows
           only the piece of one image that lands in its rect. We have no windows, so the art stayed
           panel-aligned (the caller's origin, correct) but unclipped -- each control painted the
           whole console over its neighbours and the clock panel came out as a single grey slab.
           Clip, restore, and leave the ORIGIN alone: panel-aligned is what composes the image. */
        int clipSave[4];
        bob_gdi_get_setdibits_clip(&clipSave[0], &clipSave[1], &clipSave[2], &clipSave[3]);
        bob_gdi_setdibits_clip(sx, sy, sx + dluX(r.w), sy + hpx);
        /* R21 (S395): clip the ROW TEXT to the same rect. BOB_CLIP_ROWS=1, default OFF.
           Default off on purpose: S207 deliberately made overflow rows CLICKABLE (it widened the
           hit test to contentH) because MA's title menu laid out 199 px of rows in a 100 px box and
           its lower rows -- Replay among them -- answered no click. Clipping without deciding what
           an over-long list should DO would make those rows invisible as well as unreachable, which
           trades a cosmetic defect for a functional one. The PO's Messages log wants scrolling or
           paging; a 5-row menu wants a correctly sized box. Same symptom, different fixes.
           So this switch exists to MEASURE the cosmetic half in isolation, not to ship a fix. */
        int txSave[4] = {0,0,0,0};
        const bool clipRows = getenv("BOB_CLIP_ROWS") != NULL;
        if (clipRows) {
            bob_gdi_get_text_clip(&txSave[0], &txSave[1], &txSave[2], &txSave[3]);
            bob_gdi_text_clip(sx, sy, sx + dluX(r.w), sy + hpx);
        }
        host->draw(&dc, dluX(r.w), hpx);
        bob_gdi_setdibits_clip(clipSave[0], clipSave[1], clipSave[2], clipSave[3]);
        if (clipRows) bob_gdi_text_clip(txSave[0], txSave[1], txSave[2], txSave[3]);
        host->sx = sx; host->sy = sy; host->sw = dluX(r.w); host->sh = hpx;  /* for click hit-test */
        /* S207, answering MA's ÃÂÃÂÃÂÃÂ§8-MA137: does anything we host LAY OUT more content than the rect
           we hit-test? MA's title menu drew 199px of rows inside a 100px listbox and its lower
           rows -- Replay among them -- could not be clicked by any route. We clip the art blit to
           the rect (S173), but that clip is on the SetDIBitsToDevice path and says nothing about
           row TEXT, so reading the code was not going to answer this. Measure it.
           FILTERED, not capped: only an actual overflow prints, so a clean tree is silent and a
           dirty one cannot be starved by whatever draws first (ÃÂÃÂÃÂÃÂ§8-MA83). Deduped per control. */
        {
            int ch = host->contentH();
            /* S207: hit-test what paint covered. MA's ÃÂÃÂÃÂÃÂ§8-MA137, measured true here: IDD_BOBFRAG's
               IDC_RLIST_UNITDETAILS lays out 162px of rows in a 139px box, so its bottom rows were
               painted and refused every click. Widening the per-control hit test cannot move a
               pixel -- it only makes drawn rows answer. BOB_NO_DRAWH=1 reverts. */
            host->hitH = (ch > hpx && !getenv("BOB_NO_DRAWH")) ? ch : hpx;
            if (ch > 0 && ch > hpx) {
                static int seen[64]; static int nseen = 0;
                int key = host->dlgId * 10000 + host->ctrlId, dup = 0;
                for (int k = 0; k < nseen; k++) if (seen[k] == key) { dup = 1; break; }
                if (!dup && nseen < 64) {
                    seen[nseen++] = key;
                    /* Say it in ROWS, not pixels: "23px overflow" is not actionable, "row 6 of 7
                       could not be clicked" is. Probe the control's own rowAtY, which is what both
                       the hit test and the recipe resolver use. */
                    int lastInRect = -1, lastInContent = -1;
                    for (int t = 0; t < ch; t++) {
                        int rr = host->rowAtY(t);
                        if (rr < 0) continue;
                        if (rr > lastInContent) lastInContent = rr;
                        if (t < hpx && rr > lastInRect) lastInRect = rr;
                    }
                    fprintf(stderr, "[extent] OVERFLOW dlg=%d ctrl=%d rect=(%d,%d %dx%d) "
                                    "contentH=%d (+%dpx) -- rows 0..%d fit the rect, content has "
                                    "0..%d, so row(s) %d..%d were UNCLICKABLE; hit-tested to %d\n",
                            host->dlgId, host->ctrlId, sx, sy, dluX(r.w), hpx, ch, ch - hpx,
                            lastInRect, lastInContent, lastInRect + 1, lastInContent,
                            host->hitH);
                    fflush(stderr);
                }
            }
        }
        if (bob_ole_trace()) fprintf(stderr, "[ole] draw panel ctrl id=%d at (%d,%d) %dx%d\n", host->ctrlId, sx, sy, dluX(r.w), hpx);
        /* R1 (S346): ENUMERATE A MENU WITHOUT CLICKING IT.
           R1 is blocked on "enumerate 27917's menu": leaving Sim Config lands on artnum 27917
           rather than the main menu, index 0 there is the strategic map, and that segfaults -- so
           the recipe needs to know which ctrlId and which ROW to aim at. BOB_DUMP_HITTARGETS
           cannot answer it: bob_ole_col_rect only prints when it is CALLED, and it is called from
           the click path, so a run that has not clicked yet produces nothing. That is a
           chicken-and-egg, not a missing feature.
           The host already knows its own shape -- rowAtY/colAtX/contentH are the same functions
           the hit test uses -- so read it at DRAW time, which happens whether or not anyone
           clicks. Deduped per (dlg,ctrl) so a per-frame path cannot flood the log. */
        if (getenv("BOB_DUMP_MENU")) {
            static int seenD[256], seenC[256]; static int nS = 0; int already = 0;
            for (int k = 0; k < nS; k++) if (seenD[k]==host->dlgId && seenC[k]==host->ctrlId) { already=1; break; }
            if (!already && nS < 256) {
                seenD[nS]=host->dlgId; seenC[nS]=host->ctrlId; nS++;
                int hh = host->hitH > 0 ? host->hitH : host->sh;
                int nrows = 0, lastRow = -999;
                for (int ty = 0; ty < hh; ty++) { int rw = host->rowAtY(ty);
                    if (rw >= 0 && rw != lastRow) { nrows++; lastRow = rw; } }
                int ncols = 0, lastCol = -999;
                for (int tx = 0; tx < host->sw; tx++) { int cl = host->colAtX(tx);
                    if (cl != lastCol) { ncols++; lastCol = cl; } }
                fprintf(stderr, "[menu] dlgId=%d ctrl=%d rect=(%d,%d %dx%d) hitH=%d contentH=%d "
                                "rows=%d cols=%d curIndex=%d\n",
                        host->dlgId, host->ctrlId, host->sx, host->sy, host->sw, host->sh,
                        host->hitH, host->contentH(), nrows, ncols, host->curIndex());
                fflush(stderr);
            }
        }
        drawn++;
        n++;
    }
    /* R12: one line per dialog saying what was dropped and by which filter. Reports even when
       everything drew -- a report that only appears on failure cannot prove it was running, which
       is the mistake MA's PO-82 instrument made twice. */
    if (getenv("BOB_TRACE_SKIP")) {
        static const void* seen[64]; static int nSeen = 0; int already = 0;
        for (int k = 0; k < nSeen; k++) if (seen[k] == (void*)dialog) { already = 1; break; }
        if (!already && nSeen < 64) {
            seen[nSeen++] = (void*)dialog;
            int total = drawn + skipVis + skipDead + skipTmpl + skipDlu;
            /* R3.8 (2026-08-29): print the dlgId, not just the pointer. Without it the report
               says a dialog is healthy but not WHICH dialog, so it cannot answer "was the
               campaign/briefing screen among them?" -- which is the only question R3.8 is asking.
               The host record has carried dlgId since creation; the [panel] trace already prints
               it, so the skip line was the odd one out. */
            int firstDlg = -1;
            for (auto& kv2 : hosts()) if (kv2.second->parentDlg == dialog) { firstDlg = kv2.second->dlgId; break; }
            fprintf(stderr, "[skip] dlgId=%d dialog=%p hosted-for-this-dialog=%d DREW=%d | "
                            "not-visible=%d dead-sweep-row=%d not-in-template=%d no-DLU-rect=%d%s\n",
                    firstDlg, (void*)dialog, total, drawn, skipVis, skipDead, skipTmpl, skipDlu,
                    (drawn == 0 && total > 0) ? "   <-- EVERY control was skipped: this screen is blank BY FILTER" : "");
            fflush(stderr);
        }
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
        /* S173: clip to the button rect, as the control's window did on Windows. Icon faces are
           button-sized so this is a no-op for them, but a control whose art is a PANEL-sized file
           (the clock row's controls all name FIL_TELEBACK, a 500x500 plate) otherwise paints over
           everything to its right. Same pair as in bob_ole_draw_panel. */
        int clipSave[4];
        bob_gdi_get_setdibits_clip(&clipSave[0], &clipSave[1], &clipSave[2], &clipSave[3]);
        bob_gdi_setdibits_clip(sx, sy, sx + w, sy + h);
        host->draw(&dc, w, h);
        bob_gdi_setdibits_clip(clipSave[0], clipSave[1], clipSave[2], clipSave[3]);
        bob_gdi_setdibits_origin(0, 0);
        host->sx = sx; host->sy = sy; host->sw = w; host->sh = h;
        { int ch = host->contentH();                       /* S207: see OleHost::hitH */
          host->hitH = (ch > h && !getenv("BOB_NO_DRAWH")) ? ch : h; }
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
/* S156: id of the control that last consumed a click. The full per-control hit trace lives
   behind BOB_TRACE_OLE, but that fires per-control-per-frame at draw time (85 spinners once
   wrote a 70 MB log and starved a run past its timeout), so it is unusable for answering the
   one question a click test asks: WHICH control took it. This costs one int per click. */
extern "C" int bob_ole_last_click_id = 0;

/* S160: the DRAWN screen span of one column of a hosted list control, from the control's own
   GetColFromX walk ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ the same source bob_ole_click hit-tests with.

   Why: the front-end menu row (Back / Fly / Sim Config ...) is drawn by the hosted
   CRListBoxCtrl at its OWN internal column spacing (PositionRListBox / m_horzSeperation), but
   its click rects were built separately from bob_gdi_text_width() re-measurements packed with a
   fixed gap. Two independent layouts, and they drift: measured on Quick Mission, "Fly" is painted
   at x=132..153 while its click rect sat at x=83..108 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ no overlap at all, so the button was
   unclickable, and "Back" (painted 56..97, rect 25..69) only responded on its leftmost 14px.
   Same principle as S156's OOB hit-testing and MA's ÃÂÃÂÃÂÃÂ§8-MA84 trap #1: store what paint did,
   never re-derive it. Returns 1 and fills the rect if the column is drawn. */
extern "C" int bob_ole_col_rect(CWnd* ctrl, int col, int* x, int* y, int* w, int* h) {
    bool diag = getenv("BOB_DUMP_HITTARGETS") != 0;
    auto it = hosts().find(ctrl);
    if (it == hosts().end()) {
        if (diag) fprintf(stderr, "[colrect] ctrl=%p col=%d -- NO HOST for this CWnd (hosts=%d)\n",
                          (void*)ctrl, col, (int)hosts().size());
        return 0;
    }
    OleHost* ho = it->second;
    if (!ho || ho->sw <= 0 || ho->sh <= 0) {
        if (diag) fprintf(stderr, "[colrect] ctrl=%p col=%d -- host found but NOT DRAWN rect=(%d,%d %dx%d)\n",
                          (void*)ctrl, col, ho?ho->sx:0, ho?ho->sy:0, ho?ho->sw:0, ho?ho->sh:0);
        return 0;
    }
    int start = -1, end = -1;
    for (int t = 0; t < ho->sw; t++) {
        if (ho->colAtX(t) == col) { if (start < 0) start = t; end = t; }
        else if (start >= 0) break;
    }
    if (start < 0) {
        if (diag) fprintf(stderr, "[colrect] ctrl=%p col=%d -- host drawn (%d,%d %dx%d) but NO SUCH COLUMN "
                          "(colAtX(0)=%d colAtX(w-1)=%d)\n", (void*)ctrl, col,
                          ho->sx, ho->sy, ho->sw, ho->sh, ho->colAtX(0), ho->colAtX(ho->sw-1));
        return 0;
    }
    if (x) *x = ho->sx + start;
    if (y) *y = ho->sy;
    if (w) *w = end - start + 1;
    if (h) *h = ho->sh;
    return 1;
}

/* S160: list every hosted control's LAST-DRAWN screen rect for one dialog ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ i.e. exactly the
   rects bob_ole_click hit-tests against. Used to answer "is there any clickable target under
   this button's pixels?", which distinguishes a click that never arrives from one that arrives
   and finds no handler. */
/* S173d: the union of a dialog's last-DRAWN control rects, or 0 if it drew nothing hit-testable.
   Exists because CWnd::GetWindowRect in this compat answers with the WHOLE SCREEN for every window
   (afxwin.h: left=top=0, right/bottom = bob_gdi_screen_size). Any caller asking "is this click
   inside that dialog?" therefore gets "yes" for every pixel ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ which turned S156's
   swallow-clicks-that-land-on-an-open-dialog rule into swallow-EVERY-click-once-anything-is-open.
   Same principle as the hit-testing next door: the rect comes from the paint, so it cannot drift
   from what the player sees. */
/* S174: the last-DRAWN screen rect of one named control on a dialog. Used to place the title
   band's ?/tick/X glyphs from IDJ_TITLE's actual drawn extent, so they cannot drift from the band
   they sit on -- the same paint-records-its-own-geometry rule the hit-testing already follows. */
extern "C" int bob_ole_drawn_rect(CWnd* dialog, int ctrlId, int* x, int* y, int* w, int* h) {
    for (auto& kv : hosts()) {
        OleHost* ho = kv.second;
        if (ho->parentDlg != dialog || ho->ctrlId != ctrlId) continue;
        if (ho->sw <= 0 || ho->sh <= 0) return 0;
        if (x) *x = ho->sx; if (y) *y = ho->sy; if (w) *w = ho->sw; if (h) *h = ho->sh;
        return 1;
    }
    return 0;
}
extern "C" int bob_ole_drawn_bounds(CWnd* dialog, int* x0, int* y0, int* x1, int* y1) {
    int l = 1 << 30, t = 1 << 30, r = -(1 << 30), b = -(1 << 30), n = 0;
    for (auto& kv : hosts()) {
        OleHost* h = kv.second;
        if (h->parentDlg != dialog) continue;
        if (h->sw <= 0 || h->sh <= 0) continue;          /* not drawn -> contributes nothing */
        if (h->sx < l) l = h->sx;
        if (h->sy < t) t = h->sy;
        if (h->sx + h->sw > r) r = h->sx + h->sw;
        if (h->sy + h->sh > b) b = h->sy + h->sh;
        n++;
    }
    if (!n) return 0;
    if (x0) *x0 = l; if (y0) *y0 = t; if (x1) *x1 = r; if (y1) *y1 = b;
    return 1;
}
extern "C" void bob_ole_dump_drawn_rects(CWnd* dialog) {
    int n = 0;
    for (auto& kv : hosts()) {
        OleHost* h = kv.second;
        if (h->parentDlg != dialog) continue;
        fprintf(stderr, "[hittargets]   id=%-5d rect=(%d,%d %dx%d)%s\n",
                h->ctrlId, h->sx, h->sy, h->sw, h->sh,
                (h->sw <= 0 || h->sh <= 0) ? "  <-- NOT HIT-TESTABLE (zero extent)" : "");
        n++;
    }
    fprintf(stderr, "[hittargets]   (%d hosted controls for this dialog)\n", n);
}

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
        /* S207 (ÃÂÃÂÃÂÃÂ§8-MA137): bound by what paint COVERED, not by the template rect. See OleHost::hitH. */
        int hitH = (h->hitH > 0) ? h->hitH : h->sh;
        if (x >= h->sx && x < h->sx + h->sw && y >= h->sy && y < h->sy + hitH) {
            bob_ole_last_click_id = h->ctrlId;   /* S156: report the hit control to the caller */
            /* S129: a multi-button control (RRadio tab row) -- select the button under the
               cursor and fire its genuine Selected(index) event (dispid 1, VTS_I4) via the
               general eventsink so the dialog's handler runs (e.g. CSQuick1::OnSelectedRradio
               -> QuickMissionParameters -> LaunchDial(QuickParameters), the QS page switch). */
            /* S197: offer the point to a control that needs BOTH coordinates before the
               coordinate-free onClick(). The spin button is the case: its arrows are the right
               ~15px and up-vs-down is decided by Y. */
            if (h->onClickXY(x - h->sx, y - h->sy)) {
                CWnd* par = (CWnd*)h->parentDlg;
                if (par && h->ctrlId) {
                    bob_evtA0 = h->curIndex(); bob_evtA1 = 0; bob_evtP = 0;
                    bob_evt_fire((void*)par, &typeid(*par), h->ctrlId, 2 /*TextChanged*/);
                }
                if (bob_ole_trace())
                    fprintf(stderr, "[ole] click (%d,%d) -> ctrl id=%d took it by position\n", x, y, h->ctrlId);
                return 1;
            }
            int bn = h->onButtonClick(x - h->sx);
            if (bn >= 0 && h->ctrlId) {
                bob_evtA0 = bn; bob_evtA1 = 0;
                bob_evt_fire((void*)dialog, &typeid(*dialog), h->ctrlId, 1);
                if (bob_ole_trace()) fprintf(stderr, "[ole] click (%d,%d) -> radio id=%d button=%d (Selected fired)\n", x, y, h->ctrlId, bn);
                return 1;
            }
            if (h->onClick()) {
                /* S161: supply the event ARGUMENTS. bob_evt_call marshals a (LPCTSTR,short)
                   handler as (caption, (short)bob_evtA0), and this branch never set them ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ so the
                   index parameter carried whatever the last radio/listbox click left behind.
                   Handlers that re-read the control (SController) were fine; handlers that use the
                   parameter were not: CSQuick1::OnTextChangedFamilylists does
                   `currquickfamily = index`, so choosing Dogfight applied a stale family and the
                   game launched Training/Takeoff. bob_evtP is cleared rather than left dangling ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ
                   the thunk would otherwise pass a stale pointer as the caption. */
                static const int noEvtIdx = getenv("BOB_NO_EVT_INDEX") ? 1 : 0;   /* revert */
                if (!noEvtIdx) { bob_evtA0 = h->curIndex(); bob_evtA1 = 0; bob_evtP = 0; }
                /* S33: the value already cycled (onClick); now fire the combo's TextChanged event
                   (dispid 1) on the dialog's RUNTIME type via the general eventsink so the genuine
                   handler runs (e.g. SController::OnTextChanged* applies the device/axis rebind ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ
                   it reads the combo's new GetIndex). Dialogs with no registered handler for this
                   (id,dispid) no-op faithfully (they persist via writeback-on-destroy). */
                bob_evt_fire((void*)dialog, &typeid(*dialog), h->ctrlId, 1);
                if (bob_ole_trace()) fprintf(stderr, "[ole] click (%d,%d) -> ctrl id=%d cycled (evt fired)\n", x, y, h->ctrlId);
                return 1;
            }
            /* S33: a click on a hosted LIST control (e.g. the load screen's file list) selects the
               row under the cursor ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ fire the listbox Select event (dispid 1, args row/col) via the
               general eventsink so the genuine handler runs (e.g. CLoad::OnSelectRlistboxfile sets
               selectedfile). id==0 is the FullPanelDial menu listbox (handled elsewhere) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ skip. */
            int row = h->rowAtY(y - h->sy);
            if (row >= 0 && h->ctrlId) {
                /* S141: the Select event is Select(row, COLUMN) (VTS_I4 VTS_I4) and the
                   column half was hardcoded 0. BoB's tab rows are multi-COLUMN listboxes
                   (one column per item), so handlers that switch on the column ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ e.g.
                   CSCampaign::OnSelectRlistCampaigns -> ChangeCamp(column), the campaign
                   PHASE selector ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ could only ever be told "column 0". Resolve it from the
                   control's own metrics (GetColFromX). BOB_NO_LIST_COL reverts to 0. */
                int col = getenv("BOB_NO_LIST_COL") ? 0 : h->colAtX(x - h->sx);
                bob_evtA0 = row; bob_evtA1 = col;
                if (bob_evt_fire((void*)dialog, &typeid(*dialog), h->ctrlId, 1)) {
                    if (bob_ole_trace()) fprintf(stderr, "[ole] click (%d,%d) -> list id=%d row=%d col=%d selected (evt fired)\n", x, y, h->ctrlId, row, col);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* S141: resolve a click point for control `id` on `dialog` from the CONTROL'S OWN metrics
   (its last-drawn rect + the genuine GetColFromX column walk), so headless drive recipes can
   say "click control #1000 column 1" instead of hardcoding pixels. Adopted from the MiG Alley
   port's `f,#ID[:COL]` recipe grammar (MA S62/S63): fixed pixel coordinates in test recipes
   silently break the moment a font or layout change moves a row, and they broke MA's entire
   regression gate at once. Returns 1 and fills (*px,*py) if the control is drawn. */
/* S199 (answering MA note MA115): this resolved a COLUMN and always returned the control's
   vertical CENTRE -- there was no way for a recipe to name a ROW. That is MA's S162 failure in
   mirror image: their recipes could name a row and not a column, so `:r1` on a five-column
   wave table silently addressed column 3; ours can name a column and not a row, so any recipe
   naming a multi-row list (every Order of Battle squadron list) silently clicks its MIDDLE
   ROW. Neither reads as wrong: the dialog opens, the click lands, the capture looks right and
   the content is someone else's.
   Recipe form `#ID:COL.ROW` (e.g. `#1000:0.3`); `#ID:COL` keeps the centre row as before.
   Resolved by probing the control's OWN rowAtY over its height -- the same technique the
   column half uses with colAtX, and the same one the click path already trusts (line ~573),
   so a recipe and a real click cannot disagree about which row is where. */
extern "C" int bob_ole_ctrl_point_rc(CWnd* dialog, int id, int col, int row, int* px, int* py);
extern "C" int bob_ole_ctrl_point(CWnd* dialog, int id, int col, int* px, int* py) {
    return bob_ole_ctrl_point_rc(dialog, id, col, -1, px, py);
}
extern "C" int bob_ole_ctrl_point_rc(CWnd* dialog, int id, int col, int row, int* px, int* py) {
    for (auto& kv : hosts()) {
        OleHost* h = kv.second;
        if (h->ctrlId != id) continue;
        if (dialog && h->parentDlg != dialog) continue;
        if (h->sw <= 0 || h->sh <= 0) continue;          /* not drawn (hidden / off-template) */
        int lx = h->sw / 2;
        /* S192: decide "is this a multi-column list" by probing the WHOLE width, not just the last
           pixel. The old test was `col > 0 || colAtX(sw-1) > 0`, so for **col 0** it depended
           entirely on what the control answers for its rightmost pixel -- and the campaign PHASE
           bar answers 0 there. The span search was skipped and `lx` stayed at the control's CENTRE,
           which on that four-column bar is a different phase: a recipe asking for "Convoys" (col 0)
           silently selected "Critical Period", and the capture looked like a working screen with
           the wrong content in it.
           That is MA's S85/S162 failure mode -- a recipe that quietly addresses something else --
           and the fix is the same shape: ask the control, over its whole width, instead of trusting
           one sample. A genuinely single-column control still lands on its centre, because then
           column 0's span IS the whole width. */
        int multi = (col > 0);
        if (!multi)
            for (int t = 0; t < h->sw && !multi; t++)
                if (h->colAtX(t) > 0) multi = 1;
        if (multi) {                                     /* multi-column: find the column's span */
            int start = -1, end = -1;
            for (int t = 0; t < h->sw; t++) {
                if (h->colAtX(t) == col) { if (start < 0) start = t; end = t; }
                else if (start >= 0) break;
            }
            if (start < 0) return 0;                     /* no such column */
            lx = (start + end) / 2;
        }
        int ly = h->sh / 2;
        if (row >= 0) {                              /* S199: find the row's vertical span */
            /* S207 (ÃÂÃÂÃÂÃÂ§8-MA137): scan the height paint COVERED. Bounded by h->sh this loop could
               never find a row living past the rect, so it took the S199 refusal branch and the
               recipe reported "row not mapped" -- the mirror of the hit test refusing the click.
               Both halves have to know how tall the content is or the rows stay unaddressable.
               The default (row<0) stays h->sh/2 so existing recipes keep their click point. */
            int scanH = (h->hitH > h->sh) ? h->hitH : h->sh;
            int rstart = -1, rend = -1;
            for (int t = 0; t < scanH; t++) {
                if (h->rowAtY(t) == row) { if (rstart < 0) rstart = t; rend = t; }
                else if (rstart >= 0) break;
            }
            if (rstart < 0) {
                if (bob_ole_trace())
                    fprintf(stderr, "[ole] ctrl_point id=%d row=%d not mapped by rowAtY (h=%d scanH=%d) "
                                    "-- refusing rather than clicking the middle row\n", id, row, h->sh, scanH);
                return 0;
            }
            ly = (rstart + rend) / 2;
        }
        if (px) *px = h->sx + lx;
        if (py) *py = h->sy + ly;
        if (bob_ole_trace()) fprintf(stderr, "[ole] ctrl_point id=%d col=%d row=%d -> (%d,%d) rect=(%d,%d,%d,%d)\n",
            id, col, row, h->sx + lx, h->sy + ly, h->sx, h->sy, h->sw, h->sh);
        return 1;
    }
    return 0;
}

/* S143 (SP.9, from FF note 15): summarise which dialogs are actually hosted and how many of
   their controls were last DRAWN, for the parity-capture state banner. A verdict that says
   "screen X with control set Y" must be able to point at output proving X and Y were up ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ
   otherwise a mis-selected state and a render bug are indistinguishable, which cost the
   FreeFalcon port a sprint. Writes e.g. "dlg=1481:7 dlg=2010:85" (drawn/total per dialog). */
/* S146 (SP.8, env-gated one-shot, default-off): gold #18 shows no "Sweeps" row, we draw one.
   The row is the fighter-sweep target line (IDC_FIGHTERSWEEP*), and LWDIRECT.CPP:1626 marks that
   branch dead ("Patrols removed"). The open question is only WHICH mechanism the Windows build
   uses to drop it -- template membership (S124) is the first candidate. Print, per hosted control
   on a given dialog, its id, whether the installed BDG template contains it, and its drawn rect.
   BOB_TRACE_DIR=<dlgId>. */
extern "C" void bob_ole_dump_template_membership(int dlgId) {
    static int done = 0; if (done) return; done = 1;
    int in = 0, out_ = 0;
    for (auto& kv : hosts()) {
        OleHost* h = kv.second;
        if (h->dlgId != dlgId) continue;
        int m = bob_dlg_in_template(h->dlgId, h->ctrlId);
        if (m == 0) out_++; else in++;
        /* S147: dump EVERY hosted control, not a guessed id band -- S146's guess (IDC_FIGHTERSWEEP*)
           matched nothing, and the row we draw has to be identified by where it LANDS. Sorted by
           the caller; y is what distinguishes the stray row from the six real target rows. */
        if (h->sw > 0 && h->sh > 0)
            fprintf(stderr, "[tmpl] dlg=%d id=%d in_template=%d visible=%d rect=(%d,%d,%d,%d)\n",
                h->dlgId, h->ctrlId, m, h->visible, h->sx, h->sy, h->sw, h->sh);
    }
    fprintf(stderr, "[tmpl] dlg=%d summary: %d in template, %d absent\n", dlgId, in, out_);
    fflush(stderr);
}

/* S153 (SP.10): release a dialog's hosted controls when the dialog dies.
   The host table was never pruned -- by design, per the original comment ("hosts are never erased;
   draw/click filter by parentDlg"). The S143 state banner then caught the consequence: driving the
   Directives dialog through a few open/close cycles took its hosted-control count from **184 to
   1656**, with 334 of them still being DRAWN. The game re-opens that dialog by itself, so this
   grows during ordinary play, not just under scaffolds.

   The sharper problem is correctness, not memory. The wrapper CWnd* that keys this map is usually a
   DDX_Control MEMBER of the dialog object, so once the dialog is freed the map holds keys into
   freed storage -- and a later CWnd allocated at the same address would silently find a stale host
   (wrong control type, wrong ids, wrong parent). Pruning on destroy removes a use-after-free hazard
   as well as the growth. Returns the number released so the caller can trace it. */
/* S155 (SP.23): how many hosts name this CWnd as their parent? The teardown question S154 could
   not answer by inspection -- DestroyWindow reaches SOME object, and the 184 hosts belong to
   another -- is answered definitively by asking the host table itself, per candidate object.
   Cheap, and it cannot be wrong the way four successive hook-site guesses were. */
/* S166: the WHOLE host table's size. bob_ole_count_for_dialog answers "who owns these?"; this
   answers "how many are there at all?", which is the question the S154 teardown flag was gated on
   ("default OFF until measured on the campaign paths"). Without a total, the leak can only be
   inferred from per-dialog samples. */
extern "C" int bob_ole_host_total(void) { return (int)hosts().size(); }

extern "C" int bob_ole_count_for_dialog(CWnd* dialog) {
    int n = 0;
    for (auto& kv : hosts()) if (kv.second && kv.second->parentDlg == dialog) n++;
    return n;
}

extern "C" int bob_ole_release_dialog(CWnd* dialog) {
    if (!dialog) return 0;
    auto& m = hosts();
    int n = 0;
    for (auto it = m.begin(); it != m.end(); ) {
        if (it->second && it->second->parentDlg == dialog) {
            delete it->second;
            it = m.erase(it);
            n++;
        } else ++it;
    }
    if (n && bob_ole_trace())
        fprintf(stderr, "[ole] released %d hosted control(s) with dialog %p (%zu remain)\n",
                n, (void*)dialog, m.size());
    return n;
}

extern "C" int bob_ole_state_summary(char* out, int outsz) {
    if (!out || outsz <= 0) return 0;
    out[0] = 0;
    int used = 0, ndlg = 0;
    /* group by dlgId without allocating: for each distinct id, count total + drawn */
    for (auto& kv : hosts()) {
        int id = kv.second->dlgId;
        bool seen = false;
        for (auto& kv2 : hosts()) { if (kv2.second == kv.second) break;
                                    if (kv2.second->dlgId == id) { seen = true; break; } }
        if (seen) continue;
        int total = 0, drawn = 0;
        for (auto& kv2 : hosts()) if (kv2.second->dlgId == id) {
            total++;
            if (kv2.second->visible && kv2.second->sw > 0 && kv2.second->sh > 0) drawn++;
        }
        int n = snprintf(out + used, outsz - used, "%sdlg=%d:%d/%d", used ? " " : "", id, drawn, total);
        if (n < 0 || n >= outsz - used) break;
        used += n; ndlg++;
    }
    return ndlg;
}

/* S311 (bob): an UNANSWERED dispatch used to leave the caller's return buffer untouched.
   `if (h) h->dispatch(...)` writes nothing when the control has no registered host, and an audit of
   every value-returning wrapper in this port found **31 call sites** that declare `result`
   uninitialised and hand it straight back to their caller -- CRListBox::GetCount, GetRowFromY,
   GetColFromX, GetListHeight, CRTabs::SelectTab, the CRSpinBut setters, and the rest. S289 found
   one of them (CRCombo::GetIndex) the hard way, after it silently corrupted a real preference.
   The other 30 were the same defect waiting for the same conditions.
   Patching 31 sites means inventing 31 default values. Fixing the ROUTER means one behaviour:
   when nobody answers, ZERO the return buffer and RAISE A FLAG. Zero is not claimed to be the right
   answer -- there isn't one -- but a deterministic zero beats whatever was on the stack, and the
   flag lets the callers that can do better (the settings write-back declines to store anything)
   do better. BOB_TRACE_OLE_UNANSWERED=1 reports each one. */
int g_bob_ole_unanswered = 0;          /* set by the LAST invoke; read immediately by the caller */
unsigned long g_bob_ole_unanswered_n = 0;   /* cumulative, for gates */

extern "C" void bob_ole_invoke(CWnd* self, DISPID id, WORD /*flags*/, VARTYPE vtRet, void* pvRet, const BYTE* /*pInfo*/, va_list ap) {
    /* S311 POSITIVE CONTROL. A front-end boot produces ZERO unanswered dispatches, so the branch
       below never runs and its trace never prints -- which is indistinguishable from a trace that
       cannot print. BOB_OLE_FORCE_NOHOST=1 makes every lookup miss.
       (BOB_COMBO_FORCE_UNANSWERED, added a sprint earlier, does NOT exercise this code: it forces
       the flag inside CRCombo::GetIndex and never reaches the router. Two hooks, two different
       claims -- worth keeping straight, because using the first to "prove" the second is exactly
       how a control stops controlling anything.) */
    OleHost* h = getenv("BOB_OLE_FORCE_NOHOST") ? (OleHost*)0 : findHost(self);
    if (h) { g_bob_ole_unanswered = 0; h->dispatch(id, vtRet, pvRet, ap); return; }
    g_bob_ole_unanswered = 1; g_bob_ole_unanswered_n++;
    if (pvRet) {
        /* Size by the VARTYPE the caller declared -- these wrappers pass `short` for VT_I2 and a
           4-byte long/BOOL for VT_I4/VT_BOOL. Writing 4 bytes into a `short` would corrupt the
           stack next to it, so this must match, not approximate. */
        switch (vtRet) {
            case VT_I2:   *(short*)pvRet = 0; break;
            case VT_I4:
            case VT_BOOL: *(long*)pvRet  = 0; break;
            default: break;   /* unknown width: leave it rather than guess how much to write */
        }
    }
    if (getenv("BOB_TRACE_OLE_UNANSWERED"))
        fprintf(stderr, "[ole] dispid=0x%x NOBODY ANSWERED (vtRet=%d) -> returned 0\n",
                (unsigned)id, (int)vtRet);
}
extern "C" void bob_ole_setprop(CWnd* self, DISPID id, VARTYPE /*vt*/, va_list ap) {
    OleHost* h = findHost(self); if (h) h->setprop(id, ap);
}
/* S312 (bob): the SAME hazard as bob_ole_invoke, at 206 more call sites. Every `Get*` property
   wrapper in the R* controls declares a local `result` and hands whatever GetProperty left there
   back to its caller; with no registered host, `if (h) h->getprop(...)` left it untouched. Same
   treatment: zero by the caller's VARTYPE and raise the same flag.

   !! VT_BSTR IS DELIBERATELY NOT ZEROED. Those 13 sites pass a `CString`, not a raw BSTR
   (`CString result; GetProperty(DISPID_CAPTION, VT_BSTR, &result);`). It is a default-constructed
   C++ object -- already valid and empty, so not a hazard at all -- and memset-ing four bytes of one
   would null its internal pointer and corrupt it. The audit script counted them as uninitialised
   because it looked for an `=`; a default constructor is an initialiser it could not see. So the
   real figure is 206, not the 219 the first pass reported: VT_I4 154, VT_BOOL 45, VT_I2 6, VT_CY 1.
   Anything not listed below is left alone rather than guessed at. */
extern "C" void bob_ole_getprop(CWnd* self, DISPID id, VARTYPE vt, void* pvRet) {
    OleHost* h = findHost(self);
    /* S312: count EVERY call, answered or not. The unanswered branch below never fired in a title
       boot or a settings-screen boot even with BOB_OLE_FORCE_NOHOST=1, and "the branch never ran"
       and "the function is never called" are different claims with different consequences. */
    if (getenv("BOB_TRACE_OLE_UNANSWERED"))
        fprintf(stderr, "[ole] getprop-called dispid=0x%x host=%s\n", (unsigned)id, h ? "yes" : "NO");
    if (h) { g_bob_ole_unanswered = 0; h->getprop(id, pvRet); return; }
    g_bob_ole_unanswered = 1; g_bob_ole_unanswered_n++;
    if (pvRet) {
        switch (vt) {
            case VT_I2:   *(short*)pvRet = 0; break;
            case VT_I4:
            case VT_BOOL: *(long*)pvRet  = 0; break;
            case VT_CY:   memset(pvRet, 0, 8); break;   /* CY is a POD 64-bit union */
            default: break;   /* VT_BSTR (a CString) and anything unknown: do not touch */
        }
    }
    if (getenv("BOB_TRACE_OLE_UNANSWERED"))
        fprintf(stderr, "[ole] getprop dispid=0x%x NOBODY ANSWERED (vt=%d) -> returned 0\n",
                (unsigned)id, (int)vt);
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
    /* S160: record what we just drew, like every other draw path does. This one never did, so the
       front-end menu row (Back / Fly / Sim Config ...) had NO drawn rect ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ which is exactly why the
       menu path had to re-derive its click rects from bob_gdi_text_width() instead of reading them
       back, and why those re-derived rects drifted off the painted text ("Fly" painted x=132..153,
       click rect x=83..108 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ unclickable; user-reported 2026-08-09).
       Recording it does not add a new click path: bob_ole_click is only called on the pdial[]
       panels, and this host belongs to the RFullPanelDial itself. It exists so hit rects can come
       from the paint (S156 / MA ÃÂÃÂÃÂÃÂ§8-MA84 trap 1: store what paint did, never re-derive it). */
    host->sx = x; host->sy = y; host->sw = w; host->sh = h;
    { int ch = host->contentH();                           /* S207: see OleHost::hitH */
      host->hitH = (ch > h && !getenv("BOB_NO_DRAWH")) ? ch : h; }
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

/* ── R24 (S420): THE TIMER THIS PORT NEVER HAD — cross-ported from MA PO-76/S419 ────────────────
 * `SetTimer` returned TRUE and registered nothing, `KillTimer` likewise, and `OnTimer` was a
 * non-virtual stub — so **28 OnTimer overrides and 29 SetTimer callers in this tree were dead
 * code**. MA found this the hard way: every site that services a multiplayer session sits inside an
 * OnTimer handler (`_DPlay.UIUpdateMainSheet()` → `ReceiveNextMessage`), so a hosted session was
 * opened and then never pumped, and no client could discover it. MA measured ZERO pumps after a
 * successful `Open(CREATE)`; after the timer landed, 12,500+.
 * BoB has the identical shape — `SVIEWER`, `SFLIGHT`, `RAFCOMBT`, `LWDIARYD`, `COMMSAC`, `RADIO`
 * all call `UIUpdateMainSheet()` from `OnTimer`.
 *
 * Ticked from the per-frame pump, which is where the original's WM_TIMER would arrive. An interval
 * of 0 means "every pass", which is what most call sites ask for.
 *
 * ⚠️ The tick iterates a SNAPSHOT and re-checks each entry before firing: a handler may legally
 * KillTimer, SetTimer, or destroy a window, and mutating the registry underneath the loop would be
 * a use-after-free. BOB_NO_TIMERS=1 disables the lot — the negative control for a change that
 * brings 28 handlers to life at once.
 */
#include <time.h>
#include <vector>
namespace {
struct BobTimer { CWnd* w; unsigned id; unsigned ms; unsigned long long due; };
static std::vector<BobTimer>& bobtimers() { static std::vector<BobTimer> v; return v; }
static unsigned long long bob_now_ms() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000ull + (unsigned long long)(ts.tv_nsec / 1000000);
}
}
extern "C" unsigned bob_timer_set(void* wnd, unsigned id, unsigned ms) {
    if (!wnd) return 0;
    std::vector<BobTimer>& v = bobtimers();
    for (size_t i = 0; i < v.size(); i++)
        if (v[i].w == (CWnd*)wnd && v[i].id == id) { v[i].ms = ms; v[i].due = bob_now_ms() + ms; return id; }
    BobTimer t; t.w = (CWnd*)wnd; t.id = id; t.ms = ms; t.due = bob_now_ms() + ms; v.push_back(t);
    if (getenv("BOB_TRACE_TIMER"))
        fprintf(stderr, "[mfctimer] set wnd=%p id=%u every %u ms (now %zu)\n", wnd, id, ms, v.size()), fflush(stderr);
    return id;
}
extern "C" void bob_timer_kill(void* wnd, unsigned id) {
    std::vector<BobTimer>& v = bobtimers();
    for (size_t i = 0; i < v.size(); i++)
        if (v[i].w == (CWnd*)wnd && v[i].id == id) { v.erase(v.begin() + i); return; }
}
extern "C" void bob_timer_kill_all(void* wnd) {
    std::vector<BobTimer>& v = bobtimers();
    for (size_t i = v.size(); i-- > 0; ) if (v[i].w == (CWnd*)wnd) v.erase(v.begin() + i);
}
/* ⚠️ DEFAULT OFF in BoB (BOB_TIMERS=1 enables) -- but NOT for the reason I first wrote here, and
 * the correction matters more than the conclusion.
 *
 * I saw `bob_parity.sh` fail on `config-control` (210 bytes) after enabling the timer, watched it
 * pass 4/4 with the timer disabled, and wrote that the timer caused it -- naming
 * `SController::OnTimer` (SCONTROL.CPP:1507), which really does call `GotSameDevices()` every tick
 * and rebuild the Controls display when it disagrees. Plausible, specific, and WRONG:
 * **with the timer OFF by default the screen still failed, on run 2 of 3.**
 * Four clean runs were luck, not evidence, and I had already been told this once -- S413 called the
 * same 210 bytes "deterministic" on the strength of two runs.
 *
 * What is actually established: `config-control` is INTERMITTENTLY non-deterministic inside the
 * suite (~1 run in 3), independently of the timer, while being byte-identical **9 runs out of 9**
 * when captured on its own (6 standalone, 3 after other screens). So the variable lives in the
 * suite context, and it is not this.
 *
 * The timer therefore stays opt-in here for a narrower reason: **the gate cannot currently
 * arbitrate.** With a screen that flakes on its own, a timer-induced regression and the existing
 * flake are indistinguishable, and shipping a 28-handler change that no gate can judge is how a
 * regression gets in wearing a known failure's clothes. MA keeps its timer ON -- its multiplayer
 * depends on it and its five parity screens are stable across repeated runs.
 * Blocked on: the config-control flake (filed separately), not on this code.
 */
extern "C" void bob_timers_tick(void) {
    static int off = -1;
    if (off < 0) off = getenv("BOB_TIMERS") ? 0 : 1;
    if (off) return;
    std::vector<BobTimer>& v = bobtimers();
    if (v.empty()) return;
    const unsigned long long now = bob_now_ms();
    std::vector<BobTimer> due;
    for (size_t i = 0; i < v.size(); i++) if (now >= v[i].due) due.push_back(v[i]);
    for (size_t i = 0; i < due.size(); i++) {
        bool alive = false;
        for (size_t j = 0; j < v.size(); j++)
            if (v[j].w == due[i].w && v[j].id == due[i].id) { v[j].due = now + v[j].ms; alive = true; break; }
        if (!alive) continue;
        static long fired = 0;
        if (getenv("BOB_TRACE_TIMER") && (fired == 0 || (fired % 500) == 0))
            fprintf(stderr, "[mfctimer] fire #%ld wnd=%p id=%u\n", fired, (void*)due[i].w, due[i].id), fflush(stderr);
        fired++;
        due[i].w->OnTimer(due[i].id);
    }
}
