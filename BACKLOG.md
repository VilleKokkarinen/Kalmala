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

- [x] **A8 — Shelter and storage (P2).** [Placement and behavior](docs/alpha-item-details.md#a8)
  - [x] Review shelter rules. All pieces use direct-material Hammer placement.
  - [x] Timber Door: 8 Wood; 1 × 2 m, saved open/closed state.
  - [x] Timber Beam: 2 Wood; 2 m, horizontal/vertical support.
  - [x] Timber Stairs: 6 Wood; 2 m run, 1 m rise, 1 m wide.
  - [x] Camp Bed: 4 Wood + 2 Boar Hide + 2 Deer Hide; resting pose only.
  - [x] Standing Light: 3 Wood + 1 amber; 1 fuel = 600 s, 6 m light.
  - [x] Keep extra brewing stations deferred; Kiln/Smelter/Plot belong to A1/A7.

- [x] **A8a — Biome enemies (before A9).** [Roster and stats](docs/biome-enemies.md)
  - [x] Register the three current creatures and fourteen planned enemies in GameCatalogues JSON (user request, 2026-10-21); one Wolf species, pack spawning deferred to engine work. Catalogue authoring is complete; combined runtime acceptance remains tracked under I4 below.
  - [x] Consolidate all 17 species into creatures.json and remove redundant rules JSON (user request, 2026-10-21); shared engine policies remain in design docs. Keep one flat gameplay profile per species; no status flags, runtime snapshots or AI prose. No defensive-trigger or Swarm settings; poison uses rate and duration only.
  - [x] Review the expanded roster, spawn weights and attack/loot rules.
  - [x] Review unique enemy drops and uses; approve new materials/recipes before implementation.
  - [x] Review the recommended drop-based item/recipe table; approve effects and full gear profiles.
  - [x] Restrict boar/deer spawns to Meadows, Elderwood and Shimmering Lakes only.
  - [x] Apply the overview's revised stats to existing boar/deer and all A8a enemies.
  - [x] Meadows: Rat
  - [x] Elderwood: Badger
  - [x] Elderwood: Moon Bear — rare hard enemy;
  - [x] Shimmering Lakes: Viper — lunging bite applies weak poison for 5 s.
  - [x] Shimmering Lakes: Giant Snake — rare hard shoreline ambusher; powerful lunging bite.
  - [x] Mossy Mire: Mireling — weak base enemy; spawn only in this biome.
  - [x] Mossy Mire: Death Adder — highly venomous snake; bite applies major poison for 10 s.
  - [x] Mossy Mire: Mosquito swarm — small bites + weak poison for 10 s; roll to shake off, lit fire repels.
  - [x] Mossy Mire: Peat Husk — rare hard enemy;
  - [x] Freezing Tundra: Wolf — spawns in packs of 3
  - [x] Freezing Tundra: Lynx
  - [x] Freezing Tundra: Polar Bear — rare hard enemy;
  - [x] Thunder Mountains: Horned Goat — knockback attack
  - [x] Thunder Mountains: Thunder Sentinel — rare hard enemy;
  - [x] Ocean: Reef Maw — rare hard underwater enemy;

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
  - Grouped icon batch completed 2026-10-22 in one feature commit by user direction; originals, preparation, imports, mapping and focused coverage are recorded in `docs/a8b-icon-batch.md`. All 30 source/output implementation rows are checked; combined I4 acceptance remains open.
  - [x] Rat Tail: Rat, 1 guaranteed; material + source + JSON + generated icon.
    - Original icon, mapping, import, registered material definition, and owner-bound credited-defeat reward are complete; combined I4 acceptance remains pending.
  - [x] Badger Fur: Badger, 1 guaranteed; material + source + JSON + generated icon.
  - [x] Badger Meat: Badger, 1 guaranteed; material + source + JSON + generated icon.
  - [x] Viper Fang: Viper, 1 guaranteed; material + source + JSON + generated icon.
    - Registered material/icon and the guaranteed fang are complete; each credited Viper claim now commits Fang plus baseline Bone Fragment atomically. Combined I4 acceptance remains pending.
  - [x] Giant Scale: Giant Snake, 2 guaranteed; material + source + JSON + generated icon.
  - [x] Venom Sac: Death Adder, 1 guaranteed; material + source + JSON + generated icon.
  - [x] Peat Heart: Peat Husk, 25% chance of 1; material + source + JSON + generated icon.
  - [x] Wolf Pelt: Wolf, 1 guaranteed; material + source + JSON + generated icon.
  - [x] Wolf Fang: Wolf, 25% chance of 1; material + source + JSON + generated icon.
  - [x] Wolf Meat: Wolf, 1 guaranteed; material + source + JSON + generated icon.
  - [x] Lynx Pelt: Lynx, 50% chance of 1; material + source + JSON + generated icon.
  - [x] Polar Bear Pelt: Polar Bear, 2 guaranteed; material + source + JSON + generated icon.
  - [x] Goat Horn: Horned Goat, 1 guaranteed; material + source + JSON + generated icon.
  - [x] Thunder Core: Thunder Sentinel, 25% chance of 1; material + source + JSON + generated icon.
  - [x] Reef Tooth: Reef Maw, 50% chance of 1; material + source + JSON + generated icon.
  - [x] Rat-tail Tonic: Cauldron, 2 Rat Tail + 2 Lingonberry → 1; heal 30 over 10 s; reject full health/active healing, stop on death, share the 60 s A6 recovery cooldown.
  - [x] Badger Fur Vest: Workbench, 6 Badger Fur → 1; Chest, 6 Armor.
  - [x] Roasted Badger: Cooking Rack, 1 Badger Meat → 1; Steady Meal.
  - [x] Fang Arrows: Workbench, 2 Viper Fang + 10 Wood → 20; 50 Pierce, weak poison 5 s.
  - [x] Scale Cuirass: Forge, 10 Giant Scale + 10 Bronze → 1; Chest, 20 Armor.
  - [x] Antidote: Cauldron, 1 Venom Sac + 2 Lingonberry → 1; clear weak poison, reduce major poison to weak, resist poison for 10 min; share the 60 s A6 recovery cooldown.
  - [x] Peat Heart Stew: Cauldron, 1 Peat Heart + 2 Potato + 1 Onion → 2; Steady Meal.
  - [x] Wolf Pelt Boots: Workbench, 5 Wolf Pelt + 5 Iron → 1; Boots, 10 Armor.
  - [x] Fang Spear: Forge, 4 Wolf Fang + 10 Iron + 10 Densewood → 1; 100 Stab.
  - [x] Wolf Stew: Cauldron, 1 Wolf Meat + 3 Carrot + 3 Onion → 1; Steady Meal.
  - [x] Lynx Mantle: Workbench, 4 Lynx Pelt → 1; Cape, 8 Armor.
  - [x] Polar Bear Coat: Forge, 5 Polar Bear Pelt + 20 Iron → 1; Chest, 50 Armor.
  - [x] Horn Bow: Workbench, 2 Goat Horn + 10 Densewood + 1 Bow String → 1; +100 Pierce, slow draw.
  - [x] Thunder Maul: Forge, 1 Thunder Core + 20 Iron + 10 Densewood → 1; 100 Crush + 30 Lightning.
  - [x] Reef Harpoon: Forge, 3 Reef Tooth + 8 Iron + 4 Lightwood → 1; 60 Pierce.
  - [x] Verify every approved drop/output has complete registered JSON, a reachable source and a generated runtime icon.
  - [ ] Run the combined A8a/A8b enemy and enemy-item milestone verification; record results and blockers. This gate must pass before A9 begins.
    - [x] Rebuild and rerun the sequential Badger/Moon Bear peer acceptance after the Moon Bear fixture adjustment; both cases passed. The final full queue now passes all 180 tests.
    - [x] Repair the procedural snake-head `SurfaceWinding` failure and the M9 progression assertions; the focused check and final 180-test Kalmala queue pass.
    - [ ] Finish live acceptance for remaining A8a species-specific spawn and attack/status behavior, then exercise approved A8b item progression and peer flows. `Verify-RatPeer.ps1` is the sole I4 end-to-end enemy test for loot delivery/retry, defeat persistence and combat replication; do not repeat those shared checks in other species cases. Species tests may cover their unique authored profiles, drop-table data and attack/status behavior. Prior peer passes for Boar, Deer, Badger, Moon Bear, Mireling, Viper and Giant Snake remain historical evidence, not extra required shared-system tests. `Kalmala.Gameplay.Status.PoisonEffects` is the single shared weak/major poison and Antidote status automation. Mosquito bite/poison and dodge/fire counterplay acceptance plus Peat Husk sweep acceptance are recorded below and passed live. Remaining species-specific live checks are Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw.
      - [x] Add the Development-only generated Wolf three-member pack peer fixture and `Scripts/Tests/Combat/Verify-WolfPeer.ps1` for stable formation, 180 cm server bite, and replicated telegraph.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/Verify-WolfPeer.ps1`; confirm all three generated members, formation spacing, both client telegraphs, and accepted server lunge damage (passed 2026-10-23).
      - [x] Add the Development-only generated Lynx peer fixture and `Scripts/Tests/Combat/Verify-LynxPeer.ps1` for a seeded dry-Tundra Lynx, its 150-health profile, telegraphed leap, and peer-visible cue.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/Verify-LynxPeer.ps1`; confirm the 350 cm server leap lands and both clients observe its replicated telegraph (passed 2026-10-23).
      - [x] Add the Development-only generated Polar Bear peer fixture and `Scripts/Tests/Combat/Verify-PolarBearPeer.ps1` for the 150 m start exclusion, 750-health profile, committed charge/retreat/recovery and peer-visible cue.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/Verify-PolarBearPeer.ps1 -WorldSeed 445`; require the start exclusion, accepted server charge damage and matching telegraphs on both clients.
      - [x] Add the Development-only generated Horned Goat peer fixture and `Scripts/Tests/Combat/Verify-HornedGoatPeer.ps1` for its 200-health profile, 180 cm horn hit, approved push/stagger and peer-visible cue.
       - [ ] Build the affected Editor target and run `Scripts/Tests/Combat/Verify-HornedGoatPeer.ps1 -WorldSeed 445`; confirm server damage, push/stagger, and matching client telegraphs.
      - [x] Add the Development-only generated Peat Husk peer fixture and `Scripts/Tests/Combat/Verify-PeatHuskPeer.ps1` for the 150 m start exclusion, replicated telegraph, and accepted server sweep.
      - [x] Build the affected Editor target and run `Scripts/Tests/Combat/Verify-PeatHuskPeer.ps1`; confirm start distance, both client telegraphs, and server sweep damage (passed 2026-10-23).
      - [x] Run the generated Mosquito swarm peer case for repeated server bites, ten-second weak-poison application, owner-only status, and public health replication.
      - [x] Add Development-only generated-peer assertions for an owner-client accepted dodge, rejected cooldown retry, the roll's two-second repel window, fuelled held-torch protection, unequipped-torch bite resumption, and unlit/lit placed-campfire transitions.
      - [x] Build the affected target and run `Scripts/Tests/Combat/Verify-MosquitoSwarmPeer.ps1`; confirm dodge/fire counterplay plus health replication and owner-only poison before completing this child (passed 2026-10-23).
    - [ ] Resolve the `Verify-InventoryReconnect.ps1` client-baseline timeout or document an approved non-applicable disposition before closing final I4 verification.

  The JSON-only child, all 30 icon children and all 30 source/output
  implementation rows are complete. Enemy rewards and dependent items are
  enabled through their server-side paths; their combined live I4 acceptance
  remains open.

  Bow String is reserved for bows. Review every pending non-bow binding input in its category contract; do not use Bow String there. Moonstone Ring and Dusk Ring stay in A9. Do not duplicate them.
  A8b effects and full armor/weapon profiles are resolved in [the item
  contract](docs/alpha-item-details.md#a8b). The Reef Harpoon route is approved
  but adds no skiff damage. All 15 material-producing enemy sources and their
  registered outputs are implemented; the I4 gate above retains the remaining
  live peer, reward retry/restart/privacy and item-progression acceptance.

- [ ] **A9 — gems and special gear (P2).** [Rules](docs/alpha-item-details.md#a9)
  - Completion handoff: the 50 checked JSON/recipe/icon stages below were implemented in `71fa8060` on `codex/a9-json-icons` and merged into master on 2026-10-22. Acquisition, runtime accessory behavior, reviews and milestone verification remain open.
  - [ ] Review A9.
  - [x] Review Necklace/Ring slots; one item per slot, bonuses only while equipped. Existing validated owner-only transactions retained; focused AccessorySlots regression added.
  - [ ] Apply the remaining authored accessory stats at runtime: `MaxHealthBonus` and `MaxStaminaBonus` as flat equipped bonuses, plus additive `StaminaRegenerationMultiplier` and `HealingReceivedMultiplier` deltas. Recalculate server-owned stats on equip/unequip and add focused status/healing coverage.
  - Each A9 item is completed through separate item-scoped steps in this order: add its complete item/profile JSON and register its file in `Content/Data/GameCatalogues.json` first; add and register its recipe next when crafted; generate and integrate its original icon next; add or enable its acquisition/source-table entry last when needed. Icon work includes provenance, preparation/import, runtime mapping, and validator coverage. Gem recipes are N/A; crafted gear has no drop-table entry. Each stage can be completed in its own short-path worktree and committed individually. Do not enable acquisition before its item JSON and icon are complete or its source dependency is accepted. The sequence applies only after the A9 review and I4 gate.
  - User-directed 2026-10-22 batch: use the proposal-table values for these JSON/recipe records and generated icons. The review, accessory runtime behavior, gem sources, and I4/I5 acceptance remain open.
  - [ ] Smoky Quartz (`SmokyQuartz`): Quartz rock; proposed 5% replacement of Copper/Tin spawn slots.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Quartz-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Moonstone (`Moonstone`): proposed 2% chance of 1 per credited Moon Bear defeat; requires A8a.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable the Moon Bear reward-table entry last, after item JSON/icon completion and accepted A8a source.
  - [ ] Garnet (`Garnet`): proposed 10% chance of 1 in Elderwood/Lakes world treasure chests; chest-only.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [ ] Green Jade (`GreenJade`): Jade rock; proposed 2% replacement of Tin spawn slots in Shimmering Lakes.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Jade-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Amethyst (`Amethyst`): Amethyst rock; proposed 3% replacement of Iron Ore spawn slots in Mossy Mire.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Amethyst-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Aquamarine (`Aquamarine`): proposed 5% chance of 1 in Shimmering Lakes treasure chests.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [ ] Tourmaline (`Tourmaline`): Tourmaline rock; proposed 2% replacement of slate spawn slots in Thunder Mountains.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Tourmaline-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Star Opal (`StarOpal`, special): Tundra rock; proposed 1% replacement of Frost Salt deposit slots.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Star-Opal-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [ ] Storm Sapphire (`StormSapphire`, special): proposed 2% chance of 1 in Thunder Mountains treasure.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [ ] Dusk Diamond (`DuskDiamond`, special): proposed 1% chance of 1 from Mossy Mire Mirelings.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [ ] Add or enable its Mireling reward-table entry last, after item JSON/icon completion and accepted A8a source.
  - [ ] Gemstone rocks: implement the shared rare resource-slot replacement and 10% gem-smash rule; successful extraction yields 1 gem. This shared source mechanic does not add per-item acquisition entries.
  - [ ] Quartz Necklace (`QuartzNecklace`): finalize its recipe and +10 Armor profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Moonstone Ring (`MoonstoneRing`): finalize its recipe and +20 maximum stamina profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Garnet Necklace (`GarnetNecklace`): finalize its recipe and +20 maximum health profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Jade Ring (`JadeRing`): finalize its recipe and sprint-stamina profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Amethyst Necklace (`AmethystNecklace`): finalize its recipe and support-spell profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Aquamarine Ring (`AquamarineRing`): finalize its recipe and stamina-regeneration profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Tourmaline Necklace (`TourmalineNecklace`): finalize its recipe and healing-received profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Opal Helm (`OpalHelm`): finalize its Forge recipe and armor profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/profile JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Stormcut Sword (`StormcutSword`): finalize its Forge recipe and attack profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/attack JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Dusk Ring (`DuskRing`): finalize its recipe and combined health/stamina/Armor profile in Review A9; see the A9 item contract.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [ ] Audit the four existing scroll routes; defer new scrolls, trophies and boss keys.

- [ ] **A10 — Acceptance.** [Full criteria](docs/alpha-item-details.md#a10)
  - [ ] Audit all approved item IDs against registered JSON and generated/imported runtime icons; zero missing entries, references or fallback icons.
  - [ ] Review milestones: I0 A0; I1 A1; I2 A2–A6 except Planting Trowel; I3 A7 farming + Planting Trowel; I4 A8–A8b with its final enemy gate before A9; I5 A9.
  - [ ] Check each included item's source, recipe, use, upgrades, icon and save behavior.
  - [ ] Run required full verification at each milestone's final increment.
  - [ ] Verify fresh-start progression through Bronze and Iron without debug grants.
  - [ ] Verify approved farming, shelter, cargo, accessories, peers and restart.
  - [ ] Record results/blockers; mark complete only after required gates pass.

### Monster population and camp spawn suppression — user request, 2026-10-22

Planning only, queued after existing alpha acceptance; A8a/I4 scope is unchanged.
Use original content. The supplied Valheim examples are inspiration, not a
verified specification. [Fixed targets and behavior](docs/monster-spawning.md)
are specified in advance; implement them without requesting tuning values.

**Run instructions:** Complete the first unchecked, unblocked child below,
exactly one per run, in order. Each normal checkbox targets roughly 10–20 minutes
of integrated work plus a lightweight check. If it will exceed one turn, split
that checkbox into smaller ordered children before implementing its first part.
Keep intermediate code safe: do not enable natural spawning until S26. Record
files, checks, remaining scope and next task in `PROGRESS.md`; commit only the
run's changes, then stop. Add focused tests with the helper they exercise where
practical; execute full verification only at the final gate. S35 is the required
milestone-final verification/repair run and can exceed a normal turn.

- [ ] **Implement Kalmala monster population and camp spawn suppression.**

  - [ ] **S01 — Map the fixed contract to code.** Read `docs/monster-spawning.md`;
    identify existing scheduler, descriptor, budget and spawn/defeat entry points.
    Record the implementation map in that contract. Values are already decided;
    do not reopen tuning or change runtime behavior in this step.
  - [ ] **S02 — Map lifecycle integration.** Identify callbacks enforcing permanent
    defeated-slot depletion, 30 s living-slot unload cooldown and reward escrow.
    Record integration points for the fixed lifecycle; add no save-format change.
  - [ ] **S03 — Add named tuning constants.** Use existing invisible 64 × 64 m
    spatial keys; ring 32–96 m, interval 5 s and count radius 96 m.
    Validate bounds/units; keep spawning disabled.
  - [ ] **S04 — Gather active players.** Add a server-only list of living players
    in the current world; ignore dead, disconnected and invalid pawns.
  - [ ] **S05 — Deduplicate active keys.** Derive the union of nearby loaded keys
    from S04; overlapping players must produce each key once.
  - [ ] **S06 — Add scheduler cadence.** Schedule each active key once per 5 s
    with stable staggering; remove inactive keys without accumulating retries.
  - [ ] **S07 — Bound scheduler work.** Use a 0.25 s wake, at most 4 keys,
    4 candidates per key and 16 candidates total per wake; skip missed ticks
    instead of catch-up bursts.
  - [ ] **S08 — Generate ring candidates.** Select existing stable descriptors
    within 32–96 m; use the contract's seeded 25% admission roll and rotating cursor.
    Preserve descriptor positions, identities and biome weights.
  - [ ] **S09 — Exclude nearby players.** Reject a candidate within 36 m of any
    living player, including a second player outside the selecting neighborhood.
  - [ ] **S10 — Track live creatures.** Register/unregister authoritative live
    actors; remove defeated/destroyed entries and count their current positions.
  - [ ] **S11 — Enforce area caps.** Count across loaded keys within 96 m of the
    candidate; per-species caps: common 4, rare hard enemy 1, Wolves 6.
    Also enforce the existing total actor budget; add no extra actor allowance.
  - [ ] **S12 — Reserve pack capacity.** Reserve three Wolf slots atomically;
    reject partial packs and release reservations on failed creation.
  - [ ] **S13 — Add the nine-gate evaluator.** Return a distinct rejection reason
    for scheduler budget, seeded roll, player distance, habitat, time, weather,
    progression, caps and physical placement. Unimplemented gates reject safely.
  - [ ] **S14 — Wire scheduler and roll gates.** Feed S06–S08 into the evaluator;
    one rejected attempt consumes only its bounded attempt budget.
  - [ ] **S15 — Wire distance and habitat gates.** Use S09 and existing species
    biome/dry-shore/start-exclusion rules; do not relax existing restrictions.
  - [ ] **S16 — Add time-of-day gate.** Pass for every current species;
    expose only an internal policy seam for rejection fixtures. No new clock.
  - [ ] **S17 — Add weather gate.** Pass for every current species;
    expose an internal rejection-test seam. Add no new weather restrictions.
  - [ ] **S18 — Add progression gate.** Pass for every current species;
    expose an internal rejection-test seam. Add no boss or progression locks.
  - [ ] **S19 — Wire cap gate.** Apply S11–S12 immediately before creation so
    overlapping key/player attempts cannot overfill a species or total budget.
  - [ ] **S20 — Validate land surfaces.** Require generated dry ground, existing
    start restrictions and slope at most 32 degrees for land creatures.
  - [ ] **S21 — Validate aquatic surfaces.** Reuse Reef Maw habitat/depth rules
    and check water clearance; land checks must not silently permit water spawns.
  - [ ] **S22 — Validate physical clearance.** Reject solid overhead roofs,
    colliding bodies and unsupported movement/navigation locations per species.
  - [ ] **S23 — Add station cylinder query.** Allowlist Workbench/Forge only;
    radius 18 m, extending 12 m above/below the actual placed station.
  - [ ] **S24 — Add spherical suppression query.** Use radius 18 m for
    Camp Bed, lit fuelled Campfire and lit fuelled Standing Light. Other pieces,
    cosmetics and unlisted stations emit no field; roofs retain S22 rejection.
  - [ ] **S25 — Wire placement gate.** Combine S20–S24 with current authoritative
    station state/destruction. Suppression blocks new natural spawns only; it
    does not delete existing creatures or prevent their movement into camp.
  - [ ] **S26 — Enable natural spawning.** Route one bounded creation transaction
    through all nine gates, reserve/release caps and apply S02 lifecycle rules.
    Replace the corresponding old activation path to avoid duplicate populations;
    keep harvest nodes, stable identities and owner-bound reward claims intact.
  - [ ] **S27 — Record environmental policy.** Map the fixed local-source
    policy to the shared scheduler; camp suppression applies. Local cap is 2
    within 24 m, interval 8 s. Use two finite source slots; add no world content.
  - [ ] **S28 — Add local spawner scheduling.** Implement the S27 interval and
    local living count as a reusable server policy, disabled without a configured
    source; use a narrow fixture rather than adding world content.
  - [ ] **S29 — Apply environmental safety gates.** Combine local limits with
    shared species/total caps, player exclusion, physical placement and the
    accepted suppression policy; reserve capacity before creation.
  - [ ] **S30 — Cover spatial boundaries.** Add focused cases for ring edges,
    negative key coordinates, overlapping players, moving creatures and pack
    reservation rollback; inspect assertions against the accepted contract.
  - [ ] **S31 — Cover gate rejection.** Add one rejection case per nine-gate
    reason and unrestricted time/weather/progression cases; verify bounded work.
  - [ ] **S32 — Cover suppression and placement.** Add radius/height boundaries,
    unlisted pieces, destroyed stations, extinguished/empty lights, roof, slope
    and land/water clearance cases.
  - [ ] **S33 — Cover lifecycle and local spawners.** Add unload/reload, cooldown,
    defeat/reward deduplication and local/shared cap cases using narrow fixtures.
  - [ ] **S34 — Prepare verification handoff.** Update architecture and setup
    docs with accepted tuning, server authority, lifecycle and exact build,
    automation and host/client/restart commands required for this goal.
  - [ ] **S35 — Perform milestone-final verification.** Inspect the combined
    work; run S34's prescribed full build, automation and peer/restart checks.
    Verify no duplicate actors/rewards and bounded scheduling. Repair related
    failures and rerun required checks in this run. Record actual commands/results;
    check the parent only when verification passes, otherwise record the blocker.

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
