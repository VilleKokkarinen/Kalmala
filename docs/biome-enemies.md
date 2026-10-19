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
| Boar                | Meadows / Elderwood / Shimmering Lakes | Current          |  100 |     0 | 15 Crush     | Territorial charge                  |
| Deer                | Meadows / Elderwood / Shimmering Lakes | Current          |  100 |     0 | —            | Flees; alerts nearby deer           |
| Mireling            | Mossy Mire                             | Planned retune   |   60 |     0 | 8 Crush      | Weak base enemy; simple melee       |
| Brush Rat           | Meadows                                | Planned          |   35 |     0 | 6 Stab       | Defensive bite                      |
| Woodland Badger     | Elderwood                              | Planned          |   60 |     0 | 10 Stab      | Warns; bites; retreats              |
| Moon Bear           | Elderwood                              | Planned          |  250 |     5 | 35 Crush     | Slow overhead strike; 2% Moonstone  |
| Viper               | Shimmering Lakes                       | Planned          |   50 |     0 | 10 Stab      | Weak poison: 1/s, 5 s               |
| Giant Snake         | Shimmering Lakes                       | Planned          |  160 |     3 | 25 Stab      | Coiled lunge; slow recovery         |
| Bog Adder           | Mossy Mire                             | Planned          |   70 |     0 | 14 Stab      | Major poison: 5/s, 10 s             |
| Mire Mosquito swarm | Mossy Mire                             | Planned          |   25 |     0 | 2 Stab/0.5 s | Poison: 1/s, 10 s; roll/fire repels |
| Peat Husk           | Mossy Mire                             | Planned          |  220 |     6 | 30 Crush     | Slow sweeping arm                   |
| Tundra Wolf         | Freezing Tundra                        | Planned          |  160 |     3 | 28 Slash     | Pursuit; lunging bite               |
| Snow Lynx           | Freezing Tundra                        | Planned          |  120 |     0 | 22 Slash     | Leap; punish missed landing         |
| Tundra Bear         | Freezing Tundra                        | Planned          |  300 |     6 | 40 Crush     | Committed charge                    |
| Ridge Goat          | Thunder Mountains                      | Planned          |  120 |     2 | 20 Crush     | Territorial horn charge             |
| Crag Sentinel       | Thunder Mountains                      | Planned          |  280 |    10 | 38 Crush     | Slow downward slam                  |
| Reef Maw            | Ocean                                  | Planned, blocked |  180 |     2 | 28 Stab      | Swimmer hunter; aquatic approval    |

Current defaults: [wildlife](../Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp),
[health](../Source/KalmalaGameplay/Public/KalmalaWildlifeSpawn.h) and
[defenses](../Source/KalmalaGameplay/Public/KalmalaDamageProfile.h).
Detailed proposals and exceptions follow.

Draft for manual review, 2026-10-19. Implement through A8a in
[BACKLOG.md](../BACKLOG.md), before A9 gems and special gear.
These additions extend the current three-archetype roster; they do not change
historical vertical-slice acceptance or claim that these enemies exist today.
Names, stats, spawn weights and loot below are concrete proposals for editing.

## Unique drops

**Draft for review.** One roll per credited defeat; amount is awarded only on
success. These are additional to baseline loot. Sinew and the two gem routes
retain their existing plans; other drops and uses below are new proposals.
Unique means a characteristic drop, not an exclusive source for shared Sinew.

| Enemy            | Unique drop        | Amount / chance | Intended use              |
| ---------------- | ------------------ | --------------- | ------------------------- |
| Boar             | Boar Tusk          | 1 / 50%         | Weapon fittings           |
| Deer             | Sinew              | 1 / 50%         | Bow String; 4 Sinew each  |
| Rat              | Rat Tail           | 1 / 100%        | Fishing bait ingredient   |
| Badger           | Badger Claw        | 1 / 100%        | Claw weapon component     |
| Moon Bear        | Moonstone          | 1 / 2%          | Moonstone Ring            |
| Viper            | Viper Fang         | 1 / 50%         | Poison arrow ingredient   |
| Giant Snake      | Giant Snake Skin   | 2 / 100%        | Scaled armor component    |
| Mireling         | Dusk Diamond       | 1 / 0.5%        | Special Dusk Ring         |
| Death Adder      | Death Adder Venom  | 1 / 50%         | Strong poison ammunition  |
| Mosquito swarm   | Mosquito Proboscis | 1 / 25%         | Fine needle crafting      |
| Peat Husk        | Peat Heart         | 1 / 25%         | Swamp jewelry component   |
| Wolf             | Wolf Pelt          | 1 / 100%        | Fur clothing component    |
| Lynx             | Lynx Claw          | 1 / 50%         | Light weapon component    |
| Polar Bear       | Polar Bear Pelt    | 2 / 100%        | Heavy fur armor component |
| Horned Goat      | Goat Horn          | 1 / 100%        | Weapon/grip component     |
| Thunder Sentinel | Thunder Core       | 1 / 25%         | Lightning gear component  |
| Reef Maw         | Reef Tooth         | 1 / 50%         | Harpoon component; gated  |

New materials are exclusive to their named enemy in this proposal. No shared
venom/pelt substitution, gem conversion or automatic new equipment recipe.
Keep existing gem odds independent of baseline loot. Server owns rolls and
stable claims; full packs preserve unclaimed rewards, and retries/reloads
cannot reroll or duplicate them. Every new material requires its registered
catalogue definition and original generated icon. Proposed uses need separate
reviewed recipes/effects; fishing, cold-protection clothing, lightning gear
and harpoons remain blocked on their own scope decisions. Reef Maw's entire
route remains blocked on aquatic acceptance.

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
- Proposed selection: Meadows 90% existing roster / 10% Rat. Each other
  land biome: 50% existing eligible roster / 40% new common enemy / 10% new
  hard enemy. If there is no eligible existing entry, leave that share empty;
  do not import creatures from another biome. Each new spawn is one enemy
  actor; a mosquito spawn is one swarm actor with multiple cosmetic insects.
- Mirelings spawn **only in Mossy Mire**, including seeded/reloaded candidates.
  Use the same server-side spawn-origin allowlist policy as boar/deer.
- Mossy Mire splits its common share: 50% Mireling / 20% Death Adder /
  20% Mosquito swarm / 10% Peat Husk. Keep the existing actor budget;
  the visual insects do not create extra gameplay actors or damage sources.
- Freezing Tundra splits its common share: 50% existing roster / 20%
  Wolf / 20% Lynx / 10% Polar Bear. Elderwood uses Badger for
  its 40% common share. Wolves spawn only in Freezing Tundra, never Elderwood;
  keep the existing actor budget and one wolf per spawn (no pack mechanic).
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
  Keep existing loot and the planned 0.5% Mossy Mire Dusk Diamond route.
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
