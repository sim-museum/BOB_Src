/* BoB Linux port - process entry point.
 *
 * On Windows the entry was MFC's WinMain -> AfxWinMain -> theApp.InitInstance().
 * The CWinApp subclass (CMIGApp) and its global instance live in the MFC module;
 * full runtime bring-up (SDL2 window + OpenGL present + OpenAL) is the next phase.
 * For now this provides the ELF entry so a `bob` binary links and starts. */
#ifdef FF_LINUX

#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* expose REG_* register indices in <ucontext.h> */
#endif
#include <iostream>
#include <cstdio>
#include <cstdlib>    /* getenv/setenv */
#include <cstring>    /* strstr, memset */
#include <unistd.h>   /* _exit, getcwd */
#include <execinfo.h> /* backtrace, backtrace_symbols_fd */
#include <signal.h>
#include <ucontext.h>
#include <sys/syscall.h>

/* Static-init-order fix: some game globals (e.g. the Lib3D object created via
   Inst3d::commonkeymaps' TU init) construct a std:: stream in their ctor, which
   copies the global std::locale. That locale is set up by std::ios_base::Init
   (the <iostream> static). Force that Init to run FIRST (highest init_priority)
   so it precedes every default-priority game global ctor — otherwise the stream
   ctor reads an uninitialised locale and segfaults. */
static std::ios_base::Init __bob_iostream_init __attribute__((init_priority(101)));

/* Runtime bring-up: drive the real MFC boot path (theApp.InitInstance) via the
   C-linkage hook defined in MIG.CPP. Set BOB_RUN_INIT=0 to skip (link-only run). */
extern "C" int bob_init_instance(void);
extern "C" int bob_run(void);
extern "C" int bob_video_smoketest(void);
extern "C" int bob_render_smoketest(void);
extern "C" int bob_input_smoketest(void);
extern "C" void bob_music_selftest(const char* path);   /* bob_music.cpp (DirectMusic->FluidSynth) */

/* --- crash handler (adopted from the sister MiG Alley port, 2026-07-19) -------------
   Same engine, same arch (i386), same compat lineage, so the register extraction from
   the ucontext transfers directly. Prints signal/tid/fault_addr, the full i386 register
   file, then a backtrace, then re-raises with SIG_DFL so the process still dies (and
   still drops a core) exactly as it would have.

   Why the registers matter here: most of this port's crash archaeology is vtable-slot
   and rasterizer-OOB work — comparing fault_addr against edi (span-filler destination
   write) vs esi+ebx (texture read) localises it immediately, and eip=0 pins a called
   NULL/garbage function pointer.

   BOB_NO_CRASH_BT disables installation entirely (escape hatch: gdb/ASan/valgrind runs,
   or anything that wants the pristine default disposition). */
static void bob_crash_handler(int sig, siginfo_t* si, void* ucv)
{
	void* bt[48]; int n = backtrace(bt, 48);
	fprintf(stderr, "\n=== CRASH: signal %d (tid %ld) fault_addr=%p ===\n",
		sig, (long)syscall(SYS_gettid), si ? si->si_addr : (void*)0);
#if defined(__i386__)
	if (ucv) {
		greg_t* r = ((ucontext_t*)ucv)->uc_mcontext.gregs;
		fprintf(stderr, "  eip=%08x eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x ebp=%08x esp=%08x\n",
			(unsigned)r[REG_EIP],(unsigned)r[REG_EAX],(unsigned)r[REG_EBX],(unsigned)r[REG_ECX],
			(unsigned)r[REG_EDX],(unsigned)r[REG_ESI],(unsigned)r[REG_EDI],(unsigned)r[REG_EBP],(unsigned)r[REG_ESP]);
	}
#endif
	backtrace_symbols_fd(bt, n, 2);
	signal(sig, SIG_DFL); raise(sig);
}

static void bob_install_crash_handler(int sig)
{
	struct sigaction sa; memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = bob_crash_handler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, 0);
}

static void bob_install_crash_handlers(void)
{
	if (getenv("BOB_NO_CRASH_BT")) return;
	/* Prime backtrace() before any signal can arrive (refinement adopted from the
	   FreeFalcon port): its FIRST call lazily dlopen()s the unwinder, which allocates.
	   If that first call happens inside the handler while the crashing thread already
	   holds the malloc lock, the handler deadlocks after printing the CRASH header and
	   emits no frames at all. Calling it once here makes the in-handler call reentrant. */
	{ void* prime[4]; (void)backtrace(prime, 4); }
	bob_install_crash_handler(SIGSEGV);
	bob_install_crash_handler(SIGABRT);
	bob_install_crash_handler(SIGBUS);
}

int main(int argc, char** argv)
{
	bob_install_crash_handlers();
	(void)argc; (void)argv;
	fprintf(stderr,
		"Rowan's Battle of Britain - Linux native port\n"
		"  All %d source modules link.\n", 16);

	/* Runtime bring-up is in progress: InitInstance() drives the real MFC boot
	   (registry, OLE, doc templates, command-line parse, ProcessShellCommand) and
	   currently stops at the first main-window use -- no CMainFrame is created yet
	   (the doc/view framework + window backend is the next subsystem). Opt in with
	   BOB_RUN_INIT=1 to drive it; the default run stays clean. */
	/* Music path self-test (diagnostic, default-off): drive the game's own DirectMusic
	   call sequence over the FluidSynth backend with a music file read from disk.
	   BOB_MUSIC_SELFTEST=<file.xmi|.mid>. */
	if (const char* mst = getenv("BOB_MUSIC_SELFTEST")) {
		bob_music_selftest(mst);
		_exit(0);
	}
	if (getenv("BOB_VID_SMOKETEST")) {
		fprintf(stderr, "  Video smoke test (SDL2 window + GL present)...\n");
		bob_video_smoketest();
		_exit(0);
	}
	if (getenv("BOB_RENDER_SMOKETEST")) {
		fprintf(stderr, "  Render smoke test (D3D7 device -> GL textured quad)...\n");
		bob_render_smoketest();
		_exit(0);
	}
	if (getenv("BOB_INPUT_SMOKETEST")) {
		fprintf(stderr, "  Input smoke test (DirectInput keyboard -> SDL)...\n");
		bob_input_smoketest();
		_exit(0);
	}

	/* Sprint 7 (env-var-free default boot): when run from the game's install directory we can
	   boot straight into the real front-end with NO BOB_* env vars -- the DoD end state.
	   1) Derive BOB_DRIVE_C from the cwd's `drive_c` ancestor if unset (the game is launched from
	      <drive_c>/Program Files/Rowan Software/Battle Of Britain, like the original).
	   2) Default the real front-end on (BOB_FRONTEND + BOB_OLE_DRAW) unless a scaffold/smoketest
	      mode is selected. 3) Auto-run InitInstance when the data path is known.
	   Escape hatches preserved: BOB_NO_RUN forces the old link-only safe default; BOB_RUN_INIT=1
	      still forces a run; running from a non-install dir (no data) falls back to the safe message
	      (so bare `./bob` from the repo still exits 0 cleanly). */
	if (!getenv("BOB_DRIVE_C")) {
		static char cwd[4096];
		if (getcwd(cwd, sizeof(cwd))) {
			char* hit = NULL; char* s = cwd;
			while ((s = strstr(s, "/drive_c")) != NULL) { hit = s; s += 8; }  /* last occurrence */
			if (hit && (hit[8] == '/' || hit[8] == '\0')) {
				hit[8] = '\0';                                /* truncate to <...>/drive_c */
				setenv("BOB_DRIVE_C", cwd, 1);
				fprintf(stderr, "  derived BOB_DRIVE_C=%s (from cwd)\n", cwd);
			}
		}
	}
	bool haveData = getenv("BOB_DRIVE_C") != NULL;
	bool forceRun = getenv("BOB_RUN_INIT") && getenv("BOB_RUN_INIT")[0] == '1';
	bool wantRun  = !getenv("BOB_NO_RUN") && (forceRun || haveData);

	if (wantRun) {
		/* Default the real front-end on (unless the 3D-flight scaffold is selected). */
		if (!getenv("BOB_FRONTEND") && !getenv("BOB_BOOT_FRONTEND")) setenv("BOB_FRONTEND", "1", 0);
		if (!getenv("BOB_OLE_DRAW")) setenv("BOB_OLE_DRAW", "1", 0);
		fprintf(stderr, "  Driving CMIGApp::InitInstance()...\n");
		int ok = bob_init_instance();
		fprintf(stderr, "  InitInstance() returned %d\n", ok);
		if (ok) {
			fprintf(stderr, "  Entering CMIGApp::Run()...\n");
			bob_run();
		}
	} else {
		fprintf(stderr,
			"  Runtime bring-up: no game data found (set BOB_DRIVE_C or run from the install"
			" dir; BOB_NO_RUN forces this link-only safe default).\n");
	}

	/* Global dtors (e.g. Mast3d::~Mast3d -> Sound::ShutDownSound) assume their
	   subsystems were brought up by InitInstance(), which hasn't run yet, so
	   running them at exit derefs uninitialised DirectSound/3D state. Until the
	   runtime loop initialises those subsystems, skip C++ static teardown and
	   let the OS reclaim the process. */
	fflush(NULL);
	_exit(0);
}

#endif /* FF_LINUX */
