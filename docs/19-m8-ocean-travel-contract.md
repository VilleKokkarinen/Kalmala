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

`Blocked` describes a collision or invalid-water response only. Hull damage, disablement, repair, salvage, and their recovery path are not implied by this state; the separate M8 failure/recovery backlog item must decide whether those mechanics apply.

## Replication and persistence

Replicate skiff movement at no more than 10 updates per second while moving. Replicate movement mode and the two session occupant references to relevant peers. Do not replicate raw steering samples, input sequence/freshness, or per-player control data to other players. Send transient action and failure feedback only to the requesting player; do not multicast every input sample.

Until the M8 persistence gate passes, do not save vessel identity, occupancy, transform, cargo, or travel state in any world or player save. A server-created skiff and its occupancy expire with the session. No saved-data schema changes as part of this contract.

## Multiplayer assessment

This contract preserves server authority: clients send bounded action and steering intent, while the server selects seats, validates world/collision constraints, simulates movement, and publishes accepted outcomes. The only public replicated vessel state is relevant movement, mode, and current session occupancy. Private input and all persistent state remain excluded.

## Implemented launch and occupancy increment (2026-09-24)

The first runtime increment adds one server-created skiff per session. Launch uses the existing server-side interaction trace and accepts only a hit on an active generated terrain patch within 250 cm whose sampled Ocean depth is at least 100 cm and whose position is inside the 16 km world boundary. The launch point comes from the server trace; a client cannot submit a transform. A second skiff is rejected for this two-player prototype session.

The first eligible player receives the Helm seat and the next distinct session player receives Passenger. Seat references and the `Moored` state replicate; the skiff remains unowned by either client. Occupants attach to server-selected local seat positions while their normal pawn movement and collision are disabled. An occupant may request disembarkation only while stopped; the server checks up to eight generated-world exit positions, requires deep ocean or dry land, and accepts the first capsule placement with no blocking pawn collision. If no safe exit exists, seat and pawn state remain unchanged. Travel state remains transient and is not saved.

This increment does not yet accept throttle or rudder intent or move the skiff; its mode remains `Moored`. Sequenced helm input, bounded server movement, stale-input decay, and collision sweeps remain open under the authoritative traversal backlog task. Player-facing launch denial and shallow-water guidance remain in the coast-readability task. `Kalmala.Gameplay.OceanTravel.SkiffAuthorityContract` covers launch gates, seat selection, and stopped safe-exit conditions; it does not replace a live host/client collision and interaction scenario.
