# A9 implementation and acceptance handoff

Implementation runs in isolated `codex/a9-runtime` by explicit user assignment. A8/I4 remains owned by the independent main-checkout worker. Neither I4 nor I5 is passed by these implementation commits. All executed checks/results are recorded in PROGRESS.md.

## Approved source decisions

On 2026-10-23 the user approved the documented replacement/reward rates and three accepted pick hits with 10% smash resolved before extraction on every hit. They approved population save schema 2, preserving/rejecting incompatible schema-1 files without migration. They approved treasure at existing seeded discovery points in Elderwood, Lakes and Mountains and the missing bounded Frost Salt deposit generator (one possible safe, separated deposit per 60 m key, retaining frostmoss).

The five rock sources and three treasure sources are implemented/enabled. Enemy Moonstone and Dusk Diamond code is prepared but `EnemySourcesAccepted` remains false until independent I4 acceptance is integrated. Their item/gear parents remain open. All other completed checkboxes describe implementation and lightweight checks, not executed full verification.

## Existing scroll-route audit

| Scroll/effect | Existing seeded discovery eligibility | Existing use |
| --- | --- | --- |
| Mending | Meadows, Elderwood; even Lakes descriptor seed | Server traces an injured living ally; incoming recovery respects effective maximum |
| Hearth Shield | Thunder Mountains; odd Lakes descriptor seed | Server-owned finite absorption with existing cooldown/expiry |
| Bear's Vigor | Freezing Tundra | Existing temporary strength and +40 stamina; sums with equipped flat stamina |
| Deer Call | Mossy Mire | Server validates nearby eligible deer and applies existing influence |

`WorldPopulationLayout::GetDiscoveryDefinition` retains these locations. `GameMode::ClaimDiscovery` validates the current descriptor and range, persists the discovery and allowlisted canonical effect before learning it, and rejects repeat claims. `SupportMagicComponent::LearnEffectFromServer` supplies the corresponding ordinary scroll item when missing; that runtime stack is reconstructed from the persisted entitlement rather than saved as a separate item. Player restoration relearns saved canonical effects. The optional existing Mireling boss route still selects one of the same four definitions by world seed modulo four and uses the existing player-scoped boss entitlement. No source, new scroll, trophy or boss key was added by the audit.

This audit is source inspection, not a new live discovery/restart pass. Applicable verification includes `Kalmala.Gameplay.Discovery`, support/status/inventory automation and the existing discovery peer/restart procedures in 07-development-setup.md.

## Verification still required

Use an isolated short-path mirror, unique evidence roots, ports and fixture UserDirs; never share the main worker's mirror, logs or mutable outputs. Audit generated/output absolute paths below 260 before builds/runners. Editor and Game targets, complete applicable Kalmala automation, catalogue/icon validators, source/mining/treasure/equipment peer and restart evidence and rendered gear/menu checks remain required. The installed Launcher engine's documented dedicated-server limitation remains applicable; do not attempt unsupported Server target.

Run new focused cases: `Kalmala.World.GemstoneContract`, `Kalmala.World.GemstoneGeneration`, `Kalmala.Gameplay.Inventory.AccessorySlots`, `Kalmala.Gameplay.Inventory.A9GearRecipes`, `Kalmala.Gameplay.Status.AccessoryStats`, `Kalmala.Gameplay.Inventory.OpalHelm`, `Kalmala.Gameplay.Combat.StormcutSword`. These have not yet been compiled/executed at this handoff.

Keep I4-dependent enemy enablement and the combined I5 gate open until their actual evidence exists. Ordinary material inventory and equipped gear retain their existing session-only limitations; node/treasure claim persistence does not establish general inventory restoration. No A9 cargo implementation is implied: current BACKLOG A9 scope omits the details document's older blocked cargo proposal. Record its factual existing vessel-persistence gap if broader acceptance requires it.
