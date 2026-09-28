# M9 optional land exploration rewards

This document defines the first M9 land-discovery design slate. It completes
the candidate-definition child of the M9 roadmap goal; the following backlog
child still owns server-side identity, placement, eligibility, claim handling,
and reward implementation. No runtime behavior or persistence changes here.

## Candidate slate

| Candidate ID | Player-facing name | Biome | Observation cue | Reward |
| --- | --- | --- | --- | --- |
| `lakes-rillworn-marker` | Three-Run Rillstone | Shimmering Lakes | A low, walkable shoreline stone has three water-cut channels that meet at a shallow pocket holding a small cache. The channels and pocket read by shape and relief, not colour alone. | `Stone` x2 |
| `mountains-leeward-grain` | Leeward Grain | Thunder Mountains | Parallel wind-scored lines cross a broad, walkable ridge-foot shelf; a low stone lip catches a small bundle of reed fibre on its sheltered side. The marks and bundle read by shape and text, not colour alone. | `Fibre` x2 |

Each candidate rewards noticing a local feature and inspecting it through the
ordinary interaction flow. Its short observation text describes the feature
itself; it does not point toward another candidate or reveal coordinates.
Rewards use existing catalogue item IDs and are intentionally small. They do
not add a new item, tool tier, skill unlock, magic effect, or access to
`Lightwood`, `Densewood`, `PeatAmber`, or `FrostSalt` outside their approved
sources.

## Design boundaries

- Both candidates are optional and self-contained. There is no order, quest
  chain, waypoint, compass bearing, route, or required clue sequence.
- Neither candidate requires a boss, combat, a specific tool, special weather,
  a timing window, swimming, a precision jump, or a dangerous climb. The
  intended approach is ordinary walking on stable terrain.
- The reward belongs to the player who successfully inspects the candidate;
  one player's claim must not prevent a nearby co-op partner from discovering
  the same feature. The server-side child will define exact eligibility and
  duplicate handling.
- Use the existing world presentation and fog-of-war rules. Do not publish an
  undiscovered candidate list, add a map/minimap marker, or query hidden
  discovery positions from the client.
- The canonical IDs above identify candidate kinds only. The server-side child
  will derive each world/player sparse identity, deterministic placement,
  same-world interaction eligibility, and atomic reward from authoritative
  state. Clients submit interaction intent only.
- Do not enable durable claim writes or journal state before the M9 save
  versioning and migration backlog goal passes. Until then, discovery claims
  remain transient and session-bounded; reconnect persistence is not claimed.

The slate adds no boss encounter or combat gate. Boss rewards remain optional
under the existing game-design contract and can be considered separately if a
later accepted candidate needs them.
