# Biome enemy expansion proposal

## Quick overview

**Current** = existing species; **Planned** = queued additions or retunes.
Damage is before defenses. Poison rates are per second and do not stack;
only the strongest active poison deals damage. Current wildlife uses seeded
population slots rather than the proposed biome restrictions below. Boar/deer
biomes and all stats in this table are planned targets, including boar/deer retunes.
Mireling's biome and stats are also planned changes to an existing enemy.

| Enemy               | Biome                                  | Status           | HP   | Armor | Attack       | Abilities / counters                |
| ------------------- | -------------------------------------- | ---------------- | ---: | ----: | ------------ | ----------------------------------- |
| Boar                | Meadows / Elderwood / Shimmering Lakes | Current          |  20  |   5   | 10 Crush     | Territorial charge                  |
| Deer                | Meadows / Elderwood / Shimmering Lakes | Current          |  30  |   5   | —            | Flees; fleeing alerts nearby deer   |
| Rat                 | Meadows                                | Planned          |  10  |   0   | 5 Pierce     | Defensive bite                      |
| Badger              | Elderwood                              | Planned          |  100 |   10  | 20 Slash     | Slashes with it's claws             |
| Moon Bear           | Elderwood                              | Planned          |  250 |   20  | 75 Crush     | Slow overhead strike; 2% Moonstone  |
| Viper               | Shimmering Lakes                       | Planned          |  100 |   10  | 50 Stab      | Weak poison: 1/s, 5 s               |
| Giant Snake         | Shimmering Lakes                       | Planned          |  250 |   30  | 100 Stab     | Coiled lunge; slow recovery         |
| Mireling            | Mossy Mire                             | Planned          |  75  |   15  | 20 Crush     | Weak base enemy; simple melee       |
| Death Adder         | Mossy Mire                             | Planned          |  125 |   20  | 75 Stab      | Major poison: 5/s, 10 s             |
| Mosquito swarm      | Mossy Mire                             | Planned          |  25  |   0   | 5 Stab/0.2 s | Poison: 1/s, 10 s; roll/fire repels |
| Peat Husk           | Mossy Mire                             | Planned          |  300 |   50  | 150 Crush    | Slow sweeping arm                   |
| Wolf                | Freezing Tundra                        | Planned          |  250 |   40  | 100 Slash    | Pursuit; lunging bite               |
| Lynx                | Freezing Tundra                        | Planned          |  150 |   30  | 75 Slash     | Leap at player                      |
| Polar Bear          | Freezing Tundra                        | Planned          |  750 |   60  | 200 Crush    | Committed charge                    |
| Horned Goat         | Thunder Mountains                      | Planned          |  200 |   50  | 50 Crush     | Territorial horn charge, push + stagger |
| Thunder Sentinel    | Thunder Mountains                      | Planned          |  1250|   75  | 300 Crush    | Slow downward slam                  |
| Reef Maw            | Ocean                                  | Planned          |  180 |   2   | 50 Stab      | Swimmer hunter                      |

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
chances, and intended-use links are approved. Recipe costs, consumable effects,
and complete gear profiles remain for the next A8a review.
Unique means a characteristic drop, not an exclusive source for shared Sinew.

| Enemy            | Unique drop        | Amount / chance | Intended use              |
| ---------------- | ------------------ | --------------- | ------------------------- |
| Deer             | Sinew              | 1 / 50%         | Bow String; 4 Sinew each  |
| Rat              | Rat Tail           | 1 / 100%        | Future potion ingredient  |
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

**Recommendations for review**, using the latest user-edited drop table above.
Costs and base effects below are proposals; Bow String and both rings reuse
existing backlog recipes. Every retained drop has a use; Boar and Mosquito
swarm currently have no unique drop, so neither receives a new recipe here.

| Recommended item | Station      | Recipe / output                              | Proposed use / base effect     |
| ---------------- | ------------ | -------------------------------------------- | ------------------------------ |
| Bow String       | Workbench    | 4 Sinew → 1                                  | Existing bow material          |
| Rat-tail Tonic   | Cauldron     | 2 Rat Tail + 2 Lingonberry → 1               | Heal 30 HP over 10 s           |
| Badger Fur Vest  | Workbench    | 6 Badger Fur + 2 String → 1                  | Chest; 6 Armor                 |
| Roasted Badger   | Cooking Rack | 1 Badger Meat → 1                            | Steady Meal                    |
| Moonstone Ring   | Forge        | 5 Bronze + 1 Moonstone → 1                   | +20 max stamina; existing plan |
| Fang Arrows      | Workbench    | 2 Viper Fang + 10 Wood → 20                  | 50 Pierce; weak poison 5 s     |
| Scale Cuirass    | Forge        | 10 Giant Scale + 10 Bronze → 1               | Chest; 20 Armor                |
| Dusk Ring        | Forge        | 5 Bronze + 1 Dusk Diamond → 1                | +10 HP, +10 stamina, +5 Armor  |
| Antidote         | Cauldron     | 1 Venom Sac + 2 Lingonberry → 1              | Clear weak poison + poison resistance for 10 minutes. major poison effect reduced to half.        |
| Peat Heart Stew  | Cauldron     | 1 Peat Heart + 2 Potato + 1 Onion → 2        | Steady Meal                    |
| Wolf Pelt Boots  | Workbench    | 5 Wolf Pelt + 5 Iron   → 1                   | Boots; 10 Armor                |
| Fang Spear       | Forge        | 4 Wolf fang + 10 Iron + 10 Densewood → 1     | 100 Stab                       |
| Wolf Stew        | Cauldron     | 1 Wolf meat + 3 Carrot + 3 Onion → 1         | Steady Meal                    |
| Lynx Mantle      | Workbench    | 4 Lynx Pelt + 2 String → 1                   | Cape; 8 Armor                  |
| Polar Bear Coat  | Forge        | 5 Polar Bear Pelt + 20 Iron → 1              | Chest; 50 Armor                |
| Horn Bow         | Workbench    | 2 Goat Horn + 10 Densewood + 1 Bow String → 1| 100 Pierce bow bonus, slow draw|
| Thunder Maul     | Forge        | 1 Thunder Core + 20 Iron + 10 Densewood → 1  | 100 Crush + 30 Lightning       |
| Reef Harpoon     | Forge        | 3 Reef Tooth + 8 Iron + 4 Lightwood → 1      | 60 Pierce; spear weapon        |

- Meals use the existing proposed Steady Meal: 120 s, stamina costs ×0.90;
  no stacking or eating while active. Raw meat and Peat Heart are ingredients.
- Fang Arrows apply weak poison at 1/sec for 5 s on an accepted hit; use the
  enemy poison refresh/strongest-dose rules. No repeated poison per projectile.
- Tonic and Antidote share the proposed 60 s recovery cooldown. Tonic cannot
  be used at full health. The user's revised Antidote clears weak poison,
  provides poison resistance for 600 s, and halves major-poison damage.
  Define the exact weak-poison resistance/reapplication behavior and effect
  refresh rules during planning; do not guess them during implementation.
  This new resistance proposal supersedes A6's deferral only if approved.
- Bow damage is a weapon bonus, combined once with arrow damage. Armor is
  physical-only. Fur items grant no cold protection in this recommendation.
- Thunder Maul requires approved Lightning damage/channel behavior. Reef
  Harpoon is included in the approved aquatic route; the route adds no skiff damage.
- New items still need reviewed full profiles, attacks, durability, upgrades/
  repair where applicable, server-validated crafting and original inventory
  icons. These recommendations do not silently replace existing gear recipes.

## Proposed roster

Keep existing boars, deer and Mirelings. Add fourteen enemies, one complete
enemy per increment. All damage below is one authored server-resolved hit,
before existing defenses; no client-authored damage or offensive player magic.

| Biome | Enemy / ID | Role and attack | Health / Armor | Hit damage | Guaranteed loot |
| --- | --- | --- | --- | --- | --- |
| Meadows | Rat (`Rat`) | Small scavenger; bites only when approached within 3 m or attacked | 10 / 0 | 5 Pierce | 1 Bone Fragment |
| Elderwood | Badger (`Badger`) | Territorial animal; warns within 3 m, then slashes with its claws if approached or attacked | 100 / 10 | 20 Slash | 1 Bone Fragment |
| Elderwood | Moon Bear (`MoonBear`) | Rare hard enemy; slow overhead paw strike, punishable recovery | 250 / 20 | 75 Crush | 3 Bone Fragment |
| Shimmering Lakes | Viper (`Viper`) | Snake hiding in dry shoreline reeds; visible coil, narrow lunging bite, then retreats; applies weak poison for 5 s | 100 / 10 | 50 Stab + poison below | 1 Bone Fragment |
| Shimmering Lakes | Giant Snake (`GiantSnake`) | Rare hard snake hidden among shoreline reeds; raised-head coil, powerful lunging bite, then slow recovery | 250 / 30 | 100 Stab | 2 Bone Fragment |
| Mossy Mire | Death Adder (`DeathAdder`) | Highly venomous camouflaged snake; visible coil, short bite then retreat; major poison for 10 s | 125 / 20 | 75 Stab + poison below | 1 Bone Fragment |
| Mossy Mire | Mosquito swarm (`MosquitoSwarm`) | Flying swarm surrounds a player and keeps biting; 10 s weak poison; rolls break contact, lit flames repel it | 25 / 0 per swarm | 5 Stab/0.2 s + poison below | None |
| Mossy Mire | Peat Husk (`PeatHusk`) | Rare hard, original peat-and-wood creature; slow sweeping arm | 300 / 50 | 150 Crush | 1 Amber |
| Freezing Tundra | Wolf (`Wolf`) | Tough pursuing predator; visible forward lunge and powerful bite, then recovery | 250 / 40 | 100 Slash | 2 Bone Fragment |
| Freezing Tundra | Lynx (`Lynx`) | Mobile predator; telegraphed leap, long recovery if it misses | 150 / 30 | 75 Slash | 2 Bone Fragment |
| Freezing Tundra | Polar Bear (`PolarBear`) | Rare hard bear variant; committed charge, then retreat to reset | 750 / 60 | 200 Crush | 3 Bone Fragment |
| Thunder Mountains | Horned Goat (`HornedGoat`) | Territorial wildlife; telegraphed horn charge pushes and staggers | 200 / 50 | 50 Crush | 2 Bone Fragment |
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
  Exclude both from Mossy Mire, Freezing Tundra, Thunder Mountains and Ocean.
  Apply the server-side biome allowlist before archetype selection, including
  regenerated/reloaded population candidates; seed selection cannot bypass it.
  This controls spawn origin, not a new movement barrier. Use the revised
  stats below; retain other behavior and loot. Verify biome filtering across fixed seeds.
- Proposed selection weights are percentages of eligible candidate slots.
  Meadows uses 90% existing roster / 10% Rat. Each other land biome allocates
  50% to its existing eligible roster, 40% to new common enemies, and 10% to
  new hard enemies. If no existing entry is eligible, leave that share empty;
  do not import creatures from another biome. Spawned actors use the existing
  actor budget. A mosquito swarm is one gameplay actor with cosmetic insects.
- Mirelings spawn **only in Mossy Mire**, including seeded/reloaded candidates.
  Use the same server-side spawn-origin allowlist policy as boar/deer.
- Mossy Mire uses the full 50/40/10 distribution: 50% Mireling (existing),
  20% Death Adder, 20% Mosquito swarm, and 10% Peat Husk. These are overall
  candidate-slot weights, not shares within the common-enemy allocation.
- Freezing Tundra uses 50% existing eligible roster / 20% Wolf / 20% Lynx /
  10% Polar Bear. Elderwood uses Badger for its 40% common share. Wolves spawn
  only in Freezing Tundra, never Elderwood; each Wolf pack has three Wolves.
  Count every pack member against the existing actor budget. Pack selection
  and formation remain engine implementation work.
- Ocean: use 10% of eligible aquatic candidate slots for Reef Maw; others
  stay empty. This task is blocked until aquatic navigation, combat and reward
  support are approved. It targets swimmers, never skiff hulls or occupants.
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
- Horned Goat's **50 Crush** horn charge also pushes and staggers on a
  server-confirmed hit. Push distance and stagger duration require review
  before implementation; do not invent them during coding.
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

- A server-confirmed bite applies **major poison for 10 seconds**. Proposed
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
  readable against Mire ground; this is proposed tuning, not playtested balance.

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
