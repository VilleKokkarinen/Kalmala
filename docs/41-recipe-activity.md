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
recipes. Focused automation and both rendered motion modes remain unrun;
parent integration and rendered acceptance remain open under the M11 backlog
item.


## Verification handoff

The implementation child may be checked after careful source review,
`git diff --check`, and the introduced-path audit recorded in `PROGRESS.md`.
Before checking the feature parent, run focused
`Kalmala.UI.Crafting.LocalBrowsing` coverage for add/remove, exact active-ID
filtering, station/query intersection, selection fallback, empty-state action
disabling, deterministic ranks, Recent shortcuts without bookmarks, and
per-local-player isolation. Rendered host/client checks in normal and
reduced-motion modes should cover marker coexistence, all three Recent buckets,
distinct owner-local favorites, and high-contrast/text-scale readability. The
review capture is named `*-activity-markers.png`. The combined parent gate also
checks accepted counts, receipt replay, and
marker overlap. These implementation slices have not had an editor build,
automation run, or rendered acceptance; those remain deferred to the ordered
parent verification work.

The owner receipt buffer contains only its newest 64 accepted actions. The UI
subsystem polls it every tick and consumes every still-buffered sequence once;
if more than 64 accepted actions arrive between observations, older receipts
have already been discarded by the server. This bounded loss case remains a
known limitation for parent-level verification.
