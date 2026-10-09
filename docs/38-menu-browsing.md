# Local menu browsing

The owner-directed player inventory panel now uses the fixed forty-cell grid
in `47-inventory-grid.md`, without search, filters, sorting or separate tool
rows. Earlier inventory-menu acceptance below is historical. Recipe, build,
station and chest browsing contracts remain applicable to their own views.

The first M11 browsing increment extends interactive inventory inspection in
the existing crafting/build modal. The read-only gameplay HUD remains unchanged.
Only the owning pawn's supplied inventory/tool rows can become search results;
the widget never enumerates absent catalogue entries or queries a peer.

Search matches the visible item name, case-insensitively, after trimming outer
whitespace, using at most 64 characters. It does not search quantities, private
state, descriptions, canonical IDs or hidden content. The category control
cycles All, Items and Carried tools using the supplied carried-tool flag.
Sort cycles Owner order, Name and Category / name. Name comparison ignores case,
with canonical identity as a deterministic tie-breaker. Category sorting labels
Items and Carried tools and sorts names within each group. Owner order is the
original supplied order; filtering/sorting copies rows and never rearranges
inventory storage or saves.

Tab reaches search and the labelled controls; buttons accept normal activation.
While inspection is focused, Page Up / controller left shoulder cycles category,
Page Down / right shoulder cycles sort, and arrows / D-pad select visible entries.
Text editing keeps its cursor keys. Existing Escape/B closes the modal. Controls
retain their widget identity and focus during refresh; a still-visible canonical
selection survives filter/sort/data refresh. If selection disappears, the first
visible result is selected. No results clears selection, hides details and names
Clear search / All as recovery options. Empty inventory has distinct feedback.
Previous/next disable when no rows are visible; browsing never performs an item,
craft, repair, construction or transfer action. During the local widget session,
the inventory inspector remembers its bounded query, category, sort and selected
canonical item ID in state separate from each recipe/station menu. Reopen applies
the filters to current owner-visible rows, restores the selected ID when visible,
and falls back to the first visible row (or no selection when empty) if it was
consumed or filtered out. If the inspector had focus when the menu closed, it
regains focus and its own scroll offset; otherwise the recipe/station scroll
position is restored. Scroll restoration waits for layout and clamps to the
current range after viewport or UI-scale changes. This memory is transient and
never replays an item or gameplay action.

The shared local theme, text scale and contrast style labels, buttons and search
text. Browse controls are vertically arranged inside the existing scroll view.
Filters/query/order live only on this widget; cross-menu or reopen persistence
belongs to the later remembered-menu task. Controller text entry uses Unreal's
existing editable text control; no custom virtual keyboard or binding is added.

Increment verification: compile the affected UI module in the short disposable
mirror with normal UnrealBuildTool access, then run Kalmala.UI.Inventory with
isolated UserDir/logs and the null renderer. LocalBrowsing covers search, actual
text-change delegate binding, filters, sorting, canonical selection, grouped
navigation, empty/no-results, owner refresh and unchanged source order at 150%
high contrast. The rendered Verify-Crafting probe additionally routes shoulder
keys through real Slate focus and requires category/sort, no-results and recovery
on both peers alongside the existing modal/transaction checks. Run presentation
ownership, five M5 documentation contracts and diff/path checks.

Crafting/cooking browsing and named build groups remain separate ordered children.
Full parent suite, rendered browsing-control layout at all supported scales,
physical controller text entry and packaging remain pending.

Accepted increment evidence, 2026-10-04: final affected UI compile/link passed four actions with normal UnrealBuildTool access. Focused inventory queue passed 5/5, exit0 at C:/Users/Ville/AppData/Local/Temp/kb1/final.log (before final contrast-only refinement). Initial port18583 peers crashed because UE5.8 SetWidgetStyle retains the supplied argument address in Slate; storing the style on the widget fixed it and port18584 passed. Final rendered port18585 passed after source-PNG inspection prompted search-background contrast and Clear search wrapping repairs. Both peer logs contain CategoryKey=1 SortKey=1 NoResults=1 Restored=1, FocusAndKeys=1 and Hidden=1 Restored=1. Final captures/logs are under C:/Users/Ville/AppData/Local/Temp/KalmalaCrafting-09a2058ab331410d9133d1cd7b2be721. Standard host inspection source PNG reviewed with browsing controls and complete selected details visible. Ownership/five documentation contracts and diff/path checks passed; mirror maximum209. Representative inspection does not establish full scale/contrast acceptance.

## Crafting and cooking recipe browsing

Recipe browsing searches existing display names case-insensitively, trims outer
whitespace and bounds the effective query to 64 characters. All, Other crafting
and Cooking partition the existing catalogue using ExperienceSkill == Cooking;
Other crafting includes existing construction recipes. The additional build
filters provide a narrower view without changing that existing category.
These filters always intersect the existing station scope.
They never hide an unavailable recipe by availability or reveal extra recipes.
Catalogue order and Name order operate on copied indices; name ties use RecipeId.
A still-visible selected recipe retains identity through sorting/filter changes;
a removed selection falls back to the first visible entry. No results clears
recipe cards/details and disables Craft, with Clear search/All recovery text.
Browsing cancels an obsolete placement preview and performs no transaction.

The search and labelled category/order buttons precede the recipe grid inside
the existing scroll view. Tab and normal button activation provide keyboard and
controller access; Page Up/Down cycles category/order when the panel has focus.
Search focus yields to ordinary text editing. Escape/B keeps modal close behavior.
Existing controller hearth/build shortcuts outside text editing are retained.
Theme text scale styles search font and menu labels; search uses a widget-owned
Slate style to avoid dangling style pointers. The main hammer menu and each
station menu remember query, category, sort, selected recipe identity and scroll
offset independently for the local widget session. Reopening first reapplies the
saved filters, then restores the canonical selection if it is still visible;
otherwise it selects the first visible result, or no selection for an empty
filtered view. Station scopes never inherit another menu's filter. Scroll offsets
are applied after the reopened layout updates and clamped to the current scroll
range, so catalogue changes, viewport resize and interface-scale changes do
not leave an invalid offset. Entries that remain in the catalogue but are
currently unavailable stay selectable and show their live unavailable reason.
This memory is transient to the local player's widget session and does not write
settings, gameplay saves or network state. Inventory-inspector and world-map
memory remain later ordered children.

Targeted verification: affected UI compilation, Kalmala.UI.Crafting.LocalBrowsing
for category partition, name matching, ordering/no-results and source-order
restoration; rendered Verify-Crafting requires both peers to report
Recipe browsing: SelectionKept=1 Category=1 NoResults=1 Restored=1, including
actual editable-control delegate updates and SearchFocus=1 text-edit safety, alongside existing selection/navigation,
modal restoration, authority, transaction and capture checks. Full supported-scale
browsing acceptance, station interaction input and physical text entry remain in
the later acceptance child. No RPC, gameplay availability, catalogue, save schema
or inventory ordering changes.

Recipe child evidence, 2026-10-04: final affected UI build passed four actions.
Focused LocalBrowsing passed1/1, test exit0 at Temp/kr1/tests.log before final
focus/probe refinement. Final rendered18589 passed both peers and existing
transaction/modal/capture checks at Temp/KalmalaCrafting-516e8111904045dc856c7d17023e4365;
SelectionKept/Category/NoResults/Restored/SearchFocus all1. Prior startup-readiness
and programmatic SetText probe errors were repaired;18588 passed before final
internal text-focus guard. Ownership/five documentation/parser/diff checks passed;
mirror max209. These captures do not establish comprehensive new-control pixel
acceptance; full scale/contrast/owner integration remains the ordered final child.

## Build browsing

The standalone Construction Hammer Build menu defaults to All builds and its
category control cycles only All builds, Structural pieces, Stations and Camp
utilities. It never exposes cooking or other station-service production.
Classification reads the existing recipe output and placement support
contract: Floor and shelter pieces are structural; existing crafting stations
are Stations; other supported placeables (hearth, storage, station attachments
and grinding stone) are Camp utilities. Unknown or non-placeable outputs never
enter the Build grid. No catalogue metadata, identity or save is changed. Each
build card retains its named group above the canonical display name through
refresh. All builds groups structural/station/utility entries in that order;
Catalogue order preserves original relative order within groups, and Name
order sorts within groups. Individual group filters use ordinary catalogue/name
ordering.

Page Up/Down, focused button activation, text editing and modal
close/navigation remain available in the standalone Build menu.

Search and category scope intersect these placeables. Selection remains
canonical through sorting and falls back when filtered away; no results
disables actions and clears cards/details. Bootstrap structures with no station
requirement can still be produced from the Build menu. Recipes that require a
station, and station-attachment recipes, remain visible as placeable outputs,
but their production action is disabled here and names the matching service
menu. Workbench/Forge service contexts retain their station-scoped production
recipes. Existing ingredient counts, direct-material costs, preview, and
placement remain tied to the selected supported output. Browsing only copies
indices and cancels an obsolete local placement preview; server availability,
material validation and transactions remain independent.

Build no longer shows the old pack inspector, food-use, refuelling, repair,
upgrade, or chest lists. Inventory and food use remain in the Inventory menu;
repair, upgrade, and storage remain in their Workbench, Forge, and Chest
contexts. Campfire Interact replaces the old Build refuel action. **Light
hearth** remains in Build because Campfire Interact only adds fuel and does not
relight an extinguished hearth. The Build feedback block contains only the
latest action result and local placement-preview result; it no longer lists
nearby fires, constructions, workbenches, or carried-tool condition.

`Verify-Crafting.ps1` checks the obsolete-control visibility set, retained
placement and relight actions, filtered Build feedback, and visible Workbench
Tool Rack / Forge Anvil status. Its rendered review includes a clean Build
capture after browse recovery; Inventory menu browsing remains covered by
`Kalmala.UI.InventoryMenu.Selection`.

Child verification extends Kalmala.UI.Crafting.LocalBrowsing with exact supported
build coverage, disjoint populated groups, name search, group order, unknown
output exclusion, no-results and full source-order restoration. Rendered
Verify-Crafting additionally requires both peers' Build browsing Groups,
SelectionKept, CategoryKey and NoResults all1; Groups checks refreshed card
labels as well as filter membership. Full scale/contrast pixel acceptance and
owner privacy integration remain the final ordered child; physical controller
text input and packaging are not certified by these fixtures.

Build child evidence 2026-10-04T09:13:52.5454451Z: final UI build four actions passed; focused LocalBrowsing1/1/process and test exit0 at Temp/kbb1/tests.log before final card-label correction. Final rendered18591 passed both peers including refreshed group labels, canonical selection, category key, no-results, existing recipes/modal/authority and captures at Temp/KalmalaCrafting-ac6b78b4a5644854aba1de791c491708. Ownership/five documentation contracts/parser/diff/path checks passed; mirror maximum209. Full parent integration and supported-scale/contrast pixel acceptance remain pending.

## Parent acceptance — 2026-10-04

The final ordered acceptance child and browsing parent are complete. Grouped
inventory now uses independently sized grid rows with headings spanning four
columns; the uniform grid made headings as tall as the tallest tool card,
creating large empty gaps. No RPC, gameplay, availability, save or source-order
contract changed. Query/category/order still have widget lifetime only.

Final parent-level evidence:

- UE5.8.2 KalmalaEditor Win64 Development build passed in Temp/ka with normal
  LOCALAPPDATA/UnrealBuildTool access; final compile/link took four actions.
- Full Kalmala queue passed106/106, process and test exit0 at
  C:/Users/Ville/AppData/Local/Temp/kba2/all.log after the final layout change.
- Rendered Verify-Crafting passed18595 at1280x720/100%/standard and18597
  at1024x768/150%/high contrast. Both peers passed inventory browsing,
  recipe selection/category/no-results/search focus, build groups/selection/key
  navigation, modal/HUD restoration and existing server rejection/payment/final
  state checks. All twenty required captures per run completed.
- Standard root:
  C:/Users/Ville/AppData/Local/Temp/KalmalaCrafting-5f628428509e4f5ab4f5bdc5a9b381bb.
  High-contrast root:
  C:/Users/Ville/AppData/Local/Temp/KalmalaCrafting-372c908241b247eaba385d29a1f26524.
- Final Verify-Inventory18596 passed owner privacy, capacity, read-only UI and
  server grant/rejection checks at
  C:/Users/Ville/AppData/Local/Temp/KalmalaInventory-b0572dbe1ce34bb5978f8082f173fbec.
- Presentation ownership, all five M5 documentation contracts, script parsing,
  diff and path checks passed; mirror maximum209, below260.

Representative source PNG review accepted Cooking and all three named groups,
category/name controls, selected/unavailable text, no-results recovery/disabled
Craft, grouped inventory headings/cards, scrolling and fixed Close control at
both settings. Twenty-four unchanged source captures are retained in
`menu-browse/`, plus a lossless enlarged client item crop and `pixels.txt`.
Paired central bright-text masks differ by only5–25 pixels in high contrast,
against24,443–33,762 bright pixels; this supports review rather than proving
pixel identity. A client inventory preview omitted the Joiner's bench label;
the source crop contains the name/count. No renderer defect is asserted.

The first capture-helper compile failed on an unbraced logging macro and was
fixed. A subsequent rebuild hit a loaded UI DLL while the earlier rendered
peers were active; after they exited the final rebuild passed. Earlier full
queue106/106 and rendered18592/18594 passed before the grouping correction;
only the final evidence above accepts the completed parent.

Limits: normal scroll clipping remains intentional at enlarged text; long
recipe/detail sections require scrolling. Programmatic text delegates and
Slate key events do not certify physical keyboard/controller text entry,
cooking-station physical interaction, exhaustive viewport combinations,
package inclusion or clean-HEAD/package acceptance. Earlier pending statements
above are historical; full parent verification is now complete under these limits.

## Owner-local Favorites slice

The crafting/build menu now cycles All, Other crafting, Cooking, All builds,
Structural pieces, Stations, Camp utilities, and Favorites. Favorites is the
manually bookmarked subset of the same active catalogue and intersects the
existing station filter and name query. The focusable Add to Favorites / Remove
from Favorites button applies to the current selected recipe or build entry;
it performs no gameplay action. Cards and tooltips identify bookmarks with the
text label Favorite, and unavailable reasons remain visible. If the selected
entry is removed while Favorites is active, ordinary browse fallback selects
the first remaining entry; an empty view leaves Craft disabled.

Bookmark IDs live in the owning `UKalmalaCraftingSubsystem` and are shared by
that local player across menu reopen and pawn replacement. The set is capped at
the lesser of 256 and the current recipe-catalogue size; opening the menu
prunes IDs no longer in the catalogue. This first slice ends with the local
player subsystem and does not write settings, world/player saves, or network
state. Usage ranks, accepted-action Recent entries, and configurable corner
badges remain later work; see `docs/41-recipe-activity.md`.

## M-key world map position memory

The local-player map widget remains alive while the M-key map is collapsed, so
its marker visibility categories, focused legend category, and selected
personal pin already remain transient to that local session. Closing cancels
active legend navigation and hover state; reopening returns keyboard focus to
the map and does not toggle, complete, or remove a pin. A pin hidden by its
category filter remains selected and its existing status text reports that it
is filtered. Existing removal and world-change paths keep their selection
fallbacks.

The map has no separate scrolling list. Its applicable viewport position is
the current map centre and zoom radius, so the first successful open keeps the
existing recenter/optional whole-world default and later opens retain the
owner's last local pan, zoom, and fit mode. A changed viewport aspect ratio
recomputes the map extent from the retained radius; it does not reset the
centre. Explicit Recenter still returns to the owning pawn and exits whole-world
fit. These values and the marker preferences are widget/view-model state only:
they add no settings write, map save field, RPC, replication, or gameplay action.

The M11 map-memory acceptance pass should reopen after pin filtering and
selection, pan and zoom, then exercise recenter and viewport/UI-scale changes.
Check marker focus/visibility and modal input on separate local owners, alongside
the existing `Kalmala.UI.WorldMap.LocalPresentation` and rendered
`Scripts/Verify-WorldMap.ps1` coverage. Cross-restart view restoration is not
part of this contract.
