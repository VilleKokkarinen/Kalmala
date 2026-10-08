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

This increment pins identity and membership only. The following goal-8 child
defines the shared texture lookup, asset-import path, unknown-ID fallback,
original-source retention, transparent 64×64 RGBA validator, and pilot image.
Generation must retain original sources and final images and must not bake
labels, UI chrome, or badges into the image. Existing vector icon presentation
remains active until raster assets are generated and integrated.

This is presentation metadata only. It changes no gameplay identity,
inventory rule, network authority, RPC, replicated field, or save schema.
