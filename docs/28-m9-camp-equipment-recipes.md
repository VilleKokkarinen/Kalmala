# M9 optional camp and equipment recipe contract

This page selects a bounded set from the candidate examples in
`04-roadmap.md`. It defines intended recipes and server checks before runtime
implementation. Existing item, recipe, construction, storage, hearth, food,
and skill authorities remain the implementation seams.

## Accepted recipes

All costs use canonical item IDs. Kit recipes produce one kit, allow one batch,
and require a visible, same-world level-1 Workbench within 250 cm. Assembly
does not award skill experience, matching the existing construction recipes.

| Recipe ID | Output | Ingredients | Accepted use |
| --- | --- | --- | --- |
| `RaisedStorage` | `RaisedStorageKit` | 3 `Densewood`, 2 `ConstructionSupply`, 2 `Fibre` | Shared chest storage with the existing 16 unique-stack limit; its accepted construction is exempt from server rain wear. |
| `Smokehouse` | `SmokehouseKit` | 3 `Densewood`, 2 `ConstructionSupply`, 2 `Fibre` | Roofed alternative to `SmokeFrameKit` for the existing smoked-meat recipes; it adds no private stock, timer, fuel store, or recipe bonus. |
| `DryingLine` | `DryingLineKit` | 1 `Densewood`, 2 `ConstructionSupply`, 3 `Fibre` | Visible same-world processing station for the two drying recipes below. |

Each accepted kit is an original procedural construction presentation using the
existing paid placement and collision validation path. It has one canonical
kit ID. Clients provide placement intent only; the server revalidates the
requested transform and determines the placement outcome. The raised chest
reuses the existing storage transfer rules and 16-stack container bound. The
smokehouse's accepted roof geometry participates in the existing server
shelter and hearth rain-protection traces.

| Recipe ID | Input per serving | Output per serving | Station and bounds |
| --- | --- | --- | --- |
| `DryBoarMeat` | 1 `BoarMeat` | 1 `DriedFieldMeat` | Visible same-world `DryingLineKit` within 250 cm; max batch 3; no hearth or fuel. |
| `DryDeerMeat` | 1 `DeerMeat` | 1 `DriedFieldMeat` | Visible same-world `DryingLineKit` within 250 cm; max batch 3; no hearth or fuel. |

`DriedFieldMeat` is a new catalogue item capped at 20 per stack. Eating it uses
the existing one-slot, 120-second `SteadyMeal` effect. That effect cannot
stack, refresh, or replace an active meal; an active meal rejects consumption
without removing the dried meat. A successful drying request awards one fixed
10 Cooking experience after the complete pack exchange succeeds, regardless of
batch size. Missing station, inputs, output capacity, or valid batch leaves
inventory and experience unchanged.

The smokehouse accepts the existing Cooking-level-2 boar and deer smoke
recipes unchanged:
each serving consumes one matching raw meat and one `Fuel`, outputs one
`SmokedFieldMeat`, caps the batch at 3, and awards 10 Cooking experience per
accepted request after payment. The server requires a visible, same-world
smokehouse within 250 cm of the owner and a usable Lit hearth with finite,
positive heat within 250 cm of both owner and smokehouse. Hearth fuel burns at
its ordinary rate; the recipe's one `Fuel` per serving remains the explicit
additional processing cost. The roof may protect the hearth from rain through
the existing shelter check, but cannot create heat, light a fire, or bypass
station, range, visibility, or fuel validation.

## Server transaction requirements

- The client submits only an allowlisted recipe ID and bounded batch intent for
  processing, or a kit ID plus ordinary placement intent for construction. The
  client never selects a station actor, ingredient list, cost, output, heat,
  weather state, effect, skill award, chest contents, or construction result.
- The server checks authority and the controlled pawn, validates catalogue IDs,
  unique ingredients, positive quantities, per-item stack caps, batch bounds,
  skill and output capacity, and selects a visible same-world compatible
  station within the existing 250 cm range.
- The server builds the full inventory candidate before publishing it. It
  grants Cooking experience only after the processing inventory exchange
  succeeds. Eating a prepared food item separately checks the existing one-meal
  slot. Rejection changes no ingredients, outputs, active status, skill
  ledger, construction actor, or storage contents.
- Construction placement reuses the existing server terrain, slope, bounds,
  collision, kit, and construction-ID checks. Storage requests continue to
  select the nearest visible registered chest server-side and request one
  allowlisted item per transfer; the owner alone receives its bounded content
  snapshot and the actor never replicates private contents.
- `RaisedStorageKit`, `SmokehouseKit`, `DryingLineKit`, and `DriedFieldMeat`
  remain disabled in normal save/restore paths until the M9 versioned
  persistence and migration task validates their world/player scope, bounds,
  compatibility, and round trips. Do not extend schema 1 or enable partial
  persistence as part of the recipe implementation.

## Deferred examples

- **Storm-rated shelter piece:** defer recipe acceptance until its gameplay
  difference from the existing binary roof and windbreak shelter checks is
  specified. Do not add an unmeasured weather multiplier or a renamed duplicate
  construction kit.
- **Insulated travel wrap:** defer until there is an approved existing
  equipment slot and effect contract. Do not add a parallel wearable inventory
  or client-authored warmth modifier.

## Verification for implementation

After the forced editor build, add a focused catalogue and transaction check
for each accepted recipe. Reuse `Kalmala.Gameplay.Crafting.Transactions`,
`Kalmala.Gameplay.Crafting.NetworkContract`,
`Kalmala.Gameplay.Construction.LocalPreview`,
`Kalmala.Gameplay.Construction.SaveContract`,
`Kalmala.Gameplay.Food.CampfireProcessing`, and the `Kalmala.Gameplay.Storage`
contract suite. Exercise accepted and rejected host/client requests for wrong
or distant stations, out-of-range or blocked use, missing inputs, output
capacity, meal-slot consumption rejection, malformed batches, and owner-only
chest contents. Run the storage and crafting peer verifiers after the
persistence gate is approved. The Smokehouse and Drying Line portions remain
design-only; no implementation or peer-playtest claim is made for them.

## Runtime increment: raised storage

`RaisedStorage` is implemented as a paid `RaisedStorageKit` using three
`Densewood`, two `ConstructionSupply`, and two `Fibre` at a visible same-world
level-one Workbench within 250 cm. Placement reuses the existing server ground,
slope, bounds, collision, and kit checks. The raised chest shares the existing
server-selected storage interaction, 16-unique-stack bound, and owner-only
contents view, and is exempt from rain wear.

The raised kit is a session-only M9 actor. Its bounded contents live in the
authoritative GameMode session map, do not enter either schema-one save, and
are cleared with that server session. Schema-one construction validation
rejects the new kit; the save version and record shape are unchanged. The
existing storage transfer intent still accepts only an item ID, and storage
failure leaves the player's pack unchanged. The owner is told that raised
construction and contents last for the current server session.

Focused verification: `Kalmala.Gameplay.M9.RaisedStorage` covers catalogue
identity and cost, rain immunity, same-actor session registration, bounded
server contents, rejection of forged actor identity, and schema-one exclusion.
The existing storage transfer and network contract suites continue to cover
the 16-stack transaction and owner-only replicated view. A rendered two-peer
raised-chest walkthrough has not run.

The Smokehouse and Drying Line recipes remain design-only; this increment does
not complete the optional camp/equipment backlog task or enable M9 migration.
