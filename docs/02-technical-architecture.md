# Technical architecture

Use the baseline sections below for module/network boundaries. The [consolidated contracts](#consolidated-system-contracts) preserve current system details from former docs 08–47:

- [World generation](#world-generation), [harvest sources](#harvest-source-catalogue), and [ocean travel](#ocean-travel).
- [Tools/progression](#tools-and-station-progression), [camp services](#camp-crafting-and-station-services), and [combat/support](#combat-and-support-magic).
- [Discoveries](#optional-land-discoveries), [save migration](#save-identity-and-migration), and [shared inventory](#shared-inventory-and-hotbar).
- [Theme](#shared-ui-theme), [icons](#status-and-catalogue-icons), [details](#item-and-recipe-details), and [browsing](#menu-browsing).
- [Notifications](#owner-notifications), [recipe activity](#recipe-and-build-activity), and [onboarding reference](#onboarding-reference).
- [Settings/accessibility](#settings-and-accessibility), [presentation ownership](#presentation-ownership), and [audio](#audio-cues).

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
| World identity | server | replicate immutable `WorldSeed` through `GameState` |
| Terrain surface | shared seed function | convert Elevation to continuous height and normal for terrain, collision, and server-selected spawns |
| Terrain activation | server | initialize an invisible 3x3 neighborhood of continuous terrain patches around the generated start, placing every replicated patch actor at its deterministic patch centre; then refresh the bounded union of server-observed player neighborhoods at a one-second interval up to 25 patches, retiring patches outside every player neighborhood so open-ocean travel can continue without increasing streaming density; activation cells never define biome or gameplay boundaries |
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
| Biome expansion | server | `FKalmalaBiomeExpansionContract` supplies deterministic per-biome terrain-feature intent, bounded population multipliers, normalized exposure modifiers, and stable discovery candidates. The completed Shimmering Lakes slice applies its profile only in lake-classified spatial keys and pawn samples, then materializes at most one dry, water-adjacent harvest discovery per active lake key. The Elderwood slice applies its profile only in Elderwood-classified keys and pawn samples, materializes at most one gently sloped lower-flora clearing discovery per active key, and locally derives dense field-driven canopy and roots with natural clearings. The Mossy Mire slice applies its profile only in Mire-classified keys and pawn samples, applies bounded server-owned footing drag through the existing replicated exposure state, and materializes at most one gently sloped relatively dry hummock harvest discovery per active key. The Freezing Tundra slice applies its sparse-cover, stronger-wind profile only in Tundra-classified keys and pawn samples, and materializes at most one exposed, gently rolling high-ground harvest discovery per active key. The Thunder Mountains slice applies its high-wind profile only in Mountain-classified keys and pawn samples, and materializes at most one steep-but-traversable storm-carved overlook discovery per active key. All use the normal validated harvest path and persist only sparse depletion deltas. Clients receive no candidate list or discovery location. |
| Companion minimap | client UI | `UKalmalaMinimapViewModel` derives a local terrain/water sample grid from the replicated world identity and owning pawn transform; `UKalmalaMinimapSubsystem` creates a local top-right circular presentation that draws only in-circle terrain/water samples and a centred facing marker. Local mouse-wheel input adjusts a session-only view radius between tunable 2,500–10,000 cm bounds only while CommonUI allows normal game input; a modal UI retains wheel ownership. The UI never reveals hidden server-owned content. |
| Settings menu | client UI | `UKalmalaSettingsSubsystem` binds local Escape input and opens/closes a local `UKalmalaSettingsWidget`. The main screen provides Options and safe engine quit; Options presents Video, Audio, Controls, and Settings tabs. Video persists `UGameUserSettings` resolution, V-Sync, window mode, and scalability view-distance quality. Settings also stores local text scale, whole-interface scale over the project DPI/ApplicationScale defaults, contrast, feedback, and reduced motion in the existing local `GameUserSettings` config. These choices affect only presentation, are never replicated, and cannot mutate gameplay authority or save schemas. |
| Inventory menu | client UI with validated server slot intent | UKalmalaInventoryMenuSubsystem owns the Tab/I panel and an upper-left gameplay hotbar for each local player. One 10×4 runtime grid contains item stacks and tools; its numbered top row is the hotbar. Grid positions and active identity replicate owner-only. Drag or keyboard/controller moves send source/target indices and expected identities for server validation; quantities, grants, repairs and food outcomes remain server-owned. The panel shows armor/weight beneath the grid and retains selected-item details and existing food/repair actions. The gameplay bar displays only occupied cells in 1–9, 0 order and hides during modals. Tool metadata and save schemas retain their established contracts; arrangement is session state. See `02-technical-architecture.md`. |
| Expanded world map | client UI plus bounded co-op relay | `UKalmalaWorldMapSubsystem` owns one M-toggle local map overlay per local player. `UKalmalaWorldMapWidget` uses bounded 10,000 cm world-space terrain/water tiles generated asynchronously from the immutable identity. The 33×33 tiles share edge samples, and each view/identity change drops the previous local tile set before requesting at most 64 prioritized tiles; stale workers retain their prior epoch and are never polled or uploaded into the new view. Drag panning, cursor-anchored wheel zoom, and recentering only alter local presentation. Escape closes the map before opening settings. Personal fog and pins remain independent identity/local-player-scoped saves. The only map network path is explicit owner co-op consent plus server-validated temporary pings; neither makes terrain, discoveries, player annotations, or gameplay state authoritative. |
| Cosmetics | client | derive from replicated state/events |

The expanded map's personal-pin interaction is local UI state. Shift-click opens a keyboard label draft for a player-selected map position; only the finite Cairn, Lantern, and Thread presentation styles and sanitized 1–32-character labels are accepted. Completion, visibility, and removal remain local UI actions. Pin text explicitly states its style, label, completion, and visibility so state is not colour-only, and a persistent selected-pin text line remains visible even when the selected marker is hidden. `Tab` selects each pin, including hidden pins; `Enter`, `H`, and `Delete` toggle completion, visibility, and removal, while `P` begins centred placement. Gamepad face buttons offer selection, placement/toggle, visibility/cancel, and recenter actions; left trigger removes the selected pin, while D-pad and shoulders provide map pan/zoom. A version-1 independent local save retains the newest 256 valid pins only when its immutable seed and local-player index match; a mismatched slot is discarded. Pins do not query actors or discoveries, send an RPC, claim world content, or become server gameplay instructions.

Pointer coordinates use an explicit inverse of the world-to-map transform and are regression-tested at the minimum, standard, and maximum map zoom. When visible pins overlap, selection is deterministic: the newest rendered marker wins; if it is hidden, the next visible marker becomes selectable. The map widget deliberately takes local keyboard focus when opened so keyboard/controller pin actions cannot leak into gameplay input. These checks exercise only local widget and save-memory state; they create no network request or gameplay mutation.

### Expanded-map privacy, accessibility, and authority contract

The local map derives terrain/water tiles only from the immutable replicated `WorldSeed` and the owning pawn's normal replicated transform. It never queries population, loot, hazards, discoveries, or remote personal-map data. Current sight clears only a 6,500 cm circle around that owning pawn; coarse 500 cm personal cells retain at most 8,192 locally saved entries under seed and local-player index. A mismatched identity starts fresh. Personal memory uses sea-glass teal; lichen-ember remains a reserved, deliberately data-less treatment for future shared cartography. The 64-tile cache is bounded to 278,784 CPU pixel bytes at its 33×33 sample size. Open/pan/zoom/recenter and map input stay local, and the map must be profiled before increasing tile density or range.

Pins are private, finite local annotations: Cairn, Lantern, and Thread styles; sanitized 1–32-character labels; and at most 256 newest valid entries in a separate version-1 local save. Shift-click places, click completes, Ctrl-click hides, and right-click removes. Text always communicates style, label, completion, and visibility independently of colour. `Tab` includes hidden pins in selection; `Enter`, `H`, `Delete`, and `P` control completion, visibility, removal, and centred placement. Arrow keys/Page Up/Page Down and D-pad/shoulders pan/zoom; face buttons select/place/toggle/visibility/recentre and left trigger removes. The map uses local keyboard focus while open. Screen-reader integration and remapping remain later accessibility work.

Co-op awareness is private by default and resets off when connecting or reconnecting. An owner may opt in with `C` or the gamepad Menu button. Only mutually opted-in, connected, non-spectator players can see each other's normally replicated pawn transforms, and a peer marker or temporary ping is drawn only inside the recipient's own current/revealed coverage. Middle-click or `Q`/right-stick click requests a centre/point ping, but the server alone rejects unauthorized, malformed, out-of-bounds, or over-65 m locations and applies a two-second cooldown. Accepted pings have a six-second server-time lifetime, a deterministic sender/sequence order, at most 16 owner-only inbox entries, and are removed immediately when consent or eligibility ends. No remote exploration, pin, coverage, discovery, or persistent waypoint is exchanged. Construction-gated shared cartography is intentionally absent until M2 supplies its server-owned construction and persistence prerequisites.

## Water presentation

The classifier and terrain-height sampler share `FKalmalaRegionalGeneration`. Samples carry seed/position metadata and four environmental fields. Weights and shaping are recomputed; only derived crop transforms and bounded hydrology splines are cached. Variable inland water clips against final collision triangles, and local maps interpolate depth on that lattice.

Biome classification always uses the current generator. World identity contains only the immutable server-owned `WorldSeed`. Production tuning changes may change existing seed layouts during development. Current saves use seed-only slots; former version-specific slots are neither loaded nor deleted.

The generator uses `FKalmalaMasterMap`: an independent fixed-seed 128 km land/water atlas, sampled through a game-seeded crop and rotation at the existing 16 km world radius. Its sign constrains Elevation and final ocean/land terrain; existing game-seeded Perlin fields still provide relief and environmental suitability. Special land biomes compete above hard origin-distance minima: Lakes 0.35–3 km, Elderwood at least 0.75 km, Mire 3–16 km and Tundra at least 4 km. The inclusive 350 m centre is Meadows/Ocean only. Meadows ends at 4 km; Mountains is the remaining land fallback beyond it and the elevated fallback outside the centre. Smooth eligibility ramps stay entirely within allowed distances. Lakes and Mire share all regional, climate, relief and basin parameters; only their distance ranges differ. See `02-technical-architecture.md` for exact precedence and tuning.

The master seed is a compiled constant. Crop transforms derive from the game seed. Rivers remain enabled and small streams are disabled. Authority, population IDs, collision and maps derive from the same server-owned seed and current build.

The developer `ExportWorldMaps` commandlet can temporarily install thread-local preview tuning, with installation rejected outside headless commandlets. Each render restores defaults and invalidates derived crop/hydrology caches. Its fast biome query shares the normal regional/basin classifier while skipping hydrology and height-only work; default output is checked against full sampling. A warm parameter-file watcher writes PNGs without starting gameplay. This adds no runtime console setting, client authority, replicated property, production-layout setting or save change; see `07-development-setup.md` for the workflow.

Water presentation uses `FKalmalaWaterSurfaceMesh` to clip the shared terrain triangles at sea/lake level. `FKalmalaLakeBasin` first floods the six-connected 125 cm terrain lattice below lake level, accepting only enclosed components containing a lake-biome seed. Biome boundaries never cut an accepted lake: water extends to the physical banks. Connections to sea level and components exceeding 8,192 wet vertices are conservatively omitted rather than truncated. A bounded identity/lattice-origin cache reuses component decisions across streamed patches. The minimap uses this same visible-basin decision and interpolated terrain surface; existing server wetland/exposure/discovery sampling remains unchanged. It retains partially wet cells and interpolates intersections on shared patch edges. Shore treatment covers only terrain within 12 cm below the lake surface, not every cell or biome edge. There are no additional water collision surfaces, gameplay fields, replicated values, or saved data. The configured startup map supplies lighting without Unreal's template landscape, which otherwise intersects the generated terrain.

## Companion minimap presentation

Phase 7 begins with `FKalmalaOceanSampler`: a derived sea-depth query using the same 125 cm lattice and triangle diagonal as terrain collision and clipped ocean water. The normal overload derives the lattice origin from the immutable world's generated start and caches only that origin by identity. The explicit-origin overload supports patch callers. It returns validity, interpolated terrain height, and nonnegative depth below the existing zero-height sea surface; dry ground and the exact shore have zero depth. Invalid identity and nonfinite coordinates are rejected. The minimap now consumes this triangle height and sea-depth result instead of sampling curved Perlin height between mesh vertices, so its ocean coastline follows the visible mesh. Inland lake presentation continues through the enclosed-basin query.

This query adds no world field, terrain deformation, island placement, collision volume, RPC, replicated value, or saved data. `FKalmalaIslandLocator` remains a developer-only query over existing emerging seed terrain and never reserves or creates content. The first gameplay consumer is `UKalmalaCharacterMovementComponent`: server and predicting owner sample the replicated immutable identity at the pawn's current position, enter its generated-ocean custom mode at 100 cm sea depth, and return to normal walking below 75 cm. The server remains authoritative through normal Character Movement replication and correction; the client supplies neither a depth nor a requested mode. The custom mode floats the existing pawn capsule at the zero-height visible sea and retains swept collision with the shared terrain mesh. Server terrain activation retires patches outside the bounded union of all player neighborhoods before activating needed ones, so ordinary long-distance sea movement refreshes collision/rendering without expanding the 25-patch budget. `Verify-OceanTravel.ps1` confirms a current-generator host and conflicting-seed client independently derive the same existing island, enter ocean, and arrive over normal replicated movement; its travel target and temporary pawn-collision relaxation are verification-only. Boats and inland lake-depth gameplay remain future Phase 8 work.

The companion minimap uses a `ULocalPlayerSubsystem` per local player and rebuilds its widget/input binding when that player's controller changes. Viewport sizing and positioning precede anchoring because UE 5.8 resets anchors in both setters. A transient 129x129 sRGB texture fills the circle with world-anchored original biome patterns, bilinear filtering, and a transparent feathered edge; sea-level water and inland lake water override land treatment. This is disposable presentation of the existing classifier, not a new world-generation field or persisted biome map. Unchanged position/radius/world identity reuses the raster; facing still refreshes. Texture uploads update an existing GPU resource. The configured CommonUI viewport client routes modal input; Menu mode always retains wheel ownership, including menus with captured previews. No discovery/landmark visibility contract exists yet, so the minimap does not query or draw harvest nodes, hidden discoveries, hazards, or other players.

The expanded map places a local fog texture over all generated terrain/water tiles and clears a fixed 6,500 cm circle around the owning pawn's normal replicated transform. It records the revealed 500 cm coverage cells in a separately versioned, bounded (8,192-cell) local `SaveGame`, keyed by the immutable seed and local-player index; a mismatched identity is never reused. This is separate from server-owned generated-world sparse saves and stores no terrain classification, actor, discovery, or gameplay data. Current sight is fully clear; remembered personal coverage carries a translucent sea-glass teal treatment. A contrasting warm lichen-ember palette is reserved for a future explicitly opted-in shared-cartography source, but this phase does not create, load, query, or render shared coverage. Map pan and zoom merely transform current/personal coverage into the view; neither can widen it, choose another player, issue an RPC, or query actors. Opaque fog pixels contain no terrain, water, landmark, population, or discovery treatment.

World-map tiles and the companion minimap use the same world-coordinate terrain/water sampler and RGB raster. The dimensioned raster is fully opaque even when square; only the HUD crop applies a circular alpha mask. Tile and fog drawing share the map-panel clip, preventing terrain outside the panel from disclosing unexplored surroundings. The local-player subsystem creates the collapsed map before the first M press and records owning-pawn coverage every 0.5 seconds during gameplay, saving only changed coverage through the existing identity-scoped slot. Closed maps generate no terrain tiles or fog textures. Thus walking accumulates connected explored terrain independently of opening the map; pan and zoom never record exploration.

## M2 item catalogue

`UKalmalaItemCatalogue` and `UKalmalaRecipeCatalogue` load schema-version-4 data from `Content/Data/GameCatalogues.json`; their arrays are no longer configured in `DefaultGame.ini`. Each item has a clean external catalogue ID, player-facing name, bounded description, and stack limit. No property or value in the JSON contains the old "Kit" suffix. The loader maps clean buildable IDs and station arrays back to stable runtime item and construction IDs so placement and save compatibility remain unchanged. JSON holds recipe ingredients and outputs, batches, station alternatives in `RequiredStation`, hearth gates, fuel costs, and experience awards. Recipes do not declare skill-level requirements or output tools. The loader reads through Unreal's file layer, including packaged UFS builds; both catalogues fail closed if parsing, schema, bounds, cross-references, or duplicate-ID validation fails. `DirectoriesToAlwaysStageAsUFS=(Path="Data")` keeps the source file available in packaged builds.

The server's JSON-backed catalogue is the authority for inventory transactions; a client's copy may only inform presentation. `IsValidStack` rejects unknown/empty IDs and any quantity outside 1 through the configured stack limit. `CanAddToStack` permits zero existing quantity but requires a positive addition and sufficient capacity, using subtraction after bounds checks to avoid integer overflow. A missing/duplicate/invalid catalogue fails closed, with defensive ceilings of 64 item definitions, 32 recipes, and 999 units per stack. Definition lookup uses Unreal's case-insensitive `FName` identity. These pure validation functions grant no items and do not themselves establish caller authority: the inventory and crafting components still check server ownership, capacity, and transaction intent before mutation. No RPC, inventory replication, save schema, or harvest behavior is added by the data-source change.

## M2 player inventory

`UKalmalaInventoryComponent` is a replicated default subobject on each `AKalmalaCharacter`. Its private stack array uses `COND_OwnerOnly`; simulated remote pawns receive no inventory contents. Inventory starts empty and lasts only for the pawn lifetime. There is one stack per item ID, bounded by that item's server catalogue limit, and at most 16 occupied slots. Trusted server gameplay calls `TryGrantFromServer` or `TryConsumeFromServer`; both reject non-authority callers and invalid quantities before mutation. Failed operations leave contents unchanged; consumption removes an exhausted slot. There are no client add/remove/reorder/set RPCs. The existing interaction intent is the harvest entry point described below; crafting, inventory persistence, and reconnect restoration are not wired yet.

The on-demand `UKalmalaInventoryMenuWidget` reads only its local controller's pawn inventory and carried tools. It shows the owner's pack, equipment, item details and supported actions when the player opens Inventory; it is collapsed during normal gameplay and does not query remote inventories. Display catalogue data never controls transaction limits or authority. The former persistent pack HUD and its left-side panel subsystem were removed in M12.

Generated harvest nodes now grant one material through that inventory. The server derives Wood, Stone, or Fibre by `FCrc::StrCrc32(PersistentSpawnId) % 3` in that order, including existing discovery nodes; fuel and construction supplies remain later crafting outputs. Initialization is one-use and rejects non-harvest descriptors or non-finite locations. An interaction requires an initialized, available server node and an authoritative same-world character within 250 cm. The normal no-argument character RPC still performs the server view trace; clients cannot supply a target, item ID, or quantity. The complete catalogue/capacity-validated inventory grant precedes depletion, collision removal, and the existing sparse-save delegate. A full material stack leaves the node intact for retry; repeated accepted requests cannot grant twice. Stable IDs, generated placements, and sparse-save schemas remain unchanged. Pawn inventory is still transient, so persisted node depletion does not imply item restoration after reconnect.

## M2 workbench and storage

Accepted workbench/storage kits use the existing replicated construction actor and unchanged stable placement save. Visible workbenches within 250 cm satisfy camp kit assembly. Chest contents live only in a separate server-owned schema-1 save, bounded to 128 stable construction IDs with 16 unique catalogue-limited stacks each, under exact seed identity. The game mode retains both construction and storage save containers through reflected transient references. Invalid existing storage saves fail closed without overwrite.

An accepted construction-station interaction may publish an owner-only station-context event containing the exact replicated actor reference, its station kit, stable construction ID, and a bounded serial. This is presentation context only: the client cannot choose the target, and every recipe, repair, upgrade, and storage action continues to resolve and validate its own current server-side station, range, sight line, inventory, and requirements. Context events add no RPC payload, save field, or gameplay authority.

The owning crafting component accepts no-target inspect/close intents and item-ID-only one-unit deposit/withdraw intents. The server chooses the chest, validates controlled-pawn range and sight line plus its registered construction record, checks both inventory results, saves the candidate contents, and only then publishes the private pack. Failed validation/writes leave live containers unchanged. Owner-only snapshots refresh or expire every 0.25 seconds and clear when the menu closes; shared construction actors carry no contents. UI uses only this component seam. Carried inventory still resets with its pawn; storage persistence does not imply player persistence. Full controls, save limits, and verification are in `02-technical-architecture.md`.

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
## Bounded interaction-grid contract

M3 adds one server-owned, ground-aligned interaction grid; it is a local simulation budget, not a world map, streaming partition, navigation aid, or client prediction system. A cell key is the signed integer pair `(floor(WorldX / 200 cm), floor(WorldY / 200 cm))`. It has no client-selected Z coordinate: the server samples the generated terrain height at the key's 200 cm-square centre. The key is stable across an active session and can be derived again after reload from the same immutable `WorldSeed`; cells never determine terrain, biome, population, routes, or construction placement.

The authoritative baseline material comes only from that server terrain/material sample: `Ground`, `Vegetation`, `Stone`, or `ShallowWater`. `Vegetation` is flammable; `Ground`, `Stone`, and `ShallowWater` are not. A future placed-wood or metal interaction must be introduced as a separately validated server-owned material source; ordinary client construction actors do not rewrite grid material in M3. Each active cell carries bounded server state: material, a continuous temperature in `[0,100]` with display bands `Cold` (below 25), `Normal` (25-59), `Hot` (60-84), and `Burning` (85+), plus wetness in `[0,100]` with display bands `Dry` (below 25), `Damp` (25-74), and `Soaked` (75+). The bands are presentation labels, not alternative client simulation rules. All calculations clamp non-finite values before they become state.

Only `GameMode` activates cells. At each one-second server simulation tick it takes the union of a 9x9-cell square around every possessed, non-spectator pawn and a 7x7-cell square around every lit replicated campfire. It keeps at most 1,024 cells total, selecting campfire neighborhoods first and then pawn neighborhoods in ascending stable server actor identity, with duplicate keys removed before the cap. No cell is activated by a client map position, camera, cursor, requested target, or locally reported environmental value. A cell without an eligible source leaves the active set after the tick; its derived transient heat and wetness are discarded rather than simulated in the background.

The server alone initializes a newly active cell from the deterministic terrain/material baseline and current server weather/exposure inputs, advances rain, drying, heat, and any later bounded spread, and chooses every resulting material, temperature, wetness, ignition, and delta outcome. It may replicate a compact sparse snapshot only for active cells relevant to a receiving player's existing server-observed pawn neighborhood. That snapshot contains the key and authoritative state needed for nearby rendering; it never exposes inactive cells, unactivated terrain/material classification, remote camps, hidden population, or a complete grid. Clients interpolate or render received state only and must clear snapshots that leave normal relevancy; they do not initialize cells, infer spread, retain an authoritative cache, or feed grid values back into gameplay.

Persistence is deliberately sparse and versioned independently of generated population, construction, storage, and player UI saves. The future `InteractionGrid` save container will carry a schema version, immutable `WorldSeed`, and sorted cell-key deltas. It may save only a server-accepted gameplay-changing departure from deterministic baseline (for example, a later permanent consumed or charred material transition). Rain wetness, temperature, heat, active membership, and ordinary extinguishing are transient and are never saved. M3's initial rain/heat slice creates no permanent material transition, so it must neither write empty grid saves nor add a delta merely because a cell was wet or hot. A mismatched seed, unsupported schema, malformed key, invalid material transition, or non-finite/out-of-range value fails closed and cannot overwrite a valid save.

There is no client cell-state RPC. Existing interactions continue to submit only their established intent; if a later interaction can affect a cell, the server must derive the candidate key from a fresh authoritative trace or source actor, validate controlled-pawn range, sight, source eligibility, active membership, material, wetness, temperature, rate limit, and capacity, and then compute the result itself. Client payloads may never select a cell coordinate, material, temperature, wetness, ignition/spread result, persistence delta, or save identity. This preserves the existing weather, exposure, campfire, inventory, and construction authority seams.

### Weather-cycle contract

`GameMode` advances a server-owned weather cycle and writes the current immutable-in-session state to the replicated world `GameState`. A state contains a monotonically increasing `WeatherCycleIndex`, server start time, duration, precipitation and fog intensity, a server-derived storm intensity and activity tier, wind direction, and wind strength. `WeatherCycleIndex` is derived from no client input; the server selects it at session start as zero and increments it only after the active duration elapses. A late-joining client consumes the replicated active state rather than inferring it from local time.

Each state is deterministically derived from `WorldSeed` and `WeatherCycleIndex` using a dedicated weather sub-seed. It selects a duration from 120-240 server seconds, precipitation and fog intensity in [`0,1`], wind direction as a quantized yaw in [`0,360`), and wind strength in [`0,1`]. Rain, fog, and wind remain continuous rather than making a biome a hard weather zone. Restarting a local/listen-server session restarts the deterministic sequence at index zero; persisting mid-cycle weather is intentionally deferred until persistent world-time is introduced.

Only the server advances the index, computes values, or changes the replicated state. It derives storm intensity as normalized precipitation multiplied by wind strength, and publishes a matching Calm, Active, or Highly Active tier from fog, rain, wind, and storm thresholds. The server refreshes this enum before replication; clients can read it but cannot request a weather outcome, intensity, tier, duration, direction, strength, or clock adjustment.

### M7 bounded hazard descriptors

The replicated weather state now includes deterministic fog intensity, the normalized `StormIntensity` property, and the server-derived `ActivityLevel` enum. StormIntensity is derived from the authoritative rain/wind pair rather than accepted as a separate value. The server's per-pawn exposure snapshot also reports normalized heat and cold intensities from the current generated ambient temperature; values refresh with the existing one-second environmental update and reverse when the pawn or weather conditions change. Non-finite climate samples fail closed to zero intensity, and every reported intensity is bounded to [0,1].

The per-pawn heat/cold and normalized fog/storm descriptors remain summaries; raw intensity alone does not directly apply a penalty. The server-selected weather activity tier drives the bounded M7 exposed-rain rule below. Existing server rain/wetness, ambient-temperature/warmth, wind/shelter, and hearth rules derive the environmental state. The owner receives the pawn's existing replicated exposure state, and all peers receive the existing replicated world weather state. There is no new RPC, client mutation, save field, or persistence schema. Shelter and fire recovery and calm weather timing counter wetness and warmth; low warmth also reduces stamina recovery as described below.

The first M7 hazard outcome uses that same server-owned warmth result: the movement component recovers stamina at the normal 15 points per second at 40 warmth or above, scaling smoothly down to 12 points per second at zero warmth. Cold, wetness, wind and weather lower warmth through the existing exposure sampler; accepted shelter and lit-fire warmth restore it and return stamina recovery to normal. This is applied only by the authoritative stamina update, uses the already replicated exposure and stamina state, and adds no client input, RPC, status entry, or save field.

The second M7 hazard outcome shortens the existing unroofed-rain Wet trigger by 25% only when the authoritative weather tier is Highly Active: 7.5 uninterrupted seconds instead of the ordinary 10. Calm and Active retain the prior trigger. The server resets accumulated time when rain stops, the pawn enters water, or accepted roof shelter is found; standing water still applies Wet immediately. Existing lit-fire recovery and the 120-second maximum remain unchanged. The tier and timer are read or advanced only by the server; no client input, RPC, replicated field, or save schema is added. The third M7 hazard outcome uses storm intensity above the shared 0.65 Highly Active threshold to increase exposed campfire fuel wetting by up to 25%, ramping linearly to full rain/wind intensity. The campfire receives rain only after the server accepted-roof trace and wind only after its windbreak trace, so a roof removes rain pressure and a windbreak removes the storm surge. This updates the existing replicated fuel condition through the normal server tick without a new request, RPC, replicated field, or save schema.

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

The Freezing Tundra slice consumes the same contract without an authored ridge, camp, route, travel gate, or client-selected location. Only Tundra-classified active spatial keys apply the bounded sparse population profile and may materialize one terrain-aligned, gently rolling exposed-high-ground harvest discovery. A server pawn in the Tundra receives reduced natural cover and stronger wind exposure before the unchanged shelter/fire response, making a player-built enclosed roof and windbreak meaningful preparation rather than a mandatory site. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node and replicated exposure state when relevant.

The Thunder Mountains slice consumes the same contract without a designed passage, precision gate, authored ridge, or client-selected location. Only Mountain-classified active spatial keys apply the bounded mountain population profile and may materialize one terrain-aligned, steep-but-traversable storm-carved overlook harvest discovery. A server pawn in the Mountains receives stronger wind exposure during the existing server weather cycle before the unchanged roof/windbreak shelter response, so a player-built lightning-safe enclosure is useful preparation without reserving a site. The discovery follows the normal validated interaction and sparse depletion path; clients receive only the ordinary replicated node and replicated exposure state when relevant.

`-KalmalaBiomeFeatureInspection` is a server-only developer switch. After a player joins, it logs the sampled biome, nearby classifier seam flag, profile values, and the non-materialized stable discovery candidate. It accepts no client location, creates no actor, and never directs a player toward a feature.

The sparse container must round-trip through `SaveGame` memory serialization before any slot-writing integration is added. The automated round-trip test verifies that immutable world identity and harvested IDs survive serialization without creating project `Saved/` output.

Magic-scroll discoveries and learned support effects are server-authoritative progression. Save stable scroll IDs and learned-effect IDs, validate scroll rewards once, and replicate only the effect state needed by other players (such as an active shield or stat boost), not private inventory detail.

M13 represents each learned effect as a reusable item in the owner's transient inventory; reconstruct those items from the existing learned-effect entitlement when that inventory initializes. The numbered hotbar sends only its cell index, and the server resolves the item/effect and invokes the existing support activation gates. Each accepted cast starts a shared 300-second per-caster cooldown. Hearth Shield, Bear's Vigor, and Deer Call last 180 seconds; Mending remains instantaneous. Deer Call's bounded server-selected wildlife influence lasts 180 seconds. Only one support effect may be active per caster; the server rejects another cast while one remains active and does not stack or refresh effects. Scroll activation does not consume the item or add save fields; server-owned stamina, learned entitlement, and effect-specific validation remain in force.

## Prototype player presentation and movement

`AKalmalaCharacter` constructs a collision-free, nine-part original humanoid through `UKalmalaPlayerModelComponent`. Each rendering peer generates the same rigid geometry using existing project materials; velocity drives a simple limb swing and airborne pose. Dedicated servers skip the model. The character capsule remains the collision authority.

Space uses Character Movement's built-in single jump (500 cm/s vertical launch, 0.25 air control). Held Shift requests sprint through `UKalmalaCharacterMovementComponent`: saved moves preserve the intent in `FLAG_Custom_0`, restore it during prediction replay, and prevent combining moves across sprint transitions. The server decodes intent and applies its configured 1.5 multiplier to the existing exposure-adjusted walking speed, only while grounded and not crouching. Clients send no numeric speed or new RPC. Ignored local movement clears held sprint/jump. This adds no stamina or save-data contract.

## Verification minimum

Regional sampling shares warp and motion calculations, and rejects distant supports early. The minimap reuses collision vertices within each raster call; scratch data is discarded afterward. Classification and hydrology always use the current implementation.

Minimap raster sampling runs on the thread pool from value snapshots of the replicated identity, position, radius, and resolution. Each view model retains at most one pending job, polls without waiting, and coalesces movement into the next request. Completed results from an obsolete identity or zoom are discarded. Workers access no actors or UObjects; texture publication stays on the game thread. The last completed map remains visible during movement, so terrain presentation can briefly lag the pawn while generation finishes. Reinitialization drops the future without waiting or giving the worker an object reference. The existing hydrology cache remains protected by its mutex.

Every feature needs an automated test where practical, plus a reproducible multiplayer test: host + one client or dedicated server + two clients. Profile before increasing simulation area, actor count, or replication frequency.

`-KalmalaExposureReplicationTest` is the Phase 4 host/client smoke path. It is server-configured only: the server records its sampled weather, shelter, fire contribution, and exposure result; the client logs only replicated weather, campfire, and exposure state. It starts each verification pawn wet and low on warmth beside a temporary server-owned fire so the logs show recovery without changing persistent world data.

`-KalmalaCampChoiceTest` adds a non-shipping two-player scenario through the existing GameMode exposure tick. Its server-only fixtures compare two separated naturally generated camp conditions, initialize wet/cold pawns, and light temporary fires through the normal authority/range gate after an unprepared interval. No client supplies a location, environmental input, lighting result, or exposure value. Log comparison uses replicated player IDs because actor instance names can differ between peers. The harness adds no gameplay path, camp recommendation, construction piece, or save contract.

## Gathered hearth and crafting contract

`UKalmalaCraftingComponent` accepts owner intent only; configured recipes and atomic inventory exchanges determine all costs and outputs on the server. A new character carries a level-one Construction Hammer record in the owner-only carried-tool array, and the local hammer menu requires that record. The server independently checks the same authoritative tool before any construction request. Hearth placement consumes its JSON-backed raw recipe cost of 5 Stone and 3 Wood plus one Wood, Lightwood, Densewood, or Coal to start the hearth with 60 seconds; normal crafting cannot create the former CampfireKit item. Floors, walls, and roofs are also direct hammer builds: their Wood/Fibre costs derive from the JSON-backed construction recipes, the server builds the full inventory candidate, and no catalogue item is granted or consumed. Stable internal CampfireKit, FloorKit, WallKit, and RoofKit identities remain for construction and save compatibility. Every other craftable camp item is also built from the JSON recipe's direct materials. Construction RPCs accept only a buildable identity or the existing payload-free hearth placement intent; position, yaw, materials, terrain, collision, persistence, and outcome remain server-selected or server-validated. Each peer derives original collision/presentation from the replicated construction identity. The server adds the shelter sampler's roof or windbreak tag only to accepted matching constructions, never from a client request. See `02-technical-architecture.md` for recipe costs, access rules, controls, replication and current persistence limits.

M9 supersedes the M7 tool-output replacement recipes and material-paid repair. The owner-local crafting panel submits only a carried tool ID; the server selects the owner's current record, validates a visible same-world Workbench or Forge within 250 cm, and restores any damaged or broken tool to its authored maximum at no cost. Rejected requests preserve condition, and accepted repair awards no Crafting experience. The paid buildable Grinding Stone uses the existing construction placement and schema-1 save record; its server-validated in-world Repair All builds a complete candidate from the bounded owner-only carried-tool array before publishing any change. Clients submit no list, tool ID, or condition for that action. The owner panel shows private tool condition, tool levels and costs, station requirements, and selected-tool or Grinding Stone repair guidance; M9 tool and station progression remains transient until the save migration contract passes. The parameterless sharpening multicast remains presentation-only and carries no repair state.

The normal Chest is the sole storage construction. Current schema-4 food processing uses the Cooking Rack, Cauldron, and Frying Pan; it has no Smoke Frame or smoke recipe. Recipes consume direct Wood, Fibre, Stone, and other listed material costs; intermediate Fuel and ConstructionSupply items no longer exist. Cooking recipes consume their listed ingredients only. Wood, Lightwood, Densewood, and Coal each add 60 seconds of hearth fuel, and a lit hearth burns one fuel second per elapsed server second. The server derives cooking heat requirements from the resolved station and a usable lit hearth near both the player and station. Storage schema 1 migrates saved Fuel to Wood and each ConstructionSupply into its original value of 3 Wood and 2 Fibre. These changes preserve existing chest and construction save schemas.

The accepted M9 persistence boundary bumps the existing construction and player-discovery containers to schema 2, keeping their current save owners and slots. World construction uses exact seed/revision-7 identity and bounded records for paid station attachments; station level remains derived from accepted nearby attachments. Player discovery uses exact seed/revision/player identity and bounded carried-tool and discovery records. Existing schema-1 facts migrate explicitly; the separate M7 sparse ledger stays schema 1. Normal construction and player-discovery writes use schema 2 through validated first-write migration. Player writes merge only server-revalidated current M9 claims and the owner's current tool state; a rejected candidate leaves the existing save and live gameplay unchanged. See `docs/02-technical-architecture.md`.

## M3 wetness and rain override (2026-09-14)

This section supersedes earlier references to continuous player wetness, warmth-derived wetness penalties, fuel wetness extinguishing, or rain damage below. `Wet` is only a server-owned player debuff with a reusable parameter definition: default maximum duration 120 seconds, unroofed-rain trigger 10 uninterrupted seconds in Calm or Active weather and 7.5 seconds in Highly Active weather, movement multiplier 0.92, and stamina-use multiplier 1.15. Standing in server-confirmed water applies it immediately. Rain applies it only when the server finds no accepted roof above the pawn. Reapplication cannot exceed 120 seconds; a nearby lit campfire with nonzero authoritative heat removes it. Surface moisture remains a grid material/fire input and is never a second player wetness system.

The pawn's replicated status container is the sole player-facing Wet state. Each entry has a stable status identifier and finite remaining duration; the first entry is `State.Wet`. The server alone creates, refreshes, decrements, and removes entries, and it publishes the remaining duration for display. This replaces the old continuous wetness/warmth values and warmth-derived movement penalty rather than running a second player moisture simulation. The status definition supplies the fixed movement and stamina-use multipliers, so clients render/use only replicated presence and remaining duration and never send a status ID, duration, source, multiplier, expiry, or removal request. Later debuffs add a definition and entry through this same container rather than a pawn field.

### Construction and hearth rain-response contract

Every accepted `FloorKit`, `WallKit`, `WorkbenchKit`, and `StorageKit` starts with replicated server-owned `Health = 100.0`; `RoofKit` has the same presentation field but is rain-immune. At each one-second server environmental update, an exposed eligible construction loses `0.10 * clamp(PrecipitationIntensity, 0, 1)` health. Health is clamped to `[50.0, 100.0]`, so rain alone takes at least 500 seconds of full precipitation to reach its floor and can never destroy, remove, refund, or change collision for a construction. The rate, maximum, and floor are configuration-owned defaults, not client settings. Repair and any persistence decision are deferred: M3 does not yet save rain wear, and a restart returns the existing accepted construction record without a new health delta.

An accepted roof protects an actor only when the server's `ECC_Visibility` vertical trace from the actor's collision-centre plus 60 cm first hits an actor tagged `KalmalaShelterRoof` within 400 cm. The query ignores the protected actor itself, never accepts an untagged mesh, and is rerun by the server each environmental update; a client, construction transform, camera, or local overlap cannot assert roof protection. This deliberately shares the campfire protection distance and tag rather than introducing a second shelter classifier.

The replicated server-owned hearth state is exactly one of `Extinguished`, `Lit`, or `Smouldering`. `Extinguished` has no positive fuel (or was never successfully lit), zero heat/light, and requires the existing server-validated no-payload lighting intent. `Lit` has positive fuel, was successfully lit, and is either roof-protected or under precipitation below `0.05`; it provides normalized heat `1.0` in the existing 600 cm falloff. `Smouldering` has positive fuel, was previously lit, and is exposed to precipitation at or above `0.05`; it consumes fuel at the normal one fuel-second per server second but has zero heat/light. A smouldering hearth automatically returns to `Lit` as soon as its server roof trace succeeds (or precipitation again falls below the threshold); it never self-lights from `Extinguished`. Fuel exhaustion always transitions to `Extinguished`. The state, fuel seconds, effective heat, and roof-protected bit replicate for presentation, while weather input, traces, transition selection, fuel burn, and the automatic reignition remain server-owned.

The initial M3 defaults are: Wet maximum `120 s`; normal unroofed-rain trigger `10 s` (M7 Highly Active trigger `7.5 s`); Wet movement multiplier `0.92`; Wet stamina-use multiplier `1.15`; roof trace origin offset `60 cm`; roof trace distance `400 cm`; construction maximum/floor `100.0`/`50.0`; full-rain wear `0.10 health/s`; rain threshold `0.05`; hearth heat `1.0`; hearth heat radius `600 cm`; fuel burn `1 fuel-second/s`; raw-fuel item `60 s`; fuel cap `300 s`. Clients submit no wetness, duration, health, roof, weather, fire state, fuel rate, or relight value.

### M7 weather descriptors and local activity badge

`AKalmalaWorldGenerationGameState` replicates deterministic fog, normalized
rain/wind storm intensity, and the server-derived Calm/Active/Highly Active
weather tier. The server also publishes normalized per-pawn heat/cold summaries
from its sampled environmental temperature. `UKalmalaWeatherActivitySubsystem`
reads that existing tier for each local player and shows a compact badge below
the minimap: a circle plus `CALM`, a diamond plus `ACTIVE`, or a triangle plus
`HIGHLY ACTIVE`. The text and distinct shapes carry the state without colour;
font scale and contrast follow local settings. The badge creates no request,
replicated field, gameplay mutation, or persistence entry. Weather selection,
intensity, exposure, mitigation, and all resulting gameplay remain server-owned.

### Wet modifier runtime increment

The shared status component resolves finite active entries through compiled definitions, deduplicating IDs and ignoring unknown definitions. Character Movement applies the movement multiplier after sprint selection or the ocean speed cap on server and prediction paths. Legacy exposure no longer scales MaxWalkSpeed; its continuous values remain transitional telemetry pending removal. Defaults are compiled constants shared by all peers, never client settings. CalculateStaminaCost applies the shared 1.15 multiplier to a finite nonnegative authoritative base cost. The M5 balance pass intentionally keeps Wet duration, triggers, and campfire recovery unchanged while reducing the movement/stamina tax to preserve optional travel and recovery choices. No save schema or RPC changes.

### M3 sprint stamina runtime

The movement component now owns a transient replicated stamina pool (maximum 100). On authoritative Character Movement updates only, grounded, uncrouched, moving sprint intent consumes 10 stamina/second through CalculateStaminaCost; Wet therefore consumes 12.5/second. Each finite positive movement step is capped at 0.25 seconds. Otherwise stamina recovers at 15/second, capped at 100. At zero, sprint speed is disabled until recovery reaches 20; ordinary walking and swimming remain available. Swimming, crouching, standing still, and airborne movement do not consume sprint stamina. These are compiled prototype tuning defaults, not client-supplied values.

Stamina and the exhaustion gate replicate with the movement component. Prediction reads the replicated gate; it does not mutate stamina or add a stamina RPC. The server validates normal movement intent and owns all consumption/recovery. Brief corrections around exhaustion are possible under latency because stamina is not predicted or included in saved-move replay. No persistence/schema change: a new pawn starts full. Wet continues to use only the shared status definition, with no new per-status pawn field.

### Campfire removal of Wet

The server environmental update attempts campfire removal after the water/rain refresh. A same-world authoritative lit campfire must produce finite positive warmth at the pawn's authoritative location (the existing 600 cm falloff); the radius boundary contributes zero and does not qualify. Heat therefore wins within that update, including during continued water/rain exposure; when heat ceases, ordinary triggers can apply Wet again. Roof shelter alone does not remove an existing status. Only the Wet entry is removed; movement and stamina costs immediately resolve their defaults through the shared definitions. The existing status array publishes removal without a new RPC or save field. Client-local removal calls fail before mutation.

This increment uses the existing IsLit/effective-warmth interface. The later three-state hearth implementation must keep Smouldering and Extinguished at zero heat; it is not implemented by this change. Legacy continuous exposure remains transitional telemetry, not a second source of Wet modifiers.

### Construction rain-wear runtime

Construction actors now replicate Health, initialized to 100 on a new actor. The one-second GameMode environmental update supplies trusted elapsed time and precipitation. Only initialized FloorKit, WallKit, WorkbenchKit and StorageKit actors participate; RoofKit and unknown kits are immune. Finite positive rain is clamped to one and causes 0.10 health/second at full intensity, bounded at 50. Zero rain and malformed inputs do nothing. The actor samples its own collision-centre +60 cm upward by 400 cm, ignoring itself; only a first blocking hit tagged KalmalaShelterRoof protects it. Protection stops further wear without healing. Clients receive health but local mutation calls are rejected before sampling or writing. Health changes do not rebuild mesh/collision, remove actors or modify construction/storage saves. Restoration into a new actor starts at 100; rain wear remains transient.

### Three-state hearth runtime

The replicated HearthState enum now replaces the boolean: Extinguished (0), Lit (1), Smouldering (2). IsLit is a compatibility query for Lit only. Both active states burn one fuel-second per server second; exhaustion becomes Extinguished. Server weather advancement samples roof collision internally, so its trusted rain input cannot assert protection. Exposed rain >=0.05 selects Smouldering; dry or roof-protected conditions select Lit for an already active fire. Heat is exactly 1 for Lit and 0 otherwise, retaining the existing radial falloff. Wind and legacy fuel-wetness telemetry no longer reduce active heat or prevent automatic reignition. The existing validated initial lighting gate remains required for Extinguished, and fuel alone cannot self-light. Text presentation explicitly names SMOULDERING. There is no new client mutation path or saved-data schema.

## M3 rain-response authority audit

Construction rain wear runs from server GameMode weather; its mutation seam rejects clients before tracing roof collision. Hearth Tick samples weather/wind only on authority, and the shared rain transition independently rejects clients before its roof trace. Replication callbacks derive local collision, tags, meshes and light from accepted state. Local tags cannot influence the server's separate trace world. Wet modifiers may drive client movement prediction, but status mutation remains server-owned.

`-KalmalaPersistedCampTest` also performs a live remote-client isolation probe against replicated Wet, floor/roof, hearth, and weather objects. It directly calls the server-only mutation seams on client copies with extreme forged elapsed/rain values; every call must remain local no-op, retain the replicated values, and have no authoritative GameMode/save owner. The probe is development-only and sends no RPC or save mutation; the server-created camp, its normal paid placement, and owner-only storage path remain unchanged.

Construction save/load is authority-guarded in GameMode with world-identity and record validation. Storage also validates the accepted construction ID/kit/transform and saves a candidate before replacing live contents. Construction schema 1 contains only ID, kit and transform. Rain health resets to 100 on restore; hearth fuel/state and Wet remain transient. This audit adds no persistence or schema change.

Hearth.AuthorityContract guards the absence of server RPCs on state owners, actual lifetime replication of health/hearth/protection fields, and the unchanged construction record shape. Behavioral authority, roof collision and persistence checks remain in RainWear, RainState, Wet, Crafting.NetworkContract and Storage. The combined live M3 scenario remains a later acceptance gate.

### Local Wet feedback

`UKalmalaSurvivalStatusWidget` reads only its owning pawn's replicated status component. While Wet is active, it shows the rounded-up remaining seconds, movement reduction and stamina-use increase from the shared Wet definition, plus a lit-campfire recovery hint. Removal or expiry replaces this with explicit `Wet: inactive` text. The lower-left status strip retains its wrapping/scroll behavior. It never counts down locally, changes a status, reads legacy exposure wetness, or submits a network request. Construction/fire feedback and rendered layout verification remain queued.

### Local hearth recovery feedback

The crafting panel places nearby hearth status before recipes so state and recovery instructions are visible on opening. Text derived from the existing replicated hearth fields states whether heat is on, that nearby heat removes Wet, that Smouldering has no heat but still burns fuel, and that a roof or dry weather restores it. Extinguished hints distinguish empty fuel, wet fuel and readiness to light. No state, trace, RPC or persistence contract changes. Construction rain-wear feedback remains queued.

### Local construction rain-wear feedback

The crafting panel shows the nearest initialized, visible construction within 250 cm of the owning pawn (stable construction ID breaks distance ties). A local visibility trace selects presentation only; it never determines server shelter or permission. The panel reads the existing replicated kit and health, uses the catalogue name, and states health to one decimal plus no wear, rain-worn, rain-wear floor reached, or roof immunity. A generic roof-prevention hint does not claim current roof protection; no roof-state field is replicated. Missing or obscured construction clears to an explicit none-visible message. No RPC, authority or saved-data contract changes.

### Weather mutation runtime guard

SetWeatherStateFromServer now rejects non-authority and invalid input at runtime instead of relying on assertions that can compile out. Weather validity also requires a finite, nonnegative server start time. Rejected calls preserve the complete previous weather interval and do not force replication. Valid server selection remains unchanged; there is still no weather mutation RPC or weather save field. This prevents client-local writes through this C++ seam, without claiming a remotely exploitable server endpoint previously existed.

## M7 first food transaction

The first prepared food reuses the server-local item and recipe catalogues, UKalmalaInventoryComponent::BuildExchange, the existing hearth actor, and the replicated player-status component. Roasting requires a same-world fire that the server observes as usable, within 250 cm, Lit, and producing finite positive heat; a workbench, extinguished fire, or Smouldering fire cannot satisfy it. The server checks recipe identity, batch, inputs, and output capacity before changing the private pack. No additional fuel charge is applied beyond the hearth's normal fuel burn.

Food use is an owning-client request containing only an item identity. The server allowlists catalogue-approved cooked food, checks the owner's pack and meal slot, commits one-item consumption through a candidate inventory exchange, then publishes a fixed-duration State.Food.SteadyMeal entry. The entry reduces server-derived stamina cost to 0.90 for 120 server seconds. Only one meal entry is permitted: an active effect neither stacks, refreshes, nor gets replaced; rejected consumption leaves pack and status unchanged. The existing status replication carries effect identity and remaining time, while the private inventory and crafting result remain owner-only. Food, cooking progress, and meal effects add no save field or schema.

The active schema-4 food recipes cook boar or deer meat on the Cooking Rack, prepare meat stew or root vegetable soup in the Cauldron, and prepare roasted roots or deer-root roast in the Frying Pan. The server resolves the declared station, live hearth heat, inventory costs, and accepted outputs from the catalogue. HearthBroth remains an item without a production recipe. Food and meal effects remain transient; no saved-data schema changes.

### Accepted cooking experience

The server recipe catalogue may assign an allowlisted skill and a bounded award of 1-25 experience to a recipe. The first food recipes award 10 transient Cooking experience once after a successful atomic inventory exchange, regardless of batch size. Missing stations or heat, invalid batches, insufficient inputs, output-capacity failures, and client-side calls award nothing. Recipe definitions cannot pair an unknown skill, an out-of-range award, or experience without a skill. The existing level and unlock contract derives progression on the server; no client-provided experience, new RPC, persistence field, or save schema is introduced.

The current recipe schema has no skill-level requirement field. Recipe access is gated by material quantities and any required station, hearth state, and bounded batch. Accepted food preparation still grants its existing server-owned Cooking experience after the inventory exchange; it does not block a recipe behind a skill level. Rejection leaves the private pack, station state, and progression unchanged, with no learned-recipe save field or client-authored value.
### M7 local survival status strip

`UKalmalaSurvivalStatusSubsystem` belongs to each `ULocalPlayer` and builds the persistent lower-left strip from that player's existing replicated status entries, exposure summary, support-effect fields, and `AKalmalaWorldGenerationGameState` weather. The widget reports Wet's movement/stamina modifiers, the shared non-stacking prepared-meal effect, active rain/fog/wind intensities and weather interval, low-warmth cold pressure and recovery rate, heat signals, and active support magnitude/expiry. Shape markers pair with explicit category and status names; text scale and high contrast follow local settings. The separate Calm/Active/Highly Active badge remains below the minimap.

The status component already publishes remaining seconds for Wet and food; the strip reads those values without advancing them locally. Support and weather timers use their replicated absolute server expiry/start/duration with `GetServerWorldTimeSeconds`. Wet source is labelled as exposed rain or water because the shared status entry does not retain which trigger applied it; food is labelled as prepared food because the shared meal entry does not retain the consumed recipe identity. Recovery guidance describes existing roof/fire and shelter/fire rules. The subsystem has no input binding, RPC, gameplay setter, persistence path, new replicated field, or save-schema change.


## Consolidated system contracts

The sections below consolidate the durable parts of former docs 08–47. They take precedence over older incremental descriptions above where explicitly superseded. Catalogue data and compiled definitions remain the source for exact current costs and tuning. Dated requests describe intended behavior; BACKLOG.md and PROGRESS.md determine implementation, integration, and verification status. M13 and integrated M14 acceptance are still open at this cleanup.

## World generation

## Direction

Kalmala is a seed-generated, player-directed open wilderness. The world contains no authored gameplay areas, fixed camp zones, prescribed routes, or required quest sequence. Players choose where to travel, build, gather, and take risks.

The world is built from continuous procedural maps, not a visible square grid. Streaming limits and server spatial partitions exist only to manage performance, spawning, and persistence; they must never shape biome boundaries or create gameplay zones.

Kalmala may learn from broad survival-world principles—shared seeds, player-made homes, environmental risk, and landmark-led discovery—but must remain wholly original in its world layout, content, names, visual language, and implementation.

## Goals

1. **A world worth wandering.** Terrain, weather, and natural features should invite curiosity without giving players a required route.
2. **Shelter changes travel.** Different environments alter how players prepare, build, and move through the world.
3. **A shared world.** The same seed produce the same world for every player in a session.
4. **Optional discovery.** Resources, wildlife, hazards, and points of interest enrich exploration without becoming a checklist.
5. **Multiplayer-safe scale.** The server owns gameplay state while clients receive only the information needed to render and play nearby world content.

## World seed and generation

The server owns one immutable `WorldSeed` (unsigned 64-bit). Every session runs the current generator. Generation compatibility versions and their command-line selectors are removed. Changing production tuning can change existing seed layouts during development; old version-specific save slots are not loaded or deleted. Current sparse saves and local maps use the seed as their world identity.

### Master map and environmental fields

First generate a continuous Perlin **land/water master map with its own seed**, independent of the game seed. The 128 km square atlas uses the constant `FKalmalaMasterMap::MasterSeed = 0x4b616c6d616c6137`, a 3 km main wavelength and a second 1.29 km octave (75%/25% amplitudes). Positive noise is land; zero or negative noise is ocean. It contains no biome identities, authored islands, or gameplay placements.

The game seed select an atlas crop centre and an arbitrary rotation. The crop retains the current **16 km radius / 32 km diameter** playable circle. The centre is selected from up to 128 seeded candidates: use the first with land signal above 0.10, or the strongest candidate if none qualifies. This favours inland terrain for the start without painting land into the atlas. The entire rotated circle fits inside the master map. World zero is the crop centre; rotation preserves distance and scale. No baked map image is required.

The existing four independently seeded game fields remain:

| Map | Controls |
| --- | --- |
| **Elevation** | Inland relief from the existing Perlin noise, constrained to the master's land/water sign and tapered continuously to the coast. |
| **Humidity** | Surface and ground moisture, wetland suitability, vegetation support. |
| **Temperature** | Climate, snow, frost, warmth pressure, cold-biome suitability. |
| **Flora** | Local vegetation density, undergrowth and clearings; it does not fragment broad biome identity. |

Changing the game seed changes the crop, rotation and environmental fields, but not the underlying master atlas. Master tuning and its seed are compiled developer settings, never client-controlled. All peers must run the same build.

### Land biome placement and origin distance

On land, the existing regional Perlin signals and environmental suitability select **Shimmering Lakes, Elderwood, Mossy Mire and Freezing Tundra**. **Meadows and Thunder Mountains are the default land biomes** wherever no special biome wins. Distance is measured from game-world XY `(0, 0)`, not the player start or master-map origin.

| Rule / biome | Distance from world zero |
| --- | --- |
| **Meadows or Ocean only** | **0–0.35 km inclusive** |
| **Meadows** | **At most 4 km** |
| **Shimmering Lakes** | **0.35–3 km**, with the inclusive starter rule taking precedence |
| **Elderwood** | **At least 0.75 km** |
| **Mossy Mire** | **3–16 km** |
| **Freezing Tundra** | **At least 4 km** |
| **Thunder Mountains** | Default elevated land outside the starter zone; default remaining land beyond 4 km |

Minimum-distance weights rise smoothly over 50 m on the eligible side, never leaking into excluded radii. The lowland Meadows fallback fades into Mountains over the last 50 m before 4 km, reaching zero at 4 km. Elevation also favours the mountain fallback nearer the centre, with no mountain influence at or inside 0.35 km. A winning special biome takes precedence over the fallback, including on high terrain. Remaining land beyond 4 km is Thunder Mountains even where source relief is low. These restrictions replace the earlier Gaussian distance preferences; they are not enemy levels or travel gates.

Shimmering Lakes and Mossy Mire share the same regional noise, humidity, elevation and temperature suitability, terrain relief, and analytic basin parameters. Their only generation difference is the distance range: Lakes 0.35–3 km, Mire 3–16 km. Both ends ramp over 50 m inside the allowed range; their weight is zero at the exact endpoints. Biome labels do not require standing water. Qualified bowls shape terrain and water within either wetland without overriding biome weights or distance limits.

Shared suitability defaults: humidity rises from 0.48 to full strength at 0.65, elevation falls from full strength at 0.43 to zero at 0.58, and temperature rises from 0.18 to full strength at 0.32. `FKalmalaRegionalTuning` and the preview parameter file expose these same six settings. Rivers remain enabled; small streams are disabled. Inland water can occur in a land biome without making it Ocean. Islands come from the master crop. The player-start resolver searches dry Meadows within 320 m of zero.

Terrain, collision, sea water, inland water, minimap and expanded map consume the same generator. Regional weights, biome identity and terrain remain computed functions. Only a small crop transform is memoized per worker alongside the existing bounded hydrology spline cache; no authoritative biome raster is stored or replicated. Weather, wildlife, ruins, discoveries and harvest nodes are derived later.

```text
MasterSeed -> continuous land/water atlas
WorldSeed -> crop centre and rotation -> base land/water mask
WorldSeed -> Elevation, Humidity, Temperature, Flora
  -> master-constrained terrain + regional signals + distance eligibility
  -> special land biomes; Meadows/Mountains fallback; Ocean from master water
  -> seeded content, weather, survival and sparse player changes
```

## Finite world

The playable radius is always 16 km. There is one generation implementation and no alternate stream-debug world identity.

Terrain and water triangles retain the existing radial clipping. Exterior patches, population and discoveries are rejected; decoration keeps a 10 m edge margin and new hearth/construction placement keeps 3 m clearance. Character Movement enforces the radius minus capsule clearance on authority and owner prediction while preserving tangential/vertical movement. No edge wall, actor budget, asset or authority change is introduced.

## Biome palette

Development order is not player progression. The seed decides which biomes are nearby; players decide whether and when to enter them.

| Biome | Character | Shelter and travel pressure | Discovery focus | Development order |
| --- | --- | --- | --- | --- |
| **Meadows** | Gentle hills, moderate humidity, open sightlines. | Introduces fire cover, simple timber shelter, and dry storage. | Deer routes, stones, birch stands, and calm camp locations. | First |
| **Shimmering Lakes** | Interlocking lakes, shore fog, and saturated low ground. | Favors bridges, boats, dry stores, and raised shelter. | Fishing waters, small islands, and lake-edge resources. | Second |
| **Elderwood** | Dense canopy, deep shade, and heavy growth. | Rewards marked trails, compact camps, and careful visibility management. | Ancient roots, wildlife dens, and overgrown stone sites. | Third |
| **Mossy Mire** | Wet ground, slow travel, and dry hummock paths. | Rewards raised floors, drainage, and waterproof fuel storage. | Bog iron, causeways, and Mireling scavenging sites. | Fourth |
| **Freezing Tundra** | Bitter wind, sparse cover, and rolling high ground. | Rewards insulated clothing, enclosed roofs, and windbreaks. | Ice-fed springs, exposed shrines, and weather-read routes. | Fifth |
| **Thunder Mountains** | Sheer ridges, thunder squalls, and exposed passes. | Rewards lightning-safe shelter, durable construction, and route planning. | Storm-carved overlooks, mineral seams, and deep cave systems. | Sixth |
| **Ocean** | Open water, currents, waves, and storms. | Rewards seaworthy construction, anchors, and coastal shelters. | Distant islands, sea caves, and rare shoreline materials. | Seventh |

Biomes should differ primarily through environment, travel, and shelter—not a linear increase in enemy strength. Every reachable biome needs a viable lower-risk approach and a riskier shortcut or reward opportunity.

## Seeded world content

Terrain and biomes establish the world; separate seeded systems populate it. No area is assigned a mandatory purpose.

- Wildlife, harvest nodes, and hazards use deterministic server-side spatial seeds and spawn budgets.
- Weather is a server-owned deterministic cycle derived from the immutable world identity and a cycle index; it is not a biome map and never creates weather zones.
- Landmarks and major discoveries are optional points of interest generated by their own seed rules.
- Decorative vegetation, rocks, and ambient detail are cosmetic where possible; gameplay-relevant content is server-owned.
- Player-made changes override generated content through persisted save data.

## Multiplayer, persistence, and performance

- The server generates and persists all gameplay-affecting placements, AI, harvest state, hazards, construction, and survival state.
- Clients may generate cosmetic detail locally, but never decide gameplay placement, loot, damage, or rewards.
- Save only sparse player/world deltas keyed by `WorldSeed` and a server spatial key; never serialize the entire generated base world.
- Server spatial partitions are implementation-defined and invisible to players. They may manage activation and budgets, but may not create square biome borders or authored gameplay areas.
- Use instancing or pooling for non-interactable vegetation and rocks. Promote only nearby interactive content to replicated actors.
- Profile generation time, memory, replicated actor count, save size, and late-join synchronization before increasing content density or streaming distance.

## Harvest source catalogue

## Canonical source definitions

IDs are closed canonical design tokens. Source and presentation IDs use
lower kebab case, matching the M7 biome catalogue; inventory item IDs use the
existing PascalCase item-catalogue convention. Each ID is unique and selected
from the server's generated descriptor. The client may render the replicated
source and submit the existing interaction intent, but cannot choose an ID or
reward.

| Biome | Source ID | Presentation ID | Item ID / display name | Max stack | Local presentation |
| --- | --- | --- | --- | ---: | --- |
| Meadows | `meadows-birch-trunk` | `birch-trunk-harvest` | `Lightwood` / Lightwood | 50 | Mark the lower portion of an existing generated birch trunk with pale vertical bark and two dark harvest notches; do not add a duplicate tree. |
| Elderwood | `elderwood-ironheart-trunk` | `ironheart-trunk-harvest` | `Densewood` / Densewood | 50 | A broad dark heartwood trunk with sparse copper-brown grain and a readable lower-trunk cut face. |
| Mossy Mire | `mire-peat-amber-seam` | `peat-amber-seam` | `PeatAmber` / Peat Amber | 40 | A low irregular peat-bank face with one narrow amber seam, grounded to wet terrain. |
| Freezing Tundra | `tundra-frost-salt-deposit` | `tundra-salt-crystals` | `FrostSalt` / Frost Salt | 40 | A small wind-scoured cluster of pale, angular salt crystals rooted in the tundra surface. |

The item display names are **Lightwood**, **Densewood**, **Peat Amber**, and
**Frost Salt** respectively. Presentation stays original and collision-free;
the existing server interaction collision and generated terrain remain the
authority for reach and placement. The two trunk sources reuse their generated
tree anchors. The peat and salt forms add no platform, trail, camp, or route.
Wood source stacks cap at 50 like current `Wood`; mineral source stacks cap at
40 like current `Stone`.

## Deterministic placement and population budget

- Use the existing invisible 6,000 cm `SpatialKey` and the current world seed,
  generator revision, source ID, and key coordinates for deterministic
  placement. The generator revision is part of the matching world-save
  identity; it is not a visible region boundary.
- Use standard FNV-1a 64-bit (offset basis `14695981039346656037`, prime
  `1099511628211`) over the ASCII bytes of each specified string.
- Compute FNV-1a 64-bit over the ASCII string
  `m9-place-v1|<world-seed>|<generator-revision>|<spatial-x>|<spatial-y>`;
  the remainder modulo four selects exactly one source in the table order
  above. Format the seed as unsigned base-10 and the revision and coordinates
  as signed base-10 with no leading zeroes. Thus each source can be selected on
  at most one in four keys. If that source's biome is absent from the key,
  place nothing; do not fall back to a different source.
- For the selected source, examine at most eight candidates in fixed ordinal
  order, ordinal 0 through 7. Derive each candidate from FNV-1a 64-bit over
  `m9-place-candidate-v1|<world-seed>|<generator-revision>|<spatial-x>|<spatial-y>|<source-id>|<ordinal>`;
  use the same decimal formatting and the canonical source token, then map the
  low and next 16 bits to fractions `(bits / 65536.0)` for X and Y within the
  key. Accept the first point that is inside the 16 km world, has the source's
  exact classified land biome, is dry under the existing ocean and lake
  samplers, and has a terrain normal Z of at least 0.88. If none qualifies,
  place no source on that key; do not fall back to a client or actor-selected
  location.
- The combined M9 source ceiling is at most one node per spatial key,
  regardless of how many candidates are examined. It occupies one slot from
  the existing post-biome `HarvestNode` budget, only when that budget is at
  least two, and leaves at least one first-wave harvest slot. Never append a
  source beyond `GetSpawnBudget` plus the existing biome multiplier, raise the
  25 active terrain-patch ceiling, or create a parallel population budget.
- Source positions, IDs, and presentation selection must reproduce for the
  same seed, revision, and key on every peer. Clients receive only relevant
  materialized actors, not candidate lists, placement rolls, or routes.

## Stable sparse depletion identity

Derive the source's current population spawn identity through
`FKalmalaWorldPopulationLayout::GetPersistentSpawnId` and wrap it in this
reserved resource key:

```text
resource:m9:v1:<source-id>:<kind>/<spatial-x>/<spatial-y>/<spawn-seed>
```

Store it only as a world-scoped `ResourceDepleted` sparse delta under the
existing exact world seed and generator-revision save identity. The ID contains
only the approved source token and server-derived population identity; a client
never submits it. These IDs fit the current 128-character stable-ID limit and
remain separate for different sources or spatial spawns. Persist depletion only through the approved identity-scoped sparse ledger and its validated writer; source identities add no save field.

## Bounded optional loot

Each accepted source harvest guarantees one unit of its table's primary item.
It may grant exactly one additional unit of that same item; there are no
secondary item IDs, open-ended rolls, quantity ranges, or chained drops. The
bonus is awarded when the FNV-1a 64-bit hash of the ASCII string
`m9-loot-v1|<world-seed>|<generator-revision>|<source-id>|<population-spawn-id>`,
interpreted as an unsigned integer, is divisible by four. The outcome is
therefore deterministic for a source identity and has a strict total yield of
one or two items.

The server computes the bonus and preflights the full one- or two-item grant
against the existing item catalogue and owner inventory before publishing any
reward, tool wear, or in-session depletion. A full pack, invalid source,
wrong-world target, failed server trace, invalid range, or mismatched tool
intent leaves inventory and source state unchanged. Client requests carry no
reward ID, quantity, bonus result, target, or spawn identity. The separate
server-side field-gate implementation maps the approved birch trunk to
Bronze-or-better and the ironheart trunk to Iron Axe; it does not check a
station at the source. Axe crafting or upgrade transactions will validate the
matching Workbench or Forge level when the tool-progression work is added.

First-wave sources remain birch bark and resinwood for Wood, reed clusters and frostmoss for Fibre, and bog iron and slate veins for Stone. Peat Amber and Frost Salt require Stone Pick/Mining; birch trunks require Bronze Axe or better and ironheart trunks require Iron Axe. Accepted harvest preflights the entire inventory reward, spends one tool-condition point, and depletes once. Failure preserves inventory, condition, and availability. Clients never choose reward, quantity, target, sparse ID, or bonus.

## Tools and station progression

Tools occupy the same forty-cell inventory as materials, rather than extra carrying slots. The owner-only CarriedTools array has at most six unique known records; condition and authored tool level are independent of the transient player skill ledger. Starting tools are Reed Knife, Field Hatchet, Stone Pick, and level-one Construction Hammer. The hammer has 100 condition and no use wear; axes must be crafted.

Bronze Axe level 1 costs 4 Wood, 3 Stone, and 2 Fibre at a level-one Workbench. Iron Axe level 2 replaces a level-one Bronze Axe at a level-two Forge, requires Crafting level 5, and costs 3 Lightwood, 2 PeatAmber, 4 Stone, and 2 Fibre. Densewood is deliberately excluded because Iron Axe unlocks it. Server-selected station family and effective level must match the tool definition exactly; rejection leaves both candidates unchanged.

A paid Tool Rack or Forge Anvil within 125 cm adds one level to its matching station. One attachment per station, a level-two ceiling, 32 attachments, and 128 total construction records bound progression. Placement and station use recheck visibility and the normal 250 cm range. Station levels are derived from placed actors, not saved as numbers.

Selected-tool repair is free at a visible same-world Workbench or Forge within 250 cm. It restores damaged/broken known tools without changing level or granting Crafting XP. Grinding Stone Interact performs atomic Repair All across the validated carried list; malformed records reject the whole action. Its accepted 1.2-second sharpening pose is cosmetic and carries no tool state. M9 free repair supersedes historical M7 material-paid repair and broken-tool replacement recipes. Accepted tool changes save the validated schema-2 candidate before publication.

## Camp crafting and station services

## Current recipe rules

Recipe access is limited by the required raw materials, any visible same-world
station listed in the `RequiredStation` array, the bounded batch, and live heat
at food-processing stations. The server derives the heat rule from the
resolved Cooking Rack, cauldron, or Frying Pan identity; recipe metadata does
not opt recipes into a generic hearth gate. Schema 4 contains no Smoke Frame
item or recipe. There are no recipe skill-level
requirements, `AlternateStation`, `OutputTool`, or legacy fire flags. Cooking
experience remains a server-owned reward after a successful inventory
exchange.

## Current food recipes

These six food recipes and their outputs are the active cooking catalogue.
Every batch is capped at five, and every output stack is capped at twenty.
The server requires the declared visible same-world station and a usable lit
hearth with positive heat within 250 cm of both the player and station.

| Recipe ID | Ingredients per serving | Required station | Output item |
| --- | --- | --- | --- |
| `CookedBoarMeatRecipe` | `BoarMeat` x1 | Cooking Rack | `CookedBoarMeat` |
| `CookedDeerMeatRecipe` | `DeerMeat` x1 | Cooking Rack | `CookedDeerMeat` |
| `MeatStewRecipe` | `BoarMeat` x1, `DeerMeat` x1, `Carrot` x2, `Potato` x2 | Cauldron | `MeatStew` |
| `RootVegetableSoupRecipe` | `Carrot`, `Potato`, `Rutabaga`, `Onion` x1 each | Cauldron | `RootVegetableSoup` |
| `RoastedRootVegetablesRecipe` | `Carrot`, `Potato`, `Onion` x1 each | Frying Pan | `RoastedRootVegetables` |
| `DeerRootRoastRecipe` | `DeerMeat`, `Rutabaga`, `Onion` x1 each | Frying Pan | `DeerRootRoast` |

`HearthBroth` remains a catalogue item with a stack limit of twenty but has no
production recipe. The former `RoastedFieldMeat`, `SmokedFieldMeat`, and
Smoke Frame IDs are absent from schema 4. The steady-meal item use remains
transient and does not add save data.

All construction recipes consume the raw materials listed in the catalogue.
Former `ConstructionSupply` values were expanded to 3 Wood and 2 Fibre each;
duplicate ingredients were merged. The Construction Hammer builds the hearth,
floor, wall, and roof directly from those recipe costs. Other buildable
construction recipes produce their internal construction identity through the
existing validated placement path.

| Retired item or recipe | Current behavior |
| --- | --- |
| `Fuel` | Removed. Hearth placement and refuelling consume raw Wood, Lightwood, Densewood, or Coal directly. Cooking recipes consume only their listed ingredients. |
| `ConstructionSupply` | Removed. Recipes consume its former 3 Wood + 2 Fibre value directly. |
| `RaisedStorage` | Removed. `Storage` produces the normal Chest only. |
| `Timber` | Removed as an intermediate recipe; construction consumes its raw Wood/Fibre value. |
| Smoke Frame and legacy smoke/roast recipe IDs | Absent from schema 4. Current cooking uses the six recipe IDs in the table above. |

Any one of Wood, Lightwood, Densewood, or Coal adds 60 seconds of hearth fuel.
The server burns one fuel second per elapsed server second while the hearth is
lit. Cooking does not consume extra inventory fuel per serving or batch, and
clients cannot select or charge fuel through a recipe. Coal is accepted as a
fuel item, but the current world loot catalogue has no natural Coal source.

## Compatibility and authority

The JSON source remains schema 4 and the storage SaveGame remains schema 1.
When a saved chest is loaded, legacy Fuel quantity converts one-for-one to
Wood; each legacy ConstructionSupply converts to 3 Wood and 2 Fibre. The
migration validates and splits resulting stacks within existing item and chest
limits. It preserves the former material value without adding or changing save
fields.

The server validates recipe identity, batch bounds, item IDs, required
stations, station-local hearth access and heat for food processing, and output
capacity before publishing an inventory change. It selects a visible station
in the same world; clients cannot choose station actors, ingredient costs,
outputs, rewards, fuel type, or construction results. The normal Chest keeps
its existing server-selected transfer rules, bounded contents, and owner-only
inventory view.

## Optional content still deferred

A storm-rated shelter piece remains deferred until its behavior differs from
the existing roof and windbreak. An insulated wrap remains deferred until
there is an approved equipment-slot and effect contract.

Build supports direct hearth, floor, wall, and roof construction plus placement of supported produced station/utility items. Campfire is construction-only, never a carried item. Raw construction costs and one available ignition fuel item are paid server-side; fuel priority is Wood, Lightwood, Densewood, then Coal. Placement checks the carried hammer, finite-world bounds, ground, clearance, and collisions using server-derived transforms. Local previews cannot place or pay.

Interacting with a Workbench opens Craft/Repair; Forge opens Craft/Upgrade/Repair. Cooking Rack, Cauldron, and Frying Pan show only their two declared dishes. Forge creates Frying Pan from five Iron; Build places the produced pan. Station views bind to the exact accepted actor/stable construction ID and close on destruction, range loss, or pawn replacement. Every action independently revalidates the server station and transaction.

Chest Store/Take transfers one selected allowlisted item through the existing server RPCs. Its owner-local pack/chest views do not create extra player capacity. Chest contents are bounded, private, and saved as a validated candidate before the live pack is changed; rejected/stale transfers preserve both. Campfire refuelling adds 60 seconds from one raw fuel item up to 300 seconds without lighting it. M13's requested direct Interact lighting replaces the older Build relight button only when implemented and accepted; see Menu browsing.

## Combat and support magic

The committed basic attack sends only a monotonically increasing request sequence. The server traces living wildlife within 220 cm, uses a 0.18-second windup, rechecks target validity, applies its fixed 25 damage, and enforces 0.36-second recovery. Wildlife has 100 health; defeat joins the existing unique server-owned sparse callback. There is no arbitrary damage endpoint or client target. Public action phase/serial and owner-only target-free results provide readable feedback.

Mirelings scavenge and threaten camps, boar defend territory with committed charges, and deer use bounded wary-herd flee behavior. Server-owned ecological behavior and defeat rewards never use client-selected archetypes, targets, or loot. Optional discoveries and boss scrolls commit authenticated-player entitlement before acknowledgement; duplicate claims grant nothing and failed saves leave the encounter/discovery recoverable.

Mending heals one other living damaged ally within a 350 cm forward/visibility trace by up to 30 health, without revival. Hearth Shield gives its caster 40 absorption. Bear's Vigor raises the stamina cap from 100 to 140 and supplies 1.4 strength without refilling stamina. Deer Call affects at most three existing eligible living deer within 900 cm; it cannot spawn, teleport, damage, harvest, or reward. Existing runtime behavior uses a bounded flee influence despite the broader design's attraction wording. Effects have an 18-stamina base cost through the shared Wet modifier; server definitions own magnitude, targeting, and expiry.

### M13 reusable support-scroll inventory and hotbar integration

The 2026-10-10 user request supersedes the F1–F4/Q effect-selection path above:
each existing learned support effect is represented by a reusable scroll item
in the standard carried inventory, can be assigned to one of numbered hotbar
cells, and activates from that hotbar cell. Scroll use does not consume the item.
The learned-effect entitlement remains the persisted progression gate; when a
pawn's transient inventory is initialized, the server reconstructs the matching
scroll items from that existing entitlement without adding save fields. This
preserves discovery/learned-effect records and reconnect behavior while keeping
scrolls in the ordinary inventory UI.

Hotbar intent identifies only the cell index. The server derives the scroll's
canonical item/effect mapping from that current owned cell, confirms the learned
entitlement and existing server-side request/rate, cooldown, stamina, and
effect-specific target gates, then uses the existing support activation path.
This M13 timing override supersedes earlier M4 cooldown and duration values: each
accepted support cast starts a shared 300-second per-caster server cooldown.
Hearth Shield, Bear's Vigor, and Deer Call last 180 seconds; Mending remains an
instant heal with no active duration. Deer Call's existing bounded, server-
selected deer influence lasts 180 seconds. At most one support effect may be
active per caster; reject any new cast while an ongoing effect remains active,
and do not stack or refresh effects. The cooldown remains active after the
effect expires. Preserve current costs, magnitudes, targets, owner-only feedback,
and relevant-peer effect presentation. Never accept a client effect ID, target,
cost, duration, magnitude, or result. No new spell or saved-data migration is
introduced.

| Effect | Server-selected target and result |
| --- | --- |
| Mending | Living allied pawn in range and line of sight; positive heal clamped to maximum health; reject full health; no revive |
| Hearth Shield | Caster; temporary bounded absorption; expiry removes remaining protection; reject refresh while active |
| Bear's Vigor | Caster; temporary bounded stamina/strength modifiers; reject refresh while active; do not refill stamina on activation or expiry |
| Deer Call | Bounded nearby living deer selected by server; behavior influence only; no spawn, teleport, damage, harvest, or loot |

The M13 scroll implementation is reported in the handoff files; fresh integrated acceptance remains pending. These requested 300/180-second values supersede the older five/ten-second prototype timings when that implementation is integrated.

## Ocean travel

## Travel medium and identity

The first medium is a project-original open-water skiff with two seats: one helm and one passenger. It carries no cargo. This contract does not prescribe its mesh or recipe. Each skiff is a transient server-created actor; the primary skiff receives the stable world-scoped identity `ocean-skiff:primary` when its moored state is saved, while the live actor reference remains transient.

The existing server-owned immutable `WorldSeed` remains the only generated-world identity. The skiff adds no generator input, biome, route, authored waterway, or alternate world identity. It operates on master-map Ocean; inland lakes do not qualify.

## Server-owned state

The server owns and mutates:

- the skiff actor's transform and velocity;
- movement mode: `Moored`, `Underway`, or `Blocked`;
- the helm and passenger seats, each occupied by at most one authenticated session player;
- the currently accepted helm input sequence and freshness timer; and
- launch, boarding, disembark, collision, and movement outcomes.

The server assigns seats. Only the helm occupant steers. A passenger has no movement authority. Vessel state is never created, selected, or moved by directly applying client-supplied actor transforms.

`Moored` is the launch/stopped state at or below 50 cm/s with no active throttle. `Underway` applies while speed is above 50 cm/s or a nonzero helm input is active, including coasting after input expiry. `Blocked` is entered only for a collision or invalid-water movement rejection; a fresh valid helm intent may move the skiff clear.

## Client intent and movement bounds

Launch, board, and disembark are action intents through the existing server-validated interaction path. A request may identify a candidate vessel for boarding or leaving, but the server validates that reference against the caller's current world, interaction range, visibility, and seat availability. A launch request carries no spawn point. Seat choice and exit placement are server-derived.

The helm sends only finite throttle and rudder values in `[-1, 1]` plus a monotonically increasing per-helm sequence number. Accept no more than 10 steering updates per second and expire the current input after 0.5 seconds without a fresh sequence. Older, duplicate, malformed, non-finite, or out-of-range input is rejected without state mutation. Initial shared compiled movement caps are:

| Limit | Initial cap |
| --- | ---: |
| Forward speed | 700 cm/s |
| Reverse speed | 200 cm/s |
| Acceleration | 100 cm/s^2 |
| Yaw rate | 35 degrees/s |

The server simulates bounded movement and sweeps the skiff against generated terrain collision. A locally controlled helmsman may predict presentation using the shared caps, but server movement and correction are authoritative. Disembark is accepted only at server-measured speed of at most 50 cm/s and only when the server can resolve a safe pawn capsule placement; clients cannot choose an exit location.

## Water, collision, and failure

The skiff may travel only over master-map Ocean where the existing generated-ocean depth sampler reports at least 100 cm of water. The server retains the existing 16 km playable radius and shared 25-patch terrain-streaming cap. Movement may not cross generated land, skip a collision surface, or continue beyond the finite-world boundary.

If a server sweep or water sample blocks movement, the server clamps to the last safe transform, zeros velocity, sets `Blocked`, replicates that mode to relevant peers, and sends the failure reason to the helmsman. A fresh valid helm intent may steer away from the obstruction. Stale input or a disconnected helm occupant clears throttle and server-decelerates the vessel to `Moored`. Invalid launch, boarding, steering, or disembark requests leave the affected actor and occupancy unchanged; the requester receives the denial reason.

`Blocked` describes a collision or invalid-water response only. This first session-only, no-cargo skiff has no hull health, disabled mode, repair, salvage, or accepted hull-damage source. Collision or invalid-water rejection already has a recoverable outcome: the server clamps to the last safe transform, stops the skiff, and accepts fresh helm input to steer clear; stale or disconnected helm input clears controls and decelerates the skiff to `Moored`. Adding durability now would require a new damage source and repair or salvage loop without an accepted role in this bounded travel medium. Revisit hull damage only if a later approved M8 hazard explicitly affects vessel integrity, and define its server-owned recovery before implementation.

## Replication and persistence

Replicate skiff movement at no more than 10 updates per second while moving. Replicate movement mode and the two session occupant references to relevant peers. Do not replicate raw steering samples, input sequence/freshness, or per-player control data to other players. Send transient action and failure feedback only to the requesting player; do not multicast every input sample.

The versioned M8 persistence gate below now permits one server-created skiff's moored snapshot and authenticated player seat references. Cargo, velocity, steering input, and live actor references remain excluded. Ocean discovery claims continue in the separate M7 sparse ledger.

### Versioned travel-save contract and runtime restore

`UKalmalaOceanTravelPersistenceSaveGame` defines a separate schema-1 gate and
does not extend or rewrite the M7 sparse-discovery container. It reuses the
M7 identity contract so every record is bound to the exact `WorldSeed`,
generator revision, and explicit world or authenticated-player scope. A
world-scoped record holds at most one stable `ocean-skiff:` identity and its
last server-accepted safe location and heading. The writer may capture that
snapshot only when the server has stopped the skiff in `Moored`; coordinates
must be finite and inside the 16 km world radius, with bounded vertical offset
and heading. A player-scoped record holds at most one stable vessel reference
and a `Helm` or `Passenger` seat, keyed by the authenticated player identity
in the enclosing save identity. The two records pair only when world seed,
generator revision, and vessel ID agree.

This skiff has no cargo, so the contract has no cargo field. It also omits
velocity, steering samples, session actor references, and saved live occupancy;
seat references are rebuilt from authenticated player records. The runtime
writes world state only when the server has a `Moored` skiff, then writes each
authenticated occupant's seat under that player's scoped slot. A disembark
clears the player's seat slot before moving the pawn, and fails closed if the
slot cannot be cleared. Startup accepts an existing world record only when its
identity, safe bounds, sea-level position, and full generated-ocean hull
footprint remain valid; actor spawn must also pass collision handling. On
login, the server pairs the player's exact identity and saved seat with that
world vessel, then rejects an already occupied seat, a duplicate player, or an
already attached pawn. Invalid or incompatible records are preserved without
overwrite. Ocean discovery `DiscoveryClaimed` ledgers are loaded for the
authenticated player at login, so reconnects keep duplicate rewards rejected.

Schema zero returns `MigrateBeforeLoad` and requires an explicit migration
before load; unknown future schemas fail closed. The focused
`Kalmala.World.OceanTravel.PersistenceContract` automation round-trips both
scopes in memory and through a local save slot, and rejects mismatched
identities, malformed bounds, orphaned vessel references, and invalid seats.
`Kalmala.Gameplay.OceanTravel.SkiffRestoreContract` covers server-only seat
restore gates, duplicate player/seat rejection, and safe disembark-save
feedback. Normal save and restore now run in the server game mode; the full
two-player restart/reconnect journey remains in M8 acceptance.

## Multiplayer assessment

This contract preserves server authority: clients send bounded action and steering intent, while the server selects seats, validates world/collision constraints, simulates movement, and publishes accepted outcomes. The only public replicated vessel state is relevant movement, mode, and current session occupancy. Private input and all persistent state remain excluded.

The world uses a fixed origin throughout the finite 16 km radius; travelling does not rebase origin or change identity. Server weather applies bounded crosswind pressure through the existing skiff movement authority. Optional sea discoveries use server-derived stable identities and authenticated-player claims; no cargo, hull durability, mandatory voyage, or authored sea route is added. Late join and restart must preserve moored-vessel/seat identity, owner-private discovery claims, and exact-once rewards. M8 dry-shore acceptance was waived; safe capsule disembark must not be described as proof of dry shoreline arrival.

## Optional land discoveries

## Candidate slate

| Candidate ID | Player-facing name | Biome | Observation cue | Reward |
| --- | --- | --- | --- | --- |
| `lakes-rillworn-marker` | Three-Run Rillstone | Shimmering Lakes | A low, walkable shoreline stone has three water-cut channels that meet at a shallow pocket holding a small cache. The channels and pocket read by shape and relief, not colour alone. | `Stone` x2 |
| `mountains-leeward-grain` | Leeward Grain | Thunder Mountains | Parallel wind-scored lines cross a broad, walkable ridge-foot shelf; a low stone lip catches a small bundle of reed fibre on its sheltered side. The marks and bundle read by shape and text, not colour alone. | `Fibre` x2 |

Each candidate rewards noticing a local feature and inspecting it through the
ordinary interaction flow. Its short observation text describes the feature
itself; it does not point toward another candidate or reveal coordinates.
Rewards use existing catalogue item IDs and are intentionally small. They do
not add a new item, tool tier, skill unlock, magic effect, or access to
`Lightwood`, `Densewood`, `PeatAmber`, or `FrostSalt` outside their approved
sources.

## Design boundaries

- Both candidates are optional and self-contained. There is no order, quest
  chain, waypoint, compass bearing, route, or required clue sequence.
- Neither candidate requires a boss, combat, a specific tool, special weather,
  a timing window, swimming, a precision jump, or a dangerous climb. The
  intended approach is ordinary walking on stable terrain.
- The reward belongs to the player who successfully inspects the candidate;
  one player's claim does not prevent a nearby co-op partner from discovering
  the same feature. Duplicate claims are tracked independently by authenticated
  player identity.
- Use the existing world presentation and fog-of-war rules. Do not publish an
  undiscovered candidate list, add a map/minimap marker, or query hidden
  discovery positions from the client.
- The server derives one candidate for a spatial cell only when its center is
  in the candidate's biome. Placement starts from the existing biome discovery
  point and probes up to 24 deterministic offsets to avoid overlapping its
  first-wave marker. The final position must be in bounds, dry, walkable, and
  in the same biome; the Rillstone also requires lake water within 350 cm, and
  Leeward Grain requires elevation of at least 0.80.
- Stable sparse identities use
  `land-discovery:m9:1:<candidateId>:<x>,<y>`, where the coordinates are the
  population spatial key. The server re-derives the descriptor before use.
  Clients receive only the presentation identity and submit interaction
  intent; they do not receive candidate descriptors, rewards, or hidden
  discovery positions.
- A claim requires server authority, the same world, an active population
  cell, the canonical descriptor, an authenticated player identity, and a
  distance of at most 250 cm. The server preflights the exact inventory grant,
  records the sparse identity for that player, and commits the inventory
  transaction; failed commits roll back the claim. The in-memory claim set is
  capped at 64 discoveries per player per session, and accepted feedback is
  owner-only.
- Schema-2 player saves now preserve validated land claims. Failed saves reject the claim and inventory transaction. Ordinary material rewards remain pawn-lifetime and are never re-awarded on reconnect.

The slate adds no boss encounter or combat gate. Boss rewards remain optional
under the existing game-design contract and can be considered separately if a
later accepted candidate needs them.

## Save identity and migration

## Identity and ownership

All new M9 records match the exact server-owned world seed, generator revision
7, and declared scope. World construction records use
`FKalmalaM7SaveIdentity::ForWorld(WorldSeed)`. Player tool and discovery records
use `ForPlayer(WorldSeed, AuthenticatedPlayerIdentity)`, with the existing
128-character identity bound and validation. The player identity comes from
the authenticated `PlayerState` unique ID; a local player index, display name,
client-provided ID, or shared fallback is never accepted. A missing or invalid
authenticated ID disables player-save restore and writes for that session.

Before loading or caching a player slot, the server refreshes its world
generation config from the authoritative `GameState`. This covers `PostLogin`
running before `GameMode::BeginPlay` copies the selected seed. Cached player
saves are reused only after their full world and owner identity still matches;
a stale cache fails closed without restoring or writing it.

The existing construction and player-discovery slot names remain in use. Their
schema-2 payloads carry the explicit M7 identity in addition to the legacy
seed/player fields where those fields are retained. A record is usable only
when the seed, revision, scope, and (for player state) owner all match exactly.
The existing `UKalmalaM7PersistenceSaveGame` schema-1 sparse world/player
ledger remains unchanged; its resource, creature, and ocean-discovery entries
are not reinterpreted or moved.

## State scope and bounds

| State | Scope and saved value | Bound and validation |
| --- | --- | --- |
| Carried tools | Authenticated player: canonical tool ID, authored tool level, and condition for each carried tool | At most the existing six records. IDs must be unique and in the tool catalogue; levels must match that tool's authored tier; condition must be finite and within its authored range. No skill ledger or pack inventory is included. |
| Workbench/Forge progression | World: accepted Workbench tool-rack and Forge anvil construction records (stable construction ID, existing kit identity, transform) | Existing 128 construction-record total and 32 attachment cap. At most one compatible attachment may upgrade each station. Effective level is re-derived from same-world placed actors and capped at level 2; no numeric station-level save field is added. |
| Land discoveries | Authenticated player: existing stable discovery IDs, including the two M9 land-discovery IDs | Preserve the old 64-entry first-wave allowance and add at most 64 M9 claims; schema 2 caps the combined set at 128 unique valid IDs, each no longer than the existing 128-character stable-ID limit. |
| Other camp structures | World: existing schema-1 construction records for the approved Forge, Grinding Stone, normal Chest, legacy Smoke Frame records, and other accepted structures | Preserve existing identities and records during migration; count all structures against the same 128-record cap. Smoke Frame remains a legacy save identity but has no item or recipe in the current schema-4 catalogue. |

No saved record contains a client-selected result, cost, tool level, station
level, reward, actor pointer, or hidden discovery descriptor. Clients continue
to submit intent only. World records remain server-owned and replicate through
the existing relevant construction actors. Detailed tool state remains
owner-only through the existing carried-tool replication; the save adds no
replicated field.

This contract does not persist ordinary carried materials or inventory,
skill/experience, active food or support effects, weather, exposure, tool
cooldowns, or other unapproved equipment slots. In particular, an M9 discovery
claim remains durable even if its common Stone/Fibre reward was in the existing
pawn-lifetime inventory and is lost on reconnect. The claim is not re-awarded;
the materials remain available from their ordinary sources. This contract does
not claim general inventory persistence.

## Schema and migration policy

1. Bump `UKalmalaConstructionSaveGame` and `UKalmalaPlayerDiscoverySaveGame`
   from schema 1 to schema 2 in their existing slots. Do not change the M7
   sparse-ledger schema or the storage schema.
2. Migrate a construction schema-1 payload only when its seed matches the
   requested world and every legacy record is valid, unique, and within the
   existing limit. The schema-1 format used the current generator without
   storing its revision, so the explicit migration binds it to revision 7,
   preserves every construction ID/kit/transform without moving or
   re-generating it, and begins with no attachment records.
3. Migrate a player-discovery schema-1 payload only when its seed and
   authenticated player identity match and its existing discovery/effect sets
   validate. Bind it to revision 7, preserve those sets, and initialize tool
   records and M9 land claims empty because schema 1 stored neither. Session
   claims are not imported from a schema-1 save; any still-live claims must be
   revalidated and merged on the first schema-2 write under step 4.
4. On the first schema-2 write, merge the validated migrated facts with the
   current server-owned M9 facts for that same scope. Re-derive construction
   descriptors and discovery IDs from current authoritative state; never
   accept client save contents. New worlds/players start with an empty schema-2
   record plus the normal authored starting tools.
5. Validate the complete candidate, including duplicate IDs, catalogue IDs,
   transforms, condition/level ranges, scope, and all caps, before replacing
   the saved slot or publishing the live transaction. Failure leaves the old
   slot and live state unchanged; never truncate, partially migrate, or reset
   a valid save to make it fit. A save failure rejects the associated
   progression, placement, or claim transaction.
6. Current schema 2 loads only on an exact identity match. A known schema 1
   uses only the explicit migrations above. Schema 0 requires a separately
   reviewed migration and is rejected until one exists; versions greater than
   2 fail closed. A malformed, duplicate, mismatched, or over-cap record is
   rejected without overwriting the original bytes.

Normal construction and player-discovery writers now use schema 2. Valid schema-1 facts migrate on first write and merge only re-derived server-owned current state. Failed writes preserve previous bytes and reject/roll back associated tool, construction, or reward publication. Earlier candidate-only and session-only progression handoffs are superseded.

## Frozen save and catalogue versions

These are the current formats in the source tree. The two M9 schema-2 owners
continue to read and explicitly migrate valid schema-1 records under
`docs/02-technical-architecture.md`. Other formats stay at their current
versions.

| Save owner or data contract | Frozen current version | Scope |
| --- | ---: | --- |
| Construction records and station attachments (`UKalmalaConstructionSaveGameV2`) | Save schema 2 | Server world |
| Player discoveries, learned effects, M9 claims, and carried tools (`UKalmalaPlayerDiscoverySaveGameV2`) | Save schema 2 | Authenticated player and exact world |
| M7 sparse persistence ledger (`UKalmalaM7PersistenceSaveGame`) | Save schema 1 | Identity-scoped sparse world/player facts |
| Generated population deltas (`UKalmalaWorldPopulationSaveGame`) | Save schema 1 | Server world |
| Chest storage (`UKalmalaStorageSaveGame`) | Save schema 1 | Server world |
| Ocean vessel and seat records (`UKalmalaOceanTravelPersistenceSaveGame`) | Save schema 1 | Server world and authenticated player |
| Local map exploration coverage (`UKalmalaWorldMapExplorationSaveGame`) | Save schema 1 | Local player |
| Local map pins (`UKalmalaWorldMapPinsSaveGame`) | Save schema 1 | Local player |
| `Content/Data/GameCatalogues.json` | Catalogue schema 4 | Shared authored item and recipe definitions; not a save schema |

Do not change a frozen save schema, slot identity, world identity, or migration
policy as routine release hardening. A required compatibility change needs
explicit user direction, a versioned migration and recovery plan, focused
round-trip/rejection coverage, and an updated release rationale before the
roadmap is reopened.

The accepted M10 gameplay baseline stays frozen except for explicit subsequent user-directed scope such as M13/M14. No routine documentation or release cleanup changes save schemas, identities, migrations, platform, business model, or online-service commitments.

## Shared inventory and hotbar


The owner's 2026-10-09 request supersedes the earlier sixteen-slot pack and
separate carried-tool presentation. The player has one ten-column, four-row
inventory. Materials, meals, kits and tools all occupy this same forty-cell
space; tool condition/level records are metadata for those cells, with no
additional toolbelt capacity. Existing station and chest inspection panels
read the same holdings and do not create another player inventory.

The upper row is labelled `1 2 3 4 5 6 7 8 9 0`. Its cells are the hotbar.
Moving or swapping an item into a numbered cell assigns that number directly;
moving it into a lower row removes its hotbar entry. Exhausting a stack leaves
its cell empty and preserves every other cell's position. New holdings enter
the first free cell. Existing starting tools occupy ordinary cells. Tool
upgrades retain the replaced tool's cell.

Tab / I opens the panel at the upper left. Drag between cells to move or swap.
Arrows / D-pad navigate all forty cells, including empty cells; Enter / A picks
up and places, and Escape / B cancels a pending keyboard move before closing
the panel. The selected item retains its shared description, food-use action,
and applicable free selected-tool repair. Search, category, sorting, and the
separate tool section are removed from this player panel. Its outer scroll
retains access to descriptions/actions on small viewports and at larger text
scales. The grid remains ten by four while fitting the panel width.

Normal gameplay shows only occupied numbered cells in a compact upper-left
row, preserving their original numbers in `1` through `9`, then `0` order.
For example, occupied cells 1, 4 and 0 render three entries labelled 1, 4, 0.
The row hides during inventory and other movement-blocking modals, and when
all ten cells are empty. Original catalogue icons, quantities, condition bars,
and an active-item underline remain visible. Number keys select the server's
item in that cell; supported food uses the existing validated meal transaction.
An active tool supplies subsequent harvest intent, which the server still
independently validates. The M13 support-scroll integration supersedes the
earlier F1–F4/Q support-effect selection: a reusable scroll item occupies an
ordinary inventory cell, can be assigned to a numbered hotbar cell, and is
activated from that cell. Hotbar use sends the cell index only; the server
derives the current item and its allowlisted effect, checks learned entitlement
and existing activation gates, then routes through the support-magic authority
path. Scrolls are not consumed; the existing server cooldown controls reuse.
Each accepted support cast starts the shared, server-owned five-minute cooldown.
Hearth Shield, Bear's Vigor and Deer Call last three minutes; Mending is
instantaneous. Deer Call's bounded wildlife influence also lasts three minutes.
Only one support effect may be active per caster at a time. The current
controller D-pad/Q support-magic bindings are retired when this integration is
complete.

Below the grid, show one compact line such as `Armor 0 · Weight 8/300`.
Current armor remains 0 because the game has no armor-equipment records. Carry
weight includes stack quantities and tools. Current prototype defaults are
1 kg per item unit, 2 kg per tool, and a 300 kg displayed capacity.
`FKalmalaItemDefinition::WeightKg` is an optional validated catalogue value
and capacity is a component class default. These are presentation metrics; this
increment introduces no encumbrance penalty or weight-based transaction
rejection. Armor items, equipment rules, and final weight balance require
separate gameplay work.

## M13 inventory crafting companion

The 2026-10-10 UI request extends Inventory's existing Tab / I modal. Opening
it shows the same 10x4 inventory on the left and the standard player crafting
list and selected-recipe details on the right. The right panel reads the
existing owner-visible recipe and ingredient presentation; crafting continues
through the existing server-validated action. Station-specific service views
and the separate Build menu keep their current contexts. After implementation,
the presentation rules below supersede the selected-detail and scroll
presentation described earlier in this handoff; the grid and authority
contracts remain in effect.

On open, the inventory panel slides down from above and the crafting panel
slides in from the right. Both use the existing options-opening duration and
easing by default, and both appear immediately when opening animation is
disabled or the owner's Reduced motion preference is enabled. The panels are
one owner-local modal: they share existing close, cursor/input restoration, and
keyboard/controller focus behavior. Layout, focus, reduced motion, transition
start/interruption/completion, and recipe-action visibility are part of M13's
rendered host/client acceptance at supported viewport and accessibility
settings.

Keep the inventory's always-visible summary to the compact armor/weight line.
Do not show the slot-count line, a `Selected:` heading, control instructions,
or a persistent full-width item-detail card. Hovering an occupied cell opens a
nearby, dark, opaque detail tooltip with the item name, description and any
existing authored stats arranged as tidy, separate lines. Keyboard/controller
focus or selection on an occupied cell shows the same contextual details so
the information remains reachable without a pointer. Existing item actions
remain available only for applicable items and retain their current
server-validated path.

One original Kalmala wood texture spans the complete combined Inventory and
Crafting backplate, including the narrow inset and gap around and between the
panes. The Inventory pane and Crafting pane each use an opaque solid theme
fill. Inventory cells and crafting cards use opaque, slightly darker brown
rounded surfaces with a few UI units of gap and icon inset. Keep canonical
icons, quantities, condition bars, focus states and selected states legible
over those fills. Existing high-contrast behavior suppresses decorative wood
art and uses opaque black accessibility surfaces, white text and borders.

This is a UI presentation change only. It adds no recipe, crafting outcome,
RPC, replicated field, gameplay authority, or saved-data field. Tooltip stats
are limited to existing catalogue/tool data; this task adds no combat-stat
fields such as Pierce, Poison or Knockback.

## Authority and compatibility

The server reconciles trusted item stacks and tool records into one runtime
cell array, retaining valid positions and removing depleted identities. Cell
positions and active identity replicate with `COND_OwnerOnly`. A move RPC
carries only source/target indices and their expected identities: bounds,
current server ownership, stale contents, and request cadence are checked
before swapping. It cannot grant, delete, split, duplicate, repair, or author
quantities. Hotbar intent carries only an index in 0–9; the server derives its
identity and validates food through the existing transaction path.

Live grant, exchange, storage withdrawal and tool progression account for
both tools and stacks within forty cells. World chest capacity remains sixteen
stacks; its persistence validation and schema stay unchanged. No saved-data
field or version changes. Grid arrangement and active selection are session
state; reconnects rebuild arrangement from available holdings. Existing item
and tool persistence limitations still apply.

The forty-cell and companion-panel contracts supersede older sixteen-slot player pack, separate toolbelt, searchable player-grid, and persistent item-detail panel descriptions. Legacy station/chest inspectors remain scoped to their respective services. M13 companion and scroll handoffs are implementation reports, not fresh integrated rendered acceptance.

## Shared UI theme


The first foundation increment migrates the survival-status panel and weather
activity badge to `FKalmalaUITheme` shared panel/text styling. Edit
`Config/DefaultKalmalaTheme.ini`, section `[Kalmala.UI.Theme]`, and restart the
game/editor process. Unreal's local `KalmalaTheme` config hierarchy is loaded
once, read-only; widgets share the resolved theme. No live reload is supplied.

| Key | Default | Accepted values |
| --- | --- | --- |
| Panel | (0.025, 0.035, 0.04, 0.94) | Linear RGBA components in 0–1 |
| Heading | (0.75, 0.82, 0.79, 1) | Linear RGBA components in 0–1 |
| Text | (0.93, 0.96, 0.94, 1) | Linear RGBA components in 0–1 |
| HighContrastPanel | (0, 0, 0, 0.98) | RGB always black; alpha at least 0.98 |
| HeadingSize | 10 | 8–32, rounded to nearest integer |
| BodySize | 13 | 8–32, rounded to nearest integer |
| EmphasisSize | 17 | 8–32, rounded to nearest integer |
| PaddingX / PaddingY | 12 / 9 | 0–24 logical units |
| RowSpacing | 5 | 0–16 logical units; survival rows only |

Colours use `(R=...,G=...,B=...,A=...)` syntax. Missing, malformed,
non-finite, and out-of-range values fall back independently to the established
defaults. Text falls back to Unreal's existing default font. Local 100/125/150% text
scale applies after theme sizes; local high contrast forces white text on a
black panel. The weather badge now uses the same panel padding as survival
status. No input, owner-state query, RPC, timer, authority, or gameplay/save
contract changes.

Verify the foundation with `Kalmala.UI.Theme.LocalPresentation`,
`Kalmala.UI.SurvivalStatus.LocalPresentation`, and
`Kalmala.UI.WeatherActivity.LocalPresentation` using the isolated headless
pattern in `07-development-setup.md`. Theme automation checks shared widget
application, accessibility precedence, malformed values, and missing config.
It does not establish rendered layout or packaged config inclusion.

The second foundation child adds shared components with real consumers:
settings buttons (including controls and tabs) use themed normal/hover/pressed/
disabled brushes and content padding; the settings panel uses the shared panel
and optional image; its ordinary action labels use themed font styling; the
Controls scroll view uses theme animation; existing support glyph boxes use
shared icon dimensions. HUD panels/text also consume the extended styling.
The final foundation child migrates representative inventory, build/craft,
expanded-map, and options views as described below.

| Extension key | Default | Accepted values / consumer |
| --- | --- | --- |
| FontAsset | empty | Project-owned runtime UFont object path; shared text |
| FontFace / HeadingFace | Regular / Bold | Regular or Bold; missing custom face uses font default |
| PanelImage | empty | Project-owned Texture2D object path; shared panels |
| InventoryPanelImage | `/Game/Kalmala/UI/InventoryPanel.InventoryPanel` | Texture2D for the shared Inventory-and-Crafting backplate |
| BuildPanelImage | `/Game/Kalmala/UI/BuildPanel.BuildPanel` | Texture2D override for build/craft selection |
| WorldMapPanelImage | `/Game/Kalmala/UI/WorldMapPanel.WorldMapPanel` | Texture2D override for the expanded map shell |
| EscapePanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Fallback source for empty option-tab overrides; M13 requests a transparent Escape home menu |
| VideoOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Video tab; empty falls back to EscapePanelImage |
| AudioOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Audio tab; empty falls back to EscapePanelImage |
| ControlsOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Controls tab; empty falls back to EscapePanelImage |
| SettingsOptionsPanelImage | `/Game/Kalmala/UI/OptionsPanel.OptionsPanel` | Settings tab; empty falls back to EscapePanelImage |
| OutlineSize | 0 | 0–3, rounded; black text outline |
| BorderWidth / CornerRadius | 1 / 3 | 0–3 / 0–12; geometric panels and button states |
| BorderColor | (0.35, 0.45, 0.42, 1) | Linear RGBA 0–1 |
| ButtonNormal | (0.11, 0.16, 0.19, 1) | Linear RGBA 0–1 |
| ButtonHovered | (0.18, 0.25, 0.28, 1) | Linear RGBA 0–1 |
| ButtonPressed | (0.07, 0.10, 0.12, 1) | Linear RGBA 0–1 |
| ButtonDisabled | (0.08, 0.08, 0.08, 1) | Linear RGBA 0–1 |
| SlotPadding | 3 | 0–16; button content, pressed adds one unit |
| IconWidth / IconHeight | 64 / 42 | 24–96; existing support glyph slots |
| AnimateScrolling / ScrollSpeed | False / 15 | Boolean / 1–60; Controls wheel scrolling |
| AnimateOptionsOpening | True | Boolean; set False for an instant/reduced-motion opening |
| OptionsOpeningDuration | 0.18 seconds | 0–0.8 seconds; zero selects the instant path |
| OptionsOpeningTravel | 32 logical units | 0–96; zero selects the instant path |
| OptionsOpeningEasing | `EaseOutCubic` | `EaseOutCubic`, `EaseOutQuad`, or `Linear` |
| FavoriteMarkerStyle | `Both` | `Star`, `Border`, or `Both`; local recipe/build cards |
| FavoriteMarkerBorderWidth | 2 logical units | 1–4; Favorite frame width when enabled |
| FavoriteMarkerColor | (0.98, 0.78, 0.20, 1) | Linear RGBA; Favorite star/frame |
| RankGoldColor / RankSilverColor / RankBronzeColor | gold / silver / bronze tones | Linear RGBA; top-three usage markers |
| RecentMarkerColor | (0.38, 0.82, 0.90, 1) | Linear RGBA; Recent clock badge |

Asset keys accept only valid `/Game/Package.Asset` object paths shorter than
180 characters. Empty, invalid, missing, or wrong-type objects retain the
built-in font or geometric panel. Offline bitmap fonts also fall back safely. No asset is imported/generated by this
increment. A panel image replaces the geometric panel border; high contrast
ignores it, uses a black-backed panel and white border/text, and forces button
borders to at least one unit. High contrast uses neutral button fills; pressed
padding and Unreal's existing keyboard focus indicator remain available.
Accessibility text scale still takes precedence over theme size. No focus,
click delegate, navigation, modal ownership, authority, or save binding changed.

Recipe/build activity cards reserve separate right-aligned rows for Rank and
Recent, followed by the Favorite cue at the lower right, so a single card can
show all three without overlap. Favorite style selects a star, an outer frame,
or both; Favorite text is always present. Rank markers name Gold/Silver/Bronze
and their rank. Recent uses a clock glyph and the word Recent. Local high
contrast forces marker text and the Favorite frame to white; static labels
remain visible under reduced motion. Marker changes are immediate and do not
animate.

The view-specific image keys override `PanelImage` only for their named view.
The inventory, build, and expanded-map textures are original Kalmala artwork
imported from `Content/Kalmala/UI/Source/` into `/Game/Kalmala/UI`; missing or
invalid view paths keep the geometric fallback. The Inventory texture covers
the shared backplate behind both Inventory and Crafting panes; each pane uses
an opaque solid theme fill, and high contrast suppresses the decorative wood.

`ApplyScroll` has an explicit reduced-motion override; scroll animation defaults off.
The local Reduced motion choice also overrides scrolling. The options-opening
slide is documented below. Custom
font and image packages must already be available to the runtime; this child
does not establish cooking/inclusion of config-only asset references.

Focused theme automation exercises changed brushes, padding, font face/outline,
icon dimensions, scroll animation and reduced-motion override, high contrast,
invalid numbers/paths, missing assets, and a transient in-memory image reference.
The settings and Inventory menu tests remain required alongside it. No
custom project font or rendered image appearance has been reviewed.

Hover, keyboard/controller focus, selection, and disabled states share theme brushes and non-colour cues. Decorative images/animation yield to local high contrast and reduced motion. Inline Forge Upgrade comparison reads current/target authored axe level and condition; equal values are neutral, absent axes have an explicit missing state, and other service views collapse the comparison. No invented combat statistics are displayed.

## Status and catalogue icons

The active-only owner HUD is transparent, non-focusable, and hit-test-invisible, beside the minimap. Stable order is Wet, meal, Hot, Cold, current support, then Storm. Known active effects appear once; expired entries disappear immediately and empty groups collapse. Finite player/support timers use authoritative remaining time or synchronized server expiry. Hot/Cold/Storm have no countdown. Storm requires the existing 0.65 threshold and a current server weather interval. Start/refresh cues are local; obsolete M11 Ended rows must not survive M12's active-only contract.

Each raster fills its scaled 64x64 box. Status layout wraps within a 384-unit row cap and preserves minimap/support separation. Visible names are omitted but hidden accessible labels remain; Escape's status/weather details provide text access without hover.

The accepted catalogue icon baseline covers 48 canonical item/tool/construction identities and nine status identities. Views reuse canonical IDs and one image per identity; aliases and recipes do not get duplicate art. Unknown IDs retain a question-mark fallback; missing imported textures retain existing vector fallbacks. Current runtime/catalogue additions must receive explicit coverage rather than assuming the historical counts include them.

Original PNGs live in Content/Kalmala/UI/Source/IconOriginals and prepared PNGs in Source/Icons below that UI directory. Catalogue and status importers prepare transparent 64x64 artwork and import UI textures with sRGB, no mipmaps, and no streaming. Code constructs icon paths dynamically; both catalogue/status directories must be included in package cooks. The CSV manifests are machine inputs used by import/validation scripts, not disposable narrative reports. Their pre-existing working-tree deletions were preserved by this cleanup; those tools require the manifests to be supplied/restored before use.

## Item and recipe details

Details use canonical item/tool name, description, image, quantities, and existing condition/level/food stats only. Empty/removed selections clear stale details; unknown identities use an explicit fallback. M13 Inventory uses hover/focus tooltips and compact armor/weight, superseding its old persistent detail card.

Ingredients show owner-held quantities against one-batch requirements, with explicit Enough/Missing or Waiting for pack. Direct builds use raw material costs. Requirement blocks describe applicable station alternatives, reusable tools, hearth heat, and existing placement limits; sufficient ingredients never promise server acceptance.

## M12 construction detail template

Direct construction details use the existing result description once, followed
by the existing owner-local ingredient counts and a compact Build requirements
block. It names the carried Construction Hammer state, clear/dry/gently sloped
placement, and the hearth's one-of-four raw-fuel requirement and 60-second
starting duration. The current first blocker appears once when present; a
ready recipe does not add success, skill, unlock, request, or rejection prose.
The result name and material totals stay in their existing header and ingredient
rows. Catalogue descriptions and all recipe, cost, placement, authority, and
save behavior remain unchanged.

The `Kalmala.UI.Crafting.Requirements` contract covers each direct build's
placement and single-blocker copy, the hearth fuel summary, pending/present/
missing hammer states, and omission of generic boilerplate. The rendered
Verify-Crafting presentation gate checks the selected hearth copy and ensures
its old repeated costs, output label, skill list, and request prose are absent.
Build, full automation, and rendered verification remain deferred to M12's
milestone-final run.

## M12 food and general recipe detail template

Non-construction recipe details show the canonical output description once,
the existing owner-local ingredient rows, and a compact requirement summary.
The summary names the actual result count per batch, one batch per menu press,
and the catalogue's supported maximum batch when it is greater than one. It
lists only real station alternatives, the cooking-heat rule for rack/cauldron/
pan recipes, and a reusable tool when the recipe requires one. The first
current availability blocker appears once; a ready recipe has no synthetic
success message. Recipes without station or reusable-tool requirements get no
empty-state lines, and the selected details omit unrelated skill lists,
no-lock claims, output-stack limits, request/rejection boilerplate, and repeated
ingredient totals. Catalogue prose and server recipe validation remain intact.

`Kalmala.UI.Crafting.Requirements` checks result/quantity coverage, station and
heat requirements, reusable-tool text, omission of generic boilerplate and
single-blocker behavior. `Kalmala.Gameplay.Food.CookingStationHeat` continues
to verify the server heat gate and confirms the detail description comes from
the canonical result item. The rendered Verify-Crafting review checks the live
cooking result, ingredients, heat line and blocker count for both owners. Build,
full automation and rendered checks remain deferred to M12's milestone-final
run.

Catalogue prose is authored manually in Content/Data/GameCatalogues.json; tool prose comes from tool definitions. Preserve intentional flavor. Player-facing names omit internal Kit suffixes while stable runtime/save aliases remain unchanged. Retired Fuel, Timber, ConstructionSupply, RaisedStorage and Smoke Frame names must not reappear as active content. The copy audit and image-generation batch diaries are historical; current data and lookup code define live copy/identity.

## Menu browsing

Recipe, Build, station, and Chest inspector filters are owner-local presentation. Search trims to 64 characters and matches visible names case-insensitively, not hidden IDs/descriptions. Sorting works on copies, keeps canonical tie-breakers and original owner order, and never changes inventory or save order. Preserve a still-visible selection, otherwise select the first result; empty results clear details/actions and expose recovery controls. Text editing owns cursor keys.

Local per-menu memory keeps query/category/sort/selection/scroll in the widget session and clamps offsets after layout/viewport changes. Station scopes remain separate. Build categories contain only supported structural pieces, stations, and camp utilities; production remains in the corresponding station service. The expanded map retains its local view/filter selection for that session, and personal pins/exploration use only their existing local save owners.

## M13 requested Build-menu refresh

The 2026-10-10 M13 request supersedes the standalone Build menu's older entry,
card, search-memory, and lighting-button behavior above. With the Construction
Hammer active in gameplay, right-click opens Build; the existing remappable
keyboard/controller entry remains available. Build cards display only their
original Kalmala icon. Hovering or focusing a card shows its name, short
description, applicable required-station icon, and existing material costs and
owner-visible carried counts in a tidy detail area below the menu. Existing
category/filter and grouping controls move into the top row as icon-only
controls, with no visible text in the menu header. Controls retain accessible
names and useful hover/focus labels.

Left-clicking a build card (or activating its focused entry) closes Build and
enters the existing local placement mode for that canonical buildable; selection
does not place it. Right-clicking a card toggles its existing local Favorite
state and does not trigger menu entry, close, or placement. The separate **Add
to favorites**, **Build / place selected**, and **Light hearth** actions are
removed. Existing placement controls and server validation remain in effect.
The build search does not receive focus on open; clear its query on menu close
and when placement mode starts. Other existing per-menu browse memory may remain.

Lighting moves to the Campfire's direct **Interact** action. When a visible,
same-world Campfire is unlit and eligible under existing server rules, Interact
shows **Light Campfire** and reuses the existing payload-free lighting intent.
Otherwise, the existing **Add fuel** interaction remains available when
applicable. The server still validates target visibility, access, range, fuel,
and fuel condition; this request adds no new client authority, recipe, cost, or
saved state. The attached screenshot guides layout only; use existing Kalmala
icons and UI materials.

M13 rendered/input verification replaces the earlier retained-relight-button
expectation: confirm those three named buttons are absent; test right-click
menu entry and Favorite toggling, left-click/focused entry into placement mode,
search clearing and initial focus, and direct Light Campfire interaction on
eligible host/client-owned fires. Existing server rejection/payment checks and
placement validation remain required.

This M13 request remains governed by the live backlog; do not infer completion from its specification.

## Owner notifications


The first M11 child adds passive icon/text feedback for skill **levels**, not
each experience award. The local-player subsystem reads only its local
controller's owning pawn for detailed progression and accepted item/discovery
feedback. It never reads peer-private presentation or changes gameplay state.
Gameplay's existing server-owned ledgers and owner-only replication remain the
authority boundary.

The first complete valid six-skill snapshot is a silent baseline. Missing,
partial, duplicate or invalid snapshots cannot advance that baseline. A later
level increase displays the skill name and reached level with original shared
line art. Repeated replication and XP changes within a level do not renew or
replay feedback. Pawn replacement clears the queue and baseline; a level
rollback silently rebaselines all skills. Reconnect therefore does not announce
existing levels. These are transient session notices, with no save-schema change.

At most three rows are active. A further increase of the same active skill
updates its reached level and renews its lifetime. New skills evict the oldest
row at capacity; simultaneous increases follow canonical skill order. Expired
rows are removed and never reconstructed from unchanged snapshots.
`NotificationLifetime` in `DefaultKalmalaTheme.ini` defaults to four seconds;
theme values outside 1–10 seconds fall back to the default. Runtime queue
durations are also bounded. No entrance, exit or scrolling animation occurs,
including reduced-motion use. Text scale and high contrast use the shared theme.

The widget cannot focus or hit-test. Its bottom-right anchor is 280 logical
pixels above the viewport bottom, with a maximum width of 320; this clears the
centered arrival prompt at the supported 1024x768/150% layout.
It collapses while the controller ignores movement for a modal; timers continue
so closing a modal does not replay expired messages. Tiny/unavailable viewports
collapse the widget. The combined acceptance below checks both supported
layouts and the existing movement-ignore modal signal; it does not open a
gameplay menu.

## Accepted item gains

Accepted server inventory publications send a reliable client-only
`ClientAcceptedGain` receipt to the owning connection. Direct grants record the
accepted quantity. Exchanges, validated candidate commits and successful
storage withdrawals record each positive net item increase after publication;
consumption, deposits, unchanged commits, rejected/stale candidates and failed
storage writes produce no gain. This feedback adds no server mutation RPC and
cannot mint or consume inventory. Receipts do not depend on a later inventory
snapshot, so gain followed by consumption between UI refreshes still announces
the accepted gain. A same-item exchange announces its positive **net** increase,
not its gross output. Carried-tool records are a separate inventory contract and
are not newly announced by this child.

The owner stores only the latest 32 transient receipts with locally monotonic
sequence numbers. The first observed buffer silently establishes a baseline,
including already-delivered receipts on pawn attachment. Pawn replacement
clears it through the existing session reset. Refresh, expiry, modal close and
unchanged buffers never replay receipts. A UI polling gap exceeding 32 receipts
drops older notices; it cannot change inventory. There is no receipt persistence
or save-schema change. Reliable delivery follows the existing pawn's owning
connection; there is no multicast or peer detail replication.

New item receipts share the existing three-row queue with skill notices. Same
canonical item gains coalesce into a summed `Gained N Item` label and renew the
bounded lifetime; summed labels saturate at int32 maximum. Overflow evicts the
oldest row. Canonical catalogue names and original catalogue icons are used.
Late initial skill replication does not clear already accepted item feedback.

Run `Kalmala.UI.Notifications.ItemGains` and `.SkillLevels`,
`Kalmala.Gameplay.Inventory.NetworkContract`, `Kalmala.Gameplay.Crafting.Transactions`
and `Kalmala.UI.Theme.LocalPresentation` after the affected editor build.
`Scripts/Verify-Inventory.ps1` additionally requires live owning-client receipt
presence/bounds and empty remote receipt buffers, alongside existing inventory
privacy and rejected-client-mutation gates. Rendered combined notification
placement and real reconnect pixels remain final-child acceptance scope.

## Accepted discoveries

The local notification subsystem also observes the owning pawn's existing
owner-only discovery acknowledgement. Its feedback serial establishes a silent
baseline on first observation and after pawn replacement, so joining or
reconnecting with an existing acknowledgement does not replay it. A newer
`LandmarkFound`, `ScrollFound`, `AlreadyFound`, or `Unavailable` serial adds the
server-provided acknowledgement label as a passive discovery row. Empty labels
use concise outcome-specific fallbacks, and all labels are trimmed and bounded
to 48 characters. An unchanged serial does not renew the row, and expired
feedback is never reconstructed from the last replicated label.

Discovery rows share the three-row, theme-timed queue and use an original
monochrome inspection glyph. Labels are trimmed and bounded to 48 characters,
with a safe fallback for an empty accepted label. They are transient, local
presentation: no new request, claim, reward, persistence, RPC, peer query, or
saved-data field was added. Accepted discovery rewards may also produce an
existing item-gain notice for the actual inventory increase.

Run `Kalmala.UI.Notifications.Discoveries` with the affected editor build. It
covers silent initial/reconnect baselines, accepted/already-found/unavailable
acknowledgements, unchanged refresh, expiry, label bounds, passive text and
owner-local queue behavior.

## Combat and support action results

The local queue also observes the owner's existing owner-only combat and
support feedback serials. Initial attachment and pawn replacement silently
baseline both values; a newer serial adds one short text-and-icon notice for
`Hit confirmed`, `Defeated`, `Attack unavailable`, `Support accepted`, or
`Support unavailable`. Repeated snapshots do not renew a notice, serial
rollback silently rebaselines, and the combat notice contains no target name or
identity. Discovery acknowledgements remain their own existing notice source.

These events use the same three-row bound, theme lifetime, passive presentation,
and modal collapse as skill, item-gain, and discovery notices. The notification
subsystem reads only the local owning pawn. The server-owned components still
validate and publish every result; this presentation adds no request, RPC,
replicated field, save data, or gameplay mutation. The Inventory menu no longer
duplicates combat, support, or discovery result text.

`Kalmala.UI.Notifications.CombinedPresentation` checks silent action baselines,
serial deduplication, concise text, expiry, and absence of hidden target
identity. The existing rendered capture fixture continues to review the
skill/item/discovery combination; action-result rendering remains part of the
M12 milestone-final HUD matrix.

## Recipe and build activity

## Ownership and lifetime

Favorites, usage counts, rank results, and Recent shortcuts are local
presentation state owned by the `ULocalPlayer` crafting UI subsystem. They do
not change catalogue definitions, availability, server transactions, gameplay
state, replicated properties, or world/player save schemas. They survive menu
close/reopen and pawn replacement while that local-player subsystem exists;
they are cleared when it is destroyed and are not persisted across process
restarts. The local UI never infers an accepted action from selection, a button
press, inventory appearance alone, another peer, or a replayed snapshot.

Bookmarks use canonical active `RecipeId` values. Unknown or retired IDs are
ignored and pruned. The set is bounded by the smaller of 256 entries and the
current recipe catalogue size. The Favorites category intersects the existing
station scope and search query and keeps the existing unavailable-state text;
it does not add hidden entries or change recipe ordering. Removing the selected
bookmark from Favorites follows the menu's established selection fallback,
and an empty result disables the existing action.

## Accepted-action history contract

The crafting component emits owner-only accepted-action receipts only after an
existing server-side craft or placement has committed. Each receipt carries a
monotonic sequence for that character component, a canonical recipe/menu ID,
and one of the three action kinds below. The server retains a rolling maximum
of 64 receipts so multiple accepted actions can survive property replication
coalescing; the 64-bit sequence saturates instead of wrapping. The local
subsystem silently establishes a baseline on first
observation and pawn replacement, then consumes each newer sequence once. A
component replacement does not reset the local player's accumulated history.
Rejected, failed, stale, duplicate, replayed, selected, previewed, or merely
requested actions do not update history. The receipt reports an outcome
already chosen by the server; it cannot authorize a transaction or supply a
reward. A build output with no unique current recipe/menu ID is omitted.

Accepted actions have three disjoint buckets:

- `BuiltPiece`: successful direct placement of a construction piece.
- `CookedRecipe`: successful recipe action whose existing catalogue skill is
  Cooking.
- `CraftedItem`: successful non-cooking recipe action. Cooking never enters
  this bucket.

Counters are local, non-negative, saturating integers keyed to current
canonical menu IDs. Records for IDs absent from the active catalogue are
pruned, and unused entries with count zero receive no rank. Each bucket ranks
at most three non-zero entries by descending accepted-action count, then by
`RecipeId.LexicalLess` for a locale-independent deterministic tie. The displayed
medals are gold, silver, and bronze in that order; ties do not share a medal.

Recent state holds at most one latest accepted ID per bucket. A new accepted
build replaces only the previous built piece; a new cooking action replaces
only the previous cooked recipe; a new other-crafting action replaces only
the previous crafted item. The Favorites category includes these current
Recent shortcuts even when they were not manually bookmarked. A canonical ID
appearing in multiple sources is shown once, with separate text markers so
Favorite, Rank, and Recent remain distinguishable. Ordinary recipe/build
cards also mark their matching Recent entry.

The local-player subsystem exposes bucket count, rank, and Recent queries for
the menu presentation. It validates that receipt IDs still resolve in the
current catalogue and that the receipt kind agrees with the existing Cooking
classification or build-menu membership. Each count map holds at most one entry
per current recipe ID per bucket and saturates at the unsigned 32-bit maximum.

## Presentation contract

Favorites, rank, and Recent remain distinguishable without colour. The theme
configures Favorite treatment as a bottom-right star, border, or both; rank
treatment uses a medal-coloured label with `Rank` text; and Recent uses a clock
badge plus `Recent` text. The default Favorite treatment is both star and
frame. Cards reserve separate right-aligned Rank and Recent rows, then place
Favorite at the lower right; all three labels retain their meaning and do not
overlap. Reduced motion keeps marker changes immediate and static; it does not
suppress an essential marker. Existing text scale, contrast, focus, selection,
and modal input behavior continue to apply.

## Onboarding reference


**M12 runtime status:** the local tutorial presenter is disabled. No arrival or
contextual help card appears during fresh start, pawn transition, or reconnect,
and no replacement banner is shown. The prompt rules and beat matrix below are
retained as a route-free design reference if onboarding returns in a later
approved increment.

This document defines optional, local onboarding prompts for a fresh player.
Prompts teach the existing Kalmala loop without turning the generated wilderness
into a route, quest chain, or mandatory camp. The contract is deliberately
presentation-only; it does not add a gameplay action, server state, save field,
replicated property, or online service.

## Prompt rules

- Prompts are short, dismissible, and contextual. They may appear once per
  local first-run profile, but completing or dismissing one never changes a
  server-owned value.
- A prompt may trigger from a normal local input, a readable replicated state,
  or an already visible actor. It must not query hidden population, undiscovered
  rewards, private pins, target IDs, or future route information.
- Prompts describe choices rather than objectives and use concise action names;
  they must not select a destination, camp site, creature, reward, recipe
  outcome, or support target.
- Input-binding labels are not displayed in prompts. Key/button names and
  control legends stay in Options > Controls. Prompt text and non-colour
  icon/shape cues remain available, and a player can dismiss, revisit, or
  ignore all prompts without losing normal movement or menu access.
- Prompt timing is local presentation timing. It cannot pause the server, grant
  an interaction, alter stamina or Wet, confirm a hit, complete a discovery, or
  advance a reward.

## Route-free beat matrix

| Beat | Local trigger | Player-facing prompt | Required boundary |
| --- | --- | --- | --- |
| Arrive | Fresh local pawn becomes controllable | “Move and look around. Jump or sprint when the terrain calls for it.” | Does not name a direction, coordinate, route, or destination. |
| Interact | The player first receives normal interaction focus on a visible actor | “Face a nearby usable object and choose its displayed action.” | The prompt reflects only the existing local focus; the server still validates the request and outcome. |
| Gather | The player has a visible, usable harvest node in normal range | “Gather from the visible node. Your pack shows what was accepted.” | Does not reveal distant nodes, quantities, depleted state, or a preferred resource. |
| Prepare | The player opens the existing camp crafting UI | “Choose what to make, then place it where the terrain and your materials allow.” | No fixed camp, construction transform, payment result, or shelter success is promised locally. |
| Weather | The local HUD already shows an active Wet or exposure-related cue | “Weather changes comfort and travel. Shelter, cover, and a lit hearth are options.” | Uses existing replicated presentation only; it never invents a local weather value or a guaranteed recovery. |
| Explore | The player leaves the immediate start area through normal movement | “Pick a heading and see what the generated land offers. The map is for orientation.” | Never points to an authored corridor, biome, encounter, discovery, or required return point. |
| Optional encounter | A relevant creature is already visible or the player has initiated the normal attack intent | “You can engage or move on. Attacks are committed by the server.” | Does not reveal hidden creatures, target selection, damage, cooldown, loot, or defeat outcome. |
| Discovery | A visible server-owned discovery is already interactable | “This discovery is optional. Interact normally if you want to investigate.” | Does not show undiscovered IDs, reward contents, private entitlement, or duplicate-claim state. |
| Support magic | The entitled player has already learned an effect and opens its existing local UI | “Support effects help an eligible ally or situation; choose a valid target when the UI allows.” | Learned effects, targets, durations, stamina, cooldowns, and execution remain server-owned; no effect deals direct damage. |
| Return | The player chooses to head back or remains near a visible camp | “You can return, shelter, use the hearth, store materials, or keep exploring.” | Return is a choice, not a quest completion, timer, route requirement, or persistence grant. |

The first three beats introduce movement, interaction context, and gathering.
The remaining beats are opportunistic: a player may encounter them in any
order, never encounter some of them, or dismiss them all. A fresh-player
walkthrough may therefore record which prompts were applicable rather than
requiring every prompt to appear.

## Presentation and accessibility acceptance

If onboarding returns in a later increment, a fresh local session must
demonstrate the following without developer commands or fixed fixture
coordinates:

1. Arrival teaches movement, jump, and sprint through plain action names, with
   no key/button legend.
2. A visible normal interaction can teach interaction and gathering without
   promising acceptance before the server response.
3. Camp, weather, exploration, optional encounter, discovery, support, and
   return prompts appear only when their local visible context exists.
4. Every prompt can be understood from text and shape/icon cues without colour
   or audio. Keyboard/controller mappings remain readable in Options > Controls,
   and prompts retain their full action text.
5. Dismissing or ignoring prompts leaves movement, interaction, combat intent,
   support UI, map access, and reconnect behaviour unchanged.
6. A two-player observation shows no prompt-driven replication, reward leak,
   client-selected outcome, or private discovery/learned-effect disclosure.

The former runtime presenter was a `ULocalPlayerSubsystem`. Its source remains
as a dormant design implementation, but its tick is disabled and it cannot
mount a card. The default F1/F2 dismiss/revisit mappings have been removed with
the gameplay prompts. The source-level route-free audit now also verifies that
the presenter remains inactive.

`Scripts/Verify-OnboardingContract.ps1` validates the retained design
specification; it is not a runtime presentation test. `Scripts/Verify-TutorialRouteFree.ps1`
checks the route-free source contract and confirms that the presenter is
disabled. `Scripts/Verify-LocalInputContract.ps1` checks the current
keyboard/controller movement and interaction bindings. Fresh-player prompt
walkthrough and prompt rendering are not part of current runtime acceptance.

## Authority, privacy, and persistence

Prompts are local UI state. The server continues to own world identity,
terrain, population, interactions, weather, exposure, construction, inventory,
combat targeting and damage, discoveries, learned effects, rewards, and sparse
world/player saves. A client may observe only the same relevant replicated
actors and values it could already observe during normal play. No prompt sends
a new RPC or carries an item ID, target, quantity, damage, transform, reward,
weather, status, or effect payload.

Prompt dismissal and history are session-local. A future local settings option
may persist a prompt preference; it must never be included in the gameplay save schema
or replicated to another player.


## Settings and accessibility


This document defines the local settings and accessibility surface for the
existing Kalmala settings shell. It covers presentation and input preferences
only; it does not add gameplay tuning, a replicated option, a server setting,
or a new save schema.

M11 places Wet, meal, exposure, support and weather status markers in one
transparent owner-local hotbar. Escape > Status and weather details exposes
their live text without hover. The optional feedback overlay retains nearby
hearth/construction context, with no duplicate active status rows.
Owner-local notifications now carry concise combat/support outcomes and
discovery acknowledgements; a separate text-plus-glyph strip shows the selected
support effect without control-binding labels. The colour-independent feedback
system covers Wet, hearth, construction, combat, discovery, and support: the
optional owner-only overlay shows nearby hearth/construction context while the
other cues use their dedicated text/icon HUD surfaces. See
`02-technical-architecture.md` for the status presentation contract and
`02-technical-architecture.md` for feedback rules.

Gameplay HUD and interaction prompts name the visible action without showing
key/button names or control legends. Inventory and build/crafting/repair/storage
views, the expanded map, status/detail views, and the Settings home screen follow
the same rule while keeping useful action/status names and existing focus
navigation. Options > Controls is the only player-facing view that displays
current mappings; prompt actions stay readable as text and remain remappable.

## Existing shell and option groups

The existing local menu opens and closes with **Escape**, owns modal input while
open, and exposes the Options and Quit actions. Options already contains the
Video, Audio, Controls, and Settings tabs. Video currently applies local
resolution, V-Sync, window mode, and render-distance quality through Unreal's
local `GameUserSettings` path. The remaining groups are the ordered M5 surface
to implement:

- **Audio:** local master, music, ambient, and interaction/combat feedback
  levels plus a mute path. Audio changes must have a readable text confirmation
  and must not be required to understand gameplay state.
- **Controls:** local keyboard/controller bindings for the existing movement,
  look, jump, sprint, interact, attack, map, recenter, settings, and craft
  actions. The Controls tab now exposes bounded keyboard/controller choices,
  shows each current label, applies a change to only the owning local
  `UPlayerInput`, and provides a restore-defaults action without changing what
  the server validates.
- **Settings:** local text scale and contrast choices, together with the
  colour-independent feedback preference for the optional nearby hearth and
  construction text overlay. Combat/support outcomes, discovery notices, and
  selected-support text remain explicit in their separate HUD surfaces.

The Escape home/settings shell and all four option tabs use the local shared UI
theme for their background image. Separate per-view keys allow a theme to
customize the shell and tabs independently; the default reuses one original
spruce-and-slate texture. High contrast suppresses decorative images and keeps
the black panel, white text, and focus borders. This presentation does not
change settings, input bindings, or modal ownership.

The Escape panel opens with a short theme-configured downward slide. Focus and
modal input ownership take effect immediately, and closing/reopening or
resizing during the animation does not delay gameplay actions or change menu
content. Theme configuration can disable the animation for an instant path, and
the local Reduced motion setting below also overrides decorative animation.

The M13 presentation request makes the main Escape home menu transparent over
gameplay, with a visible theme highlight for hovered and keyboard/controller-
focused options. High contrast must retain a clear non-colour focus cue. This
does not change the separate Options tabs or the menu's existing input ownership.

The later 2026-10-10 M13 Settings refresh request uses the same original wood
texture as the Inventory backplate and the shared theme's button treatment.
The Escape home list remains transparent. High contrast continues to replace
the decorative texture with an opaque black surface. A follow-up request asks
for the option groups and change controls to follow the attached references;
the candidate options and their current selection state are listed below.

## M13 Settings groups and reference-option checklist — 2026-10-10

Use the six reference group names: **Gameplay**, **Keyboard & Mouse**,
**Controller**, **Graphics**, **Audio**, and **Accessibility**. Move Kalmala's
existing Video options into Graphics, split existing Controls by device family,
and place existing Settings options in Accessibility. Audio remains Audio.
Gameplay currently has no documented Kalmala setting. Do not leave an empty
group visible if no Gameplay candidate is selected. Keep all existing Kalmala
settings available regardless of the candidate checks below.

The checkboxes are for the user to select candidate options for later M13
scope; checking one does **not** mean it has been implemented. New candidates
must have a Kalmala-supported behavior, use local configuration, and remain
reachable with keyboard/controller focus. Their default, range, and persistence
behavior should be specified when selected. Do not copy reference artwork.

### Gameplay candidates

- [ ] Language selector — previous/next arrows cycle available languages.
- [ ] Auto-run — on/off toggle.
- [ ] Attack towards look direction — on/off toggle.
- [ ] Show button hints — on/off toggle.
- [ ] Enable game hints — on/off toggle. (add but hide for now)
- [ ] Reduce background performance — on/off toggle.
- [ ] Enable Console — on/off toggle.
- [ ] Show build piece author — on/off toggle.
- [ ] Skip intro cinematic — on/off toggle. (add but hide for now)
- [ ] Auto-backup history — slider with a visible selected count.

### Keyboard & Mouse candidates

- [ ] Mouse sensitivity — slider with a visible percentage/value. [1% to 1000%]
- [ ] Invert mouse — on/off toggle.
- [ ] Direct keyboard/mouse rebinding — show one row per action and its current
  binding; activating a binding captures the next valid key or mouse input,
  Escape cancels capture, and Reset controls restores defaults. This replaces
  the current bounded-choice interaction if selected.
  - actions:
    - Attack
    - Secondary Attack
    - Block
    - Use (Action)
    - Jump
    - Run
    - Crouch
    - Dodge
    - Alternative Dodge
    - Holster Weapon
    - Hotbar keys for 1,2,3,4,5,6,7,8,9,0
    - Move keys Forward, Backward, Left, Right
    - Inventory
    - Alternative Inventory
    - Map
    - Map Zoom in
    - Map zoom out
    - Deconstruct (while holding hammer)
    - Alternative placement (while holding hammer, normal by attack key)
    - Prev snap point (in build mode)
    - Next snap point (in build mode)


### Controller candidates

- [ ] Gamepad enabled — on/off toggle.
- [ ] Swap triggers — on/off toggle.
- [ ] Invert camera X axis — on/off toggle.
- [ ] Invert camera Y axis — on/off toggle.
- [ ] Vibration strength — slider with a visible percentage/value.
- [ ] Controller sensitivity — slider with a visible percentage/value.
- [ ] Glyphs — selector; previous/next arrows cycle supported glyph styles. (xbox/ps/switch controller)
- Same re-bindable actions as above.

### Graphics candidates

- [ ] Resolution — selector cycles supported limits.
- [ ] Full screen - on/off toggle.
- [ ] 3D resolution limit — selector cycles supported limits.
- [ ] Upscaling method — selector cycles supported methods.
- [ ] Framerate limit — slider, including the supported Unlimited choice.
- [ ] Graphics preset — previous/next selector, including Custom when applicable.
- [ ] Vegetation quality — slider.
- [ ] Level of detail — slider.
- [ ] Particle lights — slider.
- [ ] Shadow quality — slider.
- [ ] Active point lights — slider.
- [ ] Active point light shadows — slider.
- [ ] SSAO — slider.
- [ ] Cloth quality — slider.
- [ ] Draw distance — slider with a readable value.
- [ ] Distant shadows — on/off toggle.
- [ ] Tessellation — on/off toggle.
- [ ] Bloom — on/off toggle.
- [ ] Depth of field — on/off toggle.
- [ ] Motion blur — on/off toggle.
- [ ] Chromatic aberration — on/off toggle.
- [ ] Sun shafts — on/off toggle.
- [ ] Soft particles — on/off toggle.
- [ ] Anti-aliasing — on/off toggle.

### Audio candidates

- [ ] Replace the existing 25%-step audio controls with sliders for the same
  master, ambient, music, and interaction/combat-feedback settings; keep
  visible percentage/value labels.
- [ ] Environmental — volume slider with a visible percentage/value.
- [ ] Continuous music — on/off toggle.

### Accessibility candidates

- [ ] Immersive camera — on/off toggle.
- [ ] Camera shake — slider 0-100%.
- [ ] Reduce flashing lights — on/off toggle.
- [ ] Toggle block — on/off toggle.

### Existing Kalmala settings retained in the new groups

- **Graphics** (currently Video): resolution, V-Sync, window mode, and
  render-distance quality. Keep these settings; their screenshot-like controls
  are a resolution selector, V-Sync toggle, window-mode toggle/selector, and
  quality slider.
- **Audio:** master volume, mute/restore, music, ambient, and
  interaction/combat feedback levels. Keep these settings; only the optional
  checkbox above changes their current 25%-step control to sliders.
- **Keyboard & Mouse / Controller** (currently Controls): existing allowlisted
  per-action binding choices and restore-defaults behavior for each device
  family. The optional direct-capture checkbox changes only the keyboard/mouse
  interaction model if selected.
- **Accessibility** (currently Settings): text scale, interface scale,
  Standard/High contrast, colour-independent feedback preference, and Reduced
  motion. Interface scale is the existing equivalent of the reference's Scale
  GUI control; keep it in this group with a visible value and do not add a
  duplicate setting.

Keep the selected tab, current values, and focus order readable. Sliders expose
their values as text; toggles expose on/off text; selectors expose the current
choice and can be changed with visible previous/next buttons plus keyboard or
controller navigation. Long Graphics and binding pages remain scrollable.
High contrast, local persistence, modal ownership, and reduced-motion behavior
continue to apply. Candidate options stay out of implementation scope until the
user checks them and their underlying behavior is confirmed.

Exact control ranges and device-specific labels remain implementation details;
they must stay bounded, reversible, and compatible with the current input
bindings. This increment makes no platform, visual-identity, or audio-content
decision.

The Audio tab provides a local master-volume cycle in 25% steps and a
mute/restore button. Ambient, music, and interaction/combat feedback each have
a focusable category button that cycles through 0%, 25%, 50%, 75%, and 100%.
Every button reports its current level or action in text. These values save in
the local `GameUserSettings` config. The master value applies through the
engine's primary output-volume multiplier; the ambient value scales the local
wind, rain, water, fire, and biome loops, and the interaction/combat value
scales owner-local movement, status, crafting/gathering, discovery, combat, and
support one-shots. The music value is ready for a future music playback path;
there is no music track in the current runtime.

The Controls tab stores only allowlisted local choices in the existing
`GameUserSettings` configuration. Movement axes retain positive/negative pairs
when a keyboard layout is changed; action and look bindings replace only the
selected keyboard or controller device family. Applying or restoring a choice
rebuilds the local player's input map immediately, and a fresh controller
instance rehydrates the same local choices. The project baseline in
`Config/DefaultInput.ini` is never rewritten. Escape remains available for the
modal close path even when the alternate Settings action key is changed.

The Settings tab now provides local text-scale choices of 100%, 125%, and 150%
and Standard or High contrast. Text choices immediately rebuild the modal with
scaled, auto-wrapped labels and buttons inside its larger bounded panel;
Controls remains scrollable at the largest size. Contrast updates the local
backdrop, panel, button surfaces, text, and focusable state controls together,
while every state continues to expose an explicit text value. Both choices
persist in the existing local `GameUserSettings` configuration and affect no
gameplay widget, replicated property, or server request.

The Settings tab also provides a whole-interface scale from 80%, 90%, 100%,
110%, or 120%, defaulting to 100%, plus a Reduced motion On/Off choice that
defaults to Off. Interface scale is a local multiplier over the project's
existing DPI curve and `UUserInterfaceSettings::ApplicationScale`; it changes
the game-layer DPI scale so all game UI reflows together and hit testing follows
the displayed geometry. It remains separate from text scale and is stored only
in the existing local `GameUserSettings` configuration. Reduced motion
overrides theme animation defaults for the options-panel slide, button/card
highlight transitions, and animated scrolling. Static focus, selection,
contrast, labels, status changes, and menu actions remain immediate and
visible. Theme animation keys continue to control those effects when Reduced
motion is Off. Neither option creates a gameplay setting, RPC, replicated
value, world/player save field, or new save schema.

The Settings tab also provides a local colour-independent feedback choice:
**Text only** keeps the existing readable state lines, while **Text + markers**
adds explicit bracketed state markers for Wet, hearth, construction, combat,
discovery, and support status in a local owner-only overlay. The marker mode
uses text rather than colour as the distinction, follows the local contrast
palette, follows the scaled viewport, and updates as the existing accepted
gameplay state changes. It is stored beside the other local settings and never
becomes a gameplay signal.

## Existing input baseline

The current normal-player baseline in `Config/DefaultInput.ini` is the source
for the Controls tab's initial labels. It must remain available while local
remapping is added:

| Action or axis | Keyboard/mouse baseline | Controller baseline |
| --- | --- | --- |
| MoveForward | W/S and Up/Down | Left stick Y |
| MoveRight | A/D and Left/Right | Left stick X |
| Turn / LookUp | Mouse X / Mouse Y | Right stick X / Right stick Y |
| MinimapZoom | Mouse wheel | — |
| Interact | E | Face button bottom |
| Attack | Left mouse button | Right shoulder |
| Jump | Space | Face button left |
| Sprint | Left Shift / Right Shift | Left stick click |
| SettingsMenu | Escape / O | — |
| WorldMap / WorldMapRecenter | M / R | — |
| InventoryMenu | Tab / I | — |
| Build and crafting menu (`CraftMenu`) | B | Special left |
| Numbered inventory hotbar | 1–9, 0 | Inventory grid via D-pad and A |
| Support selection | F1–F4 | D-pad directions |
| Support activation | Q | Face button top |

The baseline check proves that these existing names and inputs are present; it
does not make them remappable. A runtime remapping presenter must display the
actual current binding, retain a keyboard/controller path to cancel or reset,
and send the same existing intent rather than a new gameplay payload.

The owner-local Inventory menu opens only when no other modal is ignoring
movement or look input. This keeps it exclusive with crafting, Settings, and
the world map, whose existing modal paths retain their opening/closing rules.
Its Escape path uses the existing SettingsMenu action to dismiss Inventory
before Settings can open. Opening captures the current cursor visibility and
only the movement/look ignore state it acquires; closing restores those values
and returns input to gameplay. While an editable text control has keyboard
focus, Tab/I do not toggle the menu. Gamepad B closes it when focus is outside
text entry. The menu reads only the owning pawn's owner-only replicated pack
stacks and bounded carried-tool records. Previous/next controls and arrow/D-pad
input select a pack or equipment row, whose icon, description or level/condition
appears in the shared detail panel. If a selected record disappears, selection
falls back to the first remaining row; an empty owner view clears selection and
hides the detail panel. A damaged selected tool exposes the existing repair
action. It sends only the tool ID, and the server validates the carried record
and nearby visible Workbench or Forge before changing condition. A selected
supported food item shows its existing Steady Meal benefit, owner-visible
quantity and active timer, and an Eat one serving action. That action sends only
the selected food ID through the existing station-free server transaction;
rejected or replayed requests consume nothing and cannot refresh or replace an
active meal. The owner sees the server result. The previous HUD pack view
remains until the separate HUD-removal task.

Inventory also has local search, category, and sort controls. Search trims outer
whitespace, is limited to 64 characters, and matches only names already present
in the owner's visible pack/tool rows; it cannot query descriptions, quantities,
or hidden catalogue content. Category filters All, Items, or Carried tools, and
sort chooses owner order, name, or category/name without changing pack order.
Page Up/left shoulder cycles category, Page Down/right shoulder cycles sort,
arrows/D-pad change the selected row, and Tab reaches search and labelled
buttons. While text editing has focus, its cursor keys remain available. Query,
filter, sort, canonical selection, and row/menu scroll offsets survive closing
and reopening this local widget for the current session. An entry hidden by a
filter or query keeps its remembered selection; an entry removed from the owner
snapshot falls back to the first remaining result, while no results clears
details and explains recovery. Separate menu and row scroll areas keep the
browsing controls reachable when enlarged text or viewport changes need scroll
fallback. The panel follows a resized viewport up to its normal 640×560 UI-unit
size and leaves a 16-unit inset at each edge when the available area is smaller.

## Accessibility requirements

- Every setting is reachable with keyboard focus and a controller, with visible
  focus, stable tab/order navigation, and an Escape path back to the prior menu.
- Text scale changes must keep labels, values, help text, and action buttons
  readable without clipping the existing modal layout. The option itself must
  not require reading a colour or hearing a cue.
- Whole-interface scale is independently adjustable from text scale, persists
  locally, reflows the HUD and menus, and keeps their controls inside the
  supported viewport. Reduced motion bypasses decorative movement while
  keeping essential state feedback, focus, and actions immediate.
- Contrast changes must affect local UI surfaces, text, focus, and state
  indicators together. State must still be distinguished by text, shape,
  pattern, or icon when colour is unavailable.
- Audio controls must have text equivalents for their current value and a
  mute/restore path. Silence must not remove the only indication of damage,
  Wet, hearth, construction, discovery, or support state.
- The master-volume and mute/restore controls remain focusable buttons, expose
  their current value and action in text, and retain the menu's Escape return.
- Cancel, apply, and reset actions must communicate whether a local change was
  retained. Invalid or out-of-range input falls back to the last valid local
  value and never reaches gameplay code.

## Local storage and authority boundary

Settings are local presentation preferences. They may use the existing local
user/configuration path alongside `GameUserSettings`, but must not be written
to world saves, player progression saves, sparse world deltas, or replicated
properties. Changing a setting sends no RPC and cannot alter world identity,
terrain, population, interaction validation, weather, exposure, inventory,
construction, combat, discoveries, learned effects, rewards, or persistence.

The server continues to own gameplay outcomes. A client may use a local
control binding to express the same existing movement or narrow action intent,
but it cannot use settings to select a target, damage, quantity, transform,
reward, status, weather value, or effect execution. Local UI must render the
authoritative replicated result where one exists and retain a text/shape
equivalent for colour-independent use.

The master audio preference is applied independently by each running game
process. In a listen-server session it affects that process's local output; it
does not change audio or state on a connected remote client.
The category values are local configuration too. Ambient and one-shot
presenters read them from the current process and apply them only to its local
player components or cue submissions; a connected peer has separate settings.
The colour-independent feedback mode is likewise local to each player process.
Its overlay reads only that owner's existing pawn components and replicated
results; it creates no request, target, reward, or hidden-content indication.

## Acceptance and limits

The runtime implementation is accepted when a fresh local profile can open,
navigate, change, cancel, apply, reset, and persist each option group with
keyboard and controller bindings, while a second player observes no
replicated settings state. `Scripts/Verify-SettingsAccessibility.ps1` runs
isolated host/client profiles, opens the Escape shell and live Video, Audio,
Controls, and Settings tabs; it checks focusable targets, modal input
ownership, and Escape recovery, persists every local option, and compares the
pawn health, transform, and server-selected world identity before and after
the probe. It checks interface scale and reduced motion in local config, then
restarts both peers with the same user directories and confirms both choices
reload and the scale is applied over the project default. It retains fourteen
host/client PNG captures at either 1280x720 or 1024x768: standard-contrast
Escape, Video, Settings, reduced-motion Settings, and restored HUD views plus
high-contrast Controls and Audio views. Run both viewport sizes and inspect the
modal, HUD, and artwork; these checks do not replace physical
keyboard/controller or packaged verification.

`Scripts/Verify-SettingsAccessibilityContract.ps1` checks this contract
without Unreal. The focused `Kalmala.UI.Settings.LocalPresentation` automation
and the rendered peer probe cover local round-trips, bounded input mappings,
focusable controls, and the authority boundary. They do not simulate physical
controller hardware, establish audible quality, prove packaged persistence,
or replace the final full settings acceptance.
The focused `Kalmala.UI.Settings.LocalPresentation` automation checks
master-volume bounds, local config round-trip, immediate mute and restore,
category-level bounds and config round-trips, bounded control labels, local
control persistence and restore defaults, and text-scale/contrast bounds and
round-trips plus the colour-independent feedback mode bounds and local
round-trip. It also checks the 80–120% interface-scale bounds, application over
the project scale, independent text-scale state, and local reduced-motion
persistence. The rendered peer probe covers the live tabs, viewport reflow,
focus, contrast, both animation paths, local restart persistence, and gameplay
state boundary at 1280x720 and 1024x768; physical controller hardware, audible
quality, and packaged persistence remain outside this verification.

## M11 Settings label polish

The Settings tab uses two-line option controls: the option name above its current
value, with the shared theme body size plus three. This gives the 150% text-scale
view an explicit desired height without stale centered-text auto-wrap width and
removes repeated activation hints from individual values. Existing button focus,
activation, immediate local application, contrast treatment, and persistence stay
unchanged. Other option tabs remain part of the broader view polish pass.

## Development increment: Options tab memory

The owner-local settings widget retains its last Options tab (Video by default)
across ordinary close/reopen. Escape still opens the main shell; choosing Options
rebuilds the remembered tab with current labels and its normal focus target.
Restoration invokes no setting-changing click handler. The current M11 runtime
index is bounded to four tabs, with Video as the dispatch fallback. M13 replaces
that tab set with the applicable groups from the six-group Settings checklist
above; retained selection must resolve to a valid group, with Graphics as the
fallback for a stale Video index. Memory ends when the widget is recreated,
including subsystem teardown; it writes no config/save data.
Scroll position and exact previously focused control are not yet retained.
The rendered settings accessibility fixture now returns from the Settings tab
to the main shell, reopens Options, and requires the Settings tab and a focusable
control to be restored on both peers while local interface/text scales and
master volume remain unchanged. This covers the Options-tab-only increment;
other menus, scrolling, exact control focus, and full per-menu restoration remain
part of the broader M11 task.


## Presentation ownership


This document records the remaining presentation seams and the source that is
allowed to supply them. It keeps the M5 visual/audio pass original and
project-owned without changing gameplay contracts, the generated-world
identity, or the multiplayer authority model.

Every audited seam is presentation-only: it may improve readability or
feedback, but it cannot become a gameplay source.

M11 supersedes the historical status placement below: active status/weather
now appears only in the transparent owner-local top-right hotbar, with live
details in Escape > Status and weather details. The lower-left widget retains
only ocean travel, and pack preparation guidance retains no active timer.
Canonical catalogue images are shared by Inventory items, carried tools,
selected-item details, ingredient rows, and the chest selectors; the existing
vector assignment remains the missing-texture fallback. Build and crafting
result grids remain on vector assignments until the next M12 icon integration
child. See `02-technical-architecture.md` for identities and verification boundaries.

## Ownership ledger

| Presentation seam | Project-owned source | Current contract | Runtime status |
| --- | --- | --- | --- |
| Player | `UKalmalaPlayerModelComponent` procedural mesh and the generated bark/terrain/rock materials | Nine local, collision-free cosmetic parts; shape and pose never author gameplay | Faceted mantle/hood presentation verified in the rendered offscreen host/client controls fixture |
| Wildlife | `AKalmalaWildlifeSpawn::BuildArchetypePresentation` procedural low-poly geometry and vertex colours | Server-owned replicated actor state; mesh is presentation only and has no collision | Mireling's low forward hunch, reaching arms, and split crown read as a distinct close-view silhouette in the rendered host fixture; dark body planes merge somewhat. Boar has a low wedge-backed profile, broken bristle ridge, tapered muzzle, and paired tusks; deer has a lighter, long-legged alert profile with paired forked antlers |
| Environment | `AKalmalaGeneratedTerrainPatch`, campfire, and construction procedural meshes using generated materials | Terrain collision and shelter collision remain the gameplay authority; decorative meshes do not add routes or hidden content | Existing generated terrain, water, rock, tree, hearth, and kit sources are audited here |
| UI | `KalmalaUI` C++ widgets, local Slate vector glyphs, and disposable local raster textures | Local presentation reads visible/replicated state and never creates a gameplay source | The near-crosshair prompt names only the actor hit by the owning pawn's current short view trace and shows its supported action plus any owner-visible unavailable reason; it clears for no target or modal input and contains no key/button legend. The lower-left survival strip reads the owning pawn's replicated Wet/food entries, exposure, active support, and weather, with text-plus-shape categories, server-time/intensity context, source, and recovery guidance; the Inventory menu reads only that owner's pack/tools and the crafting panel reads its owner-only skill ledger; a separate top-centre support strip reflects the local character's selected effect and the owner's learned set; minimap/map remain local; the separate weather badge reads the replicated server tier and pairs circle/diamond/triangle markers with explicit text; settings Audio/Controls/Settings tabs expose local options, including independent text/interface scales and reduced motion in the existing local settings config |
| Feedback | Text and shape/icon treatments in the owner notification queue, crafting, discovery, and settings widgets | Readable without colour or audio; feedback reports accepted replicated results rather than client claims | Support glyphs reflect only the owner's learned/selected state and retain explicit text names/status; a bounded owner-only queue presents combat/support results and discovery acknowledgements; the optional Text + markers overlay remains for nearby hearth/construction context |

The ledger is an ownership and scope check, not a claim that the complete M5
art or audio pass has shipped. The player presentation passed the rendered
offscreen host/client controls fixture. The former panel glyphs have moved to a
separate owner-local strip; its learned-state, selection, scaling and rendered
host/client behavior remain for M12 final verification. The weather activity badge's tier
mapping and viewport slot passed focused UI automation; rendered viewport
readability remains unreviewed. The survival strip's data mapping and lower-left
viewport slot pass focused UI automation, but rendered host/client layout,
multi-row clipping, and scaled-font legibility remain unreviewed.

## Owner-local station context shell — 2026-10-08

The shared themed station shell opens from an owner-only event emitted after
the server accepts and revalidates a construction interaction. It binds the
current section to the exact replicated station actor and stable construction
ID. The Cooking Rack embeds its station-filtered Cook view, limited to cooked
boar/deer meat; the Cauldron uses the same owner-local shell for stew and soup;
the Frying Pan shows only roasted root vegetables and deer/rutabaga roast.
All cooking views include ingredient, quantity, and current hearth-heat feedback.
Workbench opens a Craft section scoped to its supported recipes, matching Tool Rack production,
and the owner-local Bronze Axe operation; its effective level and Tool Rack
state come from the accepted station and replicated placement presentation.
Forge exposes separate Craft, Upgrade, and Repair sections. Upgrade compares the
owner-only carried Bronze Axe against its authored Iron Axe target and reads
the owner's tool, material, and skill snapshots for prerequisites; the accepted
Forge's effective level and Anvil state stay tied to the exact context actor.
The existing progression RPC remains responsible for authoritative checks,
material exchange, and persistence.
These passive Workbench Tool Rack and Forge Anvil status lines remain visible
in their parent Craft sections. The standalone Build menu no longer duplicates
inventory, food, repair, upgrade, or storage controls; it keeps placement and
the Light hearth action, while raw-fuel addition uses Campfire Interact.
The owner can switch to a separate Repair section without changing the recipe
selection or Forge Upgrade presentation. Repair rows read only the owning pawn's
owner-only carried-tool array, show its real level and condition, and submit
only the selected tool ID to the existing repair RPC. The response remains
owner-only. The legacy CraftMenu path remains available.
An accepted Chest interaction opens a Store section with separate selectors
backed by the owning pawn's private pack and its owner-only current chest view.
Each list shows the item's current count; the summary reports both 16-stack
limits and disables a transfer when its current stack or destination slots are
full. Store/Take submit only the selected item ID through the existing
one-item server transactions, which revalidate the active chest and persist
before changing the pack. Closing the shell, losing range, changing pawn, or
destroying the chest expires the view and shell.
While open, the local subsystem asks the owning crafting component to verify
the same actor reference, stable ID, station kit, pawn world, and range. The
server validated sight during the original interaction. It closes on target
destruction, loss of range, or pawn replacement, then restores the prior
movement/look-ignore and cursor states.
The shell is presentation only: recipe and inventory changes remain on the
existing server paths, which revalidate their own station, heat, costs, and
owner inventory. No client target request, RPC, replication authority change,
or persistence field is added.

## Owner-local prepared-food inventory detail

When the owning player's private pack contains roasted field meat, Hearth
Broth, or smoked field meat, the on-demand Inventory menu explains that one
serving grants Steady Meal, reducing stamina cost by 10% for 120 seconds. If
the same pawn's existing replicated status contains an active meal, the menu
shows its remaining server-published time and the wait-for-expiry rule. The
Eat action sends only the selected supported food ID through the existing
server transaction; the server revalidates quantity and the active meal slot.
The menu adds no new RPC, replicated field, or save data. Shape markers and
explicit text remain visible with the owner's configured text scale and
contrast. `Kalmala.UI.Inventory.PreparedFoodDetails` checks the selected menu
detail and disabled action while owner data is unavailable. The host/client
inventory reconnect fixture continues to verify owner-only pack visibility;
rendered multi-row layout and scaled-font appearance remain open.

## Owner-local crafting skill and unlock detail

The Camp crafting panel reads detailed progression only from the local owning
pawn's `UKalmalaSkillProgressionComponent`. It lists the six allowlisted skills
with the current level and experience toward the next level, then names the
nearest skill-gated recipe unlock and the XP progress required. When an
accepted recipe can advance that skill, the panel names its per-request XP
award. Missing or incomplete owner replication is described as unavailable;
no aggregate peer badge is used as a substitute for private skill detail.
The selected-recipe details and skill block follow the local text-scale and
high-contrast settings inside the existing scroll view. This is read-only
presentation: it adds no RPC, focus target, progression mutation, or save
field. `Scripts/Verify-Crafting.ps1` checks the fresh-owner values and
host/client presentation; `-Rendered` retains both 1280x720 captures.

The selected-result preview reads only the existing selected recipe output,
description, requirements, and availability already shown by the local
crafting panel. It resolves a canonical icon through the shared catalogue
mapping and keeps the availability text visible when a recipe cannot currently
be crafted. Keyboard/controller selection updates this read-only panel; it
sends no request and creates no actor. No inventory, recipe, item, replicated,
or save property is added.

The inline Iron Axe upgrade comparison reads the owning player's carried
Bronze Axe level and condition plus the existing progression/lifecycle
definitions for the selected Iron Axe. It refreshes from local owner state and
reports missing or incompatible comparisons as unavailable. It adds no
request, equipment state, stat authority, replication field, or save data.

## Allowed and forbidden sources

Allowed visual sources are original project code, the committed Kalmala
materials under `Content/Kalmala/World/Materials`, project-owned map content,
and transient textures derived from local generated-world presentation. A
procedural mesh is acceptable when its geometry and colours are authored in
Kalmala code and it remains inside the documented authority boundary.

Do not introduce engine basic-shape meshes, Starter Content, Marketplace or
third-party assets, copied visual identity, or a new asset service. Developer
biome-debug materials may diagnose generation but must not become the normal
presentation path. Audio delivery includes local project-owned wind,
nearby-visible-water, nearby-visible-lit-hearth, sampled-biome, rain, and Wet
status cues, plus generated-ocean entry/exit cues from the owning pawn's local
movement-mode transitions. These traversal cues do not set or replicate swim
state. A short support-acceptance cue reads only the owning pawn's
existing owner-only accepted feedback serial. The biome layer reads only the
owning pawn's current generated-world location and does not scan or suggest a
route. All gameplay state must retain readable non-audio feedback; local
interaction/gathering acceptance and rejection cues now read owner-only crafting
results and accepted inventory increases. Discovery, movement, combat, and
support cues read their existing owner-local or replicated presentation state;
the remaining visual and accessibility work stays in M5.

## Static audit and runtime limits

`Scripts/Verify-PresentationOwnership.ps1` checks the committed project-owned
material files, the source anchors for player, wildlife, environment, UI, and
feedback presentation, and the absence of known external/prototype asset
paths. It also checks this ledger's scope and authority statements without
launching Unreal.

The audit cannot prove material loading, triangle winding, visual readability,
animation, audio mixing, packaged startup, or host/client screenshots. Those
remain runtime verification work after Unreal build access is restored.

## Multiplayer and persistence boundary

Presentation has no authority to select a world, spawn content, choose a
target, apply damage, grant a reward, change weather/exposure, mutate an
inventory or construction record, learn an effect, or write a gameplay save.
Server-owned replicated state remains the only gameplay result rendered to a
peer. Local cosmetic geometry, UI preferences, and transient raster textures
are not replicated and do not change saved-data schemas.

The expanded map's `WorldMapPanelImage` is static project artwork painted
behind its existing locally generated terrain and fog. Its owner-local legend
and personal-pin/co-op-player/temporary-ping filters read only pins already
loaded for that local player and co-op markers already returned through the
owner's visibility-gated map-awareness component. A separate local fog check
still gates every co-op marker. Legend counts describe eligible markers in the
current map view. Filters suppress painting only; they do not request hidden
content, change exploration, alter co-op consent, or mutate/remove pin data.
Their transient widget state has no authority, RPC, replication, or persistence
path.


The tutorial presenter and persistent inventory HUD are retired. Current active-only status, notification, recipe activity, and on-demand Inventory contracts are defined in the preceding consolidated sections. Historical rendered captures do not establish fresh M13/M14 acceptance.


## Audio cues


This document defines the original audio layer for the existing vertical slice.
It is a cue contract, not an imported sound library or a new gameplay system.
All future sound assets must be project-owned and every cue must have a readable
non-audio equivalent so silence never hides authoritative state.

## Cue matrix

| Cue group | Normal trigger | Audio intent | Required non-audio equivalent | Authority/privacy boundary |
| --- | --- | --- | --- | --- |
| Ambient wilderness | Local player is in normal play | Quiet wind, water, fire, and biome atmosphere establish place without a fixed route | Terrain, weather, hearth, and exposure text/shape cues remain readable | Only local visible context; no hidden population, discovery, or route hint |
| Movement and traversal | Local movement, jump, sprint, landing, or water entry is already visible | Sparse original footfall, jump, landing, and movement-state cues reinforce action | Existing movement pose and HUD state remain readable; current bindings remain in Options > Controls | Local cosmetic response; no movement result, speed, stamina, or terrain authority is authored by audio |
| Weather and exposure | Existing replicated weather/Wet presentation changes | Rain, wind, and shelter contrast communicates changing conditions | Wet duration, shelter, warmth, hearth, and recovery text/shape cues remain visible | Server owns weather/exposure/Wet; audio reads accepted replicated state only |
| Interaction and gathering | A normal interaction receives an accepted or rejected result | Short, distinct confirmation or rejection cue avoids ambiguous input | Existing interaction text and inventory result identify accepted, rejected, or unavailable state | Server validates actor, range, payment, item, quantity, and outcome; audio carries no request payload |
| Combat | Replicated committed action or feedback changes for the owning player/relevant actor | Windup, recovery, hit, defeat, and unavailable cues clarify timing and outcome | Combat phase and colour-independent HIT/DEFEAT/UNAVAILABLE text remain authoritative presentation | Server selects target, damage, cooldown, and defeat; clients never infer or author them from sound |
| Discovery | Owner receives a server-confirmed landmark or scroll feedback result | Brief discovery motif acknowledges only an accepted discovery | Owner-only text names the accepted discovery category; remote players retain relevant visible actor presentation | No sound or prompt reveals undiscovered IDs, private rewards, or duplicate state |
| Support magic | Entitled player receives accepted activation, rejection, or expiry presentation | Gentle effect-specific cue communicates support activation and end state | Existing learned-effect, target-validity, cooldown, stamina, and active-state text remains available | Server validates entitlement, effect, target, sequence, stamina, duration, and non-damaging execution |
| Camp and storage | Hearth, construction, or storage result is already visible | Fire, placement, payment, and storage cues reinforce a chosen camp action | Hearth, construction health/rain wear, inventory, and storage text remain readable | Server owns placement, payment, fuel, health, storage, and persistence; audio is presentation only |

The matrix is intentionally event-driven. No cue may start because a hidden
actor, hidden reward, private pin, or future route exists outside the player's
normal visible/relevant context. Repeated state changes should be coalesced or
rate-limited locally so audio cannot become a timing advantage or a source of
network traffic.

## Original asset and mix requirements

Sound assets must be created for Kalmala and stored under a project-owned audio
content path when the runtime pass begins. Do not copy music, field recordings,
voice, sound effects, or named audio identity from a third party. Use the local
Audio settings contract for master/music/ambient/interaction-combat levels and
mute/restore. A muted or unavailable device must still expose all state through
the text/shape equivalents above.

Every local ambient component multiplies its live bed level by the saved
Ambient category value. Owner-local movement, Wet, interaction/gathering,
discovery, combat, and support one-shots multiply their submitted cue level by
the saved Interaction/Combat Feedback value. Master volume continues to apply
to the whole game process. Music volume is saved and shown in the Audio tab,
but no music playback path or track currently exists.

The runtime ambient layer includes loop-seamed, project-generated beds at
`/Game/Kalmala/Audio/WindBed`, `/Game/Kalmala/Audio/WaterBed`,
`/Game/Kalmala/Audio/FireBed`, `/Game/Kalmala/Audio/BiomeBed`, and
`/Game/Kalmala/Audio/RainBed`, plus the one-shot
`/Game/Kalmala/Audio/WetStatusCue` and
`/Game/Kalmala/Audio/SupportAcceptedCue`,
`/Game/Kalmala/Audio/SupportMendingCue`,
`/Game/Kalmala/Audio/SupportHearthShieldCue`,
`/Game/Kalmala/Audio/SupportBearsVigorCue`,
`/Game/Kalmala/Audio/SupportDeerCallCue`,
`/Game/Kalmala/Audio/SupportHearthShieldExpiryCue`, and
`/Game/Kalmala/Audio/SupportBearsVigorExpiryCue` and
`/Game/Kalmala/Audio/CombatResultCue`,
`/Game/Kalmala/Audio/InteractionAcceptedCue`, and
`/Game/Kalmala/Audio/InteractionRejectedCue`,
`/Game/Kalmala/Audio/MovementFootfallCue`,
`/Game/Kalmala/Audio/MovementJumpCue`, and
`/Game/Kalmala/Audio/MovementLandingCue`,
`/Game/Kalmala/Audio/GeneratedOceanEntryCue`, and
`/Game/Kalmala/Audio/GeneratedOceanExitCue`.
`UKalmalaAmbientAudioSubsystem` starts wind only
for a local player whose normal generated-world state and pawn are ready. It
smoothly adjusts the wind bed from accepted replicated
`FKalmalaWeatherState::WindStrength`. Rain starts only when accepted replicated
precipitation reaches `0.01`, follows its intensity with a quiet local fade,
and stops when rain ends. WetStatusCue plays only when the owning pawn's
replicated server-owned `State.Wet` entry first appears (or is already present
when local play starts); it does not infer the cause or change the status.
Neither cue reads another pawn's private status or sends a request.
An effect-specific support activation cue plays once when the owning pawn's
existing owner-only support `FeedbackSerial` advances with
`EKalmalaSupportFeedback::Accepted`; the component's existing active effect
selects Mending, Hearth Shield, Bear's Vigor, or Deer Call. The cue does not
infer an effect from client input. A generic `SupportAcceptedCue` remains a
fallback if a specific cue cannot load. Existing learned-effect, cooldown,
stamina, and active-state text remains the readable result; unavailable
feedback does not play an acceptance cue.
Hearth Shield and Bear's Vigor each play a quieter effect-specific expiry cue
only when the owning pawn's existing replicated absorption/strength and expiry
state transitions to inactive. A shield depleted by damage uses the same end
cue. Mending and Deer Call are immediate effects without a timed gameplay
state, so they use activation cues only. No local timer guesses when an effect
ends.
CombatResultCue plays once when the owning pawn's existing owner-only combat
`FeedbackSerial` advances with `EKalmalaCombatFeedback::Hit` or `Defeat`.
Unavailable results remain readable through the existing text and do not use
the confirmation cue. Playback carries no target identity and never infers a
hit, defeat, or damage from a client request or remote actor.
MovementFootfallCue is submitted at a quiet, distance-based stride while the
owning local pawn is grounded and moving; cadence and pitch follow sampled
local speed, including actual sprint pace. MovementJumpCue plays only when that
pawn transitions from ground movement into an upward falling state, and
MovementLandingCue plays when its sampled state returns from falling to ground.
The subsystem samples only the local controller's own character and its local
movement component; it does not inspect remote pawns, claim server movement
acceptance, or trigger an RPC. Existing movement pose and HUD state remain the
readable movement cues; current input bindings remain available in Options >
Controls.
GeneratedOceanEntryCue and GeneratedOceanExitCue each play once when that same
local movement component's `IsSwimmingInGeneratedOcean()` state transitions
into or out of its generated-ocean custom movement mode. Initial state sampling
and pawn replacement establish a new baseline without a false entry/exit cue.
Water, Wet duration, exposure, and
server-authoritative movement remain readable and unchanged.
InteractionAcceptedCue plays for a new accepted owner-only crafting result or
when an existing owner-only inventory stack increases after server validation;
the first inventory snapshot only establishes a local baseline. A rejected
owner-only crafting result uses the distinct, quieter InteractionRejectedCue.
Inventory decreases do not cue. Crafting result text, an accepted-result bit,
and a monotonic serial replicate to that pawn's owner only; the server writes
all three, and a small local cooldown coalesces adjacent interaction/gathering
changes. Neither cue includes an item ID, quantity, target, or request data.
Only owner-only `LandmarkFound` or `ScrollFound` feedback plays the original
`DiscoveryAcknowledgedCue`, once per new server-confirmed feedback serial.
Duplicate or unavailable results remain readable text and do not cue; the audio
carries no discovery ID, reward, or hidden-content hint.
It samples a bounded set of points around that pawn from the existing immutable
world identity, accepts only the existing sea surface or visible inland-lake
surface, and requires an unobstructed visibility trace from the local view
before water can be heard. The water loop probes every 0.75 seconds, fades by
distance within 1,600 cm, and uses a maximum volume of 0.07. Fire probes locally
replicated campfire actors every 0.75 seconds and accepts only a lit hearth
within 1,400 cm with an unobstructed trace from the owning player's view. Its
non-spatial bed fades with distance, reaches full volume by 275 cm, and caps at
0.075. Biome ambience samples only the owning pawn's current XY from the
locally available immutable seed and existing four-field classifier every
0.75 seconds. One quiet bed uses a small biome-specific pitch/level profile
with smoothed transitions; it does not scan ahead, reveal landmarks, or imply a
route. All five local loops stop when local play ends; no hidden population,
discovery, or remote-biome query is used.

`Scripts/Generate-WildernessWind.ps1`, `Scripts/Generate-WaterAmbience.ps1`,
`Scripts/Generate-FireAmbience.ps1`, `Scripts/Generate-BiomeAmbience.ps1`,
`Scripts/Generate-WeatherExposureAudio.ps1`,
`Scripts/Generate-SupportFeedbackAudio.ps1`,
`Scripts/Generate-CombatResultAudio.ps1`,
`Scripts/Generate-InteractionFeedbackAudio.ps1`,
`Scripts/Generate-DiscoveryAcknowledgementAudio.ps1`,
`Scripts/Generate-SupportEffectAudio.ps1`, and
`Scripts/Generate-MovementTraversalAudio.ps1` and
`Scripts/Generate-WaterTraversalAudio.ps1` recreate the original mono sources
under `Content/Kalmala/Audio/Source`; the rain bed is seam-crossfaded
and the WetStatusCue, SupportAcceptedCue, six effect-specific support cues,
CombatResultCue,
DiscoveryAcknowledgedCue, interaction cues, MovementFootfallCue,
MovementJumpCue, MovementLandingCue, GeneratedOceanEntryCue, and
GeneratedOceanExitCue are short one-shots. Import all twenty-two assets to
`/Game/Kalmala/Audio` with Unreal's `ImportAssets` commandlet, then run
`Scripts/Verify-AmbientAudio.ps1`, `Scripts/Verify-AmbientAudioPeers.ps1`,
`Scripts/Verify-CombatPeer.ps1`, `Scripts/Verify-DiscoveryPeer.ps1`,
`Scripts/Verify-PlayerControls.ps1 -MovementAudio`,
`Scripts/Verify-PlayerControls.ps1 -OceanTraversalAudio`, and the forced editor build.
Pass
`-WeatherExposureOnly` to the peer runner when
focusing on the weather and Wet cues; its default mode also requires visible
water activation. The first verifier checks source format, imported
assets, local-player ownership, context gates, and teardown. The peer runner
confirms independent host/client probes for visible water and hearth context
plus each local player's current sampled biome, accepted rainy weather/Wet
state, and effect-specific Hearth Shield/Bear's Vigor activation and expiry
cues. `Verify-CombatPeer.ps1` confirms that
owner-local Hit and Defeat feedback submit CombatResultCue, Unavailable remains
text-only, and the other peer receives no private combat result cue. The
discovery peer fixture confirms that only the entitled local owner submits the
acknowledgment for a new landmark/scroll result; duplicate, unavailable, and
remote undiscovered feedback remain text-only. The
ambient peer fixture also verifies owner-local accepted crafting, accepted
gathering, and rejected crafting cues for both host and client. Its gathering
node is transient and invokes the ordinary server harvest path; it does not
write a world save. The no-build audit checks that the crafting result text,
serial, and outcome and the inventory stacks remain owner-only. The
development-only listen-server fixture selects a light rainy/windy
interval below the hearth smoulder threshold, applies Wet on the server, and
creates a visible server-lit hearth. A test-only server exposure hook keeps
Wet observable while it checks the status cue alongside the hearth bed.
Existing Wet duration, shelter, warmth, hearth, recovery, and support result text remains the
readable fallback. These checks do not establish
audible quality, hardware mixing, or packaged playback, and do not complete
other event cues or audio options.

Keep cues short, bounded, and non-blocking. Ambient layers may be local and
spatial, but they must not stream a second world simulation or reveal a server
actor outside ordinary relevant replication. Combat, discovery, support, hearth,
inventory, and construction cues should follow the same replicated feedback
serials already used by their presentation rather than adding an RPC or save
field.

## No-build audit and runtime limits

`Scripts/Verify-AudioCueContract.ps1` checks all eight cue rows, the
non-audio/accessibility requirement, project-owned asset rule, local mix scope,
server authority/privacy boundary, the owner-only combat/support feedback
sources, owner-only discovery cue gating, generated-ocean movement transitions,
and the absence of a new RPC or save field. `Verify-PlayerControls.ps1
-OceanTraversalAudio` confirms both local peers submit one entry and one exit
cue through the local movement-state transition path. Its development-only
flag injects a three-second local sample sequence because the fixed seed-418
spawn has no deep water in the fixture's short search area; it does not change
the character movement mode or network state. Normal play reads the actual
`IsSwimmingInGeneratedOcean()` mode.
It does not create sound assets, prove mixing, test spatialization, or launch
Unreal. The ambient-audio, combat, and discovery peer checks do not prove audible
playback, device mixing, spatialization, or packaged playback. The broader
original presentation pass remains in M5.

## Multiplayer and persistence boundary

Audio is local presentation. The server remains authoritative for world
identity, terrain, population, weather, exposure, interactions, inventory,
construction, combat, discoveries, learned effects, rewards, and saves. A cue
cannot select a target, damage an actor, grant an item, reveal private state,
change a timer, or mutate persistence. Audio settings and cue history are local
preferences and must not be replicated or added to gameplay save schemas.


M13's reusable-scroll timing overrides earlier instant Deer Call and short support-duration descriptions above when integrated. Audio still reads accepted replicated results; this cleanup adds no cue or gameplay behavior.
