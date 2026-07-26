/* REdit host: the genuine CREditCtrl + dispid routing. Separate TU for its own
   REDIT/resource.h context. S125 (#17 campaign enter-name): CampaignEnterName
   DDX-binds m_IDC_NAME as the CREdit wrapper; hosting the genuine control lets
   OnInitDialog's GETDLGITEM(IDC_NAME)->SetCaption("\b\t\n\v") (blocking keys) +
   SetCaption(Save_Data.CommsPlayerName) reach the real word-list machinery, and
   OnDraw renders the name text + caret line onto the panel like Windows.
   Pattern adopted from the MiG Alley port's ma_oleedit.cpp (note 13/14 traffic),
   recast on BoB's OleHost interface (bob_ole_host.h). Dispids follow
   REDITCTL.CPP's dispatch map: 1 FontNum, 2 Shadow, 3 Caption (BSTR); stock
   ForeColor/BackColor/Enabled negative. */
#include "stdafx.h"
#include "REditCtl.h"           /* the genuine CREditCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

/* REdit dispatch/event IIDs (values from REdit.odl; used by-address only). */
extern const GUID IID_DREdit       = { 0x499e2be4, 0xac32, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };
extern const GUID IID_DREditEvents = { 0x499e2be5, 0xac32, 0x11d6, { 0xa1,0xf0,0x44,0x45,0x53,0x54,0,0 } };

struct HostREdit : public CREditCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        /* m_FirstSweep=TRUE skips the parent-artwork background re-blit (the
           WM_GETARTWORK path; the panel background is already drawn) -- the same
           direct-to-pdc convention the RButton host uses. */
        m_FirstSweep = TRUE;
        OnDraw(pdc, rc, rc);
    }
    void dispatch(DISPID id, VARTYPE, void*, va_list) override {
        if (bob_ole_trace()) fprintf(stderr, "[ole] REdit: unhandled method dispid %ld\n", (long)id);
    }
    void setprop(DISPID id, va_list ap) override {
        if (id == 3 || id == DISPID_CAPTION_ || id == DISPID_TEXT_) {
            const char* t = va_arg(ap, const char*);
            SetCaption(t ? t : "");            /* '\b'-prefixed -> SetBlockingKeys */
            return;
        }
        long v = va_arg(ap, long);
        switch (id) {
        case 1: SetFontNum(v); break;
        case 2: SetShadow(v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        case DISPID_ENABLED_:   SetEnabled((BOOL)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] REdit: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetFontNum(); break;
        case 2: *(long*)pvRet = GetShadow(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] REdit: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_redit(CWnd* parent) { HostREdit* h = new HostREdit(); h->boot(parent); return h; }
