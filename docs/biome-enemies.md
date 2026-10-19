# Biome enemy expansion proposal

Draft for manual review, 2026-10-19. Implement through A8a in
[BACKLOG.md](../BACKLOG.md), before A9 gems and special gear.
These additions extend the current three-archetype roster; they do not change
historical vertical-slice acceptance or claim that these enemies exist today.
Names, stats, spawn weights and loot below are concrete proposals for editing.

## Proposed roster

Keep existing boars, deer and Mirelings. Add thirteen enemies, one complete
enemy per increment. All damage below is one authored server-resolved hit,
before existing defenses; no client-authored damage or offensive player magic.

| Biome | Enemy / ID | Role and attack | Health / Armor | Hit damage | Guaranteed loot |
| --- | --- | --- | --- | --- | --- |
| Meadows | Brush Rat (`BrushRat`) | Small scavenger; bites only when approached within 3 m or attacked | 35 / 0 | 6 Stab | 1 Bone Fragment |
| Elderwood | Forest Wolf (`ForestWolf`) | Fast hunter; short, visible forward lunge | 80 / 0 | 15 Slash | 2 Bone Fragment |
| Elderwood | Elderwood Bear (`ElderwoodBear`) | Rare hard enemy; slow overhead paw strike, punishable recovery | 250 / 5 | 35 Crush | 3 Bone Fragment |
| Shimmering Lakes | Reed Snake (`ReedSnake`) | Snake hiding in dry shoreline reeds; visible coil, narrow lunging bite, then retreats; no poison system | 50 / 0 | 10 Stab | 1 Bone Fragment |
| Shimmering Lakes | Reed Lurker (`ReedLurker`) | Rare hard, original shoreline beast; two-legged rush and broad forelimb swipe on land | 160 / 3 | 25 Crush | 2 Bone Fragment |
| Mossy Mire | Bog Adder (`BogAdder`) | Camouflaged land hunter; short bite followed by retreat | 70 / 0 | 14 Stab | 1 Bone Fragment |
| Mossy Mire | Mire Mosquito (`MireMosquito`) | Fragile flying pest; audible hover, short proboscis stab, then retreats | 25 / 0 | 8 Stab | None |
| Mossy Mire | Peat Husk (`PeatHusk`) | Rare hard, original peat-and-wood creature; slow sweeping arm | 220 / 6 | 30 Crush | 1 Amber |
| Freezing Tundra | Snow Lynx (`SnowLynx`) | Mobile predator; telegraphed leap, long recovery if it misses | 120 / 0 | 22 Slash | 2 Bone Fragment |
| Freezing Tundra | Tundra Bear (`TundraBear`) | Rare hard bear variant; committed charge, then retreat to reset | 300 / 6 | 40 Crush | 3 Bone Fragment |
| Thunder Mountains | Ridge Goat (`RidgeGoat`) | Territorial wildlife; clear horn-charge windup | 120 / 2 | 20 Crush | 2 Bone Fragment |
| Thunder Mountains | Crag Sentinel (`CragSentinel`) | Rare hard, original stone creature; short step and downward slam | 280 / 10 | 38 Crush | 3 Stone |
| Ocean | Reef Maw (`ReefMaw`) | Rare swimming predator; visible approach and single bite against swimming players | 180 / 2 | 28 Stab | 3 Bone Fragment |

New animal enemies deliberately have no meat/hide drops in this proposal:
do not grant Deer Meat/Boar Hide from unrelated species or invent extra item
identities during implementation. The listed loot uses existing items only.
Any later carcass materials require a separate reviewed item/recipe/icon plan.

## Spawn and combat rules

- Retain existing world-seed, biome suitability, relevance and actor budgets.
  Add variety within existing candidate slots, not additional unbounded actors.
- Proposed selection: Meadows 90% existing roster / 10% Brush Rat. Each other
  land biome: 50% existing eligible roster / 40% new common enemy / 10% new
  hard enemy. If there is no eligible existing entry, leave that share empty;
  do not import creatures from another biome. Each new spawn is one enemy.
- Mossy Mire splits its common share: 50% existing roster / 20% Bog Adder /
  20% Mire Mosquito / 10% Peat Husk. No additional actor budget or swarm spawns.
- Ocean: use 10% of eligible aquatic candidate slots for Reef Maw; others
  stay empty. This task is blocked until aquatic navigation, combat and reward
  support are approved. It targets swimmers, never skiff hulls or occupants.
- Require dry traversable ground for land enemies. Lakes means dry shores,
  not deep water. Hard enemies cannot spawn within 150 m of generated start.
  No new compulsory encounters, boss keys, raids or progression locks.
- Common enemies telegraph attacks for 0.5 s and recover for 0.8 s; hard
  enemies telegraph for 1.0 s and recover for 1.2 s. One damage application per
  accepted attack; server rechecks life, range and line of sight at impact.
  Use existing melee reach for the matching body size; if no suitable authored
  reach exists, propose it for review rather than guessing during coding.
- Mire Mosquito uses an original mosquito-like silhouette and audible wing
  cue. Hover 0.5–2.5 m above local terrain, telegraph for 0.5 s, dash at most
  3 m with 0.75 m impact reach, then recover/retreat for 1.2 s. One server-owned
  Stab hit per attack; normal visibility and collision block walls/closed doors.
  No poison, lifesteal, instant kill, gem roll or new loot item. Flying pursuit
  must remain bounded by the shared 25 m anchor leash and existing relevance
  budgets; this task includes required flight/collision/peer support, rather
  than assuming ground navigation can fly. It remains within Mossy Mire and
  cannot chase across deep open water. Validate aerial hit readability and
  shelter blocking in its acceptance checks.
- Pursuit ends beyond 25 m from the spawn anchor. Return home instead of
  teleporting into an attack. No enemy crosses deep water without its own
  accepted aquatic support. Variants may share locomotion foundations, but
  require distinct readable silhouettes and the listed timing/damage.
- Extend existing server-owned wildlife lifecycle, owner reward attribution,
  relevant-peer replication and depletion policy. Persist stable credited-death
  and loot claim identities so reload/duplicate requests cannot grant twice.
  Reject invalid attacks unchanged; preserve unclaimed rewards on full packs.

## Moonstone transfer

Move Moonstone exclusively to **Elderwood Bear**, with a **2% chance of exactly
1 per credited defeat**, otherwise 0, alongside its 3 Bone Fragment.
Use the authoritative enemy kind and Elderwood spawn identity, not the biome
where a player lures it. Boars, deer, Forest Wolves and Mirelings cannot drop
Moonstone. One server roll per defeat, with no retry/reload reroll.

The bear is deliberately harder than the common Forest Wolf: 250 versus 80
health, 5 versus 0 Armor, 35 versus 15 damage, balanced by longer telegraph and
recovery. This is a proposed combat target, not a claim of playtested balance.
Keep Dusk Diamond on Mossy Mire Mirelings; no other gem source changes.
A9 Moonstone implementation is blocked until this bear's acceptance passes.

## Implementation and verification

Each enemy needs original procedural/authored appearance and animation,
server attack/defeat handling, its named biome spawn entry, visible attack
telegraph, reward rules and applicable saved/relevant-peer state. No copied
reference-game creatures/assets. New attacks need explicit typed profiles and
membership; no silent fallback damage. No new loot item icons are needed for
the existing material IDs, but future Moonstone still requires its own icon.

Use lightweight checks for normal increments. Before A8a is accepted, run the
applicable full build, automation, biome/population, combat, peer/privacy and
restart checks from [development setup](07-development-setup.md). Record exact
commands/results and repair milestone defects. Demonstrate seeded population
within budgets, valid/invalid attacks, visible windups, credited loot once,
full-pack handling and relevant peers. Prove Forest Wolf/Bear difficulty in
play and check Moonstone's eligible/ineligible source cases with controlled
loot fixtures; do not require random grinding to test the probability boundary.
Aquatic acceptance must cover swimmers and unchanged skiff behavior separately.

No enemy, gem, build or runtime verification was produced by this planning edit.
