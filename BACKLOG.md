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

### M5 — Vertical-slice finish

This milestone is decomposed so its release work is ordered and independently
verifiable. M4 is complete under its revised focused-regression acceptance.

- [x] Establish the shippable vertical-slice baseline and tool-free loop.
  - [x] Document the 20–30 minute solo/listen-server co-op walkthrough using only normal player actions and the existing route-free world. See `docs/12-vertical-slice-runbook.md`.
  - [x] Define the fresh-player start, camp preparation, optional wilderness travel, creature/discovery/support choices, and return-state evidence required by M5 acceptance. See the acceptance matrix in `docs/12-vertical-slice-runbook.md`.
- [x] Keep the current M5 documentation and presentation contracts runnable as one no-build suite. See `Scripts/Verify-M5DocumentationContracts.ps1`.
- [x] Complete the original visual and audio presentation pass.
  - [x] Inventory the remaining presentation seams and add a no-build project-ownership audit. See `docs/15-presentation-ownership.md` and `Scripts/Verify-PresentationOwnership.ps1`.
  - [x] Replace remaining prototype presentation with original project-owned player, creature, environment, UI, and feedback assets without changing gameplay contracts.
    - [x] Verify the existing original faceted player model through a rendered offscreen host/client controls fixture.
    - [x] Add original project-owned vector glyphs for all four support effects to the local HUD; retain explicit text and input labels.
    - [x] Refine the original collision-free Mireling silhouette for clearer camp-pressure readability without changing its replicated gameplay behavior.
    - [x] Replace the boar's generic tetra silhouette with an original low, bristled profile, tapered muzzle, and paired tusks while preserving its replicated gameplay behavior.
    - [x] Refine the original deer silhouette into a lighter, long-legged, alert profile while retaining its existing antler identity and replicated gameplay behavior.
      - [x] Review the refined deer in a rendered, close host view after the bounded peer fixture positions the target and companion.
      - [x] Inspect the original Mireling silhouette in a rendered, bounded host/client encounter before closing the creature presentation pass.
  - [x] Define original ambient, weather, interaction, combat, discovery, and support-effect cue groups with readable non-audio equivalents and a no-build contract check. See `docs/16-audio-cue-contract.md` and `Scripts/Verify-AudioCueContract.ps1`.
  - [x] Add original ambient, weather, movement, interaction, combat, discovery, and support-effect audio cues with readable non-audio state equivalents.
    - [x] Add a quiet, project-owned wind ambience bed for each local player in normal generated-world play; preserve the existing readable weather/exposure state.
    - [x] Add local water, fire, and biome ambience layers without revealing hidden content or implying a route.
      - [x] Add an original local water bed only near line-of-sight sea or visible lake water; preserve readable state text and avoid gameplay/network state.
      - [x] Add a quiet local fire bed only for a nearby visible, lit hearth; retain readable hearth state.
      - [x] Add local biome ambience from the owning player's sampled, locally visible biome without implying a route.
    - [x] Add local weather/exposure cues from accepted replicated state while retaining Wet, warmth, shelter, hearth, and recovery text.
    - [x] Add one quiet owner-local support acceptance cue from the existing server-confirmed feedback serial; keep readable support result text.
    - [x] Add local interaction/gathering, combat, discovery, and support-effect cues from their existing accepted feedback; retain readable result text and current privacy boundaries.
      - [x] Add a local combat result cue from the owning pawn's existing server-confirmed combat feedback serial; retain readable HIT/DEFEAT/UNAVAILABLE text.
      - [x] Add local interaction/gathering result cues from their existing accepted feedback; preserve readable interaction and inventory result text.
      - [x] Add a local discovery acknowledgement cue from existing owner-only, server-confirmed landmark/scroll feedback; preserve readable discovery result text.
      - [x] Add effect-specific support activation and expiry cues from existing accepted effect state; preserve readable support result text.
    - [x] Add local movement and traversal cues from the owning player's current movement state; retain readable pose, HUD, and input feedback.
      - [x] Add quiet owner-local footfall, jump, and landing cues from sampled movement state; retain readable pose, HUD, and input feedback.
      - [x] Add local water-entry and exit cues from the owning player's generated-ocean movement mode; retain readable water and exposure state.
- [x] Add optional onboarding and tutorial beats.
  - [x] Define the route-free local prompt beats, visibility triggers, accessibility cues, and authority boundaries. See `docs/13-onboarding-and-tutorial.md`.
  - [x] Add a no-build contract check for the tutorial specification and document its limits. See `Scripts/Verify-OnboardingContract.ps1`.
  - [x] Teach movement, gathering, shelter, weather, optional combat, discoveries, and support magic through route-free local prompts.
  - [x] Verify tutorial prompts never require a fixed route, authored corridor, mandatory camp, quest chain, or developer command.
- [x] Complete local settings and accessibility coverage.
  - [x] Define local option groups, keyboard/controller access, text scale, contrast, non-colour feedback, persistence, and authority boundaries. See `docs/14-settings-and-accessibility.md`.
  - [x] Add a no-build contract check for the local settings/accessibility specification and document its limits. See `Scripts/Verify-SettingsAccessibilityContract.ps1`.
  - [x] Validate the existing keyboard/controller input baseline used by the future Controls tab without changing runtime bindings. See `Scripts/Verify-LocalInputContract.ps1`.
  - [x] Add local audio, control/remapping, text-scale, contrast, and non-colour feedback options to the existing settings shell.
    - [x] Add persisted local master-volume steps and reversible mute/restore controls with visible text values.
    - [x] Add local ambient, music, and interaction/combat feedback category levels while preserving readable state text.
    - [x] Add local keyboard/controller control remapping and restore defaults.
    - [x] Add local text scale and contrast choices with readable modal layout.
    - [x] Add local colour-independent feedback preferences for current gameplay state.
  - [x] Verify options persist locally, remain usable with keyboard/controller input, and never mutate server gameplay or replicated state.
- [x] Run the performance and startup pass.
  - [x] Profile packaged startup, generated-world traversal, population activation, weather/camp updates, replication, and map/minimap workers on the supported Windows target.
    - Passed 2026-09-22: forced UE5.8 editor build; archived Windows Development package; packaged listen readiness 12,025.8 ms; `Scripts/Verify-WorldProfile.ps1 -Port 18474`; `Scripts/Verify-PlayerControls.ps1 -Port 18478`; `Scripts/Verify-CampChoices.ps1 -Port 18479`; `Scripts/Verify-WorldMapProfile.ps1 -Port 18475`; and focused minimap/map performance automations. Evidence retained under `C:/Users/Ville/AppData/Local/Temp/KalmalaPackagedStartup-ee433834eb0448b69850c424e32f1fce`, `C:/Users/Ville/AppData/Local/Temp/KalmalaWorldProfile-02e679698cd140d7b9f9a42d7c00788b`, `C:/Users/Ville/AppData/Local/Temp/KalmalaPlayerControls-ae5a594d4b784b61a0f036c2bc7806a9`, `C:/Users/Ville/AppData/Local/Temp/KalmalaCampChoices-c5df507da1774d838485d4f21e3738f2`, `C:/Users/Ville/AppData/Local/Temp/KalmalaWorldMapProfile-19bcb9b22fab47fe8ba1b39c04abf9ef`, and `C:/Users/Ville/AppData/Local/Temp/KalmalaPerfAutomation-41b3a434bee14073bce8862f9d476108`.
  - [x] Fix regressions within existing bounded actor, memory, worker, and raster budgets without increasing world, population, or online-service scope.
    - Passed 2026-09-22: forced UE5.8 editor build; fresh `Scripts/Verify-WorldProfile.ps1 -Port 18480` and `Scripts/Verify-WorldMapProfile.ps1 -Port 18481`; focused minimap/map generation, presentation, and budget automations. No actor, memory, worker, raster, authority, or save regression was exposed, so no runtime fix was required.
- [x] Tune the survival, combat, creature, and support loop.
  - [x] Tune costs, cooldowns, durations, stamina/wetness penalties, creature pressure, rewards, and recovery so preparation creates choices without hard travel gates.
    - [x] Tune the server-owned Wet movement and sprint-stamina penalties to 0.92x and 1.15x while retaining the 120-second duration, 10-second rain trigger, and campfire recovery.
      - Passed 2026-09-22: forced editor build; focused Wet/status automation; `Scripts/Verify-PlayerControls.ps1 -WetStamina -Port 18482`; `Scripts/Verify-CampChoices.ps1 -Port 18483`; and the M5 documentation suite. Both peers retained server-owned state and the Wet penalty remained recoverable through normal fire/shelter choices.
    - [x] Tune the shared support activation cost to an 18-stamina dry base, applying the existing authoritative Wet multiplier so Wet costs 20.7 without changing cooldowns, durations, targets, rewards, or recovery rules.
      - Passed 2026-09-22: direct UE5.8 `KalmalaEditor Win64 Development` UnrealBuildTool run succeeded in 11 actions with `%LOCALAPPDATA%\UnrealBuildTool` access; `Kalmala.Gameplay.Status.Wet+Kalmala.Gameplay.Discovery.PlayerScopedPersistence` passed; all five M5 documentation contracts and `git diff --check` passed.
    - [x] Tune the committed player-combat recovery window from 0.42 to 0.36 seconds while preserving server-selected targets, fixed damage, replay gates, and relevant-peer action presentation.
      - Passed 2026-09-22: isolated UE5.8 `KalmalaEditor Win64 Development` build succeeded in 153 actions with `%LOCALAPPDATA%\\UnrealBuildTool` access; focused combat authority/rejection automations passed; `Scripts/Verify-CombatPeer.ps1 -Port 18484` passed the owner-only feedback, relevant defeat, and restart-persistence checks; all five M5 documentation contracts and `git diff --check` passed.
    - [x] Tune the shared support activation cooldown from 4.0 to 5.0 seconds while preserving the 18-stamina base, effect durations, server-selected targets, rewards, and non-damaging execution.
      - Passed 2026-09-22: forced UE5.8 `KalmalaEditor Win64 Development` build succeeded in 10 actions with `%LOCALAPPDATA%\\UnrealBuildTool` access; focused Wet/support persistence tests passed; `Scripts/Verify-M4VerticalSlice.ps1 -Port 18492` passed Mireling, boar, deer, support authority/non-damage, owner-only rewards, and matching-world persistence; documentation and diff checks passed.
    - [x] Tune Hearth Shield's server-owned protection window from 8.0 to 10.0 seconds while preserving its 40-point absorption, five-second cooldown, 18-stamina base, expiry clearing, and non-damaging execution.
      - Passed 2026-09-22: forced UE5.8 `KalmalaEditor Win64 Development` build; focused `Kalmala.Gameplay.Discovery.PlayerScopedPersistence`; `Scripts/Verify-M4VerticalSlice.ps1`; all five M5 documentation contracts; and `git diff --check` passed.
    - [x] Tune Bear's Vigor's server-owned support window from 8.0 to 10.0 seconds while preserving its 1.4x strength, 140-point stamina cap, five-second cooldown, no-refill/expiry behavior, and non-damaging execution.
      - Passed 2026-09-22: forced UE5.8 `KalmalaEditor Win64 Development` build recorded `Result: Succeeded` in `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`; `Scripts/Verify-M4VerticalSlice.ps1 -Port 18495` passed the Mireling, boar, deer, support authority/non-damage, owner-only reward, defeat-persistence, and matching-world learning checks; the five M5 documentation contracts, presentation ownership audit, and `git diff --check` passed. Evidence: `C:/Users/Ville/AppData/Local/Temp/KalmalaM4VerticalSlice-dd1ae5ede2e24d369a12138acb990f38`.
    - [x] Tune the server-owned Mireling melee repeat interval from 1.0 to 1.25 seconds while preserving its 10-point damage, 180 cm range, target selection, defeat, and reward rules.
      - Passed 2026-09-22: forced UE5.8 `KalmalaEditor Win64 Development` build recorded `Result: Succeeded` in `C:/Users/Ville/AppData/Local/UnrealBuildTool/Log.txt`; focused `Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle` passed with the exact 1.25-second assertion; a fresh `Scripts/Verify-MirelingPeer.ps1 -Port 18497` passed server pressure, target-free client rejection, relevant replication, owner-only reward, and restart persistence; `Scripts/Verify-M4VerticalSlice.ps1 -Port 18498` passed all three creature peers, support authority/non-damage, and matching-world learning persistence. Evidence: `C:/Users/Ville/AppData/Local/Temp/KalmalaMirelingBalance-830c89d64a704e0f8b99fa1211a062e9/wildlife.log` and `C:/Users/Ville/AppData/Local/Temp/KalmalaM4VerticalSlice-ca83ce4e5b3a42df87bf3aafb319b000`.
  - [x] Re-run authority, persistence, reconnect, and host/client checks after tuning; clients still provide intent only.
    - Passed 2026-09-22: rebuilt `KalmalaEditor Win64 Development`; `Scripts/Verify-M4VerticalSlice.ps1 -Port 18485`, `Scripts/Verify-PlayerControls.ps1 -WetStamina -Port 18488`, `Scripts/Verify-CampChoices.ps1 -Port 18489`, `Scripts/Verify-CombatPeer.ps1 -Port 18490`, and `Scripts/Verify-DiscoveryPeer.ps1 -Port 18491`; and the focused status/combat/discovery automation. Authority, owner privacy, relevant-peer replication, matching-world persistence, reconnect defeat/discovery state, Wet recovery, and client intent-only rejection all passed.
- [ ] Complete release regression and packaging verification.
  - Resolved 2026-09-22: the ocean-travel test now spawns a server-owned replicated deep-water ribbon to the existing seed-derived island endpoint instead of searching the current generator for an arbitrary nearby waypoint.
  - [x] Run the relevant automated, rendered host/client, reconnect, and current-generator regression suites and record retained evidence.
    - Passed 2026-09-22: disposable UE5.8 `KalmalaEditor Win64 Development` build; `Scripts/Verify-OceanTravel.ps1 -Port 18516` passed host/client fixture adoption, authoritative/predicted ocean entry, seeded-island arrival, and duplicate-free terrain-neighborhood audit; `Scripts/Verify-RegionalGeneration.ps1` passed repeated seed-418 determinism, seed-419 variation, host/client identity agreement, and fingerprint `7644800015248745432`; rendered `Scripts/Verify-PlayerControls.ps1 -Rendered -Port 18517`; `Scripts/Verify-M4VerticalSlice.ps1 -Port 18518`; and all five M5 documentation contracts passed. Evidence: `C:/Users/Ville/AppData/Local/Temp/KalmalaOceanTravel-64b0abb98881496cbf5169c2f0324a44`, `C:/Users/Ville/AppData/Local/Temp/KalmalaRegionalProof-d369797ca3fa49b8a49003dfed14a533`, `C:/Users/Ville/AppData/Local/Temp/KalmalaPlayerControls-da1927e7c6de4c5b92ab3d9d84b2f4eb`, and `C:/Users/Ville/AppData/Local/Temp/KalmalaM4VerticalSlice-b074b18eaf0b491caa5a2beee301008f`.
  - [x] Produce and smoke-launch a Windows Development package from the accepted revision without changing saved-data schemas or CI/release configuration.
    - Passed 2026-09-22: extracted accepted HEAD `9cd3662` into an isolated temporary checkout, ran the UE5.8 `RunUAT.bat BuildCookRun` Windows Development build/cook/stage/archive with `%LOCALAPPDATA%\\UnrealBuildTool` access, and smoke-launched the archived `Kalmala.exe` for 10 seconds with `-nullrhi -nosound -unattended`; the process remained alive with no immediate crash. Main-checkout `Saved/`, `Binaries/`, `Intermediate/`, and release configuration were untouched. Evidence: `C:/Users/Ville/AppData/Local/Temp/KalmalaRelease-511dae00c4734478a302d99ded81dc60/Archive/Windows/Kalmala.exe`.
  - [x] Attempt the dedicated-server playtest only with a server-capable UE 5.8 build; otherwise retain the documented Launcher-engine blocker.
    - Checked 2026-09-22: only `C:\Program Files\Epic Games\UE_5.8` is installed; `Engine\Build\InstalledBuild.txt` is present, no alternate UE 5.8 installation or `UnrealServer.exe` is available, and `docs/07-development-setup.md` plus the accepted decision log state that this Launcher distribution does not support dedicated-server targets. The retained `KalmalaServer` target was not invoked.
- [ ] Run the final M5 acceptance without developer tools.
  - [ ] Verify a fresh player can complete the documented 20–30 minute co-op loop in the packaged build with optional routes, matching peer state, and no hidden developer dependency. **BLOCKED 2026-09-22:** three packaged-launch attempts could not surface a native game window for normal input. The archived `Kalmala.exe` remained alive with `MainWindowHandle=0` in Windows session 8 after (1) a default launch, (2) `-windowed -ResX=1280 -ResY=720`, and (3) the normal `/Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=18600 -windowed -ResX=1280 -ResY=720` listen launch; each `cua.getState()` returned `apps=[]` with only the Codex in-app browser. No player-visible evidence or two-player acceptance was claimed.
  - [ ] Record remaining limitations, supported session modes, and the handoff for post-slice work.
