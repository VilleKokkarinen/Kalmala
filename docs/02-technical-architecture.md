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
| Terrain activation | server | initialize an invisible 3x3 neighborhood of continuous terrain patches around the generated start, then deduplicate player-neighborhood activation at a one-second interval up to 25 patches; activation cells never define biome or gameplay boundaries |
| Terrain rendering | client cosmetic | derive one continuous local mesh from the replicated identity and patch descriptor; mesh geometry is never replicated |
| Terrain collision | server | create collision from the server's same continuous terrain mesh; clients derive matching local collision only for prediction, never as authority |
| Surface water | client cosmetic | derive a collision-free sea-level mesh over fully submerged terrain cells from the replicated identity and patch descriptor |
| Shimmering Lakes treatment | client cosmetic | derive collision-free lake-water and shoreline meshes from the replicated identity, lake classification, and continuous terrain height; no lake geometry or physics state is replicated |
| Generated player start | server | resolve a Meadow-preferred seed-specific transform at the sampled terrain height, then spawn pawns there |
| Meadow rocks | client cosmetic | derive non-interactable low-poly procedural rocks from the replicated identity, terrain sample, and biome classification |
| Meadow trees | client cosmetic | derive non-interactable low-poly procedural trunks and canopies from the replicated identity, terrain sample, and biome classification |
| Gameplay population layout | server | activate a bounded set of invisible spatial keys around pawns, then spawn replicated server-owned harvest nodes, minimal wildlife spawns, and minimal hazard spawns from deterministic per-kind, field-informed descriptors; each carries a stable spatial ID for sparse server persistence; clients never select gameplay placements or defeat outcomes |
| Environmental exposure | server | sample ambient temperature, precipitation, wind exposure, and shelter for each pawn; update clamped wetness and warmth at a fixed server interval; replicate the resulting state for display only |
| Cosmetics | client | derive from replicated state/events |

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

The sparse container must round-trip through `SaveGame` memory serialization before any slot-writing integration is added. The automated round-trip test verifies that immutable world identity and harvested IDs survive serialization without creating project `Saved/` output.

Magic-scroll discoveries and learned support effects are server-authoritative progression. Save stable scroll IDs and learned-effect IDs, validate scroll rewards once, and replicate only the effect state needed by other players (such as an active shield or stat boost), not private inventory detail.

## Verification minimum

Every feature needs an automated test where practical, plus a reproducible multiplayer test: host + one client or dedicated server + two clients. Profile before increasing simulation area, actor count, or replication frequency.
