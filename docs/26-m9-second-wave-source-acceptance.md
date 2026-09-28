# M9 second-wave source and reward acceptance

The `Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance` automation exercises all
four M9 source definitions through the existing server harvest transaction. It
creates a transient standalone server world and initializes each source node
from a server-owned population descriptor.

For birch trunk → Lightwood, ironheart trunk → Densewood, peat amber seam →
PeatAmber, and tundra salt deposit → FrostSalt, the test checks an accepted
in-range harvest grants the deterministic one-or-two unit reward, spends one
unit of the matching server-owned tool condition, marks the node harvested,
and publishes the source's stable sparse depletion identity.

Before accepting each source, the fixture sends a forged tool ID, mismatched
action, excessive trace distance, out-of-range pawn, pawn from another world,
and non-authoritative pawn. Every rejection must leave the source available,
inventory empty, and tool condition unchanged. The separate
`Kalmala.Gameplay.M9.SecondWaveHarvestContract` test checks the approved
catalogue mappings, bounded stack and yield limits, repeated same-seed/same-
spawn reward stability, the FNV-1a test vector, and sparse ID validity.

## Verification

Build the editor target with normal UnrealBuildTool access to
`%LOCALAPPDATA%\UnrealBuildTool`, then run these focused automations with
isolated temporary user and log paths:

```powershell
Automation RunTests Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance+Kalmala.Gameplay.M9.SecondWaveHarvestContract+Kalmala.Gameplay.M9.AxeHarvestGates+Kalmala.Gameplay.Tools.LifecycleContract+Kalmala.Gameplay.Inventory.Catalogue+Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion
```

On 2026-09-27, the forced UE 5.8.2 `KalmalaEditor Win64 Development` build
passed after compiling the new fixture, and all six focused automations
reported `Result={Success}`.

## Authority and limits

The production transaction continues to derive source, item, quantity,
condition cost, and depletion identity on the server. The fixture calls the
server transaction seam directly; it does not claim a live host/client RPC or
rendered playtest. M9 sources are not yet activated by generated population,
and M9 depletion remains session-only until its versioned save and migration
gate passes. No RPC, replicated field, or saved-data schema changed.
