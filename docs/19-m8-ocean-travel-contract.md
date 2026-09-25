# M8 first ocean travel-medium contract

This is the design contract for the first M8 authoritative-traversal increment. Runtime launch, movement, damage, and persistence are not implemented by this document.

## Travel medium and identity

The first medium is a project-original open-water skiff with two seats: one helm and one passenger. It carries no cargo. This contract does not prescribe its mesh or recipe. Each skiff is a transient actor created by the server and has no stable saved vessel identity.

The existing server-owned immutable `WorldSeed` remains the only generated-world identity. The skiff adds no generator input, biome, route, authored waterway, or alternate world identity. It operates on master-map Ocean; inland lakes do not qualify.

## Server-owned state

The server owns and mutates:

- the skiff actor's transform and velocity;
- movement mode: `Moored`, `Underway`, or `Blocked`;
- the helm and passenger seats, each occupied by at most one authenticated session player;
- the currently accepted helm input sequence and freshness timer; and
- launch, boarding, disembark, collision, and movement outcomes.

The server assigns seats. Only the helm occupant steers. A passenger has no movement authority. Vessel state is never created, selected, or moved by directly applying client-supplied actor transforms.

`Moored` is the launch/stopped state at or below 50 cm/s with no active throttle. `Underway` applies while speed is above 50 cm/s or a nonzero helm input is active, including coasting after input expiry. `Blocked` is entered only for a collision or invalid-water movement rejection; a fresh valid helm intent may move the skiff clear.

## Client intent and movement bounds

Launch, board, and disembark are action intents through the existing server-validated interaction path. A request may identify a candidate vessel for boarding or leaving, but the server validates that reference against the caller's current world, interaction range, visibility, and seat availability. A launch request carries no spawn point. Seat choice and exit placement are server-derived.

The helm sends only finite throttle and rudder values in `[-1, 1]` plus a monotonically increasing per-helm sequence number. Accept no more than 10 steering updates per second and expire the current input after 0.5 seconds without a fresh sequence. Older, duplicate, malformed, non-finite, or out-of-range input is rejected without state mutation. Initial shared compiled movement caps are:

| Limit | Initial cap |
| --- | ---: |
| Forward speed | 700 cm/s |
| Reverse speed | 200 cm/s |
| Acceleration | 100 cm/s^2 |
| Yaw rate | 35 degrees/s |

The server simulates bounded movement and sweeps the skiff against generated terrain collision. A locally controlled helmsman may predict presentation using the shared caps, but server movement and correction are authoritative. Disembark is accepted only at server-measured speed of at most 50 cm/s and only when the server can resolve a safe pawn capsule placement; clients cannot choose an exit location.

## Water, collision, and failure

The skiff may travel only over master-map Ocean where the existing generated-ocean depth sampler reports at least 100 cm of water. The server retains the existing 16 km playable radius and shared 25-patch terrain-streaming cap. Movement may not cross generated land, skip a collision surface, or continue beyond the finite-world boundary.

If a server sweep or water sample blocks movement, the server clamps to the last safe transform, zeros velocity, sets `Blocked`, replicates that mode to relevant peers, and sends the failure reason to the helmsman. A fresh valid helm intent may steer away from the obstruction. Stale input or a disconnected helm occupant clears throttle and server-decelerates the vessel to `Moored`. Invalid launch, boarding, steering, or disembark requests leave the affected actor and occupancy unchanged; the requester receives the denial reason.

`Blocked` describes a collision or invalid-water response only. This first session-only, no-cargo skiff has no hull health, disabled mode, repair, salvage, or accepted hull-damage source. Collision or invalid-water rejection already has a recoverable outcome: the server clamps to the last safe transform, stops the skiff, and accepts fresh helm input to steer clear; stale or disconnected helm input clears controls and decelerates the skiff to `Moored`. Adding durability now would require a new damage source and repair or salvage loop without an accepted role in this bounded travel medium. Revisit hull damage only if a later approved M8 hazard explicitly affects vessel integrity, and define its server-owned recovery before implementation.

## Replication and persistence

Replicate skiff movement at no more than 10 updates per second while moving. Replicate movement mode and the two session occupant references to relevant peers. Do not replicate raw steering samples, input sequence/freshness, or per-player control data to other players. Send transient action and failure feedback only to the requesting player; do not multicast every input sample.

Until the M8 persistence gate passes, do not save vessel identity, occupancy, transform, cargo, or travel state in any world or player save. A server-created skiff and its occupancy expire with the session. No saved-data schema changes as part of this contract.

## Multiplayer assessment

This contract preserves server authority: clients send bounded action and steering intent, while the server selects seats, validates world/collision constraints, simulates movement, and publishes accepted outcomes. The only public replicated vessel state is relevant movement, mode, and current session occupancy. Private input and all persistent state remain excluded.

## Seeded coastline verification (2026-09-25)

The `Kalmala.Gameplay.OceanTravel.SkiffCoastlineAccess` automation samples 32
radial mainland coast directions from each resolved start for seeds 418, 999,
and 1337. Every seed contains shallow-water positions rejected by the production
hull-depth predicate and at least one candidate satisfying its nine-point
100 cm ocean-depth footprint, the server launch gates, and one of the eight
190 cm safe-exit surface checks. The candidates are 79 m, 287 m, and 167 m from
the sampled dry shoreline, respectively, at 102.4 cm, 100.4 cm, and 100.5 cm
water depth. Their accepted exits remain in qualifying ocean water at 105.3 cm,
100.7 cm, and 101.2 cm depth, so the occupant can leave the skiff at the water
surface and continue swimming.

The companion `Kalmala.World.Water.OceanDepth` test confirms coastline depth
queries use the generated collision-triangle plane and clipped mesh. This is
deterministic sampler and rule coverage; it does not spawn the skiff, exercise
the server visibility trace or pawn-overlap query, prove a dry-shore disembark,
or test the actor's live terrain collision sweep. The open coastline-presentation
subtask and live host/client collision and feedback acceptance remain separate.

## Implemented launch and occupancy increment (2026-09-24)

The first runtime increment adds one server-created skiff per session. Launch uses the existing server-side interaction trace and accepts only a hit on an active generated terrain patch within 250 cm whose sampled Ocean depth is at least 100 cm and whose position is inside the 16 km world boundary. The launch point comes from the server trace; a client cannot submit a transform. A second skiff is rejected for this two-player prototype session.

The first eligible player receives the Helm seat and the next distinct session player receives Passenger. Seat references and the `Moored` state replicate; the skiff remains unowned by either client. Occupants attach to server-selected local seat positions while their normal pawn movement and collision are disabled. An occupant may request disembarkation only while stopped; the server checks up to eight generated-world exit positions, requires deep ocean or dry land, and accepts the first capsule placement with no blocking pawn collision. If no safe exit exists, seat and pawn state remain unchanged. Travel state remains transient and is not saved.

## Implemented sequenced steering and movement increment (2026-09-25)

The owning character maps the existing forward axis to throttle and right axis to rudder, then sends bounded intent through an unreliable server RPC; the server derives the skiff only from the character's current attachment and accepts input only from its current helm occupant. Both client send cadence and server validation cap accepted steering at 10 updates per second. Finite axes in `[-1, 1]` and strictly increasing per-helm sequences are required; malformed, out-of-range, passenger, replayed, and early requests do not mutate accepted steering state. Accepted input expires after 0.5 seconds, including when the helm controller disconnects.

The server advances speed at no more than 100 cm/s² toward 700 cm/s forward or 200 cm/s reverse and turns at no more than 35 degrees/s. Each bounded movement step samples the hull centre, edges, and corners against the seed-derived ocean-depth surface, checks the 16 km boundary and excludes inland lakes, then sweeps the replicated hull against active generated-terrain collision. Water or collision rejection restores the last safe transform, zeros speed, and publishes `Blocked`. Stale input decelerates to `Moored`; a fresh helm intent can retry away from an obstruction. Replicated movement is configured for 10 updates per second, and steering axes, sequence, and freshness remain server-private. Travel remains transient and unsaved.

`Kalmala.Gameplay.OceanTravel.SkiffSteeringContract` verifies authority/helm ownership, bounds, sequence ordering, rate limit, input expiry, acceleration, and speed caps. `Kalmala.Gameplay.OceanTravel.SkiffAuthorityContract` continues to cover launch and occupancy. These pure contract checks and the editor build do not prove a live host/client collision sweep, rendered movement, or denial feedback; those remain open acceptance work. Player-facing launch denial and shallow-water guidance remain in the coast-readability task.
