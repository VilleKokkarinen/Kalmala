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
canopy components resolve project-owned materials under
`Content/Kalmala/World/Materials/`. `CreateWorldMaterialsCommandlet` generates
the terrain ground-blend material from vertex RGB weights for grass, soil, and
rock, plus vertex alpha for cold-biome snow coverage. The other generated
surface materials use constant base color and roughness, and a separate
vertex-color biome-debug material remains available. The terrain patch assigns
these materials to its procedural mesh components. The current snow blend is
biome/height-driven rather than driven by a snow-weather state. The project
brief's established mood is a northern wilderness shaped by forests, lakes, and
long twilight.

Before selecting values, inspect the actual prototype map's lights, sky, fog,
exposure, and any level post-process settings in Unreal Editor. The project
defaults alone do not define a complete scene lighting rig.

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

#### Existing weather and environment inputs (L04a, source audit 2026-10-27)

This is a map of the current inputs and effects, not a new weather contract.
Weather remains server-owned; clients may use the existing replicated values
for local presentation.

| Plan effect | Existing input and path | Support and limits |
| --- | --- | --- |
| Rain and wet-surface response | `FKalmalaWeatherState` exposes normalized `PrecipitationIntensity` and `WindStrength`, plus derived `StormIntensity`, through the replicated `AKalmalaWorldGenerationGameState`. `UKalmalaAmbientAudioSubsystem` already fades the local `RainBed` and `WindBed` from those values. | Replicated precipitation is a valid cosmetic rain/wet-material input. The authoritative interaction-grid surface moisture and per-player wetness remain gameplay state, not shared material inputs. Current wood/rock materials have no weather-wet response. The weather state has no precipitation-kind field, so rain cannot be distinguished from snow by type. |
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
- The lighting work is queued as six ordered, worktree-sized goals (L01–L06) in
  `BACKLOG.md`. Assign each goal to one agent in a fresh worktree after the
  previous goal is integrated; run them sequentially under the repository's
  single-writer rule. L06 is the integrated milestone-final verification gate.

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
