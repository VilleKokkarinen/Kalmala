# Biome height rework plan

Status: user-requested design direction, 2026-10-24. Planning only; no terrain,
forest, water or weather changes are implemented by this document. Execution is
queued in `BACKLOG.md` after A10 acceptance and before Monster Population; this
plan supersedes Meadows-specific height details where they differ.

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
+Y-facing view with about 23.6° downward pitch. BH01b must resolve exact anchor
heights and wet-site targets before rendered captures are recorded.

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
