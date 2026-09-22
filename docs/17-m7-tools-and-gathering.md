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

This increment deliberately does not create tool inventory items, repair
stations, node-health mutation, atomic inventory/durability transactions,
client RPC payloads, or persistence fields. The next tool slice wires this
contract into the existing server interaction trace and harvest-node path.

After the forced editor build, run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7ToolsUser' -abslog='C:\temp\KalmalaM7Tools.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.Tools.LifecycleContract; Quit" -TestExit="Automation Test Queue Empty"
```

The focused automation covers the six source mappings, catalogue rewards,
forged-source rejection, authority/range/trace/node gates, tool/action/skill
matching, one-point durability spend, and no-mutation rejection paths. It is
not live host/client gathering or repair evidence.
