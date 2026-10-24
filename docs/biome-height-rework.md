# Biome height rework plan

Status: user-requested design direction, 2026-10-24; implementation is in
progress under `BACKLOG.md` after A10 acceptance and before Monster Population.
The fixed baselines, basin policy, shared sampler, preview controls and Meadows
hill/rolling/floor layers are implemented incrementally; fresh-source floor
tuning remains BH25c and final acceptance remains pending BH26. This plan
supersedes Meadows-specific height details where they differ.

This plan expands [the Meadows plan](meadows-plan.md) to all land biomes. Where
they differ, this plan takes precedence: broad relief becomes a shared stack,
biomes receive separate height functions, Tundra gains an Elderwood-like forest,
and enclosed lakes are disabled. The screenshots in the Meadows plan remain
references for spatial relationships. Implement original Kalmala algorithms and
content; the earlier Valheim comparison is inspiration, not a code specification.

## Requested outcome

| System / biome | Planned treatment | Visible result |
| --- | --- | --- |
| Broad terrain | Combine several noise scales into base height. | Broad rises, valleys, rolling shoulders and local detail coexist. |
| Biome height | Evaluate a separate height function per biome, then blend. | Terrain character changes with the biome, without border steps. |
| Forest patches | Add an independently seeded forest-factor noise signal. | Coherent groves, forest interiors and clearings instead of uniform scatter. |
| Meadows | Base relief plus detail; selectively soften higher ground. | Rounded, walkable hills with flatter pockets and sustained inclines. |
| Elderwood | Base relief plus detail, without Meadows softening. | Stronger wooded slopes and hollows on the same underlying landform. |
| Shimmering Lakes | Combine Meadows softness, Elderwood relief and Mire lowland shaping. | Gentle wooded uplands flowing into broad low pockets; no enclosed lake generation for now. |
| Mossy Mire | Compress height into a narrow lowland profile. | Low, gently uneven ground with much less vertical range than surrounding hills. |
| Freezing Tundra | Reuse Elderwood terrain character farther out, with snow and slightly fewer trees. | A snowy outer forest with modestly more open canopy. |
| Thunder Mountains | Amplify already-high base terrain; add snow and storm presentation. | Pronounced peaks and ridges rather than small bumps on ordinary land. |
| Enclosed lakes | Disable analytic basin shaping and enclosed-lake water generation. | No generated inland bowls, elevated lake sheets or lake shore overlays. Ocean and rivers remain. |

## Current starting point

`KalmalaWorldFieldSampler.h` currently combines broad Elevation noise and a small
local term, then constrains land/water with the master atlas. Regional generation
converts that elevation to height and adds small biome-weighted offsets from a
shared detail signal. Lakes and Mire currently share shaping and analytic bowls.
`KalmalaForestLayout.cpp` generates Meadows/Elderwood trees; only Elderwood has
a Flora density gate. Tundra forest participation therefore requires integration,
not just a different density number.

Keep the master atlas, seed-derived crop/rotation, world radius, existing climate
fields and origin-distance eligibility. In particular: starter land through
350 m remains Meadows, Elderwood starts at 750 m, Lakes spans 350 m–3 km, Mire
3–16 km, Tundra starts at 4 km, and Meadows ends at 4 km. This request changes
height and forest character, not those placement bands or biome names.

## Shared base relief and biome evaluation

1. Retain the existing classification inputs while introducing a separate
   derived base-height stack. Broad noise supplies major landforms, middle
   noise supplies rolling relief, and fine noise supplies ground undulation.
   Keep climate/ecological Elevation distinct from final terrain height so
   profile changes do not accidentally move every biome or cause recursion.
2. Give each layer named frequency and amplitude controls in centimetres.
   Initial scale experiments can span 500–1500 m for broad relief, 80–250 m
   for hills, and 10–50 m for local relief. These are trial coordinate scales,
   not accepted defaults or promised feature spacing. Tune amplitudes against
   profiles, traversal and shoreline behavior; do not maximize every layer.
3. Use deterministic absolute-world sampling with stable domain salts. Apply
   coast attenuation continuously and retain the atlas coast-sign constraint.
   Start with the existing 125 cm collision/render lattice and bounded caches.
4. Implement pure named functions such as `ShapeMeadows`, `ShapeElderwood`,
   `ShapeShimmeringLakes`, `ShapeMire`, `ShapeTundra` and `ShapeMountains`.
   Each returns a complete candidate land height from the same base inputs.
   Blend candidate heights with normalized continuous land-biome weights;
   avoid adding the shared base once per biome or double-applying detail.
5. Keep biome labels selected by the existing classifier. A label boundary
   must not hard-switch geometry. Ocean keeps its master-constrained seabed
   treatment and transitions continuously into land.
6. Apply the existing priority river channel after biome relief and the coast
   clamp. Preserve sea-level water, depth and bank behavior initially. Check
   higher surrounding land does not produce unacceptable bank slopes before
   considering a separately documented bank adjustment.

Keep current hydrology candidate/path inputs stable initially. If new relief
requires different river placement, treat that as a distinct follow-up with
determinism and channel-connectivity acceptance, not an incidental side effect.

## Biome function details

- **Meadows:** Add bounded detail to the shared base. Above a named shoulder
  threshold, progressively reduce the height gain with a smooth monotonic
  compression curve. Preserve lower hollows and broad hills; do not clamp all
  high ground onto a flat shelf. Threshold and compression strength remain
  tunable and must be inspected in cross-section.
- **Elderwood:** Use the same base and a bounded detail layer without the
  Meadows compression curve. Establish this function as the reference for
  Tundra so the two cannot silently diverge into unrelated terrain styles.
- **Shimmering Lakes:** Interpret the requested similarity as a continuous
  mixture: Meadows-style rounded upper shoulders, Elderwood-style undulation,
  and Mire-style compressed lower pockets. Derive the lowland blend from base
  height and existing humidity, not independent square cells. Keep the mixture
  smooth and bounded; select its strengths through previews. This is distinct
  from a bowl generator and must not create new perched water surfaces.
- **Mossy Mire:** Map the base into a narrow, smooth lowland band with small
  residual detail. Choose a coastal target near sea level, but retain positive
  source-land clearance outside river corridors. Do not convert the entire
  biome into ocean, erase banks or introduce an elevated flat water plane.
  Keep the compression curve monotonic to avoid inverted ridges and pits.
- **Freezing Tundra:** Reuse Elderwood height treatment and the existing outer
  eligibility. Reuse its forest-factor pattern with a slightly lower acceptance
  probability; a 0.8–0.9 density multiplier is an initial experiment, not a final
  value. Apply snow through biome/slope presentation, not height offsets. Reuse
  original tree presentation where suitable; do not introduce new timber items,
  rewards or harvesting tiers as part of this terrain work.
- **Thunder Mountains:** Smoothly increase gain above a named high-base-height
  threshold, retaining the foothill connection. Amplify existing high relief
  rather than raising every mountain-labelled pixel equally. Bound fine detail
  to avoid spikes. Add snow coverage and storm presentation using existing
  original materials and weather paths. Weather selection remains server-owned;
  no new damage, exposure penalty, shelter requirement or survival mechanic is
  implied by the requested snowy/stormy touch.

## Forest-factor distribution

Introduce a continuous, independently seeded forest-factor noise domain, separate
from terrain detail and existing Flora sampling. It is a derived placement mask,
not a replacement for the four ecological fields or a replicated fifth map.
Expose patch scale and coverage controls; retain Flora as a separate suitability
input. A low forest factor should create clearings rather than shorter trees.

Evaluate a shared deterministic acceptance rule from forest factor, biome density,
final slope and dry-ground suitability. Feather transitions with stable per-ID
random thresholds. Preserve candidate ordinals, persistent identities and random
draw independence when candidates are rejected. Use the same descriptors for
cosmetic forest, authoritative harvest materialization and suppression. Audit
the existing repeated patch candidate pattern before changing position sampling;
world-space forest noise must not reset at patch boundaries.

Add Tundra participation explicitly, including a stable biome/presentation choice
that does not accidentally mark every Tundra tree as an Elderwood-only resource.
Keep current resource/catalogue rules explicit and consistent across rendering,
harvesting and persistence. Start with existing candidate/streaming budgets;
lower Tundra density relative to comparable Elderwood areas, not all forest
density globally. Shimmering Lakes can reuse the shared forest-factor machinery
for wooded shoulders, subject to existing content rules and wet-ground rejection.

## Disable enclosed lakes consistently

Use one compiled generation-policy switch, initially disabled, shared by sampler
and consumers. This is not a player setting or a new save option.

- Bypass analytic bowl/rim height modification, basin water-level assignment,
  and positive basin weights in `KalmalaRegionalGeneration.cpp`.
- Ensure `KalmalaShimmeringLakeSampler`, `KalmalaLakeBasin`, water/shore meshes,
  maps and gameplay wetness consumers cannot resurrect a lake from biome or
  humidity alone. Clear/invalidate derived basin caches when policy changes.
- Preserve the Shimmering Lakes biome label, climate suitability and placement
  band. Its name does not guarantee standing inland water during this phase.
- Keep sea water, carved river water, shoreline clipping and server swimming/
  wetness decisions working. With basin support absent, river targets use their
  ordinary sea-level behavior. Do not enable small streams as a substitute.
- Disable dedicated enclosed-lake generation, not every isolated sea-level wet
  polygon that can naturally result from existing coast or river geometry.
  If such artifacts become problematic, diagnose them rather than reintroducing
  basin water or silently changing the atlas.
- Retain reusable lake code and focused tests for future re-enablement. Update
  active acceptance to expect no basin-generated water, and clearly distinguish
  disabled-feature tests from current production behavior. No save migration or
  rewriting existing world files is planned.

## Implementation order

Each numbered item is a proposed increment; split larger ones before execution
and follow the one-increment-per-run protocol when this work is scheduled.

1. Capture fixed-seed baselines for height, slope, biome weights, water and tree
   counts at representative sites and boundaries. Record camera positions.
2. Add the shared enclosed-lake policy and disable bowl/water behavior across
   sampler and consumers, retaining ocean/rivers and biome identity.
3. Extract named biome height functions with output-preserving defaults.
4. Introduce the shared multiscale base-height stack and validated preview
   controls, keeping classification inputs and river paths stable.
5. Integrate Meadows softening; inspect hill shoulders and buildable pockets.
6. Integrate Elderwood relief without the softening curve.
7. Integrate Mire compression and inspect river/coastal transitions.
8. Integrate the mixed Shimmering Lakes profile without enclosed basins.
9. Integrate Tundra's shared Elderwood height function.
10. Integrate high-base Mountain amplification and check foothill continuity.
11. Add the independent forest-factor signal and shared placement acceptance.
12. Integrate Tundra forest participation and reduced density consistently with
    resource descriptors, suppression and persistence.
13. Integrate snow and storm presentation in small passes using existing
    materials/weather contracts; inspect forest and ground presentation.
14. Finalize tuning, update architecture/setup documentation, and complete the
    applicable combined verification before declaring the rework accepted.

## BH01a map overview exports and capture sites

The existing `RenderWorldGenerationVisualization` commandlet was run against
baseline commit `460339b3` for each fixed seed, with `-Size=2048`
(2048×2048 pixels), `-Extent=3200000` cm (32 km full width, centered on the
world origin), and `-SkipSplineOverlay`. Outputs are in the ignored temporary
folders `.tmp/bh01/seed418` and `.tmp/bh01/seed419`; each contains the commandlet
PPMs and `Tuning.txt`. Each P6 PPM is 12,582,929 bytes. The full-world crop
radius is 1,600,000 cm (16 km).
The world coordinate of a pixel center is
`X=(pixelX/2047−0.5)×3,200,000 cm` and likewise for Y. Pixel spacing is
1,563.263 cm (15.63 m); each selected point is a map-pixel candidate with up
to 781.6 cm uncertainty per axis (not yet an exact sampler validation).

The exports were visually inspected as composite biome, height, hydrology and
land/water views. `BiomeClassification.ppm` uses the commandlet palette and
`Weight0..Weight6.ppm` map byte-scaled weights in this order: Meadows,
Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, Thunder Mountains,
Ocean. `ShapedHeight.ppm` is a diagnostic grayscale, clamped from
`(Region.Height + 500) / 2500`; it is not a raw height measurement. The
hydrology image stores river, stream and basin weights in R/G/B. Consequently
these overview images select sites but do not establish final height, slope,
water levels or tree counts; BH01b will measure those at the fixed coordinates.

| Representative condition | Seed 418 X,Y (cm) | Seed 419 X,Y (cm) | Map selection |
| --- | --- | --- | --- |
| Starter Meadows | (-781.6, -781.6) | (-781.6, -781.6) | Nearest pixel center to origin, inside the 350 m starter band |
| Outer dry Meadows | (208695.7, -293111.9) | (-355642.4, 55495.8) | Meadow class and source-land pixel near 3.6 km, below the 4 km eligibility edge |
| Shimmering Lakes | (100830.5, -143038.6) | (-166487.5, 53932.6) | Classified pixel near 1.75 km |
| Elderwood interior | (91450.9, -205569.1) | (-205569.1, 91450.9) | Classified pixel near 2.25 km |
| Wooded Elderwood | (207132.4, -325940.4) | (294675.1, 171177.3) | High-Flora Elderwood pixels; map Flora shades 252 and 236 |
| Mossy Mire | (-669858.3, -601074.7) | (-78944.8, 896531.5) | Classified pixels near 9 km |
| Freezing Tundra | (-243087.4, -970004.9) | (-243087.4, 970004.9) | Classified pixels near 10 km |
| Thunder Mountains | (-970004.9, -243087.4) | (-970004.9, 243087.4) | Classified pixels near 10 km |
| Ocean | (970004.9, -243087.4) | (-243087.4, -970004.9) | Ocean-class pixels near 10 km |
| Meadows/Elderwood transition | (-182120.2, 171177.3) | (-193063.0, 158671.2) | Weight0/Weight2 ≈ 0.502/0.498 and 0.498/0.502 |
| Elderwood/Tundra transition | (-191499.8, -773033.7) | (314997.6, 735515.4) | Near-equal Weight2/Weight4 land pixels |
| Tundra/Mountains transition | (-268099.7, 963751.8) | (1001270.2, 7034.7) | Weight4/Weight5 ≈ 0.498/0.502 and 0.502/0.498 |
| Source coast, land side | (691744.0, -107083.5) | (-71128.5, -696433.8) | Source-land pixels adjacent to ocean pixels |
| Highland river | (679237.9, -16414.3) | (1145090.4, -66438.7) | River weight byte 255 and height-map shade 172 in each seed |

BH01c uses the same relative camera pose at every site: for land, anchor at the
BH01b final collision height; for visible water, anchor at its measured water
surface (ocean uses Z=0). Put the camera at `(X, Y−2400, Z+1200)` cm, aim at
`(X, Y, Z+150)` cm, and use a 65° horizontal field of view. This is a fixed
+Y-facing view with about 23.6° downward pitch. The BH01b numeric CSV resolves each per-site anchor: Ocean uses Z=0,
water-present non-ocean samples use WaterLevelCm, and dry land uses the collision-triangle height.

SHA-256 checksums of the principal source PPMs (the temporary exports are not
committed):

| Export | Seed 418 SHA-256 | Seed 419 SHA-256 |
| --- | --- | --- |
| `BiomeClassification.ppm` | `EB8D2BD4CF360095C452033DBF9DDAAF9665C9F71D59C4665D58E79D4B4D7072` | `7AF9963F490D0067AAD9A0DFF4E60327A439ADA8EDA96F5E4866863FE1746B7D` |
| `ShapedHeight.ppm` | `D4E7A9AA5D5099D8221C862FED0F4F80B17ABF5CC3CA9F34328615CE1DE18F54` | `9DA6668112DDD0A3625F5502B7422BF94685D14AA3C0981BA372E13878B1BD2C` |
| `Hydrology.ppm` | `2775ADD4C62014D81D8452F3D4FD145BD6A4867F52023DE68DCEDD5A48B743F7` | `FC82D5E76BCA8E3AA30E4D49528CD6DC23A9BC09BB11A63E3FE6F9ED1F894AC4` |
| `LandWaterCrop.ppm` | `B18F15BED3A9B904A433281DDF4DBDDA76D829946BF41B3C5376A0E659B9C82D` | `B8CEBA0E7DEFDCC7C4472A8CD645FE04C86B0539A170CDA7071E7349B6631CAB` |

Both commandlet exports completed successfully with zero logged errors. Each
logged the same pre-existing warning: `Default__KalmalaOceanSkiff:HullMesh:
Material index 0 is invalid`. No generation code was changed for these captures.

## BH01b1 scalar sample export

`RenderWorldGenerationVisualization` now accepts optional
`-SampleInput=<csv> -SampleOutput=<csv>` arguments. Supplying `SampleInput`
selects a sample-only path before map allocation; invocations without it retain
the ordinary overview behavior. Input has the header `Seed,Site,Xcm,Ycm`, then
one fixed coordinate per row. The output includes the same seed/site/coordinates,
`BiomeIndex`, `FinalHeightCm`, all seven normalized biome weights,
`WaterLevelCm`, and `HasWater`. Biome indices follow the `Weight0..Weight6` order
above. Height, water level and coordinates are in centimetres; weights are
unit fractions. Malformed rows or non-finite sample values fail the commandlet.

This scalar output reads `FKalmalaWorldFieldSampler` and
`FKalmalaRegionalGeneration` only; it does not alter generation. Exact
piecewise-collision triangle height/slope and deterministic forest-descriptor
counts remain in BH01b2. BH01c will record the resulting numeric table and
rendered views.

## BH01b2a collision and forest output fields

The optional sample CSV now appends the piecewise terrain mesh height and
slope at each requested coordinate, plus its containing patch metadata and
deterministic forest descriptor count. Runtime patch origin is the seed-derived
player-start XY. PatchX/PatchY use the existing floor-based
FKalmalaTerrainPatchLayout rule; PatchCenterXcm/PatchCenterYcm identify
the selected patch center.

The terrain uses the runtime 3,000 cm square patch and its 24x24 cell grid
(125 cm per cell). Each vertex is sampled with
FKalmalaTerrainHeightSampler::SampleHeight, matching the generated mesh.
PatchCellX/PatchCellY are zero-based coordinates in that patch grid.
PatchTriangle is 0 for the first emitted triangle
(BottomLeft, TopLeft, BottomRight) when local U+V <= 1, or 1 for the second
(BottomRight, TopLeft, TopRight). The recorded height is barycentric
interpolation on that exact triangle. Slope is the magnitude of that triangle
plane's XY height gradient converted to degrees; grade percent is the same
gradient magnitude multiplied by 100. At the shared diagonal, triangle 0 is
reported consistently.

ForestDescriptorCount is FKalmalaForestLayout::Generate's deterministic
descriptor count for the containing patch and seed. It is measured before
saved harvest suppression and actor/materialization state; it does not claim
that every descriptor currently renders. This diagnostic is output-only and
does not change generation. BH01b2b below records its 28-row run and
validation; the numeric measurements were produced in that increment, not by
BH01b2a.

## BH01b2b fixed-site numeric baseline

Built the Editor target in the disposable source-matched project
.tmp/bh01/m/Kalmala.uproject; 55 UnrealBuildTool actions succeeded. The
project mirror kept build outputs under .tmp/bh01/m, leaving the repository
Binaries, Intermediate, Saved and DerivedDataCache directories untouched.
UnrealBuildTool had normal LOCALAPPDATA access.

The updated RenderWorldGenerationVisualization commandlet ran in null-RHI mode
with the 28-row input .tmp/bh01/samples.csv. It wrote 28 data rows and 28
columns to the ignored temporary output .tmp/bh01/numeric.csv (5,563 bytes).
Its SHA-256 is
48d7315a014a9b33664fd1aa52eb2f574cdfcefe054c50e6b8cba7cc115c5eaf.
A second commandlet invocation wrote a byte-identical CSV with the same hash.
The output and logs remain under .tmp/bh01/ for BH01c; they are not committed.

Validation matched every seed, site and input coordinate: 14 sites each for
seeds 418 and 419. All 28 numeric rows are finite; the seven weights are within
[0,1] and sum to 1 within 0.000002. BiomeIndex follows the regional sampler's
land-weight maximum among indices 0-5 with its existing tie order; Ocean index
6 is selected by the sampler's explicit sea-elevation branch. Water presence
matches WaterLevelCm > FinalHeightCm. Patch centers match the 3,000 cm patch
layout relative to the seed-derived player start; all containing-patch,
24x24-cell and triangle indices are consistent. Slope is in [0,90), grade is
nonnegative, and the slope/grade formula agrees in every row. Forest descriptor
counts are integers in 0-72, range from 0 to 32 in these samples, and repeat
identically.

Fixed-site ranges are: final sample height -149.225 to 1,448.65 cm; water level
-29.3773 to 356.951 cm; collision-triangle height -149.224 to 1,448.74 cm;
slope 0 to 29.3808 degrees; grade 0 to 56.3029 percent. Seven rows report
water. The mesh-triangle height minus the continuous sampler height ranges from
-1.237548 to +1.237548 cm. BiomeIndex counts for 0 through 6 are 5, 2, 8, 3, 3,
5, 2.

Ten additional probes in continuity_input.csv/continuity_output.csv test both
triangle sides and the exact diagonal, plus the +X patch seam for each seed.
Triangle 0/1 heights meet within a slope-based displacement bound. Across the
seam from patch X=0 to X=1, the height deltas over 2 cm are 0.152771 cm
(seed 418; bound 0.305343 cm) and 0.053220 cm (seed 419; bound 0.070244 cm).
The commandlet logs report zero errors and the same OceanSkiff hull-material
warning recorded during BH01a: Default__KalmalaOceanSkiff:HullMesh: Material
index 0 is invalid.

This increment adds baseline evidence only; generation is unchanged. BH01c
still needs rendered fixed-camera views and the consolidated camera/numeric
record before BH01 can close.

## BH01c1 runtime rendered capture fixture

Added the development-only `-KalmalaBiomeBaselineCapture` path in
`AKalmalaGameMode` and the one-site runner
`Scripts/Tests/World/BiomeBaselineCapture.ps1`. The runner launches the actual
`-game -RenderOffscreen` viewport from a supplied project mirror, sets the
requested fixed site pose, then requires the matching seed/site runtime log and
a 1024x768 PNG with varied sampled pixels. It rejects missing, malformed,
wrong-size, or blank output. The null-RHI visualization commandlet is not used
for these images.

The seed-418 Starter Meadows smoke view is
`.tmp/bh01/captures/418/StarterMeadows-camera-only.png` (921,694 bytes; SHA-256
`25c17c4f8bdd07b71513891202209a84d8cfec717db6c181810a8564bc5e5912`). The
runtime log confirms world seed 418, coordinate (-781.6, -781.6) cm, camera
(-781.6, -3181.6, 1728.94) cm, aim (-781.6, -781.6, 678.94) cm, and 65-degree
horizontal FOV. ViewZ 528.94458 cm is the fixed site's measured collision
triangle height. Visual inspection confirms rendered terrain, tree geometry,
shadows and sky; this smoke capture does not claim the other 27 views.

The capture fixture runs only in non-shipping builds with the explicit flag and
waits until `BeginPlay` has copied the initialized world configuration. It
changes the standalone capture camera and explicitly activated terrain patches;
it does not alter ordinary gameplay behavior, save data, RPCs or networking
code. BH01c2a and BH01c2b now preserve the full numeric CSV and all 14
seed-418 live views; c2c and c2d now complete all 14 seed-419 views
and the final manifest.

## BH01c2a durable numeric baseline and first seven seed-418 captures

The validated 28-site numeric output is tracked as
Scripts/Tests/World/BiomeBaseline418419.csv (28 data rows, 28 columns,
5,563 bytes; SHA-256
48d7315a014a9b33664fd1aa52eb2f574cdfcefe054c50e6b8cba7cc115c5eaf). It is
byte-identical to the BH01b2b commandlet output, so all measured rows remain
reviewable after ignored .tmp outputs are removed. The c2a recheck matched all
28 seeds/sites/coordinates, found only finite numeric values, confirmed seven
biome weights in [0,1] summing within 0.000002 of one, and matched each water
flag against WaterLevelCm > FinalHeightCm. The previous BH01b2b checks on biome
identity, triangle continuity, slope/grade, and deterministic forest counts
remain applicable because the file hash is unchanged.

Across the 28 fixed samples, final sampled height ranges from -149.225281 to
1,448.64868 cm; collision-triangle height ranges from -149.223587 to
1,448.74426 cm; water level ranges from -29.377294 to 356.950745 cm; slope
ranges from 0 to 29.380751 degrees; and grade ranges from 0 to 56.302853
percent. Seven rows have water; forest descriptor counts range from 0 to 32.
Biome index counts 0 through 6 are 5, 2, 8, 3, 3, 5, 2. These are
current-generator measurements, not trial terrain settings.

Camera anchors follow the fixed BH01a rule: Ocean uses Z=0; a water-present
non-ocean sample uses its measured WaterLevelCm; otherwise use
CollisionTriangleHeightCm. At each anchor Z, place the camera at
(X, Y−2400, Z+1200) cm, aim at (X, Y, Z+150) cm, and use 65° horizontal
FOV. From all 28 numeric rows, the planned camera bounds are X
-970,004.9..1,145,090.4 cm, Y -972,404.9..967,604.9 cm, and Z
1,200..2,648.74426 cm; aim Z ranges 150..1,598.74426 cm. These bounds are
derived from all 28 numeric rows; the validated live captures across
c2a–c2d match these camera and aim bounds.

The first seven seed-418 rows were captured by the live -game -RenderOffscreen
runner. Each runtime log matched seed, site, XY, chosen anchor, camera, aim, FOV,
and output path within 0.2 cm for Unreal's float-rounded coordinates. Every PNG
is 1024×768, passed the sampled-pixel nonblank check (at least eight distinct
sampled colors and 40 non-black samples), and was visually reviewed. Screenshots
and logs remain ignored under .tmp/bh01/captures; no generated image is
committed.

| Site | Anchor source / ViewZ (cm) | Fixed XY (cm) | Camera (X,Y,Z cm) | Output file | PNG bytes | SHA-256 |
| --- | --- | --- | --- | --- | ---: | --- |
| Starter Meadows | Collision triangle / 528.94458 | (-781.6, -781.6) | (-781.6, -3181.6, 1728.94458) | StarterMeadows-c2a.png | 921694 | 25c17c4f8bdd07b71513891202209a84d8cfec717db6c181810a8564bc5e5912 |
| Outer dry Meadows | Collision triangle / 662.1427 | (208695.7, -293111.9) | (208695.7, -295511.9, 1862.1427) | OuterdryMeadows-c2a.png | 952607 | 7fa19c4a238cc86ce6cbbc4dcc16012065a82034f507f152018e394073754ba7 |
| Shimmering Lakes | Water level / 356.950745 | (100830.5, -143038.6) | (100830.5, -145438.6, 1556.950745) | ShimmeringLakes-c2a-water.png | 797043 | d9453364e88b4d66b88134db455e687656432df5462976b78f0caa9aba4958c8 |
| Elderwood interior | Water level / 0 | (91450.9, -205569.1) | (91450.9, -207969.1, 1200) | Elderwoodinterior-c2a-water.png | 976670 | e46b74324ab84747afb3ce36116e37356194414843c6cce37f27cf227e71412a |
| Wooded Elderwood | Collision triangle / 15.1806488 | (207132.4, -325940.4) | (207132.4, -328340.4, 1215.1806488) | WoodedElderwood-c2a.png | 895744 | 98406f6afdaa0e5c8412820e900150dcd8d4da9c251ffcd7aade6a97c401ea5c |
| Mossy Mire | Collision triangle / 399.156616 | (-669858.3, -601074.7) | (-669858.3, -603474.7, 1599.156616) | MossyMire-c2a.png | 874637 | 9bcf6b05a7ffc6e5796ed1be4cdc250df99ba8aea1d38b94a16e0f10eceb0343 |
| Freezing Tundra | Collision triangle / 824.412842 | (-243087.4, -970004.9) | (-243087.4, -972404.9, 2024.412842) | FreezingTundra-c2a.png | 854300 | 3946011d7db9ceda319b69e53ac4de05cf814068f986d6af0a70cc5ce5f105bd |

The first Shimmering Lakes and Elderwood interior attempts used the dry-land
anchor; those PNGs are marked invalid-triangle-anchor in .tmp and are not
listed above. Their replacements use the measured water surface. The images
record current prototype rendering and are not visual water/material acceptance;
c2d has completed the final seven seed-419 views and the complete image manifest.

## BH01c2b remaining seven seed-418 rendered captures

Captured the other seven seed-418 fixed sites: Thunder Mountains, Ocean,
Meadows/Elderwood transition, Elderwood/Tundra transition, Tundra/Mountains
transition, source coast land side, and highland river. Anchors use the
collision-triangle height on dry land, measured WaterLevelCm for wet non-ocean
samples, and Z=0 for Ocean. These images complete the 14 seed-418 views; all
14 seed-418 poses now have live-render captures.

Each of the seven runtime logs matched seed, site, fixed XY, anchor height,
camera, aim, FOV and output filename within 0.2 cm for Unreal float rounding.
Every PNG passed the runner's PNG-signature check, is 1024×768, and passed the
sampled-pixel diversity/nonblank check. All seven images were visually
inspected. Capture files and logs remain under ignored
.tmp/bh01/captures/418 and are not committed.

Across these 14 fixed seed-418 sites, camera bounds are X
-970,004.9..970,004.9 cm, Y -972,404.9..961,351.8 cm, and Z
1,200..2,573.79517 cm; aim Z ranges 150..1,523.79517 cm. The shared horizontal
FOV is 65°. The images document current prototype rendering, not water/material
acceptance. In the Ocean, source-coast and highland-river frames, no separate
water surface is visually distinguishable.

| Site | Anchor source / ViewZ (cm) | Fixed XY (cm) | Camera (X,Y,Z cm) | Output file | PNG bytes | Sampled colors | SHA-256 |
| --- | --- | --- | --- | --- | ---: | ---: | --- |
| Thunder Mountains | Collision triangle / 767.497437 | (-970004.9, -243087.4) | (-970004.9, -245487.4, 1967.497437) | ThunderMountains-c2b.png | 786797 | 408 | 65bd9b3c5a17b02486996b4e720aad1b3d9f388ad9d3a0a539bc002343c2f406 |
| Ocean | Ocean level / 0 | (970004.9, -243087.4) | (970004.9, -245487.4, 1200) | Ocean-c2b-water.png | 773402 | 418 | 53ba17d0dc1473348a5eeada20b967bad618a05cfdecc646fe7bd7b11182b707 |
| Meadows/Elderwood transition | Collision triangle / 302.549988 | (-182120.2, 171177.3) | (-182120.2, 168777.3, 1502.549988) | MeadowsElderwoodtransition-c2b.png | 947508 | 472 | a26f8e5e30efdd48e888557ebc7f772ca0235223fdf5c4c743999efbd52cf799 |
| Elderwood/Tundra transition | Collision triangle / 1373.79517 | (-191499.8, -773033.7) | (-191499.8, -775433.7, 2573.79517) | ElderwoodTundratransition-c2b.png | 825175 | 492 | bcdb7fb48bfde5f2ae4d7196aa15befbd30dc8dfc0eb33a96c2b0caef167ed99 |
| Tundra/Mountains transition | Collision triangle / 960.330872 | (-268099.7, 963751.8) | (-268099.7, 961351.8, 2160.330872) | TundraMountainstransition-c2b.png | 831080 | 397 | 3e29bb7059cc5b069d12cc5716c5675efd54cb4aace9dbd6decbae0f7a880d38 |
| Source coast land side | Water level / 0 | (691744.0, -107083.5) | (691744.0, -109483.5, 1200) | Sourcecoastlandside-c2b-water.png | 809491 | 419 | fc5cb1dfdb6a8ab04a103fae9b2340bf1bc4afa8f67d4fc9f265562e6b746b20 |
| Highland river | Water level / 0 | (679237.9, -16414.3) | (679237.9, -18814.3, 1200) | Highlandriver-c2b-water.png | 953864 | 505 | ad8584b626ac0fa05bd4c93154544a8f7e75cf02fc08735a6c2e90bed6211757 |

BH01c2c and c2d together captured all 14 seed-419 sites; c2d validates
the complete 28-image manifest.

## BH01c2c first seven seed-419 rendered captures

Captured the first seven seed-419 fixed sites: Starter Meadows, Outer dry
Meadows, Shimmering Lakes, Elderwood interior, Wooded Elderwood, Mossy Mire,
and Freezing Tundra. All seven numeric rows report HasWater=0, so the fixed
view anchor is the measured collision-triangle height, including Mossy Mire
where the measured water level is below the sampled surface. Cameras use
(X, Y−2400, AnchorZ+1200) cm, aim at (X, Y, AnchorZ+150) cm, and use a
65° horizontal field of view.

Each saved runtime log matched seed 419, site, fixed XY, anchor/ViewZ, camera,
aim, FOV and output path within 0.2 cm for Unreal float rounding. Every PNG
passed the signature check, measured 1024×768, and passed the sampled-pixel
nonblank check. All seven frames were visually inspected; they show the current
neutral prototype terrain/tree geometry, lighting and sky. These baseline
images are not art or terrain acceptance. Images and logs remain ignored under
.tmp/bh01/captures/419 and are not committed.

| Site | Anchor source / ViewZ (cm) | Fixed XY (cm) | Camera (X,Y,Z cm) | Aim Z (cm) | Output file | PNG bytes | Sampled colors | SHA-256 |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | --- |
| Starter Meadows | Collision triangle / 895.446655 | (-781.6, -781.6) | (-781.6, -3181.6, 2095.446655) | 1045.446655 | StarterMeadows-c2c.png | 915101 | 515 | b0f2dcf360d69f230c99279185068154800d52cca6a791f9576878ec67cd81a0 |
| Outer dry Meadows | Collision triangle / 662.852478 | (-355642.4, 55495.8) | (-355642.4, 53095.8, 1862.852478) | 812.852478 | OuterdryMeadows-c2c.png | 962176 | 472 | ba46985e10a6007ae4ecb1b3931fa7805f7464016aadecb1b5492fd1ffb4e6df |
| Shimmering Lakes | Collision triangle / 383.571716 | (-166487.5, 53932.6) | (-166487.5, 51532.6, 1583.571716) | 533.571716 | ShimmeringLakes-c2c.png | 838228 | 408 | 8d9988905562f5724cfeb372fd03755c5fd0ae22b9ef69baf8695d3aa8fc1486 |
| Elderwood interior | Collision triangle / 761.341431 | (-205569.1, 91450.9) | (-205569.1, 89050.9, 1961.341431) | 911.341431 | Elderwoodinterior-c2c.png | 915693 | 543 | 482cd8664439287d49a752fd98ac76d03a15373a04f8dfe88e24581df4473b63 |
| Wooded Elderwood | Collision triangle / 321.026794 | (294675.1, 171177.3) | (294675.1, 168777.3, 1521.026794) | 471.026794 | WoodedElderwood-c2c.png | 881027 | 512 | 3df61a0a4951e49e233eb341cd238f2734497a4cbff18d3cee4199d7fbf8cb8b |
| Mossy Mire | Collision triangle / 1 | (-78944.8, 896531.5) | (-78944.8, 894131.5, 1201) | 151 | MossyMire-c2c.png | 840682 | 430 | dc7dc3605dadc7db308387908da4e0fff1ac5322317a5c9cf85fd603d1f0de7e |
| Freezing Tundra | Collision triangle / 1041.27319 | (-243087.4, 970004.9) | (-243087.4, 967604.9, 2241.27319) | 1191.27319 | FreezingTundra-c2c.png | 898746 | 440 | 74eac6577d25247404db8e1b191496295012e581485b8a44154d0f767e4cb6c0 |

The first seven seed-419 camera bounds are X -355,642.4..294,675.1 cm,
Y -3,181.6..967,604.9 cm and Z 1,201..2,241.27319 cm; aim Z ranges
151..1,191.27319 cm. c2d captured the remaining seven seed-419 live views
and records the completed 28-row/28-image validation below.

## BH01c2d final seven seed-419 captures and complete manifest

Captured the remaining fixed seed-419 sites: Thunder Mountains, Ocean,
Meadows/Elderwood, Elderwood/Tundra and Tundra/Mountains transitions, source
coast land side, and highland river. Anchors follow the shared rule: Ocean at
Z=0; a wet non-ocean sample at WaterLevelCm; otherwise the final collision-
triangle height. Source coast is dry in seed 419 and uses its collision height;
Highland river is wet and uses its visible water level. Cameras use
(X, Y−2400, AnchorZ+1200) cm, aim at (X, Y, AnchorZ+150) cm, and use 65°
horizontal FOV.

Each runtime log matched seed, site, fixed XY, chosen anchor, camera, aim, FOV
and output path within 0.2 cm for Unreal float rounding. All seven images passed
the PNG signature check, measured 1024×768, and passed sampled-pixel diversity
and nonblank checks; each was visually inspected. The Ocean image shows the
prototype checker-grid surface rather than identifiable water, and the source-
coast and highland-river frames do not show a distinct water surface. The
images use current neutral prototype materials/tree meshes and are baseline
evidence, not terrain-art or water acceptance. Images and logs remain ignored
under .tmp/bh01/captures/419 and are not committed.

| Site | Anchor source / ViewZ (cm) | Fixed XY (cm) | Camera (X,Y,Z cm) | Aim Z (cm) | Output file | PNG bytes | Sampled colors | SHA-256 |
| --- | --- | --- | --- | ---: | --- | ---: | ---: | --- |
| Thunder Mountains | Collision triangle / 1448.74426 | (-970004.9, 243087.4) | (-970004.9, 240687.4, 2648.74426) | 1598.74426 | ThunderMountains-c2d.png | 928987 | 449 | 0083abee3f0f648e4e0d59497da83ff773284596b7bc05a9f9f6f951c62eaee6 |
| Ocean | Ocean level / 0 | (-243087.4, -970004.9) | (-243087.4, -972404.9, 1200) | 150 | Ocean-c2d-water.png | 783739 | 409 | ab0d88bdb8ccb49c7b0b5bd9d7f1c5ac0fe336ad84af1e612402667fc3891e1f |
| Meadows/Elderwood transition | Collision triangle / 755.517883 | (-193063.0, 158671.2) | (-193063.0, 156271.2, 1955.517883) | 905.517883 | MeadowsElderwoodtransition-c2d.png | 815631 | 480 | 77f7715d309da12126491f2d1df89c6ca49dfd6d1107560aa425b8caaec6f1c8 |
| Elderwood/Tundra transition | Collision triangle / 821.890747 | (314997.6, 735515.4) | (314997.6, 733115.4, 2021.890747) | 971.890747 | ElderwoodTundratransition-c2d.png | 884867 | 511 | a63cab0e050ed06e8d2b74ef439411b1fb8bdd71582a2806e51c6f171b9a5baa |
| Tundra/Mountains transition | Collision triangle / 296.832581 | (1001270.2, 7034.7) | (1001270.2, 4634.7, 1496.832581) | 446.832581 | TundraMountainstransition-c2d.png | 864211 | 501 | fa0e90e10888bd76ac6de0cde3556b75fb15816aa29f7cad7b4103164e8a05bb |
| Source coast land side | Collision triangle / 0.926416874 | (-71128.5, -696433.8) | (-71128.5, -698833.8, 1200.926416874) | 150.926416874 | Sourcecoastlandside-c2d.png | 793924 | 399 | e32324d04d86cc71078c178b5bcf6d763a230dc6161ee7652bf586d5ae104007 |
| Highland river | Water level / 0 | (1145090.4, -66438.7) | (1145090.4, -68838.7, 1200) | 150 | Highlandriver-c2d-water.png | 480880 | 267 | 0f031de1adad26f58050a80ff14a379a29f3ca8c61c5c3cd29c0054c4452888f |

The seven c2d camera bounds are X -970,004.9..1,145,090.4 cm,
Y -972,404.9..733,115.4 cm, Z 1,200..2,648.74426 cm, and aim Z
150..1,598.74426 cm. Across all 28 live views, camera bounds are X
-970,004.9..1,145,090.4 cm, Y -972,404.9..967,604.9 cm, Z
1,200..2,648.74426 cm, and aim Z 150..1,598.74426 cm, all matching the
numeric rows and fixed camera rule.

The final validation covered the tracked 28-row, 28-column
Scripts/Tests/World/BiomeBaseline418419.csv (SHA-256
48d7315a014a9b33664fd1aa52eb2f574cdfcefe054c50e6b8cba7cc115c5eaf), with 14
sites per seed. It checked finite numeric fields, normalized seven biome
weights, biome identity, water-state agreement, slopes, grades and deterministic
forest counts. All 28 table entries matched their image bytes and hashes; each
runtime log matched seed/site/XY/anchor/camera/aim/FOV/output path. Every live
capture is a -game -RenderOffscreen image; null-RHI overview images are not
counted. Sampled final height ranges -149.225281..1448.64868 cm, collision
height -149.223587..1448.74426 cm, water level -29.377294..356.950745 cm,
slope 0..29.380751 degrees, and grade 0..56.302853 percent. Seven rows have
water; forest descriptor counts range 0..32; biome index counts 0..6 are
5, 2, 8, 3, 3, 5, 2.

This closes BH01 baseline capture only. It makes no generator, gameplay,
networking or save changes. Full terrain and gameplay acceptance remains in the
later BH26 verification increment.

## Verification and completion

During normal increments use narrow checks and careful source inspection.
This planning change requires no Unreal build. At the final implementation
increment, run the applicable full verification from
[development setup](07-development-setup.md), including the Editor build,
regional/master-map/finite-world/integrated and water checks, regional-generation
and minimap peer runners, and affected forest/harvest lifecycle coverage.

Acceptance must demonstrate:

- Repeatable results across cache resets, sampling order and host/client peers;
  different seeds vary; negative coordinates and patch edges remain continuous.
- Each biome has the requested distinct height treatment, with no hard geometry
  seam at label transitions. Compare raw height and final-triangle slope profiles
  at matching coordinates, not only auto-scaled map colours.
- Meadows retains useful hills and camp pockets; Elderwood retains stronger
  relief; Mire has a narrower measured vertical range; Shimmering Lakes combines
  gentle relief and low pockets; Tundra matches Elderwood terrain character with
  lower comparable tree density; Mountains amplify genuinely high base terrain.
- No analytic enclosed bowls, perched lake surfaces or lake-only wetness remain.
  Ocean, rivers, source-coast sign and swimming still agree with final collision.
- Trees sit on dry suitable terrain, cross patch boundaries naturally, and match
  harvestable identities, depletion, late join and restart suppression.
- Snow and storms read clearly in rendered play without adding unrequested
  gameplay effects. Walk slopes, inspect banks/roots and place a camp in a gentle
  pocket; map previews alone cannot prove these outcomes.
- Sampler cost, map updates, mesh/collision generation, forest counts and active
  streaming stay within the existing bounded budgets, compared with baseline.

Primary implementation touchpoints are the world field/regional samplers,
regional tuning and preview settings, forest layout and its gameplay consumers,
generated terrain materials, lake/basin/water/map consumers, and focused tests.
The server retains all gameplay authority. Clients derive matching geometry
from the existing server-owned seed and current build; this plan adds no RPC,
client-selected height, replicated parameter map, dependency or save migration.

## BH02 integration map

Reviewed the current production and test paths before changing generation. This
is a source map for BH03 onward; it records no runtime behavior change.

| Concern | Production touchpoints | Integration contract to preserve |
| --- | --- | --- |
| Fields, labels and regional height | `Source/KalmalaWorld/Public/KalmalaWorldFieldSampler.h` (`FKalmalaWorldFieldSampler::Sample`), `KalmalaBiomeClassifier.h`, `KalmalaRegionalTuning.h`, `KalmalaRegionalGeneration.h`, `Source/KalmalaWorld/Private/KalmalaRegionalGeneration.cpp` (`SampleInternal`, `SampleBiome`) | Keep the four ecological fields, biome labels/weights, distance bands and master-map coastline stable while deriving terrain height separately. Regional sampling currently combines biome height, basin support, water level, coast constraint and priority river data; it is the central height/hydrology integration point. |
| Rendered and colliding terrain | `KalmalaTerrainHeightSampler.h`, `KalmalaTerrainPatchLayout.h`, `Source/KalmalaWorld/Private/KalmalaGeneratedTerrainPatch.cpp` | `SampleHeight` delegates to regional sampling. The generated patch uses it for terrain vertices, finite-difference normals, collision, Meadow rocks and forest root heights. Render and collision share the same 30 m patch lattice and 125 cm cells. |
| Rivers, basins and water meshes | `KalmalaLakeBasin.h/.cpp`, `KalmalaShimmeringLakeSampler.h`, `KalmalaWaterSurfaceMesh.h/.cpp`, `KalmalaOceanSampler.h/.cpp`, and `KalmalaGeneratedTerrainPatch.cpp` | Basin support is consumed after generation by containment/visible-water queries; the water mesh clips regional terrain triangles at sampled water levels, while ocean depth uses the shared collision-triangle surface and sea level. Disabling basins therefore needs both generation policy and consumer gates; ocean and Z=0 river behavior remain separate controls. |
| Maps, wetness and nearby-water consumers | `Source/KalmalaUI/Private/KalmalaMinimapViewModel.cpp`, `KalmalaWorldMapSubsystem.cpp`, `Source/KalmalaWorld/Public/KalmalaEnvironmentalExposureSampler.h`, `KalmalaCampConditionSampler.h`, `KalmalaBiomeExpansionContract.h`, `Source/KalmalaUI/Private/KalmalaAmbientAudioSubsystem.cpp`, `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp`, `KalmalaGameMode.cpp`, and `KalmalaOceanSkiff.cpp` | Minimap raster vertices sample regional outputs and classify wet cells as water; world-map tiles derive biome/water presentation from the same world config. Exposure, camp conditions, biome shore candidates and gameplay water checks use the lake/water samplers. Audit these together so removed basin-only water cannot persist as wetness, map color, audio proximity, or gameplay eligibility. |
| Forest and harvest identity | `Source/KalmalaWorld/Public/KalmalaForestLayout.h`, `Source/KalmalaWorld/Private/KalmalaForestLayout.cpp`, `KalmalaGeneratedTerrainPatch.cpp`, `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp`, `KalmalaHarvestNode.cpp`, and `KalmalaForestDeliveryVerification.cpp` | `FKalmalaForestLayout::Generate` is shared by decorative patch trees and server materialization. Harvest nodes resolve the stable candidate index from the same descriptors; suppression, depletion, late join and restart depend on preserving descriptor identity when placement filters change. |
| Preview and terrain materials | `Source/KalmalaWorld/Public/KalmalaGenerationPreview.h`, `Source/KalmalaWorld/Private/KalmalaGenerationPreview.cpp`, `Source/KalmalaEditor/Private/ExportWorldMapsCommandlet.cpp`, `RenderWorldGenerationVisualizationCommandlet.cpp`, `Scripts/WorldMapPreview.params`, and `KalmalaGeneratedTerrainPatch.cpp` | The fast PNG exporter previews master/biome classification and does not show final height or hydrology. The full visualization commandlet samples final shaped height, hydrology and weights and owns the BH01 CSV diagnostics. Runtime patch materials are the existing `M_GeneratedTerrain`, `M_GeneratedTerrainBiomeDebug`, `M_GeneratedWater`, `M_GeneratedLakeShore`, `M_GeneratedRock`, `M_GeneratedBark` and `M_GeneratedCanopy` assets under `Content/Kalmala/World/Materials`; BH23–BH24 should reuse these and existing weather paths. |

Focused source coverage is in `Source/KalmalaWorld/Private/Tests/`:
`KalmalaRegionalGenerationTest.cpp`, `KalmalaBiomeClassifierTest.cpp`,
`KalmalaMasterMapTest.cpp`, `KalmalaFiniteWorldTest.cpp`,
`KalmalaWaterSurfaceMeshTest.cpp`, `KalmalaOceanSamplerTest.cpp` and
`KalmalaWorldPlayerStartResolverTest.cpp`. Forest descriptor, lifecycle and
restart contracts are in `Source/KalmalaGameplay/Private/Tests/`:
`KalmalaForestPatchTest.cpp`, `KalmalaForestLifecycleTest.cpp` and
`KalmalaForestRestartTest.cpp`. Map agreement/performance coverage is in
`Source/KalmalaUI/Private/Tests/`: `KalmalaGenerationPerformanceTest.cpp`,
`KalmalaMinimapViewModelTest.cpp`, `KalmalaMinimapAsyncTest.cpp`,
`KalmalaWorldMapWidgetTest.cpp` and `KalmalaWorldMapPerformanceTest.cpp`.

The development runners tied to these paths are
`Scripts/Tests/World/RegionalGeneration.ps1` (preview determinism and regional
host/client agreement), `Minimap.ps1` (81-position terrain/weight/water/river
peer agreement), `WorldMapExport.ps1` and `WorldMap.ps1` (map preview and
presentation), and `BiomeExpansion.ps1` (biome peer integration plus forest
delivery/lifecycle options). The applicable automation names and full final
verification procedure remain in [development setup](07-development-setup.md).
The fast map exporter is not a substitute for final-height diagnostics or live
terrain traversal.

Implementation dependencies from this map: BH03–BH05 disable analytic-basin
generation and close all water/map/wetness consumers while retaining ocean and
rivers; BH06–BH18 modify the shared regional height before terrain mesh,
collision, map and placement consumers observe it; BH19–BH23 must use the
shared forest descriptors and existing materials; BH25–BH26 add raw-range,
cost, rendered traversal and integrated peer acceptance. Server-owned gameplay
selection stays authoritative throughout.

## BH10a Meadows hill layer and shoulder curve

The Meadows candidate now adds an independent, seed-derived hill signal in
absolute world XY. Its provisional `MeadowHillScale=16000 cm` and
`MeadowHillAmplitude=600 cm` use a salt distinct from the shared relief stack.
The broad hill signal replaces the previous 40 cm Meadows detail term, then the
candidate applies a smooth exponential upper-shoulder compression above `MeadowShoulderHeight=1000
cm` with `MeadowShoulderCompressionScale=3000 cm`. The curve is monotonic, is
bounded by the finite asymptote at threshold plus scale, and leaves lower candidates
unchanged. The existing continuous biome/coast blend and later coast/river
passes remain in place. These are provisional defaults, not an accepted tuning
result.

Focused source coverage now checks candidate output, lower-pocket preservation,
monotonic bounded compression, and positive/negative hill signal across a 500 m
neighborhood around the seed-418 outer dry Meadows baseline. The BH01 numeric
artifact contains fixed-site points, not 100–250 m Meadow transects. BH10b1
adds fixed seed-418/419 transects and a numeric amplitude sweep runner. BH25
runs the 4/6/8 m trials with freshly compiled code, compares these profiles
with the BH01 fixed-site baselines, and records the selected amplitude and
measured ranges. A pre-BH10a binary cannot measure these trials; the runner
detects missing trial logs instead of accepting stale output. No build,
automation, or rendered acceptance was run for BH10a; full integrated
verification remains BH26.

### BH10b1 reproducible profile sweep harness

`Scripts/Tests/World/BiomeHillTransects.csv` contains 88 numeric sample points:
seed 418/419 Starter Meadows and outer dry Meadows, east-west and north-south
transects, each 25,000 cm long at 2,500 cm intervals. The existing numeric
sampler exports final height and the matching runtime collision-triangle slope
for every point. `Scripts/Tests/World/BiomeHillProfiles.ps1` runs separate
4/6/8 m amplitude trials and records eight per-transect height ranges and
maximum triangle slopes for each trial. Its required effective-amplitude log
marker rejects binaries predating the sampler override. It performs no build
and selects no production tuning; BH25 must run it with freshly compiled code,
compare to the fixed-site `BiomeBaseline418419.csv`, and record the accepted
amplitude and measurements in BH25a.

## BH07 staged preview controls

`FKalmalaRegionalTuning` now names the initial shared scales and documented
Meadows trial controls in centimetres. The commandlet parameter names expose
kilometres for shared scales and metres for Meadow values; the parser rejects
non-finite or out-of-range inputs. Bounds are the design ranges, inclusive:

| Preview controls | Default | Accepted range |
| --- | ---: | ---: |
| `ReliefBroadScaleKm` | 1.0 km | 0.5–1.5 km |
| `ReliefMiddleScaleKm` | 0.15 km | 0.08–0.25 km |
| `ReliefLocalScaleKm` | 0.03 km | 0.01–0.05 km |
| `MeadowHillScaleM` / `MeadowHillAmplitudeM` | 160 m / 6 m | 100–250 m / 4–10 m |
| `MeadowShoulderHeightM` / `MeadowShoulderCompressionScaleM` | 10 m / 30 m | 6–16 m / 10–60 m |
| `MeadowRollingScaleM` / `MeadowRollingAmplitudeM` | 45 m / 1.2 m | 30–70 m / 0.6–2 m |
| `MeadowFloorScaleM` / `MeadowFloorAmplitudeM` | 12 m / 0.2 m | 8–20 m / 0.1–0.35 m |

Custom settings are accepted only by the headless preview route and reported in
`LastRender.txt`; normal gameplay uses the compiled defaults. The fast
classification PNGs do not display height and
changing terrain controls leaves their pixels unchanged. BH10a now consumes the
Meadow hill scale/amplitude and shoulder controls in regional height sampling.
BH11a consumes the independent Meadows rolling scale/amplitude, and BH12a
consumes the local Meadows floor scale/amplitude; the shared broad/middle/local
stack remains staged for the later land-biome candidates. BH10b1 supplies fixed
transects and a validated hill-amplitude override; BH25a runs the fresh-source
hill comparison and selects its setting. BH11b adds the rolling-profile sweep
harness; BH25b runs fresh-source 0.6/1.2/1.8 m trials and selects the rolling
setting. BH12b adds the floor-detail profile harness; BH25c runs the fresh-source
8/12/20 m by 0.1/0.2/0.3 m matrix. Rendered route and under-vegetation
inspection remain BH26.

## BH08 shared relief sampler

Added `FKalmalaRegionalGeneration::SampleSharedRelief` as a pure, output-only
stack of three signed normalized Perlin samples at the supplied absolute world
XY. It uses the staged scales in `FKalmalaGenerationPreviewSettings` (defaults
100,000 / 15,000 / 3,000 cm) and stable independent domains:

| Layer | Domain salt |
| --- | --- |
| Broad | `0x4B414C4D414C4101` |
| Middle | `0x4B414C4D414C4102` |
| Local | `0x4B414C4D414C4103` |

This stack does not modify the four ecological fields or current regional
height; biome shaping consumes it only in later increments. No relief cache is
introduced. Focused sampler coverage checks domain separation, seed/query-order
repeatability across hydrology-cache clearing, negative positions, unchanged
production height, and continuity on both axes of the 3,000 cm terrain patch.
The automation has been added and awaits the BH26 build and integrated run.

## BH11a Meadows rolling relief

The Meadows height candidate now adds a second, independently seeded signal at
the staged 45 m scale and 1.2 m amplitude, before applying the existing smooth
upper-shoulder compression. It uses absolute world coordinates and a dedicated
domain salt; it does not consume or alter the ecological Elevation, biome
weights, or the shared 150 m relief sample. The rolling layer can therefore
shape 30–70 m walk profiles without changing biome identity or floor-detail
frequency. The 6 m broad-hill amplitude is still provisional pending BH25a.

Focused source automation covers signed rolling contribution, lower-pocket
preservation, repeatable fixed seed-418/419 30/45/70 m profile samples, finite
slope calculations, and sub-centimetre probes on both terrain-patch axes. These
fixtures have not been built or executed. BH11b adds the reusable rolling
amplitude sweep harness; BH25b runs it with freshly compiled code and selects
the smallest accepted trial. Rendered traversal and connected-gentle-route
inspection remain for BH26.

### BH11b rolling profile sweep harness

`Scripts/Tests/World/BiomeRollingTransects.csv` contains 264 fixed sample rows:
24 profile groups spanning seed-418/419 Starter and outer Meadows along both
axes, at 30/45/70 m lengths and 5 m spacing, plus four X/Y patch-seam pairs
derived from the BH01 start-relative collision-patch origins. The numeric
commandlet accepts a finite commandlet-only `MeadowRollingAmplitudeM` override
from 0.6 to 1.8 m and resets it after sampling; an absent override leaves the
current preview defaults unchanged. `Scripts/Tests/World/BiomeRollingProfiles.ps1`
runs separate 0.6/1.2/1.8 m CSV trials, validates the fixed fixture, finite
height/slope outputs, dry Meadows weights and adjacent-patch seam deltas, and
requires a per-trial log marker so a stale binary is rejected. It does not
build the Editor or select the final amplitude. BH25b must run it with freshly
compiled source and record the measured profile ranges/slopes and selected
setting. No build or trial was run for BH11b.

## BH12a Meadows floor-detail integration

The Meadows candidate now adds an independently seeded local floor signal at
the provisional 12 m scale and 0.2 m amplitude. The term is sampled in absolute
world coordinates, added after hill and rolling contributions, and passes
through the existing monotonic shoulder compression. It is applied only through
the existing continuous Meadows/coast candidate weights and does not change
ecological fields, biome labels, other biome candidates, or the terrain mesh
lattice. At the smallest permitted 8 m coordinate scale, the 125 cm mesh
spacing gives 6.4 samples per scale unit; this source-level bound does not prove
rendered aliasing acceptance. The 0.1/0.2/0.3 m trials remain provisional
pending fresh-source measurements in BH25c and rendered inspection beneath
vegetation in BH26.

### BH12b floor-detail profile sweep harness

`Scripts/Tests/World/BiomeFloorProfiles.csv` fixes 400 dry Meadows samples:
eight seed-418/419 starter/outer profiles, each 60 m long with 49 points at
125 cm spacing, plus eight outer patch-side controls. The commandlet-only sweep
exports raw floor noise and the pre-blend floor contribution alongside final
height, biome weight, water state, collision-triangle height/slope/grade,
patch/cell coordinates and per-patch forest descriptor counts. Its nine trials
cover 8/12/20 m scales and 0.1/0.2/0.3 m amplitudes. It checks finite samples,
dry Meadows classification, signal-to-input amplitude, collision-cell
progression, forested patch coverage and patch seam continuity. A required
effective scale/amplitude log marker rejects stale binaries. The runner has a
fixture-only `-ValidateOnly` mode and does not build or select final tuning;
BH25c must run all nine combinations with freshly compiled source, then record
the selected scale/amplitude and measurements. The
descriptor count is a fixed-patch proxy for under-vegetation sampling, not a
rendered foliage acceptance result; BH26 inspects aliasing and floor readability
beneath vegetation. No build, sweep, or rendered result is claimed for BH12b.
