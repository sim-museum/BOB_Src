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
            if (dp) { char dsym[SYM_LEN]; if (sscanf(p, " %47[A-Za-z0-9_]", dsym) == 1) g_curDlgId = symLookup(dsym); }
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
     the gold campaignentername layout. `BOB_NO_DLGINIT_PROPS` reverts. ---- */
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
static void extractProps(const unsigned char* b, int n, int dlgId, int ctrlId) {
    /* bag header: DWORD unicode-char count + UTF-16 licence string, then
       WORD verMinor, WORD verMajor (ExchangeVersion). */
    if (n < 12) return;
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
