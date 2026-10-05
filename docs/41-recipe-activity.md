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

The next implementation slice will consume an owner-only accepted-action
receipt emitted only after the existing server-side action has committed. Each
receipt carries a per-owner monotonic sequence and a canonical recipe/menu ID.
The local subsystem silently establishes a baseline on first observation and
pawn replacement, then consumes each newer sequence at most once. Rejected,
failed, stale, duplicate, replayed, selected, previewed, or merely requested
actions do not update history. The receipt reports an outcome already chosen
by the server; it cannot authorize a transaction or supply a reward.

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

## Presentation contract

Favorites, rank, and Recent remain distinguishable without colour. The theme
will configure Favorite treatment as a bottom-right star, border, or both;
rank treatment as a medal plus `Rank` text; and Recent treatment as a clock
badge plus `Recent` text. When a single card has multiple markers, each label
retains its own meaning and occupies a reserved non-overlapping area. Reduced
motion keeps all state changes immediate and static; it does not suppress an
essential marker. Existing text scale, contrast, focus, selection, and modal
input behavior continue to apply.

## Implementation status

The initial implementation slice adds the local session bookmark set, a
selected-entry toggle, and Favorites browsing. Server-accepted action receipts,
counts, rank and Recent badges, configurable marker treatments, and combined
rendered acceptance remain separate unchecked work under the M11 backlog item.


## Verification handoff

The implementation child may be checked after careful source review,
`git diff --check`, and the introduced-path audit recorded in `PROGRESS.md`.
Before checking the feature parent, run focused
`Kalmala.UI.Crafting.LocalBrowsing` coverage for add/remove, exact active-ID
filtering, station/query intersection, selection fallback, empty-state action
disabling, and per-local-player isolation. Rendered host/client checks should
cover the selected-action label, card Favorite text, keyboard/controller
category cycling, and high-contrast/text-scale readability. The combined parent
gate also checks accepted counts, rank/Recent coexistence, reduced motion, and
marker overlap. The initial slice has not had an editor build, automation run,
or rendered acceptance; those remain deferred.
