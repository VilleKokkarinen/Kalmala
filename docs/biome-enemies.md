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
| Shimmering Lakes | Viper (`Viper`) | Snake hiding in dry shoreline reeds; visible coil, narrow lunging bite, then retreats; applies weak poison for 5 s | 50 / 0 | 10 Stab + poison below | 1 Bone Fragment |
| Shimmering Lakes | Reed Lurker (`ReedLurker`) | Rare hard, original shoreline beast; two-legged rush and broad forelimb swipe on land | 160 / 3 | 25 Crush | 2 Bone Fragment |
| Mossy Mire | Bog Adder (`BogAdder`) | Highly venomous camouflaged snake; visible coil, short bite then retreat; major poison for 10 s | 70 / 0 | 14 Stab + major poison below | 1 Bone Fragment |
| Mossy Mire | Mire Mosquito swarm (`MireMosquito`) | Flying swarm surrounds a player and keeps biting; 10 s weak poison; rolls break contact, lit flames repel it | 25 / 0 per swarm | Proposed 2 Stab every 0.5 s while in contact + poison below | None |
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
  do not import creatures from another biome. Each new spawn is one enemy
  actor; a mosquito spawn is one swarm actor with multiple cosmetic insects.
- Mossy Mire splits its common share: 50% existing roster / 20% Bog Adder /
  20% Mire Mosquito swarm / 10% Peat Husk. Keep the existing actor budget;
  the visual insects do not create extra gameplay actors or damage sources.
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
- Mire Mosquito uses the dedicated swarm rules below instead of isolated
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

## Viper poison

- A server-confirmed bite applies **weak poison for 5 seconds**. Proposed
  tuning: 1 Poison damage each second, first tick after 1 s, five ticks total.
  Keep the existing 10 Stab bite, 50 health, 0 Armor and loot.
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

## Bog Adder major poison

- A server-confirmed bite applies **major poison for 10 seconds**. Proposed
  tuning: **5 Poison/second**, first tick after 1 s, ten ticks (50 total before
  Poison defenses). Keep 14 Stab on the bite, 70 health, 0 Armor and loot.
- Further Bog Adder bites refresh the duration without stacking or extra
  immediate ticks; multiple adders share one major effect. Weak and major
  poison have separate expiry timers but one shared one-second damage clock:
  apply only the strongest active dose, never add their damage together.
  Weak bites cannot extend major poison; any still-active weak effect resumes
  at its normal strength after major expires. Applications never reset the clock.
- Use the Viper rules for server validation, Poison defenses, owner display
  and death/session cleanup. Physical Armor does not prevent application on
  an accepted bite. Rolls avoid bites but do not cleanse existing poison;
  fire does not repel Bog Adders or remove poison.
- Verify ten-second expiry, refresh, multiple adders, weak/major overlap,
  defenses, rejected bites and owner/client display. Keep its coiled windup
  readable against Mire ground; this is proposed tuning, not playtested balance.

## Mire Mosquito swarm

The user-selected behavior is persistent small bites until the player rolls
away or holds a lit fiery item. Mosquitoes avoid active fires and must not
attack players holding a lit torch. The following numbers are proposed tuning.

- One swarm actor represents six original cosmetic mosquito silhouettes,
  with audible buzzing and 25 shared health. Fly 0.5–2.5 m above local ground;
  approach visibly for 0.5 s before harassment. No instant spawn on a player.
- Harass one target within 1 m, applying **2 Stab every 0.5 s** with no retreat
  between bites. One attacking swarm per player; other swarms wait outside
  contact range rather than stacking bite rates. Missed/late server ticks do
  not burst catch-up damage. Visible insects never inflict separate hits.
- Apply existing typed defense, physical Armor and accepted-hit wearable-wear
  rules once per tick. Armor may reduce damage to zero; this proposal adds no
  armor bypass. Contact Stab damage is capped at 4 per second before defenses for each
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
