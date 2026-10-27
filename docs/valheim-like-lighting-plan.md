# Valheim-inspired material and lighting plan

**Status:** L01–L03 are complete. L04–L06 remain in the active lighting queue in `BACKLOG.md`; L06 is the integrated milestone-final gate.
This file records the research, implementation choices, and verification evidence.

## Goal

Explore a Kalmala-owned lighting and material treatment that takes cues from
Valheim's readable, stylized world: deliberate surface response, clear warm/cool
light separation, atmospheric distance, and materials that respond to weather.
Keep Kalmala's original northern twilight, forests, lake country, and
Kalevala-inflected fantasy direction. Do not copy Valheim shader source, textures,
models, symbols, or exact color palettes.

The request concerns lighting and shaders. Low-resolution output, texture
filtering, mesh density, and silhouette changes are outside this plan.

## What the research supports

Iron Gate describes Valheim as a combination of low-resolution textures and
sparse polygon detail with modern materials, lighting, and post effects. A direct
Iron Gate interview similarly describes a retro 3D look paired with modern post
effects. This points to a coordinated rendering stack rather than one magic
shader. ([Valheim FAQ](https://www.valheim.com/faq/), [Iron Gate interview,
Swedish](https://www.fz.se/artikel/285972-iron-gate-svarar-pa-era-valheim-fragor))

The public shader-property listing for Valheim 1.0.7 shows different custom
shader families for creatures, building pieces, and static rocks. Their exposed
properties include style textures and color adjustment, metallic/smoothness,
triplanar mapping, value noise, rain and snow controls, moss layers, and edge
glow. The list is generated from the game with JötunnDoc; it exposes property
names, not shader source or equations. It supports a per-surface material system,
but does not establish a particular light-ramp formula or exact lighting model.
The property names suggest useful effect families to prototype—sky and cloud
atmosphere, weather-aware surfaces, and selective surface glow—but they do not
reveal Valheim's post-process volume setup, effect defaults, or how those values
are driven at runtime.
([Jötunn Valheim shader listing](https://valheim-modding.github.io/Jotunn/data/prefabs/shader-list.html))

Therefore, a stepped or ramped direct-light response is a visual experiment for
Kalmala, not a claimed recreation of Valheim's internals. Compare it against
Unreal's ordinary lit response before deciding whether it helps. Unreal's
Default Lit model already combines direct and indirect lighting and specular
response. Material instances can tune shared parent materials without shader
recompilation, while Epic recommends using post-process volume settings for
ordinary grading and effects and reserving custom post-process materials for
cases that need them. ([UE 5.8 shading models](https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine),
[material instances](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-materials-in-unreal-engine),
[post-process materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-materials-in-unreal-engine))

## Kalmala starting point

Kalmala uses Unreal Engine 5.8. Generated terrain, water, shore, rock, bark, and
canopy components currently resolve project-owned materials under
`Content/Kalmala/World/Materials/`. The commandlet source builds M_GeneratedTerrain as a vertex-color blend of
grass, soil, rock, and snow; other generated surfaces use the simple
constant-color/roughness creation path, and the separate biome-debug material
passes vertex color to base and emissive. M_GeneratedSoil is generated as an
asset but is not assigned by the terrain-patch constructor. Existing generic
material packages are left untouched by the creator, so these are source
generation facts rather than confirmed loaded asset graphs. See the source
audit below for assignments and limits. The project brief's established
mood is a northern wilderness shaped by forests, lakes, and long twilight.

Before selecting values, inspect the actual prototype map's lights, sky, fog,
exposure, and any level post-process settings in Unreal Editor. The project
defaults alone do not define a complete scene lighting rig.

### L01a source audit — 2026-10-26

The table below records source-derived facts and limits. Live map and renderer
observations follow in the L01a Editor inspection section.
This is an L01a snapshot; later capture-only changes are recorded separately.

| Area | Source-derived facts and limits |
| --- | --- |
| Active prototype map | Config/DefaultEngine.ini:3-4 sets both EditorStartupMap and GameDefaultMap to /Game/Kalmala/Maps/Prototype/L_Prototype; Content/Kalmala/Maps/Prototype/L_Prototype.umap is tracked. Kalmala.uproject:3 associates the project with Unreal Engine 5.8. This establishes project defaults, not the current Editor session's loaded map. |
| Prototype map object paths | A raw scan of the tracked `.umap` finds `PersistentLevel.DirectionalLight_0`, `SkyAtmosphere_0`, and `SkyLight_0`, along with two PlayerStarts and `KalmalaInteractionTestActor_0`; the package also contains the `KalmalaEnvironmentSun`, `KalmalaEnvironmentAtmosphere`, and `KalmalaEnvironmentSkyLight` tag strings. No post-process, exponential-fog, or campfire actor path appeared in this scan. This is string-level package evidence, not a parsed actor/property report; absence in the scan does not establish the loaded level contents or component values. |
| Renderer | The tracked Config/*.ini files contain no explicit RHI, Lumen GI/reflection, exposure, or post-process selection. Kalmala.uproject records the engine association and plugins, not the resolved renderer. Active RHI and effective GI/reflection methods remain unknown. |
| Environment actor setup | Scripts/Setup-PrototypeEnvironment.py:8-39 finds actors by tag or spawns them, then sets the tagged directional light to rotation (-40, -35, 0), movable, atmosphere-sun enabled, intensity 10.0; it ensures a SkyAtmosphere without setting its properties; and sets a tagged SkyLight to movable, real-time capture, intensity 1.0. Other light, atmosphere, and sky properties are not set by this script. Because tagged existing actors are reused and the script's save action was not run for this audit, serialized actor values remain unknown. |
| Runtime weather fog | Source/KalmalaUI/Private/KalmalaWeatherActivitySubsystem.cpp:17-110 creates a transient, nonreplicated ExponentialHeightFog only in a game world with a local player, not on a dedicated server. It probes every 1 second and scales coverage by storm intensity times Thunder Mountains biome weight, fades at speed 1.5, and scales density to 0.005 and max opacity to 0.24; start distance is 2500 and the color shifts from white toward (0.32, 0.34, 0.41). This client presentation effect is not a serialized map actor; whether it is active at any baseline view is unverified. The code does not set volumetric fog or other component properties. |
| Fog, exposure, post-process | The setup script does not configure fog, exposure, or post-process. A search of tracked configuration and production C++ found no explicit post-process volume/settings or auto-exposure writer. The runtime storm fog above is the source-level exception for fog. Serialized level volumes, effective exposure, and other fog actors remain uninspected. |
| Time and weather controls | `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1014-1045` initializes weather cycle 0 from the world seed and current game elapsed time; `Source/KalmalaWorld/Public/KalmalaWeatherCycle.h:10-54` derives each interval deterministically from the seed and cycle index. The baseline runner supplies the seed but has no weather-cycle or time-of-day selector. The environment setup script sets one fixed sun rotation; no tracked production time-of-day driver was found. |
| Generated material assignments | Source/KalmalaWorld/Private/KalmalaGeneratedTerrainPatch.cpp assigns MI_GeneratedTerrain to terrain (or M_GeneratedTerrainBiomeDebug in debug mode), MI_GeneratedWater to ocean and lake water, MI_GeneratedLakeShore to lake shore, MI_GeneratedRock to Meadow rocks, MI_GeneratedBark to Meadow trunks, and MI_GeneratedCanopy to Meadow canopies. AKalmalaHarvestNode assigns MI_GeneratedBark only to tree-resource source meshes and retains M_GeneratedBark for other non-rock gatherables. The player head keeps M_GeneratedBark; held-tool wood uses MI_GeneratedWood. M_GeneratedTerrain remains the layered parent and the cloth material; M_GeneratedRock remains assigned to the player model and campfire ring. M_GeneratedSoil is not among the terrain-patch constructor's assignments. The commandlet builds M_GeneratedTerrain from vertex-color R/G/B weights for grass/soil/rock plus alpha snow, then routes that blend through MF_KalmalaSurfaceResponse. MI_GeneratedWater, MI_GeneratedLakeShore, MI_GeneratedRock, MI_GeneratedBark and MI_GeneratedWood use M_KalmalaSurfaceBase. MI_GeneratedWater uses white tint and roughness 0.28 to preserve the existing ocean and lake vertex-color palettes. MI_GeneratedCanopy uses the separate two-sided opaque M_KalmalaCanopyBase and its shared surface response, with white tint and roughness 0.82. The canopy mesh has no opacity cutouts; M_GeneratedCanopy is retained as an unassigned legacy package. Bark keeps species/cut-face vertex colors at roughness 0.88, wood keeps the prior brown tint/roughness 0.88, and shore/rock tints preserve their prior base palettes. The separate biome-debug material feeds vertex color to base and emissive. These are source-generation and constructor facts; rendered results and any map overrides were not inspected. |
| Campfire baseline support | `Source/KalmalaGameplay/Private/KalmalaCampfire.cpp:20-67,263-268` builds an original procedural stone ring using `M_GeneratedRock` and an orange point light with 600 cm radius; the light is off until the hearth is lit, then scales to 2500 × effective warmth. The baseline capture helper does not spawn or light a hearth, and the setup script does not place one. Whether the serialized map contains a placed hearth remains unknown. |
| Capture helper and scene anchors | Scripts/Tests/World/BiomeBaselineCapture.ps1:1-45 requires seed 418 or 419, a Site string, caller-supplied Xcm/Ycm/ViewZcm, and an output path; it launches the project map with -game, -windowed, -RenderOffscreen, -ForceRes, and 1024x768, with a separate log and UserDir. It does not set time of day, weather, or exposure. Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10200-10249 accepts those coordinates directly, activates nearby terrain, then creates a transient camera at (X, Y-2400, ViewZ+1200), aimed at (X, Y, ViewZ+150), with FOV 65, and requests one screenshot. Site is only checked for nonempty and logged; the capture code contains no named-scene-to-coordinate table. The runner checks the PNG signature, 1024x768 dimensions, basic nonblank samples, and the requested seed/site log entry; those checks do not establish scene content or visual suitability. An earlier launch through UnrealEditor.exe returned exit code 0 without a log or image; L01b succeeded when the runner's `-Editor` parameter selected UnrealEditor-Cmd.exe. |

Existing world-generation records provide provisional camera poses for three
lighting-scene classes. They use the same 65-degree, +Y-facing camera convention
and seed-418 generator sites, but the old captures did not lock time, weather,
or exposure. The screenshots were for terrain review, are not present in this
checkout, and do not establish lighting/material acceptance.

| Scene candidate | Seed-418 site and pose | Source evidence and limit |
| --- | --- | --- |
| Meadows daylight | Starter Meadows at `(-781.6, -781.6)` cm; `ViewZ=528.94458`; camera `(-781.6, -3181.6, 1728.94458)`; aim `(-781.6, -781.6, 678.94458)`; FOV 65 degrees. | `docs/biome-height-rework.md` records a prior smoke capture and visual review of terrain, trees, shadows, and sky. It does not establish daylight time or exposure. Provisional only. |
| Forest shade | Wooded Elderwood at `(207132.4, -325940.4)` cm; `ViewZ=15.1806488`; camera `(207132.4, -328340.4, 1215.1806488)`; aim `(207132.4, -325940.4, 165.1806488)`; FOV 65 degrees. | `Scripts/Tests/World/BiomeBaseline418419.csv` records 26 forest descriptors at this site, and the prior terrain capture was visually reviewed. Actual shade in this fixed view is unverified. Provisional only. |
| Water | Shimmering Lakes at `(100830.5, -143038.6)` cm; water-level `ViewZ=356.950745`; camera `(100830.5, -145438.6, 1556.950745)`; aim `(100830.5, -143038.6, 506.950745)`; FOV 65 degrees. | The numeric baseline records water at this site. This is an inland-lake view, not a coast view; water/material appearance is not accepted. Provisional only. |
| Coast | Source-coast land-side site `(691744.0, -107083.5)` cm. | The numeric baseline marks the sample wet, but the prior rendered source-coast frame did not show a separate water surface. Do not use this pose as the coast/water view without a better framing. |
| Overcast/rain and fire-lit dusk/night | No accepted fixed poses. | The capture helper has no named-scene-to-coordinate map and does not control time or weather. Select locations and conditions only after live scene review. |

### L01a live Editor inspection — 2026-10-26

The project was built as `KalmalaEditor` in an isolated short mirror and opened
in Unreal Editor 5.8. The loaded map was
`/Game/Kalmala/Maps/Prototype/L_Prototype`. The editor Outliner showed six
serialized actors: the tagged environment sun, SkyLight and SkyAtmosphere, two
PlayerStarts, and `KalmalaInteractionTestActor0`. No serialized fog or
PostProcessVolume actor was present. Starting PIE produced 44 runtime actors,
including nine generated terrain patches; the map had no terrain surface in the
static editor viewport before those runtime patches were generated. No map
changes were saved.

| Live area | Observed state and limit |
| --- | --- |
| Renderer | The session log reports D3D12 and active feature level SM5 on an RTX 5090. Live Editor console queries returned `r.DynamicGlobalIlluminationMethod=0` (no dynamic GI), `r.ReflectionMethod=2` (Lumen reflections), `r.RayTracing=0`, `r.DefaultFeature.AutoExposure=1`, and `r.EyeAdaptationQuality=2`. Scalability was at quality 3 for global illumination, reflections, and post process; these quality values do not select the GI method. |
| Directional light | `KalmalaEnvironmentSun`: movable, transform location `(0,0,2000)`, rotation `(-40,-35,0)`, scale `(2.5,2.5,2.5)`; 10 lux, white, source angle 0.5357 degrees, source soft angle 0, temperature disabled, world illumination and shadows enabled. |
| Sky fill | `KalmalaEnvironmentSkyLight`: movable, real-time capture enabled, captured-scene source, no cubemap, resolution 128, intensity scale 1.0, white, sky distance threshold 150000. |
| Atmosphere | `KalmalaEnvironmentAtmosphere`: movable, planet top at absolute world origin, ground radius 6360 km, atmosphere height 60 km, multiscattering 1.0. |
| Fog, exposure and post process | No fog or post-process volume actor appeared in the loaded map. The meadow PIE view had no visible storm fog. Tracked runtime source creates transient client-local exponential fog only for storm activity weighted by Thunder Mountains. Auto exposure is enabled and eye-adaptation quality is 2, but no active volume or tracked runtime writer establishes compensation or min/max EV; those effective exposure bounds remain unknown. |
| Generated materials | PIE Details verified `M_GeneratedTerrain` in a generated patch terrain slot and `M_GeneratedWater` in its surface-water slot. The source-derived constructor assignments for lake shore, rock, bark, and canopy remain listed above; their loaded runtime slots and material graphs were not individually inspected. `M_GeneratedSoil` remains unassigned by that constructor. |
| Capture settings | The existing helper uses seed 418 or 419, caller-supplied site/coordinates, `-game -windowed -RenderOffscreen -ForceRes`, 1024x768, a separate log/UserDir, a 65-degree transient camera, and one screenshot. It does not pin time, weather, or exposure, nor spawn/light a hearth. The user-requested isolated `KalmalaEditor` build completed 55/55 actions. Its default UnrealEditor.exe launch failed to create a log or PNG; the L01b run succeeded with `-Editor UnrealEditor-Cmd.exe`. |

The following five seed-418 camera anchors are selected as repeatable inputs to
the next baseline captures. Camera position and aim use the helper's existing
`(X, Y-2400, ViewZ+1200)` and `(X, Y, ViewZ+150)` convention with 65-degree
FOV. Selection records stable world locations; it does not claim that all five
frames or scene conditions have passed visual acceptance.

| Baseline scene | Site and fixed camera / aim (cm) | Capture note |
| --- | --- | --- |
| Meadows daylight | Starter Meadows `(-781.6,-781.6)`, `ViewZ=528.94458`; camera `(-781.6,-3181.6,1728.94458)`, aim `(-781.6,-781.6,678.94458)`. | Existing seed-418 terrain review site. Time and exposure are not pinned. |
| Forest shade | Wooded Elderwood `(207132.4,-325940.4)`, `ViewZ=15.1806488`; camera `(207132.4,-328340.4,1215.1806488)`, aim `(207132.4,-325940.4,165.1806488)`. | CSV records 26 forest descriptors. Shade and framing require the baseline image review. |
| Coast and water | Source-coast land-side `(691744.0,-107083.5)`, `ViewZ=0`; camera `(691744.0,-109483.5,1200)`, aim `(691744.0,-107083.5,150)`. | Fixed seed-418 coastal candidate. A prior frame at this site did not show a separate water surface; L01b must review/replace the framing. |
| Overcast / rain | Thunder Mountains `(-970004.9,-243087.4)`, `ViewZ=768.734985`; camera `(-970004.9,-245487.4,1968.734985)`, aim `(-970004.9,-243087.4,918.734985)`. | Chosen to exercise the Thunder Mountains storm-fog presentation. The helper cannot select a storm/weather cycle. |
| Fire-lit dusk / night | Starter Meadows `(-781.6,-781.6)`, `ViewZ=528.94458`; camera `(-781.6,-3181.6,1728.94458)`, aim `(-781.6,-781.6,678.94458)`. | Shares the daylight anchor; the point at `(-781.6,-781.6,528.9)` cm is the intended hearth center. The map has no placed hearth, and the helper cannot spawn/light one or select dusk/night. |

L01a's map, renderer, light, exposure, fog, material-assignment, capture-input,
and five-anchor audit is recorded. L01b reviewed the daylight, forest, and water
framing. L01c added the weather/hearth capture controls and rendered the two
remaining seed-418 conditions. All five seed-418 views are present; the fire
frame remains too dark to judge surrounding illumination. Exposure remains
automatic with unknown effective EV bounds; FireDusk has a capture-only +1.5 EV bias.

### L01b seed-418 capture set — 2026-10-26

Used the already-built UE 5.8 mirror and
`Scripts/Tests/World/BiomeBaselineCapture.ps1` with
`-Editor C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`.
The runner passed its PNG signature, 1024x768 dimensions, nonblank-sample, and
seed/site log checks for the retained images. The default `UnrealEditor.exe`
launch had exited before logging; the command-line Editor override was needed.

| Capture | Site and camera inputs | Review |
| --- | --- | --- |
| `Seed418_MeadowsDaylight.png` | Starter Meadows `(-781.6,-781.6)`, `ViewZ=528.94458`; FOV 65. | Generated ground is pale neutral gray with visible patch lines; sparse triangular rocks/trees cast long cool shadows. Daylight is visible, but this is an ungraded baseline. |
| `Seed418_ForestShade.png` | Wooded Elderwood `(207132.4,-325940.4)`, `ViewZ=15.1806488`; FOV 65. | Dense tree silhouettes are present, but foliage and ground are bright neutral gray. The fixed view does not read as dark forest shade. |
| `Seed418_OpenWater.png` | Ocean sample `(970004.9,-243087.4)`, `ViewZ=0`; FOV 65. | A broad flat gray water surface fills the frame; no shoreline is visible. Use this as the water reference, not as a combined coast-and-water frame. |
| `Seed418_CoastCandidate.png` (supplemental) | Source-coast land-side `(691744.0,-107083.5)`, `ViewZ=0`; FOV 65. | The selected coast pose shows terrain but no distinct water surface. Retained as a framing diagnostic; the ocean pose supplies the water image for this set. |

Capture files and `manifest.json` are under
`TestResults/LightingBaseline/`. The manifest records exact camera and aim,
resolution, renderer, GPU, exposure CVars/limits, per-image byte count, SHA-256,
and the runner's checks. D3D12/SM5 on an RTX 5090 was resolved in the capture
logs. Auto exposure remains enabled with eye-adaptation quality 2; compensation
and EV bounds were not pinned. The Shimmering Lakes candidate was reviewed but
not retained because its image did not provide a clearer water/shore distinction.

L01b's seed-418 daylight, forest, and water images are captured and recorded.
The source-coast framing and dark forest condition remain limitations for later
scene review; this capture increment does not tune lighting or materials.

### L01c1 capture-only condition controls — 2026-10-26

`BiomeBaselineCapture.ps1` accepts `-Condition Default`, `OvercastRain`, or
`FireDusk` and passes the value to the Development-only baseline capture path.
The selected seed-418 Thunder Mountains view sets precipitation and fog to 0.8,
wind to 0.75, reduces sun intensity to 2.0, retains sky intensity 1.0, moves
the local pawn to the fixed sample site for the existing storm-fog probe, and
delays the screenshot three seconds for its one-second probe and fade. The
Meadows dusk view clears weather, turns the sun off at rotation `(8,-35,0)`,
sets sky intensity to 0.12, and spawns a transient lit hearth at the fixed
anchor. Both use the existing 65-degree camera and automatic exposure CVars.

These branches run only with the non-Shipping `-KalmalaBiomeBaselineCapture`
fixture. The hearth state is initialized by a server-checked Development-only
method; the weather override uses the existing authoritative world-generation
game state. No map is saved, and no ordinary gameplay session or Shipping build
uses these test overrides. The existing project has no visible rain particle
system or flame mesh; the planned captures show storm-state fog and the hearth's
warm point-light response, not precipitation particles or a modeled flame.
Exposure compensation and EV bounds remain unknown.

At the L01c1 checkpoint, PowerShell parsing and diff whitespace checks had
passed, while the initial mirror builds were stopped before the changed module
compiled. L01c2 subsequently compiled and rendered the fixture as recorded
below.

### L01c2 seed-418 weather and fire captures — 2026-10-26

The fixture was compiled in the short `C:/Users/Ville/AppData/Local/Temp/klc`
mirror with UE 5.8. The first compile exposed missing concrete light-component
headers and an actor/component `RecaptureSky` call mismatch; both were fixed.
The Editor target then built successfully in 216 actions, with later
capture-only adjustments rebuilding in four or five actions. The headless
runner retained its fixed 65-degree cameras, 1024x768 output, separate user
directories, and condition log checks. Default captures request screenshots
immediately as before; weather waits three seconds and FireDusk waits five.

| Capture | Final fixture values | Visual review |
| --- | --- | --- |
| `Seed418_OvercastRain.png` | Thunder Mountains at the fixed L01a anchor; precipitation 0.8, fog 0.8, wind 0.75; sun 2.0, sky 1.0; default camera compensation. | Gray sky and distance haze over a bright shoreline/water band. No rain particles appear. The frame records storm-state fog and lighting, not visible precipitation. |
| `Seed418_FireDusk.png` | Meadows at the fixed L01a anchor; clear weather; sun 0.6, sky 0.25, default sky captured before dimming, sky real-time capture disabled, and +1.5 EV capture-camera compensation. The transient test hearth uses a 4500 cm point-light radius; ordinary campfires remain at 600 cm. | Blue-black frame with only the small procedural stone ring visible. Ground and firelight spill are not readable at this wide pose, and the project has no modeled flame or time-of-day controller. This is a captured baseline limitation, not a visual acceptance result. |

Both retained frames passed PNG signature, 1024x768 dimensions, nonblank-sample,
and requested seed/site/condition log checks. The manifest now contains the three
previous primary scenes plus these weather and fire views; the coast candidate
remains supplemental. Global auto exposure remains enabled, effective EV bounds
remain unknown, and FireDusk's +1.5 EV override is capture-only. The lack of
visible rain particles, a modeled flame, and readable firelight at the fixed
wide shot remain for later lighting work.

### L01d seed-419 daylight, forest and water captures — 2026-10-26

Reused the seed-418 fixed sample points, camera offsets and 65-degree FOV for
seed 419. The headless UE 5.8 runner produced 1024x768 images under
`TestResults/LightingBaseline/`; the manifest records coordinates, aims, byte
counts, hashes and per-frame review. Logs resolve D3D12/SM5 on an RTX 5090.
These default-condition captures used sun intensity 10, sky intensity 1,
automatic exposure at adaptation quality 2 and 0 EV capture-camera compensation.

| Capture | Site and camera inputs | Visual review |
| --- | --- | --- |
| `Seed419_MeadowsDaylight.png` | Starter Meadows `(-781.6,-781.6)`, `ViewZ=528.94458`; camera `(-781.6,-3181.6,1728.94458)`, aim `(-781.6,-781.6,678.94458)`. | Pale generated ground with patch seams, scattered rocks and sparse stylized tree geometry. A wide black horizon band remains; this is an ungraded baseline. |
| `Seed419_ForestShade.png` | Wooded Elderwood `(207132.4,-325940.4)`, `ViewZ=15.1806488`; camera `(207132.4,-328340.4,1215.1806488)`, aim `(207132.4,-325940.4,165.1806488)`. | The fixed frame shows a checkerboard-like gray surface and black horizon, with no visible forest silhouette. It does not read as forest shade. |
| `Seed419_OpenWater.png` | Ocean sample `(970004.9,-243087.4)`, `ViewZ=0`; camera `(970004.9,-245487.4,1200)`, aim `(970004.9,-243087.4,150)`. | A broad checkerboard-like gray water surface sits below a blue-gray sky and black horizon; no shoreline is visible. This does not establish an accepted water look. |

All three frames passed the runner's PNG signature, 1024x768 dimensions,
nonblank-sample and seed/site/condition log checks. The manifest now records
primary daylight, forest and open-water captures for both seeds. The fixed
seed-418 poses were retained as required, so the seed-419 forest and water
content differences remain visible baseline limitations for L01e/L01f review;
L01d does not retune scene anchors, lighting or materials.

### L01e seed-419 weather and fire captures — 2026-10-26

Reused the fixed L01a weather and fire anchors with seed 419, the same capture
fixture, 65-degree camera and 1024x768 output. The headless UE 5.8 runner
resolved D3D12/SM5 on the RTX 5090; both logs report eye-adaptation quality 2.
OvercastRain used default camera compensation; FireDusk retained its capture-only
+1.5 EV bias.

| Capture | Site and capture | Visual review |
| --- | --- | --- |
| Seed419_OvercastRain.png | Thunder Mountains (-970004.9,-243087.4), ViewZ=768.734985; camera (-970004.9,-245487.4,1968.734985), aim (-970004.9,-243087.4,918.734985); precipitation 0.8, fog 0.8, wind 0.75; sun 2.0, sky 1.0. | The frame has a dark sky/horizon and broad cyan surface with only a tiny centered avatar. No rain particles, readable mountain silhouettes or clear terrain framing appear. It is retained as a baseline limitation, not a visually accepted overcast scene. |
| Seed419_FireDusk.png | Starter Meadows (-781.6,-781.6), ViewZ=528.94458; camera (-781.6,-3181.6,1728.94458), aim (-781.6,-781.6,678.94458); clear weather, sun 0.6, sky 0.25, default sky captured before dimming, and +1.5 EV camera compensation. The transient test hearth uses a 4500 cm point-light radius; ordinary campfires remain at 600 cm. | The frame is almost entirely black, with only a tiny procedural stone ring visible at center. Surrounding ground and firelight spill are unreadable, and no modeled flame exists. This is baseline evidence, not visual acceptance. |

Both captures passed the runner's PNG signature, 1024x768 dimensions,
nonblank-sample and requested seed/site/condition checks. The shared manifest
now contains all five primary scene roles for seeds 418 and 419 plus the
supplemental seed-418 coast candidate. The lack of visible rain, the unreadable
fire-light view, and unknown effective EV bounds remain for later lighting
review; L01e did not change anchors, lighting, or materials.

### L01f scene-wide rig and grade — 2026-10-26

L01f retains Default Lit surfaces and the original pale northern daylight mood.
The startup map now has a tagged, enabled, unbound
`KalmalaEnvironmentPostProcess` volume (priority 0, blend radius 0, weight 1).
The existing directional sun is 9.0 and the movable sky fill is 1.5. Built-in
post-processing is neutral and restrained: contrast 1.02, saturation 1.04,
BloomIntensity 0.30, and no custom post-process material or color tint.

UE 5.8 readback from the saved volume confirmed `AutoExposureMinBrightness=0.03`
and `AutoExposureMaxBrightness=2.0`, with both override flags true. The
histogram range is 10–90 percent and adaptation speeds are 2.0/2.0. These are
UE brightness settings, not an EV interval; the effective EV limits remain
unknown, and continuous exposure transitions were not tested.

| Exposure trial | Saved-volume readback and fixed-anchor result |
| --- | --- |
| 0.1–4.0 | Both min/max overrides read back true. Seed-418 Meadows daylight produced a uniform black capture (10 sampled colors, 0 nonblack samples); rejected. |
| 0.03–2.0 (selected) | The unchanged minimum and lower maximum preserve the daylight, forest, water and overcast frames within small luminance changes. At the correct Meadows FireDusk anchor, only the baseline-sized procedural ring remains visible; the scene is still not readable. |
| 0.1–8.0 | Both overrides read back true. At the correct Meadows FireDusk anchor, the frame was entirely black (11 sampled colors, 0 nonblack samples); rejected. |

The seed-418 and seed-419 after captures use the same fixed poses and conditions
as their baselines. The ten image hashes, byte counts, scene labels and review
notes are in `TestResults/LightingBaseline/manifest.json`; images are named
`L01f_After_Seed{418|419}_<Scene>.png`.

| Scene pair | Visual review |
| --- | --- |
| Meadows daylight | Terrain remains pale with visible seams, sparse angular trees and hard shadows; the broad black horizon remains. |
| Forest shade | Seed 418 retains pale gray trees and hard shadows. Seed 419 has a checkerboard-like ground at the reused anchor and no readable forest silhouette. |
| Open water | Both seeds show a flat checkerboard-like gray surface under a black horizon, with no shoreline. |
| Overcast/rain | Seed 418 shows a gray sky and bright ground with a cyan water band; seed 419 shows a dark horizon and broad cyan surface. Neither has visible rain or readable mountain framing. |
| FireDusk | Both frames remain almost black, with only a tiny procedural stone ring visible; surrounding ground and firelight spill are unreadable, and there is no modeled flame. FireDusk remains unresolved for L01h. |

The existing client-local storm-fog source limits changed from maximum density
0.005 to 0.004 and maximum opacity 0.24 to 0.20. The mirror was not rebuilt, so
these source-only fog edits are not reflected in the captured runtime. No
material families, gameplay, networking, authority or save behavior changed.
The headless map setup commandlet and saved-volume readback, plus the UnrealEditor-Cmd.exe capture helper
passed; full build, movement and exposure-transition verification remain
deferred to L06.

### L01g local post-process volumes — N/A — 2026-10-26

No bounded local post-process volume is justified in the current prototype.
`Content/Kalmala/Maps` contains only `Prototype/L_Prototype.umap`, and
`Config/DefaultEngine.ini:3-4` sets it as both the game and editor startup map.
The project brief (`docs/00-project-brief.md:30`) describes this slice as
seed-generated biomes rather than authored areas. The L01a live map inspection
recorded the original map's six serialized actors (environment sun, SkyLight,
SkyAtmosphere, two PlayerStarts and the interaction-test actor), with no
serialized location-specific volume or authored scene; starting PIE generated
nine terrain patches. `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11329-11341`
confirms that terrain patches are spawned dynamically from patch coordinates.

L01f added one scene-wide `KalmalaEnvironmentPostProcess` volume. The setup
script sets it `bUnbound=true`, priority 0, blend radius 0 and blend weight 1
(`Scripts/Setup-PrototypeEnvironment.py:38-43`), so it is the global baseline,
not a bounded location treatment. The prototype exposes generated biome samples
and capture-only weather/fire conditions, not an existing authored interior or
other location with a distinct look. L01g is therefore N/A; no local volume was
added, so there is no local boundary or UI/text treatment to validate. Revisit
this only if an authored location is added. L01g is complete; L01h's final
milestone verification and visual review are recorded below.

### L01h milestone-final verification — complete — 2026-10-27

The UE 5.8 `KalmalaEditor Win64 Development` build passed against the fresh
short-path source mirror at `C:/Users/Ville/AppData/Local/Temp/klh1`. The final
incremental build compiled `KalmalaPlayerControlsTest.cpp` and linked
KalmalaGameplay; earlier final-state rebuilds compiled the changed
`KalmalaGameMode.cpp` and `KalmalaCampfire.cpp`. UnrealBuildTool used the normal
per-user location. The final unattended, null-RHI `Automation RunTests Kalmala`
queue passed 228 tests, 0 failures, exit code 0; log:
`C:/Users/Ville/AppData/Local/Temp/klh1auto3/tests.log`.

All ten fixed seed/site views were freshly rendered from that built mirror with
`Scripts/Tests/World/BiomeBaselineCapture.ps1` and `UnrealEditor-Cmd.exe`. They
passed PNG signature, 1024x768, nonblank-sample and seed/site/condition checks,
then were visually reviewed as a set. The five captures that changed from
L01f—the slightly different seed-418 daylight frame, both compiled storm-fog
frames and both FireDusk frames—are preserved with the two movement frames in
`TestResults/LightingBaseline/`; the other five final images byte-match their
preserved L01f counterparts.

The FireDusk fixture now keeps its fixed XY site but samples the terrain height
for each seed before positioning the transient player and hearth. It uses a
close camera (500 cm horizontal distance, 250 cm high, aiming 80 cm above the
surface), the campfire's ordinary 600 cm light radius, and 0.0 EV additional
compensation. Both dusk renders show the player silhouette, the procedural
stone ring, and a readable warm pool on nearby ground. The first final attempt
still failed the seed-419 nonblank-sample threshold because it reused seed-418's
terrain height; the per-seed surface sample fixed that issue and the full ten
views passed on rerun. There is no modeled flame.

For ordinary third-person movement, the development-only player-controls
fixture captured the local view immediately before and after its normal bound
sprint, move and jump inputs. The `Controls local result` passed. The attached
spring-arm camera follows the moved pawn. Across those frames, sampled sky mean
luminance changed 102→107/255 and sampled lower-frame mean changed 190.2→180.7
with the new view composition; neither sample region had a channel at or above
250. This supports stable exposure for this short movement check; the global
auto-exposure EV limits remain unknown.

The visual review found readable daylight and seed-418 forest silhouettes,
visible warm firelight, and no clipped highlights in the retained views. The
seed-419 ForestShade anchor still has checkerboard-like ground and no forest;
both water anchors remain checkerboard-like without a shore; overcast lacks
rain particles and has weak framing; the generated prototype has a broad black
horizon and pale, sparse terrain. These are pre-existing scene/content limits,
not regressions from L01h. L01h and L01 are complete; the next eligible task is
L02a after integrating L01 to master.

## Proposed implementation stages

### 1. Capture a lighting baseline

Use the same UE version, renderer settings, camera, world seed, and exposure for
before/after comparisons. Start with seeds 418 and 419 and capture a compact
reference set: open Meadows in daylight, forest shade, coast and water, rain or
overcast, and a fire-lit camp at dusk/night. Record the map's current key light,
sky fill, shadow settings, fog, exposure, color grade, and representative
material values. Preserve these captures unchanged as the baseline.

### 2. Tune the lighting rig before replacing materials

Keep solid surfaces on Default Lit initially. Establish a stable exposure range
so time of day and dark interiors do not cause distracting automatic brightness
changes. Tune the existing directional light and sky fill together: the key
light should define readable form, while cooler ambient fill keeps shadowed
surfaces legible. Add distance atmosphere with the scene's existing fog/sky
systems. Use built-in post-process settings for restrained contrast, saturation,
color grading, and bloom; do not add a full-screen custom pass at this stage.

Evaluate daylight, overcast, dusk, and campfire scenes together. Keep fire and
lamps visibly warm at night without letting their highlights flatten nearby
materials or erase shadow shape. Do not finalize a global palette until the
side-by-side captures are reviewed.

#### Effects and post-processing treatment

Treat effects as three cooperating layers: scene lighting and fog establish
shape and depth; world-space weather and local effects give the environment
motion; post-processing ties the image together. The Valheim sources support
that broad combination, but they do not identify an exact effect recipe to
copy. Unreal's [Post Process Volumes](https://dev.epicgames.com/documentation/en-us/unreal-engine/add-post-process-volumes)
provide the proposed Kalmala controls.

| Effect layer | Kalmala proposal | Evaluation rule |
| --- | --- | --- |
| Sky, key light, and fill | Keep the directional sun/moon light as the form-defining key and the sky as broad, cooler fill. Tune shadows and indirect light before relying on grading to brighten them. | Preserve readable faces, terrain relief, and silhouettes in daylight, forest shade, dusk, and night. |
| Distance atmosphere | Tune the existing sky and fog first. Use height and distance falloff to separate far trees and hills while retaining nearby terrain contrast. Evaluate volumetric fog and light scattering only if the active map and renderer support them. | Check coastlines, forests, and camp lights at the baseline views; fog must not turn the whole scene into a flat veil. UE 5.8's [Fog Screen Space Scattering](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-8-release-notes) is experimental, so treat it as an isolated optional trial, not a required feature. |
| Weather and world-space effects | Prefer world-space cloud, rain, snow, mist, and ember effects where those systems already exist. Drive wet or snow surface response from an existing presentation signal, and make weather transitions gradual. | Keep particles and material changes out of the player's main sight lines where possible. Do not add screen droplets or full-screen weather overlays in the first pass. |
| Water and reflective surfaces | Let the water material respond to the scene sky and key light; tune reflections and specular highlights with the existing water setup. | Keep shoreline and terrain boundaries clear in both bright and dark captures; do not use bloom to create water reflections. |
| Fire and glow | Use actual local lights for campfire/torch illumination, with emissive material for the visible flame and a small bloom contribution for its brightest areas. Test other emissive details separately. | Bloom changes the camera image; it does not replace a light that illuminates nearby surfaces. Avoid halos that wash out wood, faces, or foliage. |
| Exposure and color grade | Start with a stable exposure range for the whole scene. Apply a restrained global grade, then consider a small number of local grades for authored interiors or distinctive misty areas. | Avoid exposure pumping while turning toward the sky, a fire, or a bright opening. Use broad blend regions and distinct priorities so local volumes transition smoothly. Preserve neutral reference materials when tuning cool shadows and warm highlights. |
| Lens and image effects | Keep lens flare, depth of field, vignette, film grain, chromatic aberration, and lens dirt disabled by default. Prototype an effect only when it solves a specific readability or presentation problem. | These effects can obscure traversal, enemies, UI-adjacent edges, or fine silhouettes. Check normal gameplay camera movement and close third-person framing before accepting any of them. |
| Custom post-process material | Do not use a scene-color pass for the base style. If built-in controls fail a documented visual requirement, prototype one isolated pass and measure its cost. | Keep it away from UI and text, and compare it against the same captures with the pass disabled. Unreal recommends built-in volume controls for common grading and lens effects because they are more efficient. |

Use one unbound volume as the scene-wide baseline. Add bounded volumes only for a
real location-specific treatment, with explicit priority and enough blend radius
to prevent a visible boundary. Volume blending changes presentation locally; it
must not become a gameplay or weather authority signal. Keep UI and text outside
any scene-color stylization.

Prototype in this order: stabilize exposure; balance the key light, sky fill,
and fog; set the global color grade; tune fire lighting and restrained bloom;
then try local volumes and optional lens effects one at a time. Capture each
change both enabled and disabled so it is clear which layer provides the benefit.

### 3. Build a small, tunable surface shader family

Replace the flat generated material expressions in controlled steps with
project-owned, parameterized parent materials and Material Instances. Reuse
Material Functions for common surface response, but keep separate parent
materials where needs differ: opaque ground/rock/wood, masked or two-sided
canopy, and water. Maintain one clear asset path and update the runtime
assignments together; do not construct production material graphs per frame.

Expose only useful artist controls: base tint, roughness, normal strength, broad
color variation, and mapping scale. Test world-space/triplanar mapping only on
generated meshes that lack suitable UVs. Keep the base response lit so direct
lights, shadows, and indirect fill continue to work.

#### L02a — Shared surface response interface — 2026-10-27

`M_KalmalaSurfaceBase` is the opaque, Default Lit parent for later surface
instances. Its `Kalmala Surface` group exposes a neutral-white `BaseTint`,
`Roughness` (0–1, default 0.92), `NormalStrength` (0–1, default 1),
`ColorVariation` (0–0.12, default 0.05), and `MappingScale` (0.0001–0.01
inverse centimeters, default 0.001). The parent consumes vertex color as its
input base color, so terrain blending remains available when L02b adopts it.

`Functions/MF_KalmalaSurfaceResponse` applies tint, centered low-cost gradient
noise in absolute world space, roughness passthrough, and a normalized blend
between the flat tangent normal and the supplied normal. At the default mapping
scale, variation is broad (roughly ten-meter features) and consistent across
mesh and terrain-patch boundaries. The parent is not assigned to existing
runtime surfaces in L02a; integration and visual acceptance remain in L02b–g.

#### L02b — Terrain and shore integration — 2026-10-27

`CreateWorldMaterials` now builds the existing grass/soil/rock and snow blend
as the terrain parent, then applies the shared surface-response function to its
final blended color. `MI_GeneratedTerrain` is assigned to terrain patches; the
cloth component deliberately keeps using the parent. `MI_GeneratedLakeShore`
uses `M_KalmalaSurfaceBase` with a tint that offsets the shore mesh's existing
procedural vertex color to preserve its former flat material color and roughness
0.78. The terrain constructor assigns these instances before the optional
`-KalmalaBiomeDebug` material override, so the debug path still replaces the
normal terrain presentation. Source and asset-generation checks passed; the
default-off lake-basin presentation and rendered surface response remain for
the L02g material-family gate.

#### L02c — Generated rock integration — 2026-10-27

The deterministic Meadow rock mesh keeps its geometry, normals, UVs, and
vertex-color values. `MI_GeneratedRock` uses `M_KalmalaSurfaceBase`, retains
roughness 0.95, and tints the mesh's existing vertex-color mean back to the
former flat `TerrainRockColor`; its per-vertex facet contrast and subtle shared
world variation are now visible. Only the `MeadowRocks` procedural component
uses this instance. Player presentation and campfire stone continue to use
`M_GeneratedRock`. Source, instance-generation, and presentation ownership
checks passed; rendered shape and baseline separation remain for L02g.

#### L02d — Generated wood and bark integration — 2026-10-27

`MI_GeneratedBark` uses the shared `M_KalmalaSurfaceBase` parent with a white
base tint and roughness 0.88. It preserves the procedural birch, aspen, leaf,
and cut-face vertex palettes while adding the shared lit response and broad
world-space variation. The Meadow patch trunk component and tree-resource
harvest meshes use this instance. `MI_GeneratedWood` uses the same parent at
roughness 0.88 with the prior brown bark tint; it is assigned to held-tool wood
shafts and the club/bow wooden pieces. The player head, non-wood tool heads and
non-tree gatherable silhouettes keep `M_GeneratedBark`. No runtime material
graph is created; mesh geometry and vertex colors are unchanged. Source,
instance-generation, and ownership checks passed; rendered response remains
for L02g.

#### L02e — Generated foliage integration — 2026-10-27

`M_KalmalaCanopyBase` reuses `MF_KalmalaSurfaceResponse` in a separate
two-sided, opaque parent. `MI_GeneratedCanopy` uses a white tint and roughness
0.82 and is assigned to the Meadow tree-canopy component. The current canopy is
a six-sided, open-bottom cone shell with vertex-colored leaf sides and apexes;
it has no cutout textures or partial-opacity vertices, so masked blending is not
needed. Two-sided shading lets the same solid faces render from below. The
source check found no alpha clipping or time-varying material input, but no
forest render was inspected; silhouette/shadow aliasing and TAA shimmer remain
for L02g.

#### L02f — Generated water integration — 2026-10-27

`MI_GeneratedWater` uses `M_KalmalaSurfaceBase` with white tint and roughness
0.28. It is assigned to sea/sea-level river and optional Shimmering Lakes
water components; existing ocean and lake vertex colors supply the blue/teal
palettes. The legacy flat `M_GeneratedWater` remains generated but unassigned.
`FKalmalaWaterSurfaceMesh` clipping, water levels, and separate shoreline geometry are unchanged.

No water render was captured or reviewed in this increment. L02g's repaired
seed-418 source-coast frame now shows blue water on both sides of the land strip;
the same fixed coordinates for seed 419 show water without shoreline framing.
The separate open-water anchor remains the valid broad water view. These fixed
wide views do not establish water reflections or highlights.

#### L02g — Final material-family gate — 2026-10-27

The first default captures requested a screenshot immediately after setting the
view target, before local terrain activation and auto exposure had settled. Their
gray/white outputs byte-matched the pre-L02 baseline and were not treated as
visual evidence. The non-Shipping capture path now moves the local pawn to every
requested sample so its terrain neighborhood stays active, then waits three
seconds for default and overcast views and five seconds for fire-lit dusk. This
fix is limited to the baseline-capture path. A fresh project mirror was built
with UE 5.8.2 using `Build.bat KalmalaEditor Win64 Development
-Project=<mirror>/Kalmala.uproject -WaitMutex -NoHotReload
-NoHotReloadFromIDE -MaxParallelActions=4 -DisableAdaptiveUnity`; all 55 actions
succeeded. `UnrealEditor-Cmd.exe <mirror>/Kalmala.uproject
-run=CreateWorldMaterials -unattended -nop4 -nosplash -nosound` exited 0 with
zero errors and one existing warning for the invalid material index on
`Default__KalmalaOceanSkiff:HullMesh`. `PresentationOwnership.ps1` passed for 16
project-owned assets and 34 procedural/UI source seams.

The capture helper reran the six fixed roles (Meadows daylight, forest shade,
open water, coast candidate, overcast/rain, and fire dusk) for seeds 418 and 419.
All twelve fresh 1024x768 captures passed its PNG, nonblank, and requested
seed/site/condition checks. They are under the temporary
`C:\Users\Ville\AppData\Local\Temp\klgfinalcaps` output folder; the preserved
`TestResults/LightingBaseline` files were not overwritten.

| Surface | Runtime instance and parent | Tint | Roughness | Runtime/render review |
| --- | --- | --- | ---: | --- |
| Terrain | `MI_GeneratedTerrain` / `M_GeneratedTerrain` | white | 0.92 | Generated patches use the instance; default and daylight views show the vertex-color ground blend. |
| Lake shore | `MI_GeneratedLakeShore` / `M_KalmalaSurfaceBase` | (0.625, 0.657143, 0.522388) | 0.78 | Loaded instance and component reference verified; the default disabled-basin policy produced zero lake-shore triangles, so its rendered surface is unavailable. |
| Rock | `MI_GeneratedRock` / `M_KalmalaSurfaceBase` | (0.903226, 0.913793, 0.904494) | 0.95 | Generated rock component retains its vertex-color palette; facets are visible in the meadow captures. |
| Bark | `MI_GeneratedBark` / `M_KalmalaSurfaceBase` | white | 0.88 | Generated trunks and tree harvest meshes use the instance; the forest frame shows brown bark. |
| Wood | `MI_GeneratedWood` / `M_KalmalaSurfaceBase` | (0.20, 0.13, 0.09) | 0.88 | Authored held-tool shafts and wooden club/bow sections use the instance. |
| Canopy | `MI_GeneratedCanopy` / `M_KalmalaCanopyBase` | white | 0.82 | Generated canopies retain green species vertex colors; two-sided opaque leaf shells and their shadows are visible. |
| Water | `MI_GeneratedWater` / `M_KalmalaSurfaceBase` | white | 0.28 | Sea/river section reports the instance and its blue vertex color; seed-418 coast and open-water frames show blue surfaces. |

The runtime audit read these material instance parameters after loading. It also
reported `MI_GeneratedTerrain`, `MI_GeneratedWater`, and `MI_GeneratedCanopy`
on generated sections, with canopy vertex colors from the existing conifer and
aspen palettes. Source tracing and ownership checks confirm generated rock,
bark, lake-shore, authored wood and water assignments. The optional lake basin
remains disabled, and the runtime audit reports zero lake water/shoreline
triangles at the default Meadow site; no visual lake-shore response is claimed.
The fixed seed-418 coast view contains visible water and land, while seed 419 at
the same coordinates is water-only. The forest shade and daylight seed-418
frames show green solid leaf silhouettes, brown trunks, terrain and shadows.
Seed-419's fixed forest coordinates face open water, and its overcast view also
has little land framing; these are fixed-anchor/seed limitations. The overcast
captures show sky, ground and water but no visible rain particles. Both dusk
captures show terrain, the test hearth silhouette and warm ground spill; they do
not contain a modeled flame. Black world-horizon areas remain visible. The water
reads blue, but reflections/highlights were not established in the fixed wide shots.

For temporal review, capture-only instrumentation in the temporary mirror saved
16 frames for each seed-418 daylight and forest-shade camera while translating
the camera by 0.5 cm every 0.25 seconds after the three-second scene warmup. The
project default was `r.AntiAliasingMethod=4` (TSR); a second pass explicitly
forced method 2 (TAA). All four sequences produced 16/16 frames. Reviewed frames
showed stable hard-edged leaf silhouettes and shadows with no obvious temporal
foliage shimmer during this short subpixel pan. This is a visual spot check, not
a performance measurement or broader motion/coverage test. The temporary pan
instrumentation was not added to the project.

**Final L02 status:** milestone-final build, material commandlet, ownership
validator, all fixed-scene captures, runtime assignment/value audit, and the
daylight/shade temporal review passed. Outstanding visual limits are the absent
default lake-shore geometry, the seed-419 forest framing, the lack of visible
rain particles and a fire mesh, and water reflections/highlights not established
in the fixed wide views. No unrelated gameplay or material system was changed.
#### L03a — isolated direct-light comparison fixture — 2026-10-27

Added `UCreateLightingComparisonFixtureCommandlet` as a test-only generator for
`/Game/Kalmala/Tests/Lighting/L_LightingComparison`, one three-band
`TP_L03SoftThreeBand` profile, and paired smooth Default Lit / Substrate Toon
materials under `/Game/Kalmala/Tests/Lighting`. The board uses paired columns
(Default Lit left, candidate right) and this row order: ground, rock, wood,
canopy, creature, construction. The capture runner targets L01's five seed-418
views: Meadows daylight, Forest shade, OpenWater, OvercastRain, and FireDusk.
The two Meadows conditions share one board, so four physical placements cover
five view contexts.

The candidate diffuse ramp uses three broad levels (0.32, 0.58, 0.92) with
cubic transitions spanning 0.08 of the ramp around the two boundaries. Diffuse
and specular indirect scales remain at 1.0. The ramp does not fold shadow
transmittance into its band lookup, leaving the engine's separate attenuation
term active. Toon Profile shadow extinction keeps UE's default value of 10;
no hatching texture is assigned. The project's DefaultEngine configuration is
not changed. The capture helper accepts a map override, and
`Scripts/Tests/Presentation/LightingComparison.ps1` enables Substrate only for
its capture processes using an engine-config command-line override. Run the
asset commandlet with the same override in the compiled mirror.

The samples use UE BasicShapes meshes as class proxies, with representative
palette/roughness values; they do not use the production terrain, wildlife,
foliage, or buildable meshes. This fixture can compare the shading response and
framing but cannot establish final mesh silhouettes, foliage stability, GPU
cost, or production platform support. The commandlet and capture scripts passed
source inspection and PowerShell parsing. A rebuilt Editor was not run, so the
generated packages and rendered captures are still pending before visual
review; the Substrate candidate is experimental and remains test-only.

#### L03b — UE 5.8 Substrate NPR support audit — 2026-10-27

UE 5.8 exposes UMaterialExpressionSubstrateToonBSDF and UToonProfile, both
marked experimental in the installed engine headers. Epic's [UE 5.8 release
notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes)
describe Toon shading as an experimental path built on the Substrate Blendable
GBuffer. The [Substrate overview](https://dev.epicgames.com/documentation/unreal-engine/overview-of-substrate-materials-in-unreal-engine)
states existing projects upgrading to UE 5.7 or later stay on the non-Substrate
path unless they explicitly opt in. RendererSettings.h maps bEnableSubstrate
to r.Substrate and requires an editor restart; its setting notes say Blendable
GBuffer is supported on all platforms while Adaptive GBuffer is supported only
on some.

Kalmala's .uproject targets UE 5.8, but Config/DefaultEngine.ini contains no
RendererSettings/Substrate opt-in. Kalmala.Target.cs, KalmalaEditor.Target.cs,
and KalmalaServer.Target.cs declare Game, Editor, and Server target types
without a SupportedPlatforms list. L01's live capture record observed
D3D12/SM5 on an RTX 5090; that single host is not a platform support matrix.
The inspected project configuration therefore does not establish Substrate as
a production renderer path or certify it for the project's build targets.

The L03a commandlet authors a test-only Toon Profile and Toon BSDF material,
and the capture runner enables Substrate through a per-process command-line
override. This keeps the experiment isolated from production configuration and
assignments. No Editor build, commandlet asset generation, material compilation,
or rendered capture was run, so source/API availability is not proof that the
candidate compiles or renders correctly on project targets. Keep the Toon
candidate test-only; do not enable Substrate in project defaults or add a
production dependency. At the time of this support audit, L03c still needed
rendered integration, silhouette, stability, and cost evidence before L03d could
make a review recommendation.

#### L03c — focused rendered integration evidence — 2026-10-27

The user authorized a focused UE 5.8 Editor build and L03 captures outside the
normal L06 verification gate. This was a targeted L03c evidence run, not
milestone-final verification. The current KalmalaEditor build passed with four
incremental UBT actions after adding the test-only GrayTextureCube skylight
hook; an earlier clean build in this session compiled the fixture commandlet. The isolated fixture map, profile, and materials generated
successfully in the short mirror `C:\Users\Ville\AppData\Local\Temp\l03c\Kalmala.uproject`. The
incremental command was `Build.bat KalmalaEditor Win64 Development
C:\Users\Ville\AppData\Local\Temp\l03c\Kalmala.uproject -WaitMutex
-NoHotReloadFromIDE`; UnrealBuildTool had normal access to
`%LOCALAPPDATA%\UnrealBuildTool`. Intermediate temporal PNGs and capture logs
were retained in `C:\Users\Ville\AppData\Local\Temp\l03c\capture-intermediate`.

Five fixed L01 site anchors were captured at seed 418: MeadowsDaylight,
ForestShade, OpenWater, OvercastRain, and FireDusk. The test process rendered
five paired 8-frame sequences at 0.5-second intervals, then ten isolated
DefaultLit/Toon views with 120 GPU capture frames each after a five-second
settle. Each image is 1024×768. UE logged D3D12 feature level SM5 on an NVIDIA
GeForce RTX 5090. Substrate was enabled only by the test process. The L03 fixture
keeps each anchor, site condition, exposure, FOV, and camera direction; to fit
the board, non-FireDusk captures shorten camera distance/height from the L01
baseline 2400/1200 cm to 1000/500 cm. FireDusk retains 500/250 cm. These are
fixture-framed views, not identical L01 camera poses.

The FireDusk scene uses the transient production campfire spawned at the fixed
anchor, with its ordinary 600 cm light radius. No second point light was added.
All six sample classes and both variants are within the radius, with center
distances of 125–474 cm. The capture pawn is hidden only when the L03 fixture
flag is present. The revised FireDusk, OpenWater, and OvercastRain images contain
no centered pawn. The board still uses UE BasicShapes proxies rather than game
meshes.

| Check | Captured finding | Limit |
| --- | --- | --- |
| Cast shadows | Both variants show the sample proxies casting and receiving visible shadows in the fixed captures. | Visual check on proxy meshes; not a production mesh review. |
| Skylight fill | A test-only ForestShade capture hid and zeroed the tagged sun, then set the movable fixture skylight to Engine `GrayTextureCube` at intensity 1.0; both isolated variants rendered all six proxies. | Toon crop means were lower on every class. This measures specified-cubemap skylight fill only; world-bounce/GI remains untested. The original captured-scene sky source was black in the sun-off diagnostic. |
| Camp-light response | The actual FireDusk campfire is present. Projected FireDusk sample-crop mean 8-bit luma was lower for Toon for all six classes, including Ground, Wood, and Creature where Toon samples are closer to the light. | The paired samples sit at different positions (±65 cm) and the crop includes a small amount of scene edge; this is evidence of an uneven/dim candidate, not a calibrated co-located material comparison. |
| Silhouette readability | Proxy shapes remain identifiable in the captures, and their shadows remain visible. | Several Toon samples are very dark; FireDusk Construction averages 7.973 luma and loses surface detail. Production meshes were not tested. |
| Temporal stability | Adjacent-frame sample crops were measured for 8 frames at 0.5-second intervals. | Crops include some surrounding pixels; weather, sky, and fire may animate. No pass threshold was defined. This is a short proxy-pixel observation, not a broad temporal-stability certification. |
| Per-view GPU cost | UE `GPUTime` total-frame means were recorded from 120 frames per isolated view. | These are full 1024×768 view costs on one RTX 5090 system, including scene and post-process work; they are not material-only attribution or a target-hardware performance claim. |

ForestShade sky-only projected-crop mean luma (DefaultLit → Toon, 8-bit sRGB screenshot):

| Surface | DefaultLit | Toon |
| --- | ---: | ---: |
| Ground | 155.44 | 128.14 |
| Rock | 103.24 | 63.80 |
| Wood | 107.52 | 47.55 |
| Canopy | 99.78 | 58.54 |
| Creature | 71.36 | 59.18 |
| Construction | 147.14 | 85.85 |

The direct sun was off (logged intensity 0.000); the tagged skylight remained
at 1.000 and used Engine `/Engine/EngineResources/GrayTextureCube` through
`SLS_SpecifiedCubemap`. The first sun-off diagnostic used the fixture captured-
scene source and rendered black because the isolated map has no sky dome or
environment-radiance source; the measured pair above uses the neutral engine
cube. The Toon candidate therefore receives controlled skylight fill, although
all six proxy-crop means are lower than DefaultLit. This is not a world-bounce
or production-environment GI test, and the paired columns are 130 cm apart.
Images and per-sample crop values are in
`TestResults/L03c/ForestShade_SkyOnly_DefaultLit.png`,
`TestResults/L03c/ForestShade_SkyOnly_Toon.png`, and
`TestResults/L03c/SkyOnlyFill.csv`.

FireDusk sample center-crop luma (DefaultLit → Toon, 8-bit sRGB screenshot):

| Surface | Center distance from fire (cm), DefaultLit → Toon | Mean luma, DefaultLit → Toon |
| --- | ---: | ---: |
| Ground | 255 → 125 | 111.574 → 37.859 |
| Rock | 125 → 255 | 103.694 → 39.050 |
| Wood | 324.1 → 235.8 | 64.171 → 39.073 |
| Canopy | 235.8 → 324.1 | 90.891 → 42.610 |
| Creature | 474.4 → 419.1 | 54.125 → 41.892 |
| Construction | 419.1 → 474.4 | 72.190 → 7.973 |

The engine's UE 5.8 `SubstrateToonBSDF.ush` computes
`Sample.DiffuseColor = DiffuseColor * DiffuseRamp`; the equal RGB ramp is a
per-channel scalar multiplier and does not replace the authored base hue. Mean
pixel saturation in the six FireDusk center crops was higher for Toon, while
luminance was lower. The captured concern is the candidate's dark direct-light
response/readability, not a gray ramp overwriting BaseColor. No profile tuning
was made in this evidence run.

Sample-region temporal mean adjacent-frame RGB change (0–255 scale, averaged
over five views for each proxy and variant):

| Proxy | DefaultLit | Toon |
| --- | ---: | ---: |
| Ground | 1.9463 | 6.4165 |
| Rock | 3.0310 | 3.2649 |
| Wood | 5.4006 | 2.5743 |
| Canopy | 3.5663 | 2.9890 |
| Creature | 3.4531 | 3.1503 |
| Construction | 5.6432 | 3.5113 |

The per-view crop values and maxima are in `TestResults/L03c/TemporalSampleRegions.csv`.
Whole-frame adjacent-frame mean RGB changes were MeadowsDaylight 1.5661,
ForestShade 0.1840, OpenWater 2.0281, OvercastRain 0.1866, and FireDusk 0.1185;
these include background and scene animation and are retained only as context.

Per-view UE `GPUTime` mean (ms, 120 positive frame samples per capture):

| View | DefaultLit | Toon |
| --- | ---: | ---: |
| MeadowsDaylight | 0.7093 | 0.7021 |
| ForestShade | 0.6980 | 0.6883 |
| OpenWater | 0.6934 | 0.6942 |
| OvercastRain | 0.8016 | 0.8258 |
| FireDusk | 0.8752 | 0.8739 |

The five-view capture images, ten raw GPU CSVs, and four capture-summary CSVs
are in `TestResults/L03c/`, along with the two sky-only frames and
`SkyOnlyFill.csv`;
the 40 temporal-frame PNGs and capture logs remain in the temporary capture
workspace. Exact GPU min/max values, FireDusk crop RGB/saturation, full-frame
temporal metrics, sky-only crop values, and per-sample temporal metrics are
preserved in the CSVs. UE `GPU/Unaccounted` is a separate queue remainder and
is not the total GPU frame metric.

L03c evidence collection is complete, but the Substrate Toon candidate did not
meet the local-light brightness/readability check. The controlled GrayTextureCube
test confirms the Toon proxies receive skylight fill, although all six projected
crop means are lower than DefaultLit; world-bounce/GI remains untested. The
comparison uses BasicShapes proxies and one GPU. Default Lit remains the
production path; no production renderer setting or material assignment changed.
L03d was reopened at this point to review the captured results; its final
recommendation is recorded in the following section.

#### L03d — final review recommendation — 2026-10-27

**Recommendation:** Keep Default Lit as the production shading path. Do not
adopt the test-only Substrate Toon candidate for current production materials.
The captured response does not preserve enough local-light readability to
justify a broad style change, and it shows no consistent GPU-time advantage.

In the FireDusk comparison, all six Toon sample crops had lower mean luma than
their Default Lit counterparts. The most severe result was Construction at
72.190 → 7.973 (8-bit sRGB luma); the candidate also appeared much darker in the
paired image. Other classes ranged from Ground 111.574 → 37.859 to Canopy
90.891 → 42.610. The actual transient 600 cm campfire illuminated the fixture;
no duplicate light was added. Both variants retained visible proxy shadows, but
some Toon samples lost surface detail. The paired samples occupy different
positions and their crops include a small amount of scene edge, so these are
comparative fixture observations, not a co-located material calibration.

The controlled ForestShade GrayTextureCube test confirms the Toon candidate
receives skylight fill: both variants rendered all six proxies with the direct
sun off. However, all six Toon crop means were lower (Ground 155.44 → 128.14;
Rock 103.24 → 63.80; Wood 107.52 → 47.55; Canopy 99.78 → 58.54; Creature
71.36 → 59.18; Construction 147.14 → 85.85). This is specified-cubemap fill
only; it does not establish world-bounce/GI or production-environment response.

The short eight-frame adjacent-crop observations are mixed rather than a
stability pass: Toon deltas rose for Ground (1.9463 → 6.4165) and slightly for
Rock (3.0310 → 3.2649), while they fell for Wood, Canopy, Creature, and
Construction. Crops include some scene pixels, the test used only eight frames
at 0.5-second intervals, and no pass threshold was defined. Do not infer broad
temporal stability from this proxy sample.

Per-view mean UE `GPUTime` for 120 frames was Default Lit → Toon, in ms:
MeadowsDaylight 0.7093 → 0.7021; ForestShade 0.6980 → 0.6883; OpenWater
0.6934 → 0.6942; OvercastRain 0.8016 → 0.8258; FireDusk 0.8752 → 0.8739.
Differences range from Toon 0.0097 ms lower to 0.0242 ms higher, with no
consistent win. These are total 1024×768 view timings, including the scene and
post-process work, on one RTX 5090; they do not isolate material cost or qualify
supported release hardware.

The fixture uses UE BasicShapes proxies, shortened camera distance for four
views, paired materials at different positions, and a test-process-only
Substrate opt-in. It did not review production meshes or certify the project's
target platforms. UE 5.8 Toon remains experimental; configured production
renderer support, world-bounce/GI response, and broader target-hardware behavior
remain unverified. Keep the candidate isolated and unadopted. No production
renderer setting or material assignment changed. L03's review recommendation is
complete on this branch; integrated lighting acceptance and full verification
remain for L06.

As a separate prototype, compare smooth Default Lit against a restrained two- or
three-band direct-light treatment with soft transitions. UE 5.8 documents an
[experimental Substrate NPR shading path](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-8-release-notes)
with ramp-based diffuse and specular controls; inspect the project's
renderer/material configuration and target
platform support before treating it as a candidate. Verify any candidate retains
shadowing, indirect fill, and local camp-light response. If it requires an
unsupported renderer path, custom engine shading model, or global post-process
approximation, stop at the prototype gate and measure visual quality, platform
support, and cost before choosing that route. Experimental support is a research
option, not a production dependency.

### 4. Add optional environment layers

After the base daylight and night scenes read well, prototype subtle wet-surface
darkening, snow coverage, and moss/lichen variation as material parameters.
Drive these only from weather or surface information already available to the
client for presentation. Keep them cosmetic: do not add gameplay queries, save
fields, RPCs, or replicated state for shader effects. Add a separate design
decision if a required visual signal is not already available.

#### Existing weather and environment inputs (L04a, source audit 2026-10-27)

This is a map of the current inputs and effects, not a new weather contract.
Weather remains server-owned; clients may use the existing replicated values
for local presentation.

| Plan effect | Existing input and path | Support and limits |
| --- | --- | --- |
| Rain and wet-surface response | `FKalmalaWeatherState` exposes normalized `PrecipitationIntensity` and `WindStrength`, plus derived `StormIntensity`, through the replicated `AKalmalaWorldGenerationGameState`. `UKalmalaAmbientAudioSubsystem` already fades the local `RainBed` and `WindBed` from those values. | Replicated precipitation is a valid cosmetic rain/wet-material input. The authoritative interaction-grid surface moisture and per-player wetness remain gameplay state, not shared material inputs. At this audit, current wood/rock materials had no weather-wet response; L04b's implementation is recorded below. The weather state has no precipitation-kind field, so rain cannot be distinguished from snow by type. |
| Snow and biome coverage | `FKalmalaWorldGenerationConfig` is replicated; clients can derive deterministic biome weights through `FKalmalaRegionalGeneration`. `AKalmalaGeneratedTerrainPatch` writes a cold-biome snow mask to terrain vertex alpha through `FKalmalaTerrainGroundBlend`; `CreateWorldMaterialsCommandlet` consumes that alpha in `M_GeneratedTerrain`. | Biome/height-driven snow coverage already exists on generated terrain. It is not driven by snowfall. Generated rock, bark, canopy, shore, and water do not receive this snow mask, so coverage on those surfaces is unsupported by the current material inputs. No snow particle system exists. |
| Moss and lichen | The regional sampler can identify `MossyMire`, and terrain generation already samples humidity. | No moss/lichen weight is carried to the current material: terrain RGB stores grass/soil/rock weights, alpha stores snow coverage, and humidity only affects the soil blend. There is no distinct moss/lichen layer input on terrain or other generated surfaces; treat those combinations as N/A unless an existing presentation signal is made available by the planned material integration. |
| Sky and weather color | Setup-PrototypeEnvironment.py creates the directional sun, SkyAtmosphere, and movable real-time-capture skylight. | No cloud actor is created and no weather-linked sky/color treatment is wired; cloud and weather-color treatments are N/A for this increment. Do not add a new color pipeline or sky model. |
| Local fog | Replicated FogIntensity and world-generation config feed the local player ThunderMountains weight. KalmalaWeatherActivitySubsystem spawns a nonreplicated AExponentialHeightFog during valid weather intervals and probes the FogIntensity multiplied by that weight once per second. | Existing regional fog now follows FogIntensity independently of rain/wind storm intensity and fades gradually. Density, opacity, start distance, and cool in-scattering remain bounded; the effect is client-local and cosmetic. |
| World-space rain, snow, mist, or embers | Existing local rain/wind audio and FogIntensity-driven highland fog. | RainBed is played with CreateSound2D, not as a particle effect. No runtime Niagara/ParticleSystem path or authored rain, snow, mist, or ember particle asset was found in the source/content inventory. These world-space particle effects are N/A for tuning existing systems. |

Source anchors:
- `Source/KalmalaWorld/Public/KalmalaWeatherState.h` and `Source/KalmalaWorld/Public/KalmalaWeatherCycle.h`
- `Source/KalmalaWorld/Public/KalmalaWorldGenerationGameState.h` and `Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp`
- `Source/KalmalaUI/Private/KalmalaWeatherActivitySubsystem.cpp` and `Source/KalmalaUI/Private/KalmalaAmbientAudioSubsystem.cpp`
- `Source/KalmalaWorld/Public/KalmalaTerrainGroundBlend.h` and `Source/KalmalaWorld/Private/KalmalaGeneratedTerrainPatch.cpp`
- `Source/KalmalaWorld/Public/KalmalaRegionalGeneration.h` and `Source/KalmalaWorld/Private/KalmalaRegionalGeneration.cpp`
- `Source/KalmalaEditor/Private/CreateWorldMaterialsCommandlet.cpp`
- `Source/KalmalaGameplay/Private/KalmalaInteractionGrid.cpp` and `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp`
- `Scripts/Setup-PrototypeEnvironment.py`.

#### L04b — Subtle local precipitation response on selected materials (2026-10-27)

The generated shared parent now reads `PrecipitationIntensity` from the local
world's `MPC_KalmalaWeather` collection and multiplies it by the opt-in
`WeatherWetResponse` material-instance parameter. At full response, the graph
reduces linear base color by at most 8% and roughness by at most 0.12. The
generator sets the opt-in to one only for `MI_GeneratedRock`, `MI_GeneratedBark`,
and `MI_GeneratedWood`; their existing dry tints and roughness values remain
unchanged. Terrain, shore, canopy, water, player, and campfire materials remain
outside this effect.

`UKalmalaWeatherActivitySubsystem` reads the existing replicated precipitation
value only while its valid weather interval is active, clamps it, and
interpolates the world-local collection parameter toward it each frame. Its
0.75 interpolation rate smooths both onset and release. The response is a
cosmetic material input only: it does not read authoritative ground moisture or
player `Wet`, accumulate surface moisture, or add gameplay, save, RPC, or
replicated-state behavior. Clients on a listen server use the same local path;
dedicated servers have no local-player presenter.

| Surface | L04b status | Existing support and limit |
| --- | --- | --- |
| Generated Meadow rocks | Supported | `MI_GeneratedRock` opts in to the shared parent's precipitation response; player and campfire rock materials do not. |
| Generated trunks and tree harvest bark | Supported | `MI_GeneratedBark` opts in while retaining species and cut-face vertex colors. |
| Held-tool wood | Supported | `MI_GeneratedWood` opts in for the existing wood shafts and pieces. |
| Terrain and lake shore | N/A | Terrain uses its separate layered parent; lake shore keeps `WeatherWetResponse` at zero. |
| Canopy, water, player, campfire, non-tree gatherables, and construction materials | N/A | They do not use an opted-in selected instance. |

The replicated weather state has no precipitation-kind field, so the existing
precipitation signal cannot distinguish rain from snow. Every active
precipitation value therefore drives this generic wet-looking response; it is
not accumulated moisture and begins to dry visually as the value fades. The
material generator refreshes the shared parent and only the three opted-in
instances; no existing terrain or shore material contract is broadened. Rendered
appearance remains for the L02g and L04g presentation gates.

Source anchors: `KalmalaWeatherState.h`, `KalmalaWorldGenerationGameState.h/.cpp`,
`KalmalaWeatherActivitySubsystem.h/.cpp`, and
`CreateWorldMaterialsCommandlet.cpp`.

#### L04c — Existing snow coverage and surface limits (source review 2026-10-27)

The L04c terrain requirement is already met by the generated-terrain path. Clients receive the replicated world-generation config and can derive deterministic regional biome weights and final height. Terrain patch generation supplies the FreezingTundra and ThunderMountains weights, final height, and surface-normal Z for every terrain vertex. The existing mask combines tundra weight with mountain weight gated by the configured highland height transition, attenuates coverage by surface-normal Z, and clamps it to [0, 1].

Generated terrain stores that mask in vertex-color alpha. The generated terrain material reads alpha and blends the existing grass, soil, and rock ground color toward its pale snow color. This coverage is biome/height-driven, not driven by snowfall or precipitation. The eligible generated-terrain surface therefore already has the existing client-derived signal and snow blend required by L04c; this increment adds no signal or material contract.

| Surface | L04c status | Existing support |
| --- | --- | --- |
| Generated terrain | Supported; coverage already present | Replicated generation config feeds deterministic biome/height sampling; terrain vertex alpha carries the mask consumed by the generated terrain material. |
| Generated rocks | N/A | The generated rock material uses constant base color and roughness; no snow mask is supplied. |
| Generated trunks and bark | N/A | The generated bark material uses constant base color and roughness; no snow mask is supplied. |
| Generated canopy | N/A | The generated canopy material uses constant base color and roughness; no snow mask is supplied. |
| Generated lake shore | N/A | The generated lake-shore material uses constant base color and roughness; no snow mask is supplied. |
| Generated water | N/A | The generated water material uses constant base color and roughness; no snow mask is supplied. |

No existing snow/biome signal for those separate non-terrain materials was proven in the inspected source. Keep them N/A unless a suitable existing presentation input is later established; do not infer coverage from a global weather or biome state.

Source anchors: KalmalaWorldGenerationGameState.h/.cpp (replicated config), KalmalaRegionalGeneration.h, KalmalaTerrainGroundBlend.h, KalmalaGeneratedTerrainPatch.cpp, and CreateWorldMaterialsCommandlet.cpp.

#### L04d — Moss and lichen support limits (source review 2026-10-27)

Regional generation can identify MossyMire and sample humidity, but neither input currently reaches a moss/lichen material channel. On generated terrain, humidity is used to form the existing grass/soil/rock weights; the terrain ground-blend helper receives tundra and mountain weights for snow coverage, and packs only grass, soil, and rock weights into vertex RGB plus snow coverage into alpha. MossyMire weight is not passed to the material. The current terrain material consumes those existing channels and has no moss/lichen layer.

The separate generated rock, bark/trunk, canopy, lake-shore, and water materials have no biome, humidity, or moss mask. Their current material path is constant-color/roughness or fixed presentation color, with no moss/lichen input. The bounded shared surface controls recorded above do not expose a biome mask or moss layer. Although biome and climate values are available during deterministic world generation, no suitable client presentation signal is currently routed to a moss-capable material.

| Surface | L04d status | Existing support |
| --- | --- | --- |
| Generated terrain | N/A | Vertex RGB stores grass/soil/rock weights and alpha stores snow coverage; humidity only affects the soil blend and no MossyMire/moss channel reaches the material. |
| Generated rocks | N/A | The generated rock material has no biome or moss mask. |
| Generated trunks and bark | N/A | The generated bark material has no biome or moss mask; wood/bark material integration remains L02d. |
| Generated canopy | N/A | The generated canopy material has no biome or moss mask. |
| Generated lake shore | N/A | The lake-shore material has no biome or moss mask. |
| Generated water | N/A | Water has no moss/lichen material input and is not a suitable moss surface. |
| Authored wood/building surfaces | N/A | No existing client biome/surface signal reaches these materials; shared wood/bark integration remains L02d. |

L04d is complete as N/A for the currently supported surface set. Do not add a moss/lichen weight, vertex channel, parameter, or material layer in this increment. Reconsider only if a suitable existing presentation input is established by a later material integration.

Source anchors: KalmalaBiomeClassifier.h, KalmalaRegionalGeneration.h, KalmalaWorldFieldSampler.h, KalmalaGeneratedTerrainPatch.cpp, KalmalaTerrainGroundBlend.h, and CreateWorldMaterialsCommandlet.cpp.

#### L04e — Existing atmospheric path tuning (source review 2026-10-27)

The deterministic weather cycle publishes normalized FogIntensity separately from the precipitation-times-wind StormIntensity. The existing local-player fog path now uses FogIntensity multiplied by the local continuous ThunderMountains biome weight during a valid weather interval. This allows the weather cycle's foggy calm outcomes to use the same highland atmosphere even without rain or wind; storms with low FogIntensity remain visually clear. The one-second target probe and smooth local interpolation remain, as do the existing maximum density (0.005), maximum opacity (0.24), 2500 cm start distance, and muted cool in-scattering color.

The fog actor remains transient, nonreplicated, and absent on dedicated servers. No gameplay, save, RPC, or replicated-state contract changed.

Setup-PrototypeEnvironment.py creates the directional sun, SkyAtmosphere, and movable real-time-capture skylight. Source review found no cloud actor and no weather-linked sky-color treatment; cloud and weather-color treatments are N/A for the current pipeline. No new sky model, cloud actor, or color pipeline was added.

Source anchors: KalmalaWeatherCycle.h, KalmalaWeatherState.h, KalmalaWorldGenerationGameState.h/.cpp, KalmalaWeatherActivitySubsystem.h/.cpp, and Scripts/Setup-PrototypeEnvironment.py.

#### L04f — World-space weather effects inventory (2026-10-27)

The source/content inventory contains no existing world-space rain, snow, mist, or ember particle system. Production-source search found no Niagara or legacy ParticleSystem emitter API or weather-specific particle path. RainBed is an audio asset played with CreateSound2D; the existing cold-biome snow is a terrain material blend, and the L04e regional AExponentialHeightFog is a local atmospheric effect rather than a world-space particle system.

| Effect | L04f status | Existing support |
| --- | --- | --- |
| Rain | N/A | RainBed is local 2D audio; there is no rain emitter path or particle asset. |
| Snow | N/A | Existing terrain snow is biome/height material coverage, not falling-snow particles; no snow emitter path or asset exists. |
| Mist | N/A | L04e's local highland fog is non-particle; no world-space mist emitter path or asset exists. |
| Embers | N/A | No ember emitter path or particle asset exists in the current source/content inventory. |

All L04f world-space particle combinations are N/A because there is no existing system or asset to tune. No particle system, screen-space overlay, asset, or dependency was added.

Source anchors: KalmalaAmbientAudioSubsystem.cpp, KalmalaWeatherActivitySubsystem.cpp, KalmalaTerrainGroundBlend.h, KalmalaGeneratedTerrainPatch.cpp, CreateWorldMaterialsCommandlet.cpp, and the current Content asset inventory.

#### L04g — Environment-effects visual gate review (2026-10-27; complete)

The fixed L01h captures predate L04b and could not verify the wet-material
response, so the non-Shipping baseline fixture was extended with Dry,
RainOnly, OvercastOnly, and WeatherTransition conditions. OvercastRain and
FireDusk remain available. The close MeadowsWetSurface label selects the same
deterministic camera pose for comparing dry and wet generated surfaces.
WeatherTransition records four paths in one run: _OnsetStart.png, _OnsetEnd.png,
_ReleaseStart.png, and _ReleaseEnd.png. It starts from a dry state, applies
precipitation/fog after the dry-start frame, then clears weather after the
wet-onset frame. All fixture behavior is guarded by the non-Shipping
-KalmalaBiomeBaselineCapture option.

Fresh seed-418 renders were written outside the checkout under
C:/Users/Ville/AppData/Local/Temp/klc/TestResults/LightingBaseline/. The
runner validated each requested image as a nonblank 1024x768 PNG and checked
the runtime capture log. The set includes a dry daylight control and
rain-only close-up at Meadows, a separate overcast-only frame at
ThunderMountainsStorm, the combined overcast/rain views, a Meadows fire-lit
frame, and onset/release sequences at both Meadow wet-surface and highland
weather views.

Visual review found the rain response subtle at the planned maximum of 8%
linear base-color darkening and 0.12 roughness reduction. It is visible in the
same-camera dry/wet close-up without excessive texture noise or silhouette
change. Capture diagnostics show the local material response rising from 0.00
to 0.75, then fading to 0.05 by the release-end frame. At ThunderMountainsStorm,
the fog target reaches 0.71; current coverage advances in smooth increments to
0.65 and releases to 0.05 before the final frame. The captures show no geometry
popping or abrupt volume boundary. The fixed highland scene remains sparse with
a broad horizon band, which limits composition review but does not prevent
review of the local fog transition. The fire-lit frame shows warm ground spill
around the transient hearth; the project has no modeled flame asset.

The capture fixture changes only non-Shipping review behavior. Production
weather remains server-authored through existing replicated state; the wet
material collection and highland fog remain local cosmetic presentation. No
gameplay simulation, save, RPC, or replicated-state contract changed. The
missing world-space rain, snow, mist, and ember systems remain N/A per L04f.
L04g passes for the currently supported effects; full integrated lighting
verification remains the L06 gate.

Source anchors: Scripts/Tests/World/BiomeBaselineCapture.ps1,
KalmalaGameMode.cpp, and KalmalaWeatherActivitySubsystem.cpp. Capture
outputs and logs remain in the temporary review folder above; they are not
tracked runtime assets.


### 5. Tune and accept the complete look

Compare smooth and banded variants on terrain, rocks, bark, canopy, creatures,
construction, and water. Review both bright and dark environments, material
separation at a distance, shadow readability, foliage shimmer, TAA stability,
and transitions between weather states. Toggle each optional effect separately
while reviewing the five fixed-camera scenes, the normal third-person camera,
and the UI. Record the effect's visual benefit and frame cost against the
baseline. Keep custom post-process work to a single measured pass only if
built-in grading, fog, and material controls cannot produce the accepted result.
Test performance on the project's minimum supported hardware before broad
material rollout.

## Hardware ray tracing and RTX quality levels

Treat hardware ray tracing as an optional way to improve the same Kalmala look,
not as a separate lighting style or a requirement for readable gameplay. Public
Valheim sources describe its broad use of modern lighting and post effects, but
do not establish a Valheim RTX configuration; the tiers below are Unreal Engine
options for Kalmala.

In UE 5.8, Lumen supports software ray tracing through signed distance fields
and hardware ray tracing against scene triangles. On Windows, Lumen hardware
ray tracing requires DirectX 12, Shader Model 6, and a supported GPU. Epic lists
NVIDIA RTX 20-series or newer and AMD RX 6000-series or newer as supported
families. Hardware ray tracing is therefore not NVIDIA-exclusive, even though
"RTX" is NVIDIA branding. Verify the project's renderer and exact supported
device matrix before promising a minimum card. ([Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine),
[Hardware Ray Tracing](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-ray-tracing-in-unreal-engine))

### L05a — Checked-in renderer support audit (2026-10-27)

`Kalmala.uproject` targets UE 5.8 and the project brief targets Windows PC
first. At the time of the L05a audit, the tracked config, target rules, and
source contained no explicit graphics RHI, targeted shader format/SM6, Lumen
GI/reflections, Support Hardware Ray Tracing, Support Compute Skin Cache, or
`Use Hardware Ray Tracing when available` selection. The prototype map package
was not decoded for world-setting values. L05c later added an explicit
Windows D3D12/SM6 target, hardware ray-tracing and skin-cache shader support,
and a Compatibility startup profile with hardware RT disabled. The existing
2026-10-26 L01a live map/renderer audit did observe the Editor/PIE baseline on
the RTX 5090: D3D12 RHI with active feature level SM5,
`r.DynamicGlobalIlluminationMethod=0` (no dynamic GI), `r.ReflectionMethod=2`
(screen-space reflections), and `r.RayTracing=0` (hardware RT off). This is recorded
in the 2026-10-26 L01a live map/renderer audit in the main-checkout
`PROGRESS.md` and commit `de201f5`.
These are dated observed values, not a guarantee of the effective renderer
state after later changes; recheck after any renderer configuration update.
The observed SM5 session did not qualify for the planned DX12/SM6 hardware-RT
path.

For the planned Windows path, UE 5.8's Lumen requirements specify Windows 10
build 1909.1350 or newer, DirectX 12, and SM6 enabled. Epic's hardware-RT
specific Lumen list requires Windows 10 or newer with DirectX 12 and NVIDIA RTX
2000-series or newer, or AMD RX 6000-series or newer. Epic's broader Lumen list
also includes Intel Arc A-series, but the hardware-RT-specific list does not;
this plan does not treat Arc as verified Lumen hardware-RT support. Project
support also requires Support Hardware Ray Tracing and Compute Skin Cache, and
Lumen requires `Use Hardware Ray Tracing when available` to select that path.
When the GPU, RHI, or OS is unsupported, Unreal falls back to software ray
tracing. Linux Vulkan support is documented as experimental and is outside this
Windows-only plan. These are engine capability conditions, not Kalmala minimum
or recommended specifications. ([UE 5.8 Hardware and Software Specifications](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-and-software-specifications-for-unreal-engine),
[Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine),
[Hardware Ray Tracing](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-ray-tracing-in-unreal-engine))

The recorded i7-14700K/RTX 5090 system remains a reference PC only; it does not
establish a minimum or recommended GPU, performance tier, or release support
floor. Device-level performance and image-quality measurements remain for L05e.

L05b preserves the accepted L01h visual baseline, which was reviewed with the
L01a-observed `r.RayTracing=0` setting. Compatibility is explicitly configured
as no dynamic GI and screen-space reflections, with `r.RayTracing.Enable=0`
and `r.Lumen.HardwareRayTracing=0` at startup. The project compiles optional
D3D12 SM6 RT shaders for Balanced, enables the `bUseHardwareRayTracingForLumen`
project option, and retains Compatibility as the saved/default local choice.
Runtime path resolution still fails closed unless hardware RT was requested
and the project, shader platform, and RHI satisfy the L05b capability gate. UE 5.8
defines `r.ReflectionMethod=1` as Lumen and `=2` as screen-space reflections;
the recorded L01a value was SSR, not Lumen.

| Quality level | Proposed rendering path | Intended support and limits |
| --- | --- | --- |
| **Compatibility — hardware RT off** | Keep hardware ray tracing off, dynamic GI off, and use screen-space reflections with the existing raster shadow path. This is the explicit non-hardware fallback for all supported Windows adapters. | Required visual baseline. The accepted L01h review used the observed RT-off baseline. The scene must retain readable lighting, shadows, and material separation without an RTX-capable GPU. |
| **Ray Tracing Balanced** | On supported Windows DX12/SM6 hardware, use Lumen hardware ray tracing for global illumination and reflections with Surface Cache lighting. Keep Hit Lighting disabled. | Optional uplift gated by project, shader-platform, and RHI capability checks. Unsupported systems stay on Compatibility. The compiled project setting only makes the shaders available; the local Compatibility profile remains the startup default. |
| **Ray Tracing High** | On supported Windows DX12/SM6 hardware, use Lumen hardware ray tracing with Unreal's Epic (quality level 3) GI and reflection scalability groups. Keep Surface Cache lighting and force Hit Lighting off after applying scalability. | This is an opt-in high-quality preset, not a universal default. Ray-traced shadows remain disabled until a key-light comparison demonstrates visible benefit. Change generated-geometry visibility only where per-object controls preserve the scene. |
| **Path-traced capture** | Use Unreal's Path Tracer for offline stills or slow cinematic captures when a converged reference image is useful. | A reference and presentation mode only. Do not use it to define runtime gameplay lighting, minimum hardware, or performance targets. |

Build the first three levels around the existing art and post-process controls.
Ray tracing may improve indirect light, reflections, and selected shadows, but
it must not be required for the warm/cool palette, visibility, weather cues,
fire readability, or the proposed stylized surface response. Keep quality levels
scalable through Unreal's existing scalability settings; do not invent fixed
GPU-specific numerical settings before measuring the scene. Hardware ray
tracing has additional scene-update cost, especially with many overlapping
instances, so check the generated forests, grass, sky geometry, and other
repeated meshes for ray-tracing visibility and culling before enabling it broadly.

The local Graphics-menu tier is stored in `GameUserSettings`. Compatibility
applies `r.RayTracing.Enable=0`, `r.Lumen.HardwareRayTracing=0`,
`r.DynamicGlobalIlluminationMethod=0`, and `r.ReflectionMethod=2`. Balanced and
High are offered only when the runtime gate accepts the active Windows
DX12/SM6 adapter. Both enable Lumen GI/reflections with Surface Cache lighting;
Balanced applies Unreal's quality level 2 to the GI and reflection groups,
while High applies quality level 3 to those groups only. The previous GI and
reflection quality values are restored when returning to Compatibility. Since
the Epic GI group enables Hit Lighting, the profile reapplies
`r.Lumen.HardwareRayTracing.HitLighting.Allowed=0` after scalability. Ray-traced
shadows remain disabled pending a rendered comparison at L06c. The
project sets `r.RayTracing=1` to permit dynamic RT, enables
`r.RayTracing.EnableOnDemand=1`, compiles the required skin-cache shaders, and
starts with `r.RayTracing.Enable=0`, allowing local profiles to turn RT on while
preserving Compatibility as the default. The
`Kalmala.Rendering.RayTracing.QualityProfiles` test guards project support,
on-demand and skin-cache settings, and RT-off startup.

L05d's generated-geometry inspection found that Meadow tree trunks and canopies
are each consolidated into one `UProceduralMeshComponent` per generated patch.
No generated grass or sky mesh, or instanced static-mesh components, were found
in the scanned project source. Procedural meshes retain UE's default
ray-tracing visibility. Hiding either consolidated tree component would remove
all trunks or canopies in a patch, so this increment changes no RT visibility
flags; per-object forest culling needs a different generated-geometry layout.
The level-authored directional sun is the only clear key-light candidate, but
no RT-on/off comparison has established a visible shadow benefit. This
increment adds no per-light or global ray-traced-shadow override; the
project-wide default remains disabled. Rendered visual comparison is reserved
for the integrated L06c gate.

Keep NVIDIA DLSS separate from the ray-tracing levels. Super Resolution is an
optional upscaling choice; DLAA, Ray Reconstruction, and Frame Generation are
separate capabilities with their own hardware and quality tradeoffs. If the
project later adopts NVIDIA's UE 5.8 DLSS plugin, version-pin and test that
integration independently. Do not make DLSS a prerequisite for the baseline or
count generated frames as a substitute for meeting the game's native input and
frame-time targets. ([NVIDIA DLSS for Unreal Engine](https://developer.nvidia.com/rtx/dlss))

### L05f — Upscaling remains separate (2026-10-27)

The project descriptor, module dependency files, source, and runtime config were
audited for DLSS, Streamline, Ray Reconstruction, and Frame Generation; no
integration or vendor dependency is present. L05's quality selector covers
only the renderer's Compatibility, Balanced, and High profiles. If optional
upscaling becomes a release requirement, specify and approve it as a separate
feature with its own supported-hardware, image-quality, and native frame-time
acceptance; this audit adds no plugin or runtime behavior.

The existing performance record uses an i7-14700K and RTX 5090; it does not
certify minimum or recommended hardware, or low- and mid-tier performance. For
this plan, capture identical scenes with hardware RT off, Balanced, and High on
the current reference PC and on representative lower-tier systems selected
before release. Include a supported non-RT system, an entry supported hardware
RT system, and AMD hardware if the release target intends to support it. Record
GPU frame time, frame rate, internal/output resolution, memory use, and any
temporal artifacts. Set a release floor only after these measurements and the
game's frame-time target are agreed.

L05e remains open. The user selected the RTX 5090 reference PC for the current
measurement pass and explicitly left lower-tier coverage pending. `nvidia-smi`
reports an RTX 5090 with 32,607 MiB VRAM and driver 610.88, but this is system
telemetry rather than an in-game tier measurement. The user chose to keep L05e
open until L06, where the Editor build and reference profile captures will run.
No Kalmala Editor build is available in the L05 worktree or main checkout. Do
not infer performance or a hardware floor from these observations.
Lower-tier/AMD measurements remain future scope.

### L05e1 — Capture-only quality selection (2026-10-27)

`Scripts/Tests/World/BiomeBaselineCapture.ps1` accepts
`-RayTracingQuality Compatibility|Balanced|High` and forwards it to the
non-Shipping settings subsystem only when `-KalmalaBiomeBaselineCapture` is
present. The override uses `SetRayTracingQualityLevel`, so Balanced and High
still pass the existing DX12/SM6/RHI capability gate. A successful profile is
logged explicitly; the runner rejects output without that confirmation. GPU
CSV names use an `L05_GPU_<tier>_...` prefix and the runtime capture log records
the requested tier. The existing L03 path and ordinary saved graphics settings
are unchanged. The capture runner's isolated `-UserDir` keeps each requested
profile out of the user's normal settings.

This adds repeatable capture control. L06b supplied the integrated Editor build.
L05e2a Compatibility, L05e2b Balanced, and L05e2c High have now been measured
on the selected RTX 5090 reference PC. Lower-tier and AMD hardware coverage
remains pending selection; no release hardware floor is inferred from these
reference-PC measurements.

### L05e2a — Compatibility reference capture (2026-10-27)

The integrated UE 5.8.2 Editor mirror at `C:\Users\Ville\AppData\Local\Temp\k06b`
captured the ten fixed views from the seed-418/419 lighting manifest using
`Scripts/Tests/World/BiomeBaselineCapture.ps1`. Each view explicitly requested
`-RayTracingQuality Compatibility`, `-GpuCaptureFrames 120`,
`-TemporalFrames 8`, and `-TemporalIntervalSeconds 0.5`; the runner accepted each
capture only after checking its profile-applied marker. Output screenshots are
1024x768. The machine reported an NVIDIA GeForce RTX 5090, driver 610.88, and
32,607 MiB device VRAM. `nvidia-smi` telemetry was sampled about once per second
before, during, and after each view; the capture samples report temperature,
utilization, device-used VRAM, and board power. The GPU CSVs also retain Unreal
physical/virtual/shader memory fields. The capture path reports output size but
does not report a separate internal render resolution.

Per-view GPUTime is whole-view total frame GPU time, not material attribution.
Each raw GPU CSV contains 120 positive GPUTime samples plus profiler metadata.
The table records the 120-sample mean/min/max, device-used VRAM peak, and the
whole-frame temporal RGB metric. Full per-view UE memory telemetry and device
samples are in `TestResults/L05e2/Compatibility/GpuSummary.csv` and
`NvidiaSmiTelemetry.csv`; raw GPU captures and screenshots are in the adjacent
`Seed418/` and `Seed419/` folders. The full temporal sequences remain in the
temporary capture output; seed-418 OvercastRain T01/T06/T07 are also retained
beside its base screenshot so L06c can review the transient silhouette. The
computed temporal summary is preserved in `TemporalStability.csv`.

| Seed | View | Mean GPUTime ms | Min–max GPUTime ms | Peak device-used VRAM MiB | Mean temporal RGB delta | Max adjacent delta |
|---:|---|---:|---:|---:|---:|---:|
| 418 | MeadowsDaylight | 0.8147 | 0.7542–1.0917 | 3,858 | 0.2372 | 0.2429 |
| 418 | ForestShade | 0.8448 | 0.8028–1.0819 | 3,758 | 0.2528 | 0.2721 |
| 418 | OpenWater | 0.7654 | 0.7370–0.9605 | 3,678 | 0.1873 | 0.2077 |
| 418 | OvercastRain | 0.7777 | 0.7390–1.0135 | 3,792 | 0.2706 | 0.4492 |
| 418 | FireDusk | 0.8382 | 0.7770–1.2283 | 4,068 | 0.1747 | 0.2237 |
| 419 | MeadowsDaylight | 0.8503 | 0.7756–1.3773 | 3,849 | 0.2239 | 0.2550 |
| 419 | ForestShade | 0.7584 | 0.7204–0.9558 | 3,877 | 0.2053 | 0.2161 |
| 419 | OpenWater | 0.7867 | 0.7473–1.0181 | 3,758 | 0.2046 | 0.2238 |
| 419 | OvercastRain | 0.7808 | 0.7423–0.9876 | 3,760 | 0.2140 | 0.2567 |
| 419 | FireDusk | 0.8142 | 0.7756–1.0510 | 3,950 | 0.1583 | 0.1844 |

Across the ten views, mean GPUTime ranged from 0.7584 to 0.8503 ms. `nvidia-smi`
device-used VRAM ranged from 3,060–3,205 MiB before capture, 3,471–3,741 MiB
mean during capture, and 3,678–4,068 MiB peak during capture. Unreal's recorded
mean physical usage ranged from 2,591.02–2,678.99 MB; peak virtual usage ranged
from 3,251.72–3,535.32 MB. These values describe this one machine and fixed
editor view, not minimum hardware requirements. The seed-418 OvercastRain row
has an 8.6383 ms mean queue-unaccounted value while its mean GPUTime is 0.7777
ms; this separate queue metric is retained as an outlier rather than folded
into GPUTime.

Image review found stable sampled water and fire compositions between temporal
endpoints. A tiny distant silhouette appears in seed-418 OvercastRain frames
T06/T07 but is absent from T01–T04; its cause is unidentified and L06c should
review it. Several fixed views include broad black distant-horizon/empty-sky
areas, and seed-419 ForestShade faces mostly open water rather than dense
foliage. These are composition limitations to keep in mind during integrated
visual review, not a visual-acceptance result. No temporal pass threshold is
defined. This subsection records only Compatibility; Balanced and High are documented in
L05e2b and L05e2c.
Lower-tier/AMD hardware remains open, and no release floor is claimed.

### L05e2b — Balanced reference capture (2026-10-27)

The integrated UE 5.8.2 Editor mirror at `C:\Users\Ville\AppData\Local\Temp\k06b`
captured the ten seed-418/419 fixed views with
`Scripts/Tests/World/BiomeBaselineCapture.ps1`. Each view explicitly requested
`-RayTracingQuality Balanced`, 120 GPUTime samples, eight temporal frames at
0.5-second intervals, and 1024x768 output. The capture reports output
dimensions but not separate internal render resolution. The RTX 5090 reported
driver 610.88 and 32,607 MiB device VRAM. The capture records Unreal
physical/virtual/shader memory, plus roughly once-per-second `nvidia-smi`
temperature, utilization, device-used VRAM, and board-power telemetry. These are
editor reference-PC measurements, not release hardware floors.

Profile logs were checked for effective runtime state, not only the selector's
success marker. At startup, Compatibility had `r.RayTracing.Enable=0` and
`r.Lumen.HardwareRayTracing=0`; its post-profile query remained
`0/0/0` for ray tracing enable, Lumen hardware ray tracing, and
`IsRayTracingEnabled`. The Balanced smoke and all ten Balanced view logs
reported effective `1/1/1` for those values. The 120-sample GPUTime rows are
positive in every raw CSV; all 80 base/temporal PNGs are 1024x768. Full evidence
is preserved in `TestResults/L05e2/Balanced/`; it includes ten base screenshots,
three selected OvercastRain temporal frames, ten raw GPU CSVs, and GPU, device
telemetry, and temporal summary CSVs. The temporal metric is whole-frame RGB
difference sampled every 16 pixels, not a defined pass threshold.

| Seed | View | Mean GPUTime ms | Min–max GPUTime ms | Peak device-used VRAM MiB | Mean temporal RGB delta | Max adjacent delta |
|---:|---|---:|---:|---:|---:|---:|
| 418 | MeadowsDaylight | 1.2812 | 1.1758–1.6816 | 4,339 | 0.2652 | 0.2850 |
| 418 | ForestShade | 1.3797 | 1.2406–1.7213 | 4,381 | 0.3765 | 0.3957 |
| 418 | OpenWater | 1.3140 | 1.2293–1.6700 | 4,268 | 0.2193 | 0.2423 |
| 418 | OvercastRain | 1.2646 | 1.1776–1.5656 | 4,412 | 0.2735 | 0.4389 |
| 418 | FireDusk | 1.3287 | 1.1906–1.5546 | 4,334 | 0.2360 | 0.2535 |
| 419 | MeadowsDaylight | 1.3456 | 1.2379–1.7079 | 4,288 | 0.2614 | 0.2984 |
| 419 | ForestShade | 1.3182 | 1.2036–1.7421 | 4,268 | 0.2520 | 0.2604 |
| 419 | OpenWater | 1.3402 | 1.2138–1.6554 | 4,268 | 0.2716 | 0.2918 |
| 419 | OvercastRain | 1.3326 | 1.2082–1.5798 | 4,335 | 0.2173 | 0.2798 |
| 419 | FireDusk | 1.3456 | 1.2426–1.7717 | 4,278 | 0.1935 | 0.2110 |

Across the views, mean GPUTime ranged from 1.2646–1.3797 ms and individual
samples from 1.1758–1.7717 ms. Device-used VRAM was 3,101–3,246 MiB before
capture, 3,829.37–3,942.40 MiB mean during capture, and 4,268–4,412 MiB peak.
Unreal mean physical usage ranged from 2,713.39–2,776.82 MB; peak physical
usage was 2,715.90–2,777.54 MB, minimum free memory was 5,522.62–6,284.76 MB,
and peak virtual usage was 3,895.62–4,020.41 MB. Per-view telemetry, including
sample counts, temperature, utilization, and power, remains in the evidence
CSVs.

Visual comparison against the Compatibility captures showed the clearest
Balanced change in seed-418 ForestShade tree shadows and trunk shading; most
other base frames looked similar or subtly different. Seed-419 ForestShade's
anchor is open-water/horizon-like rather than dense forest, as in Compatibility.
Tiny silhouettes in seed-418 OvercastRain T06/T07 also appear in Compatibility;
they are not attributed to the Balanced profile. Retain these views for L06c
review; this is measurement evidence and does not claim visual acceptance.
High is now measured in L05e2c. Lower-tier/AMD hardware selection, L06c
acceptance, and L06e verification remain pending.

### L05e2c — High reference capture (2026-10-27)

The integrated UE 5.8.2 Editor mirror at `C:\Users\Ville\AppData\Local\Temp\k06b`
captured the ten seed-418/419 fixed views using
`Scripts/Tests/World/BiomeBaselineCapture.ps1`. Each view requested
`-RayTracingQuality High`, 120 positive GPUTime samples, eight temporal frames
at 0.5-second intervals, and 1024x768 output. The capture reports output
dimensions but not separate internal render resolution. The RTX 5090 reported
driver 610.88 and 32,607 MiB device VRAM. The coordinator recorded per-view
Unreal memory and `nvidia-smi` temperature, utilization, device-used VRAM, and
board-power samples.

All ten per-view logs contain both the explicit
`L05 GPU profile applied: Quality=High.` marker and the post-application
effective state `r.RayTracing.Enable=1`,
`r.Lumen.HardwareRayTracing=1`, and `IsRayTracingEnabled=1`. Every raw CSV
contains exactly 120 positive GPUTime samples. The runner validated eight
1024x768 frames for every view. The full temporal sequences remain in the
temporary capture output. `TestResults/L05e2/High/` preserves ten base
screenshots, ten per-view logs, ten raw GPU CSVs, the GPU/device/temporal
summary CSVs, and seed-418 OvercastRain T01/T06/T07 for artifact review.
Temporal RGB change is sampled every 16 pixels with the established whole-frame
method; no pass threshold is defined.

| Seed | View | Mean GPUTime ms | Min–max GPUTime ms | Peak device-used VRAM MiB | Mean temporal RGB delta | Max adjacent delta |
|---:|---|---:|---:|---:|---:|---:|
| 418 | MeadowsDaylight | 1.3506 | 1.2754–1.6158 | 4,706 | 0.2212 | 0.2554 |
| 418 | ForestShade | 1.4480 | 1.3336–1.9456 | 4,638 | 0.2938 | 0.3081 |
| 418 | OpenWater | 1.4179 | 1.2946–1.7339 | 4,720 | 0.2128 | 0.2342 |
| 418 | OvercastRain | 1.3763 | 1.2830–1.6178 | 4,822 | 0.3011 | 0.4470 |
| 418 | FireDusk | 1.3621 | 1.2769–1.5975 | 4,682 | 0.2325 | 0.2398 |
| 419 | MeadowsDaylight | 1.3700 | 1.2937–1.6701 | 4,734 | 0.2584 | 0.2887 |
| 419 | ForestShade | 1.3754 | 1.2882–1.5969 | 4,772 | 0.2405 | 0.2629 |
| 419 | OpenWater | 1.4092 | 1.3398–1.6542 | 4,992 | 0.2646 | 0.2696 |
| 419 | OvercastRain | 1.3911 | 1.3190–1.6267 | 4,880 | 0.2179 | 0.2904 |
| 419 | FireDusk | 1.3532 | 1.2783–1.6680 | 5,035 | 0.1865 | 0.1904 |

Mean GPUTime across views ranged from 1.3506–1.4480 ms; individual samples
ranged from 1.2754–1.9456 ms. Device-used VRAM ranged from 3,065–3,496 MiB
before capture, 3,571.07–4,379.20 MiB mean during capture, and 4,638–5,035 MiB
peak. Unreal mean physical use ranged from 2,710.86–2,809.60 MB; peak physical
use was 2,713.46–2,810.90 MB, minimum free memory 4,520.29–8,208.25 MB, and
peak virtual use 4,217.65–4,413.26 MB. The 254 device telemetry rows include
before, during, and after samples; each view has 17–61 in-capture samples.
During capture, device temperature ranged 40–50 C, utilization 0–56%, and board
power 41.79–189.96 W. These results describe the selected editor reference
machine and do not set a release hardware floor.

Visual review found High broadly similar to Balanced in these fixed views; tree
shadows remain strongly defined, and the warm FireDusk ground spill remains
readable. Seed-419 ForestShade is still an open-water/horizon-like anchor rather
than dense foliage. Tiny distant silhouettes in seed-418 OvercastRain T06/T07
also occur in the earlier Compatibility and Balanced captures; their cause is
unidentified. Retain the evidence for L06c, which owns integrated visual
acceptance. High is now measured; lower-tier/AMD hardware selection, L06c
acceptance, and L06e verification remain open.

### L06c1 — Cross-tier fixed-view review (2026-10-27)

The fixed-view review used the already integrated UE 5.8.2 L05e2 evidence; no
new capture or build was needed. All 30 base screenshots were reviewed
side-by-side by scene across Compatibility, Balanced, and High. Each tier
contains both seed sets for MeadowsDaylight, ForestShade, OpenWater,
OvercastRain, and FireDusk:

| Tier | Seed 418 | Seed 419 |
| --- | --- | --- |
| Compatibility | TestResults/L05e2/Compatibility/Seed418/{MeadowsDaylight,ForestShade,OpenWater,OvercastRain,FireDusk}.png | TestResults/L05e2/Compatibility/Seed419/{MeadowsDaylight,ForestShade,OpenWater,OvercastRain,FireDusk}.png |
| Balanced | TestResults/L05e2/Balanced/Seed418/{MeadowsDaylight,ForestShade,OpenWater,OvercastRain,FireDusk}.png | TestResults/L05e2/Balanced/Seed419/{MeadowsDaylight,ForestShade,OpenWater,OvercastRain,FireDusk}.png |
| High | TestResults/L05e2/High/Seed418/{MeadowsDaylight,ForestShade,OpenWater,OvercastRain,FireDusk}.png | TestResults/L05e2/High/Seed419/{MeadowsDaylight,ForestShade,OpenWater,OvercastRain,FireDusk}.png |

These 30 files are accepted as the cross-tier static lighting evidence set,
not as complete L06c acceptance. The nine retained temporal stills were also
reviewed: Seed418/OvercastRain_T01.png, T06.png, and T07.png under each tier
directory. A PowerShell image check confirmed that all 39 retained PNGs are
1024x768. All three TemporalStability.csv files contain ten 8-frame,
0.5-second rows; no temporal acceptance threshold is defined.

The clearest tier difference is in seed-418 ForestShade: Balanced and High
show darker, higher-contrast trunk and ground shadows than Compatibility while
the tree silhouettes remain readable. The other fixed comparisons show no
obvious new tier-specific readability loss; Balanced and High look broadly
similar. FireDusk retains a warm, localized ground pool without washing out
the nearby ground. No fixed-view tier introduced a confirmed rendering
regression.

In seed-418 OvercastRain, T01 has no distant silhouette; a tiny silhouette
appears in T06 and remains in T07 in all three tiers. The corresponding maximum
adjacent sampled RGB deltas are 0.4492 for Compatibility, 0.4389 for Balanced,
and 0.4470 for High. The static stills do not identify the silhouette's source,
and the measurements do not separate world motion from shader change; this is
recorded as an unexplained scene change, not a confirmed shader shimmer defect.
The sampled frames show no other obvious temporal artifact.

Composition limits remain: seed-419 ForestShade is mostly open water/horizon,
and seed-419 OvercastRain is nearly all flat water/sky. Those captures cannot
establish dense-foliage or rain-material readability. Several views also have
large black/empty sky or distant-frame regions. These are limits of the fixed
anchors; this review does not establish whether they reflect a production
camera or world-bound issue. The L06c2 PIE spot-check is recorded below; it
covers default framing, horizontal look, and HUD readability, while dynamic
exposure transitions remain unverified. No gameplay, network, save, or runtime
code changed. Full lighting verification remains for L06e.

### L06c2 — Default third-person camera and HUD spot-check (2026-10-27)

The integrated C:\Users\Ville\AppData\Local\Temp\k06b\Kalmala.uproject was
opened in UE 5.8 PIE on /Game/Kalmala/Maps/Prototype/L_Prototype, which is
both EditorStartupMap and GameDefaultMap. In the ordinary third-person view,
the player spawned centered in front of the follow camera without start
position clipping. Horizontal mouse-look rotated the player/view and kept the
follow view stable. The code defaults are a 320 cm spring arm, a 45 cm vertical
boom offset, controller rotation on the boom, and no independent follow-camera
rotation.

The visible HUD did not overlap: a six-slot hotbar was at top-left; Meadows
label, circular minimap, sun, and water/timer indicators were at top-right.
Player and terrain shadows were distinct and high-contrast. No obvious exposure
jump, lens effect, or bloom artifact appeared in the brief view.

This was a short camera/HUD spot-check, not a full movement or post-process
transition run. Dynamic exposure transitions and volume boundaries were not
exercised, so this observation does not establish that they remain stable while
walking. The map uses rough placeholder generated terrain/character materials
and has a black horizon band, consistent with the limited prototype content;
these are recorded as scope limitations, not L06 tier regressions. PIE was
stopped; no map/assets or production code were changed. L06c fixed-view and
default-camera/HUD checks are complete; L06d and the L06e final verification
remain open.

### L06d — Combined lighting regression review (2026-10-27)

The review covered the integrated lighting/runtime changes and retained evidence,
with no source defect or confirmed L06 regression requiring repair.

- **Integrated implementation:** The project keeps Compatibility as the RT-off
  startup profile while compiling optional on-demand RT support. The capability
  gate requires project/runtime support plus Windows D3D12, SM6, shader-platform,
  and RHI capabilities. Balanced and High capture logs report post-apply
  r.RayTracing.Enable=1, r.Lumen.HardwareRayTracing=1, and
  IsRayTracingEnabled=1 for all ten views each. Compatibility has ten explicit
  profile markers and startup config lines for RT/Lumen off; the later L06b
  rendered Compatibility smoke independently reports effective 0/0/0. The ten
  older Compatibility measurement logs predate the post-apply diagnostic, so
  they do not each report effective runtime state.
- **Build and focused checks:** The integrated UE 5.8 Editor build passed 56
  actions in the isolated k06b mirror. The corrected focused automation rerun
  passed all six named RT, material, hearth, and weather tests with exit 0.
  The first queue wrapper invocation was a false negative caused by assuming
  marker order; after the wrapper was corrected, the same queue passed. Two
  generic Condition failed diagnostics appear before the named automation queue
  and match earlier successful runs; no named failure, fatal, assertion, or
  ensure was recorded.
- **Rendered/performance evidence:** The Compatibility smoke validated its
  profile marker, D3D12/SM6 startup, PNG signature, 1024x768 output, and
  nonblank image. The ten-view RTX 5090 captures preserve 120 positive GPUTime
  samples and 1024x768 images for each scene/profile; Balanced and High have
  post-apply state evidence. No project performance threshold or release
  hardware floor is defined. These measurements do not establish one.
- **L06c findings:** Cross-tier review found the clearest change in seed-418
  ForestShade; no fixed-view tier-specific readability regression was confirmed.
  PIE inspection found no default-camera/HUD defect. Dynamic walking and
  exposure/post-process transitions were not exercised.

No production fix was needed, and no build or automation was rerun in L06d.
Known evidence limits remain for L06e: dynamic exposure transitions were not
tested; Compatibility's ten measurement logs lack post-apply state diagnostics
although the later smoke verifies 0/0/0; the seed-418 rain silhouette remains
unidentified without a temporal acceptance threshold; seed-419 ForestShade and
OvercastRain are composition-limited; only the RTX 5090 has measurements, with
lower-tier/AMD hardware still pending selection. Do not infer a support floor.
The full milestone verification remains L06e.

## Acceptance checks

- Fixed-camera captures show a consistent lighting direction and readable
  silhouettes across the five baseline scenes; no scene depends on crushed
  shadows or clipped highlights for its mood.
- Grass/soil, rock, bark, foliage, water, and fire remain distinguishable in
  daylight, shadow, rain, and night scenes.
- Campfire/torch light stays warm and legible without washing out nearby colors;
  terrain and foliage do not gain visible temporal shimmer from the shader.
- Exposure and local post-process transitions do not visibly pump, pop, or expose
  volume boundaries during normal movement; lens effects do not obscure UI,
  traversal cues, or creature silhouettes.
- Bloom stays attached to the brightest emissive/light sources and does not
  substitute for local illumination or erase the material response around fire.
- The compatibility tier preserves accepted lighting and gameplay readability
  with hardware ray tracing disabled; RT tiers add measurable image quality
  without changing gameplay visibility or environment rules.
- Hardware RT selection falls back cleanly on unsupported GPUs/renderers, and
  each supported tier has measured GPU frame time on representative hardware.
- Weather material changes transition without popping and do not alter world
  simulation or gameplay results.
- The approved appearance uses original Kalmala materials and does not alter
  collision, world generation, save data, server authority, or network behavior.
- Record capture settings and measured render cost against the existing target
  hardware. Do not invent a performance budget before measuring the baseline.

## Guardrails and open decisions

- Keep the existing Kalmala art direction and asset silhouettes. The plan does
  not authorize a global color-identity change, reduced-resolution rendering, or
  a custom engine fork.
- Do not apply a scene-color quantizer to UI, text, or all translucency. Avoid
  outlines unless a separate reference study shows they are part of the desired
  result.
- Decide after the prototype whether lighting should remain smoothly lit or use
  a subtle banded direct component, and whether weather layers belong on all
  surfaces or only selected materials.
- The lighting work is queued as six ordered, worktree-sized goals (L01–L06)
  in `BACKLOG.md`. Assign each goal to one agent in a dedicated worktree
  after the preceding lighting goal has been integrated. Independent Kalmala
  runs may proceed concurrently in distinct worktrees under `AGENTS.md`; these
  lighting stages remain ordered because each depends on earlier lighting
  outputs. L06 is the integrated milestone-final verification gate.

## References

- [Valheim FAQ — visual style](https://www.valheim.com/faq/)
- [Iron Gate interview on Valheim's graphics direction (Swedish)](https://www.fz.se/artikel/285972-iron-gate-svarar-pa-era-valheim-fragor)
- [Jötunn-generated Valheim 1.0.7 shader property listing](https://valheim-modding.github.io/Jotunn/data/prefabs/shader-list.html)
- [Unreal Engine 5.8 shading models](https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine)
- [Unreal Engine 5.8 material instances](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-materials-in-unreal-engine)
- [Unreal Engine 5.8 Post Process Effects](https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-effects-in-unreal-engine)
- [Unreal Engine 5.8 Post Process Volumes](https://dev.epicgames.com/documentation/en-us/unreal-engine/add-post-process-volumes)
- [Unreal Engine 5.8 Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine)
- [Unreal Engine 5.8 post-process materials](https://dev.epicgames.com/documentation/en-us/unreal-engine/post-process-materials-in-unreal-engine)
- [Unreal Engine 5.8 release notes — Substrate NPR shading and experimental fog scattering](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-8-release-notes)
- [Unreal Engine 5.8 Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine)
- [Unreal Engine 5.8 Hardware Ray Tracing](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-ray-tracing-in-unreal-engine)
- [Unreal Engine 5.8 Lumen Performance Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-performance-guide-for-unreal-engine)
- [Unreal Engine 5.8 Ray Tracing Performance Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/ray-tracing-performance-guide-in-unreal-engine)
- [Unreal Engine 5.8 Path Tracer](https://dev.epicgames.com/documentation/en-us/unreal-engine/path-tracer-in-unreal-engine)
- [NVIDIA DLSS for Unreal Engine](https://developer.nvidia.com/rtx/dlss)
