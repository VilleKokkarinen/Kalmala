# Technical architecture

## UE5 baseline

- Create a C++ UE5 project named `Kalmala`, using Enhanced Input, Common UI, Gameplay Ability System (GAS), and Online Subsystem interfaces.
- Use source control and Unreal-friendly ignore rules before creating content.
- Target a current supported UE 5.x version chosen during project bootstrap; record the exact version in the decision log.
- Prefer Data Assets/Data Tables for tunable game content; avoid gameplay constants scattered through Blueprints.

## Multiplayer contract

The server owns player state, inventories, construction, damage, AI decisions, simulation state, loot, and progression. Clients send intent through validated server RPCs and receive replicated state. Clients may predict responsiveness only where reconciliation is supported.

| System | Authority | Replication approach |
| --- | --- | --- |
| Movement | UE Character Movement server authority | built-in movement replication |
| Interaction | server reruns a short trace from the owning pawn | client sends intent only; no target actor is trusted |
| Abilities | server validates activation and outcomes | GAS replication/prediction where suitable |
| Inventory/crafting | server | replicated components / owner-only detail |
| Building | server | replicated building actors; save stable IDs |
| AI | server | replicate actor state, not decision logic |
| Element grid | server | replicate sparse changes near relevant players |
| World identity | server | replicate immutable `WorldSeed` and `GeneratorRevision` through `GameState` |
| Terrain surface | shared seed function | convert Elevation to continuous height and normal for terrain, collision, and server-selected spawns |
| Terrain activation | server | initialize an invisible 3x3 neighborhood of continuous terrain patches around the generated start, placing every replicated patch actor at its deterministic patch centre; then deduplicate player-neighborhood activation at a one-second interval up to 25 patches; activation cells never define biome or gameplay boundaries |
| Terrain rendering | client cosmetic | derive one continuous local mesh from the replicated identity and patch descriptor; mesh geometry is never replicated |
| Biome debug material | local developer cosmetic | with `-KalmalaBiomeDebug`, replace the generated terrain material with classifier-driven vertex colours from the replicated identity and patch descriptor; it changes no terrain, collision, or gameplay state |
| Terrain collision | server | create collision from the server's same continuous terrain mesh; clients derive matching local collision only for prediction, never as authority |
| Surface water | client cosmetic | clip sea-level water against the same linear terrain triangles from the replicated identity and patch descriptor, retaining partially submerged areas |
| Shimmering Lakes treatment | client cosmetic | derive collision-free lake-water and shoreline meshes from the replicated identity, lake classification, and continuous terrain height; no lake geometry or physics state is replicated |
| Generated player start | server | resolve a Meadow-preferred seed-specific transform above the sampled terrain height, then place each pawn with its actual collision half-height plus server clearance after the terrain patch collision exists |
| Meadow rocks | client cosmetic | derive non-interactable low-poly procedural rocks from the replicated identity, terrain sample, and biome classification |
| Meadow trees | client cosmetic | derive non-interactable low-poly procedural trunks and canopies from the replicated identity, terrain sample, and biome classification |
| Gameplay population layout | server | activate a bounded set of invisible spatial keys around pawns, then spawn replicated server-owned harvest nodes, minimal wildlife spawns, and minimal hazard spawns from deterministic per-kind, field-informed descriptors; each carries a stable spatial ID for sparse server persistence; clients never select gameplay placements or defeat outcomes |
| Environmental exposure | server | sample ambient temperature, precipitation, wind exposure, and shelter for each pawn; update clamped wetness and warmth at a fixed server interval; replicate the resulting state for display only |
| Biome expansion | server | `FKalmalaBiomeExpansionContract` supplies deterministic per-biome terrain-feature intent, bounded population multipliers, normalized exposure modifiers, and stable discovery candidates. The completed Shimmering Lakes slice applies its profile only in lake-classified spatial keys and pawn samples, then materializes at most one dry, water-adjacent harvest discovery per active lake key. The Elderwood slice applies its profile only in Elderwood-classified keys and pawn samples, materializes at most one gently sloped lower-flora clearing discovery per active key, and locally derives dense field-driven canopy and roots with natural clearings. The Mossy Mire slice applies its profile only in Mire-classified keys and pawn samples, applies bounded server-owned footing drag through the existing replicated exposure state, and materializes at most one gently sloped relatively dry hummock harvest discovery per active key. All use the normal validated harvest path and persist only sparse depletion deltas. Clients receive no candidate list or discovery location. |
| Companion minimap | client UI | `UKalmalaMinimapViewModel` derives a local terrain/water sample grid from the replicated world identity and owning pawn transform; `UKalmalaMinimapSubsystem` creates a local top-right circular presentation that draws only in-circle terrain/water samples and a centred facing marker. Local mouse-wheel input adjusts a session-only view radius between tunable 2,500–10,000 cm bounds only while CommonUI allows normal game input; a modal UI retains wheel ownership. The UI never reveals hidden server-owned content. |
| Cosmetics | client | derive from replicated state/events |

## Water presentation

Water presentation uses `FKalmalaWaterSurfaceMesh` to clip the shared terrain triangles at sea/lake level and, for lakes, the existing field-classifier boundaries. It retains partially wet cells and interpolates intersections on shared patch edges. Shore treatment covers only terrain within 12 cm below the lake surface, not every cell or biome edge. There are no additional water collision surfaces, gameplay fields, replicated values, or saved data. The configured startup map supplies lighting without Unreal's template landscape, which otherwise intersects the generated terrain.

## Companion minimap presentation

The companion minimap uses a `ULocalPlayerSubsystem` per local player and rebuilds its widget/input binding when that player's controller changes. Viewport sizing and positioning precede anchoring because UE 5.8 resets anchors in both setters. A transient 129x129 sRGB texture fills the circle with world-anchored original biome patterns, bilinear filtering, and a transparent feathered edge; sea-level water and inland lake water override land treatment. This is disposable presentation of the existing classifier, not a new world-generation field or persisted biome map. Unchanged position/radius/world identity reuses the raster; facing still refreshes. Texture uploads update an existing GPU resource. The configured CommonUI viewport client routes modal input; Menu mode always retains wheel ownership, including menus with captured previews. No discovery/landmark visibility contract exists yet, so the minimap does not query or draw harvest nodes, hidden discoveries, hazards, or other players.

## Module boundaries

- `KalmalaCore`: tags, logging, shared data types, save interfaces.
- `KalmalaGameplay`: character, GAS, inventory, crafting, construction, interaction.
- `KalmalaWorld`: world generation, element grid, weather, harvesting, AI encounter logic.
- `KalmalaUI`: Common UI screens and view models.
- `KalmalaServer`: dedicated-server configuration and server-only services.

Do not make UI call world actors directly. Use components, interfaces, gameplay messages, or subsystem APIs.

## Environmental exposure contract

Each pawn has server-owned, replicated display state: ambient temperature, precipitation intensity, wind exposure, wetness, warmth, and shelter. Inputs are normalized to `[0,1]` except ambient temperature; wetness and warmth are clamped to `[0,100]`. On each fixed server tick, precipitation and wind increase wetness and reduce warmth, while shelter reduces both effects; future fire warmth is an additional server input. Clients never submit or simulate inputs or outcomes.

The server derives ambient temperature from the continuous Temperature field. Ground wetness combines Humidity with a continuous low-ground contribution and a short deterministic Shimmering Lakes adjacency probe; wind exposure combines continuous ridge elevation and terrain slope, then subtracts continuous Flora-derived natural cover. Server weather supplies precipitation and wind strength. These inputs are sampled at the pawn's server transform; they are environmental conditions, never authored zones or client-selected values. At a fixed one-second server interval, exposure advances wetness and warmth, replicates both with a warmth-derived travel-speed multiplier, and applies that multiplier through Character Movement. Cold, wet, wind-exposed travel can reduce movement to 68% at minimum warmth; this is reversible, not a hard gate. Sheltered players dry and recover slowly once dry, and a nearby lit campfire materially accelerates both recovery paths.

Shelter is also sampled by the server at the pawn. Natural cover supplies continuous partial shelter from Flora. A player-built roof contributes only when an overhead `ECC_Visibility` trace hits collision geometry tagged `KalmalaShelterRoof`; a player-built windbreak contributes only when a trace toward the current wind source hits collision geometry tagged `KalmalaShelterWindbreak`. Construction must add those tags only after server-authoritative placement. The sampler never queries authored shelter volumes, accepts client hit results, or treats untagged terrain/level geometry as shelter.

The campfire seam is a replicated, server-owned actor. Only a validated server interaction may light dry fuel; the server advances fuel wetness from the replicated weather's precipitation and wind, replicates the resulting lit state and effective warmth, and extinguishes soaked fuel. Its nearby warmth contribution is a server-readable falloff value for the later per-pawn exposure update; clients only render the replicated firelight and cannot submit fuel wetness, weather, or warmth.

`FKalmalaCampConditionSampler` evaluates any freely chosen position from the same deterministic terrain, exposure, lake, and harvest-descriptor contracts. It reports natural cover, ground wetness, bounded nearest-water distance, and nearby generated harvest-node availability for developer inspection and validation only. It does not spawn, reserve, reveal, or direct players to a camp site; `GameMode` invokes the optional inspection only on the server.

### Weather-cycle contract

`GameMode` advances a server-owned weather cycle and writes the current immutable-in-session state to the replicated world `GameState`. A state contains a monotonically increasing `WeatherCycleIndex`, server start time, duration, precipitation intensity, wind direction, and wind strength. `WeatherCycleIndex` is derived from no client input; the server selects it at session start as zero and increments it only after the active duration elapses. A late-joining client consumes the replicated active state rather than inferring it from local time.

Each state is deterministically derived from `WorldSeed`, `GeneratorRevision`, and `WeatherCycleIndex` using a dedicated weather sub-seed. It selects a duration from 120–240 server seconds, precipitation intensity in `[0,1]`, wind direction as a quantized yaw in `[0,360)`, and wind strength in `[0,1]`. The first weather increment uses dry/calm, drizzle, and rain outcomes; the resulting intensity and strength remain continuous values, so no biome becomes a hard weather zone. Restarting a local/listen-server session restarts the deterministic sequence at index zero; persisting mid-cycle weather is intentionally deferred until persistent world-time is introduced.

Only the server advances the index, computes values, or changes the replicated state. Clients use that state for presentation and their replicated exposure display, but cannot request a weather outcome, duration, direction, strength, or clock adjustment.

## Content conventions

- `/Game/Kalmala/Core`, `/Characters`, `/World`, `/Items`, `/Abilities`, `/UI`, `/Audio`, `/Maps`, `/Developer`.
- Prefixes: `BP_`, `WBP_`, `DA_`, `DT_`, `GA_`, `GE_`, `GCN_`, `IA_`, `IMC_`, `M_`, `MI_`, `T_`, `SK_`, `SM_`, `SFX_`.
- Use Gameplay Tags for semantic states (for example `State.Wet`, `State.Sheltered`, `Damage.Fire`, `Ability.Support.Mending`, `Effect.Shielded`).

## Persistence and online progression

Vertical slice persistence is a versioned `SaveGame` schema for local/listen-server testing. Isolate persistence behind interfaces so a later backend can replace it. Do not connect production identity, payments, analytics, or cloud databases until there is an explicit product decision.

Generated population saves store only sparse server deltas. `UKalmalaWorldPopulationSaveGame` records its schema version and immutable world identity, then separately records harvested and defeated stable spawn IDs; it never serializes the generated base population. During a session, `GameMode` owns this container, records harvests only after the server accepts them, and consults it before recreating a generated harvest node. Future wildlife and hazards must use the defeated set only after server-owned defeat validation and must consult it before activation.

The developer-only reconnect harness verifies both harvest and wildlife paths across two listen-server processes with the same world identity: it writes one accepted server delta, reloads the slot, and succeeds only when the corresponding deterministic spawn is not recreated.

## Biome-expansion shared contract

Each land-biome slice consumes `FKalmalaBiomeExpansionContract` rather than creating a parallel seed, placement, exposure, or persistence scheme. A profile supplies bounded terrain-feature intent, per-kind population multipliers, and normalized wetness, wind, and natural-cover modifiers. The current exposure simulation continues unchanged until a completed biome slice explicitly adopts its profile. Discovery candidates are deterministic, terrain-aligned server inputs with stable IDs derived from world identity, biome, and invisible spatial key; candidates are neither spawned, replicated, revealed, nor saved by this contract alone.

The Shimmering Lakes slice consumes that contract without adding water traversal physics or a boat requirement. Existing local terrain patches draw their continuous interlocking lake water and shore treatment. On the server, only lake-classified active spatial keys apply the bounded lake population profile and attempt a fixed deterministic search for one dry, water-adjacent discovery. The resulting replicated one-use harvest node uses the stable discovery ID and existing server-only interaction/depletion save path. A lake pawn's server exposure sample applies the lake wet-ground and reduced-cover modifiers before the unchanged shelter/fire response; clients only receive the resulting replicated exposure state.

The Elderwood slice consumes the same contract without a route, reserved camp, or authored clearing. Local terrain patches derive larger field-driven trees, canopy, and non-colliding root buttresses from the replicated identity; continuous Flora controls density so lower-flora pockets remain natural clearings. On the server, only Elderwood-classified active spatial keys apply the bounded population profile and may materialize one terrain-aligned, gently sloped lower-flora harvest discovery. A server pawn in Elderwood receives the profile's increased natural cover and reduced wind exposure before the unchanged shelter/fire response. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node when relevant.

The Mossy Mire slice consumes the same contract without a crossing, route, or authored dry ground. Only Mire-classified active spatial keys apply the bounded population profile and may materialize one terrain-aligned, gently sloped, relatively dry hummock harvest discovery. A server pawn in the Mire receives its wetter profile through the unchanged shelter/fire response, then a bounded 12% footing drag through the existing replicated travel-speed state; minimum travel speed remains the existing 68%, so the ground is slower but never impassable. Dry hummocks are optional terrain-selected preparation sites for raised shelter and drainage choices, not reserved camps. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node and replicated exposure state when relevant.

`-KalmalaBiomeFeatureInspection` is a server-only developer switch. After a player joins, it logs the sampled biome, nearby classifier seam flag, profile values, and the non-materialized stable discovery candidate. It accepts no client location, creates no actor, and never directs a player toward a feature.

The sparse container must round-trip through `SaveGame` memory serialization before any slot-writing integration is added. The automated round-trip test verifies that immutable world identity and harvested IDs survive serialization without creating project `Saved/` output.

Magic-scroll discoveries and learned support effects are server-authoritative progression. Save stable scroll IDs and learned-effect IDs, validate scroll rewards once, and replicate only the effect state needed by other players (such as an active shield or stat boost), not private inventory detail.

## Prototype player presentation and movement

`AKalmalaCharacter` constructs a collision-free, nine-part original humanoid through `UKalmalaPlayerModelComponent`. Each rendering peer generates the same rigid geometry using existing project materials; velocity drives a simple limb swing and airborne pose. Dedicated servers skip the model. The character capsule remains the collision authority.

Space uses Character Movement's built-in single jump (500 cm/s vertical launch, 0.25 air control). Held Shift requests sprint through `UKalmalaCharacterMovementComponent`: saved moves preserve the intent in `FLAG_Custom_0`, restore it during prediction replay, and prevent combining moves across sprint transitions. The server decodes intent and applies its configured 1.5 multiplier to the existing exposure-adjusted walking speed, only while grounded and not crouching. Clients send no numeric speed or new RPC. Ignored local movement clears held sprint/jump. This adds no stamina or save-data contract.

## Verification minimum

Every feature needs an automated test where practical, plus a reproducible multiplayer test: host + one client or dedicated server + two clients. Profile before increasing simulation area, actor count, or replication frequency.

`-KalmalaExposureReplicationTest` is the Phase 4 host/client smoke path. It is server-configured only: the server records its sampled weather, shelter, fire contribution, and exposure result; the client logs only replicated weather, campfire, and exposure state. It starts each verification pawn wet and low on warmth beside a temporary server-owned fire so the logs show recovery without changing persistent world data.

`-KalmalaCampChoiceTest` adds a non-shipping two-player scenario through the existing GameMode exposure tick. Its server-only fixtures compare two separated naturally generated camp conditions, initialize wet/cold pawns, and light temporary fires through the normal authority/range gate after an unprepared interval. No client supplies a location, environmental input, lighting result, or exposure value. Log comparison uses replicated player IDs because actor instance names can differ between peers. The harness adds no gameplay path, camp recommendation, construction piece, or save contract.
