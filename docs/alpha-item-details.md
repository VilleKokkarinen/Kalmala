# Alpha item details

Task status lives in [BACKLOG.md](../BACKLOG.md). The user approved all queued
P0/P1/P2 scope across A0–A9 on 2026-10-19, including Ocean Reef Maw, Reef Tooth
and Reef Harpoon. Explicit exclusions in this proposal remain deferred. The
reviewed A10 plan uses I0–I5, with the combined A8a/A8b enemy gate after A8b
and before A9. Reviewed conflicts are resolved below; category-specific tasks
still finalize their detailed contracts before implementation.

**Planning basis:** [current inventory, Valheim sources and gap analysis](alpha-items.md).
This queues item work; it does not declare a release or rewrite historical
roadmap acceptance. Follow
the categories in order after the river queue. P0 closes existing loops; P1 is
recommended alpha breadth; P2 adds new systems, whose scope is approved. Each
child is a separate increment; split larger children into integrated increments
before starting.
Blocked children require their named dependencies. Agents must follow the
reviewed values; missing design values must be proposed here before
implementation, not invented while coding.

**Completion for every item:** normal acquisition and usable behavior; applicable
manifest-registered item/profile/recipe/upgrade/reward/attack data; original
generated/imported icon, provenance, runtime mapping and icon validation;
server-owned payment/use/targets/quantities; documented persistence and explicit
non-applicable records. Buildables require supported hammer placement and raw
materials, never inventory kits. No retired Reed Fibre or offensive magic.

## A0

Existing item acquisition and use (P0).
- The 2026-10-19 user review includes all queued P0/P1/P2 scope across A0–A9. Review the material/source table below and record the A10 gates; keep category-specific implementation reviews in order.
- Audit normal world-to-use reachability against the material/source proposal below; report deviations and bootstrap cycles rather than selecting replacement sources. Verify generated nodes for the current direct-bar Iron path; its proposed replacement is explicitly listed under A1.
- Add Sinew (`Sinew`) as a crafting material. User-selected deer rule: each credited defeat has a 1/2 (50%) chance to grant exactly 1 Sinew, otherwise 0. Roll once on the server per defeat; retries/duplicate reward requests must not reroll. Boar source remains a proposal of 1 per credited defeat. Sinew is implemented alongside existing deer rewards, with stack 20 and weight 0.1 kg; it has no equipment, attacks or upgrades. Its registered item and original generated icon are in place. The existing validated attacker/reward path is used; no new creature, butcher tool or corpse-processing system.
- Bow String (`BowString`) is the only string item and is reserved for bows. Workbench level 1 converts 4 Sinew into 1 Bow String; stack 20, weight 0.05 kg, description "A bowstring twisted from animal sinew." Its definition, recipe and original generated icon are in place. Bow acquisition and upgrade each use 1 Bow String. It has no equipment, attacks or upgrades. Review pending non-bow binding costs in their category contracts; do not use Bow String for them.
- Add harvestable aspen trees in Meadows yielding 4 Lightwood per completed tree/log/bundle lifecycle; require Stone Axe or better. Preserve existing birch Wood rewards. Verify Bronze-family recipes/upgrades are reachable without requiring a Bronze tool to obtain Lightwood.
- Bone Fragment (`BoneFragment`) is awarded once per server-confirmed boar or deer defeat alongside existing meat/hide rewards, to the validated attacker only. The existing inventory grant enforces authority and capacity; the one-shot defeat reward gate prevents duplicates, and uncredited defeats receive no rewards. The Boar Hide Cape acquisition and level-one upgrade each use one Bone Fragment. Its registered material definition and original generated icon already exist.
- Reconcile the six previously authored cooking outputs with server food-use dispatch; remove stale food-ID assumptions and verify paid cooking-to-eating behavior.
- Hearth Broth (`HearthBroth`) is produced at a Cauldron with the existing lit-hearth requirement: 1 Deer Meat + 1 Onion produces 1 serving; MaxBatch 5, Cooking XP 10 per accepted craft. Its registered item definition and original generated icon were already in place; it has no equipment, attacks or upgrades.
- Implemented under A0: hand-gathered wild carrot patches in Meadows grant 2 Carrot and 1 Carrot Seed; no tool required. Seed use remains blocked on A7 farming.
- Implemented under A0: hand-gathered wild potato patches in Meadows grant 2 Potato; no tool required. Do not grant Potato Seed until A7 explicitly resolves seed versus tuber propagation.
- Implemented under A0: hand-gathered wild rutabaga patches on dry Shimmering Lakes banks grant 2 Rutabaga and 1 Rutabaga Seed; no tool required. Seed use remains blocked on A7 farming.
- Implemented under A0: hand-gathered wild onion patches in Elderwood grant 2 Onion and 1 Onion Seed; no tool required. Seed use remains blocked on A7 farming. The four vegetables now have natural routes into existing cooking recipes without farming/debug grants.

**Material/source proposal — scope approved 2026-10-19; implementation pending.**
Agents must implement the reviewed entries, not invent alternative biomes,
drops, recipes or quantities. User edits take precedence. If an entry is not
implementable, report the conflict before changing its design. Rows marked
**retain** describe existing routes to preserve and verify; **add/change** rows
are included in the approved queue.

| Material / ingredient | Proposed source and biome | Gathering / production and yield | State / dependency |
| --- | --- | --- | --- |
| Wood | Existing Meadow birch and existing wood-resource trees | Existing axe/tree/log/bundle flow, Stone Axe or better where already required; retain each source's authored yield | Retain; no reed-source reward changes |
| Densewood | Existing Elderwood ironheart trees | Existing Iron Axe tree/log/bundle flow and authored yield | Retain; Iron Pick/ore route below must not require Densewood |
| Lightwood | New aspen trees in Meadows | Stone Axe or better; 4 Lightwood on final bundle pickup | Add under A0; no Bronze prerequisite |
| Sinew | Existing deer; boar source proposed | Deer: 50% chance of 1, otherwise 0, once per credited defeat. Boar: proposed 1 per credited defeat | Deer rate user-selected; boar awaits review |
| Sinew | Credited deer defeats | One server roll; 50% grants exactly 1 | Implemented under A0; boar source remains deferred |
| Bow String | Animal sinew, at Workbench | Craft 4 Sinew into 1 Bow String | Bow-only material; implemented under A0 |
| Stone | Existing mountain slate veins plus new dry slate outcrops across all six land biomes, including Meadows | Stone Pick or better; one Stone per new outcrop, matching the existing slate source | Added under the user-directed A1 source-availability request; existing Stone sources remain |
| Boar Hide / Boar Meat | Existing boars | Retain existing server-confirmed defeat rewards and quantities | Retain |
| Deer Hide / Deer Meat | Existing deer | Retain existing server-confirmed defeat rewards and quantities | Retain |
| Bone Fragment | Existing boars and deer | Additional 1 Bone Fragment per credited defeat | Implemented under A0; no new creature |
| Amber | Existing Mossy Mire peat-amber seams | Stone Pick or better; retain existing deterministic source yield | Retain |
| Frost Salt | Existing Freezing Tundra deposits and frostmoss | Retain existing source-specific pick/knife actions and authored yields | Retain |
| Copper Ore | New exposed copper-bearing rock nodes in Elderwood | Stone Pick or better; 3 Copper Ore per exhausted node | A1; biome user-confirmed |
| Tin Ore | New tin-bearing gravel nodes on dry Shimmering Lakes shores | Stone Pick or better; 3 Tin Ore per exhausted node | A1; biome user-confirmed |
| Iron Ore | Existing Mossy Mire bog-iron nodes | Bronze Pick or better; 3 Iron Ore per exhausted node, replacing direct Iron-bar reward | Change under A1; preserve existing behavior until approved |
| Coal | Wood burned in a placed charcoal station | 1 Wood produces 1 Coal; no mined Coal source, no new fuel item | Add under A1; scope approved |
| Copper | Copper Ore refined at placed smelter | 1 Copper Ore + 1 Coal produces 1 Copper | Add under A1; scope approved |
| Tin | Tin Ore refined at placed smelter | 1 Tin Ore + 1 Coal produces 1 Tin | Add under A1; scope approved |
| Bronze | Copper/Tin alloying at Forge | 2 Copper + 1 Tin produces 1 Bronze | Add under A1; no direct Bronze drops |
| Iron | Iron Ore refined at placed smelter | 1 Iron Ore + 1 Coal produces 1 Iron | Add under A1; replaces direct-bar acquisition only after review |
| Carrot / Carrot Seed | New wild carrot patches in Meadows | Hand gather 2 Carrot + 1 Carrot Seed per patch | Implemented under A0; seed use waits for A7 |
| Potato | New wild potato patches in Meadows | Hand gather 2 Potato per patch | Implemented under A0; no Potato Seed until A7 resolves tuber propagation |
| Potato Seed | No A0 acquisition | Existing definition retained without drops; A7 proposes Potato tuber propagation instead | Explicitly deferred, not agent-selected |
| Rutabaga / Rutabaga Seed | New wild rutabaga patches on dry Shimmering Lakes banks | Hand gather 2 Rutabaga + 1 Rutabaga Seed per patch | Implemented under A0; seed use waits for A7 |
| Onion / Onion Seed | New wild onion patches in Elderwood | Hand gather 2 Onion + 1 Onion Seed per patch | Implemented under A0; seed use waits for A7 |
| Hearth Broth | Cauldron near usable lit hearth | 1 Deer Meat + 1 Onion produces 1 Hearth Broth; MaxBatch 5, Cooking XP 10 | Implemented under A0 |

New harvest nodes use stable generated identities and the existing matching
node depletion/restart policy; this proposal adds no timed respawn system.
A full inventory must leave a harvest unclaimed, with no partial plant reward
or depletion. New-tree quantities apply at bundle pickup, not per axe hit.
Station placement costs, processing timing/capacity and farming rules are
specified in the approved A1/A7 scope; their category review tasks still apply.

## A1

Ores, bars and fuel (P0; processing scope approved 2026-10-19).
- Review A0's source table and the station proposal below; confirm Stone Pick → Copper/Tin → Bronze Pick → Iron Ore bootstrap before implementation. Preserve current behavior until the A1 processing review.
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

**User-directed pick acquisition costs (2026-10-20):**

| Pick | Station | Acquisition cost |
| --- | --- | --- |
| Stone Pick | Workbench 1 | 10 Wood + 5 Stone |
| Bronze Pick | Forge 1 | 10 Wood + 10 Bronze |
| Iron Pick | Forge 1 | 10 Wood + 10 Iron |

The existing fresh-start Stone Pick grant remains. The runtime acquisition
recipes still use their previous values; apply these targets as the A1 metal
sources and production become usable, so a Bronze Pick never becomes
unobtainable between increments.

**User-directed mining ceilings (2026-10-20):** Stone Pick can mine resources
through Bronze grade; Bronze Pick can mine through Iron; Iron Pick currently
also mines through Iron. Copper and Tin Ore are Bronze-grade sources and remain
available to Stone Pick or better. Iron Ore is Iron grade and requires Bronze
Pick or better. The server checks both the source's minimum tool tier and the
pick's maximum mineable resource tier.

**Progression review (2026-10-20).** Fresh characters receive a Stone Pick and
Construction Hammer but no material stacks; Stone still requires travel to
Thunder Mountains slate or the optional Three-Run Rillstone in Shimmering
Lakes, as recorded in the A0 source audit. Stone Pick opens the proposed Copper
and Tin nodes. Existing Forge placement and the proposed Kiln/Smelter
placements use only Wood and Stone; Wood becomes Coal at the Kiln, then
matching ore and Coal become bars at the Smelter. Forge level 1 alloys 2
Copper + 1 Tin into Bronze. The user-directed Bronze Pick cost makes Bronze a
real prerequisite before the planned Iron Ore nodes. Smelting that Iron Ore
then supplies the Iron Pick recipe, so the tier chain has no Iron-material
cycle. Preserve the current direct Iron reward until the A1 source and
refining path are implemented together.

## A2

Melee weapons (P1).
- Review the sword/club table below; retain the proposed Wooden Club as an early Wood-only option alongside Stone Mace unless manually removed from scope.
- Add Bronze Sword (`BronzeSword`) through the complete reviewed craft/attack/upgrade/appearance/icon path; its Bronze inputs are reachable through A1.
- Add Iron Sword (`IronSword`) through the complete reviewed path after Bronze Sword; its Iron inputs are reachable through A1.
- Add Wooden Club (`WoodenClub`) through the complete reviewed path; no gathering role or metal prerequisite.

Review completed 2026-10-21: retain all three proposals. A1 provides reachable
Bronze and Iron inputs, so the backlog's Bronze-then-Iron implementation order
is viable. Use the concrete A2
checklist recipes for both swords: 10 Bronze + 2 Lightwood + 2 Deer Hide for
Bronze Sword, and 20 Iron + 2 Densewood + 2 Deer Hide for Iron Sword. Deer
Hide is the existing non-bow binding input; do not add a generic string item
or use bow-only Bow String. Wooden Club remains an optional Workbench craft
with the reviewed Wood-only recipe and Basic melee behavior. It has no
gathering use or new attack rules.

| Item | Craft station / cost for 1 | Level-one profile | Upgrade base cost / per-level reward |
| --- | --- | --- | --- |
| Bronze Sword | Forge 1; 10 Bronze + 2 Lightwood + 2 Deer Hide | One hand, 100 condition, 35 Slash, 2 kg | 2 Bronze + 1 Lightwood; +30 condition, +4 Slash |
| Iron Sword | Forge 1; 20 Iron + 2 Densewood + 2 Deer Hide | One hand, 180 condition, 55 Slash, 2.5 kg | 2 Iron + 1 Densewood; +30 condition, +4 Slash |
| Wooden Club | Workbench 1; 4 Wood | One hand, 40 condition, 10 Crush, 1 kg | 2 Wood; +20 condition, +2 Crush |

All use the existing Basic melee timing/reach, zero attack-style bonus on
top of the listed profile damage, and one condition per accepted hit. No
alternate attacks, harvesting, elemental channels or new block/parry rules.
Stack 1; use existing level cap and upgrade-cost scaling. Upgrade at the
crafting station; free existing station repair. Original sword/club silhouettes
and icons follow the established art treatment.

## A3

Bows and ammunition (P1).
- Review and add Light Bow (`LightBow`): Workbench 1, 25 Lightwood + 1 Bow String + 5 Deer Hide → 1; stack 1, 1.5 kg, 140 condition, two hands, 2-second full draw. Keep existing arrow-dependent damage, velocity and spread curves; no bow damage bonus. Upgrade base 3 Lightwood + 1 Bow String, +30 condition/level, existing cost scaling/cap; repair at Workbench (blocked on A0 Bow String/Lightwood). The user selected Light Bow and its backlog recipe/draw values on 2026-10-19; remaining profile values carry forward from the detailed bow proposal.
- Add Stone Arrow (`StoneArrow`): Workbench 1, 20 Wood + 5 Stone → 20; stack 100, 0.05 kg each. Each committed shot deals its authored fixed 25 Pierce; bow charge continues to control velocity and spread. No recovery or elemental damage.
- Review and add Bronze Arrow (`BronzeArrow`): Workbench 1, 20 Wood + 1 Bronze bar → 20; stack 100, 0.05 kg each; 50 Pierce. Retain current bow velocity/spread, gravity, collision and lifetime. No recovery or elemental damage (blocked on A1 Bronze).
- Extend preferred-stack selection to the authored arrow IDs: exact selected stack first, otherwise the first supported arrow in top-row-first inventory order. Consume one arrow and one bow condition only on a committed server shot. Show selected arrow damage/type; no silent spending of a different type while the preferred stack remains usable.

## A4

Armour, capes and shields (P1).
- Review completed 2026-10-21: retain the existing Boar Hide Boots (`BoarHideBoots`) and Boar Hide Cape (`BoarHideCape`); defer second-tier Boots/Cape and insulation/cold protection for this alpha set.
- Add Bronze Helmet (`BronzeHelmet`), Head slot, using the complete reviewed path.
- Add Bronze Tunic (`BronzeTunic`), Chest slot, using the complete reviewed path.
- Add Bronze Leggings (`BronzeLeggings`), Legs slot, using the complete reviewed path.
- Add Bronze Shield (`BronzeShield`), Shield slot, using the complete reviewed path; passive Armor only.

| Item | Forge 1 craft cost for 1 | Armor / condition / weight | Upgrade base cost |
| --- | --- | --- | --- |
| Bronze Helmet | 10 Bronze + 5 Boar Hide | 3 / 100 / 2 kg | 1 Bronze + 1 Boar Hide |
| Bronze Tunic | 15 Bronze + 5 Boar Hide | 5 / 120 / 5 kg | 2 Bronze + 1 Boar Hide |
| Bronze Leggings | 15 Bronze + 5 Boar Hide | 5 / 100 / 4 kg | 2 Bronze + 1 Boar Hide |
| Bronze Shield | 10 Bronze + 5 Lightwood + 5 Boar Hide | 10 / 100 / 5 kg | 1 Bronze + 1 Lightwood |

Stack 1. Each level adds 1 Armor and 10 maximum condition with existing
upgrade scaling/cap at Forge; existing free station repair and damage wear.
No movement penalties, elemental resistances, active block/parry or set bonus.

## A5

Carried utility tools (P1/P2).
- Basic Torch (`BasicTorch`): inventory craft 2 Wood + 1 Amber → 1, stack 1, 0.5 kg. Main Hand only; 300 seconds of burning while held, local light radius 6 m from replicated lit state. No damage, warmth, Wet removal or world ignition. Uncovered rain extinguishes it; dry re-equip relights it. Stored/unequipped torches do not burn; spent torch remains unusable until inventory refuel with 1 Wood restores 300 seconds. Persist remaining burn time with item state; no repair/upgrade recipe or generic melee fallback.

The server exposes a flame-eligibility query for the future user-requested
[mosquito swarm counter](biome-enemies.md#mosquito-swarm). A8a still owns swarm
targeting, damage prevention and repulsion behavior; unlit, stored and empty
torches must not count. This does not change torch recipes or imply warmth,
world ignition or direct torch damage.

- Add Planting Trowel (`PlantingTrowel`) with A7: Forge level 1, 10 Wood + 5 Bronze → 1; stack 1, 1 kg, 100 condition. Hold to plant/harvest approved garden plots; one condition per accepted planting, none on harvest/rejection. Free Workbench repair, no upgrades, damage or terrain editing.
- Record terrain-editing Hoe as deferred from this alpha proposal: no height edits, smoothing, roads or grass-painting system. The Planting Trowel works only on A7 plots and does not modify the shared terrain sampler.

## A6

Forage, meals and recovery consumables (P1; scope approved 2026-10-19).
- Review and add Lingonberry (`Lingonberry`): hand-gather Meadow berry shrubs, 3 berries per depleted shrub; no tool, stack 20, 0.05 kg each. Eating one applies the existing Steady Meal effect.
- Review and add Forest Chanterelle (`ForestChanterelle`): hand-gather Elderwood mushroom patches, 2 per depleted patch; no tool, stack 20, 0.1 kg each. Eating one applies the existing Steady Meal effect. The Elderwood patch is its acquisition route; it has no crafting recipe, equipment profile, upgrade cost, or attack data.
- Retain one Steady Meal effect for all seven existing prepared foods and the two forage foods: 120 seconds, stamina-use multiplier 0.90, no stacking or eating while active. Raw meats/vegetables/seeds remain ingredients, not consumable meals. Defer differentiated nutrition, additional meal slots and new prepared-dish volume.
- Review the recovery table below's server-owned effect rules before implementation; use Cauldron at a lit hearth, no new brewing station, alcohol/base items or fermentation step.
- Add Berry Recovery Draught (`BerryRecoveryDraught`) with the complete reviewed recipe/effect/icon path.
- Add Root Stamina Draught (`RootStaminaDraught`) with the complete reviewed recipe/effect/icon path.
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

Review outcome — 2026-10-21: retain the two effects and their authored values,
the shared cross-draught 60-second cooldown, rejection without consumption,
and session-only status. Berry healing stops on death; Root Stamina remains
clamped to the current maximum. Keep support-magic cooldowns separate and use
the existing Cauldron/lit-hearth route. Do not add hazard resistance, alcohol,
fermentation, or another station. A0 supplies Carrot, Potato, and Onion, and
A6 supplies Lingonberry, so neither recipe remains source-blocked; A7 seed
propagation is not a recipe prerequisite.

## A7

Crops and seeds (P2; scope approved 2026-10-19; fishing deferred at the user's request on 2026-10-21).
- Garden Plot (`GardenPlot`) is a direct-material Hammer buildable placed through the Build menu for 4 Wood + 1 Stone on dry Meadows/Elderwood/Shimmering Lakes terrain, grounded within a maximum 15-degree slope. It uses a dedicated build-menu icon keyed by the placed-object identity `GardenPlotKit`; it is never a portable inventory item. One crop per plot, open sky, no watering/fertilizer/season or terrain deformation.
- Interacting with a plot while holding a usable A5 Planting Trowel opens its Cultivation menu. Show the seven reviewed crop/mode choices below with their exact payment and current growth state/remaining active seconds; offer one harvest when mature. The server validates the held Planting Trowel, plot, selection, payment, and harvest before changing state. Reject invalid or occupied plots before payment.
- Implement Carrot planting, maturation, harvest and seed renewal as the first full loop, with A5 Planting Trowel and direct-material plot placement (after the A7 farming review).
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


## A8

Buildables, processing and sanctuary supplies (P2; scope approved 2026-10-19; no kits).
- Review the shelter table below for its placement/support/interaction contracts. Every row is a Buildable with a direct-material Hammer recipe and original build-menu icon; no inventory definition/kit or item upgrade tree.
- Add Timber Door (`TimberDoor`) with its reviewed paid placement, collision and saved open/closed state.
- Add Timber Beam (`TimberBeam`) with reviewed support rules.
- Add Timber Stairs (`TimberStairs`) with reviewed walkable collision.
- Add Camp Bed (`CampBed`), furniture-only resting behavior below; do not add sleep/respawn binding.
- Add Standing Light (`StandingLight`) with reviewed fuel/rain behavior.
- Add Reinforced Chest (`ReinforcedChest`) as a separate placed chest, not an in-place inventory-capacity upgrade (blocked on A1).
- Record recovery-station expansion as deferred: A6 uses the existing Cauldron; no fermenter, brewing tree or extra cooking station. A1 Kiln/Smelter and A7 Garden Plot are owned by their sections, not duplicate tasks here.

| Buildable | Hammer material cost | Dimensions / behavior |
| --- | --- | --- |
| Timber Door | 4 Wood + 1 non-bow binding input (ID/cost to review) | 1 m wide × 2 m high; snaps to existing wall edge grid, takes place of a wall segment rather than overlapping it; opens/closes by server interaction within 2 m, reject closing through a pawn |
| Timber Beam | 2 Wood | 2 m beam, 0.15 m square; horizontal or vertical snap to existing structural endpoints; requires ground or supported endpoint, uses existing structural support model without new material tiers |
| Timber Stairs | 6 Wood | 2 m run × 1 m width × 1 m rise; grounded lower edge or supported floor attachment; solid walkable steps, no ladder/climb mechanic |
| Camp Bed | 6 Wood + 2 Boar Hide + 2 non-bow binding input (ID/cost to review) | 2 m × 1 m; supported dry floor, 1 m clear approach; interaction within 2 m starts a local resting pose until movement/damage; no extra stat effect, time skip or spawn change |
| Standing Light | 3 Wood + 1 non-bow binding input (ID/cost to review) | Grounded/floor-supported, 1.5 m tall; starts unlit, consume 1 Wood for 600 active lit seconds, fuel cap 1200 seconds; 6 m cosmetic light, uncovered rain pauses flame/fuel, no warmth/Wet removal/world ignition |
| Reinforced Chest | 8 Wood + 2 Bronze | Grounded/floor-supported 1.5 m × 1 m; 24 shared stack cells, existing per-item stack limits and validated GUID transfers, no weight/owner-lock mechanic |

Use the existing construction health/rain-wear/repair rules of the matching
Wall (door/beam/stairs) or Storage (bed/light/chest) category; no new defense
stats. Reject overlap/unsupported placement. Save placed identities/transforms,
door state, light fuel and chest contents through the construction/storage
owners; cosmetic resting is session-only. Reject occupied-chest removal;
empty-piece removal follows existing material-return policy. No new offline
fuel drain. If existing wall geometry cannot host the proposed door, report
that contract conflict before substituting a new wall/frame design.

## A8b

Enemy materials and crafted outputs are individually queued in
[BACKLOG.md](../BACKLOG.md), using the current
[drop and recipe tables](biome-enemies.md#recommended-items-and-recipes).
Complete each approved item with its item JSON and applicable acquisition,
station recipe, equipment, attack and upgrade records; register every new file
in `Content/Data/GameCatalogues.json`. Record N/A data categories explicitly.
Generate an original inventory icon, retain its source/provenance, prepare and
import it, register its runtime mapping and extend icon-validation coverage
using [the established workflow](material-icons.md). No placeholder/fallback
icon counts as completion. Run relevant JSON/reference and icon checks before
checking the item task; final acceptance audits every approved ID across A0–A9.

## A9

Cargo, ten gems, special gear and discovery rewards (P2; scope approved 2026-10-19).
Enemy expansion A8a precedes this category. See the
[biome roster](biome-enemies.md); Moonstone requires its accepted Moon Bear.

- Review travel scope: propose only Skiff Cargo Chest (`SkiffCargoChest`), defer cart/carry-pack progression, encumbrance and hull durability. Hammer placement costs 6 Lightwood + 2 non-bow binding input (ID/cost to review) + 1 Bronze, on one fixed cargo socket per existing skiff; no skiff inventory item or new boat tier.
- Implement the approved cargo chest: 12 shared stack cells, interact within 2 m while skiff speed is at most 100 cm/s, same GUID/quantity/capacity rules as land storage. Save cargo against stable vessel ID; reject detachment/removal with contents, retain contents across validated vessel reload, no spill/duplication on reconnect. In-flight transfer rejects unchanged (blocked on existing vessel-persistence support; report any persistence gap).
- Add exactly ten gems from the table below, each as a separate integrated increment. These are concrete source/rate/recipe/stat proposals for review, not a new alpha release commitment. Seven are ordinary rare gems; three special gems are required by named special gear. No empty-purpose collectibles or universal gem pool.
- All raw gems stack 20, proposed weight 0.1 kg, with no combat/equip/upgrade behavior. Each needs a registered material definition and its own original generated icon. Normal resource nodes keep their existing yields; dedicated gemstone rocks replace a small share of spawn slots rather than adding gems to ore rewards. Wildlife/treasure rules remain separate.
- Treasure means server-owned generated world loot containers, never player storage. Stable treasure loot and sparse resource/enemy reward claims are explicit prerequisites; do not invent new creatures or silently treat an unsupported source as implemented.

| Gem / item ID | Exclusive eligible source | Proposed rarity / successful yield | Required use |
| --- | --- | --- | --- |
| Smoky Quartz (`SmokyQuartz`) | Dedicated Quartz rock replacing Copper slots in Elderwood or Tin slots in Shimmering Lakes | 5% of eligible spawn slots; 1 gem if extracted | Quartz Necklace |
| Moonstone (`Moonstone`) | Credited Moon Bear defeats only; A8a prerequisite | 2% (1/50) | Moonstone Ring |
| Garnet (`Garnet`) | World treasure chests in Elderwood or Shimmering Lakes | 10% (1/10) | Garnet Necklace |
| Green Jade (`GreenJade`) | Dedicated Jade rock replacing Tin slots in Shimmering Lakes only | 2% of eligible spawn slots; 1 gem if extracted | Jade Ring |
| Amethyst (`Amethyst`) | Dedicated Amethyst rock replacing Iron Ore slots in Mossy Mire only | 3% of eligible spawn slots; 1 gem if extracted | Amethyst Necklace |
| Aquamarine (`Aquamarine`) | World treasure chests in Shimmering Lakes only | 5% (1/20) | Aquamarine Ring |
| Tourmaline (`Tourmaline`) | Dedicated Tourmaline rock replacing slate slots in Thunder Mountains only | 2% of eligible spawn slots; 1 gem if extracted | Tourmaline Necklace |
| **Special: Star Opal** (`StarOpal`) | Dedicated Star Opal rock replacing Frost Salt mineral-deposit slots in Freezing Tundra; not frostmoss | 1% of eligible spawn slots; 1 gem if extracted | Opal Helm |
| **Special: Storm Sapphire** (`StormSapphire`) | World treasure chests in Thunder Mountains only | 2% (1/50) | Stormcut Sword |
| **Special: Dusk Diamond** (`DuskDiamond`) | Credited Mireling defeats in Mossy Mire only | 1% (1/100) | Dusk Ring |

**Gemstone-rock generation:** choose the rock type during seeded resource
generation, not after exhausting an ore vein. Retain the existing candidate
budget, suitability and stable identity. Use one mutually exclusive selection
per candidate: Copper 5% Quartz/95% Copper; Tin 5% Quartz/2% Jade/93% Tin;
Iron Ore 3% Amethyst/97% Iron Ore; slate 2% Tourmaline/98% slate; Frost Salt
deposits 1% Star Opal/99% Frost Salt. These replacement rates are proposed
tuning, not user-approved values. No extra gemstone spawn on top of the ore
candidate, no ore co-drop and no hidden gem roll from ordinary ore nodes.
Give each rock an original visible mineral inclusion and its own target name.

**Mining rule selected by the user:** every accepted pickaxe hit has a 1/10
(10%) chance to smash the gem. Resolve that roll before extraction, including
the final hit. A smash ends the rock with zero gem reward. If extraction
finishes without a smash, expose exactly one matching gem, with no additional
drop-chance roll. No gem shards, secondary ore reward or retry after a smash.

**Proposed hit tuning for review:** extraction takes 3 accepted hits per rock;
pick level/damage cannot skip hits or change the 10% risk. This gives 72.9%
overall success (0.9 cubed), not 90%. Quartz/Jade/Tourmaline/Star Opal require
Stone Pick or better; Amethyst requires Bronze Pick or better, matching its
replaced Iron Ore source. Spend one pick condition per accepted hit, including
a smash. Misses, duplicate requests and invalid/broken/wrong-family/out-of-range
tools cannot roll, progress or spend condition. No bare-hand/axe extraction.

The server owns type, hit count, smash results and reward. Persist partial
hit progress, terminal outcome and loose gem/claim state under the stable node
identity; streaming, reconnect or restart cannot reset progress, reroll hits,
revive smashed gems or duplicate extraction. Commit the hit result before
publishing it. A full inventory leaves a successfully extracted gem available
for normal pickup. Show cracking progress and an explicit "Gem shattered"
result on failure. This requires dedicated attack-driven mining and persistence
support; it is not assumed to exist in current interaction-based mining.

Enemy/treasure gems still roll once per credited defeat/chest generation.
Persist claims; opening, retries or reload cannot reroll. Lake treasure may
roll Garnet/Aquamarine independently. Moonstone now belongs only to A8a's
hard Moon Bear, replacing the boar proposal. Check enemy kind and
authoritative Elderwood spawn identity. Resolve once per credited defeat
alongside its listed ordinary loot; no retry/reload reroll. Wolves,
boars, deer and Mirelings cannot drop Moonstone. Dusk Diamond remains exclusive
to Mossy Mire Mirelings. No universal source fallback.
Existing Amber is not counted among the ten gems; no gem conversion or random
crafting outcomes. Existing non-gem resource generation is unchanged except
for the explicitly selected replacement slots.

- Review Necklace/Ring slot support. Equip one item in each named slot using validated GUID/replace/return and full-pack rules; copies cannot stack bonuses. The varied effect table below supersedes the uniform necklace-Armor/ring-stamina proposal. Quartz Necklace and Moonstone Ring retain the earlier +10 Armor/+20 maximum stamina roles; other bonuses are concrete reviewable proposals.
- Add Quartz Necklace (`QuartzNecklace`) with the reviewed craft/equip/stat/icon path (blocked on gem acquisition and accessory support).
- Add Moonstone Ring (`MoonstoneRing`) with the reviewed craft/equip/stat/icon path, replacing the Quartz Ring proposal (blocked on gem acquisition and accessory support).
- Add Garnet Necklace (`GarnetNecklace`) for maximum health (blocked on treasure loot and accessory support).
- Add Jade Ring (`JadeRing`), Amethyst Necklace (`AmethystNecklace`), Aquamarine Ring (`AquamarineRing`) and Tourmaline Necklace (`TourmalineNecklace`), one complete item per increment, using the distinct endurance/support/recovery roles below.
- Add Opal Helm (`OpalHelm`), Stormcut Sword (`StormcutSword`) and Dusk Ring (`DuskRing`) as separate special-gear entries below. They require their specific special gem; no ordinary-gem substitute.
- Audit the four existing Mending/Hearth Shield/Bear's Vigor/Deer Call scrolls against their current authored discovery-to-unlock/use paths. Preserve existing locations/entitlements; propose no new scroll, trophy, creature or boss key for this alpha set. Record missing current routes as defects against the existing contract, not permission to invent a reward source.

| Accessory | Proposed Forge 1 cost for 1 | Server-owned effect while equipped |
| --- | --- | --- |
| Quartz Necklace | 3 Bronze + 1 Smoky Quartz | +10 integer Armor, added once to the existing physical-defense total |
| Moonstone Ring | 2 Bronze + 1 Moonstone | +20 maximum stamina; no direct stamina refill |
| Garnet Necklace | 3 Bronze + 1 Garnet | +20 maximum health |
| Jade Ring | 2 Bronze + 1 Green Jade | Sprint stamina cost ×0.90; other actions unchanged |
| Amethyst Necklace | 3 Bronze + 1 Amethyst | Authored support-spell stamina cost ×0.85 |
| Aquamarine Ring | 2 Bronze + 1 Aquamarine | Existing stamina regeneration rate ×1.15 |
| Tourmaline Necklace | 3 Bronze + 1 Tourmaline | Accepted Mending/draught healing received ×1.10 |
| Special Dusk Ring | 5 Bronze + 1 Dusk Diamond | +10 maximum health, +10 maximum stamina and +5 integer Armor |

All accessories stack 1; proposed weight 0.1 kg each. No durability, upgrades, repair or
attacks. Accessory Armor remains active while equipped without wear;
use the existing physical-only Armor calculation, not elemental resistance.
Maximum-stat bonuses never increase current health/stamina on equip;
unequip/replacement clamps current values to the new maxima. Repeated equip
cannot heal, refill stamina or accumulate bonuses. Recompute derived stats on
equipment changes; normal regeneration uses the applicable maximum.

Jade affects sprint expenditure only, multiplying the existing authoritative
sprint cost after its normal modifiers; no discount to jumps, dodges, swimming,
attacks, spells or crafting. Amethyst multiplies existing stamina costs for
Mending, Hearth Shield, Bear's Vigor and Deer Call; no new cost if an authored
cost is zero, no cooldown/duration/potency/unlock change. Aquamarine changes
only the active stamina regeneration rate, not the delay or conditions that
permit regeneration; no passive stamina gain while normal regeneration is
blocked. Tourmaline boosts actual incoming health recovery from Mending and
healing-draught ticks once, clamped to maximum health; no passive regeneration,
food bonus, shield bonus, outgoing-heal multiplier or resurrection. Wearing
it never grants healing by itself. Evaluate cost/regen/heal modifiers when
the action/tick resolves; unequip removes them from subsequent actions/ticks.

Combine flat maximum-stat/Armor contributions additively. Percentage effects
multiply their respective server-owned base calculations once alongside
existing Wet/Steady Meal rules; never multiply one effect twice through both
equipment and status paths. Keep calculations finite/bounded and use the
existing stat precision without early integer rounding. This needs explicit
derived-stat/modifier support; these effects are not current runtime features.

Acceptance must cover each effect's allowed and excluded actions, injured/full
health, exhausted/full stamina, blocked regeneration, partial healing ticks,
Wet/Steady Meal combinations and repeated equip/replace/unequip. Verify that
removing an item immediately removes its future modifiers, equipment modifiers
do not leak into another owner's derived stats, and rejected equipment requests
change nothing.

Gear remains owner-private and session-only under the existing equipped-gear
persistence limit; no general gear-save system. Add explicit accessory/derived-
stat handling rather than assuming the current durable-armour schema supports
it. Alternative rings/necklaces compete for the same one-per-slot positions;
their bonuses cannot stack within a slot. All ten gems and every resulting
equipment item need original generated icons and applicable manifest-registered
catalogue/recipe/stat records. Authored base spell costs stay unchanged; only
the equipped Amethyst modifier changes the resolved stamina cost.

**Special weapon/armour proposals:** both crafted at Forge level 1, output 1,
stack 1; each gem is consumed during crafting, not socketed or recoverable.

| Item | Crafting materials | Level-one profile | Upgrade base / per-level reward |
| --- | --- | --- | --- |
| Opal Helm | 20 Iron + 5 Deer Hide + 1 Star Opal | Head; 12 Armor, 150 condition, 5 kg | 5 Iron + 2 Deer Hide; +1 Armor, +10 condition |
| Stormcut Sword | 20 Iron + 2 Densewood + 2 Deer Hide + 1 Storm Sapphire | One hand; 70 Slash, 220 condition, 2.5 kg | 5 Iron + 1 Densewood; +4 Slash, +30 condition |

Use existing level cap/cost scaling, Forge upgrades and free station repair.
Helm follows normal durable wearable hit-wear and physical Armor rules; its
name does not grant cold immunity or introduce insulation. Sword uses existing
Basic melee timing/reach, zero extra attack-style damage, one condition per
accepted hit, no gathering or elemental/offensive-magic effect. Author all
required profiles, attack membership, costs/rewards, held/worn presentation and
icons. Missing catalogue-family/derived-stat support is implementation work,
not permission to replace these items with unsupported generic definitions.

## A10

- Audit all approved materials, consumables and equipment, including A8b:
  registered item/applicable JSON, valid cross-references, actual acquisition,
  original generated/imported icon and runtime mapping. No missing catalogue
  entry or fallback icon may pass final acceptance. Validate explicit N/A cases.

Approved alpha item-set acceptance.
- The user approved every queued P0/P1/P2 item and system in A0–A9 on 2026-10-19; fishing was later deferred at the user's request on 2026-10-21. Detailed conflicts are resolved in this document. Milestone groups: I0 A0 current-loop repairs; I1 A1 production; I2 A2–A6 equipment/forage/consumables except Planting Trowel; I3 A7 farming plus A5 Planting Trowel; I4 A8–A8b, ending with enemy milestone verification after A8b and before A9; I5 A9 cargo/accessories. Each group requires its own final verification. This plan does not rewrite historical roadmap acceptance.
- Reconcile delivery against the frozen checklist: each included ID must have source, payment, use/equip, upgrade or explicit non-applicability, original icon/provenance/mapping, relevant save behavior and linked verification evidence. Record every omitted candidate as deferred, not implicitly complete.
- At each resulting milestone's final increment, run required Editor/project builds, applicable automation, catalogue/icon validation and relevant peer/persistence runners from `docs/07-development-setup.md`; repair milestone defects and rerun failing checks plus required full verification. Record actual commands/results; do not equate source inspection with execution.
- Final progression scenario: fresh world, existing starter loadout only, gather normal resources → Workbench/Sinew/Bow String/Lightwood → Bow/arrows → Copper/Tin/Coal/Bronze → Bronze Pick → Iron/Densewood. Craft/equip/upgrade one of every included durable item; cook/eat each included meal and use each draught, retaining the specified cooldowns. No debug grants. Record biome/node availability rather than fabricating missing resources.
- Final approved-system scenarios: plant/renew every included crop, restart planted plots/processing jobs, exercise door/stairs/light/chest, reload cargo with vessel and verify accessory Armor/maximum-stamina bonuses without stacking/refill exploits. Verify gem exhaustion/reload/retry cannot reroll or duplicate drops. For each applicable flow use host/client plus two competing requests: no duplicate or partial reward, invalid/range/full-pack requests unchanged, no private inventory leakage. Skip only explicitly deferred systems; verify session-only effects clear as specified.
- Mark the approved item milestone complete only after required gates pass; document blockers and pre-existing failures with evidence. No whole-alpha release declaration, packaging/publication or broader scope change follows automatically from this checklist.
