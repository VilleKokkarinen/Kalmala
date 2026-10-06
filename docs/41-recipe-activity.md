# Local recipe and build activity

## Ownership and lifetime

Favorites, usage counts, rank results, and Recent shortcuts are local
presentation state owned by the `ULocalPlayer` crafting UI subsystem. They do
not change catalogue definitions, availability, server transactions, gameplay
state, replicated properties, or world/player save schemas. They survive menu
close/reopen and pawn replacement while that local-player subsystem exists;
they are cleared when it is destroyed and are not persisted across process
restarts. The local UI never infers an accepted action from selection, a button
press, inventory appearance alone, another peer, or a replayed snapshot.

Bookmarks use canonical active `RecipeId` values. Unknown or retired IDs are
ignored and pruned. The set is bounded by the smaller of 256 entries and the
current recipe catalogue size. The Favorites category intersects the existing
station scope and search query and keeps the existing unavailable-state text;
it does not add hidden entries or change recipe ordering. Removing the selected
bookmark from Favorites follows the menu's established selection fallback,
and an empty result disables the existing action.

## Accepted-action history contract

The crafting component emits owner-only accepted-action receipts only after an
existing server-side craft or placement has committed. Each receipt carries a
monotonic sequence for that character component, a canonical recipe/menu ID,
and one of the three action kinds below. The server retains a rolling maximum
of 64 receipts so multiple accepted actions can survive property replication
coalescing; the 64-bit sequence saturates instead of wrapping. The local
subsystem silently establishes a baseline on first
observation and pawn replacement, then consumes each newer sequence once. A
component replacement does not reset the local player's accumulated history.
Rejected, failed, stale, duplicate, replayed, selected, previewed, or merely
requested actions do not update history. The receipt reports an outcome
already chosen by the server; it cannot authorize a transaction or supply a
reward. A build output with no unique current recipe/menu ID is omitted.

Accepted actions have three disjoint buckets:

- `BuiltPiece`: successful direct placement of a construction piece.
- `CookedRecipe`: successful recipe action whose existing catalogue skill is
  Cooking.
- `CraftedItem`: successful non-cooking recipe action. Cooking never enters
  this bucket.

Counters are local, non-negative, saturating integers keyed to current
canonical menu IDs. Records for IDs absent from the active catalogue are
pruned, and unused entries with count zero receive no rank. Each bucket ranks
at most three non-zero entries by descending accepted-action count, then by
`RecipeId.LexicalLess` for a locale-independent deterministic tie. The displayed
medals are gold, silver, and bronze in that order; ties do not share a medal.

Recent state holds at most one latest accepted ID per bucket. A new accepted
build replaces only the previous built piece; a new cooking action replaces
only the previous cooked recipe; a new other-crafting action replaces only
the previous crafted item. The Favorites category includes these current
Recent shortcuts even when they were not manually bookmarked. A canonical ID
appearing in multiple sources is shown once, with separate text markers so
Favorite, Rank, and Recent remain distinguishable. Ordinary recipe/build
cards also mark their matching Recent entry.

The local-player subsystem exposes bucket count, rank, and Recent queries for
the menu presentation. It validates that receipt IDs still resolve in the
current catalogue and that the receipt kind agrees with the existing Cooking
classification or build-menu membership. Each count map holds at most one entry
per current recipe ID per bucket and saturates at the unsigned 32-bit maximum.

## Presentation contract

Favorites, rank, and Recent remain distinguishable without colour. The theme
configures Favorite treatment as a bottom-right star, border, or both; rank
treatment uses a medal-coloured label with `Rank` text; and Recent uses a clock
badge plus `Recent` text. The default Favorite treatment is both star and
frame. Cards reserve separate right-aligned Rank and Recent rows, then place
Favorite at the lower right; all three labels retain their meaning and do not
overlap. Reduced motion keeps marker changes immediate and static; it does not
suppress an essential marker. Existing text scale, contrast, focus, selection,
and modal input behavior continue to apply.

## Implementation status

The first implementation slice adds the local session bookmark set, a
selected-entry toggle, and Favorites browsing. The accepted-action slice adds
the bounded owner-only receipt queue, unique sequence consumption, local bucket
counts, deterministic top-three rank queries, and one Recent ID per bucket.
The marker slice adds configurable star/frame/color treatment, three separate
card rows, Recent-only shortcuts in Favorites, and a two-peer presentation
fixture that checks ordinary-slot Recent markers and distinct local-owner
recipes. Focused authority/replay automation, theme and local-browsing tests,
and both rendered host/client modes pass; the four reviewed card captures are
retained in `docs/ui-recipe-activity/`. The Favorites parent integration child
is complete, while the wider M11 acceptance matrix remains open.


## Parent integration result

The isolated `KalmalaEditor Win64 Development` build passed after two
`UWidget`-member shadowing errors were fixed by renaming locals in
`KalmalaCraftingSubsystem.cpp`. The focused headless automation queue passed
`Kalmala.Gameplay.Crafting.NetworkContract`,
`Kalmala.UI.Crafting.ActivityReceiptReplay`,
`Kalmala.UI.Crafting.LocalBrowsing`, and
`Kalmala.UI.Theme.LocalPresentation`. Receipt coverage confirms the owner-only
property has no SaveGame flag, plus server-authority gating,
first-observation baselining, one-time receipt consumption,
repeated-snapshot suppression, and separate build/cooking/other-craft buckets.
Local-player test fixtures use valid `ULocalPlayer` outers.

The standard rendered host/client run at 1280x720/100% and the reduced-motion
high-contrast run at 1024x768/150% both passed marker coexistence, ordinary
Recent rows, unbookmarked Recent shortcuts, static motion, owner isolation,
existing server rejection/payment checks, and final authoritative state. All
four `*-activity-markers.png` captures were visually reviewed and retained in
`docs/ui-recipe-activity/`. The Favorite, Rank, and Recent labels remain visible
in the high-contrast captures with no row overlap.

The active catalogue has builds and cooking recipes but no ordinary
non-building, non-cooking recipe. Automation therefore appends a transient
ordinary-craft fixture row only during the `-KalmalaCraftingTest` review session
and in the receipt automation test; shipped catalogue data and save schema are
unchanged. The live crafted-item bucket cannot be observed from a production
recipe until such a recipe exists. This parent integration result does not
complete the wider M11 acceptance matrix.

The owner receipt buffer contains only its newest 64 accepted actions. The UI
subsystem polls it every tick and consumes every still-buffered sequence once;
if more than 64 accepted actions arrive between observations, older receipts
have already been discarded by the server. This bounded loss case remains a
known limitation for parent-level verification.
