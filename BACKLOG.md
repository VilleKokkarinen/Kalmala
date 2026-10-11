# Autonomous development backlog

Documentation layout: former docs 08–47 are consolidated into the core docs (00–07). See [Game features](docs/game-features.md), [architecture contracts](docs/02-technical-architecture.md#consolidated-system-contracts), and [verification index](docs/07-development-setup.md#consolidated-verification-index). This user-directed cleanup does not close any milestone.

This is the short execution queue and completion index. The roadmap and feature contracts in docs/ remain authoritative for scope and acceptance criteria.

## Roadmap status

M0–M13 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. M13 integrated-main acceptance passed on 2026-10-11. M14 implementation and acceptance passed on an isolated integrated source; its main-branch integration remains pending.

## Completed roadmap log

- **M0 — Bootstrap:** project setup, prototype map, packaged build, and dedicated-server availability recorded.
- **M1 — Networked traversal and interaction:** replicated movement and server-validated interaction.
- **World generation, Phases 1–9:** deterministic world generation, survival, biome expansion, ocean travel, and expanded maps.
- **M2 — Survival camp loop:** gathering, crafting, construction, and camp persistence.
- **M3 — Elemental world prototype:** server-owned weather, wetness, exposure, and recovery.
- **M4 — Combat and support magic:** combat, creature encounters, discoveries, and support effects.
- **M5 — Vertical-slice finish:** completed under the recorded acceptance scope.
- **M6 — Production hardening and supported-session validation:** completed under the recorded release scope and limits.
- **M7 — Survival progression and readable gameplay UI:** completed.
- **M8 — Ocean and long-distance travel:** completed; dry-shore acceptance was waived.
- **M9 — Expanded biome content and encounter depth:** completed under the recorded scope.
- **M10 — Release completion and launch validation:** completed under the recorded scope and limits.
- **M11 — User experience and visual UI upgrades:** final acceptance passed 2026-10-06.
- **M12 — Inventory menu and cleaner gameplay HUD:** final acceptance passed 2026-10-09.
- **M13 — Revalidated UI on main:** final acceptance passed 2026-10-11; evidence in docs/evidence/m13/.
- **M14 — Expanded world map:** isolated integrated acceptance passed 2026-10-10 on source 1a411c6; main integration remains pending.

## Post-roadmap user-directed work

- **2026-10-11 — Escape home border transparency:** disabled drawing the home panel brush so its themed outline cannot remain visible over gameplay. Option hover/focus and high-contrast styling remain intact; lightweight checks only, full verification deferred.
- **2026-10-09 — Inventory simplification:** replaced the pack/toolbelt presentation with one 10×4 grid and occupied-cell hotbar. The editor build passes after the follow-up repair; runtime, rendered, input, and peer acceptance remain deferred. See [shared player inventory](docs/02-technical-architecture.md#shared-inventory-and-hotbar).
- **2026-10-09 — Project-file recovery and build repairs:** regenerated missing project files, validated solution references, and repaired the inventory editor build. See [build repairs](docs/07-development-setup.md#project-file-recovery).
- **2026-10-10 — Win64 package launcher:** added a Development/Shipping package-and-run script with fresh mirrors, profiles, and startup checks. See [Windows package smoke](docs/07-development-setup.md).

## M13 — Rebuild and revalidate M11/M12 UI on main

**Complete on main:** merge `e16b2f7` passed final acceptance on 2026-10-11. [Scope and criteria](docs/04-roadmap.md#m13--rebuild-and-revalidate-the-m11m12-ui-on-main) remain authoritative; unselected Settings candidates are not implemented features.

- [x] Confirm custody, lookup mappings and cooking for all 48 catalogue and nine status icons.
- [x] Revalidate the shared theme, accessibility, focus/input, local browsing, Favorites/ranks/Recent, notifications, recipe activity and Forge comparison.
- [x] Restore the 10×4 Inventory/crafting companion, contextual details, numbered hotbar and server-owned support-scroll behavior.
- [x] Reconcile HUD/service contexts, hammer Build browsing/placement and direct server-validated Campfire lighting.
- [x] Apply the minimap biome label, transparent Escape home and grouped local Settings presentation.
- [x] Verify complete icon rendering in editor peers and the Development package.
- [x] Pass the final editor build, 122/122 automation tests, rendered host/client matrix, input/privacy/reconnect checks and Development/Shipping smoke.

[Acceptance evidence](docs/evidence/m13/README.md) records exact commands, tested source, captures and scope limits. No M13 increment remains. The later Esc-border change has its own post-roadmap verification record above.

## M14 — Expanded world-map rework

**Complete on isolated integrated source:** `1a411c6d2b76b3d9b4bd438b8449632fd5bac1f4`, accepted 2026-10-10. **Main integration is pending**; this result does not claim the reworked map is present on main. [Scope and criteria](docs/04-roadmap.md#m14--expanded-world-map-rework) remain authoritative.

- [x] Render distinct original biome textures across supported zoom levels.
- [x] Preserve owner exploration fog without revealing hidden terrain through view changes or markers.
- [x] Provide owner-local marker selection and optional labels, including icon-only pins.
- [x] Relay server-validated right-click world pings globally for ten seconds, without saving them.
- [x] Remove map co-op controls/presence; keep map sharing and shared pins out of scope.
- [x] Cache bounded terrain/fog tiles across pan/zoom and reject stale async work.
- [x] Pass final editor build, 123/123 automation tests, four live map runners, viewport/scale/contrast/zoom/theme matrix and rendered/performance checks.

The accepted evidence includes 22 native PNGs, biome raster, exact commands, hashes and performance measurements. Recover the report and manifest from commit `9f11e09` with `git show 9f11e09:docs/evidence/m14/README.md` and `git show 9f11e09:docs/evidence/m14/acceptance.json`. Detailed verification and superseded retry history remain in [PROGRESS.md](PROGRESS.md) and Git history.

No isolated M14 implementation increment remains. The remaining handoff is integration of the accepted M14 source into main with applicable integration verification; this documentation cleanup does not perform that work. Map sharing remains deferred.
