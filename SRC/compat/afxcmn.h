/* FreeFalcon Linux Port - afxcmn.h (MFC common controls shim) */
#ifndef FF_STUB_afxcmn_h
#define FF_STUB_afxcmn_h

/* Minimal common-control stubs. The R* ActiveX control *property pages*
   (design-time only, never instantiated at runtime) declare these as members,
   so the headers must parse. They are CWnd-derived no-ops. */
#ifndef BOB_AFXCMN_STUBS
#define BOB_AFXCMN_STUBS
class CSpinButtonCtrl : public CWnd {
public:
    int  SetPos(int n) { return n; }
    int  GetPos() const { return 0; }
    DWORD SetRange(short, short) { return 0; }
    DWORD SetRange32(int, int) { return 0; }
    CWnd* SetBuddy(CWnd*) { return NULL; }
};
class CProgressCtrl : public CWnd {
public:
    void SetRange(short, short) {}
    int  SetPos(int n) { return n; }
    int  StepIt() { return 0; }
};
class CSliderCtrl : public CWnd {
public:
    void SetRange(int, int, BOOL = FALSE) {}
    void SetPos(int) {}
    int  GetPos() const { return 0; }
};
class CToolTipCtrl : public CWnd {
public:
    BOOL Create(CWnd*, DWORD = 0) { return TRUE; }
    BOOL AddTool(CWnd*, LPCSTR, LPCRECT = NULL, UINT = 0) { return TRUE; }
    void Activate(BOOL) {}
};
#endif /* BOB_AFXCMN_STUBS */

#endif
