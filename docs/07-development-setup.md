# Development setup

## Baseline

- **Engine:** Installed Unreal Engine 5.8.2 build at `C:\Program Files\Epic Games\UE_5.8`.
- **Project:** `E:\dev\Kalmala\Kalmala.uproject`.
- **IDE:** Visual Studio 2026 with the Game development with C++ workload, MSVC tools, and a Windows SDK.

## First build

Open PowerShell and run:

```powershell
& 'C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe' /projectfiles 'E:\dev\Kalmala\Kalmala.uproject'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaEditor Win64 Development -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex
```

Open the project in the editor with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'E:\dev\Kalmala\Kalmala.uproject'
```

The first editor launch must create the prototype map at `/Game/Kalmala/Maps/Prototype/L_Prototype`. Do not set it as the default map until it exists.

The existing `L_Prototype` is now the configured editor startup and game default map. It contains the M1 fixtures and tagged sun/sky lighting, with no template floor or landscape underneath the generated terrain. `Scripts/Setup-PrototypeEnvironment.py` reproducibly adds that lighting through Unreal's Python commandlet and saves only this map. Run it while other editor/test processes are closed to avoid a map file lock. Press Play in `L_Prototype`; an already-open `/Engine/Maps/Templates/OpenWorld` has its own landscape that intersects the generated world and can show checkerboard patches in depressions. Restart the editor to use the configured startup map, or open `L_Prototype` explicitly before Play.

## Bounded generated-world profile

After an editor build, run `Scripts/Verify-WorldProfile.ps1`. It starts a revision-4 seed-418 listen server, then joins a conflicting-seed client and waits for the existing replicated immutable identity. The server logs initial generated-world setup time, process physical-memory snapshot, total and replicated actor counts, active terrain patches/population keys, and sparse population-save bytes serialized to memory. It does not mutate the save, change the 25-patch budget, adjust density, or accept client-selected world data. The runner succeeds only after the late-joining client reports `Seed=418 Revision=4` and the server reports two players plus successful save serialization.

## Water surface regression check

`Kalmala.World.Water.ClippedSurface` checks partial-cell coverage, winding, flat levels, shallow shore treatment, seeded closed basins, rejection of sea-connected/unbounded/unseeded basins, deterministic meshes, and nonempty matching patch-edge intersections. Lake-biome fields seed enclosed terrain basins rather than clipping floating sheets at humidity/temperature boundaries. Basins exceeding 8,192 wet lattice vertices are conservatively omitted. The minimap uses the same visible water decision. Terrain/collision, server wetland rules, generator revision, and saved-data schemas are unchanged. Run `Scripts/Verify-PlayerControls.ps1 -Rendered` for rendered host/client traversal and screenshots after building. Restart the editor to load the repaired native module.

## Ocean-depth regression

After an editor build, run headless automation with `-unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -ExecCmds="Automation RunTests Kalmala.World.Water+Kalmala.World.Ocean.IslandLocator+Kalmala.UI.Minimap.LocalPresentation" -TestExit="Automation Test Queue Empty"`, directing `-UserDir` and `-abslog` into a unique temporary directory. `Kalmala.World.Water.OceanDepth` verifies depth against both collision-triangle planes, negative coordinates, equivalent adjacent-patch origins, same-identity reproduction, different-seed variation, and actual clipped coastline vertices; the fixture must contain wet sea floor, dry land, and mixed coastal triangles. `Kalmala.World.Ocean.IslandLocator` verifies a revision-4 seed resolves a repeatable naturally emergent island using the same terrain triangles. These checks do not verify a full host/client ocean crossing or extended streaming.

## Generated-ocean swimming regression

After an editor build, run `Scripts/Verify-Swimming.ps1`. It starts a memory-only listen server with seed 418 and a conflicting-seed client with seed 999. Each owning pawn walks to the nearest deterministic sea-depth fixture and must enter the generated-ocean custom movement mode; the server log proves authoritative entry and the client log proves prediction from the server-replicated identity. Entry requires at least 100 cm depth and return-to-land uses a 75 cm hysteresis threshold. The test adds no water volume, RPC, client depth input, island, boat, or streaming change.

## Long-distance ocean travel regression

After an editor build, run `Scripts/Verify-OceanTravel.ps1`. It starts a revision-4 seed-418 listen server and a conflicting-seed client. Each locally controlled pawn derives the same existing nearest emergent-island target from the server-replicated immutable identity, crosses generated ocean through ordinary predicted Character Movement, and must log both ocean entry and island arrival. The server's bounded terrain-patch refresh follows authoritative pawn positions throughout; the test rejects failed identity replacement and does not add a boat, route, island actor, client target request, replication property, or save mutation.

## Item catalogue verification

For the M2 material contract, run `Kalmala.Gameplay.Inventory.Catalogue` with the headless automation flags above after building the editor. It loads the real project catalogue and checks required materials, exact and exceeded stack limits, empty/unknown IDs, zero/negative/extreme quantities, overflow-safe additions, full stacks, and malformed/duplicate configuration. This is a pure contract check; live inventory replication and harvest-grant verification follow when the inventory component exists.

## Player inventory verification

Run `Scripts/Verify-Inventory.ps1` after an editor build for the inventory component increment. A development-only `-KalmalaInventoryTest` fixture grants ten wood and consumes three on each server pawn, rejects unknown/overflow grants and invalid/insufficient consumption, and verifies removal of an exhausted stone stack. The runner requires two successful server results, seven wood on the remote owner, rejected client-local mutation calls, empty remote contents after owner replication, matching immutable world identity, and read-only local pack presentation on both peers. Separate temporary user directories keep the scenario out of project-generated data. This headless check verifies widget data binding, not rendered layout; rendered harvest feedback and reconnect persistence remain subsequent tasks.

The inventory runner also requires `Harvest inventory: Passed=1` for both server pawns. Its development-only fixture exercises twelve isolated initialized nodes covering Wood, Stone, and Fibre, uninitialized-node and distant rejection, full-stack rejection without depletion, successful retry after capacity is freed, duplicate rejection, and exactly one sparse-save callback per accepted grant. It restores the original seven-wood inventory before owner replication checks and destroys its temporary actors; it writes no world-save slot. Run `Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion` and `Kalmala.Gameplay.Inventory.Catalogue` with the headless flags above for the pure authority and malformed quantity gates. This does not yet exercise a client's actual harvest RPC, inventory reconnect restoration, or simultaneous competing player input.

## Dedicated-server build

The Epic Games Launcher engine distribution does not include dedicated-server support. The `KalmalaServer` target remains in the project, but building it requires a UE 5.8 source build or another UE 5.8 distribution with server support. Do not attempt the command below with the installed Launcher engine.

With a server-capable engine, build the target with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaServer Win64 Development -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex
```

## Generated-world traversal smoke test

For the developer-only two-player traversal verification, launch a listen server and a client with `-KalmalaTraversalTest`. Both peers derive the lake target from the server-selected world identity; locally controlled pawns move through normal Character Movement input, while the server remains authoritative. The test only disables pawn-to-pawn capsule blocking and only while the switch is present. A passing run logs that both server pawns reached the Shimmering Lakes target and that the client observed replicated movement.

## Generated harvest reconnect smoke test

Run the prototype map twice as a listen server with `-KalmalaReconnectVerification=Harvest` and then `-KalmalaReconnectVerification=Verify`. The first process activates a deterministic harvest node, uses the normal server interaction path to harvest it, saves its sparse delta, and exits. The restarted server uses the same immutable world identity and succeeds only when that node is suppressed. `WildlifeDefeat` and `WildlifeVerify` run the equivalent server-only path for one deterministic wildlife spawn. This developer-only switch is server-local and accepts no client-supplied node identifier or outcome.

## Exposure inspection

Launch a listen server with `-KalmalaExposureInspection` to log server-sampled terrain and field inputs plus the active replicated weather values and provisional exposure state. Once a player joins, the output includes continuous low-ground wetness, deterministic lake-adjacency shoreline wetness, ridge/slope wind exposure, Flora-derived natural cover, and server-traced roof/windbreak shelter inputs. Player-built collision geometry must carry `KalmalaShelterRoof` or `KalmalaShelterWindbreak`; authored volumes and client trace results are ignored. The weather cycle is selected and advanced only by the server. Every second, the server replicates actual wetness, warmth, and the resulting 68–100% Character Movement travel multiplier; shelter dries and restores warmth slowly when dry, while a nearby lit fire accelerates recovery.

## Exposure replication smoke test

Launch a headless listen server and then join one headless client with `-KalmalaExposureReplicationTest`. The server spawns a temporary lit campfire and initializes each normal player pawn to a wet, low-warmth state. Its server log records weather, sampled shelter, fire warmth, and the resulting exposure values; the client log records the replicated weather, campfire, and exposure values. The developer-only path has no client RPCs for weather, exposure, or campfire mutation and adds no persistent gameplay content.

## Two-player camp-choice scenario

After the editor build, run `Scripts/Verify-CampChoices.ps1` from PowerShell. It starts a hidden listen server (seed 418) and a conflicting-seed client (999) on port 17841, with logs and user data in a unique temporary directory. Both use `-KalmalaCampChoiceTest`; this developer-only scenario requires two normal possessed pawns including a remote player. It compares separated low/high-cover dry-land fixtures within the generated starting neighborhood, logs ground wetness, water distance, and harvest availability, and samples twelve seconds without fires followed by twenty seconds with normally interacted server-owned fires. Both players must dry, warm, and improve travel speed. The runner checks matching world identity, weather, and at least ten exact exposure snapshots per replicated player ID, then stops only its own processes. It fails on timeout or scenario assertions and retains logs for diagnosis.

The fixtures represent two optional camp choices, not runtime camp recommendations: no route, camp marker, authored shelter, or persistent content is added. This is automated systems verification in cold, dry starting weather, not a human usability test or validation of inventory, construction, rain preparation, or rendered feedback. Those systems retain their existing scope and limitations.

## Camp-condition inspection

Launch a listen server with `-KalmalaCampConditionInspection` and join a player. The server logs the freely chosen position's continuous natural cover, ground wetness, bounded nearest-water distance, and nearby deterministic harvest-node count. It only explains local tradeoffs; it neither creates nor marks a camp location, and clients cannot invoke or alter it.

## Biome-colour debug overlay

Launch the game with `-KalmalaBiomeDebug` to replace the generated terrain's normal material with a project-owned vertex-colour debug material. It uses the same continuous four-field biome classifier as terrain generation: Meadows are green, Shimmering Lakes cyan, Elderwood dark green, Mossy Mire olive, Freezing Tundra pale blue, Thunder Mountains grey, and Ocean blue. The material is developer-only, is built independently on each peer from the replicated world identity and patch descriptor, and changes no terrain, collision, gameplay state, or replication.

## Companion minimap presentation

Movement-triggered raster generation is asynchronous. Run `Kalmala.UI.Minimap.AsyncRefresh` after rebuilding to check nonblocking refresh timing, full-resolution worker equivalence, stationary reuse, movement coalescing, obsolete zoom/identity rejection, and reinitialization. `Kalmala.UI.Minimap.GenerationPerformance` retains the exact revision-3/4 fingerprints and reports total raster time separately from the game-thread refresh cost. Use `Scripts/Verify-Minimap.ps1 -Rendered -GeneratorRevision 4` for current-world HUD/identity checks and `Scripts/Verify-PlayerControls.ps1 -Rendered -GeneratorRevision 4` for host/client movement. Both scripts retain revision 1 as their default legacy fixture. Restart any existing editor after the native module build.

The local `UKalmalaMinimapSubsystem` creates a 208-pixel `UKalmalaMinimapWidget` for each local player once its controller is available. The widget is anchored to the top-right viewport corner with a 24-pixel margin. It refreshes local seed-derived terrain/water samples around that player's pawn and draws only samples inside a circular radius, together with a centred marker rotated to the pawn's facing yaw. `MouseWheelAxis` changes only the local session's sampled radius, clamped from 2,500 to 10,000 cm in 750 cm steps; CommonUI's normal-game-input gate leaves wheel input to any modal UI. It reads no world actors, population, landmarks, or gameplay state beyond the already-replicated world identity and owning pawn transform.

Run `Scripts/Verify-Minimap.ps1` after an editor build for the two-peer identity check. It starts a memory-only hidden listen server with seed 418 and a conflicting-seed client with seed 999, then confirms that the client receives the server world identity. The focused `Kalmala.UI.Minimap.LocalPresentation` automation checks the two player-centred local views from that same identity, circular clipping, min/max zoom, modal input gating, and the top-right footprint at 4:3/75%, 16:9/100%, and ultrawide/125% UI scales. These checks are local presentation only and create no replicated, gameplay, or save mutation.

### Rendered minimap regression check

The Phase 5 visibility repair replaces the sparse terrain dots with a filled, circular 129x129 texture containing original patterns for Meadows, Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, Thunder Mountains, and Ocean. Each pattern stays anchored in world space. The minimap now belongs to each local player rather than only the first game-instance controller. Its 208-unit size and 24-unit margin scale with Unreal's UI DPI curve. Size/position must be set before the top-right anchor: `SetPositionInViewport` resets it to top-left in UE 5.8 and previously put the map off-screen.

After building, run `Scripts/Verify-Minimap.ps1 -Rendered -Width 1920 -Height 1080`. Repeat with `-Width 1024 -Height 768` and `-Width 3440 -Height 1440` for different aspect ratios and automatic DPI scales. The runner checks actual paint geometry and full texture sample count on both peers, server identity replication, and the bound input delegate with minimum/maximum zoom, CommonUI Menu ownership, and zoom resumption. It captures `host.png` and `client.png` in the printed temporary log directory for visual inspection. Hidden offscreen captures verify the Slate HUD; the world background may be black and these images do not validate terrain rendering. The configured `CommonGameViewportClient` is required for CommonUI routing. Without `-Rendered`, this script remains an identity-only smoke test.

`Kalmala.UI.Minimap.LocalPresentation` now also exercises the production viewport setters, checks filled/transparent raster coverage, distinct deterministic texture patterns for all seven biomes, and sea-level ocean treatment. These checks replace the previous assumption that mathematical placement assertions alone proved on-screen visibility. The launch-gated input scenario temporarily changes local input configuration and restores it and zoom; it sends no RPC or gameplay request. A normal launch requires no minimap flag. Restart the editor/game to load rebuilt C++ modules and viewport configuration; an older packaged executable needs a separate rebuild/package.

## Local settings menu

Press Escape during normal play to open the local Settings menu; press Escape again to close it and restore game input. The menu's main screen provides Options and Quit. Options contains Video, Audio, Controls, and Settings tabs; Video immediately applies and saves resolution, V-Sync, window mode, and render-distance quality through Unreal `GameUserSettings`. Run `Kalmala.UI.Settings.LocalPresentation` after an editor build for the render-distance bounds seam. This is local presentation/preferences only: no setting, menu action, or quit request is sent to the server.

## Expanded world map

The expanded map uses full-stretch viewport anchors with zero offsets. In UE 5.8, `SetDesiredSizeInViewport` becomes right/bottom margins under stretch anchors; setting a fixed 1920x1080 size collapses the surface at common viewport sizes and leaves only a marker near the top-left. Keep those margins zero and derive tile aspect ratio from the same DPI-scaled Slate geometry used for painting and pointer input. `Scripts/Verify-WorldMap.ps1` now requires actual full-player-viewport paint geometry, completed terrain tiles, and a fog texture on both peers before accepting screenshots at all three resolutions. Its zoom-bound probes restore the normal opening zoom before capture.

Press `M` to open the local expanded map. It occupies the viewport with cached 10,000 cm world-space terrain/water tiles, generated asynchronously from the replicated world identity and owning pawn, so it never reveals population, discoveries, or other server-only data. Drag with the left mouse button to pan, use the mouse wheel to zoom at the pointer, and press `R` to recenter on the owning player. Press `M` or Escape to close it; Escape closes the map before opening Settings. A view or identity change immediately drops obsolete local tile handles before requesting at most 64 centre-prioritized tiles; stale workers are epoch-rejected and never block the game thread. A 6,500 cm circle around only the owning pawn's already replicated transform is clear; revealed 500 cm cells persist in a bounded 8,192-cell local save keyed by seed, generator revision, and local-player index. Remembered personal coverage has a translucent sea-glass teal tint; a contrasting lichen-ember tint is reserved for future explicitly opted-in shared coverage, which is not yet loaded or rendered. The local save is independent of generated-world sparse saves and identity mismatches are rejected. Pan and zoom cannot change the reveal source or radius. A pale facing triangle shows only the owning pawn at its true world-to-map position (centred after recentering); world-aligned reference lines use a local 2,500, 5,000, or 10,000 cm scale and provide no route or destination. Run `Kalmala.UI.WorldMap.LocalPresentation`, `Kalmala.UI.WorldMap.PerformanceBudget`, and `Kalmala.UI.Minimap.LocalPresentation` after an editor build to verify deterministic tile edges, a 278,784-byte maximum CPU tile-pixel payload, a 250 ms single-tile worker budget, and a 1.5 s full-minimap worker budget while four map tiles are running. `Scripts/Verify-WorldMapTiles.ps1` starts a revision-4 host and conflicting-seed client, checks the client receives the immutable server identity, and compares their completed visible-tile count and deterministic pixel fingerprint. It then restarts both peers with their own retained local user directories and requires matching-identity personal coverage to report `Loaded=1` while a remote probe remains opaque. `Scripts/Verify-WorldMap.ps1` renders the same authoritative host/client pair at 1024x768, 1280x720, and 2560x1080, requiring both peers' open/input/zoom/pan/recenter trace, the client identity receipt, and one screenshot per peer per resolution. The runtime logs that fingerprint only after polling ready workers; it never waits on a game-thread future. These are development hardware guardrails rather than shipping frame-time targets; screen-reader integration and input remapping remain later accessibility increments.

Run `Scripts/Verify-WorldMapProfile.ps1` before increasing the map tile density or range. It starts the same revision-4 host and conflicting-seed late-joining client, opens/pans/zooms/recentres each local map through the existing verification path, and requires both peers to report open latency, aggregate/maximum tile-worker time, aggregate/maximum local map-tick time, ready-tile count, and bounded CPU tile-cache bytes. The client must first receive the server identity; this is a profile-only local UI run with no world mutation, RPC, or new replication.

Shift-click the expanded map to begin a personal pin: type a 1–32-character label, choose `1` Cairn, `2` Lantern, or `3` Thread, and press Enter to place. Click a visible pin to mark it complete, Ctrl-click to hide it, and right-click to remove it. Every marker displays textual style, label, completion, and visibility information in addition to its visual treatment. `Tab` selects pins (including hidden ones), `Enter` toggles completion, `H` toggles visibility, `Delete` removes, and `P` begins placement at the map centre; arrow keys pan, Page Up/Down zoom, and `R` recentres. Gamepad face buttons select, place/toggle, visibility/cancel, and recenter; left trigger removes while D-pad/shoulders pan/zoom. Pins persist locally in an independent version-1 slot keyed by immutable seed/revision and local-player index; only the newest 256 finite, validated annotations are retained, and a mismatched world slot is discarded. They never create an RPC, discovery claim, route, or gameplay instruction. `Kalmala.UI.WorldMap.LocalPresentation` verifies memory serialization/reload, mismatch rejection, and non-colour keyboard pin state actions alongside the existing local rendering checks.

Co-op awareness is off by default and returns to private after reconnecting. With the map open, press `C` or the gamepad Menu button to opt in; only connected, mutually opted-in non-spectators appear, and their markers/pings remain hidden outside your own current or remembered coverage. Middle-click a map point or press `Q`/right-stick click for a map-centre ping. The server accepts only finite locations within 65 m of the sender, limits requests to one every two seconds, relays to eligible nearby opted-in recipients through owner-only inboxes, and expires pings after six seconds. Opting out immediately removes ineligible pings. This shares neither exploration coverage nor personal pins; shared cartography remains unavailable until the M2 construction/persistence prerequisite exists.

## Biome feature inspection

`Kalmala.World.Biomes.TerrainSelection` verifies terrain precedence, exact boundaries, lowland/upland climate distinctions, forest moisture support, legacy selection, all-seven-biome coverage, same-identity agreement, different-seed variation, and Meadow starts for revisions 1 and 2. Run it with the headless automation flags above. `Kalmala.World.BiomeExpansion.IntegratedScenario` now covers both revisions. Existing two-peer fixture scripts explicitly select server revision 1 to retain their established terrain/weather fixtures; normal game launches default to revision 2. To retain an existing world, launch with its original seed and `-GeneratorRevision=1`.

Launch a listen server with `-KalmalaBiomeFeatureInspection` and join a player to log the server-sampled biome-expansion profile, a nearby classifier-seam flag, and a stable but non-materialized discovery candidate. The switch only inspects deterministic inputs from the replicated world identity; it does not spawn, save, reveal, or route toward content, and clients cannot request it.

## Shimmering Lakes slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.ShimmeringLakesSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a dry Shimmering Lakes discovery location, verifies adjacent seed-derived water and the wet-shore exposure tradeoff, and confirms that the stable discovery ID reproduces. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active lake key and requires neither water physics nor a boat.

## Elderwood slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.ElderwoodSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a gently sloped, lower-flora Elderwood clearing discovery, verifies the compact-canopy exposure tradeoff and stable ID reproduction, and does not create a trail or reserve a camp. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Elderwood key; field-driven canopy and root presentation are local cosmetic meshes derived from the replicated world identity and patch descriptor.

## Mossy Mire slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.MossyMireSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a gently sloped, relatively dry Mire hummock discovery, verifies increased wet-ground preparation pressure and stable ID reproduction, and does not create a crossing, route, or reserved camp. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Mire key; the server applies the bounded Mire footing drag through the existing replicated travel-speed state, while clients receive only that normal replicated state.

## Freezing Tundra slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.FreezingTundraSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for an exposed, gently rolling Tundra discovery, verifies stronger wind pressure, bounded sparse natural cover, and stable ID reproduction, and does not create an authored ridge, route, camp, or travel gate. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Tundra key; the server applies the existing profile through normal replicated exposure state while enclosed roof/windbreak shelter remains player-built counterplay.

## Thunder Mountains slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.ThunderMountainsSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a high, steep-but-traversable Mountain overlook discovery, verifies stronger weather-driven wind pressure, bounded cover, and stable ID reproduction, and does not create a designed passage, authored ridge, reserved shelter, or precision gate. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Mountain key; the existing server weather and player-built roof/windbreak shelter provide storm and lightning-safe-enclosure counterplay through ordinary replicated exposure state.

## Integrated biome-expansion scenario

After an editor build, run `Scripts/Verify-BiomeExpansion.ps1`. It runs `Kalmala.World.BiomeExpansion.IntegratedScenario`, which compares same-seed host/client terrain, classification, exposure inputs, seams, stable optional-discovery IDs, and roof/windbreak counterplay across Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, and Thunder Mountains. It then runs the existing conflicting-seed two-player camp scenario to verify replicated world identity, weather, exposure state, and recovery from two freely selected camps. This is verification only: it does not activate remote biome content, add paths, or let a client choose a server location or outcome.

## Basic player controls

Restart the editor after building the native modules, then play `L_Prototype`. The third-person character has a simple segmented humanoid model with walking and airborne poses. Press Space to jump once; hold either Shift key to sprint at 1.5 times the current walking speed. Releasing Shift restores walking speed, including the existing exposure penalty. Sprint has no stamina cost in this prototype.

After an editor build, run `Scripts/Verify-PlayerControls.ps1 -Rendered`. It launches a hidden listen server and client, exercises the bound jump/sprint/release delegates through normal movement prediction, and checks server-observed remote sprint, upward jump, release, landing, matching world identity, and nine collision-free model parts. It retains host/client screenshots and logs in its printed temporary directory and stops its own processes. Physical keyboard input is not simulated by this test. Run the focused `Kalmala.Gameplay.Movement.SprintSavedMoves` headless automation to verify compressed flags, release, move-combination boundaries, and saved-move clearing.

## Regional generation verification (Phase 7)

### Generation performance regression

After an editor build, run `Kalmala.UI.Minimap.GenerationPerformance` with the headless automation flags above and a temporary `-UserDir`/`-abslog`. It measures three moving 129x129 raster builds at each of the 2,500/5,000/10,000 cm zoom radii for seed 418, revisions 3 and 4. Timings include raster construction and output hashing, exclude engine startup, and are reported rather than asserted against a machine-dependent deadline. Fixed pre-optimization fingerprints verify exact terrain heights, water flags and colours. An additional 4 km sampling fixture compares batched output with independent ocean, inland-water and biome queries for seeds 418/419 and revisions 3/4, including negative coordinates.

Measured on 2026-09-07 in Editor Development/NullRHI: revision 3 refreshes improved from 384/381/386 ms to 19/25/47 ms; revision 4 improved from 449/455/453 ms to 18/25/47 ms (zoom radii in ascending order). These are CPU refresh timings, not whole-game FPS. The minimap retains 129x129 resolution and its existing 0.10-second refresh cadence. Maximum zoom can still exceed a 60 FPS frame budget; asynchronous presentation and wider streaming remain separate profiling work.

The repair shares warp and regional-motion calculations, rejects distant region/basin supports before expensive work, and reuses collision-vertex results within a single minimap raster build. Only existing spline data persists as a generator cache. No generator revision or world-layout change is required. Run the regional/water/island/minimap regressions and `Scripts/Verify-Minimap.ps1 -GeneratorRevision 3 -Rendered` for the existing live peer fixture.

Build `KalmalaEditor Win64 Development -WaitMutex -NoHotReload -MaxParallelActions=4`, then run `Scripts/Verify-RegionalGeneration.ps1`. The runner explicitly rejects failed automation results even when Unreal returns exit code zero, generates two seed-418 previews and one seed-419 preview, hashes every PPM for reproduction, checks biome/hydrology/height variation, and launches a revision-3 host with a conflicting-seed client. It uses a unique temporary user/cache/output directory and retains all logs and images. `-SkipPeers` is available for local analysis only; full Phase 7 acceptance requires the peer run.

`Kalmala.World.Regional.Integrated` measures seven-biome coverage, boundary density, connected components, tiny components, and maximum nearby height/weight change over a 4 km square for both seeds. It also verifies Flora independence, spline regeneration, GridCell continuity, shaped ocean-depth triangle planes, and nonempty matching water-patch edges. Legacy `Kalmala.World.Biomes.TerrainSelection`, water, sparse-save and minimap tests run alongside it. The older `BiomeExpansion.IntegratedScenario` remains a revision-1/2 local-scale fixture; it is not the regional test.

`Verify-Minimap.ps1 -GeneratorRevision 3` enables the read-only `-KalmalaRegionalVerification` fingerprint over 81 world positions on host and client. The fingerprint includes dominant biomes, all seven weights, final terrain height, water levels, and river/stream weights. It creates no client RPC or saved inspection data. Normal launches default to revision 3; existing worlds must explicitly retain their original seed/revision.

## Source-control rules

- Commit `Config/`, `Source/`, `.uproject`, and `.uasset`/`.umap` content assets.
- Never commit generated `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/` folders.
- Unreal assets are marked as binary in `.gitattributes`; resolve asset conflicts in the editor, not through text merging.

## Fuelled hearth and crafting verification

After the editor build, run `Kalmala.Gameplay.Crafting`, `Kalmala.Gameplay.Inventory` and `Kalmala.Gameplay.Campfire` with the headless temporary-user/log flags above, then `Scripts/Verify-Crafting.ps1 -Rendered`. Rendered runs need access to Unreal's shader working directory as well as the build-tool cache. The runner requires exact inventory conservation for both players, rejection of real forged/malformed RPCs and insufficient placement, two matching dry/rain-extinguished fire states, paid placement and fuel/protection gates, local menu input restoration, and retained host/client screenshots. Also run `Scripts/Verify-InventoryReconnect.ps1` and `Scripts/Verify-CampChoices.ps1`. Full contract and test limits: `10-campfire-and-crafting.md`.
