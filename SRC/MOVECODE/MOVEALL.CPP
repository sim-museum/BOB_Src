/*
	 Battle of Britain
	 Copyright (C) 2000, 2001 Empire Interactive (Europe) Ltd,
	 677 High Road, North Finchley, London N12 0DA

	 Please see the document licence.doc for the full licence agreement

2. LICENCE
 2.1 	
 	Subject to the provisions of this Agreement we now grant to you the 
 	following rights in respect of the Source Code:
  2.1.1 
  	the non-exclusive right to Exploit  the Source Code and Executable 
  	Code on any medium; and 
  2.1.2 
  	the non-exclusive right to create and distribute Derivative Works.
 2.2 	
 	Subject to the provisions of this Agreement we now grant you the
	following rights in respect of the Object Code:
  2.2.1 
	the non-exclusive right to Exploit the Object Code on the same
	terms and conditions set out in clause 3, provided that any
	distribution is done so on the terms of this Agreement and is
	accompanied by the Source Code and Executable Code (as
	applicable).

3. GENERAL OBLIGATIONS
 3.1 
 	In consideration of the licence granted in clause 2.1 you now agree:
  3.1.1 
	that when you distribute the Source Code or Executable Code or
	any Derivative Works to Recipients you will also include the
	terms of this Agreement;
  3.1.2 
	that when you make the Source Code, Executable Code or any
	Derivative Works ("Materials") available to download, you will
	ensure that Recipients must accept the terms of this Agreement
	before being allowed to download such Materials;
  3.1.3 
	that by Exploiting the Source Code or Executable Code you may
	not impose any further restrictions on a Recipient's subsequent
	Exploitation of the Source Code or Executable Code other than
	those contained in the terms and conditions of this Agreement;
  3.1.4 
	not (and not to allow any third party) to profit or make any
	charge for the Source Code, or Executable Code, any
	Exploitation of the Source Code or Executable Code, or for any
	Derivative Works;
  3.1.5 
	not to place any restrictions on the operability of the Source 
	Code;
  3.1.6 
	to attach prominent notices to any Derivative Works stating
	that you have changed the Source Code or Executable Code and to
	include the details anddate of such change; and
  3.1.7 
  	not to Exploit the Source Code or Executable Code otherwise than
	as expressly permitted by  this Agreement.

questions about this file may be asked at bob@rowansoftware.com a
better place to ask is http://www.simhq.com/ or even :-
http://www.simhq.com/cgi-bin/boards/cgi-bin/forumdisplay.cgi?action=topics&forum=Battle+of+Britain&number=40&DaysPrune=20&LastLogin=
*/

//------------------------------------------------------------------------------
//Filename       moveall.cpp
//System         
//Author         Jim Taylor
//Date           Thu 15 Feb 1996
//Description    The code which moves all the items in the world
//				 This module also manages the lists which move the objects
//------------------------------------------------------------------------------
//DeadCode RJS 11Jul00 #define		DBGMEMTEST

#include <time.h>
#include <math.h>
#include <string.h>
#include	"dosdefs.h"											//JIM 17Nov96
	#define F_COMMON
	#define F_GRAFIX
	#define F_BATTLE
#include	"files.g"  
//TEMPCODE DAW 13/04/00 #include	"textref.h"
#include	"world.h"
#include	"worldinc.h"
#include	"flymodel.h"
#include	"transite.h"
#include	"Files.g"
#include	"persons2.h"
#include	"ai.h"
#include	"ranges.h"
#include	"mymath.h"
#include	"miles.h"
#include	"winmove.h"											//ARM 06Dec96
#include	"replay.h"
#include	"files.g"
#include	"worldinc.h"
#include	"planetyp.h"
#include	"myangles.h"
#include	"enumbits.m"
#include	"mytime.h"											//RDH 25Apr96
#if BOB_LINUX
#include	"keytest.h"	/* PATCH-PORT-1: BDG autopilot key */
#endif
//MATHABLE	(ANGLES)
#include	"mymath.h"
#include	"vertex.h"
#include	"matrix.h"											//RDH 23May96
//TEMPCODE DAW 13/04/00 #include	"textref.h"
#include	"viewsel.h"											//PD 19Feb96
#include	"hardpasm.h"										//RDH 14Mar96
#include	"3dcom.h"											//PD 16Apr96
#include	"landscap.h"									//PD 15Mar96
#include	"missman2.h"										//JIM 22Aug96
#include	"savegame.h"
#include	"miles.h"											//DAW 04Jul96
#include	"world.h"											//DAW 04Jul96
#include	"transite.h"										//RDH 26Jul96
#include	"collided.h"										//PD 06Sep96
//DEADCODE DAW 04/02/00 #include	"speed.h"
#include	"ranges.h"
#include	"modvec.h"											//ARM 08Jan97
#include	"model.h"											//ARM 07Jan97
#include	"shapes.h"
#define	 sagmove_REQUIRED	SAGAirstruc:AirStruc
#include "sagmove.h"

#include	"monotxt.h"

//#define DEBUGMOVE

MobileItemPtr	mobileitem::SAGList=NULL;
MobileItemPtr	mobileitem::ACList=NULL;
MobileItemPtr	mobileitem::MobileList=NULL;
TransientItemPtr	TransientItem::TransientList=NULL;
WorldStuff*		mobileitem::currworld=NULL;
UByte	mobileitem::timerseed=0;
int		mobileitem::timeofday=0;


#ifndef	NDEBUG
//#define	_FRAMECOUNT_
#endif

static	bool	checkMemNow = false;

//extern Replay	_Replay;
//extern DPlay _DPlay;

#ifdef	_FRAMECOUNT_
static	int	frameCount = 0;												//RJS 14Aug00

#define AFX_CDECL __cdecl
typedef char CHAR;
#define CONST               const
typedef CONST CHAR *LPCSTR, *PCSTR;
typedef LPCSTR LPCTSTR;
void AFX_CDECL AfxTrace(LPCTSTR lpszFormat, ...);

#endif
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveAll
//Author		Jim Taylor
//Date			Thu 15 Feb 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
/* LOS-BFM-1 (item 48): where o is in ac's body frame (bank included): 0 visible, 1 under the nose, 2 under the belly or
   wings, 3 behind (armour, headrest, tail), 4 within 10 deg of the sun, 5 beyond 4 km */
int bdg_sight_zone(AirStrucPtr ac, AirStrucPtr o)
{
	const float K = 6.2831853f / 65536.0f;
	const float h = float(UWord(ac->hdg)) * K, pt = float(SWord(ac->pitch)) * K, rl = float(SWord(ac->roll)) * K;
	const float fx = sinf(h) * cosf(pt), fy = sinf(pt), fz = cosf(h) * cosf(pt);
	const float rx = cosf(h), ry = 0, rz = -sinf(h);
	const float ux = -sinf(h) * sinf(pt), uy = cosf(pt), uz = -cosf(h) * sinf(pt);
	const float R1x = rx * cosf(rl) - ux * sinf(rl), R1y = ry * cosf(rl) - uy * sinf(rl), R1z = rz * cosf(rl) - uz * sinf(rl);
	const float U1x = ux * cosf(rl) + rx * sinf(rl), U1y = uy * cosf(rl) + ry * sinf(rl), U1z = uz * cosf(rl) + rz * sinf(rl);
	const float dx = float(o->World.X - ac->World.X), dy = float(o->World.Y - ac->World.Y), dz = float(o->World.Z - ac->World.Z);
	const float d = sqrtf(dx * dx + dy * dy + dz * dz); if (d < 1) return 0;
	if (d > 400000.0f) return 5;
	const COORDS3D& sun = mobileitem::currworld->sunpos;
	const float sx = float(sun.X - ac->World.X), sy = float(sun.Y - ac->World.Y), sz = float(sun.Z - ac->World.Z);
	const float sd = sqrtf(sx * sx + sy * sy + sz * sz);
	if (sd > 1 && (dx * sx + dy * sy + dz * sz) / (d * sd) > 0.985f) return 4;
	const float bx = (dx * fx + dy * fy + dz * fz), by = (dx * R1x + dy * R1y + dz * R1z), bz = (dx * U1x + dy * U1y + dz * U1z);
	const float az = atan2f(by, bx) * 57.3f, el = atan2f(bz, sqrtf(bx * bx + by * by)) * 57.3f;
	if (el < -20.0f) return 2;
	if (fabsf(az) < 20.0f && el < -5.0f) return 1;
	if (fabsf(az) > 155.0f && el < 15.0f) return 3;
	return 0;
}

/* LOS-BFM-1 (item 48, PO 10-16 "BFM depends on line of sight - lose sight, lose the fight"): an AI fighter pilot whose
   target stays out of sight (bdg_sight_zone != 0) longer than he can keep track of it from memory -- 3 s for a novice,
   +1 s per 20 skill points (regular 7.5 s, ace 12 s; twice that for a bomber) -- loses it: Rowan's ArtInt::BreakOff (back to his waypoints, no
   target), and he must spot it again through ArtInt::Seen like any other bandit. A target held under the nose inside
   300 m (a tracking shot) is not lost. Sampled every 0.25 s. BOB_NO_SIGHT_LOSS=1 reverts; BOB_TRACE_SIGHT=1 traces. */
static void bdg_sight_loss(MobileItemPtr list)
{
	static int on = -1, tr = 0; if (on < 0) { on = getenv("BOB_NO_SIGHT_LOSS") ? 0 : 1; tr = getenv("BOB_TRACE_SIGHT") ? 1 : 0; }
	static long t = 0, tnext = 25, nlost = 0;
	if (!on || (t += Timer_Code.FRAMETIME) < tnext) return;
	tnext += 25;
	enum { NS = 1024 };
	static struct { unsigned id, uid; long unseen; } s[NS];
	static struct { unsigned id; long t; } s_lost[NS];	/* trace: time each pilot lost his target */
	if (tr) for (AirStrucPtr ac = AirStrucPtr(list); ac; ac = AirStrucPtr(ac->nextmobile)) {
		auto& g = s_lost[(unsigned(ac->uniqueID.count) * 2654435761u >> 8) & (NS - 1)];
		if (g.id == unsigned(ac->uniqueID.count) && g.t && ac->movecode == AUTO_COMBAT) {
			fprintf(stderr, "[sightregain] id=%d back in combat after %.1fs\n", int(ac->uniqueID.count), (t - g.t) / 100.0); g.t = 0; } }
	for (AirStrucPtr ac = AirStrucPtr(list); ac; ac = AirStrucPtr(ac->nextmobile))
	{
		if (ac->movecode != AUTO_COMBAT || ac->Status.deaded || ac == Persons2::PlayerSeenAC || ac == Persons2::PlayerGhostAC) continue;
		const int vis = int(ac->classtype->visible);
		if (vis != SPIT && vis != HURR && vis != ME109) continue;
		if (!ac->ai.unfriendly || ac->ai.unfriendly->Status.size != AIRSTRUCSIZE) continue;
		AirStrucPtr u = AirStrucPtr(ac->ai.unfriendly);
		auto& e = s[(unsigned(ac->uniqueID.count) * 2654435761u >> 8) & (NS - 1)];
		if (e.id != unsigned(ac->uniqueID.count) || e.uid != unsigned(u->uniqueID.count)) { e.id = ac->uniqueID.count; e.uid = u->uniqueID.count; e.unseen = 0; }
		const int z = bdg_sight_zone(ac, u);
		if (z == 0 || (z == 1 && ac->Distance3DSquared(&u->World) < 30000.0 * 30000.0)) { e.unseen = 0; continue; }
		e.unseen += 25;
		long limit = 300 + long(ac->ai.combatskill) * 5;
		if (u->classtype->aerobaticfactor == AEROBATIC_LOW) limit *= 2;	/* a bomber formation is big and flies straight on */
		if (e.unseen < limit) continue;
		nlost++;
		if (tr) fprintf(stderr, "[sightlost] t=%lds id=%d skill=%d target=%d zone=%d unseen=%.1fs range=%.0f m man=%d total=%ld\n", t / 100,
			int(ac->uniqueID.count), int(ac->ai.combatskill), int(u->uniqueID.count), z, e.unseen / 100.0,
			sqrt(ac->Distance3DSquared(&u->World)) / 100.0, int(ac->ai.manoeuvre), nlost);
		e.unseen = 0;
		Art_Int.BreakOff(ac);
		if (tr) { auto& g = s_lost[(unsigned(ac->uniqueID.count) * 2654435761u >> 8) & (NS - 1)]; g.id = ac->uniqueID.count; g.t = t; }
	}
}

#if defined(BOB_LINUX)
int bdg_rnd_who = 0;
void bdg_rnd_hook(void* ra) { fprintf(stderr, "[rndw] %p\n", ra); }
#endif
void mobileitem::MoveAll(WorldStuff& world)
{
	for(AirStrucPtr ac = AirStrucPtr(ACList); ac; ac = AirStrucPtr(ac->nextmobile))
		ac->ai.moved = FALSE;

#if defined(BOB_LINUX)
	static int bdg_trr = -1; if (bdg_trr < 0) bdg_trr = getenv("BOB_TRACE_RND") ? 1 : 0;	/* BOB_TRACE_RND=1: Rowan's shared rnd counter */
	static int bdg_rf = 0; unsigned bdg_rc0 = (unsigned)Math_Lib.GetRndCount();		/* around the sim move, first 100 frames */
	bdg_rnd_who = bdg_trr && bdg_rf < 100;
#endif
	MoveList(ACList, world);

#if defined(BOB_LINUX)
	bdg_rnd_who = 0;
	if (bdg_trr && bdg_rf < 100) fprintf(stderr, "[rnd] f=%d in=%u out=%u\n", bdg_rf++, bdg_rc0, (unsigned)Math_Lib.GetRndCount());
	{ extern long bdg_simtime_cs; bdg_simtime_cs += Timer_Code.FRAMETIME; }	/* sim clock for the per-pilot wander (ACMSIMPL.CPP) */
	bdg_sight_loss(ACList);
	{	/* LOS-BFM-1 (item 48, PO 10-16 "lose sight, lose the fight"): in combat Rowan's AI never loses its bandit
		   (ArtInt::Seen runs only for pilots not yet in combat). Census, every 1 s, for single-seat fighters in AUTO_COMBAT:
		   where in the pilot's own body frame (bank included) are (a) his target and (b) each enemy within 1.5 km that is
		   attacking him -- visible, under the nose, under the belly/wings, behind (armour, headrest, tail), in the sun, or
		   beyond 4 km. Also a roll-sign check (roll vs heading rate). BOB_TRACE_SIGHT=1. */
		static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_SIGHT") ? 1 : 0;
		static long t = 0, tnext = 100, trep = 3000; static long cnt[2][6], rsame, ropp;
		if (tr && (t += Timer_Code.FRAMETIME) >= tnext)
		{
			tnext += 100;
			const float K = 6.2831853f / 65536.0f;
			const COORDS3D& sun = mobileitem::currworld->sunpos;
			for (AirStrucPtr ac = AirStrucPtr(ACList); ac; ac = AirStrucPtr(ac->nextmobile))
			{
				if (ac->movecode != AUTO_COMBAT || ac->Status.deaded) continue;
				const int vis = int(ac->classtype->visible);
				if (vis != SPIT && vis != HURR && vis != ME109) continue;
				{ const float rl = float(SWord(ac->roll)) * K;
				  if (ac->fly.pModel && fabsf(rl) > 0.5f && fabsf(rl) < 1.4f) { const float dh = float(ac->fly.dhdg);
					if (dh != 0) { if ((dh > 0) == (rl > 0)) rsame++; else ropp++; } } }
				auto zone = [&](AirStrucPtr o) -> int { return bdg_sight_zone(ac, o); };
				AirStrucPtr u = (ac->ai.unfriendly && ac->ai.unfriendly->Status.size == AIRSTRUCSIZE) ? AirStrucPtr(ac->ai.unfriendly) : NULL;
				if (u) cnt[0][zone(u)]++;
				for (AirStrucPtr e = AirStrucPtr(ACList); e; e = AirStrucPtr(e->nextmobile))
					if (e->nationality != ac->nationality && e->ai.unfriendly == ac && !e->Status.deaded
					    && ac->Distance3DSquared(&e->World) < 150000.0 * 150000.0)
						cnt[1][zone(e)]++;
			}
			if (t >= trep) { trep += 3000;
				for (int k = 0; k < 2; k++) { long n = 0; for (int z = 0; z < 6; z++) n += cnt[k][z];
					fprintf(stderr, "[sight] t=%lds %s n=%ld visible=%ld nose=%ld belly=%ld behind=%ld sun=%ld far=%ld\n", t / 100,
						k ? "threat" : "target", n, cnt[k][0], cnt[k][1], cnt[k][2], cnt[k][3], cnt[k][4], cnt[k][5]); }
				fprintf(stderr, "[sight] rollsign same=%ld opp=%ld (roll>0 with dhdg>0)\n", rsame, ropp); }
		}
	}
	extern long bdg_pursuit_count;
	{	/* BOB-CLONE-SWEEP-1 (item 50, PO 10-17): find AI aircraft flying as clones in combat. Every 1 s each aircraft in
		   AUTO_COMBAT is sampled; every 10 s each same-side pair is tested over its last 10 s for a TRAIL clone (one flies
		   within 50 m of where the other was 1..8 s earlier) or a SIDE clone (a constant offset to within 15 m while more
		   than 60 m apart). BOB_TRACE_CLONE=1 prints each pair at most every 30 s with both manoeuvres. */
		static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_CLONE") ? 1 : 0;
		if (tr)
		{
			enum { NS = 512, RH = 64 };
			struct CS { unsigned id; int run, head; float x[RH], y[RH], z[RH]; AirStrucPtr ac; unsigned frame; };
			static CS cs[NS]; static long t = 0, tnext = 100, tpair = 1000; static unsigned frame = 0;
			static unsigned lastrep[NS][NS / 32]; static long s_pairs, s_trail, s_side;	/* bit set: reported in this 30 s block */
			t += Timer_Code.FRAMETIME;
			if (t >= tnext) {
				tnext += 100; frame++;
				for (AirStrucPtr ac = AirStrucPtr(ACList); ac; ac = AirStrucPtr(ac->nextmobile)) {
					CS& c = cs[(unsigned(ac->uniqueID.count) * 2654435761u >> 9) & (NS - 1)];
					if (c.id != unsigned(ac->uniqueID.count)) { c.id = ac->uniqueID.count; c.run = 0; c.head = 0; }
					if (ac->movecode != AUTO_COMBAT || ac->Status.deaded) { c.run = 0; continue; }
					c.head = (c.head + 1) % RH; c.x[c.head] = ac->World.X * 0.01f; c.y[c.head] = ac->World.Y * 0.01f; c.z[c.head] = ac->World.Z * 0.01f;
					c.run++; c.ac = ac; c.frame = frame;
				}
				if (t >= tpair) {
					tpair += 1000;
					{	/* census (cumulative, every 10 s): the detector can only speak when aircraft are in combat, and fights differ
						   in length, so compare clones per 1000 same-side pair tests */
						int nc = 0; for (int i = 0; i < NS; i++) if (cs[i].frame == frame && cs[i].run > 0) nc++;
						extern long bdg_pursuit_count;
						fprintf(stderr, "[clone] census t=%lds in_combat=%d pairs=%ld trail=%ld side=%ld per1000=%.1f pursuit_offsets=%ld\n", t / 100, nc,
						        s_pairs, s_trail, s_side, s_pairs ? 1000.0 * double(s_trail + s_side) / double(s_pairs) : 0.0, bdg_pursuit_count);
					}
					if ((tpair / 1000) % 3 == 0) memset(lastrep, 0, sizeof lastrep);
					for (int i = 0; i < NS; i++) {
						CS& A = cs[i]; if (A.run < 18 || A.frame != frame) continue;
						for (int j = i + 1; j < NS; j++) {
							CS& B = cs[j]; if (B.run < 18 || B.frame != frame || A.ac->nationality != B.ac->nationality) continue;
							if (lastrep[i][j >> 5] & (1u << (j & 31))) continue;
							s_pairs++;
							#define CPOS(C, k, v) C.v[((C.head - (k)) % RH + RH) % RH]
							float ox = 0, oy = 0, oz = 0;
							for (int k = 0; k < 10; k++) { ox += CPOS(B, k, x) - CPOS(A, k, x); oy += CPOS(B, k, y) - CPOS(A, k, y); oz += CPOS(B, k, z) - CPOS(A, k, z); }
							ox /= 10; oy /= 10; oz /= 10;
							float side = 0;
							for (int k = 0; k < 10; k++) { const float ex = CPOS(B, k, x) - CPOS(A, k, x) - ox, ey = CPOS(B, k, y) - CPOS(A, k, y) - oy, ez = CPOS(B, k, z) - CPOS(A, k, z) - oz; side += sqrtf(ex * ex + ey * ey + ez * ez); }
							side /= 10;
							const float sep = sqrtf(ox * ox + oy * oy + oz * oz);
							float best = 1e9f; int blag = 0;
							for (int lag = 1; lag <= 8; lag++) for (int dir = 0; dir < 2; dir++) {
								CS& P = dir ? A : B; CS& Q = dir ? B : A;	/* P follows Q's path lag s later */
								if (Q.run < 10 + lag) continue;
								float e = 0;
								for (int k = 0; k < 10; k++) { const float ex = CPOS(P, k, x) - CPOS(Q, k + lag, x), ey = CPOS(P, k, y) - CPOS(Q, k + lag, y), ez = CPOS(P, k, z) - CPOS(Q, k + lag, z); e += sqrtf(ex * ex + ey * ey + ez * ez); }
								e /= 10; if (e < best) { best = e; blag = dir ? lag : -lag; }
							}
							#undef CPOS
							const bool trail = best < 50.0f && sep > 60.0f, sidec = side < 15.0f && sep > 60.0f;
							if (trail || sidec) {
								lastrep[i][j >> 5] |= 1u << (j & 31); if (trail) s_trail++; else s_side++;
								fprintf(stderr, "[clone] t=%lds %s a=%d shape=%d man=%d b=%d shape=%d man=%d sep=%.0f trail_err=%.0f lag=%d side_err=%.0f lead=%d skill=%d,%d unf=%d,%d urange=%.0f,%.0f\n", t / 100,
								        trail ? "TRAIL" : "SIDE", int(A.ac->uniqueID.count), int(A.ac->classtype->visible), int(A.ac->ai.manoeuvre),
								        int(B.ac->uniqueID.count), int(B.ac->classtype->visible), int(B.ac->ai.manoeuvre), sep, best, blag, side,
								        int(A.ac->Follower() == B.ac || B.ac->Follower() == A.ac), int(A.ac->ai.combatskill), int(B.ac->ai.combatskill),
								        A.ac->ai.unfriendly ? int(A.ac->ai.unfriendly->uniqueID.count) : -1, B.ac->ai.unfriendly ? int(B.ac->ai.unfriendly->uniqueID.count) : -1,
								        A.ac->ai.unfriendly ? sqrtf(float(A.ac->Distance3DSquared(&A.ac->ai.unfriendly->World))) * 0.01f : -1.0f,
								        B.ac->ai.unfriendly ? sqrtf(float(B.ac->Distance3DSquared(&B.ac->ai.unfriendly->World))) * 0.01f : -1.0f);
							}
						}
					}
				}
			}
		}
	}
#endif
#if defined(BOB_LINUX)
	/* MP-6 (2026-09-13): DOES A REMOTE PLAYER'S AIRCRAFT EXIST IN THIS WORLD?
	   MA measured exactly this and found it does NOT: its host alone has 8 aircraft, and with a
	   client joined it still has 8, while ~1900 packets a side cross in flight. MP-5 proved BoB's
	   two instances both reach the 3-D; it did not prove they can SEE each other, and the same
	   question has to be asked here -- if both ports show it, the cause is in the shared Rowan comms
	   design rather than either port's plumbing.
	   The obvious oracle does not work: the ACMI walk's `listlen` trace lives in the replay recorder,
	   which is gated on `!_DPlay.Implemented` -- recording is DISABLED in multiplayer BY DESIGN, so a
	   two-instance run prints zero acmi lines (measured). Hence a census of its own, here, walking
	   the list the way the loop above it does.
	   BOB_TRACE_ACCOUNT=1. Reports a running MAX so the sample point cannot flatter the answer. */
	{
		static int dbg = -1;
		if (dbg < 0) dbg = getenv("BOB_TRACE_ACCOUNT") ? 1 : 0;
		if (dbg)
		{
			static long frames = 0; static int maxseen = 0;
			int cnt = 0;
			for (AirStrucPtr a = AirStrucPtr(ACList); a && cnt < 256; a = AirStrucPtr(a->nextmobile)) cnt++;
			if (cnt > maxseen) maxseen = cnt;
			if ((frames % 600) == 0)
				fprintf(stderr, "[accnt] aircraft in this world: now=%d MAX=%d (frame sample %ld)\n",
				        cnt, maxseen, frames / 600), fflush(stderr);
			frames++;
		}
	}
#endif

	SAGAirstruc::MoveAllSAGs(world);

	MoveList(MobileList, world);

#ifdef	_FRAMECOUNT_
	UWord cw=GETFPCW();
	::AfxTrace("Frame: %d\n",frameCount++);
	SETFPCW(cw);	
#endif
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveAllTrans
//Author		Martin Alderton
//Date			Wed 20 Mar 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void TransientItem::MoveAllTrans(WorldStuff& world)
{
	MoveTransList(TransientList, world);
}

static UByte dontmove=0;
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveList
//Author		Jim Taylor
//Date			Thu 15 Feb 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
#if BOB_LINUX
static void bdg_bomberfix(MobileItemPtr entry)
{
	/* PATCH-PORT-1, BDG BOB_BOMBERFIX (bdg.txt; the PO's is ON; manual 2.2.3): "a bomber with only one good engine and
	   the second on fire or feathered will not be able to keep up with the rest of the formation ... If a bomber has an
	   engine start to smoke due to damage, there is now a chance that it will pull out of formation and try to make it
	   back home. Every bomber will not do this." Not BDG's code (its new routine was not decoded): a Luftwaffe bomber
	   (AEROBATIC_LOW) flying in formation whose engine is damaged (BS_DAMLV1 or worse) decides once, 50/50 on the
	   game's own random generator, to break formation (Rowan's BreakForm) and head for home with Rowan's
	   MANOEUVRE_SCREWYOUGUYSIMGOINGHOME (full throttle, home base, then the egress route). Off in multiplayer.
	   BOB_TRACE_BOMBERFIX=1; test hook BOB_TEST_BOMBERDAMAGE_AT=<s> damages one engine of the first 3 bombers met. */
	static unsigned s_bfLeft[64]; static int s_bfNLeft = 0; static AirStrucPtr s_bfLead[64];
	if (entry->Status.size == AirStrucSize && getenv("BOB_TRACE_BOMBERFIX"))
		for (int q = 0; q < s_bfNLeft; q++)
			if (s_bfLeft[q] == entry->uniqueID.count)
			{
				static time_t last[64];
				if (time(0) - last[q] >= 5) { last[q] = time(0);
					AirStrucPtr b = (AirStrucPtr)entry, L = s_bfLead[q];
					const double d = L ? sqrt(double(b->Distance3DSquared(&L->World))) / 100.0 : -1;
					fprintf(stderr, "[bomberfix] uid=0x%x after leaving: movecode %d manoeuvre %d, %.0f m from its old leader uid=0x%x\n",
					        (unsigned)b->uniqueID.count, (int)b->movecode, (int)b->ai.manoeuvre, d, L ? (unsigned)L->uniqueID.count : 0u); }
			}
	if (entry->Status.size == AirStrucSize && !_DPlay.Implemented)
	{
		static int on = -1;
		if (on < 0) { extern int rowan_bdg_int(const char*, int); on = rowan_bdg_int("BOB_BOMBERFIX", 0); }
		AirStrucPtr b = (AirStrucPtr)entry;
		if (getenv("BOB_TRACE_BOMBERFIX") && b->nationality == NAT_LUF && b->classtype->aerobaticfactor == AEROBATIC_LOW)
		{	static unsigned seenU[64]; static int ns = 0; bool f = true;
			for (int q = 0; q < ns; q++) if (seenU[q] == b->uniqueID.count) f = false;
			if (f && ns < 64) { seenU[ns++] = b->uniqueID.count;
				fprintf(stderr, "[bomberfix] bomber uid=0x%x movecode %d anim %d numinsag %d formpos 0x%x\n", (unsigned)b->uniqueID.count,
				        (int)b->movecode, (int)SHAPESTUFF.GetShapePtr(b->shape)->AnimDataSize, (int)b->fly.numinsag, (int)b->formpos); } }
		if (on && b != Persons2::PlayerSeenAC && b != Persons2::PlayerGhostAC && b->nationality == NAT_LUF
			&& b->classtype->aerobaticfactor == AEROBATIC_LOW && b->movecode == AUTO_FOLLOWWP && !b->Status.deadtime
			&& SHAPESTUFF.GetShapePtr(b->shape)->AnimDataSize == AIRCRAFTANIM)
		{
			AircraftAnimData* ad = (AircraftAnimData*)b->Anim;
			static int testAt = -2; static time_t t0 = 0; static int damaged = 0;
			if (testAt == -2) { const char* e = getenv("BOB_TEST_BOMBERDAMAGE_AT"); testAt = e ? atoi(e) : -1; }
			if (testAt >= 0) { if (!t0) t0 = time(0); if (damaged < 3 && time(0) - t0 >= testAt && ad->ENGINELEFT < BS_DAMLV1) {
				ad->ENGINELEFT = BS_DAMLV2; damaged++;
				fprintf(stderr, "[bomberfix] test: left engine of uid=0x%x damaged\n", (unsigned)b->uniqueID.count); } }
			if (ad->ENGINELEFT >= BS_DAMLV1 || ad->ENGINERIGHT >= BS_DAMLV1)
			{
				static unsigned decided[256]; static int nd = 0;
				bool seen = false;
				for (int q = 0; q < nd; q++) if (decided[q] == b->uniqueID.count) seen = true;
				if (!seen && nd < 256)
				{
					decided[nd++] = b->uniqueID.count;
					const bool leave = Math_Lib.rnd(2) == 0;
					if (getenv("BOB_TRACE_BOMBERFIX"))
						fprintf(stderr, "[bomberfix] uid=0x%x engines %d/%d: %s\n", (unsigned)b->uniqueID.count,
						        (int)ad->ENGINELEFT, (int)ad->ENGINERIGHT, leave ? "leaves the formation for home" : "stays in formation");
					if (leave && s_bfNLeft < 64) { s_bfLead[s_bfNLeft] = b->FindFormpos0(); if (s_bfLead[s_bfNLeft] == b) s_bfLead[s_bfNLeft] = b->Follower(); s_bfLeft[s_bfNLeft++] = b->uniqueID.count; }
					if (leave)
					{
						b->BreakForm();
						b->movecode = AUTO_COMBAT;
						b->ai.manoeuvre = MANOEUVRE_SCREWYOUGUYSIMGOINGHOME;
						b->ai.ManStep = 0;
						b->manoeuvretime = 0;
					}
				}
			}
		}
	}
}
#endif

#if BOB_LINUX
/* BOB-PATCH-REST-1 40b, BDG gunners wounded/killed (manual 2.2.3, part of BOB_BOMBERFIX: "It is now possible to kill
   and wound the gunners in the bombers, including the Ju-87 and the Me-110"). Decoded from BDG 0.99 bob.exe:
   - Ju 87 / Bf 110 (shapes 0x20/0x18, routine 0x543df6): crew-area damage (CANOPY, anim 0x9d) > 2.0 and < 0xfd ->
     gunner hit, once (plus a radio call 1 in 2, not ported);
   - Do 17 / He 111 / Ju 88 (routine 0x543a38, second damage stage): average engine damage > 150, average wing damage
     (LEFT/RIGHT WINGOUT/IN) > 200 or crew-area damage > 100 -> gunner hit 1 in 2 / 1 in 3 / 1 in 2 (Ju 52 never);
   - effect (Rowan's LiveList::Event, BDG's skip at 0x481d4a): an aircraft whose gunner is hit fires no turret guns.
   AI aircraft only; off in multiplayer; same switch as the bomber fix (BOB_BOMBERFIX). BOB_TRACE_GUNNERS=1. */
static unsigned s_gunHit[512]; static int s_gunHitN = 0;
bool bdg_gunner_silenced(ItemPtr it)
{
	if (!s_gunHitN || !it || it->Status.size != AirStrucSize) return false;
	for (int q = 0; q < s_gunHitN; q++) if (s_gunHit[q] == it->uniqueID.count) {
		static unsigned told[64]; static int nt = 0; bool t = false;
		for (int k = 0; k < nt; k++) if (told[k] == it->uniqueID.count) t = true;
		if (!t && nt < 64 && getenv("BOB_TRACE_GUNNERS")) { told[nt++] = it->uniqueID.count;
			fprintf(stderr, "[gunners] uid=0x%x: turret turn skipped (gunner hit)\n", (unsigned)it->uniqueID.count); }
		return true; }
	return false;
}
static void bdg_gunnerfix(MobileItemPtr entry)
{
	if (entry->Status.size != AirStrucSize || _DPlay.Implemented) return;
	static int on = -1;
	if (on < 0) { extern int rowan_bdg_int(const char*, int); on = rowan_bdg_int("BOB_BOMBERFIX", 0); }
	if (!on) return;
	AirStrucPtr a = (AirStrucPtr)entry;
	if (a == Persons2::PlayerSeenAC || a == Persons2::PlayerGhostAC || a->Status.deadtime) return;
	const ShapeNum vis = a->classtype->visible;
	const bool twoSeat = (vis == JU87 || vis == ME110), bomber = (vis == DO17 || vis == HE11 || vis == JU88);
	if (!twoSeat && !bomber) return;
	if (SHAPESTUFF.GetShapePtr(a->shape)->AnimDataSize != AIRCRAFTANIM) return;
	static unsigned decided[512]; static int nd = 0;
	for (int q = 0; q < nd; q++) if (decided[q] == a->uniqueID.count) return;
	AircraftAnimData* ad = (AircraftAnimData*)a->Anim;
	{	/* test hook: BOB_TEST_GUNNERDAMAGE_AT=<s> hits the crew area (CANOPY = BS_DAMLV2) of the first 3 bombers/two-seaters met */
		static int testAt = -2; static time_t t0 = 0; static int n = 0;
		if (testAt == -2) { const char* e = getenv("BOB_TEST_GUNNERDAMAGE_AT"); testAt = e ? atoi(e) : -1; }
		if (testAt >= 0 && n < 3) { if (!t0) t0 = time(0); if (time(0) - t0 >= testAt && ad->CANOPY < BS_DAMLV1) { ad->CANOPY = BS_DAMLV2; n++;
			fprintf(stderr, "[gunners] test: crew area of uid=0x%x hit\n", (unsigned)a->uniqueID.count); } }
	}
	bool stage, hit;
	if (twoSeat)
	{
		stage = (ad->CANOPY > 2 && ad->CANOPY < 0xfd);
		hit = stage;
	}
	else
	{
		const float eng = (ad->ENGINELEFT + ad->ENGINERIGHT) / 2.0f;
		const float wing = (ad->LEFTWINGOUT + ad->LEFTWINGIN + ad->RIGHTWINGOUT + ad->RIGHTWINGIN) / 4.0f;
		stage = (eng > 150.0f || wing > 200.0f || ad->CANOPY > 100);
		hit = stage && (Math_Lib.rnd(vis == HE11 ? 3 : 2) == 0);
	}
	if (!stage) return;
	if (nd < 512) decided[nd++] = a->uniqueID.count;
	if (hit && s_gunHitN < 512) s_gunHit[s_gunHitN++] = a->uniqueID.count;
	if (getenv("BOB_TRACE_GUNNERS"))
		fprintf(stderr, "[gunners] uid=0x%x shape %d engines %d/%d canopy %d: %s\n", (unsigned)a->uniqueID.count, (int)vis,
		        (int)ad->ENGINELEFT, (int)ad->ENGINERIGHT, (int)ad->CANOPY, hit ? "gunner hit -- turret silent" : "gunner unhurt");
}
#endif

#if BOB_LINUX
/* PILOT-MODEL-1 49a (PO 10-17): AI pilot fatigue. Shaw (preface; roll performance, appendix 1): 1940 fighters needed
   physical strength -- no hydraulic boost -- and the aileron stick force grows with dynamic pressure, so at high IAS a
   pilot can hold only part of the deflection (RAE 1940: Spitfire I fabric ailerons, ~1/5 aileron with 30-50 lb at
   400 mph); sustained lag pursuit at higher load factor is "very taxing physically". Model per AI pilot: fatigue F in
   0..1 grows with sustained G above 2 and with the share of his strength spent on the ailerons; it recovers slowly when
   he unloads. A tired pilot has less stick force (slower, lazier rolls at speed) and pulls less sustained G, which an
   opponent can see and use. Roll capacity 1 = Rowan's maxrollrate, which a fresh pilot keeps up to k * (300 mph IAS)^2.
   Per-pilot endurance 0.8..1.25 (hash of the uid). BOB_NO_FATIGUE=1 reverts; BOB_TRACE_FATIGUE=1 traces every 2 s. */
struct BdgPilot { unsigned id; float F, tiring, roll, tr; float vx, vy, vz, gs, gsus, bo, pull, sax, say, saz; SWord ohdg; bool seen; };
static BdgPilot s_bdg_pil[2048];
static BdgPilot* bdg_pil(AirStrucPtr a, bool create)
{
	const unsigned id = a->uniqueID.count;
	BdgPilot& p = s_bdg_pil[((id * 2654435761u) >> 9) & 2047];
	if (p.id != id) {
		if (!create) return 0;
		unsigned h = id * 2246822519u; h ^= h >> 13;
		p.id = id; p.F = 0; p.roll = 0; p.tr = 0; p.gs = 1.0f; p.gsus = 1.0f; p.bo = 0; p.pull = 1.0f; p.sax = 0; p.say = 9.80665f; p.saz = 0; p.seen = false; p.tiring = 0.8f + 0.45f * float(h & 1023) / 1023.0f;
	}
	return &p;
}
bool bdg_fatigue_on() { static int on = -1; if (on < 0) on = getenv("BOB_NO_FATIGUE") ? 0 : 1; return on != 0; }
static float bdg_roll_k(AirStrucPtr a)
{
	switch (a->classtype->visible) {
	case SPIT: return 1.15f;	// fabric ailerons, summer 1940
	case HURR: return 0.9f;
	case ME109: return 1.0f;
	case ME110: return 1.25f;
	default: return 1.4f;
	}
}
static float bdg_q300(AirStrucPtr a)	// (IAS / 300 mph)^2, standard atmosphere
{
	float s = 1.0f - 2.2558e-5f * float(a->World.Y) * 0.01f; if (s < 0.2f) s = 0.2f;
	const float v = float(a->vel_) / float(VEL_1MPH * 300);
	return v * v * powf(s, 4.2559f);
}
float bdg_pilot_fatigue(AirStrucPtr a) { if (!bdg_fatigue_on()) return 0; BdgPilot* p = bdg_pil(a, false); return p ? p->F : 0; }
/* Pulling out of a dive near the ground is an emergency, not sustained manoeuvring: the pilot pulls with everything he
   has. The roll/G limits and blackout onset stand aside in MANOEUVRE_LOWALT (a blackout already under way runs its
   course). Found in the MA port (MA-KILL-RATIO-1): tired pilots flew into the ground in the pull-out. */
static bool bdg_emergency(AirStrucPtr a, const BdgPilot* p) { return a->ai.manoeuvre == MANOEUVRE_LOWALT && !(p && p->bo > 0); }
float bdg_roll_capacity(AirStrucPtr a)	// share of Rowan's roll rate the pilot can deliver now
{
	if (!bdg_fatigue_on()) return 1.0f;
	BdgPilot* p = bdg_pil(a, false); const float F = p ? p->F : 0;
	const float need = bdg_roll_k(a) * bdg_q300(a), have = 1.0f - 0.45f * F;
	if (p && p->bo > 0) return 0.0f;	// blacked out: hands off
	if (bdg_emergency(a, p)) return 1.0f;
	return ((need > have) ? have / need : 1.0f) * (1.0f - 0.2f * F);
}
/* Sprint 2, G tolerance (Shaw p.377): "most fighter pilots, with training, can learn to sustain about 5 Gs for a
   considerable time"; tolerance falls with fatigue; past it the pilot greys out, then blacks out -- he "collapses and
   relaxes completely, releasing the aircraft", and recovery "can be a fairly slow process". 1940: no G suit (+1 G).
   Trained pilots hold the G just under their tolerance; novices (combatskill < REGULAR) snap it on past it. */
static float bdg_g_tol(const BdgPilot& p) { return 5.4f - 0.8f * (p.tiring - 1.0f) - p.F; }
static float bdg_g_lim(AirStrucPtr a, const BdgPilot& p) { return (p.bo > 0) ? 0.0f : bdg_g_tol(p) + ((a->ai.combatskill < SKILL_REGULAR) ? 1.5f : 0.0f); }
/* Ported from MA (ma ae99259, PO 10-18): the tolerance above is for SUSTAINED G (blackout follows G held for some seconds;
   the brain's oxygen reserve covers ~4-6 s). A pilot fresh in G may pull up to 1.5 G more for a short tracking pull; the
   allowance shrinks as the ~5 s mean G builds. In MA, capping every instant at the sustained figure (and easing off on
   measured-G spikes) halved the MiG's kills and took its slashing pass away. */
static float bdg_g_inst(AirStrucPtr a, const BdgPilot& p)
{
	const float lim = bdg_g_lim(a, p); if (lim <= 1.0f) return lim;
	float f = (lim - p.gsus) / (lim - 1.0f); if (f < 0) f = 0; if (f > 1) f = 1;
	return lim + 1.5f * f;
}
float bdg_g_limit(AirStrucPtr a)	// G this pilot will pull now; 0 = blacked out
{ if (!bdg_fatigue_on()) return 99.0f; BdgPilot* p = bdg_pil(a, false); return !p || bdg_emergency(a, p) ? 99.0f : bdg_g_inst(a, *p); }
bool bdg_blacked_out(AirStrucPtr a) { if (!bdg_fatigue_on()) return false; BdgPilot* p = bdg_pil(a, false); return p && p->bo > 0; }
float bdg_g_capacity(AirStrucPtr a)	// flight model: share of full pull, fed back from the felt G
{ if (!bdg_fatigue_on()) return 1.0f; BdgPilot* p = bdg_pil(a, false); return !p || bdg_emergency(a, p) ? 1.0f : (p->bo > 0) ? 0.0f : p->pull; }
void bdg_fatigue_roll(AirStrucPtr a, float rollfrac) { if (bdg_fatigue_on()) { BdgPilot* p = bdg_pil(a, true); if (rollfrac > p->roll) p->roll = rollfrac; } }
/* PILOT-MODEL-1 49b (PO 10-17): theory of mind -- each AI pilot builds a model of the bandit he is fighting from what
   he can SEE of it (its flight path; never its internal state), and the model sharpens the longer they fight. Shaw
   p.180: the energy fighter "can assess his opponent's energy management by observing the bogey's angular gains";
   p.131: assess the type and magnitude of a disadvantage; PO: rolls get less crisp as a pilot tires. Observed here:
   acquaintance (time in contact, faster for a better pilot), the bandit's sustained turn rate against his own when both
   pull hard (who out-turns whom), and the bandit's roll crispness (recent peak roll rate vs the best seen). */
struct BdgTom { unsigned bandit; float acq, turn, crisp, best, peak, win, tr; SWord broll, bhdg; float bvx, bvy, bvz, bg, pt; int nref, pman; unsigned char fail[128], ok[128]; float w3[3], rrs; bool seen; };
static BdgTom s_bdg_tom[2048];
static BdgTom* bdg_tom(AirStrucPtr me, bool create)
{
	BdgPilot* p = bdg_pil(me, create);
	return p ? &s_bdg_tom[p - s_bdg_pil] : 0;
}
static float bdg_tom_skill(AirStrucPtr me)	// how fast and how well he reads a bandit: novice 0.4 .. ace 1.0
{
	const int s = me->ai.combatskill;
	return (s >= SKILL_ACE) ? 1.0f : (s >= SKILL_VETERAN) ? 0.85f : (s >= SKILL_REGULAR) ? 0.7f : 0.4f;
}
float bdg_tom_knowledge(AirStrucPtr me)	// 0 = knows nothing of this bandit, 1 = has his measure
{
	if (!bdg_fatigue_on()) return 0; BdgTom* t = bdg_tom(me, false);
	return (t && me->ai.unfriendly && t->bandit == me->ai.unfriendly->uniqueID.count) ? t->acq * bdg_tom_skill(me) : 0;
}
float bdg_tom_turn(AirStrucPtr me) { BdgTom* t = bdg_tom(me, false); const float k = bdg_tom_knowledge(me); return t ? 1.0f + (t->turn - 1.0f) * k : 1.0f; }
float bdg_tom_crisp(AirStrucPtr me) { BdgTom* t = bdg_tom(me, false); const float k = bdg_tom_knowledge(me); return t ? 1.0f + (t->crisp - 1.0f) * k : 1.0f; }
/* Sprint 3, learning from the bandit's answers (Shaw, preface: "almost any tactic can be defeated if it is totally
   predictable"): when he is threatened (bandit inside 800 m, behind him, nose on him) the defence he picks is
   remembered; 8 s later it worked (bandit off his tail) or failed. A defence that keeps failing against THIS bandit is
   dropped and Rowan's selector rolls again -- after one net failure for a veteran/ace, two for a regular, three below. */
bool bdg_tom_threatened(AirStrucPtr me)
{
	if (!me->ai.unfriendly || me->ai.unfriendly->Status.size != AIRSTRUCSIZE) return false;
	AirStrucPtr b = AirStrucPtr(me->ai.unfriendly);
	const float dx = float(me->World.X - b->World.X), dy = float(me->World.Y - b->World.Y), dz = float(me->World.Z - b->World.Z);
	const float r = sqrtf(dx * dx + dy * dy + dz * dz); if (r > 100000.0f || r < 1.0f) return false;
	const float bv = sqrtf(float(b->vel_x) * b->vel_x + float(b->vel_y) * b->vel_y + float(b->vel_z) * b->vel_z) + 1.0f;
	const float mv = sqrtf(float(me->vel_x) * me->vel_x + float(me->vel_y) * me->vel_y + float(me->vel_z) * me->vel_z) + 1.0f;
	const float nose = (dx * b->vel_x + dy * b->vel_y + dz * b->vel_z) / (r * bv);	// bandit pointing at me: 1
	const float tail = (dx * me->vel_x + dy * me->vel_y + dz * me->vel_z) / (r * mv);	// bandit behind me: -1 (dx is me - bandit)
	return nose > 0.766f && tail > 0.342f;	// inside 1 km, his nose within 40 deg, he is within 70 deg of my tail
}
static int bdg_tom_need(AirStrucPtr me) { const int s = me->ai.combatskill; return (s >= SKILL_VETERAN) ? 1 : (s >= SKILL_REGULAR) ? 2 : 3; }
bool bdg_tom_failed(AirStrucPtr me, int man)
{
	if (!bdg_fatigue_on() || getenv("BOB_NO_TOM") || man < 0 || man >= 128) return false;
	BdgTom* t = bdg_tom(me, false);
	return t && me->ai.unfriendly && t->bandit == me->ai.unfriendly->uniqueID.count && t->fail[man] >= t->ok[man] + bdg_tom_need(me);
}
void bdg_tom_tried(AirStrucPtr me, int man) { BdgTom* t = bdg_tom(me, false); if (t && man >= 0 && man < 128) { t->pman = man; t->pt = 8.0f; } }
static void bdg_tom_tick(AirStrucPtr me, BdgPilot* p, float dt, float myg)
{
	BdgTom& t = s_bdg_tom[p - s_bdg_pil];
	AirStrucPtr b = (me->ai.unfriendly && me->ai.unfriendly->Status.size == AIRSTRUCSIZE) ? AirStrucPtr(me->ai.unfriendly) : 0;
	if (!b) { t.acq -= dt / 300.0f; if (t.acq < 0) t.acq = 0; return; }
	if (t.bandit != b->uniqueID.count) { t.bandit = b->uniqueID.count; t.acq = 0; t.turn = 1; t.crisp = 1; t.best = 0; t.peak = 0; t.win = 0; t.tr = 0; t.bg = 1; t.rrs = 0; t.nref = 0; t.pt = 0; t.pman = -1; memset(t.fail, 0, sizeof t.fail); memset(t.ok, 0, sizeof t.ok); t.seen = false; }
	if (t.pt > 0 && (t.pt -= dt) <= 0 && t.pman >= 0) {
		const bool still = bdg_tom_threatened(me);
		unsigned char& c = still ? t.fail[t.pman] : t.ok[t.pman]; if (c < 255) c++;
		if (getenv("BOB_TRACE_TOM")) fprintf(stderr, "[tom] id=%u learn man=%d %s (failed %d, worked %d)\n", p->id, t.pman, still ? "FAILED" : "worked", t.fail[t.pman], t.ok[t.pman]);
		t.pman = -1;
	}
	const float dx = float(b->World.X - me->World.X), dy = float(b->World.Y - me->World.Y), dz = float(b->World.Z - me->World.Z);
	const bool sees = (dx * dx + dy * dy + dz * dz < 2.5e5f * 2.5e5f) && p->bo <= 0;	// within 2.5 km, not blacked out
	const float bvx = float(b->vel_x) * 1e-4f, bvy = float(b->vel_y) * 1e-4f, bvz = float(b->vel_z) * 1e-4f;
	if (sees && t.seen) {
		t.acq += dt * bdg_tom_skill(me) / 90.0f; if (t.acq > 1) t.acq = 1;
		const float rr = fabsf(float(SWord(SWord(b->roll) - t.broll))) * 5.4932e-3f / dt;	// deg/s
		if (rr < 720.0f) t.rrs += (rr - t.rrs) * (dt < 0.3f ? dt / 0.3f : 1.0f);	// 0.3 s smoothing: a sustained roll, not a one-frame flick
		if (rr < 720.0f && t.rrs > t.peak) t.peak = t.rrs;
		const float ax = (bvx - t.bvx) / dt, ay = (bvy - t.bvy) / dt + 9.80665f, az = (bvz - t.bvz) / dt;
		float bg = sqrtf(ax * ax + ay * ay + az * az) / 9.80665f; if (bg > 9.0f) bg = 9.0f;
		t.bg += (bg - t.bg) * (dt < 1.0f ? dt : 1.0f); bg = t.bg;
		const float bw = fabsf(float(SWord(SWord(b->hdg) - t.bhdg))) * 5.4932e-3f / dt;
		const float mw = fabsf(float(SWord(SWord(me->hdg) - p->ohdg))) * 5.4932e-3f / dt;
		if (bg > 2.5f && myg > 2.5f && bw < 60.0f && mw > 5.0f && mw < 60.0f && bw > 5.0f)	// both pulling hard: who gains?
			t.turn += (bw / mw - t.turn) * dt / 20.0f;
		if ((t.win += dt) >= 8.0f) {	// 8 s windows: his peak roll rate now vs his fresh one (mean of the first 6 hard-rolling windows)
			if (t.peak > 30.0f) {
				if (t.nref < 6) { t.best = (t.best * t.nref + t.peak) / (t.nref + 1); t.w3[t.nref % 3] = t.peak; t.nref++; }
				else {	// his best roll in the last 3 rolling windows: one window without a hard roll is not fatigue
					t.w3[t.nref % 3] = t.peak; t.nref++;
					const float m = (t.w3[0] > t.w3[1]) ? ((t.w3[0] > t.w3[2]) ? t.w3[0] : t.w3[2]) : ((t.w3[1] > t.w3[2]) ? t.w3[1] : t.w3[2]);
					if (m > t.best) t.best = m;	// his best roll so far: the read only falls when he can no longer match it
					t.crisp += (m / t.best - t.crisp) * 0.25f;
				}
			}
			t.peak = 0; t.win = 0;
		}
	}
	t.broll = SWord(b->roll); t.bhdg = SWord(b->hdg); t.bvx = bvx; t.bvy = bvy; t.bvz = bvz; t.seen = true;
	static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_TOM") ? 1 : 0;
	if (tr && (t.tr += dt) >= 5.0f) {
		t.tr = 0;
		fprintf(stderr, "[tom] id=%u skill=%d bandit=%u shape=%d sees=%d acq=%.2f know=%.2f turn=%.2f crisp=%.2f best=%.0f\n", p->id,
		        int(me->ai.combatskill), t.bandit, int(b->classtype->visible), int(sees), t.acq, bdg_tom_knowledge(me), t.turn, t.crisp, t.best);
	}
}
/* SHAW-TACTIC-1 (57, PO 10-19; port of ma sprints 1-5): every hero-level AI fighter unit carries the Shaw picture it is fighting in, shared by
   the unit and re-assessed every 2 s. PO 10-19: the state is a RECORD of the whole picture as Shaw sees it -- several
   tactics and considerations apply at once -- not one label. Layers (Shaw chapter): fight size, mine v theirs (ch.3-8);
   similar or dissimilar aircraft (ch.3 v ch.4); section mode (ch.5-6: welded wing, loose deuce = engaged + free,
   sandwich, bracket, split = two separate 1v1s); per member a 1v1 posture (angles / energy / defensive / extend) and
   role (engaged / free); energy state and position of the leader against his bandit. The enemy's tactic AS PERCEIVED
   (theory of mind, item 49) is a later layer. The unit is the element (leader + follower); the record lives with the
   element leader. Hero = SKILL_HERO (the top AI skill); units led by the player are left alone (under BOB_AI_PILOT the
   player's jet is AI-flown, so its unit counts as AI-led). Sprint 1 is READ-ONLY: assessed and logged, nothing in the
   flying reads it yet. Counts use the true positions within 5 km for now (SENSE limits come with the perceived layer).
   BOB_NO_SHAW=1 turns it off; BOB_TRACE_SHAW=1 logs the whole record whenever any layer changes. */
extern "C" void bob_acmi_event(unsigned long id, const char* type, const char* text);
static AirStrucPtr bdg_shaw_bandit(AirStrucPtr me) { return (me->ai.unfriendly && me->ai.unfriendly->Status.size == AIRSTRUCSIZE) ? AirStrucPtr(me->ai.unfriendly) : AirStrucPtr(0); }
enum { SZ_NONE, SZ_1V1, SZ_2V1, SZ_1V2, SZ_2V2, SZ_FEWVMANY, SZ_MANYVFEW, SZ_DIVISION };
enum { SEC_ALONE, SEC_WELDED, SEC_LOOSE, SEC_SANDWICH, SEC_BRACKET, SEC_SPLIT };
enum { PO_NONE, PO_ANGLES, PO_ENERGY, PO_DEFENSIVE, PO_EXTEND };
enum { RO_NONE, RO_ENGAGED, RO_FREE };
enum { EN_NONE, EN_ADV, EN_PAR, EN_DIS };
enum { PS_NONE, PS_OFF, PS_NEU, PS_DEF };
static const char* const s_shaw_size[] = { "none", "1v1", "2v1", "1v2", "2v2", "few-v-many", "many-v-few", "division" };
static const char* const s_shaw_sec[] = { "alone", "welded-wing", "loose-deuce", "sandwich", "bracket", "split" };
static const char* const s_shaw_post[] = { "-", "angles", "energy", "defensive", "extend" };
static const char* const s_shaw_role[] = { "-", "engaged", "free" };
static const char* const s_shaw_en[] = { "-", "advantage", "parity", "disadvantage" };
static const char* const s_shaw_pos[] = { "-", "offensive", "neutral", "defensive" };
/* The PLAN: the section tactic Shaw prescribes for this picture (ch.5-6). Read-only like the rest of the record.
   press      -- engaged fighter presses the attack, free fighter covers (double attack / loose deuce, pp.203-204, 212)
   switch     -- engaged fighter losing the offensive (energy depleted): call the free fighter in, roles reverse (p.204, 213)
   free-attack-- engaged fighter defensive, partner near: "the free fighter is obligated to attack the bogey
                 immediately, while the engaged fighter pilot does his best not to get shot" (p.213)
   bracket    -- 2v2 with mutual support: brackets and drags to eliminate one bogey (p.264)
   separate   -- two separate 1v1s, sound only with a performance or position superiority (p.264)
   regain     -- separated without that superiority: regain mutual support (p.264)
   singleton  -- alone against two or more: hit and run, both bogeys on one side, high energy, watch for escape (p.235)
   two-v-one  -- our section in mutual support (not welded) against a bandit SEEN without it -- his pair welded, or
                 him alone or separated: "against welded wing bogeys, two-versus-one tactics can be employed with high
                 probability of success" (p.241); a welded pair "can be considered the singleton" (p.240-241). In 1940
                 terms: the Rotte against the RAF's tight vic.
   Welded against welded: "the two-versus-two scenario essentially becomes one-versus-one" (p.240) -> 1v1. */
enum { PL_NONE, PL_1V1, PL_PRESS, PL_SWITCH, PL_FREEATTACK, PL_BRACKET, PL_SEPARATE, PL_REGAIN, PL_SINGLETON, PL_TWOVONE };
static const char* const s_shaw_plan[] = { "-", "1v1", "press", "switch", "free-attack", "bracket", "separate", "regain", "singleton", "two-v-one" };
/* The ENEMY AS PERCEIVED (item 49's theory of mind; PO 10-19: inferred from what is seen, never read from his state,
   so a hero cannot cheat). From positions and velocities only: his section -- the bandit's nearest other aircraft
   of his side: welded (< 1 km), mutual support (< 3 km), separated, alone (none within 5 km); his posture --
   offensive (on my tail), energy (flight path 20 deg or more off the horizon, up or down, range not opening: zoom,
   climb, slashing dive), angles (turning 6 deg/s or harder over the last 2 s, about the jets' sustained rate at
   height), extending (range opening and his velocity pointed away from me), else neutral.
   Calibrated on QM 4 (BOB_TRACE_SHAW=2 prints [shawp] every assessment): steep flight separates Rowan's energy
   manoeuvres from TURNINGFIGHT (climb q10/q90 -0.69/+0.61 v -0.21/+0.32); turn rate does not -- TURNINGFIGHT's
   median is 1.6 deg/s, the label covering fast slashing attacks too -- so the turn limit is physical, not fitted. */
enum { ES_NONE, ES_ALONE, ES_WELDED, ES_MUTUAL, ES_SEPARATED };
enum { EP_NONE, EP_OFF, EP_ENERGY, EP_ANGLES, EP_EXTEND, EP_NEUTRAL };
static const char* const s_shaw_esec[] = { "-", "alone", "welded", "mutual-support", "separated" };
static const char* const s_shaw_epost[] = { "-", "offensive", "energy", "angles", "extending", "neutral" };
struct BdgShawRec { unsigned char mine, theirs, size, dissimilar, section, post[2], role[2], energy, pos, esec, epost, plan; };
static bool operator==(const BdgShawRec& a, const BdgShawRec& b) { return !memcmp(&a, &b, sizeof a); }
struct BdgShaw { unsigned id; float next; BdgShawRec rec, cand; unsigned bid; float bt, brange, bv[3];  float steer_at;};	// b*: the bandit as last seen
static BdgShaw s_bdg_shaw[1024];
static float s_shaw_t;				// sim seconds, advanced by one owning aircraft (re-elected when it stops ticking)
static unsigned s_shaw_owner, s_shaw_orphan;
static bool bdg_shaw_on() { static int on = -1; if (on < 0) on = getenv("BOB_NO_SHAW") ? 0 : 1; return on != 0; }
static bool bdg_shaw_live(AirStrucPtr a)
{
	return a && a->Status.size == AIRSTRUCSIZE && !a->Status.deaded && a->movecode != AUTO_SPIRAL2GROUND
	    && a->movecode != AUTO_DEATHSEQUENCE && a->movecode != AUTO_CRASHSKID && a->movecode != AUTO_CRASHROLL;
}
static float bdg_shaw_rng(AirStrucPtr a, AirStrucPtr b)	// m
{
	const float dx = float(a->World.X - b->World.X), dy = float(a->World.Y - b->World.Y), dz = float(a->World.Z - b->World.Z);
	return sqrtf(dx * dx + dy * dy + dz * dz) * 0.01f;
}
static float bdg_shaw_es(AirStrucPtr a)	// energy height, m
{
	const float vx = float(a->vel_x) * 0.0001f, vy = float(a->vel_y) * 0.0001f, vz = float(a->vel_z) * 0.0001f;	// BoB: 0.0001 m/s
	return float(a->World.Y) * 0.01f + (vx * vx + vy * vy + vz * vz) / (2.0f * 9.80665f);
}
static bool bdg_shaw_on_tail(AirStrucPtr att, AirStrucPtr vic)	// att inside 1.5 km, nose within 40 deg of vic, vic within 70 deg of his tail
{
	const float dx = float(vic->World.X - att->World.X), dy = float(vic->World.Y - att->World.Y), dz = float(vic->World.Z - att->World.Z);
	const float r = sqrtf(dx * dx + dy * dy + dz * dz); if (r > 150000.0f || r < 1.0f) return false;
	const float av = sqrtf(float(att->vel_x) * att->vel_x + float(att->vel_y) * att->vel_y + float(att->vel_z) * att->vel_z) + 1.0f;
	const float vv = sqrtf(float(vic->vel_x) * vic->vel_x + float(vic->vel_y) * vic->vel_y + float(vic->vel_z) * vic->vel_z) + 1.0f;
	return (dx * att->vel_x + dy * att->vel_y + dz * att->vel_z) / (r * av) > 0.766f && (dx * vic->vel_x + dy * vic->vel_y + dz * vic->vel_z) / (r * vv) > 0.342f;
}
static AirStrucPtr bdg_shaw_engaged(AirStrucPtr m) { return (bdg_shaw_live(m) && m->movecode == AUTO_COMBAT) ? bdg_shaw_bandit(m) : AirStrucPtr(0); }
static int bdg_shaw_post(AirStrucPtr m)	// one fighter's 1v1 posture against his bandit
{
	AirStrucPtr b = bdg_shaw_engaged(m); if (!b) return PO_NONE;
	if (bdg_shaw_on_tail(b, m)) return PO_DEFENSIVE;
	switch (int(m->ai.manoeuvre)) {
	case MANOEUVRE_EXTENSION: case MANOEUVRE_DIVEFORHOME: case MANOEUVRE_GOHOME: case MANOEUVRE_MOVEAWAY:
	case MANOEUVRE_CLIMBFORHOME: case MANOEUVRE_MAKEFORFRIENDLY: case MANOEUVRE_STRAIGHTDIVE: return PO_EXTEND;
	case MANOEUVRE_DIVEANDZOOM: case MANOEUVRE_ZOOM: case MANOEUVRE_ZOOMANDDROP:	/* BoB dead-coded HIGHYOYO (CSB 7Sep00) */
	case MANOEUVRE_GAINHEIGHT: case MANOEUVRE_INTERCEPTHIGH: return PO_ENERGY;
	}
	// Shaw ch.3: the energy fighter keeps an energy advantage and stays out of the opponent's turn
	return (bdg_shaw_es(m) > bdg_shaw_es(b) + 500.0f && m->World.Y > b->World.Y) ? PO_ENERGY : PO_ANGLES;
}
static int bdg_shaw_section(AirStrucPtr L, AirStrucPtr W, AirStrucPtr bL, AirStrucPtr bW)
{
	const bool wl = W && bdg_shaw_live(W);
	if (!wl) return SEC_ALONE;
	const bool welded = W->ai.manoeuvre == MANOEUVRE_WELDEDWINGMAN;
	const float d = bdg_shaw_rng(L, W);
	if (bL && bW) {
		if (welded || (bL == bW && d < 600.0f)) return SEC_WELDED;
		if ((bdg_shaw_on_tail(bW, L) && !bdg_shaw_on_tail(bW, W)) || (bdg_shaw_on_tail(bL, W) && !bdg_shaw_on_tail(bL, L))) return SEC_SANDWICH;
		if (bL == bW) return SEC_BRACKET;
		return (d > 3000.0f) ? SEC_SPLIT : SEC_LOOSE;
	}
	if (welded) return SEC_WELDED;
	return (d < 3000.0f) ? SEC_LOOSE : SEC_SPLIT;		// one engaged, the partner free (Shaw ch.6) -- or gone off alone
}
static BdgShawRec bdg_shaw_assess(AirStrucPtr L, AirStrucPtr W, BdgShaw& s)
{
	BdgShawRec r; memset(&r, 0, sizeof r);
	AirStrucPtr bL = bdg_shaw_engaged(L), bW = bdg_shaw_engaged(W);
	if (!bL && !bW) return r;
	AirStrucPtr me = bL ? L : W, b = bL ? bL : bW;		// the member whose fight describes the unit's energy/position
	int mine = 0, theirs = 0;
	for (MobileItemPtr e = mobileitem::GetACList(); e; e = e->nextmobile) {
		AirStrucPtr ac = (AirStrucPtr)e;
		if (!bdg_shaw_live(ac) || bdg_shaw_rng(L, ac) > 5000.0f) continue;
		if (ac->nationality == L->nationality) { if (ac->movecode == AUTO_COMBAT) mine++; }
		else theirs++;
	}
	if (mine < 1) mine = 1; if (theirs < 1) theirs = 1;
	r.mine = (unsigned char)(mine > 255 ? 255 : mine); r.theirs = (unsigned char)(theirs > 255 ? 255 : theirs);
	r.size = (mine == 1 && theirs == 1) ? SZ_1V1 : (mine == 2 && theirs == 1) ? SZ_2V1 : (mine == 1 && theirs == 2) ? SZ_1V2
	       : (mine == 2 && theirs == 2) ? SZ_2V2 : (mine <= 2 && theirs >= 3) ? SZ_FEWVMANY : (mine >= 3 && theirs <= 2) ? SZ_MANYVFEW
	       : SZ_DIVISION;
	r.dissimilar = (me->classtype != b->classtype) ? 1 : 0;
	{	// the enemy as perceived
		float dmin = 5000.0f;
		for (MobileItemPtr e = mobileitem::GetACList(); e; e = e->nextmobile) {
			AirStrucPtr ac = (AirStrucPtr)e;
			if (ac != b && bdg_shaw_live(ac) && ac->nationality == b->nationality) { const float d = bdg_shaw_rng(b, ac); if (d < dmin) dmin = d; }
		}
		r.esec = (dmin < 1000.0f) ? ES_WELDED : (dmin < 3000.0f) ? ES_MUTUAL : (dmin < 5000.0f) ? ES_SEPARATED : ES_ALONE;
		const float vx = float(b->vel_x), vy = float(b->vel_y), vz = float(b->vel_z), vm = sqrtf(vx * vx + vy * vy + vz * vz) + 1.0f;
		const float bv[3] = { vx / vm, vy / vm, vz / vm }, rng = bdg_shaw_rng(me, b);
		float turn = 0.0f;			// deg/s since the last look, if it was this bandit 0.5-5 s ago
		if (s.bid == b->uniqueID.count && s_shaw_t - s.bt > 0.5f && s_shaw_t - s.bt < 5.0f) {
			float c = bv[0] * s.bv[0] + bv[1] * s.bv[1] + bv[2] * s.bv[2]; c = c > 1.0f ? 1.0f : c < -1.0f ? -1.0f : c;
			turn = acosf(c) * 57.29578f / (s_shaw_t - s.bt);
		}
		const float ax = float(b->World.X - me->World.X), ay = float(b->World.Y - me->World.Y), az = float(b->World.Z - me->World.Z);
		const float am = sqrtf(ax * ax + ay * ay + az * az) + 1.0f;
		const bool opening = s.bid == b->uniqueID.count && rng > s.brange + 100.0f && (bv[0] * ax + bv[1] * ay + bv[2] * az) / am > 0.5f;
		r.epost = bdg_shaw_on_tail(b, me) ? EP_OFF : (fabsf(bv[1]) > 0.342f) ? (opening ? EP_EXTEND : EP_ENERGY) : (turn > 6.0f) ? EP_ANGLES
		        : opening ? EP_EXTEND : EP_NEUTRAL;
		static int pr = -1; if (pr < 0) { const char* t = getenv("BOB_TRACE_SHAW"); pr = (t && atoi(t) >= 2) ? 1 : 0; }
		if (pr && turn > 0.0f)		// every assessment, for calibrating the perception against the bandit's true manoeuvre
			fprintf(stderr, "[shawp] t=%.0f bandit=%u man%d turn=%.1f climb=%.2f open=%d tail=%d\n", double(s_shaw_t),
			        unsigned(b->uniqueID.count), int(b->ai.manoeuvre), double(turn), double(bv[1]), opening ? 1 : 0, bdg_shaw_on_tail(b, me) ? 1 : 0);
		s.bid = b->uniqueID.count; s.bt = s_shaw_t; s.brange = rng; s.bv[0] = bv[0]; s.bv[1] = bv[1]; s.bv[2] = bv[2];
	}
	r.section = (unsigned char)bdg_shaw_section(L, W, bL, bW);
	r.post[0] = (unsigned char)bdg_shaw_post(L); r.role[0] = bL ? RO_ENGAGED : (L && bdg_shaw_live(L)) ? RO_FREE : RO_NONE;
	r.post[1] = W ? (unsigned char)bdg_shaw_post(W) : PO_NONE; r.role[1] = bW ? RO_ENGAGED : (W && bdg_shaw_live(W)) ? RO_FREE : RO_NONE;
	const float de = bdg_shaw_es(me) - bdg_shaw_es(b);
	r.energy = (de > 500.0f) ? EN_ADV : (de < -500.0f) ? EN_DIS : EN_PAR;
	r.pos = bdg_shaw_on_tail(me, b) ? PS_OFF : bdg_shaw_on_tail(b, me) ? PS_DEF : PS_NEU;
	const bool pair = r.section != SEC_ALONE, near = pair && r.section != SEC_SPLIT;
	const bool def = r.post[0] == PO_DEFENSIVE || r.post[1] == PO_DEFENSIVE;
	if (r.mine < 2 && r.theirs >= 2) r.plan = PL_SINGLETON;		// alone here against two or more, whatever the partner does
	else if (pair && !near) r.plan = (r.pos == PS_OFF || r.energy == EN_ADV) ? PL_SEPARATE : PL_REGAIN;
	else if (!pair || r.mine < 2) r.plan = (r.pos == PS_OFF) ? PL_PRESS : PL_1V1;
	else if (def) r.plan = PL_FREEATTACK;
	else if (r.section == SEC_WELDED && r.esec == ES_WELDED) r.plan = PL_1V1;		// p.240
	else if (r.section != SEC_WELDED && (r.esec == ES_WELDED || r.esec == ES_ALONE || r.esec == ES_SEPARATED)) r.plan = PL_TWOVONE;	// p.241
	else if (r.pos == PS_OFF && r.energy == EN_DIS) r.plan = PL_SWITCH;
	else if (r.theirs >= 2) r.plan = PL_BRACKET;
	else r.plan = PL_PRESS;
	return r;
}
/* SHAW-TACTIC-1 (57) steering (PO 10-20: the plan may steer hero pilots' flying; port of ma). Only the FREE member is
   moved, and only onto a bandit: an engaged fighter is never pulled off his fight, so Rowan's manoeuvres are
   untouched. free-attack: the free fighter attacks the bandit seen on the defensive member's tail (p.213); two-v-one:
   the free fighter takes the engaged member's bandit (p.241). Through Rowan's SetEngage, which also hands the decision
   to the multiplayer owner. OPT-IN: BOB_SHAW_STEER=1 (census 10-20, BoB
   hero QMs: QM 20 losses unchanged, QM 13 RAF losses 0 -> 3; 10-21 on top of SHAW-BFM-1, QM 13 x 8 starts: Shaw RAF
   13 lost / 35 killed without steering, 13 / 30 with; Shaw Luftwaffe 30 lost without, 42 with -- the free 109 is
   sent into a Spitfire's turning fight). OPT-IN; without it the record steers no one.
   BOB_TRACE_SHAW=1 logs each steer. */
static bool bdg_shaw_steer_on() { static int on = -1; if (on < 0) { const char* e = getenv("BOB_SHAW_STEER"); on = e ? (atoi(e) != 0) : 0; } return on != 0; }
static bool bdg_shaw_side(AirStrucPtr a);
static void bdg_shaw_steer(AirStrucPtr L, AirStrucPtr W, const BdgShawRec& r, float& steer_at)
{
	if (s_shaw_t - steer_at < 30.0f) return;	// once per 30 s: Rowan takes the target back, and each SetEngage restarts his manoeuvre
	if (!bdg_shaw_steer_on() || !W || W->Status.size != AIRSTRUCSIZE || (r.plan != PL_FREEATTACK && r.plan != PL_TWOVONE)) return;
	AirStrucPtr F, E;
	if (r.role[0] == RO_FREE && r.role[1] == RO_ENGAGED) { F = L; E = W; }
	else if (r.role[1] == RO_FREE && r.role[0] == RO_ENGAGED) { F = W; E = L; }
	else return;
	AirStrucPtr t = 0;
	if (r.plan == PL_TWOVONE) t = bdg_shaw_bandit(E);
	else {
		float dmin = 1e9f;
		for (AirStrucPtr ac = AirStrucPtr(mobileitem::GetACList()); ac; ac = AirStrucPtr(ac->nextmobile)) {
			if (ac->nationality != E->nationality && bdg_shaw_live(ac) && bdg_shaw_on_tail(ac, E)) { const float d = bdg_shaw_rng(ac, E); if (d < dmin) { dmin = d; t = ac; } }
		}
	}
	if (!t || !bdg_shaw_live(t) || bdg_shaw_bandit(F) == t || !bdg_shaw_side(F)) return;
	Art_Int.SetEngage(F, t, MANOEUVRE_SELECT, ANGLES_0Deg, ANGLES_0Deg, false);
	steer_at = s_shaw_t;
	static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_SHAW") ? 1 : 0;
	if (tr) fprintf(stderr, "[shaws] t=%.0f %s: free %u -> bandit %u (engaged %u)\n", double(s_shaw_t), s_shaw_plan[r.plan],
	                unsigned(F->uniqueID.count), unsigned(t->uniqueID.count), unsigned(E->uniqueID.count));
}
/* SHAW-BFM-1 (PO 10-21: "use shaw, not rowan, for hero pilots"): a hero pilot's next manoeuvre against a fighter is
   chosen here from Shaw's 1v1 geometry, and Rowan's code only flies it (ACMMAN SelectNextEngageManoeuvre, and MSGAI
   ChooseEvasiveManoeuvre when he is shot at). Rowan keeps the decisions that are not BFM: going home, disengaging on
   fuel/ammo/morale, bomber attacks. Posture (Shaw ch.2): DEFENSIVE = the bandit's nose within 30 deg of me and he is
   behind my 3/9 line, inside 1.5 km; OFFENSIVE = my nose within 60 deg of him and I am within 60 deg of his tail,
   inside 3 km; HEAD-ON = both noses within 30 deg, closing, inside 2 km; else NEUTRAL. Head-on: the angles fighter
   takes the gun pass and the energy fighter zooms (census 10-21, QM 13 x 8 starts: a lead turn or a jink at the
   merge gave up the Spitfire's shot -- 109 kills per run 2.9 -> 1.25). Defensive: guns defence (out-of-plane jink)
   when he is inside 800 m with his nose on; an extension (half-roll and dive) only for the energy fighter out of guns range and not slower (p.97); otherwise break
   into him, which also forces his overshoot (p.86). Offensive: in range and nose on, track (lead pursuit); closing fast
   with angle-off, lag pursuit so as not to overshoot (p.27, the yo-yo's purpose); the energy fighter with height
   makes a slashing pass and zooms (ch.4); else lead pursuit to close. Neutral: close after an overshoot, reverse into
   him (p.92); the energy fighter goes vertical or rebuilds energy and never turns with the angles fighter (ch.4);
   the angles fighter turns. Outnumbered (plan singleton, ch.7): no sustained turning -- a slashing attack, else extend.
   Angles/energy fighter (ch.4): Spitfire/Hurricane are the angles fighters against Bf 109/110; similar types fight
   angles unless one has a 300 m energy-height margin. BOB_NO_SHAW_BFM=1 hands the choice back to Rowan;
   BOB_TRACE_SHAW=1 logs each choice ([shawm]). Returns -1 for "Rowan decides". */
static bool bdg_shaw_bfm_on() { static int on = -1; if (on < 0) on = getenv("BOB_NO_SHAW_BFM") ? 0 : 1; return on != 0; }
static bool bdg_shaw_side(AirStrucPtr a)	// test hook BOB_SHAW_NAT=1|2 (RAF|Luftwaffe): only that side flies Shaw, the other Rowan
{
	static int nat = -2; if (nat == -2) nat = getenv("BOB_SHAW_NAT") ? atoi(getenv("BOB_SHAW_NAT")) : -1;
	return nat < 0 || int(a->nationality) == nat;
}
static int bdg_shaw_style(AirStrucPtr m, AirStrucPtr b)	// +1 energy fighter, -1 angles fighter, 0 similar
{
	const int mt = m->fly.pModel->Type, bt = b->fly.pModel->Type;
	const bool mE = mt == AT_BF109 || mt == AT_BF110, bE = bt == AT_BF109 || bt == AT_BF110;
	const bool mA = mt == AT_SPITFIRE || mt == AT_HURRICANE, bA = bt == AT_SPITFIRE || bt == AT_HURRICANE;
	return (mE && bA) ? 1 : (mA && bE) ? -1 : 0;
}
int bdg_shaw_bfm(AirStrucPtr m, AirStrucPtr b, bool shot)
{
	if (!bdg_shaw_on() || !bdg_shaw_bfm_on() || !m || !b || m->ai.combatskill < SKILL_HERO || !bdg_shaw_live(m) || !bdg_shaw_live(b)
	    || !m->fly.pModel || !b->fly.pModel || b->classtype->aerobaticfactor == AEROBATIC_LOW || !bdg_shaw_side(m)) return -1;
	const float dx = float(b->World.X - m->World.X) * 0.01f, dy = float(b->World.Y - m->World.Y) * 0.01f, dz = float(b->World.Z - m->World.Z) * 0.01f;
	const float R = sqrtf(dx * dx + dy * dy + dz * dz) + 0.1f;
	const float mx = float(m->vel_x) * 1e-4f, my = float(m->vel_y) * 1e-4f, mz = float(m->vel_z) * 1e-4f;
	const float bx = float(b->vel_x) * 1e-4f, by = float(b->vel_y) * 1e-4f, bz = float(b->vel_z) * 1e-4f;
	const float vm = sqrtf(mx * mx + my * my + mz * mz) + 0.1f, vb = sqrtf(bx * bx + by * by + bz * bz) + 0.1f;
	const float ata = (dx * mx + dy * my + dz * mz) / (R * vm);		// cos of my nose to him
	const float bata = -(dx * bx + dy * by + dz * bz) / (R * vb);		// cos of his nose to me
	const float tail = (dx * bx + dy * by + dz * bz) / (R * vb);		// cos of my angle off his tail (1 = dead astern)
	const float vc = ((mx - bx) * dx + (my - by) * dy + (mz - bz) * dz) / R;	// closure, m/s
	const float dE = bdg_shaw_es(m) - bdg_shaw_es(b);
	const int style0 = bdg_shaw_style(m, b), style = style0 ? style0 : (dE > 300.0f ? 1 : -1);
	const AirStrucPtr L = m->leader ? AirStrucPtr(m->leader) : m;
	const BdgShaw& s = s_bdg_shaw[((L->uniqueID.count * 2654435761u) >> 9) & 1023];
	const bool alone = s.id == L->uniqueID.count && s.rec.plan == PL_SINGLETON;
	const char* post; int man;
	if (bata > 0.866f && ata > 0.866f && R < 2000.0f && vc > 0.0f) {
		post = "head-on";			// guns head-on pass: the angles fighter takes the shot (Spitfire/Hurricane batteries win it here,
		man = (style > 0) ? MANOEUVRE_ZOOM : MANOEUVRE_TURNINGFIGHT;	// census 10-21); the energy fighter zooms through it
	} else if (bata > 0.866f && ata < 0.0f && R < 1500.0f) {
		post = "defensive";
		if (R < 800.0f && bata > 0.94f)										man = (vm > 100.0f) ? MANOEUVRE_HIGBARRELROLL : MANOEUVRE_JINK;
		else if (style > 0 && R > 900.0f && vm >= vb && !shot)				man = MANOEUVRE_SPLITS;
		else																man = MANOEUVRE_BREAKTURN;
	} else if (ata > 0.5f && tail > 0.5f && R < 3000.0f) {
		post = "offensive";
		if (R < 400.0f && ata > 0.94f)										man = MANOEUVRE_TURNINGFIGHT;
		else if (alone)														man = MANOEUVRE_SLASHINGATTACK;
		else if (R < 800.0f && vc > 40.0f && tail < 0.87f)					man = MANOEUVRE_LAGPURSUIT;
		else if (style > 0 && dE >= 0.0f)									man = MANOEUVRE_DIVEANDZOOM;
		else																man = MANOEUVRE_TURNINGFIGHT;
	} else {
		post = "neutral";
		if (R < 400.0f && vc < 0.0f && ata > 0.0f)							man = (style > 0) ? MANOEUVRE_ZOOM : MANOEUVRE_REVERSETURN;
		else if (alone)														man = (dE > 300.0f) ? MANOEUVRE_DIVEANDZOOM : MANOEUVRE_EXTENSION;
		else if (style > 0)
		{
			if (dE >= 300.0f)												man = MANOEUVRE_DIVEANDZOOM;
			else if (vm > 1.5f * vb || (vm > 120.0f && R < 1500.0f))		man = MANOEUVRE_ZOOM;
			else															man = MANOEUVRE_GAINSPEED;
		}
		else																man = MANOEUVRE_TURNINGFIGHT;
	}
	static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_SHAW") ? 1 : 0;
	if (tr) fprintf(stderr, "[shawm] t=%.0f id=%u v %u %s%s%s R=%.0f ata=%.0f bata=%.0f tail=%.0f vc=%.0f dE=%.0f %s -> man%d\n", double(s_shaw_t),
	                unsigned(m->uniqueID.count), unsigned(b->uniqueID.count), post, alone ? " alone" : "", shot ? " shot" : "", double(R),
	                double(acosf(fmaxf(-1.f, fminf(1.f, ata))) * 57.3f), double(acosf(fmaxf(-1.f, fminf(1.f, bata))) * 57.3f),
	                double(acosf(fmaxf(-1.f, fminf(1.f, tail))) * 57.3f), double(vc), double(dE), style > 0 ? "energy" : "angles", man);
	return man;
}
static void bdg_shaw_tick(AirStrucPtr a, float dt)
{
	if (!bdg_shaw_on() || !bdg_shaw_live(a)) return;
	if (!s_shaw_owner || ++s_shaw_orphan > 400) { s_shaw_owner = a->uniqueID.count; s_shaw_orphan = 0; }
	if (a->uniqueID.count == s_shaw_owner) { s_shaw_t += dt; s_shaw_orphan = 0; }
	if (a->leader || a->ai.combatskill < SKILL_HERO) return;			// element leaders only; hero only
	if (!getenv("BOB_AI_PILOT") && (a->InPlayersElement() || a->PlayerInFlight())) return;
	const unsigned id = a->uniqueID.count;
	BdgShaw& s = s_bdg_shaw[((id * 2654435761u) >> 9) & 1023];
	if (s.id != id) { s.id = id; s.next = 0; memset(&s.rec, 0, sizeof s.rec); s.cand = s.rec; s.steer_at = -1e9f; }
	if (s_shaw_t < s.next) return;
	s.next = s_shaw_t + 2.0f;
	AirStrucPtr W = a->Follower();
	const BdgShawRec v = bdg_shaw_assess(a, (W && W->Status.size == AIRSTRUCSIZE) ? W : AirStrucPtr(0), s);
	const bool changed = !(v == s.rec) && v == s.cand;			// hysteresis: two assessments 2 s apart must agree
	s.cand = v;
	if (changed) s.rec = v;
	bdg_shaw_steer(a, (W && W->Status.size == AIRSTRUCSIZE) ? W : AirStrucPtr(0), s.rec, s.steer_at);
	if (!changed) return;
	AirStrucPtr b = bdg_shaw_bandit(a); const bool w = W && W->Status.size == AIRSTRUCSIZE;
	char txt[320];
	snprintf(txt, sizeof txt, "Shaw: %s (%dv%d)%s; %s; lead %s/%s man%d; wing %d %s/%s man%d; energy %s; %s; enemy %s/%s; plan %s",
	         s_shaw_size[v.size], int(v.mine), int(v.theirs), v.dissimilar ? " dissimilar" : "", s_shaw_sec[v.section],
	         s_shaw_role[v.role[0]], s_shaw_post[v.post[0]], int(a->ai.manoeuvre),
	         w ? int(W->uniqueID.count) : -1, s_shaw_role[v.role[1]], s_shaw_post[v.post[1]], w ? int(W->ai.manoeuvre) : -1,
	         s_shaw_en[v.energy], s_shaw_pos[v.pos], s_shaw_esec[v.esec], s_shaw_epost[v.epost], s_shaw_plan[v.plan]);
	bob_acmi_event(a->uniqueID.count, "Message", txt);		// BoB's ACMI object id is uniqueID.count (REPLAY.CPP _oid)
	static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_SHAW") ? 1 : 0;
	if (tr)
		fprintf(stderr, "[shaw] t=%.0f unit=%u nat=%d %s; bandit=%d man%d range=%.0f\n", double(s_shaw_t), id, int(a->nationality), txt,
		        b ? int(b->uniqueID.count) : -1, b ? int(b->ai.manoeuvre) : -1, b ? double(bdg_shaw_rng(a, b)) : -1.0);
}

void bdg_fatigue_tick(AirStrucPtr a, float g, int path)	// once a frame; g = load factor the pilot feels, < 0: from his path
{
	bdg_shaw_tick(a, Timer_Code.FRAMETIME * 0.01f);
	if (!bdg_fatigue_on()) return;
	BdgPilot* p = bdg_pil(a, a->movecode == AUTO_COMBAT);
	if (!p) return;
	const float dt = Timer_Code.FRAMETIME * 0.01f, q = bdg_q300(a);
	if (g < 0) {	// load factor from the change of the velocity vector (simple movecode): |dv/dt - gravity| / g
		const float vx = float(a->vel_x) * 1e-4f, vy = float(a->vel_y) * 1e-4f, vz = float(a->vel_z) * 1e-4f;
		if (p->seen) {
			const float k = (dt < 0.5f) ? dt / 0.5f : 1.0f;	// signed 0.5 s average first: frame-to-frame velocity jitter cancels
			float ax = (vx - p->vx) / dt, ay = (vy - p->vy) / dt + 9.80665f, az = (vz - p->vz) / dt;
			const float am = sqrtf(ax * ax + ay * ay + az * az);	// one-frame jumps (speed set directly, resets): clip to 10 G
			if (am > 98.0665f) { const float s = 98.0665f / am; ax *= s; ay *= s; az *= s; }
			p->sax += (ax - p->sax) * k; p->say += (ay - p->say) * k; p->saz += (az - p->saz) * k;
			g = sqrtf(p->sax * p->sax + p->say * p->say + p->saz * p->saz) / 9.80665f; if (g > 9.0f) g = 9.0f;
		} else g = 1.0f;
		p->vx = vx; p->vy = vy; p->vz = vz; p->seen = true;
	}
	float force = p->roll * bdg_roll_k(a) * q; if (force > 1.0f) force = 1.0f;	// share of his strength on the stick
	if (path == 0) { const float lim = bdg_g_inst(a, *p); if (lim > 1.0f && g > lim) g = lim; }	// simple movecode: some manoeuvres set pitch and heading
		// directly, past any physical rate -- that excess is the model's, not the pilot's; he flies at most at his limit
	const float graw = g;
	p->gs += (g - p->gs) * (dt < 1.0f ? dt : 1.0f);	// felt G: 1 s average
	p->gsus += (graw - p->gsus) * (dt < 5.0f ? dt / 5.0f : 1.0f);	// sustained G: ~5 s mean (blackout, ease-off)
	g = p->gs;
	float ge = (g - 2.5f) / 2.5f; if (ge < 0) ge = 0;	// 1940: no G suit, no straining drill (Shaw p.377 is the trained modern pilot)
	float d = (ge * ge / 40.0f + force * 0.006f) * p->tiring;	// roll term light: the simple movecode always rolls at full rate
	if (g < 2.0f && force < 0.3f) d -= 0.003f;
	static int tr = -1; if (tr < 0) tr = getenv("BOB_TRACE_FATIGUE") ? 1 : 0;
	const float tol = bdg_g_tol(*p);
	if (p->bo > 0) { if ((p->bo -= dt) <= 0 && tr) fprintf(stderr, "[fatigue] id=%u comes round\n", p->id); }
	else if (path == 1 && p->gsus > tol + 0.7f && !bdg_emergency(a, p)) {	// blackout on the flight model only, where the G is physical
		p->bo = 3.0f + 3.0f * p->F;
		if (tr) fprintf(stderr, "[fatigue] id=%u shape=%d BLACKOUT g=%.2f tol=%.2f skill=%d F=%.2f for %.1f s\n", p->id, int(a->classtype->visible), g, tol, int(a->ai.combatskill), p->F, p->bo);
	}
	if (path == 1 && !bdg_emergency(a, p)) {	// he feels the grey-out coming and eases the pull, in proportion to the overshoot
		const float o1 = p->gsus - (bdg_g_lim(a, *p) - ((a->ai.combatskill < SKILL_REGULAR) ? 0.0f : 0.3f)), o2 = p->gs - bdg_g_inst(a, *p);
		const float over = (o1 > o2) ? o1 : o2;
		p->pull += (over > 0) ? -(1.0f + 2.0f * over) * dt : 1.0f * dt; if (p->pull < 0.1f) p->pull = 0.1f; if (p->pull > 1.0f) p->pull = 1.0f; }
	p->F += d * dt; if (p->F < 0) p->F = 0; if (p->F > 1) p->F = 1;
	if (tr && (p->tr += dt) >= 2.0f) {
		p->tr = 0;
		fprintf(stderr, "[fatigue] id=%u shape=%d path=%d mc=%d g=%.2f ias=%.0f roll=%.2f force=%.2f F=%.3f rollcap=%.2f gcap=%.2f glim=%.1f gsus=%.2f\n", p->id,
		        int(a->classtype->visible), path, int(a->movecode), g, 300.0 * sqrt(q), p->roll, force, p->F, bdg_roll_capacity(a), bdg_g_capacity(a), bdg_g_limit(a), p->gsus);
	}
	if (a->movecode == AUTO_COMBAT) bdg_tom_tick(a, p, dt, g);
	p->ohdg = SWord(a->hdg);
	p->roll = 0;
}
#endif
#if BOB_LINUX
/* BOB-PATCH-REST-1 40c, BDG BOB_FIGHTERFIX (bdg.txt, undocumented; the PO's is ON). Decoded from BDG 0.99 bob.exe:
   the AI move (0x54858e) calls a new routine (0x543ea2) for each aircraft when the switch is on; once per aircraft it
   rolls Math_Lib 1 in 3 and tests engine damage -- single-engine: the engine byte > 80.0 (0xa15740); Bf 110 (shape
   0x18 = ME110): the average of both engines > 80 * 0.667 -- then calls 0x450b11 = TransObj::LaunchCanopyEject (it
   matches line for line: not the player, not below FT_4000 = 0x1dc40). In words: an AI fighter pilot whose engine is
   hit may bail out. Port: an AI Spitfire/Hurricane/Bf 109/Bf 110 in combat (AUTO_COMBAT with a target) whose engine
   damage crosses those thresholds (BS_DAMLV1 = 85, so any engine damage on a single; one engine at BS_DAMLV2 or both
   damaged on a 110) decides once, 1 in 3, to bail out with Rowan's own MANOEUVRE_BAILOUT (throttle off,
   LaunchCanopyEject, inverted). Off in multiplayer. BOB_TRACE_FIGHTERFIX=1; test hooks BOB_TEST_FIGHTERDAMAGE_AT=<s>
   (damage the first enemy fighter's engine at s seconds) and BOB_TEST_FIGHTERFIX_ALWAYS=1 (skip the 1 in 3 roll). */
static void bdg_fighterfix(MobileItemPtr entry)
{
	if (entry->Status.size != AirStrucSize || _DPlay.Implemented) return;
	static int on = -1;
	if (on < 0) { extern int rowan_bdg_int(const char*, int); on = rowan_bdg_int("BOB_FIGHTERFIX", 0); }
	if (!on) return;
	AirStrucPtr a = (AirStrucPtr)entry;
	if (a == Persons2::PlayerSeenAC || a == Persons2::PlayerGhostAC || a->Status.deadtime || a->weap.Ejected) return;
	const ShapeNum vis = a->classtype->visible;
	if (vis != SPIT && vis != HURR && vis != ME109 && vis != ME110) return;
	if (SHAPESTUFF.GetShapePtr(a->shape)->AnimDataSize != AIRCRAFTANIM) return;
	AircraftAnimData* ad = (AircraftAnimData*)a->Anim;
	static int testAt = -2; static time_t t0 = 0; static bool testDone = false;
	if (testAt == -2) { const char* e = getenv("BOB_TEST_FIGHTERDAMAGE_AT"); testAt = e ? atoi(e) : -1; }
	if (testAt >= 0 && !testDone && Persons2::PlayerSeenAC && a->nationality != Persons2::PlayerSeenAC->nationality) {
		if (!t0) t0 = time(0);
		if (time(0) - t0 >= testAt) { testDone = true; ad->ENGINELEFT = BS_DAMLV1;
			fprintf(stderr, "[fighterfix] test: engine of uid=0x%x damaged (alt %.0f m)\n", (unsigned)a->uniqueID.count, a->World.Y / 100.0); } }
	if (a->movecode != AUTO_COMBAT || !a->ai.unfriendly) return;
	const int L = ad->ENGINELEFT, R = ad->ENGINERIGHT;
	const bool hit = (vis == ME110) ? ((L + R) / 2.0 > 80.0 * 0.667) : (((L > R) ? L : R) > 80);
	if (!hit) return;
	static unsigned decided[256]; static int nd = 0;
	for (int q = 0; q < nd; q++) if (decided[q] == a->uniqueID.count) return;
	if (nd < 256) decided[nd++] = a->uniqueID.count;
	const bool bail = getenv("BOB_TEST_FIGHTERFIX_ALWAYS") || Math_Lib.rnd(3) == 0;
	if (getenv("BOB_TRACE_FIGHTERFIX"))
		fprintf(stderr, "[fighterfix] uid=0x%x shape %d engines %d/%d alt %.0f m: %s\n", (unsigned)a->uniqueID.count, (int)vis, L, R,
		        a->World.Y / 100.0, bail ? (a->World.Y >= FT_4000 ? "bails out" : "would bail, below 4000 ft") : "stays with it");
	if (bail && a->World.Y >= FT_4000)
	{
		a->ai.manoeuvre = MANOEUVRE_BAILOUT;
		a->ai.ManStep = PHASE0;
	}
}
#endif

void mobileitem::MoveList(MobileItemPtr entry,WorldStuff& world)
{
#if BOB_LINUX
	/* BOB_MANUAL=1: hand the player's aircraft to MANUAL pilot control.
	   Measured 2026-09-04: under BOB_BOOT_FRONTEND the player sits at `movecode=12` =
	   AUTO_TRAININGTAKEOFF -- an AUTOMATIC training controller owns its movement, so the throttle
	   is stored (HUD `Power 90`) but never acted on, and the aircraft never rolls (`Speed 0Kts`).
	   That is why no headless in-flight capture has ever worked in this port.
	   The game's own transition is Manual_Pilot.AutoToggle(ManualPilot::MANUAL) (OVERLAY.CPP:6297
	   and three other sites), so use it rather than writing movecode directly -- AutoToggle is
	   what the game runs when a player takes over, and it does whatever else that entails.
	   One-shot, and only once the player entry actually exists. Default off. */
	if (getenv("BOB_MANUAL") && entry == Persons2::PlayerSeenAC) {
		static int handed = 0;
		if (!handed) {
			handed = 1;
			fprintf(stderr, "[manual] movecode was %d -> AutoToggle(MANUAL)\n", (int)entry->movecode);
			fflush(stderr);
			Manual_Pilot.AutoToggle(ManualPilot::MANUAL);
		}
	}
	/* REPLAY-LAB-1 S6: BOB_AI_PILOT=<seconds> hands the player's aircraft to the game's AI that many seconds
	   into the flight, with the game's own AutoToggle(TOGGLE) (MANUAL -> AUTO, Associate()), so the AI flies
	   YOUR aircraft at normal speed and the Tacview export records it at its full rate -- the gold standard for
	   replaylab. (In the game the only hand-over is accelerated time, which records at a fraction of the rate.)
	   Also traces the player's movecode every 10 s, which names the AI routine flying it. Default off. */
#if BOB_LINUX
	/* PATCH-PORT-1, BDG "Autopilot" (manual 2.2.7, Ctrl+A = BDG command 286 in the PO's key map): hands the player's
	   aircraft to the game's AI and back, with the game's own AutoToggle(TOGGLE) -- the hand-over accelerated time
	   uses. (BDG also forced an outside view to dodge problems of its own; not copied.) BOB_NO_BDG_AUTOPILOT=1
	   reverts; BOB_TRACE_AUTOPILOT=1 prints the toggle and the altitude every 5 s after it. */
	if (entry == Persons2::PlayerSeenAC && !getenv("BOB_NO_BDG_AUTOPILOT")) {
		static time_t apAt = 0, apTrace = 0;
		if (Key_Tests.KeyPress3d((KeyVal3D)(286*2))) {
			Manual_Pilot.AutoToggle(ManualPilot::TOGGLE);
			apAt = time(0);
			if (getenv("BOB_TRACE_AUTOPILOT"))
				fprintf(stderr, "[autopilot] Ctrl+A: AutoToggle(TOGGLE), movecode now %d\n", (int)entry->movecode);
		}
		if (apAt && getenv("BOB_TRACE_AUTOPILOT") && time(0) - apTrace >= 5) {
			apTrace = time(0);
			fprintf(stderr, "[autopilot] +%lds movecode=%d alt=%.0f m vel=%d\n", (long)(apTrace - apAt),
			        (int)entry->movecode, entry->World.Y / 100.0, (int)entry->vel_);
		}
	}
#endif
#if BOB_LINUX
	/* BOB-QM-ENGAGE-1 (PO 10-16: in QM One on One the 109 "flies away with no clear purpose" for minutes before its boom
	   and zoom, and the Enemy Position Indicator stays blank meanwhile): a duel's premise is that both pilots know of
	   each other, but Rowan's spot sweep (MSGAI Seen(), one probabilistic look per pair about every 25 s) left the 109 on
	   its waypoints until luck had it see the player. When the flight holds exactly one aircraft per side, the enemy
	   spots the player 2 s in (SpottedUnfriendly -- what a successful Seen() does). Once per flight (a 5 s gap in the
	   calls = a new flight), and only if the enemy has no target yet. BOB_NO_DUEL_AWARE=1 reverts; BOB_TRACE_DUEL_AWARE=1 traces. */
	if (entry == Persons2::PlayerSeenAC) {
		static int s_off = -1; if (s_off < 0) s_off = getenv("BOB_NO_DUEL_AWARE") ? 1 : 0;
		/* the 2 s is SIM time (bdg_simtime_cs): a wall-clock second made the spot land 0.5-2 s into the sim at random,
		   so no QM run repeated (57 SHAW-TACTIC-1 read-only proof); the wall clock only spots a new flight */
		extern long bdg_simtime_cs;
		static time_t s_last = 0; static long s_t0 = 0; static bool s_done = false;
		time_t now = time(0);
		if (!s_last || now - s_last > 5) { s_t0 = bdg_simtime_cs; s_done = false; }
		s_last = now;
		if (!s_off && !s_done && bdg_simtime_cs - s_t0 >= 200) {
			s_done = true;
			struct DuelSpot : ArtInt { static bool Call(AirStruc* a, AirStruc* t) { return ((DuelSpot&)Art_Int).SpottedUnfriendly(a, t); }
			                           static AirStruc* Ac(int i) { return ACArray[i]; } static int N() { return ACARRAYSIZE; } };
			AirStrucPtr me = (AirStrucPtr)entry, foe = NULL; int nf = 0, ne = 0;
			for (int i = 0; i < DuelSpot::N(); i++) {
				AirStrucPtr a = DuelSpot::Ac(i);
				if (!a || a->Status.deadtime) continue;
				if (a->nationality == me->nationality) nf++; else { ne++; foe = a; }
			}
			bool fire = (nf == 1 && ne == 1 && foe && !foe->ai.unfriendly);
			if (fire) DuelSpot::Call(foe, me);
			if (foe && nf == 1 && ne == 1 && getenv("BOB_TEST_FOE_SKILL"))	/* AI-TACTICS-1 test: the duel opponent's skill (180 = ACE) */
				foe->ai.combatskill = atoi(getenv("BOB_TEST_FOE_SKILL"));
			if (getenv("BOB_TRACE_DUEL_AWARE"))
				fprintf(stderr, "[duelaware] friends=%d enemies=%d foe=0x%x -> %s\n", nf, ne, foe ? (unsigned)foe->uniqueID.count : 0u,
				        fire ? "enemy spots the player" : "no change");
		}
	}
#endif
#if BOB_LINUX
	/* test hook (PATCH-PORT-1 ground/radio turn, BDG manual 4.4.11): BOB_TEST_BANDITCALL_AT=<s> -- s seconds into the
	   flight the player "tells his comrades" about the nearest enemy aircraft: ArtInt::SpottedUnfriendly(player,
	   enemy), exactly what the radio Bandit call does once it has found the padlocked target. */
	/* AI-TACTICS-1: BOB_TEST_SPOTPLAYER_AT=<s> -- the reverse: the nearest enemy spots the player (SpottedUnfriendly(enemy,
	   player)), so a one-on-one QM becomes a fight; with Ctrl+A (BOB_DIK_MS=3000,30,29) both sides are AI-flown. */
	if (entry == Persons2::PlayerSeenAC && (getenv("BOB_TEST_BANDITCALL_AT") || getenv("BOB_TEST_SPOTPLAYER_AT"))) {
		const char* atS = getenv("BOB_TEST_SPOTPLAYER_AT") ? getenv("BOB_TEST_SPOTPLAYER_AT") : getenv("BOB_TEST_BANDITCALL_AT");
		struct BdgSpot : ArtInt { static bool Call(AirStruc* a, AirStruc* t) { return ((BdgSpot&)Art_Int).SpottedUnfriendly(a, t); }
		                          static void Engage(AirStruc* a, AirStruc* t) { ((BdgSpot&)Art_Int).SetEngage(a, t, MANOEUVRE_SELECT, ANGLES_0Deg, ANGLES_0Deg, false); }
		                          static AirStruc* Ac(int i) { return ACArray[i]; } static int N() { return ACARRAYSIZE; } };
		static time_t t0 = 0; static bool done = false;
		if (!t0) t0 = time(0);
		if (!done && time(0) - t0 >= atoi(atS)) {
			done = true;
			AirStrucPtr me = (AirStrucPtr)entry, best = NULL; FP bd = 0;
			for (int i = 0; i < BdgSpot::N(); i++) {
				AirStrucPtr a = BdgSpot::Ac(i);
				if (!a || a->nationality == me->nationality) continue;
				FP d = me->Distance3DSquared(&a->World);
				if (!best || d < bd) { best = a; bd = d; }
			}
			fprintf(stderr, "[groundturn] test: player calls bandit uid=0x%x at %.0f m\n", best ? (unsigned)best->uniqueID.count : 0u,
			        best ? sqrt(double(bd)) / 100.0 : 0.0);
			if (best && getenv("BOB_TEST_SPOTPLAYER_AT")) { BdgSpot::Call(best, me); BdgSpot::Engage(me, best); }	/* both sides engage (the player's own spot call waits on the radio) */
			else if (best && !getenv("BOB_TEST_BANDITCALL_DRY")) BdgSpot::Call(me, best);	/* _DRY: trace only (baseline) */
		}
		static time_t lastG = 0;
		if (done && getenv("BOB_TRACE_GROUNDTURN") && time(0) - lastG >= 5) {
			lastG = time(0);
			for (AirStrucPtr nf = ((AirStrucPtr)entry)->FindFormpos0(); nf; nf = nf->fly.nextflight)
				for (AirStrucPtr f = nf; f; f = f->Follower())
					fprintf(stderr, "[groundturn] +%lds uid=0x%x movecode=%d manoeuvre=%d gear=%d alt=%.0fm vel=%d\n",
					        (long)(lastG - t0), (unsigned)f->uniqueID.count, (int)f->movecode, (int)f->ai.manoeuvre,
					        f->fly.pModel ? (int)f->fly.pModel->GearTouched : -1, f->World.Y / 100.0, (int)f->vel_);
		}
	}
#endif
#if BOB_LINUX
	/* test hook (NO_FRIENDLY_COLLISIONS): BOB_TEST_RAM_AT=<s> -- once, s seconds into the flight, put the player 2 m
	   above the nearest friendly aircraft (re-sectored), so the collision code must decide */
	if (entry == Persons2::PlayerSeenAC && getenv("BOB_TEST_RAM_AT")) {
		struct BdgRam : ArtInt { static AirStruc* Ac(int i) { return ACArray[i]; } static int N() { return ACARRAYSIZE; } };
		static time_t t0 = 0; static bool done = false;
		if (!t0) t0 = time(0);
		AirStrucPtr me = (AirStrucPtr)entry;
		if (!done && time(0) - t0 >= atoi(getenv("BOB_TEST_RAM_AT")) && me->fly.pModel) {
			done = true;
			AirStrucPtr best = NULL; FP bd = 0;
			for (int i = 0; i < BdgRam::N(); i++) {
				AirStrucPtr a = BdgRam::Ac(i);
				if (!a || a == me || a->nationality != me->nationality) continue;
				FP d = me->Distance3DSquared(&a->World);
				if (!best || d < bd) { best = a; bd = d; }
			}
			if (best) {
				currworld->RemoveFromWorld(me);
				me->fly.pModel->ResetPosition(me, best->World.X, best->World.Y + 200, best->World.Z);
				currworld->AddToWorld(me);
				fprintf(stderr, "[friendcoll] test: player put 2 m above friendly uid=0x%x (was %.0f m away)\n",
				        (unsigned)best->uniqueID.count, sqrt(double(bd)) / 100.0);
			}
		}
	}
#endif
	if (getenv("BOB_AI_PILOT") && entry == Persons2::PlayerSeenAC) {
		static time_t t0 = 0, lastTrace = 0;
		static int handed = 0;
		/* SIM seconds, not time(0): a wall-clock handover lands at a different game time whenever the run is
		   slower or faster, so two identical AI-v-AI runs diverged at +1.6 s (SHAW-TACTIC-1 57 read-only proof). */
		static double simt = 0; simt += Timer_Code.FRAMETIME * 0.01;
		time_t now = (time_t)simt;
		if (!t0) t0 = now;
		if (!handed && now - t0 >= atoi(getenv("BOB_AI_PILOT"))) {
			handed = 1;
			fprintf(stderr, "[aipilot] movecode %d -> AutoToggle(TOGGLE) at +%lds\n", (int)entry->movecode, (long)(now - t0));
			fflush(stderr);
			Manual_Pilot.AutoToggle(ManualPilot::TOGGLE);
		}
		static int lastMc = -1;
		if (now - lastTrace >= 1 || (int)entry->movecode != lastMc) {
			lastTrace = now; lastMc = (int)entry->movecode;
			SLong gl = Land_Scape.GetGroundLevel(entry);		/* game cm, like World.Y */
			fprintf(stderr, "[aipilot] +%lds player movecode=%d handed=%d alt=%.2f m ground=%.2f m above=%.2f m\n",
					(long)(now - t0), (int)entry->movecode, handed, entry->World.Y / 100.0, gl / 100.0,
					(entry->World.Y - gl) / 100.0);
			fflush(stderr);
		}
	}
	/* 2026-09-04: does the world update run at all under BOB_BOOT_FRONTEND? A first version of
	   this probe sat at ONE of the two MoveItem call sites (446 of 446/480) and printed nothing --
	   which proves only that THAT BRANCH was not taken, not that the loop is idle. Placed at the
	   function entry, before any branch, so silence here means the update genuinely does not run.
	   BOB_TRACE_LOOP=1; default off. */
	if (getenv("BOB_TRACE_LOOP")) {
		static long calls = 0;
		if ((calls++ % 600) == 0)
			fprintf(stderr, "[loop] MoveList calls=%ld PlayerSeenAC=%p entry=%p isplayer=%d\n",
			        calls, (void*)Persons2::PlayerSeenAC, (void*)entry,
			        entry == Persons2::PlayerSeenAC), fflush(stderr);
		/* per-entry: does the PLAYER ever reach this function, and by which path? Counting is the
		   point -- an absence at one call site proved nothing twice today. */
		if (entry == Persons2::PlayerSeenAC) {
			static long pn = 0;
			if ((pn++ % 300) == 0)
				fprintf(stderr, "[loop] PLAYER entry seen %ld times, movecode=%d\n",
				        pn, (int)entry->movecode), fflush(stderr);
		}
	}
#endif
#ifdef _OLDWORLD														//CSB 7Aug00
	static bool firstframe = true;
	if(firstframe)					//CSB 7Aug00
	{
		for(MobileItemPtr mob = ACList; mob; mob = mob->nextmobile)		//CSB 7Aug00
		{																//CSB 8Aug00
			mob->OldWorld = mob->World;									//CSB 7Aug00
			mob->OldSector = world.GetSector(mob);						//CSB 8Aug00
		}																//CSB 8Aug00
		for(mob = SAGList; mob; mob = mob->nextmobile)	//CSB 7Aug00
		{																//CSB 8Aug00
			mob->OldWorld = mob->World;									//CSB 7Aug00
			mob->OldSector = world.GetSector(mob);						//CSB 8Aug00
		}																//CSB 8Aug00
		firstframe = false;
	}
#endif																	//CSB 7Aug00

//	static x=0;
	currworld=&world;											//PD 12Jul96

#ifdef DEBUGMOVE
	FILE* fp;
	if (_Replay.Record)
	{
		fp=fopen("record.txt","at");
	}
	else if (_Replay.Playback)
	{
		fp=fopen("playback.txt","at");
	}
	else if (_DPlay.Implemented)
	{
		fp=fopen("commsac.txt","at");
	}
#endif

#if BOB_LINUX
	/* AI-TACTICS-1: BOB_TRACE_ACPOS=<n> dumps every aircraft each n list passes */
	static unsigned s_posTick = 0; static int s_posN = -1;
	if (s_posN < 0) { const char* e = getenv("BOB_TRACE_ACPOS"); s_posN = e ? atoi(e) : 0; }
	if (s_posN > 0) s_posTick++;
#endif
	while (entry)
	{
#if BOB_LINUX
		bdg_bomberfix(entry);	/* every aircraft in the list, not just its head */
		bdg_fighterfix(entry);
		bdg_gunnerfix(entry);
		if (entry->Status.size == AirStrucSize && getenv("BOB_TRACE_DUEL")) {	/* AI-TACTICS-1: outcome per aircraft */
			AirStrucPtr ac = AirStrucPtr(entry); static unsigned s_id[64]; static int s_st[64]; static time_t s_t0 = 0;
			if (!s_t0) s_t0 = time(0);
			int slot = ac->uniqueID.count & 63;
			int st = (ac->AircraftDamaged() ? 1 : 0) | ((ac->Status.deadtime || ac->Status.deaded) ? 2 : 0)
			       | ((ac->movecode == AUTO_NOPPILOT || ac->movecode == AUTOSAG_DESTROYED) ? 4 : 0);
			if (s_id[slot] != ac->uniqueID.count) { s_id[slot] = ac->uniqueID.count; s_st[slot] = 0; }
			if (st != s_st[slot]) {
				AirStrucPtr k = AirStrucPtr(ac->ai.attacker);
				fprintf(stderr, "[duel] +%lds id=%d type=%d state %d->%d (1 damaged, 2 dead, 4 no pilot/destroyed) alt=%d attacker=%d\n",
					long(time(0) - s_t0), int(ac->uniqueID.count), ac->fly.pModel ? int(ac->fly.pModel->Type) : -1, s_st[slot], st,
					int(ac->World.Y / 100), k ? int(k->uniqueID.count) : -1);
				s_st[slot] = st;
			}
		}
		if (s_posN > 0 && (s_posTick % s_posN) == 0 && entry->Status.size == AirStrucSize) {
			AirStrucPtr ac = AirStrucPtr(entry); AirStrucPtr u = AirStrucPtr(ac->ai.unfriendly);
			fprintf(stderr, "[acpos] t=%u id=%d nat=%d type=%d mc=%d man=%d pos=(%d,%d,%d) vel=%d unf=%d dead=%d\n", s_posTick,
				int(ac->uniqueID.count), int(ac->nationality), ac->fly.pModel ? int(ac->fly.pModel->Type) : -1, int(ac->movecode), int(ac->ai.manoeuvre),
				int(ac->World.X / 100), int(ac->World.Y / 100), int(ac->World.Z / 100), int(ac->vel_), u ? int(u->uniqueID.count) : -1, int(ac->Status.deaded));
		}
#endif
		ULong state=_Replay.GetState();

		if (_Replay.IsCampState(state))
		{
// only recycle ac in campaign.....

			if(!(timeofday & 0xff))
				if(entry->Status.size == AIRSTRUCSIZE)
					if(entry != Persons2::PlayerSeenAC)
						if(entry->movecode != AUTO_SPAREAC)
						{
							AirStrucPtr ac = AirStrucPtr(entry);
							MinAnimData* mad = (MinAnimData*)ac->Anim;
							if (	mad->IsInvisible					//RJS 10Oct00
								||	(ac->fly.pModel->CrashTime >= DEADTIMEOUT_RECYCLE)	)	//RJS 10Oct00
								ac->RecycleAC();
						}
		}

		MobileItemPtr	next=entry->nextmobile;
		UWord	oldsector=-1;										  //JIM 17/02/99
		if (entry->movecode!=AUTO_NOPPILOT)								  //JIM 17/02/99
			oldsector=	world.GetSector(*entry);					  //JIM 17/02/99
//DeadCode JIM 3Aug00 		else
//DeadCode JIM 3Aug00 			oldsector=-1;

		assert (entry!=Persons2::PlayerGhostAC)
		if((entry == Persons2::PlayerSeenAC) && (Persons2::PlayerSeenAC->classtype->aerobaticfactor != AEROBATIC_LOW))
		{
// Set fake engine sound...

// should go in here to keep ghostac upto date 

#ifdef DEBUGMOVE
			if (_Replay.Record || _Replay.Playback || _DPlay.Implemented)
			{
				UWord uid=entry->uniqueID.count;
//DeadCode AMM 12May99 				fprintf(fp,"%d X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d (ghost)\n",
				fprintf(fp,"%d ID=%x X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d (ghost)\n",
					_Replay.replayframecount,
					uid,
					Persons2::PlayerGhostAC->World.X,
					Persons2::PlayerGhostAC->World.Y,
					Persons2::PlayerGhostAC->World.Z,
					Persons2::PlayerGhostAC->hdg.a,
					Persons2::PlayerGhostAC->pitch.a,
					Persons2::PlayerGhostAC->roll.a,
					Persons2::PlayerGhostAC->vel_
					);
			}
#endif
			_DPlay.CheckGhost();

//DeadCode AMM 3Aug00 			FILE* g=fopen("preddel.txt","at");
//DeadCode AMM 3Aug00 			fprintf(g,"Prev: X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d\n",
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->World.X,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->World.Y,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->World.Z,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->hdg.a,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->pitch.a,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->roll.a,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->vel_);

			_DPlay.AddDeltas(Persons2::PlayerGhostAC,TRUE,4);
			_DPlay.PredictMove(Persons2::PlayerGhostAC);
			_DPlay.AddDeltas(Persons2::PlayerGhostAC,FALSE,4);
//DeadCode AMM 3Aug00 			fprintf(g,"Pred: X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d\n",
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->World.X,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->World.Y,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->World.Z,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->hdg.a,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->pitch.a,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->roll.a,
//DeadCode AMM 3Aug00 				Persons2::PlayerGhostAC->vel_);
//DeadCode AMM 3Aug00 
//DeadCode AMM 3Aug00 			fclose(g);

			_DPlay.BackupGhost();
		}
		else
		{
#ifdef DEBUGMOVE
				if ((_Replay.Record || _Replay.Playback || _DPlay.Implemented) && entry->Status.size==AIRSTRUCSIZE )
				{
					ULong m=entry->movecode;
					ULong man=((AirStrucPtr)entry)->ai.manoeuvre;
					UWord uid=entry->uniqueID.count;
//DeadCode AMM 26Jul00 					UByte inform=((AirStrucPtr)entry)->information;

//DeadCode AMM 12May99 					fprintf(fp,"ID=%d inform=%d move=%d man=%d X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d\n",
//DEADCODE AMM 31/01/00 					fprintf(fp,"ID=%x X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d inform=%d move=%d man=%d\n",

//DeadCode AMM 26Jul00 					if (_Replay.replayframecount==9)
//DeadCode AMM 26Jul00 					{
//DeadCode AMM 26Jul00 						_Replay.DumpModelData((AirStrucPtr)entry);
//DeadCode AMM 26Jul00 					}

//DeadCode AMM 11Jul00 					fprintf(fp,"ID=%x X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d \n",
//DeadCode AMM 11Jul00 					fprintf(fp,"ID=%x if=%d mc=%d man=%d H=%d P=%d R=%d V=%d \n",

//DeadCode AMM 25Jul00 					UWord u=0;
//DeadCode AMM 25Jul00 					
//DeadCode AMM 25Jul00 					if (((AirStrucPtr)entry)->ai.unfriendly)
//DeadCode AMM 25Jul00 						u=entry->uniqueID.count;

//DeadCode AMM 26Jul00 					AirStrucPtr a=((AirStrucPtr)entry);

//DeadCode AMM 25Jul00 					for(PENGINE pEngine = a->fly.pModel->EngineList; pEngine; pEngine = pEngine->List.NextItem())
//DeadCode AMM 25Jul00 					{
//DeadCode AMM 25Jul00 						fprintf(fp,"ptx=%f pty=%f ptz=%f\n",
//DeadCode AMM 25Jul00 							pEngine->Speed);
//DeadCode AMM 25Jul00 					}

					fprintf(fp,"ID=%x MO=%d MA=%d X=%d Y=%d Z=%d H=%d P=%d R=%d V=%d \n",
							uid,
//DeadCode AMM 25Jul00 							u,
//DeadCode AMM 11Jul00 							Math_Lib.rnd(),
//DeadCode AMM 11Jul00 							inform,
							m,
							man,
							entry->World.X,
							entry->World.Y,
							entry->World.Z,
							entry->hdg.a,
							entry->pitch.a,
							entry->roll.a,
							entry->vel_
							);
			}

#endif
			AutoMoveCodeTypeSelect	lastmovecode=entry->movecode;
  			if (entry->uniqueID.commsmove)
 			{
				if (entry->shape!=EMPTY)
				{
// AC with empty shape are not in world, so dont move them

					if (!mobileitem::currworld->vp->Accel())
					{
// not in accel predict 
// make other players ac props turn....

						AirStrucPtr	ac = AirStrucPtr(entry);			//RJS 12Jul00

						ac->fly.rpm = ac->fly.rpm1 = 2000;				//RJS 12Sep00

						ShapeDescPtr	sdptr = SHAPESTUFF.GetShapePtr(ac->shape);
						if (sdptr->AnimDataSize == AIRCRAFTANIM)
						{
							AircraftAnimData*	adptr = (AircraftAnimData*)ac->Anim;
							adptr->acrpm = (ac->fly.rpm * 3)/5;
							adptr->acrpm1 = (ac->fly.rpm1 * 3)/5;
						}
						else
						{
							if (sdptr->AnimDataSize == SIMPLEAIRCRAFTANIM)
							{
								SimpleAircraftAnimData*	adptr = (SimpleAircraftAnimData*)ac->Anim;
								adptr->acrpm = (ac->fly.rpm * 3)/5;
							}
						}

//DeadCode AMM 8Sep00 #ifndef NDEBUG															//CSB 20Jul00
//DeadCode AMM 8Sep00 						_DPlay.CheckPlayer2();
//DeadCode AMM 8Sep00 #endif																	//CSB 20Jul00
						_DPlay.PredictMove((AirStrucPtr)entry);
//DeadCode AMM 8Sep00 #ifndef NDEBUG															//CSB 20Jul00
//DeadCode AMM 8Sep00 						_DPlay.BackupPlayer2();
//DeadCode AMM 8Sep00 #endif																	//CSB 20Jul00
					}
					else
					{
// in accel need to move as no delta packets will arrive

						entry->movecode=AUTO_COMMSACCEL;
						if (getenv("BOB_TRACE_LOOP") && entry == Persons2::PlayerSeenAC) {
							static long a1 = 0;
							if ((a1++ % 300) == 0) fprintf(stderr, "[loop] player -> MoveItem SITE-A (accel) %ld\n", a1), fflush(stderr);
						}
						MoveItem(entry,world);
					}

// update model pos.Y so that ac leaves behind contrails?

					AirStrucPtr ac = AirStrucPtr(entry);
					if(ac->fly.pModel)
					{
						ac->fly.pModel->Pos.x = ac->World.X;
						ac->fly.pModel->Pos.y = ac->World.Y;
						ac->fly.pModel->Pos.z = ac->World.Z;
					}
				}
			}
			else
			{
				ANGLES currhdg=entry->hdg;						//AMM 31Jul98

// only need temp checks if we are having gun camera when playing as gunner

				if(entry==Persons2::PlayerSeenAC)
				{
//DeadCode CSB 18Jul00 					_DPlay.CheckSeen();
					_DPlay.UpdateBomberSeen();
					Three_Dee.viewer_oldhdg   = Persons2::PlayerSeenAC->hdg;
					Three_Dee.viewer_oldpitch = Persons2::PlayerSeenAC->pitch;
					Three_Dee.viewer_oldroll  = Persons2::PlayerSeenAC->roll;
//DeadCode CSB 18Jul00 					_DPlay.BackupSeen();
				}


//DeadCode AMM 8Oct00 				if (_Replay.replayframecount==353 && entry->uniqueID.count==0x1300)
//DeadCode AMM 8Oct00 					INT3;

				MoveItem(entry,world);

				if(entry==Persons2::PlayerSeenAC)
				{
					// Fake engine sound for bombers...
					AircraftAnimData*	adptr = (AircraftAnimData*)Persons2::PlayerSeenAC->Anim;	//RJS 11May00
					static EngineInfo	enginedesc;						

					enginedesc.power = (Persons2::PlayerSeenAC->fly.rpm*255)/Persons2::PlayerSeenAC->classtype->maxrpm;
					if (enginedesc.power > 255)	enginedesc.power = 255;

#if BOB_LINUX
					/* 2026-09-04: is the player's flight model being STEPPED? The QM scramble sits
					   at Speed 0 with Power 90, and the engine-starter theory was refuted (the
					   starter is only polled under FD_ENGINEMANAGMENT, which is not a default).
					   This block runs every frame for the player, so it is the honest place to
					   ask. rpm > 0 means the engine turns; vel changing means the model integrates.
					   BOB_TRACE_MOVE=1; default off. */
					if (getenv("BOB_TRACE_MOVE")) {
						static int mn = 0;
						if ((mn++ % 120) == 0)
							fprintf(stderr, "[move] rpm=%d vel=%d thrust%%=%d\n",
							        (int)Persons2::PlayerSeenAC->fly.rpm,
							        (int)Persons2::PlayerSeenAC->vel_,
							        (int)Persons2::PlayerSeenAC->fly.thrustpercent), fflush(stderr);
					}
#endif
					enginedesc.rpm_engine = Persons2::PlayerSeenAC->fly.rpm;
					enginedesc.rpm_starter = 0;
					enginedesc.rattle = adptr->ENGINELEFT;			
					enginedesc.cockpitnobble = 0;
					enginedesc.velocity = Persons2::PlayerSeenAC->vel_;	//RJS 9Jun00
					enginedesc.xoffset = 0;

					_Miles.PlayEngine(Persons2::PlayerSeenAC,enginedesc,enginedesc);
			 
					int gunnerammo = adptr->weaponlaunchers[SHAPE.GetRearGunner_Index()].LoadedStores;
					OverLay.SetInfoLineVals(Persons2::PlayerSeenAC->vel_ * 0.001, gunnerammo, Persons2::PlayerSeenAC->World.Y * (3.2808 / 100), Persons2::PlayerSeenAC->hdg / 182, Persons2::PlayerSeenAC->fly.thrustpercent); 

//DEADCODE CSB 11Oct00						PolyPitAnimData*	ppadptr = (PolyPitAnimData*)Persons2::PlayerSeenAC->Anim;
//DEADCODE CSB 11Oct00						int		totalammo = ppadptr->actotalammoleft;

//DEADCODE CSB 11Oct00				 		OverLay.SetInfoLineVals(Persons2::PlayerSeenAC->vel_ * 0.001, totalammo, Persons2::PlayerSeenAC->World.Y * (3.2808 / 100), Persons2::PlayerSeenAC->hdg / 182, Persons2::PlayerSeenAC->fly.thrustpercent); 	//RJS 5Oct00
					
					_DPlay.CheckGhost();
					_DPlay.UpdateBomberGhost();
					_DPlay.BackupGhost();
				}

				if (entry->uniqueID.changed)					//AMM 31Jul98
				{												//AMM 31Jul98
					if (entry->Status.size==AirStrucSize)		//AMM 31Jul98
					{											//AMM 31Jul98
						((AirStrucPtr)(entry))->fly.dhdg= (SWord)(entry->hdg - currhdg);			//AMM 31Jul98
					}											//AMM 31Jul98
				}												//AMM 31Jul98
			}
		}
		AutoMoveCodeTypeSelect	nextmovecode=entry->movecode;
#ifndef	NDEBUG
		if(		(!entry->uniqueID.changed) && (entry->movecode != AUTO_SPAREAC)		//AMM 31Jul98	//CSB 10Aug00
			&&	(oldsector != 0xffff) && (oldsector != world.GetSector(*entry))		)	//AMM 31Jul98	//CSB 10Aug00
		{
#ifdef __WATCOMC__
				INT3();
#endif
#ifdef __MSVC__
				INT3;
#endif
		}
#endif
		if (entry->uniqueID.changed && oldsector!=0xFFFF)				  //JIM 17/02/99
		{
			UWord	newsector=	world.GetSector(*entry);
#ifdef NDEBUG
			if (newsector!=oldsector)
#endif			//By not doing this test in debug mode I can track down problems more quickly.
			{
#ifndef NDEBUG
				AirStrucPtr ac = NULL;
				if(entry->Status.size == AIRSTRUCSIZE)
					ac = AirStrucPtr(entry);
#endif
				world.RemoveFromSector(entry,oldsector);
				world.AddToWorld(entry);
			}
			entry->uniqueID.changed = FALSE;						//RDH 26Jun96
		}
		else
			oldsector=oldsector;

#ifdef _OLDWORLD															//CSB 7Aug00
		entry->OldWorld = entry->World;										//CSB 7Aug00
		entry->OldSector = world.GetSector(*entry);							//CSB 8Aug00
		for(MobileItemPtr mob = ACList; mob; mob = mob->nextmobile)			//CSB 7Aug00
			if(mob != Persons2::PlayerGhostAC)								//CSB 7Aug00
			{
				AirStrucPtr ac = AirStrucPtr(mob);							//CSB 8Aug00
				if((mob->OldWorld.X != mob->World.X) || (mob->OldWorld.Z != mob->World.Z))
					INT3;													//CSB 7Aug00
				if(mob->OldSector != world.GetSector(mob))					//CSB 8Aug00
					INT3;													//CSB 8Aug00
			}																//CSB 8Aug00
#endif																		//CSB 7Aug00

		if(entry->Status.size == AIRSTRUCSIZE)							//CSB 12Oct00
			AirStrucPtr(entry)->ai.moved = TRUE;						//CSB 12Oct00

		entry=next;												//RDH 23Feb96
	}
#ifdef DEBUGMOVE
	if (_Replay.Record || _Replay.Playback || _DPlay.Implemented)
	{
		fprintf(fp,"\n");
		fclose(fp);
	}
#endif
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveList
//Author		Jim Taylor
//Date			Thu 15 Feb 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
//DEADCODE JIM 31/05/00 #ifndef	NDEBUG
//DEADCODE JIM 31/05/00 	  int	Count_SetFlightParams=0;
//DEADCODE JIM 31/05/00 #endif
void mobileitem::MoveItem(MobileItemPtr entry,WorldStuff& world)
{
//DEADCODE JIM 31/05/00 #ifndef	NDEBUG
//DEADCODE JIM 31/05/00 	Count_SetFlightParams=0;
//DEADCODE JIM 31/05/00 #endif

#ifndef NDEBUG
/* TEST CODE CSB 14/07/99 */
/* TEST CODE CSB 14/07/99 */
/* TEST CODE CSB 14/07/99 */
/*
if((entry == Persons2::PlayerGhostAC->follower) || (entry == Persons2::PlayerSeenAC->follower))
	if(((!currworld->vp) || (!currworld->vp->Accel())) && (!(Persons2::PlayerSeenAC->fly.pModel->FrameCount & EVERY256cs)))
	{
		SWord y = 0;
		for(AirStrucPtr lead = Persons2::PlayerGhostAC->FindFormpos0(); lead; lead = lead->fly.nextflight)
			for(AirStrucPtr ac = lead; ac; ac = ac->Follower())
			{
				PrintVar(40, y, "%.0f ", FP(y));
				if(ac->movecode != AUTO_COMBAT)
				{
					PrintVar(43, y, "%.0f  ", FP(ac->movecode));
					PrintString(47, y, "----------------");
				}
				else
				{
					PrintString(43, y, "COM ");
					if((!ac->ai.unfriendly) || (ac->ai.unfriendly->Status.size != AirStrucSize))
						PrintString(47, y, "None ");
					else
						switch(AirStrucPtr(ac->ai.unfriendly)->fly.pModel->Type)
						{
							case AT_SPITFIRE:	PrintString(47, y, "SPIT ");	break;
							case AT_HURRICANE:	PrintString(47, y, "HURR ");	break;
							case AT_BF109:		PrintString(47, y, "M109 ");	break;
							case AT_BF110:		PrintString(47, y, "M110 ");	break;
							case AT_JU87B:		PrintString(47, y, "JU87 ");	break;
							case AT_DEFIANT:	PrintString(47, y, "DEFI ");	break;
							case AT_BLENHIEM:	PrintString(47, y, "BLEN ");	break;
							case AT_JU88:		PrintString(47, y, "JU88 ");	break;
							case AT_HE111:		PrintString(47, y, "H111 ");	break;
							case AT_DO17:		PrintString(47, y, "DO17 ");	break;
							case AT_HE59:		PrintString(47, y, "HE59 ");	break;
						}
					
					switch(ac->ai.manoeuvre)
					{
						case MANOEUVRE_SELECT:				PrintString(52, y, "SELECT ");				break;
						case MANOEUVRE_LOOKROUND:			PrintString(52, y, "LOOKROUND ");			break;
						case MANOEUVRE_WELDEDWINGMAN:		PrintString(52, y, "WELDEDWINGMAN ");		break;
						case MANOEUVRE_BAILOUT:				PrintString(52, y, "BAILOUT ");				break;
						case MANOEUVRE_TOPCOVER:			PrintString(52, y, "TOPCOVER ");			break;
						case MANOEUVRE_LINEABREAST:			PrintString(52, y, "LINEABREAST ");			break;
						case MANOEUVRE_PINCER:				PrintString(52, y, "PINCER ");				break;
						case MANOEUVRE_MULTIWAVE:			PrintString(52, y, "MULTIWAVE ");			break;
						case MANOEUVRE_DIVEANDZOOM:			PrintString(52, y, "DIVEANDZOOM ");			break;
						case MANOEUVRE_LEADPURSUIT:			PrintString(52, y, "LEADPURSUIT ");			break;
						case MANOEUVRE_LAGPURSUIT:			PrintString(52, y, "LAGPURSUIT ");			break;
						case MANOEUVRE_SPLITMANOEUVRE:		PrintString(52, y, "SPLITMANOEUVRE ");		break;
						case MANOEUVRE_HEADON:				PrintString(52, y, "HEADON ");				break;
						case MANOEUVRE_LINEASTERN:			PrintString(52, y, "LINEASTERN ");			break;
						case MANOEUVRE_BARRELROLLATTACK:	PrintString(52, y, "BARRELROLLATTACK ");	break;
						case MANOEUVRE_SCISSORS:			PrintString(52, y, "SCISSORS ");			break;
						case MANOEUVRE_MILDSCISSORS:		PrintString(52, y, "MILDSCISSORS ");		break;
						case MANOEUVRE_TURNINGFIGHT:		PrintString(52, y, "TURNINGFIGHT ");		break;
						case MANOEUVRE_SPLITS:				PrintString(52, y, "SPLITS ");				break;
						case MANOEUVRE_ZOOMANDDROP:			PrintString(52, y, "ZOOMANDDROP ");			break;
						case MANOEUVRE_STRAIGHTANDLEVEL:	PrintString(52, y, "STRAIGHTANDLEVEL ");	break;
						case MANOEUVRE_SPINOUT:				PrintString(52, y, "SPINOUT ");				break;
						case MANOEUVRE_DIVEFORHOME:			PrintString(52, y, "DIVEFORHOME ");			break;
						case MANOEUVRE_GOHOME:				PrintString(52, y, "GOHOME ");				break;
						case MANOEUVRE_MAKEFORFRIENDLY:		PrintString(52, y, "MAKEFORFRIENDLY ");		break;
						case MANOEUVRE_MOVEAWAY:			PrintString(52, y, "MOVEAWAY ");			break;
						case MANOEUVRE_ACCIDENTALSPINOUT:	PrintString(52, y, "ACCIDENTALSPINOUT ");	break;
						case MANOEUVRE_VERTICALLOOP:		PrintString(52, y, "VERTICALLOOP ");		break;
						case MANOEUVRE_IMMELMANNTURN:		PrintString(52, y, "IMMELMANNTURN ");		break;
						case MANOEUVRE_IMMELMANN:			PrintString(52, y, "IMMELMANN ");			break;
						case MANOEUVRE_STAYWITHPREY:		PrintString(52, y, "STAYWITHPREY ");		break;
						case MANOEUVRE_CLIMBFORHOME:		PrintString(52, y, "CLIMBFORHOME ");		break;
						case MANOEUVRE_STRAIGHTDIVE:		PrintString(52, y, "STRAIGHTDIVE ");		break;
						case MANOEUVRE_STANDONTAIL:			PrintString(52, y, "STANDONTAIL ");			break;
						case MANOEUVRE_SHOOTTOFRIGHTEN:		PrintString(52, y, "SHOOTTOFRIGHTEN ");		break;
						case MANOEUVRE_SHOOTTOOEARLY:		PrintString(52, y, "SHOOTTOOEARLY ");		break;
						case MANOEUVRE_GAINHEIGHT:			PrintString(52, y, "GAINHEIGHT ");			break;
						case MANOEUVRE_LAGROLL:				PrintString(52, y, "LAGROLL ");				break;
						case MANOEUVRE_EXTENSION:			PrintString(52, y, "EXTENSION ");			break;
						case MANOEUVRE_DIVINGSPIN:			PrintString(52, y, "DIVINGSPIN ");			break;
						case MANOEUVRE_REVERSETURN:			PrintString(52, y, "REVERSETURN ");			break;
						case MANOEUVRE_SELFASBAIT:			PrintString(52, y, "SELFASBAIT ");			break;
						case MANOEUVRE_JINK:				PrintString(52, y, "JINK ");				break;
						case MANOEUVRE_BREAKTURN:			PrintString(52, y, "BREAKTURN ");			break;
						case MANOEUVRE_LAZYTURN:			PrintString(52, y, "LAZYTURN ");			break;
						case MANOEUVRE_BREAKLOW:			PrintString(52, y, "BREAKLOW ");			break;
						case MANOEUVRE_BREAKHIGH:			PrintString(52, y, "BREAKHIGH ");			break;
						case MANOEUVRE_BREAK90:				PrintString(52, y, "BREAK90 ");				break;
						case MANOEUVRE_HIGBARRELROLL:		PrintString(52, y, "HIGBARRELROLL ");		break;
						case MANOEUVRE_PANICTURN:			PrintString(52, y, "PANICTURN ");			break;
						case MANOEUVRE_UNBALANCEDFLIGHT:	PrintString(52, y, "UNBALANCEDFLIGHT ");	break;
						case MANOEUVRE_LOWALT:				PrintString(52, y, "LOWALT ");				break;
						case MANOEUVRE_TURNTOHDGANDPITCH:	PrintString(52, y, "TURNTOHDGANDPITCH ");	break;
						case MANOEUVRE_HIGHYOYO:			PrintString(52, y, "HIGHYOYO ");			break;
						case MANOEUVRE_ZOOM:				PrintString(52, y, "ZOOM ");				break;
						case MANOEUVRE_LOWYOYO:				PrintString(52, y, "LOWYOYO ");				break;
						case MANOEUVRE_INTERCEPTHIGH:		PrintString(52, y, "INTERCEPTHIGH ");		break;
						case MANOEUVRE_GAINSPEED:			PrintString(52, y, "GAINSPEED ");			break;
						case MANOEUVRE_HEADONATTACK:		PrintString(52, y, "HEADONATTACK ");		break;
						case MANOEUVRE_LUFBERRY:			PrintString(52, y, "LUFBERRY ");			break;
						case MANOEUVRE_STEEPDIVE:			PrintString(52, y, "STEEPDIVE ");			break;
						case MANOEUVRE_UPANDUNDER:			PrintString(52, y, "UPANDUNDER ");			break;
						case MANOEUVRE_STALLTURN:			PrintString(52, y, "STALLTURN ");			break;
						case MANOEUVRE_SLASHINGATTACK:		PrintString(52, y, "SLASHINGATTACK ");		break;
						case MANOEUVRE_CLIMBTURN:			PrintString(52, y, "CLIMBTURN ");			break;
						case MANOEUVRE_ROLL360:				PrintString(52, y, "ROLL360 ");				break;
						case MANOEUVRE_STRAFFE:				PrintString(52, y, "STRAFFE ");				break;
						case MANOEUVRE_FLYTHROUGHCLOUD:		PrintString(52, y, "FLYTHROUGHCLOUD ");		break;
						case MANOEUVRE_SPINRECOVERY:		PrintString(52, y, "SPINRECOVERY ");		break;
						case MANOEUVRE_COLLISIONAVOIDANCE:	PrintString(52, y, "COLLISIONAVOIDANCE ");	break;
						case MANOEUVRE_COLLIDE:				PrintString(52, y, "COLLIDE ");				break;
					}
				}
				y++;
			}
	}
*/			
/*
	if( (entry->Status.size==AIRSTRUCSIZE) && ((entry == Persons2::PlayerGhostAC) || (entry == Persons2::PlayerGhostAC)))
	{
		if(((!currworld->vp) || (!currworld->vp->Accel())) || (!(Persons2::PlayerSeenAC->fly.pModel->FrameCount & EVERY256cs)))
		switch(Persons2::PlayerGhostAC->movecode)
		{
			case AUTO_FOLLOWWP:         PrintString(60, 2, "G AUTO_FOLLOWWP  ");     break;
			case AUTO_NOPPILOT:         PrintString(60, 2, "G AUTO_NOPPILOT  ");     break;
			case AUTO_WAIT4TIME:        PrintString(60, 2, "G AUTO_WAIT4TIME  ");     break;
			case AUTO_TAXI:             PrintString(60, 2, "G AUTO_TAXI  ");     break;
			case AUTO_TAKEOFF:          PrintString(60, 2, "G AUTO_TAKEOFF  ");     break;
			case GROUND_TAXI:		    PrintString(60, 2, "G GROUND_TAXI  ");     break;
			case GROUND_CORNERUPTO:		    PrintString(60, 2, "G GROUND_TAXI  ");     break;
			case GROUND_CORNERTURN:		    PrintString(60, 2, "G GROUND_TAXI  ");     break;
			case AUTO_TELLLEADER:		PrintString(60, 2, "G AUTO_TELLLEADER  ");     break;
															  
			case AUTO_SPIRAL2GROUND:	PrintString(60, 2, "G AUTO_SPIRAL2GROUND  ");     break;
			case AUTO_RESURRECT:		PrintString(60, 2, "G AUTO_RESURRECT  ");     break;
			case AUTO_DEATHSEQUENCE:	PrintString(60, 2, "G AUTO_DEATHSEQUENCE  ");     break;
			case AUTO_TRAININGPREMERGE: PrintString(60, 2, "G AUTO_TRAININGPREMERGE  ");     break;
			case AUTO_TRAININGACM:	    PrintString(60, 2, "G AUTO_TRAININGACM  ");     break;
			case AUTO_CRASHSKID:		PrintString(60, 2, "G AUTO_CRASHSKID  ");     break;
			case AUTO_CRASHROLL:		PrintString(60, 2, "G AUTO_CRASHROLL  ");     break;
															  
			case AUTO_BOMB:             PrintString(60, 2, "G AUTO_BOMB  ");     break;
			case AUTO_COVER:            PrintString(60, 2, "G AUTO_COVER  ");     break;
			case AUTO_STRAFFE:          PrintString(60, 2, "G AUTO_STRAFFE  ");     break;
			case AUTO_BALLOON:          PrintString(60, 2, "G AUTO_BALLOON  ");     break;
			case AUTO_RECONN:           PrintString(60, 2, "G AUTO_RECONN  ");     break;
			case AUTO_ARTSPOT:          PrintString(60, 2, "G AUTO_ARTSPOT  ");     break;
			case AUTO_HITWATER:		    PrintString(60, 2, "G AUTO_HITWATER  ");     break;
															  
			case AUTO_TRACKINGBOGIE:	PrintString(60, 2, "G AUTO_TRACKINGBOGIE  ");     break;
			case AUTO_TRACKINGBANDIT:   PrintString(60, 2, "G AUTO_TRACKINGBANDIT  ");     break;
			case AUTO_PRECOMBAT:		PrintString(60, 2, "G AUTO_PRECOMBAT  ");     break;
			case AUTO_LANDING:          PrintString(60, 2, "G AUTO_LANDING  ");     break;
			case AUTO_FAC:              PrintString(60, 2, "G AUTO_FAC  ");     break;
			case AUTO_CAS:              PrintString(60, 2, "G AUTO_CAS  ");     break;
			case AUTO_ACCELERATE:       PrintString(60, 2, "G AUTO_ACCELERATE  ");     break;
			case AUTO_MISSIONDICETHROW: PrintString(60, 2, "G AUTO_MISSIONDICETHROW  ");     break;
			case AUTO_COMMSACCEL:	    PrintString(60, 2, "G AUTO_COMMSACCEL  ");     break;
			case GROUND_TOGLOBREF:      PrintString(60, 2, "G GROUND_TOGLOBREF  ");     break;
			case AUTO_COMBAT:			PrintString(60, 2, "G AUTO_COMBAT  ");     break;
		}
		switch(Persons2::PlayerSeenAC->movecode)
		{
			case AUTO_FOLLOWWP:         PrintString(60, 3, "S AUTO_FOLLOWWP  ");     break;
			case AUTO_NOPPILOT:         PrintString(60, 3, "S AUTO_NOPPILOT  ");     break;
			case AUTO_WAIT4TIME:        PrintString(60, 3, "S AUTO_WAIT4TIME  ");     break;
			case AUTO_TAXI:             PrintString(60, 3, "S AUTO_TAXI  ");     break;
			case AUTO_TAKEOFF:          PrintString(60, 3, "S AUTO_TAKEOFF  ");     break;
			case GROUND_TAXI:		    PrintString(60, 3, "S GROUND_TAXI  ");     break;
			case AUTO_TELLLEADER:		PrintString(60, 3, "S AUTO_TELLLEADER  ");     break;
															  
			case AUTO_SPIRAL2GROUND:	PrintString(60, 3, "S AUTO_SPIRAL2GROUND  ");     break;
			case AUTO_RESURRECT:		PrintString(60, 3, "S AUTO_RESURRECT  ");     break;
			case AUTO_DEATHSEQUENCE:	PrintString(60, 3, "S AUTO_DEATHSEQUENCE  ");     break;
			case AUTO_TRAININGPREMERGE: PrintString(60, 3, "S AUTO_TRAININGPREMERGE  ");     break;
			case AUTO_TRAININGACM:	    PrintString(60, 3, "S AUTO_TRAININGACM  ");     break;
			case AUTO_CRASHSKID:		PrintString(60, 3, "S AUTO_CRASHSKID  ");     break;
			case AUTO_CRASHROLL:		PrintString(60, 3, "S AUTO_CRASHROLL  ");     break;
															  
			case AUTO_BOMB:             PrintString(60, 3, "S AUTO_BOMB  ");     break;
			case AUTO_COVER:            PrintString(60, 3, "S AUTO_COVER  ");     break;
			case AUTO_STRAFFE:          PrintString(60, 3, "S AUTO_STRAFFE  ");     break;
			case AUTO_BALLOON:          PrintString(60, 3, "S AUTO_BALLOON  ");     break;
			case AUTO_RECONN:           PrintString(60, 3, "S AUTO_RECONN  ");     break;
			case AUTO_ARTSPOT:          PrintString(60, 3, "S AUTO_ARTSPOT  ");     break;
			case AUTO_HITWATER:		    PrintString(60, 3, "S AUTO_HITWATER  ");     break;
															  
			case AUTO_TRACKINGBOGIE:	PrintString(60, 3, "S AUTO_TRACKINGBOGIE  ");     break;
			case AUTO_TRACKINGBANDIT:   PrintString(60, 3, "S AUTO_TRACKINGBANDIT  ");     break;
			case AUTO_PRECOMBAT:		PrintString(60, 3, "S AUTO_PRECOMBAT  ");     break;
			case AUTO_LANDING:          PrintString(60, 3, "S AUTO_LANDING  ");     break;
			case AUTO_FAC:              PrintString(60, 3, "S AUTO_FAC  ");     break;
			case AUTO_CAS:              PrintString(60, 3, "S AUTO_CAS  ");     break;
			case AUTO_ACCELERATE:       PrintString(60, 3, "S AUTO_ACCELERATE  ");     break;
			case AUTO_MISSIONDICETHROW: PrintString(60, 3, "S AUTO_MISSIONDICETHROW  ");     break;
			case AUTO_COMMSACCEL:	    PrintString(60, 3, "S AUTO_COMMSACCEL  ");     break;
			case GROUND_TOGLOBREF:      PrintString(60, 3, "S GROUND_TOGLOBREF  ");     break;
			case AUTO_COMBAT:			PrintString(60, 3, "S AUTO_COMBAT  ");     break;
		}
	}
*/

/*
	if (entry->Status.size == AIRSTRUCSIZE)
	{
		AirStrucPtr currac = (AirStrucPtr)entry;

		SWord acnum = 4 * (currac->formpos & 0x0f) + (currac->formpos & 0xf0) / 16;
		SWord ufnum = -1;
		AirStrucPtr uf = AirStrucPtr(currac->ai.unfriendly);
		if(uf)
			ufnum = 4 * (uf->formpos & 0x0f) + (uf->formpos & 0xf0) / 16;

		if(currac->nationality == NAT_RAF)
		{
			PrintVar(40, acnum, "M %.0f ", FP(currac->movecode));
			PrintVar(45, acnum, "Unf %.0f ", FP(ufnum));
		}
		else
		{
			PrintVar(60, acnum, "M %.0f ", FP(currac->movecode));
			PrintVar(65, acnum, "Unf %.0f ", FP(ufnum));
		}
	}
*/
/*
		if((!currworld->vp) || (!currworld->vp->Accel()))
			if(!(MOD.FrameCount & EVERY256cs) && (currac->fly.callnum <= 25))
			{
				if(currac->PlayerInGroup())
				{
					if(!currac->Status.deadtime) 
						PrintVar(0, currac->fly.callnum, "%.0f ", FP(currac->ai.flyingskill));
					else
						PrintString(0, currac->fly.callnum, "**DEAD**");
				}
				else if(currac->vel)
				{
					if(currac->classtype->visible == SPIT)
						if(!currac->Status.deadtime) 
							PrintVar(10, currac->fly.callnum, "SPIT %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(10, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == F84)
						if(!currac->Status.deadtime) 
							PrintVar(20, currac->fly.callnum, "F84 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(20, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == F80)
						if(!currac->Status.deadtime) 
							PrintVar(30, currac->fly.callnum, "F80 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(30, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == F51)
						if(!currac->Status.deadtime) 
							PrintVar(40, currac->fly.callnum, "F51 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(40, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == B26)
						if(!currac->Status.deadtime) 
							PrintVar(50, currac->fly.callnum, "B26 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(50, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == B29)
						if(!currac->Status.deadtime) 
							PrintVar(60, currac->fly.callnum, "B29 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(60, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == MIG15)
						if(!currac->Status.deadtime) 
							PrintVar(70, currac->fly.callnum, "M15 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(70, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == CMIG15)
						if(!currac->Status.deadtime) 
							PrintVar(70, currac->fly.callnum, "M15 %.0f ", FP(currac->ai.flyingskill));
						else
							PrintString(70, currac->fly.callnum, "**DEAD**");

					if(currac->classtype->visible == B26)
						PrintVar(57, currac->fly.callnum, "%.0f ", FP(currac->vel * 0.1));
	
				}
			}

		}
*/
/*		if( ((!currworld->vp) || (!currworld->vp->Accel())) && (currac->fly.callnum <= 8))
			if(!(MOD.FrameCount & EVERY8cs) || (currac->Status.deadtime))
				if((currac->PlayerInGroup()) || (	(currac->ai.unfriendly)
												 && (currac->ai.unfriendly->Status.size == AIRSTRUCSIZE)
												 && (AirStrucPtr(currac->ai.unfriendly)->PlayerInGroup())) )
//DEADCODE RDH 23/06/99 		if  (		(		(currac->fly.callname == 3)		//hot shot f86 = 3
//DEADCODE RDH 23/06/99 						||	(currac->fly.callname == 5)		//hot shot mig		
//DEADCODE RDH 23/06/99 					)
//DEADCODE RDH 23/06/99 					&&	(currac->fly.callnum <= 8)
//DEADCODE RDH 23/06/99 			)
		{

			if (currac->ai.unfriendly)
				currac->InterceptandRange (&currac->ai.unfriendly->World);						  //RDH 13/06/99
			else
				Range = 0;
			int grouppos = currac->fly.callnum;
			int line = grouppos + 1;
			if((MOD.Type == AT_MIG15) || (MOD.Type == AT_MIG15BIS) || (MOD.Type == AT_YAK9))
				line += 9;

			if((MOD.Type == AT_MIG15) || (MOD.Type == AT_MIG15BIS) || (MOD.Type == AT_YAK9))
				PrintVar(0, line, "RED %.0f      ", FP(grouppos));
			else
				PrintVar(0, line, "BLU %.0f      ", FP(grouppos));

			if (currac->Status.deadtime )
			{
				PrintString(32, line, "DEAD");

			}else
			{

				if(currac->ai.unfriendly) PrintVar(77, line, "v%.0f", FP(AirStrucPtr(currac->ai.unfriendly)->fly.callnum));
				if(currac->ai.attacker) 
					PrintVar(73, line, "a%.0f", FP(AirStrucPtr(currac->ai.attacker)->fly.callnum));
				else
					PrintVar(73, line, "a%.0f", FP(0));

				PrintVar(10, line, "Vel %.1f      ", FP(currac->vel / 10.0));
				PrintVar(20, line, "Range %.0f      ", FP(currac->Range / 100.0));
				
				if(currac->ai.simpleacm) PrintString(32, line, "simple    ");
				else			 PrintString(32, line, "COMPLEX   ");

				switch (currac->movecode)
				{
					case AUTO_TRACKINGBOGIE:	{ PrintString(40, line, "Track Bogie						");    break; }
					case AUTO_PRECOMBAT:	{ PrintString(40, line, "Precombat						");    break; }
					case AUTO_COMBAT:
					{
						switch (currac->ai.manoeuvre)
						{
							case MANOEUVRE_SELECT:						{ PrintString(40, line, "SELECT						");    break; }
							case MANOEUVRE_LOOKROUND:					{ PrintString(40, line, "LOOKROUND					");    break; }
							case MANOEUVRE_WELDEDWINGMAN:				{ PrintString(40, line, "WELDEDWINGMAN				");    break; }
							case MANOEUVRE_BAILOUT:						{ PrintString(40, line, "BAILOUT					");    break; }
							case MANOEUVRE_TOPCOVER:					{ PrintString(40, line, "TOPCOVER					");    break; }
							case MANOEUVRE_LINEABREAST:					{ PrintString(40, line, "LINEABREAST				");    break; }
							case MANOEUVRE_SCATTER:						{ PrintString(40, line, "SCATTER					");    break; }
							case MANOEUVRE_PINCER:						{ PrintString(40, line, "PINCER						");    break; }
							case MANOEUVRE_MULTIWAVE:					{ PrintString(40, line, "MULTIWAVE					");    break; }
							case MANOEUVRE_DIVEANDZOOM:					{ PrintString(40, line, "DIVEANDZOOM				");    break; }
							case MANOEUVRE_LEADPURSUIT:					{ PrintString(40, line, "LEADPURSUIT				");    break; }
							case MANOEUVRE_LAGPURSUIT:					{ PrintString(40, line, "LAGPURSUIT					");    break; }
							case MANOEUVRE_CIRCUMVENT:					{ PrintString(40, line, "CIRCUMVENT					");    break; }
							case MANOEUVRE_ROUNDABOUT:					{ PrintString(40, line, "ROUNDABOUT					");    break; }
							case MANOEUVRE_SPLITMANOEUVRE:				{ PrintString(40, line, "SPLITMANOEUVRE				");    break; }
							case MANOEUVRE_HEADON:						{ PrintString(40, line, "HEADON						");    break; }
							case MANOEUVRE_LINEASTERN:					{ PrintString(40, line, "LINEASTERN					");    break; }
							case MANOEUVRE_BARRELROLLATTACK:			{ PrintString(40, line, "BARRELROLLATTACK			");    break; }
							case MANOEUVRE_SCISSORS:					{ PrintString(40, line, "SCISSORS					");    break; }
							case MANOEUVRE_MILDSCISSORS:				{ PrintString(40, line, "MILDSCISSORS				");    break; }
							case MANOEUVRE_TURNINGFIGHT:				{ PrintString(40, line, "TURNINGFIGHT				");    break; }
							case MANOEUVRE_SPLITS:						{ PrintString(40, line, "SPLITS						");    break; }
							case MANOEUVRE_ZOOMANDDROP:					{ PrintString(40, line, "ZOOMANDDROP				");    break; }
							case MANOEUVRE_STRAIGHTANDLEVEL:			{ PrintString(40, line, "STRAIGHTANDLEVEL			");    break; }
							case MANOEUVRE_SPINOUT:						{ PrintString(40, line, "SPINOUT					");    break; }
							case MANOEUVRE_DIVEFORHOME:					{ PrintString(40, line, "DIVEFORHOME				");    break; }
							case MANOEUVRE_GOHOME:						{ PrintString(40, line, "GOHOME						");    break; }
							case MANOEUVRE_MAKEFORFRIENDLY:				{ PrintString(40, line, "MAKEFORFRIENDLY			");    break; }
							case MANOEUVRE_MOVEAWAY:					{ PrintString(40, line, "MOVEAWAY					");    break; }
							case MANOEUVRE_ACCIDENTALSPINOUT:			{ PrintString(40, line, "ACCIDENTALSPINOUT			");    break; }
							case MANOEUVRE_VERTICALLOOP:				{ PrintString(40, line, "VERTICALLOOP				");    break; }
							case MANOEUVRE_IMMELMANNTURN:				{ PrintString(40, line, "IMMELMANNTURN				");    break; }
							case MANOEUVRE_HEADONOFFSET:				{ PrintString(40, line, "HEADONOFFSET				");    break; }
							case MANOEUVRE_IMMELMANN:					{ PrintString(40, line, "IMMELMANN					");    break; }
							case MANOEUVRE_STAYWITHPREY:				{ PrintString(40, line, "STAYWITHPREY				");    break; }
							case MANOEUVRE_CLIMBFORHOME:				{ PrintString(40, line, "CLIMBFORHOME				");    break; }
							case MANOEUVRE_STRAIGHTDIVE:				{ PrintString(40, line, "STRAIGHTDIVE				");    break; }
							case MANOEUVRE_STANDONTAIL:					{ PrintString(40, line, "STANDONTAIL				");    break; }
							case MANOEUVRE_SHOOTTOFRIGHTEN:				{ PrintString(40, line, "SHOOTTOFRIGHTEN			");    break; }
							case MANOEUVRE_SHOOTTOOEARLY:				{ PrintString(40, line, "SHOOTTOOEARLY				");    break; }
							case MANOEUVRE_GAINHEIGHT:					{ PrintString(40, line, "GAINHEIGHT					");    break; }
							case MANOEUVRE_LAGROLL:						{ PrintString(40, line, "LAGROLL					");    break; }
							case MANOEUVRE_EXTENSION:					{ PrintString(40, line, "EXTENSION					");    break; }
							case MANOEUVRE_DIVINGSPIN:					{ PrintString(40, line, "DIVINGSPIN					");    break; }
							case MANOEUVRE_REVERSETURN:					{ PrintString(40, line, "REVERSETURN				");    break; }
							case MANOEUVRE_SELFASBAIT:					{ PrintString(40, line, "SELFASBAIT					");    break; }
							case MANOEUVRE_JINK:						{ PrintString(40, line, "JINK						");    break; }
							case MANOEUVRE_BREAKTURN:					{ PrintString(40, line, "BREAKTURN					");    break; }
							case MANOEUVRE_LAZYTURN:					{ PrintString(40, line, "LAZYTURN					");    break; }
							case MANOEUVRE_BREAK180:					{ PrintString(40, line, "BREAK180					");    break; }
							case MANOEUVRE_BREAKLOW:					{ PrintString(40, line, "BREAKLOW					");    break; }
							case MANOEUVRE_BREAKHIGH:					{ PrintString(40, line, "BREAKHIGH					");    break; }
							case MANOEUVRE_BREAK90:						{ PrintString(40, line, "BREAK90					");    break; }
							case MANOEUVRE_HIGBARRELROLL:				{ PrintString(40, line, "HIGBARRELROLL				");    break; }
							case MANOEUVRE_PANICTURN:					{ PrintString(40, line, "PANICTURN					");    break; }
							case MANOEUVRE_UNBALANCEDFLIGHT:			{ PrintString(40, line, "UNBALANCEDFLIGHT			");    break; }
							case MANOEUVRE_LOWALT:						{ PrintString(40, line, "LOWALT						");    break; }
							case MANOEUVRE_SNAPSHOT:					{ PrintString(40, line, "SNAPSHOT					");    break; }
							case MANOEUVRE_STAYONTAIL:					{ PrintString(40, line, "STAYONTAIL					");    break; }
							case MANOEUVRE_TURNTOHDGANDPITCH:			{ PrintString(40, line, "TURNTOHDGANDPITCH			");    break; }
							case MANOEUVRE_HOTSIDELAGPURSUITFIGHT:		{ PrintString(40, line, "HOTSIDELAGPURSUITFIGHT		");    break; }
							case MANOEUVRE_SANDWICHMANOEUVRES:			{ PrintString(40, line, "SANDWICHMANOEUVRES			");    break; }
							case MANOEUVRE_SUSTAINEDTURN:				{ PrintString(40, line, "SUSTAINEDTURN				");    break; }
							case MANOEUVRE_HIGHYOYO:					{ PrintString(40, line, "HIGHYOYO					");    break; }
							case MANOEUVRE_ZOOM:						{ PrintString(40, line, "ZOOM						");    break; }
							case MANOEUVRE_LOWYOYO:						{ PrintString(40, line, "LOWYOYO					");    break; }
							case MANOEUVRE_INTERCEPTHIGH:				{ PrintString(40, line, "INTERCEPTHIGH				");    break; }
							case MANOEUVRE_CLIMBATSUSTAINEDTURNSPEED:	{ PrintString(40, line, "CLIMBATSUSTAINEDTURNSPEED	");    break; }
							case MANOEUVRE_GAINSPEED:					{ PrintString(40, line, "GAINSPEED					");    break; }
							case MANOEUVRE_SPINRECOVERY:				{ PrintString(40, line, "SPINRECOVERY				");    break; }
						}
					}
				}

				if (currac->ai.unfriendly)
				{
					COORDS3D trg = currac->ai.unfriendly->World;
					COORDS3D my  = currac->World;
					FP fAttack = (trg.X - my.X) * currac->velx + (trg.Y - my.Y) * currac->vely + (trg.Z - my.Z) * currac->velz;
					fAttack *= 100.0 / FP(currac->vel * Range);

					PrintVar(59, line, "*%.0f ", fAttack);  
				}

				PrintVar(64, line, "f%.0f ", FP(currac->ai.combatskill));
				PrintVar(68, line, "m%.0f ", FP(currac->ai.morale));  
			}
		}
	}
*/
#endif

/* TEST CODE CSB 01/06/99 */
/* TEST CODE CSB 01/06/99 */
/* TEST CODE CSB 01/06/99 */

#ifndef	NDEBUG
	//Someone has set the movecode for a dead ac!!!
//DeadCode CSB 13/07/99		if (	(entry->movecode != AUTO_NOPPILOT)				//RJS 27Jun99
//DeadCode CSB 13/07/99			&&	(entry->shape == PDEATH)			)
//DeadCode CSB 13/07/99			INT3;
#endif

#if defined(BOB_LINUX)
	/* S201: what are the aircraft actually DOING? Neither the quick mission nor the campaign
	   raid ever enters AUTO_COMBAT, and "no combat happened" has several very different causes
	   (no enemies spawned, enemies never intercept, the AI never switches mode). A histogram of
	   the movecodes in play answers it in one run instead of three guesses.
	   NB this sits BEFORE the switch: my first attempt put it between two `case` labels, after a
	   `break`, where it is unreachable -- and it produced a confident "zero" that was really
	   "never executed". BOB_TRACE_MOVECODE=1. */
	if (getenv("BOB_TRACE_MOVECODE")) {
		static long _mc[64]; static long _n = 0;
		_mc[((int)entry->movecode) & 63]++;
		if ((_n++ % 4000) == 0) {
			fprintf(stderr,"[movecode] after %ld dispatches:", _n);
			for (int _i = 0; _i < 64; _i++) if (_mc[_i]) fprintf(stderr," %d=%ld", _i, _mc[_i]);
			fprintf(stderr,"  (AUTO_COMBAT=%d AUTO_FOLLOWWP=%d)\n", (int)AUTO_COMBAT, (int)AUTO_FOLLOWWP);
			fflush(stderr);
		}
	}
#endif
	switch (entry->movecode)
	{
	case AUTO_TRACKINGBOGIE:
		((AirStrucPtr) *entry)->Move().TrackBogey();
		break;
	case AUTO_TRACKINGBANDIT:
		((AirStrucPtr) *entry)->Move().TrackBandit();
		break;
	case AUTO_PRECOMBAT:
		((AirStrucPtr) *entry)->Move().AutoPreCombat();
		break;
	case AUTO_FOLLOWWP:												  //RDH 13/06/99
	case AUTO_TELLLEADER:										//JIM 03Oct96
		((AirStrucPtr) *entry)->Move().AutoFollowWp();
		break;
	case AUTO_TRAININGTAKEOFF:
		entry->movecode = AUTO_TAKEOFF;
		((AirStrucPtr) *entry)->Move().AutoTakeOff();
		break;
	case AUTO_TRAININGLANDING:
		entry->movecode = AUTO_LANDING;
		((AirStrucPtr) *entry)->Move().AutoLanding();
		break;
	case AUTO_TRAININGFORMATION:
		entry->movecode = AUTO_FOLLOWWP;
		((AirStrucPtr) *entry)->Move().AutoFollowWp();
		break;
//DEADCODE JIM 16/11/99 	case AUTO_FAC:
//DEADCODE JIM 16/11/99 		((AirStrucPtr) *entry)->AutoFAC();
//DEADCODE JIM 16/11/99 		break;
//DEADCODE JIM 16/11/99 	case AUTO_CAS:
//DEADCODE JIM 16/11/99 		((AirStrucPtr) *entry)->AutoCAS();
//DEADCODE JIM 16/11/99 		break;
//rdh 5/1/99	case AUTO_TRAININGPREMERGE:									//RDH 09Sep96
//rdh 5/1/99		((AirStrucPtr) *entry)->AutoTrainingPreMerge();			//RDH 09Sep96
//rdh 5/1/99		break;
//rdh 5/1/99	case AUTO_TRAININGACM:										//RDH 09Sep96
//rdh 5/1/99		((AirStrucPtr) *entry)->AutoTrainingACM();				//RDH 09Sep96
//rdh 5/1/99		break;													//RDH 09Sep96
	case AUTO_FOLLOWTHELEADER:
		((AirStrucPtr)*entry)->Move().AutoFollowTheLeader();			  //CSB 03/02/00
		break;
	case AUTO_CRASHROLL:										//RDH 12Sep96
		((AirStrucPtr) *entry)->Move().AutoCrashRoll();						//RDH 12Sep96
		break;
	case AUTO_CRASHSKID:										//RDH 12Sep96
		((AirStrucPtr) *entry)->Move().AutoCrashSkid();				//RDH 12Sep96
		break;
	case AUTO_CRASHONEWHEEL:
		((AirStrucPtr) *entry)->Move().AutoCrashOneWheel();	
		break;
	case AUTO_CRASHNOSE:
		((AirStrucPtr) *entry)->Move().AutoCrashNose();	
		break;
	case AUTO_CRASHFLIP:
		((AirStrucPtr) *entry)->Move().AutoCrashFlip();	
		break;
	case AUTO_CRASHTUMBLE:
		((AirStrucPtr) *entry)->Move().AutoCrashTumble();	
		break;
	case AUTO_WAIT4TIME:												//CSB 6Jul00
		((AirStrucPtr) *entry)->Move().AutoWait4Time();					//CSB 6Jul00
		break;															//CSB 6Jul00
	case AUTO_TAKEOFF:
		((AirStrucPtr) *entry)->Move().AutoTakeOff();
		break;
	case AUTO_COMBAT:
#if defined(BOB_LINUX)
		/* S201: the combat-soak metric. `DefenceManoeuvre` (S200) additionally needs an enemy
		   inside DANGERRANGE, so counting it answers "did anyone get shot at" -- too narrow to
		   tell whether the AI ran at all. THIS is the gate: an aircraft in AUTO_COMBAT is
		   running the whole ACM decision tree every tick. BOB_TRACE_ACM=1. */
		if (getenv("BOB_TRACE_ACM")) {
			static long _ac = 0;
			if ((_ac++ % 200) == 0) { fprintf(stderr,"[acm] AirCombat ticks=%ld\n", _ac); fflush(stderr); }
		}
#endif
		((AirStrucPtr) *entry)->AirCombat();
		break;
	case AUTO_NOPPILOT:
//DeadCode AMM 01Dec98 		if (entry=Persons2::PlayerSeenAC)

		
//DEADCODE RDH 28/04/99 		if (entry->uniqueID.count>RedAcBAND && entry->uniqueID.count<BlueAcBANDEND)
//DEADCODE RDH 28/04/99 			UByteP(0xB0000)[2*entry->uniqueID.count&0xfff]='N';
//DeadCode JIM 28Apr99 			UByteP(0xB0000)[2*entry->uniqueID.count&0xfff]='N';

		if (!_Replay.Playback)									//RJS 10Aug00
			entry->vel_ = entry->vel_x = entry->vel_y = entry->vel_z = 0;					//RJS 10Aug00

		if (entry==Persons2::PlayerSeenAC)						//AMM 01Dec98
		{
			_DPlay.ResurrectMe(Persons2::PlayerSeenAC,false);
		}
		break;
	case GROUND_TAXI:
		((FormationItemPtr) *entry)->AutoTaxi();
	break;
	case GROUND_CORNERUPTO:
		((FormationItemPtr) *entry)->AutoTaxiCornerUpTo();
	break;
	case GROUND_CORNERTURN:
		((FormationItemPtr) *entry)->AutoTaxiCornerTurn();
	break;

	case GROUND_TOGLOBREF:
		((FormationItemPtr) *entry)->WaitToTaxi();
		break;
//	case AUTO_TAXI:
//		((AirStrucPtr) *entry)->AutoTaxi();
//		break;
	case AUTO_SPIRAL2GROUND:
		((AirStrucPtr) *entry)->Move().AutoSpiral2Ground(world);		//PD 01May96
		break;
	case AUTO_RESURRECT:
		((AirStrucPtr) *entry)->Move().AutoResurrect((AirStrucPtr)entry);			//ARM 05Dec96
		break;
	case AUTO_DEATHSEQUENCE:
		((AirStrucPtr) *entry)->Move().AutoDeathSequence();			//RDH 10Sep96
		break;
	case AUTO_DEATHGLIDE:
		((AirStrucPtr) *entry)->Move().AutoDeathGlide();
		break;
	case AUTO_LANDING:											//JIM 02Aug96
		((AirStrucPtr) *entry)->Move().AutoLanding();				//JIM 02Aug96
		break;													//JIM 02Aug96
	case AUTO_STRAFFE:
		((AirStrucPtr) *entry)->Move().AutoStraffe();				//JIM 02Aug96
		break;													//JIM 02Aug96
	case AUTO_BOMB:
		((AirStrucPtr) *entry)->Move().AutoBomb();				//JIM 02Aug96
		break;													//JIM 02Aug96
	case AUTO_HITWATER:											//RJS 24Aug98
		((AirStrucPtr) *entry)->Move().AutoCrashSkid();				//RJS 24Aug98
		break;													//RJS 24Aug98
	case AUTO_ACCELERATE:
		((AirStrucPtr) *entry)->Move().AutoAccelerate();
		break;
	case AUTO_MISSIONDICETHROW:
		((AirStrucPtr) *entry)->Move().MoveMissionDiceThrow();
		break;
	case AUTO_COMMSACCEL:
		((AirStrucPtr) *entry)->Move().AutoCommsAccel();
		break;
	case GROUND_BLOKE:											//RJS 10Jan00
	case GROUND_BLOKEFREAK:											//RJS 10Jan00
		((FormationItemPtr) *entry)->AutoBloke();
		break;
	case GROUND_EMERGENCY:
		((FormationItemPtr) *entry)->AutoEmergencyVehicle();
		break;
	case GROUND_HALTTRUCK:
		((FormationItemPtr) *entry)->AutoSlowTaxi();
		break;
	}
//DEADCODE JIM 31/05/00 #ifndef	NDEBUG
//DEADCODE JIM 31/05/00 	if(!Count_SetFlightParams)
//DEADCODE JIM 31/05/00 		int breakpoint = 0;//INT3;
//DEADCODE JIM 31/05/00 	Count_SetFlightParams=0;
//DEADCODE JIM 31/05/00 #endif
	if(entry->Status.size == AIRSTRUCSIZE)
	{
		AirStrucPtr ac = AirStrucPtr(entry);
		ac->weap.ShootDelay = ac->weap.ShootDelay - Timer_Code.FRAMETIME;
		if(ac->weap.ShootDelay < 0)
			ac->weap.ShootDelay = 0;
	}
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveList
//LastModified:	PD 26Apr96
//Author		Jim Taylor
//Date			Thu 15 Feb 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
extern void	CheckE3();
void TransientItem::MoveTransList(TransientItemPtr entry,WorldStuff& world)
{
	currworld=&world;
	while (entry)
	{
		TransientItemPtr	next= entry->nexttrans;
		if (entry->movecode != MOBILE_NOPPILOT)						//RJS 09Dec98
		{
			UWord	oldsector=	world.GetSector(*entry);			//PD 25Jun96
	
			MoveTransItem(entry,world);
	
			if (oldsector!=world.GetSector(*entry))					//PD 25Jun96
			{
				world.RemoveFromSector((TransientItemPtr)entry,oldsector);
				world.AddToWorld((TransientItemPtr)entry);
			}
		}

		entry=next;
	}

// Update smoke clouds...
	Land_Scape.ManageSmokeClouds();								//RJS 24May00
}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		MoveList
//Author		Jim Taylor
//Date			Thu 15 Feb 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void TransientItem::MoveTransItem(TransientItemPtr entry,WorldStuff& world)//PD 26Apr96
{
/*#ifdef	DBGMEMTEST
#ifndef	NDEBUG
	TransientItem*	tmpentry = TransientList;
	while (tmpentry)
	{
		DbgMemTest2(tmpentry);
		tmpentry->Anim.MemTest();
		tmpentry = tmpentry->nexttrans;
	}
#endif
#endif*/
	switch (entry->movecode)// & AutoMoveCodeMask)					//RJS 24Nov97
	{
  	case	MOBILE_BURNIT:			Trans_Obj.MobileExplosion(entry,world);	break;//PD 26Apr96
 	case	MOBILE_CRATER:			Trans_Obj.MobileCrater(entry,world);	break;//PD 26Apr96
	case	MOBILE_BULLET:			Trans_Obj.MobileBullet(entry,world); 	break;//PD 26Apr96
	case    MOBILE_DEADACPART:		Trans_Obj.MobileDeadACPart(entry,world);break;//PD 26Apr96
	case 	MOBILE_SMOKETRAIL:		Trans_Obj.MobileSmokeTrail(entry,world);break;//PD 30Apr96
	case 	MOBILE_GANDF:			Trans_Obj.MobileGAndF(entry,world);		break;//PD 30Apr96
	case 	MOBILE_GROUNDEXP:		Trans_Obj.MobileGroundExp(entry,world);	break;//PD 01May96
	case 	MOBILE_DEBRIS:			Trans_Obj.MobileGroundDebris(entry,world);	break;//RJS 21Jun96
	case 	MOBILE_FIRE:			Trans_Obj.MobileFire(entry,world);	break;//RJS 01Jul96
	case 	MOBILE_TROOP:			Trans_Obj.MobileTroop(entry,world);	break;//RJS 01Jul96
	case 	MOBILE_PARACHUTE:		Trans_Obj.MobileParachute(entry,world);	break;//RJS 02Jul96
	case 	MOBILE_BARAGE:			Trans_Obj.MobileBarage(entry,world);	break;//RJS 02Jul96
	case 	MOBILE_DELEXP:			Trans_Obj.MobileDelayedExplosion(entry,world);	break;//RJS 02Jul96
	case 	MOBILE_FLAK:			Trans_Obj.MobileFlak(entry,world);	break;//RJS 02Jul96
	case 	MOBILE_RICOCHET:		Trans_Obj.MobileSparkTrail(entry,world);	break;//RJS 11Dec98
	case 	MOBILE_ATEAM:			Trans_Obj.MobileATeam(entry,world);	break;//RJS 10Jul96
	case 	MOBILE_BOMBDROP:		Trans_Obj.MobileBombDrop(entry,world);	break;//RJS 24Jul96
	case 	MOBILE_BIGEXP:			Trans_Obj.MobileBigExplosion(entry,world);	break;//RJS 12Aug96
	case 	MOBILE_MOVSMK:			Trans_Obj.MobileMovingSmoke(entry,world);	break;//RJS 02Sep96
	case 	MOBILE_STATIONARY:		Trans_Obj.MobileRicochetSmoke(entry,world);	break;//RJS 05Sep96
	case 	MOBILE_PUFFTRAVEL:		Trans_Obj.MobilePuffTravel(entry,world);	break;//RJS 13Oct96
	case 	MOBILE_BIRD:			Trans_Obj.MobileBird(entry,world);	break;//RJS 01Oct97
	case 	MOBILE_AMMODEATH:		Trans_Obj.MobileAmmoBoxes(entry,world);	break;//RJS 17Oct96
	case 	MOBILE_MUSHROOM:		Trans_Obj.MobileDummyBomb(entry,world);	break;//RJS 01Apr99
	case 	MOBILE_ROCKET:			Trans_Obj.MobileRocket(entry,world);	break;//RJS 04Jun97
	case 	MOBILE_CLONEGUN:		Trans_Obj.MobileCloneGun(entry,world);	break;//RJS 01Oct97
	case 	MOBILE_TIMEREXPLOSION:	Trans_Obj.MobileTimerExplosion(entry,world);	break;//RJS 22Jan98
	case 	MOBILE_NAPALM:			Trans_Obj.MobileNapalm(entry,world);	break;//RJS 22Jan98
	case 	MOBILE_DODAMAGE:		Trans_Obj.MobileDoDamage(entry,world);	break;//RJS 18Feb98
	case 	MOBILE_THUG:			Trans_Obj.MobileThuggery(entry,world);	break;//RJS 02Mar98
	case 	MOBILE_SMALLTHUG:		Trans_Obj.MobileSmallThug(entry,world);	break;//RJS 03Dec98
	case 	MOBILE_FUELDROP:		Trans_Obj.MobileFuelDrop(entry,world);	break;//RJS 28May98
	case 	MOBILE_DEADBRIDGE:		Trans_Obj.MobileDeadBridge(entry,world);	break;//RJS 30Oct98
	case 	MOBILE_COLLAPSE:		Trans_Obj.MobileCollapse(entry,world);	break;//RJS 18Nov98
//DeadCode RJS 15Dec99 	case 	MOBILE_CARPETBOMB:		Trans_Obj.MobileCarpetStrike(entry,world);	break;//RJS 31May99
	case 	MOBILE_SCALEFIST:		Trans_Obj.MobileScaleFist(entry,world);	break;//RJS 10Feb99
	case 	MOBILE_CANOPY:			Trans_Obj.MobileCanopy(entry,world);	break;//RJS 23Mar99
	case 	MOBILE_SINK:			Trans_Obj.MobileSink(entry,world);	break;//RJS 13May99
	case 	MOBILE_FLARE:			Trans_Obj.MobileFlare(entry,world);	break;//RJS 17May99
	case 	MOBILE_SUPERBOMB:		Trans_Obj.MobileSuperBomb(entry,world);	break;//RJS 23Nov99
	case 	MOBILE_DEADAC:			Trans_Obj.MobileDeadAC(entry,world);	break;//RJS 25Nov99
	case 	MOBILE_FUELFIRE:		Trans_Obj.MobileFuelFire(entry,world);	break;//RJS 13Dec99
	case 	MOBILE_FIRESMOKE:		Trans_Obj.MobileSmokeBlob(entry,world);	break;//RJS 14Dec99
	case 	MOBILE_DEADBALLOON:		Trans_Obj.MobileDeadBalloon(entry,world);break;//RJS 15Dec99
	case 	MOBILE_PAC:				Trans_Obj.MobilePAC(entry,world);break;//RJS 03Feb00
	case	MOBILE_ENGINESTART:		Trans_Obj.MobileEngineStart(entry,world);break;//RJS 22Feb00
	case	MOBILE_BOMB1STFRAME:	Trans_Obj.MobileBomb1stFrame(entry,world);break;	//RJS 3Oct00

	}
/*#ifdef	DBGMEMTEST
#ifndef	NDEBUG
	tmpentry = TransientList;
	while (tmpentry)
	{
		DbgMemTest2(tmpentry);
		tmpentry->Anim.MemTest();
		tmpentry = tmpentry->nexttrans;
	}
#endif
#endif*/
}

//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		AddToList
//Author		Jim Taylor
//Date			Thu 15 Feb 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void AirStruc::AddToList()
{
	if (uniqueID.count<SagBANDEND)
	{
		nextmobile=SAGList;
		SAGList=*this;
		Art_Int.Add(this);
	}
	else
	{
		nextmobile=ACList;
		ACList=*this;
		if(		(!formpos && movecode != AUTO_SPAREAC)							//JON 22/02/00
			&&	((this != Persons2::PlayerSeenAC) || (!Persons2::PlayerSeenAC->fly.expandedsag))	)	//CSB 27Jul00
			Art_Int.Add(this);
	}
}

void mobileitem::AddToList()
{
	nextmobile=MobileList;
	MobileList=*this;
}

void TransientItem::AddToList()
{
	nexttrans=TransientList;
	TransientList=this;

// transients have ID equal to frame when launched. When a collision occurs the player 
// who processes the collision (owner of transient) send a message indicating that all
// transients with that frame number launched by him should be removed from the world

	this->uniqueID.count=(UniqueID)_DPlay.FrameCount;

// if this was launched by a transient then it inherits its parents flags

	if (Launcher->Status.size==TRANSIENTSIZE)
	{
		this->CollTestEnabled=((TransientItemPtr)Launcher)->CollTestEnabled;
		this->CollSuccessTrans=((TransientItemPtr)Launcher)->CollSuccessTrans;
	}
	else
	if ((Launcher==Persons2::PlayerSeenAC && !_Replay.Playback)
	|| (!_DPlay.Implemented && !_Replay.Record && !_Replay.Playback))
	{
// comms/replay off or player launched 
// if replay is turned on then these bullets will cause collisions to be recorded
// even though bullets are not seen on replay

		this->CollTestEnabled=TRUE;
		this->CollSuccessTrans=TRUE;
	}
	else if (!Launcher->uniqueID.commsmove && Launcher!=Persons2::PlayerSeenAC)
	{
// AI launched - want to test all AI bullets against all other AI planes and against player
// unless in playback, which is dealt with in PossTransmitColl

		this->CollTestEnabled=TRUE;

// if this hits player then collision will be sent if in record or comms. Other collisions do
// not need to be sent as they are AI against AI, or AI against comms player in which case
// that player will send his own collision info packets. 

// if this hits a player then check CommsOwner ID. If it is me then trasmit, otherwise ignore.

		this->CollSuccessTrans=FALSE;
	}
	else
	{
// comms received - dont test for anything just show them. All player collisions are dealt with
// via separate packet info

		this->CollTestEnabled=FALSE;
		this->CollSuccessTrans=FALSE;
	}

//DeadCode JIM 07Oct96  	TransientItemPtr temp = new TransientItem;
//DeadCode JIM 07Oct96   	if (TransientList==NULL)
//DeadCode JIM 07Oct96   	{
//DeadCode JIM 07Oct96   		TransientList=*this;
//DeadCode JIM 07Oct96   	}
//DeadCode JIM 07Oct96   	else
//DeadCode JIM 07Oct96   	{
//DeadCode JIM 07Oct96  		temp=TransientList;
//DeadCode JIM 07Oct96  		TransientList=*this;
//DeadCode JIM 07Oct96  
//DeadCode JIM 07Oct96  		TransientList->nexttrans=temp;
//DeadCode JIM 07Oct96   	}

}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		GetWpName
//Author		Jim Taylor
//Date			Thu 5 Dec 1996
//
//Description	
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
//TextSnip0	WayPoint::GetWpName()
//{
//	TextSnip0	retval=TEXT_NULL;
//	if (action!=wpacnoactionno)// && action<16)
//		retval=(_Text.ConvSnip0((TextList)LIST_WPACTIONS0,action));
//	elseif (target)
//		retval=(_Text.ConvSnip0(target->uniqueID.count));
//	if (retval==TEXT_NULL)
//		retval=(TEXT_WPACNOACTIONNO);
//	return(retval);
//}
//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		ExecuteWayPointAction
//Author		R. Hyde 
//Date			Mon 19 Feb 1996
//
//Description	Now pseudo recursive. Compiler should optimise to itteration.
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	mobileitem::ExecuteWayPointAction ()
{
//TEMP	Persons_2.PossibleEvent(currworld,waypoint,this);
	if (	Status.size==AIRSTRUCSIZE 
		&&	AirStrucPtr(*this)->fly.numinsag>1)
	{
		AirStrucPtr(*this)->SAG().SAGWPAction();
		return;
	}

	AutoMoveCodeTypeSelect	oldmove=movecode;
	WayPointPtr	oldwp=waypoint;
//DeadCode JIM 18Oct96 	if (oldwp)
	//NULL WP MEANS START TO LAND...
	if (waypoint)
		DoWayPointAction(oldmove,oldwp,waypoint->action);
	else
		DoWayPointAction(oldmove,oldwp,wpforcelanding);
	if (Status.size==AIRSTRUCSIZE)
	{
		AirStrucPtr a=*this;
		if((oldwp->wpname == WPNAME_IP) && (a->follower))
			_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_INITIALPOINT, MSG_FACCONTACT_X, a, NULL, a->Follower()));
		if (	a->ai.eventlog->stage==EventLog::TAKEOFF
			&&	waypoint && waypoint->target
			)
		{
			if	(	waypoint->action==wpbombno
				||	waypoint->action==wpengagetargetno
				||	waypoint->action==wpdivebombstraffeno
//				||	waypoint->action==wpstraffeno
				||	waypoint->action==wpballoonbustno
				)
			{
//TEMP				Persons_2.AddMessage(UserMsg::SIGHTED,TEXT_OBJECTIVESIGHTED,waypoint->target);
				_Miles.SequenceAudible(FIL_MUSIC_OBJECTIVE_SIGHTED);
			}
		}
	}
}

void	mobileitem::DoWayPointAction (AutoMoveCodeTypeSelect	oldmove,WayPointPtr	oldwp,BFieldWaypointActions action)
{
	if(Status.size == AIRSTRUCSIZE)										//CSB 22Aug00
	{
		AirStrucPtr a = AirStrucPtr(this);
		a->information = IF_OUT_POS;					//CSB 22Aug00
		a->manoeuvretime = 0;

		if(Manual_Pilot.controlmode == ManualPilot::AUTOACCEL_WAYPT)
			if((a == Persons2::PlayerSeenAC) || (a == Persons2::PlayerGhostAC))
				OverLay.CancelAccel();

		if((Manual_Pilot.controlmode == ManualPilot::AUTOACCEL_MISS) && (oldwp->wpname == WPNAME_IP))
			if((a == Persons2::PlayerSeenAC) || (a == Persons2::PlayerGhostAC))
				OverLay.CancelAccel();
	}

	if (movecode==oldmove && waypoint==oldwp)
	{
		if (waypoint)
		{
			if (!waypoint->Status.deadtime && this==Manual_Pilot.ControlledAC2)
			{
//				UserMsg::Reason	r=UserMsg::PASSEDWP;
//DEADCODE JIM 25/03/99 				TextSnip0	t=TEXT_NULL;
//				t=waypoint->GetWpName();
//TEMP				Persons_2.AddMessage(r,t,*FindDesPos ());
//DeadCode RDH 15Oct96 				Persons_2.ClrMessage(UserMsg::COURSE);
			}
		}


		switch (action)
		{
		case	wpatakeoffno:
		case	wpacircleno:
		case	wptruckbridge:
		case	wptruckescort:
		case	wpengagetargetno:	//Used for interceptions!			//CSB 18Jul00
				NextWayPoint ();
		break;
//
//	PROPER WAYPOINT ACTIONS START HERE:
//	the ones above fall through to noaction
//
		case	wpacnoactionno:
			{
				NextWayPoint ();
				if(Status.size != AIRSTRUCSIZE)
					break;

				if(AirStrucPtr(this)->formpos)
					break;

				AirStrucPtr AC = *this;
				AirStrucPtr callee = AC->PlayerInGroup();
				if((callee) && (!AC->AcIsPlayer()))
				{
					AirStrucPtr AC = *this;
					AirStrucPtr callee = AC->PlayerInGroup();
					if((callee) && (callee != AC))
					{
						AC->InterceptandRange(AC->waypoint);
						SWord dhdg = SWord(AC->hdg) - SWord(AC->HdgIntercept);
						bool sentmessage = false;
						if(((AC->formation & FORMTYPE_WING) == FTW_SCWARM) || ((AC->formation & FORMTYPE_WING) == FTW_BSCRWARM))
						{
								if(dhdg > ANGLES_45Deg)
								{
									_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_TURNRIGHT, MSG_NEW_HEADING, AC, NULL, callee));
									sentmessage = true;
								}
								else if(-dhdg > ANGLES_45Deg)
								{
									_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_TURNLEFT, MSG_NEW_HEADING, AC, NULL, callee));
									sentmessage = true;
								}
								if(sentmessage)
									for(AirStrucPtr nf = AC; nf; nf = nf->fly.nextflight)
										for(AirStrucPtr f = nf; f; f = f->Follower())
										{
											if((f->formation & FORMTYPE_WING) == FTW_SCWARM)
												f->formation = (f->formation % FORMTYPE_WING) | FTW_BSCRWARM;
											else
												f->formation = (f->formation % FORMTYPE_WING) | FTW_SCWARM;
											f->information = IF_OUT_POS;
//DeadCode CSB 31Oct00 											f->formation = f->formation | FTI_SCWARM_SWAPPING_FLAG;
										}
						}
						if(!sentmessage)
							if((dhdg > ANGLES_20Deg) || (-dhdg > ANGLES_20Deg))
							{
								_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_TURNTONEWWAYPOINT, MSG_NEW_HEADING, AC, waypoint, callee));
								if(waypoint->World.Y <= World.Y)
									_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_LINER, MSG_NEW_SPEED, AC, waypoint, callee));
							}
					}
				}
				break;
			}
		case	wpdmbtrknoactno:
				NextWayPoint ();
				movecode=GROUND_CORNERTURN;
				action=wpretrywp;
		break;
		case	wpcheckfordamage:								//RJS 11Jan0
				movecode=GROUND_CORNERTURN;
				action=wpretrywp;
				EmergencyService();
		break;
		case	wpretrywp:
		//Truck followers come here
		if (movecode==GROUND_TAXI)
			movecode=GROUND_CORNERUPTO;
		break;												//RDH 17Oct96

		case	wpballoonbustno:											//MS 18Jul96
		case	wpdivebombstraffeno:										//MS 18Jul96
//		case	wpstraffeno:												//MS 18Jul96
			StartStraffe();
		break;

		case	wpbombno:													//MS 18Jul96
//DeadCode JIM 18Jul00 		case	wpengagetargetno:	//USED FOR INTERCEPTIONS!
		case	wpattackconvoyno:
			StartBombing();
		break;

		case	wpreccyno:										//JIM 17Dec96
 		case	wpartspotno:									//JIM 17Dec96
				KillItNow();
		break;

		case	wpteleportnext:
			TeleportWayPoint();
		break;

		case	wpstoploopchkno :							//JIM 30Jul96
			action=EndLoopWayPoint();								//JIM 30Jul96
		break;												//JIM 30Jul96

		case	wpbreakloop :							//JIM 30Jul96 //JIM 19/03/99
		case	wpstartloopno:										  //JIM 19/03/99
			action=MidLoopWayPoint();								//JIM 30Jul96
		break;												//JIM 30Jul96
		
		case	wpforcebreakloop:
			ForceBreakLoopWayPoint();
		break;

		case	wpwaituntildeadno :							//JIM 30Jul96
			WaitDeadWayPoint();								//JIM 30Jul96
		break;												//JIM 02Aug96
		case	wpalandingno:								//JIM 02Aug96
			LandingWayPoint();								//JIM 02Aug96
		break;												//JIM 02Aug96
		case	wpdisperseno:
			FormationItemPtr(*this)->DisperseWayPoint();
		break;
		case	WP_fighterlimit:
			FormationItemPtr(*this)->NextWayPoint();					//CSB 25Jul00
			FormationItemPtr(*this)->FighterLimitWayPoint();
		break;


		case	wpdmbtrkstopno:
		case	wpstopno:									//RDH 17Oct96
			vel_x=vel_y=vel_z=0;
			movecode=AUTO_NOPPILOT;

		break;
		case wpforcelanding:
		{
			AirStrucPtr ac = AirStrucPtr(*this);
			if((nationality == NAT_RAF) && (ac->classtype->aerobaticfactor == AEROBATIC_HIGH))
			{
				if(		(waypoint->target->World.X != waypoint->World.X)
					||	(waypoint->target->World.Y != waypoint->World.Y)
					||	(waypoint->target->World.Z != waypoint->World.Z)	)
				{
					SLong totfuel  = 0;
					SLong fuelleft = 0;
					for(SWord i = 0; i < 4; i++)
					{
						totfuel  += ac->classtype->fueltankcap[i];
						fuelleft += ac->fly.fuel_content[i] / 1000;
					}
					if(fuelleft < totfuel / 4)
						waypoint->target->World = waypoint->World;
					else
					{
						const COORDS3D offsetE = { 1000000, 0, -1000000};
						const COORDS3D offsetW = {-1000000, 0, -1000000};
						if(		(waypoint->target->World.X == waypoint->World.X + offsetE.X)
							&&	(waypoint->target->World.Y == waypoint->World.Y + offsetE.Y)
							&&	(waypoint->target->World.Z == waypoint->World.Z + offsetE.Z)	)
						{
							waypoint->target->World.X = waypoint->World.X + offsetW.X;
							waypoint->target->World.Y = waypoint->World.Y + offsetW.Y;
							waypoint->target->World.Z = waypoint->World.Z + offsetW.Z;
						}
						else
						{
							waypoint->target->World.X = waypoint->World.X + offsetE.X;
							waypoint->target->World.Y = waypoint->World.Y + offsetE.Y;
							waypoint->target->World.Z = waypoint->World.Z + offsetE.Z;
						}
						break;
					}
				}
			}
			
//DeadCode CSB 5Oct00 			if((AC == Persons2::PlayerGhostAC) || (AC == Persons2::PlayerSeenAC))
//DeadCode CSB 5Oct00 				OverLay.CancelAccel();
			movecode = AUTO_LANDING;
			if (Anim!=NULL)
				SHAPE.NavigationLightsActive(this, TRUE);
			AirStrucPtr AC = *this;
			AC->ai.ManStep = 0;
			AC->SetManoeuvreTime(0);
			
			if(AC->formpos == 0)
			{
				AirStrucPtr ThePlayer = AC->PlayerInGroup();
				if(/*(ThePlayer) &&*/ (AC != ThePlayer))
				{
					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GIMMEAVECTOR, MSG_STATUSRESPONSE_DELAY_LP_RPT, AC, AC->ai.homebase, AC));

					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CALLEE_STEER_RANGE, MSG_STATUSRESPONSE_DELAY_LP_RPT, VOICE_TOWER, AC->ai.homebase, AC));

					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_SURFACEWINDREQUEST, MSG_WIND_REQUEST, AC, AC->ai.homebase, AC));

					ScriptTables windrep = SCRIPT_NOWIND;
			 		if(Save_Data.flightdifficulty [FD_WINDEFFECTS])
						if(Save_Data.flightdifficulty [FD_WINDGUSTS])
							windrep = SCRIPT_CALLEE_SURFACEWIND_BEARINGNO_SPEED_GUSTS;
						else
							windrep = SCRIPT_CALLEE_SURFACEWIND_BEARINGNO_SPEED;
					
					_Radio.TriggerMsg(MESSAGE_STRUC(windrep, MSG_STATUSRESPONSE_DELAY_LP_RPT, VOICE_TOWER, NULL, AC));
				}
			}
			
			if(AC->classtype->aerobaticfactor == AEROBATIC_LOW)
			{
				bool sentmessage = false;
				for(int i = SagBAND; i < SagBANDEND; i++)
				{
					AirStrucPtr as = Persons2::ConvertPtrUID(UniqueID(i));
					if((as) && (as->nationality == AC->nationality) && (as->classtype->aerobaticfactor == AEROBATIC_HIGH))
					{
						if((as->fly.numinsag > 0) && (as->fly.expandedsag))
							as = as->fly.expandedsag;
						if(as->fly.leadflight == AC)
						{
							as->fly.leadflight = NULL;
							if(!sentmessage)
							{
								_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_THANKSANDGO, MSG_MIGREPORTED, AC, NULL, as));
								sentmessage = true;
							}
						}
					}
				}
			}
				
			break;
		}

		case	wp_start_cas:
//DEADCODE JIM 16/11/99 			NextWayPoint();
//DEADCODE JIM 16/11/99 			if (	(AirStrucPtr(*this)->duty&DUTYMASK)==DC_CAS 
//DEADCODE JIM 16/11/99 				&& (AirStrucPtr(*this)->duty&ACTIONCODE)!=DA_CAP
//DEADCODE JIM 16/11/99 				)
//DEADCODE JIM 16/11/99 				AirStrucPtr(*this)->SetToCAS(true);
		break;

		case	wp_threat_combfriendly:
		case	wp_threat_engfriendly:
		case	wp_threat_firing:
		case	wp_threat_weapon:
		case	wp_threat_danger:
		case	wp_threat_engage:
		case	wp_threat_any:
			assert (Status.size==AIRSTRUCSIZE);
			AirStrucPtr(*this)->ai.threatlevel=tThreatLevel(action-wp_threat_combfriendly);
			NextWayPoint();
		break;


		}
	}

		if (Status.size>=FormationSize)							//JIM 04Aug96
		{	//find a child and recurse down it					//JIM 04Aug96
			FormationItemPtr	f=*this;						//JIM 04Aug96
			if(f->follower)									//JIM 04Aug96
				f->follower->DoWayPointAction(oldmove,oldwp,action);				//JIM 04Aug96
			elseif (Status.size==AirStrucSize)					//JIM 04Aug96
				if(f->leader && ((AirStrucPtr)*f->leader)->fly.nextflight)//JIM 04Aug96
					((AirStrucPtr)*f->leader)->fly.nextflight->DoWayPointAction(oldmove,oldwp,action);		//JIM 04Aug96
				elseif (((AirStrucPtr)*this)->fly.nextflight)
					((AirStrucPtr)*this)->fly.nextflight->DoWayPointAction(oldmove,oldwp,action);		//JIM 04Aug96
		}														//JIM 04Aug96
		//else finished											//JIM 04Aug96

}


//컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
//Procedure		CheckManualCourse
//Author		Jim Taylor
//Date			Sun 21 Jul 1996
//
//Description	
//				Basically does equivalents to auto movecode for player
//
//				Checks if manual is on course and has reached waypoints
//				Checks for all break off
//
//Inputs		
//
//Returns	
//
//------------------------------------------------------------------------------
void	AirStruc::CheckManualCourse()
{
	if(Status.deadtime)
		return;

//#define PRINT_MANUAL
#ifdef PRINT_MANUAL

#pragma warnmsg("*****  DIRTY HACK ***** GHOST VELOCITY STOMPED *****")
#pragma warnmsg("*****  DIRTY HACK ***** GHOST VELOCITY STOMPED *****")
#pragma warnmsg("*****  DIRTY HACK ***** GHOST VELOCITY STOMPED *****")

	Persons2::PlayerGhostAC->vel_ = Persons2::PlayerSeenAC->vel_;

PrintVar(0, 0, "Movecode %.0f ", FP(movecode));
PrintVar(0, 1, "Waypoint %.0f ", FP(waypoint->wpname));
COORDS3D wp = waypoint->World;
if(waypoint->target)
{	wp.X += waypoint->target->World.X;	wp.Y += waypoint->target->World.Y;	wp.Z += waypoint->target->World.Z;	}
InterceptandRange(&wp);
PrintVar(0, 2, "Dist     %.0f ", FP(Range / 100));
PrintVar(0, 3, "Heading  %.0f ", FP(UWord(HdgIntercept) / 182));

#endif

	switch(movecode)
	{
		case AUTO_TAKEOFF:
		case AUTO_TRAININGTAKEOFF:
		{
			if(World.Y > ai.homebase->World.Y + FT_100)
				movecode = AUTO_FOLLOWWP;
			break;
		}

		case AUTO_FOLLOWWP:
		{
			if(FlyEscortTo())
			{
				AirStrucPtr leadac = Leader();
				if(!leadac)
					leadac = fly.leadflight;

				InterceptandRange(PositionWRTLeader());
				if(Range < METRES100)
					information = IF_NEAR_POS;
				else
				{
					information = IF_OUT_POS;
					leadac->slowdownleader = TRUE;
					if(fly.leadflight)
						fly.leadflight->slowdownleader = TRUE;
					int r = (int)FSqrt(Range / METRES500);				//CSB 07/06/99	
					if(ai.desiredrange)									  //JIM 18/05/99
					if(r > ai.desiredrange)								  //JIM 18/05/99
					{
						if(r < 2)
							_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CLOSEUP, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
						else if(r < 4)
							_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CLOSER, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
						else if(r < 8)
							_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GETBACK, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
						else
							_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GETCLOSER, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
					}
					ai.desiredrange = r;										  //JIM 18/05/99

				}
	
				if((!(leadac->fly.pModel->FrameCount & 0xFF)) && (leadac->Distance3DSquared(&World) < FP(METRES15) * FP(METRES15)))
					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_NOTTHATCLOSE, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
			}
			else
			{
				if((fly.leadflight) && (waypoint->NextWP(this) == fly.leadflight->waypoint))
				{
					if(Distance3DSquared(FindDesPos()) < FP(desposrange + METRES10KM) * FP(desposrange + METRES10KM))
						ExecuteWayPointAction();
				}
				else
				{
					COORDS3D thiswpworld = waypoint->World;
					if(waypoint->target)
					{
						thiswpworld.X += waypoint->target->World.X;
//DeadCode CSB 12Sep00 						thiswpworld.Y += waypoint->target->World.Y;
						thiswpworld.Z += waypoint->target->World.Z;	
					}
					thiswpworld.Y = World.Y;
					FP dist2wp = Distance3DSquared(&thiswpworld);

					if(waypoint->range)
					{
						if(dist2wp < FP(waypoint->range) * FP(waypoint->range))
							ExecuteWayPointAction();
					}
					else
					{
						WayPointPtr	nextwp = waypoint->NextWP(this);
						if(nextwp != waypoint)
						{
							COORDS3D nextwpworld = nextwp->World;
							if(nextwp->target)
							{
								nextwpworld.X += nextwp->target->World.X;
	//DeadCode CSB 12Sep00 							nextwpworld.Y += nextwp->target->World.Y;
								nextwpworld.Z += nextwp->target->World.Z;	
							}
							nextwpworld.Y = World.Y;
							FP dist2apart =		FP(nextwpworld.X - thiswpworld.X) * FP(nextwpworld.X - thiswpworld.X)
											+	FP(nextwpworld.Z - thiswpworld.Z) * FP(nextwpworld.Z - thiswpworld.Z);
							FP dist2next = Distance3DSquared(&nextwpworld);
							if(		((dist2wp < dist2apart * 0.05) && (dist2next < dist2apart))
								/*||	(dist2next < dist2wp)*/	) 
								ExecuteWayPointAction();
						}
						else
							if(dist2wp < FP(waypoint->range + METRES1000) * FP(waypoint->range + METRES1000))
								ExecuteWayPointAction();
					}
				}
			}
			break;
		}

		case AUTO_BOMB:
		{
			if((ai.ManStep == 0) && (!formpos))
			{
				for(AirStrucPtr ld = this; ld; ld = ld->fly.nextflight)
					for(AirStrucPtr ac = ld; ac; ac = ac->Follower())
						ac->ai.ManStep = 1;
				ai.ManStep = 3;
			}

			if(ai.ManStep == 15)
			{
				bool exitbomb = true;
				for(AirStrucPtr ld = this; ld; ld = ld->fly.nextflight)
					for(AirStrucPtr ac = ld; ac; ac = ac->Follower())
						if(ac->ai.ManStep != 15)
							exitbomb = false;
				if(exitbomb)
					for(AirStrucPtr ld = this; ld; ld = ld->fly.nextflight)
						for(AirStrucPtr ac = ld; ac; ac = ac->Follower())
						{
							ac->movecode = AUTO_FOLLOWWP;
							ac->waypoint = ac->waypoint->NextWP(ac);
							ac->information = IF_OUT_POS;
							ac->manoeuvretime = 0;
						}
			}
		}
		
		case AUTO_TRAININGFORMATION:
		{
			static bool finished;
			static SLong timeinform;
			const SLong totaltime = 2 * 60 * SECS100;
			const FP gooddist2 = FP(METRES100) * FP(METRES100);
			const FP baddist2  = FP(METRES500) * FP(METRES500);
			const FP goodscore   = 0.8;
			const FP badscore    = 0.5;

			if((!_Replay.Playback) && (Persons2::PlayerSeenAC->fly.pModel->FirstCycle))
			{
				finished = false;
				timeinform = 0;
				SetManoeuvreTime(totaltime);
			}

			AirStrucPtr lead = FindFormpos0();
			if((!finished) && (lead) && (lead != this))
			{
				despos = *PositionWRTLeader();
				FP dist2 = Distance3DSquared(&despos);

	//DEADCODE CSB 02/03/00 PrintVar(60, 20, "Dist %.0f ", FP(FSqrt(dist2) * 0.01));

				if(dist2 > baddist2)
				{
					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_WHATHELLDOING, MSG_STATUS, lead, NULL, this));
					finished = true;
				}
				else if(dist2 < gooddist2)
					timeinform += Timer_Code.FRAMETIME;

	//DEADCODE CSB 02/03/00 PrintVar(60, 21, "Good %.0f ", FP(timeinform * 0.01));
	//DEADCODE CSB 02/03/00 PrintVar(60, 22, "Time %.0f ", FP(manoeuvretime * 0.01));

				if(!TimeLeft())
				{
					FP score = timeinform / totaltime;
					if(score > goodscore)
						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GOOD, MSG_STATUS, lead, NULL, this));
					else if(score > badscore)
						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_NOTBAD, MSG_STATUS, lead, NULL, this));
					else
						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_NOTGOOD, MSG_STATUS, lead, NULL, this));

					finished = true;
				}
			}
			break;
		}
	}
}
		
//DeadCode CSB 12Sep00 	ANGLES	currintercept,originalintercept;
//DeadCode CSB 12Sep00 	SLong	range,dprange;
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 	COORDS3D* dp;
//DeadCode CSB 12Sep00 	if (movecode==AUTO_PRECOMBAT)
//DeadCode CSB 12Sep00 		if (ai.unfriendly==NULL)
//DeadCode CSB 12Sep00 			if (formpos)
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 				InterceptandRange (dp=PositionWRTLeader ());
//DeadCode CSB 12Sep00 				range=Range;
//DeadCode CSB 12Sep00 				currintercept=originalintercept=HdgIntercept;
//DeadCode CSB 12Sep00 				dprange=0;
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 			else
//DeadCode CSB 12Sep00 				movecode=AUTO_FOLLOWWP;
//DeadCode CSB 12Sep00 		else
//DeadCode CSB 12Sep00 		{
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 			dp=&ai.unfriendly->World;
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 			dprange=METRES500;
//DeadCode CSB 12Sep00 			InterceptandRange (dp);
//DeadCode CSB 12Sep00 			if (Range>dprange)
//DeadCode CSB 12Sep00 				InterceptandRange (dp=FindCombatPos());
//DeadCode CSB 12Sep00 			else
//DeadCode CSB 12Sep00 				currintercept=originalintercept=HdgIntercept;
//DeadCode CSB 12Sep00 			currintercept=originalintercept=HdgIntercept;
//DeadCode CSB 12Sep00 			range=Range;
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 	elseif (movecode==AUTO_COMBAT)
//DeadCode CSB 12Sep00 	{
//DeadCode CSB 12Sep00 		if (formpos)
//DeadCode CSB 12Sep00 			if (fly.leadflight->movecode!=AUTO_COMBAT)
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 				movecode=(AutoMoveCodeTypeSelect)fly.leadflight->movecode;
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 			elseif (	fly.leadflight->ai.manoeuvre==MANOEUVRE_TOPCOVER
//DeadCode CSB 12Sep00 					&&	ai.manoeuvre!=MANOEUVRE_TOPCOVER
//DeadCode CSB 12Sep00 					)
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 //TEMP			 	Persons_2.AddMessage(UserMsg::GOHOME,TEXT_LEADERGOINGHOME,fly.leadflight);
//DeadCode CSB 12Sep00 				ai.manoeuvre=MANOEUVRE_TOPCOVER;
//DeadCode CSB 12Sep00 				SetUnfriendly(NULL);									//CSB 1Aug00
//DeadCode CSB 12Sep00 //DeadCode CSB 1Aug00 				if (	ai.unfriendly
//DeadCode CSB 12Sep00 //DeadCode CSB 1Aug00 					&&	ai.unfriendly->Status.size==AIRSTRUCSIZE
//DeadCode CSB 12Sep00 //DeadCode CSB 1Aug00 					&&	((AirStrucPtr)ai.unfriendly)->ai.attacker==this
//DeadCode CSB 12Sep00 //DeadCode CSB 1Aug00 					)
//DeadCode CSB 12Sep00 //DeadCode CSB 1Aug00 					((AirStrucPtr)ai.unfriendly)->ai.attacker=NULL;
//DeadCode CSB 12Sep00 //DeadCode CSB 1Aug00 				ai.unfriendly=NULL;
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 		if (ai.unfriendly)
//DeadCode CSB 12Sep00 		{
//DeadCode CSB 12Sep00 			InterceptandRange (dp=&ai.unfriendly->World);
//DeadCode CSB 12Sep00 			range=Range;
//DeadCode CSB 12Sep00 			currintercept=originalintercept=HdgIntercept;
//DeadCode CSB 12Sep00 			dprange=METRES100;
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 		else
//DeadCode CSB 12Sep00 		{
//DeadCode CSB 12Sep00 			if (	//ai.manoeuvre==MANOEUVRE_TOPCOVER &&			//JIM 03Dec96
//DeadCode CSB 12Sep00 				formpos==0)
//DeadCode CSB 12Sep00 				if (ACMAS().AllOnTopCover())
//DeadCode CSB 12Sep00 				{
//DeadCode CSB 12Sep00 					Art_Int.AllBreakOff(this);									//JIM 16Jul96
//DeadCode CSB 12Sep00 //TEMP				 	Persons_2.AddMessage(UserMsg::GOHOME,TEXT_SQUADRONREGROUPED,NULL);
//DeadCode CSB 12Sep00 				}
//DeadCode CSB 12Sep00 				elseif (ACMAS().AllButLdrNotOnCombat ())
//DeadCode CSB 12Sep00 				{
//DeadCode CSB 12Sep00 					ai.manoeuvre=MANOEUVRE_TOPCOVER;
//DeadCode CSB 12Sep00 				}
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 			return;
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 	elseif (movecode==AUTO_BOMB || movecode==AUTO_CAS)
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 	{
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		if (!ai.unfriendly)
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		{
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 			if (movecode==AUTO_BOMB)
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 				movecode=AUTO_FOLLOWWP;
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		}
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		else
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		{
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 			if (!SelectedElement(ai.unfriendly,ai.manoeuvre.value))
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 			{
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 				int newelt;
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 				ItemPtr trg=SelectTarget(ai.unfriendly,&newelt);
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 				if (!SelectedElement(trg,newelt))
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 					trg=NULL;
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 				OverLay.DecisionMessage(FACMsg,FAC_BombsAway,this,trg,this,FALSE);	//RJS 09Jun99
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 			}
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		}
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		OverLay.CancelAccel();
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		range = 1;		//CSB 13/07/99	Force it to fail later test
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 		dprange = 0;	//CSB 13/07/99	
//DeadCode CSB 12Sep00 //DEADCODE CSB 14/02/00 	}
//DeadCode CSB 12Sep00 	elseif (!FlyEscortTo())
//DeadCode CSB 12Sep00 	{	//followwp
//DeadCode CSB 12Sep00 		InterceptandRange (dp=FindDesPos ());
//DeadCode CSB 12Sep00 		dprange=desposrange+MILES01;	//make it easy for player
//DeadCode CSB 12Sep00 		if (PitchIntercept>ANGLES_85Deg)
//DeadCode CSB 12Sep00 			Range=0;
//DeadCode CSB 12Sep00 		if (waypoint && !formpos && fly.leadflight && fly.leadflight->waypoint==waypoint->next)
//DeadCode CSB 12Sep00 			dprange+=METRES10KM;			//escortee just gone through. See AutoFollowWP
//DeadCode CSB 12Sep00 		originalintercept=currintercept=HdgIntercept;
//DeadCode CSB 12Sep00 		range=Range;
//DeadCode CSB 12Sep00 		WayPointPtr	wp=fly.backupWP;
//DeadCode CSB 12Sep00 		if (wp==NULL)
//DeadCode CSB 12Sep00 			wp=waypoint;
//DeadCode CSB 12Sep00 		if (movecode==AUTO_TAXI && vel_ > 5*60 * 1000)					//RDH 11Dec96
//DeadCode CSB 12Sep00 			movecode=AUTO_FOLLOWWP;								//RDH 11Dec96
//DeadCode CSB 12Sep00 //TempCode JIM 14Nov96 		if (wp && wp->prev)
//DeadCode CSB 12Sep00 //TempCode JIM 14Nov96 		{
//DeadCode CSB 12Sep00 //TempCode JIM 14Nov96 			InterceptandRange(wp->prev->FindDesPos());
//DeadCode CSB 12Sep00 //TempCode JIM 14Nov96 			originalintercept=(ANGLES)(HdgIntercept+ANGLES_180Deg);	//still have old intercept
//DeadCode CSB 12Sep00 //TempCode JIM 14Nov96 		}
//DeadCode CSB 12Sep00 //TempCode JIM 14Nov96 //complicates dp calculation
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 	else if(!Status.deadtime)	//CSB 09/06/99	
//DeadCode CSB 12Sep00 	{	//followwp wing
//DeadCode CSB 12Sep00 		AirStrucPtr leadac=Leader();
//DeadCode CSB 12Sep00 		if (!leadac)
//DeadCode CSB 12Sep00 			leadac=fly.leadflight;
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 		InterceptandRange (dp=PositionWRTLeader ());
//DeadCode CSB 12Sep00 		range=Range;
//DeadCode CSB 12Sep00 		currintercept=originalintercept=HdgIntercept;
//DeadCode CSB 12Sep00 		dprange=0;
//DeadCode CSB 12Sep00 		if (range>METRES100)
//DeadCode CSB 12Sep00 		{
//DeadCode CSB 12Sep00 			information=IF_OUT_POS;
//DeadCode CSB 12Sep00 			leadac->slowdownleader=TRUE;
//DeadCode CSB 12Sep00 			if (fly.leadflight)
//DeadCode CSB 12Sep00 				fly.leadflight->slowdownleader=TRUE;
//DeadCode CSB 12Sep00 //DeadCode CSB 07/06/99				int r=range/METRES250;	  //7 bits=12800 metres
//DeadCode CSB 12Sep00 			int r = (int)FSqrt(range/METRES500);				//CSB 07/06/99	
//DeadCode CSB 12Sep00 			if (ai.desiredrange)									  //JIM 18/05/99
//DeadCode CSB 12Sep00 				if (r>ai.desiredrange)								  //JIM 18/05/99
//DeadCode CSB 12Sep00 					if(r < 2)
//DeadCode CSB 12Sep00 						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CLOSEUP, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
//DeadCode CSB 12Sep00 					else if(r < 4)
//DeadCode CSB 12Sep00 						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CLOSER, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
//DeadCode CSB 12Sep00 					else if(r < 8)
//DeadCode CSB 12Sep00 						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GETBACK, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
//DeadCode CSB 12Sep00 					else
//DeadCode CSB 12Sep00 						_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GETCLOSER, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
//DeadCode CSB 12Sep00 			ai.desiredrange=r;										  //JIM 18/05/99
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 		else
//DeadCode CSB 12Sep00 			information=IF_NEAR_POS;
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 		if((!(leadac->fly.pModel->FrameCount & EVERY256cs)) && (leadac->Distance3DSquared(&World) < FP(METRES15) * FP(METRES15)))
//DeadCode CSB 12Sep00 			_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_NOTTHATCLOSE, MSG_FORMATIONFLYING_RPT, leadac, NULL, this));
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 	//now what?
//DeadCode CSB 12Sep00 	if (range<dprange)											//JIM 20Aug96
//DeadCode CSB 12Sep00 	{	//reached waypoint near enough							//JIM 20Aug96
//DeadCode CSB 12Sep00 //DeadCode JIM 03Dec96 		if (waypoint && movecode<AUTO_PRECOMBAT)				//JIM 20Aug96
//DeadCode CSB 12Sep00 		if(		(movecode != AUTO_CRASHSKID)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_CRASHROLL)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_CRASHONEWHEEL)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_CRASHNOSE)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_CRASHFLIP)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_CRASHTUMBLE)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_DEATHSEQUENCE)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_FOLLOWTHELEADER)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_TRAININGTAKEOFF)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_TRAININGLANDING)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_PRECOMBAT)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_COMBAT)
//DeadCode CSB 12Sep00 			&&	(movecode != AUTO_BOMB)		)
//DeadCode CSB 12Sep00 //DeadCode CSB 22Jun00 		if (movecode<AUTO_PRECOMBAT)				//JIM 20Aug96
//DeadCode CSB 12Sep00 		{														//JIM 20Aug96
//DeadCode CSB 12Sep00 //			UserMsg::Reason	r=UserMsg::PASSEDWP;
//DeadCode CSB 12Sep00 			//tests for special waypoints
//DeadCode CSB 12Sep00 //DeadCode AMM 10Jun98 			if (!waypoint || waypoint->uniqueID.deaded)			//RJS 27Feb98
//DeadCode CSB 12Sep00 //DeadCode AMM 30Nov98 			if (!waypoint || waypoint->uniqueID.commsmove)		//AMM 10Jun98
//DeadCode CSB 12Sep00 			if (!waypoint || waypoint->Status.deaded)			//AMM 30Nov98
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 				ExecuteWayPointAction();
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 			else
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 //DeadCode JIM 06Dec96 				TextSnip0	t=waypoint->GetWpName();
//DeadCode CSB 12Sep00 				ExecuteWayPointAction();
//DeadCode CSB 12Sep00 //DeadCode JIM 06Dec96 				Persons_2.AddMessage(r,t,*FindDesPos ());
//DeadCode CSB 12Sep00 //DeadCode RDH 15Oct96 				Persons_2.ClrMessage(UserMsg::COURSE);
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 		elseif (movecode==AUTO_PRECOMBAT)
//DeadCode CSB 12Sep00 		{	//switch to engage
//DeadCode CSB 12Sep00 			Art_Int.AllEngage(this);
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 	elseif (range>METRES500+dprange)
//DeadCode CSB 12Sep00 	{
//DeadCode CSB 12Sep00 //DEADCODE JIM 16/11/99 		despos=*dp;
//DeadCode CSB 12Sep00 //DEADCODE JIM 16/11/99 		if (Math_Lib.AbsSign((SWord)(currintercept-hdg))>ANGLES_30Deg)
//DeadCode CSB 12Sep00 //DEADCODE JIM 16/11/99 //hard:   if (Math_lib.AbsSign(currintercept-originalintercept)>ANGLES_30Deg)
//DeadCode CSB 12Sep00 //DEADCODE JIM 16/11/99 //TEMP			Persons_2.AddMessage(UserMsg::COURSE,TEXT_NULL,despos);
//DeadCode CSB 12Sep00 //DEADCODE JIM 16/11/99 			;
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 	if (movecode != AUTO_COMBAT)
//DeadCode CSB 12Sep00 	{//don't want this message when in combat						  //RDH 30/04/99
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		ItemBasePtr corner=Persons2::ConvertPtrUID(UID_StNamsanni);
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		if (corner->World.X>World.X)
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		{	//sinuiju corner
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 			if ((corner->World.X-World.X)>(corner->World.Z-World.Z))
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 			{
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 				AirStrucPtr buddy=FindBuddy();
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 				if (buddy)
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CROSSRIVER, MSG_CROSSRIVER, buddy, NULL, this));
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 			}
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		}
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		else
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		{	//north korea
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 			if (corner->World.Z<World.Z)
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 			{
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 				AirStrucPtr buddy=FindBuddy();
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 				if (buddy)
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_CROSSRIVER, MSG_CROSSRIVER, buddy, NULL, this));
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 			}
//DeadCode CSB 12Sep00 //DEADCODE RJS 16/11/99 		}
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 	if(movecode == AUTO_TRAININGFORMATION)
//DeadCode CSB 12Sep00 	{
//DeadCode CSB 12Sep00 		static bool finished;
//DeadCode CSB 12Sep00 		static SLong timeinform;
//DeadCode CSB 12Sep00 		const SLong totaltime = 2 * 60 * SECS100;
//DeadCode CSB 12Sep00 		const FP gooddist2 = FP(METRES100) * FP(METRES100);
//DeadCode CSB 12Sep00 		const FP baddist2  = FP(METRES500) * FP(METRES500);
//DeadCode CSB 12Sep00 		const FP goodscore   = 0.8;
//DeadCode CSB 12Sep00 		const FP badscore    = 0.5;
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 		if((!_Replay.Playback) && (Persons2::PlayerSeenAC->fly.pModel->FirstCycle))
//DeadCode CSB 12Sep00 		{
//DeadCode CSB 12Sep00 			finished = false;
//DeadCode CSB 12Sep00 			timeinform = 0;
//DeadCode CSB 12Sep00 			SetManoeuvreTime(totaltime);
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 		AirStrucPtr lead = FindFormpos0();
//DeadCode CSB 12Sep00 		if((!finished) && (lead) && (lead != this))
//DeadCode CSB 12Sep00 		{
//DeadCode CSB 12Sep00 			despos = *PositionWRTLeader();
//DeadCode CSB 12Sep00 			FP dist2 = Distance3DSquared(&despos);
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 //DEADCODE CSB 02/03/00 PrintVar(60, 20, "Dist %.0f ", FP(FSqrt(dist2) * 0.01));
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 			if(dist2 > baddist2)
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 				_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_WHATHELLDOING, MSG_STATUS, lead, NULL, this));
//DeadCode CSB 12Sep00 				finished = true;
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 			else if(dist2 < gooddist2)
//DeadCode CSB 12Sep00 				timeinform += Timer_Code.FRAMETIME;
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 //DEADCODE CSB 02/03/00 PrintVar(60, 21, "Good %.0f ", FP(timeinform * 0.01));
//DeadCode CSB 12Sep00 //DEADCODE CSB 02/03/00 PrintVar(60, 22, "Time %.0f ", FP(manoeuvretime * 0.01));
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 			if(!TimeLeft())
//DeadCode CSB 12Sep00 			{
//DeadCode CSB 12Sep00 				FP score = timeinform / totaltime;
//DeadCode CSB 12Sep00 				if(score > goodscore)
//DeadCode CSB 12Sep00 					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_GOOD, MSG_STATUS, lead, NULL, this));
//DeadCode CSB 12Sep00 				else if(score > badscore)
//DeadCode CSB 12Sep00 					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_NOTBAD, MSG_STATUS, lead, NULL, this));
//DeadCode CSB 12Sep00 				else
//DeadCode CSB 12Sep00 					_Radio.TriggerMsg(MESSAGE_STRUC(SCRIPT_NOTGOOD, MSG_STATUS, lead, NULL, this));
//DeadCode CSB 12Sep00 
//DeadCode CSB 12Sep00 				finished = true;
//DeadCode CSB 12Sep00 			}
//DeadCode CSB 12Sep00 		}
//DeadCode CSB 12Sep00 	}
//DeadCode CSB 12Sep00 }

