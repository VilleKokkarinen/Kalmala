# Autonomous development backlog

This file is the execution queue for Codex automations. Keep items small, ordered, and independently verifiable. Only start work in the earliest milestone whose acceptance criteria are not yet met.

## M0 — Bootstrap

- [x] Verify that `KalmalaEditor Win64 Development` builds with the command in `docs/07-development-setup.md`.
- [x] Open the project in Unreal Editor and create the prototype map at `/Game/Kalmala/Maps/Prototype/L_Prototype`; document the result.
- [x] Configure and verify a packaged development build launches after the prototype map exists.
- [x] Confirm whether a server-capable Unreal 5.8 build is available; otherwise document the dedicated-server build blocker.

## M1 — Networked traversal and interaction

Do not begin until M0 acceptance criteria in `docs/04-roadmap.md` are met.

- [x] Implement the smallest server-authoritative replicated character and camera setup needed for the two-player prototype map.
- [x] Add a server-validated interaction trace and an interactable interface.
- [x] Add a two-player test map flow and verify invalid client interactions are rejected.

## World generation track

Start this track only after M1 passes. `docs/08-world-generation-and-biomes.md` is the authoritative contract: the world is seed-generated from continuous `Elevation`, `Humidity`, `Temperature`, and `Flora` maps; do not introduce authored gameplay regions, pre-built roads or trails, or additional biome maps.

### Phase 1 — Seed and map proof

- [x] Define the immutable server-owned `WorldSeed` and `GeneratorRevision` world-generation contract.
- [x] Implement deterministic sub-seed derivation for `Elevation`, `Humidity`, `Temperature`, and `Flora`.
- [x] Implement continuous Perlin sampling and normalization for all four maps at a world position.
- [x] Implement deterministic biome classification and continuous transition blending from the four sampled values.
- [x] Add a developer-only visualization of the four fields and final biome classification.
  - [x] Add the deterministic `RenderWorldGenerationVisualization` editor commandlet and committed field/biome previews.
- [x] Verify same-seed reproducibility and visible different-seed variation in host/client play.
  - [x] Replicate the server-selected world identity through `GameState` and verify a conflicting-seed client receives it.
  - [x] Verify field and biome previews reproduce for the same seed and vary for a different seed.

### Phase 2 — First playable generated world

- [x] Generate traversable terrain from the seed, including a seed-generated player start, Meadows, lakes, trees, and rocks.
  - [x] Resolve and spawn a deterministic Meadow-preferred player start on the server.
  - [x] Add one continuous Elevation-derived terrain height and normal contract for spawns, rendering, and collision.
  - [x] Generate a 24×24-cell continuous terrain mesh with server-authoritative collision and matching client prediction collision.
  - [x] Replace the earlier coarse collision tiles with collision from the continuous mesh itself.
  - [x] Render collision-free seed-derived sea-level surface water over submerged terrain cells and verify world-scale coverage.
  - [x] Add local deterministic, non-interactable Meadow rock instances.
  - [x] Add local deterministic, non-interactable Meadow tree trunk and canopy instances.
  - [x] Activate an initial server-owned 3×3 terrain-patch neighborhood around the generated start.
  - [x] Add bounded, deduplicated server-side patch activation around connected players.
  - [x] Add distinct Shimmering Lakes water and shoreline treatment beyond sea-level coverage.
  - [x] Replace temporary engine primitive meshes/materials with original terrain, rock, tree, and water assets.
    - [x] Create and apply project-owned generated terrain, water, and lake-shore materials.
    - [x] Create and apply project-owned rock and tree meshes/materials.
- [x] Verify host and client traverse matching generated terrain and observe the same meaningful natural features.
  - [x] Verify a conflicting-seed client receives the server identity and builds all nine initial terrain surfaces without movement-base warnings.
  - [x] Run an actual two-player traversal test across matching terrain, water, rocks, and trees.

### Phase 3 — Natural population

- [x] Add deterministic server-side spatial seeds and spawn budgets for wildlife, harvest nodes, and hazards.
  - [x] Define deterministic invisible spatial keys, per-kind seeds, and field-informed spawn budgets.
  - [x] Derive deterministic terrain-aligned spawn descriptors within each bounded spatial budget.
  - [x] Activate bounded server-owned population markers around players from those descriptors.
  - [x] Replace deterministic harvest markers with server-owned, one-use harvest nodes.
  - [x] Verify generated harvest nodes reject client, distant, and depleted requests.
  - [x] Assign a stable spatial spawn identifier to each generated harvest node.
- [x] Persist consumed or defeated gameplay content as sparse world deltas keyed by seed, generator revision, and server spatial key.
  - [x] Define a versioned sparse server save container for harvest depletion keyed to immutable world identity and stable spawn IDs.
  - [x] Define sparse defeated-state storage for deterministic wildlife and hazard spawn IDs before those actor types are activated.
  - [x] Apply server-validated defeated deltas when the first generated wildlife or hazard actor is activated.
  - [x] Apply server-validated defeated deltas when generated hazards replace their placeholder markers.
  - [x] Verify a defeated generated wildlife or hazard remains absent after a listen-server restart.
  - [x] Apply in-session harvest depletion deltas before server activation recreates a generated harvest node.
  - [x] Verify sparse harvest depletion deltas round-trip through `SaveGame` memory serialization without writing generated project files.
  - [x] Persist and reload server harvest-depletion deltas through the local/listen-server save slot only when world identity matches.
- [x] Verify the same seed produces matching gameplay content and that state remains consistent after reconnecting.

### Phase 4 — Weather, shelter, and survival

**Intent:** turn the generated wilderness into a legible preparation loop: weather and terrain create exposure; natural cover, player-built shelter, and fires provide counterplay. Use M2's server-owned campfire, construction, and inventory primitives rather than parallel systems. Do not create pre-built roads, trails, safe corridors, mandatory camp sites, or a guided direction of travel.

- [x] Establish the server-authoritative environmental exposure contract.
  - [x] Define the per-pawn state and update rules for ambient temperature, precipitation, wind exposure, wetness, warmth, and shelter.
  - [x] Derive terrain-dependent inputs from the continuous world fields, local terrain, and server weather; clients may display state but never determine it.
  - [x] Add a developer-only inspection view for the sampled inputs, resulting exposure state, and active mitigation.
- [x] Add a small replicated server weather cycle.
  - [x] Define deterministic weather-state selection, duration, rain intensity, wind direction, and wind strength.
  - [x] Make exposed ridges, low wet ground, shorelines, and natural cover produce different exposure without turning biomes into hard zones.
- [x] Connect player counterplay to the existing survival-camp systems.
  - [x] Make natural cover and player-built roof/windbreak geometry contribute shelter; no authored shelter volumes.
  - [x] Make a lit, server-owned campfire add warmth and interact correctly with rain, wind, and wet materials.
- [x] Add recoverable survival consequences that create choices rather than a hard travel gate.
  - [x] Let prolonged exposure reduce warmth and apply a clear, reversible travel or stamina penalty; shelter, a fire, and preparation must offer viable recovery.
- [x] Ensure generated terrain offers varied local conditions for freely chosen camps, with understandable differences in cover, ground wetness, distance, and resources.
- [ ] Verify host/client agreement and meaningful choices.
  - [ ] Verify host and client observe matching weather, exposure, shelter, fire, and recovery state, and that clients cannot alter any authoritative value.
- [ ] Run a two-player scenario showing freely chosen camp locations with different weather preparation tradeoffs and no built or guided path.

### Phase 5 — Biome expansion

**Intent:** add one biome at a time as a distinct, seed-generated place to explore, prepare, and discover—not as a combat tier or a separate authored region. Each biome must use the same continuous four-field classifier and Phase 4 exposure contract. Terrain can create local environmental variation, but never designed travel corridors, shortcuts, roads, or trails.

- [ ] Establish the shared biome-expansion delivery contract.
- [ ] Define the deterministic, server-owned rules for each biome's terrain features, interactive population budgets, exposure modifiers, and stable discovery identifiers.
- [ ] Require each new biome to have a visual silhouette, a travel pressure, a shelter response, and an optional discovery payoff that cannot be mistaken for another biome.
- [ ] Add developer inspection coverage for continuous seam sampling, feature placement, exposure inputs, and biome-specific population budgets.
- [ ] Expand Shimmering Lakes beyond its existing visual water and shoreline treatment.
- [ ] Add interlocking lakes, saturated low ground, and seed-generated lake-edge or island discoveries without requiring a boat before Phase 6.
- [ ] Make wet shore conditions, dry storage, raised shelter, and available natural cover create understandable camp tradeoffs without guiding travel.
- [ ] Add Elderwood as a dense-canopy biome.
- [ ] Add field-driven dense vegetation, shade, roots, and clearings that change visibility and camp footprint without creating a trail or directed path.
- [ ] Add optional ancient-root, wildlife-den, or overgrown-stone discoveries and make compact camps versus open clearings a legible shelter choice.
- [ ] Add Mossy Mire as a wet-ground biome.
- [ ] Add saturated terrain, slower traversable ground, and naturally generated dry hummocks; never create a required crossing or directional corridor.
- [ ] Make raised floors, drainage, waterproof fuel storage, and bog-iron or scavenging discoveries meaningful camp-preparation choices.
- [ ] Add Freezing Tundra as a cold, wind-exposed biome.
- [ ] Add sparse cover, rolling high ground, and terrain-derived wind exposure that rewards enclosed roofs, windbreaks, and warmth preparation.
- [ ] Add optional ice-fed springs, exposed shrines, or weather-read discoveries that players may approach from any freely chosen direction.
- [ ] Add Thunder Mountains as a high-exposure biome.
- [ ] Add steep but traversable ridges, exposed passes, and storm pressure without mandatory precision traversal or designed passages.
- [ ] Make durable, lightning-safe shelter and open-ended expedition preparation meaningful; add optional storm-carved overlooks, mineral seams, or cave discoveries.
- [ ] Verify every completed biome is independently playable and blends into its neighbours.
- [ ] For each biome, verify same-seed reproducibility, different-seed variation, continuous seam samples, collision/traversal continuity, and stable server-owned discovery/population identifiers.
- [ ] Run a host/client scenario in each biome that demonstrates matching terrain, weather/exposure, shelter response, and freely chosen camp locations; clients must not alter gameplay state.

### Phase 6 — Ocean and long-distance travel

- [ ] Add ocean terrain, islands, and the systems required for long-distance movement.
- [ ] Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing density or streaming distance.
- [ ] Verify land-to-ocean travel has no terrain gaps, duplicate content, or host/client disagreement.

## Later gameplay milestones

Use `docs/04-roadmap.md` as the source of truth. Add decomposed M2–M5 tasks here only after their preceding milestone acceptance criteria pass.

### M5 — Companion minimap

- [ ] Add a top-right circular minimap through a `KalmalaUI` view model, derived from local generated-world presentation and the owning player's replicated transform.
- [ ] Render terrain, water, known player-facing landmarks, and a centred owning-player facing marker without revealing hidden server-owned content or adding a biome map.
- [ ] Bind mouse-wheel zoom with tunable, clamped minimum and maximum levels; modal UI must retain its own input.
- [ ] Verify circular clipping, UI-scale/aspect-ratio placement, min/max zoom clamping, and host/client player-centred views with no authoritative state mutation or information leak.
