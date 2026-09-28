# M9 optional land exploration rewards

This document defines the first M9 land-discovery design slate and its runtime
contract. The optional discoveries are generated only for eligible active
60-metre population cells, and the server derives each stable identity,
placement, claim, and reward. Claims remain session-only until the separate M9
save migration is complete.

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
  one player's claim does not prevent a nearby co-op partner from discovering
  the same feature. Duplicate claims are tracked independently by authenticated
  player identity.
- Use the existing world presentation and fog-of-war rules. Do not publish an
  undiscovered candidate list, add a map/minimap marker, or query hidden
  discovery positions from the client.
- The server derives one candidate for a spatial cell only when its center is
  in the candidate's biome. Placement starts from the existing biome discovery
  point and probes up to 24 deterministic offsets to avoid overlapping its
  first-wave marker. The final position must be in bounds, dry, walkable, and
  in the same biome; the Rillstone also requires lake water within 350 cm, and
  Leeward Grain requires elevation of at least 0.80.
- Stable sparse identities use
  `land-discovery:m9:1:<candidateId>:<x>,<y>`, where the coordinates are the
  population spatial key. The server re-derives the descriptor before use.
  Clients receive only the presentation identity and submit interaction
  intent; they do not receive candidate descriptors, rewards, or hidden
  discovery positions.
- A claim requires server authority, the same world, an active population
  cell, the canonical descriptor, an authenticated player identity, and a
  distance of at most 250 cm. The server preflights the exact inventory grant,
  records the sparse identity for that player, and commits the inventory
  transaction; failed commits roll back the claim. The in-memory claim set is
  capped at 64 discoveries per player per session, and accepted feedback is
  owner-only.
- Do not enable durable claim writes or journal state before the M9 save
  versioning and migration backlog goal passes. The claim set is held in
  server memory only; reconnect and restart persistence are not claimed, and
  no saved-data schema is extended.

The slate adds no boss encounter or combat gate. Boss rewards remain optional
under the existing game-design contract and can be considered separately if a
later accepted candidate needs them.
