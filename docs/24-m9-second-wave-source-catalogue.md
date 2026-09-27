# M9 second-wave source catalogue contract

This page defines the M9 source identities, presentation, placement limits,
depletion identity, and bounded optional loot. It extends the source map in
`23-m9-second-wave-biome-sources.md`. Runtime integration status is recorded in
`25-m9-second-wave-harvest-integration.md`; generated population activation and
persistent M9 state remain gated by their later tasks.

## Canonical source definitions

IDs are closed canonical design tokens. Source and presentation IDs use
lower kebab case, matching the M7 biome catalogue; inventory item IDs use the
existing PascalCase item-catalogue convention. Each ID is unique and selected
from the server's generated descriptor. The client may render the replicated
source and submit the existing interaction intent, but cannot choose an ID or
reward.

| Biome | Source ID | Presentation ID | Item ID / display name | Max stack | Local presentation |
| --- | --- | --- | --- | ---: | --- |
| Meadows | `meadows-birch-trunk` | `birch-trunk-harvest` | `Lightwood` / Lightwood | 50 | Mark the lower portion of an existing generated birch trunk with pale vertical bark and two dark harvest notches; do not add a duplicate tree. |
| Elderwood | `elderwood-ironheart-trunk` | `ironheart-trunk-harvest` | `Densewood` / Densewood | 50 | A broad dark heartwood trunk with sparse copper-brown grain and a readable lower-trunk cut face. |
| Mossy Mire | `mire-peat-amber-seam` | `peat-amber-seam` | `PeatAmber` / Peat Amber | 40 | A low irregular peat-bank face with one narrow amber seam, grounded to wet terrain. |
| Freezing Tundra | `tundra-frost-salt-deposit` | `tundra-salt-crystals` | `FrostSalt` / Frost Salt | 40 | A small wind-scoured cluster of pale, angular salt crystals rooted in the tundra surface. |

The item display names are **Lightwood**, **Densewood**, **Peat Amber**, and
**Frost Salt** respectively. Presentation stays original and collision-free;
the existing server interaction collision and generated terrain remain the
authority for reach and placement. The two trunk sources reuse their generated
tree anchors. The peat and salt forms add no platform, trail, camp, or route.
Wood source stacks cap at 50 like current `Wood`; mineral source stacks cap at
40 like current `Stone`.

## Deterministic placement and population budget

- Use the existing invisible 6,000 cm `SpatialKey` and the current world seed,
  generator revision, source ID, and key coordinates for deterministic
  placement. The generator revision is part of the matching world-save
  identity; it is not a visible region boundary.
- Use standard FNV-1a 64-bit (offset basis `14695981039346656037`, prime
  `1099511628211`) over the ASCII bytes of each specified string.
- Compute FNV-1a 64-bit over the ASCII string
  `m9-place-v1|<world-seed>|<generator-revision>|<spatial-x>|<spatial-y>`;
  the remainder modulo four selects exactly one source in the table order
  above. Format the seed as unsigned base-10 and the revision and coordinates
  as signed base-10 with no leading zeroes. Thus each source can be selected on
  at most one in four keys. If that source's biome is absent from the key,
  place nothing; do not fall back to a different source.
- For the selected source, examine at most eight candidates in fixed ordinal
  order, ordinal 0 through 7. Derive each candidate from FNV-1a 64-bit over
  `m9-place-candidate-v1|<world-seed>|<generator-revision>|<spatial-x>|<spatial-y>|<source-id>|<ordinal>`;
  use the same decimal formatting and the canonical source token, then map the
  low and next 16 bits to fractions `(bits / 65536.0)` for X and Y within the
  key. Accept the first point that is inside the 16 km world, has the source's
  exact classified land biome, is dry under the existing ocean and lake
  samplers, and has a terrain normal Z of at least 0.88. If none qualifies,
  place no source on that key; do not fall back to a client or actor-selected
  location.
- The combined M9 source ceiling is at most one node per spatial key,
  regardless of how many candidates are examined. It occupies one slot from
  the existing post-biome `HarvestNode` budget, only when that budget is at
  least two, and leaves at least one first-wave harvest slot. Never append a
  source beyond `GetSpawnBudget` plus the existing biome multiplier, raise the
  25 active terrain-patch ceiling, or create a parallel population budget.
- Source positions, IDs, and presentation selection must reproduce for the
  same seed, revision, and key on every peer. Clients receive only relevant
  materialized actors, not candidate lists, placement rolls, or routes.

## Stable sparse depletion identity

Derive the source's current population spawn identity through
`FKalmalaWorldPopulationLayout::GetPersistentSpawnId` and wrap it in this
reserved resource key:

```text
resource:m9:v1:<source-id>:<kind>/<spatial-x>/<spatial-y>/<spawn-seed>
```

Store it only as a world-scoped `ResourceDepleted` sparse delta under the
existing exact world seed and generator-revision save identity. The ID contains
only the approved source token and server-derived population identity; a client
never submits it. These IDs fit the current 128-character stable-ID limit and
remain separate for different sources or spatial spawns. Runtime writes and
loads for M9 sources remain disabled until the M9 persistence/migration gate
and its rejection checks pass. This contract adds no save field or schema.

## Bounded optional loot

Each accepted source harvest guarantees one unit of its table's primary item.
It may grant exactly one additional unit of that same item; there are no
secondary item IDs, open-ended rolls, quantity ranges, or chained drops. The
bonus is awarded when the FNV-1a 64-bit hash of the ASCII string
`m9-loot-v1|<world-seed>|<generator-revision>|<source-id>|<population-spawn-id>`,
interpreted as an unsigned integer, is divisible by four. The outcome is
therefore deterministic for a source identity and has a strict total yield of
one or two items.

The server computes the bonus and preflights the full one- or two-item grant
against the existing item catalogue and owner inventory before publishing any
reward, tool wear, or in-session depletion. A full pack, invalid source,
wrong-world target, failed server trace, invalid range, or mismatched tool
intent leaves inventory and source state unchanged. Client requests carry no
reward ID, quantity, bonus result, target, or spawn identity. The separate
server-side field-gate implementation maps the approved birch trunk to
Bronze-or-better and the ironheart trunk to Iron Axe; it does not check a
station at the source. Axe crafting or upgrade transactions will validate the
matching Workbench or Forge level when the tool-progression work is added.

## Authority and compatibility boundary

The server chooses descriptors, placement, stable IDs, bonus outcomes, and
inventory changes. Existing M7 harvest and sparse-resource contracts remain
the implementation seam. This catalogue design adds no RPC, runtime actor,
persistence write, or save-schema change. The M9 field gate adds owner-only
transient axe-condition fields; no tool condition or level is persisted. The
existing harvest transaction now uses these primary items, deterministic bonus
rules, and source-specific session depletion IDs; see
`25-m9-second-wave-harvest-integration.md`.
