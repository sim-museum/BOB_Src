/* ==========================================================================
 *  bob_dlgtemplate.cpp — runtime parser for the game's dialog resource templates.
 *
 *  The R* ActiveX controls' on-screen rects come from the .rc DIALOG templates
 *  (in dialog units). Parse them at runtime: RESOURCE.H gives symbol->id, MIG.RC
 *  gives each CONTROL's id + rect. We build a flat controlId -> DLU rect map
 *  (control ids are unique for the widgets we host; bob_ole_draw_panel's per-panel
 *  parent filter keeps the rare shared ids unambiguous in practice).
 *
 *  Source location: BOB_SRC_DIR (compile define from CMake) or $BOB_RC_DIR.
 *
 *  Plain C only (fixed arrays, no libstdc++ containers): this TU is built with the
 *  project-wide -fpack-struct=1, which mis-lays std::string/std::unordered_map and
 *  would crash. See PORT.md principle #2 (the packing-ABI hazard).
 * ======================================================================== */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <strings.h>   /* strncasecmp (S124 PE class GUIDs vary in case) */

#ifndef BOB_SRC_DIR
#define BOB_SRC_DIR "."
#endif

#define MAX_SYMS   20000
#define MAX_RECTS   6144
#define SYM_LEN       48

/* control-class kind (S124): known only for PE-sourced entries (the .rc text parse
   doesn't capture the class); drives template-driven static hosting. */
enum { K_UNKNOWN = 0, K_RSTATIC, K_RCOMBO, K_RLISTBOX, K_RBUTTON, K_REDIT };

struct Sym  { char name[SYM_LEN]; int id; };
struct CR   { int id, dlgId, x, y, w, h; unsigned char pe, kind; };

static Sym  g_syms[MAX_SYMS];   static int g_nsyms  = 0;
static CR   g_rects[MAX_RECTS]; static int g_nrects = 0;
static int  g_loaded = 0;
static int  g_pe     = 0;       /* S124: PE (BDG-oracle) resources loaded */

static int symLookup(const char* name) {
    for (int i = 0; i < g_nsyms; i++) if (strcmp(g_syms[i].name, name) == 0) return g_syms[i].id;
    return -1;   /* ids are non-negative in RESOURCE.H */
}

static void parseResourceH(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char sym[256]; long val;
        /* "#define SYM 1234" — %ld rejects expression/string macros */
        if (sscanf(line, " #define %255s %ld", sym, &val) == 2) {
            if (g_nsyms < MAX_SYMS && val >= 0) {
                strncpy(g_syms[g_nsyms].name, sym, SYM_LEN - 1);
                g_syms[g_nsyms].name[SYM_LEN - 1] = 0;
                g_syms[g_nsyms].id = (int)val;
                g_nsyms++;
            }
        }
    }
    fclose(f);
}

/* Does the trimmed line start a new RC statement/section (a CONTROL-statement boundary)? */
static int isBoundary(const char* p) {
    while (*p && isspace((unsigned char)*p)) p++;
    static const char* kw[] = {
        "CONTROL", "LTEXT", "RTEXT", "CTEXT", "EDITTEXT", "COMBOBOX", "LISTBOX",
        "GROUPBOX", "PUSHBUTTON", "DEFPUSHBUTTON", "CHECKBOX", "RADIOBUTTON",
        "ICON", "SCROLLBAR", "STATE3", "AUTOCHECKBOX", "AUTORADIOBUTTON",
        "BEGIN", "END", 0
    };
    for (int i = 0; kw[i]; i++) {
        size_t n = strlen(kw[i]);
        if (strncmp(p, kw[i], n) == 0 && (isspace((unsigned char)p[n]) || p[n] == 0)) return 1;
    }
    /* "<SYM> DIALOG[EX] ..." starts a new dialog */
    if (strstr(p, " DIALOG")) return 1;
    return 0;
}

static int g_curDlgId = -1;   /* the dialog whose CONTROL statements we're currently parsing */
/* S181: the DIALOG statement's OWN size, in DLU.

   The parser recorded every CONTROL rect but threw away the `IDDS_MODAL_DIALOG DIALOG 0,0,272,104`
   line it was already matching to set g_curDlgId. On Windows that size is what clips the dialog:
   a child window's DC stops at its client rect, so RMdlDlg::OnPaint blitting a 780x585 background
   through it shows only a 272x104-DLU window onto that art. With no window and no clip, the port
   splatted the whole bitmap across the screen from the origin -- the PO's "Caught Naping dialog
   too big". Keep the size so the modal can clip to it. */
struct DlgSz { int dlgId, w, h; };
static DlgSz g_dlgsz[128]; static int g_ndlgsz = 0;
static void storeDlgSize(int dlgId, int w, int h) {
    if (dlgId <= 0 || w <= 0 || h <= 0) return;
    for (int i = 0; i < g_ndlgsz; i++) if (g_dlgsz[i].dlgId == dlgId) { g_dlgsz[i].w = w; g_dlgsz[i].h = h; return; }
    if (g_ndlgsz < (int)(sizeof g_dlgsz / sizeof *g_dlgsz))
        { g_dlgsz[g_ndlgsz].dlgId = dlgId; g_dlgsz[g_ndlgsz].w = w; g_dlgsz[g_ndlgsz].h = h; g_ndlgsz++; }
}

/* Parse one accumulated CONTROL statement: pull the id (token after the caption)
   and the last four integers outside quotes (x,y,w,h). */
static void parseControl(const char* s) {
    /* control id: after the first quoted caption, the field up to the next comma */
    const char* q1 = strchr(s, '"');
    const char* q2 = q1 ? strchr(q1 + 1, '"') : 0;
    const char* c1 = q2 ? strchr(q2 + 1, ',') : 0;
    if (!c1) return;
    const char* a = c1 + 1; while (*a && isspace((unsigned char)*a)) a++;
    const char* b = strchr(a, ',');
    if (!b) return;
    char idsym[SYM_LEN]; int n = (int)(b - a); if (n >= SYM_LEN) n = SYM_LEN - 1;
    memcpy(idsym, a, n); idsym[n] = 0;
    while (n > 0 && isspace((unsigned char)idsym[n-1])) idsym[--n] = 0;

    /* collect integers outside quotes; last four = x,y,w,h. Identifiers (WS_*, IDC_*)
       and the "{clsid}" digits are skipped. */
    int nums[8]; int nn = 0; int inq = 0;
    for (const char* k = s; *k; ) {
        if (*k == '"') { inq = !inq; k++; continue; }
        int prevId = (k > s) && (isalpha((unsigned char)k[-1]) || k[-1] == '_');
        if (!inq && !prevId && (isdigit((unsigned char)*k) || (*k == '-' && isdigit((unsigned char)k[1])))) {
            long v = strtol(k, NULL, 10);
            const char* e = k; if (*e == '-') e++;
            while (isdigit((unsigned char)*e)) e++;
            /* reject hex / identifier-embedded digits (e.g. 11D1 inside a guid, already in quotes) */
            if (!(isalpha((unsigned char)*e) || *e == '_' || *e == 'x' || *e == 'X')) {
                if (nn < 8) nums[nn++] = (int)v;
                else { for (int j = 0; j < 7; j++) nums[j] = nums[j+1]; nums[7] = (int)v; }
            }
            k = e; continue;
        }
        k++;
    }
    int id = symLookup(idsym);
    if (id >= 0 && nn >= 4 && g_nrects < MAX_RECTS) {
        /* Keep a per-(dialog,control) entry so shared control ids (e.g. IDC_PAUSE reused across
           dialogs) don't collide — a plain by-id lookup returned the last-parsed dialog's rect,
           putting the map's accel buttons at the wrong place (S94). Update in place if this exact
           (dlgId,id) was already seen; else append.
           S124: a PE (BDG-oracle) entry is never overwritten by the .rc text parse — the .rc runs
           after loadFromPE() purely as a fallback for pairs the installed resources lack. */
        for (int i = 0; i < g_nrects; i++) if (g_rects[i].id == id && g_rects[i].dlgId == g_curDlgId) {
            if (g_rects[i].pe) return;
            g_rects[i].x = nums[nn-4]; g_rects[i].y = nums[nn-3];
            g_rects[i].w = nums[nn-2]; g_rects[i].h = nums[nn-1]; return;
        }
        g_rects[g_nrects].id = id; g_rects[g_nrects].dlgId = g_curDlgId;
        g_rects[g_nrects].x = nums[nn-4]; g_rects[g_nrects].y = nums[nn-3];
        g_rects[g_nrects].w = nums[nn-2]; g_rects[g_nrects].h = nums[nn-1];
        g_rects[g_nrects].pe = 0; g_rects[g_nrects].kind = K_UNKNOWN;
        g_nrects++;
    }
}

static void parseRc(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[1024];
    char stmt[4096]; int slen = 0; int inControl = 0;
    while (fgets(line, sizeof line, f)) {
        if (isBoundary(line)) {
            if (inControl) { parseControl(stmt); }
            slen = 0; stmt[0] = 0;
            const char* p = line; while (*p && isspace((unsigned char)*p)) p++;
            /* "<SYM> DIALOG[EX] ..." starts a new dialog -> remember its id for the CONTROLs within */
            const char* dp = strstr(p, " DIALOG");
            if (dp) { char dsym[SYM_LEN]; if (sscanf(p, " %47[A-Za-z0-9_]", dsym) == 1) g_curDlgId = symLookup(dsym);
                /* "<SYM> DIALOG[EX] [DISCARDABLE] x, y, cx, cy" -- take the last four integers */
                int dx, dy, dcx, dcy;
                const char* np = dp; while (*np && *np != '0' && !isdigit((unsigned char)*np)) np++;
                if (sscanf(np, "%d , %d , %d , %d", &dx, &dy, &dcx, &dcy) == 4)
                    storeDlgSize(g_curDlgId, dcx, dcy); }
            inControl = (strncmp(p, "CONTROL", 7) == 0 &&
                         (isspace((unsigned char)p[7]) || p[7] == 0));
        }
        if (inControl) {
            int n = (int)strlen(line);
            if (slen + n < (int)sizeof(stmt) - 1) { memcpy(stmt + slen, line, n); slen += n; stmt[slen] = 0; }
        }
    }
    if (inControl) parseControl(stmt);
    fclose(f);
}

/* ---- DLGINIT property bags: design-time control properties (the labels' text).
   For RStatic the readable, non-resource-id, non-licence string in a control's
   blob is its label ("Display Driver:", ...). Keyed by (dialogId, controlId)
   because static-control ids repeat across dialogs. ---- */
#define MAX_CAPS 6144
struct Cap { int dlgId, ctrlId; unsigned char pe; char s[64]; char ids[40]; };
static Cap g_caps[MAX_CAPS]; static int g_ncaps = 0;
static int g_loadingPE = 0;   /* S124: inside loadFromPE() — entries stored now are oracle data */

static void storeCaption(int dlgId, int ctrlId, const char* s, const char* idsName) {
    if (!s[0] && !idsName[0]) return;
    for (int j = 0; j < g_ncaps; j++) if (g_caps[j].dlgId == dlgId && g_caps[j].ctrlId == ctrlId) {
        if (g_caps[j].pe && !g_loadingPE) return;   /* .rc never overwrites a PE caption (S124) */
        strncpy(g_caps[j].s, s, 63); g_caps[j].s[63] = 0;
        strncpy(g_caps[j].ids, idsName, 39); g_caps[j].ids[39] = 0; return; }
    if (g_ncaps >= MAX_CAPS) return;
    g_caps[g_ncaps].dlgId = dlgId; g_caps[g_ncaps].ctrlId = ctrlId;
    g_caps[g_ncaps].pe = (unsigned char)g_loadingPE;
    strncpy(g_caps[g_ncaps].s, s, 63); g_caps[g_ncaps].s[63] = 0;
    strncpy(g_caps[g_ncaps].ids, idsName, 39); g_caps[g_ncaps].ids[39] = 0;
    g_ncaps++;
}
/* the caption = the last length-prefixed printable string that isn't a resource id
   (ID*) or the licence ("Copyright ...").
   S124: ALSO capture the last "IDS_*" name — the genuine CRStaticCtrl resolves its
   runtime caption via WM_GETSTRING(ResourceNumber) -> LoadString from the language
   DLL's string table (RSTATICC.CPP GetParentWndInfo; the literal is design-time
   only). bob_dlg_caption prefers the string-table text when the name resolves. */
static void extractCaption(const unsigned char* b, int n, int dlgId, int ctrlId) {
    char best[64]; best[0] = 0;
    char ids[40];  ids[0]  = 0;
    for (int i = 0; i < n; i++) {
        int len = b[i];
        if (len < 2 || len > 62 || i + 1 + len > n) continue;
        int ok = 1; for (int k = 0; k < len; k++) { unsigned char c = b[i+1+k]; if (c < 32 || c > 126) { ok = 0; break; } }
        if (!ok) continue;
        char t[64]; memcpy(t, b + i + 1, len); t[len] = 0;
        if (strncmp(t, "IDS_", 4) == 0 && len < 40) { strcpy(ids, t); continue; }
        if (strncmp(t, "ID", 2) == 0 || strncmp(t, "Copyright", 9) == 0) continue;
        strcpy(best, t);   /* keep the last qualifying string */
    }
    if (best[0] || ids[0]) storeCaption(dlgId, ctrlId, best, ids);
}

/* ---- button art (S89): the R* buttons' NormalFileNumString is a "FIL_..." equate stored
   in the same DLGINIT property bag; capture it per (dlg,ctrl) so HostRButton can resolve it
   to a FileNum via GetFileNum. Distinct from the caption (which is the hint text). ---- */
struct Art { int dlgId, ctrlId; unsigned char pe; char s[48]; };
static Art g_arts[MAX_CAPS]; static int g_narts = 0;
static void storeArt(int dlgId, int ctrlId, const char* s) {
    for (int j = 0; j < g_narts; j++) if (g_arts[j].dlgId==dlgId && g_arts[j].ctrlId==ctrlId) {
        if (g_arts[j].pe && !g_loadingPE) return;   /* .rc never overwrites a PE art name (S124) */
        strncpy(g_arts[j].s, s, 47); g_arts[j].s[47]=0; return; }
    if (g_narts >= MAX_CAPS) return;
    g_arts[g_narts].dlgId=dlgId; g_arts[g_narts].ctrlId=ctrlId;
    g_arts[g_narts].pe = (unsigned char)g_loadingPE;
    strncpy(g_arts[g_narts].s, s, 47); g_arts[g_narts].s[47]=0; g_narts++;
}
static void extractArtName(const unsigned char* b, int n, int dlgId, int ctrlId) {
    for (int i = 0; i < n; i++) {              /* first length-prefixed "FIL_..." string */
        int len = b[i];
        if (len < 5 || len > 46 || i + 1 + len > n) continue;
        if (memcmp(b + i + 1, "FIL_", 4) != 0) continue;
        int ok = 1; for (int k = 0; k < len; k++) { unsigned char c = b[i+1+k]; if (c < 32 || c > 126) { ok = 0; break; } }
        if (!ok) continue;
        char t[48]; memcpy(t, b + i + 1, len); t[len] = 0;
        storeArt(dlgId, ctrlId, t); return;
    }
}
/* ---- S125 (#16/#17): design-time layout props from the DLGINIT bag.
   The genuine controls load these in DoPropExchange from the persisted property
   stream; our hosts boot from an EMPTY CPropExchange, so they were lost:
   - RListBox column widths/aligns (`A0..A8` PX_Shorts + `C0..C8` PX_Longs, the
     LAST 54 bytes of a version&0x4 bag — RLISTBXC.CPP DoPropExchange writes them
     last). CSCampaign's tab row (dlg 289 ctrl 1000) persists 4x180px columns
     with C2=C3=1 (right-aligned) — exactly the gold full-width spread.
   - RButton `ResourceNumber` with m_alignment packed in bits 24..31
     (RBUTTONC.CPP DoPropExchange: m_alignment=m_ResourceNumber>>24; 0=centre
     1=left 2=right). In the bag stream it is the DWORD immediately after the
     persisted design caption (the same "best" string extractCaption keeps):
     verified IDC_ROLE/IDC_SIDE=2(right), IDC_PERIOD=1(left), dates=0(centre) —
     the gold campaignentername layout. `BOB_NO_DLGINIT_PROPS` reverts.
   S126: when the control CLASS is known (PE templates), the S125 offset-anchor
   decode is replaced by an EXACT sequential stream walk (see the layout note at
   bagWalk below); the anchors remain the fallback for kind-less .rc data. ---- */
struct Props { int dlgId, ctrlId; unsigned char pe, hasRes, ncols; unsigned resnum; short colw[9]; int cola[9]; };
static Props g_props[MAX_CAPS]; static int g_nprops = 0;
static Props* propSlot(int dlgId, int ctrlId) {
    for (int j = 0; j < g_nprops; j++)
        if (g_props[j].dlgId == dlgId && g_props[j].ctrlId == ctrlId) {
            if (g_props[j].pe && !g_loadingPE) return NULL;   /* .rc never overwrites PE */
            return &g_props[j];
        }
    if (g_nprops >= MAX_CAPS) return NULL;
    Props* p = &g_props[g_nprops++];
    memset(p, 0, sizeof *p);
    p->dlgId = dlgId; p->ctrlId = ctrlId; p->pe = (unsigned char)g_loadingPE;
    return p;
}
/* ---- S126: raw property-stream storage + exact sequential walk. --------------
   The full persisted-property-stream layout (reverse-engineered from boblang.dll,
   validated offline against ALL 1280 R*-class RT240 bags — zero parse failures):

     [DWORD licence-wchar-count][UTF-16 licence string]
     [DWORD version]                  (ExchangeVersion; LOWORD=verMinor)
     [DWORD extentX][DWORD extentY]   (COleControl::ExchangeExtent, HIMETRIC)
     [DWORD stockPropMask]            (COleControl::ExchangeStockProps)
       mask&0x02 Caption   = CString  (MFC archive: BYTE len; 0xFF -> WORD; ...)
       mask&0x08 ForeColor = DWORD
       mask&0x01 BackColor = DWORD
       mask&0x40 Enabled   = BYTE
       (no other bits occur in the shipped data; unknown bits abort the walk)
     [control's own DoPropExchange fields, in source order:
       PX_Bool=BYTE, PX_Short=WORD, PX_Long/PX_Color=DWORD, PX_String=CString]

   Trailing bytes beyond what DoPropExchange reads are editor slop (verified:
   heap garbage/old-string remnants) — unread, exactly as on Windows.
   The raw bags are kept (bob_dlg_propbag) so the hosts can feed each genuine
   control's DoPropExchange through the CPropExchange stream reader (afxwin.h);
   the walk below only fills the Props table (columns/resnum) exactly, replacing
   the S125 offset anchors whenever the control class is known. ---- */
#define BAG_ARENA (512*1024)
struct BagRef { int dlgId, ctrlId, off, len; unsigned char pe; };
static unsigned char g_bagArena[BAG_ARENA]; static int g_bagUsed = 0;
static BagRef g_bags[MAX_CAPS]; static int g_nbags = 0;

static void storeBag(int dlgId, int ctrlId, const unsigned char* b, int n) {
    if (n <= 0) return;
    for (int j = 0; j < g_nbags; j++) if (g_bags[j].dlgId == dlgId && g_bags[j].ctrlId == ctrlId) {
        if (g_bags[j].pe && !g_loadingPE) return;   /* .rc never overwrites a PE bag */
        if (n <= g_bags[j].len) { memcpy(g_bagArena + g_bags[j].off, b, n); g_bags[j].len = n; return; }
        break;  /* need a fresh slot-size; fall through to append + retarget */
    }
    if (g_nbags >= MAX_CAPS || g_bagUsed + n > BAG_ARENA) return;
    for (int j = 0; j < g_nbags; j++) if (g_bags[j].dlgId == dlgId && g_bags[j].ctrlId == ctrlId) {
        g_bags[j].off = g_bagUsed; g_bags[j].len = n; g_bags[j].pe = (unsigned char)g_loadingPE;
        memcpy(g_bagArena + g_bagUsed, b, n); g_bagUsed += n; return;
    }
    g_bags[g_nbags].dlgId = dlgId; g_bags[g_nbags].ctrlId = ctrlId;
    g_bags[g_nbags].off = g_bagUsed; g_bags[g_nbags].len = n;
    g_bags[g_nbags].pe = (unsigned char)g_loadingPE; g_nbags++;
    memcpy(g_bagArena + g_bagUsed, b, n); g_bagUsed += n;
}

static int bagKindOf(int dlgId, int ctrlId) {           /* K_* from the PE template class GUID */
    for (int i = 0; i < g_nrects; i++)
        if (g_rects[i].id == ctrlId && g_rects[i].dlgId == dlgId) return g_rects[i].kind;
    return K_UNKNOWN;
}

struct BagWalk { const unsigned char* b; int n, p, ok; };
static int bwNeed(BagWalk* w, int k) { if (!w->ok || w->p + k > w->n) { w->ok = 0; return 0; } return 1; }
static unsigned bwU32(BagWalk* w) {
    if (!bwNeed(w, 4)) return 0;
    unsigned v = (unsigned)w->b[w->p] | ((unsigned)w->b[w->p+1] << 8)
               | ((unsigned)w->b[w->p+2] << 16) | ((unsigned)w->b[w->p+3] << 24);
    w->p += 4; return v;
}
static unsigned bwU16(BagWalk* w) {
    if (!bwNeed(w, 2)) return 0;
    unsigned v = (unsigned)w->b[w->p] | ((unsigned)w->b[w->p+1] << 8);
    w->p += 2; return v;
}
static unsigned bwU8(BagWalk* w) { if (!bwNeed(w, 1)) return 0; return w->b[w->p++]; }
/* MFC CString archive: BYTE len, 0xFF -> WORD len, 0xFF/0xFFFF -> DWORD len
   (0xFFFE unicode marker never occurs in the shipped bags -> treated as bad). */
static int bwStr(BagWalk* w, char* out, int outsz) {
    unsigned n = bwU8(w);
    if (n == 0xFF) {
        n = bwU16(w);
        if (n == 0xFFFE) { w->ok = 0; return 0; }
        if (n == 0xFFFF) n = bwU32(w);
    }
    if (!bwNeed(w, (int)n)) return 0;
    if (out) {
        int c = (int)n < outsz - 1 ? (int)n : outsz - 1;
        if (c > 0) memcpy(out, w->b + w->p, c);
        if (outsz > 0) out[c < 0 ? 0 : c] = 0;
    }
    w->p += (int)n; return 1;
}
/* licence + version + extent + stock props; returns the version DWORD (0 = bad). */
static unsigned bwHeader(BagWalk* w) {
    unsigned lic = bwU32(w);
    if (lic < 8 || lic > 128) { w->ok = 0; return 0; }
    if (!bwNeed(w, 2 * (int)lic)) return 0;
    w->p += 2 * (int)lic;
    unsigned ver = bwU32(w);
    bwU32(w); bwU32(w);                       /* extent */
    unsigned mask = bwU32(w);
    if (mask & ~0x4Bu) { w->ok = 0; return 0; }
    if (mask & 0x02) bwStr(w, NULL, 0);       /* Caption */
    if (mask & 0x08) bwU32(w);                /* ForeColor */
    if (mask & 0x01) bwU32(w);                /* BackColor */
    if (mask & 0x40) bwU8(w);                 /* Enabled */
    return w->ok ? ver : 0;
}
/* Exact per-class walk to the two Props fields (resnum / listbox columns).
   Returns 1 when the walk covered the class (even if the ver-gated tail is absent). */
static int seqProps(const unsigned char* b, int n, int kind,
                    int* hasRes, unsigned* resnum, int* ncols, short colw[9], int cola[9]) {
    BagWalk w = { b, n, 0, 1 };
    unsigned ver = bwHeader(&w);
    if (!w.ok) return 0;
    switch (kind) {
    case K_RSTATIC:
        bwU32(&w);                            /* FontNum */
        bwStr(&w, NULL, 0);                   /* String */
        *resnum = bwU32(&w); *hasRes = w.ok;  /* ResourceNumber */
        return w.ok;
    case K_RBUTTON:
        bwU8(&w); bwU32(&w);                  /* MovesParent, FontNum */
        bwU8(&w); bwU8(&w); bwU8(&w);         /* CloseButton, TickButton, ShowShadow */
        bwU32(&w);                            /* ShadowColor */
        bwStr(&w, NULL, 0);                   /* String */
        *resnum = bwU32(&w); *hasRes = w.ok;  /* ResourceNumber (alignment in bits 24..31) */
        return w.ok;
    case K_RLISTBOX: {
        for (int i = 0; i < 6; i++) bwU8(&w); /* IsStripey..DragAndDrop */
        for (int i = 0; i < 7; i++) bwU32(&w);/* StripeColor..HeaderColor */
        bwU32(&w); bwU32(&w);                 /* FontNum, FontNum2 */
        bwU8(&w); bwU8(&w);                   /* Blackboard, SelectWholeRows */
        if (ver & 1) { bwU8(&w); bwU32(&w); bwU32(&w); bwU8(&w); }
        if (ver & 2) { bwU32(&w); bwU32(&w); }
        if (!(ver & 4) || !w.ok) return w.ok;
        short cw[9]; int ca[9]; int sane = 1;
        for (int c = 0; c < 9; c++) cw[c] = (short)bwU16(&w);
        for (int c = 0; c < 9; c++) ca[c] = (int)bwU32(&w);
        if (!w.ok) return 0;
        for (int c = 0; c < 9; c++)
            if (cw[c] < 0 || cw[c] > 2000 || ca[c] < 0 || ca[c] > 15) { sane = 0; break; }
        if (sane) {
            int nc = 0; for (int c = 0; c < 9 && cw[c]; c++) nc = c + 1;
            *ncols = nc; memcpy(colw, cw, sizeof cw); memcpy(cola, ca, sizeof ca);
        }
        return 1;
    }
    case K_RCOMBO: case K_REDIT:
        return 1;                             /* no Props-table fields */
    }
    return 0;
}

static void extractProps(const unsigned char* b, int n, int dlgId, int ctrlId) {
    /* bag header: DWORD unicode-char count + UTF-16 licence string, then
       WORD verMinor, WORD verMajor (ExchangeVersion). */
    if (n < 12) return;
    storeBag(dlgId, ctrlId, b, n);            /* S126: keep the raw stream for the hosts */
    /* S126: exact sequential walk when the control class is known (PE templates). */
    {
        int kind = bagKindOf(dlgId, ctrlId);
        if (kind != K_UNKNOWN) {
            int hasRes = 0, ncols = 0; unsigned resnum = 0; short colw[9]; int cola[9];
            if (seqProps(b, n, kind, &hasRes, &resnum, &ncols, colw, cola)) {
                if (hasRes || ncols) {
                    Props* p = propSlot(dlgId, ctrlId);
                    if (p) {
                        p->hasRes = (unsigned char)hasRes; p->resnum = resnum;
                        p->ncols = (unsigned char)ncols;
                        if (ncols) { memcpy(p->colw, colw, sizeof p->colw); memcpy(p->cola, cola, sizeof p->cola); }
                    }
                }
                return;                       /* exact — skip the S125 anchor heuristics */
            }
        }
    }
    unsigned nch = (unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24);
    int verOff = 4 + 2 * (int)nch;
    if (nch < 8 || nch > 128 || verOff + 4 > n) return;
    int verMinor = b[verOff] | (b[verOff + 1] << 8);
    unsigned resnum = 0; int hasRes = 0;
    /* ResourceNumber: the DWORD right after the last caption-qualifying string
       (same qualification rule as extractCaption). */
    int bestEnd = -1;
    for (int i = 0; i < n; i++) {
        int len = b[i];
        if (len < 2 || len > 62 || i + 1 + len > n) continue;
        int ok = 1; for (int k = 0; k < len; k++) { unsigned char c = b[i+1+k]; if (c < 32 || c > 126) { ok = 0; break; } }
        if (!ok) continue;
        if (b[i+1] == 'I' && b[i+2] == 'D') continue;                 /* ID*/
        if (len >= 9 && memcmp(b + i + 1, "Copyright", 9) == 0) continue;
        if (len >= 4 && memcmp(b + i + 1, "FIL_", 4) == 0) continue;
        bestEnd = i + 1 + len;
    }
    if (bestEnd > 0 && bestEnd + 4 <= n) {
        resnum = (unsigned)b[bestEnd] | ((unsigned)b[bestEnd+1] << 8)
               | ((unsigned)b[bestEnd+2] << 16) | ((unsigned)b[bestEnd+3] << 24);
        hasRes = 1;
    }
    short colw[9]; int cola[9]; int ncols = 0;
    if ((verMinor & 0x4) && n >= verOff + 4 + 54) {   /* A0..A8 + C0..C8 close the bag */
        const unsigned char* t = b + n - 54;
        int sane = 1;
        for (int c = 0; c < 9; c++) {
            colw[c] = (short)(t[2*c] | (t[2*c+1] << 8));
            const unsigned char* cc = t + 18 + 4*c;
            cola[c] = (int)(cc[0] | (cc[1] << 8) | (cc[2] << 16) | ((unsigned)cc[3] << 24));
            if (colw[c] < 0 || colw[c] > 2000 || cola[c] < 0 || cola[c] > 15) { sane = 0; break; }
        }
        if (sane) for (int c = 0; c < 9 && colw[c]; c++) ncols = c + 1;
    }
    if (!hasRes && !ncols) return;
    Props* p = propSlot(dlgId, ctrlId);
    if (!p) return;
    p->hasRes = (unsigned char)hasRes; p->resnum = resnum;
    p->ncols = (unsigned char)ncols;
    if (ncols) { memcpy(p->colw, colw, sizeof colw); memcpy(p->cola, cola, sizeof cola); }
}
static void load(void);
extern "C" int bob_dlg_resnum(int dlgId, int ctrlId, unsigned* rn) {
    load();
    if (getenv("BOB_NO_DLGINIT_PROPS")) return 0;
    for (int j = 0; j < g_nprops; j++)
        if (g_props[j].dlgId == dlgId && g_props[j].ctrlId == ctrlId && g_props[j].hasRes) {
            if (rn) *rn = g_props[j].resnum; return 1; }
    return 0;
}
extern "C" int bob_dlg_columns(int dlgId, int ctrlId, short w[9], int a[9]) {
    load();
    if (getenv("BOB_NO_DLGINIT_PROPS")) return 0;
    for (int j = 0; j < g_nprops; j++)
        if (g_props[j].dlgId == dlgId && g_props[j].ctrlId == ctrlId && g_props[j].ncols) {
            if (w) memcpy(w, g_props[j].colw, 9 * sizeof(short));
            if (a) memcpy(a, g_props[j].cola, 9 * sizeof(int));
            return g_props[j].ncols;
        }
    return 0;
}
/* S126: the raw persisted property stream for (dlg,ctrl) — fed to the genuine
   control's DoPropExchange via the CPropExchange stream reader (afxwin.h).
   BOB_NO_PROP_STREAM reverts to the S125 spot-fix behaviour (columns/resnum
   above); BOB_NO_DLGINIT_PROPS reverts the whole design-prop layer. */
extern "C" int bob_dlg_propbag(int dlgId, int ctrlId, const unsigned char** p, int* n) {
    load();
    if (getenv("BOB_NO_DLGINIT_PROPS") || getenv("BOB_NO_PROP_STREAM")) return 0;
    for (int j = 0; j < g_nbags; j++) if (g_bags[j].dlgId == dlgId && g_bags[j].ctrlId == ctrlId) {
        if (p) *p = g_bagArena + g_bags[j].off;
        if (n) *n = g_bags[j].len;
        return 1;
    }
    return 0;
}
/* S126: control-class kind from the PE template (0 unknown, 1 RStatic, 2 RCombo,
   3 RListBox, 4 RButton, 5 REdit — the K_* enum). Drives the covered-static
   erase emulation in bob_ole_draw_panel. */
extern "C" int bob_dlg_kind(int dlgId, int ctrlId) {
    load();
    return bagKindOf(dlgId, ctrlId);
}

extern "C" int bob_dlg_artname(int dlgId, int ctrlId, char* out, int outsz) {
    load();
    for (int j = 0; j < g_narts; j++) if (g_arts[j].dlgId==dlgId && g_arts[j].ctrlId==ctrlId) {
        strncpy(out, g_arts[j].s, outsz-1); out[outsz-1]=0; return 1; }
    return 0;
}

static void parseDlgInit(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return;
    char line[1024];
    int curDlg = -1, curCtrl = -1;
    unsigned char buf[4096]; int blen = 0;
    while (fgets(line, sizeof line, f)) {
        char* dl = strstr(line, " DLGINIT");
        if (dl) {                                   /* "<SYM> DLGINIT" */
            if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl); extractProps(buf, blen, curDlg, curCtrl);
            char sym[256]; curDlg = (sscanf(line, " %255[A-Za-z0-9_]", sym) == 1) ? symLookup(sym) : -1;
            curCtrl = -1; blen = 0;
            continue;
        }
        char cs[256];
        if (curDlg >= 0 && sscanf(line, " %255[A-Za-z0-9_] ,", cs) == 1 && strstr(line, "0x376")) {
            if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl); extractProps(buf, blen, curDlg, curCtrl);   /* flush previous */
            curCtrl = symLookup(cs); blen = 0;
            continue;
        }
        {   /* trimmed "END" closes the block */
            const char* p = line; while (*p && isspace((unsigned char)*p)) p++;
            if (strncmp(p, "END", 3) == 0 && (p[3] == 0 || isspace((unsigned char)p[3]))) {
                if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl); extractProps(buf, blen, curDlg, curCtrl);
                curDlg = curCtrl = -1; blen = 0; continue;
            }
        }
        if (curCtrl >= 0) {                          /* "0xNNNN, 0xNNNN, ..." -> bytes */
            const char* p = line;
            while ((p = strstr(p, "0x")) != NULL) {
                long w = strtol(p, NULL, 16);
                if (blen + 2 <= (int)sizeof(buf)) { buf[blen++] = (unsigned char)(w & 0xff); buf[blen++] = (unsigned char)((w >> 8) & 0xff); }
                p += 2;
            }
        }
    }
    if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl); extractProps(buf, blen, curDlg, curCtrl);
    fclose(f);
}

/* ==========================================================================
 * S124: primary source = the INSTALLED build's PE resources (boblang.dll, i.e.
 * the BDG 0.99 patched data the gold parity shots run — the PO's oracle),
 * via the bob_resources.cpp enumerators. The source-checkout .rc text parse
 * stays as the fallback for anything the PE lacks. `BOB_NO_PE_RSRC` reverts
 * to the pre-S124 .rc-only behaviour.
 * ======================================================================== */
extern "C" int bob_res_enum_dialog_items(
        void (*itemcb)(void* ctx, int dlgId, int ctrlId, int x, int y, int w, int h, const char* cls),
        void* ctx);
extern "C" int bob_res_enum_dlginit(
        void (*initcb)(void* ctx, int dlgId, int ctrlId, const unsigned char* data, int len),
        void* ctx);

/* R* coclass CLSID strings as they appear in dialog templates (case varies) */
static int classifyClass(const char* cls) {
    if (!cls || cls[0] != '{') return K_UNKNOWN;
    if (!strncasecmp(cls+1, "C42BAC3D", 8)) return K_RSTATIC;
    if (!strncasecmp(cls+1, "737CB0C9", 8)) return K_RCOMBO;
    if (!strncasecmp(cls+1, "48814009", 8)) return K_RLISTBOX;
    if (!strncasecmp(cls+1, "78918646", 8)) return K_RBUTTON;
    if (!strncasecmp(cls+1, "499E2BE6", 8)) return K_REDIT;
    return K_UNKNOWN;
}
static void peItemCb(void*, int dlgId, int ctrlId, int x, int y, int w, int h, const char* cls) {
    if (ctrlId <= 0) return;
    int kind = classifyClass(cls);
    for (int i = 0; i < g_nrects; i++) if (g_rects[i].id == ctrlId && g_rects[i].dlgId == dlgId) {
        g_rects[i].x = x; g_rects[i].y = y; g_rects[i].w = w; g_rects[i].h = h;
        g_rects[i].pe = 1; g_rects[i].kind = (unsigned char)kind; return;
    }
    if (g_nrects >= MAX_RECTS) return;
    g_rects[g_nrects].id = ctrlId; g_rects[g_nrects].dlgId = dlgId;
    g_rects[g_nrects].x = x; g_rects[g_nrects].y = y;
    g_rects[g_nrects].w = w; g_rects[g_nrects].h = h;
    g_rects[g_nrects].pe = 1; g_rects[g_nrects].kind = (unsigned char)kind;
    g_nrects++;
}
static void peInitCb(void*, int dlgId, int ctrlId, const unsigned char* data, int len) {
    /* same byte format as the .rc DLGINIT hex dump — reuse the proven extractors */
    extractCaption(data, len, dlgId, ctrlId);
    extractArtName(data, len, dlgId, ctrlId);
    extractProps(data, len, dlgId, ctrlId);
}
static int loadFromPE(void) {
    g_loadingPE = 1;
    int items = bob_res_enum_dialog_items(peItemCb, NULL);
    int inits = bob_res_enum_dlginit(peInitCb, NULL);
    g_loadingPE = 0;
    if (getenv("BOB_TRACE_OLE"))
        fprintf(stderr, "[ole] PE resources (BDG oracle): %d dialog items, %d DLGINIT records\n",
                items, inits);
    return items > 0;
}

static void load(void) {
    if (g_loaded) return;
    g_loaded = 1;
    const char* base = getenv("BOB_RC_DIR"); if (!base) base = BOB_SRC_DIR;
    char path[1024];
    snprintf(path, sizeof path, "%s/MFC/RESOURCE.H", base); parseResourceH(path);
    /* S124: installed-build PE resources first (the BDG 0.99 parity oracle; also the
       packaging story — no source checkout needed when the PE covers a dialog). */
    if (!getenv("BOB_NO_PE_RSRC")) g_pe = loadFromPE();
    /* positions: MIG.RC then BOB.RC (BOB.RC's IDD_3DI is the real config dialog -> last wins).
       With PE data loaded these fill only the pairs the installed resources lack. */
    snprintf(path, sizeof path, "%s/ENGLISH/MIG.RC", base); parseRc(path);
    snprintf(path, sizeof path, "%s/ENGLISH/BOB.RC", base); parseRc(path);
    /* design-time captions (labels) from the DLGINIT property bags */
    snprintf(path, sizeof path, "%s/ENGLISH/BOB.RC", base); parseDlgInit(path);
    snprintf(path, sizeof path, "%s/ENGLISH/MIG.RC", base); parseDlgInit(path);
    if (getenv("BOB_TRACE_OLE"))
        fprintf(stderr, "[ole] dialog templates: %d symbols, %d rects, %d captions, %d arts from %s%s\n",
                g_nsyms, g_nrects, g_ncaps, g_narts, base, g_pe ? " (+PE oracle)" : "");
}

/* S124: is (dlgId, ctrlId) part of the installed build's template for dlgId?
   1 = yes; 0 = the PE covers this dialog but the control is NOT in its template
   (a source-only control — on Windows the dialog manager would never create it,
   e.g. CSSound's IDC_CBO_MUSIC/SFX2/SFX3 which BDG 0.99 dropped from IDD_SSOUND);
   -1 = the PE doesn't cover this dialog (no filtering possible). */
extern "C" int bob_dlg_in_template(int dlgId, int ctrlId) {
    load();
    if (!g_pe || dlgId <= 0) return -1;
    int seen = 0;
    for (int i = 0; i < g_nrects; i++) if (g_rects[i].dlgId == dlgId && g_rects[i].pe) {
        if (g_rects[i].id == ctrlId) return 1;
        seen = 1;
    }
    return seen ? 0 : -1;
}

/* S124: the PE template's RStatic control ids for a dialog — the labels the game
   never DDX_Control-binds (on Windows the dialog manager creates EVERY template
   item; our DDX-driven creation missed them, e.g. the whole Sim-Config Mission
   tab label column). Returns the count written to ids[]. */
extern "C" int bob_dlg_enum_statics(int dlgId, int* ids, int maxn) {
    load();
    int n = 0;
    for (int i = 0; i < g_nrects && n < maxn; i++)
        if (g_rects[i].dlgId == dlgId && g_rects[i].pe && g_rects[i].kind == K_RSTATIC)
            ids[n++] = g_rects[i].id;
    return n;
}

/* S136: the PE template's RButton control ids for a dialog — buttons the game never
   DDX_Control-binds (like IDC_RETURNTOPLAYER on IDD_BOBFRAG, gold #3). Same rationale as
   bob_dlg_enum_statics: Windows' dialog manager creates every template item; our DDX-driven
   creation misses the non-bound ones. Returns the count written to ids[]. */
extern "C" int bob_dlg_enum_buttons(int dlgId, int* ids, int maxn) {
    load();
    int n = 0;
    for (int i = 0; i < g_nrects && n < maxn; i++)
        if (g_rects[i].dlgId == dlgId && g_rects[i].pe && g_rects[i].kind == K_RBUTTON)
            ids[n++] = g_rects[i].id;
    return n;
}

/* S176 (PO: "the campaign resolution dropdown is missing"): the PE template's RCombo ids.
   Third instance of one gap. On Windows the dialog manager creates EVERY template item; this port
   creates them from DDX_Control bindings, so any template control the game does not bind is simply
   absent. S124 fixed that for STATICS (the Sim-Config label column) and S136 for BUTTONS
   (IDC_RETURNTOPLAYER) -- each completing one KIND and leaving the rest. Combos were never done, so
   every unbound template combo in the whole front end is missing, of which the GFX screen's
   Campaign Resolution is the one a player happens to notice. Same filter, K_RCOMBO. */
extern "C" int bob_dlg_enum_combos(int dlgId, int* ids, int maxn) {
    load();
    int n = 0;
    for (int i = 0; i < g_nrects && n < maxn; i++)
        if (g_rects[i].dlgId == dlgId && g_rects[i].pe && g_rects[i].kind == K_RCOMBO)
            ids[n++] = g_rects[i].id;
    return n;
}

extern "C" int bob_load_string(void* h, unsigned id, char* buf, int maxlen);   /* bob_resources.cpp */

extern "C" int bob_dlg_caption(int dlgId, int ctrlId, char* out, int outsz) {
    load();
    for (int i = 0; i < g_ncaps; i++) if (g_caps[i].dlgId == dlgId && g_caps[i].ctrlId == ctrlId) {
        if (out && outsz > 0) {
            out[0] = 0;
            /* S124: faithful runtime path first — resolve the persisted IDS_* name via
               RESOURCE.H and read the language DLL's (BDG-patched) string table, exactly
               what CRStaticCtrl's WM_GETSTRING does on Windows ("Gamma Level" vs the
               design-time literal "Gamma Correction"). Fall back to the literal. */
            if (g_caps[i].ids[0]) {
                int sid = symLookup(g_caps[i].ids);
                if (sid > 0) bob_load_string(NULL, (unsigned)sid, out, outsz);
            }
            if (!out[0]) { strncpy(out, g_caps[i].s, outsz - 1); out[outsz - 1] = 0; }
        }
        return 1;
    }
    return 0;
}

/* S181: the dialog's own template size in DLU (0 if the template is unknown). */
extern "C" int bob_dlg_dialog_size(int dlgId, int* w, int* h) {
    load();
    for (int i = 0; i < g_ndlgsz; i++) if (g_dlgsz[i].dlgId == dlgId) {
        if (w) *w = g_dlgsz[i].w; if (h) *h = g_dlgsz[i].h;
        return 1;
    }
    return 0;
}
extern "C" int bob_dlg_lookup(int ctrlId, int* x, int* y, int* w, int* h) {
    load();
    for (int i = 0; i < g_nrects; i++) if (g_rects[i].id == ctrlId) {
        if (x) *x = g_rects[i].x; if (y) *y = g_rects[i].y;
        if (w) *w = g_rects[i].w; if (h) *h = g_rects[i].h;
        return 1;
    }
    return 0;
}
/* Dialog-scoped rect: prefer the (dlgId,ctrlId) entry (disambiguates shared control ids across
   dialogs, e.g. the toolbars' IDC_PAUSE), falling back to any-dialog by-id. */
extern "C" int bob_dlg_lookup_in(int dlgId, int ctrlId, int* x, int* y, int* w, int* h) {
    load();
    for (int i = 0; i < g_nrects; i++) if (g_rects[i].id == ctrlId && g_rects[i].dlgId == dlgId) {
        if (x) *x = g_rects[i].x; if (y) *y = g_rects[i].y;
        if (w) *w = g_rects[i].w; if (h) *h = g_rects[i].h;
        return 1;
    }
    return bob_dlg_lookup(ctrlId, x, y, w, h);
}
