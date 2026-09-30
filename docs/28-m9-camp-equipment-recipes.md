# M9 camp and equipment recipe contract

This page records the current runtime catalogue. The schema-version-4
`Content/Data/GameCatalogues.json` is the source for item and recipe
definitions. No property or value in the JSON contains the Kit suffix.

## Current recipe rules

Recipe access is limited by the required raw materials, any visible same-world
station listed in the `RequiredStation` array, the bounded batch, and live heat
at food-processing stations. The server derives the heat rule from the
resolved Cooking Rack, cauldron, or Frying Pan identity; recipe metadata does
not opt recipes into a generic hearth gate. Schema 4 contains no Smoke Frame
item or recipe. There are no recipe skill-level
requirements, `AlternateStation`, `OutputTool`, or legacy fire flags. Cooking
experience remains a server-owned reward after a successful inventory
exchange.

## Current food recipes

These six food recipes and their outputs are the active cooking catalogue.
Every batch is capped at five, and every output stack is capped at twenty.
The server requires the declared visible same-world station and a usable lit
hearth with positive heat within 250 cm of both the player and station.

| Recipe ID | Ingredients per serving | Required station | Output item |
| --- | --- | --- | --- |
| `CookedBoarMeatRecipe` | `BoarMeat` x1 | Cooking Rack | `CookedBoarMeat` |
| `CookedDeerMeatRecipe` | `DeerMeat` x1 | Cooking Rack | `CookedDeerMeat` |
| `MeatStewRecipe` | `BoarMeat` x1, `DeerMeat` x1, `Carrot` x2, `Potato` x2 | Cauldron | `MeatStew` |
| `RootVegetableSoupRecipe` | `Carrot`, `Potato`, `Rutabaga`, `Onion` x1 each | Cauldron | `RootVegetableSoup` |
| `RoastedRootVegetablesRecipe` | `Carrot`, `Potato`, `Onion` x1 each | Frying Pan | `RoastedRootVegetables` |
| `DeerRootRoastRecipe` | `DeerMeat`, `Rutabaga`, `Onion` x1 each | Frying Pan | `DeerRootRoast` |

`HearthBroth` remains a catalogue item with a stack limit of twenty but has no
production recipe. The former `RoastedFieldMeat`, `SmokedFieldMeat`, and
Smoke Frame IDs are absent from schema 4. The steady-meal item use remains
transient and does not add save data.

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
| `Timber` | Removed as an intermediate recipe; construction consumes its raw Wood/Fibre value. |
| Smoke Frame and legacy smoke/roast recipe IDs | Absent from schema 4. Current cooking uses the six recipe IDs in the table above. |

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
fields.

The server validates recipe identity, batch bounds, item IDs, required
stations, station-local hearth access and heat for food processing, and output
capacity before publishing an inventory change. It selects a visible station
in the same world; clients cannot choose station actors, ingredient costs,
outputs, rewards, fuel type, or construction results. The normal Chest keeps
its existing server-selected transfer rules, bounded contents, and owner-only
inventory view.

## Optional content still deferred

A storm-rated shelter piece remains deferred until its behavior differs from
the existing roof and windbreak. An insulated wrap remains deferred until
there is an approved equipment-slot and effect contract.

## Verification

After a forced editor build, run the catalogue, crafting transaction and
network, campfire processing, placement preview, construction save, and storage
save/transfer automations described in `docs/07-development-setup.md`.
Run `Scripts/Verify-Crafting.ps1` to verify server-selected raw fuel, direct
material transactions, rejection gates, and exact peer inventory. Pair it with
`Scripts/Verify-InventoryReconnect.ps1` and `Scripts/Verify-Storage.ps1` for
owner-only inventory and Chest snapshots, rejected transfers, and stable Chest
identities over reconnect. Null-renderer text checks do not claim rendered
layout or assistive-technology acceptance. No RPC, replicated gameplay field,
or saved-data schema is added.
