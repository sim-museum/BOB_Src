/* FreeFalcon Linux Port - afxwin.h: minimal MFC compatibility layer.
 *
 * BoB uses MFC for its app/UI (CWinApp, CWnd, CDialog, CView, CFrameWnd, CDC...).
 * Neither bob's nor FreeFalcon's original afxwin.h has an implementation, so this
 * is a from-scratch minimal MFC: enough of the class hierarchy, message-map
 * macros and GDI/value types to COMPILE the MFC sources. Behaviour is stubbed
 * (windows never really open); the real UI is to be backed by SDL/GL at runtime.
 *
 * NOTE: CString is bob's OWN class (SRC/H/cstring.h), not provided here.
 */
#ifndef FF_COMPAT_AFXWIN_H
#define FF_COMPAT_AFXWIN_H
#ifdef FF_LINUX

/* bob headers gate on this (e.g. MIG.h: "#ifndef __AFXWIN_H__ #error ...") */
#define __AFXWIN_H__
#define __AFX_H__
#define _AFXWIN_H_

#include "windows.h"
#include "objbase.h"

/* ============================================================
 * Message-map / runtime-class macros — all no-ops. BoB's handlers are wired by
 * these on Windows; on Linux input/events are driven by SDL, so we drop them.
 * ============================================================ */
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(theClass, baseClass)
#define END_MESSAGE_MAP()
#define DECLARE_DYNAMIC(class_name)
#define IMPLEMENT_DYNAMIC(class_name, base_class)
#define DECLARE_DYNCREATE(class_name)
#define IMPLEMENT_DYNCREATE(class_name, base_class)
#define DECLARE_SERIAL(class_name)
#define IMPLEMENT_SERIAL(class_name, base_class, quan)
#define DECLARE_OLECREATE(class_name)
#define IMPLEMENT_OLECREATE(class_name, ext, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DECLARE_DISPATCH_MAP()
#define BEGIN_DISPATCH_MAP(theClass, baseClass)
#define END_DISPATCH_MAP()
#define DECLARE_INTERFACE_MAP()
#define BEGIN_INTERFACE_MAP(theClass, baseClass)
#define END_INTERFACE_MAP()
#define afx_msg
#define RUNTIME_CLASS(class_name) (NULL)

/* MFC diagnostic macros — no-ops (NDEBUG-style) */
#ifndef ASSERT
#define ASSERT(f)         ((void)0)
#endif
#define ASSERT_VALID(p)   ((void)0)
#define ASSERT_KINDOF(class_name, p) ((void)0)
#define VERIFY(f)         ((void)(f))
#define TRACE             (void)
#define TRACE0(s)         ((void)0)
#define TRACE1(s,a)       ((void)0)
#define TRACE2(s,a,b)     ((void)0)
#define TRACE3(s,a,b,c)   ((void)0)
#define TRACEN(s)         ((void)0)
#define DEBUG_ONLY(f)     ((void)0)

/* ON_* message-map entries (only valid inside BEGIN/END_MESSAGE_MAP, which are
   empty, but define them so stray expansions vanish too). */
#define ON_COMMAND(id, memberFxn)
#define ON_COMMAND_RANGE(id1, id2, memberFxn)
#define ON_UPDATE_COMMAND_UI(id, memberFxn)
#define ON_CONTROL(code, id, memberFxn)
#define ON_MESSAGE(message, memberFxn)
#define ON_NOTIFY(code, id, memberFxn)
#define ON_BN_CLICKED(id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn)
#define ON_WM_CREATE()
#define ON_WM_DESTROY()
#define ON_WM_PAINT()
#define ON_WM_SIZE()
#define ON_WM_TIMER()
#define ON_WM_CLOSE()
#define ON_WM_KEYDOWN()
#define ON_WM_KEYUP()
#define ON_WM_CHAR()
#define ON_WM_LBUTTONDOWN()
#define ON_WM_LBUTTONUP()
#define ON_WM_RBUTTONDOWN()
#define ON_WM_RBUTTONUP()
#define ON_WM_MOUSEMOVE()
#define ON_WM_ERASEBKGND()
#define ON_WM_SETFOCUS()
#define ON_WM_KILLFOCUS()
#define ON_WM_ACTIVATE()
#define ON_WM_ACTIVATEAPP()
#define ON_WM_SYSCOMMAND()
#define ON_WM_INITMENUPOPUP()
#define ON_WM_HSCROLL()
#define ON_WM_VSCROLL()

/* ============================================================
 * Value types
 * ============================================================ */
struct CSize : public SIZE {
    CSize() { cx = cy = 0; }
    CSize(int initCX, int initCY) { cx = initCX; cy = initCY; }
    CSize(SIZE s) { cx = s.cx; cy = s.cy; }
};

struct CPoint : public POINT {
    CPoint() { x = y = 0; }
    CPoint(int initX, int initY) { x = initX; y = initY; }
    CPoint(POINT p) { x = p.x; y = p.y; }
    void Offset(int dx, int dy) { x += dx; y += dy; }
};

struct CRect : public RECT {
    CRect() { left = top = right = bottom = 0; }
    CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    CRect(RECT r) { left = r.left; top = r.top; right = r.right; bottom = r.bottom; }
    int Width()  const { return right - left; }
    int Height() const { return bottom - top; }
    CPoint TopLeft() const { return CPoint(left, top); }
    CSize  Size()    const { return CSize(right - left, bottom - top); }
    void SetRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    void SetRectEmpty() { left = top = right = bottom = 0; }
    bool IsRectEmpty() const { return left == right || top == bottom; }
    bool PtInRect(POINT p) const { return p.x >= left && p.x < right && p.y >= top && p.y < bottom; }
    void OffsetRect(int dx, int dy) { left += dx; right += dx; top += dy; bottom += dy; }
    void InflateRect(int dx, int dy) { left -= dx; right += dx; top -= dy; bottom += dy; }
    CRect& operator+=(POINT p) { OffsetRect(p.x, p.y); return *this; }
    CRect& operator-=(POINT p) { OffsetRect(-p.x, -p.y); return *this; }
    operator LPRECT() { return this; }
    operator LPCRECT() const { return this; }
};

/* ============================================================
 * Object / command-target roots
 * ============================================================ */
class CObject {
public:
    CObject() {}
    virtual ~CObject() {}
    virtual void Serialize(class CArchive&) {}
};

class CCmdTarget : public CObject {
public:
    CCmdTarget() {}
};

/* ============================================================
 * GDI objects
 * ============================================================ */
class CGdiObject : public CObject {
public:
    HGDIOBJ m_hObject;
    CGdiObject() : m_hObject(NULL) {}
    HGDIOBJ GetSafeHandle() const { return m_hObject; }
    BOOL DeleteObject() { m_hObject = NULL; return TRUE; }
    BOOL Attach(HGDIOBJ h) { m_hObject = h; return TRUE; }
    HGDIOBJ Detach() { HGDIOBJ h = m_hObject; m_hObject = NULL; return h; }
};

class CFont : public CGdiObject {
public:
    BOOL CreateFontIndirect(const LOGFONT*) { return TRUE; }
    BOOL CreateFont(int, int, int, int, int, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, BYTE, LPCSTR) { return TRUE; }
    BOOL CreatePointFont(int, LPCSTR, CDC* = NULL);
    operator HFONT() const { return (HFONT)m_hObject; }
};

class CPen : public CGdiObject {
public:
    BOOL CreatePen(int, int, COLORREF) { return TRUE; }
    operator HPEN() const { return (HPEN)m_hObject; }
};

class CBrush : public CGdiObject {
public:
    BOOL CreateSolidBrush(COLORREF) { return TRUE; }
    operator HBRUSH() const { return (HBRUSH)m_hObject; }
};

class CBitmap : public CGdiObject {
public:
    BOOL CreateCompatibleBitmap(CDC*, int, int) { return TRUE; }
    operator HBITMAP() const { return (HBITMAP)m_hObject; }
};

class CDC : public CObject {
public:
    HDC m_hDC;
    CDC() : m_hDC(NULL) {}
    HDC GetSafeHdc() const { return m_hDC; }
    BOOL Attach(HDC h) { m_hDC = h; return TRUE; }
    HDC Detach() { HDC h = m_hDC; m_hDC = NULL; return h; }
    CGdiObject* SelectObject(CGdiObject*) { return NULL; }
    CFont* SelectObject(CFont*) { return NULL; }
    CPen*  SelectObject(CPen*)  { return NULL; }
    CBrush* SelectObject(CBrush*) { return NULL; }
    COLORREF SetTextColor(COLORREF c) { return c; }
    COLORREF SetBkColor(COLORREF c) { return c; }
    int SetBkMode(int) { return 0; }
    BOOL TextOutA(int, int, LPCSTR, int) { return TRUE; }
    BOOL TextOut(int x, int y, LPCSTR s, int n) { return TextOutA(x, y, s, n); }
    BOOL Rectangle(int, int, int, int) { return TRUE; }
    BOOL MoveTo(int, int) { return TRUE; }
    BOOL LineTo(int, int) { return TRUE; }
    BOOL BitBlt(int, int, int, int, CDC*, int, int, DWORD) { return TRUE; }
    BOOL CreateCompatibleDC(CDC*) { return TRUE; }
    int FillRect(LPCRECT, CBrush*) { return 0; }
    void FillSolidRect(LPCRECT, COLORREF) {}
    void FillSolidRect(int, int, int, int, COLORREF) {}
    void Draw3dRect(LPCRECT, COLORREF, COLORREF) {}
    void Draw3dRect(int, int, int, int, COLORREF, COLORREF) {}
    BOOL StretchBlt(int, int, int, int, CDC*, int, int, int, int, DWORD) { return TRUE; }
    int  GetDeviceCaps(int) const { return 0; }
    CSize GetTextExtent(LPCSTR, int) const { return CSize(0, 0); }
    int  DrawText(LPCSTR, int, LPRECT, UINT) { return 0; }
    UINT SetTextAlign(UINT) { return 0; }
    int  SetMapMode(int) { return 0; }
    int  SetROP2(int) { return 0; }
    POINT SetViewportOrg(int, int) { POINT p = {0,0}; return p; }
};

inline BOOL CFont::CreatePointFont(int, LPCSTR, CDC*) { return TRUE; }

/* ============================================================
 * Window / app hierarchy (stubbed — no real windows on Linux)
 * ============================================================ */
class CWnd : public CCmdTarget {
public:
    HWND m_hWnd;
    CWnd() : m_hWnd(NULL) {}
    HWND GetSafeHwnd() const { return m_hWnd; }
    operator HWND() const { return m_hWnd; }
    BOOL Attach(HWND h) { m_hWnd = h; return TRUE; }
    HWND Detach() { HWND h = m_hWnd; m_hWnd = NULL; return h; }
    CWnd* GetDlgItem(int) const { return NULL; }
    int GetDlgItemTextA(int, LPSTR, int) { return 0; }
    void SetDlgItemTextA(int, LPCSTR) {}
    BOOL SetWindowTextA(LPCSTR) { return TRUE; }
    int GetWindowTextA(LPSTR, int) { return 0; }
    BOOL ShowWindow(int) { return TRUE; }
    BOOL UpdateWindow() { return TRUE; }
    BOOL DestroyWindow() { return TRUE; }
    BOOL MoveWindow(int, int, int, int, BOOL = TRUE) { return TRUE; }
    void GetClientRect(LPRECT r) const { if (r) { r->left = r->top = 0; r->right = r->bottom = 0; } }
    void GetWindowRect(LPRECT r) const { if (r) { r->left = r->top = r->right = r->bottom = 0; } }
    void ClientToScreen(LPPOINT) const {}
    void ClientToScreen(LPRECT) const {}
    void ScreenToClient(LPPOINT) const {}
    void ScreenToClient(LPRECT) const {}
    BOOL CreateControl(LPCSTR, LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return FALSE; }
    CDC* GetDC() { return NULL; }
    int  ReleaseDC(CDC*) { return 1; }
    BOOL EnableWindow(BOOL = TRUE) { return TRUE; }
    CWnd* SetFocus() { return NULL; }
    int MessageBoxA(LPCSTR, LPCSTR = NULL, UINT = 0) { return 0; }
    LRESULT SendMessageA(UINT, WPARAM = 0, LPARAM = 0) { return 0; }
    BOOL PostMessageA(UINT, WPARAM = 0, LPARAM = 0) { return TRUE; }
    void Invalidate(BOOL = TRUE) {}
    void InvalidateRect(LPCRECT, BOOL = TRUE) {}
    void ClientToScreenRect(LPRECT) const {}
    BOOL SetTimer(UINT, UINT, void* = NULL) { return TRUE; }
    BOOL KillTimer(UINT) { return TRUE; }
    void SetWindowPos(const CWnd*, int, int, int, int, UINT) {}
    void BringWindowToTop() {}
    BOOL IsWindowVisible() const { return FALSE; }
    void SetCapture() {}
    void GetParentFrame() const {}
    BOOL UpdateData(BOOL = TRUE) { return TRUE; }
    virtual BOOL OnInitDialog() { return TRUE; }
    virtual void DoDataExchange(class CDataExchange*) {}
    virtual LRESULT WindowProc(UINT, WPARAM, LPARAM) { return 0; }
};

/* Common control wrappers (all CWnd-derived stubs) */
class CStatic : public CWnd {
public:
    BOOL Create(LPCSTR, DWORD, const RECT&, CWnd*, UINT = 0) { return TRUE; }
    void SetBitmap(HBITMAP) {}
};
class CButton : public CWnd {
public:
    BOOL Create(LPCSTR, DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    UINT GetState() const { return 0; }
    void SetState(BOOL) {}
    int  GetCheck() const { return 0; }
    void SetCheck(int) {}
};
class CEdit : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    void SetSel(int, int, BOOL = FALSE) {}
    void GetSel(int&, int&) const {}
    int  LineLength(int = -1) const { return 0; }
};
class CListBox : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  AddStringA(LPCSTR) { return 0; }
    int  GetCurSel() const { return -1; }
    int  SetCurSel(int) { return -1; }
    int  GetCount() const { return 0; }
    void ResetContent() {}
    DWORD GetItemData(int) const { return 0; }
    int  SetItemData(int, DWORD) { return 0; }
};
class CComboBox : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  AddStringA(LPCSTR) { return 0; }
    int  GetCurSel() const { return -1; }
    int  SetCurSel(int) { return -1; }
    int  GetCount() const { return 0; }
    void ResetContent() {}
    DWORD GetItemData(int) const { return 0; }
};
class CScrollBar : public CWnd {
public:
    BOOL Create(DWORD, const RECT&, CWnd*, UINT) { return TRUE; }
    int  GetScrollPos() const { return 0; }
    int  SetScrollPos(int, BOOL = TRUE) { return 0; }
    void SetScrollRange(int, int, BOOL = TRUE) {}
    void GetScrollRange(LPINT, LPINT) const {}
};
class CToolBar : public CWnd {
public:
    BOOL Create(CWnd*, DWORD = 0, UINT = 0) { return TRUE; }
};

class CDialog : public CWnd {
public:
    CDialog() {}
    CDialog(UINT, CWnd* = NULL) {}
    CDialog(LPCSTR, CWnd* = NULL) {}
    virtual int DoModal() { return -1; }   /* IDCANCEL-ish */
    BOOL Create(UINT, CWnd* = NULL) { return TRUE; }
    virtual void OnOK() {}
    virtual void OnCancel() {}
    void EndDialog(int) {}
};

class CView : public CWnd {
public:
    virtual void OnDraw(CDC*) {}
};

class CFrameWnd : public CWnd {
public:
    BOOL Create(LPCSTR, LPCSTR, DWORD = 0, const RECT& = CRect(), CWnd* = NULL, LPCSTR = NULL) { return TRUE; }
};

class CDocument : public CCmdTarget {
public:
    virtual BOOL OnNewDocument() { return TRUE; }
    virtual void Serialize(CArchive&) {}
};

class COleDocument : public CDocument {};

class CWinThread : public CCmdTarget {
public:
    CWnd* m_pMainWnd;
    CWinThread() : m_pMainWnd(NULL) {}
    virtual BOOL InitInstance() { return TRUE; }
    virtual int  ExitInstance() { return 0; }
    virtual int  Run() { return 0; }
};

class CWinApp : public CWinThread {
public:
    LPCSTR m_pszAppName;
    HINSTANCE m_hInstance;
    LPSTR  m_lpCmdLine;
    int    m_nCmdShow;
    CWinApp(LPCSTR n = NULL) : m_pszAppName(n), m_hInstance(NULL), m_lpCmdLine(NULL), m_nCmdShow(0) {}
    virtual BOOL InitInstance() { return TRUE; }
    BOOL InitApplication() { return TRUE; }
    HCURSOR LoadStandardCursor(LPCSTR) const { return NULL; }
    HCURSOR LoadCursor(LPCSTR) const { return NULL; }
    HCURSOR LoadCursor(UINT) const { return NULL; }
    HICON   LoadIcon(LPCSTR) const { return NULL; }
    HICON   LoadIcon(UINT) const { return NULL; }
    HICON   LoadStandardIcon(LPCSTR) const { return NULL; }
    int     DoMessageBox(LPCSTR, UINT, UINT) { return 0; }
    void    ParseCommandLine(CCommandLineInfo&) {}
    BOOL    ProcessShellCommand(CCommandLineInfo&) { return TRUE; }
    void    EnableShellOpen() {}
    void    LoadStdProfileSettings(UINT = 0) {}
    BOOL    OnIdle(LONG) { return FALSE; }
};

/* ============================================================
 * Container templates (afxtempl) — minimal, std-backed
 * ============================================================ */
template <class TYPE, class ARG_TYPE = const TYPE&>
class CArray : public CObject {
    std::vector<TYPE> v;
public:
    int  GetSize() const { return (int)v.size(); }
    int  GetCount() const { return (int)v.size(); }
    void SetSize(int n, int = -1) { v.resize(n); }
    void RemoveAll() { v.clear(); }
    int  Add(ARG_TYPE x) { v.push_back(x); return (int)v.size() - 1; }
    TYPE& operator[](int i) { return v[i]; }
    const TYPE& operator[](int i) const { return v[i]; }
    TYPE& GetAt(int i) { return v[i]; }
    void SetAt(int i, ARG_TYPE x) { v[i] = x; }
    void RemoveAt(int i, int n = 1) { v.erase(v.begin()+i, v.begin()+i+n); }
};

template <class TYPE, class ARG_TYPE = const TYPE&>
class CList : public CObject {
    std::list<TYPE> l;
public:
    typedef typename std::list<TYPE>::iterator POSITION_t;
    int  GetCount() const { return (int)l.size(); }
    BOOL IsEmpty() const { return l.empty(); }
    void RemoveAll() { l.clear(); }
    void AddTail(ARG_TYPE x) { l.push_back(x); }
    void AddHead(ARG_TYPE x) { l.push_front(x); }
    TYPE& GetHead() { return l.front(); }
    TYPE& GetTail() { return l.back(); }
};

class CCommandLineInfo {
public:
    BOOL m_bShowSplash;
    BOOL m_bRunEmbedded;
    BOOL m_bRunAutomated;
    CCommandLineInfo() : m_bShowSplash(TRUE), m_bRunEmbedded(FALSE), m_bRunAutomated(FALSE) {}
    void ParseParam(LPCSTR, BOOL, BOOL) {}
};

class CDataExchange {
public:
    BOOL m_bSaveAndValidate;
    CWnd* m_pDlgWnd;
    CWnd* PrepareCtrl(int) { return NULL; }
    CWnd* PrepareEditCtrl(int) { return NULL; }
};

class COleDispatchDriver {
public:
    LPDISPATCH m_lpDispatch;
    BOOL m_bAutoRelease;
    COleDispatchDriver() : m_lpDispatch(NULL), m_bAutoRelease(TRUE) {}
    void AttachDispatch(LPDISPATCH p, BOOL = TRUE) { m_lpDispatch = p; }
    LPDISPATCH DetachDispatch() { LPDISPATCH p = m_lpDispatch; m_lpDispatch = NULL; return p; }
    void ReleaseDispatch() {}
    BOOL CreateDispatch(REFCLSID, void* = NULL) { return FALSE; }
    BOOL CreateDispatch(LPCSTR, void* = NULL) { return FALSE; }
    void InvokeHelper(DISPID, WORD, VARTYPE, void*, const BYTE*, ...) {}
    void SetProperty(DISPID, VARTYPE, ...) {}
    void GetProperty(DISPID, VARTYPE, void*) const {}
};

/* The global application object (defined by IMPLEMENT'd CWinApp subclass in bob) */
extern CWinApp* AfxGetApp();
extern HINSTANCE AfxGetInstanceHandle();
extern CWnd* AfxGetMainWnd();
inline void AfxMessageBox(LPCSTR) {}

/* ANSI/Unicode-neutral aliases bob calls without the A suffix */
#ifndef GetDlgItemText
#define GetDlgItemText GetDlgItemTextA
#endif

#endif /* FF_LINUX */
#endif /* FF_COMPAT_AFXWIN_H */
