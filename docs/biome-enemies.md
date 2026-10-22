# Biome enemy expansion proposal

## Quick overview

**Current** = implemented species; **Partial** = runtime behavior is present
but rewards or acceptance remain open; **Planned** = queued additions or retunes.
Damage is before defenses. Poison rates are per second and do not stack;
only the strongest active poison deals damage. Current wildlife uses seeded
population slots. Boar/deer spawn origins use a server-side allowlist for the
biomes below. Runtime health, armor, and base attack values match this table for
the existing Boar, Deer, and Mireling actors. Rat now uses a deterministic 10%
Meadows spawn share, its server-owned short-range bite, and credited Bone
Fragment/Rat Tail rewards. Badger now uses its deterministic 40% Elderwood
common share, a distinct procedural silhouette, and a server-owned claw cycle.
Credited defeats queue the approved Bone Fragment, Badger Fur, and Badger Meat
together for the authenticated attacker until their pack can accept the complete
bundle. Moon Bear fills Elderwood's seeded 10% hard-enemy slot, uses an original
large-bear silhouette and a server-owned overhead-paw cycle, and queues three
Bone Fragments for the credited attacker. Viper fills 40% of Shimmering Lakes'
shoreline slots, uses a server-owned lunge that applies the shared five-second
weak-poison effect on an accepted bite, and queues one Viper Fang with its
baseline Bone Fragment. Giant Snake fills the remaining 10% hard slot with a
dry-shore lunge and a durable claim for two Bone Fragments plus two Giant
Scales. Mireling now fills
50% of Mossy Mire candidate slots and is restricted to dry ground in that biome.
Death Adder occupies 20% of those slots and shares the dry Mossy Mire gate;
its server-owned coiled bite applies non-stacking major poison and its credited
Bone Fragment and guaranteed Venom Sac are owner-bound in one durable claim.
Mosquito swarm now occupies a separate 20% share
with bounded server flight, persistent contact bites, weak poison, and active
fire/dodge counterplay. Peat Husk fills the final 10% hard slot, queues baseline
Amber, and saves the credited owner's 25% Peat Heart outcome. Wolf occupies its
two deterministic Freezing Tundra candidate slots and spawns as a three-member
pack in a stable triangle; each member has its own defeat and owner-bound
two-Bone, one-Pelt, and one-Meat claim with a persisted 25% Wolf Fang roll.
Lynx fills the next two seeded Tundra slots with a dry-ground pounce, a
350 cm server impact limit, and an owner-bound two-Bone claim.

| Enemy               | Biome                                  | Status           | HP   | Armor | Attack       | Abilities / counters                |
| ------------------- | -------------------------------------- | ---------------- | ---: | ----: | ------------ | ----------------------------------- |
| Boar                | Meadows / Elderwood / Shimmering Lakes | Current          |  20  |   5   | 10 Crush     | Territorial charge                  |
| Deer                | Meadows / Elderwood / Shimmering Lakes | Current          |  30  |   5   | —            | Flees; fleeing alerts nearby deer   |
| Rat                 | Meadows                                | Current          |  10  |   0   | 5 Pierce     | 3 m approach trigger; 100 cm bite   |
| Badger              | Elderwood                              | Partial          |  100 |   10  | 20 Slash     | 3 m warning; 180 cm claws; queued Bone Fragment, Fur, and Meat; I4 pending |
| Moon Bear           | Elderwood                              | Partial          |  250 |   20  | 75 Crush     | 3 m warning; 220 cm overhead paw; 1.2 s recovery; queued 3 Bone Fragment; I4 pending |
| Viper               | Shimmering Lakes                       | Partial          |  100 |   10  | 50 Stab      | 40% dry-shore slots; 180 cm lunge; weak poison: 1/s, 5 s; queued Bone Fragment; I4 pending |
| Giant Snake         | Shimmering Lakes                       | Partial          |  250 |   30  | 100 Stab     | 10% dry-shore hard slot; coiled 360 cm lunge; 1 s warning; 1.2 s recovery; queued 2 Bone Fragment + 2 Giant Scale; I4 pending |
| Mireling            | Mossy Mire                             | Partial          |  75  |   15  | 20 Crush     | 50% dry-ground share; server simple melee; boss-scroll path unchanged; I4 pending |
| Death Adder         | Mossy Mire                             | Partial          |  125 |   20  | 75 Stab      | 20% dry-ground slots; close coiled bite; major poison: 5/s, 10 s; queued Bone Fragment + Venom Sac; I4 pending |
| Mosquito swarm      | Mossy Mire                             | Partial          |  25  |   0   | 5 Stab/0.2 s | 20% dry-ground slots; one swarm per target; poison 1/s for 10 s; dodge/fire repel; I4 pending |
| Peat Husk           | Mossy Mire                             | Partial          |  300 |   50  | 150 Crush    | 10% dry-ground hard slot; 260 cm/120° sweep; 1 s warning; 1.2 s recovery; queued Amber + rolled 25% Peat Heart; I4 pending |
| Wolf                | Freezing Tundra                        | Partial          |  250 |   40  | 100 Slash    | 20% dry-ground slots; three-member pack; 180 cm lunge bite; queued 2 Bone Fragment + Wolf Pelt + Wolf Meat + 25% Wolf Fang; I4 pending |
| Lynx                | Freezing Tundra                        | Partial          |  150 |   30  | 75 Slash     | 20% dry-ground slots; 350 cm leap; 0.5 s warning; 0.8 s hit / 1.2 s miss recovery; queued 2 Bone Fragment + 50% Lynx Pelt; I4 pending |
| Polar Bear          | Freezing Tundra                        | Partial          |  750 |   60  | 200 Crush    | 10% hard slot; 1 s warning; 220 cm charge; retreat; queued 3 Bone Fragment + 2 Polar Bear Pelt; I4 pending |
| Horned Goat         | Thunder Mountains                      | Partial          |  200 |   50  | 50 Crush     | 40% dry-ground slots; 0.5 s telegraph; 180 cm horn hit; up to 250 cm push and 0.75 s stagger; queued 2 Bone Fragment + 1 Goat Horn; I4 pending |
| Thunder Sentinel    | Thunder Mountains                      | Partial          | 1250 |   75  | 300 Crush    | 10% dry-ground hard slot; 150 m start exclusion; 1 s warning, 120 cm step, 300 cm horizontal slam; 1.2 s recovery; queued 3 Stone; I4 pending |
| Reef Maw            | Ocean                                  | Partial          |  180 |   2   | 50 Stab      | 10% deep-water slot; 180 cm swimmer-only bite; queued 3 Bone Fragment; I4 pending |

Mireling uses the reviewed 75-health, 15-armor, 20-Crush profile. Its
deterministic 50% Mossy Mire share is limited to dry spawn surfaces; all other
biomes reject Mireling seeds. Its existing melee and optional server-owned
boss-scroll reward are unchanged. Death Adder and Mosquito swarm each occupy
20% dry-ground slots, and Peat Husk occupies the remaining 10% hard slot. The
planned 1% Dusk Diamond source remains an A9 task and is not enabled.

Death Adder uses its reviewed 125-health, 20-armor, 75-Stab profile in 20% of
Mossy Mire candidate slots. Spawn and return movement remain on dry Mossy Mire
ground. Its original thicker olive-brown procedural snake visibly coils,
strikes within close range, and retreats on the shared server-owned snake
cycle. Accepted bites apply the owner-visible major poison effect: 5 Poison per
second for 10 seconds. Weak and major doses keep independent expiry times and
share one tick clock; only the strongest active dose deals damage, and a weak
bite cannot extend major poison. Each credited defeat queues its one baseline
Bone Fragment and one guaranteed Venom Sac for the authenticated owner. The
server commits both atomically and retains the claim until the complete bundle
fits. Antidote remains a separate A8b output task.

Peat Husk uses the reviewed 300-health, 50-armor, 150-Crush profile in the
remaining 10% Mossy Mire slot. It is limited to dry Mossy Mire ground and its
generated spawn is kept 150 m from the player start. The existing original
peat-and-wood silhouette raises a root arm for a one-second warning, then makes
a slow 260 cm sweep through a 120-degree forward arc and recovers for 1.2
seconds. The server approaches at 100 cm/s, validates the live target, dry
ground path, line of sight and 25 m leash, and resolves damage only within the
authored sweep bounds. Each credited defeat queues one baseline Amber and rolls
once for one Peat Heart at 25%. The outcome is stored in the durable owner-bound
claim, so retries and reloads cannot reroll it. The server commits Amber and an
awarded Peat Heart together; the claim stays queued until the complete bundle
fits.

Moon Bear uses its reviewed server-side stat profile (250 health, 20 armor,
75 Crush). Its seeded spawn slot, dry-Elderwood eligibility, 150 m player-start
exclusion, close-range overhead attack, and durable three-fragment reward claim
are implemented. The separate 2% Moonstone route remains an A9 source task and
is not enabled by the A8a baseline reward.

Polar Bear uses its 750-health, 60-armor, 200-Crush profile in the seeded final
10% Freezing Tundra slot. The server excludes spawns within 150 m of the
generated start, keeps the bear on dry traversable Tundra, and bounds pursuit
to its 25 m spawn leash. A visible one-second warning precedes a committed
charge of up to 620 cm; it can damage once within 220 cm, then retreats before
the hard-enemy 1.2-second recovery. The attack cue and movement replicate from
the server. Credited defeats queue three Bone Fragments and two guaranteed
Polar Bear Pelts for the authenticated owner in population-save escrow. The
complete reward commits together when inventory capacity permits; otherwise
the claim remains pending for retry.

Viper uses the reviewed 100-health, 10-armor, 50-Stab profile and the seeded
40% Shimmering Lakes share. It only spawns on eligible dry shoreline, visibly
coils before a narrow lunge, and applies the shared weak poison after an
accepted server bite. Each credited Viper defeat now queues the one-Bone
baseline reward plus one guaranteed Viper Fang under the same authenticated,
owner-bound claim. The server commits both together; if both do not fit, the
claim stays in population-save escrow. Fang Arrows remain a separate A8b task.

Giant Snake uses the reviewed 250-health, 30-armor, 100-Stab profile in the
remaining 10% Shimmering Lakes candidate slot. It is restricted to dry lake
shoreline, stays within its 25 m spawn leash, and is kept at least 150 m from
the generated player start. The original long-bodied procedural model coils
and hisses for one second before its strong forward lunge; it recovers for
1.2 seconds and the bite has a narrow 180 cm impact reach. It does not apply
poison. Each credited defeat queues two Bone Fragments and two guaranteed Giant
Scales for the authenticated owner in one durable claim. The server commits the
full bundle atomically; it remains queued until the complete bundle fits.

Current defaults: [wildlife](../Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp),
[health](../Source/KalmalaGameplay/Public/KalmalaWildlifeSpawn.h) and
[defenses](../Source/KalmalaGameplay/Public/KalmalaDamageProfile.h).
Detailed proposals and exceptions follow.

User-approved scope, 2026-10-19. Implement through A8a in
[BACKLOG.md](../BACKLOG.md), including Reef Maw, then complete the combined
A8a/A8b enemy and item verification after A8b and before A9 gems and special gear.
These additions extend the current three-archetype roster without rewriting
historical roadmap acceptance or claiming these enemies exist today. Names,
stats, spawn weights and loot below remain task-level implementation details.

## Unique drops

**A8a review complete (2026-10-22).** For the planned loot contract, use one
server-owned roll per credited defeat and award the listed amount only on
success. The creature profiles match all 18 rows below; each drop is additional
to baseline loot. Deer Sinew and the two gem routes retain their existing
plans. The 15 A8b material identities, named enemy sources, quantities,
chances, intended uses, recipe costs, consumable effects, and A8b gear profiles
are approved below. This review does not implement or activate any new drops. Material acquisition remains unreachable until each named A8a enemy and its credited-defeat reward have an accepted server-side implementation.
Unique means a characteristic drop, not an exclusive source for shared Sinew.

| Enemy            | Unique drop        | Amount / chance | Intended use              |
| ---------------- | ------------------ | --------------- | ------------------------- |
| Deer             | Sinew              | 1 / 50%         | Bow String; 4 Sinew each  |
| Rat              | Rat Tail           | 1 / 100%        | Rat-tail Tonic ingredient |
| Badger           | Badger Fur         | 1 / 100%        | Armor component           |
| Badger           | Badger Meat        | 1 / 100%        | Cooking component         |
| Moon Bear        | Moonstone          | 1 / 2%          | Moonstone Ring            |
| Viper            | Viper Fang         | 1 / 100%        | Poison arrow ingredient   |
| Giant Snake      | Giant Scale        | 2 / 100%        | Heavy armor component     |
| Mireling         | Dusk Diamond       | 1 / 1%          | Special Dusk Ring         |
| Death Adder      | Venom Sac          | 1 / 100%        | Anti Poison potion ingr.  |
| Peat Husk        | Peat Heart         | 1 / 25%         | Cooking component         |
| Wolf             | Wolf Pelt          | 1 / 100%        | Armor component           |
| Wolf             | Wolf fang          | 1 / 25%         | Weapon component          |
| Wolf             | Wolf meat          | 1 / 100%        | Cooking component         |
| Lynx             | Lynx Pelt          | 1 / 50%         | Armor/Weapon component    |
| Polar Bear       | Polar Bear Pelt    | 2 / 100%        | Heavy armor component     |
| Horned Goat      | Goat Horn          | 1 / 100%        | Horn Bow component        |
| Thunder Sentinel | Thunder Core       | 1 / 25%         | Lightning gear component  |
| Reef Maw         | Reef Tooth         | 1 / 50%         | Harpoon component         |

New A8b materials are exclusive to their named enemy. No shared
venom/pelt substitution, gem conversion or automatic new equipment recipe.
Keep existing gem odds independent of baseline loot. Server owns rolls and
stable claims; full packs preserve unclaimed rewards, and retries/reloads
cannot reroll or duplicate them. Every new material requires its registered
catalogue definition and original generated icon before its drop is enabled;
the source enemy must also exist in runtime. This review does not activate any
new drops. The next A8a review covers the recipe costs, effects, and full item
profiles. The approved scope includes Lightning gear, the Harpoon and the full
Reef Maw route. Cold-protection clothing remains explicitly deferred; the
aquatic route adds no skiff damage.

## Recommended items and recipes

Implementation is queued in [BACKLOG A8b](../BACKLOG.md), one material or output
per increment. Bow String remains in A0; the rings remain in A9. Every item
includes applicable catalogue JSON/manifest entries and an original generated,
prepared/imported runtime icon with validation; none may be checked complete
with placeholder data or a fallback icon.

**A8a item review complete (2026-10-22).** The A8b checklist remains canonical
for recipe stations, costs, and output quantities. Bow String remains bow-only;
Moonstone Ring and Dusk Ring stay in A9; their rows are downstream references,
not additional A8b outputs. Their A9 recipe costs remain for the separate A9
review.
The user delegated A8b effects and gear-profile decisions to Codex on
2026-10-21; this review confirms that contract without enabling any items.
Every retained drop has a use; Boar and Mosquito swarm have no new unique drop.

| Recommended item | Station      | Recipe / output                              | Proposed use / base effect     |
| ---------------- | ------------ | -------------------------------------------- | ------------------------------ |
| Bow String       | Workbench    | 4 Sinew → 1                                  | Existing bow material          |
| Rat-tail Tonic   | Cauldron     | 2 Rat Tail + 2 Lingonberry → 1               | Heal 30 HP over 10 s; shared 60 s cooldown |
| Badger Fur Vest  | Workbench    | 6 Badger Fur → 1                             | Chest; 6 Armor                 |
| Roasted Badger   | Cooking Rack | 1 Badger Meat → 1                            | Steady Meal                    |
| Moonstone Ring   | Forge        | 5 Bronze + 1 Moonstone → 1                   | +20 max stamina; existing plan |
| Fang Arrows      | Workbench    | 2 Viper Fang + 10 Wood → 20                  | 50 Pierce; weak poison 5 s     |
| Scale Cuirass    | Forge        | 10 Giant Scale + 10 Bronze → 1               | Chest; 20 Armor                |
| Dusk Ring        | Forge        | 5 Bronze + 1 Dusk Diamond → 1                | +10 HP, +10 stamina, +5 Armor  |
| Antidote         | Cauldron     | 1 Venom Sac + 2 Lingonberry → 1              | Clear weak (minor); reduce major to weak; 10 min resistance |
| Peat Heart Stew  | Cauldron     | 1 Peat Heart + 2 Potato + 1 Onion → 2        | Steady Meal                    |
| Wolf Pelt Boots  | Workbench    | 5 Wolf Pelt + 5 Iron   → 1                   | Boots; 10 Armor                |
| Fang Spear       | Forge        | 4 Wolf Fang + 10 Iron + 10 Densewood → 1     | 100 Stab                       |
| Wolf Stew        | Cauldron     | 1 Wolf Meat + 3 Carrot + 3 Onion → 1         | Steady Meal                    |
| Lynx Mantle      | Workbench    | 4 Lynx Pelt → 1                              | Cape; 8 Armor                  |
| Polar Bear Coat  | Forge        | 5 Polar Bear Pelt + 20 Iron → 1              | Chest; 50 Armor                |
| Horn Bow         | Workbench    | 2 Goat Horn + 10 Densewood + 1 Bow String → 1| 100 Pierce bow bonus, slow draw|
| Thunder Maul     | Forge        | 1 Thunder Core + 20 Iron + 10 Densewood → 1  | 100 Crush + 30 Lightning       |
| Reef Harpoon     | Forge        | 3 Reef Tooth + 8 Iron + 4 Lightwood → 1      | 60 Pierce; spear weapon        |

- Roasted Badger, Peat Heart Stew and Wolf Stew use A6's accepted Steady Meal:
  120 s, stamina costs ×0.90, no stacking or eating while active. Raw meat and
  Peat Heart are ingredients.
- Fang Arrows use the existing weak-poison profile, 1 damage/s for 5 s, on an
  accepted hit. Apply the existing strongest-dose/refresh rules once per
  projectile hit; do not repeat the poison application.
- Rat-tail Tonic weighs 0.2 kg and stacks to 10. It restores up to 30 health
  over 10 seconds at 3 health/second, clamped to maximum health and ending early
  when full. Reject use when dead, at full health, while this or Berry Recovery
  healing is active, or while the shared recovery cooldown is active; rejected
  use consumes nothing. Healing ends on death, with no item or cooldown refund.
  It shares A6's 60-second cooldown with Berry Recovery Draught, Root Stamina
  Draught and Antidote. Consumable crafting awards 10 Cooking XP per dose.
- Antidote weighs 0.2 kg and stacks to 10. A living player may use it at full
  health, including before poison is applied. On use it clears active weak
  poison; an active major dose becomes weak poison at 1 damage/second, with its
  remaining duration capped at 5 seconds. It grants 600 seconds of resistance:
  new weak poison is blocked and new major poison is reduced to the weak profile
  (1 damage/second for up to 5 seconds). Converted weak doses use the existing
  strongest-dose refresh rules and never stack. On resistance expiry, converted
  weak poison continues only for its remaining duration and never returns to
  major strength. Reject use during the shared 60-second A6 recovery cooldown or
  while resistance is already active; rejected use consumes nothing. Death ends
  resistance without refunding the dose or cooldown. A8b Antidote is the sole
  poison-resistance exception; cold and fire resistance remain deferred.
- Armor profiles. These are base values; Armor is physical-only and fur grants
  no cold protection. Armor repairs use the existing free server-validated
  Workbench repair, with no materials. Each armor item has one Workbench or
  Forge upgrade step and the shared reward of +1 Armor and +10 MaxDurability.
  No second upgrade step is planned.

  | Item | Weight / MaxDurability | Slot / Armor | Upgrade station and cost |
  | ---- | --------------------- | ------------ | ------------------------ |
  | Badger Fur Vest | 4 kg / 100 | Chest / 6 | Workbench: 3 Badger Fur |
  | Scale Cuirass | 8 kg / 150 | Chest / 20 | Forge: 5 Giant Scale + 5 Bronze |
  | Wolf Pelt Boots | 3 kg / 120 | Boots / 10 | Workbench: 2 Wolf Pelt + 2 Iron |
  | Lynx Mantle | 2 kg / 100 | Cape / 8 | Workbench: 2 Lynx Pelt + 1 Bone Fragment |
  | Polar Bear Coat | 12 kg / 200 | Chest / 50 | Forge: 2 Polar Bear Pelt + 10 Iron |

- Weapon profiles. Melee uses current server combat timing (0.18 s windup,
  0.36 s recovery) and 220 cm reach. Repair uses the existing free
  server-validated Workbench repair. Each weapon has one upgrade step; no
  second step is planned.

  | Item | Weight / hands / MaxDurability | Attack and action | Upgrade station, cost and reward |
  | ---- | ----------------------------- | ----------------- | -------------------------------- |
  | Fang Spear | 4 kg / one / 200 | 100 Stab; standard melee timing and reach | Forge: 2 Wolf Fang + 5 Iron + 5 Densewood; +5 Stab, +50 MaxDurability |
  | Horn Bow | 2.5 kg / two / 140 | 100 Pierce weapon bonus; 3-second full draw; existing arrow curves | Workbench: 1 Goat Horn + 5 Densewood + 1 Bow String; +30 MaxDurability, no damage bonus |
  | Thunder Maul | 10 kg / two / 220 | 100 Crush + 30 Lightning; standard melee timing and reach | Forge: 1 Thunder Core + 10 Iron + 5 Densewood; +5 Crush, +50 MaxDurability, Lightning stays 30 |
  | Reef Harpoon | 4 kg / one / 150 | 60 Pierce; standard spear-like melee timing and reach; not thrown | Forge: 1 Reef Tooth + 4 Iron + 2 Lightwood; +5 Pierce, +30 MaxDurability |

- Thunder Maul applies its 30 Lightning as instant additional damage on the
  same accepted server hit. It bypasses wearable physical Armor, then uses the
  target's general Armor reduction; it adds no Lightning resistance. It does
  not chain, deal damage over time, stun or interact with the environment. Bow
  damage is a weapon bonus combined once with arrow damage. Reef Harpoon adds
  no skiff damage.
- Materials have no equipment, attack or upgrade records; armor has no attack
  record; weapons have no equipment slot; consumables have no equipment,
  attack or upgrade records. The current runtime catalogue lacks Lightning, so
  A8b must add that channel explicitly. All catalogue data, item-specific
  source/use behavior, icons, runtime mappings and validation remain required
  in the A8b implementation tasks.

## Proposed roster

Keep existing boars, deer and Mirelings. Add fourteen enemies, one complete
enemy per increment. All damage below is one authored server-resolved hit,
before existing defenses; no client-authored damage or offensive player magic.

| Biome | Enemy / ID | Role and attack | Health / Armor | Hit damage | Guaranteed loot |
| --- | --- | --- | --- | --- | --- |
| Meadows | Rat (`Rat`) | Small scavenger; bites only when approached within 3 m or attacked; 100 cm hit reach | 10 / 0 | 5 Pierce | 1 Bone Fragment |
| Elderwood | Badger (`Badger`) | Territorial animal; warns within 3 m, then slashes with its claws if approached or attacked | 100 / 10 | 20 Slash | 1 queued Bone Fragment; Fur/Meat pending A8b |
| Elderwood | Moon Bear (`MoonBear`) | Rare hard enemy; slow overhead paw strike, punishable recovery | 250 / 20 | 75 Crush | 3 Bone Fragment |
| Shimmering Lakes | Viper (`Viper`) | Snake hiding in dry shoreline reeds; visible coil, narrow lunging bite, then retreats; applies weak poison for 5 s | 100 / 10 | 50 Stab + poison below | 1 Bone Fragment |
| Shimmering Lakes | Giant Snake (`GiantSnake`) | Rare hard snake hidden among shoreline reeds; raised-head coil, powerful lunging bite, then slow recovery | 250 / 30 | 100 Stab | 2 Bone Fragment |
| Mossy Mire | Death Adder (`DeathAdder`) | Highly venomous camouflaged snake; visible coil, short bite then retreat; major poison for 10 s | 125 / 20 | 75 Stab + poison below | 1 Bone Fragment |
| Mossy Mire | Mosquito swarm (`MosquitoSwarm`) | Flying swarm surrounds a player and keeps biting; 10 s weak poison; rolls break contact, lit flames repel it | 25 / 0 per swarm | 5 Stab/0.2 s + poison below | None |
| Mossy Mire | Peat Husk (`PeatHusk`) | Rare hard, original peat-and-wood creature; slow sweeping arm | 300 / 50 | 150 Crush | 1 Amber |
| Freezing Tundra | Wolf (`Wolf`) | Tough pursuing predator; visible forward lunge and powerful bite, then recovery | 250 / 40 | 100 Slash | 2 Bone Fragment |
| Freezing Tundra | Lynx (`Lynx`) | Mobile predator; telegraphed leap, long recovery if it misses | 150 / 30 | 75 Slash | 2 Bone Fragment |
| Freezing Tundra | Polar Bear (`PolarBear`) | Rare hard bear variant; committed charge, then retreat to reset | 750 / 60 | 200 Crush | 3 Bone Fragment |
| Thunder Mountains | Horned Goat (`HornedGoat`) | Territorial wildlife; telegraphed horn charge pushes and staggers | 200 / 50 | 50 Crush | 2 Bone Fragment + 1 Goat Horn |
| Thunder Mountains | Thunder Sentinel (`ThunderSentinel`) | Rare hard, original stone creature; short step and downward slam | 1250 / 75 | 300 Crush | 3 Stone |
| Ocean | Reef Maw (`ReefMaw`) | Rare swimming predator; visible approach and single bite against swimming players | 180 / 2 | 50 Stab | 3 Bone Fragment |

The roster's guaranteed loot remains the baseline. The unique-drop table adds
reviewable material proposals; it supersedes the earlier blanket deferral of
new carcass materials. Never substitute Deer Meat/Boar Hide for other species.
New materials and their proposed uses require approved item/recipe definitions
and original inventory icons before implementation; the uses are not recipes.

## Spawn and combat rules

- Retain existing world-seed, biome suitability, relevance and actor budgets.
  Add variety within existing candidate slots, not additional unbounded actors.
- Boars and deer may spawn **only in Meadows, Elderwood and Shimmering Lakes**.
  The server samples the biome at each generated spawn origin and filters the
  deterministic seed before actor initialization, including regenerated/reloaded
  population candidates; a seed cannot bypass the allowlist. This controls
  spawn origin and does not add a movement barrier. The focused automation
  covers each allowed and excluded biome; run the fixed-seed world checks at I4.
  Use the revised stats below; retain other behavior and loot. Verify biome
  filtering across fixed seeds.
- Proposed selection weights are percentages of eligible candidate slots.
  Meadows uses 90% existing roster / 10% Rat, selected deterministically from
  each tenth candidate slot. Rat attacks only within its 3 m trigger, telegraphs
  for 0.5 s, checks line of sight and the approved 100 cm hit reach at impact,
  and recovers for 0.8 s. Its 25 m pursuit leash is measured from spawn.
  Each other land biome allocates
  50% to its existing eligible roster, 40% to new common enemies, and 10% to
  new hard enemies. If no existing entry is eligible, leave that share empty;
  do not import creatures from another biome. Spawned actors use the existing
  actor budget. A mosquito swarm is one gameplay actor with cosmetic insects.
- Mirelings spawn **only in Mossy Mire**, including seeded/reloaded candidates.
  Use the same server-side spawn-origin allowlist policy as boar/deer.
- Mossy Mire uses the full 50/20/20/10 distribution: 50% Mireling (existing),
  20% Death Adder, 20% Mosquito swarm, and 10% Peat Husk. These are overall
  candidate-slot weights, not shares within the common-enemy allocation.
- Freezing Tundra uses 50% existing eligible roster / 20% Wolf / 20% Lynx /
  10% Polar Bear. Elderwood uses Badger for its 40% common share. Wolves spawn
  only in Freezing Tundra, never Elderwood; each Wolf pack has three Wolves.
  The Tundra per-key wildlife actor cap has a floor of three to fit one Wolf
  pack. Count each living pack member against that cap and skip a pack that
  cannot fit completely. Wolf selection and formation are implemented; the
  rare Polar Bear now occupies the final 10% hard share; the existing 50% share
  stays empty because no current baseline species is eligible in Tundra.
- Thunder Mountains uses 40% Horned Goat, 10% Thunder Sentinel, and leaves
  the existing 50% share empty. Both species require dry traversable mountain
  ground; the hard Sentinel also uses the shared 150 m generated-start exclusion.
  Each credited Goat defeat queues two Bone Fragments and one guaranteed Goat
  Horn for its authenticated owner as one retryable inventory bundle.
  Its server-owned one-second warning includes a short step before a single
  downward hit checked against the user-confirmed 300 cm horizontal reach;
  credited defeats queue three Stone for the authenticated owner.
- Ocean: use 10% of eligible aquatic candidate slots for Reef Maw; others
  stay empty. Spawn and movement require generated Ocean water at least 150 cm
  deep, excluding Shimmering Lakes. The Maw visibly approaches swimming
  players, gives a one-second warning, and attempts one 50-Stab bite within
  180 cm before a 1.2-second recovery. The server rechecks swimming, deep-water
  habitat, leash, path and visibility at impact. It never targets skiff hulls
  or attached occupants. Credited defeats queue three Bone Fragments for the
  authenticated owner; Reef Tooth remains disabled until its A8b item task.
- Require dry traversable ground for land enemies. Lakes means dry shores,
  not deep water. Hard enemies cannot spawn within 150 m of generated start.
  No new compulsory encounters, boss keys, raids or progression locks.
- Other common enemies telegraph attacks for 0.5 s and recover for 0.8 s; hard
  enemies telegraph for 1.0 s and recover for 1.2 s. One damage application per
  accepted attack; server rechecks life, range and line of sight at impact.
  Use existing melee reach for the matching body size; if no suitable authored
  reach exists, propose it for review rather than guessing during coding.
- Mosquito uses the dedicated swarm rules below instead of isolated
  stab/retreat attacks. This includes bounded flying pursuit, fire avoidance,
  roll escape, collision and relevant-peer support; ground navigation alone
  is not sufficient. Includes weak poison below; no lifesteal, gem roll or new loot item.
- Pursuit ends beyond 25 m from the spawn anchor. Return home instead of
  teleporting into an attack. No enemy crosses deep water without its own
  accepted aquatic support. Variants may share locomotion foundations, but
  require distinct readable silhouettes and the listed timing/damage.
- Extend existing server-owned wildlife lifecycle, owner reward attribution,
  relevant-peer replication and depletion policy. Persist stable credited-death
  and loot claim identities so reload/duplicate requests cannot grant twice.
  Reject invalid attacks unchanged; preserve unclaimed rewards on full packs.

## Existing wildlife retunes and changed abilities

- Boar: **20 health, 5 Armor, 10 Crush**; keep its territorial charge.
  Deer: **30 health, 5 Armor**, no attack; fleeing alerts nearby deer.
  These replace current runtime defaults, not claims of implemented stats.
- Badger uses **20 Slash** claw attacks instead of bites. Retain its warning
  and the common enemy windup/recovery rules.
- Horned Goat's **50 Crush** horn charge has a **180 cm** impact range and,
  on a server-confirmed living-target hit, pushes up to **250 cm** and staggers
  for **0.75 seconds**. Collision can shorten the push. The values were
  confirmed by the user on 2026-10-22; see the decision log.
- All detailed roster stats follow the overview. Update runtime health/damage
  validation limits to support the approved targets (up to 1,250 health and
  300 damage), while retaining server-owned authored values and rejecting
  client-authored damage. Verify each enemy against its own profile.

## Mireling base enemy

- Make the existing Mireling the frequent, weak Mossy Mire enemy. Proposed
  tuning: **75 health, 15 Armor, 20 Crush** (currently 100 health/10 Crush).
  Retain its approach and simple melee; no poison or new special ability.
- Use the Mire's 50% base-enemy share; no Mireling spawns in other biomes.
  Keep existing loot and the planned 1% Mossy Mire Dusk Diamond route.
- Preserve server-owned damage, rewards and relevant-peer state. Verify
  seeded/reloaded biome filtering, updated combat stats and unchanged loot.

## Giant Snake

- Replace Reed Lurker with a large, thick-bodied snake, proposed 4 m long.
  Original scale pattern and readable raised-head silhouette distinguish it
  from Viper. Slithers on dry shore; no limbs or forelimb-swipe attack.
- Telegraph the bite with a visible coil and hiss for 1 s; strike forward
  once, then recover for 1.2 s. A sideways roll avoids the narrow lunge.
  Use 250 health, 30 Armor and 2 Bone Fragment; the 100-damage bite uses Stab.
  No poison or constriction/grab mechanic in this proposal.
- Occupy the existing Lakes 10% hard-enemy share, within normal spawn budgets,
  25 m leash and dry-ground rules. No deep-water pursuit. Verify body collision,
  readable bite timing, server hit validation and peer appearance.

## Freezing Tundra Wolf

- Wolf occupies exactly two of each ten deterministic Tundra candidate slots;
  Lynx occupies slots seven and eight, while the remaining unimplemented Tundra
  shares remain empty. A Wolf candidate creates three
  actors in a seed-rotated triangle with a 240 cm radius. All three positions
  must remain in dry, traversable Freezing Tundra. Stable member suffixes keep
  defeat state and reward claims independent across reloads.
- The server acquires a target within 3 m, pursues only while both Wolf and
  target stay within 25 m of the pack member's spawn anchor, and returns home
  when its target is lost. Approach speed is 170 cm/s. It warns for the common
  0.5 s, then visibly lunges for up to 180 cm and applies one 100-Slash bite
  only if line of sight and the user-reviewed 180 cm impact reach still pass.
  The attack recovers for 0.8 s. Invalid targets, blocked routes, water and
  biome-boundary crossings prevent movement or damage.
- Each credited defeat queues two Bone Fragments, one guaranteed Wolf Pelt,
  and one guaranteed Wolf Meat for its authenticated attacker, plus a server-side
  25% roll for one Wolf Fang. The durable owner claim stores that roll so
  retries, reloads, and duplicate defeat callbacks cannot reroll or duplicate
  it. The complete awarded bundle commits together; insufficient capacity
  retains the claim for retry. Validate host/client appearance, pack
  cap/partial saved packs, attack rejection, full-pack retry, reconnect and
  restart in combined I4.

## Freezing Tundra Lynx

- Lynx occupies exactly two of each ten deterministic Tundra candidate slots,
  after Wolf's two slots. It spawns only on dry Freezing Tundra with the same
  traversable-slope check; no pack members or extra actor-cap allowance are
  added. Other unimplemented Tundra shares stay empty.
- The server acquires and pursues a target within a 25 m spawn leash on dry,
  traversable Tundra. It crouches for the common 0.5 s warning, then visibly
  pounces toward the target. At impact the server rechecks visibility, habitat,
  target validity and the user-reviewed inclusive 350 cm maximum distance.
  A landed hit recovers for the authored 0.8 s; a miss recovers for 1.2 s to
  make the reviewed long miss recovery visible. Invalid targets, blocked routes,
  water and biome-boundary crossings prevent movement or damage.
- Each credited defeat queues two Bone Fragments and a server-side 50% roll for
  one Lynx Pelt for the authenticated attacker through the population-save
  escrow. The owner claim stores the roll, and the complete awarded candidate
  commits together; insufficient capacity retains it for retry. Legacy
  unsuffixed Lynx claims remain baseline-only. Validate the readable crouch/leap
  poses, attack rejection, reward retry, reconnect and restart in combined I4.

## Viper poison

- A server-confirmed bite applies **weak poison for 5 seconds**. Proposed
  tuning: 1 Poison damage each second, first tick after 1 s, five ticks total.
  Use 50 Stab on the bite, 100 health and 10 Armor; retain loot.
- Further bites refresh the five-second duration without stacking damage or
  causing an immediate extra tick. Vipers and mosquito swarms share one weak
  poison effect at 1 Poison/second: expiry becomes the later of its current
  expiry or application time plus the source duration (5 s Viper, 10 s swarm).
  Refresh never restarts the tick timer or shortens an existing effect.
- Resolve ticks through existing Poison defenses; physical Armor does not
  reduce Poison. Poison can apply even when Armor absorbs the physical bite.
  Dodged, missed or rejected bites never apply it; rolling after application
  does not cleanse it. Torches repel mosquitoes, not Viper poison.
- Server owns application, ticks and expiry; show the owner a poison indicator
  and remaining time. Clear on death/session end; no saved poison or offline
  catch-up ticks. Status support is part of this enemy's implementation.
- Verify five-tick expiry, refreshed duration, multiple Vipers, defenses,
  rejected bites and owner/client display. Damage tuning remains reviewable.

## Death Adder major poison

- A server-confirmed bite applies **major poison for 10 seconds**. Implemented
  tuning: **5 Poison/second**, first tick after 1 s, ten ticks (50 total before
  Poison defenses). Use 75 Stab on the bite, 125 health and 20 Armor; retain loot.
- Further Death Adder bites refresh the duration without stacking or extra
  immediate ticks; multiple adders share one major effect. Weak and major
  poison have separate expiry timers but one shared one-second damage clock:
  apply only the strongest active dose, never add their damage together.
  Weak bites cannot extend major poison; any still-active weak effect resumes
  at its normal strength after major expires. Applications never reset the clock.
- Use the Viper rules for server validation, Poison defenses, owner display
  and death/session cleanup. Physical Armor does not prevent application on
  an accepted bite. Rolls avoid bites but do not cleanse existing poison;
  fire does not repel Death Adders or remove poison.
- Verify ten-second expiry, refresh, multiple adders, weak/major overlap,
  defenses, rejected bites and owner/client display. Keep its coiled windup
  readable against Mire ground; this remains unplaytested balance.

## Mosquito swarm

The user-selected behavior is persistent small bites until the player rolls
away or holds a lit fiery item. Mosquitoes avoid active fires and must not
attack players holding a lit torch. The following numbers are proposed tuning.

- One swarm actor represents six original cosmetic mosquito silhouettes,
  with audible buzzing and 25 shared health. Fly 0.5–2.5 m above local ground;
  approach visibly for 0.5 s before harassment. No instant spawn on a player.
- Harass one target within 1 m, applying **5 Stab every 0.2 s** with no retreat
  between bites. One attacking swarm per player; other swarms wait outside
  contact range rather than stacking bite rates. Missed/late server ticks do
  not burst catch-up damage. Visible insects never inflict separate hits.
- Apply existing typed defense, physical Armor and accepted-hit wearable-wear
  rules once per tick. Armor may reduce damage to zero; this proposal adds no
  armor bypass. Contact Stab damage is capped at 25 per second before defenses for each
  player, regardless of cosmetic insect count.
- Each accepted contact bite applies **weak poison for 10 seconds**. Proposed
  tuning: 1 Poison/second, first tick after 1 s, ten ticks without refresh.
  Use the shared non-stacking Viper poison rules, defenses and owner display.
  Armor absorbing the Stab damage does not prevent poison application.
  Rolls and fire stop new bites/applications; existing poison runs to expiry.
- An accepted server-owned dodge roll immediately breaks swarm contact and
  prevents reacquisition for 2 s. Normal roll stamina/cooldown still apply;
  rejected roll requests cannot cancel bites. Escaping beyond 1 m avoids
  contact damage while the swarm catches up. This adds no invulnerability
  against other enemies and cannot carry insects through walls.
- A usable **lit, held torch** cancels targeting and pending contact bites
  immediately. A torch in the pack, unlit, rain-extinguished or out of fuel
  provides no protection. Unequipping/extinguishing removes protection; the
  swarm must complete its normal 0.5 s approach before bites resume.
- Proposed avoidance radii: 3 m around held torches/placed flame lights and
  5 m around a lit campfire or exposed burning-world fire. Swarms steer away
  and cannot attack players inside these zones with line of sight to the
  flame. Fire protection can therefore cover nearby allies. Roofs, walls and
  closed doors still block flight/attacks normally.
- Fire eligibility comes from authoritative flame/fuel state, never a client
  light/particle flag or item name. Lit campfires require nonzero heat;
  smouldering/rain-suppressed fires do not repel. Lit placed lights can repel
  without supplying warmth. Sealed Kiln/Smelter heat does not count as exposed
  flame. Preserve existing wetness, warmth, ignition and damage rules.
- Validate roll/fire/contact state before every bite, not only on acquisition.
  Repelled swarms orbit outside the fire radius or return to their anchor;
  keep the 25 m leash, Mossy Mire boundary, relevance caps and no deep-water
  crossing. Death, despawn or target loss clears harassment timers.

Contact damage ends on disengagement; applied poison lasts until its expiry.
Both use session-only state cleared on death/session end, with no saved debuff,
offline catch-up damage or persistent protection.
Verify host/client targeting, concurrent swarms, roll success/rejection,
lit/unlit/empty/rainy torches, allies near fires, walls, fuel expiry and the
race where a torch lights on the same tick as a bite. Check ten-second poison
expiry, refresh without stacking, mixed Viper/swarm bites, and poison continuing
after rolling or lighting a torch. Define a server flame
query shared with the approved torch/fire implementations; do not infer an
existing heat-only API can recognize every new flame source.

The runtime swarm uses the deterministic 20% Mossy Mire share and the dry
Mossy Mire gate. One replicated wildlife actor presents six original low-poly
insects and a spatial procedural buzz. Server flight stays 0.5–2.5 m above
sampled dry Mire ground, stops at visibility-blocking geometry, and stays within
its 25 m spawn leash. One swarm reserves a target at a time; contact applies
one 5-Stab bite every 0.2 s and refreshes the existing owner-visible weak
poison for ten seconds. A server-accepted grounded dodge roll costs 25 stamina
after current status modifiers, launches forward at 620 cm/s for 0.35 s, has a
1 s cooldown, grants no invulnerability, and breaks swarm contact for 2 s.
Held fuelled torches and rain-safe fuelled Standing Lights repel within 3 m;
Lit fuelled campfires with effective warmth repel within 5 m. The server tests
visibility to each authoritative flame so roofs, walls and closed doors block
protection. Unimplemented exposed-world-fire actors remain unsupported. These
source contracts are not live multiplayer acceptance; the full fire, roll,
peer-privacy and identity-matched restart scenarios remain in combined I4.

## Moonstone transfer

Move Moonstone exclusively to **Moon Bear**, with a **2% chance of exactly
1 per credited defeat**, otherwise 0, alongside its 3 Bone Fragment.
Use the authoritative enemy kind and Elderwood spawn identity, not the biome
where a player lures it. Boars, deer, Badgers, Wolves and Mirelings cannot drop
Moonstone. One server roll per defeat, with no retry/reload reroll.

The bear is deliberately harder than the common Badger: 250 versus 100
health, 20 versus 10 Armor, 75 versus 20 damage, balanced by longer telegraph and
recovery. This is a proposed combat target, not a claim of playtested balance.
Keep Dusk Diamond on Mossy Mire Mirelings; no other gem source changes.
A9 Moonstone implementation is blocked until this bear's acceptance passes.

## Implementation and verification

Each enemy needs original procedural/authored appearance and animation,
server attack/defeat handling, its named biome spawn entry, visible attack
telegraph, reward rules and applicable saved/relevant-peer state. No copied
reference-game creatures/assets. New attacks need explicit typed profiles and
membership; no silent fallback damage. Existing baseline material IDs reuse
their icons; every approved new unique drop and future gem requires its own
original generated icon and catalogue registration before its loot is enabled.

Use lightweight checks for normal increments. Before A8a is accepted, run the
applicable full build, automation, biome/population, combat, peer/privacy and
restart checks from [development setup](07-development-setup.md). Record exact
commands/results and repair milestone defects. Demonstrate seeded population
within budgets, valid/invalid attacks, visible windups, credited loot once,
full-pack handling and relevant peers. Prove Badger/Moon Bear
difficulty and Wolf/Lynx/Polar Bear roles in
play and check Moonstone's eligible/ineligible source cases with controlled
loot fixtures; do not require random grinding to test the probability boundary.
Aquatic acceptance must cover swimmers and unchanged skiff behavior separately.

No enemy, gem, build or runtime verification was produced by this planning edit.
