/* ============================================================================
 * bob_msgmap.cpp — S158: a real message-map dispatch for the compat layer.
 *
 * WHY THIS EXISTS (S157, doc/scaffold-audit.md §3)
 * ------------------------------------------------
 * `CWnd::SendMessageA` was an ALLOWLIST OF THREE (WM_GETFILE, WM_GETGLOBALFONT,
 * WM_GETSTRING) and answered 0 for everything else, while
 * `DECLARE_MESSAGE_MAP` / `BEGIN_MESSAGE_MAP` / `ON_MESSAGE` / `END_MESSAGE_MAP`
 * all expanded to NOTHING — so every ON_MESSAGE row in the game was decorative.
 * The game sends 20 distinct WM_* types; S157 caught four of them firing and
 * dying in one ordinary run (WM_GETARTWORK, WM_GETXYOFFSET, WM_RELEASELASTFILE,
 * WM_GETX2FLAG), several with real implemented handlers on the other side.
 *
 * This is the §8-MA83 class ("the dispatcher answers 0 for routes it never
 * implemented") and the §8-MA91 class (empty macros silently discarding a
 * registration the game source makes) at the same time.
 *
 * HOW IT WORKS
 * ------------
 * The macros now register, per class, a table of (message -> thunk). The thunk
 * is emitted INSIDE a static member function of that class (declared by
 * DECLARE_MESSAGE_MAP), which is what gives it access to the game's own
 * `MSG2_*` adapters — those are `private:` and wrap every handler in a uniform
 * `LRESULT MSG2_<name>(int,int)`, which is the original authors' own answer to
 * the handlers' varying signatures (0/1/2 args, void/non-void).
 *
 * THE §8z TRAP, AVOIDED BY CONSTRUCTION
 * -------------------------------------
 * The OCX eventsink matches `type_info` EXACTLY with no base-class walk, which
 * made every event registered on a base class dead for the port's entire life
 * (S144). The same trap applies here: a message sent to a derived dialog must
 * still find a handler registered on RDialog. `BEGIN_MESSAGE_MAP(theClass,
 * baseClass)` already names the base, so we record the chain and WALK IT at
 * lookup — derived first, then up. Exact-match-only would silently reproduce
 * the very bug this file exists to fix.
 * ========================================================================== */

#include <typeinfo>
#include <map>
#include <vector>
#include <cstdio>
#include <cstdlib>

typedef long (*bob_mm_thunk)(void* self, int a, int b);

namespace {

struct Key {
    const std::type_info* cls;
    unsigned msg;
    bool operator<(const Key& o) const {
        if (cls != o.cls) return cls < o.cls;
        return msg < o.msg;
    }
};

std::map<Key, bob_mm_thunk>& table() {
    static std::map<Key, bob_mm_thunk> t;
    return t;
}

/* class -> its base, as named by BEGIN_MESSAGE_MAP(theClass, baseClass) */
std::map<const std::type_info*, const std::type_info*>& chain() {
    static std::map<const std::type_info*, const std::type_info*> c;
    return c;
}

}  /* namespace */

/* S158b: registered classes and a dynamic_cast probe for each (see below). */
std::vector<std::pair<const std::type_info*, int (*)(void*)> >& probes() {
    static std::vector<std::pair<const std::type_info*, int (*)(void*)> > p;
    return p;
}

namespace {

bool trace() { return getenv("BOB_TRACE_MSG") != 0; }

}  /* namespace */

extern "C" void bob_msgmap_add(const void* tinfo, unsigned msg, bob_mm_thunk fn)
{
    if (!tinfo || !fn) return;
    Key k; k.cls = (const std::type_info*)tinfo; k.msg = msg;
    table()[k] = fn;
}

/* S158b: per-class runtime type probe, `dynamic_cast<T*>(p) != 0`, emitted from the macro
   where T is known. Needed because the declared map base is often NOT the real C++ base:
   LWDirectives declares `BEGIN_MESSAGE_MAP(LWDirectives, CDialog)` but derives from
   RowanDialog, and has no ON_MESSAGE rows of its own -- so the chain walk goes
   LWDirectives -> CDialog and never reaches RDialog's handlers. The S158 census measured
   exactly that: WM_GETARTWORK/WM_GETXYOFFSET/WM_RELEASELASTFILE dispatch for CRToolBar and
   RFullPanelDial, and the SAME ids still miss for other receivers. Probes make the lookup
   inheritance-correct regardless of what the map declares. */
std::vector<std::pair<const std::type_info*, int (*)(void*)> >& probes();

extern "C" void bob_msgmap_probe(const void* tinfo, int (*fn)(void*))
{
    if (!tinfo || !fn) return;
    probes().push_back(std::make_pair((const std::type_info*)tinfo, fn));
}

extern "C" void bob_msgmap_chain(const void* derived, const void* base)
{
    if (!derived || !base || derived == base) return;
    chain()[(const std::type_info*)derived] = (const std::type_info*)base;
}

/* Walk derived -> base looking for a handler for `msg`. Returns 1 and writes
   *out if one ran. `depth` is bounded so a malformed chain cannot spin. */
extern "C" int bob_msgmap_call(const void* tinfo, unsigned msg, void* self,
                               int a, int b, long* out)
{
    if (!tinfo || !self) return 0;
    const std::type_info* c = (const std::type_info*)tinfo;
    for (int depth = 0; c && depth < 16; depth++) {
        Key k; k.cls = c; k.msg = msg;
        std::map<Key, bob_mm_thunk>::iterator it = table().find(k);
        if (it != table().end()) {
            long r = it->second(self, a, b);
            if (out) *out = r;
            if (trace()) {
                static int shown = 0;
                if (shown < 40) {   /* deduped-ish: S142's per-call trace wrote 70 MB */
                    shown++;
                    fprintf(stderr, "[msg] DISPATCHED 0x%03x (WM_USER+%d) to %s (depth %d) -> %ld\n",
                            msg, (int)msg - 0x400, c->name(), depth, r);
                }
            }
            return 1;
        }
        std::map<const std::type_info*, const std::type_info*>::iterator ch = chain().find(c);
        c = (ch == chain().end()) ? 0 : ch->second;
    }

    /* S158b: the declared chain missed. Fall back to asking each registered class whether
       this object actually IS one, via dynamic_cast -- correct regardless of what
       BEGIN_MESSAGE_MAP declared as the base. Only reached on a miss, so the O(n) scan costs
       nothing on the common path. */
    for (size_t i = 0; i < probes().size(); i++) {
        const std::type_info* cand = probes()[i].first;
        Key k; k.cls = cand; k.msg = msg;
        std::map<Key, bob_mm_thunk>::iterator it = table().find(k);
        if (it == table().end()) continue;             /* this class doesn't handle it */
        if (!probes()[i].second(self)) continue;       /* object isn't of this class */
        long r = it->second(self, a, b);
        if (out) *out = r;
        if (trace()) {
            static int shown = 0;
            if (shown < 40) {
                shown++;
                fprintf(stderr, "[msg] DISPATCHED 0x%03x (WM_USER+%d) to %s (via probe) -> %ld\n",
                        msg, (int)msg - 0x400, cand->name(), r);
            }
        }
        return 1;
    }
    return 0;
}

/* Diagnostic: how much did the macros actually register? A registration count
   of zero would mean the macro rewrite compiled but wired nothing -- the exact
   silent-success failure this file exists to remove, so it is worth being able
   to see it directly rather than inferring it from behaviour. */
extern "C" void bob_msgmap_stats(int* entries, int* classes)
{
    if (entries) *entries = (int)table().size();
    if (classes) *classes = (int)chain().size();
}
