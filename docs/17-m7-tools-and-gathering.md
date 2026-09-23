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
callback. Tool condition replicates only to its owner. Tool items, repair,
condition persistence, and player-facing condition controls remain later work.
Source-less one-use discovery nodes retain their existing legacy interaction.

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
