# Monster spawning implementation targets

User-directed planning, 2026-10-22. This is the fixed implementation target for
BACKLOG S01–S35. It supersedes the earlier provisional wording for this goal.
Implement these defaults without requesting further tuning approval. Routine
code structure is the working agent's choice. Report a genuine technical blocker
with evidence; do not turn ordinary tuning or implementation choices into questions.
Existing alpha acceptance scope, creature combat/loot stats and species biome
weights remain governed by `biome-enemies.md`. This goal changes spawn admission.

## Spatial scheduling and random rolls

- Retain invisible 60 m square spatial keys, floor division for negative X/Y,
  existing world bounds, 9 active population keys and 25 terrain patches. No
  separate streaming grid or increase to existing budgets.
- Use the existing active-key selection and union of living players. Only loaded
  keys intersecting a player's 84 m horizontal radius are eligible for attempts.
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
- Horizontal distance to at least one living player must be in [36, 84] m;
  distance to every living player must be at least 36 m. Exact boundaries pass.
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

- Workbench and Forge: inclusive 18 m horizontal cylinder, inclusive vertical
  range [-12, +12] m relative to the placed actor origin. Require an existing,
  completed, non-destroyed station; fuel and roof are irrelevant.
- Camp Bed: inclusive 18 m 3D sphere about the placed origin, whenever intact.
- Campfire and Standing Light: inclusive 18 m sphere only while server state is
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
  84 m of any living player; continue counting it against its home-key budget.
- Session-only state includes cursors, roll ordinals, due times and unload
  cooldowns. Retain that state across key unload for the session; restart resets
  it and starts admission after the normal stagger. No offline catch-up.
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
