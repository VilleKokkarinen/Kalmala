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
- [x] Add an original basic player model, Space jumping, and held-Shift sprinting with predicted, server-authoritative movement.

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
  - [x] Replace floating biome-cut lake sheets with enclosed terrain-basin presentation and matching minimap water.
  - [x] Repair partial-cell water clipping and floating shore frames; configure a lit startup map without overlapping template terrain.
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
- [x] Verify host/client agreement and meaningful choices.
  - [x] Verify host and client observe matching weather, exposure, shelter, fire, and recovery state, and that clients cannot alter any authoritative value.
- [x] Run a two-player scenario showing freely chosen camp locations with different weather preparation tradeoffs and no built or guided path.
  - Verified automated low/high-cover camp fixtures and cold-weather fire recovery with matching host/client state; human choice usability, construction, and rain preparation remain outside this scenario.

### Phase 5 — Companion minimap

**Intent:** give each player an unobtrusive, circular, player-centred orientation aid without creating a second world simulation, revealing hidden server-owned content, or directing a route through the wilderness.

- [x] Add a top-right circular minimap through a `KalmalaUI` view model, derived from local generated-world presentation and the owning player's replicated transform.
- [x] Render terrain, water, known player-facing landmarks, and a centred owning-player facing marker without revealing hidden server-owned content or adding a biome map.
- [x] Bind mouse-wheel zoom with tunable, clamped minimum and maximum levels; modal UI must retain its own input.
- [x] Verify circular clipping, UI-scale/aspect-ratio placement, min/max zoom clamping, and host/client player-centred views with no authoritative state mutation or information leak.
- [x] Repair off-screen viewport anchoring, replace sparse dots with original biome textures, and verify actual host/client HUD painting and modal-safe bound wheel input.

### Phase 6 — Biome expansion

**Intent:** add one biome at a time as a distinct, seed-generated place to explore, prepare, and discover—not as a combat tier or a separate authored region. Each biome must use the same continuous four-field classifier and Phase 4 exposure contract. Terrain can create local environmental variation, but never designed travel corridors, shortcuts, roads, or trails.

- [x] Establish the shared biome-expansion contract: deterministic server-owned terrain, population budgets, exposure modifiers, stable discoveries, and developer seam/feature inspection.
- [x] Deliver the full Shimmering Lakes slice: interlocking water, saturated low ground, lake-edge or island discoveries, and wet-shore camp tradeoffs; no boat requirement before Phase 7.
- [x] Deliver the full Elderwood slice: field-driven canopy, shade, roots, and clearings plus an optional discovery and a compact-versus-open camp tradeoff; never create a trail.
- [x] Deliver the full Mossy Mire slice: saturated, slower traversable ground, dry hummocks, drainage/raised-shelter preparation, and an optional discovery; never require a crossing.
- [x] Deliver the full Freezing Tundra slice: sparse cover, rolling high ground, wind exposure, enclosed-shelter preparation, and an optional discovery.
- [x] Deliver the full Thunder Mountains slice: steep but traversable ridges, storm pressure, lightning-safe shelter preparation, and an optional discovery; no designed passages or precision gate.
- [x] Verify each completed biome as one integrated scenario: same-seed reproduction, different-seed variation, continuous seams and collision, stable server IDs, and matching host/client terrain, exposure, shelter, and freely chosen camps.

### Phase 7 — Coherent biome generation and hydrology

**Intent:** replace threshold-speckled biome placement with large, coherent, seed-generated regions. Biomes are sampled procedurally from the existing world identity and source heightfield; do not create serialized, replicated, or independently authoritative biome maps. Local vegetation, terrain shaping, rivers, and streams are derived from deterministic functions, with only hydrology spline data cached where required for efficient lookup.

- [x] **Establish the deterministic regional generation contract.**

  - [x] Derive all generation seeds from the server-owned `WorldSeed`, including unique biome noise offsets, biome motion/warp noise, rivers, and streams.
  - [x] Keep `Elevation`, `Humidity`, `Temperature`, and `Flora` as the authoritative continuous source fields; regional biome data must remain derived rather than stored.
  - [x] Centralize biome scale, edge distortion, overlap, and other generation constants so region size can be tuned without rewriting classifier logic.
  - [x] Preserve world-coordinate continuity, same-seed reproducibility, different-seed variation, and `GeneratorRevision` compatibility.

- [x] **Generate broad overlapping biome regions from layered noise rings.**

  - [x] Give each biome deterministic multi-layer concentric regional noise with unique seeded offsets and substantially larger scale than local terrain or vegetation noise.
  - [x] Bound regional biome viability using the source heightfield and relevant environmental fields so physically invalid placements are rejected.
  - [x] Distort the inner and outer edges of regions with continuous seeded motion noise and sine-based edge variation, producing irregular overlapping boundaries without small biome speckles.
  - [x] Resolve overlapping biome weights deterministically, keeping physical overrides explicit: submerged terrain becomes `Ocean`, mountain-height terrain becomes `ThunderMountains`, and `Meadows` remains the natural fallback.
  - [x] Use broad suitability for `Elderwood`, `MossyMire`, and `FreezingTundra`; keep high-frequency `Flora` for vegetation density, undergrowth, and clearings rather than biome identity.
  - [x] Require `ShimmeringLakes` to agree with deterministic standing-water/basin logic rather than humidity alone.

- [x] **Generate deterministic rivers and streams.**

  - [x] Generate seeded candidate points on a coarse world grid and merge nearby candidates into a stable final point set.
  - [x] Build rivers by connecting eligible points within a defined range, then generate spline points between them using deterministic wavelength, amplitude, and direction variation.
  - [x] Generate streams through the same spline system, but restrict their start and end points to land within an appropriate low-altitude range.
  - [x] Spatially index river and stream spline points by `GridCell` so nearby water-course weights can be queried without scanning the full network.
  - [x] Cache only the generated hydrology spline data; biome regions, field values, weights, and terrain shaping remain function-derived.

- [x] **Derive final terrain height from biome and hydrology weights.**

  - [x] Sample the source heightfield, resolved biome weights, and nearby river/stream weights at any world position.
  - [x] Give each biome a deterministic terrain-shaping function that derives its final height from those inputs rather than requiring a baked biome heightmap.
  - [x] Apply river and stream weights to carve or reshape the terrain continuously while preserving terrain-patch seams, collision agreement, shorelines, and water-depth queries.
  - [x] Ensure overlapping biome edges blend continuously so terrain shaping transitions naturally between neighboring regions.

- [x] **Add visualization and tuning coverage.**

  - [x] Extend `RenderWorldGenerationVisualization` to show biome-region weights, overlap boundaries, resolved biome identity, hydrology splines, and final shaped height.
  - [x] Make macro biome scale, regional frequencies, edge-wave strength, motion/warp strength, river spacing, spline amplitude, and spline wavelength developer-visible tuning values.
  - [x] Add large-area checks that make isolated biome fragments, excessive boundary density, broken rivers, and terrain-patch seams easy to identify.

- [x] **Verify Phase 7 as one integrated generation change.**

  - [x] Confirm repeated renders of the same seed are identical and different seeds produce meaningfully different biome regions and hydrology.
  - [x] Confirm `Elderwood`, `MossyMire`, `FreezingTundra`, `Meadows`, `ThunderMountains`, `ShimmeringLakes`, and `Ocean` form coherent regions appropriate to their physical constraints.
  - [x] Confirm local `Flora` creates clearings and density variation without changing broad biome identity.
  - [x] Confirm rivers and streams reproduce deterministically, remain continuous across grid and terrain-patch boundaries, and produce matching terrain deformation.
  - [x] Confirm host and client sample identical biome, terrain, and hydrology results while the server-selected `WorldSeed` and `GeneratorRevision` remain authoritative.
  - [x] Preserve the revision-1 generator for existing worlds and place the new regional generator behind a new `GeneratorRevision` where required.

### Phase 8 — Ocean and long-distance travel

- [x] Add ocean terrain, islands, and the systems required for long-distance movement.
  - [x] Establish a shared sea-depth query over the actual terrain triangles and integrate matching minimap coastlines.
  - [x] Add server-authoritative swimming entry, movement, and return to land using shared water-depth sampling; verify host/client agreement.
  - [x] Complete seed-derived island and long-distance ocean travel support within the existing profiling constraints.
- [x] Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing density or streaming distance.
- [x] Verify land-to-ocean travel has no terrain gaps, duplicate content, or host/client disagreement.

### Phase 9 - Expanded world map

**Intent:** add an original, nearly full-screen navigational map that expands the existing companion-minimap presentation without creating a second world simulation, exposing hidden server-owned content, or prescribing routes. It adopts familiar survival-map interactions—toggle, pan, zoom, personal pins, and intentional co-op sharing—without copying another game's UI, art, terminology, icons, or map data.

- [ ] **Establish the local expanded-map contract.**
  - [x] Bind `M` to open/close an almost full-screen local map overlay; Escape closes it before the Settings menu can open.
  - [x] Make opening the map pause local movement/look input and retain pointer/wheel input; closing restores the prior game-input state.
  - [x] Define continuous world-space pan, cursor-anchored wheel zoom, explicit zoom bounds, and a recenter-on-owning-player action.
  - [x] Keep one local map instance per local player, with no RPC, replicated UI state, world mutation, or gameplay authority change.
  - [x] Verify toggle/input priority, zoom clamping, panning bounds, player recentering, 4:3/16:9/ultrawide layout, and coexistence with the minimap and Settings menu.

- [ ] **Build a scalable full-map presentation from the existing generated-world contract.**
  - [x] Reuse immutable replicated world identity and local owning-pawn transform; do not sample actors, population layouts, hidden discoveries, hazards, or server-only state.
  - [x] Replace a single huge synchronous raster with bounded, cached, asynchronously generated world-space tiles and discard obsolete jobs after pan, zoom, identity, or player changes.
  - [x] Render original Kalmala terrain, ocean, inland water, and player-facing map treatment at multiple scales, preserving coastline agreement with terrain triangles.
  - [x] Establish measurable refresh/memory budgets and retain minimap responsiveness while the expanded map is open.
  - [x] Verify deterministic same-identity tiles, different-seed variation, seamless tile edges, no game-thread waits, and host/client presentation agreement.

- [ ] **Add local exploration and fog-of-war without information leaks.**
  - [x] Define a bounded owning-player reveal radius driven only by that pawn's already replicated movement; unexplored areas must not disclose terrain classification, water, landmarks, population, or discoveries.
  - [ ] Persist personal explored coverage under the immutable world identity and version it independently from generated-world/save data.
  - [ ] Distinguish personal exploration from later shared exploration visually with original Kalmala treatment.
  - [ ] Verify reconnect/restart persistence, identity mismatch rejection, reveal-edge continuity, and that a client cannot reveal remote terrain or another player's exploration through UI input.

- [ ] **Add personal map pins and player orientation.**
  - [ ] Draw a centred, facing owning-player marker and optional local coordinate/grid aids without route guidance.
  - [ ] Support an original finite pin palette, label entry with validation/length limits, click placement, click-to-toggle completion/visibility, and explicit removal.
  - [ ] Persist pins per player and world identity; never accept client pin data as a server gameplay instruction or discovery claim.
  - [ ] Add accessible non-colour-only pin states and keyboard/controller alternatives for every pointer interaction.
  - [ ] Verify map-to-world coordinate conversion at every zoom level, pin persistence, overlap selection, input focus, and no network/gameplay side effects.

- [ ] **Add opt-in co-op awareness and temporary pings.**
  - [ ] Define an owner-controlled opt-in for visible connected-player markers, using only normal replicated transforms and clear privacy/offline handling.
  - [ ] Add a short-lived, rate-limited map ping that the server validates and relays only to eligible session members; it must not reveal unexplored terrain or create a persistent waypoint.
  - [ ] Verify server rejection of malformed, distant, excessive, or unauthorized pings and matching expiry/order across host and clients.

- [ ] **Defer shared-cartography interaction until construction and persistence prerequisites exist.**
  - [ ] After the M2 construction system is available, design an original server-owned cartography interaction that explicitly exchanges opted-in explored coverage and selected shared pins.
  - [ ] Define permissions, conflict/duplicate handling, sparse storage limits, world-identity compatibility, and a no-spoiler default before implementation.
  - [ ] Verify clients cannot forge shared exploration/pins, access another session's data, or use shared map state to materialize hidden gameplay content.

- [ ] **Validate the full map as one integrated feature.**
  - [ ] Run build plus focused automation for modal input, tiled rendering, fog, pins, and pings; include rendered host/client screenshots at three aspect ratios.
  - [ ] Profile full-map open/pan/zoom memory, worker time, game-thread time, and late-join behavior before raising tile density or map range.
  - [ ] Document all map/pin/share contracts, accessibility controls, known limits, and multiplayer authority decisions.

## Later gameplay milestones

Use `docs/04-roadmap.md` as the source of truth. Add decomposed M2–M5 tasks here only after their preceding milestone acceptance criteria pass.
