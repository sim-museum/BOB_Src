/* RSpinBut host: the genuine CRSpinButCtrl (spin/up-down value control) + dispid routing.
   S142 (gold #18): the LW Directives allocation grid is built almost entirely out of
   CRSpinBut — ~50 of them (LWDIRECT.H: IDC_RSPINBUT_MORN/_MID, the Ju87/He111/Ju88/Do17
   x target-row matrix, the escort/reconn counts). It was the 8th and LAST R* control type
   with no host, so every wrapper InvokeHelper on it was a silent no-op and the grid drew
   its labels but none of its numbers. This host completes the R* set.

   Follows the §8p recipe (dispids from the WRAPPER, SRC/MFC/RSPINBUT.CPP):
     stock ForeColor | 0x1 RepeatDelay | 0x2 Index | 0x3 FontNum | 0x4 CurrentValue
     0x5 AddString(BSTR) | 0x6 DeleteString(I4) | 0x7 Clear
     0x8 SetPriceOption(I4,I4,I4)->BOOL | 0x9 SetValueOption(...)->BOOL
     0xa SetPassWord(BOOL) | 0xb SetSearchValueOption(...)->BOOL
     0xc SetPlayerNegPriceOption(...)->BOOL

   TWO CRSpinBut-specific traps this control has and its siblings don't (RSPINBTC.CPP):

   (1) m_bDrawing is a STATIC (class-wide) reentrancy flag, and it is cleared only inside
       DrawBitmap() -- NOT at the end of OnDraw. OnDraw's `if (m_bDrawing || !m_hWnd) return;`
       therefore latches: any draw that takes the artnum==0 branch (FillRect black) leaves the
       flag TRUE forever, and EVERY subsequent spinner on EVERY later frame returns immediately.
       With ~50 instances on one dialog that is the difference between a grid and one lonely box.
       Neutralised here, host-side, by clearing it before each draw -- no game-code edit, and it
       cannot latch even if the art lookup fails.

   (2) It has no m_FirstSweep. Every other hosted R* uses that flag to skip the parent-artwork
       re-blit (the panel background is already drawn by the time we drive OnDraw, and our
       WM_GETXYOFFSET/GetWindowRect are degenerate so the re-blit would land at the wrong
       offset). CRSpinBut re-blits unconditionally -- but harmlessly here: DrawBitmap fetches
       the art via WM_GETFILE and no-ops unless it gets a 'BM' blob back, and it clears the
       static flag on the way out either way. Verified by capture, not by assumption. */
#include "stdafx.h"
#include "RSPINBTC.H"          /* the genuine CRSpinButCtrl */
#include "../RLISTBOX/bob_ole_host.h"
#include <cstdarg>
#include <cstdio>

extern int g_bobListFontH;

struct HostRSpinBut : public CRSpinButCtrl, public OleHost {
    void boot(CWnd* parent) {
        m_pBobParent = parent;
        m_hWnd = (HWND)1;                    /* OnDraw bails on !m_hWnd (and skips the arrows) */
        OnResetState();
        CPropExchange px; DoPropExchange(&px);
    }
    /* S126: replay the genuine persisted DLGINIT property stream (RepeatDelay/FontNum). */
    void applyDesignProps() override {
        const unsigned char* bp; int bn;
        if (!bob_dlg_propbag(dlgId, ctrlId, &bp, &bn)) return;
        CPropExchange px;
        if (!px.Attach(bp, bn)) return;
        HWND save = m_hWnd; m_hWnd = 0;
        DoPropExchange(&px);
        m_hWnd = save;
        if (bob_ole_trace()) fprintf(stderr,
            "[ole] RSpinBut dlg=%d id=%d stream: FontNum=%ld\n", dlgId, ctrlId, (long)m_FontNum);
    }
    /* S197: report OUR drawn rect, not the whole window (see the note on CWnd::GetClientRect).
       CRSpinButCtrl::OnLButtonDown measures the arrow strip from `rect.right-15` and picks
       up-vs-down from `rect.bottom/2`; with the window-sized default every spin click was
       rejected before it began. */
    void GetClientRect(LPRECT r) const override {
        if (!r) return;
        r->left = r->top = 0;
        r->right  = sw > 0 ? sw : 16;
        r->bottom = sh > 0 ? sh : 16;
    }
    /* S197 (answering §8-MA111 in the affirmative for this type): a spin button now takes its own
       click. Drive the control's GENUINE OnLButtonDown -- it owns the arrow-strip test, the
       up/down split and the repeat timer, and it calls OnTimer itself to apply the change. The
       host supplies only the geometry that handler asks for, which is what was missing. */
    int onClickXY(int localX, int localY) override {
        /* the control's VALUE is m_index into m_list (OnTimer moves that, not m_CurrentValue --
           which is a separate price/value field). Report both, plus the list size, because an
           empty list makes both of OnTimer's guards false and the spinner cannot move at all. */
        long bi = m_index, bv = m_CurrentValue;
        OnLButtonDown(0, CPoint(localX, localY));
        if (bob_ole_trace())
            fprintf(stderr, "[ole] RSpinBut id=%d click local=(%d,%d) rect=%dx%d state=%d list=%d index %ld -> %ld (value %ld -> %ld)\n",
                    ctrlId, localX, localY, sw, sh, (int)m_SpinState, (int)m_list.GetCount(),
                    bi, (long)m_index, bv, (long)m_CurrentValue);
        return (m_index != bi) || (m_CurrentValue != bv);
    }
    void draw(CDC* pdc, int w, int h) override {
        g_bobListFontH = pdc->m_bobTextH;
        m_bDrawing = FALSE;                  /* trap (1): static, latches if a draw ever bails */
        CRect rc(0, 0, w, h);
        OnDraw(pdc, rc, rc);
        m_bDrawing = FALSE;                  /* and don't leave it set for the next instance */
    }
    void dispatch(DISPID id, VARTYPE, void* pvRet, va_list ap) override {
        switch (id) {
        case 5: { const char* t = va_arg(ap, const char*); AddString(t ? t : ""); } break;
        case 6: { long i = va_arg(ap, long); DeleteString(i); } break;
        case 7: Clear(); break;
        case 8: { long mn = va_arg(ap, long), mx = va_arg(ap, long), c = va_arg(ap, long);
                  BOOL r = SetPriceOption(mn, mx, c); if (pvRet) *(BOOL*)pvRet = r; } break;
        case 9: { long mn = va_arg(ap, long), mx = va_arg(ap, long), c = va_arg(ap, long);
                  BOOL r = SetValueOption(mn, mx, c); if (pvRet) *(BOOL*)pvRet = r;
                  if (bob_ole_trace()) fprintf(stderr,
                      "[ole]   RSpinBut id=%d SetValueOption(%ld,%ld,%ld)\n", ctrlId, mn, mx, c); } break;
        case 10: { long b = va_arg(ap, long); SetPassWord((BOOL)b); } break;
        case 11: { long mn = va_arg(ap, long), mx = va_arg(ap, long), c = va_arg(ap, long);
                   BOOL r = SetSearchValueOption(mn, mx, c); if (pvRet) *(BOOL*)pvRet = r; } break;
        case 12: { long mn = va_arg(ap, long), mx = va_arg(ap, long), c = va_arg(ap, long);
                   BOOL r = SetPlayerNegPriceOption(mn, mx, c); if (pvRet) *(BOOL*)pvRet = r; } break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RSpinBut: unhandled method dispid %ld\n", (long)id); break;
        }
    }
    void setprop(DISPID id, va_list ap) override {
        long v = va_arg(ap, long);
        switch (id) {
        case 1: SetRepeatDelay(v); break;
        case 2: SetIndex(v); break;
        case 3: SetFontNum(v); break;
        case 4: SetCurrentValue(v); break;
        case DISPID_FORECOLOR_: SetForeColor((OLE_COLOR)v); break;
        case DISPID_BACKCOLOR_: SetBackColor((OLE_COLOR)v); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RSpinBut: unhandled setprop dispid %ld\n", (long)id); break;
        }
    }
    void getprop(DISPID id, void* pvRet) override {
        if (!pvRet) return;
        switch (id) {
        case 1: *(long*)pvRet = GetRepeatDelay(); break;
        case 2: *(long*)pvRet = GetIndex(); break;
        case 3: *(long*)pvRet = GetFontNum(); break;
        case 4: *(long*)pvRet = GetCurrentValue(); break;
        default: if (bob_ole_trace()) fprintf(stderr, "[ole] RSpinBut: unhandled getprop dispid %ld\n", (long)id); break;
        }
    }
};

OleHost* bob_make_rspinbut(CWnd* parent) { HostRSpinBut* h = new HostRSpinBut(); h->boot(parent); return h; }
