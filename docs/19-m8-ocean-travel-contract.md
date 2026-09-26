# M8 first ocean travel-medium contract

This is the design contract for the first M8 authoritative-traversal increment. Runtime launch, movement, damage, and persistence are not implemented by this document.

## Travel medium and identity

The first medium is a project-original open-water skiff with two seats: one helm and one passenger. It carries no cargo. This contract does not prescribe its mesh or recipe. Each skiff is a transient server-created actor; the primary skiff receives the stable world-scoped identity `ocean-skiff:primary` when its moored state is saved, while the live actor reference remains transient.

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

The versioned M8 persistence gate below now permits one server-created skiff's moored snapshot and authenticated player seat references. Cargo, velocity, steering input, and live actor references remain excluded. Ocean discovery claims continue in the separate M7 sparse ledger.

### Versioned travel-save contract and runtime restore

`UKalmalaOceanTravelPersistenceSaveGame` defines a separate schema-1 gate and
does not extend or rewrite the M7 sparse-discovery container. It reuses the
M7 identity contract so every record is bound to the exact `WorldSeed`,
generator revision, and explicit world or authenticated-player scope. A
world-scoped record holds at most one stable `ocean-skiff:` identity and its
last server-accepted safe location and heading. The writer may capture that
snapshot only when the server has stopped the skiff in `Moored`; coordinates
must be finite and inside the 16 km world radius, with bounded vertical offset
and heading. A player-scoped record holds at most one stable vessel reference
and a `Helm` or `Passenger` seat, keyed by the authenticated player identity
in the enclosing save identity. The two records pair only when world seed,
generator revision, and vessel ID agree.

This skiff has no cargo, so the contract has no cargo field. It also omits
velocity, steering samples, session actor references, and saved live occupancy;
seat references are rebuilt from authenticated player records. The runtime
writes world state only when the server has a `Moored` skiff, then writes each
authenticated occupant's seat under that player's scoped slot. A disembark
clears the player's seat slot before moving the pawn, and fails closed if the
slot cannot be cleared. Startup accepts an existing world record only when its
identity, safe bounds, sea-level position, and full generated-ocean hull
footprint remain valid; actor spawn must also pass collision handling. On
login, the server pairs the player's exact identity and saved seat with that
world vessel, then rejects an already occupied seat, a duplicate player, or an
already attached pawn. Invalid or incompatible records are preserved without
overwrite. Ocean discovery `DiscoveryClaimed` ledgers are loaded for the
authenticated player at login, so reconnects keep duplicate rewards rejected.

Schema zero returns `MigrateBeforeLoad` and requires an explicit migration
before load; unknown future schemas fail closed. The focused
`Kalmala.World.OceanTravel.PersistenceContract` automation round-trips both
scopes in memory and through a local save slot, and rejects mismatched
identities, malformed bounds, orphaned vessel references, and invalid seats.
`Kalmala.Gameplay.OceanTravel.SkiffRestoreContract` covers server-only seat
restore gates, duplicate player/seat rejection, and safe disembark-save
feedback. Normal save and restore now run in the server game mode; the full
two-player restart/reconnect journey remains in M8 acceptance.

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

`Kalmala.Gameplay.OceanTravel.SkiffSteeringContract` verifies authority/helm ownership, bounds, sequence ordering, rate limit, input expiry, acceleration, and speed caps. `Kalmala.Gameplay.OceanTravel.SkiffAuthorityContract` continues to cover launch and occupancy. These pure contract checks and the editor build do not prove a live host/client collision sweep or rendered movement; those remain open M8 acceptance work. Coast and action guidance is covered by the presentation contract below.

In the development-only integrated journey fixture, local movement-axis callbacks are suppressed while scripted helm intent is active so neutral axis samples cannot overwrite the fixture throttle. After the fixture verifies its crosswind and calm states, automatic weather-cycle advancement is held at calm until the route completes; this prevents unrelated later wind from steering the test skiff off its sampled corridor. Neither guard changes normal movement input or production weather behavior.

On 2026-09-26, the forced UE5.8.2 build, `SkiffAuthorityContract`, `SkiffSteeringContract`, and `Scripts/Verify-OceanSkiffJourney.ps1 -Port 18169` passed. The seed-418 host and client observed a 242,511 cm crossing from terrain patch (71,0) to (152,5), with 9 active patches, both crosswind/calm states, and the final moored state. The fixture keeps `OriginShift=inactive`; this remains a bounded NullRHI integration check, not physical helm input or rendered travel acceptance.

## Coast and travel feedback presentation (2026-09-25)

The server records launch, boarding, and disembark results on the requesting
character's owner-only `UKalmalaOceanTravelFeedbackComponent`. It reports
shallow launch depth, an invalid nine-point hull footprint, the finite-world
edge, an existing session skiff, blocked coast placement, seat availability,
and safe-stop/exit requirements. A local row in the existing survival status
panel shows that result for eight seconds. While attached, the same row remains
visible with helm/passenger controls, the 100 cm depth rule across all nine
hull samples, generated-land and finite-world collision limits, and the stopped
disembark rule. A server-published skiff `BlockReason` distinguishes an invalid
deep-ocean footprint, generated-terrain collision, and a rejected movement
sweep; the local row gives recovery guidance from that replicated state.

Feedback does not change launch, seat, movement, or exit outcomes. Clients
cannot set it; action validation, hull sampling, movement, and safe placement
remain server-owned. Per-player reasons and serials replicate only to that
character's owner. The skiff's relevant movement mode and block reason remain
visible to relevant peers. No save field, client-selected position, or new
interaction authority was added. `Kalmala.Gameplay.OceanTravel.FeedbackAuthority`
checks the denial mapping and feedback authority; `Kalmala.UI.SurvivalStatus.LocalPresentation`
checks the local text. `Scripts/Verify-OceanSkiffFeedback.ps1` checks distinct
listen-host/client owner results and that the remote client sees no host-only
feedback. This host/client fixture verifies owner-scoped message transport and
the local text path; it does not exercise an actual coastline trace, rendered
skiff travel, or terrain collision sweep.

## Bounded ocean crosswind pressure (2026-09-25)

While underway, the server reads the existing replicated, server-selected
`FKalmalaWeatherState` and adds a heading-relative crosswind yaw rate to helm
rudder input. Head or following wind adds no turn; perpendicular wind adds at
most 8 degrees/second at full wind strength and 700 cm/second vessel speed.
The weather and rudder rates together remain within the existing 35
degrees/second steering cap. The helmsman can steer against the drift, and the
pressure scales continuously with wind strength, wind angle, and vessel speed;
calm weather removes it. Malformed weather values contribute no pressure.

The local survival status panel identifies crosswind while the player is in the
skiff and explains counter-steering. The pressure is derived only from the
server-selected weather and server-owned skiff movement; no client weather or
mitigation outcome is accepted, replicated separately, or saved. The focused
`Kalmala.Gameplay.OceanTravel.SkiffWeatherPressure` automation covers direction,
scaling, bounds, counter-steering, and malformed values, while
`Kalmala.UI.SurvivalStatus.LocalPresentation` covers the local guidance and its
removal in calm weather. The cross-peer accepted-weather and recovery check is
recorded below; live skiff travel remains in the longer M8 journey task.

## Cross-peer ocean weather verification (2026-09-25)

`Scripts/Verify-OceanSkiffWeather.ps1` starts a development-only listen host
and conflicting-seed client. The server selects a full beam crosswind
(cycle 7001, 90 degrees, strength 1), followed by calm weather (cycle 7002).
Both locally controlled peers read those accepted replicated snapshots and
run the production `CalculateWeatherYawRate` helper at a 350 cm/s verification
speed: the wind produces 4 degrees/second, a -0.2 rudder produces -3
degrees/second after counter-steering, and wind pressure returns to zero when
the calm state arrives. The client also calls the public server weather setter
with a forged calm cycle and verifies its local accepted snapshot stays on
cycle 7001; no cycle 7003 is replicated.

The focused authority tests continue to require server-owned weather and
steering outcomes. This peer fixture verifies replicated state and the
production pressure calculation, but it does not launch or move a skiff,
measure replicated hull transforms, or inspect a rendered viewport. Those
remain part of the longer two-player travel acceptance task.

## Sea-discovery placement and claim path (2026-09-25)

The bounded first-wave catalogue defines three original optional discoveries:
`ocean-driftwood-cache` (`Wood` x2), `ocean-shellbank-shoal` (`Fibre` x2),
and `ocean-stormmark-islet` (`Stone` x1). Each has a separate stable
presentation ID and readable name. Rewards reuse existing server item IDs and
are validated against the item catalogue; no new inventory item is introduced.

The canonical sparse fact ID is
`ocean-discovery:1:<discovery-id>:<spatial-x>,<spatial-y>`. It is stable for
that discovery kind and deterministic spatial key; the enclosing world save
identity supplies world seed and generator revision. Catalogue entries carry
no route, waypoint, direction, or coordinate data. A server hash selects at
most one optional discovery per 60 m population cell; a bounded retry search
requires an in-bounds master-ocean sample at least 100 cm deep. The server
materializes candidates only in the rolling 3-by-3 cell neighborhoods around
connected players, retaining at most 18 cells for the two-player prototype and
retiring actors as those neighborhoods move. The replicated actor contains
only its presentation ID; the client never selects or receives candidate
lists, stable IDs, positions, or rewards.

The existing server interaction trace resolves the target. The actor and game
mode recheck server authority, same-world identity, canonical generated
descriptor, and 250 cm range before claiming. The catalogue alone selects the
reward, and the server validates and builds the complete inventory change
before publishing it. A full pack leaves the discovery available. Successful
claims use the existing M7 version-1 sparse save container with explicit
player scope, authenticated owner identity, world seed, and generator revision.
The server persists a `DiscoveryClaimed` delta before granting the item, caches
the accepted ledger, and rejects duplicates after reconnect/load. Existing or
future-incompatible saves fail closed without overwrite. This reuses the
existing schema; vessel, seat, and ocean-claim state use their separate
versioned world/player save contracts described above.

`Kalmala.Gameplay.OceanTravel.DiscoveryCatalogue` checks bounded definitions,
known optional rewards, forged-input rejection, identity reproducibility, and
sparse-ID validity. `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract`
checks deterministic deep-ocean placement, forged kind/position rejection,
exact server reward construction, player/world/revision scope, sparse-ledger
round-trip, and replay rejection. Live host/client interaction and rendered
presentation remain part of the later M8 two-player journey acceptance. The
shared M7 ledger is bounded at 256 sparse facts per player; expanding that
budget requires the M8 persistence/budget review.

## Owner-scoped discovery and safe disembark peer check (2026-09-25)

`Scripts/Verify-OceanSkiffDiscoveryDisembark.ps1 -Port 18170` uses a separate
development-only listen-host/client fixture so discovery and exit verification
does not depend on the blocked helm driver. From a bounded deep-ocean search,
the server selects a canonical optional discovery, places the two authenticated
players in helm and passenger seats, then commits one claim and exact
catalogue reward into each player's existing sparse ledger and owner inventory.
With the vessel already `Moored`, it calls the production safe-disembark path
for both occupants. The server requires both seats empty, both players detached,
the vessel still moored, both owner feedback states accepted, and exactly one
reward per player. The remote client independently requires its private reward
and discovery acknowledgement, its disembark result, and the replicated empty
seats and moored mode.

This focused peer check does not simulate a voyage, decelerate a moving vessel,
exercise a client-originated discovery trace, verify restart/reconnect, or
measure long-session budgets. The existing journaled two-player helm crossing
remains blocked and is not marked as passing here.

## Late-join and authenticated restart/reconnect verification (2026-09-25)

`Scripts/Verify-OceanSkiffReconnect.ps1` runs two isolated seed-418 sessions.
In the first, the server creates one moored primary skiff, assigns the remote
owner to Helm and the host to Passenger, accepts the owner's one optional
discovery reward, and admits a third peer. The server and late peer require
one vessel with the original seats; the late peer remains unattached and has
no discovery feedback or reward. After all processes stop, the runner restarts
the host with the same profile and reconnects the owner with the owner's same
profile. The server restores the same moored vessel and both authenticated seat
associations. The owner cannot replay the accepted discovery; feedback is
`AlreadyFound` and the fresh inventory stays empty. A post-restart late join
again sees the single public vessel without private reward state.

The listen host can enter `PostLogin` before its pawn and PlayerState are ready
for the travel restore path. The server now queues that controller and retries
after both are available. The restore path is idempotent when that player
already occupies the saved seat. This does not add client authority, an RPC,
replicated gameplay state, or a save field. The runner assigns stable
development-only test-provider IDs because the headless null driver does not
provide a persistent external identity; actual platform authentication remains
unverified.

Verification (2026-09-25): The UE5.8.2 `KalmalaEditor Win64 Development`
build and `Verify-OceanSkiffReconnect.ps1` passed in a disposable project
mirror with normal `%LOCALAPPDATA%\\UnrealBuildTool` access. The full flow
verified the host Passenger and owner Helm restores, late-join state before
and after restart, matching host/owner identity hashes and discovery values,
and replay rejection without a second reward. This is a focused two-peer save
check, not a long-session actor, memory, replication, save-size, or frame-time
profile. The integrated 2.4 km crossing remains blocked separately.
