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

### Phase 7 — Ocean and long-distance travel

- [ ] Add ocean terrain, islands, and the systems required for long-distance movement.
  - [x] Establish a shared sea-depth query over the actual terrain triangles and integrate matching minimap coastlines.
  - [x] Add server-authoritative swimming entry, movement, and return to land using shared water-depth sampling; verify host/client agreement.
  - [ ] Complete seed-derived island and long-distance ocean travel support within the existing profiling constraints.
- [ ] Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing density or streaming distance.
- [ ] Verify land-to-ocean travel has no terrain gaps, duplicate content, or host/client disagreement.

**### Phase 8 — Macro-biome coherence and regional shaping**

**Intent:** replace the current threshold-speckled biome distribution with broad, coherent, Valheim-like natural regions while preserving the authoritative four-field world-generation contract. Biomes must remain deterministic, seed-derived, continuous, and free of authored gameplay regions, roads, trails, or hand-placed corridors. Do not add serialized, replicated, or independently authoritative biome maps. Any macro-region signal must be derived deterministically from the existing world seed and the existing `Elevation`, `Humidity`, `Temperature`, and `Flora` generation contract.

- [ ] Establish a macro-scale biome-region sampling contract that prevents small isolated biome dots.
  - [ ] Keep `Elevation`, `Humidity`, `Temperature`, and `Flora` as the only authoritative continuous world fields.
  - [ ] Add deterministic low-frequency regional samples used only by biome classification. Derive their seeds from the existing world-generation seed contract rather than introducing authored or saved biome regions.
  - [ ] Use substantially lower frequencies for biome-scale regional variation than for local terrain or vegetation detail so biome regions span large world areas instead of repeatedly crossing thresholds over short distances.
  - [ ] Add a single tunable biome-scale control, or equivalent clearly named constants, so the physical size of regional biome features can be adjusted without retuning every classifier threshold.
  - [ ] Preserve same-seed reproducibility, different-seed variation, world-coordinate continuity, and generator-revision compatibility.
- [ ] Separate biome identity from local vegetation density.
  - [ ] Stop using the high-frequency `Flora` value as the direct deciding threshold for whether a sample belongs to `Elderwood`.
  - [ ] Determine whether an area is part of a broad Elderwood-capable region using a low-frequency regional signal plus environmental suitability such as humidity and terrain.
  - [ ] Keep `Flora` as a local-detail field used after biome selection to vary tree density, undergrowth, clearings, shrubs, and other vegetation inside the chosen biome.
  - [ ] Verify that a clearing inside a large Elderwood remains classified as Elderwood instead of becoming a small Meadows island solely because local flora density dropped.
- [ ] Refactor biome classification into ordered physical constraints plus regional suitability.
  - [ ] Keep physical terrain overrides first: submerged terrain remains `Ocean`, and sufficiently high terrain remains `ThunderMountains`.
  - [ ] Gate `FreezingTundra` by broad cold-region suitability, temperature, and upland or mountain influence so tundra forms coherent high-country regions rather than isolated cold pixels.
  - [ ] Gate `MossyMire` by a broad wetland-region signal plus low elevation, humidity, and minimum temperature so mire appears as connected lowland regions.
  - [ ] Gate `Elderwood` by a broad forest-region signal plus moisture suitability rather than local `Flora` threshold crossings.
  - [ ] Leave `Meadows` as the natural fallback biome where no stronger region and physical rule wins.
  - [ ] Keep classifier priority explicit and deterministic so overlapping viable regions always resolve identically for the same world position and seed.
- [ ] Replace brittle hard-threshold-only decisions with deterministic biome suitability scoring where it improves continuity.
  - [ ] Define a suitability score for each non-physical land biome from its broad regional signal and relevant environmental fields.
  - [ ] Apply hard viability constraints only where physically meaningful, such as sea level, mountain elevation, minimum wetland humidity, or tundra temperature.
  - [ ] Among viable land biomes, prefer the highest deterministic suitability score instead of allowing a tiny crossing of one independent threshold to immediately create a biome island.
  - [ ] Add deterministic tie-breaking with no dependence on iteration order, frame state, actor state, or client-local data.
  - [ ] Keep scoring constants centralized and documented so later tuning does not require rewriting classifier logic.
- [ ] Make biome boundaries irregular without reintroducing small-scale noise.
  - [ ] Add optional low-frequency domain warping to the coordinates used for macro-region sampling.
  - [ ] Ensure warp frequency is lower than or comparable to biome-region frequency and never use high-frequency warping that creates small islands or noisy borders.
  - [ ] Derive warp offsets and seeds deterministically from the existing world identity.
  - [ ] Centralize warp strength and frequency as tuning constants and ensure warping is continuous across terrain-patch boundaries.
  - [ ] Verify domain warping bends and elongates large biome borders without changing world determinism or producing visible patch seams.
- [ ] Make lakes follow terrain-basin logic rather than humidity alone.
  - [ ] Stop classifying arbitrary humid lowland samples as `ShimmeringLakes` when no enclosed standing-water basin exists.
  - [ ] Use the existing deterministic basin or standing-water query as the primary requirement for `ShimmeringLakes`.
  - [ ] Allow humidity, elevation, shoreline conditions, or a broad wet-region signal to influence lake suitability, but never let humidity alone create disconnected lake-biome dots.
  - [ ] Preserve the existing lake water, shoreline, minimap, collision, and host/client consistency contracts.
- [ ] Improve regional relationships between mountains, tundra, forest, wetland, and meadow.
  - [ ] Derive a smooth mountain or upland influence from elevation so tundra naturally tends to occupy cold uplands surrounding or approaching mountain terrain.
  - [ ] Keep wetland suitability concentrated in low terrain and prevent mire from appearing on physically implausible ridges.
  - [ ] Allow forest and meadow to form large neighboring regions with local vegetation variation inside each region rather than alternating at vegetation-noise frequency.
  - [ ] Do not create mandatory biome rings, authored progression bands, guaranteed routes, or fixed travel corridors; all relationships must remain seed-generated and probabilistic.
- [ ] Tune field and regional frequencies by semantic scale.
  - [ ] Use very-low-frequency sampling for macro biome regions and climate-scale structure.
  - [ ] Keep `Flora` at a meaningfully higher frequency than biome-region signals so it can create local clearings and density variation without changing biome identity.
  - [ ] Document the assumed Unreal world-unit scale and the approximate world distance represented by each important frequency so future tuning is based on physical size rather than arbitrary constants.
  - [ ] Add developer-visible tuning values for macro biome scale, regional frequencies, warp frequency, and warp strength without exposing them as client-authoritative state.
- [ ] Add developer visualization and quantitative checks for biome coherence.
  - [ ] Extend `RenderWorldGenerationVisualization` to render the macro regional signals, final biome classification, and optionally biome suitability scores alongside the four authoritative fields.
  - [ ] Add a visualization mode that makes isolated biome components and narrow one-cell or few-cell biome slivers easy to identify.
  - [ ] Add deterministic automated sampling over a large fixed world area and report biome component statistics such as approximate connected-region count, median region area, small-component count, and boundary density.
  - [ ] Define a regression threshold that fails when the classifier returns to highly fragmented "biome confetti" behavior.
  - [ ] Verify that broad regions remain irregular and varied rather than collapsing into oversized uniform blobs.
- [ ] Preserve generator revision compatibility.
  - [ ] Do not silently change the layout of existing revision-1 worlds.
  - [ ] Introduce the coherent macro-biome classifier behind a new `GeneratorRevision` when required by the existing save/world-identity contract.
  - [ ] Keep the revision-1 classifier available for worlds that explicitly use revision 1.
  - [ ] Ensure the server-selected generator revision remains authoritative and clients reproduce the same regional samples and biome decisions.
- [ ] Verify Phase 8 as an integrated world-generation change.
  - [ ] Render at least two large-area previews for the same seed and confirm pixel-identical biome classification and regional signals.
  - [ ] Render at least one different seed and confirm meaningfully different macro-region placement.
  - [ ] Verify that Elderwood, Mossy Mire, Freezing Tundra, Meadows, Thunder Mountains, Shimmering Lakes, and Ocean appear as geographically coherent regions appropriate to their physical constraints.
  - [ ] Verify that local `Flora` variation produces clearings and density changes without repeatedly changing Elderwood to Meadows.
  - [ ] Verify lake classification agrees with deterministic standing-water basins rather than humidity-only patches.
  - [ ] Verify no biome or warp seam appears at terrain-patch boundaries.
  - [ ] Verify host and client classify matching biomes at sampled world positions and that no client can alter macro-region, scoring, or biome-selection state.
  - [ ] Run 

## Later gameplay milestones

Use `docs/04-roadmap.md` as the source of truth. Add decomposed M2–M5 tasks here only after their preceding milestone acceptance criteria pass.
