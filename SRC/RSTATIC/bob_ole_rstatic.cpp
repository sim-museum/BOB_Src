/* RStatic host: the genuine CRStaticCtrl + dispid routing. Separate TU for its
   own resource context. RStatic is a label: text comes from SetString (runtime,
   used by campaign/name/reminder screens) or a design-time ResourceNumber (the
   config labels — not yet available without the DLGINIT property bag). */
#include "stdafx.h"
#include "RStaticC.h"           /* the genuine CRStaticCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>
#include <set>
#include <cstdlib>
#include <cstring>

extern "C" int bob_load_string(void* h, unsigned id, char* buf, int maxlen);

extern int g_bobListFontH;

extern const GUID IID_DRStatic       = { 0xc42bac3e, 0xca3c, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const GUID IID_DRStaticEvents = { 0xc42bac3f, 0xca3c, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

struct HostRStatic : public CRStaticCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    void applyDesignProps() override {
        /* S126: replay the genuine persisted property stream (stock Caption/ForeColor,
           FontNum, design String, ResourceNumber, ShadowColor) through the control's
           own DoPropExchange — the full CPropExchange surrogate. m_hWnd=0 during the
           replay: on Windows loading precedes window creation. */
        const unsigned char* p; int n; int streamed = 0;
        if (bob_dlg_propbag(dlgId, ctrlId, &p, &n)) {
            CPropExchange px;
            if (px.Attach(p, n)) {
                HWND save = m_hWnd; m_hWnd = 0;
                DoPropExchange(&px);
                m_hWnd = save;
                streamed = 1;
                if (bob_ole_trace()) fprintf(stderr,
                    "[ole] RStatic dlg=%d id=%d stream: FontNum=%ld ResNum=%ld fore=%06lx \"%s\"\n",
                    dlgId, ctrlId, (long)GetFontNum(), (long)GetResourceNumber(),
                    (unsigned long)GetForeColor(), (const char*)m_string);
            }
        }
        if (streamed && GetResourceNumber()) { capDesign = m_string; return; }
        /* runtime caption resolves genuinely: GetParentWndInfo -> WM_GETSTRING
           (ResourceNumber) -> BDG string table, at first draw. Without a stream
           (BOB_NO_PROP_STREAM) or without a ResourceNumber, keep the S124 caption
           path (IDS-name -> string table, falling back to the design literal). */
        char cap[64];
        if (bob_dlg_caption(dlgId, ctrlId, cap, sizeof cap) && cap[0]) SetString(cap);
        capDesign = m_string;      /* S2: what the design/bag path left behind */
    }
    CString capDesign;             /* S2: see cappath_report */
    /* ---- XPORT-CAPTION-1 S2 (2026-09-16): WHICH path does each caption take? ----
     *
     * applyDesignProps above early-returns on `streamed && GetResourceNumber()`, leaving the
     * caption to the control's own CRStaticCtrl::GetParentWndInfo -> WM_GETSTRING at first
     * draw; everything else falls through to bob_dlg_caption (the DLGINIT bag, which itself
     * prefers the string table over the design literal). S1 counted neither, and the two can
     * disagree -- the whole reason S124 preferred the runtime path is that the design literal
     * says "Gamma Correction" where the shipped game says "Gamma Level".
     *
     * Reported AFTER OnDraw, because the WM_GETSTRING fetch happens inside it. Three columns,
     * so a disagreement is visible rather than inferred:
     *   shown  -- what m_string actually holds now, i.e. what the player sees
     *   table  -- what the string table returns for this control's ResourceNumber
     *   bag    -- what bob_dlg_caption would have supplied
     * One line per (dlg,id). BOB_TRACE_CAPPATH=1. */
    void cappath_report() {
        static std::set<long> seen;
        long key = (long)dlgId * 100000L + (long)ctrlId;
        if (seen.count(key)) return;
        seen.insert(key);
        long rn = (long)GetResourceNumber();
        char table[100]; table[0] = 0;
        int ntab = rn ? bob_load_string(NULL, (unsigned)rn, table, (int)sizeof table) : -1;
        char bag[100]; bag[0] = 0;
        int haveBag = bob_dlg_caption(dlgId, ctrlId, bag, (int)sizeof bag);
        const char* shown = (const char*)m_string;
        if (!shown) shown = "";
        /* CLASSIFY BY OBSERVATION, not by inference from ResourceNumber. A first cut of this
           instrument labelled the path `rn ? "WM_GETSTRING" : "bag"` and reported 66/66
           WM_GETSTRING -- which is wrong for at least two controls, because IDS_NONE is 8
           (SRC/MFC/RESOURCE.H:15), so a control can carry a NON-ZERO ResourceNumber and still
           have CRStaticCtrl::GetParentWndInfo write "" into m_string. There is also a THIRD
           source the two-way split has no room for: a runtime SetString from the screen's own
           code, which is what actually supplies the long phase/training descriptions. */
        const char* des = (const char*)capDesign; if (!des) des = "";
        const char* path;
        if (ntab > 0 && strcmp(shown, table) == 0)            path = "WM_GETSTRING";
        else if (shown[0] && strcmp(shown, des) == 0)         path = "design/bag";
        else if (!shown[0])                                   path = "(empty)";
        else                                                  path = "runtime";
        /* AGREE/DISAGREE is about the two DESIGN-TIME SOURCES, not about which one won: a
           control whose table text and bag text differ is one where the choice changes the
           screen. `runtime` controls are marked n/a -- neither source reaches the player. */
        const char* verdict = "n/a";
        if (rn && rn != 8 /*IDS_NONE*/ && haveBag && ntab > 0)
            verdict = (strcmp(table, bag) == 0) ? "AGREE" : "DISAGREE";
        /* newlines in a caption would split the record and hide its verdict (the phase
           description did exactly that on the first run) -- flatten them. */
        char flat[256]; int fo = 0;
        for (const char* q = shown; *q && fo < (int)sizeof flat - 4; q++)
            flat[fo++] = (*q == '\n' || *q == '\r') ? ' ' : *q;
        flat[fo] = 0;
        fprintf(stderr, "[cappath] dlg=%d id=%d resnum=%ld path=%-12s shown=\"%s\" table=%d:\"%s\" bag=%d:\"%s\" %s\n",
                dlgId, ctrlId, rn, path, flat, ntab, table, haveBag, bag, verdict);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        OnDraw(pdc, rc, rc);
        if (getenv("BOB_TRACE_CAPPATH")) cappath_report();
    }
    void dispatch(DISPID id, VARTYPE, void*, va_list) override {
        if (bob_ole_trace()) fprintf(stderr, "[ole] RStatic: unhandled method dispid %ld\n", (long)id);
    }
    void setprop(DISPID id, va_list ap) override {
        /* String/Caption are BSTR; the rest are long/BOOL/COLOR */
        if (id == 3 || id == DISPID_CAPTION_ || id == DISPID_TEXT_) {
            const char* t = va_arg(ap, const char*);
            if (id == 3) SetString(t ? t : ""); else InternalSetText(t ? t : "");
            return;
        }
        long v = va_arg(ap, long);
        switch (id) {
        case 1: /* UpdateCaption */ break;
        case 2: SetFontNum(v); break;
        case 4: SetResourceNumber(v); break;
        case 5: SetPictureFileNum(v); break;
        case 6: SetCentral((BOOL)v); break;
        case 7: SetShadowColor((OLE_COLOR)v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RStatic: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 2: *(long*)pvRet = GetFontNum(); break;
        case 4: *(long*)pvRet = GetResourceNumber(); break;
        case 6: *(BOOL*)pvRet = GetCentral(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RStatic: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rstatic(CWnd* parent) { HostRStatic* h = new HostRStatic(); h->boot(parent); return h; }
