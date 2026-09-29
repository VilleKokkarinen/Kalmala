# M9 camp and equipment recipe contract

This page records the current runtime catalogue and the user-directed removal of
the earlier raised-storage and Smokehouse additions. The schema-version-4
`Content/Data/GameCatalogues.json` is the source for item and recipe
definitions. No property or value in the JSON contains the Kit suffix.

## Current recipe rules

Recipe access is limited by the required raw materials, any visible same-world
station listed in the `RequiredStation` array, the bounded batch, and live heat
at food-processing stations. The server derives the heat rule from the
resolved Cooking Rack, cauldron, or Smoke Frame identity; recipe metadata does
not opt recipes into a generic hearth gate. There are no recipe skill-level
requirements, `AlternateStation`, `OutputTool`, or legacy fire flags. Cooking
experience remains a server-owned reward after a successful inventory
exchange.

All construction recipes consume the raw materials listed in the catalogue.
Former `ConstructionSupply` values were expanded to 3 Wood and 2 Fibre each;
duplicate ingredients were merged. The Construction Hammer builds the hearth,
floor, wall, and roof directly from those recipe costs. Other buildable
construction recipes produce their internal construction identity through the
existing validated placement path.

| Retired item or recipe | Current behavior |
| --- | --- |
| `Fuel` | Removed. Hearth placement and refuelling consume raw Wood, Lightwood, Densewood, or Coal directly. Cooking recipes consume only their listed ingredients. |
| `ConstructionSupply` | Removed. Recipes consume its former 3 Wood + 2 Fibre value directly. |
| `RaisedStorage` | Removed. `Storage` produces the normal Chest only. |
| `Smokehouse` | Removed. Smoking recipes use the Smoke Frame only. |
| `Timber` | Removed as an intermediate recipe; construction consumes its raw Wood/Fibre value. |

Any one of Wood, Lightwood, Densewood, or Coal adds 60 seconds of hearth fuel.
The server burns one fuel second per elapsed server second while the hearth is
lit. Cooking does not consume extra inventory fuel per serving or batch, and
clients cannot select or charge fuel through a recipe. Coal is accepted as a
fuel item, but the current world loot catalogue has no natural Coal source.

## Compatibility and authority

The JSON source remains schema 4 and the storage SaveGame remains schema 1.
When a saved chest is loaded, legacy Fuel quantity converts one-for-one to
Wood; each legacy ConstructionSupply converts to 3 Wood and 2 Fibre. The
migration validates and splits resulting stacks within existing item and chest
limits. It preserves the former material value without adding or changing save
fields. RaisedStorage and Smokehouse were session-only additions and were not
stored by schema 1.

The server validates recipe identity, batch bounds, item IDs, required
stations, station-local hearth access and heat for food processing, and output
capacity before publishing an inventory change. It selects a visible station
in the same world; clients cannot choose station actors, ingredient costs,
outputs, rewards, fuel type, or construction results. The normal Chest keeps its existing
server-selected transfer rules, bounded contents, and owner-only inventory
view.

## No-hearth Drying Line

The schema-4 catalogue adds a clean `DryingLine` output alias that the loader
maps to the stable runtime `DryingLineKit` construction identity. Its one-use
construction recipe costs 6 Wood, 7 Fibre, and 1 Densewood at a visible
same-world Workbench within 250 cm. The two retired ConstructionSupply units
were converted to their accepted 3 Wood + 2 Fibre value each and merged with
the original Fibre cost. The procedural line uses the validated construction
placement path, has a five-line server-session cap, and is explicitly excluded
from construction save schema 1 until M9 migration is approved.

`DryBoarMeat` and `DryDeerMeat` each exchange one matching raw meat for one
`DriedFieldMeat`. Each recipe requires a visible same-world Drying Line within
250 cm, caps the batch at three servings, and requires no hearth or raw fuel.
One fixed 10 Cooking experience is awarded per accepted request after the
complete inventory exchange. Dried field meat stacks to 20 and uses the
existing one-slot, 120-second `SteadyMeal` effect; active meals still cannot
stack, refresh, or replace one another. Item inventory, Cooking progression,
and meal status remain transient.

## Optional content still deferred

A storm-rated shelter piece remains deferred until its behavior differs from
the existing roof and windbreak. An insulated wrap remains deferred until
there is an approved equipment-slot and effect contract.

## Verification

After a forced editor build, run the catalogue, crafting transaction and
network, campfire processing, Drying Line, placement preview, construction save, and storage
save/transfer automations described in `docs/07-development-setup.md`.
The M9.RaisedStorage and M9.Smokehouse automation names now assert that those
items and placement identities are absent and that the normal Chest and Smoke
Frame remain. Run `Scripts/Verify-Crafting.ps1` to verify server-selected raw
fuel, direct material transactions, rejection gates, and exact peer inventory.
The peer run checks the Drying Line's invalid batch, missing Workbench, and
missing-line processing requests from the client, with no inventory or Cooking
experience mutation. Both owner panels must expose camp recipe costs, station
requirements, the no-hearth/no-fuel drying rule, smoking fuel, and direct
hammer costs as readable text. Pair it with `Scripts/Verify-InventoryReconnect.ps1`
and `Scripts/Verify-Storage.ps1` for owner-only inventory and Chest snapshots,
rejected transfers, and stable Chest identities over reconnect. Null-renderer
text checks do not claim rendered layout or assistive-technology acceptance.
No RPC, replicated gameplay field, or saved-data schema changes for this
contract.
