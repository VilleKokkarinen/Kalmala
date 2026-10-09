# M12 goal 9: Catalogue copy audit

The pinned before/after table is [catalogue-copy-audit.csv](catalogue-copy-audit.csv).
It snapshots current copy at baseline commit 0dfad8e (schema 4), before any
goal 9 text or catalogue implementation changes. Review notes and proposed
wording are candidates for the later ordered copy increments; they are not
runtime changes or an approval to change gameplay data.

## Coverage and order

The frozen baseline table has one row per canonical identity in
[the icon manifest](catalogue-icon-manifest.csv): 42 original item identities
and six carried tools. After the Campfire item retirement, the live catalogue
has 41 normal item definitions; the CampfireKit row remains in the audit as a
construction-only identity and its original item copy stays frozen for
baseline comparison. It captures the exact item name/description source ID, current
and proposed name/description, matching recipe IDs and recipe display names,
and a copy decision. Recipe labels are aligned with their output identity.
There are 19 recipe labels, including the Campfire construction recipe. Tool
rows state where no authored description currently exists.

The batch field pins six ordered groups of eight identities, following the
existing canonical icon-manifest order:

| Batch | Canonical identities |
| --- | --- |
| 01 | Wood, Lightwood, Densewood, Coal, Stone, Iron, Fibre, PeatAmber |
| 02 | FrostSalt, MirelingAsh, CampfireKit, WorkbenchKit, ForgeKit, WorkbenchToolRackKit, ForgeAnvilKit, GrindingStoneKit |
| 03 | StorageKit, CookingRackKit, FryingPanKit, CauldronKit, FloorKit, WallKit, RoofKit, BoarMeat |
| 04 | DeerMeat, BoarHide, DeerHide, CookedBoarMeat, CookedDeerMeat, HearthBroth, MeatStew, RootVegetableSoup |
| 05 | RoastedRootVegetables, DeerRootRoast, Carrot, Potato, Rutabaga, Onion, CarrotSeed, PotatoSeed |
| 06 | RutabagaSeed, OnionSeed, ReedKnife, FieldHatchet, StonePick, BronzeAxe, IronAxe, ConstructionHammer |

The CSV batch column is authoritative; each canonical identity occurs once and
each batch has exactly eight rows. Later copy work should apply these batches
in order and must not change the frozen current-copy columns.

## Batch 01 applied

Batch 01 updates the Wood, Lightwood, Densewood, Iron, Fibre, and Peat Amber
descriptions and normalizes the player-facing Fibre name to **Reed Fibre**.
Coal's concise fuel description and Stone's pet-rock line are retained
verbatim. Densewood's candidate was tightened during implementation: current
recipes and construction do not use it for structures, so its copy names the
documented Ironheart-trunk source and hearth-fuel use without implying a
structure recipe. The frozen current-copy columns above remain unchanged.

The catalogue automation pins all eight reviewed names, descriptions, and
existing stack limits. The development check compares every non-text field in
these eight JSON item rows against baseline commit 0dfad8e.

## Batch 02 applied

Batch 02 applies the reviewed Frost Salt and Mireling Ember Ash copy and
capitalizes the Workbench Tool Rack, Forge Anvil, and Grinding Stone labels in
both their item and recipe rows. Workbench keeps the earlier roadmap-mandated
name and now uses its concise camp-workbench description; Forge keeps its
station name and clarifies its toolmaking and repair role. The rack description
states its verified level-2 effect without claiming that it unlocks a current
Workbench upgrade recipe. The Campfire remains construction-only: its short
result description now comes from the direct-build result path, with no normal
inventory item or pack description. The focused catalogue automation pins the
seven live item names/descriptions and stack limits, and checks recipe labels
against their stable output identities.

The frozen current-copy columns are unchanged. Item stack limits, recipe output
IDs, ingredients, batch counts, station requirements, aliases, placement costs,
and construction/save identities remain unchanged.

## Batch 03 applied

Batch 03 aligns Chest, Cooking Rack, Frying Pan, Cauldron, Timber Floor,
Windbreak Wall, and Reed Roof with their recipe names, and updates Boar Meat's
name. Descriptions remove implementation wording, retain verified cooking and
shelter uses, and keep Chest capacity in the structured storage details. Boar
Meat keeps a food-focused line without claiming an effect. Focused catalogue
assertions pin all eight names, descriptions, and existing stack limits, then
check the seven recipe labels against their stable output identities.

The frozen current-copy columns are unchanged. Item stack limits, recipe output
IDs, ingredients, batch counts, station requirements, aliases, placement costs,
and construction/save identities remain unchanged.

## Batch 04 applied

Batch 04 capitalizes Deer Meat, Boar Hide, Deer Hide, Cooked Boar Meat,
Cooked Deer Meat, Hearth Broth, Meat Stew, and Root Vegetable Soup, and aligns
the four matching cooked-meat/soup recipe labels. Deer Meat now names its
supported roasting and stew uses; the hide descriptions retain their accurate
sources; cooked meat keeps its ready-to-eat cue. Hearth Broth no longer implies
a warming effect or a production recipe. Meat Stew's authored hearty flavor and
ingredient list remain verbatim, while Root Vegetable Soup retains its current
ingredients and cauldron process.

The focused catalogue assertions pin all eight names, descriptions, and stack
limits, then check recipe/output name alignment and stable output IDs. The
frozen current-copy columns remain unchanged. Item IDs, stack limits, recipe
outputs, ingredients, batch counts, station requirements, and cooking effects
are unchanged.

## Batch 05 applied

Batch 05 capitalizes Roasted Root Vegetables, Deer and Rutabaga Roast, Carrot
Seeds, and Potato Seeds, aligning the two frying-pan recipe labels with their
outputs. The descriptions retain the exact food ingredients and cooking
context, preserve Carrot's orange-stick wordplay and Onion's tear joke, replace
Potato's recognizable borrowed line with original cooking copy, and avoid
implying that seeds have a planting mechanic. The focused catalogue assertions
pin all eight names, descriptions, and stack limits plus the two stable recipe
outputs. Frozen current-copy columns and all non-copy catalogue fields remain
unchanged.

## Batch 06 applied; audit coverage complete

Batch 06 capitalizes Rutabaga Seeds and Onion Seeds and replaces planting
instructions with factual source descriptions. Reed Knife, Field Hatchet, Stone
Pick, Bronze Axe, Iron Axe, and Construction Hammer now have the reviewed
role-specific descriptions in the carried-tool detail; the existing names,
live owner-only level/condition, and tool identities remain intact. The action
copy matches the current tool contract: Bronze Axe harvests Lightwood, Iron Axe
harvests Densewood, and Construction Hammer places structures. Focused
assertions pin both seed names/descriptions/stack limits and all six tool
descriptions, including visible-state and unknown-tool fallback behavior.

All six ordered batches are now applied. The pinned CSV contains all 48
canonical identities exactly once across six groups of eight; its original
current-copy columns remain frozen for comparison. The 41 live item rows, the
construction-only Campfire identity, and six carried-tool descriptions have
complete proposal coverage. No tool became an inventory item and no planting,
crafting, harvest, or construction rule changed.

## Preserve intentional manual flavor

The source history and copy review identify these distinctive owner-authored
lines as intentional flavor to keep:

- Stone's friendly pet-rock sentence remains unchanged.
- Carrot's orange-stick wordplay remains, with capitalization normalized.
- Onion's eye-watering joke remains unchanged.
- The Meat Stew description keeps its hearty tone and actual ingredient list.

Batch 05 replaces Potato's recognizable borrowed line with original cooking
copy while retaining a playful food cue. The Workbench label is also an
intentional earlier rename, but the roadmap explicitly replaces it with
Workbench; its useful furnishing and repair description is retained. The other
proposals make only the narrow changes called out in each CSV review note. Do
not apply generic prose across the catalogue.

Every proposed use must be checked against the current recipes, tool actions,
construction rules, and cooking contracts before its batch is implemented.
The candidates avoid costs, quantities, conditions, levels, invented effects,
new unlocks, and planting mechanics. Campfire copy is for a construction result
only: the classification work removed the normal HearthRing item presentation,
preserved legacy/runtime aliases and saved-construction compatibility, and kept
existing placement costs and server validation. The construction-only Campfire
result description was implemented in copy batch 02.

## Live text-source map

- Content/Data/GameCatalogues.json is the live source for 41 normal item
  DisplayName/Description pairs and 19 recipe DisplayName fields. Normal
  recipe rows carry item output IDs; the loader routes the four direct-build
  outputs into the runtime `BuildableOutput` descriptor instead. Normal result
  descriptions are resolved from their matching item definition.
- Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp maps
  legacy catalogue IDs such as HearthRing and Workbench to stable runtime
  construction IDs. Direct-build outputs become construction descriptors and
  are validated separately from item outputs. The legacy HearthRing recipe
  output maps to CampfireKit only as a construction descriptor; there is no
  Campfire inventory row. The Campfire recipe provides its construction-result
  description separately from item lookup. Keep IDs and aliases stable while
  changing text.
- Source/KalmalaGameplay/Private/KalmalaItemCatalogue.cpp and
  KalmalaRecipeCatalogue.cpp load and validate those JSON definitions.
  Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp defines
  tool identities/actions but has no display names or descriptions.
- Tool display names are hard-coded in
  Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp
  (GetCarriedToolDisplayName) and
  Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp
  (GetToolDisplayName). The inventory detail panel uses the reviewed role
  description in UKalmalaItemDetailWidget::DescribeCarriedTool, alongside the
  generic Carried equipment label and live owner-visible level/condition.
- Catalogue-backed item names flow through
  Source/KalmalaUI/Private/KalmalaCatalogueRowsWidget.cpp,
  KalmalaIngredientWidget.cpp, KalmalaInventoryInspectWidget.cpp,
  KalmalaStationContextWidget.cpp, and KalmalaInventoryMenuWidget.cpp. Item
  descriptions flow through KalmalaItemDetailWidget.cpp and recipe result
  detail in KalmalaCraftingComponent.cpp. Recipe/build menus and interaction
  prompts are assembled in Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp;
  prompts resolve construction names from the shared catalogue, with Campfire
  named directly.
- Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp uses the current
  Workbench name in its readiness/result text and title-cased Cooking Rack and
  Frying Pan interaction results. The shared nearby-storage guidance in
  Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp now says Chest, matching
  the canonical item name; the Build heading's Construction hammer label
  remains for the following recipe/build/station text-alignment increment.
- Source/KalmalaUI/Private/Tests/, Source/KalmalaGameplay/Private/Tests/,
  Scripts/Verify-Crafting.ps1, and the goal 9 copy documentation contain
  expectations or explanatory wording to review alongside each changed
  batch. They are checks/documentation, not additional runtime catalogue
  sources.
- The art_subject column in catalogue-icon-manifest.csv is an internal
  asset-review aid, not player-facing copy. Preserve the icon identity and
  mapping while updating text.

The inventory browse/detail rows, ingredient rows, storage selector and
interaction prompt retain canonical identities and icon mappings while showing
the current reviewed display names. Focused UI assertions cover Reed Fibre's
visible name in search, sorting, ingredients and chest contents, its existing
Fibre icon, and the Workbench catalogue name in its interaction prompt. The
Hearth Broth food action prompt and status use the reviewed item capitalization.

The original CSV remains a frozen copy baseline, including the retired
HearthRing item copy. The Campfire retirement removes that normal item
definition and renames its direct-build recipe/result; no costs, placement
authority, icon identity, or save schema change. Across the six applied batches
and this cross-surface label cleanup, names stay aligned across inventory,
ingredients, storage, prompts and previews. Recipe, build, station, upgrade and
result text alignment plus wrapping review remain in the following increment;
compare all non-text catalogue fields against the baseline during final
milestone verification.
