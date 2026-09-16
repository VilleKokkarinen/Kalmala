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
Definitions supply bounded range, cost, duration and magnitude. Unknown effects
fail closed. No support execution calls the damage path.

| Effect | Server-selected target and result |
| --- | --- |
| Mending | Living allied pawn in range and line of sight; positive heal clamped to maximum health; reject full health; no revive |
| Hearth Shield | Caster; temporary bounded absorption; expiry removes remaining protection; reject refresh while active |
| Bear's Vigor | Caster; temporary bounded stamina/strength modifiers; reject refresh while active; do not refill stamina on activation or expiry |
| Deer Call | Bounded nearby living deer selected by server; behavior influence only; no spawn, teleport, damage, harvest, or loot |

Bear's Vigor may modify a later validated weapon attack, but activation never
inflicts damage. Modifier removal must restore the baseline and clamp current
stamina if a temporary maximum decreases, without duplicating stamina ownership.
Deer Call must use a bounded actor query and behavior budget; an empty eligible
set rejects activation before payment. Specific balance numbers belong in shared
validated definitions when the respective runtime increment is implemented.

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

Later contract coverage must test direct client-role mutation and actual owner
RPCs, malformed enum/sequence/configuration, repeat and competing requests,
distant/occluded/foreign-world targets, friendly-fire rejection, phase/cooldown
gates, Wet-adjusted shared stamina, shield expiry, exactly-once defeat, and
support effects that never execute direct damage. Compare health, stamina,
effects, rewards and save bytes before and after rejection. Live host/client
verification must include late join and reconnect without redistributing rewards
or disclosing other players' learned effects. Passing a build alone does not
prove any of these future behaviors.
