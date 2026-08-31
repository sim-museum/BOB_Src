/* BoB Linux port - threads + multimedia timer backend (Phase 2c).
 *
 * The compat layer already has a real pthread-backed handle/event system
 * (compat_winbase.h: CreateThread/CreateEvent/SetEvent/WaitForSingleObject).
 * Two MFC/MM entry points were still stubbed and gate the render loop:
 *   - AfxBeginThread  -> the per-view draw thread (View3d::drawloop) never ran.
 *   - timeSetEvent    -> the periodic "move cycle" (Mast3d::StaticTimeProc ->
 *                        TimeProc) never fired.
 * Both are implemented here over pthreads. The Win32 event handshake the draw
 * thread/move-cycle use (doneframe etc.) is already real in compat_winbase.h.
 */
#ifdef FF_LINUX

#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <execinfo.h>
#include <sched.h>

/* ---- S46 diagnostic: trace what drives a view's drawing state to D_CLOSE ---
 * The per-view draw thread (View3d::drawloop) returns permanently when
 * drawing==D_CLOSE; if nothing re-spawns it the window freezes ("hang"). This
 * env-gated (BOB_TRACE_DRAW) helper prints a backtrace at each D_CLOSE setter
 * site (STUB3D.CPP) plus draw-thread start/exit below, so a native repro names
 * the exact culprit without gdb perturbing the race. Default-off, no-op unless
 * BOB_TRACE_DRAW is set. */
extern "C" void bob_trace_draw(const char* where)
{
	if (!getenv("BOB_TRACE_DRAW")) return;
	fprintf(stderr, "[draw] %s (tid=%lu)\n", where, (unsigned long)pthread_self());
	void* bt[32];
	int n = backtrace(bt, 32);
	backtrace_symbols_fd(bt, n, 2 /*stderr*/);
	fflush(stderr);
}

/* ---- S61: draw-thread liveness registry -------------------------------------
 * AfxBeginThread's only caller is View3d's draw thread (drawloop). It renders
 * `View3d::View_Point`; View3d teardown (~View3d -> MakeResize) `delete`s View_Point.
 * The old CEvent handshake (View3d::WaitEndDraw) is racy -- it only waits when
 * drawing==D_YES at call time -- so the main thread could free View_Point while the
 * draw thread was still in RenderLandscape (ASan heap-use-after-free on the render
 * thread, S60/S61, in the front-end->flight launch where views are created/destroyed).
 * This registry lets ~View3d block until the draw thread has actually LEFT drawloop
 * (and the trampoline has returned -> no more rendering), keyed by the View3d* (arg).
 * The draw thread clears `active` only AFTER a.proc returns, so once active==0 the
 * render is provably finished. bob_wait_drawthread_exit() never waits when called ON
 * that draw thread (it's exiting itself -- avoids a self-deadlock). */
typedef unsigned int (*bob_threadproc)(void*);

#define BOB_MAXDRAW 16
struct DrawReg { void* arg; pthread_t tid; volatile int active; };
static DrawReg g_draw[BOB_MAXDRAW];
static pthread_mutex_t g_drawlock = PTHREAD_MUTEX_INITIALIZER;

static int dt_register(void* arg, pthread_t tid) {
	pthread_mutex_lock(&g_drawlock);
	int slot = -1;
	for (int i=0;i<BOB_MAXDRAW;i++) if (!g_draw[i].arg || !g_draw[i].active) { slot=i; break; }
	if (slot < 0) slot = 0;	/* pool exhausted (never expected) -- clobber oldest */
	g_draw[slot].arg=arg; g_draw[slot].tid=tid; g_draw[slot].active=1;
	pthread_mutex_unlock(&g_drawlock);
	return slot;
}
static void dt_done(int slot) {
	if (slot>=0 && slot<BOB_MAXDRAW) g_draw[slot].active=0;	/* render finished */
}
extern "C" void bob_wait_drawthread_exit(void* arg) {
	pthread_t self = pthread_self();
	pthread_mutex_lock(&g_drawlock);
	DrawReg* r = NULL;
	for (int i=0;i<BOB_MAXDRAW;i++) if (g_draw[i].arg==arg && g_draw[i].active) { r=&g_draw[i]; break; }
	bool isself = (r && pthread_equal(r->tid, self));
	pthread_mutex_unlock(&g_drawlock);
	if (!r || isself) return;	/* no live draw thread for this view, or we ARE it */
	for (int guard=0; r->active; ++guard) {	/* r points into the static pool -> stays valid */
		sched_yield();
		if (guard > 2000000) { fprintf(stderr,"[draw] wait_drawthread_exit: timeout (%p)\n", arg); fflush(stderr); break; }
	}
}

/* ---- AfxBeginThread: run an MFC AFX_THREADPROC on a detached pthread ----- */
struct ThreadArg { bob_threadproc proc; void* arg; };
static void* thread_trampoline(void* p) {
	ThreadArg a = *(ThreadArg*)p; free(p);
	int dtslot = dt_register(a.arg, pthread_self());	/* S61: mark this view's draw thread live */
	bob_trace_draw("thread_trampoline: ENTER (draw/worker thread start)");
	if (a.proc) a.proc(a.arg);
	bob_trace_draw("thread_trampoline: EXIT (thread returned -- e.g. drawloop saw D_CLOSE)");
	dt_done(dtslot);	/* S61: render finished; ~View3d may now free View_Point. Before any _exit. */
	/* S46: AfxBeginThread's only caller is View3d::drawloop (the flight draw
	 * thread), so reaching here means flight ended (View3d::drawloop saw
	 * D_CLOSE -- e.g. the pilot pressed F12/KEY_CONFIGMENU or Alt+X/EXITKEY).
	 * In the BOB_BOOT_FRONTEND boot-to-flight scaffold there is NO front-end
	 * menu to return to, so the window would otherwise freeze on the last
	 * frame (CMIGApp::Run keeps pumping with nothing to draw). Exit cleanly
	 * instead -- mirrors the SDL_QUIT path (bob_video.cpp). NOT done for the
	 * real front-end (BOB_FRONTEND), where closing flight returns to the menu
	 * and a new view/draw thread is created for the next mission. */
	if (getenv("BOB_BOOT_FRONTEND")) {
		fprintf(stderr, "[draw] flight draw thread ended in boot-to-flight scaffold "
		                "(no menu to return to) -> clean exit\n");
		fflush(stderr);
		_exit(0);
	}
	return NULL;
}
extern "C" void bob_begin_thread(bob_threadproc proc, void* arg)
{
	if (!proc) return;
	ThreadArg* a = (ThreadArg*)malloc(sizeof(ThreadArg));
	a->proc = proc; a->arg = arg;
	pthread_t t;
	if (pthread_create(&t, NULL, thread_trampoline, a) == 0)
		pthread_detach(t);          /* draw loops run until their view closes */
	else
		free(a);
}

/* ---- timeSetEvent: a periodic (or one-shot) multimedia timer ------------- */
typedef void (*bob_timecb)(unsigned, unsigned, unsigned long, unsigned long, unsigned long);

struct Timer {
	bob_timecb    cb;
	unsigned long user;
	unsigned      delayMs;
	int           periodic;
	volatile int  kill;
	pthread_t     th;
	int           inUse;
};
#define BOB_MAX_TIMERS 32
static Timer g_timers[BOB_MAX_TIMERS];
static pthread_mutex_t g_timerLock = PTHREAD_MUTEX_INITIALIZER;

/* R18 (2026-08-31): the periodic timer must keep an ABSOLUTE schedule.
   The old loop slept `delayMs` in 5 ms usleep slices and then ran the callback, so every period cost
   delayMs + 8 sleep-wakeup overshoots + the callback's own runtime, and the error ACCUMULATED: the
   move cycle ran slow, and with it `timeofday += FRAMETIME` -- the game's own clock. R16-S5 measured
   the consequence downstream (the padlock extrapolation drifted ~20 ms per second against wall time,
   reaching its 1000 ms clamp in ~50 s) and fixed that symptom by reading the wall clock instead of
   counting ticks. This is the cause.
   clock_nanosleep(TIMER_ABSTIME) against a fixed next-deadline keeps the long-run rate exact: a late
   callback is absorbed by the next period rather than pushing the whole schedule back. If the loop
   falls more than one period behind (a stall), the deadline is re-based to now so it does not then
   fire a burst of catch-up callbacks.
   BOB_TIMER_SLICES=1 restores the old accumulate-and-drift loop -- the negative control for
   tools/bob_r18_timer.sh. BOB_TRACE_TIMER=1 reports the achieved period. */
static void* timer_thread(void* p)
{
	Timer* t = (Timer*)p;
	const int legacy = getenv("BOB_TIMER_SLICES") != NULL;
	const int trace  = getenv("BOB_TRACE_TIMER") != NULL;
	struct timespec next; clock_gettime(CLOCK_MONOTONIC, &next);
	struct timespec t0 = next; long fired = 0;
	const long periodNs = (long)(t->delayMs ? t->delayMs : 1) * 1000000L;
	do {
		if (legacy) {
			/* sleep in small slices so timeKillEvent is responsive */
			unsigned remaining = t->delayMs ? t->delayMs : 1;
			while (remaining && !t->kill) {
				unsigned slice = remaining > 5 ? 5 : remaining;
				usleep(slice * 1000);
				remaining -= slice;
			}
		} else {
			next.tv_nsec += periodNs;
			while (next.tv_nsec >= 1000000000L) { next.tv_nsec -= 1000000000L; next.tv_sec++; }
			struct timespec now; clock_gettime(CLOCK_MONOTONIC, &now);
			long behindNs = (now.tv_sec - next.tv_sec) * 1000000000L + (now.tv_nsec - next.tv_nsec);
			if (behindNs > periodNs) next = now;      /* stalled: re-base, do not burst */
			else while (!t->kill &&
			            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR) { }
		}
		if (t->kill) break;
		if (t->cb) t->cb((unsigned)(t - g_timers) + 1, 0, t->user, 0, 0);
		if (trace && ++fired % 250 == 0) {
			struct timespec now; clock_gettime(CLOCK_MONOTONIC, &now);
			double el = (now.tv_sec - t0.tv_sec) + (now.tv_nsec - t0.tv_nsec) / 1e9;
			fprintf(stderr, "[timer] id=%u fired=%ld in %.2fs -> %.3f ms/period (nominal %u) %s\n",
			        (unsigned)(t - g_timers) + 1, fired, el, 1000.0 * el / (double)fired,
			        t->delayMs, legacy ? "[legacy slices]" : "[absolute deadline]");
			fflush(stderr);
		}
	} while (t->periodic && !t->kill);
	pthread_mutex_lock(&g_timerLock);
	t->inUse = 0;
	pthread_mutex_unlock(&g_timerLock);
	return NULL;
}

/* fdwTimer: TIME_ONESHOT=0, TIME_PERIODIC=1 (bit 0) */
extern "C" unsigned int bob_time_set_event(unsigned delayMs, unsigned /*res*/,
		bob_timecb cb, unsigned long user, unsigned fdwTimer)
{
	if (getenv("BOB_NO_TIMER")) return 1;   /* diagnostic: no real timer thread */
	pthread_mutex_lock(&g_timerLock);
	int slot = -1;
	for (int i = 0; i < BOB_MAX_TIMERS; i++) if (!g_timers[i].inUse) { slot = i; break; }
	if (slot < 0) { pthread_mutex_unlock(&g_timerLock); return 0; }
	Timer* t = &g_timers[slot];
	memset(t, 0, sizeof(*t));
	t->cb = cb; t->user = user; t->delayMs = delayMs;
	t->periodic = (fdwTimer & 1) ? 1 : 0;
	t->inUse = 1; t->kill = 0;
	if (pthread_create(&t->th, NULL, timer_thread, t) != 0) { t->inUse = 0; pthread_mutex_unlock(&g_timerLock); return 0; }
	pthread_detach(t->th);
	pthread_mutex_unlock(&g_timerLock);
	return (unsigned)slot + 1;          /* timer id (non-zero) */
}

extern "C" unsigned int bob_time_kill_event(unsigned id)
{
	if (id == 0 || id > BOB_MAX_TIMERS) return 0;
	Timer* t = &g_timers[id - 1];
	t->kill = 1;                        /* the timer thread frees its slot */
	return 0;                           /* MMSYSERR_NOERROR */
}

#endif /* FF_LINUX */
