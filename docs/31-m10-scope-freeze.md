# M10 feature and content scope freeze — 2026-10-01

## Decision

M9 cross-system acceptance passed on 2026-10-01 (see `PROGRESS.md`). The
accepted M0–M9 gameplay and content contracts are now the M10 launch baseline.
M10 is release validation and hardening; it does not add another gameplay
system or content wave. `BACKLOG.md` has no unchecked gameplay/content task
outside M10. The documented M5 external-surface skip remains an explicit
limitation and is not player-visible packaged acceptance.

The detailed contracts remain authoritative in the linked project brief,
design, roadmap, decision log, and system documents. This record freezes their
accepted behavior and data identities for release work; it does not replace
those specifications.

## Product and gameplay baseline

- Keep the Windows PC target and the existing 1–4 player solo, listen-server,
  and dedicated-server session model. Dedicated-server validation remains
  conditional on the documented Unreal capability. The business model remains
  undecided; this freeze does not choose monetization or add an online service.
- Keep the seed-generated, route-free wilderness and the accepted survival,
  construction, weather, combat, support-magic, traversal, accessibility, and
  owner/server authority contracts from M0–M8. The M8 travel scope remains the
  accepted no-cargo skiff and optional ocean discoveries described in
  `docs/19-m8-ocean-travel-contract.md` through
  `docs/22-m8-integrated-discovery-voyage.md`.
- Keep the accepted M9 source set and bounded server-owned harvest, tool,
  station, optional-discovery, and persistence behavior described in
  `docs/23-m9-second-wave-biome-sources.md` through
  `docs/30-m9-persistence-migration.md`.
- Keep `Content/Data/GameCatalogues.json` at schema 4. Keep the normal Chest
  and the Cooking Rack, Cauldron, and Frying Pan; the six active recipe IDs and
  retired aliases are listed in
  `docs/28-m9-camp-equipment-recipes.md`. Smoke Frame and the former
  smoke/roast IDs are not current content. Optional storm shelter and insulated
  wrap examples remain deferred.
- Keep optional discoveries route-free, and keep gameplay outcomes,
  persistence writes, rewards, and hidden-content selection server-owned.
  Clients submit intent only; private inventory and progression details remain
  owner-scoped.

The accepted generated-world identity and persistence matching rules remain
unchanged. M9 saves use the exact server world seed, generator revision 7, and
explicit world/player scope as applicable.

## Frozen save and catalogue versions

These are the current formats in the source tree. The two M9 schema-2 owners
continue to read and explicitly migrate valid schema-1 records under
`docs/30-m9-persistence-migration.md`. Other formats stay at their current
versions.

| Save owner or data contract | Frozen current version | Scope |
| --- | ---: | --- |
| Construction records and station attachments (`UKalmalaConstructionSaveGameV2`) | Save schema 2 | Server world |
| Player discoveries, learned effects, M9 claims, and carried tools (`UKalmalaPlayerDiscoverySaveGameV2`) | Save schema 2 | Authenticated player and exact world |
| M7 sparse persistence ledger (`UKalmalaM7PersistenceSaveGame`) | Save schema 1 | Identity-scoped sparse world/player facts |
| Generated population deltas (`UKalmalaWorldPopulationSaveGame`) | Save schema 1 | Server world |
| Chest storage (`UKalmalaStorageSaveGame`) | Save schema 1 | Server world |
| Ocean vessel and seat records (`UKalmalaOceanTravelPersistenceSaveGame`) | Save schema 1 | Server world and authenticated player |
| Local map exploration coverage (`UKalmalaWorldMapExplorationSaveGame`) | Save schema 1 | Local player |
| Local map pins (`UKalmalaWorldMapPinsSaveGame`) | Save schema 1 | Local player |
| `Content/Data/GameCatalogues.json` | Catalogue schema 4 | Shared authored item and recipe definitions; not a save schema |

Do not change a frozen save schema, slot identity, world identity, or migration
policy as routine release hardening. A required compatibility change needs
explicit user direction, a versioned migration and recovery plan, focused
round-trip/rejection coverage, and an updated release rationale before the
roadmap is reopened.

## Post-freeze change rule

Defect fixes, bounded tuning, accessibility and presentation improvements,
and implementation optimizations may proceed when they preserve the frozen
gameplay, network-authority, owner-privacy, and persistence contracts. Every
post-freeze change must include a focused regression and a written release
rationale that states why the change is required and how it preserves the
baseline. A change that adds gameplay/content or alters a frozen contract must
first reopen the M10 scope in `docs/04-roadmap.md` and record the explicit
decision in `docs/05-decision-log.md`.

## Remaining M10 validation and known limits

The remaining five M10 goals are release-validation work, not permission to
expand feature scope: full clean-profile validation; save compatibility and
recovery; representative performance closeout; release packaging and the
fresh-player co-op walkthrough; and release evidence archival. They remain
unchecked in `BACKLOG.md` until completed.

The M5 fresh-player packaged co-op loop, native physical-input session,
production online identity, and extended/peak performance profiles remain
unverified. Dedicated-server validation is conditional on engine capability.
Available world-profile values are single-run diagnostics; approved numeric
actor, memory, and replication ceilings have not been established. These are
tracked for M10 validation and do not reopen gameplay scope by themselves.
