/* RRadio host: the genuine CRRadioCtrl + dispid routing. Separate TU for its own
   RRADIO/resource.h context. S128 (#2 Quick-Shots page tabs): CSQuick1::OnInitDialog
   binds IDC_RRADIO as a CRRadio and AddButton()s the page tabs ("Scenario" /
   "Parameters" / "UN" / "RED"); hosting the genuine control draws the tab captions +
   the selection tick via its own OnDraw onto the panel framebuffer. Mirrors
   bob_ole_redit.cpp / bob_ole_rbutton.cpp on BoB's OleHost interface.
   Dispids (from the CRRadio wrapper, SRC/MFC/RRADIO.CPP): 1 FontNum, 2 Cols,
   3 CurrentSelection, 4 ColumnWidth, 5 AddButton (BSTR method), 6 Clear; stock
   ForeColor negative. (RRADIOC.CPP defines IID_DRRadio internally; no extern needed.) */
#include "stdafx.h"
#include "RRadioC.h"             /* the genuine CRRadioCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

struct HostRRadio : public CRRadioCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    /* S126-style: replay the persisted DLGINIT property stream (FontNum/Cols/
       ColumnWidth + stock ForeColor) through the control's own DoPropExchange.
       m_hWnd=0 during the replay: on Windows loading precedes window creation. */
    void applyDesignProps() override {
        const unsigned char* bp; int bn;
        if (!bob_dlg_propbag(dlgId, ctrlId, &bp, &bn)) return;
        CPropExchange px;
        if (!px.Attach(bp, bn)) return;
        HWND save = m_hWnd; m_hWnd = 0;
        DoPropExchange(&px);
        m_hWnd = save;
        if (bob_ole_trace()) fprintf(stderr,
            "[ole] RRadio dlg=%d id=%d stream: FontNum=%ld Cols=%ld ColW=%ld\n",
            dlgId, ctrlId, (long)m_FontNum, (long)m_Cols, (long)m_ColumnWidth);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        CRect rc(0, 0, w, h);
        /* m_FirstSweep=TRUE skips the WM_GETARTWORK/offscreen-DC path AND the
           !m_hWnd black-fill (OnDraw line 337) -- the panel background is already
           drawn; the same direct-to-pdc convention the RButton/REdit hosts use. */
        m_FirstSweep = TRUE;
        OnDraw(pdc, rc, rc);
    }
    /* S129: a click on the tab row selects the button under the cursor. The buttons are
       laid out left-to-right in equal columns (OnDraw: x += m_ColumnWidth*avgCharWidth,
       m_Cols columns), so map local X across the drawn width to the button index; set the
       selection (updates m_CurSel + the tick) and return the index so bob_ole_click can
       fire the genuine Selected(index) event -> CSQuick1::OnSelectedRradio -> page switch. */
    int onButtonClick(int localX) override {
        int n = (int)m_list.GetCount();
        if (n <= 0 || sw <= 0) return -1;
        int idx = localX * n / sw;
        if (idx < 0) idx = 0; if (idx >= n) idx = n - 1;
        SetCurrentSelection(idx);
        if (bob_ole_trace()) fprintf(stderr, "[ole]   RRadio button click localX=%d/%d -> idx=%d\n", localX, sw, idx);
        return idx;
    }
    void dispatch(DISPID id, VARTYPE, void* /*pvRet*/, va_list ap) override {
        switch (id) {
        case 5: { const char* t = va_arg(ap, const char*); AddButton(t ? t : "");
                  if (bob_ole_trace()) fprintf(stderr, "[ole]   RRadio AddButton \"%s\" (n=%d)\n",
                      t ? t : "", (int)m_list.GetCount()); } break;
        case 6: Clear(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RRadio: unhandled method dispid %ld\n", (long)id); break;
        }
    }
    void setprop(DISPID id, va_list ap) override {
        long v = va_arg(ap, long);
        switch (id) {
        case 1: SetFontNum(v); break;
        case 2: SetCols(v); break;
        case 3: SetCurrentSelection(v); break;
        case 4: SetColumnWidth(v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RRadio: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetFontNum(); break;
        case 2: *(long*)pvRet = GetCols(); break;
        case 3: *(long*)pvRet = GetCurrentSelection(); break;
        case 4: *(long*)pvRet = GetColumnWidth(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RRadio: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rradio(CWnd* parent) { HostRRadio* h = new HostRRadio(); h->boot(parent); return h; }
