# M10 camp-tradeoff sampling regression

## Release rationale

The M10 clean-profile automation run was blocked by
`Kalmala.World.CampConditions.LocalTradeoffs`: its natural-cover spread was
0.120766, below the accepted 0.30 minimum. The fixture advanced both coordinates
by 16,000 cm while Flora uses a frequency of 0.00075. Each step therefore
advanced exactly 12 Perlin cells, retaining the same fractional noise phase.
The fixture undersampled the continuous local vegetation field.

Use 15,700 cm probe spacing across the same -480,000..480,000 cm bounds. Each
step advances 11.775 noise cells and visits different fractional phases. A
fixture assertion requires the fractional step to remain between 0.1 and 0.9
so future frequency tuning cannot silently reintroduce whole-cell aliasing.
Log the measured wetness, cover, water-distance, and resource-count ranges for
reproducible diagnosis. Retain the 0.30 wetness and natural-cover criteria and
the existing water/resource variation assertions.

This fixes verification sampling only. No generated field, terrain, visual
asset, exposure rule, authority model, world/save identity, or save schema
changes. The accepted route-free camp tradeoffs and M10 freeze remain intact;
no roadmap reopening is required.

## Verification

Build `KalmalaEditor Win64 Development` in a disposable project mirror as
described in `07-development-setup.md`. Run the focused test and then the
entire `Kalmala` automation namespace with separate fresh temporary `-UserDir`
profiles, `-abslog`, `-nullrhi`, `-DDC-ForceMemoryCache`, and
`-TestExit="Automation Test Queue Empty"`. Run the mirror's
`Scripts/Verify-CampChoices.ps1` for server-owned exposure, matching client
weather/state, and fire recovery. Record results and evidence paths in
`PROGRESS.md`.

Resolving this assertion permits the M10 clean-profile release suite to
continue. It does not by itself complete the rendered, persistence, travel,
accessibility, and performance acceptance goal.

On 2026-10-01 the editor mirror build passed four incremental actions. The
focused test passed and all 98 tests in the full namespace passed from a
second fresh profile (exit code 0). Both runs measured cover
0.063414..0.892245 (spread 0.828831), wetness 0.025909..0.900000, water
distance 0..3,200 cm, and nearby harvest counts 0..6. The two-player camp
regression passed with 32 matching exposure snapshots per pawn, distinct
cover 0.15/0.80, and normal fire recovery.
