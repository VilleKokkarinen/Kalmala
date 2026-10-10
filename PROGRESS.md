# Autonomous development progress

## Current state

- M0–M12 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. M13 is active; its goal is to re-establish and freshly accept the combined M11/M12 UI on main.
- The main tree already tracks 48 catalogue icon packages and 9 status icon packages, along with validated source/prepared images. Import commits 2d83c90 and d96d532 are ancestors of main. This proves the files are integrated; M13 still needs to verify runtime loading, cooking, and rendered visibility.
- The inventory screenshot showed the legacy vector fallback. The lookup constructs texture paths dynamically, so the cooker did not have a declared reference to include those icon packages. `DefaultGame.ini` now always cooks both catalogue and status icon directories.
- The 2026-10-09 user-directed 10×4 inventory is implemented and the UE5.8.2 editor target builds after the recorded repair. Runtime, rendered, input, and peer acceptance for that layout remains deferred; see [inventory handoff](docs/47-inventory-grid.md).
- M13 now also includes a paired inventory/crafting modal, contextual item detail, compact armor/weight summary, and one continuous wood-textured backplate behind opaque solid panes. The tooltip will show only existing catalogue/tool values; item combat-stat data is not currently defined.
- The latest M13 request adds a right-aligned current-biome label above the top-right minimap and a transparent main Escape menu with visible hover/focus highlighting; both are queued as presentation work.
- M13 now also includes reusable support-scroll items in numbered inventory hotbar cells. They use the existing learned-effect entitlement, share a five-minute cooldown, last three minutes where effects are sustained, and allow only one active effect per caster. Deer Call also lasts three minutes; Mending remains instant. Scrolls are reconstructed from learned progression without new save fields, and server authority remains in force.
- M13 now includes a Build-menu refresh: right-click entry with the hammer active, icon-only cards with contextual details, icon-only top-row filters, click-to-enter placement mode, right-click favorites, transient search, and direct Campfire lighting by Interact. Existing build/fuel costs and server validation remain the contract; implementation is queued.
- M13 Settings scope now uses the six reference group names: Gameplay, Keyboard & Mouse, Controller, Graphics, Audio, and Accessibility. Existing local options are mapped to those groups. `docs/14-settings-and-accessibility.md` contains the unchecked screenshot-derived option checklist; only candidates the user checks enter implementation scope. It describes optional per-action keyboard/mouse capture and a candidate previous/next controller-layout selector with synchronized diagram.
- A Win64 Development package launched and loaded the prototype map on 2026-10-10. This confirms startup and map loading; rendered icon visibility and the remaining M13 UI acceptance are still pending.

## Completed work log

- **M0–M12 and world-generation Phases 1–9:** completed under their recorded acceptance scope; milestone evidence remains linked from [the docs index](docs/README.md).
- **M13 baseline audit — 2026-10-10:** confirmed main-branch icon source/package custody and validated all manifest entries. M13 implementation and final acceptance remain open.

## 2026-10-10 — Add reusable support-scroll hotbar integration to M13

- **Completed:** added the requested support-scroll inventory/hotbar integration to M13. Existing learned support effects become reusable inventory items, activatable from numbered hotbar cells; scroll items are reconstructed from saved learned-effect entitlement, and activation retains the server-owned shared five-second cooldown and current effect validation. The previous F1–F4/Q effect-selection path is retired by this scope.
- **Files changed:** `BACKLOG.md`, `docs/02-technical-architecture.md`, `docs/04-roadmap.md`, `docs/11-combat-and-support-magic.md`, `docs/47-inventory-grid.md`, and `PROGRESS.md`.
- **Lightweight checks:** reviewed current support-effect, discovery, inventory, hotbar, and persistence contracts; `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to M13 milestone-final verification; this scope update changed documentation only. No runtime, build, automation, or rendered check ran.
- **Impact and authority:** planning only; the requested behavior is now an M13 requirement. Future hotbar use must resolve the owned cell and effect on the server and preserve learned entitlement, cooldown, costs, target checks, and save schema.
- **Limits:** reusable scroll items and hotbar activation are not implemented or rendered-verified. Existing inventory remains pawn-lifetime; scroll reconstruction from learned progression is part of the queued integration.
- **Next eligible task:** finish the active inventory/HUD/service revalidation, including the support-scroll hotbar integration, then proceed to the minimap and Escape-menu refinements in backlog order.

## 2026-10-10 — Set support-magic timing for M13 scrolls

- **Completed:** updated the M13 support-scroll contract to use one shared, server-owned five-minute cooldown per caster. Hearth Shield, Bear's Vigor, and Deer Call last three minutes; Mending remains an instant heal. Deer Call's bounded server-selected deer influence lasts three minutes, and only one support effect may be active per caster at once.
- **Files changed:** `BACKLOG.md`, `PROGRESS.md`, `docs/02-technical-architecture.md`, `docs/04-roadmap.md`, `docs/11-combat-and-support-magic.md`, and `docs/47-inventory-grid.md`.
- **Lightweight checks:** reviewed existing support effect timing and target contracts; `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to M13 milestone-final verification; documentation only, no runtime/build/automation/rendered check ran.
- **Impact and authority:** M13 now allows only these requested timing and exclusivity changes; activation, effect, target, and cooldown gates remain server-owned. No saved-data schema changes.
- **Limits:** implementation and rendered behavior are still queued. Scroll cooldown, effect timers, Deer Call duration, and mutual exclusion are not runtime-verified.
- **Next eligible task:** continue the active inventory/HUD/service revalidation by implementing and verifying support-scroll hotbar use with these timing rules.

## 2026-10-10 — Add Build-menu refresh to M13

- **Completed:** added the requested hammer Build-menu and Campfire interaction changes to M13 scope. The screenshot is recorded as a layout reference; implementation should use existing Kalmala icon and UI assets.
- **Files changed:** `BACKLOG.md`, `PROGRESS.md`, `docs/04-roadmap.md`, `docs/10-campfire-and-crafting.md`, and `docs/38-menu-browsing.md`.
- **Lightweight checks:** reviewed the existing Build browsing, Favorite, search-memory, placement, Campfire fuel, and lighting contracts; confirmed this workspace is the only active Kalmala run. `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to M13 milestone-final verification; this scope update changes documentation only. No runtime, build, automation, or rendered check ran.
- **Impact and authority:** planning only. Build selection continues into the existing local placement mode and server-validated placement path. The direct Light Campfire prompt reuses the existing payload-free server lighting intent and preserves target, access, distance, fuel, and fuel-condition checks. No saved-data or authority change is planned.
- **Limits:** the redesigned Build menu, right-click favorite/open routing, transient search, direct Campfire lighting prompt, and rendered behavior are not implemented or verified.
- **Next eligible task:** implement and verify support-scroll hotbar use under the active inventory/HUD/service revalidation; the Build-menu refresh follows in backlog order.

## 2026-10-10 — Add Settings visual refresh to M13

- **Completed:** added the requested Settings/options presentation refresh to M13. The screenshots are visual references only; Kalmala keeps its existing option tabs and content.
- **Files changed:** `BACKLOG.md`, `PROGRESS.md`, `docs/04-roadmap.md`, `docs/14-settings-and-accessibility.md`, and `docs/35-ui-theme.md`.
- **Lightweight checks:** reviewed the existing local settings, theme image keys, shared button states, high-contrast precedence, and M13 Escape-home-menu contract. `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to M13 milestone-final verification; this scope update changes documentation only. No runtime, build, automation, or rendered check ran.
- **Impact and authority:** planning only. Existing settings remain local and keep their current values, persistence, actions, and modal/input ownership; no replicated setting or save-schema change is introduced.
- **Limits:** the shared wood backplate and theme-button appearance across Settings/options tabs are not implemented or rendered-verified.
- **Next eligible task:** implement and verify support-scroll hotbar use under the active inventory/HUD/service revalidation; Build-menu and Settings visual refreshes remain later in backlog order.

## 2026-10-10 — Add selectable Settings options to M13

- **Completed:** mapped the six reference groups and added unchecked option candidates under Gameplay, Keyboard & Mouse, Controller, Graphics, Audio, and Accessibility. Documented the optional per-action keyboard/mouse rebinding flow and controller layout previous/next selector with synchronized diagram. Marked Reset raven hints, PlayFab account deletion, and Radial Menu as actions outside this settings checklist.
- **Files changed:** `BACKLOG.md`, `PROGRESS.md`, `docs/04-roadmap.md`, `docs/14-settings-and-accessibility.md`, and `docs/35-ui-theme.md`.
- **Lightweight checks:** inspected the updated requirements and scoped screenshot-only options against current Kalmala settings; `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to M13 milestone-final verification; documentation only, with no runtime, build, automation, or rendered checks.
- **Impact and authority:** planning only. Existing settings remain local. Candidate options are not implementation scope until the user checks them; no runtime, replicated state, or saved-data changes were made.
- **Limits:** option candidates need user selection, and selected candidates need implementation/runtime verification. The Settings visual refresh and content reorganization are not implemented.
- **Next eligible task:** complete support-scroll hotbar integration in the active inventory/HUD/service revalidation, then proceed through the queued M13 tasks in backlog order.

## 2026-10-10 — M13 shared foundation revalidation and inventory craft request

- **Completed:** revalidated the first M13 shared-foundation child against the settings/accessibility, theme, and input contracts; no defect was found in the M11 shared foundation. Added the requested right-side inventory crafting view and directional opening transitions to M13 scope and the `docs/47` contract.
- **Files changed:** `BACKLOG.md`, `docs/04-roadmap.md`, `docs/47-inventory-grid.md`, and `PROGRESS.md`.
- **Lightweight checks:** `Verify-SettingsAccessibilityContract.ps1`, `Verify-LocalInputContract.ps1`, and `Verify-PresentationOwnership.ps1` passed. `Verify-MenuInputCopy.ps1` did not pass: it still requires the removed `Previous item` / `Next item` controls from the prior inventory layout. This M12 audit repair remains for the queued inventory revalidation. `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to M13 milestone-final verification; no build, automation suite, rendered UI, physical input, or package check ran.
- **Impact and authority:** M13 now requires Inventory to open with the standard crafting list/details on the right; inventory enters from above and crafting from the right, following reduced-motion and existing owner-local modal behavior. This run changed documentation/backlog only; runtime behavior and server authority are unchanged.
- **Limits:** the requested companion panels are not implemented yet. Icon cooking/rendering, complete M11/M12 revalidation, peer acceptance, and the stale M12 menu-copy audit remain open.
- **Next eligible task:** restore and verify local menu browsing, Favorites/usage ranks/Recent indicators, notifications, recipe activity, and Forge comparison against docs/37–41 and docs/46.

## 2026-10-10 — M13 inventory presentation refinement and M11 menu audit

- **Completed:** refined the M13 inventory companion contract with the compact `Armor 0 · Weight 8/300` line, transient item details on hover or keyboard/controller focus, wood-panel backgrounds, and rounded inset dark-brown cells. Rechecked the M11 local menu browsing, Favorites/ranks/Recent, notification, recipe activity, and Forge comparison source against docs/37–41 and docs/46; no implementation defect was found.
- **Files changed:** `BACKLOG.md`, `PROGRESS.md`, `docs/04-roadmap.md`, and `docs/47-inventory-grid.md`.
- **Lightweight checks:** `Verify-PresentationOwnership.ps1`, `Verify-M5DocumentationContracts.ps1`, and `Verify-LocalInputContract.ps1` passed. Source review confirmed local-player browsing/activity state, owner-local notices, and the existing Forge upgrade comparison. `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to the M13 milestone-final gate; no build, full automation suite, rendered capture, physical input, or package smoke ran.
- **Impact and authority:** M13 scope now specifies the requested compact inventory, contextual tooltip, and panel/cell styling. No runtime, gameplay, network, or save behavior changed; existing crafting and item actions remain server validated.
- **Limits:** the requested UI is not implemented. The item catalogue has no Pierce/Poison/Knockback fields, so M13 will display existing data only. Rendered M11/M12 checks and the stale M12 `Verify-MenuInputCopy.ps1` expectations remain open for the later inventory task/final acceptance.
- **Next eligible task:** re-establish the current 10×4 inventory, numbered hotbar, owner-local actions, and modal/input behavior, including the M13 companion crafting view and presentation refinements.

## 2026-10-10 — M13 inventory and crafting companion implementation

- **Completed:** added the standard item recipe list/details beside the owner inventory; the Inventory pane and Crafting pane open together with the existing options easing, Inventory moving down from above and Crafting moving left from the right. A single Inventory texture backs both panes; each pane and each inventory/recipe cell is solid and opaque. Reduced motion skips the transitions. Inventory now presents the compact armor/weight summary, hides details until hover/focus/selection, and omits the previous persistent navigation/state clutter.
- **Files changed:** `BACKLOG.md`, `Scripts/Verify-MenuInputCopy.ps1`, `Scripts/Verify-PresentationOwnership.ps1`, `Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp`, `Source/KalmalaUI/Private/KalmalaInventoryGridWidget.cpp`, `Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp`, `Source/KalmalaUI/Private/KalmalaItemDetailWidget.cpp`, `Source/KalmalaUI/Private/KalmalaUITheme.cpp`, `Source/KalmalaUI/Private/Tests/KalmalaInventoryMenuFoodTest.cpp`, `Source/KalmalaUI/Private/Tests/KalmalaInventoryMenuSelectionTest.cpp`, `Source/KalmalaUI/Public/KalmalaCraftingSubsystem.h`, `Source/KalmalaUI/Public/KalmalaInventoryGridWidget.h`, `Source/KalmalaUI/Public/KalmalaInventoryMenuWidget.h`, `Source/KalmalaUI/Public/KalmalaUITheme.h`, `docs/04-roadmap.md`, `docs/35-ui-theme.md`, and `docs/47-inventory-grid.md`.
- **Lightweight checks:** `Verify-MenuInputCopy.ps1`, `Verify-LocalInputContract.ps1`, `Verify-PresentationOwnership.ps1`, `Verify-M5DocumentationContracts.ps1`, and `Verify-SettingsAccessibilityContract.ps1` passed. `git diff --check` and the 260-character path audit passed. No project build or automation suite was run.
- **Full verification:** deferred to M13 milestone-final verification; no runtime/rendered, input-device, peer, package, or build validation ran for this increment.
- **Impact and authority:** local inventory opening now also displays the recipe companion. Craft requests still use the existing crafting component/server validation; no recipe, gameplay stat, replication, or saved-data schema was added or changed.
- **Limits:** rendered layout and modal interaction have not been verified in Unreal. Existing catalogue/tool details are used; new combat stat fields were not introduced. The Crafting companion uses the existing non-build recipe catalogue, with station-required recipes showing their existing requirement/availability context.
- **Next eligible task:** reconcile inventory details, construction/build, station menus, recipe copy, the retired persistent panel/tutorial card, and the active-only status group against docs/35–45.

## 2026-10-10 — Add M13 minimap and Escape-menu refinements

- **Completed:** added the requested top-right minimap biome label and transparent main Escape home-menu treatment to the M13 scope and contracts. The label is right-aligned above the minimap, which moves down slightly; it uses the existing current-position biome classification. The home menu keeps its current options/actions and gains visible hover/focus highlighting over gameplay; individual Options tabs retain their documented backgrounds.
- **Files changed:** `BACKLOG.md`, `PROGRESS.md`, `docs/04-roadmap.md`, `docs/08-world-generation-and-biomes.md`, `docs/14-settings-and-accessibility.md`, and `docs/35-ui-theme.md`.
- **Lightweight checks:** reviewed the existing minimap, theme, accessibility, and M13 roadmap contracts; `git diff --check` and the 260-character path audit passed.
- **Full verification:** deferred to the M13 final gate. No runtime code or rendered UI was changed or verified in this scope update.
- **Impact and authority:** planning/documentation only. The biome label is specified to read existing local position/world classification; no new replicated state or gameplay authority change is requested. The Escape-menu refinement is local presentation only.
- **Limits:** both visual refinements remain queued for implementation and rendered validation. Existing M11 menu/background implementation is still present until that M13 work is done.
- **Next eligible task:** finish the active inventory/HUD/service revalidation, then implement the queued minimap label and transparent Escape home-menu refinements.

## 2026-10-10 — Include dynamic icon assets in package cooks

- **Completed:** configured game cooks to include the dynamically loaded item and status texture directories; documented why this is required.
- **Files changed:** Config/DefaultGame.ini, docs/36-status-icons.md, docs/43-catalogue-icon-manifest.md, BACKLOG.md, PROGRESS.md.
- **Lightweight checks:** confirmed both package paths exist and contain the tracked assets; `git diff --check` passed.
- **Full verification:** deferred to M13 final verification. A later Development package launch loaded the prototype map, but it did not verify rendered icon visibility or complete UI acceptance.
- **Impact and authority:** packaged UI lookups can find the catalogue and status textures; gameplay and server authority are unchanged.
- **Limits:** cooked asset presence and visible rendering still need packaged smoke verification; the inventory screenshot symptom is not claimed visually fixed until then.
- **Next eligible task:** revalidate and repair the M11 shared theme, accessibility, common menu, and local browsing foundation.

## 2026-10-10 — Add Win64 package-and-run script

- **Completed:** added `Scripts/Package-And-Run.ps1` for fresh-mirror Development and Shipping packages, visible launch, and startup checks.
- **Files changed:** Scripts/Package-And-Run.ps1, docs/07-development-setup.md, BACKLOG.md, and PROGRESS.md.
- **Lightweight checks:** PowerShell parser and Dev/Prod `-WhatIf` checks; `git diff --check`.
- **Full verification:** packaging was not run during this script change; full build and M13 validation remain deferred.
- **Impact and authority:** creates external Win64 package outputs and launches the selected build; gameplay and server authority are unchanged.
- **Limits:** a Shipping process check does not replace rendered gameplay validation. M13 still needs runtime icon, input, peer, and full milestone acceptance.
- **Next eligible task:** revalidate and repair the M11 shared theme, accessibility, common menu, and local browsing foundation.

## 2026-10-10 — Add M13 and audit main-branch icon custody

- **Completed:** added M13 to the roadmap and backlog; confirmed the icon import commits and assets are present in main.
- **Files changed:** docs/04-roadmap.md, BACKLOG.md, and PROGRESS.md.
- **Lightweight checks:** all 48 catalogue IDs passed Validate-CatalogueIcon.ps1; all 9 status IDs passed Validate-StatusIconSet.ps1; both icon-import commits are ancestors of main; git diff --check passed.
- **Full verification:** deferred. No build, automation suite, cook, package, or rendered UI check ran.
- **Impact and authority:** planning and documentation only; no runtime, gameplay, networking, persistence, or server-authority behavior changed.
- **Limits:** tracked assets and valid source files do not prove cooked or visible runtime output. M13 requires fresh checks on the final integrated main tree.
- **Next eligible task:** revalidate and repair the M11 shared theme, accessibility, common menu, and local browsing foundation.
