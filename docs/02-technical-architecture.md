# Technical architecture

## UE5 baseline

- Create a C++ UE5 project named `Kalmala`, using Enhanced Input, Common UI, Gameplay Ability System (GAS), and Online Subsystem interfaces.
- Use source control and Unreal-friendly ignore rules before creating content.
- Target a current supported UE 5.x version chosen during project bootstrap; record the exact version in the decision log.
- Prefer Data Assets/Data Tables for tunable game content; avoid gameplay constants scattered through Blueprints.

## Multiplayer contract

The server owns player state, inventories, construction, damage, AI decisions, simulation state, loot, and progression. Clients send intent through validated server RPCs and receive replicated state. Clients may predict responsiveness only where reconciliation is supported.

| System | Authority | Replication approach |
| --- | --- | --- |
| Movement | UE Character Movement server authority | built-in movement replication |
| Interaction | server reruns a short trace from the owning pawn | client sends intent only; no target actor is trusted |
| Abilities | server validates activation and outcomes | GAS replication/prediction where suitable |
| Inventory/crafting | server | replicated components / owner-only detail |
| Building | server | replicated building actors; save stable IDs |
| AI | server | replicate actor state, not decision logic |
| Element grid | server | replicate sparse changes near relevant players |
| World identity | server | replicate immutable `WorldSeed` and `GeneratorRevision` through `GameState` |
| Terrain surface | shared seed function | convert Elevation to continuous height and normal for terrain, collision, and server-selected spawns |
| Terrain rendering | client cosmetic | derive one continuous local mesh from the replicated identity and patch descriptor; mesh geometry is never replicated |
| Terrain collision | server | create collision from the server's same continuous terrain mesh; clients derive matching local collision only for prediction, never as authority |
| Surface water | client cosmetic | derive a collision-free sea-level mesh over fully submerged terrain cells from the replicated identity and patch descriptor |
| Generated player start | server | resolve a Meadow-preferred seed-specific transform at the sampled terrain height, then spawn pawns there |
| Meadow rocks | client cosmetic | derive non-interactable instanced rocks from the replicated identity, terrain sample, and biome classification |
| Meadow trees | client cosmetic | derive non-interactable instanced trunks and canopies from the replicated identity, terrain sample, and biome classification |
| Cosmetics | client | derive from replicated state/events |

## Module boundaries

- `KalmalaCore`: tags, logging, shared data types, save interfaces.
- `KalmalaGameplay`: character, GAS, inventory, crafting, construction, interaction.
- `KalmalaWorld`: world generation, element grid, weather, harvesting, AI encounter logic.
- `KalmalaUI`: Common UI screens and view models.
- `KalmalaServer`: dedicated-server configuration and server-only services.

Do not make UI call world actors directly. Use components, interfaces, gameplay messages, or subsystem APIs.

## Content conventions

- `/Game/Kalmala/Core`, `/Characters`, `/World`, `/Items`, `/Abilities`, `/UI`, `/Audio`, `/Maps`, `/Developer`.
- Prefixes: `BP_`, `WBP_`, `DA_`, `DT_`, `GA_`, `GE_`, `GCN_`, `IA_`, `IMC_`, `M_`, `MI_`, `T_`, `SK_`, `SM_`, `SFX_`.
- Use Gameplay Tags for semantic states (for example `State.Wet`, `State.Sheltered`, `Damage.Fire`, `Ability.Support.Mending`, `Effect.Shielded`).

## Persistence and online progression

Vertical slice persistence is a versioned `SaveGame` schema for local/listen-server testing. Isolate persistence behind interfaces so a later backend can replace it. Do not connect production identity, payments, analytics, or cloud databases until there is an explicit product decision.

Magic-scroll discoveries and learned support effects are server-authoritative progression. Save stable scroll IDs and learned-effect IDs, validate scroll rewards once, and replicate only the effect state needed by other players (such as an active shield or stat boost), not private inventory detail.

## Verification minimum

Every feature needs an automated test where practical, plus a reproducible multiplayer test: host + one client or dedicated server + two clients. Profile before increasing simulation area, actor count, or replication frequency.
