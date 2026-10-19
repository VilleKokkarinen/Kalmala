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

P0 = existing gaps; P1 = item variety; P2 = new systems requiring approval.
One child task per run. Every item needs complete data, a real source and an
original integrated icon. Buildables use Hammer + raw materials, never kits.
Agents must follow reviewed values and report gaps instead of inventing designs.

- [ ] **A0 — Existing items (P0).** [Details](docs/alpha-item-details.md#a0)
  - [ ] Review scope and the A10 milestone plan.
  - [ ] Audit current sources and starter progression; check the Iron-source mismatch.
  - [ ] Sinew: deer have a 50% chance to drop 1.
  - [ ] Bow String: Workbench, 4 Sinew → 1 Bow String; bows use 1 each.
  - [ ] Lightwood: Meadow aspens, Stone Axe+, 4 per collected tree bundle.
  - [ ] Bone Fragment: add 1 to each credited boar/deer defeat.
  - [ ] Fix food use for the six current cooking outputs.
  - [ ] Hearth Broth: Cauldron, 1 Deer Meat + 1 Onion → 1 broth.
  - [ ] Wild carrots: Shimmering Lakes, hand-pick for 2 Carrot Seed per patch.
  - [ ] Wild potatoes: Elderwood, hand-pick 1 Potato per patch; no Potato Seed.
  - [ ] Wild rutabaga: Mossy Mire, hand-pick 1 Rutabaga.
  - [ ] Wild onions: Freezing Tundra, found in treasure chests at a stack size randomly of 1-10.

  Preserve existing Wood, Densewood, Stone, hides/meat, Amber and Frost Salt sources.
  New vegetable seeds become plantable in A7.

- [ ] **A1 — Metals and fuel (P0).** [Details](docs/alpha-item-details.md#a1)
  - [ ] Review processing rules and Stone → Bronze → Iron progression.
  - [ ] Copper: Elderwood rocks; Tin: Shimmering Lakes shore gravel. Stone Pick+, 3 ore/node.
  - [ ] Charcoal Kiln: Hammer, 20 Stone; 1 Wood → 1 Coal in 30 seconds.
  - [ ] Smelter: Hammer, 20 Stone + 20 Wood; 1 ore + 1 Coal → 1 bar in 30 seconds.
    Forge alloy: 2 Copper + 1 Tin → 1 Bronze.
  - [ ] Mire bog iron: Bronze Pick+, 3 Iron Ore/node; replace direct-bar drops.

  Kiln: 10 input/10 output. Smelter: 10 ore/10 Coal/10 output. No offline processing.

- [ ] **A2 — Melee weapons (P1).** [Stats and upgrades](docs/alpha-item-details.md#a2)
  - [ ] Review the sword/club proposal.
  - [ ] Bronze Sword: Forge, 10 Bronze + 2 Lightwood + 2 Deer hide
  - [ ] Iron Sword: Forge, 20 Iron + 2 Densewood + 2 Deer hide

- [ ] **A3 — Bows and arrows (P1).** [Stats and upgrades](docs/alpha-item-details.md#a3)
  - [ ] Light Bow: Workbench, 25 Lightwood + 1 Bow String + 5 Deer Hide; 2 s draw.
  - [ ] Stone Arrow: Workbench, 20 Wood + 5 stone → 20; 25 Pierce.
  - [ ] Bronze Arrow: Workbench, 20 Wood + 1 Bronze bar → 20; 50 Pierce.
  - [ ] Support both arrow types; prefer selected stack, first stack in inventory. (top row of inventory left to right, going down 1 row at a time until arrows found)

- [ ] **A4 — Armour and shields (P1).** [Stats and upgrades](docs/alpha-item-details.md#a4)
  - [ ] Review armour; retain hide Boots/Cape and defer insulation.
  - [ ] Bronze Helmet: Forge, 10 Bronze + 5 Boar Hide; 3 Armor.
  - [ ] Bronze Tunic: Forge, 15 Bronze + 5 Boar Hide; 5 Armor.
  - [ ] Bronze Leggings: Forge, 15 Bronze + 5 Boar Hide; 5 Armor.
  - [ ] Bronze Shield: Forge, 10 Bronze + 5 Lightwood + 5 Boar Hide; 10 Armor.

- [ ] **A5 — Utility tools (P1/P2).** [Rules](docs/alpha-item-details.md#a5)
  - [ ] Basic Torch: inventory, 2 Wood + 1 amber; 300 s burn, 6 m light.
  - [ ] Cultivation Rake: Forge, 10 Wood + 5 Bronze bar; requires A7.

- [ ] **A6 — Food and recovery (P1).** [Effects and limits](docs/alpha-item-details.md#a6)
  - [ ] Lingonberry: hand-pick Meadow shrubs, 3 per shrub.
  - [ ] Forest Chanterelle: hand-pick Elderwood patches, 2 per patch.
  - [ ] Keep one Steady Meal: 120 s, 10% less stamina use; no meal stacking.
  - [ ] Review draught effects; use existing Cauldron, no brewing station.
  - [ ] Berry Recovery Draught: 3 Lingonberry + 1 Carrot; heals 20 over 10 s.
  - [ ] Root Stamina Draught: 2 Potato + 1 Onion; restores 20 stamina.
  - [ ] Keep cold/poison/fire-resistance preparations deferred.

  Draughts: one dose per craft; shared 60 s cooldown. All cooking needs a lit hearth.

- [ ] **A7 — Farming and fishing (P2).** [Growth and catch rules](docs/alpha-item-details.md#a7)
  - [ ] Review farming. Garden Plot: Cultivation Rake, cultivation menu addition.
  - [ ] Carrot: 1 seed → 3 carrots; 1 carrot → 3 seeds.
  - [ ] Potato: 1 potato → 3 potatoes; Potato Seed stays unused.
  - [ ] Rutabaga: 1 seed → 3 roots; 1 root → 3 seeds.
  - [ ] Onion: 1 seed → 3 onions; 1 onion → 3 seeds.

  Crops take 600 active seconds; Cultivation Rake required. No offline/inactive-region growth.

- [ ] **A8 — Shelter and storage (P2).** [Placement and behavior](docs/alpha-item-details.md#a8)
  - [ ] Review shelter rules. All pieces use direct-material Hammer placement.
  - [ ] Timber Door: 4 Wood + 1 String; 1 × 2 m, saved open/closed state.
  - [ ] Timber Beam: 2 Wood; 2 m, horizontal/vertical support.
  - [ ] Timber Stairs: 6 Wood; 2 m run, 1 m rise, 1 m wide.
  - [ ] Camp Bed: 6 Wood + 2 Boar Hide + 2 String; resting pose only.
  - [ ] Standing Light: 3 Wood + 1 amber; 1 fuel = 600 s, 6 m light.
  - [ ] Keep extra brewing stations deferred; Kiln/Smelter/Plot belong to A1/A7.

- [ ] **A8a — Biome enemies (before A9).** [Roster and stats](docs/biome-enemies.md)
  - [ ] Review the expanded roster, spawn weights and attack/loot rules.
  - [ ] Restrict boar/deer spawns to Meadows, Elderwood and Shimmering Lakes only.
  - [ ] Meadows: Rat — small defensive scavenger.
  - [ ] Elderwood: Badger — easier territorial enemy; short bite and retreat.
  - [ ] Elderwood: Moon Bear — rare hard enemy; future Moonstone source.
  - [ ] Shimmering Lakes: Viper — shoreline ambusher; lunging bite applies weak poison for 5 s.
  - [ ] Shimmering Lakes: Giant Snake — rare hard shoreline ambusher; powerful lunging bite.
  - [ ] Mossy Mire: Mireling — weak base enemy; spawn only in this biome.
  - [ ] Mossy Mire: Death Adder — highly venomous snake; bite applies major poison for 10 s.
  - [ ] Mossy Mire: Mosquito swarm — small bites + weak poison for 10 s; roll to shake off, lit fire repels.
  - [ ] Mossy Mire: Peat Husk — rare hard sweeping attacker.
  - [ ] Freezing Tundra: Wolf — tougher pursuing predator; powerful lunging bite.
  - [ ] Freezing Tundra: Lynx — leaping predator.
  - [ ] Freezing Tundra: Polar Bear — rare hard charging predator.
  - [ ] Thunder Mountains: Horned Goat — territorial horn charges.
  - [ ] Thunder Mountains: Thunder Sentinel — rare hard stone attacker.
  - [ ] Run enemy milestone verification before A9; record results and blockers.

- [ ] **A9 — Cargo, gems and special gear (P2).** [Rules](docs/alpha-item-details.md#a9)
  - [ ] Review skiff cargo; defer carts, carry packs, encumbrance and hull durability.
  - [ ] Skiff Cargo Chest: Hammer, 6 Lightwood + 2 String + 1 Bronze; 12 cells/skiff.
  - [ ] Gemstone rocks: rare resource-slot replacements; 10% gem-smash risk per hit, success yields 1 gem.
  - [ ] Quartz rock: Elderwood/Lakes, proposed 5% replacement of Copper/Tin spawn slots.
  - [ ] Moonstone: proposed 2% chance of 1 per credited Moon Bear defeat; requires A8a.
  - [ ] Garnet: proposed 10% chance of 1 in Elderwood/Lakes world treasure chests; chest-only.
  - [ ] Jade rock: Shimmering Lakes, proposed 2% replacement of Tin spawn slots.
  - [ ] Amethyst rock: Mossy Mire, proposed 3% replacement of Iron Ore spawn slots.
  - [ ] Aquamarine: proposed 5% chance of 1 in Shimmering Lakes treasure chests.
  - [ ] Tourmaline rock: Thunder Mountains, proposed 2% replacement of slate spawn slots.
  - [ ] Star Opal rock (special): Tundra, proposed 1% replacement of Frost Salt deposit slots.
  - [ ] Storm Sapphire (special): proposed 2% chance of 1 in Thunder Mountains treasure.
  - [ ] Dusk Diamond (special): proposed 0.5% chance of 1 from Mossy Mire Mirelings.
  - [ ] Review Necklace/Ring slots; one item per slot, bonuses only while equipped.
  - [ ] Quartz Necklace: Forge, 5 Iron + 1 Smoky Quartz; +10 Armor.
  - [ ] Moonstone Ring: Forge, 5 Bronze + 1 Moonstone; +20 maximum stamina.
  - [ ] Garnet Necklace: Forge, 5 Bronze + 1 Garnet; +20 maximum health.
  - [ ] Jade Ring: Forge, 5 Bronze + 1 Green Jade; sprinting costs 10% less stamina.
  - [ ] Amethyst Necklace: Forge, 5 Bronze + 1 Amethyst; casting spells cost 20% less stamina.
  - [ ] Aquamarine Ring: Forge, 5 Bronze + 1 Aquamarine; +20% stamina regeneration rate.
  - [ ] Tourmaline Necklace: Forge, 5 Iron + 1 Tourmaline; +10% damage dealt.
  - [ ] Star Opal Helmet: Forge, 20 Iron + 5 Deer Hide + 1 Star Opal; 20 Armor + 10% Damage reduction.
  - [ ] Stormcut Sword: Forge, 20 Iron + 2 Densewood + 2 Deer Hide + 1 Storm Sapphire; 100 Slash + 20 lightning damage.
  - [ ] Dusk Ring: Forge, 5 Iron + 1 Dusk Diamond; +10 max health, +10 max stamina, +5 Armor.
  - [ ] Audit the four existing scroll routes; defer new scrolls, trophies and boss keys.

- [ ] **A10 — Acceptance.** [Full criteria](docs/alpha-item-details.md#a10)
  - [ ] Review milestones: I0 A0; I1 A1; I2 A2–A6 except Rake; I3 A7 + Rake; I4 A8–A9.
  - [ ] Check each included item's source, recipe, use, upgrades, icon and save behavior.
  - [ ] Run required full verification at each milestone's final increment.
  - [ ] Verify fresh-start progression through Bronze and Iron without debug grants.
  - [ ] Verify approved farming/fishing, shelter, cargo, accessories, peers and restart.
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
