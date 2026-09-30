# M9 progression and discovery persistence contract

This accepted design defines the version and migration boundary required before
M9 tool progression, station progression, and land-discovery claims may use normal saves. It extends the existing world
construction and player-discovery save owners; it does not add a parallel save
authority or enable persistence by itself. Runtime changes remain gated on the
round-trip and rejection coverage in the next backlog child.

## Identity and ownership

All new M9 records match the exact server-owned world seed, generator revision
7, and declared scope. World construction records use
`FKalmalaM7SaveIdentity::ForWorld(WorldSeed)`. Player tool and discovery records
use `ForPlayer(WorldSeed, AuthenticatedPlayerIdentity)`, with the existing
128-character identity bound and validation. The player identity comes from
the authenticated `PlayerState` unique ID; a local player index, display name,
client-provided ID, or shared fallback is never accepted. A missing or invalid
authenticated ID disables player-save restore and writes for that session.

The existing construction and player-discovery slot names remain in use. Their
schema-2 payloads carry the explicit M7 identity in addition to the legacy
seed/player fields where those fields are retained. A record is usable only
when the seed, revision, scope, and (for player state) owner all match exactly.
The existing `UKalmalaM7PersistenceSaveGame` schema-1 sparse world/player
ledger remains unchanged; its resource, creature, and ocean-discovery entries
are not reinterpreted or moved.

## State scope and bounds

| State | Scope and saved value | Bound and validation |
| --- | --- | --- |
| Carried tools | Authenticated player: canonical tool ID, authored tool level, and condition for each carried tool | At most the existing six records. IDs must be unique and in the tool catalogue; levels must match that tool's authored tier; condition must be finite and within its authored range. No skill ledger or pack inventory is included. |
| Workbench/Forge progression | World: accepted Workbench tool-rack and Forge anvil construction records (stable construction ID, existing kit identity, transform) | Existing 128 construction-record total and 32 attachment cap. At most one compatible attachment may upgrade each station. Effective level is re-derived from same-world placed actors and capped at level 2; no numeric station-level save field is added. |
| Land discoveries | Authenticated player: existing stable discovery IDs, including the two M9 land-discovery IDs | Preserve the old 64-entry first-wave allowance and add at most 64 M9 claims; schema 2 caps the combined set at 128 unique valid IDs, each no longer than the existing 128-character stable-ID limit. |
| Other camp structures | World: existing schema-1 construction records for the approved Forge, Grinding Stone, normal Chest, legacy Smoke Frame records, and other accepted structures | Preserve existing identities and records during migration; count all structures against the same 128-record cap. Smoke Frame remains a legacy save identity but has no item or recipe in the current schema-4 catalogue. |

No saved record contains a client-selected result, cost, tool level, station
level, reward, actor pointer, or hidden discovery descriptor. Clients continue
to submit intent only. World records remain server-owned and replicate through
the existing relevant construction actors. Detailed tool state remains
owner-only through the existing carried-tool replication; the save adds no
replicated field.

This contract does not persist ordinary carried materials or inventory,
skill/experience, active food or support effects, weather, exposure, tool
cooldowns, or other unapproved equipment slots. In particular, an M9 discovery
claim remains durable even if its common Stone/Fibre reward was in the existing
pawn-lifetime inventory and is lost on reconnect. The claim is not re-awarded;
the materials remain available from their ordinary sources. This contract does
not claim general inventory persistence.

## Schema and migration policy

1. Bump `UKalmalaConstructionSaveGame` and `UKalmalaPlayerDiscoverySaveGame`
   from schema 1 to schema 2 in their existing slots. Do not change the M7
   sparse-ledger schema or the storage schema.
2. Migrate a construction schema-1 payload only when its seed matches the
   requested world and every legacy record is valid, unique, and within the
   existing limit. The schema-1 format used the current generator without
   storing its revision, so the explicit migration binds it to revision 7,
   preserves every construction ID/kit/transform without moving or
   re-generating it, and begins with no attachment records.
3. Migrate a player-discovery schema-1 payload only when its seed and
   authenticated player identity match and its existing discovery/effect sets
   validate. Bind it to revision 7, preserve those sets, and initialize tool
   records and M9 land claims empty because schema 1 stored neither. Session
   claims are not imported from a schema-1 save; any still-live claims must be
   revalidated and merged on the first schema-2 write under step 4.
4. On the first schema-2 write, merge the validated migrated facts with the
   current server-owned M9 facts for that same scope. Re-derive construction
   descriptors and discovery IDs from current authoritative state; never
   accept client save contents. New worlds/players start with an empty schema-2
   record plus the normal authored starting tools.
5. Validate the complete candidate, including duplicate IDs, catalogue IDs,
   transforms, condition/level ranges, scope, and all caps, before replacing
   the saved slot or publishing the live transaction. Failure leaves the old
   slot and live state unchanged; never truncate, partially migrate, or reset
   a valid save to make it fit. A save failure rejects the associated
   progression, placement, or claim transaction.
6. Current schema 2 loads only on an exact identity match. A known schema 1
   uses only the explicit migrations above. Schema 0 requires a separately
   reviewed migration and is rejected until one exists; versions greater than
   2 fail closed. A malformed, duplicate, mismatched, or over-cap record is
   rejected without overwriting the original bytes.

## Required verification before enabling writes

The implementation child must add focused memory round-trip tests for both
schema-2 containers and fixture migrations from each known schema-1 container.
Coverage must prove that old construction, discovery, and learned-effect facts
survive migration; the new fields start empty when absent; and valid current
tool, attachment, and claim records survive serialization and
reload. Rejection coverage must include wrong seed, revision, scope, and
player; unsupported schema 0/future versions; malformed and duplicate IDs;
invalid transforms, tools, levels, and conditions; and every cap plus cap+1.
Each rejected migration or transaction must leave the source save and live
state unchanged. A host/client reconnect check must confirm server agreement,
replay rejection, and owner-only tool details without new replicated state.

Do not enable schema-2 writes in normal play until these checks pass. M9
cross-system acceptance remains responsible for restart/reconnect behavior and
documented actor, memory, replication, and save budgets.

## Implementation handoff

The schema-2 world construction candidate has memory round-trip and schema-1
migration coverage for exact identity, malformed records, duplicates, and
capacity rejection. The isolated schema-2 player candidate now round-trips
first-wave discoveries, learned effects, M9 claims, and carried tools; it
migrates validated schema-1 discovery/effect facts, binds the exact player
identity, and starts claims/tools empty. Its rejection coverage includes seed,
revision, scope, and player mismatches; unsupported versions; malformed IDs,
tools, levels, and conditions; duplicate claims/tools; and discovery, claim,
and tool bounds. Failed player migrations preserve the source save bytes.

The combined two-client host restart check passed for dedicated schema-2 test
slots: server world records and the owner's discovery, learned effect, M9
claim, and carried tools reloaded with matching seed/revision/owner identity;
re-adding the persisted claim was rejected; both clients received only their
own tool details. Normal construction and player-discovery slots remain schema
1, and no schema-2 runtime writes were enabled. The test uses a deterministic
test-provider identity and does not validate a production online identity
provider.

The player candidate still does not merge and revalidate live session claims on
its first normal write. M9 runtime save integration and cross-system acceptance,
including restart behavior and resource budgets, remain open.

### Normal construction writer handoff — 2026-09-30

The user authorized normal schema-2 writes to the existing construction and
player-discovery save slots. The construction slot now loads through the
schema-2 container, explicitly migrates a matching schema-1 save, and leaves an
invalid or mismatched existing slot untouched. A construction transaction
copies the validated current candidate, adds only the server-created record,
revalidates all records and caps, then saves before publishing the new in-memory
container. Approved Workbench racks and Forge anvils now use this saved path;
their levels remain derived from the restored nearby actors. A focused
write-candidate check covers legacy fact preservation, unchanged legacy source
bytes, and attachment serialization; the host/client construction restart
check covers schema-2 writes through the normal world slot. Player slots remain
schema 1 while their writer and
first-write merge/revalidation are implemented in the next increment.
