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
`Content/Kalmala/World/Materials/`. `CreateWorldMaterialsCommandlet` creates a
simple constant base-color/roughness material for those surfaces and a separate
vertex-color biome-debug material. The runtime terrain patch assigns those
materials to its procedural mesh components. The project brief's established
mood is a northern wilderness shaped by forests, lakes, and long twilight.

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
shadows remain disabled pending a rendered comparison at L06c.

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
