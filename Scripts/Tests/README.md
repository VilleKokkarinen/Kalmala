# Tests

Choose tests by the behavior changed. Run only relevant checks during development;
use the full verification prescribed by `docs/07-development-setup.md` at a
milestone gate. Nothing in this directory is an automatic run-everything suite.

## Items: shared coverage

Use Unreal automation for the shared rules:

| Behavior | Automation |
| --- | --- |
| Craft payment, output and rejection | `Kalmala.Gameplay.Crafting.Transactions` |
| One representative per wearable slot | `Kalmala.Gameplay.Inventory.WornGearTransactions` |
| Main Hand and two-handed equipment | `Kalmala.Gameplay.Inventory.TwoHanded` |
| Poison rules | `Kalmala.Gameplay.Status.PoisonEffects` |
| Bow draw and ammunition | `Kalmala.Gameplay.Combat.BowDrawAndArrows` |

Use `Inventory/Verify-ItemPeers.ps1` when a change needs live RPC/replication
coverage. Each scenario starts a listen host, remote owner and observer:

| Scenario | Representative and purpose |
| --- | --- |
| `CraftEquipment` | Badger Fur Vest: craft/payment, GUID equip, armor/wear and privacy |
| `WeaponProgression` | Fang Spear: Forge progression, Main Hand and accepted melee hit |
| `Meal` | Peat Heart Stew: heated cooking, consumption, Steady Meal and repeat rejection |
| `Antidote` | Poison treatment, resistance, cooldown and dose preservation |
| `PoisonedArrows` | Fang Arrows: craft, real shot, ammunition cost and poison damage |

```powershell
Scripts/Tests/Inventory/Verify-ItemPeers.ps1 -ListScenarios
Scripts/Tests/Inventory/Verify-ItemPeers.ps1 -Scenario CraftEquipment -Port 17892 -Project C:/km/Kalmala.uproject
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

## Other tests

| Directory | Choose for |
| --- | --- |
| `Acceptance` | Integrated vertical-slice and documentation gates |
| `Audio` | Ambient audio contracts and peer playback |
| `Combat` | `Verify-RatPeer.ps1` for shared reward/retry/defeat persistence; species runners for their distinct attack behavior; combat/discovery peers |
| `Construction` | Hammer placement/movement, persistence, crafting, storage and Garden Plot cultivation |
| `Inventory` | Inventory transactions/receipts, menu and reconnect; item peer scenarios above |
| `Performance` | Explicit constrained-performance profiles (`-ListProfiles`) |
| `Presentation` | Input/UI contracts and focused live or rendered checks |
| `Progression` | Exploration rewards and current-format reconnect |
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
