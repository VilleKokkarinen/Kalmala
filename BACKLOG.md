# Development backlog

## Active queue

M0–M14, world-generation Phases 1–9, and the defined attack-driven
tree-harvesting increment remain complete under their recorded acceptance
scope. The user-requested post-roadmap goals below are queued in order.

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
  `Scripts/Tests/World/RegionalGeneration.ps1`.
- [x] Run the minimap host/client terrain, water, and river-signal agreement
  check with `Scripts/Tests/World/Minimap.ps1`.

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

- [x] **A8b — Enemy materials and crafted items (P1/P2).** [Recipes](docs/biome-enemies.md#recommended-items-and-recipes)
  - [x] Reconcile the user-edited drop amounts/chances and recipe station/cost/output values with existing approved contracts; align source ownership and the proposal table.
  - [x] Resolve the remaining effect and full equipment-profile decisions before implementing affected outputs; see [A8b review](docs/alpha-item-details.md#a8b).
  - [x] Add the registered catalogue JSON records for all 15 materials and 15 outputs, including output recipes, equipment profiles, upgrade costs/rewards, and supported attack records.
  - Item icon tasks are independent of their source/output implementation tasks. Each checkbox covers one item's original generated artwork and provenance, prepared/imported texture, runtime mapping, and icon-validator coverage. Per the user's 2026-10-22 direction, complete all remaining A8b icon checkboxes together in one short-path A8b work batch and one feature commit. Completing icons does not complete their item rows or enable acquisition. The importer retains its supported limit of 16 textures per import batch.
  - [x] Rat Tail icon (`RatTail`): original, provenance, mapping, import, and catalogue icon validation are complete; see `docs/icon-provenance.md#rat-tail-icon`.
  - [x] Badger Fur icon (`BadgerFur`).
  - [x] Badger Meat icon (`BadgerMeat`): original, provenance, mapping, import, and focused catalogue icon validation are complete; see `docs/icon-provenance.md#badger-meat-icon`.
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
  - Grouped icon batch completed 2026-10-22 in one feature commit by user direction; originals, preparation, imports, mapping and focused coverage are recorded in `docs/icon-provenance.md#a8b-icon-batch`. All 30 source/output implementation rows are checked; combined I4 acceptance remains open.
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
  - [x] Run the combined A8a/A8b enemy and enemy-item milestone verification; record results and blockers. This gate must pass before A9 begins.
    - [x] Rebuild and rerun the sequential Badger/Moon Bear peer acceptance after the Moon Bear fixture adjustment; both cases passed. The full queue at that verification point passed all 180 tests.
    - [x] Repair the procedural snake-head `SurfaceWinding` failure and the M9 progression assertions; the focused check and full 180-test Kalmala queue at that verification point passed.
    - [x] Finish live acceptance for A8a species-specific spawn and attack/status behavior, then exercise approved A8b item progression and peer flows. `RatPeer.ps1` is the sole I4 end-to-end enemy test for loot delivery/retry, defeat persistence and combat replication; do not repeat those shared checks in other species cases. Species tests may cover their unique authored profiles, drop-table data and attack/status behavior. Prior peer passes for Boar, Deer, Badger, Moon Bear, Mireling, Viper, Giant Snake, Thunder Sentinel and Reef Maw remain historical species-specific evidence, not extra required shared-system tests. `Kalmala.Gameplay.Status.PoisonEffects` is the single shared weak/major poison and Antidote status automation. Mosquito bite/poison and dodge/fire counterplay acceptance plus Peat Husk sweep acceptance are recorded below and passed live.
      - [x] Add the Development-only generated Wolf three-member pack peer fixture and `Scripts/Tests/Combat/WolfPeer.ps1` for stable formation, 180 cm server bite, and replicated telegraph.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/WolfPeer.ps1`; confirm all three generated members, formation spacing, both client telegraphs, and accepted server lunge damage (passed 2026-10-23).
      - [x] Add the Development-only generated Lynx peer fixture and `Scripts/Tests/Combat/LynxPeer.ps1` for a seeded dry-Tundra Lynx, its 150-health profile, telegraphed leap, and peer-visible cue.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/LynxPeer.ps1`; confirm the 350 cm server leap lands and both clients observe its replicated telegraph (passed 2026-10-23).
      - [x] Add the Development-only generated Polar Bear peer fixture and `Scripts/Tests/Combat/PolarBearPeer.ps1` for the 150 m start exclusion, 750-health profile, committed charge/retreat/recovery and peer-visible cue.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/PolarBearPeer.ps1 -WorldSeed 445`; require the start exclusion, accepted server charge damage and matching telegraphs on both clients.
      - [x] Add the Development-only generated Horned Goat peer fixture and `Scripts/Tests/Combat/HornedGoatPeer.ps1` for its 200-health profile, 180 cm horn hit, approved push/stagger and peer-visible cue.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/HornedGoatPeer.ps1 -WorldSeed 445`; confirm server damage, push/stagger, and matching client telegraphs (passed 2026-10-23).
      - [x] Add the Development-only generated Thunder Sentinel peer fixture and `Scripts/Tests/Combat/ThunderSentinelPeer.ps1` for its 150 m start exclusion, 1250-health/75-Armor profile, short step, 300-Crush slam, recovery, and peer-visible attack cues.
       - [x] Build the affected Editor target and run `Scripts/Tests/Combat/ThunderSentinelPeer.ps1 -WorldSeed 445`; confirm the generated hard-enemy profile, step into slam reach, accepted damage, recovery, and matching telegraph/slam cues on both clients (passed 2026-10-23).
      - [x] Add the Development-only generated Peat Husk peer fixture and `Scripts/Tests/Combat/PeatHuskPeer.ps1` for the 150 m start exclusion, replicated telegraph, and accepted server sweep.
      - [x] Build the affected Editor target and run `Scripts/Tests/Combat/PeatHuskPeer.ps1`; confirm start distance, both client telegraphs, and server sweep damage (passed 2026-10-23).
      - [x] Run the generated Mosquito swarm peer case for repeated server bites, ten-second weak-poison application, owner-only status, and public health replication.
      - [x] Add Development-only generated-peer assertions for an owner-client accepted dodge, rejected cooldown retry, the roll's two-second repel window, fuelled held-torch protection, unequipped-torch bite resumption, and unlit/lit placed-campfire transitions.
      - [x] Build the affected target and run `Scripts/Tests/Combat/MosquitoSwarmPeer.ps1`; confirm dodge/fire counterplay plus health replication and owner-only poison before completing this child (passed 2026-10-23).
    - [x] Resolve the `InventoryReconnect.ps1` client-baseline timeout or document an approved non-applicable disposition before closing final I4 verification.

  The JSON-only child, all 30 icon children and all 30 source/output
  implementation rows are complete. Enemy rewards and dependent items are
  enabled through their server-side paths; combined live I4 acceptance passed
  in `fdda0f39` on 2026-10-24.

  Bow String is reserved for bows. Review every pending non-bow binding input in its category contract; do not use Bow String there. Moonstone Ring and Dusk Ring stay in A9. Do not duplicate them.
  A8b effects and full armor/weapon profiles are resolved in [the item
  contract](docs/alpha-item-details.md#a8b). The Reef Harpoon route is approved
  but adds no skiff damage. All 15 material-producing enemy sources and their
  registered outputs are implemented; the I4 gate above records accepted
  live peer, reward retry/restart/privacy and item-progression verification.

- [x] **A9 — gems and special gear (P2).** [Rules](docs/a9-acceptance.md#retained-a9-gameplay-contract)
  - Completion handoff: JSON/recipe/icon stages were merged from `71fa8060`; runtime accessories, all ten gem sources and special gear are now implemented on `codex/a9-runtime`. Combined I5 passed on 2026-10-24 (187 automation cases plus applicable builds, validators, peers/restart and renders). Verified runtime commit `d0ead245` is integrated into local master by the final normal merge; all A9 implementation and I5 acceptance work is complete.
  - [x] Review A9. User approved the documented source rates, mutually exclusive replacements and three-hit extraction on 2026-10-23; existing source prerequisites and I4/I5 acceptance remain required.
  - [x] Review Necklace/Ring slots; one item per slot, bonuses only while equipped. Existing validated owner-only transactions retained; focused AccessorySlots regression added.
  - [x] Apply the remaining authored accessory stats at runtime: `MaxHealthBonus` and `MaxStaminaBonus` as flat equipped bonuses, plus additive `StaminaRegenerationMultiplier` and `HealingReceivedMultiplier` deltas. Recalculate server-owned stats on equip/unequip and add focused status/healing coverage.
  - Each A9 item is completed through separate item-scoped steps in this order: add its complete item/profile JSON and register its file in `Content/Data/GameCatalogues.json` first; add and register its recipe next when crafted; generate and integrate its original icon next; add or enable its acquisition/source-table entry last when needed. Icon work includes provenance, preparation/import, runtime mapping, and validator coverage. Gem recipes are N/A; crafted gear has no drop-table entry. Each stage can be completed in its own short-path worktree and committed individually. Do not enable acquisition before its item JSON and icon are complete or its source dependency is accepted. The sequence applies only after the A9 review and I4 gate.
  - User-directed 2026-10-22 batch: use the proposal-table values for these JSON/recipe records and generated icons. The review, accessory runtime behavior, gem sources, and I4/I5 acceptance remain open.
  - [x] Smoky Quartz (`SmokyQuartz`): Quartz rock; proposed 5% replacement of Copper/Tin spawn slots.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its Quartz-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [x] Moonstone (`Moonstone`): 2% chance of 1 per credited generated Elderwood Moon Bear defeat; accepted A8a/I4 source enabled.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable the Moon Bear reward-table entry last, after item JSON/icon completion and accepted A8a source.
      - [x] Prepare validated generated-Moon-Bear selection, persisted optional gem payload and atomic full-reward delivery; icon check passed.
      - [x] Enable the prepared Moonstone source after integrated I4 acceptance; `MoonstoneSourceAccepted` is true. Runtime acceptance remains in combined I5.
  - [x] Garnet (`Garnet`): proposed 10% chance of 1 in Elderwood/Lakes world treasure chests; chest-only.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [x] Green Jade (`GreenJade`): Jade rock; proposed 2% replacement of Tin spawn slots in Shimmering Lakes.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its Jade-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [x] Amethyst (`Amethyst`): Amethyst rock; proposed 3% replacement of Iron Ore spawn slots in Mossy Mire.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its Amethyst-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [x] Aquamarine (`Aquamarine`): proposed 5% chance of 1 in Shimmering Lakes treasure chests.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [x] Tourmaline (`Tourmaline`): Tourmaline rock; proposed 2% replacement of slate spawn slots in Thunder Mountains.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its Tourmaline-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
  - [x] Star Opal (`StarOpal`, special): Tundra rock; proposed 1% replacement of Frost Salt deposit slots.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its Star-Opal-rock acquisition/source-table entry after the item JSON and icon are complete and the source is accepted.
      - User approved the bounded real-deposit pass on 2026-10-23: at most one dry, slope-safe and separated deposit per 60 m key; 1% become Star Opal, leaving frostmoss unchanged. The 2026-10-24 S03 shared-layout change supersedes only the key width to 64 m; all other source conditions remain.
  - [x] Storm Sapphire (`StormSapphire`, special): proposed 2% chance of 1 in Thunder Mountains treasure.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its eligible world-treasure source-table entry last, after item JSON/icon completion and accepted treasure support.
  - [x] Dusk Diamond (`DuskDiamond`, special): approved 1% chance of 1 from credited generated Mossy Mire Mirelings; source enabled, combined I5 verification pending.
    - [x] Add and register the complete raw-gem item JSON; record recipe and combat/equipment/upgrade categories as N/A.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
    - [x] Add or enable its Mireling reward-table entry last, after item JSON/icon completion and accepted A8a source.
      - [x] Prepare generated-Mossy-Mire Mireling validation, 1/100 selection and persistent credited reward payload; icon check passed.
      - [x] Enable the prepared Dusk Diamond source after integrated I4 acceptance; `DuskDiamondSourceAccepted` is true.
  - [x] Gemstone rocks: implement the shared rare resource-slot replacement and 10% gem-smash rule; successful extraction yields 1 gem. This shared source mechanic does not add per-item acquisition entries.
    - [x] Add shared source-selection/hit-outcome contract and validated sparse gemstone save state; no sources enabled yet.
    - [x] Connect attack-driven pickaxe hits, durable progress/outcomes and loose-gem pickup; then enable item sources in their listed order.
    - [x] Enable and inspect seeded resource replacements after per-item acquisition entries are registered; full executed verification remains I5 scope.
  - [x] Quartz Necklace (`QuartzNecklace`): finalized Forge inventory-crafting route and +10 Armor profile; focused paid exchange/equip/physical-only/no-wear coverage added.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Moonstone Ring (`MoonstoneRing`): approved Forge recipe and equipped +20 maximum stamina path are implemented, with the Moonstone source enabled.
    - [x] Complete executable Forge inventory recipe and paid craft/GUID equip coverage; Moonstone source enablement followed accepted I4.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Garnet Necklace (`GarnetNecklace`): finalized executable Forge recipe and +20 maximum health; paid craft/equip, no-refill and above-100 healing/full-health checks added.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Jade Ring (`JadeRing`): finalized executable Forge recipe and global -0.10 stamina-cost delta, including additive Wet/Meal and immediate-removal coverage.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Amethyst Necklace (`AmethystNecklace`): finalized executable Forge recipe and support-only -0.15 cost delta; ordinary/zero-cost exclusions and additive Jade/Wet/Meal coverage added.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Aquamarine Ring (`AquamarineRing`): finalized executable Forge recipe and +0.15 active-regeneration delta; sprint/full-stamina/draught exclusions and removal covered.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Tourmaline Necklace (`TourmalineNecklace`): finalized executable Forge recipe and incoming-only +0.10 healing; partial ticks, removal, full health, outgoing exclusion and no resurrection covered.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Opal Helm (`OpalHelm`): finalized Forge acquisition, session-only levels/upgrades/free repair, level-aware physical Armor/wear/condition display and original icon.
    - [x] Complete ordinary Forge crafting and paid-output/GUID Head-slot equip coverage.
    - [x] Integrate session-owned levels, Forge upgrades/free repair and level-aware wear/Armor/presentation; preserve existing gear-save limits.
    - [x] Add and register the complete item/equipment/profile JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Stormcut Sword (`StormcutSword`): finalized supported runtime sword kind, Forge acquisition/upgrades/free repair, Basic Slash and held presentation; focused paid lifecycle coverage added.
    - [x] Add and register the complete item/equipment/attack JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Dusk Ring (`DuskRing`): approved paid Forge recipe and hybrid equipped stats implemented; Dusk Diamond source enabled, combined I5 pending.
    - [x] Complete executable Forge recipe and combined flat-stat/physical-Armor craft/equip coverage; source enablement remains I4-dependent.
    - [x] Add and register the complete item/equipment/stat JSON first, with applicable N/A categories recorded.
    - [x] Add and register its Forge recipe using the values finalized by Review A9.
    - [x] Generate original icon and provenance; prepare/import it, register runtime mapping, and add catalogue icon validation.
  - [x] Audit the four existing scroll routes; defer new scrolls, trophies and boss keys. Source audit and exact routes recorded in `docs/a9-acceptance.md`; live acceptance remains I5 scope.
  - [x] Complete applicable I5 builds/automation/catalogue/icon/peer/persistence/rendered verification, repair A9 defects, and integrate enemy-source acceptance only after I4 passes.
    - [x] Execute isolated Editor/Game builds, full automation, new gemstone/treasure peer-and-restart evidence, shared crafting/discovery/inventory checks and equipped-inventory renders; record exact passes, repairs and remaining shared-check failures.
    - [x] Verify prepared enemy-gem chance bounds and owner-bound pending-payload serialization/rejection without enabling I4-gated sources.
    - [x] Repair the presentation/M5 checker invocation and stale prose/source anchors, preserving negative checks and leaving runtime inventory-baseline disposition with I4.
    - [x] Integrate the committed A8b progression snapshot a33711ec into the isolated A9 branch, preserve both handoffs, and rerun no-build contracts; I4 acceptance remains pending.
    - [x] Integrate master through `9a8cbc0f`, retaining completed I4 acceptance and the inventory-baseline fix; enemy-source activation and combined I5 remain next.
    - [x] Integrate passed I4 source acceptance, enable and verify Moonstone/Dusk Diamond rewards, and resolve the inherited inventory notification-baseline verification disposition. Canonical source and persisted payload tests, inventory and two-visit reconnect checks passed.
    - [x] Pass combined integrated I5 verification and merge A9 normally into local master at a safe clean boundary, preserving the other worker's handoff. Final runtime commit `d0ead245`; 187/187 automation cases passed, with applicable builds, validators, peers/restart and renders.

- [x] **A10 — Acceptance.** [Full criteria](docs/alpha-item-details.md#a10)
  - [x] Audit all approved item IDs against registered JSON and generated/imported runtime icons; zero missing entries, references or fallback icons.
    Audit 2026-10-24: 61 registered JSON files and 777 references resolve;
    all 152 definitions have ledger-backed runtime mappings and generated,
    prepared, imported catalogue icons. The four support scrolls use their own
    assets. `python Scripts/Validate-CatalogueReferences.py` and
    `Scripts/Validate-UIIcons.ps1 -Category Catalogue` both pass.
  - [x] Review milestones: I0 A0; I1 A1; I2 A2–A6 except Planting Trowel; I3 A7 farming + Planting Trowel; I4 A8–A8b with its final enemy gate before A9; I5 A9.
    Review 2026-10-24: historical gate evidence and remaining gaps are mapped in
    PROGRESS.md. Final A10 verification on 2026-10-25 includes the post-gate
    Planting Trowel menu, no-debug Bronze/Iron fresh-start route, shelter,
    accessory peers/restart and rendered inventory acceptance. The retired
    cargo proposal is excluded/N/A under the accepted A9 scope; it has no
    implementation or verification requirement.
  - [x] Check each included item's source, recipe, use, upgrades, icon and save behavior.
    A10 item audit 2026-10-24: Sinew, Bow String, Lightwood, Bone Fragment,
    six cooking outputs, Hearth Broth, Wild carrots (Carrot, CarrotSeed),
    Wild potatoes (Potato, PotatoSeed), Wild rutabaga (Rutabaga, RutabagaSeed),
    Wild onions (Onion, OnionSeed), retained Wood/Densewood/Stone, and
    Boar/Deer Hide and Meat (BoarHide, DeerHide, BoarMeat, DeerMeat), Amber,
    Frost Salt, CopperOre, TinOre, Coal, the Copper/Tin/Bronze bar family,
    IronOre/Iron, A2 melee weapons (`BronzeSword`, `IronSword`, `WoodenClub`),
    A3 ranged items (`LightBow`, `StoneArrow`, `BronzeArrow`), A4 wearables
    (`BoarHideBoots`, `BoarHideCape`, `BronzeHelmet`, `BronzeTunic`,
    `BronzeLeggings`, `BronzeShield`), and A5 `BasicTorch` reviewed; see
    PROGRESS.md. The two approved A1 pick acquisition costs were corrected on
    2026-10-25: BronzePick now costs Wood x10 + Bronze x10; IronPick now costs
    Wood x10 + Iron x10. Their separate pickaxe upgrade costs remain
    unchanged. The no-debug fresh-start acceptance below passed in a
    source-matched compiled mirror: the harness used normal source harvest,
    Kiln/Smelter processing, Forge recipes and source-depletion persistence;
    deterministic placed fixtures do not establish procedural-world traversal.
    A4 items, A5 `BasicTorch`, A6 foraged foods (`Lingonberry`,
    `ForestChanterelle`) and recovery draughts (`BerryRecoveryDraught`,
    `RootStaminaDraught`) are audited; no A6 family remains in scope. The
    A7 `PlantingTrowel`/`GardenPlot` contract and crop-use audits for
    `Carrot`/`CarrotSeed`, `Potato`/`PotatoSeed`, `Rutabaga`/`RutabagaSeed`, and
    `Onion`/`OnionSeed` are complete; prior A0 Carrot, Potato, Rutabaga, and
    Onion source-pair reviews were not repeated. `PotatoSeed` remains
    intentionally unused in A7. The A8 buildable audits for `TimberDoor`, `TimberBeam`, `TimberStairs`,
    `CampBed`, and `StandingLight` are recorded in PROGRESS.md. Cold/warmth
    preparations beyond the recorded Standing Light behavior remain deferred.
    No A8 buildable remains. The A8b per-item audits for Antidote, Peat Heart
    Stew, Wolf Pelt Boots, Fang Spear (`FangSpear`), Wolf Stew (`WolfStew`),
    Lynx Mantle (`LynxMantle`), Polar Bear Coat (`PolarBearCoat`), Horn Bow
    (`HornBow`), Thunder Maul (`ThunderMaul`), and Reef Harpoon (`ReefHarpoon`)
    are recorded in PROGRESS.md;
    A8b recipe/output audits and per-item A9 audits for Smoky Quartz
    (`SmokyQuartz`), Moonstone (`Moonstone`), Garnet (`Garnet`), Green Jade
    (`GreenJade`), Amethyst (`Amethyst`), Aquamarine (`Aquamarine`),
    Tourmaline (`Tourmaline`), Star Opal (`StarOpal`), Storm Sapphire
    (`StormSapphire`), Dusk Diamond (`DuskDiamond`), and Dusk Ring
    (`DuskRing`) are recorded in PROGRESS.md; all per-item A9 audits are
    complete.
    Cross-item follow-up completed 2026-10-25: Stormcut Sword now spends one
    condition for each accepted wildlife hit using the exact active server-side
    carried GUID. Condition persistence commits before target health changes;
    failed player persistence rolls condition back and rejects the hit. Focused
    authority, miss, wrong-GUID, depleted, persistence-failure, accepted-hit,
    saved-condition, and other-melee no-wear assertions passed in the final
    current-source automation queue. No other weapon's wear behavior changed.
    All approved item audits and N/A dispositions are recorded.
  - [x] Run required full verification at each milestone's final increment.
  - [x] Verify fresh-start progression through Bronze and Iron without debug grants.
  - [x] Verify approved farming, shelter, cargo, accessories, peers and restart.
  - [x] Record results/blockers; mark complete only after required gates pass.
    Final A10 verification 2026-10-25: isolated Editor and Game Development
    builds passed; the current-source Kalmala automation queue passed 209/209
    tests (eight tests emitted non-failing context-free world teardown warnings).
    Catalogue references/icons, creature catalogue and documentation contracts
    passed. The fresh-start runner earned Bronze and Iron Picks through ordinary
    server harvest, processing and Forge transactions, with 75/75 source IDs
    retained after reload. Garden Plot, camp/shelter persistence, construction
    persistence, Seed/Resume reconnect, A9Gear peer/restart and rendered
    equipped-inventory acceptance passed. Cargo is excluded/N/A under accepted
    scope. Exact commands, logs, warnings, repairs and evidence limits are in
    the 2026-10-25 A10 final-verification record in PROGRESS.md. The fixture
    progression uses deterministic placed actors and does not claim procedural
    traversal; no required A10 gate remains open.

### Biome height rework and Meadows terrain — user request, 2026-10-24

**Goal:** Replace nearly level, shared land shaping with smooth, distinct terrain
profiles for each land biome, starting with readable Meadows hills and natural
forest-floor relief. Add terrain-driven forest placement and surface presentation
while preserving the existing world identity, biome labels, climate fields,
distance bands, master-map coastline, server authority, and bounded streaming.

**Design sources:** [Biome height rework](docs/biome-height-rework.md) expands
and takes precedence over [the Meadows plan](docs/meadows-plan.md) where their
details differ. In particular, analytic enclosed lakes stay disabled throughout
this work; ocean and sea-level rivers remain. Do not add erosion, fixed camp
sites, new gameplay effects, new ground-cover assets, network fields/RPCs, or
save migrations as part of this goal.

**Implementation trial defaults selected for this plan:** Shared relief scales
start at 1,000 m / 150 m / 30 m (broad / middle / local). Meadows uses the
BH25a-selected 160 m / 4 m broad hills and BH25b1-selected 45 m / 0.6 m rolling
relief, and BH25b2-selected 1,000 m / 4 m, 150 m / 0.6 m, and 30 m / 0.1 m
shared Elderwood/Tundra relief layers. BH25c selected Meadows floor detail at
8 m / 0.1 m. Tundra forest density starts at 0.85 of comparable Elderwood
density. Remaining experiment settings are agent-selected, not prior
user-approved production values; compare low / nominal / high documented
amplitudes one layer at a time, then retain the smallest values that satisfy
measured acceptance. Do not maximize layers together or leave tuning as a later
user decision.

**Acceptance direction:** Candidate heights blend through existing continuous
biome weights; labels and ecological fields do not move. Meadows should show
connected rises and hollows across 100–250 m and smaller undulations over
10–50 m walks. Ordinary walking slopes should mostly stay below 15°, with
occasional 15–25° shoulders and gentler connected routes. Every biome blend,
coast, patch edge, river, rendered/collision surface, map, forest descriptor,
and gameplay harvest identity must remain deterministic and agree. No analytic
basin water, elevated lake sheet, or lake-only wetness may return. Preserve the
existing bounded generation, map, collision, and decoration budgets.

**Run instructions:** Complete the first unchecked, unblocked child below,
exactly one per run; split a child first if it will exceed one turn. Keep
intermediate generation safe, use narrow checks during ordinary increments, and
reserve the full Editor, world, water, forest/harvest, traversal, and peer matrix
for BH26. Update `BACKLOG.md` and `PROGRESS.md`, commit only run-owned changes,
and stop after the increment. BH26 is the final integrated verification/repair
run for this goal and may exceed one turn.

- [x] **Implement biome height rework and Meadows terrain dressing.** BH01–BH26
  completed 2026-10-26; final acceptance is recorded in `PROGRESS.md`.
  - [x] **BH01 — Capture fixed-seed baselines.** Keep this parent open until
    BH01a–BH01c are complete.
    - [x] **BH01a — Export seed overviews and choose sites.** Render full-world
      seed 418 and 419 diagnostic overviews with the existing visualization
      commandlet. Inspect the outputs, record their configuration and hashes,
      and select representative coordinates plus fixed camera poses for the
      requested biomes, transitions, coast, and high-land river.
    - [x] **BH01b — Add quantitative sampler diagnostics.** Keep open until
      BH01b1–BH01b2 are complete.
      - [x] **BH01b1 — Export scalar sample data.** Add an optional CSV-driven,
        output-only commandlet path for final sampled height, biome identity and
        seven weights, water level, and water presence at BH01a's seed 418/419
        coordinates. Keep ordinary map exports and generation behavior intact.
      - [x] **BH01b2 — Add collision slope and forest counts.** Keep open until
        BH01b2a–BH01b2b are complete.
        - [x] **BH01b2a — Add mesh and forest output fields.** Match the exact
          piecewise terrain-triangle surface height/slope on the runtime
          start-relative collision grid, and count deterministic forest
          descriptors in each sample's containing patch. Do not change generation.
        - [x] **BH01b2b — Produce and validate fixed-site output.** Run the
          diagnostic for all BH01a coordinates and both seeds; validate every
          scalar, mesh, water, weight, and forest-count row and retain the output
          for BH01c. No terrain behavior changes.
    - [x] **BH01c — Capture rendered views and consolidate baselines.** Keep
      this parent open until BH01c1–BH01c2 are complete.
      - [x] **BH01c1 — Add and smoke-test actual runtime capture.** Add a
        development-only one-shot fixed-camera path and runner; build the mirror
        and capture a nonblank live-render seed-418 Starter Meadows view at its
        recorded pose. Null-RHI maps do not count as rendered captures.
      - [x] **BH01c2 — Capture and consolidate all baselines.** Keep open
        until BH01c2a–BH01c2d are complete.
        - [x] **BH01c2a — Preserve numeric baseline and capture first seven
          seed-418 views.** Add the validated 28-row numeric CSV under
          Scripts/Tests and capture/validate the first seven fixed sites for
          seed 418, recording image hashes and camera poses.
        - [x] **BH01c2b — Capture remaining seed-418 views.** Capture and
          validate the other seven fixed seed-418 sites; record hashes and poses.
        - [x] **BH01c2c — Capture first seven seed-419 views.** Capture and
          validate the first seven fixed sites for seed 419; record hashes and
          poses.
        - [x] **BH01c2d — Complete seed-419 views and manifest.** Capture the
          final seven views and verify all 28 image/numeric rows, range summary,
          camera poses and vertical bounds; distinguish live captures from
          null-RHI overviews.
  - [x] **BH02 — Map integration points.** Recheck the current height, lake,
    river, map/wetness, forest-layout, harvest, preview, material, and test
    consumers; record the exact source/test touchpoints in the plan docs.
  - [x] **BH03 — Disable analytic basin generation.** Add one compiled shared
    policy, default disabled, and bypass basin bowl/rim shaping, water-level
    assignment, and positive basin weights without changing biome identity.
  - [x] **BH04 — Prevent basin resurrection.** Apply the policy to lake samplers,
    shoreline/water meshes, maps, and gameplay wetness; invalidate derived basin
    caches when policy changes. Retain reusable disabled-feature tests.
  - [x] **BH05 — Protect ocean and rivers.** Add focused controls proving ocean,
    source-coast sign, sea-level rivers, swimming/wetness, and water/map
    classification still agree while basin-only water stays absent.
  - [x] **BH06 — Extract biome height functions.** Add named candidate functions
    for Meadows, Elderwood, Shimmering Lakes, Mire, Tundra, and Mountains with
    output-preserving defaults and focused sampler coverage.
  - [x] **BH07 — Add validated preview controls.** Name and bound shared relief
    and biome controls in the regional tuning/preview path; keep production
    output unchanged until the corresponding shaping increment is integrated.
  - [x] **BH08 — Add the shared relief stack.** Sample broad, middle, and local
    noise in absolute world coordinates with stable independent salts; keep
    climate/ecological Elevation separate and preserve deterministic cache and
    patch-edge behavior.
  - [x] **BH09 — Blend candidate heights continuously.** Use normalized existing
    land-biome weights and continuous coast attenuation; avoid hard label
    switches, double-applied detail, and changes to classifier inputs/bands.
  - [x] **BH10 — Shape Meadows hills.** Hill source behavior, sweep harness,
    and fresh-source 4 m amplitude selection are complete; rendered acceptance
    remains in BH26.
    - [x] **BH10a — Integrate the hill layer and shoulder curve.** Add bounded,
      deterministic broad Meadows hills with smooth upper-height compression;
      retain the lower source shape and continuous biome/coast blend.
    - [x] **BH10b1 — Add a reproducible Meadows hill profile sweep harness.**
      Add fixed seed-418/419 100–250 m transects and a validated commandlet-only
      hill-amplitude override to the numeric final-height/collision-slope
      sampler; preserve default sampling when the override is absent. Provide a
      runner that exports distinct 4/6/8 m trial CSVs and records the effective
      trial so a stale binary fails visibly. Do not select final tuning here.
  - [x] **BH11 — Add Meadows rolling relief.** Implementation, sweep harness and
    fresh numeric 0.6 m amplitude selection are complete; rendered connected-
    route acceptance remains in BH26.
    - [x] **BH11a — Integrate the rolling layer.** Add the independently seeded
      30–70 m Meadows noise to its candidate before shoulder compression; retain
      continuous biome/coast blending and preserve lower pockets.
    - [x] **BH11b — Add rolling profile acceptance harness.** Add fixed-seed
      30–70 m height/slope transects, amplitude trials and patch-seam controls;
      retain confirmed Z=0 river controls separately from dry-land metrics,
      reject stale binaries, and defer numeric selection to BH25b1.
  - [x] **BH12 — Add Meadows floor detail.** The integrated layer and
    reproducible profile sweep harness are complete. BH25c selected the
    measured 8 m / 0.1 m default; rendered under-vegetation acceptance remains BH26.
    - [x] **BH12a — Integrate local Meadows floor detail.** Add the separately
      seeded local signal at the BH25c-selected 8 m / 0.1 m settings, within the
      documented 8–20 m / 0.1–0.35 m bounds, before shoulder compression.
      Preserve continuous biome/coast blending, broad hills and rolling relief.
    - [x] **BH12b — Add floor-detail profile harness.** Add fixed-seed local
      profiles, lattice/aliasing metrics and validated commandlet-only scale /
      amplitude overrides. Sweep 8/12/20 m by 0.1/0.2/0.3 m and reject stale
      binaries; numeric selection is in BH25c and rendered acceptance is BH26.
  - [x] **BH13 — Shape Elderwood.** The shared uncompressed candidate and its
    profile sweep harness are complete. BH25b2 selected measured
    broad/middle/local amplitudes of 4 / 0.6 / 0.1 m within trial ranges of
    4–16 / 0.6–4 / 0.1–1 m; rendered profile acceptance remains BH26.
    - [x] **BH13a — Integrate shared Elderwood relief.** Blend the shared
      absolute-world stack plus bounded detail into Elderwood without applying
      Meadows shoulder compression. Make Tundra call the Elderwood function
      directly; preserve classifier inputs, distance bands and coast blending.
      Add source-level parity, bounds, negative-coordinate and transition tests.
    - [x] **BH13b — Add Elderwood/Tundra profile sweep harness.** Provide fixed
      Elderwood and Tundra profile samples plus validated commandlet-only
      broad/middle/local amplitude overrides. Reject stale binaries; BH25b2
      selected measured defaults and rendered profile acceptance remains BH26.
  - [x] **BH14 — Shape Shimmering Lakes.** Blend Meadows-like rounded shoulders,
    Elderwood relief, and compressed low pockets continuously, with no basin
    bowl or new perched water surface.
    - [x] **BH14a — Integrate the Lakes terrain blend.** Mix existing Meadows
      and Elderwood candidates, and add a smooth humidity/base-height lowland
      compression independent of basin support. Add focused formula, weight-
      continuity and disabled-basin tests; keep coefficients provisional.
    - [x] **BH14b — Add the Lakes numeric profile harness.** Provide fixed
      Lakes profiles and bounded commandlet-only overrides for blend share and
      lowland controls. Reject stale binaries; leave measurement and numeric
      selection to BH25d and rendered acceptance to BH26.
  - [x] **BH15 — Shape Mossy Mire.** The source integration and stale-safe
    numeric profile harness are complete with provisional bounded controls.
    Numeric selection remains BH25e and rendered acceptance remains BH26.
    - [x] **BH15a — Integrate the monotonic lowland candidate.** Soft-compress
      shared source height toward a bounded lowland band with clipped residual
      detail. Preserve positive source-land clearance outside river corridors,
      continuous biome blending, river-last priority and disabled-basin behavior.
      Add focused formula, deterministic blend, river override and dry former-
      basin tests; keep all coefficients provisional.
    - [x] **BH15b — Add the Mire numeric profile harness.** Add fixed seed-418/419
      Mire transects, bounded commandlet-only controls and per-run effective
      tuning logs that reject stale binaries. Export raw heights/slopes and
      candidate diagnostics; do not select values here. Leave fresh-source
      trials to BH25e and rendered acceptance to BH26.
  - [x] **BH16 — Shape Freezing Tundra.** Reuse the Elderwood height function
    inside the existing outer eligibility band; confirm profile parity and
    keep snow as presentation rather than a height offset. Source-level
    parity coverage is in BH13a; numeric profile confirmation was measured in BH25b2,
    snow presentation remains BH24, and rendered acceptance remains BH26.
  - [x] **BH17 — Shape Thunder Mountains.** `ShapeMountains` now amplifies only
    source height above a 1,000 cm threshold, ramps gain smoothly over 1,000 cm,
    and scales the added gain by the continuous Mountain region field. Fine
    detail is clamped to ±1 at a provisional 80 cm amplitude; validated preview
    ranges bound the threshold, blend distance, gain and detail. Focused source
    coverage checks sub-threshold preservation, bounded detail, threshold
    continuity, monotonic foothills and the fixed Tundra/Mountain transition.
    Rendered acceptance remains BH26.
  - [x] **BH18 — Reapply priority river carving.** Confirmed the existing
    channel runs after biome candidate relief and source-coast shaping. Focused
    high-ground and two-bank regressions compare the final sample with its
    post-relief no-course baseline; profile assertions preserve Z=0 water, the
    80 cm bed and 1,800 cm bank extent. Coast controls and seam coverage protect
    shared consumers. BH25b2 later found and repaired a relief-caused bank
    water regression at a fixed source-land control; rendered acceptance
    remains BH26.
  - [x] **BH19 — Add independent forest factor.** Shared forest descriptors now
    carry a normalized absolute-world noise sample from a separate seed domain
    at a provisional 48,000 cm scale. It is independent of terrain detail and
    Flora and consumes no candidate-stream draws. Candidate counts, positions,
    ordinals and IDs remain unchanged; suitability gating remains BH20.
  - [x] **BH20 — Integrate Meadows forest suitability.** Use the shared descriptor
    path for decorative and harvestable trees; gate by Flora, forest factor,
    final slope, and dry-ground suitability while preserving collision and
    harvest lifecycle identity.
    - [x] **BH20a — Apply Meadows Flora suitability.** The shared layout now
      uses the existing Flora probability curve for Meadow candidates with a
      stable patch/ordinal roll after existing candidate draws. Both decorative
      and harvestable paths see the same filtering; later draws, original
      ordinals and IDs are preserved.
    - [x] **BH20b — Apply forest-factor patchiness.** Meadow admission
      probability now multiplies the existing Flora probability by the
      independent absolute-world factor and compares a stable world/patch/
      original-ordinal threshold. Low-factor areas become clearings; candidate
      limits, patch continuity and Elderwood behavior remain unchanged.
    - [x] **BH20c — Gate on final terrain slope.** Meadows descriptors now pass
      the shared final-surface normal-Z floor of 0.88 before admission. The gate
      affects both decorative and harvestable trees while retaining final-height
      roots, Elderwood behavior and stable candidate identity.
    - [x] **BH20d — Gate on dry-ground suitability.** Meadows descriptors now
      reject final regional water and wet/invalid ocean samples on the explicit
      patch collision lattice. The final regional sample also supplies root
      height; original ordinals, IDs, basin policy and lifecycle consumers stay
      unchanged. Focused helper and identity coverage was added; full integrated
      acceptance remains in BH26.
  - [x] **BH21 — Add Tundra forest participation.** Reuse the forest factor
    with reduced density; keep resource identity explicit and consistent across
    rendering, harvesting, suppression, and persistence. BH21a–BH21c are complete.
    - [x] **BH21a — Make forest tree kind explicit.** Shared descriptors now
      use explicit Meadows birch, Meadows aspen, Elderwood ironheart and
      Freezing Tundra birch kinds. Existing renderer and harvest consumers use
      the kind contract; Tundra birch maps to the existing Meadows birch source
      and presentation. Current placements, appearance, sources and random
      draws are unchanged.
    - [x] **BH21b — Add reduced-density Tundra forest candidates.** Existing
      Tundra-classified candidates use the Flora curve, absolute-world factor
      and stable patch/ordinal threshold with provisional 0.85 density. They
      use the explicit Tundra Birch kind and existing Birch source/presentation.
      Tundra appearance draws use a separate patch/candidate stream, preserving
      the Meadow/Elderwood candidate stream, counts and IDs; BH25 finalizes the
      multiplier and BH21c proves lifecycle behavior.
    - [x] **BH21c — Verify Tundra forest identity and lifecycle.** Focused
      Tundra Birch fixtures now trace the shared descriptor kind and persistent
      candidate ID through server materialization, original-ordinal cosmetic
      suppression, sparse depletion and patch reconstruction. The process
      restart fixture now persists and reloads Tundra identities. Late-join
      delivery uses the same descriptors and checks source, ID, suppression,
      replicated transform and stage; integrated peer acceptance remains BH26.
  - [x] **BH22 — Add eligible Lakes forest shoulders.** Shared forest placement
    now admits only Shimmering Lakes-classified Birch candidates, reuses the
    existing 18-ordinal lowland cap and resource/presentation, and applies the
    stable Flora/forest-factor roll. Final slope and regional/collision-lattice
    water checks restrict placement to suitable dry shoulders without changing
    candidate positions, existing identities or the shared descriptor path.
  - [x] **BH23 — Add terrain-driven ground presentation.** Slope/moisture
    vertex weights now drive the grass/soil/rock terrain material blend, and
    decorative Meadow rocks fit the final surface. Material package generation
    and rendered acceptance remain in BH26. No cosmetic reeds are planned
    without a matching original visual asset.
    - [x] **BH23a — Fit decorative Meadow rocks to final terrain.** Keep the
      existing 48 deterministic patch candidates, positions, dimensions and
      random draws; place rock bases at the shared final height and align them
      to the final sampled surface normal. Rocks remain cosmetic and
      collision-free.
    - [x] **BH23b — Blend terrain ground materials by slope and moisture.**
      Terrain vertex colors carry normalized grass, soil and rock weights from
      final surface slope, world humidity and final regional water state. The
      existing material generator now updates `M_GeneratedTerrain` to blend the
      established grass and rock colors with a new original muted
      `M_GeneratedSoil` material; it adds no grass or reed asset. The Editor-
      generated material packages and rendered result remain for BH26.
  - [x] **BH24 — Add cold-biome presentation.** BH24a–BH24c are complete;
    rendered acceptance remains BH26.
    - [x] **BH24a — Add Tundra snow coverage.** Use continuous Tundra biome
      weight and the final terrain slope in the snow mask of the existing
      generated terrain material. Preserve RGB grass/soil/rock weights and the
      existing player-cloth palette; do not change terrain height, weather
      authority or gameplay behavior.
    - [x] **BH24b — Add Mountain snow coverage.** Extend the existing snow
      mask with continuous Mountain weight, final terrain height and slope;
      snow fades in above the named highland threshold. Preserve foothill
      height, Tundra coverage, RGB ground weights and player-cloth palette.
    - [x] **BH24c — Add Mountain storm presentation.** Fade a client-local cool
      fog visual from replicated storm intensity multiplied by the continuous
      Mountain weight at the local player; do not add exposure, damage, shelter
      mechanics, RPCs or replicated fields.
  - [x] **BH25 — Finalize tuning and contracts.** Fresh-source numeric
    selections and cost contracts BH25a–BH25e are complete; rendered
    acceptance remains in BH26.
    - [x] **BH25a — Run BH10b1 Meadows hill sweep.** Fresh-source 4/6/8 m
      trials were compared with all 28 BH01 fixed sites and eight 100–250 m
      transects; 4 m was selected and measured ranges/slopes were recorded.
    - [x] **BH25b — Finalize rolling/Elderwood relief tuning and contracts.**
      BH25b1–BH25b3 are complete: selected numeric defaults, raw profile ranges,
      fixed-site comparisons and cost/budget evidence are recorded. Integrated
      generation/collision timing and rendered acceptance remain in BH26.
      - [x] **BH25b1 — Select Meadows rolling relief.** Run fresh-source
        0.6/1.2/1.8 m trials and all 28 BH01 fixed sites, select the smallest
        accepted amplitude and record 30/45/70 m ranges, slopes, river controls
        and patch-seam results. The 0.6 m setting is selected.
      - [x] **BH25b2 — Select shared Elderwood/Tundra relief.** Ran fresh-source
        low/nominal/high broad, middle and local amplitudes one layer at a time,
        exercised each trial bound, compared BH01 profiles/slopes, confirmed
        exact Tundra candidate parity, and selected the smallest accepted
        measured defaults: 4 / 0.6 / 0.1 m. The sweep caught and repaired a
        relief-caused wet river-bank sample and the fixed 1 cm coast shelf;
        numeric measurements and remaining render limits are recorded in the
        biome-height plan.
      - [x] **BH25b3 — Aggregate terrain tuning and cost contracts.** Updated
        architecture/setup and height-preview documentation with raw ranges;
        compared current sampler/map timings and fixed patch, active-streaming,
        collision and decoration budgets with BH01 evidence. No fixed mesh/map
        payload or candidate-loop budget grew; unavailable BH01 generation and
        collision timings remain explicitly assigned to BH26. BH25b is closed.
    - [x] **BH25c — Run Meadows floor-detail trials.** Freshly rebuilt the
      current-source Editor and ran all nine 8/12/20 m by 0.1/0.2/0.3 m trials.
      The smallest numerically resolved setting, 8 m / 0.1 m, sampled at 6.4
      points per 125 cm lattice interval; its maximum adjacent floor input step
      was 2.765 cm, fixed-site water/biome state matched current source, and
      maximum final/collision seam deltas were 0.026 / 0.028 cm. Raw profiles
      and comparisons are in `docs/meadows-plan.md`. Rendered readability and
      under-vegetation acceptance remain BH26.
    - [x] **BH25d — Run Shimmering Lakes profile trials.** The current-source
      BH14b mirror ran all 12 one-control trials against the fixed 44 samples.
      The measured center settings remain selected: Meadow share 0.50, floor
      200 cm, ceiling 1,200 cm and compression 0.20. All trials preserved
      water/course state, biome labels and weights, with zero basin weight; the
      nominal output's 44-sample nearest-rank slope p50/p90/p95/max was
      0.907/13.286/18.073/23.543 degrees. Raw profile ranges, monotonic checks,
      and BH01 center comparisons are recorded in
      `docs/biome-height-rework.md`. Rendered
      shoreline/traversal acceptance remains BH26.
    - [x] **BH25e — Run Mossy Mire profile trials.** The source-matched BH15b
      mirror ran all nine one-control trials over 252 samples. Retained the
      measured nominal settings: 300 cm target, 700 cm compression scale and
      25 cm residual detail. All variants preserved the nominal 50 wet samples,
      water coverage and active river/basin fields; none added wet terrain. The
      minimum positive source-land clearance was 19.538 cm, and the lowland
      curve passed 180,000 monotonic comparisons across every target/scale
      pairing. Raw ranges and BH01 comparisons are in
      `docs/biome-height-rework.md`; rendered acceptance remains BH26.
  - [x] **BH26 — Run final integrated verification and repair.** The Editor
    build, world and water automation, RegionalGeneration/Minimap peers, forest
    lifecycle/restart, material generation, and rendered traversal acceptance
    passed 2026-10-26. Results and remaining limits are in `PROGRESS.md`.

### Monster population and camp spawn suppression — user request, 2026-10-22

Planning only, queued after existing alpha acceptance; A8a/I4 scope is unchanged.
Use original content. The supplied Valheim examples are inspiration, not a
verified specification. [Fixed targets and behavior](docs/monster-spawning.md)
are specified in advance; implement them without requesting tuning values.

**Run instructions:** Complete the first unchecked, unblocked child below,
exactly one per run, in order. Each normal checkbox targets roughly 10–20 minutes
of integrated work plus a lightweight check. If it will exceed one turn, split
that checkbox into smaller ordered children before implementing its first part.
Keep intermediate code safe: S26 is the first increment that enables natural
spawning. Record
files, checks, remaining scope and next task in `PROGRESS.md`; commit only the
run's changes, then stop. Add focused tests with the helper they exercise where
practical; execute full verification only at the final gate. S35 is the required
milestone-final verification/repair run and can exceed a normal turn.

- [x] **Implement Kalmala monster population and camp spawn suppression.**

  - [x] **S01 — Map the fixed contract to code.** Read `docs/monster-spawning.md`;
    identify existing scheduler, descriptor, budget and spawn/defeat entry points.
    Record the implementation map in that contract. Values are already decided;
    do not reopen tuning or change runtime behavior in this step.
  - [x] **S02 — Map lifecycle integration.** Identify callbacks enforcing permanent
    defeated-slot depletion, 30 s living-slot unload cooldown and reward escrow.
    Record integration points for the fixed lifecycle; add no save-format change.
  - [x] **S03 — Add named tuning constants.** Use existing invisible 64 × 64 m
    spatial keys; ring 32–96 m, interval 5 s and count radius 96 m.
    Validate bounds/units; keep spawning disabled.
  - [x] **S04 — Gather active players.** Add a server-only list of living players
    in the current world; ignore dead, disconnected and invalid pawns.
  - [x] **S05 — Deduplicate active keys.** Derive the union of nearby loaded keys
    from S04; overlapping players must produce each key once.
  - [x] **S06 — Add scheduler cadence.** Schedule each active key once per 5 s
    with stable staggering; remove inactive keys without accumulating retries.
  - [x] **S07 — Bound scheduler work.** Use a 0.25 s wake, at most 4 keys,
    4 candidates per key and 16 candidates total per wake; skip missed ticks
    instead of catch-up bursts.
  - [x] **S08 — Generate ring candidates.** Select existing stable descriptors
    within 32–96 m; use the contract's seeded 25% admission roll and rotating cursor.
    Preserve descriptor positions, identities and biome weights.
  - [x] **S09 — Exclude nearby players.** Reject a candidate within 36 m of any
    living player, including a second player outside the selecting neighborhood.
  - [x] **S10 — Track live creatures.** Register/unregister authoritative live
    actors; remove defeated/destroyed entries and count their current positions.
  - [x] **S11 — Enforce area caps.** Count across loaded keys within 96 m of the
    candidate; per-species caps: common 4, rare hard enemy 1, Wolves 6.
    Also enforce the existing total actor budget; add no extra actor allowance.
  - [x] **S12 — Reserve pack capacity.** Reserve three Wolf slots atomically;
    reject partial packs and release reservations on failed creation.
  - [x] **S13 — Add the nine-gate evaluator.** Return a distinct rejection reason
    for scheduler budget, seeded roll, player distance, habitat, time, weather,
    progression, caps and physical placement. Unimplemented gates reject safely.
  - [x] **S14 — Wire scheduler and roll gates.** Feed S06–S08 into the evaluator;
    one rejected attempt consumes only its bounded attempt budget.
  - [x] **S15 — Wire distance and habitat gates.** Use S09 and existing species
    biome/dry-shore/start-exclusion rules; do not relax existing restrictions.
  - [x] **S16 — Add time-of-day gate.** Pass for every current species;
    expose only an internal policy seam for rejection fixtures. No new clock.
  - [x] **S17 — Add weather gate.** Pass for every current species;
    expose an internal rejection-test seam. Add no new weather restrictions.
  - [x] **S18 — Add progression gate.** Pass for every current species;
    expose an internal rejection-test seam. Add no boss or progression locks.
  - [x] **S19 — Wire cap gate.** Apply S11–S12 immediately before creation so
    overlapping key/player attempts cannot overfill a species or total budget.
  - [x] **S20 — Validate land surfaces.** Require generated dry ground, existing
    start restrictions and slope at most 32 degrees for land creatures.
  - [x] **S21 — Validate aquatic surfaces.** Reuse Reef Maw habitat/depth rules
    and check water clearance; land checks must not silently permit water spawns.
  - [x] **S22 — Validate physical clearance.** Reject solid overhead roofs,
    colliding bodies and unsupported movement/navigation locations per species.
    - [x] **S22a — Check body collision clearance.** Reuse each wildlife
      archetype's configured collision shapes and reject out-of-bounds or
      colliding body placements; do not change descriptor identity or enable
      natural spawning.
    - [x] **S22b — Reject solid construction roofs.** Trace upward 24 m from
      the body top and classify only active solid RoofKit construction hits.
      Ignore foliage/canopy and non-solid cosmetics.
    - [x] **S22c — Preserve movement support.** Apply each species' existing
      support/traversability query; do not require navigation mesh for land
      species whose contract only requires generated-ground support and clear
      collision.
  - [x] **S23 — Add station cylinder query.** Allowlist Workbench/Forge only;
    radius 18 m, extending 12 m above/below the actual placed station.
  - [x] **S24 — Add spherical suppression query.** Use radius 18 m for
    Camp Bed, lit fuelled Campfire and lit fuelled Standing Light. Other pieces,
    cosmetics and unlisted stations emit no field; roofs retain S22 rejection.
  - [x] **S25 — Wire placement gate.** Combine S20–S24 with current authoritative
    station state/destruction. Suppression blocks new natural spawns only; it
    does not delete existing creatures or prevent their movement into camp.
  - [x] **S26 — Enable natural spawning.** Route one bounded creation transaction
    through all nine gates, reserve/release caps and apply S02 lifecycle rules.
    Replace the corresponding old activation path to avoid duplicate populations;
    keep harvest nodes, stable identities and owner-bound reward claims intact.
  - [x] **S27 — Record environmental policy.** Map the fixed local-source
    policy to the shared scheduler; camp suppression applies. Local cap is 2
    within 24 m, interval 8 s. Use two finite source slots; add no world content.
  - [x] **S28 — Add local spawner scheduling.** Implement the S27 interval and
    local living count as a reusable server policy, disabled without a configured
    source; use a narrow fixture rather than adding world content.
  - [x] **S29 — Apply environmental safety gates.** Combine local limits with
    shared species/total caps, player exclusion, physical placement and the
    accepted suppression policy; reserve capacity before creation.
  - [x] **S30 — Cover spatial boundaries.** Add focused cases for ring edges,
    negative key coordinates, overlapping players, moving creatures and pack
    reservation rollback; inspect assertions against the accepted contract.
  - [x] **S31 — Cover gate rejection.** Add one rejection case per nine-gate
    reason and unrestricted time/weather/progression cases; verify bounded work.
  - [x] **S32 — Cover suppression and placement.** Add radius/height boundaries,
    unlisted pieces, destroyed stations, extinguished/empty lights, roof, slope
    and land/water clearance cases.
  - [x] **S33 — Cover lifecycle and local spawners.** Add unload/reload, cooldown,
    defeat/reward deduplication and local/shared cap cases using narrow fixtures.
  - [x] **S34 — Prepare verification handoff.** Update architecture and setup
    docs with accepted tuning, server authority, lifecycle and exact build,
    automation and host/client/restart commands required for this goal.
  - [x] **S35 — Perform milestone-final verification.** Inspect the combined
    work; run S34's prescribed full build, automation and peer/restart checks.
    Verify no duplicate actors/rewards and bounded scheduling. Repair related
    failures and rerun required checks in this run. Record actual commands/results;
    check the parent only when verification passes, otherwise record the blocker.

## User-requested fixes

- [x] Rename the camp exposure/recovery runner to ExposureRecoveryPeers.ps1 and update callers and references.

- [x] Consolidate enemy peer launchers into one behavior-scenario runner, retain one shared reward/restart gate and distinct mechanics, and move discovery/mining tests into Progression.

- [x] Rename Scripts/Tests runners by behavior, removing Verify and milestone prefixes and updating callers and documentation references.

- [x] Simplify Scripts/Tests into shared behavior coverage: consolidate item peer runners, remove repeated per-item runs and document test selection.

- [x] Consolidate redundant item icon addition documents into one provenance reference and shared workflow, preserving artwork and updating active links.

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

### Valheim-inspired Kalmala lighting and materials — user request, 2026-10-25

**Goal:** Build an original Kalmala lighting/material stack with readable
day/night scenes, atmospheric distance, weather-aware surfaces, and optional
hardware ray-tracing quality levels. The detailed, researched implementation
contract and acceptance scenes are in the [lighting plan](docs/valheim-like-lighting-plan.md).

**Planning status:** This adds no runtime behavior and makes no release-hardware
promise. Keep the accepted Kalmala art direction, client-only cosmetic behavior,
and existing weather/gameplay authority. Hardware ray tracing and DLSS remain
optional presentation features; the RT-off tier must retain the complete visual
baseline. Default Lit remains the production shading path unless a later visual
review accepts a different look.

**Worktree sequence:** L01–L06 are ordered, dependent goals. Assign each goal
one agent in a dedicated worktree based on integrated `master` after the preceding
lighting goal has been integrated. Independent Kalmala runs may proceed concurrently
in distinct worktrees under `AGENTS.md`; this lighting sequence remains ordered
because later stages depend on earlier lighting outputs.

**Run rule:** In the first unchecked goal, complete exactly its first unchecked
child in order; split it before coding if it exceeds one turn. Run applicable
lightweight checks, update backlog and progress, commit only run-owned changes,
and stop. L06 is the integrated milestone-final verification and repair goal;
do not mark the lighting milestone complete before it passes.

- [ ] **L01 — Establish the lighting baseline and core post-process look.**
  Record repeatable scene captures, stabilize the existing light/sky/fog/exposure
  stack, and use built-in post-process volume controls before custom passes.
  - [x] **L01a — Inspect the active prototype map and renderer.** Record the
    active RHI, GI/reflection method, directional/sky lights, fog, exposure,
    post-process volumes, generated material assignments, and capture settings
    in the existing lighting plan. Select and record fixed camera positions for
    all five baseline scenes. Do not change project behavior in this audit.
    The live map/renderer audit and five fixed camera anchors are recorded in
    `docs/valheim-like-lighting-plan.md`; L01b reviewed the daylight, forest,
    and water images, while L01c will capture the weather and fire conditions.
  - [x] **L01b — Capture seed-418 daylight, forest, and water.** Use the fixed
    camera poses for Meadows daylight, forest shade, and coast/water. Preserve
    captures and manifest under `TestResults/LightingBaseline/`; record
    resolution, exposure, hardware, and renderer settings. Do not write under
    `Saved/`, `Intermediate/`, or `DerivedDataCache/`.
  - [ ] **L01c — Capture seed-418 weather and fire scenes.** Capture
    overcast/rain and fire-lit dusk/night using L01a's matching camera and
    exposure records; verify all five seed-418 scenes are present.
  - [ ] **L01d — Capture seed-419 daylight, forest, and water.** Reuse L01a's
    fixed views and the seed-418 manifest for Meadows daylight, forest shade,
    and coast/water.
  - [ ] **L01e — Capture seed-419 weather and fire scenes.** Capture the
    matching overcast/rain and fire-lit dusk/night views; verify the full
    two-seed baseline set before changing the rig.
  - [ ] **L01f — Tune the scene-wide rig and grade.** Keep solid surfaces on
    Default Lit. Stabilize exposure, balance the existing directional key and
    sky fill, tune existing fog, then apply a restrained global grade and bloom
    using built-in settings. Save the selected values with before/after captures.
  - [ ] **L01g — Add only justified local post-process volumes.** Use bounded
    volumes only for an existing authored location needing a distinct look;
    set explicit priority/blend radius and confirm no visible boundary or UI/text
    treatment. If no such location exists, record this child N/A in the plan.
  - [ ] **L01h — Pass the core-lighting gate.** Compare all ten fixed views and
    ordinary third-person movement; confirm readable silhouettes, stable
    exposure, legible firelight, and no crushed shadows or clipped highlights.

- [ ] **L02 — Build and integrate the shared surface-material family.** Replace
  flat generated surface materials through project-owned parents and instances,
  retaining direct light, shadow, indirect fill, water clipping, and runtime
  material assignment behavior.
  - [ ] **L02a — Define the shared material interface.** Use the existing
    `Content/Kalmala/World/Materials/` path and the lighting plan's bounded
    controls: base tint, roughness, normal strength, broad color variation, and
    mapping scale. Keep common surface functions in Material Functions.
  - [ ] **L02b — Integrate terrain and shore.** Replace flat generated terrain
    and shore expressions with instances of the shared lit parent; verify
    procedural assignment and biome-debug material override still work.
  - [ ] **L02c — Integrate rock.** Assign the opaque lit parent to generated
    rocks and verify visible shape, roughness, and baseline material separation.
  - [ ] **L02d — Integrate wood and bark.** Assign appropriate instances to
    generated trunks and authored wood without creating material graphs at runtime.
  - [ ] **L02e — Add the foliage family.** Use a separate masked/two-sided canopy
    parent where required; inspect edge aliasing and temporal shimmer in forest
    shade and daylight.
  - [ ] **L02f — Integrate water separately.** Use a dedicated water parent or
    instance; preserve sea/lake clipping and shoreline geometry, and inspect
    reflections/highlights in the fixed coast and water views.
  - [ ] **L02g — Pass the material-family gate.** Verify terrain, shore, rock,
    bark, canopy, and water retain distinct values across all baseline scenes;
    record material assignments and remaining limitations in the lighting plan.

- [ ] **L03 — Prototype the stylized direct-light response.** Keep Default Lit
  as the shipped default while measuring a restrained banded alternative; this
  goal produces evidence and a recommendation, not an unreviewed global style
  change.
  - [ ] **L03a — Create an isolated comparison fixture.** Compare Default Lit
    with a soft two- or three-band direct-light treatment on representative
    ground, rock, wood, canopy, creature, and construction surfaces using L01's
    fixed views.
  - [ ] **L03b — Evaluate UE 5.8 Substrate NPR support.** Prototype its
    experimental ramp-based shading only if the project's configured renderer,
    build targets, and materials support it; do not add an engine fork or make
    the experimental feature a production dependency.
  - [ ] **L03c — Check lighting integration.** Confirm each candidate preserves
    cast shadows, indirect fill, camp-light response, silhouette readability,
    and temporal stability; record per-view GPU cost.
  - [ ] **L03d — Record the review recommendation.** Keep Default Lit as the
    production path and report the captures, tradeoffs, and recommended next
    choice. Any change to the game's overall visual identity requires explicit
    user review before broad adoption.

- [ ] **L04 — Add supported weather and environment effects.** Use only existing
  client presentation signals and effects; keep this cosmetic, gradual, and
  outside gameplay, save, RPC, and replicated-state contracts.
  - [ ] **L04a — Map existing weather inputs.** Trace currently available
    client-side precipitation, wetness, snow/biome, sky, fog, and world-space
    effect inputs. Record which plan effects have valid inputs; do not add a new
    weather type or network/save field to supply a missing one.
  - [ ] **L04b — Add subtle wet-surface response.** Apply rain/wet darkening and
    roughness changes to the plan-selected wood/rock materials using only an
    existing presentation signal; smooth state changes to avoid popping.
  - [ ] **L04c — Add eligible snow coverage.** Apply it only to suitable
    surfaces with an existing client snow/biome signal. Mark unsupported
    combinations N/A rather than inventing a signal.
  - [ ] **L04d — Add eligible moss/lichen variation.** Apply it only to suitable
    surfaces with an existing client biome/surface signal; record unsupported
    combinations as N/A in the lighting plan.
  - [ ] **L04e — Tune existing sky and atmospheric effects.** Adjust available
    cloud, local fog, and weather color treatments through the existing pipeline;
    add no new sky model or weather type.
  - [ ] **L04f — Tune existing world-space weather effects.** Adjust available
    rain, snow, mist, or ember particles and keep them out of key sight lines.
    Add no full-screen droplets or overlays.
  - [ ] **L04g — Pass the environment-effects gate.** Review daylight, rain,
    overcast, and fire-lit captures and transitions; confirm no popping,
    excessive visual noise, or gameplay/simulation change.

- [ ] **L05 — Add scalable hardware ray-tracing quality levels.** Use the RT-off
  baseline, Lumen hardware ray tracing for supported Windows DX12/SM6 devices,
  and the measured levels in the lighting plan. Do not claim a minimum GPU until
  representative hardware has passed.
  - [ ] **L05a — Verify the actual renderer support path.** Check project RHI,
    SM6, Lumen and hardware-ray-tracing settings; record the exact supported
    device/OS conditions and keep the current `RTX 5090` performance record
    labelled as the reference system, not a minimum-spec test.
  - [ ] **L05b — Preserve the RT-off compatibility path.** Confirm the baseline
    lighting works without hardware RT, using supported Lumen software tracing
    or the project's selected non-hardware fallback. Make the fallback explicit
    and test that unsupported adapters do not enter the hardware path.
  - [ ] **L05c — Implement the Balanced tier.** On supported adapters, enable
    Lumen hardware tracing for GI/reflections using Surface Cache lighting;
    compare against RT-off and keep Hit Lighting disabled for this tier.
  - [ ] **L05d — Implement the High tier selectively.** Scale GI/reflection
    quality and test ray-traced shadows only on key lights with visible benefit.
    Set ray-tracing culling/visibility for generated forest, grass, sky, and
    repeated meshes; do not enable expensive features indiscriminately.
  - [ ] **L05e — Measure supported hardware.** Compare RT-off, Balanced, and
    High on the reference PC and the actual lower-tier devices selected for
    release; include AMD hardware if it is within the support target. Record
    frame time, resolution, memory, image quality, and artifacts. Do not infer
    minimum/recommended hardware from UE's compatibility list alone.
  - [ ] **L05f — Keep upscaling separate.** Record NVIDIA DLSS as optional and
    separate from ray tracing. Add no vendor plugin/dependency or Frame
    Generation path in this goal; seek a separate user-approved plan if they
    become a release requirement.

- [ ] **L06 — Integrate and perform lighting milestone-final verification.**
  This is the only full verification gate for the lighting milestone; complete
  it after L01–L05 are integrated.
  - [ ] **L06a — Inspect the combined implementation.** Review the merged
    material, lighting, environment, and RT changes; confirm there are no
    duplicate material assignments, unintended asset changes, or new gameplay,
    authority, save, or network behavior.
  - [ ] **L06b — Run the project-prescribed build and focused checks.** Use the
    applicable Editor build, rendered-world checks, and performance procedures
    in `docs/07-development-setup.md`. Record actual commands and results.
  - [ ] **L06c — Capture integrated visual acceptance.** Repeat the ten L01
    fixed views with the final material/effect stack for RT-off, Balanced, and
    High where supported; inspect regular third-person camera movement and UI.
  - [ ] **L06d — Repair lighting regressions.** Fix milestone-related build,
    rendering, temporal stability, fallback, or performance-check failures, then
    rerun affected checks and the required full verification.
  - [ ] **L06e — Finalize the handoff.** Record tested hardware, per-tier GPU
    costs, accepted captures, remaining limitations, and the actual release
    support statement in the lighting plan and `PROGRESS.md`. Mark L06 and the
    lighting milestone complete only after required verification passes.

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
