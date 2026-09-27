# M9 second-wave harvest integration

The existing server tool transaction now integrates the four canonical M9
source definitions from `24-m9-second-wave-source-catalogue.md` with the item
catalogue, owner inventory, and sparse depletion handoff.

## Accepted server transaction

The server derives a source's primary item from its allowlisted source ID:
`meadows-birch-trunk` grants `Lightwood`, `elderwood-ironheart-trunk` grants
`Densewood`, `mire-peat-amber-seam` grants `PeatAmber`, and
`tundra-frost-salt-deposit` grants `FrostSalt`. Lightwood/Densewood retain the
Bronze Axe/Iron Axe field gates. Peat Amber and Frost Salt use the existing
Stone Pick and Mining action; no new tool tier is introduced.

An accepted M9 harvest grants one primary unit plus at most one deterministic
bonus unit. The server hashes the ASCII key
`m9-loot-v1|<world-seed>|7|<source-id>|<population-spawn-id>` with standard
FNV-1a 64-bit and grants the bonus only when the hash is divisible by four.
The item catalogue caps Lightwood/Densewood at 50 and PeatAmber/FrostSalt at
40. The normal candidate inventory grant preflights the complete one- or
two-unit output before tool condition or node state changes. Rejected tool,
trace, range, source, catalogue, or capacity checks leave the live pack,
tool, and source unchanged.

After acceptance, M9 nodes publish
`resource:m9:v1:<source-id>:<kind>/<spatial-x>/<spatial-y>/<spawn-seed>` to a
dedicated server-only depletion callback. The GameMode keeps these identities
in a session-only set and uses it when materializing any M9 descriptors. It
does not route M9 callbacks through the current schema-1 SaveGame slot, read
M9 entries from existing saves, or add a save field. First-wave resource
depletion continues to use its existing M7 persistence callback unchanged.

## Verification

After the forced `KalmalaEditor Win64 Development` build from
`docs/07-development-setup.md`, run the focused contract tests with isolated
temporary user and log paths:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM9HarvestUser' -abslog='C:\temp\KalmalaM9Harvest.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.M9.SecondWaveHarvestContract+Kalmala.Gameplay.M9.AxeHarvestGates+Kalmala.Gameplay.Tools.LifecycleContract+Kalmala.Gameplay.Inventory.Catalogue; Quit" -TestExit="Automation Test Queue Empty"
```

`Kalmala.Gameplay.M9.SecondWaveHarvestContract` checks all four canonical
source, presentation, item, action, stack-cap, deterministic-yield, FNV, and
sparse-ID mappings. It verifies that the inventory candidate builder accepts
the guaranteed yield and rejects a bonus when the complete grant would exceed
the stack cap. `Kalmala.Gameplay.M9.AxeHarvestGates` and the existing inventory
contracts retain the earlier tool-tier and owner-inventory checks.

## Current limits

No M9 source is activated in generated population yet. The transaction,
canonical material definitions, presentation IDs, and depletion callback are
ready for that placement work, but this increment does not claim live host/
client acceptance of any newly generated source. M9 depletion is session-only
until its versioned persistence and migration gate is implemented.
