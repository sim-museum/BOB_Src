/* REdtBt host: the genuine CREdtBtCtrl (edit-button control) + dispid routing.
   S140 (#3 gold, the "Bob" pilot name box): BoBFrag's pilot slots IDC_PILOT_0..14 are
   CREdtBt (an edit-button), DDX_Control-bound; BoBFrag::OnInitDialog does
   GETDLGITEM(IDC_PILOT_[i])->SetCaption(playerslotname / leadername) to fill each pilot's
   name, and the PLAYER's slot shows "Bob" (Save_Data.CommsPlayerName) -- gold #3's name box.
   Hosting the genuine control renders those names via its own OnDraw onto the panel.

   Mirrors bob_ole_redit.cpp on the OleHost interface. Two CREdtBt-specific notes:
   (1) its Caption is a STOCK property -- CREdtBt::SetCaption -> SetProperty(DISPID_CAPTION,..)
       (REDTBT.CPP), unlike CREdit's custom dispid 3 -- and compat's CWnd::SetText is a no-op,
       so the caption is set via InternalSetText (m_bobText, read by InternalGetText).
   (2) OnDraw draws the member `captiontext`, which the genuine control only refreshes in its
       handlers/OnTextChanged (REDTBTC.CPP:536/671), NOT inside OnDraw -- so refresh it from
       InternalGetText() in draw() before calling OnDraw. */
#include "stdafx.h"
#include "REDTBTC.H"           /* the genuine CREdtBtCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

struct HostREdtBt : public CREdtBtCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    /* S126: replay the genuine persisted DLGINIT property stream (FontNum/Shadow/ForeColor). */
    void applyDesignProps() override {
        const unsigned char* bp; int bn;
        if (!bob_dlg_propbag(dlgId, ctrlId, &bp, &bn)) return;
        CPropExchange px;
        if (!px.Attach(bp, bn)) return;
        HWND save = m_hWnd; m_hWnd = 0;
        DoPropExchange(&px);
        m_hWnd = save;
        if (bob_ole_trace()) fprintf(stderr,
            "[ole] REdtBt dlg=%d id=%d stream: FontNum=%ld\n", dlgId, ctrlId, (long)m_FontNum);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        m_FirstSweep = TRUE;                 /* skip parent-artwork re-blit (panel bg already drawn) */
        captiontext = InternalGetText();     /* OnDraw draws this member; only refreshed in handlers */
        OnDraw(pdc, rc, rc);
    }
    void dispatch(DISPID id, VARTYPE, void*, va_list) override {
        if (bob_ole_trace()) fprintf(stderr, "[ole] REdtBt: unhandled method dispid %ld\n", (long)id);
    }
    void setprop(DISPID id, va_list ap) override {
        if (id == DISPID_CAPTION_ || id == DISPID_TEXT_) {   /* CREdtBt::SetCaption -> stock caption */
            const char* t = va_arg(ap, const char*);
            InternalSetText(t ? t : "");
            return;
        }
        long v = va_arg(ap, long);
        switch (id) {
        case 1: SetFontNum(v); break;                        /* dispatch map order: 1 FontNum */
        case 3: SetShadow(v); break;                         /* 3 Shadow (2 = DragAndDropID) */
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] REdtBt: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetFontNum(); break;
        case 3: *(long*)pvRet = GetShadow(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] REdtBt: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_redtbt(CWnd* parent) { HostREdtBt* h = new HostREdtBt(); h->boot(parent); return h; }
