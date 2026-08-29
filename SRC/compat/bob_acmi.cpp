/* bob_acmi.cpp -- Tacview ACMI export for Battle of Britain (EPIC R / R2).
 *
 * PO: "get replay working, add tacview export as well as .cam export to bob".
 *
 * CROSS-PORTED FROM MiG Alley (EPIC L). The MA writer was deliberately built to take PLAIN C
 * TYPES and know nothing about the game structures -- the game side walks its own world and
 * hands over numbers -- which is exactly what makes this port a rename plus two constants
 * rather than a rewrite. Changed here: the theatre origin (southern England, not Korea), the
 * ReferenceTime epoch (10 Jul 1940, the Battle's opening), and the VIDEOS directory casing
 * that S260 lost two searches to.
 *
 * MA carried two hard-won constraints into this design; both apply here unchanged:
 *   1. TEE FROM THE SIM, do not convert the .cam -- a REPLAYPACKET is packed deltas against a
 *      reconstructed world (MA S211/L0).
 *   2. ADDITIVE ONLY, so the existing replay path is its own control.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern "C" {

static FILE* g_acmi = 0;
static char  g_acmi_path[512];
static int   g_acmi_objects = 0;
static double g_acmi_lastT = -1.0;
static unsigned long g_acmi_base = 0;
static long          g_acmi_lastRaw = -1;

/* Theatre origin. BoB's map is flat and local; one reference point anchors the whole file.
   Roughly Seoul -- the exact value only shifts where Tacview draws the map under the track, and
   U/V carry the real geometry. */
static const double ACMI_REF_LON =  0.5;   /* southern England */
static const double ACMI_REF_LAT = 51.0;

int bob_acmi_enabled(void)
{
    const char* e = getenv("BOB_ACMI");
    return (e && e[0] == '0') ? 0 : 1;
}

/* Begin a recording. `title` names the sortie. Safe to call repeatedly; a second call closes the
   first. Returns 1 if a file is open. */
int bob_acmi_begin(const char* title)
{
    if (!bob_acmi_enabled()) return 0;
    if (g_acmi) { fclose(g_acmi); g_acmi = 0; }
    g_acmi_objects = 0; g_acmi_lastT = -1.0;
    g_acmi_base = 0; g_acmi_lastRaw = -1;   /* S275: new recording, new clock */
    snprintf(g_acmi_path, sizeof(g_acmi_path), "%s", "acmi_current.txt");
    g_acmi = fopen(g_acmi_path, "wb");
    if (!g_acmi) return 0;
    fprintf(g_acmi, "FileType=text/acmi/tacview\r\n");
    fprintf(g_acmi, "FileVersion=2.2\r\n");
    /* Global properties live on object 0. ReferenceTime is an ISO-8601 instant; MA's campaign is
       Korea 1950-53, and the mission date is not plumbed here yet (L4), so a fixed epoch keeps the
       file valid and the RELATIVE timeline -- which is what a debrief reads -- exact. */
    fprintf(g_acmi, "0,ReferenceTime=1940-07-10T00:00:00Z\r\n");
    fprintf(g_acmi, "0,ReferenceLongitude=%.6f\r\n", ACMI_REF_LON);
    fprintf(g_acmi, "0,ReferenceLatitude=%.6f\r\n",  ACMI_REF_LAT);
    fprintf(g_acmi, "0,DataSource=Battle of Britain (Rowan, 2000) -- Linux port\r\n");
    fprintf(g_acmi, "0,DataRecorder=bob_acmi (EPIC L)\r\n");
    fprintf(g_acmi, "0,Title=%s\r\n", (title && *title) ? title : "Battle of Britain sortie");
    fflush(g_acmi);
    return 1;
}

/* Advance the timeline. ACMI requires time markers to be MONOTONIC; emit one only when the time
   actually moves forward, so a repeated or stale call cannot corrupt the file. */
/* S275 (PO 2026-08-29: "tacview file works! But very short" -- 20.40 s of a full dogfight).
   REPLAY.CPP fed this `replayframecount / hz`, and replayframecount RESETS EVERY BLOCK
   (`#define FRAMESINBLOCK 512`, REPLAY.CPP:106, wrapped at :556). At the first wrap the timestamp
   jumped back to 0, the `seconds <= g_acmi_lastT` guard below returned 0, and the caller's
   `goto acmi_done` then skipped EVERY remaining sample -- so recording stopped dead at
   512/25 = 20.48 s and never resumed. That is exactly the length the PO got, and it is the same
   defect as MA's PO-79 in the same engine.
   A free-running counter would NOT do: S274 established that StoreDeltas runs ~7.7x per
   replayframecount advance, and duplicate suppression depends on the raw counter STAYING PUT
   across those redundant calls. Incrementing per call would restore the ~7.7x bloat that made a
   sortie 57 MB. So keep the raw counter as the de-duplication key and only lift it past each wrap.
   BOB_ACMI_BLOCKCLOCK=1 restores the old behaviour for an A/B. */
unsigned long bob_acmi_frame_monotonic(unsigned long raw)
{
    if (g_acmi_lastRaw >= 0 && (long)raw < g_acmi_lastRaw)
        g_acmi_base += (unsigned long)(g_acmi_lastRaw + 1);   /* a block wrapped */
    g_acmi_lastRaw = (long)raw;
    return g_acmi_base + raw;
}

int bob_acmi_time(double seconds)
{
    if (!g_acmi) return 0;
    if (seconds <= g_acmi_lastT) return 0;   /* S274: caller uses this to skip duplicate samples */
    fprintf(g_acmi, "#%.2f\r\n", seconds);
    g_acmi_lastT = seconds;
    /* S275b: the PO's first export ended mid-record because the process still held unflushed
       stdio. Bound the loss to about a second of track rather than a whole buffer. */
    { static int n = 0; if (++n >= 25) { n = 0; fflush(g_acmi); } }
    return 1;
}

/* One object's state at the current time marker.
   u/v/alt are METRES (callers convert from the sim's centimetres); roll/pitch/yaw in DEGREES. */
void bob_acmi_object_ias(unsigned long id, double u, double v, double alt,
                        double roll, double pitch, double yaw,
                        const char* name, const char* type, const char* color, int isPlayer,
                        double ias);

void bob_acmi_object(unsigned long id, double u, double v, double alt,
                    double roll, double pitch, double yaw,
                    const char* name, const char* type, const char* color, int isPlayer)
{
    bob_acmi_object_ias(id, u, v, alt, roll, pitch, yaw, name, type, color, isPlayer, -1.0);
}

/* L4: same, plus indicated airspeed in METRES PER SECOND (the ACMI spec is metric throughout).
   Pass a negative ias to omit the property. */
void bob_acmi_object_ias(unsigned long id, double u, double v, double alt,
                        double roll, double pitch, double yaw,
                        const char* name, const char* type, const char* color, int isPlayer,
                        double ias)
{
    if (!g_acmi || !id) return;
    /* S284: EMIT REAL LON/LAT, not just U/V.
       This wrote "T=||alt|roll|pitch|yaw|u|v|hdg" -- Lon and Lat deliberately blank, on the belief
       that Tacview would position objects from the native flat-world U/V. It does not: Lon/Lat are
       the authoritative spherical position and U/V are supplementary. With both blank every object
       stayed pinned at the reference origin for the whole recording while its attitude and altitude
       kept updating.
       Ã¢Â­Â THE PO DIAGNOSED THIS FROM THE PICTURE, and the report is worth preserving because of how
       precise it was: "each aircraft seems constrained to stay at the same X,Y location - it can
       rotate and move up and down, but not translate in the X-Y plane". That splits the transform
       exactly along the line between the fields written into non-Lon/Lat slots (Alt, Roll, Pitch,
       Yaw -- all working) and the one field encoded ONLY as U/V (X-Y -- dead). No other fault has
       that shape.
       It also explains two earlier reports I had misattributed to file truncation: "nothing makes
       aircraft start moving" (MA) and "I don't see motion, though there is a slow motion of the z
       axis" (BoB). The truncation was real and separate; this is why motion was missing even in the
       part of the file that survived. A fix that makes a symptom smaller is not proof it was the
       cause -- S278 shortened these files' visible span and I read the remaining stillness as "not
       enough file", when the aircraft were never going to move at any length.
       Flat-earth conversion about the reference point: at these scales (a theatre a few hundred km
       across) the error from ignoring curvature is far below what a debrief can perceive. U/V are
       still emitted so the native coordinates remain available. */
    {
    /* S296: the reference point and world origin are RUNTIME-ADJUSTABLE, because I could not
       calibrate them from the data this sprint and a wrong constant baked in is worse than a knob.
       WHAT IS AND IS NOT KNOWN: all RELATIVE geometry -- ranges, headings, closure, formation
       shape -- is correct and verified (S284: separation from Lon/Lat agreed with the U/V figure to
       4 m in 3.3 km). What is arbitrary is where on Earth the theatre is PINNED. The sim's U/V are
       absolute world metres from a map origin of (0,0) (MAPS.H KOREAMAPORIGINX/Y), and nothing in
       the source ties that origin to a real latitude: the map extents are not in the headers, and
       the node tables name real places (Seoul, Pyongyang, Kimpo, Sinuiju) but carry UIDs, not
       coordinates -- the positions live in the world item data, reachable only at runtime.
       BOB_ACMI_REF="lat,lon" moves the reference; BOB_ACMI_ORIGIN="u,v"
       subtracts a world origin in metres before converting. A proper calibration wants two known
       landmarks: dump a named airfield's runtime World.X/Z, pair it with its real coordinates
       (Kimpo 37.558N 126.791E, Sinuiju 40.100N 124.400E), and solve for offset and scale. That is
       a bounded sprint, and it is NOT this one -- said plainly rather than left as a silent
       approximation the next reader would take for a measured value. */
    double _refLat = ACMI_REF_LAT, _refLon = ACMI_REF_LON, _oU = 0.0, _oV = 0.0;
    { const char* r = getenv("BOB_ACMI_REF");
      if (r) sscanf(r, "%lf,%lf", &_refLat, &_refLon);
      const char* o = getenv("BOB_ACMI_ORIGIN");
      if (o) sscanf(o, "%lf,%lf", &_oU, &_oV); }
    const double _mPerDegLat = 111132.0;
    const double _mPerDegLon = 111320.0 * cos(_refLat * 3.14159265358979323846 / 180.0);
    double _lat = _refLat + (v - _oV) / _mPerDegLat;
    double _lon = _refLon + (u - _oU) / _mPerDegLon;
    fprintf(g_acmi, "%lx,T=%.7f|%.7f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f|%.2f",
            id, _lon, _lat, alt, roll, pitch, yaw, u, v, yaw);
    }
    if (name  && *name)  fprintf(g_acmi, ",Name=%s", name);
    if (type  && *type)  fprintf(g_acmi, ",Type=%s", type);
    if (color && *color) fprintf(g_acmi, ",Color=%s", color);
    if (isPlayer)        fprintf(g_acmi, ",Pilot=Player");
    if (ias >= 0.0)      fprintf(g_acmi, ",IAS=%.2f", ias);
    fprintf(g_acmi, "\r\n");
    g_acmi_objects++;
}

/* An object left the world (destroyed or despawned). */
void bob_acmi_remove(unsigned long id)
{
    if (!g_acmi || !id) return;
    fprintf(g_acmi, "-%lx\r\n", id);
}

int bob_acmi_active(void)   { return g_acmi ? 1 : 0; }
int bob_acmi_count(void)    { return g_acmi_objects; }

void bob_acmi_end(void)
{
    if (!g_acmi) return;
    fclose(g_acmi); g_acmi = 0;
}

/* Publish the recording beside the .cam the game just wrote. `camname` is what SaveReplayData was
   given (e.g. "260825test6.cam"); the .acmi takes the same stem. Returns 1 on success.
   The current recording stays open -- saving mid-flight must not stop the tee. */
int bob_acmi_save_as(const char* camname)
{
    if (!bob_acmi_enabled() || !camname || !*camname) return 0;
    if (g_acmi) fflush(g_acmi);
    char stem[512]; snprintf(stem, sizeof(stem), "%s", camname);
    char* dot = strrchr(stem, '.'); if (dot) *dot = 0;
    char out[600]; snprintf(out, sizeof(out), "VIDEOS/%s.acmi", stem);
    FILE* in = fopen(g_acmi_path, "rb");
    if (!in) return 0;
    FILE* o = fopen(out, "wb");
    if (!o) { fclose(in); return 0; }
    /* S259 (L5): COPY WHOLE LINES ONLY.
       The working file is written continuously while the sim runs, so at the instant of a save --
       or of a kill -- its last line can be half-formed. The L5 gate found exactly that: 171 object
       lines with a valid 9-field transform and ONE trailing line cut mid-number
       ("2,T=||1555.24|71.65|14.56|145.76|413327.26|8"). Harmless in the scratch file, NOT harmless
       in a published .acmi: a debrief tool should never be handed a truncated record, and a partial
       line is exactly the sort of thing that makes a parser reject an otherwise good file.
       So the copy stops at the last newline. Whatever follows it is an incomplete sample and is
       dropped -- one lost frame out of thousands, against a file that is always well-formed. */
    char buf[8192]; size_t n; long carry = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        size_t last = 0, i;
        for (i = 0; i < n; i++) if (buf[i] == '\n') last = i + 1;
        if (last) { fwrite(buf, 1, last, o); carry = 0; }
        if (last < n) {
            /* tail without a newline: hold it -- if more data follows it completes a line, and if
               this was the final read it is the partial sample we deliberately drop. */
            memmove(buf, buf + last, n - last);
            carry = (long)(n - last);
            size_t got = fread(buf + carry, 1, sizeof(buf) - carry, in);
            if (got == 0) break;
            n = carry + got;
            size_t l2 = 0; for (i = 0; i < n; i++) if (buf[i] == '\n') l2 = i + 1;
            if (l2) fwrite(buf, 1, l2, o);
            if (l2 >= n) continue;
            memmove(buf, buf + l2, n - l2); carry = (long)(n - l2);
        }
    }
    fclose(o); fclose(in);
    if (getenv("BOB_TRACE_ACMI"))
        fprintf(stderr, "[acmi] wrote %s (%d object samples)\n", out, g_acmi_objects);
    return 1;
}

} /* extern "C" */
