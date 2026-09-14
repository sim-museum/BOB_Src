/* SRC/compat/bob_dplay.cpp -- DirectPlay over UDP sockets (R6.1 object, R6.2 transport).
 *
 * WHY THIS FILE EXISTS. Multiplayer was never missing game code: the engine's DPlay class and
 * Aggrgtor packet layer are compiled in, and the lobby screens render and navigate. What was
 * missing was the OBJECT, and the gap was ONE call --
 *
 *     DPlay::CreateDPlayInterface()  (SRC/COMMS/Comms.cpp:807)
 *       -> CoCreateInstance(CLSID_DirectPlay, ..., IID_IDirectPlay4A, &lpDP4)
 *
 * -- against a compat CoCreateInstance that answered E_NOINTERFACE for every CLSID.
 * (Both backlogs first recorded the gap as a missing `DirectPlayCreate`. True, and irrelevant:
 * the game never calls it. Corrected in ma S323 / bob R6-S318.)
 *
 * WHAT IS IMPLEMENTED, AND WHY EXACTLY THIS SET. Not chosen from the header -- OBSERVED. Every
 * unimplemented method logs itself under BOB_TRACE_DPLAY=1, so walking the UI made the game name
 * what it needs:
 *
 *     Multi-Player  -> CoCreateInstance, EnumConnections            (R6.1)
 *     Join Game     -> InitializeConnection, EnumSessions           (R6.2)
 *     Back          -> CancelMessage, Close, Release  (ref -> 0, no leak)
 *
 * IDirectPlay4 is declared with DECLARE_INTERFACE_, which this compat layer expands to a C++
 * abstract class, so this SUBCLASSES it and the compiler lays out the 53-entry vtable. The 36
 * still-unimplemented overrides are GENERATED from SRC/H/DPLAY.H, never typed: hand-ordering COM
 * function pointers is a silent-corruption trap.
 *
 * THE TRANSPORT is deliberately plain UDP on one socket, in the spirit of what DirectPlay's
 * TCP/IP provider did: a host binds a port; clients discover it with a broadcast-style probe and
 * then exchange datagrams. The game's own Aggrgtor already handles sequencing, reserve packets and
 * loss -- duplicating that here would be building a second protocol beside the one the game ships.
 *
 * BOB_DPLAY_PORT   override the port (default 47624, DirectPlay's classic port)
 * BOB_DPLAY_HOST   client: where to look for a host (default 127.0.0.1)
 * BOB_TRACE_DPLAY  log every call, including the unimplemented ones
 * BOB_NO_DPLAY     restore E_NOINTERFACE -- the negative control for tools/bob_mp_connect.sh
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DPLAY.H"

static int dp_trace(void) { static int t = -1; if (t < 0) t = getenv("BOB_TRACE_DPLAY") ? 1 : 0; return t; }
#define DPT(...) do { if (dp_trace()) { fprintf(stderr, "[dplay] " __VA_ARGS__); } } while (0)
/* R6.4: log each unimplemented method ONCE. The first host run produced a 24.7-MILLION-line log
   because the game calls SendEx every frame once a session is live -- a per-call trace turns a
   useful "the next blocker names itself" signal into an unreadable flood, and fills the disk. */
#define UNIMPL(n) do { if (dp_trace()) { static int _once = 0; \
        if (!_once) { _once = 1; fprintf(stderr, "[dplay] %s: not implemented yet\n", (n)); } } } while (0)

static int dp_port(void) { const char* e = getenv("BOB_DPLAY_PORT"); int p = e ? atoi(e) : 0; return p > 0 ? p : 47624; }
static const char* dp_host(void) { const char* e = getenv("BOB_DPLAY_HOST"); return (e && *e) ? e : "127.0.0.1"; }

/* Wire framing. Deliberately tiny and self-describing: the whole point of the GPL-era design is
 * that a peer update is a few dozen bytes. */
enum { DPMAGIC = 0x424f4250 };            /* 'BOBP' */
enum { MSG_PROBE = 1, MSG_OFFER = 2, MSG_JOIN = 3, MSG_DATA = 4, MSG_ASSIGN = 5 };
struct WireHdr { unsigned int magic, kind, from, to; };

static const int MAXQ = 64;
struct QMsg { unsigned int from, to, len; char data[1024]; };

static GUID  g_tcpGuid = { 0x36E95EE0, 0x8577, 0x11cf, { 0x96,0x0c,0x00,0x80,0xc7,0x53,0x4e,0x82 } };
static char  g_tcpName[] = "Internet TCP/IP Connection For DirectPlay";
static DWORD g_tcpBlob[16];

class BobDPlay4 : public IDirectPlay4
{
    int  ref;
    int  fd;                 /* the one UDP socket */
    int  isHost;
    DPID nextPid;
    DPID myPid;
    /* MP S5, ported from MA 77299a7 (2026-09-14). This process can own MORE THAN ONE player:
       measured in MA, the host creates the AGGREGATOR as a player and then its own game player,
       and the two talk to each other inside the one process. A shim that remembers only the
       last-created id mis-handles both directions -- traffic addressed to the other local player
       is not recognised as a known player, so the receive filter's catch-all hands it to the
       wrong caller, and a send between two local players leaves on the wire and is never
       delivered at home. The two shims share their ancestry, so the defect is shared too. */
    DPID localPids[8]; int nlocal;
    DPID assignedPid;      /* R6.3: what the host gave us (client side); 0 until it answers */
    DPID groups[8]; int gmembers[8]; DPID gplayers[8][8]; int ngroups;   /* R6.4 */
    /* MP-5 (PO 2026-09-05): pids this HOST has handed to joining clients. The game only ever
       calls AddPlayerToGroup for its OWN player, so a group held one member (the host) and
       SendMessageToGroup -- which is how UISendFlyNow broadcasts PID_FLYNOW -- reached nobody.
       Measured on a working two-instance join: "AddPlayerToGroup player 3 -> group 2" and nothing
       for the client's pid 4, so the client sat in the Ready Room forever waiting for a Fly-Now
       that was never transmitted to it. Real DirectPlay tells the host about a remote player via a
       DPSYS_CREATEPLAYERORGROUP system message and the game adds it; this shim never delivered
       one. Track them here so joiners land in the groups the game broadcasts to. */
    DPID joined[8]; int njoined;
    struct sockaddr_in peer; /* host: last client seen. client: the host. */
    int  havePeer;
    char sessName[128];
    GUID sessGuid;
    char offers[8][128]; int offerCount = 0;   /* R24/S427: offers drained by pump(), not by EnumSessions */
    QMsg q[MAXQ]; int qh, qt;

    /* MP-6 (2026-09-13), the same instrument MA carries: count the ANNOUNCE at every stage so
       "the client never saw it" can be told from "it was delivered and something else ate it".
       The game's packet begins with ULong PacketID (struct Generic) and PID_IAMIN is 0x08.
       BOB_TRACE_IAMIN=1. */
    static bool isAnnounce(const char* d, unsigned n) {
        if (n < 4) return false;
        unsigned id; memcpy(&id, d, 4);
        return id == 0x08u;
    }
    void noteAnnounce(const char* stage, unsigned f, unsigned t, const char* d, unsigned n) {
        if (!isAnnounce(d, n) || !getenv("BOB_TRACE_IAMIN")) return;
        fprintf(stderr, "[iamin-wire] %s from=%u to=%u len=%u\n", stage, f, t, n);
        fflush(stderr);
    }

    void qpush(unsigned f, unsigned t, const char* d, unsigned n) {
        int nx = (qt + 1) % MAXQ;
        if (nx == qh) { DPT("queue full, dropping a packet\n"); return; }
        q[qt].from = f; q[qt].to = t; q[qt].len = n > sizeof(q[qt].data) ? sizeof(q[qt].data) : n;
        memcpy(q[qt].data, d, q[qt].len); qt = nx;
        noteAnnounce("QUEUED", f, t, d, n);
    }
    int qcount() const { return (qt - qh + MAXQ) % MAXQ; }

    /* Drain the socket: answer discovery probes, absorb joins, queue data. Called from every path
     * the game pumps (Receive / GetMessageCount / EnumSessions) so a host answers probes while it
     * is simply sitting in its own message loop. */
    void pump() {
        if (fd < 0) return;
        char buf[2048];
        for (;;) {
            struct sockaddr_in from; socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fl);
            if (n < (ssize_t)sizeof(WireHdr)) break;
            WireHdr* h = (WireHdr*)buf;
            if (h->magic != DPMAGIC) continue;
            if (h->kind == MSG_PROBE && isHost) {
                char out[sizeof(WireHdr) + sizeof(sessName)];
                WireHdr* oh = (WireHdr*)out;
                oh->magic = DPMAGIC; oh->kind = MSG_OFFER; oh->from = 0; oh->to = 0;
                memcpy(out + sizeof(WireHdr), sessName, sizeof(sessName));
                sendto(fd, out, sizeof(out), 0, (struct sockaddr*)&from, fl);
                DPT("probe from a client -> offered session \"%s\"\n", sessName);
            } else if (h->kind == MSG_JOIN && isHost) {
                peer = from; havePeer = 1;
                /* R6.3: THE HOST OWNS THE ID SPACE. Before this, each object started nextPid at
                   DPID_SERVERPLAYER independently, so host and client both allocated pid 1 -- the
                   packets still crossed (R6.2 passed) but every player was indistinguishable, and
                   the Aggrgtor addresses its packets BY pid. Found by reading the R6.2 trace, not
                   by a failure: a two-node echo cannot expose an id collision. */
                DPID given = nextPid++;
                /* MP-5: remember the joiner and put it in the groups that already exist, so a
                   group broadcast (UISendFlyNow) actually reaches it. */
                if (njoined < 8) joined[njoined++] = given;
                for (int gi = 0; gi < ngroups; gi++)
                    if (gmembers[gi] < 8)
                    {
                        gplayers[gi][gmembers[gi]++] = given;
                        DPT("auto-added joining pid %u to group %u (%d members)\n",
                            (unsigned)given, (unsigned)groups[gi], gmembers[gi]);
                    }
                WireHdr ah; ah.magic = DPMAGIC; ah.kind = MSG_ASSIGN;
                ah.from = (unsigned)DPID_SERVERPLAYER; ah.to = (unsigned)given;
                sendto(fd, &ah, sizeof(ah), 0, (struct sockaddr*)&from, fl);
                DPT("client joined from %s:%d -> assigned pid %u\n",
                    inet_ntoa(from.sin_addr), (int)ntohs(from.sin_port), (unsigned)given);
            } else if (h->kind == MSG_OFFER && !isHost) {
                /* R24/S427: STASH IT. `EnumSessions` runs its own recvfrom loop on this same
                   socket, so there are two readers -- and before this branch existed only one of
                   them understood an OFFER. `pump()` fell through every case and DISCARDED it, so
                   whenever anything else drained the socket first, discovery came back empty.
                   That is not hypothetical: enabling the MFC timer (R24) made
                   DPlay::UIUpdateMainSheet -> ReceiveNextMessage -> Receive -> pump() run on the
                   client, and bob_mp_uijoin went from "1 session listed" to "0 listed", 3 runs out
                   of 3 each way. The timer only exposed it; the race was always there, because any
                   Receive() during a session scan could win the same packet.
                   Cache the offer so whichever reader gets it, EnumSessions can still report it. */
                const char* nm = (const char*)buf + sizeof(WireHdr);
                size_t maxn = (size_t)n - sizeof(WireHdr);
                if (maxn > 0 && offerCount < (int)(sizeof(offers)/sizeof(offers[0]))) {
                    size_t cn = strnlen(nm, maxn);
                    if (cn >= sizeof(offers[0])) cn = sizeof(offers[0]) - 1;
                    memcpy(offers[offerCount], nm, cn); offers[offerCount][cn] = 0;
                    offerCount++;
                    DPT("cached a session offer \"%s\" seen outside EnumSessions\n", offers[offerCount-1]);
                }
            } else if (h->kind == MSG_ASSIGN && !isHost) {
                assignedPid = (DPID)h->to;
                DPT("host assigned us pid %u\n", (unsigned)assignedPid);
            } else if (h->kind == MSG_DATA) {
                qpush(h->from, h->to, buf + sizeof(WireHdr), (unsigned)(n - sizeof(WireHdr)));
                DPT("received %d data bytes from pid %u\n", (int)(n - sizeof(WireHdr)), h->from);
            }
        }
    }
    int mksock(int bindIt) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) return 0;
        int on = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        if (bindIt) {
            struct sockaddr_in a; memset(&a, 0, sizeof(a));
            a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons(dp_port());
            if (bind(fd, (struct sockaddr*)&a, sizeof(a)) != 0) {
                DPT("bind(%d) failed: %s\n", dp_port(), strerror(errno));
                close(fd); fd = -1; return 0;
            }
            DPT("host bound to UDP %d\n", dp_port());
        }
        return 1;
    }
public:
    BobDPlay4() : ref(1), fd(-1), isHost(0), nextPid(DPID_SERVERPLAYER), myPid(0), nlocal(0), assignedPid(0),
                  havePeer(0), ngroups(0), njoined(0), qh(0), qt(0) {
        memset(&peer, 0, sizeof(peer)); memset(sessName, 0, sizeof(sessName));
        memset(&sessGuid, 0, sizeof(sessGuid));
    }
    ~BobDPlay4() { if (fd >= 0) close(fd); }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObj) override {
        (void)riid; if (!ppvObj) return E_POINTER;
        *ppvObj = (LPVOID)this; ref++; return DP_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return (ULONG)++ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        int r = --ref; DPT("Release -> ref=%d\n", r);
        if (r <= 0) { delete this; return 0; }
        return (ULONG)r;
    }

    HRESULT STDMETHODCALLTYPE EnumConnections(LPCGUID, LPDPENUMCONNECTIONSCALLBACK cb,
                                              LPVOID ctx, DWORD) override {
        DPT("EnumConnections -> 1 provider\n");
        if (cb) {
            DPNAME nm; memset(&nm, 0, sizeof(nm));
            nm.dwSize = sizeof(nm); nm.lpszShortNameA = g_tcpName; nm.lpszLongNameA = g_tcpName;
            cb(&g_tcpGuid, (LPVOID)g_tcpBlob, sizeof(g_tcpBlob), &nm, 0, ctx);
        }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE InitializeConnection(LPVOID lpConnection, DWORD) override {
        (void)lpConnection;
        DPT("InitializeConnection (TCP/IP provider selected)\n");
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE Open(LPDPSESSIONDESC2 d, DWORD flags) override {
        if (flags & DPOPEN_CREATE) {
            isHost = 1;
            if (d && d->lpszSessionNameA) { strncpy(sessName, d->lpszSessionNameA, sizeof(sessName)-1); }
            else strncpy(sessName, "Battle of Britain", sizeof(sessName)-1);
            if (d) sessGuid = d->guidInstance;
            if (!mksock(1)) return DPERR_CANTCREATEPLAYER;
            DPT("Open(CREATE) session \"%s\"\n", sessName);
            return DP_OK;
        }
        if (flags & DPOPEN_JOIN) {
            isHost = 0;
            if (!mksock(0)) return DPERR_NOCONNECTION;
            memset(&peer, 0, sizeof(peer));
            peer.sin_family = AF_INET; peer.sin_port = htons(dp_port());
            peer.sin_addr.s_addr = inet_addr(dp_host());
            havePeer = 1;
            WireHdr h; h.magic = DPMAGIC; h.kind = MSG_JOIN; h.from = 0; h.to = 0;
            sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&peer, sizeof(peer));
            DPT("Open(JOIN) -> host %s:%d\n", dp_host(), dp_port());
            for (int i = 0; i < 40 && assignedPid == 0; i++) { pump(); usleep(25000); }  /* R6.3 */
            if (assignedPid == 0) DPT("host did not assign a pid (joining anyway)\n");
            return DP_OK;
        }
        UNIMPL("Open(other flags)");
        return DPERR_UNSUPPORTED;
    }
    HRESULT STDMETHODCALLTYPE Close() override {
        DPT("Close\n");
        if (fd >= 0) { close(fd); fd = -1; }
        isHost = 0; havePeer = 0; qh = qt = 0;
        return DP_OK;
    }

    /* Probe for a host and report what answers. With no host running, no callback fires and this
     * returns DP_OK with an empty list -- which is the honest answer, not an error. */
    HRESULT STDMETHODCALLTYPE EnumSessions(LPDPSESSIONDESC2 d, DWORD timeout,
                                           LPDPENUMSESSIONSCALLBACK2 cb, LPVOID ctx, DWORD) override {
        (void)d;
        if (fd < 0 && !mksock(0)) return DPERR_NOCONNECTION;
        struct sockaddr_in to; memset(&to, 0, sizeof(to));
        to.sin_family = AF_INET; to.sin_port = htons(dp_port()); to.sin_addr.s_addr = inet_addr(dp_host());
        WireHdr h; h.magic = DPMAGIC; h.kind = MSG_PROBE; h.from = 0; h.to = 0;
        sendto(fd, &h, sizeof(h), 0, (struct sockaddr*)&to, sizeof(to));
        DPT("EnumSessions: probing %s:%d\n", dp_host(), dp_port());

        int found = 0;
        unsigned waitms = timeout ? (timeout > 2000 ? 2000 : timeout) : 400;
        /* Report anything pump() already took off the socket for us. */
        for (int i = 0; i < offerCount; i++) {
            DPSESSIONDESC2 sd; memset(&sd, 0, sizeof(sd)); sd.dwSize = sizeof(sd);
            sd.lpszSessionNameA = offers[i]; sd.dwMaxPlayers = 8; sd.dwCurrentPlayers = 1;
            DWORD tmo = waitms; found++;
            DPT("EnumSessions: found \"%s\" (from the pump cache)\n", offers[i]);
            if (cb && !cb(&sd, &tmo, 0, ctx)) { offerCount = 0; DPT("EnumSessions -> %d session(s)\n", found); return DP_OK; }
        }
        offerCount = 0;
        for (unsigned t = 0; t < waitms; t += 20) {
            char buf[2048]; struct sockaddr_in from; socklen_t fl = sizeof(from);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fl);
            if (n >= (ssize_t)sizeof(WireHdr)) {
                WireHdr* rh = (WireHdr*)buf;
                /* MP-5 (PO 2026-09-05): DO NOT EAT THE GAME'S PACKETS.
                   This loop reads the SHARED socket for up to `waitms` and used to discard
                   everything that was not an OFFER. The Select-Session screen's 2365 timer keeps
                   calling EnumSessions long after that screen closes (its OnDestroy/KillTimer has
                   never run in this port), so a client sitting in the Ready Room ran this loop 245
                   times -- swallowing the host's traffic, including the FlyNow that tells it to
                   launch. Measured: the client received 6 packets in a whole session and never set
                   FlyNowFlag, so it never followed the host into 3D.
                   Hand anything that is not an OFFER to the SAME queue pump() fills, so a packet
                   that arrives during an enumeration is delivered instead of destroyed. */
                if (rh->magic == DPMAGIC && rh->kind == MSG_DATA) {
                    qpush(rh->from, rh->to, buf + sizeof(WireHdr), (unsigned)(n - sizeof(WireHdr)));
                    DPT("EnumSessions: rescued %d data bytes from pid %u (would have been dropped)\n",
                        (int)(n - sizeof(WireHdr)), rh->from);
                }
                else if (rh->magic == DPMAGIC && rh->kind == MSG_ASSIGN && !isHost) {
                    assignedPid = (DPID)rh->to; havePeer = 1; peer = from;
                    DPT("EnumSessions: rescued host pid assignment %u\n", (unsigned)rh->to);
                }
                if (rh->magic == DPMAGIC && rh->kind == MSG_OFFER) {
                    DPSESSIONDESC2 sd; memset(&sd, 0, sizeof(sd));
                    sd.dwSize = sizeof(sd);
                    sd.lpszSessionNameA = buf + sizeof(WireHdr);
                    sd.dwMaxPlayers = 8; sd.dwCurrentPlayers = 1;
                    sd.dwFlags = 0;
                    DWORD tmo = waitms;
                    found++;
                    DPT("EnumSessions: found \"%s\"\n", sd.lpszSessionNameA);
                    if (cb && !cb(&sd, &tmo, 0, ctx)) break;
                }
            }
            usleep(20000);
        }
        DPT("EnumSessions -> %d session(s)\n", found);
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE CreatePlayer(LPDPID pid, LPDPNAME nm, HANDLE, LPVOID, DWORD, DWORD) override {
        /* R6.3: a client uses the id the HOST gave it; only the host mints ids. */
        myPid = (!isHost && assignedPid != 0) ? assignedPid : nextPid++;
        if (nlocal < 8) localPids[nlocal++] = myPid;   /* MP S5: every local player, not just the last */
        if (pid) *pid = myPid;
        DPT("CreatePlayer \"%s\" -> pid %u\n",
            (nm && nm->lpszShortNameA) ? nm->lpszShortNameA : "(unnamed)", (unsigned)myPid);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DestroyPlayer(DPID id) override { DPT("DestroyPlayer %u\n", (unsigned)id); return DP_OK; }

    HRESULT STDMETHODCALLTYPE Send(DPID from, DPID to, DWORD, LPVOID data, DWORD len) override {
        /* R26 (S431): NO PEER IS NOT AN ERROR.
         * This used to return DPERR_NOCONNECTION whenever nobody had connected yet, and the game
         * reads that as "comms are broken". Real DirectPlay does not: a Send to a GROUP with no
         * members transmits nothing and returns DP_OK. Only an absent socket is a connection error.
         *
         * It matters because the game broadcasts to a group at the moment it starts a flight:
         * UISendFlyNow -> SendMessageToPlayers(playergroupID) -> SendEx. With this guard a HOST
         * WHO IS ALONE can never take off -- UISendFlyNow returns FALSE, UINetworkSelectFly
         * refuses, CommsSelectFly returns FALSE, and the FLY click silently does nothing.
         * MA traced exactly that chain end to end (MA PO-76/S430) and this shim carries the same
         * line; the two shims share their ancestry, so the defect is shared too.
         * BOB_STRICT_SEND=1 restores the old behaviour as the negative control. */
        noteAnnounce("SENT", (unsigned)from, (unsigned)to, (const char*)data, (unsigned)len);
        /* MP S5, ported from MA 77299a7: LOCAL LOOPBACK. Measured in MA the same day -- the host's
           aggregator sent the sync packet to a group containing the host's own game player twelve
           times a second, the shim put it on the wire and nothing else, and the host's game half,
           which waits for exactly that packet, therefore never synchronised. Deliver a copy into
           the local queue when the destination is a local player other than the sender, or a group
           with a local member; the receive filter then routes it by membership exactly as it routes
           the wire copy. BOB_NO_LOOPBACK=1 reverts. */
        if (!getenv("BOB_NO_LOOPBACK") && (unsigned)to != (unsigned)from &&
            (isLocalPlayer((unsigned)to) || isGroupWithLocalMember((unsigned)to, (unsigned)from)))
        {
            qpush((unsigned)from, (unsigned)to, (const char*)data, (unsigned)len);
            if (getenv("BOB_TRACE_AGG")) {
                static long n = 0; static time_t last = 0; time_t now = time(0); n++;
                if (now != last) { last = now;
                    fprintf(stderr, "[agg] loopback %ld/s  from=%u to=%u (local player or group)\n",
                            n, (unsigned)from, (unsigned)to);
                    fflush(stderr); n = 0; }
            }
        }
        if (fd < 0) return DPERR_NOCONNECTION;
        if (!havePeer) {
            if (getenv("BOB_STRICT_SEND")) return DPERR_NOCONNECTION;
            DPT("Send %u bytes pid %u -> %u (no peer yet -- DP_OK, nothing transmitted)\n",
                (unsigned)len, (unsigned)from, (unsigned)to);
            return DP_OK;
        }
        char out[2048];
        if (len > sizeof(out) - sizeof(WireHdr)) len = sizeof(out) - sizeof(WireHdr);
        WireHdr* h = (WireHdr*)out;
        h->magic = DPMAGIC; h->kind = MSG_DATA; h->from = (unsigned)from; h->to = (unsigned)to;
        memcpy(out + sizeof(WireHdr), data, len);
        ssize_t s = sendto(fd, out, sizeof(WireHdr) + len, 0, (struct sockaddr*)&peer, sizeof(peer));
        DPT("Send %u bytes pid %u -> %u (%s)\n", (unsigned)len, (unsigned)from, (unsigned)to,
            s > 0 ? "ok" : strerror(errno));
        return s > 0 ? DP_OK : DPERR_GENERIC;
    }
    /* MP-5 S11 (2026-09-12): DPRECEIVE_TOPLAYER was IGNORED, and the game depends on it.
     * `DPlay::ReceiveNextMessageToMe` passes its own id IN through lpidTo and asks DirectPlay for
     * "messages addressed to this player only" -- its comment says so in four exclamation marks
     * ("Dont want to receive packets sent to aggregator here!!!!"). This shim treated lpidTo as
     * output and returned whatever was at the queue head, so every caller drained every other
     * caller's traffic: whoever polled first consumed the packet, and a wait loop looking for one
     * specific message could lose it to an unrelated pump. The client's 20 s "Receive Random List"
     * wait (WINMOVE.CPP:1499) is exactly such a loop.
     * Filter here, where DirectPlay would: deliver the first queued message addressed TO that
     * player, to a GROUP the player belongs to (real DirectPlay expands a group send to its
     * members), or to 0 (the game's own "request" broadcast address). Anything else stays queued
     * for the caller it belongs to. DPRECEIVE_ALL / flags 0 keep the old take-the-head behaviour.
     * BOB_NO_RECV_FILTER=1 restores it as the negative control. */
    /* every player id this side has seen: our own, the host's, and any joiner the host handed a pid. */
    bool isLocalPlayer(unsigned pid) const {
        for (int i = 0; i < nlocal; i++) if ((unsigned)localPids[i] == pid) return true;
        return false;
    }
    /* MP S6, ported from MA 0c707a8 (2026-09-14): a group send must reach local members OTHER than
       the sender. The first version asked only "does this group have a local member", which is true
       of the sender itself, so a player's own broadcast came back to it -- and in MA that was
       measured turning the entry announcement into a self-join that double-counted the player.
       BOB_LOOPBACK_SELF=1 restores the looser rule as the negative control. */
    bool isGroupWithLocalMember(unsigned gid, unsigned exceptPid) const {
        const bool self = getenv("BOB_LOOPBACK_SELF") != 0;
        for (int gi = 0; gi < ngroups; gi++) {
            if ((unsigned)groups[gi] != gid) continue;
            for (int k = 0; k < gmembers[gi]; k++) {
                const unsigned m = (unsigned)gplayers[gi][k];
                if (!isLocalPlayer(m)) continue;
                if (!self && m == exceptPid) continue;
                return true;
            }
        }
        return false;
    }
    bool isKnownPlayer(unsigned pid) const {
        if (isLocalPlayer(pid)) return true;
        if (pid == (unsigned)myPid) return true;
        for (int i = 0; i < njoined; i++) if ((unsigned)joined[i] == pid) return true;
        return false;
    }
    bool inGroup(unsigned gid, unsigned pid) const {
        for (int gi = 0; gi < ngroups; gi++) {
            if ((unsigned)groups[gi] != gid) continue;
            for (int k = 0; k < gmembers[gi]; k++)
                if ((unsigned)gplayers[gi][k] == pid) return true;
        }
        return false;
    }
    HRESULT STDMETHODCALLTYPE Receive(LPDPID from, LPDPID to, DWORD flags, LPVOID data, LPDWORD size) override {
        const unsigned toIn   = to   ? (unsigned)*to   : 0u;   /* BEFORE *to is overwritten below */
        const unsigned fromIn = from ? (unsigned)*from : 0u;
        pump();
        if (qcount() == 0) return DPERR_NOMESSAGES;
        static int nofilter = -1;
        if (nofilter < 0) nofilter = getenv("BOB_NO_RECV_FILTER") ? 1 : 0;
        int idx = qh;
        /* MP-6: DPRECEIVE_FROMPLAYER was ignored, so a caller waiting on one specific peer took
           whatever was at the head. Measured: a joiner's announce-carrying packet was delivered
           with flags=0x4, i.e. to a FROMPLAYER wait loop, never reaching ProcessPlayerMessage.
           Real DirectPlay returns only traffic from that player. BOB_MP_NOFROMFILTER=1 reverts. */
        if (!nofilter && (flags & DPRECEIVE_FROMPLAYER) && from
            && !getenv("BOB_MP_NOFROMFILTER")) {
            int found = -1;
            for (int i = qh; i != qt; i = (i + 1) % MAXQ)
                if (q[i].from == fromIn) { found = i; break; }
            if (found < 0) return DPERR_NOMESSAGES;
            idx = found;
        }
        else if (!nofilter && (flags & DPRECEIVE_TOPLAYER) && to) {
            unsigned want = (unsigned)*to;
            int found = -1;
            for (int i = qh; i != qt; i = (i + 1) % MAXQ) {
                unsigned dst = q[i].to;
                /* MP-5 cont.13: a JOINER's shim knows the groups it was auto-added to only if the host
                   told it; the host's own group bookkeeping does not cross the wire. A destination that
                   is neither this player nor any player id this side knows is therefore a GROUP (or an
                   id from the other side's numbering) and must be DELIVERED -- dropping it silently
                   eats the FlyNow broadcast, which is the one packet the Ready Room is waiting for.
                   The filter still does its job: traffic addressed to ANOTHER KNOWN PLAYER (the
                   aggregator included, which is what ReceiveNextMessageToMe's comment is about)
                   stays queued for that caller. */
                if (dst == want || dst == 0 || inGroup(dst, want) || !isKnownPlayer(dst)) { found = i; break; }
            }
            if (found < 0) return DPERR_NOMESSAGES;
            idx = found;
        }
        QMsg& m = q[idx];
        if (size && *size < m.len) { *size = m.len; return DPERR_BUFFERTOOSMALL; }
        if (from) *from = (DPID)m.from;
        if (to)   *to   = (DPID)m.to;
        if (data && size) { memcpy(data, m.data, m.len); *size = m.len; }
        if (isAnnounce(m.data, m.len) && getenv("BOB_TRACE_IAMIN")) {
            /* which clause of the filter let it through, and who asked */
            fprintf(stderr, "[iamin-wire] DELIVERED from=%u to=%u len=%u flags=0x%lx toarg=%u"
                            " (dst==want:%d dst==0:%d inGroup:%d unknownPlayer:%d)\n",
                    m.from, m.to, m.len, (unsigned long)flags, toIn,
                    (unsigned)m.to == toIn, m.to == 0,
                    inGroup((unsigned)m.to, toIn) ? 1 : 0,
                    isKnownPlayer((unsigned)m.to) ? 0 : 1);
            fflush(stderr);
        }
        /* remove q[idx], preserving the order of everything still queued */
        for (int i = idx; i != qh; i = (i - 1 + MAXQ) % MAXQ)
            q[i] = q[(i - 1 + MAXQ) % MAXQ];
        qh = (qh + 1) % MAXQ;
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE GetMessageCount(DPID, LPDWORD n) override {
        pump(); if (n) *n = (DWORD)qcount(); return DP_OK;
    }
    /* R6.4: GROUPS. The game creates a group immediately after Open(CREATE) -- the trace named
       CreateGroup as the next stop, which is what the per-method logging is for. For a session
       this size a group is just an id plus a membership list; the Aggrgtor addresses traffic by
       PLAYER id, so group routing is not on the packet path yet. Implemented as real bookkeeping
       rather than DP_OK-and-forget, so EnumGroups/EnumGroupPlayers can answer truthfully. */
    HRESULT STDMETHODCALLTYPE CreateGroup(LPDPID pid, LPDPNAME nm, LPVOID, DWORD, DWORD) override {
        DPID g = nextPid++;
        if (pid) *pid = g;
        if (ngroups < 8) {
            groups[ngroups] = g; gmembers[ngroups] = 0;
            /* MP-5: a group created after clients joined must contain them too, or the same
               broadcast-to-nobody happens with the order reversed. */
            for (int j = 0; j < njoined && gmembers[ngroups] < 8; j++)
            {
                gplayers[ngroups][gmembers[ngroups]++] = joined[j];
                DPT("seeded group %u with already-joined pid %u\n", (unsigned)g, (unsigned)joined[j]);
            }
            ngroups++;
        }
        DPT("CreateGroup \"%s\" -> gid %u\n",
            (nm && nm->lpszShortNameA) ? nm->lpszShortNameA : "(unnamed)", (unsigned)g);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DestroyGroup(DPID g) override { DPT("DestroyGroup %u\n", (unsigned)g); return DP_OK; }
    HRESULT STDMETHODCALLTYPE AddPlayerToGroup(DPID g, DPID p) override {
        bool matched = false;
        for (int i = 0; i < ngroups; i++)
            if (groups[i] == g && gmembers[i] < 8) { gplayers[i][gmembers[i]++] = p; matched = true; break; }
        /* MP-6: if this side has never heard of the group, ADOPT it. Measured on a joiner:
           "AddPlayerToGroup player 4 -> group 2 : NO SUCH GROUP on this side (ngroups=0)". The
           host creates the group and sends its id over the wire (COMMS.CPP:2095 -> :2187); the
           guest then calls this for its own player with an id in the HOST's numbering, matches
           nothing, and silently records no membership -- so inGroup() can never route a broadcast
           to a joiner, and the host's PID_IAMIN is delivered by the catch-all clause to whichever
           caller polls first. Adopting the id makes the guest's bookkeeping agree with the host's.
           BOB_MP_NOADOPT=1 restores the silent miss as the negative control. */
        if (!matched && !getenv("BOB_MP_NOADOPT") && ngroups < 8) {
            groups[ngroups] = g; gmembers[ngroups] = 0;
            gplayers[ngroups][gmembers[ngroups]++] = p;
            ngroups++;
            matched = true;
            DPT("adopted group %u from the wire and joined player %u to it\n",
                (unsigned)g, (unsigned)p);
        }
        /* MP-6: a GUEST calls this for its own player with the group id the HOST created and sent
           over the wire (COMMS.CPP:2095 -> :2187). That id is in the host's numbering and is not in
           this side's groups[], so the loop matches nothing and the guest never records its own
           membership -- which is why inGroup() can never be the clause that delivers a broadcast to
           a joiner. Report it rather than assume it. */
        if (getenv("BOB_TRACE_IAMIN")) {
            fprintf(stderr, "[group] AddPlayerToGroup player %u -> group %u : %s (ngroups=%d)\n",
                    (unsigned)p, (unsigned)g, matched ? "recorded" : "NO SUCH GROUP on this side", ngroups);
            fflush(stderr);
        }
        DPT("AddPlayerToGroup player %u -> group %u\n", (unsigned)p, (unsigned)g);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE DeletePlayerFromGroup(DPID g, DPID p) override {
        DPT("DeletePlayerFromGroup %u from %u\n", (unsigned)p, (unsigned)g); return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE SetGroupData(DPID, LPVOID, DWORD, DWORD) override { return DP_OK; }
    HRESULT STDMETHODCALLTYPE GetGroupData(DPID, LPVOID, LPDWORD sz, DWORD) override {
        if (sz) *sz = 0; return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE SetGroupName(DPID, LPDPNAME, DWORD) override { return DP_OK; }
    HRESULT STDMETHODCALLTYPE GetGroupName(DPID, LPVOID, LPDWORD sz) override {
        if (sz) *sz = 0; return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE EnumGroups(LPGUID, LPDPENUMPLAYERSCALLBACK2 cb, LPVOID ctx, DWORD) override {
        DPT("EnumGroups -> %d\n", ngroups);
        if (cb) for (int i = 0; i < ngroups; i++) {
            DPNAME nm; memset(&nm, 0, sizeof(nm)); nm.dwSize = sizeof(nm);
            if (!cb(groups[i], 0, &nm, 0, ctx)) break;
        }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE EnumGroupPlayers(DPID g, LPGUID, LPDPENUMPLAYERSCALLBACK2 cb,
                                               LPVOID ctx, DWORD) override {
        for (int i = 0; i < ngroups; i++) if (groups[i] == g) {
            DPT("EnumGroupPlayers group %u -> %d\n", (unsigned)g, gmembers[i]);
            if (cb) for (int k = 0; k < gmembers[i]; k++) {
                DPNAME nm; memset(&nm, 0, sizeof(nm)); nm.dwSize = sizeof(nm);
                if (!cb(gplayers[i][k], 0, &nm, 0, ctx)) break;
            }
            break;
        }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE CancelMessage(DWORD, DWORD) override { return DP_OK; }
    HRESULT STDMETHODCALLTYPE GetCaps(LPDPCAPS c, DWORD) override {
        if (c) { DWORD sz = c->dwSize ? c->dwSize : sizeof(DPCAPS); memset(c, 0, sz); c->dwSize = sz;
                 c->dwMaxBufferSize = 1024; c->dwMaxPlayers = 8; }
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE SetSessionDesc(LPDPSESSIONDESC2 d, DWORD) override {
        if (d && d->lpszSessionNameA) strncpy(sessName, d->lpszSessionNameA, sizeof(sessName)-1);
        return DP_OK;
    }
    HRESULT STDMETHODCALLTYPE GetSessionDesc(LPVOID data, LPDWORD size) override {
        DWORD need = sizeof(DPSESSIONDESC2);
        if (!size) return E_POINTER;
        if (!data || *size < need) { *size = need; return DPERR_BUFFERTOOSMALL; }
        DPSESSIONDESC2* d = (DPSESSIONDESC2*)data;
        memset(d, 0, need); d->dwSize = need;
        d->lpszSessionNameA = sessName; d->guidInstance = sessGuid;
        d->dwMaxPlayers = 8; d->dwCurrentPlayers = havePeer ? 2 : 1;
        *size = need;
        return DP_OK;
    }

    HRESULT STDMETHODCALLTYPE EnumPlayers(LPGUID a0, LPDPENUMPLAYERSCALLBACK2 a1, LPVOID a2, DWORD a3) override {
        (void)a0;
        (void)a1;
        (void)a2;
        (void)a3;
        UNIMPL("EnumPlayers");
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
    HRESULT STDMETHODCALLTYPE Initialize(LPGUID a0) override {
        (void)a0;
        UNIMPL("Initialize");
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
    /* R6.4: SendEx is Send with async/priority/timeout extras the game does not depend on here.
       The session goes live and the game sends every frame through it, so this is on the hot path:
       forward to the same datagram send rather than duplicating it. */
    HRESULT STDMETHODCALLTYPE SendEx(DPID from, DPID to, DWORD flags, LPVOID data, DWORD len,
                                     DWORD, DWORD, LPVOID, LPDWORD msgid) override {
        if (msgid) *msgid = 0;
        return Send(from, to, flags, data, len);
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
    HRESULT STDMETHODCALLTYPE CancelPriority(DWORD a0, DWORD a1, DWORD a2) override {
        (void)a0;
        (void)a1;
        (void)a2;
        UNIMPL("CancelPriority");
        return DPERR_UNSUPPORTED;
    }
};

extern "C" HRESULT bob_dplay_create(void** ppv)
{
    if (!ppv) return E_POINTER;
    BobDPlay4* p = new BobDPlay4();
    *ppv = (void*)static_cast<IDirectPlay4*>(p);
    DPT("CoCreateInstance(CLSID_DirectPlay) -> %p\n", (void*)p);
    return DP_OK;
}
