# M7 tools and gathering contract

The first tool-lifecycle increment is a bounded server-side contract, not a
new interaction RPC or save schema. `FKalmalaToolLifecycleContract` defines
three transient first-wave tools: `ReedKnife` for Gathering,
`FieldHatchet` for Woodcutting, and `StonePick` for Mining. Each definition
has a fixed minimum skill level, maximum durability, and one-point use cost.

The server derives the gathering source from the existing replicated world
population descriptor and maps it to the action, required tool, skill, and one
existing catalogue reward. Meadows and Elderwood select Wood, Lakes and Tundra
select Fibre, and Mire and Mountains select Stone. Forged or unknown source IDs
and rewards fail closed through the existing item catalogue.

Before a use can be accepted, the server contract requires authority, a hit
from the server trace, same-world membership, an available node, finite range
within the server maximum, a matching client-selected tool/action, a valid
server-owned skill state, and positive in-bounds durability. Accepted use
spends one fixed durability point and returns only the server-selected reward
identity and quantity. Rejected requests leave the tool state and reward output
unchanged.

## Server interaction and harvest transaction

Generated harvest nodes with a valid biome source now use the existing
`ServerRequestInteract` intent. The owning client sends only the allowlisted
tool ID and action it derived from the locally visible source. The server
repeats the visibility trace, chooses the node and source itself, and rejects
unknown or mismatched intent, unavailable nodes, out-of-range hits, invalid
server skill state, and zero or over-maximum condition. The request contains no
target, reward, quantity, damage, condition, or outcome.

Each new server pawn receives transient full condition for the three first-wave
tools. Accepted gathering builds a catalogue-validated inventory candidate
before it changes live state; a full pack leaves condition and node state
unchanged. Once the pack candidate is committed, the server spends one tool
condition point, depletes the node, and emits the existing sparse-depletion
callback. Tool condition replicates only to its owner. Tool items and condition
persistence remain later work; the first repair controls use the existing Camp
crafting panel and transient condition.
Source-less one-use discovery nodes retain their existing legacy interaction.

## Validated tool repair

**Superseded by M9:** the M7 material-paid repair and zero-condition
replacement recipes below are retained as historical implementation notes.
Current selected-tool repair is free, accepts damaged and broken carried tools,
uses a visible same-world Workbench or Forge within 250 cm, spends no materials,
and grants no Crafting experience. See
`27-m9-carried-tool-inventory.md` for the active contract.

Camp crafting offers one repair button per first-wave tool. Each owning-client
request contains only the allowlisted tool ID. The server requires the same
world's visible Joiner's workbench within 250 cm, reads current owner tool
condition itself, and selects the material and cost from the server tool
definition. Reed Knife uses Fibre, Field Hatchet uses Wood, and Stone Pick uses
Stone; one gathered material restores up to four missing condition points, so
the server rounds the amount up and repairs the tool to full condition.

Repair builds an inventory exchange before publishing it. Unknown, full, or
out-of-bounds tools, a missing or invalid workbench, insufficient materials,
and stale pack snapshots leave both pack and condition unchanged. Accepted
repair publishes the paid pack and full condition in the same server action.
The condition remains transient and owner-only; repair adds no gameplay save
field or schema.

## Broken Field Hatchet replacement recipe

The Camp crafting catalogue includes one server-validated replacement recipe
for the existing transient Field Hatchet. It is available only when that
owner's condition is exactly zero and a visible same-world Joiner's workbench
is within 250 cm. The request still contains only recipe identity and batch;
the server resolves the configured recipe, verifies the tool definition and
condition, and atomically consumes two Wood, two Stone, and one Fibre before
restoring condition to 24/24. Rejected station, material, batch, duplicate, or
malformed requests do not change the pack or tool. The recipe creates no
inventory item: tools remain the existing owner-only condition fields, so the
player can still repair a worn tool with the separate dynamic repair action.
One fixed 10 Crafting experience is awarded only after the replacement
exchange succeeds. Condition and progression remain pawn-lifetime; no save
schema changed.

`Scripts/Verify-InventoryReconnect.ps1` wears the hatchet through accepted
server harvests, then checks distant-workbench and insufficient-material
rejection without mutation, successful replacement and experience award, and
duplicate rejection. The same two-visit host/client run verifies the restored
condition reaches its owner and remains hidden from the other peer. The
fixture is headless and does not establish rendered recipe focus or
player-visible crafting.

## Broken Stone Pick replacement recipe

The Camp crafting catalogue also includes a server-validated replacement for
the existing transient Stone Pick. It is available only when the owner's pick
condition is exactly zero and a visible same-world Joiner's workbench is within
250 cm. The request remains recipe identity plus batch; the server resolves
the cost, station, and tool definition, then atomically consumes 2 Wood,
3 Stone, and 1 Fibre before restoring the existing owner-only condition to
20/20. A worn pick remains eligible for the separate gathered-Stone repair
action. Missing/distant workbench, insufficient materials, malformed or
batched requests, and duplicate replacement preserve pack and condition.
Accepted replacement awards 10 transient Crafting experience after payment;
no tool item, RPC, or save-schema field is added.

`Scripts/Verify-InventoryReconnect.ps1` wears the pick through accepted,
server-selected Mire mining nodes. Across two client visits to one listen
server, it checks zero-condition and visible-workbench gates, exact payment,
insufficient-material rejection, post-payment experience, duplicate rejection,
owner condition replication, and privacy from the simulated remote peer. The
fixture does not establish rendered recipe focus or saved tool condition.

## Broken Reed Knife replacement recipe

The Camp crafting catalogue includes a server-validated replacement for the
existing transient Reed Knife. It is available only when the owner's knife
condition is exactly zero and a visible same-world Joiner's workbench is within
250 cm. The request remains recipe identity plus batch; the server resolves
the cost, station, and tool definition, then atomically consumes 1 Wood,
1 Stone, and 2 Fibre before restoring the existing owner-only condition to
16/16. A worn knife remains eligible for the separate gathered-Fibre repair
action. Intact-tool, missing/distant workbench, insufficient material,
malformed or batched requests, and duplicate replacement preserve pack and
condition. Accepted replacement awards 10 transient Crafting experience after
payment; no tool item, RPC, or save-schema field is added.

`Scripts/Verify-InventoryReconnect.ps1` wears the knife through accepted,
server-selected Lakes reed nodes, rejects replacement while the intact knife
has enough materials, then checks missing-station and insufficient-material
rejection, successful payment/restoration/experience, duplicate rejection,
owner condition replication, and privacy from the simulated remote peer over
two visits to one listen server. The fixture does not establish rendered
recipe focus or saved tool condition.

After the forced editor build, `Kalmala.Gameplay.Tools.LifecycleContract`
checks each tool's material mapping, rounded cost, full-condition outcome,
workbench/authority gates, malformed condition rejection, and insufficient
material exchange. `Kalmala.Gameplay.Crafting.NetworkContract` checks that the
repair RPC carries only a tool identity. `Scripts/Verify-InventoryReconnect.ps1`
spawns a real workbench fixture for each server pawn and checks absent/distant
station rejection, material failure without mutation, accepted repair, full
condition rejection, owner condition replication, and remote condition
privacy. It uses transient test actors and materials and does not establish
repair-condition persistence or rendered aiming at the bench.

## Verification

After the forced editor build, run the focused contracts with isolated user
and log paths:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7ToolsUser' -abslog='C:\temp\KalmalaM7Tools.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.Tools.LifecycleContract+Kalmala.Gameplay.Inventory.NetworkContract+Kalmala.Gameplay.Inventory.Catalogue+Kalmala.Gameplay.Crafting.Transactions; Quit" -TestExit="Automation Test Queue Empty"
```

`Scripts/Verify-InventoryReconnect.ps1` also exercises generated-source tool
gathering on each server-owned host/client pawn. It verifies wrong-tool and
full-stack rejection without condition/depletion changes, accepted catalogue
reward and one-point wear, duplicate rejection, sparse depletion callback,
owner-only inventory, and existing reconnect behavior. It calls the same
server transaction after supplying a server-trace distance; it does not measure
rendered tool selection or provide repair/persistence evidence.

After the forced editor build, run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7ToolsUser' -abslog='C:\temp\KalmalaM7Tools.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.Tools.LifecycleContract; Quit" -TestExit="Automation Test Queue Empty"
```

The focused automation covers the six source mappings, catalogue rewards,
forged-source rejection, authority/range/trace/node gates, tool/action/skill
matching, one-point durability spend, and no-mutation rejection paths. It is
not live host/client gathering or repair evidence.

## M9 Bronze and Iron Axe field gates

The canonical `meadows-birch-trunk` source selects `Lightwood` and requires a
server-owned Bronze Axe or better. The canonical
`elderwood-ironheart-trunk` source selects `Densewood` and requires a
server-owned Iron Axe. The owner sends only the selected tool identity and
Woodcutting action; the server resolves the source and reward, checks the
owner-only condition, same-world server trace, range, source availability,
skill, and full inventory grant, then spends condition and depletes the node
atomically. An uncrafted axe has the owner-only condition sentinel `-1` and
cannot pass field use or repair validation.

The source-use contract contains no Workbench or Forge state: field harvest
does not check a station. Bronze Axe crafting now uses the server-selected
visible same-world level-one Workbench. Iron Axe replaces a carried level-one
Bronze Axe at a level-two Forge after the same server-side material and tool
candidate checks. Both base stations currently start at level one, so the
level-two Iron Axe action stays unavailable until the later paid Forge
attachment task. Tool levels remain owner-only and transient.

The separate `Kalmala.Gameplay.M9.AxeHarvestGates` automation checks the two
canonical source/reward mappings, Bronze and Iron eligibility,
Iron-for-Bronze substitution, lower-tier and uncrafted rejection, owner tool
condition spend, and bounded server-selected reward. Run it with the existing
tool-lifecycle and item-catalogue contracts after a forced build:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM9AxeUser' -abslog='C:\temp\KalmalaM9AxeAutomation.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.M9.AxeHarvestGates+Kalmala.Gameplay.Tools.LifecycleContract+Kalmala.Gameplay.Inventory.Catalogue; Quit" -TestExit="Automation Test Queue Empty"
```
