# M5 vertical-slice runbook

This runbook defines the first M5 baseline: a fresh-player, tool-free
20–30-minute co-op session using the existing Kalmala systems. It is a test
charter, not a prescribed route or quest. The server's generated identity and
the players' choices determine where the session goes.

M4 is complete under its revised focused-regression acceptance. This runbook
defines the later M5 tool-free session; it does not reintroduce a rendered
two-player M4 acceptance gate.

## Session rules

- Start from a fresh local user directory and a normal packaged Windows
  Development build once packaging verification is available.
- Use a listen server with one joining player for the co-op baseline. A solo
  run may validate local flow, but cannot satisfy the two-player acceptance.
- Launch and play normally. Do not use console commands, developer-only
  verification flags, fixed fixture coordinates, editor tools, or scripted
  teleportation as acceptance inputs.
- Let the host select the immutable world identity. The joining client may
  start with a conflicting local seed, but must use the replicated server
  identity for the shared world.
- Players choose their camp site, heading, optional discoveries, encounters,
  and return timing. The run must not require a road, corridor, quest chain,
  mandatory camp, or fixed order of points of interest.

## Player-facing walkthrough

The timings are guidance for a readable session, not hard gates.

### 0–3 minutes — Start and orient

The host starts a listen session and the second player joins through the normal
game flow. Both players confirm that they can move, look, jump with **Space**,
and sprint while holding **Shift**. The host and client should see the same
generated terrain and each other's relevant player presentation.

Use **M** only when useful for local orientation; **R** recentres the local
map. The minimap and expanded map are orientation aids, not route instructions.

### 3–8 minutes — Choose and prepare a camp

The players choose a practical nearby site from the generated terrain, gather
the available materials through the normal interaction path (**E**), and open
Camp crafting with **B**. They craft and place a starter hearth and simple
construction as resources allow. Placement is paid and server-validated; the
client supplies no transform or outcome.

Light the hearth through the crafting interaction, then add at least one
roofed or wind-protected position. The players should be able to read the
hearth, shelter, Wet, warmth, inventory, and construction state without relying
on colour alone. A camp is a player choice and is not a reserved M5 location.

### 8–15 minutes — Travel and gather by choice

The players leave camp in a self-selected direction, gather at least one useful
resource, and observe a meaningful local tradeoff such as water, cover, slope,
weather exposure, shelter, or distance from camp. They may split up or remain
together. The server remains authoritative for gathering, terrain, weather,
exposure, inventory, and construction.

The group may open or close the local map during travel. No map surface may
reveal hidden population, undiscovered rewards, another player's private pins,
or a recommended destination.

### 15–24 minutes — Optional encounter and discovery choices

The players engage with whatever optional generated content they choose. The
M5-complete version of this run should cover the Mireling, boar, and deer
archetypes, a server-validated discovery or scroll reward, and all four learned
support effects: Mending, Hearth Shield, Bear's Vigor, and Deer Call. No effect
may deal direct damage or replace ordinary preparation and combat.

Use normal player intent only: basic attack is **Left Mouse Button** (or the
controller right shoulder), and support activation uses the player-facing
learned-effect panel. Keyboard **1–4** or the matching per-effect controller
D-pad direction selects an effect; **Q** or the controller top face button
requests activation. That local UI may send only the selected allowlisted
effect and a monotonic request sequence; it must not expose private learned
effects, discovery acknowledgement, wildlife targets, damage, rewards, or
server-selected outcomes. Deer Call can influence only existing nearby deer.
Targets, damage,
durations, rewards, discovery IDs, and behaviour outcomes remain server-selected.

The other player must receive only relevant replicated presentation. Learned
effects and discovery acknowledgement remain private to the entitled player;
inventory rewards remain owner-only.

### 24–30 minutes — Prepare and return

The players choose whether to return to their camp, shelter from weather, use
the hearth, store gathered materials, or stop after an optional discovery.
Their final state should show that recovery and preparation are viable choices,
not a mandatory travel gate.

For the M5 acceptance run, reconnect the entitled player through
the normal session flow and confirm that the matching-world progression reward
and the relevant sparse defeat/discovery state remain available exactly once.
The remote player must not receive the entitled player's private reward or
learned-effect acknowledgement.

## Fresh-player acceptance matrix

This matrix separates what the players do from what must be observable in the
session. It is the evidence definition for the first M5 baseline child; it does
not turn any row into a mandatory route or a hidden server query.

| Phase | Normal player choice | Required observable evidence | Authority/privacy pass condition |
| --- | --- | --- | --- |
| Start | Host creates a session; a second player joins | Fresh spawn, movement/look/jump/sprint, server identity, matching terrain and relevant player presentation | The host owns world identity; the client accepts the replicated identity and sends no world-selection data |
| Camp | Players choose a nearby site and gather/craft/place what they can afford | At least one gathered resource, paid hearth or construction result, readable inventory, hearth, shelter, Wet/warmth, and construction state | Server validates interaction, recipe, payment, placement, fire, and exposure; failed actions do not consume resources |
| Travel | Players choose a heading and whether to split up, gather, map-check, or return | A self-selected position or direction plus one environmental/resource tradeoff; local map/minimap remains an orientation aid | No road, corridor, fixed coordinate, recommended destination, hidden population, or private pin is revealed |
| Optional content | Players choose whether and where to engage with wildlife or a discovery | Readable combat/discovery/support feedback; the M5 acceptance covers all three creatures and four support effects without direct-damage magic | Server selects targets, damage, behaviour, rewards, and learned effects; owner-only data stays private and relevant actors replicate normally |
| Return | Players decide when to shelter, use the hearth, store materials, or stop | Camp/recovery state is visible and the players can return without a hard travel gate | Hearth, inventory, construction, exposure, and reward state remain server-owned; recovery does not require a prescribed site |
| Reconnect | Entitled player reconnects normally to the matching world | The accepted reward/defeat/discovery state is present exactly once and the remote player still lacks private acknowledgement | Matching immutable identity restores only the entitled player/world delta; duplicate claims and cross-world state are rejected |

The first M5 baseline run records every applicable row. Optional-content rows
remain part of final M5 evidence; they do not create a separate M4 gate.

## Acceptance evidence

The observer records player-visible evidence for one run:

- fresh start and normal input, with no developer tools;
- two-player listen-session identity and matching generated presentation;
- freely chosen camp, paid construction, hearth state, shelter/weather cues,
  and readable inventory feedback;
- a self-selected travel choice with gathering or discovery feedback;
- optional encounter/support choices, including the M5 creature/effect set;
- owner-only rewards and learned effects, relevant-peer combat/exposure state,
  and no hidden-content leak;
- return, reconnect, and matching-world persistence without duplicate reward;
- completion within 20–30 minutes, with no prescribed route or mandatory site.

Developer logs and focused automation may diagnose failures, but they cannot be
the sole proof of the final M5 acceptance.

## External native-surface skip

The packaged co-op run is the preferred M5 closure. It may be skipped only
when the active validation environment cannot expose a targetable native
Windows application surface, not because gameplay coverage is inconvenient or
because an automated fixture is available. The skip requires all of the
following:

- the M5 release-regression and package-smoke aggregate has passed;
- at least three normal packaged launches have remained alive in the active
  user session with `MainWindowHandle=0`, including a fresh-profile windowed
  launch;
- the available computer-use inventory has no targetable native game window;
- every attempt, launch mode, process/session observation, and cleanup result
  is retained in `BACKLOG.md` and `PROGRESS.md`.

Marking this skip closes the M5 execution queue only. It must explicitly state
that it does not pass the player-visible 20–30-minute co-op loop, normal peer
joining or reconnect, physical input, audible quality, packaged persistence,
or long-session balance. M6 must restore a native surface and run this
unchanged player-facing charter before treating any of those deferred checks as
accepted. No developer fixture, offscreen capture, console command, or
scripted gameplay may substitute for the skipped run.

## Authority and persistence boundary

This runbook changes no runtime contract. The server owns world identity,
terrain, population, targeting, damage, weather, exposure, construction,
inventory, discoveries, learned effects, rewards, and sparse world/player save
deltas. Clients provide normal movement and narrow intent only, and render
their own local settings/map state. Settings, map presentation, and tutorial
prompts remain local and cannot mutate replicated gameplay state.
