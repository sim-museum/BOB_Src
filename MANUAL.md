# Battle of Britain — Pilot's and Commander's Manual (Linux port)

*New manual, 2026-10-01. Written for the native Linux port of Rowan's Battle of Britain with the BDG 0.99
patch data, the build the port runs.*

**Where this comes from**

- **The base game:** Rowan's 2000 manual (`DOC/rowanbob manual.pdf`).
- **The patch:** the BDG 0.99 patch manual.
- **The campaign:** the in-game online help (summarised in `DOC/OnlineDocSummary.md` and
  `DOC/campaign_summary.md`), plus the BDG RAF-tactics chapter.
- **Flying tips:** the getting-started guide in `~/sgl/TUE/BattleOfBritain/DOC`.
- **Linux specifics:** the port's own notes.
- **Keys:** every key comes from the game's **live key table**, read out of the running program
  ([section 14](#14-keyboard-reference)).

Where a document and the program disagree, the program wins; [section 15](#15-corrections-to-the-older-manuals)
lists every correction. Two of them matter immediately:
- **1** and **2** are aileron trim, not 10 % and 20 % throttle.
- Several BDG-patch keys (S centre padlock, Ctrl+A autopilot, roving camera, WinAmp) **are not
  implemented** in this build.

---

## Contents

1. [The game](#1-the-game)
2. [Installing and starting](#2-installing-and-starting)
3. [Main menu](#3-main-menu)
4. [Configuration](#4-configuration)
5. [Quick Shots](#5-quick-shots)
6. [The campaign](#6-the-campaign)
7. [Commanding the RAF](#7-commanding-the-raf)
8. [Commanding the Luftwaffe](#8-commanding-the-luftwaffe)
9. [The aircraft and their controls](#9-the-aircraft-and-their-controls)
10. [Engines, propellers and starting](#10-engines-propellers-and-starting)
11. [Flying and fighting](#11-flying-and-fighting)
12. [Views, radio and the gun camera](#12-views-radio-and-the-gun-camera)
13. [Multiplayer](#13-multiplayer)
14. [Keyboard reference](#14-keyboard-reference)
15. [Corrections to the older manuals](#15-corrections-to-the-older-manuals)
16. [Troubleshooting](#16-troubleshooting)

---

## 1. The game

Summer 1940, 10 July to 15 September. You can fight the battle three ways:
- as a **pilot** of one of five aircraft, or as a gunner in the bombers;
- as the **RAF commander** defending south-east England;
- as the **Luftwaffe commander** trying to win air superiority before *Sea Lion*.

The campaign is dynamic: hundreds of aircraft fly whether or not you are in one of them.

| Flyable aircraft | Side | Character |
|---|---|---|
| Supermarine Spitfire I | RAF | Best turning fighter; Merlin with a gravity carburettor (cuts out under negative g) |
| Hawker Hurricane I | RAF | Two-thirds of Fighter Command; the better bomber-killer |
| Messerschmitt Bf 109E | Luftwaffe | Climbs better and dives better; fuel injection (no negative-g cut). A good beginner's choice: constant-speed propeller, no cut-out. Only 20 minutes over England. |
| Messerschmitt Bf 110 | Luftwaffe | Twin-engined long-range fighter with a rear gunner; sluggish in a turning fight |
| Junkers Ju 87 Stuka | Luftwaffe | Precise dive bomber, very vulnerable to fighters |

You can also man the guns of the bombers (He 111, Do 17, Ju 88). Picking a bomber puts you in a
gunner position.

---

## 2. Installing and starting

**The AppImage.**

```bash
chmod +x BattleOfBritain-x86_64-<ver>.AppImage
./BattleOfBritain-x86_64-<ver>.AppImage
```

**From an installed tree** (developer layout):

```bash
cd "~/sgl/TUE/BattleOfBritain/WP/drive_c/Program Files/Rowan Software/Battle Of Britain"
BOB_RUN_INIT=1 BOB_FRONTEND=1 BOB_OLE_DRAW=1 ~/bob/build/bob
```

`BOB_BOOT_FRONTEND=1` instead jumps straight into a Quick Shot cockpit.

- **Data folder:** `BOB_HOME=<dir>` gives a second copy its own saves and settings.
- **Window position:** `BOB_WINPOS=x,y` places the window, which helps when running two copies.
- **Quitting:** use **Quit** on the main menu. **Ctrl+Esc** is a port-added emergency exit.
- **Settings file:** `SAVEGAME/SETTINGS.CFG`. Delete it if the game will not start after a bad setting.

**No Wine and no dual monitors.** The native port draws one window. The Wine-era advice in the old
getting-started guide does not apply: winehq-staging, dual-monitor mode, "safe mode" and Alt+F6 to
hide the black box. The campaign map's buttons and right-click menus all work in that one window.

---

## 3. Main menu

| Item | What it does |
|---|---|
| **Quick Shots** | 28 single missions ([section 5](#5-quick-shots)) |
| **Campaigns** | The dynamic campaign, either side, from any of four phases ([section 6](#6-the-campaign)) |
| **Multi-Player** | Death Match, Team Play and Quick Missions ([section 13](#13-multiplayer)) |
| **Load Game** | Load a saved campaign; choose RAF or Luftwaffe at the bottom first |
| **Replay** | Gun-camera footage ([section 12](#12-views-radio-and-the-gun-camera)) |
| **PC Config** | Display, controllers, sound, 2-D screens; plus **BDG** pages |
| **Sim Config** | Flight model, game rules, mission AI, views |
| **Credits**, **Quit** | |

Every setting is a "combo" box: click the arrow for a list, or click the box to step to the next
value.

---

## 4. Configuration

### PC Config

**3D**
- Display driver and resolution.
- Gamma.
- Lowest frame rate and auto frame-rate: auto lowers detail to hold the frame rate.
- Ground and item shading.
- Reflections: cockpit mirror and glass.
- Weather: individual clouds, or off.

**3DII**
- Filtering, smoke and fog banks, texture quality.
- Town and forest raises, routes (small roads, rivers, rail).
- Aircraft and item shadows, horizon distance.
- **Detail level:** High is the designed level.
- **G effects, injury effects, white-outs.**

**Controller**
- Input device, enable, and force feedback (gunfire, buffet, aerodynamic, airframe).
- Per axis: the device, its dead zone (small / medium / large), and its mode.
  - Stick, aileron and rudder: *Realistic* or the more forgiving *Sim*.
  - Other axes: sensitivity.
  - Optional flip.
- Axes: **stick, rudder, throttle** (a second throttle for the Bf 110), **prop pitch** (a second for
  the Bf 110), **view pan, zoom, cockpit** and **gunner**.
- **Cockpit** is the pointer used for the interactive cockpit *and for the in-flight radio and map
  menus*. If the mouse is on view pan, assign the *shifted* mouse to the cockpit. The **right mouse
  button** then switches between the two.
- With *Gunner* set to None, the bomber guns cannot be moved, so don't fly bombers.

**Sound**
- Driver, 3-D effects volume (the master for radio and engine), stereo or 3-D processing.
- Interface, ambient, radio and engine volumes; effects quality.

**2D**
- Resolutions for the campaign map screen, intro, map events (frag, end-of-day) and the scramble
  (menu) screens.
- The art is designed for 1024×768.

**BDG pages (patch 0.99).**

*Field of view:*
- Small, medium and large FOV.
- Minimum and maximum zoom.
- Initial FOV.
- The padlock-centre wedge.

*Game toggles:*
- No friendly collisions, no engine cut-out (needs manual engine management), no stall horn.
- Bomber stragglers, fighter damage (AI bails out), padlock-centre box, enemy position indicator.
- Video skipping and continue/quit boxes.
- Head bobbing.
- Preloading.
- Keyboard-rudder fix (keyboard rudder deflects fully; don't use it with pedals).
- Hi-res landscape, AutoGen, TrackIR, near clip, sky poly, EPI size, pan speed, horizon size, view
  distance, `WK_LANDSCAPE`, gun convergence (250 yd default).

These settings and more live in `bdg.txt`: labels, infoline colour, `USE_NATIONAL_UNITS`,
`HIDE_AMMO_COUNTER`, `START_FROM_PEN`, `ALWAYS_BEHIND_GUNSIGHT`. *This build implements only part of
BDG; a toggle can be present but do nothing, see [section 15](#15-corrections-to-the-older-manuals).*

### Sim Config

**Flight**

| Option | Effect |
|---|---|
| Flight options | Minimum to Maximum presets, or Custom |
| Flight model | *Realistic*, or *Novice* (cannot stall or spin, accurate performance) |
| Engine management | *Manual* (magnetos, fuel cocks, primer, starter; interactive cockpit only, Realistic model only) or *Auto* |
| Prop pitch control | *Manual* or *Auto*. Auto gives the most power but no windmill braking or feathering. |
| Power boost | About 50 % extra power |
| Wind, gusts | |
| Airframe stress | Wings can fail in a hard pull-out |
| Torque / slipstream | |
| 109 fuel capacity | *Realistic* (400 l) or *High* (700 l, no leaky drop tank) |

**Game**

| Option | Effect |
|---|---|
| Weapons | Realistic, or Unlimited; **Ctrl+R** reloads anyway |
| Vulnerable to fire, ground collisions, mid-air collisions | Mid-air also covers buildings and vehicles |
| Complex AI | All aircraft on the full flight model, or only you and your opponent |
| Accel off | *Tactical* (early drop to real time) or *Engage* |
| Target size | |
| Autocanopy | Opens and closes the canopy at low speed |
| Aircraft names | Labels |

**Mission** (campaign difficulty)
- LW and RAF skill modifiers.
- Luftwaffe tactics: *Historic* or *Optimum*. Optimum shortens the convoy phase and extends the
  airfield phases.
- Luftwaffe intelligence: *Historic* or *Accurate*.
- Map plotting: historic radar plots, or always accurate.
- Auto vectoring.

The hardest RAF campaign is LW skill Maximum, RAF skill Minimum, Optimum tactics, Accurate
intelligence.

**Views**

| Option | Effect |
|---|---|
| Restricted views | Cockpit only (for fair multiplayer) |
| Peripheral vision | Roundels at the screen edge |
| Auto external | |
| View mode | Panning or fixed; **NumLock** toggles it in flight |
| Padlock when visible | |
| Info line | Off, flight info or view info |
| Units | |
| Gun camera | Off, Trigger or On |
| Head-up display | Virtual threat indicator and horizon |

---

## 5. Quick Shots

There are 28 ready-made missions. Pick a **type** in the upper box and a **mission** in the lower box.

| Type | Missions |
|---|---|
| **Basic training** | Take-off, Landing, Circuits, Squadron Take-off, Squadron Landing |
| **Advanced training** | Follow the Leader, Formation Flying, Free Flight, Landing – Engine Failure, Landing – Heavy Damage |
| **Dogfighting** | Turkey Shoot, One on One, Random Advantage, RAF Advantage, LUF Advantage |
| **Ground attack** (Luftwaffe) | Dive Bomb Attack, Anti-Shipping, Low Level Attack |
| **Interceptions** | Lone Fighter vs Lone Bomber, Front Attack against Bombers, Rear Attack against Bombers, Scattered Attackers / Scattered Escort |
| **Historic** | 19 Jul End of the Defiant, 13 Aug Eagle Day, 15 Aug Black Thursday (morning, afternoon), 9 Sep London, 15 Sep Battle of Britain Day |

**The four buttons**
- **Scenario:** the description.
- **Parameters:** target area and ID, weather (clear, patchy, low or high cloud), time (dawn to
  dusk), and your callsign.
- **RAF** and **Luftwaffe:** a panel for each unit. Click the **aircraft icon** to fly in that unit.
  You can change the type (within the same role), the number of flights, the altitude and the skill.

Flights are 3 aircraft (Spitfire, Hurricane, Ju 87), 4 (Bf 109) or 5 (medium bombers). In Turkey
Shoot you can fly a German aircraft by selecting *Luftwaffe* and clicking the icon beside a German
type.

**Fly** opens the **Frag** screen, which lists units, aircraft, duty and callsign, with the pilot slots
in formation. Click a slot to change your position; *Return to Player* finds you again. **Fly** again
to launch.

**Ending.** **Alt+X** ends the mission. When the mission's goal is met, a *Continue / Stand down*
prompt appears for a few seconds. The **combat report** follows, with Report, Diary and Replay.

---

## 6. The campaign

**Starting**
1. Campaigns → choose **RAF** or **Luftwaffe** command.
2. Choose a phase:

   | Phase | Dates | What happens |
   |---|---|---|
   | **Convoys** | 10 Jul – 11 Aug | Channel convoys, to draw the RAF out |
   | **Eagle Attack** | 12 – 23 Aug | Radar stations and forward airfields |
   | **Critical Period** | 23 Aug – 6 Sep | Inland airfields and aircraft factories |
   | **Blitz** | 7 – 15 Sep | London, culminating on 15 September |

3. **Begin**, edit your name, then **Begin** again.

The whole battle starts at Convoys and carries your forces forward from phase to phase.

**Saving**
- Use **Filing** on the map toolbar (save or load at any time).
- You are offered a save when you quit.
- **AutoSave** is written every time you go to fly, at the Frag screen.

**The campaign screen** has a map with four levels of zoom (400 miles down to a 2-mile square). Zoom
with the scale toolbar, the right-click menu or the mouse wheel; scroll by dragging. It also has five
toolbars:

| Toolbar | What it holds |
|---|---|
| **Time** | Date, time and rate; Pause / Play / Accel (up to ×600); Game Speed opens Time Control |
| **Main** (notebook icons) | Aircraft Allocation, Resources, Geschwader/Squadron lists, Weather, Review, Pilot Info, Target/Asset list, Mission Folder; for the RAF, the Hostiles list |
| **Map** (telephone icons) | Thumbnail or Zoom Level/Toggle, Directives toggle, Map Filters, Filing, Replay |
| **Scale** | Dockable, in nautical miles or km |
| **Teletype** | The last three messages; click one for the full text |

**Help.** Every dialogue has a **?** that opens context help for each element. Right-click away
from the map for a menu that duplicates the toolbars. Hovering names every icon.

**Flying in a campaign.**
- Click **Fly** or **Frag** when a dialogue offers it, or click a friendly flight on the map.
- On the Frag screen, the scroll bar at the upper right picks the aircraft.
- The **Fly** tab of Time Control sets when you are offered take-overs (by unit or by type, and which
  squadron is your "favourite").
- With *Pause on raid forming*, you never miss a raid.

---

## 7. Commanding the RAF

**Your force**
- 53 squadrons (34 Hurricane, 19 Spitfire), about 600 aircraft, in four Groups:

  | Group | Area |
  |---|---|
  | **11** | The south-east, the front line |
  | **10** | The south-west |
  | **12** | The Midlands |
  | **13** | The north, for rest |

- A squadron is up to 12 aircraft, in flights and vics of three.
- **Radar:** Chain Home sees about 150 km out; Chain Home Low sees low raiders to about 30 km.
  Inland, the **Royal Observer Corps** tracks raids, and loses them in heavy cloud.
- Your job is to keep Fighter Command fighting while protecting airfields, radar, docks, convoys and
  factories. Neglecting the "political" targets gets you sacked. BDG fixed the old convoy-loss bug
  that sacked commanders around 4 August.

**Readiness: time is the scarce resource**
- Each morning, set a minimum number of squadrons at **2 minutes** readiness, with the rest at **5
  minutes**. Rotate who sits at 2 minutes, and keep both Spitfires and Hurricanes ready.
- **Release** tired squadrons, especially in bad weather; set "rest if" minimums so it happens
  automatically. Replace battered units with care, because fresh squadrons suffer at first.
- Set standing patrols only over convoys and other high-priority targets. Cancel automatic responses
  and scrambles, or the computer fights the battle for you.
- Afternoons ("tea-time") are busy if the morning was quiet. The AI plans raids in three periods.

**Deployment, when a raid is detected**
- The raid marker shows raid number, strength and current height, for the bombers only. Turn on
  per-Gruppe markers in **Map Filters** to see the fighters.
- **Intercept lone reconnaissance aircraft.** The Luftwaffe picks its targets from what its
  reconnaissance tells it.
- A raid takes 15–30 minutes to form and reach south-east targets. A patrol needs 10–20 minutes to
  scramble and climb to 15,000 ft, so decide now.
- Put patrols in **holding positions** beside the likely path (towns are good markers), not on it.
  Think in three dimensions.
- Commit about **one fighter for every three raiders**.
- **Hurricanes against the bombers, Spitfires against the escort.** Stagger the Spitfires' heights
  (18,000 and 21,000 ft): bombers usually come in near 15,000 ft, with escorts above.
- Tactics:
  - Avoid **Vics**; it is nearly suicidal against escorts.
  - **Individual** is the only good choice against fighters.
  - **Line astern** breaks up bomber formations.
  - **Head-on** is deadly, but only if you can get into position.
  - **Dive and zoom** is poor against bombers.
- Paired squadrons and Big Wings hit harder but take much longer to assemble.
- Once the raid is in your airspace, stop committing new squadrons, because they will arrive too late.

**Patrols**
- **Directives** (shown each period) set patrols over radar holes, convoys (continuous relays;
  enough squadrons for every convoy) and the coast (a one-shot spread over working radar stations).
  Size them as *squadron or less*, *paired* or *Big Wing*.
- **By hand:** click a target, such as Dover CH, to open its Intel dialogue, then authorise a patrol.
  Set readiness, preferred targets (bombers or fighters), attack method and altitude.
- Edit routes waypoint by waypoint. Moving the IP moves the waypoints after it. Use Time on Target
  to stagger patrols for continuous cover.

**A single-squadron career.** Set the Fly tab to your favourite squadron only, uncheck "All", and pause
on raid forming. You fly when that squadron is tasked (BDG fixed the case where it never woke you).

---

## 8. Commanding the Luftwaffe

**Your force**
- At the start, three Geschwader (about 300 aircraft); later up to 24 (about 2,000).
- Luftflotte 2 (Brussels) and Luftflotte 3 (Paris).
- A **Geschwader** is about 100 aircraft of one type, divided into Gruppen and Staffeln, flying in
  Schwärme of four or Ketten of three.
- At most 90 Staffel missions per period.

**Your job** is air superiority over the south-east: destroy Fighter Command in the air and on the
ground. Radar stations and fighter airfields matter most. **Reconnaissance** keeps your intelligence
on targets current, which matters with *Historic* intelligence.

**Directives**
1. Set the aircraft allocation per target type.
2. Assign bombers (for example Ju 87s) and escorts (Bf 109s).
3. Set the escort split: % tied, % free, and return escort.

Free escorts sweep ahead and high; tied escorts stay with the bombers. The 109's 20 minutes over
England limit how deep you can go; London is about its edge.

**Following a raid.** Raid tokens show a yellow band (raid ID), a red band (aircraft count) and a
blue band (height in thousands of feet). Afterwards, read the **Mission Folder** and **Gruppe
Diary**, and adjust for the next period.

---

## 9. The aircraft and their controls

**Primary controls**

| Control | Keys |
|---|---|
| Pitch | **Up** (nose down) / **Down** (nose up) |
| Roll | **Left** / **Right** |
| Rudder | **Num0** / **Num.** |
| Keyboard sensitivity | **K** / **Shift+K** |
| Throttle | **3**–**9** = 30–90 %, **0** = 100 %, **`** or **\\** = zero, **=** / **-** = ±1 % |
| Through the throttle gate (boost cut-out) | **0** |
| Prop pitch | **Shift+9** maximum pitch (fewest rpm), **Shift+0** minimum pitch (most rpm), **Shift+-** / **Shift+=** step up / down |
| Bf 110 engine select (both, port, starboard) | **E** |

There are no throttle keys for 10 % and 20 %: **1** and **2** are aileron trim, and 10 % sits on a
joystick button.

**Trim**

| Trim | Keys |
|---|---|
| Elevator | **Ctrl+Home** forward, **Ctrl+End** aft (with BDG the wheel keeps turning, faster, while held) |
| Aileron | **1** / **Home** / **Insert** / **Ctrl+PgUp** right; **2** / **End** / **Delete** / **Ctrl+Insert** left |
| Rudder (where fitted; not on the 109) | **Ctrl+PgDn** right, **Ctrl+Delete** left |
| Zero all trim | **Alt+End** |

**Systems**

| System | Keys |
|---|---|
| Flaps | **F** up one stage, **V** down one stage. The Spitfire has up and down only; the others have 4 stages. |
| Undercarriage | **G**. Lowering it fast can damage the hydraulics. |
| Emergency undercarriage (one gas bottle, once) | **Ctrl+G** |
| Wheel brakes | **,** left, **.** right |
| Air brake (Ju 87 dive brakes) | **D** |
| Canopy | **O**. The 109 and 110 canopies are hinged and tear off if opened in flight. |
| Fuel gauge selector | **Ctrl+F** |
| Eject / bail out | **Ctrl+E** |

**Interactive cockpit.** It works only with *Engine management: Manual*.
1. Press the **right mouse button** for the cockpit pointer (white arrow); hovering shows each
   control's name.
2. Click switches; hold buttons; drag levers and wheels.
3. Press the right button again to go back to view panning.

It does not work while paused.

---

## 10. Engines, propellers and starting

| Engine | Merlin II/III (Spitfire, Hurricane) | DB 601A (Bf 109, Bf 110) | Jumo 211 (Ju 87) |
|---|---|---|---|
| Rated power and altitude | 990 hp at 16,250 ft | 1,020 hp at 14,800 ft | 1,150 hp at 5,000 ft |
| Maximum | 1,020 hp, 3,000 rpm | 1,175 hp, 2,400 rpm | 1,200 hp, 2,400 rpm |
| Fuel | Gravity carburettor: **cuts under negative g** | Direct injection | Direct injection |

**Merlin limits** (the throttle settings apply to every type)

| Condition | Time limit | Throttle | rpm | Boost |
|---|---|---|---|---|
| Take-off to 1,000 ft | 1 min | 100 % | 3,000 | +6¼ |
| Climb | 1 hr | 90 % | 2,850 | +4¼ |
| Maximum continuous | — | 75 % | 2,650 | +3½ |
| Combat | 5 min | 100 % (through the gate) | 3,000 | +6 |

With manual engine management, exceeding these overheats the engine and damages it.

**Propellers.** Think of pitch as a gearbox: fine pitch is a low gear, coarse is a high one.

| Propeller | Fitted to | How to use it |
|---|---|---|
| **Two-pitch** | Early Spitfires and Hurricanes, Ju 87 | *Fine* for start, taxi, take-off and landing; *coarse* for climb, cruise and combat. Fine pitch with the throttle closed gives **windmill braking**. |
| **Constant-speed** | Later Spitfires and Hurricanes | Pick 1,800–3,000 rpm and the pitch follows |
| **Variable-pitch** | Bf 109, Bf 110 | Set continuously; can **feather** a dead engine |

Typical settings:
- Take-off: about 100 mph, 2,600 rpm, fine pitch.
- Climb and cruise: 200 mph, 2,600 rpm, coarse pitch.
- Glide with a dead engine: 100 mph, feathered.

**Cold start** (manual engine management; Spitfire procedure)
1. Undercarriage DOWN, with the green lights; flaps UP.
2. Both fuel cocks ON. Throttle about 20 %. Pitch fully forward.
3. Prime the engine: 3 strokes at +20 °C, up to 15 strokes at −20 °C.
4. Ignition (both magnetos) ON.
5. Hold the starter until the engine fires evenly.
6. Warm up and check each magneto (a drop of no more than 150 rpm). Keep the radiator below 100 °C
   before taxiing.

German aircraft use an **inertia starter**: hold the starter handle about 10 seconds to spin the
flywheel up, then release it to engage.

**Taxi and take-off**
- Taildraggers sit 12° nose-up, so **weave** to see ahead, or open the canopy (Spitfire,
  Hurricane, Ju 87).
- Set the elevator trim one division nose-down, rudder trim fully right, pitch fully forward, flaps
  up and brakes off.
- Open up to the gate, holding the swing with rudder.
- Raise the tail early for a view and less drag, or hold it down to track straight.
- After gear-up, check the red light. Don't climb before 140 mph.

**Recommended speeds (mph)**

| | Spitfire | Hurricane | Bf 109 | Bf 110 | Ju 87 |
|---|---|---|---|---|---|
| Take-off | 80 | 80 | 100 | 90 | 75–85 |
| Best climb | 180–200 | 165–185 | 180–190 | 180–190 | 120 |
| Cruise at 15,000 ft | 300 | 280 | 305 | 295 | 160 |
| Maximum at 15,000 ft | 346 | 315 | 340 | 330 | 230 |
| Maximum flaps / gear | 140 | 140 | 160 | 160 | 150 |
| Approach | 120 | 120 | 130 | 120 | 90 |
| Touchdown | 80 | 80 | 100 | 90 | 75 |
| Controllable dive | 400 | 400 | 380–400 | 380–400 | 400 |
| Stall (clean) | 75 | 80 | 100 | 90 | 70 |

At high speed the fabric control surfaces stiffen. Close the throttle and slow down before trying to
pull out of a dive.

With BDG you can **belly-land**, gear up, on land or water within 20 % of the landing speed, propeller
first.

---

## 11. Flying and fighting

**Stalls and spins**
- The **Novice** model cannot stall.
- The **Spitfire** shudders, then can flick over and enter a flat spin. Push forward at once, and
  reach 150 mph before pulling out.
- The **Hurricane** drops a wing sharply.
- The **Bf 109**'s slats give a gentle stall that never flat-spins.
- The **Bf 110** is dangerous on one engine; limit the flaps to 25° on a single-engine approach.
- The **Ju 87** is almost spin-proof.

To recover from a spin: stick central or forward, full opposite rudder, and pull out once the speed
returns. **Shift+S** is the spin-recovery cheat; hold it.

**Turning at 16,000 ft** (sustained, at the stall limit)

| | Speed | Turn rate | Radius |
|---|---|---|---|
| Spitfire | 226 mph | 18.9°/s | 307 m |
| Hurricane | 219 mph | 16.7°/s | 335 m |
| Bf 109 | 247 mph | 14.9°/s | 423 m |
| Bf 110 | 247 mph | 15.4°/s | 411 m |

**Climb at 16,000 ft:** Bf 109 2,640, Spitfire 2,500, Hurricane 2,420, Bf 110 2,010 ft/min.

The 1940 Farnborough comparison of the 109E and the Spitfire:
- equal speed in a shallow dive;
- the 109 slightly faster flat out;
- the Spitfire decidedly more manoeuvrable;
- the 109 better at pulling out of a steep dive and climbing away.

**Tactics by type**

| Aircraft | Tactics |
|---|---|
| **Spitfire** | Win the level turning fight. To dive, use a **split-S** (roll inverted and pull) so the Merlin stays fed. A **climbing turn** is a good defence. |
| **Hurricane** | Split-S as for the Spitfire. Head-on attacks on bombers were deadly but costly, with closing speeds near 500 mph. |
| **Bf 109** | Fight in the vertical. **Dive and zoom** out of the sun. **Up and under** from below and behind, slightly right (Galland's attack). **Bunt** into a steep dive to escape a Spitfire. |
| **Bf 110** | Stall turns to reverse. Form a **Lufbery** circle when bounced (the Ju 87 does this too). |
| **Ju 87 dive attack** | 10–15,000 ft, 160 mph, throttle back, dive brakes out (**D**). Dive at 80° to about 350 mph. The horn starts at about 4,300 ft and stops at 2,300 ft: release the bomb (**B**). The aircraft pulls out by itself (6 g limit). Retract the brakes, open the throttle and climb away. **L** looks through the floor window. |

Formations: RAF vics fly two wingspans apart. Luftwaffe Schwärme fly 200 yd apart and use the
**cross-over turn**.

**Weapons**

| Action | Keys |
|---|---|
| Fire | **Space** or the trigger |
| Drop a bomb | **B** |
| Next / previous weapon | **]** / **[** |
| Cycle weapons | **N** |
| Dump stores | **Ctrl+W** |
| Reload (cheat) | **Ctrl+R** |
| +1,000 ft (cheat) | **U** |

---

## 12. Views, radio and the gun camera

**Cameras**

| View | Key |
|---|---|
| Cockpit | **F7** |
| Outside / track | **F6** (**Shift+F6** fixed or floating camera) |
| Forward view, no cockpit | **F8** |
| Chase | **F9** |
| Satellite | **F10** |
| Impact | **F11** |
| Inside / outside toggle | **Backspace** |
| Configuration (Preferences) | **F12** |

**Padlocks**

| Target | Next | Previous | Reset |
|---|---|---|---|
| Enemy | **F1** | **Shift+F1** | **Ctrl+F1** |
| Friend | **F2** | **Shift+F2** | **Ctrl+F2** |
| Ground target | **F3** | **Shift+F3** | **Ctrl+F3** |
| Waypoint | **F4** | **Shift+F4** | **Ctrl+F4** |

- **F5** padlocks the subject of the last radio message.
- **Alt+F1** gives the AI enemy's view, and **Alt+F2** your escortee.
- **Ctrl+F6** is the outside reverse lock, which the getting-started guide calls "view padlocked
  enemy aircraft".
- **Enter** turns the padlock on and off; **Esc** resets the view.
- **Shift+T** draws a red box round the padlocked aircraft.

**Moving the view**
- **Num8/2/4/6** and the diagonals look round; **Shift** with them is fast.
- **Num5** resets. **Num-** zooms in and **Num+** out; **Ctrl+Num+** / **Ctrl+Num-** change the field
  of view.
- **NumLock** switches between panning and fixed views. **Num/** and **Num*** pan left and right.
- **Alt+1…9** repeat the panning keys and **Ctrl+1…9** the fixed looks.
- **Sticky looks** (while held): **PgUp** forward-right, **PgDn** back-right, **Alt+Num4** left,
  **Alt+Num6** right.
- **Ctrl+Num1…9** look up; **ScrollLock** turns look-up on for all sticky looks.
- **Q** and **W** lean out left and right.

**Situational aids**

| Key | Aid |
|---|---|
| **H** | Virtual threat indicator and horizon |
| **I** | Info line |
| **Shift+M** | Radio text on or off |
| **Ctrl+M** | Enemy aircraft on the 3-D map (`SEEMIGS`) |
| **Ctrl+V** | Your own voice on or off |
| **Ctrl+D** / **Shift+D** | 3-D detail up / down; the getting-started guide recommends lowering it over cities |
| **Tab** / **Shift+Tab** | Accelerated time in the cockpit / on the map |
| **M** | Map |
| **P** | Pause |
| **Ctrl+P** or **PrintScreen** | Screenshot |
| **Alt+X** | End the flight |

**Radio.** **R** opens the radio menu; choose with the number keys or the cockpit pointer.

| Menu | Shortcut |
|---|---|
| Group info | **Shift+1** |
| Precombat | **Shift+2** |
| Combat | **Shift+3** |
| Postcombat | **Shift+4** |
| Tower | **Shift+5** |
| FAC | **Shift+6** |

One-key calls: **A** "Any bandits?", **/** "Clear?" / "Clear", **Z** "Break!". With auto-vectoring
off, a leader is offered tactical choices when contact is made.

**Gun camera and replay**
- **C** toggles the camera and **X** discards the footage.
- *Trigger* mode records for 10 s after firing and for 1 minute after a bomb release.
- Save and view footage from a Quick Shot debrief, from the map toolbar's Replay in a campaign, or from
  **Replay** on the main menu.
- Replay has 12 icons, chosen with the number row: 1–0, then **-** and **=** set the start and end
  markers.
- From the main menu: **Replay**, click a recording in the list (`VIDEOS/*.cam`), then **Load**. The flight
  opens paused on the replay icons; press **4** (play/pause) to start. Verified in this port on 2026-10-04:
  a recorded landing plays back with its descent, circuit and final.
- `VIDEOS/replay.dat` holds only the latest flight, and the next flight overwrites it. Use the replay icons'
  **Save** (8) to keep a recording as a `.cam`.

---

## 13. Multiplayer

The port replaces DirectPlay with its own UDP transport on port **47624**. The four 2000-era services
(IPX, TCP/IP, modem, serial) are not used, and the game has no address box.

**Host**
1. Allow the port in: `sudo ufw allow 47624/udp`.
2. Start: `./BattleOfBritain-x86_64-<ver>.AppImage`.
3. Main menu → **Multi-Player** → **Create Game** (TCP/IP is chosen for you).
4. **Locker Room:** session name, player name, game type and side, then **Continue**.
5. **Ready Room:** the game is open. Press **Fly** when the joiner is ready, or fly first; joining in
   flight works.

**Joiner**
1. `BOB_DPLAY_HOST=<host's LAN IP> ./BattleOfBritain-x86_64-<ver>.AppImage`
2. **Multi-Player** → **Join Game** → click the session → **Select** → Locker Room → **Continue**.
3. Wait for the host's Fly, or press **Fly** if the host is already airborne.

**Game types**

| Type | Starts |
|---|---|
| **Death Match** | Implode, Pairs, Wheel, Same altitudes, Different altitudes, Explode, Implode/Explode, Flyby |
| **Team Play** | Flights with no / Luftwaffe / RAF / random advantage, Elements scattered or close, Singles implode or explode |
| **Quick Missions** | 15 of the Quick Shots, with AI, cooperative or adversarial |
| **Campaign** | the co-op campaign: guests fly in the host's campaign, on either side |

Any flyable aircraft can be used; gunner positions cannot.

**Campaign (co-op)**
1. **Host:** in the Locker Room choose **Campaign**, a side (RAF or Luftwaffe) and the phase, then **Continue**.
   You command your side on the campaign map as in single player.
2. **Opening the session:** take an interception (RAF) or plan raids (Luftwaffe) and frag a flight. The session
   opens at the campaign Ready Room, and guests receive your campaign state as they join.
3. **Guests:** join, then pick a seat in the host's flight on the Frag screen. A guest may also choose the other
   side, for a mixed-sides campaign.
4. **Fly:** everyone launches together. After the debrief, guests return to their Ready Room and the host to the
   map.

Tested on one PC with up to five players, RAF, Luftwaffe and mixed sides. Not yet tested: joining while the
host is on the map between sorties.

**Finding games: squeak (the Serious Games Week matchmaker)**
- Once per PC, choose the matchmaker: `sgw url http://<matchmaker-host>:8090`. The AppImage carries its own `sgw`;
  `pipx install git+https://github.com/sim-museum/squeak` puts one on your PATH.
- Hosting lists your session while it is open, and withdraws it when it closes. A campaign host's session opens at
  the campaign Ready Room.
- Join's session list includes the sessions the matchmaker lists, so a joiner needs no host address.
- A game is listed only on its day of the week (Tuesday for MiG Alley and Battle of Britain, in your own time
  zone). On other days the game still hosts normally; the log says why it isn't listed.

**Internet play**
- Lobby, chat, seat and launch messages are sent reliably: retransmitted until acknowledged, and delivered in
  order. Two players stay in step through 10% packet loss, which was measured with the loss simulator
  (`BOB_NET_LOSS=10`, a percentage). `BOB_NO_RELIABLE=1` turns this off.
- The host must accept UDP 47624 from outside (router port-forward). There is no NAT traversal.


**Ready Room**
- **Chat** to everyone, your side, or one player (click their name).
- **Set Up:** your preset radio messages, and your aircraft in Death Match and Team Play.
- **Edit Mission** (host) or **Brief** (guests) in Quick Missions.
- **Config:** the host controls difficulty.
- **Visitors' Book:** a player without the password clicks *Add my name*. The host opens the book and
  changes *Excluded* to *Accepted*.
- **Frag** (Quick Missions): pick a seat. KIA seats cannot be taken.

**In flight**
- **R** gives *Comms Msg Recipient* (all players or your team) and *Comms Message*.
- **Dead in Death Match / Team Play:** you are resurrected and climb on autopilot. **J** takes control
  at once; otherwise you get it back at 10,000 ft, or 2,000 ft above the highest aircraft. **Alt+S**
  cuts the death sequence short.
- **Dead in a Quick Mission:** back to the Ready Room. Take a free AI seat from the Frag.

**State of the port (2026-10-04, AppImage 261004).** Death Match, Team Play, Quick Missions and the co-op
campaign all reach a shared flight. All four have join in flight (except the campaign), chat, kill and assist
credit, and matching scoreboards. Reliable delivery and the squeak matchmaker are in. Untested: two PCs since
19 September, real internet play, and whether a guest sees the host's aircraft move (unproven in every mode).

**Two copies on one PC.** Use `BOB_HOME=~/bob2` for the second copy, and `BOB_WINPOS` to put the
windows on separate monitors (host `0,0`, joiner `1920,0`). A covered window drops to 1 fps under
XWayland and stalls the sync. The joiner reaches the 3-D world about 4 minutes in.

**For a bug report**
- Run both sides with `BOB_TRACE_DPLAY=1 BOB_TRACE_AGG=1 BOB_TRACE_PLAYERSQ=1`.
- A crash ends the log with `=== CRASH: signal N`; send the last ~100 lines of both logs.

---

## 14. Keyboard reference

The complete map is **[`KEYMAP.md`](KEYMAP.md)**, generated from the game's live key table (549
entries) by `tools/bob_keymap.py`:

```bash
BOB_DUMP_BINDINGS=/tmp/keys.txt BOB_RUN_INIT=1 BOB_BOOT_FRONTEND=1 ~/bob/build/bob   # into a cockpit once
python3 ~/bob/tools/bob_keymap.py /tmp/keys.txt ~/bob/SRC/H/KEYMAPS.H > KEYMAP.md
```

**How BoB's keys work**
- One **modifier state** at a time. Only **Alt** (either side), **Ctrl** (either side) and **Shift**
  (either side) are live modifiers.
- The table also holds entries for *ExtShift*, *Msg*, *ShMsg* and *Overlay* states. Nothing in the
  running code can enter them: Rowan removed that code on 27 July 2000. Those 151 entries are inert.
- 80 bound actions have no handler in this program. That includes every BDG 0.99 addition:
  - **S** padlock at screen centre;
  - **Ctrl+A** autopilot;
  - **Ctrl+I** other-aircraft view;
  - the **Alt+V/G/B/F** roving camera;
  - **Ctrl+F9–F12** and **Alt+Num+/-** WinAmp.

  The BDG manual describes them, but this build does not implement them.

**The keys you will use most**

| Flight | | Fighting | | Views and radio | |
|---|---|---|---|---|---|
| Throttle | 3–0, =/-, `\` = 0 | Fire / bomb | Space / B | Cockpit / outside | F7 / F6 |
| Prop pitch | Shift+9/0, Shift+-/= | Weapon next / prev / cycle | ] / [ / N | Chase / satellite | F9 / F10 |
| Gear / flaps | G / F up, V down | Padlock enemy / friend | F1 / F2 | Padlock on/off | Enter |
| Air brake (Ju 87) | D | Box target | Shift+T | Look back right/left | PgDn / hats |
| Wheel brakes | , / . | Reload (cheat) | Ctrl+R | Radio | R |
| Elevator trim | Ctrl+Home / Ctrl+End | Any bandits / clear / break | A / / / Z | Map | M |
| Engine select (110) | E | Gun camera | C | Pause / accel | P / Tab |
| Canopy | O | Eject | Ctrl+E | End flight | Alt+X |

---

## 15. Corrections to the older manuals

| Function | Older document | Program (live table) |
|---|---|---|
| Throttle 10 % and 20 % | **1** and **2** (2000 manual, getting-started steps) | **1** / **2** are **aileron trim** right / left. 10 % is on a joystick button; there is no 20 % key. **3**–**0** are 30–100 %. |
| Aileron trim | (not in the manual) | 1, Home, Insert, Ctrl+PgUp right; 2, End, Delete, Ctrl+Insert left |
| Sticky look forward / back / back-left / forward-left | Home / End / Delete / Insert, as in MiG Alley | Those keys are aileron trim in BoB. The looks are on joystick hats. Only **PgUp**, **PgDn**, **Alt+Num4** and **Alt+Num6** remain on the keyboard. |
| Outside view | "t, Backspace or joystick button 4" (2000 manual) | **Backspace** (inside/outside toggle) or **F6**. **T** shows or hides description text. |
| BDG keys: S centre padlock, Ctrl+A autopilot, Ctrl+I other-aircraft view, Alt+V roving camera, WinAmp, Ctrl+F frame counter | BDG 0.99 manual | **Not implemented in this build.** Ctrl+F is the fuel-gauge selector. |
| Instrument zoom Shift+F8, frame rate Shift+F | getting-started guide (Wine build) | Bound in the table (indices 331 / 333), but this program has **no handler** for them |
| Clear call | C (as in MiG Alley) | **/**. **C** is the gun-camera toggle. |
| Spin recovery | m S | **Shift+S** |
| Death-sequence skip | S | **Alt+S** |

Keys that are as documented: Alt+X exit, Ctrl+G emergency gear, Ctrl+E eject, F/V flaps, G gear, D
dive brakes, O canopy, E engine select, Ctrl+Home/End elevator trim, Ctrl+PgDn/Ctrl+Delete rudder
trim, Alt+End reset trim, Shift+9/0 prop pitch, Ctrl+F6 reverse padlock, Shift+T box target, Ctrl+D
detail.

---

## 16. Troubleshooting

| Symptom | What to do |
|---|---|
| The game will not start after a settings change | Delete `SAVEGAME/SETTINGS.CFG`. |
| Throttle keys 1 and 2 roll the aircraft | That is the BDG key table: 1 and 2 are aileron trim (**Alt+End** zeroes it). Use 3–0, =/-, or a throttle axis. |
| A BDG feature does nothing (S padlock, autopilot, roving camera) | Not implemented in this build ([section 14](#14-keyboard-reference)). |
| Cockpit switches won't respond | Engine management must be *Manual*. Right-click for the cockpit pointer, and don't pause. |
| No guns as gunner | PC Config → Controller → Gunner must not be *None*. |
| Joiner never sees the session | Check that the host is in the Ready Room and that UDP 47624 is open, and use `BOB_TRACE_DPLAY=1`. |
| Two copies on one PC lose sync | Separate monitors (`BOB_WINPOS`) and data folders (`BOB_HOME`). |
| Low frame rate over cities | Ctrl+D lowers detail; turn off town raises and individual clouds. |
