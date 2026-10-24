# Meadows terrain and biome plan

Status: proposed implementation plan, 2026-10-24. This document records the
requested terrain direction; no generator parameters or gameplay are changed.
It does not reorder or complete the active item backlog.

## Reference interpretation

The three user-supplied screenshots are visual references, not heightmaps or
instructions. They support these terrain goals; exact dimensions, underlying
algorithms and slopes cannot be recovered from perspective images.

| Reference | Observable structure | Meadows generation goal |
| --- | --- | --- |
| `1.png` (open hillside) | A long grassy incline, a broad hollow, wooded upper ground, exposed earth/rock on steeper faces and scattered boulders. | Readable hills and valleys over tens to hundreds of metres, with open traversable slopes and occasional steeper shoulders. |
| `2.png` (wooded ground) | Uneven forest floor, shallow depressions and rises, dense grass, saplings and irregular tree spacing. | Smaller continuous relief beneath groves; vegetation follows terrain suitability and forms patches rather than a uniform cover. |
| `3.png` (water edge) | A gently shelving shore, shallow submerged ground, small emergent banks, reeds and a wooded far bank. | Continuous land-to-water profiles, irregular shore contours and vegetation selected by actual water depth and bank slope. |

Use these spatial relationships with Kalmala's original art and world rules.
Do not reproduce the reference maps, assets, characters, lighting or UI. Dense
grass and trees alone cannot provide the missing ground silhouette.

## Current implementation and likely flatness causes

Reviewed source paths are relative to the repository root:

- `Source/KalmalaWorld/Public/KalmalaRegionalTuning.h` defines macro Elevation
  frequency `1/100000` cm (a 1000 m coordinate scale), amplitude `0.85`, and
  local relief frequency `0.00035` (about 28.6 m), amplitude `0.015`.
  Noise coordinate scales are not exact hill spacing.
- `KalmalaWorldFieldSampler.h` builds normalized fields, then applies the master
  map. Inland Elevation is `0.22 + coastFade * (0.10 + 0.65 * elevationNoise)`.
  `coastFade` is smoothstep of the clamped master mask divided by `0.12`.
  The atlas controls the coast; it is not a walking-scale relief generator.
- `Source/KalmalaWorld/Private/KalmalaRegionalGeneration.cpp` converts Elevation
  to centimetres with `(Elevation - 0.22) * 2000`. Its Meadows shape contributes
  `40 * Detail`, with Detail frequency `0.00015` (about 66.7 m), attenuated by
  the Meadows weight and an additional land factor. The field's local relief
  contributes at most 19.5 cm before coast attenuation
  (`0.015 * 0.65 * 2000`); the Meadows term has a 40 cm coefficient.
  These are coefficient bounds, not measured peak-to-trough heights.
- Broad source relief can provide elevation differences across a region, but
  the small local terms explain why a short walk can still look almost flat.
  This is a source-based hypothesis; baseline captures and profiles must verify
  it before tuning. Also confirm `L_Prototype` is loaded without an intersecting
  template Landscape, as described in the setup guide.
- `KalmalaGeneratedTerrainPatch.cpp` and `KalmalaWaterSurfaceMesh` use a shared
  30 m patch with 24 cells per side, giving 125 cm triangle spacing. Keep this
  lattice initially: it can represent broad hills and metre-scale shoulders.
  Tiny noise would alias, and changing mesh density would increase cost without
  addressing the small height amplitudes.
- `KalmalaForestLayout.cpp` permits the first 18 candidates per Meadows patch;
  its Flora density gate currently applies to Elderwood only. Meadows trees
  sample height but have no wetness/slope rejection there. Changes to grove
  placement must use the shared forest descriptor path for both decorative
  trees and authoritative harvest materialization.

## Proposed terrain model

Keep the four environmental fields, atlas coastline, distance eligibility,
16 km world radius and seed-only identity. Add Meadows-specific **derived
height shaping** in the regional sampler, rather than increasing the global
Elevation amplitude or changing biome thresholds to obtain hills. Global field
changes also move ecological suitability and hydrology candidates.

Evaluate broad and middle-scale seeded relief in absolute world XY, using
separate stable noise domains. Blend the result by the existing continuous
Meadows weight, never by a hard biome-label switch or patch coordinate. Other
biomes retain their own shaping terms. A reference formula for the land stage is:

```text
Hland = Hsource + HotherBiomes
      + meadowWeight * coastFade * (hillHeight * Nwide
                                    + rollingHeight * Nmiddle
                                    + floorHeight * Nlocal)
```

This replaces the current Meadows `40 * Detail` term; it must not accidentally
add that term twice. Define and name the coast factor explicitly, including
whether an existing extra land factor is retained. Noise values are signed;
amplitudes below are coefficients, not guaranteed terrain range. The first
version can use unwarped continuous noise; add a small domain warp only if
profiles show repetitive shapes. Do not add erosion simulation or a heightmap
asset dependency for this work.

### Initial tuning experiments

All values are proposed starting ranges, not accepted production defaults or
measurements from the screenshots. Author centimetres in code, metres here.
Compare the low, middle and high settings separately with fixed seeds.

| Proposed named control | Initial trial | Range to compare | Purpose |
| --- | --- | --- | --- |
| `MeadowHillScale` | 160 m | 100–250 m | Broad inclines and hollows visible across several patches. |
| `MeadowHillAmplitude` | 6 m | 4–10 m | A meaningful skyline and sustained uphill/downhill travel. |
| `MeadowRollingScale` | 45 m | 30–70 m | Rounded shoulders, smaller valleys and grove undulations. |
| `MeadowRollingAmplitude` | 1.2 m | 0.6–2 m | Relief readable near the player without continuous steep bumps. |
| `MeadowFloorScale` | 12 m | 8–20 m | Gentle forest-floor variation; avoid sub-lattice noise. |
| `MeadowFloorAmplitude` | 0.2 m | 0.1–0.35 m | Small rises and depressions beneath vegetation. |

Prefer broad form first, then rolling relief, then floor detail. Do not maximize
all amplitudes at once. Reject candidates producing noisy corrugations or
uniformly steep ground. As an initial acceptance target, ordinary walking
slopes should mostly sit below 15 degrees, with occasional 15–25 degree
shoulders and connected gentler alternatives. Retain naturally flatter pockets
for camps; do not flatten the entire starter radius or author fixed camp sites.
Evaluate the distribution from final collision triangles and compare against
the actual Character Movement and building-placement limits before finalizing.

### Coast, rivers and lakes

Insert the Meadows relief before the existing basin and channel operations.
Retain this order: source and biome relief, analytic basins, optional debug
streams, source coast clamp, final priority river carving. Preserve sea-level
river water at Z=0, `RiverBedDepth=80` cm, `RiverBankExtent=1800` cm and existing
lake-support precedence in the first terrain pass. Stronger land relief must
not refill a river or flatten an elevated enclosed lake to sea level.

Fade new relief continuously toward the atlas shore. A negative noise trough
on source land must still pass the existing coast clamp. Inspect whether that
clamp creates long shelves near Z=1 cm; reduce or smoothly bound the additive
term before the clamp if it does. Do not remove the coast-sign contract.

The shoreline reference is a profile goal, not authorization to scatter new
lakes throughout Meadows. First use existing coast, river banks and eligible
lake transitions. Confirm enclosed-basin acceptance and lattice clipping before
promising a visible pond. The current wetland basin rules and 350 m exclusion
remain intact. Any new Meadows pond generation needs a separate scoped design.
If bank slopes become excessive under the hill trials, assess a bounded bank
extent adjustment separately, preserving continuous returns on both sides and
all existing river controls. Do not quietly retune global river parameters.

## Terrain-driven Meadows dressing

Once landform is accepted, derive slope from final terrain, height above the
applicable visible water surface, and a continuous Flora-based patch mask.
These are derived suitability values, not new authoritative environmental maps.

- Open grassland occupies dry gentle slopes and clearings; groves concentrate
  in suitable Flora patches, with feathered edges and gaps. Add a Meadows Flora
  gate to the shared forest layout rather than raising density everywhere.
- Reject tree candidates in water and on unsuitable slopes. Keep trunks upright
  and seat roots on the final collision surface. Preserve original candidate
  ordinals and spawn IDs through filtering; avoid sequential-random shifts that
  relocate later candidates when a new early rejection is added.
- Blend grass/soil/rock presentation using final slope and moisture context:
  exposed shoulders receive more soil/rock, gentle ground more grass, and wet
  banks reduced terrestrial grass. Start with existing materials and rocks.
- Shallow wet margins can receive reeds only where matching original content
  exists or is separately authored. Grass, saplings and reeds require a later
  bounded cosmetic pass if absent; they are not delivered by height tuning.
- Fit decorative rocks to terrain and inspect gameplay resource nodes, wildlife,
  player starts and construction on the new slopes. Keep server-owned resource
  decisions and the existing suppression/depletion path authoritative.

No promise of screenshot-equivalent foliage density or lighting is part of the
terrain increment. Profile additional decoration before raising active budgets.

## Ordered implementation increments

These are proposed work units, not completion claims or an insertion ahead of
the active backlog. Follow the repository's one-increment-per-run protocol when
implementation is scheduled; split a unit further if necessary.

1. Record seed-418/419 baselines: fixed dry Meadows sites in the starter region
   and outer eligible range, wooded ground, coast and a high-land river. Export
   local final-height profiles and slope distributions with coordinates, bounds,
   vertical scale and water levels; capture gameplay views with fixed cameras.
2. Extract the existing Meadows detail coefficient/frequency into named tuning
   values without changing output. Add corresponding commandlet-only preview
   controls with finite/range validation; keep production defaults unchanged.
3. Add the broad hill layer, continuous Meadows blend and coast attenuation.
   Compare conservative parameter trials with the baseline; select the smallest
   amplitude yielding readable connected hills. Preserve hydrology ordering.
4. Add middle-scale rolling relief, then floor detail as separate small passes.
   Inspect local profiles, patch seams, camp pockets and traversal after each.
5. Correct only demonstrated shoreline/clamp or river-bank defects caused by
   the new relief. Preserve outside-influence controls and enclosed-lake levels.
6. Add shared Meadows forest suitability and patchiness, preserving candidate
   identity, harvest suppression, late join and depletion behavior.
7. Add slope/water-aware surface presentation using existing original assets;
   schedule missing ground-cover content separately with a measured budget.
8. Finalize parameters and document the new terrain contract and inspection
   workflow. Run the complete applicable terrain verification before presenting
   the terrain work as accepted.

Likely implementation files: `KalmalaRegionalTuning.h`,
`KalmalaRegionalGeneration.cpp`, `KalmalaGenerationPreview.h/.cpp`,
`Source/KalmalaEditor/Private/ExportWorldMapsCommandlet.cpp`,
`Scripts/WorldMapPreview.params`, `KalmalaForestLayout.cpp`,
`KalmalaGeneratedTerrainPatch.cpp`, relevant focused tests, and the architecture
and setup guides. Change the global field sampler only if baseline evidence
shows a separately justified need.

## Validation and acceptance

For ordinary increments use source inspection, parameter parsing, focused
sampler checks and small preview exports as appropriate. Reserve full builds
and complete prescribed verification for the final implementation increment.
No Unreal build, preview export or gameplay acceptance was run for this plan.

- Compare baseline and proposed local height profiles at identical positions.
  Across representative 100–250 m dry Meadows transects, seek several metres
  of continuous rise/fall; demonstrate smaller undulations on 10–50 m walks.
  Record actual ranges and slope percentiles rather than relying on colour
  contrast. Include multiple sites and seeds, not a single favourable hill.
- Check exact same-seed repeatability, reversed sampling/cache order,
  different-seed variation, negative coordinates, patch edges and biome blends.
  Non-Meadows control sites outside the blend must retain their outputs.
- Keep atlas sea/land sign outside hydrology, distance eligibility, dry central
  start selection, river depth/bank continuity and enclosed-lake acceptance.
  Compare mesh collision, water clipping and map classification at the same
  coordinates, including mixed wet/dry triangles and river/lake overlaps.
- Inspect real rendered open slopes, wooded ground and shores from player
  height. Walk uphill/downhill, place a small camp in a gentle pocket, and
  inspect tree roots, rock contact and swimming transitions. Map exports alone
  cannot accept these outcomes.
- For the final gate, use the applicable checks in
  [development setup](07-development-setup.md): the Editor build, focused
  regional/master-map/finite-world/integrated and water automation,
  `Scripts/Tests/World/Verify-RegionalGeneration.ps1` and
  `Scripts/Tests/World/Verify-Minimap.ps1`. Include forest/harvest lifecycle and
  peer checks when placement changes, plus rendered traversal and start/build
  checks for changed terrain. Inspect every required result, not exit code only.
- Compare initial-generation time, sampler/map cost, active-patch count and
  decoration counts with baseline. Keep the current bounded streaming budget;
  investigate material regressions before accepting the new defaults.

The fast `Scripts/Export-WorldMaps.ps1 -Watch` workflow currently exports atlas,
crop and biome PNGs, not a local height/slope diagnostic. Use the existing full
visualization commandlet's `ShapedHeight.ppm` for current height inspection;
add explicitly scaled local height/slope outputs if needed. Its current fixed
height normalization can saturate after relief increases, so record raw heights
and widen the diagnostic range rather than interpreting clipped colours as flat
terrain. New relief preview controls and outputs must be implemented before use.

All peers derive the same geometry from the server-owned immutable seed and
current build. Gameplay collision, placement, wetness and resource validation
remain server-side. This plan introduces no RPC, replicated parameter, save
schema or migration. Development layouts may change under the current seed-only
contract; use disposable acceptance worlds and do not rewrite existing saves.
