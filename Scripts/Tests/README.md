# Tests

Choose tests by the behavior changed. Run only relevant checks during development;
use the full verification prescribed by `docs/07-development-setup.md` at a
milestone gate. Nothing in this directory is an automatic run-everything suite.

Name runners after the behavior they cover, without `Verify-` or roadmap
milestone prefixes. For example, `Progression/GemMiningTreasurePeer.ps1` covers gem
mining and treasure claims, and `Acceptance/DocumentationContracts.ps1` runs
the lightweight documentation gates. Internal Unreal fixture flags and log
markers retain their existing names.

## Items: shared coverage

Use Unreal automation for the shared rules:

| Behavior | Automation |
| --- | --- |
| Craft payment, output and rejection | `Kalmala.Gameplay.Crafting.Transactions` |
| One representative per wearable slot | `Kalmala.Gameplay.Inventory.WornGearTransactions` |
| Main Hand and two-handed equipment | `Kalmala.Gameplay.Inventory.TwoHanded` |
| Poison rules | `Kalmala.Gameplay.Status.PoisonEffects` |
| Bow draw and ammunition | `Kalmala.Gameplay.Combat.BowDrawAndArrows` |

Use `Inventory/ItemPeers.ps1` when a change needs live RPC/replication
coverage. Each scenario starts a listen host, remote owner and observer:

| Scenario | Representative and purpose |
| --- | --- |
| `CraftEquipment` | Badger Fur Vest: craft/payment, GUID equip, armor/wear and privacy |
| `WeaponProgression` | Fang Spear: Forge progression, Main Hand and accepted melee hit |
| `Meal` | Peat Heart Stew: heated cooking, consumption, Steady Meal and repeat rejection |
| `Antidote` | Poison treatment, resistance, cooldown and dose preservation |
| `PoisonedArrows` | Fang Arrows: craft, real shot, ammunition cost and poison damage |

```powershell
Scripts/Tests/Inventory/ItemPeers.ps1 -ListScenarios
Scripts/Tests/Inventory/ItemPeers.ps1 -Scenario CraftEquipment -Port 17892 -Project C:/km/Kalmala.uproject
```

`-Editor` overrides the UE executable. `-OutputDirectory` must name a new, short
directory; omitted output goes to a fresh temporary directory. Logs are retained
on success and failure. Every scenario requires all four role markers and both
clients receiving the server seed. Processes started by the runner are stopped
on exit. These headless tests stage ingredients; they do not prove natural
acquisition, rendered menus or physical input.

The nine former `Construction/Verify-A8b*Peer.ps1` runners are replaced by this
entry point. Tonic repeated shared crafting; Scale Cuirass and Wolf Pelt Boots
repeated armor/equip; Roasted Badger repeated food/meal behavior. Their separate
runs are no longer required. Existing C++ fixtures retain their internal names;
historical results remain in `PROGRESS.md`.

## Enemy peers

Use `Combat/EnemyPeers.ps1 -ListScenarios` to select one behavior. There is no
run-all mode. All scenarios accept `-Project`, `-Editor`, `-Port`, a fresh short
`-OutputDirectory`, and optional `-WorldSeed` (otherwise the fixture's tested
seed is used). The runner retains logs and stops its processes on failure.

```powershell
Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario SharedCombat -Project C:/km/Kalmala.uproject
Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario WolfPack -Project C:/km/Kalmala.uproject -Port 18196
```

| Scenario | Distinct coverage |
| --- | --- |
| `SharedCombat` | Rat: server damage, invalid RPC rejection, owner-only full-pack reward escrow/retry, both peers' replicated defeat/health, exact-ID restart |
| `CombatFeedback` | Owner-local hit/defeat cues, unavailable text without audio, remote cue privacy |
| `BoarCharge` / `DeerHerd` | Charging server damage / deterministic herd alert |
| `WolfPack` / `LynxLeap` | Three-member formation and lunge / bounded leap |
| `PolarBearCharge` / `HornedGoatPush` | Charge/retreat/recovery / bounded knockback and stagger |
| `MosquitoCounterplay` | Owner-only poison, dodge cooldown, lit/unlit torch and campfire protection |
| `PeatHuskSweep` / `ThunderSentinelSlam` | Sweep / approach step and authored slam damage |
| `ReefMawBite` | Deep-water swimming attack, bounded bite reach and replicated cues |

Only `SharedCombat` launches a persistence restart. The other scenarios retain
their mechanic-specific assertions, including peer cues and numeric bounds.
Legacy native fixtures may still assert shared rules internally; the runner
does not add another loot/retry/restart gate for each species.

The 16 former enemy launchers are replaced by this entry point. Separate
Mireling, Badger/Moon Bear, Viper and Giant Snake live reward runs are retired:
their repeated loot, poison or ordinary attack coverage does not justify another
shared-system journey. Species-specific Unreal contract tests remain available.
Use `Kalmala.Gameplay.Wildlife.Stats.AllReviewedArchetypes` for the table of all
17 enemies' combat stats and `python Scripts/Validate-CreatureCatalogues.py` for
all species' catalogue, attack, drop-reference and biome data. Use
`Kalmala.Gameplay.Status.PoisonEffects` for shared weak/major poison rules.

`Progression/DiscoveryPeer.ps1` and `Progression/GemMiningTreasurePeer.ps1` cover
discovery claims and gem acquisition independently of enemy combat. Historical
per-species commands and rendered captures in older handoffs describe the old
launchers; the commands above supersede them. The new runner is headless.

## Other tests

| Directory | Choose for |
| --- | --- |
| `Acceptance` | Integrated vertical-slice and documentation gates |
| `Audio` | Ambient audio contracts and peer playback |
| `Combat` | `EnemyPeers.ps1 -Scenario ...` for shared combat or a distinct enemy mechanic |
| `Construction` | Hammer placement/movement, persistence, crafting, storage and Garden Plot cultivation |
| `Inventory` | Inventory transactions/receipts, menu and reconnect; item peer scenarios above |
| `Performance` | Explicit constrained-performance profiles (`-ListProfiles`) |
| `Presentation` | Input/UI contracts and focused live or rendered checks |
| `Progression` | Discovery claims, gem mining/treasure, exploration rewards and current-format reconnect |
| `Survival` | Camp choices and persisted camp restart |
| `Travel` | Swimming, ocean travel and skiff journeys/reconnect |
| `World` | Generation, terrain/biome peers, minimap, awareness and map export |

Check a runner's parameters with `Get-Help <path>`. Contract/source audits are
lightweight; live peers, rendered checks and performance captures need a built
Editor and are selected deliberately. Use a short compiled project mirror to
keep generated Unreal files outside the source checkout. `TestPaths.ps1` only
resolves repository paths. Catalogue/icon validators and asset/build/export
utilities remain directly under `Scripts/`.

Add coverage for a distinct rule or failure, rather than another item using an
already-covered transaction. Keep unique species attacks and acquisition sources
focused; do not repeat the shared enemy loot/persistence gate for each species.
