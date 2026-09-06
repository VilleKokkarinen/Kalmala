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

## Water surface regression check

`Kalmala.World.Water.ClippedSurface` checks partial-cell coverage, winding, flat levels, shallow shore treatment, seeded closed basins, rejection of sea-connected/unbounded/unseeded basins, deterministic meshes, and nonempty matching patch-edge intersections. Lake-biome fields seed enclosed terrain basins rather than clipping floating sheets at humidity/temperature boundaries. Basins exceeding 8,192 wet lattice vertices are conservatively omitted. The minimap uses the same visible water decision. Terrain/collision, server wetland rules, generator revision, and saved-data schemas are unchanged. Run `Scripts/Verify-PlayerControls.ps1 -Rendered` for rendered host/client traversal and screenshots after building. Restart the editor to load the repaired native module.

## Ocean-depth regression

After an editor build, run headless automation with `-unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -ExecCmds="Automation RunTests Kalmala.World.Water+Kalmala.UI.Minimap.LocalPresentation" -TestExit="Automation Test Queue Empty"`, directing `-UserDir` and `-abslog` into a unique temporary directory. `Kalmala.World.Water.OceanDepth` verifies depth against both collision-triangle planes, negative coordinates, equivalent adjacent-patch origins, same-identity reproduction, different-seed variation, and actual clipped coastline vertices; the fixture must contain wet sea floor, dry land, and mixed coastal triangles. It also rejects invalid identity and nonfinite query positions. Run `Scripts/Verify-Minimap.ps1 -Rendered` for live host/client identity and HUD integration. These checks do not verify swimming, island travel, or extended streaming.

## Generated-ocean swimming regression

After an editor build, run `Scripts/Verify-Swimming.ps1`. It starts a memory-only listen server with seed 418 and a conflicting-seed client with seed 999. Each owning pawn walks to the nearest deterministic sea-depth fixture and must enter the generated-ocean custom movement mode; the server log proves authoritative entry and the client log proves prediction from the server-replicated identity. Entry requires at least 100 cm depth and return-to-land uses a 75 cm hysteresis threshold. The test adds no water volume, RPC, client depth input, island, boat, or streaming change.

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

The local `UKalmalaMinimapSubsystem` creates a 208-pixel `UKalmalaMinimapWidget` for each local player once its controller is available. The widget is anchored to the top-right viewport corner with a 24-pixel margin. It refreshes local seed-derived terrain/water samples around that player's pawn and draws only samples inside a circular radius, together with a centred marker rotated to the pawn's facing yaw. `MouseWheelAxis` changes only the local session's sampled radius, clamped from 2,500 to 10,000 cm in 750 cm steps; CommonUI's normal-game-input gate leaves wheel input to any modal UI. It reads no world actors, population, landmarks, or gameplay state beyond the already-replicated world identity and owning pawn transform.

Run `Scripts/Verify-Minimap.ps1` after an editor build for the two-peer identity check. It starts a memory-only hidden listen server with seed 418 and a conflicting-seed client with seed 999, then confirms that the client receives the server world identity. The focused `Kalmala.UI.Minimap.LocalPresentation` automation checks the two player-centred local views from that same identity, circular clipping, min/max zoom, modal input gating, and the top-right footprint at 4:3/75%, 16:9/100%, and ultrawide/125% UI scales. These checks are local presentation only and create no replicated, gameplay, or save mutation.

### Rendered minimap regression check

The Phase 5 visibility repair replaces the sparse terrain dots with a filled, circular 129x129 texture containing original patterns for Meadows, Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, Thunder Mountains, and Ocean. Each pattern stays anchored in world space. The minimap now belongs to each local player rather than only the first game-instance controller. Its 208-unit size and 24-unit margin scale with Unreal's UI DPI curve. Size/position must be set before the top-right anchor: `SetPositionInViewport` resets it to top-left in UE 5.8 and previously put the map off-screen.

After building, run `Scripts/Verify-Minimap.ps1 -Rendered -Width 1920 -Height 1080`. Repeat with `-Width 1024 -Height 768` and `-Width 3440 -Height 1440` for different aspect ratios and automatic DPI scales. The runner checks actual paint geometry and full texture sample count on both peers, server identity replication, and the bound input delegate with minimum/maximum zoom, CommonUI Menu ownership, and zoom resumption. It captures `host.png` and `client.png` in the printed temporary log directory for visual inspection. Hidden offscreen captures verify the Slate HUD; the world background may be black and these images do not validate terrain rendering. The configured `CommonGameViewportClient` is required for CommonUI routing. Without `-Rendered`, this script remains an identity-only smoke test.

`Kalmala.UI.Minimap.LocalPresentation` now also exercises the production viewport setters, checks filled/transparent raster coverage, distinct deterministic texture patterns for all seven biomes, and sea-level ocean treatment. These checks replace the previous assumption that mathematical placement assertions alone proved on-screen visibility. The launch-gated input scenario temporarily changes local input configuration and restores it and zoom; it sends no RPC or gameplay request. A normal launch requires no minimap flag. Restart the editor/game to load rebuilt C++ modules and viewport configuration; an older packaged executable needs a separate rebuild/package.

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

## Source-control rules

- Commit `Config/`, `Source/`, `.uproject`, and `.uasset`/`.umap` content assets.
- Never commit generated `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/` folders.
- Unreal assets are marked as binary in `.gitattributes`; resolve asset conflicts in the editor, not through text merging.
