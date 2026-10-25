# Meadows terrain and biome plan

Status: active implementation plan, updated 2026-10-25. BH25a selected the
4 m broad-hill amplitude, BH25b1 selected 0.6 m rolling amplitude, and BH25b2
selected 4 / 0.6 / 0.1 m shared Elderwood/Tundra amplitudes from fresh-source
profiles. BH25c selected the 8 m / 0.1 m Meadows floor detail. BH10a/BH11a
implement the Meadows hill and rolling layers, and BH12a adds the selected
floor-detail layer. BH10b1/BH11b/BH12b provide profile harnesses; remaining
fresh-source tuning is in BH25d–BH25e and aggregate cost contracts are in
BH25b3. Rendered acceptance is in BH26. No gameplay, forest or
presentation changes are implemented by this document. Execution is tracked in
`BACKLOG.md` before Monster Population under the combined biome-height goal.

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
- At baseline commit `460339b3`, `KalmalaRegionalGeneration.cpp` converted
  Elevation to centimetres with `(Elevation - 0.22) * 2000` and used a
  `40 * Detail` Meadows term. BH10a replaced that term with broad hills; BH11a
  and BH12a add separate rolling and floor signals. The old 40 cm coefficient
  is baseline evidence, not the current height contract or a measured terrain
  range. The field's local relief at baseline contributed at most 19.5 cm
  before coast attenuation (`0.015 * 0.65 * 2000`).
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
- `KalmalaForestLayout.cpp` permits the first 18 candidates per Meadows patch.
  The shared descriptor path applies Flora, forest-factor, final-slope and dry-
  ground suitability to both decorative trees and authoritative harvest
  materialization. Dry-ground checks use final regional water state and the
  explicit patch collision-lattice ocean sample; candidate ordinals and IDs are
  retained through rejection.

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
| `MeadowHillAmplitude` | 4 m selected (BH25a) | 4/6/8 m swept; preview accepts 4–10 m | Smallest tested amplitude with multi-metre 100–250 m profiles. |
| `MeadowShoulderHeight` / `MeadowShoulderCompressionScale` | 10 m / 30 m | 6–16 m / 10–60 m | Preserve lower pockets while gradually softening higher Meadows shoulders. |
| `MeadowRollingScale` | 45 m | 30–70 m | Rounded shoulders, smaller valleys and grove undulations. |
| `MeadowRollingAmplitude` | 0.6 m selected (BH25b1) | 0.6–2 m | Smallest tested amplitude with measured 30–70 m profiles and the lowest sampled slope percentiles. |
| `MeadowFloorScale` | 8 m selected (BH25c) | 8–20 m | Gentle forest-floor variation; sampled at least 6.4 times per scale on the 125 cm lattice. |
| `MeadowFloorAmplitude` | 0.1 m selected (BH25c) | 0.1–0.35 m | Small rises and depressions beneath vegetation. |

### BH25a fresh-source hill measurements

The Editor target was freshly compiled from the selected 4 m source setting.
The sweep then overrode the amplitude to 4, 6 and 8 m and exported all 88
samples (eight seed-418/419 profiles, 11 points each, 25 m spacing). The table
reports the minimum, mean and maximum of the eight final-height ranges and the
collision-triangle slope distribution across all 88 samples.

| Hill amplitude | Profile range min / mean / max | Slope p50 / p90 / p95 / max | Samples <15° / 15–25° / >25° |
| --- | --- | --- | --- |
| 4 m | 2.61 / 5.36 / 12.01 m | 2.38 / 13.08 / 16.73 / 30.40° | 90.9% / 4.5% / 4.5% |
| 6 m | 2.68 / 5.75 / 12.83 m | 3.13 / 12.74 / 16.48 / 30.79° | 89.8% / 5.7% / 4.5% |
| 8 m | 2.78 / 6.31 / 13.67 m | 3.61 / 13.23 / 16.23 / 32.12° | 90.9% / 4.5% / 4.5% |

The 4 m trial gives at least 3.48 m of variation in seven of eight profiles;
the remaining outer north–south profile spans 2.61 m. Its 90th-percentile
slope is 13.08°, while 4 of 88 samples exceed 25° (including a short paired
shoulder in Starter419NS). Higher amplitudes add mean range but do not reduce
the steep-slope tail. BH25a therefore selected 4 m as the smallest accepted
numeric trial; BH26 must inspect the steep shoulders and connected gentler
routes in the rendered world.

All 28 BH01 fixed-site coordinates were sampled at each trial amplitude. At
the four Starter Meadows and outer dry Meadows sites, 4 m differs from BH01 by
at most 111.7 cm in final height (85.1 cm mean absolute difference) and 2.58°
in collision slope; corresponding maxima are 188.5 cm / 2.78° at 6 m and
263.3 cm / 2.98° at 8 m. The 17 fixed points with zero Meadows weight are
identical across the three overrides. Other weighted fixed points include
later Elderwood/Lakes shaping, so their full BH01 deltas are not attributed to
the hill amplitude.

### BH25b1 fresh-source rolling measurements

The fresh Editor mirror retained the BH25a-selected 4 m hill default. Separate
0.6/1.2/1.8 m rolling overrides each exported 264 samples: 24 fixed 30/45/70 m
profiles and eight patch-side seam points. All exact trial markers were
present. Four confirmed seed-419 river controls remained wet at Z=0, no other
Meadows sample was wet, and each patch seam remained within the 5 cm limit.
The four wet channel beds were retained in raw outputs and excluded from
dry-land range/slope summaries, leaving 260 collision-slope samples per trial.
Outer seed-419 profiles retain continuous Meadows influence down to weight
0.4125; the fixture requires positive weight rather than an artificial 0.5
cutoff.

Final-height ranges include the 4 m broad hills, existing floor detail and
Meadows blending, so they describe the complete terrain profile rather than an
isolated rolling-noise contribution. Each value is the min/mean/max range among
the eight profiles of that length, in centimetres.

| Rolling amplitude | 30 m range | 45 m range | 70 m range |
| --- | ---: | ---: | ---: |
| 0.6 m | 42.4 / 176.2 / 767.8 | 49.5 / 226.8 / 935.1 | 66.4 / 286.3 / 1,074.0 |
| 1.2 m | 50.4 / 179.8 / 744.1 | 52.5 / 233.2 / 921.0 | 65.9 / 297.3 / 1,072.3 |
| 1.8 m | 60.1 / 183.6 / 720.3 | 68.0 / 242.1 / 906.7 | 74.9 / 310.8 / 1,070.6 |

Dry-sample slope p50/p90/p95/maximum was 2.31/4.55/28.07/36.20 degrees at
0.6 m, 2.60/5.29/27.36/35.93 degrees at 1.2 m, and
2.97/6.37/26.64/35.76 degrees at 1.8 m. Twenty-two samples were at least
15 degrees and 16 were at least 25 degrees in each trial. The 0.6 m option
had the smallest median per-profile maximum slope (3.59 degrees) and the
smallest fixed-site maximum slope change (2.04 degrees).

At the four BH01 fixed Starter/outer Meadows sites, maximum/mean absolute
height differences from BH01 were 136.18/90.43 cm at 0.6 m,
111.75/85.13 cm at 1.2 m, and 130.93/79.78 cm at 1.8 m. Maximum absolute
collision-slope changes were 2.04, 2.58 and 3.12 degrees. The 17 other fixed
sites with zero Meadows weight were identical across the three trials.
Maximum terrain/collision seam deltas stayed below 0.06 cm for every amplitude.

BH25b1 selected 0.6 m as the smallest tested amplitude. It retained measurable
dry-land variation in every profile length; larger values modestly increased
mean ranges while raising median and p90 collision slopes.
This numeric choice does not establish rendered readability or traversability;
player-height route inspection remains part of BH26. Raw trial CSVs and logs
were exported to the temporary `brpcff2dd9b` and `bh25b1` folders during the
measurement run.

Prefer broad form first, then rolling relief, then floor detail. Do not maximize
all amplitudes at once. Reject candidates producing noisy corrugations or
uniformly steep ground. As an initial acceptance target, ordinary walking
slopes should mostly sit below 15 degrees, with occasional 15–25 degree
shoulders and connected gentler alternatives. Retain naturally flatter pockets
for camps; do not flatten the entire starter radius or author fixed camp sites.
Evaluate the distribution from final collision triangles and compare against
the actual Character Movement and building-placement limits before finalizing.

### BH25c fresh-source Meadows floor-detail measurements

The current-source Editor mirror was refreshed from the selected worktree
sources and built successfully (24 actions). The BH12b runner then completed all
nine 8/12/20 m by 0.1/0.2/0.3 m trials with the exact effective-parameter marker.
Each output contained 400 rows: 387 dry profile samples, five fixed seed-419
river controls, and eight patch-side controls. The five river controls were
verified separately at sea level with no basin or stream weight; the runner
requires them to remain wet and excludes them only from dry-floor metrics. The
12 outer seed-419 rows below 0.5 Meadows weight retain positive Meadows blend
and remain in the dry profile metrics. All other samples remained dry, all
profiles advanced one 125 cm collision cell at a time, and all four patch seams
passed the 5 cm bound.

| Scale / amplitude | Floor input min–max (cm) | Max adjacent input step (cm) | 60 m profile height-span min / mean / max (cm) | Max final adjacent step (cm) | Collision slope p50 / p95 / max (degrees) | Max height / collision seam delta (cm) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 m / 0.1 m | −6.726 to 6.497 | 2.765 | 43.036 / 249.158 / 1,143.148 | 72.396 | 1.751 / 28.043 / 34.523 | 0.026 / 0.028 |
| 8 m / 0.2 m | −13.453 to 12.995 | 5.530 | 47.587 / 250.410 / 1,140.367 | 73.775 | 2.021 / 28.129 / 34.614 | 0.033 / 0.029 |
| 8 m / 0.3 m | −20.179 to 19.492 | 8.296 | 52.141 / 252.633 / 1,137.583 | 75.154 | 2.455 / 28.117 / 34.894 | 0.040 / 0.036 |
| 12 m / 0.1 m | −5.812 to 7.661 | 1.786 | 40.231 / 248.509 / 1,146.036 | 71.323 | 1.641 / 27.795 / 34.648 | 0.025 / 0.026 |
| 12 m / 0.2 m | −11.624 to 15.323 | 3.572 | 43.380 / 249.180 / 1,146.145 | 70.796 | 1.868 / 27.654 / 34.648 | 0.023 / 0.024 |
| 12 m / 0.3 m | −17.435 to 22.984 | 5.358 | 50.684 / 250.681 / 1,146.253 | 70.269 | 2.132 / 27.513 / 34.649 | 0.028 / 0.023 |
| 20 m / 0.1 m | −5.623 to 5.714 | 1.313 | 47.135 / 249.397 / 1,145.873 | 72.375 | 1.622 / 27.937 / 34.781 | 0.022 / 0.023 |
| 20 m / 0.2 m | −11.247 to 11.427 | 2.626 | 54.135 / 250.599 / 1,145.819 | 72.900 | 1.595 / 27.917 / 34.913 | 0.025 / 0.020 |
| 20 m / 0.3 m | −16.870 to 17.141 | 3.939 | 61.171 / 251.958 / 1,145.764 | 73.424 | 1.626 / 27.926 / 35.045 | 0.029 / 0.024 |

Final-height spans, adjacent final steps and collision slopes include the
existing hill, rolling and regional relief; only the floor contribution range
and its adjacent step isolate this layer.

The selected 8 m / 0.1 m scale contributes a 13.223 cm peak-to-peak range;
its maximum adjacent input step is 2.765 cm over 125 cm (about 1.27 degrees of
local floor-only grade). The sampled floor noise ranges 1.322 across the dry
profiles. Collision slope percentiles and maximum vary only modestly across
the matrix, with no scale-linked lattice spike; the selected scale has 6.4
collision intervals per scale length. The source's fixed BH01 anchor samples
remain dry and retain their biome labels. At all 28 BH01 sites, selected 8 m /
0.1 m versus the retained pre-floor-selection current-source export changes no
water or biome state; height deltas range −4.105 to +4.799 cm, maximum
collision-height delta is 4.661 cm, and maximum slope delta is 0.942 degrees.
Two BH01 water flags differ from the selected output, but those sites were
already dry in the current-source control and their state did not change with
floor selection. These numeric results select the smallest tested scale and
amplitude without evidence of numerical aliasing; rendered readability beneath
vegetation remains part of BH26 and is not claimed here.

The selected defaults are compiled into `KalmalaRegionalTuning.h`; focused
source assertions now require 8 m / 0.1 m. The corrected fixed fixture uses the
repository's unquoted simple-CSV convention, while the runner retains the five
confirmed river controls in raw output and removes only those coordinates from
dry-floor ranges.

### Coast, rivers and lakes

Insert the Meadows relief before the existing basin and channel operations.
Retain this order: source and biome relief, analytic basins, optional debug
streams, source-relative coast sign transition, final priority river carving. Preserve sea-level
river water at Z=0, `RiverBedDepth=80` cm, `RiverBankExtent=1800` cm and existing
lake-support precedence in the first terrain pass. Stronger land relief must
not refill a river or flatten an elevated enclosed lake to sea level.

Fade new relief continuously toward the atlas shore. The fixed 1 cm source-land
floor made a long shelf on the seed-419 WoodedElderwood profile during BH25b2.
The sampler now uses a smooth positive softplus coast transition with a maximum
300 cm scale reduced in proportion to source-land clearance; the scale reaches
zero at the source coast. This preserves land sign without a constant-height
shelf. River carving also fades its target from the positive bank floor to the
full bed by narrow channel weight, so the wide bank envelope cannot create new
sea-level water where RiverWeight is zero. The fresh-source profiles verified
the six BH01 center water states and found no other wet sample or basin support.

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
- Blend the existing generated grass and rock palettes with an original
  project-generated soil material using final slope and moisture context:
  exposed shoulders receive more soil/rock, gentle ground more grass, and wet
  banks reduced terrestrial grass. Do not add grass or reed assets in this
  material increment.
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
   Keep the hill amplitude provisional and preserve hydrology ordering. BH10b1
   adds the reproducible profile harness; after all terrain layers are
   integrated, BH25 compares freshly compiled 4/6/8 m trials with the fixed
   baseline in BH25a and selects the smallest amplitude yielding readable
   connected hills.
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
BH25a ran a fresh targeted Editor build and numeric-only 4/6/8 m commandlet
profiles; it did not run rendered preview or gameplay acceptance. BH26 retains
the full integrated and rendered gate.

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
  `Scripts/Tests/World/RegionalGeneration.ps1` and
  `Scripts/Tests/World/Minimap.ps1`. Include forest/harvest lifecycle and
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
