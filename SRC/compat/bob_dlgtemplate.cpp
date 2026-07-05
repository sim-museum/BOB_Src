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

#ifndef BOB_SRC_DIR
#define BOB_SRC_DIR "."
#endif

#define MAX_SYMS   20000
#define MAX_RECTS   4096
#define SYM_LEN       48

struct Sym  { char name[SYM_LEN]; int id; };
struct CR   { int id, dlgId, x, y, w, h; };

static Sym  g_syms[MAX_SYMS];   static int g_nsyms  = 0;
static CR   g_rects[MAX_RECTS]; static int g_nrects = 0;
static int  g_loaded = 0;

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
           (dlgId,id) was already seen; else append. */
        for (int i = 0; i < g_nrects; i++) if (g_rects[i].id == id && g_rects[i].dlgId == g_curDlgId) {
            g_rects[i].x = nums[nn-4]; g_rects[i].y = nums[nn-3];
            g_rects[i].w = nums[nn-2]; g_rects[i].h = nums[nn-1]; return;
        }
        g_rects[g_nrects].id = id; g_rects[g_nrects].dlgId = g_curDlgId;
        g_rects[g_nrects].x = nums[nn-4]; g_rects[g_nrects].y = nums[nn-3];
        g_rects[g_nrects].w = nums[nn-2]; g_rects[g_nrects].h = nums[nn-1];
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
struct Cap { int dlgId, ctrlId; char s[64]; };
static Cap g_caps[MAX_CAPS]; static int g_ncaps = 0;

static void storeCaption(int dlgId, int ctrlId, const char* s) {
    if (!s[0]) return;
    for (int j = 0; j < g_ncaps; j++) if (g_caps[j].dlgId == dlgId && g_caps[j].ctrlId == ctrlId) {
        strncpy(g_caps[j].s, s, 63); g_caps[j].s[63] = 0; return; }
    if (g_ncaps >= MAX_CAPS) return;
    g_caps[g_ncaps].dlgId = dlgId; g_caps[g_ncaps].ctrlId = ctrlId;
    strncpy(g_caps[g_ncaps].s, s, 63); g_caps[g_ncaps].s[63] = 0; g_ncaps++;
}
/* the caption = the last length-prefixed printable string that isn't a resource id
   (ID*) or the licence ("Copyright ..."). */
static void extractCaption(const unsigned char* b, int n, int dlgId, int ctrlId) {
    char best[64]; best[0] = 0;
    for (int i = 0; i < n; i++) {
        int len = b[i];
        if (len < 2 || len > 62 || i + 1 + len > n) continue;
        int ok = 1; for (int k = 0; k < len; k++) { unsigned char c = b[i+1+k]; if (c < 32 || c > 126) { ok = 0; break; } }
        if (!ok) continue;
        char t[64]; memcpy(t, b + i + 1, len); t[len] = 0;
        if (strncmp(t, "ID", 2) == 0 || strncmp(t, "Copyright", 9) == 0) continue;
        strcpy(best, t);   /* keep the last qualifying string */
    }
    if (best[0]) storeCaption(dlgId, ctrlId, best);
}

/* ---- button art (S89): the R* buttons' NormalFileNumString is a "FIL_..." equate stored
   in the same DLGINIT property bag; capture it per (dlg,ctrl) so HostRButton can resolve it
   to a FileNum via GetFileNum. Distinct from the caption (which is the hint text). ---- */
struct Art { int dlgId, ctrlId; char s[48]; };
static Art g_arts[MAX_CAPS]; static int g_narts = 0;
static void storeArt(int dlgId, int ctrlId, const char* s) {
    for (int j = 0; j < g_narts; j++) if (g_arts[j].dlgId==dlgId && g_arts[j].ctrlId==ctrlId) {
        strncpy(g_arts[j].s, s, 47); g_arts[j].s[47]=0; return; }
    if (g_narts >= MAX_CAPS) return;
    g_arts[g_narts].dlgId=dlgId; g_arts[g_narts].ctrlId=ctrlId;
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
static void load(void);
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
            if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl);
            char sym[256]; curDlg = (sscanf(line, " %255[A-Za-z0-9_]", sym) == 1) ? symLookup(sym) : -1;
            curCtrl = -1; blen = 0;
            continue;
        }
        char cs[256];
        if (curDlg >= 0 && sscanf(line, " %255[A-Za-z0-9_] ,", cs) == 1 && strstr(line, "0x376")) {
            if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl);   /* flush previous */
            curCtrl = symLookup(cs); blen = 0;
            continue;
        }
        {   /* trimmed "END" closes the block */
            const char* p = line; while (*p && isspace((unsigned char)*p)) p++;
            if (strncmp(p, "END", 3) == 0 && (p[3] == 0 || isspace((unsigned char)p[3]))) {
                if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl);
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
    if (curCtrl >= 0) extractCaption(buf, blen, curDlg, curCtrl); extractArtName(buf, blen, curDlg, curCtrl);
    fclose(f);
}

static void load(void) {
    if (g_loaded) return;
    g_loaded = 1;
    const char* base = getenv("BOB_RC_DIR"); if (!base) base = BOB_SRC_DIR;
    char path[1024];
    snprintf(path, sizeof path, "%s/MFC/RESOURCE.H", base); parseResourceH(path);
    /* positions: MIG.RC then BOB.RC (BOB.RC's IDD_3DI is the real config dialog -> last wins) */
    snprintf(path, sizeof path, "%s/ENGLISH/MIG.RC", base); parseRc(path);
    snprintf(path, sizeof path, "%s/ENGLISH/BOB.RC", base); parseRc(path);
    /* design-time captions (labels) from the DLGINIT property bags */
    snprintf(path, sizeof path, "%s/ENGLISH/BOB.RC", base); parseDlgInit(path);
    snprintf(path, sizeof path, "%s/ENGLISH/MIG.RC", base); parseDlgInit(path);
    if (getenv("BOB_TRACE_OLE"))
        fprintf(stderr, "[ole] dialog templates: %d symbols, %d rects, %d captions, %d arts from %s\n",
                g_nsyms, g_nrects, g_ncaps, g_narts, base);
}

extern "C" int bob_dlg_caption(int dlgId, int ctrlId, char* out, int outsz) {
    load();
    for (int i = 0; i < g_ncaps; i++) if (g_caps[i].dlgId == dlgId && g_caps[i].ctrlId == ctrlId) {
        if (out && outsz > 0) { strncpy(out, g_caps[i].s, outsz - 1); out[outsz - 1] = 0; }
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
