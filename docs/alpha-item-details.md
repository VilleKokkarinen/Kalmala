# Alpha item details

Draft values for manual review. Task status lives in [BACKLOG.md](../BACKLOG.md).
These details preserve the existing proposals; implementation still needs review.

**Planning basis:** [current inventory, Valheim sources and gap analysis](alpha-items.md).
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

## A0

Existing item acquisition and use (P0).
- Before implementation, review the concrete A0–A9 proposals and A10 milestone groups with the user, recording included/deferred scope and final verification gates. New-system proposals stay blocked until their scope decisions are accepted; do not delegate their design back to implementation agents.
- Audit normal world-to-use reachability against the material/source proposal below; report deviations and bootstrap cycles rather than selecting replacement sources. Verify generated nodes for the current direct-bar Iron path; its proposed replacement is explicitly listed under A1.
- Add Sinew (`Sinew`) as a crafting material. User-selected deer rule: each credited defeat has a 1/2 (50%) chance to grant exactly 1 Sinew, otherwise 0. Roll once on the server per defeat; retries/duplicate reward requests must not reroll. Boar source remains a proposal of 1 per credited defeat. Add Sinew alongside existing rewards. Stack 20, proposed weight 0.1 kg; no equipment, attacks or upgrades. Register the definition and integrate an original generated icon before completion. Use the existing validated attacker/reward path; no new creature, butcher tool or corpse-processing system.
- Replace the planned sinew-to-String recipe with Bow String (`BowString`): Workbench level 1, consume 4 Sinew to produce exactly 1 Bow String. User-selected on 2026-10-19. Proposed stack 20, weight 0.05 kg; description "A bowstring twisted from animal sinew." Add its material definition and original generated icon; no equipment, attacks or upgrades. In the planned base Bow acquisition and upgrade recipes, replace 2 String with 1 Bow String and retain 5 Wood. Verify Sinew can be acquired with existing melee gear before crafting Bow.
- Keep generic `String` for the unrelated sword/tool/building/accessory proposals. Its source is unresolved after replacing the sinew-cord recipe: record a separate source proposal for review before implementing those dependent tasks. Do not silently substitute Bow String into them or rename existing String data/saves; no runtime catalogue change or migration is made by this plan edit.
- Add harvestable aspen trees in Meadows yielding 4 Lightwood per completed tree/log/bundle lifecycle; require Stone Axe or better. Preserve existing birch Wood rewards. Verify Bronze-family recipes/upgrades are reachable without requiring a Bronze tool to obtain Lightwood.
- Add 1 Bone Fragment to each server-confirmed boar or deer defeat, alongside existing meat/hide rewards, paid once to the validated attacker. Retain no-reward behavior for uncredited defeats; validate reward capacity and prevent duplicate grants. Verify Boar Hide Cape crafting/upgrades.
- Reconcile the six current cooking outputs with server food-use dispatch; remove stale food-ID assumptions and verify paid cooking-to-eating behavior.
- Add Hearth Broth at a Cauldron with the existing lit-hearth requirement: 1 Deer Meat + 1 Onion produces 1 Hearth Broth; MaxBatch 5, Cooking XP 10 per accepted craft. This proposed recipe changes the earlier intentional no-recipe state only after review.
- Add hand-gathered wild carrot patches in Meadows: each harvest grants 2 Carrot and 1 Carrot Seed; no tool required. Seed use remains blocked on A7 farming.
- Add hand-gathered wild potato patches in Meadows: each harvest grants 2 Potato; no tool required. Do not grant Potato Seed until A7 explicitly resolves seed versus tuber propagation.
- Add hand-gathered wild rutabaga patches in Shimmering Lakes on dry banks: each harvest grants 2 Rutabaga and 1 Rutabaga Seed; no tool required. Seed use remains blocked on A7 farming.
- Add hand-gathered wild onion patches in Elderwood: each harvest grants 2 Onion and 1 Onion Seed; no tool required. Seed use remains blocked on A7 farming. Verify all four vegetables reach the existing cooking recipes without farming/debug grants.

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
| Sinew | Existing deer; boar source proposed | Deer: 50% chance of 1, otherwise 0, once per credited defeat. Boar: proposed 1 per credited defeat | Deer rate user-selected; boar awaits review |
| Bow String | Animal sinew, at Workbench | Craft 4 Sinew into 1 Bow String | User-selected recipe; implementation pending |
| String | Source unresolved for non-bow proposals | No sinew-to-String recipe; needs a separate reviewed source | Dependent tasks blocked until source review |
| Stone | Existing mountain slate veins and existing Stone sources | Existing Stone Pick or better and authored yields; retain existing starter acquisition | Retain; audit fresh-start bootstrap |
| Boar Hide / Boar Meat | Existing boars | Retain existing server-confirmed defeat rewards and quantities | Retain |
| Deer Hide / Deer Meat | Existing deer | Retain existing server-confirmed defeat rewards and quantities | Retain |
| Bone Fragment | Existing boars and deer | Additional 1 Bone Fragment per credited defeat | Add under A0; no new creature |
| Amber | Existing Mossy Mire peat-amber seams | Stone Pick or better; retain existing deterministic source yield | Retain |
| Frost Salt | Existing Freezing Tundra deposits and frostmoss | Retain existing source-specific pick/knife actions and authored yields | Retain |
| Copper Ore | New exposed copper-bearing rock nodes in Elderwood | Stone Pick or better; 3 Copper Ore per exhausted node | A1; biome user-confirmed |
| Tin Ore | New tin-bearing gravel nodes on dry Shimmering Lakes shores | Stone Pick or better; 3 Tin Ore per exhausted node | A1; biome user-confirmed |
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

## A1

Ores, bars and fuel (P0; new processing scope gated).
- Review A0's source table and the station proposal below; confirm Stone Pick → Copper/Tin → Bronze Pick → Iron Ore bootstrap and approve processing scope before implementation. Preserve current behavior until approved.
- Add the table's Elderwood Copper Ore and Shimmering Lakes Tin Ore nodes: Stone Pick minimum, 3 matching ore per exhausted node (blocked on A1 contract).
- Implement the table's charcoal-station conversion of 1 Wood to 1 Coal with direct-material hammer placement; no alternative Coal source (blocked on A1 contract).
- Implement the table's smelter conversions and Forge alloying: 1 Copper/Tin Ore + 1 Coal → 1 matching bar; 2 Copper + 1 Tin → 1 Bronze (blocked on ore/Coal acquisition and A1 contract).
- Change Mossy Mire bog-iron nodes from direct Iron bars to 3 Iron Ore per node with Bronze Pick minimum; smelt 1 Iron Ore + 1 Coal → 1 Iron and reconcile documentation (blocked on A1 contract and refining).

**Processing proposal — review before implementation:**

| Buildable / process | Direct-material placement or recipe | Operation |
| --- | --- | --- |
| Charcoal Kiln (`CharcoalKiln`) | Hammer: 20 Stone | 1 Wood → 1 Coal in 30 seconds; 10-input/10-output capacity, one job at a time |
| Smelter (`Smelter`) | Hammer: 20 Stone + 20 Wood | 1 ore + 1 Coal → 1 matching bar in 30 seconds; 10 ore/10 Coal/10 output capacity; Copper, Tin, Iron only |
| Bronze alloying | Forge level 1: 2 Copper + 1 Tin → 1 Bronze | Immediate paid craft, MaxBatch 5; no additional Coal or skill requirement |

Kiln/smelter need grounded placement, a clear 1 m interaction approach and
2 m overhead clearance. Internal heat works in rain and needs no campfire.
Pause when output is full; reserve inputs once per job. Server-save queued
inputs, outputs and remaining active-game seconds; no offline processing.
Reject removal while contents/jobs remain. Output collection uses validated
inventory capacity and cannot duplicate rewards after restart. No station
inventory kit, upgrades, extra ore types or new fuel identity.

## A2

Melee weapons (P1).
- Review the sword/club table below; retain the proposed Wooden Club as an early Wood-only option alongside Stone Mace unless manually removed from scope.
- Add Bronze Sword (`BronzeSword`) through the complete reviewed craft/attack/upgrade/appearance/icon path (blocked on reachable Bronze inputs).
- Add Iron Sword (`IronSword`) through the complete reviewed path (blocked on sword support and reachable Iron inputs).
- Add Wooden Club (`WoodenClub`) through the complete reviewed path; no gathering role or metal prerequisite.

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

## A3

Bows and ammunition (P1).
- Review and add Layered Bow (`LayeredBow`): Workbench 1, 6 Lightwood + 1 Bow String + 2 Deer Hide → 1; stack 1, 1.5 kg, 140 condition, two hands, 1.2-second full draw. Keep existing arrow-dependent damage, velocity and spread curves; no bow damage bonus. Upgrade base 3 Lightwood + 1 Bow String, +30 condition/level, existing cost scaling/cap; repair at Workbench (blocked on A0 Bow String/Lightwood).
- Review and add Bronze Arrow (`BronzeArrow`): Workbench 1, 4 Wood + 1 Bronze → 20; stack 100, 0.05 kg each. Pierce 14–56 across the existing charge curve versus Wooden Arrow's 10–40; retain current velocity/spread, gravity, collision and lifetime. No recovery or elemental damage (blocked on A1 Bronze).
- Extend preferred-stack selection to both arrow IDs: exact selected stack first, otherwise Wooden Arrow before Bronze Arrow, lowest occupied cell first within a type. Consume one arrow and one bow condition only on committed server shot. Show selected arrow damage/type; no silent spending of a different type while the preferred stack remains usable.

## A4

Armour, capes and shields (P1).
- Review the armour/shield proposal below; retain existing hide Boots/Cape and explicitly defer second-tier Boots/Cape and insulation/cold protection for this alpha set.
- Add Bronze Helmet (`BronzeHelmet`), Head slot, using the complete reviewed path (blocked on A1).
- Add Bronze Tunic (`BronzeTunic`), Chest slot, using the complete reviewed path (blocked on A1).
- Add Bronze Leggings (`BronzeLeggings`), Legs slot, using the complete reviewed path (blocked on A1).
- Add Bronze Shield (`BronzeShield`), Shield slot, using the complete reviewed path; passive Armor only (blocked on A1).

| Item | Forge 1 craft cost for 1 | Armor / condition / weight | Upgrade base cost |
| --- | --- | --- | --- |
| Bronze Helmet | 3 Bronze + 1 Boar Hide | 3 / 100 / 2 kg | 1 Bronze + 1 Boar Hide |
| Bronze Tunic | 5 Bronze + 2 Boar Hide | 5 / 120 / 5 kg | 2 Bronze + 1 Boar Hide |
| Bronze Leggings | 4 Bronze + 2 Boar Hide | 4 / 100 / 4 kg | 2 Bronze + 1 Boar Hide |
| Bronze Shield | 3 Bronze + 3 Lightwood + 1 Boar Hide | 9 / 100 / 5 kg | 1 Bronze + 1 Lightwood |

Stack 1. Each level adds 1 Armor and 10 maximum condition with existing
upgrade scaling/cap at Forge; existing free station repair and damage wear.
No movement penalties, elemental resistances, active block/parry or set bonus.

## A5

Carried utility tools (P1/P2).
- Review and add Bark Torch (`BarkTorch`): inventory craft 2 Wood + 1 String → 1, stack 1, 0.5 kg. Main Hand only; 300 seconds of burning while held, local light radius 6 m from replicated lit state. No damage, warmth, Wet removal or world ignition. Uncovered rain extinguishes it; dry re-equip relights it. Stored/unequipped torches do not burn; spent torch remains unusable until inventory refuel with 1 Wood restores 300 seconds. Persist remaining burn time with item state; no repair/upgrade recipe or generic melee fallback.
- Review and add Cultivation Rake (`CultivationRake`) only with A7: Workbench 1, 2 Wood + 2 Stone + 2 String → 1; stack 1, 1 kg, 100 condition. Hold to plant/harvest approved garden plots; one condition per accepted planting, none on harvest/rejection. Free Workbench repair, no upgrades, damage or terrain editing.
- Record terrain-editing Hoe as deferred from this alpha proposal: no height edits, smoothing, roads or grass-painting system. The Rake works only on A7 plots and does not modify the shared terrain sampler.

## A6

Forage, meals and recovery consumables (P1; effects gated).
- Review and add Lingonberry (`Lingonberry`): hand-gather Meadow berry shrubs, 3 berries per depleted shrub; no tool, stack 20, 0.05 kg each. Eating one applies the existing Steady Meal effect (blocked on A0 food dispatch).
- Review and add Forest Chanterelle (`ForestChanterelle`): hand-gather Elderwood mushroom patches, 2 per depleted patch; no tool, stack 20, 0.1 kg each. Eating one applies the same Steady Meal effect (blocked on A0 food dispatch).
- Retain one Steady Meal effect for all seven existing prepared foods, the two forage foods and A7 Cooked Lake Fish: 120 seconds, stamina-use multiplier 0.90, no stacking or eating while active. Raw meats/vegetables/seeds remain ingredients, not consumable meals. Defer differentiated nutrition, additional meal slots and new prepared-dish volume.
- Review the recovery table below and approve its new server-owned consumable effects before implementation; use Cauldron at a lit hearth, no new brewing station, alcohol/base items or fermentation step.
- Add Berry Recovery Draught (`BerryRecoveryDraught`) with the complete reviewed recipe/effect/icon path (blocked on recovery approval and Lingonberry).
- Add Root Stamina Draught (`RootStaminaDraught`) with the complete reviewed recipe/effect/icon path (blocked on recovery approval and vegetable access).
- Explicitly defer cold/poison/fire-resistance preparations: no consumable hazard bypass or new resistance channel in this alpha set.

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

## A7

Crops, seeds and fishing (P2; scope gated).
- Review and approve Garden Plot (`GardenPlot`) plus the farming table below: Hammer placement 4 Wood + 1 Stone, grounded 2 m × 2 m plot on dry Meadows/Elderwood/Shimmering Lakes terrain, maximum 15-degree slope. One crop per plot, open sky, no watering/fertilizer/season or terrain deformation. Require held usable A5 Rake for planting/harvest; reject conflicting/occupied plots before payment.
- Implement Carrot planting, maturation, harvest and seed renewal as the first full loop, with A5 Rake and direct-material plot placement (blocked on farming approval).
- Implement Potato tuber propagation; retain `PotatoSeed` as an unused development definition with no drops/recipes, not an alternative planting input (blocked on first crop loop).
- Implement Rutabaga food/seed propagation from the table (blocked on first crop loop).
- Implement Onion food/seed propagation from the table (blocked on first crop loop).

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

- Review fishing scope: use visible stationary Lake Fish schools on Shimmering Lakes water, not a new combat creature. Each stable school permits 3 catches then depletes under the existing resource-node policy; no timed respawn. Approve the rod/bait/catch proposal before implementation.
- Add Fishing Rod (`FishingRod`): Workbench 1, 4 Lightwood + 3 String → 1; stack 1, 1 kg, 80 condition, two hands, free Workbench repair, no upgrades/combat. Add Bait (`FishingBait`): inventory craft 1 Forest Chanterelle → 5, stack 50, 0.02 kg each (blocked on fishing approval/A0/A6).
- Add school interaction: usable held Rod, selected school within 15 m and clear line of sight; reserve 1 Bait and a free result cell. After 5 seconds, if player stayed within 1 m of start and target remains valid, grant 1 Lake Fish and spend bait/1 rod condition/1 school catch atomically. Cancellation/rejection releases reservation unchanged; one angler per school, no arbitrary client fish IDs or timing rewards.
- Add Lake Fish (`LakeFish`), stack 20, 0.5 kg, raw ingredient only; Cooking Rack at lit hearth converts 1 to 1 Cooked Lake Fish (`CookedLakeFish`), stack 20, 0.4 kg, MaxBatch 5, Cooking XP 10, existing Steady Meal effect. Complete fish/bait/rod icons and cooking-to-use path (blocked on fishing and A0 food dispatch).

## A8

Buildables, processing and sanctuary supplies (P2; no kits).
- Review the shelter table below and approve the new snapping/support/interaction scope. Every row is a Buildable with a direct-material Hammer recipe and original build-menu icon; no inventory definition/kit or item upgrade tree.
- Add Timber Door (`TimberDoor`) with its reviewed paid placement, collision and saved open/closed state (blocked on shelter approval).
- Add Timber Beam (`TimberBeam`) with reviewed support rules (blocked on shelter approval).
- Add Timber Stairs (`TimberStairs`) with reviewed walkable collision (blocked on shelter approval).
- Add Camp Bed (`CampBed`), furniture-only resting behavior below; do not add sleep/respawn binding (blocked on shelter approval).
- Add Standing Light (`StandingLight`) with reviewed fuel/rain behavior (blocked on shelter approval).
- Add Reinforced Chest (`ReinforcedChest`) as a separate placed chest, not an in-place inventory-capacity upgrade (blocked on shelter approval/A1).
- Record recovery-station expansion as deferred: A6 uses the existing Cauldron; no fermenter, brewing tree or extra cooking station. A1 Kiln/Smelter and A7 Garden Plot are owned by their sections, not duplicate tasks here.

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

## A9

Travel, accessories and discovery rewards (P2; scope gated).
- Review travel scope: propose only Skiff Cargo Chest (`SkiffCargoChest`), defer cart/carry-pack progression, encumbrance and hull durability. Hammer placement costs 6 Lightwood + 2 String + 1 Bronze, on one fixed cargo socket per existing skiff; no skiff inventory item or new boat tier.
- Implement the approved cargo chest: 12 shared stack cells, interact within 2 m while skiff speed is at most 100 cm/s, same GUID/quantity/capacity rules as land storage. Save cargo against stable vessel ID; reject detachment/removal with contents, retain contents across validated vessel reload, no spill/duplication on reconnect. In-flight transfer rejects unchanged (blocked on explicit approval and existing vessel-persistence support; report any persistence gap).
- Add three gems with separate source tables below. These are concrete drop-rate/location proposals for review; no shared universal gem-drop pool. All stack 20, proposed weight 0.1 kg; no combat/equip/upgrade behavior. Gem and source implementations remain pending.
- Add Smoky Quartz (`SmokyQuartz`) as a mining-only extra reward; preserve ordinary ore yields.
- Add Moonstone (`Moonstone`) to credited Mireling defeats only; preserve existing rewards. Boar/deer and other enemies do not inherit this gem drop.
- Add Garnet (`Garnet`) to generated world treasure chests only, not player storage. Requires a server-owned, stable treasure-container loot path; if missing, implement it as an explicitly approved prerequisite rather than treating player chests as treasure sources.

| Gem | Exclusive proposed source | Chance / quantity | Accessory use |
| --- | --- | --- | --- |
| Smoky Quartz | Exhausted Copper Ore nodes in Elderwood or Tin Ore nodes in Shimmering Lakes | 1/20 (5%) for exactly 1, otherwise 0 | Quartz Necklace |
| Moonstone | Credited Mireling defeats | 1/50 (2%) for exactly 1, otherwise 0 | Moonstone Ring |
| Garnet | Generated treasure chests in Elderwood or Shimmering Lakes | 1/10 (10%) per chest for exactly 1, otherwise 0 | Garnet Necklace |

Resolve rolls once on the server per node exhaustion, enemy defeat or treasure
chest generation. Opening, pickup retries, reconnect or reload cannot reroll.
Persist rolled world rewards/claims with stable source identities; full
inventory leaves an unclaimed reward available. Gem tables are mutually
exclusive by source: no mining Moonstone/Garnet, no enemy Quartz/Garnet and no
treasure Quartz/Moonstone. Existing Amber is not replaced or added to this pool.

- Review Necklace/Ring slot support. Equip one item in each named slot using validated GUID/replace/return and full-pack rules; copies cannot stack bonuses. User-selected effects are +10 Armor for Necklace and +20 maximum stamina for Ring, replacing the earlier spell-cost discounts.
- Add Quartz Necklace (`QuartzNecklace`) with the reviewed craft/equip/stat/icon path (blocked on gem acquisition and accessory support).
- Add Moonstone Ring (`MoonstoneRing`) with the reviewed craft/equip/stat/icon path, replacing the Quartz Ring proposal (blocked on gem acquisition and accessory support).
- Add Garnet Necklace (`GarnetNecklace`) as an alternative route to the same +10 Armor; no stronger or additional bonus (blocked on treasure loot and accessory support).
- Audit the four existing Mending/Hearth Shield/Bear's Vigor/Deer Call scrolls against their current authored discovery-to-unlock/use paths. Preserve existing locations/entitlements; propose no new scroll, trophy, creature or boss key for this alpha set. Record missing current routes as defects against the existing contract, not permission to invent a reward source.

| Accessory | Proposed Forge 1 cost for 1 | Server-owned effect while equipped |
| --- | --- | --- |
| Quartz Necklace | 3 Bronze + 1 Smoky Quartz | +10 integer Armor, added once to the existing physical-defense total |
| Moonstone Ring | 2 Bronze + 1 Moonstone | +20 maximum stamina; no direct stamina refill |
| Garnet Necklace | 3 Bronze + 1 Garnet | +10 integer Armor; alternative to Quartz Necklace, same slot |

All accessories stack 1; proposed weight 0.1 kg each. No durability, upgrades, repair or
attacks. The necklace's Armor remains active while equipped without wear;
use the existing physical-only Armor calculation, not elemental resistance.
Ring equip raises maximum stamina without increasing current stamina;
unequip/replacement clamps current stamina to the new maximum. Repeated
equip/unequip must never refill stamina or accumulate bonuses. Recompute
derived stats on equipment changes; normal regeneration uses the new maximum.
Gear remains owner-private and session-only under the existing equipped-gear
persistence limit; no general gear-save system. Add explicit accessory/derived-
stat handling rather than assuming the current durable-armour schema supports
it. All three gems and three accessories need original generated icons and all applicable
catalogue/recipe/stat records. Existing scroll/spell costs remain unchanged.

## A10

Approved alpha item-set acceptance.
- Before coding, record user-reviewed include/defer status for every named item/buildable above and freeze their reviewed recipes/sources/stats. Proposed milestone groups: I0 A0 current-loop repairs; I1 A1 production; I2 A2–A6 equipment/forage/consumables except Rake; I3 A7 farming/fishing plus A5 Rake; I4 A8–A9 sanctuary/cargo/accessories. Each group requires its own final verification; scope-gated groups remain blocked until approved. This is a proposed schedule, not new completed roadmap history.
- Reconcile delivery against the frozen checklist: each included ID must have source, payment, use/equip, upgrade or explicit non-applicability, original icon/provenance/mapping, relevant save behavior and linked verification evidence. Record every omitted candidate as deferred, not implicitly complete.
- At each resulting milestone's final increment, run required Editor/project builds, applicable automation, catalogue/icon validation and relevant peer/persistence runners from `docs/07-development-setup.md`; repair milestone defects and rerun failing checks plus required full verification. Record actual commands/results; do not equate source inspection with execution.
- Final progression scenario: fresh world, existing starter loadout only, gather normal resources → Workbench/String/Lightwood → Bow/arrows → Copper/Tin/Coal/Bronze → Bronze Pick → Iron/Densewood. Craft/equip/upgrade one of every included durable item; cook/eat each included meal and use each draught, retaining the specified cooldowns. No debug grants. Record biome/node availability rather than fabricating missing resources.
- Final approved-system scenarios: plant/renew every included crop, restart planted plots/processing jobs, catch/cook fish, exercise door/stairs/light/chest, reload cargo with vessel and verify accessory Armor/maximum-stamina bonuses without stacking/refill exploits. Verify gem exhaustion/reload/retry cannot reroll or duplicate drops. For each applicable flow use host/client plus two competing requests: no duplicate or partial reward, invalid/range/full-pack requests unchanged, no private inventory leakage. Skip only explicitly deferred systems; verify session-only effects clear as specified.
- Mark the approved item milestone complete only after required gates pass; document blockers and pre-existing failures with evidence. No whole-alpha release declaration, packaging/publication or broader scope change follows automatically from this checklist.
