/* SRC/compat/bob_dplay.cpp -- R6.1: a real DirectPlay object, so StartCommsSession() can succeed.
 *
 * WHY THIS FILE EXISTS. Multiplayer is not missing game code -- the engine's own DPlay class and
 * Aggrgtor packet layer are present and compiled, and the lobby screens render and navigate. What
 * was missing is the OBJECT underneath, and the gap is one call:
 *
 *     DPlay::CreateDPlayInterface()  (SRC/COMMS/Comms.cpp:807)
 *       -> CoCreateInstance(CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER,
 *                           IID_IDirectPlay4A, (LPVOID*)&lpDP4)
 *
 * and compat's CoCreateInstance was a blanket `*ppv=NULL; return E_NOINTERFACE;` for EVERY CLSID.
 * So CreateDPlayInterface() -> FALSE -> UIMultiPlayInit() -> FALSE -> StartCommsSession() -> FALSE
 * -> the not-connected box, exactly as the game intends when there is no DirectPlay.
 *
 * (An earlier note in both backlogs said the gap was a missing `DirectPlayCreate`. True, and
 * irrelevant: the game never calls it. Verified in BOTH ports -- MiG Alley makes the identical
 * CoCreateInstance call with the identical IID.)
 *
 * SCOPE OF THIS STEP -- deliberately the smallest thing that can be gated. StartCommsSession() is:
 *
 *     if (!UIMultiPlayInit()) return FALSE;   // -> CreateDPlayInterface()
 *     UIAssignServices();                     // -> lpDP4->EnumConnections(...)
 *     return TRUE;
 *
 * so ONLY QueryInterface/AddRef/Release + EnumConnections need to be real. Every other method is a
 * logged DPERR_UNSUPPORTED. Open/Send/Receive come next, with a two-process packet gate.
 *
 * IDirectPlay4 is declared with DECLARE_INTERFACE_, which compat expands to a C++ abstract class
 * (objbase.h: `struct iface : public baseiface`, STDMETHOD -> `virtual ... = 0`). So this SUBCLASSES
 * it and lets the compiler lay the vtable out -- the same pattern as the D3D7 GLSurface7/GLDD7
 * objects. Hand-ordering 53 function pointers would be a silent-corruption trap.
 *
 * BOB_TRACE_DPLAY=1 logs every call, including the unimplemented ones -- so "multiplayer does
 * nothing" is always attributable to a named method rather than guessed at.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DPLAY.H"

static int dp_trace(void) { static int t = -1; if (t < 0) t = getenv("BOB_TRACE_DPLAY") ? 1 : 0; return t; }
#define UNIMPL(n) do { if (dp_trace()) fprintf(stderr, "[dplay] %s: not implemented yet\n", (n)); } while (0)

/* The one service provider we advertise. The game copies the name into its lobby list and
 * InitializeConnection()s the blob we hand back, so the blob must stay valid for the run. */
static GUID  g_tcpGuid = { 0x36E95EE0, 0x8577, 0x11cf, { 0x96,0x0c,0x00,0x80,0xc7,0x53,0x4e,0x82 } };
static char  g_tcpName[] = "Internet TCP/IP Connection For DirectPlay";
static DWORD g_tcpBlob[16];   /* opaque connection blob; identity is all the game needs of it */

class BobDPlay4 : public IDirectPlay4
{
    int ref;
public:
    BobDPlay4() : ref(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override {
        (void)riid;
        if (!ppvObj) return E_POINTER;
        *ppvObj = (LPVOID)this; ref++;
        if (dp_trace()) fprintf(stderr, "[dplay] QueryInterface -> self (ref=%d)\n", ref);
        return DP_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return (ULONG)++ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        int r = --ref;
        if (dp_trace()) fprintf(stderr, "[dplay] Release -> ref=%d\n", r);
        if (r <= 0) { delete this; return 0; }
        return (ULONG)r;
    }

    /* The only other method StartCommsSession() reaches: UIAssignServices() calls this and the
     * game builds its service list from the callback. One provider, TCP/IP. */
    HRESULT STDMETHODCALLTYPE EnumConnections(LPCGUID lpguidApp,
                                              LPDPENUMCONNECTIONSCALLBACK cb,
                                              LPVOID ctx, DWORD flags) override {
        (void)lpguidApp; (void)flags;
        if (dp_trace()) fprintf(stderr, "[dplay] EnumConnections -> 1 provider\n");
        if (cb) {
            DPNAME nm; memset(&nm, 0, sizeof(nm));
            nm.dwSize = sizeof(nm);
            nm.lpszShortNameA = g_tcpName;
            nm.lpszLongNameA  = g_tcpName;
            cb(&g_tcpGuid, (LPVOID)g_tcpBlob, sizeof(g_tcpBlob), &nm, 0, ctx);
        }
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE AddPlayerToGroup(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("AddPlayerToGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Close() override {
        UNIMPL("Close");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreateGroup(LPDPID a0, LPDPNAME a1, LPVOID a2, DWORD a3, DWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("CreateGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreatePlayer(LPDPID a0, LPDPNAME a1, HANDLE a2, LPVOID a3, DWORD a4, DWORD a5) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        (void)a5;
        UNIMPL("CreatePlayer");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeletePlayerFromGroup(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("DeletePlayerFromGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DestroyGroup(DPID a0) override {
        (void)a0;
        UNIMPL("DestroyGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DestroyPlayer(DPID a0) override {
        (void)a0;
        UNIMPL("DestroyPlayer");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumGroupPlayers(DPID a0, LPGUID a1, LPDPENUMPLAYERSCALLBACK2 a2, LPVOID a3, DWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("EnumGroupPlayers");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumGroups(LPGUID a0, LPDPENUMPLAYERSCALLBACK2 a1, LPVOID a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("EnumGroups");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumPlayers(LPGUID a0, LPDPENUMPLAYERSCALLBACK2 a1, LPVOID a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("EnumPlayers");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumSessions(LPDPSESSIONDESC2 a0, DWORD a1, LPDPENUMSESSIONSCALLBACK2 a2, LPVOID a3, DWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("EnumSessions");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetCaps(LPDPCAPS a0, DWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetCaps");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupData(DPID a0, LPVOID a1, LPDWORD a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetGroupData");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupName(DPID a0, LPVOID a1, LPDWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetGroupName");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetMessageCount(DPID a0, LPDWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetMessageCount");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerAddress(DPID a0, LPVOID a1, LPDWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetPlayerAddress");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerCaps(DPID a0, LPDPCAPS a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetPlayerCaps");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerData(DPID a0, LPVOID a1, LPDWORD a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetPlayerData");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerName(DPID a0, LPVOID a1, LPDWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("GetPlayerName");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetSessionDesc(LPVOID a0, LPDWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetSessionDesc");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Initialize(LPGUID a0) override {
        (void)a0;
        UNIMPL("Initialize");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Open(LPDPSESSIONDESC2 a0, DWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("Open");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Receive(LPDPID a0, LPDPID a1, DWORD a2, LPVOID a3, LPDWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("Receive");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Send(DPID a0, DPID a1, DWORD a2, LPVOID a3, DWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("Send");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetGroupData(DPID a0, LPVOID a1, DWORD a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SetGroupData");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetGroupName(DPID a0, LPDPNAME a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("SetGroupName");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetPlayerData(DPID a0, LPVOID a1, DWORD a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SetPlayerData");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetPlayerName(DPID a0, LPDPNAME a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("SetPlayerName");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetSessionDesc(LPDPSESSIONDESC2 a0, DWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("SetSessionDesc");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE AddGroupToGroup(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("AddGroupToGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CreateGroupInGroup(DPID a0, LPDPID a1, LPDPNAME a2, LPVOID a3, DWORD a4, DWORD a5) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        (void)a5;
        UNIMPL("CreateGroupInGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE DeleteGroupFromGroup(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("DeleteGroupFromGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE EnumGroupsInGroup(DPID a0, LPGUID a1, LPDPENUMPLAYERSCALLBACK2 a2, LPVOID a3, DWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("EnumGroupsInGroup");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupConnectionSettings(DWORD a0, DPID a1, LPVOID a2, LPDWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetGroupConnectionSettings");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE InitializeConnection(LPVOID a0, DWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("InitializeConnection");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SecureOpen(LPCDPSESSIONDESC2 a0, DWORD a1, LPCDPSECURITYDESC a2, LPCDPCREDENTIALS a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SecureOpen");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SendChatMessage(DPID a0, DPID a1, DWORD a2, LPDPCHAT a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("SendChatMessage");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetGroupConnectionSettings(DWORD a0, DPID a1, LPDPLCONNECTION a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("SetGroupConnectionSettings");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE StartSession(DWORD a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("StartSession");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupFlags(DPID a0, LPDWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetGroupFlags");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupParent(DPID a0, LPDPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetGroupParent");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerAccount(DPID a0, DWORD a1, LPVOID a2, LPDWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("GetPlayerAccount");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetPlayerFlags(DPID a0, LPDWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetPlayerFlags");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetGroupOwner(DPID a0, LPDPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("GetGroupOwner");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SetGroupOwner(DPID a0, DPID a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("SetGroupOwner");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE SendEx(DPID a0, DPID a1, DWORD a2, LPVOID a3, DWORD a4, DWORD a5, DWORD a6, LPVOID a7, LPDWORD a8) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        (void)a5;
        (void)a6;
        (void)a7;
        (void)a8;
        UNIMPL("SendEx");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE GetMessageQueue(DPID a0, DPID a1, DWORD a2, LPDWORD a3, LPDWORD a4) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        (void)a4;
        UNIMPL("GetMessageQueue");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CancelMessage(DWORD a0, DWORD a1) override {
        (void)a0;
        (void)a1;
        UNIMPL("CancelMessage");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE CancelPriority(DWORD a0, DWORD a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("CancelPriority");
        return DPERR_UNSUPPORTED;
    }
};

/* Hooked from compat's CoCreateInstance (objbase.h) for CLSID_DirectPlay. Returns DP_OK with a
 * live object; every other CLSID keeps the old E_NOINTERFACE. */
extern "C" HRESULT bob_dplay_create(void** ppv)
{
    if (!ppv) return E_POINTER;
    BobDPlay4* p = new BobDPlay4();
    *ppv = (void*)static_cast<IDirectPlay4*>(p);
    if (dp_trace()) fprintf(stderr, "[dplay] CoCreateInstance(CLSID_DirectPlay) -> %p\n", (void*)p);
    return DP_OK;
}
