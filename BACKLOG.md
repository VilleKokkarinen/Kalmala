# Development backlog

## Active queue

M0–M14, world-generation Phases 1–9, and the defined attack-driven
tree-harvesting increment remain complete under their recorded acceptance
scope. The following user-requested post-roadmap goal is now queued.

### Sea-level rivers and priority terrain carving — user request, 2026-10-18

**Goal:** Put generated river water at the existing zero-centimetre sea plane
and give river corridors priority over land height shaping. A river crossing
high ground must carve below the water plane and form smooth banks on both
sides, rather than leaving a raised ridge or abrupt trench. Retain seeded path
determinism and the shared terrain sampler used by rendering, collision and
maps.

**Plan and acceptance direction:**

- Keep current deterministic spline placement and biome identity. Flatten river
  water to Z=0, carve a bed below that plane, and apply the channel profile
  after the surrounding land/coast height constraints so the land clamp cannot
  refill it.
- Shape the channel with a bounded, continuous two-sided bank blend that
  returns to the unmodified terrain outside the river influence. Tune the bank
  extent against generated high ground; preserve the source land/water coast
  away from river corridors.
- Make the final sampled channel drive terrain rendering/collision and
  sea-level water clipping consistently. Ensure minimap/world-map water
  classification follows the resulting channel. Preserve enclosed-lake levels
  and define lake/rivers overlap so the join has no height or water-level step.
- Add focused generation coverage for a river crossing high land, both bank
  profiles, deterministic repeat/seed behavior, spline-cell and terrain-patch
  seams, and unchanged terrain/coast/lake samples outside or beneath river
  influence. Inspect generated height/water previews and run the applicable
  regional-generation and host/client sampler checks.
- Update the terrain/hydrology contract and verification instructions to match
  the accepted behavior. No new RPC, replicated property, or save data is
  planned.

**Implementation sequence (each checkbox targets roughly 15 minutes):**

- [x] Add named river-bed depth and bank-extent tuning values in
  `KalmalaRegionalTuning.h`; leave generated samples unchanged.
- [x] Extract the existing deterministic river-segment projection and distance
  calculation into a small helper, preserving current sample outputs.
- [x] Add and integrate a bounded channel profile: set river water to Z=0,
  place the channel floor below it, and apply the result after the land/coast
  clamp using the existing river influence width.
- [x] Extend the profile into a smooth two-sided bank shoulder and confirm it
  returns to the original terrain outside its influence radius.
- [x] Preserve lake-basin level precedence where rivers overlap and confirm
  river shaping does not move the ocean coast outside the channel corridor.
- [x] Check the existing sea-water mesh and terrain collision against the new
  shared channel sample; fix a mismatch only if inspection finds one.
- [x] Align minimap river-depth sampling with the fixed sea-level channel and
  confirm the expanded map classifies the carved channel as water.
- [x] Add a focused regression for a high-ground river center: water level is
  Z=0 and the sampled bed lies below it.
- [x] Add a cross-section regression for both banks, including the smooth
  return to unchanged terrain beyond the bank influence.
- [x] Add a same-seed repeatability regression for river height and water
  samples, plus a different-seed variation check.
- [x] Add a hydrology-grid seam regression for channel height and water output.
- [x] Add a terrain-patch seam regression for channel height and sea-water
  clipping on both sides of a patch boundary.
- [x] Add control samples proving terrain away from rivers, the source coast,
  and enclosed-lake levels retain their established outputs.
- [x] Update the architecture contract for sea-level channels, bank shaping,
  lake precedence, and shared deterministic sampling.
- [x] Update the setup guide with the focused generation checks and expected
  preview/peer results.
- [x] Build the affected Editor target once the sampler and consumer changes
  are complete.
- [x] Run the focused regional-generation automation cases and inspect their
  pass/fail output.
- [x] Export and inspect a high-ground river preview; repeat the same seed and
  compare it with a different seed.
- [x] Run the regional-generation checks for deterministic preview and
  host/client sampler agreement with
  `Scripts/Tests/World/Verify-RegionalGeneration.ps1`.
- [x] Run the minimap host/client terrain, water, and river-signal agreement
  check with `Scripts/Tests/World/Verify-Minimap.ps1`.

If any checkbox expands beyond the target, split it further before starting
that increment and keep the remaining work in this order.

### Alpha item coverage by category — user request, 2026-10-19

**Planning basis:** [current inventory, Valheim sources and gap analysis](docs/alpha-items.md).
This queues item work; it does not declare a release, change completed roadmap
acceptance, or approve new systems beyond the existing vertical slice. Follow
the categories in order after the river queue. P0 closes existing loops; P1 is
recommended alpha breadth; P2 is scope-dependent. Each child is a separate
increment; split larger children into integrated increments before starting.
Blocked children require the named decision/dependency; do not treat proposals
as settled design. All names, costs, sources, stats and rules below are concrete
draft proposals for user review/editing, not permission to implement new scope.
Agents must follow the reviewed values; missing design values must be proposed
here for review before implementation, not invented while coding.

**Completion for every item:** normal acquisition and usable behavior; applicable
manifest-registered item/profile/recipe/upgrade/reward/attack data; original
generated/imported icon, provenance, runtime mapping and icon validation;
server-owned payment/use/targets/quantities; documented persistence and explicit
non-applicable records. Buildables require supported hammer placement and raw
materials, never inventory kits. No retired Reed Fibre or offensive magic.

- [ ] **A0 — Existing item acquisition and use (P0).**
  - [ ] Before implementation, review the concrete A0–A9 proposals and A10 milestone groups with the user, recording included/deferred scope and final verification gates. New-system proposals stay blocked until their scope decisions are accepted; do not delegate their design back to implementation agents.
  - [ ] Audit normal world-to-use reachability against the material/source proposal below; report deviations and bootstrap cycles rather than selecting replacement sources. Verify generated nodes for the current direct-bar Iron path; its proposed replacement is explicitly listed under A1.
  - [ ] Add Workbench bark-string crafting: consume 2 Wood to produce 1 String, representing stripped and twisted inner bark; no new fibre item or tool requirement. Verify a fresh player can gather Wood with Stone Axe, craft and maintain Bow without debug grants.
  - [ ] Add harvestable aspen trees in Meadows yielding 4 Lightwood per completed tree/log/bundle lifecycle; require Stone Axe or better. Preserve existing birch Wood rewards. Verify Bronze-family recipes/upgrades are reachable without requiring a Bronze tool to obtain Lightwood.
  - [ ] Add 1 Bone Fragment to each server-confirmed boar or deer defeat, alongside existing meat/hide rewards, paid once to the validated attacker. Retain no-reward behavior for uncredited defeats; validate reward capacity and prevent duplicate grants. Verify Boar Hide Cape crafting/upgrades.
  - [ ] Reconcile the six current cooking outputs with server food-use dispatch; remove stale food-ID assumptions and verify paid cooking-to-eating behavior.
  - [ ] Add Hearth Broth at a Cauldron with the existing lit-hearth requirement: 1 Deer Meat + 1 Onion produces 1 Hearth Broth; MaxBatch 5, Cooking XP 10 per accepted craft. This proposed recipe changes the earlier intentional no-recipe state only after review.
  - [ ] Add hand-gathered wild carrot patches in Meadows: each harvest grants 2 Carrot and 1 Carrot Seed; no tool required. Seed use remains blocked on A7 farming.
  - [ ] Add hand-gathered wild potato patches in Meadows: each harvest grants 2 Potato; no tool required. Do not grant Potato Seed until A7 explicitly resolves seed versus tuber propagation.
  - [ ] Add hand-gathered wild rutabaga patches in Shimmering Lakes on dry banks: each harvest grants 2 Rutabaga and 1 Rutabaga Seed; no tool required. Seed use remains blocked on A7 farming.
  - [ ] Add hand-gathered wild onion patches in Elderwood: each harvest grants 2 Onion and 1 Onion Seed; no tool required. Seed use remains blocked on A7 farming. Verify all four vegetables reach the existing cooking recipes without farming/debug grants.

  **Material/source proposal for manual review — not implemented or approved.**
  The following fixes the proposed sources, actions and yields now. Agents must
  implement the reviewed entries, not invent alternative biomes, drops, recipes
  or quantities. User edits take precedence. If an entry is not implementable,
  report the conflict before changing its design. Rows marked **retain** describe
  existing routes to preserve and verify; **add/change** rows await review.

  | Material / ingredient | Proposed source and biome | Gathering / production and yield | State / dependency |
  | --- | --- | --- | --- |
  | Wood | Existing Meadow birch and existing wood-resource trees | Existing axe/tree/log/bundle flow, Stone Axe or better where already required; retain each source's authored yield | Retain; no reed-source reward changes |
  | Densewood | Existing Elderwood ironheart trees | Existing Iron Axe tree/log/bundle flow and authored yield | Retain; Iron Pick/ore route below must not require Densewood |
  | Lightwood | New aspen trees in Meadows | Stone Axe or better; 4 Lightwood on final bundle pickup | Add under A0; no Bronze prerequisite |
  | String | Inner bark from existing Wood, at Workbench | Craft 2 Wood into 1 String; no fibre intermediate | Add under A0 |
  | Stone | Existing mountain slate veins and existing Stone sources | Existing Stone Pick or better and authored yields; retain existing starter acquisition | Retain; audit fresh-start bootstrap |
  | Boar Hide / Boar Meat | Existing boars | Retain existing server-confirmed defeat rewards and quantities | Retain |
  | Deer Hide / Deer Meat | Existing deer | Retain existing server-confirmed defeat rewards and quantities | Retain |
  | Bone Fragment | Existing boars and deer | Additional 1 Bone Fragment per credited defeat | Add under A0; no new creature |
  | Amber | Existing Mossy Mire peat-amber seams | Stone Pick or better; retain existing deterministic source yield | Retain |
  | Frost Salt | Existing Freezing Tundra deposits and frostmoss | Retain existing source-specific pick/knife actions and authored yields | Retain |
  | Copper Ore | New exposed copper-bearing rock nodes in Elderwood | Stone Pick or better; 3 Copper Ore per exhausted node | Add under A1 |
  | Tin Ore | New tin-bearing gravel nodes on dry Shimmering Lakes shores | Stone Pick or better; 3 Tin Ore per exhausted node | Add under A1 |
  | Iron Ore | Existing Mossy Mire bog-iron nodes | Bronze Pick or better; 3 Iron Ore per exhausted node, replacing direct Iron-bar reward | Change under A1; preserve existing behavior until approved |
  | Coal | Wood burned in a placed charcoal station | 1 Wood produces 1 Coal; no mined Coal source, no new fuel item | Add under A1; processing scope gated |
  | Copper | Copper Ore refined at placed smelter | 1 Copper Ore + 1 Coal produces 1 Copper | Add under A1; processing scope gated |
  | Tin | Tin Ore refined at placed smelter | 1 Tin Ore + 1 Coal produces 1 Tin | Add under A1; processing scope gated |
  | Bronze | Copper/Tin alloying at Forge | 2 Copper + 1 Tin produces 1 Bronze | Add under A1; no direct Bronze drops |
  | Iron | Iron Ore refined at placed smelter | 1 Iron Ore + 1 Coal produces 1 Iron | Add under A1; replaces direct-bar acquisition only after review |
  | Carrot / Carrot Seed | New wild carrot patches in Meadows | Hand gather 2 Carrot + 1 Carrot Seed per patch | Add under A0; seed use waits for A7 |
  | Potato | New wild potato patches in Meadows | Hand gather 2 Potato per patch | Add under A0 |
  | Potato Seed | No A0 acquisition | Existing definition retained without drops; A7 proposes Potato tuber propagation instead | Explicitly deferred, not agent-selected |
  | Rutabaga / Rutabaga Seed | New wild rutabaga patches on dry Shimmering Lakes banks | Hand gather 2 Rutabaga + 1 Rutabaga Seed per patch | Add under A0; seed use waits for A7 |
  | Onion / Onion Seed | New wild onion patches in Elderwood | Hand gather 2 Onion + 1 Onion Seed per patch | Add under A0; seed use waits for A7 |
  | Hearth Broth | Cauldron near usable lit hearth | 1 Deer Meat + 1 Onion produces 1 Hearth Broth; MaxBatch 5, Cooking XP 10 | Add under A0 after review |

  New harvest nodes use stable generated identities and the existing matching
  node depletion/restart policy; this proposal adds no timed respawn system.
  A full inventory must leave a harvest unclaimed, with no partial plant reward
  or depletion. New-tree quantities apply at bundle pickup, not per axe hit.
  Station placement costs, processing timing/capacity and farming rules are
  proposed in A1/A7 below and likewise await manual review.

- [ ] **A1 — Ores, bars and fuel (P0; new processing scope gated).**
  - [ ] Review A0's source table and the station proposal below; confirm Stone Pick → Copper/Tin → Bronze Pick → Iron Ore bootstrap and approve processing scope before implementation. Preserve current behavior until approved.
  - [ ] Add the table's Elderwood Copper Ore and Shimmering Lakes Tin Ore nodes: Stone Pick minimum, 3 matching ore per exhausted node (blocked on A1 contract).
  - [ ] Implement the table's charcoal-station conversion of 1 Wood to 1 Coal with direct-material hammer placement; no alternative Coal source (blocked on A1 contract).
  - [ ] Implement the table's smelter conversions and Forge alloying: 1 Copper/Tin Ore + 1 Coal → 1 matching bar; 2 Copper + 1 Tin → 1 Bronze (blocked on ore/Coal acquisition and A1 contract).
  - [ ] Change Mossy Mire bog-iron nodes from direct Iron bars to 3 Iron Ore per node with Bronze Pick minimum; smelt 1 Iron Ore + 1 Coal → 1 Iron and reconcile documentation (blocked on A1 contract and refining).

  **Processing proposal — review before implementation:**

  | Buildable / process | Direct-material placement or recipe | Operation |
  | --- | --- | --- |
  | Charcoal Kiln (`CharcoalKiln`) | Hammer: 12 Stone + 6 Wood | 1 Wood → 1 Coal in 30 seconds; 10-input/10-output capacity, one job at a time |
  | Smelter (`Smelter`) | Hammer: 18 Stone + 8 Wood | 1 ore + 1 Coal → 1 matching bar in 30 seconds; 10 ore/10 Coal/10 output capacity; Copper, Tin, Iron only |
  | Bronze alloying | Forge level 1: 2 Copper + 1 Tin → 1 Bronze | Immediate paid craft, MaxBatch 5; no additional Coal or skill requirement |

  Kiln/smelter need grounded placement, a clear 1 m interaction approach and
  2 m overhead clearance. Internal heat works in rain and needs no campfire.
  Pause when output is full; reserve inputs once per job. Server-save queued
  inputs, outputs and remaining active-game seconds; no offline processing.
  Reject removal while contents/jobs remain. Output collection uses validated
  inventory capacity and cannot duplicate rewards after restart. No station
  inventory kit, upgrades, extra ore types or new fuel identity.

- [ ] **A2 — Melee weapons (P1).**
  - [ ] Review the sword/club table below; retain the proposed Wooden Club as an early Wood-only option alongside Stone Mace unless manually removed from scope.
  - [ ] Add Bronze Sword (`BronzeSword`) through the complete reviewed craft/attack/upgrade/appearance/icon path (blocked on reachable Bronze inputs).
  - [ ] Add Iron Sword (`IronSword`) through the complete reviewed path (blocked on sword support and reachable Iron inputs).
  - [ ] Add Wooden Club (`WoodenClub`) through the complete reviewed path; no gathering role or metal prerequisite.

  | Item | Craft station / cost for 1 | Level-one profile | Upgrade base cost / per-level reward |
  | --- | --- | --- | --- |
  | Bronze Sword | Forge 1; 4 Bronze + 2 Lightwood + 1 String | One hand, 100 condition, 35 Slash, 2 kg | 2 Bronze + 1 Lightwood; +30 condition, +4 Slash |
  | Iron Sword | Forge 1; 4 Iron + 2 Densewood + 1 String | One hand, 180 condition, 55 Slash, 2.5 kg | 2 Iron + 1 Densewood; +30 condition, +4 Slash |
  | Wooden Club | Workbench 1; 4 Wood | One hand, 40 condition, 10 Crush, 1 kg | 2 Wood; +20 condition, +2 Crush |

  All use the existing Basic melee timing/reach, zero attack-style bonus on
  top of the listed profile damage, and one condition per accepted hit. No
  alternate attacks, harvesting, elemental channels or new block/parry rules.
  Stack 1; use existing level cap and upgrade-cost scaling. Upgrade at the
  crafting station; free existing station repair. Original sword/club silhouettes
  and icons follow the established art treatment.

- [ ] **A3 — Bows and ammunition (P1).**
  - [ ] Review and add Layered Bow (`LayeredBow`): Workbench 1, 6 Lightwood + 4 String + 2 Deer Hide → 1; stack 1, 1.5 kg, 140 condition, two hands, 1.2-second full draw. Keep existing arrow-dependent damage, velocity and spread curves; no bow damage bonus. Upgrade base 3 Lightwood + 2 String, +30 condition/level, existing cost scaling/cap; repair at Workbench (blocked on A0 String/Lightwood).
  - [ ] Review and add Bronze Arrow (`BronzeArrow`): Workbench 1, 4 Wood + 1 Bronze → 20; stack 100, 0.05 kg each. Pierce 14–56 across the existing charge curve versus Wooden Arrow's 10–40; retain current velocity/spread, gravity, collision and lifetime. No recovery or elemental damage (blocked on A1 Bronze).
  - [ ] Extend preferred-stack selection to both arrow IDs: exact selected stack first, otherwise Wooden Arrow before Bronze Arrow, lowest occupied cell first within a type. Consume one arrow and one bow condition only on committed server shot. Show selected arrow damage/type; no silent spending of a different type while the preferred stack remains usable.

- [ ] **A4 — Armour, capes and shields (P1).**
  - [ ] Review the armour/shield proposal below; retain existing hide Boots/Cape and explicitly defer second-tier Boots/Cape and insulation/cold protection for this alpha set.
  - [ ] Add Bronze Helmet (`BronzeHelmet`), Head slot, using the complete reviewed path (blocked on A1).
  - [ ] Add Bronze Tunic (`BronzeTunic`), Chest slot, using the complete reviewed path (blocked on A1).
  - [ ] Add Bronze Leggings (`BronzeLeggings`), Legs slot, using the complete reviewed path (blocked on A1).
  - [ ] Add Bronze Shield (`BronzeShield`), Shield slot, using the complete reviewed path; passive Armor only (blocked on A1).

  | Item | Forge 1 craft cost for 1 | Armor / condition / weight | Upgrade base cost |
  | --- | --- | --- | --- |
  | Bronze Helmet | 3 Bronze + 1 Boar Hide | 3 / 100 / 2 kg | 1 Bronze + 1 Boar Hide |
  | Bronze Tunic | 5 Bronze + 2 Boar Hide | 5 / 120 / 5 kg | 2 Bronze + 1 Boar Hide |
  | Bronze Leggings | 4 Bronze + 2 Boar Hide | 4 / 100 / 4 kg | 2 Bronze + 1 Boar Hide |
  | Bronze Shield | 3 Bronze + 3 Lightwood + 1 Boar Hide | 9 / 100 / 5 kg | 1 Bronze + 1 Lightwood |

  Stack 1. Each level adds 1 Armor and 10 maximum condition with existing
  upgrade scaling/cap at Forge; existing free station repair and damage wear.
  No movement penalties, elemental resistances, active block/parry or set bonus.

- [ ] **A5 — Carried utility tools (P1/P2).**
  - [ ] Review and add Bark Torch (`BarkTorch`): inventory craft 2 Wood + 1 String → 1, stack 1, 0.5 kg. Main Hand only; 300 seconds of burning while held, local light radius 6 m from replicated lit state. No damage, warmth, Wet removal or world ignition. Uncovered rain extinguishes it; dry re-equip relights it. Stored/unequipped torches do not burn; spent torch remains unusable until inventory refuel with 1 Wood restores 300 seconds. Persist remaining burn time with item state; no repair/upgrade recipe or generic melee fallback.
  - [ ] Review and add Cultivation Rake (`CultivationRake`) only with A7: Workbench 1, 2 Wood + 2 Stone + 2 String → 1; stack 1, 1 kg, 100 condition. Hold to plant/harvest approved garden plots; one condition per accepted planting, none on harvest/rejection. Free Workbench repair, no upgrades, damage or terrain editing.
  - [ ] Record terrain-editing Hoe as deferred from this alpha proposal: no height edits, smoothing, roads or grass-painting system. The Rake works only on A7 plots and does not modify the shared terrain sampler.

- [ ] **A6 — Forage, meals and recovery consumables (P1; effects gated).**
  - [ ] Review and add Lingonberry (`Lingonberry`): hand-gather Meadow berry shrubs, 3 berries per depleted shrub; no tool, stack 20, 0.05 kg each. Eating one applies the existing Steady Meal effect (blocked on A0 food dispatch).
  - [ ] Review and add Forest Chanterelle (`ForestChanterelle`): hand-gather Elderwood mushroom patches, 2 per depleted patch; no tool, stack 20, 0.1 kg each. Eating one applies the same Steady Meal effect (blocked on A0 food dispatch).
  - [ ] Retain one Steady Meal effect for all seven existing prepared foods, the two forage foods and A7 Cooked Lake Fish: 120 seconds, stamina-use multiplier 0.90, no stacking or eating while active. Raw meats/vegetables/seeds remain ingredients, not consumable meals. Defer differentiated nutrition, additional meal slots and new prepared-dish volume.
  - [ ] Review the recovery table below and approve its new server-owned consumable effects before implementation; use Cauldron at a lit hearth, no new brewing station, alcohol/base items or fermentation step.
  - [ ] Add Berry Recovery Draught (`BerryRecoveryDraught`) with the complete reviewed recipe/effect/icon path (blocked on recovery approval and Lingonberry).
  - [ ] Add Root Stamina Draught (`RootStaminaDraught`) with the complete reviewed recipe/effect/icon path (blocked on recovery approval and vegetable access).
  - [ ] Explicitly defer cold/poison/fire-resistance preparations: no consumable hazard bypass or new resistance channel in this alpha set.

  | Consumable | Cauldron cost / output | Effect / limit |
  | --- | --- | --- |
  | Berry Recovery Draught | 3 Lingonberry + 1 Carrot → 1 | Restore 20 health over 10 seconds; stop on death; 60-second recovery cooldown |
  | Root Stamina Draught | 2 Potato + 1 Onion → 1 | Restore 20 stamina immediately, clamped to existing maximum; 60-second recovery cooldown |

  Both stack 10, weigh 0.2 kg, MaxBatch 5, Cooking XP 10 per crafted dose;
  no upgrades/equipment/attack records. Shared cooldown prevents alternating
  doses; reject without consumption during cooldown, at full relevant stat,
  while dead or during active healing from the same draught. Support magic
  remains separately governed by its existing rules. Effects/cooldown are
  session state and clear on reconnect; inventories retain existing persistence
  limits. No released-save migration or new general inventory persistence.

- [ ] **A7 — Crops, seeds and fishing (P2; scope gated).**
  - [ ] Review and approve Garden Plot (`GardenPlot`) plus the farming table below: Hammer placement 4 Wood + 1 Stone, grounded 2 m × 2 m plot on dry Meadows/Elderwood/Shimmering Lakes terrain, maximum 15-degree slope. One crop per plot, open sky, no watering/fertilizer/season or terrain deformation. Require held usable A5 Rake for planting/harvest; reject conflicting/occupied plots before payment.
  - [ ] Implement Carrot planting, maturation, harvest and seed renewal as the first full loop, with A5 Rake and direct-material plot placement (blocked on farming approval).
  - [ ] Implement Potato tuber propagation; retain `PotatoSeed` as an unused development definition with no drops/recipes, not an alternative planting input (blocked on first crop loop).
  - [ ] Implement Rutabaga food/seed propagation from the table (blocked on first crop loop).
  - [ ] Implement Onion food/seed propagation from the table (blocked on first crop loop).

  | Crop mode | Planting payment | Harvest after 600 active-game seconds |
  | --- | --- | --- |
  | Carrot food | 1 Carrot Seed | 3 Carrot |
  | Carrot seed | 1 Carrot | 3 Carrot Seed |
  | Potato food | 1 Potato | 3 Potato |
  | Rutabaga food | 1 Rutabaga Seed | 3 Rutabaga |
  | Rutabaga seed | 1 Rutabaga | 3 Rutabaga Seed |
  | Onion food | 1 Onion Seed | 3 Onion |
  | Onion seed | 1 Onion | 3 Onion Seed |

  Persist plot mode and remaining active-game seconds with construction state;
  pause growth while its region is inactive and while server is stopped. No
  offline growth. Harvest once, retain mature crop on full inventory, then empty
  plot for reuse; reject removal while planted. Plants are plot state, never
  portable buildable items. Seeds/produce use their existing icons/stack sizes.

  - [ ] Review fishing scope: use visible stationary Lake Fish schools on Shimmering Lakes water, not a new combat creature. Each stable school permits 3 catches then depletes under the existing resource-node policy; no timed respawn. Approve the rod/bait/catch proposal before implementation.
  - [ ] Add Fishing Rod (`FishingRod`): Workbench 1, 4 Lightwood + 3 String → 1; stack 1, 1 kg, 80 condition, two hands, free Workbench repair, no upgrades/combat. Add Bait (`FishingBait`): inventory craft 1 Forest Chanterelle → 5, stack 50, 0.02 kg each (blocked on fishing approval/A0/A6).
  - [ ] Add school interaction: usable held Rod, selected school within 15 m and clear line of sight; reserve 1 Bait and a free result cell. After 5 seconds, if player stayed within 1 m of start and target remains valid, grant 1 Lake Fish and spend bait/1 rod condition/1 school catch atomically. Cancellation/rejection releases reservation unchanged; one angler per school, no arbitrary client fish IDs or timing rewards.
  - [ ] Add Lake Fish (`LakeFish`), stack 20, 0.5 kg, raw ingredient only; Cooking Rack at lit hearth converts 1 to 1 Cooked Lake Fish (`CookedLakeFish`), stack 20, 0.4 kg, MaxBatch 5, Cooking XP 10, existing Steady Meal effect. Complete fish/bait/rod icons and cooking-to-use path (blocked on fishing and A0 food dispatch).

- [ ] **A8 — Buildables, processing and sanctuary supplies (P2; no kits).**
  - [ ] Review the shelter table below and approve the new snapping/support/interaction scope. Every row is a Buildable with a direct-material Hammer recipe and original build-menu icon; no inventory definition/kit or item upgrade tree.
  - [ ] Add Timber Door (`TimberDoor`) with its reviewed paid placement, collision and saved open/closed state (blocked on shelter approval).
  - [ ] Add Timber Beam (`TimberBeam`) with reviewed support rules (blocked on shelter approval).
  - [ ] Add Timber Stairs (`TimberStairs`) with reviewed walkable collision (blocked on shelter approval).
  - [ ] Add Camp Bed (`CampBed`), furniture-only resting behavior below; do not add sleep/respawn binding (blocked on shelter approval).
  - [ ] Add Standing Light (`StandingLight`) with reviewed fuel/rain behavior (blocked on shelter approval).
  - [ ] Add Reinforced Chest (`ReinforcedChest`) as a separate placed chest, not an in-place inventory-capacity upgrade (blocked on shelter approval/A1).
  - [ ] Record recovery-station expansion as deferred: A6 uses the existing Cauldron; no fermenter, brewing tree or extra cooking station. A1 Kiln/Smelter and A7 Garden Plot are owned by their sections, not duplicate tasks here.

  | Buildable | Hammer material cost | Dimensions / behavior |
  | --- | --- | --- |
  | Timber Door | 4 Wood + 1 String | 1 m wide × 2 m high; snaps to existing wall edge grid, takes place of a wall segment rather than overlapping it; opens/closes by server interaction within 2 m, reject closing through a pawn |
  | Timber Beam | 2 Wood | 2 m beam, 0.15 m square; horizontal or vertical snap to existing structural endpoints; requires ground or supported endpoint, uses existing structural support model without new material tiers |
  | Timber Stairs | 6 Wood | 2 m run × 1 m width × 1 m rise; grounded lower edge or supported floor attachment; solid walkable steps, no ladder/climb mechanic |
  | Camp Bed | 6 Wood + 2 Boar Hide + 2 String | 2 m × 1 m; supported dry floor, 1 m clear approach; interaction within 2 m starts a local resting pose until movement/damage; no extra stat effect, time skip or spawn change |
  | Standing Light | 3 Wood + 1 String | Grounded/floor-supported, 1.5 m tall; starts unlit, consume 1 Wood for 600 active lit seconds, fuel cap 1200 seconds; 6 m cosmetic light, uncovered rain pauses flame/fuel, no warmth/Wet removal/world ignition |
  | Reinforced Chest | 8 Wood + 2 Bronze | Grounded/floor-supported 1.5 m × 1 m; 24 shared stack cells, existing per-item stack limits and validated GUID transfers, no weight/owner-lock mechanic |

  Use the existing construction health/rain-wear/repair rules of the matching
  Wall (door/beam/stairs) or Storage (bed/light/chest) category; no new defense
  stats. Reject overlap/unsupported placement. Save placed identities/transforms,
  door state, light fuel and chest contents through the construction/storage
  owners; cosmetic resting is session-only. Reject occupied-chest removal;
  empty-piece removal follows existing material-return policy. No new offline
  fuel drain. If existing wall geometry cannot host the proposed door, report
  that contract conflict before substituting a new wall/frame design.

- [ ] **A9 — Travel, accessories and discovery rewards (P2; scope gated).**
  - [ ] Review travel scope: propose only Skiff Cargo Chest (`SkiffCargoChest`), defer cart/carry-pack progression, encumbrance and hull durability. Hammer placement costs 6 Lightwood + 2 String + 1 Bronze, on one fixed cargo socket per existing skiff; no skiff inventory item or new boat tier.
  - [ ] Implement the approved cargo chest: 12 shared stack cells, interact within 2 m while skiff speed is at most 100 cm/s, same GUID/quantity/capacity rules as land storage. Save cargo against stable vessel ID; reject detachment/removal with contents, retain contents across validated vessel reload, no spill/duplication on reconnect. In-flight transfer rejects unchanged (blocked on explicit approval and existing vessel-persistence support; report any persistence gap).
  - [ ] Review the accessory table below before enabling Necklace/Ring slots. One item per named slot, existing validated equip/replace/return and full-pack rules; no damage/Armor/weight-capacity bonus or stacking copies. New effects require scope approval.
  - [ ] Add Amber Necklace (`AmberNecklace`) with reviewed craft/equip/effect/icon path (blocked on accessory approval).
  - [ ] Add Bone Ring (`BoneRing`) with reviewed craft/equip/effect/icon path (blocked on accessory approval).
  - [ ] Audit the four existing Mending/Hearth Shield/Bear's Vigor/Deer Call scrolls against their current authored discovery-to-unlock/use paths. Preserve existing locations/entitlements; propose no new scroll, trophy, creature or boss key for this alpha set. Record missing current routes as defects against the existing contract, not permission to invent a reward source.

  | Accessory | Workbench 1 cost for 1 | Server-owned effect while equipped |
  | --- | --- | --- |
  | Amber Necklace | 2 Amber + 2 String | Multiply the wearer's Mending stamina cost by 0.90; no change to heal amount, cooldown or unlock requirement |
  | Bone Ring | 2 Bone Fragment + 1 String | Multiply the wearer's Hearth Shield stamina cost by 0.90; no change to shield amount, cooldown or unlock requirement |

  Both stack 1, weigh 0.1 kg, no durability/upgrades/repair/attacks. Effects
  are evaluated at activation, never retroactively; removing an accessory
  removes its future discount. Gear remains owner-private and session-only
  under the existing equipped-gear persistence limit; no new general gear-save
  system. Add explicit slot handling rather than disguising jewellery as armour.

- [ ] **A10 — Approved alpha item-set acceptance.**
  - [ ] Before coding, record user-reviewed include/defer status for every named item/buildable above and freeze their reviewed recipes/sources/stats. Proposed milestone groups: I0 A0 current-loop repairs; I1 A1 production; I2 A2–A6 equipment/forage/consumables except Rake; I3 A7 farming/fishing plus A5 Rake; I4 A8–A9 sanctuary/cargo/accessories. Each group requires its own final verification; scope-gated groups remain blocked until approved. This is a proposed schedule, not new completed roadmap history.
  - [ ] Reconcile delivery against the frozen checklist: each included ID must have source, payment, use/equip, upgrade or explicit non-applicability, original icon/provenance/mapping, relevant save behavior and linked verification evidence. Record every omitted candidate as deferred, not implicitly complete.
  - [ ] At each resulting milestone's final increment, run required Editor/project builds, applicable automation, catalogue/icon validation and relevant peer/persistence runners from `docs/07-development-setup.md`; repair milestone defects and rerun failing checks plus required full verification. Record actual commands/results; do not equate source inspection with execution.
  - [ ] Final progression scenario: fresh world, existing starter loadout only, gather normal resources → Workbench/String/Lightwood → Bow/arrows → Copper/Tin/Coal/Bronze → Bronze Pick → Iron/Densewood. Craft/equip/upgrade one of every included durable item; cook/eat each included meal and use each draught, retaining the specified cooldowns. No debug grants. Record biome/node availability rather than fabricating missing resources.
  - [ ] Final approved-system scenarios: plant/renew every included crop, restart planted plots/processing jobs, catch/cook fish, exercise door/stairs/light/chest, reload cargo with vessel and verify accessory discounts. For each applicable flow use host/client plus two competing requests: no duplicate or partial reward, invalid/range/full-pack requests unchanged, no private inventory leakage. Skip only explicitly deferred systems; verify session-only effects clear as specified.
  - [ ] Mark the approved item milestone complete only after required gates pass; document blockers and pre-existing failures with evidence. No whole-alpha release declaration, packaging/publication or broader scope change follows automatically from this checklist.

## User-requested fixes

- [x] Remove all portable construction kits, separate buildable metadata from inventory, require direct-material hammer placement for every object, and guard against future kit generation.

- [x] Reuse the Options ornament as the centered divider between inventory and equipment within the existing one-tile gap.

- [x] Move Grinding Stone from Workbench crafting to direct-material hammer construction only, retaining its 6 Wood / 4 Stone cost.

- [x] Remove the inventory ScrollBox, remembered scroll state and bottom scroll-hint line; mount content directly on the panel.

- [x] Combine inventory/equipment on one backplate, remove equipment heading/footer, add one-tile separation, Necklace/Ring placeholders and detailed empty-slot silhouettes.

- [x] Repair menu preview cache dependency mismatch by building the complete Editor target and invalidating the cache for all native project sources.

- [x] Present Worn Gear in the reference body layout with empty slot silhouettes, equipped catalogue icons, and inventory plank/tile backgrounds.

- [x] Keep equipped tools in their assigned hotbar cells, mark the exact equipped copy blue, and toggle with the same number key.

- [x] Restore hammer building-only controls and add bow hold/draw/release shooting, draw-circle feedback, DrawDuration and preferred/automatic arrow stacks.
- [x] Allow target-free melee swings with every usable equipped item and empty hands.
- [x] Add nearby automatic pickup with an Options toggle and Action-key crosshair pickup showing item amount/name.
- [x] Rotate and anchor every held tool to fit the player hand through arm animation.
- [x] Meadow trees accept Stone Axe or better and their logs yield Wood.

## Completed roadmap

| Milestone | Completed scope / acceptance |
| --- | --- |
| M0 — Bootstrap | Project setup, prototype map, packaged build and dedicated-server availability recorded. |
| M1 — Networked traversal and interaction | Replicated movement and server-validated interaction. |
| World generation, Phases 1–9 | Deterministic generation, survival, biome expansion, ocean travel and expanded maps. |
| M2 — Survival camp loop | Gathering, crafting, construction and camp persistence. |
| M3 — Elemental world prototype | Server-owned weather, wetness, exposure and recovery. |
| M4 — Combat and support magic | Combat, creature encounters, discoveries and support effects. |
| M5 — Vertical-slice finish | Complete under recorded acceptance scope. |
| M6 — Production hardening | Complete under recorded supported-session scope and limits. |
| M7 — Survival progression and gameplay UI | Complete. |
| M8 — Ocean and long-distance travel | Complete; dry-shore acceptance waived. |
| M9 — Expanded biome content | Complete under recorded scope. |
| M10 — Release completion and launch validation | Complete under recorded scope and limits. |
| M11 — User experience and visual UI | Final acceptance passed 2026-10-06. |
| M12 — Inventory menu and cleaner HUD | Final acceptance passed 2026-10-09. |
| M13 — Revalidated UI on main | Integrated-main acceptance passed 2026-10-11: 122/122 tests, rendered peers, input/privacy/reconnect and Development/Shipping smoke. [Evidence](docs/evidence/m13/README.md). |
| M14 — Expanded world map | Integrated-main acceptance passed 2026-10-12: editor build, 123/123 tests, live map runners, rendered/performance matrix. [Evidence](docs/evidence/m14/main-integration.md). |

## Completed post-roadmap work

**Player and creature surface winding — verified 2026-10-17.** Corrected
back-facing tapered player/tool faces and creature tetrahedra. Editor build and
four focused tests passed; normals remain outward and materials are unchanged.

**Held-tool appearance — verified 2026-10-17.** Replaced the fixed hammer with
Main Hand family silhouettes and empty-hand cleanup. Cosmetic family replication
keeps inventory details private. Editor build and four focused regressions passed.

**Live map heading — verified 2026-10-17.** Player arrows read the local pawn's
current facing without a terrain refresh. Editor build and three focused
map/minimap tests passed, including turns through all four map directions.

**Axe swing and animation — verified 2026-10-17.** Usable axes can swing without
an in-range target; misses complete cooldown quietly. Replicated accepted actions
drive the procedural arm animation. Editor build and four focused tests passed;
actual hit authority and material-tier gates remain unchanged.

**Hammer right-click crash repair — verified 2026-10-17.** Progression sorting
now handles spawn-granted equipment without an acquisition recipe. Editor build,
upgrade-only hammer regression and Build browsing passed; broader progression
cost/count failures are recorded in [PROGRESS.md](PROGRESS.md).

**Development build repair — verified 2026-10-17.** Removed local identifier
collisions exposed by unity compilation. Win64 Development game and Editor
unity builds passed; no gameplay behavior changed. See the [build repair
record](PROGRESS.md#user-directed-increment-2026-10-17--repair-development-game-unity-build).

**Attack-driven tree harvesting — accepted 2026-10-17.** Server-owned axe hits
advance generated Meadow/Elderwood trees through Standing, Falling, settled
Trunk and collectible Drop. Stable identities, cosmetic suppression, log/bundle
targeting, validated pickup, sparse depletion, patch eviction/reactivation and
real population-slot restart passed the defined final gates. The final audit
closed the parent tasks in commit `2b8c8cf`.

[Final acceptance and limits](docs/07-development-setup.md#final-tree-feature-evidence-audit-2026-10-17)
record the editor/focused automation, exact-ID peers/late joins/pickup, streaming
and restart evidence. Rendered/audible usability, Falling-phase late join, slope
fitting, continuous travel, adverse networking, two remote contenders, live peer
process restart, full suite and packaging remain outside that acceptance.
Partial health and uncollected stages reset on regeneration. These limitations
are not approved new backlog tasks.

## Scope and history

The [roadmap](docs/04-roadmap.md) and [feature contracts](docs/game-features.md)
remain authoritative for scope; the [verification index](docs/07-development-setup.md#consolidated-verification-index)
records project checks. Detailed implementation history and retired checklists
remain in [PROGRESS.md](PROGRESS.md) and Git history. No acceptance status or
product scope changes as part of this cleanup.
