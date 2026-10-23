# A9 implementation and acceptance handoff

Implementation runs in isolated `codex/a9-runtime` by explicit user assignment. A8/I4 remains owned by the independent main-checkout worker. Neither I4 nor I5 is passed by these implementation commits. All executed checks/results are recorded in PROGRESS.md.

## Approved source decisions

On 2026-10-23 the user approved the documented replacement/reward rates and three accepted pick hits with 10% smash resolved before extraction on every hit. They approved population save schema 2, preserving/rejecting incompatible schema-1 files without migration. They approved treasure at existing seeded discovery points in Elderwood, Lakes and Mountains and the missing bounded Frost Salt deposit generator (one possible safe, separated deposit per 60 m key, retaining frostmoss).

The five rock sources and three treasure sources are implemented/enabled. Enemy Moonstone and Dusk Diamond code is prepared but `EnemySourcesAccepted` remains false until independent I4 acceptance is integrated. Their item/gear parents remain open. The independent verification increment is executed as recorded below; it does not close I4 or combined I5.

## Existing scroll-route audit

| Scroll/effect | Existing seeded discovery eligibility | Existing use |
| --- | --- | --- |
| Mending | Meadows, Elderwood; even Lakes descriptor seed | Server traces an injured living ally; incoming recovery respects effective maximum |
| Hearth Shield | Thunder Mountains; odd Lakes descriptor seed | Server-owned finite absorption with existing cooldown/expiry |
| Bear's Vigor | Freezing Tundra | Existing temporary strength and +40 stamina; sums with equipped flat stamina |
| Deer Call | Mossy Mire | Server validates nearby eligible deer and applies existing influence |

`WorldPopulationLayout::GetDiscoveryDefinition` retains these locations. `GameMode::ClaimDiscovery` validates the current descriptor and range, persists the discovery and allowlisted canonical effect before learning it, and rejects repeat claims. `SupportMagicComponent::LearnEffectFromServer` supplies the corresponding ordinary scroll item when missing; that runtime stack is reconstructed from the persisted entitlement rather than saved as a separate item. Player restoration relearns saved canonical effects. The optional existing Mireling boss route still selects one of the same four definitions by world seed modulo four and uses the existing player-scoped boss entitlement. No source, new scroll, trophy or boss key was added by the audit.

This audit is source inspection, not a new live discovery/restart pass. Applicable verification includes `Kalmala.Gameplay.Discovery`, support/status/inventory automation and the existing discovery peer/restart procedures in 07-development-setup.md.

## Executed independent verification and remaining gates

The isolated mirror is `C:/Users/Ville/AppData/Local/Temp/a9v`; general evidence is `C:/Users/Ville/AppData/Local/Temp/a9e`. Editor and Game builds passed. Final full queue `auto4.log/r4` passed all 185 tests (179 clean, six with warnings), zero failures/not-run, exit 0. All 148 catalogue icons, nine status icons and 17 creature profiles passed validation. All 1,083 build/runner inputs matched the source by SHA-256; generated/output path audit found a maximum of 210 and no MAX_PATH violations. The installed Launcher engine's documented dedicated-server limitation remains applicable; no unsupported Server target was attempted.

New focused cases are `Kalmala.World.GemstoneContract`, `Kalmala.World.GemstoneGeneration`, `Kalmala.Gameplay.Inventory.AccessorySlots`, `Kalmala.Gameplay.Inventory.A9GearRecipes`, `Kalmala.Gameplay.Status.AccessoryStats`, `Kalmala.Gameplay.Inventory.OpalHelm`, and `Kalmala.Gameplay.Combat.StormcutSword`. All passed in the final full queue. Initial failures exposed new Stormcut/recipe test expectations and an older biome identity assertion that omitted approved replacements; repairs and rerun history are recorded in PROGRESS.md. Opal's real nearby-Forge/Anvil route passed upgrade/payment, station rejection, damage/wear and free repair. Current station attachments reach Forge two; higher authored item tiers retain that existing prerequisite.

`Verify-A9Peer.ps1 -Port 19309 -OutputDirectory C:/Users/Ville/AppData/Local/Temp/a9p6` passed with one listen host and two actual owning remote clients, a canonical seeded Quartz descriptor and a canonical seeded Garnet discovery descriptor. It exercises ordinary owning attack/interaction RPCs, saved partial progress, full-pack rejection, competing claims, owner-only accessory stats, relevant-peer node state and fresh-process claimed-ledger restoration. The fixture provisions equipment/materials, holds players at interaction height while distant terrain streams, and removes duplicate actors for its identities; it does not establish fresh-character progression without grants. Both remote logs contain treasure requests. Shared crafting (`19311`) and discovery (`19313`, `a9d1`) peer runners also passed.

Final equipped-inventory renders `gear720b.png` and `gear150b.png` were inspected at 1280x720/100%/contrast0 and 1024x768/150%/contrast1. The Opal backplate height, accessibility styling and overflowing labels were repaired before those captures. This is commandlet-render evidence, not physical input certification.

The existing presentation static audit fails a pre-existing `Tool Rack` text anchor in untouched `KalmalaCraftingSubsystem.cpp`; preceding inventory/static checks pass. The aggregate M5 runner has an existing argument-binding failure; direct settings contract fails a stale `Empty Gameplay group stays hidden` anchor while audio/local-input/onboarding contracts pass. Both anchors are already absent from baseline `22deb8c1c7ff79053b6e5fb09babba1cfec5b96b`, with unchanged validators. The inventory peer runner failed its missing client notification-baseline log (`KalmalaInventory-e6aba8b04c2a4faa9d98ea44e5c6bb9b`), corresponding to the known I4 baseline disposition. These runner results are not passes; retain their disposition and combined I5 gate.

Keep I4-dependent enemy enablement and the combined I5 gate open until their actual evidence exists. Ordinary material inventory and equipped gear retain their existing session-only limitations; node/treasure claim persistence does not establish general inventory restoration. No A9 cargo implementation is implied: current BACKLOG A9 scope omits the details document's older blocked cargo proposal. Record its factual existing vessel-persistence gap if broader acceptance requires it.

Committed I4 snapshot `09ea50e4` was subsequently integrated into the isolated A9 branch, preserving both handoffs and fixture additions. Combined Editor/Game builds passed; `ia1.log/ir1` passed all 185 tests (178 clean, seven with warnings). The repeated A9 peer/restart case passed at port 19319 in `C:/Users/Ville/AppData/Local/Temp/a9ip1`. This is a tested committed snapshot, not accepted I4 or a merge of A9 into main; the active main worker remains independent.
