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
as settled design. Costs, stats and final names remain to be authored.

**Completion for every item:** normal acquisition and usable behavior; applicable
manifest-registered item/profile/recipe/upgrade/reward/attack data; original
generated/imported icon, provenance, runtime mapping and icon validation;
server-owned payment/use/targets/quantities; documented persistence and explicit
non-applicable records. Buildables require supported hammer placement and raw
materials, never inventory kits. No retired Reed Fibre or offensive magic.

- [ ] **A0 — Existing item acquisition and use (P0).**
  - [ ] Before implementation, reconcile retained/deferred candidates with user-approved alpha scope and divide approved work into finite milestones with explicit final verification gates. New-system proposals stay blocked until their scope decisions are accepted.
  - [ ] Audit normal world-to-use reachability for every current recipe and upgrade input; record sources, blockers and bootstrap cycles. Resolve the documented Iron-source discrepancy against actual generated nodes without changing acquisition behavior in this audit.
  - [ ] Give String a repeatable normal acquisition path using an original approved source; verify a fresh player can craft and maintain Bow without debug grants.
  - [ ] Make Lightwood obtainable through a suitable existing-world source and verify Bronze-family recipes/upgrades are reachable.
  - [ ] Add a validated Bone Fragment source within the existing creature/discovery roster and verify Boar Hide Cape crafting/upgrades.
  - [ ] Reconcile the six current cooking outputs with server food-use dispatch; remove stale food-ID assumptions and verify paid cooking-to-eating behavior.
  - [ ] Complete Hearth Broth acquisition and ordinary access to the four vegetable ingredients using approved sources; document which seed/farming paths remain blocked on A7.

- [ ] **A1 — Ores, bars and fuel (P0; new processing scope gated).**
  - [ ] Decide the alpha metal-production contract: source locations, Stone-to-Bronze-to-Iron bootstrap, ore versus direct-bar rewards, Coal production, alloy station and input/output roles. Obtain user scope approval before adding processing systems; preserve existing behavior until decided.
  - [ ] Add normal Copper Ore and Tin Ore sources with server tool/tier checks (blocked on A1 contract).
  - [ ] Give Coal a repeatable production/source loop, including a direct-material charcoal station only if approved (blocked on A1 contract).
  - [ ] Implement paid Copper/Tin refining and Bronze alloying, with complete placed-station support where approved (blocked on ore/Coal acquisition and A1 contract).
  - [ ] Implement the agreed Iron Ore/Iron route and reconcile the direct-bar Mire source and documentation (blocked on A1 contract and required refining).

- [ ] **A2 — Melee weapons (P1).**
  - [ ] Add one original Bronze-tier sword with a distinct slash role, craft/upgrade data, server attack dispatch, held appearance and icon (blocked on reachable Bronze inputs).
  - [ ] Add its Iron-tier continuation using the existing material progression (blocked on sword support and reachable Iron inputs).
  - [ ] Decide whether the design-target Wooden Club adds a useful zero-metal starting choice alongside Stone Mace; implement it only if retained, otherwise document that Stone Mace covers the role.

- [ ] **A3 — Bows and ammunition (P1).**
  - [ ] Add one improved bow using reachable materials, authored draw/durability tuning, crafting/upgrades, held appearance and icon (blocked on A0 String and its chosen material sources).
  - [ ] Add one metal-tipped arrow with a complete recipe/icon and server ammunition selection, damage, consumption and full-pack checks; preserve Wooden Arrow fallback (blocked on A1 metal production).

- [ ] **A4 — Armour, capes and shields (P1).**
  - [ ] Add a Bronze Helmet with a complete craft/equip/wear/upgrade/icon path using the existing integer Armor contract (blocked on A1).
  - [ ] Add a matching Bronze Tunic through the same complete path (blocked on A1).
  - [ ] Add matching Bronze Leggings through the same complete path (blocked on A1).
  - [ ] Add one metal shield with a distinct authored passive Armor/durability role; active block/parry is not implied (blocked on A1).
  - [ ] Decide whether second-tier Boots/Cape or cold-protection clothing belongs in alpha; new insulation rules require scope approval before item implementation. Keep existing hide pieces useful.

- [ ] **A5 — Carried utility tools (P1/P2).**
  - [ ] Add the design-target carried Torch with reachable fuel/materials, validated equip/use/expiry behavior, original appearance/icon and appropriate fire/wetness integration.
  - [ ] Decide whether terrain-editing Hoe and cultivation tool roles belong in alpha; define bounded terrain/crop authority and persistence first. Implementation is blocked on approved scope and A7's farming contract.

- [ ] **A6 — Forage, meals and recovery consumables (P1; effects gated).**
  - [ ] Add one original locally appropriate berry food and its server-owned forage source, icon and food-use behavior (blocked on A0 food dispatch).
  - [ ] Add one edible mushroom and its forage source, icon and food-use behavior (blocked on A0 food dispatch).
  - [ ] Decide whether alpha retains the single Steady Meal effect or adds differentiated meal roles; verify all seven existing prepared foods before expanding recipes. New nutrition rules require scope approval.
  - [ ] Define recovery preparation effects, ingredients, crafting station, cooldowns and interaction with support magic; obtain scope approval for a new consumable system.
  - [ ] Add one healing preparation with acquisition, consumption, feedback and icon (blocked on approved recovery contract and reachable ingredients).
  - [ ] Add one stamina preparation through the same complete path (blocked on approved recovery contract and reachable ingredients).
  - [ ] Evaluate a cold/poison preparation only for an approved alpha hazard; otherwise document its deferral. Do not add unused resistance items.

- [ ] **A7 — Crops, seeds and fishing (P2; scope gated).**
  - [ ] Decide alpha farming scope for the existing Carrot/Potato/Rutabaga/Onion and seed definitions; specify seed renewal, planting, growth, harvest and restart behavior without assuming every vegetable uses identical propagation.
  - [ ] Implement one complete approved crop loop plus its cultivation tool and icons where needed (blocked on farming approval); retain the remaining crops as explicitly incomplete.
  - [ ] Complete each remaining approved root crop's acquisition/renewal loop as separate increments (blocked on first working crop); connect it to existing cooking recipes.
  - [ ] Decide whether fishing belongs in alpha; define a bounded fish source, rod/bait acquisition, catch validation and food role before adding items or creatures.
  - [ ] Add one complete rod/bait/fish-to-cooked-fish loop with original icons/appearance (blocked on approved fishing scope; split into integrated increments).

- [ ] **A8 — Buildables, processing and sanctuary supplies (P2; no kits).**
  - [ ] Decide the alpha shelter/furniture subset: door, support beam, stairs, bed, placed light and storage upgrade. Define each missing interaction, snapping/support, ownership and persistence rule before implementation.
  - [ ] Implement the approved door with direct-material placement and server interaction/save behavior (blocked on shelter decision).
  - [ ] Implement approved support beam and stair pieces as separate increments with valid placement/structure behavior (blocked on shelter decision).
  - [ ] Implement the approved bed and placed light as separate complete increments; do not infer a sleep/respawn system from a furniture definition (blocked on shelter decision).
  - [ ] Implement one approved storage upgrade with server capacity/transfer/save validation (blocked on shelter decision).
  - [ ] Add any approved recovery-production station through complete direct-material placement; no fermenter or brewing tree is required unless selected in A6 (blocked on A6 contract).

- [ ] **A9 — Travel, accessories and discovery rewards (P2; scope gated).**
  - [ ] Decide whether carry-pack progression, hauling/cart or skiff cargo is required for alpha; inventory weight, encumbrance and hull durability are separate unapproved systems.
  - [ ] Implement only the selected hauling/cargo increment with original presentation and validated transfer/persistence behavior (blocked on travel scope decision).
  - [ ] Decide whether Necklace/Ring placeholders remain deferred or gain support-only accessories; define slots/effects/authority before adding jewellery.
  - [ ] Audit the four existing scroll discovery-to-unlock/use paths; add a new support scroll or optional trophy only if an approved unmet reward role exists. Preserve non-damaging magic and the existing creature roster.

- [ ] **A10 — Approved alpha item-set acceptance.**
  - [ ] Reconcile the delivered set against the approved alpha scope and record the final item/source/recipe/use/icon checklist, including explicit deferrals and unresolved blockers.
  - [ ] At each resulting milestone's final implementation increment, run the prescribed full verification and repair its failures. At the final item-set gate, demonstrate fresh-start acquisition/crafting/upgrading/use without debug grants, catalogue/icon coverage, authoritative host/client rejection/privacy, and applicable restart behavior. Do not mark alpha complete from item counts alone.

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
