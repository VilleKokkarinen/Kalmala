# M9 carried-tool inventory contract

**Current player inventory:** the 2026-10-09 owner request places these tool
records in the same forty-cell inventory as item stacks. Records retain their
existing condition/level and persistence roles; they provide no extra player
carrying capacity or separate toolbelt. See `47-inventory-grid.md`. The
sections below retain the tool gameplay and persistence contract.

The first tool-progression increment replaces separate per-tool condition
properties with the character's bounded `CarriedTools` record array. Each
`FKalmalaToolState` carries a canonical tool ID, condition, and authored tool
level. Tool level belongs to the tool record and stays separate from the
player's skill progression ledger.

The server creates the initial three first-wave gathering tools and a level-one
Construction Hammer when an authoritative character begins play. The hammer
has 100 condition, no durability cost, and an original right-hand procedural
model. The owner-local B / View build menu requires the carried hammer; each
construction request independently validates its server-owned record. The
inventory is bounded to six records for the three first-wave gathering tools,
the hammer, and two M9 axe tiers. Uncrafted Bronze and Iron Axes remain absent
from the starting list.

## M9 axe progression entries

`FKalmalaToolProgressionContract` defines the two server-authored axe paths.
The target tool level equals the required effective station level, following
the roadmap's station-match rule.

| Tool | Target level | Prerequisite | Station | Materials |
| --- | ---: | --- | --- | --- |
| Bronze Axe | 1 | None | Workbench level 1 | 4 Wood, 3 Stone, 2 Fibre |
| Iron Axe | 2 | Bronze Axe level 1 | Forge level 2 | 3 Lightwood, 2 PeatAmber, 4 Stone, 2 Fibre |

All costs use existing item-catalogue IDs. Densewood is excluded from the Iron
Axe recipe because Iron Axe is itself required to harvest it. The catalogue
validates tool identities, one-level progression, station/target matching,
and bounded costs against the item catalogue.

The paid Forge recipe uses 5 Lashed timber and 6 Fieldstone. It uses the
existing generic construction placement and schema-1 construction record;
Workbench and Forge levels are derived from their kit identity, with each base
station at level 1. No new save field stores a station level.

## Paid station attachments

Two paid construction kits add one effective level to a matching nearby
station: the Workbench tool rack costs 2 Lashed timber and 2 Reed fibre; the
Forge anvil costs 3 Lashed timber and 4 Fieldstone. Each recipe requires its
matching visible same-world Workbench or Forge within the normal 250 cm
station-use range. Placement is server-derived from the pawn, rechecks the
matching usable station, and requires the attachment to land within 125 cm of
that station. One attachment upgrades a station from level 1 to level 2; a
second attachment at the same station is rejected. Effective level is capped
at the authored level-2 tier.

The placed attachment actor and level bonus are session-only until the M9
versioned save contract passes migration and round-trip checks. Attachments do
not enter the existing construction save, and no tool or construction save
schema changed. The server limits live attachments to 32, checks placement and
kit costs before committing, and derives level from initialized same-world
construction actors. Clients submit only the attachment kit ID for placement
and the tool ID for an upgrade; they cannot select the station or author a
distance, level, or outcome.

The owner crafting panel now offers Bronze Axe crafting and Iron Axe upgrading.
The client sends only the target tool ID. The server selects the nearest
visible same-world station of the required family within 250 cm, derives its
base level, validates an exact level match, checks the carried-tool prerequisite
and condition bounds, and builds both the paid inventory candidate and carried
tool candidate before committing either. Bronze Axe is added at level 1 with
full condition from a level-1 Workbench. Iron Axe replaces a carried level-1
Bronze Axe and requires a level-2 Forge. The level-1 Forge rejects that
upgrade until a nearby paid anvil is placed and accepted by the server.
Rejected calls leave materials and carried tools unchanged.

The Iron Axe level-two recipe also requires the owner's server-ledger Crafting
skill to have reached level 5, which carries the existing second-tier unlock.
The level, unlock mask, Bronze Axe level and condition, matching level-two
Forge, and material exchange are checked before either candidate is committed.
The upgrade replaces the carried Bronze Axe in its existing tool-record slot
and creates a full-condition Iron Axe. The owner crafting panel shows the
required Crafting level, and a rejected request explains the missing
second-tier unlock. The client still submits only the target tool ID; skill,
carried-tool state, station, costs, and result are derived by the server.

Axes remain absent from starting inventory. The existing owner-only
`CarriedTools` replication carries the server-owned tool level and condition;
schema-2 player-discovery saves now preserve those records across reconnects.
The client still submits only tool intent. See the M9 free selected-tool repair
section below for the repair contract.

## M9 station transaction verification

Kalmala.Gameplay.M9.ToolProgressionCatalogue checks the axe and attachment
entries, compatible station families, bounded level derivation, placement
range and duplicate rejection, paid recipes, and item-catalogue-valid costs.
Kalmala.Gameplay.M9.ToolStationProgression checks paid Bronze
Axe creation and Iron Axe replacement, exact station family/level validation,
prerequisite checks, full-condition output, material consumption, and
no-mutation rejection. It verifies that level 1 and level 4 Crafting cannot
perform the Iron Axe upgrade, level 5 can, and a missing skill ledger fails
closed without changing candidate tools or materials. It also verifies that the tool progression RPC carries
only the target tool ID, without client-supplied station, level, cost, or result.

`CarriedTools` is replicated with `COND_OwnerOnly`. Harvest and repair code
reads and changes a record on the server; existing client requests continue to
send only tool or recipe intent. The detail array is not client-authored;
schema-2 player saves preserve the owner's validated tool records. Tool and
workstation levels remain derived from server-owned tool records and placed
attachment actors. Free selected-tool repair
is available at a Workbench or Forge; Grinding Stone `Repair All` and its
cosmetic pose are implemented and documented below.

## M9 free selected-tool repair

The owner crafting panel has a separate repair action for each of the three
starting tools and both carried axe tiers. Each request sends only the chosen
tool ID. The server finds that tool in the owner's carried records, validates
a visible same-world Workbench or Forge within 250 cm, and restores a damaged
or zero-condition record to its authored maximum. The action spends no pack
materials and awards no Crafting experience. Unknown, absent, full-condition,
or out-of-range tools leave condition and inventory unchanged. Tool level is
preserved, and the existing `CarriedTools` array remains owner-only. Accepted
repair writes the complete tool candidate to the matching schema-2 player slot
before publishing the new condition.

This M9 rule retires M7's material-paid repair and zero-condition replacement
recipes.

## M9 Grinding Stone Repair All

The paid `GrindingStoneKit` costs 2 Lashed timber and 4 Fieldstone and is
crafted at a visible same-world Workbench within 250 cm. It uses the existing
construction placement checks and schema-1 construction record; the Grinding
Stone adds no save fields or repair inventory. The owner interacts with the
placed stone to request Repair All. The server revalidates that exact accepted
stone as same-world, visible, and within 250 cm before reading the owner's
server-owned `CarriedTools` array.

Repair All builds and validates a complete candidate for the bounded list
before publishing it. Every damaged or zero-condition known tool returns to
its authored maximum; already-full tools remain unchanged, and identity, list
slot, and tool level are preserved. Unknown, duplicate, oversized, or malformed
tool state rejects the whole action without partial repair. The client sends
no tool list, tool ID, or condition value. Repair costs no materials and gives
no Crafting experience. The result and detailed tool condition remain
owner-only.

After the server accepts Repair All, it sends a parameterless cosmetic
multicast to the character's procedural player model. Each peer locally plays
a 1.2-second, three-stroke sharpening pose: one arm braces while the other
draws across the stone, then both arms blend back to their current gait pose.
The procedural parts remain collision-free. The multicast carries no tool,
inventory, condition, cost, or result data; the pose changes no gameplay or
saved state. `Kalmala.Gameplay.Tools.SharpeningPresentation` checks the bounded
pose duration, alternating stroke, smooth rest-pose endpoints, and rejection
of invalid elapsed time. Rendered in-world readability remains a separate
visual review.

`Kalmala.Gameplay.M9.GrindingStoneRepairAll` checks the paid recipe and
existing save/placement kit contracts, all five carried-tool definitions,
broken and damaged repair, unchanged full tools and levels, and atomic rejection
for invalid authority, missing stone validation, and malformed lists.

## Verification

After the forced editor build, run
`Kalmala.Gameplay.Tools.CarriedToolInventoryContract` with the existing
`Kalmala.Gameplay.Tools.LifecycleContract`,
`Kalmala.Gameplay.M9.AxeHarvestGates`,
`Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance`, and
`Kalmala.Gameplay.Crafting.NetworkContract` tests. The carried-inventory
contract checks the bounded starting list, full initial condition, level-1
baseline, owner-only state, and owner-only replication condition.
`Kalmala.Gameplay.M9.ToolProgressionCatalogue` checks axe and attachment
catalogues, station matches, level derivation, attachment placement bounds,
paid recipes, and item-catalogue-valid costs.
`Scripts/Verify-InventoryReconnect.ps1` also checks two live client visits:
the owner receives tool condition and the remote peer receives no tool details.

## Owner-local M9 tool feedback

The Camp crafting panel reads tool records and material counts from the owning
pawn. Its private tool-status block names all supported tools, shows carried
condition and repair availability, and keeps uncrafted axes unavailable until
the server creates them. The progression block shows axe target/current levels,
the previous-tool and skill prerequisites, every material cost with the
owner's available quantity, and the matching Workbench or Forge requirement.
When no matching station is nearby, the message includes its required level and
the visible same-world 2.5 m range. Paid attachment guidance explains the
session-only level bonus and the M9 save-migration gate.

Repair guidance distinguishes free selected-tool repair at a Workbench/Forge
from Repair All at a visible Grinding Stone. The UI only presents owner state;
repair RPCs remain intent-only and the server derives the tool list and
outcome. Tool state uses the existing schema-2 player-discovery container; no
new RPC or replicated field was added. `Scripts/Verify-Crafting.ps1` checks the
level, material, station, and repair text on the listen-server owner and
joining client; the M9 schema-2 reconnect fixture verifies saved owner state.
