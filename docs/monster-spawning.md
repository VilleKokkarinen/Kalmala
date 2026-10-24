# Monster spawning implementation targets

User-directed planning, 2026-10-22. This is the fixed implementation target for
BACKLOG S01–S35. It supersedes the earlier provisional wording for this goal.
Implement these defaults without requesting further tuning approval. Routine
code structure is the working agent's choice. Report a genuine technical blocker
with evidence; do not turn ordinary tuning or implementation choices into questions.
Existing alpha acceptance scope, creature combat/loot stats and species biome
weights remain governed by `biome-enemies.md`. This goal changes spawn admission.

## S01 implementation map — 2026-10-24

This records the pre-change code map for S01. It is orientation for the ordered
implementation steps, not an acceptance waiver; the gaps below remain work for
their corresponding backlog items.

- **Keys, deterministic descriptors and IDs:**
  `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h` owns
  `FKalmalaWorldPopulationLayout::GetSpatialKey`, `DeriveSpatialSeed`,
  `GetSpawnBudget`, `BuildSpawnDescriptors` and `GetPersistentSpawnId`.
  Descriptors use the world seed, key, population kind and candidate index to
  derive stable positions, IDs and biome content. `IsSafeWildlifeLocation`
  filters generated wildlife candidates before they become descriptors.
  **S03 resolution:** the shared SpatialKeySizeCm is now 6,400 cm (64 m),
  matching this fixed target and the backlog. This changes deterministic
  descriptor positions and sparse IDs for population and discovery systems that
  share the layout. No generator revision or save schema change was made, as
  required by this goal.
- **Species selection and existing habitat filters:**
  `AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome` maps a descriptor seed
  and biome to its existing species share. The server-side helpers
  `KalmalaGameMode::TryGetWildlifeArchetypeForSpawn` and
  `FilterEligibleWildlifeSpawns` in
  `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp` apply species habitat,
  water, shoreline and traversability rules; `ActivatePopulationKey` also
  applies the existing hard-enemy start exclusion. Species values and weights
  remain documented in `docs/biome-enemies.md`.
- **Current per-key budgets:**
  `GetSpawnBudget` computes the base wildlife, harvest and hazard budgets from
  sampled fields. `FKalmalaBiomeExpansionContract::ApplyPopulationBudget` in
  `Source/KalmalaWorld/Public/KalmalaBiomeExpansionContract.h` applies the
  existing biome multiplier. `AKalmalaGameMode::ActivatePopulationKey` applies
  that budget to its descriptor list, then limits materialized wildlife to the
  per-key actor budget, with the existing Tundra pack-room adjustment in
  `FKalmalaWolfPackContract`. There is no cross-key species cap or shared
  pending-capacity reservation yet.
- **Current activation and scheduling:**
  `AKalmalaGameMode::Tick` periodically visits each player controller's current
  pawn key and calls `ActivatePopulationKey`. That authority-checked function
  materializes the key's wildlife, harvest and hazard descriptors once;
  `ActivePopulationSpatialKeys` is a nine-key session set. It does not derive
  the 96 m union around living players, run a recurring per-key attempt
  schedule, or remove keys/actors when players leave. No wildlife unregister or
  living-slot unload cooldown path is present in this activation flow. A local
  source scheduler is not present either.
- **Spawn and defeat lifecycle entry points:**
  `ActivatePopulationKey` creates `AKalmalaWildlifeSpawn` and
  `AKalmalaHazardSpawn` actors and binds their `OnDefeated` delegates to
  `AKalmalaGameMode::RecordDefeatedSpawn`. The actors' `DefeatServer` methods
  enforce authority; wildlife first routes species rewards through the
  `Record*DefeatReward` GameMode methods. The bound callback records the stable
  ID through `UKalmalaWorldPopulationSaveGame::MarkDefeated` and saves it.
  `UKalmalaWorldPopulationSaveGame` also owns durable pending reward claims,
  which `DeliverPendingWildlifeRewards` retries from the authoritative GameMode
  tick. There is no distinct defeat cooldown: existing defeated IDs are
  excluded when a key is activated.

The current activation map is server-owned, but it is eager and one-shot. S02
records the existing defeat and reward callbacks and the missing living-actor
unload lifecycle before any unload/reload behavior is added.

## S02 lifecycle integration map — 2026-10-24

This is a source map for the fixed lifecycle above. It adds no runtime hook or
save data.

- **Permanent defeated-slot depletion:** `AKalmalaGameMode::ActivatePopulationKey`
  derives stable descriptor IDs and skips wildlife/hazard IDs already present in
  `UKalmalaWorldPopulationSaveGame::DefeatedSpawnIds`. Spawned wildlife members
  and hazards bind `OnDefeated` to `AKalmalaGameMode::RecordDefeatedSpawn`.
  Authority-only `DefeatServer` implementations broadcast only after accepting
  the defeat; the GameMode callback calls `MarkDefeated` and writes the matching
  population save slot. Wildlife reward handlers also mark and save their IDs as
  part of the reward transaction before `OnDefeated` is broadcast. Thus unload,
  later key activation, reconnect and restart must never remove a defeated ID;
  each wolf member retains its own stable depleted ID.
- **Living-actor unload/reload:** `ActivatePopulationKey` only adds keys to
  `ActivePopulationSpatialKeys`; it does not retain actor references or remove
  actors/keys. The wildlife and hazard classes have no `EndPlay`, destruction or
  unregister lifecycle callback, and GameMode has no actor-destroyed registry
  for them. Add a GameMode-owned live-actor registry alongside S10, then route
  intentional unload through an explicit GameMode transaction. Generic actor
  destruction is not enough to infer an unload: defeat, pack-spawn rollback and
  world teardown have different outcomes. A living actor that is eligible to
  unload must first be outside every living player's 96 m radius; record its
  stable ID's 30 s cooldown in session state that survives key unload, without
  calling `RecordDefeatedSpawn`, a reward handler or any save mutation. Recreate
  only the same descriptor after expiry and all gates pass. Keep the actor
  registered and charged to its home-key budget while it roams.
- **Reward escrow:** `AKalmalaWildlifeSpawn::DefeatServer` routes credited
  species rewards through the matching authoritative
  `AKalmalaGameMode::Record*DefeatReward` handler before broadcasting
  `OnDefeated`. These handlers persist the defeated ID and, for a credited
  authenticated owner, queue a stable owner-bound claim in
  `UKalmalaWorldPopulationSaveGame::PendingWildlifeRewards`; optional gem
  payloads are attached to that same claim. `AKalmalaGameMode::Tick` retries
  `DeliverPendingWildlifeRewards` every 0.5 s. Delivery requires the authenticated
  owner to be present and enough inventory capacity; the durable claim remains
  available when delivery cannot complete. Keep this path intact: unload,
  suppression, failed spawn and rollback must neither create nor consume claims.

Implementation consequence: the next live-actor and unload work must separate
defeat from intentional unload at the GameMode boundary. Reuse the existing
defeat ledger and escrow unchanged; cooldowns belong to session-only GameMode
state and do not require a save-format change.

## S04 active player source — 2026-10-24

`AKalmalaGameMode::GetLivingPlayerPawnsForPopulation` is the server-only source
for the existing population-key activation loop. It enumerates the current
world's `PlayerControllers` and keeps only controllers with a valid same-world,
active non-spectator `PlayerState` and a valid same-world `AKalmalaCharacter` that
is still possessed by that controller and has finite health above 1.0. Returned
pawns are ordered by PlayerState ID, with controller object path as a stable
within-world tie breaker, and duplicate pawn pointers are removed. The current
server controller list excludes disconnected controllers; inactive PlayerStates
are also rejected. S04 itself did not derive the 96 m neighborhood union,
deduplicate active keys, or add an attempt scheduler.

## S05 loaded-key candidate union — 2026-10-24

`AKalmalaGameMode::GetPopulationSpatialKeyCandidates` filters the existing
`ActivePopulationSpatialKeys` set, which contains keys already materialized by
`ActivatePopulationKey`. A loaded key is included once if its 64 m square has
an inclusive horizontal intersection with any living player's 96 m radius.
Results use lexicographic X/Y key order and remain candidates only; this query
does not activate, remove, or otherwise materialize keys. The current normal
`Tick` path still materializes each living player's containing key, subject to
the unchanged nine-key cap, so nearby but unloaded cells are not added here.
The returned union is input to the S06 cadence state. That scheduler state drops
keys and their due-time entries when they leave the current player neighborhood.

## S06 per-key cadence — 2026-10-24

`AKalmalaGameMode::Tick` updates cadence from the S05 loaded-key candidate union
on the server. Each newly active key receives a stable initial phase from its
wildlife spatial-seed hash modulo 20, multiplied by 0.25 seconds. Due
keys are returned in lexicographic order; servicing a key advances it to
`now + 5` seconds. Keys leaving the player neighborhood are removed from the
active due set and their per-key due entry is discarded. Reactivation derives a
fresh phase from the same key seed, so an inactive key cannot accumulate missed
attempts. This cadence state is separate from S02's 30 s living-creature unload
cooldown, which remains keyed by stable spawn ID and survives key unload. S07
adds the bounded 0.25 s wake; S06 and S07 add no descriptor generation or spawn
admission.

## S07 bounded scheduler wake — 2026-10-24

The server checks for a scheduler wake every 0.25 seconds. When a frame arrives
late, the next wake is set to `now + 0.25` rather than replaying missed ticks.
Each wake selects at most four due keys in stable order. Each key carries a
candidate budget of at most four, and the total candidate budget is capped at
16. S07 records only this bounded key/budget plan; S08 generates descriptors and
S14 connects the plan to evaluation and admission.

## Spatial scheduling and random rolls

- Retain invisible 64 m square spatial keys, floor division for negative X/Y,
  existing world bounds, 9 active population keys and 25 terrain patches. No
  separate streaming grid or increase to existing budgets.
- Use the existing active-key selection and union of living players. Only loaded
  keys intersecting a player's 96 m horizontal radius are eligible for attempts.
  Player IDs, key order and duplicate removal use stable server ordering.
- Scheduler wake interval: 0.25 server seconds. Process at most 4 due keys per
  wake, 4 candidate descriptors per key and 16 candidates per wake in total.
  A key becomes due every 5 seconds. Initial phase is its existing deterministic
  spatial-seed hash modulo 20, multiplied by 0.25 seconds. Carry due keys fairly
  round-robin; after servicing a key set its next due time to now + 5 seconds.
  Never replay missed checks. No players means no attempts.
- Use existing deterministic candidate positions, species weights and stable
  spawn IDs. Ring generation means selecting eligible existing descriptors,
  never moving or rerolling the position/species behind an existing identity.
  Rotate a per-key candidate cursor so the four-candidate budget cannot starve
  later descriptors. Advance the cursor even when a descriptor is rejected.
- For each otherwise eligible descriptor make one seeded 25% admission roll
  per serviced check. Hash world identity, key, spawn ID and the key's serviced
  check ordinal in a dedicated spawn-roll domain; use an unbiased integer roll
  0–99 and accept 0–24. Ordinal advances once per serviced check, not per player.
  No repeat roll in the same check. Empty biome-weight slots stay empty.
- Horizontal distance to at least one living player must be in [32, 96] m;
  distance to every living player must be at least 32 m. Exact boundaries pass.
  Every Wolf member must pass the distance and placement checks.

## Counts and caps

- Count living actors by current horizontal position within an inclusive 96 m
  radius of the candidate, across all loaded keys and all spawn sources. Include
  pending reservations. Do not multiply limits by player count or key overlap.
- Per-species limits: 4 for Boar, Deer, Rat, Badger, Viper, Mireling, Death Adder,
  Mosquito swarm, Lynx and Horned Goat; 1 for Moon Bear, Giant Snake, Peat Husk,
  Polar Bear, Thunder Sentinel and Reef Maw; 6 individuals for Wolf. One swarm
  is one actor. No additional group quota beyond these and existing budgets.
- Existing post-biome per-key wildlife/hazard actor budgets remain additional
  limits. Reserve against the descriptor's home key even if actors later roam.
  Use the existing three-actor Tundra floor for Wolves. Caps may reduce density;
  never enlarge a budget to reach the species cap.
- Reserve one actor or all three Wolves in a single server transaction. Count
  against every pack member's candidate-centered area. If any member cannot
  fit or spawn, roll back the entire new pack and release every reservation;
  no defeat, loot or save callback may run for rollback actors.

## Nine gates and physical placement

Evaluate in this order: active-key/scheduler budget, seeded admission roll,
player distance, species habitat, time, weather, progression, caps, placement.
Return a distinct first rejection reason. All decisions are server-owned.

- Time, weather and progression are explicitly **unrestricted for all current
  species**. These three gates return pass. Provide internal policy seams and
  fixture overrides for rejection tests; do not build a new clock, boss system,
  public settings UI or new species restrictions to implement this goal.
- Preserve biome weights, dry-shore rules and the 150 m generated-start
  exclusion for hard enemies. Land slope is at most 32 degrees, inclusive.
- Reef Maw uses its existing Ocean-only, non-lake minimum-depth constant and
  swimming support; do not introduce a second competing depth value. A spawn
  must fit its existing body collision shape fully in water with no terrain or
  solid overlap. Other species retain their existing locomotion support.
- Use the actor's actual collision shape, adding no guessed generic body size.
  Require nonpenetrating placement and the existing species support query.
  For land actors with no navigation mesh requirement, a valid generated-ground
  support sample and clear collision suffice; do not install navigation tooling.
- Reject any species beneath a solid construction roof: vertical trace from
  body top upward for 24 m, ignoring the proposed actor itself. Trees/canopy and
  non-solid cosmetics are not construction roofs. Existing world bounds apply.

## Camp suppression

- Workbench and Forge: inclusive 16 m horizontal cylinder, inclusive vertical
  range [-16, +16] m relative to the placed actor origin. Require an existing,
  completed, non-destroyed station; fuel and roof are irrelevant.
- Camp Bed: inclusive 16 m 3D sphere about the placed origin, whenever intact.
- Campfire and Standing Light: inclusive 16 m sphere only while server state is
  Lit with fuel remaining. Smouldering, extinguished or empty sources do not
  suppress. Changes apply at the next attempt without a separate polling delay.
- No other buildable suppresses. Floors/walls/roofs do not emit fields; roofs
  still reject direct overhead placement. Destroyed/removed sources stop
  suppressing. Suppression affects new spawns, never existing enemy movement.

## Lifecycle and persistence

This goal retains the existing finite deterministic population and defeat
ledger. It schedules admission of surviving slots; it does not add renewable
enemy farming or change the development save format.

- Defeated stable spawn IDs stay depleted across unload, reconnect and restart;
  respawn cooldown for defeated slots is disabled, not an unspecified timer.
  Natural checks can admit eligible slots that have not been defeated.
- Unloading a living actor is not a defeat and grants nothing. Its stable slot
  can reactivate at its original descriptor after a 30-second session cooldown,
  subject to all gates. Do not unload a roaming actor while it remains within
  96 m of any living player; continue counting it against its home-key budget.
- Session-only state includes cursors, roll ordinals, due times and unload
  cooldowns. Retain the 30 s living-creature cooldown, keyed by stable spawn ID,
  across key unload for the session. Remove per-key scheduler due state while a
  key is inactive; reactivation assigns its deterministic initial phase again.
  Restart resets session state. No offline catch-up.
- Preserve durable reward escrow and authenticated-owner claims. Failed spawn,
  rollback, unload and suppression produce no reward. Never clear a saved defeat
  ID to make room. No new save fields, migrations or generator revision bump.

## Environmental spawner policy

- Reusable server policy and test fixture only; no new placed world content.
- Local limit: 2 living creatures owned by the source, within inclusive 24 m
  horizontal distance of its origin. Escaped creatures still count against
  shared species/home-key budgets even when outside the local radius.
- Check once per 8 seconds, at most one creature per check, at most 4 fixed
  candidates within 12 m of source origin. Use the same 25% admission roll,
  player ring, all-player exclusion, habitat, unrestricted time/weather/
  progression gates, placement and shared caps. Camp suppression applies.
- Use deterministic source/slot IDs and the same permanent-depletion policy;
  no replacement after a slot's defeat. Fixtures have two slots. Source-owned
  reservations share natural-spawn budgets and the global 16-candidate wake
  budget. Inactive sources accrue no missed attempts.

## Acceptance

S30–S33 cover every numerical boundary, each first rejection reason, duplicate
players/keys, roaming counts, pack rollback, station changes and slot/reward
deduplication. S34 selects the existing setup-guide build/test commands for the
affected targets and supplies a deterministic two-peer fixture. S35 executes
those commands and verifies two players, overlapping/disjoint neighborhoods,
unload/reload and restart. Required result: no cap violation, duplicate actor or
reward, no client-authoritative spawn choice, and no wake over 16 candidates.
Record actual results; do not mark the goal complete on source inspection alone.
