# Valheim-inspired material and lighting plan

**Status:** Research and implementation plan. This file records no runtime
change. The ordered L01–L06 lighting goals are now queued in `BACKLOG.md`.

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
| Generated material assignments | Source/KalmalaWorld/Private/KalmalaGeneratedTerrainPatch.cpp:180-264 assigns M_GeneratedTerrain to terrain (or M_GeneratedTerrainBiomeDebug in debug mode), M_GeneratedWater to ocean and lake water, M_GeneratedLakeShore to lake shore, M_GeneratedRock to Meadow rocks, M_GeneratedBark to Meadow trunks, and M_GeneratedCanopy to Meadow canopies. M_GeneratedSoil is not among this constructor's assignments. Source/KalmalaEditor/Private/CreateWorldMaterialsCommandlet.cpp:18-190, 257-264 builds M_GeneratedTerrain from vertex-color R/G/B weights for grass/soil/rock plus alpha snow and roughness 0.92; its generic creator defines constant-color/roughness materials, while its debug material feeds vertex color to base and emissive. The generic creator returns early for an existing package. These are source-generation and constructor facts; package graphs and any map overrides were not inspected. |
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

| Quality level | Proposed rendering path | Intended support and limits |
| --- | --- | --- |
| **Compatibility — hardware RT off** | Keep the hardware ray tracing path disabled. Use Lumen software ray tracing only where the chosen renderer and GPU support it; otherwise use the project's selected non-hardware fallback. | This is the required visual baseline. The scene must retain readable lighting, shadows, and material separation without an RTX-capable GPU. Epic lists DX12/SM6 and GTX 1070-class requirements for Lumen software ray tracing, so verify the actual supported floor rather than assuming it runs on every PC. |
| **Ray Tracing Balanced** | On supported DX12/SM6 hardware, use Lumen hardware ray tracing for global illumination and reflections with the Surface Cache lighting mode. Keep hit lighting off initially. | Optional quality uplift for compatible RTX 20-series-and-newer or RX 6000-series-and-newer hardware. Preserve an automatic fallback to the hardware-RT-off path on unsupported systems. |
| **Ray Tracing High** | Raise Lumen GI/reflection quality only as measured performance allows. Test ray-traced shadows on selected key lights where they visibly improve contact or foliage shadows; keep other lights on their normal shadow method unless they pass the same test. | This is an opt-in high-quality preset, not a universal default. Cull ray tracing to relevant scene ranges and profile forests, water, camp lights, and moving creatures. Avoid Hit Lighting for gameplay unless a separate test proves its cost acceptable; Epic describes it as substantially more expensive. |
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

Keep NVIDIA DLSS separate from the ray-tracing levels. Super Resolution is an
optional upscaling choice; DLAA, Ray Reconstruction, and Frame Generation are
separate capabilities with their own hardware and quality tradeoffs. If the
project later adopts NVIDIA's UE 5.8 DLSS plugin, version-pin and test that
integration independently. Do not make DLSS a prerequisite for the baseline or
count generated frames as a substitute for meeting the game's native input and
frame-time targets. ([NVIDIA DLSS for Unreal Engine](https://developer.nvidia.com/rtx/dlss))

The existing performance record uses an i7-14700K and RTX 5090; it does not
certify minimum or recommended hardware, or low- and mid-tier performance. For
this plan, capture identical scenes with hardware RT off, Balanced, and High on
the current reference PC and on representative lower-tier systems selected
before release. Include a supported non-RT system, an entry supported hardware
RT system, and AMD hardware if the release target intends to support it. Record
GPU frame time, frame rate, internal/output resolution, memory use, and any
temporal artifacts. Set a release floor only after these measurements and the
game's frame-time target are agreed.

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
