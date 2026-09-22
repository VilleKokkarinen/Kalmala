# Roadmap

Each milestone must leave the project playable. Do not begin a later milestone while the earlier one fails its acceptance criteria.

## M0 — Bootstrap

Create the UE5 C++ project, source-control setup, module skeleton, default map, basic input, CI/build notes, and developer onboarding.

**Accept:** editor opens, a packaged development build launches, and a dedicated-server target can compile (or its exact setup blocker is documented).

## M1 — Networked traversal and interaction

Implement replicated character movement, camera, interaction trace, interactable interface, and a two-player test map.

**Accept:** host/client can join, move, and reliably interact with the same object; invalid client interaction is rejected server-side.

## M2 — Survival camp loop

Add server-owned inventory, harvesting, campfire warmth, basic crafting, placement preview, and three construction pieces.

**Accept:** two players can gather, craft, build, save/load a camp, and observe matching state after reconnect.

## M3 — Elemental world prototype

Build the bounded interaction grid, fire/wetness/temperature rules, and client visual feedback.

**Accept:** a player who stands in rain receives the **Wet** status effect. Moving to a camp and standing near a lit campfire removes the effect, with clear client-side visual feedback for both state changes.

## M4 — Combat and support magic

Add combat attributes, damage execution, the Mireling, boar, and deer creature archetypes, open-world points of interest, and scroll-learned support magic. Implement Mending, Hearth Shield, Bear's Vigor, and Deer Call; place scroll discoveries across the biome and make one a boss reward.

**Accept:** focused server-authority, persistence, and peer regressions cover the
three creature archetypes, optional discoveries, and all four support effects.
No magic effect directly damages an enemy. A rendered two-player acceptance
scenario is not required by the revised roadmap scope.

## M5 — Vertical-slice finish

The world-generation track places **Phase 7 — Coherent biome generation and hydrology** before **Phase 8 — Ocean and long-distance travel**. Its current user-directed revision-7 rework samples a separately seeded land/water master atlas through a game-seeded crop and rotation, then applies the specified hard origin-distance biome limits with Meadows/Mountains fallback. It retains existing regional signals, enclosed lakes, large rivers and the 16 km radius. Revision 8 provides opt-in debug streams; revisions 1–6 remain compatible. Detailed contracts and acceptance checks are in `docs/08-world-generation-and-biomes.md` and `BACKLOG.md`. This does not expand the vertical-slice creature, platform, or online-service scope.

Add original art/audio pass, tutorial beats, settings/accessibility, performance pass, balance, regression tests, packaging, and a dedicated-server playtest.

**Accept:** a new player can complete the documented 20–30 minute co-op loop without developer tools.

## M6 — Production hardening and supported-session validation

Start only after M5's vertical-slice acceptance passes. Turn the accepted M5
loop into a reliable release candidate without adding new gameplay content,
changing saved-data schemas, expanding online services, or changing the
current PC solo/listen-server co-op scope.

M6 has four ordered goals:

1. **Close acceptance findings.** Fix issues found during the final fresh-player
   20–30 minute packaged co-op run. Convert each finding into a focused
   regression or an explicit documented non-goal. Prioritize crash and hang
   triage, reconnect and late-join reliability, save-identity safety, input,
   accessibility, and readable non-audio feedback.
2. **Re-run the complete release suite.** From clean temporary user
   directories, run the automated, rendered, current-generator, authority,
   persistence, reconnect, and performance checks twice. Preserve the logs,
   screenshots, package metadata, and known-limit record.
3. **Validate the Windows Development package.** Produce the accepted package,
   smoke-launch it, and verify that a fresh player can complete the documented
   loop using normal player actions only. Keep actor, memory, worker, raster,
   startup, replication, and save budgets within their recorded limits.
4. **Attempt dedicated-server validation only when unblocked.** If a
   server-capable UE 5.8 source build or distribution becomes available,
   compile the retained `KalmalaServer` target and run a bounded two-to-four-
   player playtest covering join, movement, interaction, weather and camp
   recovery, creatures, support effects, late join, reconnect, and sparse
   persistence. If the installed Launcher engine remains the only available
   distribution, retain the documented blocker and do not invent a replacement
   service.

**M6 multiplayer boundary:** the server continues to own world generation,
combat, support, discovery, rewards, persistence, and all accepted outcomes.
Clients provide intent only, and the hardening work must not add client-
selected targets, damage, timing, rewards, hidden-content queries, or save
values.

**M6 accept:** the supported Windows Development package passes the complete
release suite twice from clean temporary user directories; a fresh player can
complete the 20–30 minute co-op loop without developer tools; relevant peer
state, reconnect, and persistence remain correct; documented performance
budgets remain green; and the only remaining limitations are explicitly
recorded. Dedicated-server acceptance is conditional on the documented UE5.8
engine capability.

### World-generation Phase 5 — Companion minimap delivery plan

Deliver this UI feature as Phase 5 of the world-generation track, before biome expansion. It is a navigation aid, not a separate world simulation or a source of hidden gameplay information.

1. Add a `KalmalaUI` minimap view model that converts the locally available generated-world presentation and the owning player's replicated transform into map-space data. It must not query world actors directly or expose undiscovered server-owned population, loot, hazards, or other players beyond the normal game presentation contract.
2. Add a circular minimap widget anchored to the top-right HUD. Clip all terrain, water, and markers to the circle; keep the owning-player marker visible at the centre and rotate it to communicate facing direction.
3. Render a lightweight local representation of terrain, water, and known player-facing landmarks. Reuse the replicated world identity and deterministic terrain/biome sampling where appropriate; do not add a second biome map or replicate minimap textures.
4. Bind mouse-wheel input to the minimap zoom only while no modal UI owns the input. Expose tunable `MinZoom` and `MaxZoom` limits, clamp every wheel update to that range, and retain the selected local zoom for the active session.
5. Verify at multiple aspect ratios and UI scales that the circular mask remains top-right, the player marker remains legible, zooming clamps at both limits, and opening/closing other UI cannot trap movement or mouse-wheel input.
6. Run a host/client test confirming both players see a minimap derived from the same world identity while each sees only their own player-centred view; minimap interaction must neither mutate nor reveal server-authoritative gameplay state.
