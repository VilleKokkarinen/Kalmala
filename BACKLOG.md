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

### Alpha items — proposals for review

Review and edit the recipes/sources below before implementation.
[Detailed stats and rules](docs/alpha-item-details.md) retain all proposed values.
[Comparison and sources](docs/alpha-items.md).

P0 = existing gaps; P1 = item variety; P2 = new systems. The user approved
all queued P0/P1/P2 scope across A0–A9 on 2026-10-19, including the aquatic
enemy and item route. Explicit exclusions described in the details remain out.
One child task per run. Every item needs complete data, a real source and an
original integrated icon. Buildables use Hammer + raw materials, never kits.
Agents must follow reviewed values and report gaps instead of inventing designs.

**Every approved item is complete only with:** its item JSON, applicable recipe,
equipment/attack/upgrade JSON, and registration in `Content/Data/GameCatalogues.json`;
an original generated icon, prepared/imported through [the icon workflow](docs/material-icons.md),
runtime mapping and icon-validator coverage; and passing JSON/reference/icon checks.
This applies to raw drops and crafted outputs across every category. Record explicit
N/A reasons for unused data types. A missing icon or catalogue entry blocks completion.

- [x] **A0 — Existing items (P0).** [Details](docs/alpha-item-details.md#a0)
  - [x] Review scope and the A10 milestone plan; all queued P0/P1/P2 work is included, with I4 ending before A9 and I5 covering A9.
  - [x] Audit current sources and starter progression; check the Iron-source mismatch.
  - [x] Sinew: credited deer defeats have a server-rolled 50% chance to grant 1.
  - [x] Bow String: Workbench, 4 Sinew → 1 Bow String; bows use 1 each.
  - [x] Lightwood: Meadow aspens, Stone Axe+, 4 per collected tree bundle.
  - [x] Bone Fragment: add 1 to each credited boar/deer defeat.
  - [x] Fix food use for the six current cooking outputs.
  - [x] Hearth Broth: Cauldron, 1 Deer Meat + 1 Onion → 1 broth.
  - [x] Wild carrots: Meadows, hand-pick 2 Carrot + 1 Carrot Seed per patch.
  - [x] Wild potatoes: Meadows, hand-pick 2 Potato per patch; no Potato Seed.
  - [x] Wild rutabaga: Shimmering Lakes dry banks, hand-pick 2 Rutabaga + 1 Rutabaga Seed per patch.
  - [x] Wild onions: Elderwood, hand-pick 2 Onion + 1 Onion Seed per patch.

  Preserve existing Wood, Densewood, Stone, hides/meat, Amber and Frost Salt sources.
  New vegetable seeds become plantable in A7.

- [x] **A1 — Metals and fuel (P0).** [Details](docs/alpha-item-details.md#a1)
  - [x] Review processing rules and Stone → Bronze → Iron progression.
  - [x] Copper: Elderwood rocks; Tin: Shimmering Lakes shore gravel. Stone Pick+, 3 ore/node.
  - [x] Charcoal Kiln: Hammer, 20 Stone; 1 Wood → 1 Coal in 30 seconds.
  - [x] Smelter: Hammer, 20 Stone + 20 Wood; 1 ore + 1 Coal → 1 bar in 30 seconds.
    Forge alloy: 2 Copper + 1 Tin → 1 Bronze.
  - [x] Mire bog iron: Bronze Pick+, 3 Iron Ore/node; replace direct-bar drops.

  Kiln: 10 input/10 output. Smelter: 10 ore/10 Coal/10 output. No offline processing.

- [x] **A2 — Melee weapons (P1).** [Stats and upgrades](docs/alpha-item-details.md#a2)
  - [x] Review the sword/club proposal.
  - [x] Bronze Sword: Forge, 10 Bronze + 2 Lightwood + 2 Deer hide
  - [x] Iron Sword: Forge, 20 Iron + 2 Densewood + 2 Deer hide
  - [x] Wooden Club: Workbench, 4 Wood; 10 Crush, 40 condition; upgrade with 2 Wood (+20 condition, +2 Crush)

- [x] **A3 — Bows and arrows (P1).** [Stats and upgrades](docs/alpha-item-details.md#a3)
  - [x] Light Bow: Workbench, 25 Lightwood + 1 Bow String + 5 Deer Hide; 2 s draw.
  - [x] Stone Arrow: Workbench, 20 Wood + 5 stone → 20; 25 Pierce.
  - [x] Bronze Arrow: Workbench, 20 Wood + 1 Bronze bar → 20; 50 Pierce.
  - [x] Support both arrow types; prefer selected stack, first stack in inventory. (top row of inventory left to right, going down 1 row at a time until arrows found)

- [x] **A4 — Armour and shields (P1).** [Stats and upgrades](docs/alpha-item-details.md#a4)
  - [x] Review armour; retain hide Boots/Cape and defer insulation.
  - [x] Bronze Helmet: Forge, 10 Bronze + 5 Boar Hide; 3 Armor.
  - [x] Bronze Tunic: Forge, 15 Bronze + 5 Boar Hide; 5 Armor.
  - [x] Bronze Leggings: Forge, 15 Bronze + 5 Boar Hide; 5 Armor.
  - [x] Bronze Shield: Forge, 10 Bronze + 5 Lightwood + 5 Boar Hide; 10 Armor.

- [x] **A5 — Utility tools (P1/P2).** [Rules](docs/alpha-item-details.md#a5)
  - [x] Basic Torch: inventory, 2 Wood + 1 amber; 300 s burn, 6 m light.
  - [x] Planting Trowel: Forge, 10 Wood + 5 Bronze bar; requires A7.

- [x] **A6 — Food and recovery (P1).** [Effects and limits](docs/alpha-item-details.md#a6)
  - [x] Lingonberry: hand-pick Meadow shrubs, 3 per shrub.
  - [x] Forest Chanterelle: hand-pick Elderwood patches, 2 per patch.
  - [x] Keep one Steady Meal: 120 s, 10% less stamina use; no meal stacking.
  - [x] Review draught effects; use existing Cauldron, no brewing station.
  - [x] Berry Recovery Draught: 3 Lingonberry + 1 Carrot; heals 20 over 10 s.
  - [x] Root Stamina Draught: 2 Potato + 1 Onion; restores 20 stamina.
  - [x] Keep cold/fire-resistance preparations deferred; A8b Antidote is the sole poison-resistance exception.

  Draughts: one dose per craft; shared 60 s cooldown. All cooking needs a lit hearth.

- [x] **A7 — Farming (P2).** [Growth rules](docs/alpha-item-details.md#a7)
  - [x] Review farming. Garden Plot: Planting Trowel, cultivation menu addition.
  - [x] Carrot: 1 seed → 3 carrots; 1 carrot → 3 seeds.
  - [x] Potato: 1 potato → 3 potatoes; Potato Seed stays unused.
  - [x] Rutabaga: 1 seed → 3 roots; 1 root → 3 seeds.
  - [x] Onion: 1 seed → 3 onions; 1 onion → 3 seeds.

  Crops take 600 active seconds; Planting Trowel required. No offline/inactive-region growth.

- [ ] **A8 — Shelter and storage (P2).** [Placement and behavior](docs/alpha-item-details.md#a8)
  - [x] Review shelter rules. All pieces use direct-material Hammer placement.
  - [x] Timber Door: 8 Wood; 1 × 2 m, saved open/closed state.
  - [x] Timber Beam: 2 Wood; 2 m, horizontal/vertical support.
  - [x] Timber Stairs: 6 Wood; 2 m run, 1 m rise, 1 m wide.
  - [x] Camp Bed: 4 Wood + 2 Boar Hide + 2 Deer Hide; resting pose only.
  - [x] Standing Light: 3 Wood + 1 amber; 1 fuel = 600 s, 6 m light.
  - [x] Keep extra brewing stations deferred; Kiln/Smelter/Plot belong to A1/A7.

- [ ] **A8a — Biome enemies (before A9).** [Roster and stats](docs/biome-enemies.md)
  - [x] Register the three current creatures and fourteen planned enemies in GameCatalogues JSON (user request, 2026-10-21); one Wolf species, pack spawning deferred to engine work. Data authoring only; runtime enemy tasks below remain open.
  - [x] Consolidate all 17 species into creatures.json and remove redundant rules JSON (user request, 2026-10-21); shared engine policies remain in design docs. Keep one flat gameplay profile per species; no status flags, runtime snapshots or AI prose. No defensive-trigger or Swarm settings; poison uses rate and duration only.
  - [x] Review the expanded roster, spawn weights and attack/loot rules.
  - [x] Review unique enemy drops and uses; approve new materials/recipes before implementation.
  - [x] Review the recommended drop-based item/recipe table; approve effects and full gear profiles.
  - [x] Restrict boar/deer spawns to Meadows, Elderwood and Shimmering Lakes only.
  - [ ] Apply the overview's revised stats to existing boar/deer and all A8a enemies.
  - [ ] Meadows: Rat
  - [ ] Elderwood: Badger
  - [ ] Elderwood: Moon Bear — rare hard enemy;
  - [ ] Shimmering Lakes: Viper — lunging bite applies weak poison for 5 s.
  - [ ] Shimmering Lakes: Giant Snake — rare hard shoreline ambusher; powerful lunging bite.
  - [ ] Mossy Mire: Mireling — weak base enemy; spawn only in this biome.
  - [ ] Mossy Mire: Death Adder — highly venomous snake; bite applies major poison for 10 s.
  - [ ] Mossy Mire: Mosquito swarm — small bites + weak poison for 10 s; roll to shake off, lit fire repels.
  - [ ] Mossy Mire: Peat Husk — rare hard enemy;
  - [ ] Freezing Tundra: Wolf — spawns in packs of 3
  - [ ] Freezing Tundra: Lynx
  - [ ] Freezing Tundra: Polar Bear — rare hard enemy;
  - [ ] Thunder Mountains: Horned Goat — knockback attack
  - [ ] Thunder Mountains: Thunder Sentinel — rare hard enemy;
  - [ ] Ocean: Reef Maw — rare hard underwater enemy;

- [ ] **A8b — Enemy materials and crafted items (P1/P2).** [Recipes](docs/biome-enemies.md#recommended-items-and-recipes)
  - [x] Reconcile the user-edited drop amounts/chances and recipe station/cost/output values with existing approved contracts; align source ownership and the proposal table.
  - [x] Resolve the remaining effect and full equipment-profile decisions before implementing affected outputs; see [A8b review](docs/alpha-item-details.md#a8b).
  - [x] Add the registered catalogue JSON records for all 15 materials and 15 outputs, including output recipes, equipment profiles, upgrade costs/rewards, and supported attack records.
  - Item icon tasks are independent of their source/output implementation tasks. Each checkbox covers one item's original generated artwork and provenance, prepared/imported texture, runtime mapping, and icon-validator coverage. Per the user's 2026-10-22 direction, complete all remaining A8b icon checkboxes together in one short-path A8b work batch and one feature commit. Completing icons does not complete their item rows or enable acquisition. The importer retains its supported limit of 16 textures per import batch.
  - [x] Rat Tail icon (`RatTail`): original, provenance, mapping, import, and catalogue icon validation are complete; see `docs/rat-tail-icon.md`.
  - [x] Badger Fur icon (`BadgerFur`).
  - [x] Badger Meat icon (`BadgerMeat`): original, provenance, mapping, import, and focused catalogue icon validation are complete; see `docs/badger-meat-icon.md`.
  - [x] Viper Fang icon (`ViperFang`).
  - [x] Giant Scale icon (`GiantScale`).
  - [x] Venom Sac icon (`VenomSac`).
  - [x] Peat Heart icon (`PeatHeart`).
  - [x] Wolf Pelt icon (`WolfPelt`).
  - [x] Wolf Fang icon (`WolfFang`).
  - [x] Wolf Meat icon (`WolfMeat`).
  - [x] Lynx Pelt icon (`LynxPelt`).
  - [x] Polar Bear Pelt icon (`PolarBearPelt`).
  - [x] Goat Horn icon (`GoatHorn`).
  - [x] Thunder Core icon (`ThunderCore`).
  - [x] Reef Tooth icon (`ReefTooth`).
  - [x] Rat-tail Tonic icon (`RatTailTonic`).
  - [x] Badger Fur Vest icon (`BadgerFurVest`).
  - [x] Roasted Badger icon (`RoastedBadger`).
  - [x] Fang Arrows icon (`FangArrows`).
  - [x] Scale Cuirass icon (`ScaleCuirass`).
  - [x] Antidote icon (`Antidote`).
  - [x] Peat Heart Stew icon (`PeatHeartStew`).
  - [x] Wolf Pelt Boots icon (`WolfPeltBoots`).
  - [x] Fang Spear icon (`FangSpear`).
  - [x] Wolf Stew icon (`WolfStew`).
  - [x] Lynx Mantle icon (`LynxMantle`).
  - [x] Polar Bear Coat icon (`PolarBearCoat`).
  - [x] Horn Bow icon (`HornBow`).
  - [x] Thunder Maul icon (`ThunderMaul`).
  - [x] Reef Harpoon icon (`ReefHarpoon`).
  - Grouped icon batch completed 2026-10-22 in one feature commit by user direction; originals, preparation, imports, mapping and focused coverage are recorded in `docs/a8b-icon-batch.md`. The 30 item/source rows remain open.
  - [ ] Rat Tail: Rat, 1 guaranteed; material + source + JSON + generated icon.
    - Original icon, mapping, and import are complete; item remains open until Rat has an accepted runtime drop.
  - [ ] Badger Fur: Badger, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Badger Meat: Badger, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Viper Fang: Viper, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Giant Scale: Giant Snake, 2 guaranteed; material + source + JSON + generated icon.
  - [ ] Venom Sac: Death Adder, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Peat Heart: Peat Husk, 25% chance of 1; material + source + JSON + generated icon.
  - [ ] Wolf Pelt: Wolf, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Wolf Fang: Wolf, 25% chance of 1; material + source + JSON + generated icon.
  - [ ] Wolf Meat: Wolf, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Lynx Pelt: Lynx, 50% chance of 1; material + source + JSON + generated icon.
  - [ ] Polar Bear Pelt: Polar Bear, 2 guaranteed; material + source + JSON + generated icon.
  - [ ] Goat Horn: Horned Goat, 1 guaranteed; material + source + JSON + generated icon.
  - [ ] Thunder Core: Thunder Sentinel, 25% chance of 1; material + source + JSON + generated icon.
  - [ ] Reef Tooth: Reef Maw, 50% chance of 1; material + source + JSON + generated icon.
  - [ ] Rat-tail Tonic: Cauldron, 2 Rat Tail + 2 Lingonberry → 1; heal 30 over 10 s; reject full health/active healing, stop on death, share the 60 s A6 recovery cooldown.
  - [ ] Badger Fur Vest: Workbench, 6 Badger Fur → 1; Chest, 6 Armor.
  - [ ] Roasted Badger: Cooking Rack, 1 Badger Meat → 1; Steady Meal.
  - [ ] Fang Arrows: Workbench, 2 Viper Fang + 10 Wood → 20; 50 Pierce, weak poison 5 s.
  - [ ] Scale Cuirass: Forge, 10 Giant Scale + 10 Bronze → 1; Chest, 20 Armor.
  - [ ] Antidote: Cauldron, 1 Venom Sac + 2 Lingonberry → 1; clear weak poison, reduce major poison to weak, resist poison for 10 min; share the 60 s A6 recovery cooldown.
  - [ ] Peat Heart Stew: Cauldron, 1 Peat Heart + 2 Potato + 1 Onion → 2; Steady Meal.
  - [ ] Wolf Pelt Boots: Workbench, 5 Wolf Pelt + 5 Iron → 1; Boots, 10 Armor.
  - [ ] Fang Spear: Forge, 4 Wolf Fang + 10 Iron + 10 Densewood → 1; 100 Stab.
  - [ ] Wolf Stew: Cauldron, 1 Wolf Meat + 3 Carrot + 3 Onion → 1; Steady Meal.
  - [ ] Lynx Mantle: Workbench, 4 Lynx Pelt → 1; Cape, 8 Armor.
  - [ ] Polar Bear Coat: Forge, 5 Polar Bear Pelt + 20 Iron → 1; Chest, 50 Armor.
  - [ ] Horn Bow: Workbench, 2 Goat Horn + 10 Densewood + 1 Bow String → 1; +100 Pierce, slow draw.
  - [ ] Thunder Maul: Forge, 1 Thunder Core + 20 Iron + 10 Densewood → 1; 100 Crush + 30 Lightning.
  - [ ] Reef Harpoon: Forge, 3 Reef Tooth + 8 Iron + 4 Lightwood → 1; 60 Pierce.
  - [ ] Verify every approved drop/output has complete registered JSON, a reachable source and a generated runtime icon.
  - [ ] Run the combined A8a/A8b enemy and enemy-item milestone verification; record results and blockers. This gate must pass before A9 begins.

  The JSON-only child and all 30 icon children are complete; all 30 item rows
  remain unchecked until their reachable source drops (for materials) and
  server-side item behavior are complete. No A8b enemy drop has been enabled.

  Bow String is reserved for bows. Review every pending non-bow binding input in its category contract; do not use Bow String there. Moonstone Ring and Dusk Ring stay in A9. Do not duplicate them.
  Enable new drops only when their material task is complete. A8b effects and
  full armor/weapon profiles are resolved in
  [the item contract](docs/alpha-item-details.md#a8b). The Reef Harpoon route is
  approved but adds no skiff damage. The current committed baseline has none of
  the 15 A8b material-producing enemies or their credited-defeat rewards in
  accepted runtime; the creature catalogue is authoring data only. Keep each
  material and dependent output incomplete until its named enemy has a
  reachable, accepted server-side drop. Recheck after A8a runtime/loot work is
  integrated and accepted.
  All 30 new material/output tasks include the item-completion gate above.

- [ ] **A9 — gems and special gear (P2).** [Rules](docs/alpha-item-details.md#a9)
  - [ ] Review A9.
  - [ ] Review Necklace/Ring slots; one item per slot, bonuses only while equipped.
  - Each A9 item is completed through separate item-scoped steps in this order: add its complete item/profile JSON and register its file in `Content/Data/GameCatalogues.json` first; add and register its recipe next when crafted; generate and integrate its original icon next; add or enable its acquisition/source-table entry last when needed. Icon work includes provenance, preparation/import, runtime mapping, and validator coverage. Gem recipes are N/A; crafted gear has no drop-table entry. Each stage can be completed in its own short-path worktree and committed individually. Do not enable acquisition before its item JSON and icon are complete or its source dependency is accepted. The sequence applies only after the A9 review and I4 gate.
  - [ ] Smoky Quartz (`SmokyQuartz`): Quartz rock; proposed 5% replacement of Copper/Tin spawn slots.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Quartz-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Moonstone (`Moonstone`): proposed 2% chance of 1 per credited Moon Bear defeat; requires A8a.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable the Moon Bear reward-table entry last, after item JSON/icon completion and accepted A8a source.
  - [ ] Garnet (`Garnet`): proposed 10% chance of 1 in Elderwood/Lakes world treasure chests; chest-only.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [ ] Green Jade (`GreenJade`): Jade rock; proposed 2% replacement of Tin spawn slots in Shimmering Lakes.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Jade-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Amethyst (`Amethyst`): Amethyst rock; proposed 3% replacement of Iron Ore spawn slots in Mossy Mire.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Amethyst-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Aquamarine (`Aquamarine`): proposed 5% chance of 1 in Shimmering Lakes treasure chests.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [ ] Tourmaline (`Tourmaline`): Tourmaline rock; proposed 2% replacement of slate spawn slots in Thunder Mountains.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Tourmaline-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Star Opal (`StarOpal`, special): Tundra rock; proposed 1% replacement of Frost Salt deposit slots.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Star-Opal-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Storm Sapphire (`StormSapphire`, special): proposed 2% chance of 1 in Thunder Mountains treasure.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [ ] Dusk Diamond (`DuskDiamond`, special): proposed 1% chance of 1 from Mossy Mire Mirelings.
    - [ ] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Mireling reward-table entry last, after item JSON/icon completion and accepted A8a source.
  - [ ] Gemstone rocks: implement the shared rare resource-slot replacement and 10% gem-smash rule; successful extraction yields 1 gem. This shared source mechanic does not add per-item acquisition entries.
  - [ ] Quartz Necklace (`QuartzNecklace`): finalize its recipe and +10 Armor profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Moonstone Ring (`MoonstoneRing`): finalize its recipe and +20 maximum stamina profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Garnet Necklace (`GarnetNecklace`): finalize its recipe and +20 maximum health profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Jade Ring (`JadeRing`): finalize its recipe and sprint-stamina profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Amethyst Necklace (`AmethystNecklace`): finalize its recipe and support-spell profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Aquamarine Ring (`AquamarineRing`): finalize its recipe and stamina-regeneration profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Tourmaline Necklace (`TourmalineNecklace`): finalize its recipe and healing-received profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Opal Helm (`OpalHelm`): finalize its Forge recipe and armor profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/profile JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Stormcut Sword (`StormcutSword`): finalize its Forge recipe and attack profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/attack JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Dusk Ring (`DuskRing`): finalize its recipe and combined health/stamina/Armor profile in Review A9; see the A9 item contract.
    - [ ] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [ ] Add and register its Forge recipe using the values finalized by Review A9.
    - [ ] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Audit the four existing scroll routes; defer new scrolls, trophies and boss keys.

- [ ] **A10 — Acceptance.** [Full criteria](docs/alpha-item-details.md#a10)
  - [ ] Audit all approved item IDs against registered JSON and generated/imported runtime icons; zero missing entries, references or fallback icons.
  - [ ] Review milestones: I0 A0; I1 A1; I2 A2–A6 except Planting Trowel; I3 A7 farming + Planting Trowel; I4 A8–A8b with its final enemy gate before A9; I5 A9.
  - [ ] Check each included item's source, recipe, use, upgrades, icon and save behavior.
  - [ ] Run required full verification at each milestone's final increment.
  - [ ] Verify fresh-start progression through Bronze and Iron without debug grants.
  - [ ] Verify approved farming, shelter, cargo, accessories, peers and restart.
  - [ ] Record results/blockers; mark complete only after required gates pass.

## User-requested fixes

- [x] Resize inventory crafting to a 256 px item list/search field and 384 px selected-item pane, preserving the existing gutter and outer padding.

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
