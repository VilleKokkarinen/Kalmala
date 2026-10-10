# Autonomous development backlog

This is the short execution queue and completion index. The roadmap and feature contracts in docs/ remain authoritative for scope and acceptance criteria.

## Roadmap status

M0–M12 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. No unchecked roadmap increment remains.

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

## Post-roadmap user-directed work

- **2026-10-09 — Inventory simplification:** replaced the pack/toolbelt presentation with one 10×4 grid and occupied-cell hotbar. The editor build passes after the follow-up repair; runtime, rendered, input, and peer acceptance remain deferred. See [shared player inventory](docs/47-inventory-grid.md).
- **2026-10-09 — Project-file recovery and build repairs:** regenerated missing project files, validated solution references, and repaired the inventory editor build. See [build repairs](docs/42-build-repair.md).

No roadmap task is currently eligible. New implementation work requires a new direction or backlog entry.
