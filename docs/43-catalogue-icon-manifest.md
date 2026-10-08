# M12 canonical catalogue icon manifest

The pinned identity table is
[catalogue-icon-manifest.csv](catalogue-icon-manifest.csv). It defines one
canonical raster-image identity for each current item or carried tool, then
records the aliases, consumer views, recipe outputs, and fixed image/import
batches for that identity.

## Sources and coverage

The manifest is derived from schema-4
Content/Data/GameCatalogues.json, its runtime identity normalization in
KalmalaGameCatalogueLoader.cpp, the six carried tool definitions in
KalmalaToolLifecycleContract.cpp, and the existing identity map in
KalmalaIconWidget.cpp.

The source catalogue has 42 item definitions, 19 recipe outputs, and 13
construction outputs. Those outputs share item identities rather than adding
separate images. The six carried tool IDs are separate identities, giving 48
unique canonical rows. The manifest keys each row by the normalized runtime
ID; catalogue_aliases lists source catalogue IDs normalized to that key, and
recipe_ids lists recipes whose output resolves to it. Neither an alias nor a
recipe gets a duplicate image. Every view uses the same canonical ID recorded
in that row.

art_subject is an internal review aid and is not user-facing catalogue copy or
text to place inside an image. shared_views is a semicolon-separated consumer
list. inventory_scope is pack, carried_tool, or construction_only.

CampfireKit is intentionally construction_only: its HearthRing source alias
and Campfire recipe output resolve to the construction image used by Build,
recipe-result, and placement-preview views. It has no pack-inventory or
storage image assignment. The existing gameplay catalogue still has the
legacy HearthRing item record; retiring that item identity remains in M12
goal 9 and is not part of this manifest increment.

## Pinned batches

The image batches preserve the order of the existing canonical icon map.
Each batch has exactly four unique image identities. Import batches pin the
same manifest order in groups of sixteen: A is image batches 01–04, B is
05–08, and C is 09–12. The next generation increment therefore begins with
the four rows in image batch 01.

| Batch | Canonical IDs |
| --- | --- |
| 01 | Wood, Lightwood, Densewood, Coal |
| 02 | Stone, Iron, Fibre, PeatAmber |
| 03 | FrostSalt, MirelingAsh, CampfireKit, WorkbenchKit |
| 04 | ForgeKit, WorkbenchToolRackKit, ForgeAnvilKit, GrindingStoneKit |
| 05 | StorageKit, CookingRackKit, FryingPanKit, CauldronKit |
| 06 | FloorKit, WallKit, RoofKit, BoarMeat |
| 07 | DeerMeat, BoarHide, DeerHide, CookedBoarMeat |
| 08 | CookedDeerMeat, HearthBroth, MeatStew, RootVegetableSoup |
| 09 | RoastedRootVegetables, DeerRootRoast, Carrot, Potato |
| 10 | Rutabaga, Onion, CarrotSeed, PotatoSeed |
| 11 | RutabagaSeed, OnionSeed, ReedKnife, FieldHatchet |
| 12 | StonePick, BronzeAxe, IronAxe, ConstructionHammer |

If the live identity set grows beyond these 48 rows, append identities in new
four-image batches before claiming complete coverage. Keep existing batch
membership stable; do not renumber or silently replace a pinned identity.

The shared lookup in `FKalmalaCatalogueIconLibrary` accepts only canonical
runtime IDs from the existing vector icon map. It maps them to
`/Game/Kalmala/UI/Icons/Items/<ID>.<ID>` and returns no texture for an unknown
ID or an asset that has not been imported. `UKalmalaIconWidget::SetCatalogueIcon`
uses the existing vector assignment when the raster is missing and the
question-mark vector for unknown IDs. Existing consumers remain on their
current vector path until the later menu-integration child.

The pilot is Wood. Keep the original generated RGBA PNG at
`Content/Kalmala/UI/Source/IconOriginals/Wood.png` and the prepared 64×64
import PNG at `Content/Kalmala/UI/Source/Icons/Wood.png`; the intended imported
object path is `/Game/Kalmala/UI/Icons/Items/Wood.Wood`. Run
`Scripts/Validate-CatalogueIcon.ps1 -Id Wood` to verify the pinned identity,
source retention, both PNG signatures/8-bit RGBA channels, real transparency,
the final dimensions, and all relevant paths against the Windows path limit.
The validator accepts one canonical ID at a time so batches remain bounded.

Pilot prompt: “Two short cut firewood logs crossed in a compact bundle, one
with visible growth rings and bark edge; original hand-drawn game UI line art,
muted oak brown and warm tan fill, crisp ivory contour lines and a thin dark
outline; centered strong silhouette with transparent margin, readable at
64×64; no text, labels, badge, border, props, watermark, or scenery.” Keep the
same framing, contour weight, restrained palette, and even lighting across each
four-identity batch.

Generation must retain original sources and final images and must not bake
labels, UI chrome, or badges into the image. The project source and prepared
PNG remain separate from the imported `.uasset`; actual batch imports and
cross-view consumption are later children.

## Batch 01 review — 2026-10-08

Completed the pinned Wood, Lightwood, Densewood, and Coal batch. Wood is the
previously prepared pilot; its 1254×1254 original and transparent 64×64 final
were reviewed beside the three new identities. New source images are retained
at `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`; prepared import images
are at `Content/Kalmala/UI/Source/Icons/<ID>.png`.

The prompts kept one visual family: a close, centered, hand-painted bundle or
cluster, soft warm light, a strong dark contour, a transparent margin, no
background or labels, and a silhouette that survives 64×64 reduction.
Lightwood uses pale birch-like crossed logs, Densewood uses dark dense-grained
hardwood logs, and Coal uses three graphite-black fuel chunks. Each final image
was inspected at 64×64 beside Wood; the timber values and coal silhouette
remain distinct at native size. `Scripts/Validate-CatalogueIcon.ps1` passed
for Lightwood, Densewood, and Coal, confirming retained 1254×1254 RGBA source
images and transparent 64×64 RGBA finals with the pinned import targets.

These images are prepared for import only. No `.uasset` was created and
existing menu consumers continue to use vector icons until the later import
and integration children.

This is presentation metadata only. It changes no gameplay identity,
inventory rule, network authority, RPC, replicated field, or save schema.

## Batch 02 review — 2026-10-08

Completed the pinned Stone, Iron, Fibre, and PeatAmber batch. The retained
originals are 1254×1254 RGBA PNGs in
`Content/Kalmala/UI/Source/IconOriginals/`; the prepared import files are
transparent 64×64 RGBA PNGs in `Content/Kalmala/UI/Source/Icons/`. All four
were reviewed at native size alongside Wood and the batch 01 icons. Stone
reads as a three-stone gray cluster, Iron as charcoal ore with pale metallic
veins, Fibre as pale reed strands tied with a muted green band, and PeatAmber
as dark peat with honey-amber inclusions. Their silhouettes remain distinct
at 64×64.

The prompt set kept the batch 01 framing and treatment: centered close-up
material icons with warm soft light, ivory edge highlights, a thin dark
contour, transparent margins, and no text, UI chrome, badges, props, scenery,
or watermark. Each asset used its own subject: cool-gray fieldstones; a dark
iron-ore chunk with silver-gray seams; a tied bundle of pale reed fibres; and
a compact peat chunk with visible amber deposits. The four batch 01 PNGs were
style references only.

`Scripts/Validate-CatalogueIcon.ps1` passed for each identity and confirmed
the retained source, 64×64 RGBA final, non-opaque transparency, manifest row,
and import target. These are prepared for import only; no `.uasset` was made,
and current catalogue consumers remain on vector icons until the later
import/integration children.

This is presentation content only. It changes no gameplay identity,
inventory rule, network authority, RPC, replicated field, or save schema.


## Batch 03 review — 2026-10-08

Completed FrostSalt, MirelingAsh, CampfireKit, and WorkbenchKit. Each generated
original is retained as a 1254×1254 RGBA PNG in
`Content/Kalmala/UI/Source/IconOriginals/`; its import preparation is a
transparent 64×64 RGBA PNG in `Content/Kalmala/UI/Source/Icons/`. The visible
artwork was fit proportionally into a centered 56×56 maximum area, leaving a
clear transparent margin.

At native size, FrostSalt reads as pale blue-gray crystalline salt, distinct
from the ordinary Stone cluster. MirelingAsh is a loose charcoal-gray ash
heap with small ember flecks, distinct from solid Coal and PeatAmber.
CampfireKit shows a low ring of hearth stones around a small flame and remains
construction-only; the image is for recipe/build/result/placement views, not
pack or storage. WorkbenchKit reads as a compact oak joiner's bench with an
end vise. All four share the batch 01 framing, warm highlights, dark contours,
and contain no labels, UI chrome, or badges.

`Scripts/Validate-CatalogueIcon.ps1` passed for all four IDs, confirming the
manifest identity, retained high-resolution source, prepared 64×64 RGBA file,
transparency, and import target. These remain prepared for import only; no
`.uasset` was created, and current catalogue consumers remain on vector icons
until the later import and integration children.

This changes presentation assets and documentation only. It does not change
gameplay identity, inventory rules, network authority, RPCs, replicated fields,
or save schemas.

## Batch 04 review — 2026-10-08

Generated ForgeKit, WorkbenchToolRackKit, ForgeAnvilKit, and
GrindingStoneKit. Each original is retained as a 1254×1254 RGBA PNG in
`Content/Kalmala/UI/Source/IconOriginals/`; each prepared image is a transparent
64×64 RGBA PNG in `Content/Kalmala/UI/Source/Icons/`. Artwork was alpha-cropped,
proportionally fitted to a centered 56×56 maximum area, and reviewed at native
size against the earlier batches.

ForgeKit reads as a squat stone-and-iron furnace with a short flue and ember-lit
mouth, distinct from the CampfireKit's low open stone ring. WorkbenchToolRackKit
shows an oak A-frame with a hand axe, mallet, and pick hanging from its bar.
ForgeAnvilKit has a broad iron face and short horn on a cut oak stump.
GrindingStoneKit has a prominent upright abrasive wheel with its crank and oak
stand. All four retain the warm highlights, ivory edge accents, dark outlines,
and restrained material palette used by the earlier batches; none contains
text, labels, badges, borders, or scenery.

The four individual prompts used the Wood and WorkbenchKit icons as style
references only. Each requested one of the subjects above, centered framing
within a 56×56 safe area, a genuinely transparent background, hand-painted UI
line art, warm soft highlights, an ivory edge accent and thin dark contour, and
no text or UI chrome. Subject details were written separately so station
silhouettes remain distinct at 64×64.

`Scripts/Validate-CatalogueIcon.ps1` passed for all four IDs, confirming the
pinned identities, retained RGBA originals, transparent 64×64 finals, and import
targets. These files are prepared for import only: no `.uasset` was created,
and existing catalogue consumers remain on their vector icons pending the
later import and menu-integration children. This changes presentation assets
only; no gameplay identity, network authority, RPC, replicated field, or save
schema changed.

## Batch 05 review — 2026-10-08

Generated StorageKit, CookingRackKit, FryingPanKit, and CauldronKit. Each
original is retained as a 1254×1254 RGBA PNG in
`Content/Kalmala/UI/Source/IconOriginals/`; each prepared icon is a transparent
64×64 RGBA PNG in `Content/Kalmala/UI/Source/Icons/`. Visible artwork was
alpha-cropped, proportionally fit inside a centered 56×56 area, and reviewed at
native size.

StorageKit reads as a squat oak chest with a domed lid, iron straps, and a
front clasp. CookingRackKit shows an empty iron grate held by an oak A-frame.
FryingPanKit has a shallow iron bowl and long handle. CauldronKit has a rounded
open iron pot, loop handles, and three short feet. Their silhouettes remain
distinct at 64×64 while keeping the warm highlights, ivory edge accents, and
dark contours from the earlier batches. None contains labels, UI chrome,
badges, food, or scenery.

Each separate image prompt used WorkbenchKit, ForgeKit, and Coal as style
references only, with one pinned subject per image. Prompts requested genuine
transparency, close three-quarter framing, warm soft light, a thin dark contour,
and no text or extra props.

`Scripts/Validate-CatalogueIcon.ps1` passed for all four IDs, confirming their
pinned manifest identities, retained RGBA originals, transparent 64×64 finals,
and import targets. These files are prepared for import only: no `.uasset` was
created, and current catalogue consumers remain on vector icons pending the
later import and menu-integration children. This changes presentation assets
only; no gameplay identity, network authority, RPC, replicated field, or save
schema changed.

## Batch 06 review — 2026-10-08

Generated FloorKit, WallKit, RoofKit, and BoarMeat as separate original images
with the built-in image generator. Retained their generated RGBA PNG sources in
`Content/Kalmala/UI/Source/IconOriginals/`; the FloorKit source is 1536×1024
and the other three are 1254×1254. Prepared transparent 64×64 RGBA images in
`Content/Kalmala/UI/Source/Icons/`, with visible artwork proportionally fit
inside a centered 56×56 area.

FloorKit reads as a square oak plank floor tile with visible support beams;
WallKit as a vertical plank panel with cross-braces; RoofKit as a sloped
reed-thatch panel with a timber ridge; and BoarMeat as a fresh red pork cut
with pale fat and a small bone. Their construction silhouettes and ingredient
silhouette remain distinct at 64×64. The prompt set requested one subject per
icon, transparent background, warm soft highlights, ivory edge accents, a thin
dark contour, and no text, labels, UI chrome, scenery, or extra props.

`Scripts/Validate-CatalogueIcon.ps1` passed for all four identities,
confirming their pinned manifest rows, retained RGBA sources, transparent
64×64 finals, and import targets. These remain prepared for import only: no
`.uasset` was created, and current consumers remain on vector icons pending the
later import and menu-integration children. This changes presentation assets
only; no gameplay identity, network authority, RPC, replicated field, or save
schema changed.

## Batch 07 review — 2026-10-08

Generated DeerMeat, BoarHide, DeerHide, and CookedBoarMeat as four separate
original images with the built-in image generator. The retained originals are
1254×1254 RGBA PNGs in Content/Kalmala/UI/Source/IconOriginals/; the
prepared import files are transparent 64×64 RGBA PNGs in
Content/Kalmala/UI/Source/Icons/. Artwork was alpha-cropped and
proportionally fit into a centered 56×56 maximum area.

The prompts used earlier BoarMeat, DeerMeat, BoarHide, and Wood icons as style
references only. They kept the hand-painted UI linework, warm highlights,
ivory edge accents, and thin dark contour. DeerMeat is a lean crimson venison cut with a pale bone
edge, distinct from the existing raw BoarMeat. BoarHide is a dark, coarse
bristle pelt; DeerHide is a lighter fawn-colored soft pelt. CookedBoarMeat is
a seared golden-brown chop with a warm cooked center and small bone. Each
prompt specified genuine transparency and excluded labels, UI chrome, extra
props, and scenery.

All four prepared icons were reviewed at native 64×64 size. The
Scripts/Validate-CatalogueIcon.ps1 validator passed for each ID, confirming
the manifest row, retained RGBA original, transparent 64×64 final, and import
target. These are prepared for import only: no .uasset was created, and
current catalogue consumers remain on vector icons pending later import and
menu-integration children.

This changes presentation assets only. It does not change gameplay identity,
inventory rules, network authority, RPCs, replicated fields, or save schemas.

## Batch 08 review — 2026-10-08

Generated CookedDeerMeat, HearthBroth, MeatStew, and RootVegetableSoup as
separate original images with the built-in image generator. Each retained
source is a 1254×1254 RGBA PNG in Content/Kalmala/UI/Source/IconOriginals/;
each prepared import image is a transparent 64×64 RGBA PNG in
Content/Kalmala/UI/Source/Icons/. Artwork was alpha-cropped and proportionally
fit within a centered 56×56 maximum area.

CookedDeerMeat is a lean seared venison cut with a warm cooked center.
HearthBroth is clear golden broth in a small wooden bowl. MeatStew uses a
darker broth with visible meat and root-vegetable pieces; RootVegetableSoup
has a lighter broth and clearly visible vegetables without meat. Each prompt
used existing icons as style references only and excluded text and scenery.

All four finals were reviewed at native size and passed
Scripts/Validate-CatalogueIcon.ps1. Their PNGs are prepared for import only;
no .uasset was created and current consumers remain on vector icons.

## Batch 09 review — 2026-10-08

Generated RoastedRootVegetables, DeerRootRoast, Carrot, and Potato as separate
original images with the built-in image generator. Each retained source is a
1254×1254 RGBA PNG in Content/Kalmala/UI/Source/IconOriginals/; each prepared
import image is a transparent 64×64 RGBA PNG in
Content/Kalmala/UI/Source/Icons/. Artwork was alpha-cropped and proportionally
fit within a centered 56×56 maximum area.

RoastedRootVegetables is a compact pile of browned carrot, potato, and onion
pieces. DeerRootRoast pairs seared venison with rutabaga and onion. Carrot is
a tapered orange root with a leafy crown; Potato is an earthy oval with a
cream-colored cut end. These subjects follow the catalogue identities and
share the established hand-painted linework, warm highlights, ivory edge
accents, and thin dark contour.

All four finals were reviewed at native size and passed
Scripts/Validate-CatalogueIcon.ps1. Their PNGs are prepared for import only;
no .uasset was created and current consumers remain on vector icons.

## Batch 10 review — 2026-10-08

Generated Rutabaga, Onion, CarrotSeed, and PotatoSeed as separate original
images with the built-in image generator. Each retained source is a
1254×1254 RGBA PNG in Content/Kalmala/UI/Source/IconOriginals/; each prepared
import image is a transparent 64×64 RGBA PNG in
Content/Kalmala/UI/Source/Icons/. Artwork was alpha-cropped and proportionally
fit within a centered 56×56 maximum area.

Rutabaga has a pale golden body and muted purple shoulder; Onion has a rounded
amber bulb with papery skin. CarrotSeed uses elongated ochre-brown seeds,
while PotatoSeed uses a separate cluster of smooth pale-tan botanical seeds
without tuber eyes or sprouts. Each prompt kept a transparent cutout, a
simple strong silhouette, and the established hand-painted icon treatment.

All four finals were reviewed at native size and passed
Scripts/Validate-CatalogueIcon.ps1. Their PNGs are prepared for import only;
no .uasset was created and current consumers remain on vector icons.

## Batch 11 review — 2026-10-08

Generated RutabagaSeed, OnionSeed, ReedKnife, and FieldHatchet as separate
original images with the built-in image generator. Each retained source is a
1254×1254 RGBA PNG in Content/Kalmala/UI/Source/IconOriginals/; each prepared
import image is a transparent 64×64 RGBA PNG in
Content/Kalmala/UI/Source/Icons/. Artwork was alpha-cropped and proportionally
fit within a centered 56×56 maximum area.

RutabagaSeed is a cluster of russet oval seeds, distinct from the lighter
CarrotSeed and PotatoSeed. OnionSeed uses smaller charcoal teardrop seeds.
ReedKnife has a short gathering blade and pale reed-wrapped grip; FieldHatchet
has a broad iron edge and compact wooden handle. Each remains an individual
tool or seed identity with no baked-in labels, UI chrome, or scenery.

All four finals were reviewed at native size and passed
Scripts/Validate-CatalogueIcon.ps1. Their PNGs are prepared for import only;
no .uasset was created and current consumers remain on vector icons.
