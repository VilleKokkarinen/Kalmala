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

Start this track only after M1 passes. `docs/08-world-generation-and-biomes.md` is authoritative: the current generator uses a separately seeded master land/water crop, continuous environmental fields and distance-gated biomes. World identity is the server-owned seed only. Lakes (0.35�3 km) and Mire (3�16 km) share all generation parameters except distance ranges. Earlier completed tasks record historical work, not compatibility requirements.

### Phase 1 — Seed and map proof

- [x] Define the immutable server-owned `WorldSeed` world-generation contract.
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

- [x] **Establish the local expanded-map contract.**
  - [x] Bind `M` to open/close an almost full-screen local map overlay; Escape closes it before the Settings menu can open.
  - [x] Make opening the map pause local movement/look input and retain pointer/wheel input; closing restores the prior game-input state.
  - [x] Define continuous world-space pan, cursor-anchored wheel zoom, explicit zoom bounds, and a recenter-on-owning-player action.
  - [x] Keep one local map instance per local player, with no RPC, replicated UI state, world mutation, or gameplay authority change.
  - [x] Verify toggle/input priority, zoom clamping, panning bounds, player recentering, 4:3/16:9/ultrawide layout, and coexistence with the minimap and Settings menu.

- [x] **Build a scalable full-map presentation from the existing generated-world contract.**
  - [x] Reuse immutable replicated world identity and local owning-pawn transform; do not sample actors, population layouts, hidden discoveries, hazards, or server-only state.
  - [x] Replace a single huge synchronous raster with bounded, cached, asynchronously generated world-space tiles and discard obsolete jobs after pan, zoom, identity, or player changes.
  - [x] Render original Kalmala terrain, ocean, inland water, and player-facing map treatment at multiple scales, preserving coastline agreement with terrain triangles.
  - [x] Establish measurable refresh/memory budgets and retain minimap responsiveness while the expanded map is open.
  - [x] Verify deterministic same-identity tiles, different-seed variation, seamless tile edges, no game-thread waits, and host/client presentation agreement.

- [x] **Add local exploration and fog-of-war without information leaks.**
  - [x] Define a bounded owning-player reveal radius driven only by that pawn's already replicated movement; unexplored areas must not disclose terrain classification, water, landmarks, population, or discoveries.
  - [x] Persist personal explored coverage under the immutable world identity and version it independently from generated-world/save data.
  - [x] Distinguish personal exploration from later shared exploration visually with original Kalmala treatment.
  - [x] Verify reconnect/restart persistence, identity mismatch rejection, reveal-edge continuity, and that a client cannot reveal remote terrain or another player's exploration through UI input.

- [x] **Add personal map pins and player orientation.**
  - [x] Draw a centred, facing owning-player marker and optional local coordinate/grid aids without route guidance.
  - [x] Support an original finite pin palette, label entry with validation/length limits, click placement, click-to-toggle completion/visibility, and explicit removal.
  - [x] Persist pins per player and world identity; never accept client pin data as a server gameplay instruction or discovery claim.
  - [x] Add accessible non-colour-only pin states and keyboard/controller alternatives for every pointer interaction.
  - [x] Verify map-to-world coordinate conversion at every zoom level, pin persistence, overlap selection, input focus, and no network/gameplay side effects.

- [x] **Add opt-in co-op awareness and temporary pings.**
  - [x] Define an owner-controlled opt-in for visible connected-player markers, using only normal replicated transforms and clear privacy/offline handling.
  - [x] Add a short-lived, rate-limited map ping that the server validates and relays only to eligible session members; it must not reveal unexplored terrain or create a persistent waypoint.
  - [x] Verify server rejection of malformed, distant, excessive, or unauthorized pings and matching expiry/order across host and clients.s

- [x] **Validate the full map as one integrated feature.**
  - [x] Repair square tiles inheriting circular minimap clipping, clip edge tiles to the map panel, and record exploration during gameplay while the main map is closed (user-requested correction).
  - [x] Run build plus focused automation for modal input, tiled rendering, fog, pins, and pings; include rendered host/client screenshots at three aspect ratios.
  - [x] Profile full-map open/pan/zoom memory, worker time, game-thread time, and late-join behavior before raising tile density or map range.
  - [x] Document all map/pin/share contracts, accessibility controls, known limits, and multiplayer authority decisions.

### M2 — Survival camp loop

- [x] Establish server-owned inventory, harvesting, fuelled campfires, basic crafting, validated placement, and persisted construction/storage.
- [x] Verify a two-player gathered camp restores with exact construction IDs, private storage contents, sparse harvest deltas, and no cross-world reuse.

**M2 acceptance:** passed. Two players can gather, craft, build, save/load a camp, and observe matching state after reconnect.

### M3 — Elemental world prototype

**Authoritative wetness update (2026-09-14):** Player wetness is only the server-owned `Wet` debuff. It is a reusable parameterized status effect: default maximum duration 120 seconds, rain trigger 10 uninterrupted seconds, movement multiplier 0.90, and stamina-use multiplier 1.25. Water applies it immediately; rain applies it only without an accepted roof overhead. Near a lit heat-producing campfire removes it. Surface moisture in the interaction grid is a fire/material value, not player wetness.

- [x] **Revise the M3 interaction and status contract.**
  - [x] Replace the legacy continuous player wetness/warmth penalty with a reusable server-owned `Wet` status definition and replicated remaining duration.
  - [x] Keep surface moisture as a bounded interaction-grid material input only; it must not create an independent player stat.
  - [x] Define authoritative construction health, roof protection, campfire `Lit`/`Smouldering`/`Extinguished` states, and all tunable defaults.

- [x] **Implement player Wet.**
  - [x] Apply Wet immediately on server-confirmed water occupancy and after 10 uninterrupted seconds of rain without an accepted overhead roof; clamp reapplication to 120 seconds.
  - [x] Apply the tunable 10% movement penalty and 25% stamina-use increase through the shared status-effect path; support future debuffs without bespoke player fields.
    - [x] Apply compiled shared status modifiers to walking, sprinting, and swimming; provide the 1.25 stamina-cost calculation and verify expiry restores defaults.
    - [x] Integrate the shared cost calculation with authoritative sprint stamina consumption, then verify host/client movement agreement.
  - [x] Remove Wet only through expiry or a nearby lit, heat-producing campfire; clients cannot set duration, source, multipliers, or removal.

- [x] **Implement roof, rain-wear, and campfire response.**
  - [x] Make roofs rain-immune. Apply slow server-owned rain wear only to exposed floors, walls, workbenches, and storage; clamp their health at 50% and prevent wear below an accepted roof.
  - [x] In dry weather, fuelled lit campfires remain lit. In rain without a roof, they become Smouldering with zero heat and automatically reignite when roof-protected again.
  - [x] Keep all building health, roof traces, rain exposure, fire transitions, and persistence server-authoritative; clients only render replicated state.

- [x] **Add clear local feedback and verify the vertical slice.**
  - [x] Present Wet duration, movement/stamina penalties, construction rain wear, and fire state using colour-independent player-facing cues.
    - [x] Show owning-player Wet duration and configured penalties in the read-only HUD, with an explicit inactive state.
    - [x] Add construction rain-wear and complete fire-state cues; verify rendered feedback.
      - [x] Explain hearth heat, fuel consumption and recovery in text; verify host/client smouldering feedback is visible when the crafting panel opens.
      - [x] Add construction health/rain-wear cues and verify rendered feedback; retain the combined feedback acceptance until covered.
  - [x] Verify invalid client wetness, building-health, roof, rain, or fire requests cannot alter authority or saves.
    - [x] Replace assertion-only weather mutation checks with runtime authority/input rejection and verify unchanged state for client-role and malformed calls.
    - [x] Complete live invalid-client state probes and confirm authoritative state and saves remain unchanged across Wet, construction, roof/rain and fire.
  - [x] Run a host/client scenario for immediate water Wet, delayed unroofed-rain Wet, roof immunity, capped rain wear, smoulder/reignite, and campfire Wet removal.

**M3 acceptance:** water immediately applies Wet; ten seconds of unroofed rain applies Wet; Wet is capped at two minutes and applies the configured movement/stamina penalties. A roof blocks rain exposure and protects structures. Exposed structures never fall below 50% health from rain. An exposed rainy campfire smoulders without heat and reignites once roofed.

### M4 — Combat and support magic

Start only after M3 acceptance passes. Preserve the open-world, route-free survival loop: no authored combat corridors, quests, direct-damage magic, new online services, or client-authoritative damage, rewards, discoveries, or learned effects.

- [x] Establish the server-authoritative combat and support-magic contract.
  - [x] Define replicated combat attributes, validated damage execution, defeat state, and narrow client intent RPCs for player attacks and support-effect activation. Contract: docs/11-combat-and-support-magic.md; definition only, runtime coverage remains below.
  - [x] Define stable server-owned IDs and sparse world deltas for creature defeats, points of interest, scroll discoveries, and per-player learned effects; preserve immutable world-identity validation. Contract: docs/11-combat-and-support-magic.md; definition only, schema/runtime work remains below.
  - [x] Add focused contract coverage proving malformed, distant, duplicate, and client-only combat/progression requests cannot change health, defeat state, rewards, or saves.
- [x] Add the smallest player combat loop with readable committed attacks and server-validated targets, damage, and cooldowns.
  - [x] Add a basic owned-pawn attack intent with server-selected wildlife trace target, committed windup/recovery, fixed validated damage, and monotonic replay/cooldown gating; no client target or damage payload.
  - [x] Replicate player-facing combat state and colour-independent hit, defeat, and unavailable-action feedback without exposing hidden server-owned targets.
  - [x] Verify host/client combat agreement, rejected invalid attack requests, and reconnect-safe player and world state.
- [x] Add deterministic server-owned wildlife population and behaviour foundations for the M4 archetypes.
  - [x] Activate bounded, seed-derived creature descriptors with stable IDs and terrain-safe spawning; clients only receive relevant replicated actors.
  - [x] Add server-owned idle, flee, investigate, and return behaviour primitives with deterministic budgets and no client-selected spawn or behaviour outcome.
- [x] Deliver the Mireling archetype as an optional camp-pressure encounter.
  - [x] Add original replicated Mireling presentation, close-range scavenger behaviour, and validated melee damage/defeat rewards.
  - [x] Verify seed reproduction, bounded activation, authority rejection, defeat persistence, and host/client combat presentation.
- [x] Deliver the boar archetype as an optional territorial charge encounter.
  - [x] Add original replicated boar presentation, resting-area threat response, charge/return behaviour, and validated meat/hide rewards.
  - [x] Verify seed reproduction, server-owned charge, invalid target-free client attack rejection, relevant replication, owner-only rewards, and defeat persistence across restart.
- [x] Verify server-owned targeting/damage, defeat persistence, reconnect consistency, and matching host/client behaviour.
- [x] Deliver the deer archetype as wary herd wildlife.
  - [x] Add original replicated deer presentation, bounded herd/flee behaviour, and validated meat/hide rewards.
  - [x] Verify deterministic group activation, combat/noise flight, defeat persistence, and matching host/client behaviour.
- [x] Add optional deterministic open-world points of interest and scroll discoveries across suitable biomes.
  - [x] Derive bounded, stable point-of-interest and scroll descriptors from the existing world identity without routes, mandatory crossings, or hidden client discovery queries.
  - [x] Add server-validated one-time discovery rewards with colour-independent local feedback and persistence across reconnect only for the entitled player.
  - [x] Verify same-seed reproduction, different-seed variation, duplicate/distant request rejection, and privacy of undiscovered content.
- [x] Implement the scroll-learned, non-damaging support-magic foundation.
  - [x] Add server-owned learned-effect validation, stamina/cooldown rules, replicated active-state presentation, and persistence keyed to the entitled player and immutable world identity.
  - [x] Implement Mending as a validated ally heal that cannot target invalid actors or damage enemies.
  - [x] Implement Hearth Shield as a temporary validated protective shield with explicit expiry and replicated feedback.
  - [x] Implement Bear's Vigor as a temporary validated stamina/strength boost with explicit expiry and replicated feedback.
  - [x] Implement Deer Call as a bounded validated behaviour influence on existing nearby deer only; it must not create wildlife or bypass harvest/loot rules.
  - [x] Verify each effect rejects invalid client payloads, never directly damages an enemy, persists learning correctly, and agrees across host/client presentation.
    - [x] Focused regression covers all allowlisted effects, malformed/client/replay gates, reconnect learning persistence, and non-damaging execution; rendered live-peer casts remain in the M4 vertical slice.
- [x] Place one scroll discovery as a server-validated Mireling boss reward and verify it remains optional to route selection.
**M4 acceptance:** complete under the revised scope. Focused authority, persistence,
and peer regressions cover the three archetypes, optional discoveries, and all four
non-damaging support effects. A rendered two-player acceptance scenario is no
longer a roadmap requirement.

### M5 — Vertical-slice finish (complete)

**Status:** Closed for backlog sequencing through the documented external-surface skip. The packaged fresh-player co-op loop remains unverified; see `docs/12-vertical-slice-runbook.md` and `PROGRESS.md`.

- [x] Complete original presentation and audio, onboarding, settings/accessibility, balance, performance, and release-regression work.
- [x] Smoke-launch the Windows Development package and record the conditional dedicated-server blocker.
- [x] Close M5 through the documented skip path; do not claim player-visible packaged co-op acceptance.

### M6 — Production hardening and supported-session validation (complete)

**Status:** Administratively closed by explicit user direction on 2026-09-22. Package smoke and regression subsets passed; the full release suite, tool-free packaged loop, and dedicated-server playtest remain unverified or blocked. See `docs/04-roadmap.md`, `docs/07-development-setup.md`, and `PROGRESS.md`.

- [x] Retain available host/client regression and package-smoke evidence.
- [x] Record the installed Launcher engine's dedicated-server limitation; do not claim server validation.

### M7 — Content update: survival progression and readable gameplay UI (complete)

**Status:** Closed through the documented headless acceptance path. Physical input and the player-visible packaged walkthrough remain unverified; details are in `PROGRESS.md`.

- [x] Establish versioned persistence and migration boundaries without silently extending earlier save schemas.
- [x] Complete server-owned skills, first-wave biome content, tool gathering and repair, food/crafting, hazards, and readable HUD work.
- [x] Pass fresh-profile host/client acceptance and the available M6 regression; record remaining player-session limitations.

See `docs/04-roadmap.md` for the gameplay and authority contract and `PROGRESS.md` for retained verification evidence.

### M8 — Ocean and long-distance travel (complete; dry-shore acceptance waived)

**Status:** Closed under the user-approved 2026-09-26 scope decision. Safe stopped disembark in qualifying deep water passed; generated dry-shore placement was waived and is not claimed as verified.

- [x] Confirm the generation/ocean baselines and implement server-owned skiff travel, weather pressure, optional discoveries, and identity-safe persistence.
- [x] Pass the integrated two-peer voyage, discovery, weather, safe disembark, late-join, and restart/reconnect checks.
- [x] Record actor, memory, replication, save-size, network, and frame-time snapshots. These are diagnostic samples, not approved ceilings or long-session profiles; see `docs/20-m8-ocean-performance-budget.md`.

The M8 travel and authority contract is in `docs/19-m8-ocean-travel-contract.md`; acceptance and the waiver are recorded in `docs/04-roadmap.md` and `PROGRESS.md`.

### M9 — Expanded biome content and encounter depth

Start only after M8 acceptance above passes. Follow the five ordered goals in
`docs/04-roadmap.md`; keep new gameplay optional, original, bounded, and
server-owned. Do not enable new persistent M9 state before the M9 persistence
contract and migration checks pass.

- [x] Add the second-wave biome source and loot catalogue.
  - [x] Map second-wave source materials across supported land biomes. See `docs/23-m9-second-wave-biome-sources.md`.
  - [x] Define four source catalogue entries, deterministic placement budgets, sparse depletion IDs, and bounded optional loot. See `docs/24-m9-second-wave-source-catalogue.md`.
  - [x] Gate Lightwood and Densewood harvesting on server-validated Bronze Axe and Iron Axe requirements from the roadmap. The required tool tier is checked on hitting the trunks; the matching Workbench or Forge level is checked when that axe is crafted or upgraded, not at the field source.
  - [x] Integrate approved materials and loot with the existing item catalogue, harvest transaction, owner inventory, and sparse world-delta contracts.
  - [x] Verify each approved source and reward is deterministic, bounded, same-world, in range, and unchanged by rejected client requests.
- [x] Add tool levels, workstation levels, and free repair.
  - [x] Extend the existing owner-only tool-condition records into the owner's carried-tool inventory with server-owned tool level and condition; keep tool level separate from skill level.
  - [x] Define the Bronze Axe and Iron Axe as entries in the carried-tool progression, with their target levels, upgrade costs, and matching Workbench/Forge levels before enabling their biome harvest gates.
  - [x] Add the buildable Forge and level-one baseline for the existing Workbench. Require the server-selected Workbench or Forge to match the target tool level when crafting or upgrading that tool.
  - [x] Add paid, buildable station attachments that each contribute +1 to the matching nearby Workbench or Forge level; validate same-world placement and station-use range on the server. Candidate attachments are a Workbench tool rack or vise and a Forge anvil.
  - [x] Use the existing level-five skill unlock for tier-two recipes. Validate tool level, skill, materials, station level, condition, and any output slot atomically; clients provide intent but no levels or outcomes.
  - [x] Replace material-paid repair and zero-condition replacement with free repair of damaged and broken tools. Add selected-tool repair through the Workbench/Forge GUI and a buildable Grinding Stone with an in-world `Repair All` action over the server-owned carried-tool list.
  - [x] Add the short tool-appropriate sharpening animation for Grinding Stone repair; keep it presentation-only. Repair requests must not accept a client-provided inventory list or condition value, and repair grants no crafting experience.
  - [x] Add readable level, material, station, and repair feedback. Keep detailed tool state owner-only and level/attachment persistence gated on the M9 save contract.
- [x] Extend optional camp and equipment progression through existing systems.
  - [x] Review optional camp and equipment examples and select additions that fit existing systems: the normal Chest; the current schema-4 catalogue has no Smoke Frame item or recipe.
  - [x] Keep storage, processing, construction, equipment effects, repair, costs, and skill awards within their existing authorities and accessibility feedback paths.
    - [x] Add the carried Construction Hammer and owner-local build menu; directly build floors, walls, and roofs from JSON-backed raw-material costs with server-owned placement and unchanged construction-save identities.
    - [x] Move the remaining kit-based camp structures to direct hammer builds from raw materials; remove their kit outputs from normal crafting while preserving existing save identities and station validation.
      - [x] Build the hearth ring directly from Stone and Wood with the Construction Hammer; consume one raw fuel item for ignition, with the placement checks and internal identity unchanged.
      - [x] Remove the Fuel and ConstructionSupply intermediate items; directly consume raw fuel or the equivalent Wood and Fibre recipe costs, and migrate stored legacy supplies without changing the save schema.
      - [x] Remove "Kit" from item and recipe names and external catalogue IDs/fields, add bounded descriptions to every item, and retain stable runtime/save IDs.
    - [x] Retire the raised chest variant; use the normal Chest as the only storage construction.
    - [x] Verify host/client privacy, rejected-mutation behavior, costs, and accessible feedback across the accepted camp additions.
- [x] Add optional exploration rewards without quest routing.
  - [x] Define original clue, landmark, treasure, boss, or environmental-discovery candidates that reward observation without a prescribed route or mandatory combat gate. See `docs/29-m9-exploration-rewards.md`.
  - [x] Derive candidate identity, placement, interaction eligibility, and any reward on the server; use stable sparse identities and reject duplicate, forged, or replayed claims.
- [x] Remove per-serving recipe fuel and generic fire metadata; resolve cooking heat from the server-owned station and live fire state.
- [x] Add Carrot, Potato, Rutabaga (swede), Onion, and one matching seed item for each to the item catalogue.
- [x] Add MeatStew and CookedDeerMeat output items and align their configured recipes with the stable item IDs.
- [x] Add Iron, a placeable Frying pan made from five Iron at a Forge, Root vegetable soup, Roasted root vegetables, and Deer and rutabaga roast recipes.
  - [x] Place the pan through the existing construction/save path; show look-at interaction text and open the selected Cooking rack, cauldron, or pan recipe GUI from the server-validated Interact input.
- [x] Version and migrate newly persistent M9 state before normal saves use it.
  - [x] Define explicit world/player scope, bounded records, exact seed/revision matching, compatibility policy, and migration behavior for tool levels, station progression, discoveries, and any approved camp/equipment state. See `docs/30-m9-persistence-migration.md`.
  - [x] Add round-trip, migration, mismatch, malformed-data, duplicate, and over-cap rejection coverage before enabling persistence.
    - [x] Implement the schema-2 world construction candidate and test its round-trip, schema-1 migration, exact identity, malformed/duplicate rejection, and bounds.
    - [x] Implement the schema-2 player discovery/tool/claim candidate and test its round-trip, schema-1 migration, exact identity, malformed/duplicate rejection, and bounds.
    - [x] Verify both candidates together across host/client reconnect before enabling schema-2 writes in normal play.
- [x] Run M9 cross-system acceptance after implementation. (2026-10-01; see `PROGRESS.md`.)
  - [x] Enable normal schema-2 construction-slot writes with schema-1 migration, complete-candidate revalidation, and persistent station attachments. (2026-09-30; see `PROGRESS.md`.)
  - [x] Enable normal schema-2 player-slot writes with migrated discovery/effect facts, server-revalidated M9 claims, and owner tool state. (2026-09-30; see `PROGRESS.md`.)
  - [x] Verify host/client agreement, rejected-mutation no-change behavior, owner-only tool/progression state, normal-save/reconnect behavior, and document bounded actor, memory, replication, and save observations. (2026-10-01; see `PROGRESS.md`.)
  - [x] Re-run the accepted M8 ocean travel loop and M6 supported-session regression; preserve documented physical-input or packaged walkthrough limitations. (2026-10-01; M8 restart journey and M6 camp/combat/rain peer checks passed; see `PROGRESS.md`.)

**M9 multiplayer boundary:** the server owns biome content selection, tool and
station levels, repair outcomes, crafting/progression outcomes, encounters,
loot, hazards, rewards, and persistent facts. Clients submit intent only;
private carried-tool and progression details remain owner-scoped.

**M9 acceptance:** the second content wave is playable across supported biomes
and ocean travel without developer commands; tool upgrades require matching
station levels, free repair is readable and authoritative, exploration rewards
remain optional, new saves migrate as documented, rejected client mutations
leave state unchanged, and M8 plus M6 regression gates remain green.

### M10 — Release completion and launch validation

Start only after M9 acceptance passes. Freeze feature scope and prepare the complete supported game for release without adding another gameplay layer. Follow the six ordered goals in `docs/04-roadmap.md`:

- [x] Freeze feature and content scope: close or defer remaining backlog items, lock accepted gameplay and save schemas, and require a focused regression plus explicit release rationale for any post-freeze change. (2026-10-01; see `docs/31-m10-scope-freeze.md`.)
- [x] Run the complete automated, rendered, authority, persistence, reconnect, world-generation, land-travel, ocean-travel, combat, support, construction, crafting, progression, weather, HUD, accessibility, and performance suite from clean temporary user directories. (2026-10-01; parent-level clean-profile suite passed, including 98 automations, rendered HUD/settings/map checks, host/client authority and reconnect scenarios, integrated ocean travel, and world-profile/performance checks; see `PROGRESS.md`.)
- [x] Validate current-version saves and all approved migrations, corrupt or malformed rejection, world/player identity mismatch handling, supported backup/recovery behavior, and no partial mutation on failed loads. (2026-10-01; see `PROGRESS.md`.)
- [x] Reconfirm startup, frame-time, actor, memory, streaming, worker, raster, replication, package-size, and save budgets across the documented representative hardware/profile matrix; record remaining limitations.
  - **Completed by product-owner direction (2026-10-02):** owner accepted closeout using the existing one-machine measurements and passing Potato/Low/Med/High/Ultra simulated profiles plus Reference/8-thread/4-thread CPU-only profiles (`docs/20-m8-ocean-performance-budget.md`, `docs/34-m10-constrained-performance.md`). Remaining limitations are retained: no physical low/mid-tier hardware verification or approved numerical release ceilings; simulations do not emulate lower-end GPUs; packaged representative startup, sustained travel, long-session peaks/growth and extended worker/raster coverage remain deferred. Completion records owner acceptance of this evidence and its limits, not measured compliance with unspecified budgets.
- [x] Produce the release candidate package and run the documented fresh-player co-op loop plus extended progression and long-distance travel using normal player actions. Attempt dedicated-server validation only if the documented engine capability is available. (2026-10-02: completed manually and confirmed by the product owner; prior package build/smoke passed. Dedicated-server validation remains skipped because the documented engine capability is unavailable. See PROGRESS.md.)
- [x] Archive build metadata, test results, logs, screenshots, migration notes, accessibility checks, known limitations, and reproducible release steps. (2026-10-02; see docs/release-evidence/2026-10-02/README.md.)

**M10 release boundary:** after feature freeze, do not introduce a gameplay system, platform, online service, save schema, or authority model unless the roadmap is explicitly reopened. Release work may fix defects, tune bounded values, improve presentation/accessibility, or optimize implementation while preserving accepted contracts.

**M10 multiplayer boundary:** preserve server authority; launch hardening must not add client-selected targets, outcomes, rewards, hidden-content queries, save values, timing authority, or private-state leakage.

**M10 acceptance:** the release candidate passes the complete documented suite from clean profiles; approved saves load or migrate correctly; full co-op progression from first spawn through biome exploration and long-distance ocean travel works without developer tools; performance and accessibility budgets stay within recorded limits; rejected requests leave authoritative state unchanged; and remaining limitations are documented.

## M11 — User experience and visual UI upgrades

Start only after M10 acceptance closes under its recorded owner-approved scope and limitations. Follow `docs/04-roadmap.md`: presentation and usability only, preserving gameplay, server authority, save contracts, and the established visual identity.

- [x] Establish and document one easily configurable theme file covering colours, fonts/text sizes and weights, outlines/borders, buttons, panels/background image references, padding/spacing, slot/icon sizes, interaction states, high-contrast variants, and animation settings. Build/reuse shared themed components for most UI and migrate existing HUD/menu styling in small increments; verify changes propagate across HUD, inventory, build, map, and options, with local accessibility overrides and safe invalid/missing-value fallbacks. Preserve gameplay bindings and save contracts.
  - [x] Add the bounded local theme loader and shared panel/text styling; migrate survival-status and weather HUD consumers, preserve accessibility overrides, document fallbacks, and verify the foundation. See `docs/35-ui-theme.md`.
  - [x] Extend this same theme foundation with font assets/weights, outlines/borders, buttons and interaction states, panel/background image references, slot/icon dimensions, and animation settings through shared components with real consumers and safe fallbacks.
  - [x] Migrate representative inventory, build, map, and options styling to the shared theme; verify cross-view propagation, rendered layout and local accessibility overrides, then run parent-level integration verification. (2026-10-02; see docs/35-ui-theme.md and PROGRESS.md.)
- [x] Add an owner-local invisible top-right status hotbar parent beside/below the minimap, with stable ordering, spacing/wrapping, no background/border/empty-slot chrome, no minimap overlap, and no ordinary gameplay-input capture. Verify empty and populated layout at supported viewport sizes and UI scales.
- [x] Move existing active player-status and weather presentation from the left panel/separate weather badge into the top-right parent. Give Wet, existing Hot/heat feedback, Storm, and other currently supported entries distinct original icons, readable names, and their respective server-derived timers; label untimed conditions as ongoing. Preserve accessible category, intensity/stack, source, and recovery details, owner privacy, and unrelated left-panel information. Verify transitions, expiry/removal, simultaneous effects, and no duplicate list; add no gameplay effect or fabricated countdown.
- [x] Audit the complete current inventory-item and buildable catalogues and add an original icon for every inventory item (including carried tools shown there) and every build-menu entry, mapped to canonical identities. Reuse the assigned icon across menus for a shared identity; verify complete coverage and retain readable names/details without changing IDs or gameplay data.
- [x] Visually update inventory and build menus with original background images and individually framed slot grids. Show icons plus existing counts, condition, selection, costs, requirements, availability, and rejection details where applicable; verify empty/selected/focused/unavailable cells, scrolling/reflow, text scale, contrast, and keyboard/controller navigation. Preserve capacity, stacking, transactions, and persistence; add no slot-index save or drag/drop mechanic. (2026-10-02; see PROGRESS.md and docs/ui-inventory-build/).
- [x] Visually update the expanded world-map menu opened with M and add an original background image behind readable terrain, fog, markers, pins, and controls. Preserve M/Escape behavior, pan/zoom/recenter, pin actions, modal input, privacy, and map saves; verify the decoration reveals no undiscovered content. (2026-10-02; see PROGRESS.md and docs/ui-world-map/.)
- [x] Add original background images to the main Escape options/settings shell and every option screen/tab (Video, Audio, Controls, Settings), using theme-configured images and shared panel components; verify readable labels, values, tabs, focus, text scale, and contrast while preserving option behavior and modal input. (2026-10-02; see PROGRESS.md and docs/ui-options-backgrounds/.)
- [x] Add a slight, fast opening animation to the main Escape options menu, sliding the panel down from the top. Configure duration, travel, and easing in the theme; retain immediate modal focus and correctly aligned hit testing. Verify close/reopen/resize interruptions, keyboard/controller access, and an instant/reduced-motion path without changing gameplay timing or scrolling menu content. (2026-10-03; see docs/35-ui-theme.md and PROGRESS.md.)
- [x] Polish the existing HUD and crafting, construction, equipment, and settings UI through small view-specific increments for typography, spacing, alignment, icons, hierarchy, and feedback. Record each affected view's concrete UX improvement and verify existing actions, focus/modal behavior, text scaling, contrast, and accessible details before checking this parent.
  - [x] Correct the Settings tab option-label overlap at 150% text scale; verify rendered owner-local values, focus, contrast, persistence, and modal input.
  - [x] Prevent the left inventory/support HUD overlapping the crafting/cooking modal; hide it while open and restore it on close. (2026-10-03; user-requested fix, docs/ui-polish/modal-hud.md.)
  - [x] Read fresh completed host/client inventory capture logs after delayed screenshots; reject missing states and peer errors. (2026-10-03; docs/ui-polish/capture-logs.md. Visual rendering blockers remain open.)
  - [x] Add lossless source-pixel text-review crops and paired peer evidence to distinguish screenshot-preview omissions from saved PNG content. (2026-10-03; docs/ui-polish/text-review.md. Existing visual candidates and parent acceptance remain pending.)
  - [x] Polish the existing HUD typography, spacing, hierarchy, and feedback with view-specific verification. (2026-10-03; unblocked and accepted; see docs/ui-polish/accepted.md.)
  - [x] Polish crafting and construction typography, spacing, alignment, and requirements/feedback with view-specific verification. (2026-10-03; unblocked and accepted; see docs/ui-polish/accepted.md.)
  - [x] Polish equipment presentation and accessible details with view-specific verification. (2026-10-03; unblocked and accepted; see docs/ui-polish/accepted.md.)
  - [x] Verify the affected views together and complete parent-level integration checks. (2026-10-03; unblocked and accepted; see docs/ui-polish/accepted.md.)
- [x] Add a shared item detail panel for selected/focused inventory slots with a larger icon, description, existing weight/condition where supported, and existing available actions; verify removal/consumption updates and keyboard/controller access without new statistics or actions.
  - [x] Add the shared themed detail-widget foundation using canonical descriptions, larger catalogue icons, and supplied owner-visible count/condition; attach it to populated catalogue cards and verify data binding/fallbacks. Interactive access remains pending.
  - [x] Connect selected/focused inventory slots to the shared panel with keyboard/controller access and existing action guidance; preserve gameplay input outside the inventory interaction flow. (2026-10-04; docs/37-item-details.md.)
  - [x] Verify removal/consumption refresh, rendered text scale/contrast and owner privacy, then run parent-level integration checks. (2026-10-04; resumed and accepted; docs/37-item-details.md.)
- [x] Add local search, category filters, sorting, and appropriate category grouping to inventory/crafting/build menus, including named build groups for existing structural pieces, stations, and camp utilities. Retain All/no-results views, stable focus/selection, and keyboard/controller access; change presentation only, not IDs, availability, saved inventory order, or hidden-content visibility.
  - [x] Add owner-local name search, All/items/carried-tool filters, stable presentation sorting and category grouping to interactive inventory inspection; verify selection, refresh, empty/no-results and keyboard/controller access. (2026-10-04; docs/38-menu-browsing.md.)
  - [x] Add local search, category filters and sorting to existing crafting/cooking recipe views without changing availability or server transactions. (2026-10-04; docs/38-menu-browsing.md.)
  - [x] Add local search, filters, sorting and named Structural pieces, Stations and Camp utilities build groups from current catalogue presentation metadata. (2026-10-04; docs/38-menu-browsing.md.)
  - [x] Verify inventory/crafting/build browsing together at supported scales/contrast with owner privacy, focus/modal and authority regressions, then complete parent-level verification. (2026-10-04; docs/38-menu-browsing.md.)
- [x] Show ingredient icons and owned/required counts plus supported station/tool/skill/unlock requirements and explicit unavailable reasons in crafting; update from existing owner-visible state while retaining independent server transaction validation. (2026-10-04; parent acceptance, docs/39-crafting-ingredients.md.)
  - [x] Add canonical ingredient icons and live owner-visible owned/required counts for batch-one recipes and direct-build costs; verify consumption refresh, separate owners and no-selection clearing. (2026-10-04; docs/39-crafting-ingredients.md.)
  - [x] Present supported station/tool/skill/unlock requirements and explicit unavailable reasons truthfully from existing contracts, without inventing recipe locks. (2026-10-04; docs/39-crafting-ingredients.md.)
  - [x] Verify counts/requirements together at supported scales/contrast with owner privacy, modal/focus and transaction regressions, then complete parent-level verification. (2026-10-04; docs/39-crafting-ingredients.md.)
- [x] Add compact icon-plus-text notifications for accepted owner-visible item gains, discoveries, and skill increases with a bounded/coalesced queue, no duplicate refresh/reconnect messages, unobstructed HUD/modal placement, and theme/reduced-motion timing. (2026-10-04; parent acceptance, docs/40-notifications.md.)
  - [x] Add owner-only skill-level notifications with silent session baselines, bounded/coalesced expiry and passive themed icon/text presentation. (2026-10-04; docs/40-notifications.md.)
  - [x] Add accepted owner-visible item-gain notifications without refresh/reconnect duplicates. (2026-10-04; docs/40-notifications.md.)
  - [x] Add accepted owner-visible discovery notifications without refresh/reconnect duplicates. (2026-10-04; docs/40-notifications.md.)
  - [x] Verify combined notifications, supported scales/contrast, HUD/modal placement, reduced motion, owner privacy and reconnect suppression; run parent-level checks. (2026-10-04; docs/40-notifications.md.)
- [x] Apply shared theme-driven hover/focus highlights and short transitions across buttons, tabs, slots, and selectable UI; distinguish focus/selection/disabled states without colour alone and retain essential cues and focus in refresh/reduced-motion paths. (2026-10-05; parent acceptance, docs/35-ui-theme.md.)
- [x] Improve near-crosshair interaction prompts with the existing target name, supported action, and current remapped keyboard/controller binding; verify unavailable/no-target/modal/rebinding cases while preserving server trace/range validation and avoiding hidden-target queries. (2026-10-05; parent-level acceptance, docs/07-development-setup.md, docs/35-ui-theme.md.)
- [x] Add a world-map symbol legend and local category visibility filters for already visible markers; preserve fog/co-op privacy and pin data, truthful selection, and keyboard/controller access without discovering content or deleting filtered markers. (2026-10-05; parent-level verification, docs/07-development-setup.md, docs/15-presentation-ownership.md, docs/35-ui-theme.md, docs/ui-world-map/verification.txt.)
- [x] Add configurable whole-interface scale and a reduced-motion/disable-decorative-animation option in existing local settings, layered over theme defaults and distinct from text scale. Document bounded ranges/defaults and verify HUD/menu reflow, focus, contrast, restart persistence via local settings, and immediate motion-free feedback; preserve gameplay and save schemas. (2026-10-05; parent-level verification, docs/14-settings-and-accessibility.md, docs/35-ui-theme.md.)
- [ ] Add a larger recipe/build-result image or icon beside selected-result descriptions and requirements, using shared preview/detail components and canonical icon mappings; verify unavailable states and keyboard/controller selection without gameplay spawns or new item properties.
- [ ] Add local recipe/build bookmarks and a Favorites category, with theme-configurable bottom-right star, border, or both. Separately mark the most-built piece, most-cooked recipe, and most-crafted item gold, second silver, third bronze; distinguish manual favorites from usage ranks and include non-colour Favorite/Rank labels. Add Recent shortcuts for the latest successfully crafted item, built piece, and cooked recipe in Favorites, plus a distinct theme-configurable clock/corner badge and Recent text on their ordinary menu slots. Replace only the relevant category's previous Recent entry on a new accepted action; exclude failed requests, selections, and replayed events. Verify Recent/favorite/rank marker coexistence, no overlap, and readable reduced-motion cues without requiring manual bookmarking. Count successful actions once, exclude failures/replayed events and cooking from other-crafting counts, bound records to current IDs, and document deterministic ties/unused entries/local history lifecycle before implementation; preserve gameplay, privacy, and save schemas.
- [ ] Restore each menu's last applicable tab/category, selection, and scroll position when reopened during the local session; handle removed/filtered/consumed entries and resize/UI-scale changes with safe fallback/clamping, preserve map pan/zoom/modal behavior, and never replay a gameplay action. Cross-restart persistence is not required.
- [ ] Add brief theme-driven status-icon cues for authoritative start, refresh, and end transitions, with clearly ended feedback, coalescing, no countdown/reconnect replay, and a static reduced-motion path; never alter the effect or its duration.
- [ ] Add inline current-versus-selected item stats to existing equipment/forge/crafting details: selected value plus signed delta, green increases/red decreases for crush/slash bonuses, neutral equal values, and accessible text. Verify the illustrative Iron Sword (crush 0/slash 40) to Iron Mace (crush 60/slash 0) format `crush 60 (+60)` / `slash 0 (-40)` using supported real data or presentation-only fixtures; add no items, damage types, balance values, comparison card, or new stat authority. Handle missing/incompatible stats and equipment/selection changes truthfully.
- [ ] Complete rendered host/client M11 acceptance covering top-right/minimap placement, status/weather timers and transitions, complete inventory/buildable icon coverage, inventory/build/map and all option-menu backgrounds, inventory/build slot grids and states, theme propagation/fallbacks and shared components, fast options slide-down and interrupted/reduced-motion paths, item details, search/filter/sort/category grouping (including building), crafting counts/reasons, notifications, hover/focus, remapped interaction prompts, map legend/marker filters, UI scale/reduced motion, recipe/build previews, favorite/rank/Recent markers, coexistence, category replacement, and accepted-use counting, remembered menu positions, status transition cues, inline stat deltas, M-key map interactions, separate owners, reconnect, empty/multi-effect layout, supported resolutions/aspect ratios, text scale, high contrast, and keyboard/controller access. Run relevant existing UI/status/weather/inventory/construction/map/authority/input/reconnect regressions and check affected UI performance budgets; retain screenshots and known limits.

**M11 acceptance:** the top-right status parent, complete item/buildable icons, inventory/build slot grids, background images for inventory/build/M-key map/all option menus, fast main-options slide-down opening, a configurable shared-component theme, all approved detail/navigation/feedback/accessibility improvements (including build-category grouping, previews, favorite/usage ranks/most-recent markers, remembered menus, status cues, and inline comparisons), and graphical UI improvements meet `docs/04-roadmap.md` without changing gameplay, balance, weather/effect rules, authoritative durations, persistence, or product scope.

## User-directed build repair — 2026-10-07

- [x] Repair the main-checkout DebugGame editor compilation and unity-name collisions, exclude the two failing installed-engine utilities from `Kalmala.slnx` solution builds, and verify the editor target plus solution configuration. See `docs/42-build-repair.md`. This explicit repair adds no roadmap feature or new milestone; existing M11 acceptance records remain unchanged.

## M12 — Inventory menu and cleaner gameplay HUD

Start after M11 acceptance. `docs/04-roadmap.md` defines the ten product goals.
Unchecked items below remain pending; completed historical milestones are unchanged.

**Run sizing:** top-level entries group features; each direct child is one selectable
increment. Estimates include context, implementation, lightweight inspection/checks,
backlog/progress updates and commit. Target 20–30 minutes, not a guaranteed deadline.
No nested executable children: do not select an aggregate task accidentally.
Keep every increment integrated/playable; retain an old entry point until its replacement
works. Full builds, rendered peer runs and broad regressions remain deferred to the
milestone-final run under `AGENTS.md`. Each child includes careful inspection and
useful narrow checks; prepare/update affected acceptance helpers rather than running
the full rendered matrix during normal increments. Document full verification as deferred.

- [x] Add the dedicated Inventory menu (roadmap goal 1).
  - [x] [20–30 min] Add the local inventory-toggle action with default Tab/I mappings and a minimal themed menu shell; both keys open/close the same owner-local instance through existing remapping conventions. (2026-10-07; remappable Tab/I action and themed owner-local shell)
  - [x] [20–30 min] Integrate inventory modal priority with crafting/settings/map, text-entry and focus handling, Escape close, and cursor/movement/look restoration; inspect empty-shell open/close paths and update narrow input checks. (2026-10-07; owner-local modal/input integration and Tab/I contract)
  - [x] [20–30 min] Populate the shell with the existing owner pack grid/counts and empty state, reusing shared widgets and owner-only data; retain the old pack view until the new grid is usable. (2026-10-07; read-only 16-slot owner pack grid reuses the shared catalogue rows widget; empty/pending state and live local refresh added.)
  - [x] [20–30 min] Bind selection to the existing item icon/description/detail component and safe fallback when an item disappears; prepare a focused selection/privacy check. (2026-10-07; owner-local selected-slot detail, disappearance fallback, and focused automation coverage)
  - [x] [20–30 min] Add carried-tool/equipment rows and existing inspection/actions with real levels/condition; preserve the bounded owner-only tool contract. (2026-10-07; up to six owner-only tools are selectable with live level/condition and existing server-validated repair action)
  - [x] [20–30 min] Wire supported carried-food Eat/use actions and their actual availability/effect through existing server paths; add no station gate and keep rejected/replayed use safe. (2026-10-07; owner-only Eat action routes supported food through the existing station-free server transaction, with live effect/availability/result feedback; replay remains server-rejected.)
  - [x] [20–30 min] Reuse inventory search/filter/sort, keyboard/controller focus and session selection/scroll restoration; adapt narrow browsing checks for removed entries and resize/scale fallback. (2026-10-07; owner-row search/filter/sort, focus navigation, session restoration, responsive panel and empty/no-results recovery checks)
- [x] Remove the persistent left-side panel (goal 2).
  - [x] [20–30 min] Preserve support/combat selection and concise action-result/discovery feedback through existing appropriate HUD surfaces independently of the old panel; update narrow feedback/input expectations. (2026-10-07; owner-local support strip and deduplicated transient combat/support/discovery notices now sit outside the legacy panel)
  - [x] [20–30 min] Remove the old panel, glyph row, diagnostic/help text and pack/tool grid now covered by Inventory; inspect normal/empty paths for residual chrome and prepare the absence regression. (2026-10-07; retired the persistent pack HUD and its overlap/capture hooks; Inventory remains on-demand with the sixteen-slot empty-state regression)
- [x] Halve minimap edge padding (goal 3).
  - [x] [15–25 min] Change both margins from 24 to 12 UI units and update the narrow placement/layout validator for DPI, 4:3/16:9/ultrawide and status separation; keep size/zoom/circular clipping unchanged. (2026-10-07; top/right offsets are 12 UI units and rendered inset checks account for viewport DPI)
- [x] Remove the bottom tutorial/help card (goal 4).
  - [x] [15–25 min] Remove the pictured onboarding banner across fresh-start/transition/reconnect presentation, preserve essential existing notifications, and update the narrow absence check; no replacement banner. (2026-10-07; disabled the local presenter, removed its default dismiss/revisit mappings, and added fresh-start/reconnect absence preflights; see docs/13-onboarding-and-tutorial.md and PROGRESS.md.)
- [x] Confine input-binding text to Options (goal 5).
  - [x] [20–30 min] Remove key/button names and control legends from persistent HUD, onboarding and interaction prompts; preserve concise action names, binding behavior and nonvisual accessibility. (2026-10-07; owner-local prompts now show readable target/action text without binding labels; mappings remain in Options > Controls.)
  - [x] [20–30 min] Remove binding/help legends from inventory and build/crafting/repair/storage views and tooltips; preserve actual button labels and focus navigation. (2026-10-07; removed key/button legends and input-binding display lookup, with action-label/focus source coverage)
  - [x] [20–30 min] Remove remaining map/status/detail-view legends and audit Options as the only player-facing binding-text source; add a narrow source/text audit and update stale text expectations. (2026-10-07; map keeps symbol/status/action labels without key legends, and the copy audit covers map/status/detail/menu sources.)
- [x] Simplify recipe/detail presentation templates (goal 6; catalogue prose is goal 9).
  - [x] [20–30 min] Simplify construction recipe details to useful description, ingredients, real placement/fuel requirements and one blocker; remove repeated names/costs and server/request boilerplate. (2026-10-07; concise construction descriptions, placement/fuel requirements, and first blocker)
  - [x] [20–30 min] Simplify food/general crafting detail templates to result, ingredients, supported quantity/real station/heat requirements and one blocker; remove empty/no-lock boilerplate and unrelated skill lists. (2026-10-07; canonical output descriptions, result/batch summary and concise real requirements)
  - [x] [20–30 min] Simplify upgrade/repair templates to real tool comparison/condition, relevant prerequisites/costs and one blocker; update narrow detail assertions without changing catalogue/gameplay data. (2026-10-07; owner-local tool condition and upgrade comparison show authored requirements/costs with one current blocker)
- [x] Separate construction and world-service contexts (goal 7).
  - [x] [20–30 min] Add a shared themed station-context shell and validated interaction routing, with object identity, close/leave/destruction handling and input restoration; integrate one existing station entry while preserving old service access. (2026-10-08; owner-only accepted actor/kit/ID routes the Cooking Rack through the shared shell; CraftMenu remains available.)
  - [x] [20–30 min] Workbench Craft section: reuse supported recipe/tool operations, including Bronze Axe and Grinding Stone, with material/station requirements and effective level/attachment state; no unrelated catalogue. (2026-10-08; exact accepted Workbench context now shows only its recipes, Tool Rack production and Bronze Axe action with owner material/station detail.)
  - [x] [20–30 min] Workbench Repair section: owner carried-tool condition/selection and existing free selected-tool repair; prepare rejection/privacy checks and keep crafting selection separate. (2026-10-08; dedicated owner-tool selection and condition, separate Repair section, existing ID-only server request, and focused owner/stale-context assertions prepared.)
  - [x] [20–30 min] Forge Craft section: Forge-compatible production including Frying Pan, real material/station requirements and effective level/attachment state; route the existing world interaction to it. (2026-10-08; accepted Forge interaction opens the scoped Craft shell with effective level/anvil state and focused assertions prepared)
  - [x] [20–30 min] Forge Upgrade section: existing Bronze Axe-to-Iron Axe progression, target comparison, material/skill/station prerequisites and server transaction; retain correct unavailable states. (2026-10-08; added Forge Upgrade tab with owner-local comparison, costs and first blocker; reused server ID-only transaction)
  - [x] [20–30 min] Forge Repair section: reuse selected-tool repair with owner conditions and exact existing validation; isolate its state from Craft/Upgrade and prepare focused checks. (2026-10-08; Forge Repair uses the independent owner-tool inspector and existing ID-only repair path, with stale-context assertions prepared)
  - [x] [20–30 min] Cooking Rack interaction/menu: only Cooked boar/deer meat, ingredient counts, supported quantity and live hearth heat through existing cooking paths; prepare one station/rejection check. (2026-10-08; dedicated two-recipe Cook section, live owner-local heat status, batch limits, stale-context request guard, and host/client scope marker; docs/07-development-setup.md and docs/10-campfire-and-crafting.md.)
  - [x] [20–30 min] Cauldron interaction/menu: only Meat stew/Root vegetable soup, reusing the cooking shell with correct ingredients, quantity and heat; prepare one station-scope check. (2026-10-08; shared Cook shell is scoped to the two Cauldron recipes with live owner-local ingredient/heat feedback, existing batch limits, stale-context suppression, and host/client scope marker; docs/07-development-setup.md, docs/10-campfire-and-crafting.md, and docs/15-presentation-ownership.md.)
  - [x] [20–30 min] Frying Pan interaction/menu: only Roasted root vegetables/Deer and rutabaga roast with existing heat/count rules; distinguish Forge production, Build placement and pan cooking in narrow checks. (2026-10-08; accepted pan interaction opens the scoped Cook shell with ingredient, quantity, description, and live heat presentation; the focused fixture separates five-Iron Forge production, Build-category placement, and pan cooking.)
  - [x] [20–30 min] Chest interaction/storage menu: reuse the existing private chest/owner-pack selector and Deposit/Withdraw transactions with capacity/count feedback; expire stale object data and prepare a privacy/transfer check. (2026-10-08; accepted Chest interaction opens the Store shell with owner-only pack/chest selectors, 16-stack count/capacity feedback, selected-ID one-item routes, stale-context closure, and a focused host/client privacy/request marker; see docs/07-development-setup.md, docs/10-campfire-and-crafting.md, and docs/15-presentation-ownership.md.)
  - [x] [15–25 min] Grinding Stone: retain direct remappable Interact/default E -> free Repair All with no menu/confirmation; adapt the interaction regression for one accepted action, no extra mutation and action-only feedback. (2026-10-08; the visible stone prompt says Repair all, and the server fixture asserts one result, unchanged tools/pack/context, and no menu; repeat repair is an unchanged no-op.)
  - [x] [20–30 min] Campfire: Interact/default E -> exactly one available fuel item using existing priority/duration/cap and range validation; no menu/picker/extra lighting toggle; prepare no-fuel/full/rejection checks. (2026-10-08; the visible prompt says Add fuel, server Interact consumes one priority-selected raw item for 60 seconds without changing fire state, and the transaction fixture covers full/no-fuel/range rejection.)
  - [x] [20–30 min] Scope the Build catalogue/grid to construction and placement, including bootstrap construction and station/attachment placement; keep station-required item production in its service menu and preserve ingredient/preview behavior. (2026-10-08; standalone Build defaults to supported placeables, limits categories to Build groups, and disables station-required/attachment production while retaining preview and placement; see docs/07, docs/10, and docs/38.)
  - [x] [20–30 min] Remove obsolete inventory/food/refuel/repair/upgrade/storage actions and unrelated lists from Build after their destinations are integrated; retain Light hearth because Campfire Interact only refuels; audit passive rack/anvil state in parent-station menus and prepare cross-context regressions. (2026-10-08; legacy pack/food/refuel/repair/upgrade/storage controls and unrelated status lists are gone from standalone Build, while placement/preview and the distinct Light hearth action remain; cross-context markers retain Workbench Tool Rack and Forge Anvil status.)
- [ ] Generate/integrate catalogue icons (goal 8).
  - [x] [20–30 min] Create a pinned canonical icon manifest for all live items/tools/build/results/upgrade targets, with aliases, cross-view reuse and fixed batch membership; exclude Campfire from inventory but include its construction image. (2026-10-08; pinned 48 canonical runtime IDs in docs/43-catalogue-icon-manifest.md and docs/catalogue-icon-manifest.csv, including all aliases, recipe/build result reuse, 12 four-image batches and three 16-image import batches; CampfireKit is construction-only.)
  - [x] [20–30 min] Add the shared UI texture lookup/import preparation and unknown-ID fallback using one original pilot icon; establish transparent 64x64 RGBA output, source retention and a narrow image/path validator. (2026-10-08; canonical soft-object path lookup, vector/question-mark fallback, retained Wood source, prepared transparent 64x64 PNG, and one-ID validator.)
  - [x] [20–30 min] Generate/review icon batch 01: next up to four uncompleted manifest objects; retain original sources and final validated 64x64 PNGs, with no baked-in labels/chrome/badges. (2026-10-08; Wood pilot, Lightwood, Densewood, and Coal sources retained; transparent 64x64 finals validated and reviewed at native size in docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 02: next up to four uncompleted manifest objects; same framing/lighting and final-image checks. (2026-10-08; original and validated 64×64 transparent PNGs for Stone, Iron, Fibre, and PeatAmber, reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 03: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; FrostSalt, MirelingAsh, CampfireKit, and WorkbenchKit originals and transparent 64×64 PNGs generated, validated, and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 04: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; ForgeKit, WorkbenchToolRackKit, ForgeAnvilKit, and GrindingStoneKit sources and transparent 64×64 finals generated, validated, and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 05: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; StorageKit, CookingRackKit, FryingPanKit, and CauldronKit originals retained and transparent 64×64 finals validated and reviewed; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 06: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; FloorKit, WallKit, RoofKit, and BoarMeat originals retained with centered transparent 64×64 finals; all four identities validated and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 07: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; DeerMeat, BoarHide, DeerHide, and CookedBoarMeat originals retained; transparent 64×64 finals validated and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 08: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; CookedDeerMeat, HearthBroth, MeatStew, and RootVegetableSoup originals retained; transparent 64×64 finals validated and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 09: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; RoastedRootVegetables, DeerRootRoast, Carrot, and Potato originals retained; transparent 64×64 finals validated and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 10: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; Rutabaga, Onion, CarrotSeed, and PotatoSeed originals retained; transparent 64×64 finals validated and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 11: next up to four uncompleted manifest objects; same source/output and native-size readability checks. (2026-10-08; RutabagaSeed, OnionSeed, ReedKnife, and FieldHatchet originals retained; transparent 64×64 finals validated and reviewed at native size; see docs/43-catalogue-icon-manifest.md.)
  - [x] [20–30 min] Generate/review icon batch 12: remaining up to four manifest objects; audit full source/final-image coverage. Add ordered four-object batches before this audit if the pinned manifest exceeds capacity; never mark uncovered objects complete. (2026-10-08; StonePick, BronzeAxe, IronAxe, and ConstructionHammer originals retained with transparent 64×64 finals, reviewed at native size, and all 48 manifest identities pass source/final validation; see docs/43-catalogue-icon-manifest.md.)
  - [ ] [20–30 min] Import/map texture batch A: first up to sixteen manifest outputs/aliases through the shared importer; preserve dimensions/alpha and validate only this batch's identity/asset assignments.
  - [ ] [20–30 min] Import/map texture batch B: next up to sixteen manifest outputs/aliases through the same path; validate this batch's identity/asset assignments.
  - [ ] [20–30 min] Import/map texture batch C: remaining up to sixteen manifest outputs/aliases; add bounded batches if needed, then audit full mappings without editor-wide verification.
  - [ ] [20–30 min] Integrate shared images in Inventory/tool/item/ingredient/storage components and prepare focused overlay/count/unknown-ID checks; retain readable text and owner privacy.
  - [ ] [20–30 min] Integrate shared images in Build/crafting/upgrade grids and selected-result previews; update coverage/alias assertions and prepare final rendered Favorite/Rank/Recent coexistence checks.
- [ ] Rework names/descriptions and Campfire classification (goal 9).
  - [ ] [20–30 min] Create a pinned before/after copy audit covering live catalogue/tool/build/result text sources and manual edits to retain; divide ordered copy batches of at most eight identities.
  - [ ] [20–30 min] Separate the Campfire construction descriptor from inventory-item lookup in loaders/validators, retaining legacy/internal aliases; keep direct placement playable and prepare focused compatibility/no-stack checks.
  - [ ] [20–30 min] Remove HearthRing's normal inventory-item definition/presentation once construction resolution works, name the recipe/placed result Campfire, and update icon/count contracts; inspect failed placement, current costs and saved-construction restoration paths.
  - [ ] [20–30 min] Rename the Workbench item/recipe and their shared prompts/results to Workbench without changing stable identities; leave other audited name changes to the bounded copy batches.
  - [ ] [20–30 min] Apply copy batch 01: first up to eight audited identities, preserving manual improvements and truthful uses; validate text/name consistency and unchanged non-text data.
  - [ ] [20–30 min] Apply copy batch 02: next up to eight audited identities; same preservation, consistency and non-text-data checks.
  - [ ] [20–30 min] Apply copy batch 03: next up to eight audited identities; same preservation, consistency and non-text-data checks.
  - [ ] [20–30 min] Apply copy batch 04: next up to eight audited identities; same preservation, consistency and non-text-data checks.
  - [ ] [20–30 min] Apply copy batch 05: next up to eight audited identities; same preservation, consistency and non-text-data checks.
  - [ ] [20–30 min] Apply copy batch 06: remaining up to eight audited identities; add bounded batches if needed and close the before/after audit with complete coverage, not blanket regeneration.
  - [ ] [20–30 min] Remove leftover hardcoded old labels from inventory/ingredients/storage/prompts and update focused search/sort/name/icon expectations for these sources.
  - [ ] [20–30 min] Align recipe/build/station/upgrade/result text with the revised shared catalogue and update text assertions/documentation; audit wrapping expectations and preserved manual copy.
- [ ] Compact active-only status/weather HUD (goal 10).
  - [ ] [20–30 min] Anchor the status group to the minimap's actual upper-left layout, shared 12-unit top margin and 12-unit separation; update the narrow scaled placement validator.
  - [ ] [20–30 min] Filter to active statuses, qualifying Hot/Cold and current Storm; hide normal weather and collapse expired/empty entries without retained end icons or reserved slots; update narrow snapshot/expiry assertions.
  - [ ] [20–30 min] Pin a supported status/weather icon manifest and generate/review its first up to four original 64x64 transparent images through the established asset pipeline.
  - [ ] [20–30 min] Generate/review the next up to four status/weather images; validate identities, alpha, dimensions and recognizable native-size silhouettes.
  - [ ] [20–30 min] Generate/review remaining up to four status/weather images and import/map the complete bounded set; add bounded batches if necessary and validate coverage.
  - [ ] [20–30 min] Render 64x64 icons without visible names, with centred m:ss beneath finite player effects only; remove weather/untimed durations/ongoing labels including player detail views while retaining accessible names and authoritative timings.
  - [ ] [20–30 min] Replace wide cells with compact columns, four-unit gaps and safe leftward/downward wrap; update zero/one/three/many-state layout assertions for scales/aspect ratios.
  - [ ] [20–30 min] Adapt the status-hotbar rendered helper for active-only/no-name/player-timer presentation and register the already prepared inventory/station/direct-interaction checks in the final capture matrix; parse/inspect now, defer execution to final verification.
- [ ] Complete M12 milestone-final verification (required exception to normal run sizing).
  - [ ] [Variable; may exceed 30 min] After the final implementation increment, inspect all milestone changes/handoffs; perform the prescribed full build, applicable automation/UI/input/inventory/construction/crafting/repair/cooking/storage/authority/reconnect and performance checks, plus rendered host/client/accessibility matrix and retained captures for all ten goals. Diagnose/repair milestone defects and rerun required verification until it passes or a genuine blocker prevents completion. Record commands/results/limits and commit only verified repairs; mark M12 complete only after success. This verification runs in the same final implementation run under AGENTS.md and is never split into unchecked passing-looking partial milestones.

**M12 status:** In progress; implementation and milestone-final verification pending.

**Sizing audit:** every unchecked item is in M12. Parent headings describe cumulative
features, not 30-minute promises. Executable direct children are estimated at 15–30
minutes with lightweight checks. Image-service waits/retries, unknown integration
issues and milestone-final verification can exceed estimates; preserve the smallest
integrated increment and record real results. Batch membership is pinned in manifests,
with additional bounded children inserted in order if live coverage requires them.
