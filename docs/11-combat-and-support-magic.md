# Combat and support-magic contract

This defines the first M4 contract increment. It specifies future runtime work;
it does not claim that combat components, abilities, RPCs, or progression saves
already exist. The project brief, game design, and multiplayer rules in
`02-technical-architecture.md` apply. The next backlog child defines persistent
IDs and sparse deltas; this document does not change any saved-data schema.

## State and ownership

One server-owned combat state belongs to each combat-capable pawn. Implement
attributes and effect execution through the architecture's GAS integration where
appropriate; do not add a second stamina or Wet implementation. The existing
character stamina owner remains authoritative until an explicitly verified
integration replaces it. All activation costs use the shared status cost path,
including Wet's multiplier, and compete atomically with sprint expenditure.

| State | Invariant | Visibility |
| --- | --- | --- |
| Health and maximum health | Finite; maximum positive; health clamped to zero through maximum | Relevant actor peers |
| Defeated | Server transition exactly once when health reaches zero; health zero implies defeated | Relevant actor peers |
| Action phase and action serial | Idle, windup, active, recovery, or defeated; serial selected by server | Relevant actor peers |
| Phase end time | Finite server-world time; clients use it for presentation only | Relevant actor peers |
| Protective shield and expiry | Finite nonnegative bounded absorption; expired shield contributes zero | Relevant actor peers |
| Active support effects and expiry | Server allowlisted effects, bounded entries, no implicit stacking | Relevant actor peers where visibly relevant |
| Stamina and cooldown eligibility | Existing stamina authority; server checks costs and time | Owning player only, except visible action phase |
| Learned support effects and rejection reason | Entitlement and bounded local feedback, never a discovery catalogue | Owning player only |

Replicate a coherent combat snapshot or reconcile attribute notifications against
the action serial so UI cannot treat notification ordering as another hit or
defeat. Initial replication must explain current state to late joiners without
replaying damage or granting rewards. Do not expand actor relevancy to disclose
hidden targets. UI displays text/symbol cues for hit, defeat, and unavailable
actions; colour alone is insufficient.

The initial attack loop replicates action phase and action serial to relevant
peers for readable presentation. Its result feedback is an owner-only,
monotonic serial plus one of `Hit`, `Defeat`, or `Unavailable`; it contains no
actor name, location, health, or target ID. The local pack HUD renders explicit
text for WINDUP, RECOVERING, READY, and the latest result. Rejected requests
are feedback-rate-limited, so invalid input cannot create replicated feedback
spam.

## Narrow client intent

Route reliable server RPCs through the requesting player's owned pawn/component.
These signatures describe the intended surface, not existing callable methods:

- `ServerRequestAttack(uint32 RequestSequence)` asks for the currently equipped
  basic attack. The server selects the weapon definition and target using the
  authoritative pawn transform, facing, and bounded collision query.
- `ServerRequestSupport(ESupportEffect Effect, uint32 RequestSequence)` asks for
  one allowlisted learned effect. The server resolves an ally from its own short
  targeting trace, the caster for self effects, or existing nearby deer for Deer
  Call. No client actor, discovery ID, position, damage, duration, cost, clock,
  health, shield value, or reward is accepted.

Sequence numbers are monotonically increasing within the owned pawn's session;
zero, repeats, and older values are rejected. Use one sequence space for both
intents, clear it only when the server creates a new pawn session, and reject
wraparound until a new session. Sequence numbers are replay guards, not authority.
Apply a bounded per-owner request rate gate before expensive traces. Excess and
invalid requests generate no replicated spam or unbounded queued actions.

Check authority and ownership again inside local mutation functions, including
when called directly in Shipping builds. Reject unpossessed, defeated, foreign
world, pending-destruction, busy, cooling-down, unaffordable, and invalid-definition
requests. Verify all numeric configuration and intermediate results are finite
and bounded. Failed activation changes no health, stamina, cooldown, effect,
defeat, reward, or save; internal rate/replay bookkeeping may change.

## Committed attacks and damage execution

On accepted attack, the server reserves the action serial, charges the validated
cost once, and starts windup. A miss still consumes that committed action and
recovery. Initial combat uses current server transforms with no client rewind.
At the active phase, rerun range, facing, collision obstruction, target life and
world membership checks. Validate the server's friendly-fire setting for player
targets; clients cannot select allegiance or bypass that setting.

Only an internal validated execution context may apply damage. It identifies the
server-owned instigator, action serial, target and weapon/archetype definition.
Never expose a generic damage-amount RPC or accept an arbitrary engine damage
callback as proof of a validated attack. Derive damage from server definitions
and authorized modifiers; reject nonfinite, negative, or excessive results.
Deduplicate target hits per action serial with a bounded target count. A basic
melee action selects at most one target. Check shield expiry on execution, absorb
at most its remaining strength, then clamp health loss. Each action reaches
recovery and returns to idle only by server time; defeat cancels pending actions.

At zero health, set defeat once, cancel active attacks and support effects, and
prevent further damage, healing, activation, or rewards on that actor. Mending
does not revive. Player respawn and loss policy are outside this contract and
must be defined before a playable player-defeat loop is accepted.

Creature defeat must join the existing authoritative `DefeatServer` and stable
spawn callback path, not create an independent defeat flag or reward callback.
No reward or persistence is enabled by this definition-only increment. The next
contract child must settle durable defeat/reward atomicity and retry behavior
before combat execution can grant loot or save progression.

## Stable identities and sparse progression deltas

M4 extends the existing generated-population persistence seam; it must not add
an actor-name, transform, client-generated GUID, or replicated discovery list as
an identity source. A stable content ID is a bounded canonical ASCII token made
only by the server from the immutable world identity, a content kind/version,
the deterministic spatial key, the descriptor ordinal, and its server-owned
archetype/effect definition. The canonical input is length-delimited before it
is hashed or formatted, so different fields cannot collide through separator
ambiguity. Runtime actor names, spawn order, net GUIDs, and client observations
are never part of this input.

| Content | Stable server-owned ID | Durable delta owner |
| --- | --- | --- |
| Generated creature | `Creature:<revision>:<archetype>:<spatial-key>:<ordinal>` derived with the existing population descriptor | World: defeated set |
| Point of interest | `Poi:<revision>:<poi-definition>:<spatial-key>:<ordinal>` derived from its bounded deterministic descriptor | World: resolved/claimed set only if its definition has a world-wide transition |
| Scroll discovery | `Scroll:<revision>:<scroll-definition>:<spatial-key>:<ordinal>` derived from its bounded deterministic descriptor or validated boss reward definition | Entitled player: discovered-scroll set and learned-effect set |
| Support effect | Canonical allowlisted `Effect:<effect-definition>` token, never free text | Entitled player: learned-effect set |

`revision` is the generator/content revision used to derive that descriptor, not
the actor's engine class version. Future runtime code must reject an empty,
overlong, malformed, unknown-kind, or non-canonical ID before lookup. It must
also recompute the descriptor from the server's current immutable identity and
require an exact ID match; possession of a syntactically valid ID is not proof
that the content exists or is relevant to the requester.

World deltas remain sparse, append-only semantic facts: a creature can be
`Defeated` once, and a POI can record only an explicitly defined world-wide
transition. They are keyed by the complete immutable world identity used by the
population save contract, including `WorldSeed` and generator revision whenever
that revision participates in descriptor generation. On load, a schema or
identity mismatch discards the entire M4 delta container and starts an empty
container; records from another world are never merged or replayed. The current
population save's generated harvest/wildlife/hazard sets remain unchanged by
this contract. A later approved schema migration may either extend that
container or introduce a distinct versioned one, but must preserve this
identity gate and bounded unique-record limits.

Player progression is separate from world state. Each record binds the same
immutable world identity to a server-authenticated stable player identity; a
display name, local controller index, connection address, or client payload
cannot select that identity. It contains only the bounded discovered-scroll and
learned-effect sets for that player. It is owner-only when replicated for local
feedback and is never used to enumerate undiscovered scrolls, POIs, creatures,
or another player's learning. A reconnect may restore only that same entitled
player's matching-world record.

Every future defeat or discovery transition is one server transaction: validate
the live descriptor, world membership, range/trace and pre-transition state;
write the one unique delta; durably save it; then expose the defeat, reward, or
learned entitlement. If validation or persistence fails, reveal no reward and
leave the actor/discovery available. Retried interaction after a committed
delta observes the existing fact and grants nothing again. A server restart
between persistence and presentation may restore the fact but must not replay a
reward. Future implementation must set explicit finite caps, reject overflow
without mutation, and test save bytes before/after every rejected request.

This is a contract only. It adds no save fields, slot names, serialization,
replication, actor, RPC, reward, POI, scroll, or learned-effect runtime behavior.
Implementing the versioned containers requires explicit approval for the saved
data schema change and focused migration/reconnect coverage.

## Non-damaging support execution

All effects require the requesting player's server-owned learned entitlement,
valid target, enough shared stamina, and an expired server cooldown. Validate
before charging once; start cooldown and apply the effect as one server operation.
Definitions supply bounded range, cost, duration and magnitude. The shared
server-owned support cooldown is five seconds for every effect; it changes only
the next-activation eligibility and is never supplied by the client. Unknown
effects fail closed. No support execution calls the damage path.

The foundation records only canonical allowlisted `Effect:<definition>` tokens in
the existing authenticated-player, immutable-world discovery save. On scroll
claim, the server commits both the discovery and learned token before the
owner-only acknowledgement; a failed save rolls both back. The owner receives a
bounded learned-effect bitmask, while active-effect kind and finite expiry are
relevant-actor presentation only. `ServerRequestActivateSupportEffect` accepts
only an enum and monotonic sequence, checks the server-owned entitlement,
stamina and cooldown before one cost is charged, and never accepts a client
target, damage, duration, magnitude, or effect ID. Until the individual effect
increments provide their validated result, the short active state is purely
replicated presentation and has no combat or world mutation.

The local learned-effect panel reads the owner-only learned mask, the owning
pawn's replicated stamina, owner-only cooldown expiry and result, and the
relevant active-effect presentation. Keyboard 1–4 or controller D-pad selects
Mending, Hearth Shield, Bear's Vigor, or Deer Call; Q or the controller top
face button requests activation. The owned pawn sends only the selected
allowlisted enum and its monotonically increasing session sequence. Server
acceptance or generic unavailability feedback and cooldown expiry replicate to
the owner only; rejected-result updates are rate-limited. Other peers do not
receive learned-effect or private discovery acknowledgement state. Each effect
row names its matching controller D-pad direction and keyboard key.

| Effect | Server-selected target and result |
| --- | --- |
| Mending | Living allied pawn in range and line of sight; positive heal clamped to maximum health; reject full health; no revive |
| Hearth Shield | Caster; temporary bounded absorption; expiry removes remaining protection; reject refresh while active |
| Bear's Vigor | Caster; temporary bounded stamina/strength modifiers; reject refresh while active; do not refill stamina on activation or expiry |
| Deer Call | Bounded nearby living deer selected by server; behavior influence only; no spawn, teleport, damage, harvest, or loot |

### Mending runtime increment

Mending now resolves exactly one other `AKalmalaCharacter` from the caster's server-owned forward Pawn trace (350 cm), then performs a separate server visibility trace to that pawn. It cannot name a target in the activation RPC and cannot select wildlife or another non-player actor. The target must be in the same world, alive (health above 1), damaged, and within 350 cm. After that validation, the server consumes the shared 18-stamina base activation cost, adjusted by the authoritative status definition (20.7 while Wet), applies a finite 30-point heal clamped to 100, and starts the existing five-second cooldown/presentation transaction. A full-health, dead, distant, occluded, foreign-world, self, non-character, or invalid target leaves health, stamina, cooldown, active state, and request sequence unchanged. Mending has no damage call and does not revive.

### Hearth Shield runtime increment

Hearth Shield targets only its caster. Once the existing server-owned entitlement, sequence, cooldown, and shared 18-stamina base activation transaction succeeds (20.7 while Wet), the server creates 40 points of protection for ten seconds and replicates its remaining strength and finite expiry to relevant peers alongside the active-effect presentation. A second Hearth Shield request is rejected while positive protection remains before its expiry, without charging stamina or advancing the request sequence. The existing server wildlife-damage gate alone asks the caster's component to absorb the smaller of its validated damage and the remaining shield; clients cannot supply damage, strength, duration, expiry, or an absorption result. Expiry clears any unused protection before later damage can use it. Hearth Shield never damages an enemy and introduces no reward, save, or targeting payload.

### Bear's Vigor runtime increment

Bear's Vigor targets only its caster. After the shared server-owned entitlement,
sequence, cooldown, and shared 18-stamina base transaction succeeds (20.7 while
Wet), it raises the existing
authoritative stamina cap from 100 to 140 and supplies a 1.4 strength multiplier
for ten seconds. It never refills stamina on activation, and expiry restores
the 100 cap while clamping only any excess current stamina. A second request
while the finite modifier is active changes no stamina, cooldown, sequence, or
presentation state. The existing server combat execution may apply the bounded
modifier to its own 25-point attack definition (at most 35); activation itself
does not trace, target, damage, spawn, reward, or persist anything. Relevant
peers receive the active kind, finite expiry, and bounded strength presentation;
the authoritative stamina cap remains normal movement state. Clients supply no
target, maximum, multiplier, duration, expiry, damage, or combat result.

Deer Call uses a server-only query for existing, living, idle Deer within 900 cm
of the caster and a maximum three-actor behavior budget. Each selected Deer
receives a bounded 1.5-second flee leg away from the caster. An empty eligible
set rejects activation before payment. The effect never spawns, teleports,
damages, harvests, defeats, or grants loot, and no target or destination is
provided by the client.

### Deer Call runtime increment

After the shared server-owned entitlement, monotonic sequence, cooldown, and
authoritative stamina checks succeed, the server selects at most three existing
living idle Deer within 900 cm and applies a deterministic bounded flee
influence away from the caster. Selection and behavior mutation are server-only;
the activation RPC remains enum plus sequence. A defeated, non-Deer, moving,
distant, or absent actor is ineligible. The effect has no spawn, teleport,
damage, harvest, defeat, reward, or persistence path.

## Required implementation evidence

The first focused automation coverage is
`Kalmala.Gameplay.Combat.IntentContract.RejectionDoesNotMutate`. It exercises
the reusable `FKalmalaCombatIntentContract` server-derived validation seam with
client-only, distant, duplicate, and malformed attack/progression attempts.
For every rejected attempt it compares health, defeat state, reward count, and
memory-serialized sparse world-save bytes before and after the request. This is
contract coverage only: it does not add a combat RPC, actor, reward, or player
progression save schema. Runtime handlers must use this seam in addition to
their own authoritative trace, descriptor, transaction, and persistence work.

The support-magic regression covers every allowlisted effect's malformed enum,
client-role, replay/zero-sequence, learned-token reconnect, and non-damaging
gates. Active-effect fields remain ordinary relevant-peer replication state and
are audited by the replication contract; a rendered live-peer cast remains part
of the later M4 vertical slice.

Later contract coverage must test direct client-role mutation and actual owner
RPCs, malformed enum/sequence/configuration, repeat and competing requests,
distant/occluded/foreign-world targets, friendly-fire rejection, phase/cooldown
gates, Wet-adjusted shared stamina, shield expiry, exactly-once defeat, and
support effects that never execute direct damage. Compare health, stamina,
effects, rewards and save bytes before and after rejection. Live host/client
verification must include late join and reconnect without redistributing rewards
or disclosing other players' learned effects. Passing a build alone does not
prove any of these future behaviors.

## Basic attack runtime increment

The first executable combat increment attaches `UKalmalaCombatComponent` to
each player pawn. Left mouse / controller right shoulder sends only
`ServerRequestAttack(uint32 RequestSequence)`. On the server, a request must
advance the owned pawn's session sequence and find an alive
`AKalmalaWildlifeSpawn` through the pawn's forward visibility trace within 220
cm. The server alone records a replicated action serial, enters an 0.18-second
windup, rechecks the selected target at execution, applies the fixed 25 damage,
then holds a 0.36-second recovery before another request can be accepted. The
shorter recovery keeps a committed attack readable while making repeated
optional wildlife encounters less stop-start; it remains a positive server-time
busy window and does not alter target selection, damage, replay gates, or
relevant-peer action presentation.

Wildlife now owns replicated 100-point health and accepts only finite, positive,
server-local combat damage capped at 100 per execution. Zero health joins its
existing `DefeatServer` callback, so the established authoritative population
defeat persistence path remains the only defeat transition. There is no player
friendly fire, generic damage endpoint, client target reference, reward, new
save data, or client-selected cooldown/timing value. The component replicates
action phase/serial plus owner-only, target-free feedback. The local HUD shows
the feedback with text rather than colour alone.

## Basic attack peer verification

After a forced editor build, run `Scripts/Verify-CombatPeer.ps1` on an unused
port. Its development-only fixture uses the first deterministic wildlife
descriptor for the host player's active spatial key. A conflicting-seed client
sends one ordinary target-free attack sequence while positioned beyond range;
the server must reject it without changing the target. The host then completes
four normal server-validated attacks. The client must observe only the relevant
shared action serial and wildlife defeat, while its owner-only unavailable
feedback contains no target identity. Finally, the runner restarts the same
host save directory and requires that exact sparse defeated-spawn record to
keep the wildlife absent.

The fixture adds no gameplay target/damage payload, reward, player progression
record, save schema, or normal-play mutation. Player combat state is
pawn-lifetime and begins at its replicated defaults after reconnect; the
restart assertion covers the existing identity-scoped world defeat delta.

## Wildlife population foundation

The existing server-only spatial-key activation now treats wildlife descriptors
as dry, terrain-safe candidates. For each bounded wildlife budget it considers
at most four deterministic candidates per requested slot, retaining only
candidates inside the immutable-world bounds that are outside ocean and lake
water and whose generated collision normal has Z at least 0.82. The accepted
candidate seed remains part of the existing stable sparse-delta identifier, so
defeat persistence still suppresses precisely the same server-derived actor on
restart. Fewer than the nominal budget is valid when no safe candidate exists.

Only the server materializes accepted descriptors and owns their defeat state.
Wildlife uses ordinary distance relevancy; a client receives an actor only once
it is relevant, never a population descriptor or a client-selected fallback.
This establishes placement constraints only—archetype appearance and behaviour
remain subsequent increments.

## Wildlife behaviour foundation

Each relevant wildlife actor runs a small server-only, bounded behaviour cycle.
It starts `Idle` at its accepted descriptor origin. Validated nonlethal server
combat damage alone starts a 1.5-second `Flee` toward a deterministic,
spawn-ID-derived point no farther than 300 cm from that origin. It then spends
at most one second `Investigating` a second deterministic point within 120 cm,
and `Returns` to origin for at most two seconds before settling at `Idle`.
Movement is server-ticked in steps no larger than 0.10 seconds at 220 cm/s and
uses ordinary actor movement replication; no behaviour enum, destination,
descriptor, client command, or client-selected outcome is replicated.

Only the ordered `Idle → Flee → Investigate → Return → Idle` transitions are
accepted, and defeated actors cannot enter the cycle. This is intentionally a
shared primitive, not an archetype policy: Mireling, boar, deer, noise, Deer
Call, targets, rewards, damage, and herd decisions remain later work. The
focused `Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle` automation proves
client and defeated paths are rejected and that the server cannot skip the
bounded transition order.

## Mireling encounter increment

The first generated wildlife presentation is an original low-poly Mireling
silhouette assembled from procedural lichen, root and crown forms, with ordinary
actor movement replication. On the server only, an idle Mireling closes toward
the nearest living player within 500 cm and can apply fixed 10-point melee at
most once every 1.25 seconds inside 180 cm. The receiving pawn rejects non-authority,
wrong-world, non-finite, excessive and out-of-range calls; this increment clamps
at one health because player defeat/respawn is still undefined. A validated
player melee hit records that player as the eligible attacker; the one-time
existing defeated transition then grants a bounded `MirelingAsh` stack through
the owner-only inventory. Clients supply no target, damage, cooldown, health or
reward input. Full persistence-before-reward and host/client encounter evidence
is now covered by `Scripts/Verify-MirelingPeer.ps1`. The development-only
two-peer fixture repeats the server seed descriptor build, bounds active actors
to that descriptor set, places the Mireling through its ordinary relevant-actor
path, and proves the remote owner's target-free request cannot change its
health. It records server-owned melee as relevant replicated player health,
shared action/defeat presentation, one owner-only `MirelingAsh` reward, and the
existing sparse defeated delta after a listen-server restart. The fixture uses
the conflicting client seed 999 against server seed 418 and does not add a
normal-play RPC, descriptor replication, reward payload, or save schema.

## Boar territorial encounter increment

The server derives a boar from the existing stable wildlife descriptor seed
(`SpawnSeed % 3 == 0`); this preserves the descriptor's sparse-delta ID and
does not send archetype selection input from a client. Relevant peers receive
only the normal replicated wildlife actor and its bounded replicated archetype
for presentation. Boars use an original low-poly brown body, head, legs and
tusks assembled from project procedural geometry.

While resting at their generated origin, a living server-authoritative pawn
within 500 cm is a valid territorial threat. A nonlethal validated player hit
also begins a charge. For at most 1.25 seconds, the server alone follows that
validated target at 520 cm/s, applies at most one 15-point melee hit every
1.25 seconds inside 180 cm through the pawn's shared wildlife-damage gate, and
then returns to its origin at 260 cm/s. Clients cannot name a boar, target,
resting area, charge destination, damage, timing, health, or reward.

At the one existing server defeat transition, the established sparse world
defeat callback runs before owner-only inventory presentation. A valid boar
attacker can receive one catalogue-validated `BoarMeat` and one `BoarHide`;
inventory authority, stack ceilings and owner-only replication remain the
existing inventory contract. This increment neither changes the population
save schema nor introduces a loot RPC. The focused
`Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle` automation now covers
deterministic boar selection and charge rejection for client, defeated and
distant paths. `Scripts/Verify-BoarPeer.ps1` adds the bounded two-peer proof:
the listen server derives a stable nearby boar descriptor, demonstrates its
ordinary resting-area charge and melee against the host, then holds that same
already-relevant actor in the normal 220 cm player-combat trace during each
separate windup/recheck. The remote peer sends only its existing target-free
sequence and observes its rejection plus relevant action, player-health, and
defeat replication. Four server-committed attacks must record the existing
sparse defeat before the authoritative attacker receives one `BoarMeat` and
one `BoarHide`; the remote inventory remains empty. A fresh listen server uses
the same server-derived bounded boar selector to confirm that exact defeated
spawn remains absent. No client provides a descriptor, persistent ID, target,
damage, reward, or save value.

## Deer wary-herd increment

The server selects Deer from the existing stable wildlife descriptor seed only
after the established Boar branch, preserving descriptor identity and sparse
defeat compatibility. Deer have an original low-poly body, legs, head, and antler
silhouette assembled from project procedural geometry; relevant peers receive
only the normal replicated actor, movement, and bounded archetype presentation.

A validated nonlethal server combat hit starts the target Deer's existing bounded
flee cycle and alerts only idle, living Deer within 800 cm. Each alerted Deer
derives its own stable spawn-ID-based destination and runs the existing
`Flee -> Investigate -> Return -> Idle` budget. This is the initial combat-noise
response; no client can choose a herd, noise source, flight destination, damage,
or reward. At the established sparse defeat transition, the validated attacker
receives owner-only catalogue-validated `DeerMeat` and `DeerHide`; no loot RPC,
save schema, or descriptor replication is added. `Scripts/Verify-DeerPeer.ps1`
provides the dedicated two-peer/restart coverage. It derives both the target and
companion from bounded server descriptor keys, positions only the normal
generated companion inside the existing 800 cm server noise radius, confirms
its non-idle flight after the first validated hit, rejects the remote target-free
request, proves the peer's normal replicated action/defeat presentation, and
restarts the host save to confirm the same server-derived target stays absent.
The companion, noise, flight state, target, damage, rewards, and persistence
decision all remain server-owned.

## Optional discovery descriptor foundation

`FKalmalaWorldPopulationLayout` now derives at most one `PointOfInterest` and
one `Scroll` descriptor for an invisible 6 km spatial key. Each kind has its
own server-only world-seed domain and considers no more than eight deterministic
candidates. A candidate must remain inside the finite world, dry, gently
traversable, and in a suitable biome (including optional Meadow stones); it has no actor, marker, map
pin, route, reward, save record, RPC, or replication. POIs select an original
biome-local definition such as `elderwood-root-hollow` or
`mountains-storm-overlook`; scrolls select
only the allowlisted future effect definitions `mending`, `hearth-shield`,
`bears-vigor`, or `deer-call`.

The canonical descriptor ID is `Poi:1:<definition>:<x,y>:<ordinal>` or
`Scroll:1:<definition>:<x,y>:<ordinal>`, with the matching immutable world
identity remaining the enclosing server save key. The server must recompute the
candidate and exact ID before any later materialization or entitlement check;
clients cannot enumerate, request, or infer undiscovered descriptors. This
foundation deliberately leaves one-time rewards, player progression persistence,
and active support effects to later increments.

## Discovery reward and entitlement increment

The server materializes only descriptors in an already active population key as
ordinary relevant interaction actors. The normal owner-pawn interaction trace
still carries no discovery ID, target, definition, position, reward, or player
identity. On interaction the authoritative game mode recomputes the descriptor
from the immutable world identity, checks exact canonical ID/location, range,
world membership, and a server-authenticated `UniqueNetId` before it considers
a claim. A malformed, distant, duplicate, foreign-world, or unauthenticated
claim changes nothing.

Each authenticated player has a bounded, versioned save record containing only
the matching world identity, authenticated identity, and canonical discovered
IDs. It is saved before owner-only acknowledgement. The slot uses a CRC of the
authenticated identity rather than the raw ID; the raw identity remains inside
the save identity check. A reconnect loads only that same matching record. No
player record is replicated as a catalogue. The owning pawn receives an
owner-only feedback serial with explicit text: landmark/scroll found, already
found, or unavailable. Scroll discovery records the entitled discovery and
allowlisted learned effect; the optional Mireling boss reward uses the same
player-scoped save and feedback seam without adding a route, target, or client
reward payload.

## Mireling boss scroll reward

The server designates a bounded subset of generated Mirelings as boss-reward
candidates from their stable population ID (`CRC32 % 5 == 0`). Defeating one
through the existing validated combat path can award the one world-derived
scroll ID `Scroll:1:<effect>:mireling-boss`, where the allowlisted effect is
selected only from the immutable world seed. The server confirms that the ID
belongs to an actually defeated Mireling in the current world before loading
the authenticated attacker's player save.

The player save adds the canonical discovery and learned-effect token, saves
successfully before support-state replication or owner-only feedback, and
rolls back both facts on save failure. A repeated claim is rejected without a
second entitlement. Ordinary Mireling Ash remains the existing inventory
reward. The boss designation and reward ID contain no route, transform,
client-selected target, or client-supplied reward value; route choice only
determines whether the player happens to encounter the optional candidate.
