# Autonomous development backlog

This is the short execution queue and completion index. The roadmap and feature contracts in docs/ remain authoritative for scope and acceptance criteria.

## Roadmap status

M0–M12 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. M13 is now the earliest incomplete milestone.

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
- **2026-10-10 — Win64 package launcher:** added a Development/Shipping package-and-run script with fresh mirrors, profiles, and startup checks. See [Windows package smoke](docs/07-development-setup.md).

## M13 — Rebuild and revalidate M11/M12 UI on main

The M13 scope and acceptance criteria are in [the roadmap](docs/04-roadmap.md). Keep work on the current main integration and verify the final integrated tree.

- [x] Confirm main-branch icon asset custody and static lookup mappings.
  - [x] Compare the canonical manifests, source/prepared PNGs, imported packages, and code lookup paths with the main tree. (2026-10-10; all 48 catalogue IDs passed Validate-CatalogueIcon.ps1, all 9 status IDs passed Validate-StatusIconSet.ps1, both icon-import commits are ancestors of main, and all 57 imported packages are tracked.)
- [x] Include dynamically loaded icon assets in game package cooks.
  - [x] Always cook the item and status icon directories because their lookup paths are assembled at runtime. (2026-10-10; added both package directories and documented the cooker requirement.)
- [x] Rebuild and revalidate the M11 shared UI foundation. (2026-10-10; both children received source/contract revalidation on the integrated tree; rendered host/client verification remains in the M13 final gate.)
  - [x] Reconcile shared theme, settings/accessibility, common menu styling, focus, and input behavior against docs/14 and docs/35. (2026-10-10; settings/accessibility contract, local input baseline, and presentation ownership audits pass; no defect found in the M11 shared foundation.)
  - [x] Restore and verify local menu browsing, Favorites/usage ranks/Recent indicators, notifications, recipe activity, and Forge comparison against docs/37–41 and docs/46. (2026-10-10; current source matched the documented local-owner contracts; presentation-ownership and M5 documentation audits pass. Rendered M13 acceptance remains deferred.)
- [ ] Rebuild and revalidate the M12 inventory, HUD, and service contexts.
  - [ ] Re-establish the current 10×4 inventory, numbered hotbar, owner-local actions, and modal/input behavior from docs/47.
    - [ ] Add the standard player crafting list/details beside Inventory; animate Inventory from above and crafting from the right. Keep only compact armor/weight at rest, show tidy item details on hover/focus, and use wood panels with rounded, inset dark-brown cells. Respect reduced motion and the owner-local modal/input lifecycle; use existing item/tool stats only. (User request, 2026-10-10; contract in docs/47.)
  - [ ] Reconcile inventory details, construction/build, station menus, recipe copy, retired persistent panel/tutorial card, and active-only status group against docs/35–45.
- [ ] Verify icon assets from source through packaged, rendered use on main.
  - [ ] Confirm all 48 catalogue and 9 status images load and visibly render in their supported consumers; restore or reimport only a demonstrated gap, and ensure any repair is committed in the integrated main tree.
- [ ] Complete M13 milestone-final verification.
  - [ ] Run the prescribed build, relevant automation/UI/input/authority/reconnect checks, rendered host/client matrix, and packaged icon smoke check against the final integrated tree; repair M13 defects, rerun affected checks, and retain fresh evidence before closing the milestone.

The next task is to re-establish the current 10×4 inventory, numbered hotbar, owner-local actions, and modal/input behavior, including the M13 companion crafting view and presentation refinements. Dynamic item and status icon directories are included in package cooks; cooked loading and rendered visibility remain to be verified in M13.
