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
    - [x] Add the standard player crafting list/details beside Inventory; animate Inventory from above and crafting from the right. Keep only compact armor/weight at rest and show tidy item details on hover/focus. Use one continuous wood-textured backing behind both panes, with each pane solid and opaque plus rounded, inset dark-brown cells/cards. Respect reduced motion and the owner-local modal/input lifecycle; use existing item/tool stats only. (2026-10-10; shared backplate, opaque panes, contextual details, and paired opening transitions implemented; lightweight input, presentation, docs, diff, and path checks pass. Full verification deferred to M13 final gate.)
    - [ ] Represent the existing support effects as reusable inventory scrolls assignable to numbered hotbar cells and usable from there. Rebuild scroll items from learned-effect entitlements; each accepted cast starts a shared server-owned five-minute cooldown, and timed effects last three minutes with at most one active per caster. Mending remains instantaneous; Deer Call's bounded influence lasts three minutes. Retire F1–F4/Q selection without changing costs, magnitudes, targets, or save fields. (User request, 2026-10-10; contracts in docs/04, docs/11, and docs/47.)
  - [ ] Reconcile inventory details, construction/build, station menus, recipe copy, retired persistent panel/tutorial card, and active-only status group against docs/35–45.
    - [ ] Rework Build as specified in the M13 roadmap and docs/38: open from right-click with the hammer active; icon-only build cards with hover/focus details below; icon-only top-row filter/group controls and no visible header text; left-click/activation starts placement; right-click toggles Favorite; clear search on close or placement entry without focusing it on open. Remove Add to favorites, Build / place selected, and Light hearth actions; light an eligible unlit Campfire through direct Interact while preserving server validation and existing costs. (User request, 2026-10-10; scope only, implementation pending.)
- [ ] Apply the M13 minimap, Escape home-menu, and Settings presentation refinements.
  - [ ] Put a right-aligned current-biome label above the top-right minimap and move the minimap slightly down to fit it. Use the existing current-position biome classification; preserve status-group spacing and supported viewport/text-scale layouts without new biome-map or replication state. (User request, 2026-10-10; contract in docs/04 and docs/08.)
  - [ ] Make the main Escape home menu fully transparent over gameplay and show only its existing options list with a visible hovered/focused option highlight. Preserve high-contrast focus cues, reduced-motion behavior, existing actions, and modal input ownership; retain per-tab backgrounds. (User request, 2026-10-10; contract in docs/04, docs/14, and docs/35.)
  - [ ] Rework Settings using the six reference groups and the same original wood texture as Inventory, with shared-theme button states. Retain and regroup existing local options. Candidate checkboxes in docs/14 determine which new options are implemented; preserve high-contrast override, local persistence, and modal/input behavior. (User request, 2026-10-10; implementation pending; contracts in docs/04, docs/14, and docs/35.)
    - **Gameplay candidates:** Language selector; Auto-run; Attack towards look direction; Show button hints; Enable game hints; Reduce background performance; Enable Console; Show build piece author; Skip intro cinematic; Auto-backup history slider. Add the game-hints and intro-skip settings but keep their controls hidden for now, as marked in docs/14.
    - **Keyboard & Mouse candidates:** Mouse sensitivity slider, 1%–1000%; Invert mouse; direct per-action rebind capture with Escape cancel and Reset controls. Rebind rows: Attack, Secondary Attack, Block, Use (Action), Jump, Run, Crouch, Dodge, Alternative Dodge, Holster Weapon, Hotbar 1–0, Move Forward/Backward/Left/Right, Inventory, Alternative Inventory, Map, Map Zoom In/Out, Deconstruct while holding the hammer, Alternative placement while holding the hammer (normal placement uses Attack), and Previous/Next snap point in build mode.
    - **Controller candidates:** Gamepad enabled; Swap triggers; Invert camera X/Y; Vibration strength slider; Controller sensitivity slider; Glyphs selector for Xbox, PlayStation, and Switch. Use the same rebindable action rows listed under Keyboard & Mouse.
    - **Graphics candidates:** Resolution selector; Full Screen toggle; 3D resolution limit selector; Upscaling method selector; Framerate limit slider including Unlimited; Graphics preset selector; Vegetation quality, Level of detail, Particle lights, Shadow quality, Active point lights, Active point light shadows, SSAO, Cloth Quality, and Draw distance sliders; Distant shadows, Tessellation, Bloom, Depth of field, Motion blur, Chromatic aberration, Sun shafts, Soft particles, and Anti-aliasing toggles.
    - **Audio candidates:** Replace the existing 25%-step controls with labeled sliders for master, ambient, music, and interaction/combat-feedback levels; add an Environmental volume slider; Continuous music toggle.
    - **Accessibility candidates:** Immersive camera; Camera shake slider from 0% to 100%; Reduce flashing lights; Toggle block.
    - Keep current Graphics settings (resolution, V-Sync, window mode, render-distance quality), Audio settings, Controls mappings/default reset, and Accessibility settings (text/interface scale, contrast, feedback preference, reduced motion). Do not add a Controller layout selector; it was removed from the latest docs/14 candidate list.
- [ ] Verify icon assets from source through packaged, rendered use on main.
  - [ ] Confirm all 48 catalogue and 9 status images load and visibly render in their supported consumers; restore or reimport only a demonstrated gap, and ensure any repair is committed in the integrated main tree.
- [ ] Complete M13 milestone-final verification.
  - [ ] Run the prescribed build, relevant automation/UI/input/authority/reconnect checks, rendered host/client matrix, and packaged icon smoke check against the final integrated tree; repair M13 defects, rerun affected checks, and retain fresh evidence before closing the milestone.

The next task is to finish the current inventory/HUD/service revalidation, including the support-scroll hotbar integration, then reconcile the queued Build-menu refresh and proceed to the M13 minimap, transparent Escape home-menu, and Settings visual refinements. Dynamic item and status icon directories are included in package cooks; cooked loading and rendered visibility remain to be verified in M13.

## M14 — Expanded world-map rework

M14 starts after M13 milestone-final verification. Its scope and acceptance criteria are in [the roadmap](docs/04-roadmap.md).

- [ ] Rework the expanded map presentation and interactions.
  - [ ] Replace flat biome colours with distinct original per-biome textures that stay recognizable at every supported zoom level.
  - [ ] Restore fog over undiscovered terrain using the owner's existing exploration coverage; pan, zoom, pins, and pings must not reveal hidden map content.
  - [ ] Add selectable bottom-right personal-marker icons. Select an icon and click the map to place it; allow an optional text label and keep the pin icon-only when the label is blank. Keep pins owner-local.
  - [ ] Replace the ping control box with direct right-click map pings at any valid world location, without distance, discovery, or biome restrictions. Server-validate and relay a world beacon to all connected players; show it at the clicked position for ten seconds, then expire it without saving.
  - [ ] Remove map-screen co-op text, player-presence entries, visibility/share toggles, and other co-op controls. Do not implement map sharing or shared pins in M14; preserve the requested temporary global ping.
  - [ ] Keep pan/zoom responsive by caching bounded terrain/biome and fog tiles independently of view-only zoom/pan changes; avoid rebuilding all visible data on every input and reject stale async results.
- [ ] Complete M14 milestone-final verification.
  - [ ] Verify fog/privacy, pin ownership and labels, host/client ping visibility and ten-second expiry, zoom-stable biome textures, input/focus, and responsive pan/zoom on the integrated tree; retain rendered and performance evidence.

M13 remains the earliest incomplete milestone. M14's map-sharing scope is explicitly deferred; no co-op map text or sharing controls are planned for this map view.
