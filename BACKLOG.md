# Autonomous development backlog

Documentation layout: former docs 08–47 are consolidated into the core docs (00–07). See [Game features](docs/game-features.md), [architecture contracts](docs/02-technical-architecture.md#consolidated-system-contracts), and [verification index](docs/07-development-setup.md#consolidated-verification-index). This user-directed cleanup does not close any milestone.

This is the short execution queue and completion index. The roadmap and feature contracts in docs/ remain authoritative for scope and acceptance criteria.

## Roadmap status

M0–M14 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. M13 integrated-main acceptance passed on 2026-10-11; M14 integrated-main acceptance passed on 2026-10-12. See [M14 integration evidence](docs/evidence/m14/main-integration.md).

- [x] **2026-10-13 — Omit unused item damage channels:** remove zero-valued item DamageBonuses fields and default omissions to zero before strict loading. Lightweight checks passed; Unreal verification deferred.

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
- **M14 — Expanded world map:** isolated acceptance passed 2026-10-10 on source 1a411c6; integrated-main build, acceptance matrix, and final 123/123 automation queue passed 2026-10-12. Evidence is in docs/evidence/m14/.

## Post-roadmap user-directed work

- [x] **2026-10-13 — Station-only crafting/upgrade eligibility:** remove obsolete player-skill/unlock checks and requirement text, retain station/material/item constraints, and regress missing/uninitialized ledgers. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Omit default item skill levels:** remove MinimumSkillLevel from all equipment JSON and default it internally to 1. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Remove empty recipe skill requirements:** omit None/0 skill properties from all authored acquisition/upgrade JSON and default them in the loader. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Separate axe/pick resource families:** enforce family before material tier; regress all axe/pick tiers against wood/ore sources and high item levels. Lightweight checks passed; Unreal verification deferred.

- [ ] **2026-10-13 — Attack-driven tree harvesting:** axes swing at trees, fell them into 3D trunks, and break trunks into collectible wood.
  - [x] Integrate the server-authoritative tree/trunk/drop loop into existing wood-resource actors, preserving tiers, yields and sparse depletion; replicate stages and remove tree interaction harvesting.
  - [ ] Convert decorative generated forest trees into individually attackable sources and integrate their depletion/streaming policy.
  - [ ] Add a moving fall/log presentation and verify the integrated tree flow in Unreal and on peers.

- [x] **2026-10-13 — Remove redundant item categories:** derive weapon/tool classification from item family; remove ItemCategory authoring/runtime property while retaining Inventory grouping. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Condense equipment identity:** schema 12 profiles use ItemId and derive legacy set/kind/action/skill; keep authored stats and behavior. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Classify carried weapons:** mark knives/axes/maces/spears Weapon, retain pickaxes/hammer as Tool, and expose separate Inventory categories and detail labels. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Authored axe/pickaxe material tiers:** expose MaterialTier 1/2/3 in profiles and use it in server harvesting gates, independent of level. Lightweight validation passed; Unreal verification deferred.

- [x] **2026-10-13 — Rewards for all weapon families:** add shared knife/pickaxe/mace/spear rewards, preserving user base stats; allow omitted zero reward channels and durability. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Share attacks by weapon family:** move item Attacks arrays into tools/Attacks.json; axe/knife/pickaxe/spear/mace tiers each share one list with validated membership and unchanged Basic values.

- [x] **2026-10-13 — Share level rewards across items:** move rewards to tools/LevelUpRewards.json with explicit item groups; all axes share +5 slash/+50 durability per level, with validated references and zero default gains for unlisted items.

- [x] **2026-10-13 — Replace item Levels with LevelUpReward:** define base durability/damage plus additive per-level gains; derive stats through 20 and configured station upgrade recipes through 10, preserving existing level-one/two gains.

- [x] **2026-10-13 — Simplify durability costs:** remove DurabilityCost from every tool profile/runtime definition and use a shared server cost of one point per accepted durability-consuming use.

- [x] **2026-10-13 — Add mace tiers and icons:** Stone/Bronze/Iron maces use knife-tier durability and costs with crush bonuses, starter/station acquisition, inventory/detail/icon mappings and imported original transparent icons.

- [x] **2026-10-13 — Allow negative per-type resistance:** bounded negative reductions act as flat weaknesses on active damage channels; retain nonnegative damage/Armor and prevent absent channels from adding damage.

- [x] **2026-10-13 — Define typed weapon bonuses and entity defenses:** migrate scalar damage to slash/stab/crush/pierce/fire/water/holy/poison channels; add weapon attack-style bonuses and flat resistance/Armor definitions to production gameplay entities; resolve current player/wildlife hits on the server and display mixed weapon bonuses.

- [x] **2026-10-13 — Separate item level caps:** station upgrades stop at level 10; any-source item levels and authored stats stop at 20; validate higher authored item levels independently of station recipes. Existing station content retained.

- [x] **2026-10-13 — Simplify item upgrade definitions:** stable acquisition RecipeIds with Level; one JSON file for level-one upgrade costs; derive doubling costs and target station level; keep upgrade RequiredSkillLevel at zero and remove JSON predecessor fields. Current playable levels and existing ingredients retained.

- [x] **2026-10-13 — Finish CampFire identifier cleanup:** replace superseded catalogue, component, icon, manifest, fixture, and documentation names with `CampFire`; retain the internal `CampfireKit` construction identity.

- [x] **2026-10-13 — Split ordinary building and station recipes:** put construction/general building recipes in `recipes/building.json`, each station's recipe in its own JSON file, and preserve the existing runtime order through the manifest.

- [x] **2026-10-13 — Rename Stone starter tools in the UI:** display the starter knife and axe as Stone Knife and Stone Axe across inventory, crafting, repair labels, and item details; their former IDs were retained at this stage.

- [x] **2026-10-13 — Finish Bronze Axe level-two gains:** keep harvesting tier tied to item identity; set level 1 to 100 durability/50 slash damage and level 2 to 150/55, apply the slash values in server combat, and show the stat deltas in the upgrade comparison.
  - [x] Remove the named-item upgrade button text and the carried-level prerequisite sentence from the comparison header; stop Bronze Axe level from unlocking Iron harvesting.
  - [x] Apply and display the specified durability and slash-damage values at both levels.

- [x] **2026-10-13 — Fix duplicate progression format compile error:** use separate checked format literals in the shared progression contract, matching the server result-message fix.

- [x] **2026-10-13 — Fix crafting result format compile error:** use separate checked format literals for crafted and upgraded tool result messages so the Unreal `FString::Printf` overload compiles.

- [x] **2026-10-13 — Upgrade tools in place:** Forge advances a carried Bronze Axe from level 1 to level 2 under the same item ID, charges materials, preserves condition and inventory cell. Its original Iron woodcutting tier/Densewood gain was removed by the follow-up task above. Cross-item upgrades are rejected; legacy Iron Axe saves remain readable.

- [x] **2026-10-13 — Use the shared checkbox for Audio mute:** replace the Mute/Restore button with the shared stone checkbox, keeping the master-volume restore behavior and grouped slider layout.

- [x] **2026-10-13 — Set Graphics render distance in meters:** use 64–576 m values in 32 m increments and apply them as local primitive-component cull-distance caps while retaining shorter authored limits.

- [x] **2026-10-13 — Make Graphics render distance a slider:** replace the text-in-button cycle control with the shared snapped slider row and keep its selected value visible; the follow-up meter increment defines the current range and behavior.

- [x] **2026-10-13 — Restyle Graphics settings controls:** use the shared stone checkbox for V-Sync, shared dropdowns for resolution presets and window mode, and an optional bounded custom resolution input with separate width/height fields.

- [x] **2026-10-13 — Use a shared dropdown for Accessibility feedback:** separate the colour-independent feedback label from its selection control and use a reusable stone-styled dropdown with themed option rows.

- [x] **2026-10-13 — Retire station Repair All render targets:** remove `workbench.repair` and `forge.repair` from the PowerShell and native menu renderers; inspect the direct Repair All actions in Workbench Craft and Forge captures.

- [x] **2026-10-13 — Center Accessibility text and contrast controls:** center 100% text scale and Standard contrast by adding supported lower choices while retaining the existing higher scale and High contrast mode.

- [x] **2026-10-13 — Right-align and enlarge Workbench/Forge headers:** let the station icon/name group size naturally at the right of the header, use a 64×64 station icon, and keep the name and level right-aligned.

- [x] **2026-10-13 — Rename and split Settings controls:** shorten Controls/Controller action labels to Build, Inventory, Settings, and Map; default Settings to Escape; show controller movement as separate Forward/Backward/Right/Left rows.

- [x] **2026-10-13 — Vertically center Settings row text:** center shared slider labels/values and the Reduced Motion label alongside their controls; preserve the existing centered binding-row layout.

- [x] **2026-10-13 — Center Reduced Motion checkmark:** align the checkmark in the middle of the 44×44 checkbox content area on both axes.

- [x] **2026-10-13 — Enlarge and simplify Reduced Motion toggle:** increase the stone checkbox target to 44×44 and remove the separate On/Off text, leaving the checkmark to show its state.

- [x] **2026-10-12 — Use shared sliders and a reusable checkbox in Accessibility:** convert interface scale, text scale, and contrast to snapped shared-theme sliders; add the reusable stone checkbox style for Reduced Motion; retain local values and repair high-contrast slider thumb sizing.

- [x] **2026-10-12 — Make Workbench/Forge Repair All direct actions:** remove the per-tool station repair page; clicking either station's button repairs all damaged carried tools through server validation and atomic persistence.

- [x] **2026-10-12 — Match Workbench Repair and Forge Repair/Upgrade styling:** reuse the wood-backed station content area, place repair browsing beside item details, and frame Forge upgrade requirements and comparison together.

- [x] **2026-10-12 — Polish Workbench/Forge selection cues:** use gold text for the active station tab, remove selection carets, and align recipe names left beside their icons.

- [x] **2026-10-12 — Match Workbench and Forge menus to Inventory Craft:** use the shared split list/detail layout for both Craft tabs, keep station tabs and identity/status anchored at the top, frame Repair and Upgrade content, and expose render targets for every Workbench/Forge section.

- [x] **2026-10-12 — Simplify the Options divider ornament:** use the provided reference to generate four progressively smaller mirrored loops on each side, joined cleanly at center.

- [x] **2026-10-12 — Consolidate menu captures and polish the Options divider:** use the combined Inventory/player-crafting preview in the default render set, make `crafting` show Workbench Craft, and replace the rough divider art while removing its gold underlay.

- [x] **2026-10-12 — Give the Options divider a generated medieval ornament:** import a transparent knotwork separator image and layer it over the shared Options rule; suppress the image in high contrast.

- [x] **2026-10-12 — Add a shared divider below Options tabs:** add a full-width, high-contrast-aware divider between the category tabs and page content, preserving the 10-unit content gap.

- [x] **2026-10-12 — Tighten Options page content layout:** remove explanatory footer labels, add a 10-unit gap below the category tabs, and move Audio mute above a grouped volume-slider stack.

- [x] **2026-10-12 — Match Controller settings alignment to Controls:** present controller bindings in the same centered, balanced two-column layout with matching row spacing, while preserving controller mappings and paired stick axes.

- [x] **2026-10-12 — Remove duplicate headings from Settings pages:** use each selected Options tab as the page label on Controller, Graphics, Audio, and Accessibility, matching Controls.

- [x] **2026-10-12 — Render all Options selections together:** make `-Menu settings.options` produce separate previews for Controls, Controller, Graphics, Audio, and Accessibility.

- [x] **2026-10-12 — Rename the Keyboard & Mouse Settings tab:** display the input group under the concise “Controls” tab label.

- [x] **2026-10-12 — Fill Settings Options tab row:** make all five category tabs share the full panel content width equally with consistent horizontal gutters.

- [x] **2026-10-12 — Polish Settings tab selection and control alignment:** remove the selected-tab text marker and duplicate Keyboard & Mouse heading, highlight the active tab, and center the equal-width control columns with balanced row padding.

- [x] **2026-10-12 — Repair Keyboard & Mouse column compilation and button fill:** set Unreal fill weights through `FSlateChildSize::Value`, and stretch binding buttons to fill their row slots while preserving Controller alignment.

- [x] **2026-10-12 — Use two-column Keyboard & Mouse controls:** remove the row-cycling hint and split the bindings into two equal-width columns, keeping each label and value button aligned within its half of the panel.

- [x] **2026-10-12 — Align Settings control rows:** split keyboard movement into separate Forward, Backward, Right, and Left settings with independent key cycling; right-align labels beside uniform binding buttons on the right, and reduce vertical row padding.

- [x] **2026-10-12 — Unify square item-icon tiles:** add a shared 64×64 tile theme with a 4-unit gap between tiles and the requested translucent orange-brown fill; use it in Inventory, Build, item details, and crafting views, with crafting material quantities below the tile. Follow-up ingredient row slot typing now compiles and the inventory-crafting render succeeds.

- [x] **2026-10-12 — Fix Build square button sizing chain:** center each standalone Build tile in its UniformGrid cell and prevent shared selectable styling from reapplying content insets inside the 64×64 button.

- [x] **2026-10-12 — Square Build selection buttons:** constrain the themed card border itself to the 64×64 tile so the grid cannot stretch its background across a row.

- [x] **2026-10-12 — Match Build icon tiles to Inventory sizing:** use compact 64×64 selectable tiles and native 64×64 catalogue-image rendering instead of larger button-like cards and extra image scaling.

- [x] **2026-10-12 — Pack Build tiles like Inventory:** use ten left-aligned columns of the shared 64×64 item tiles with a 4-unit gap, keeping the existing four-column layout for crafting/station grids.

- [x] **2026-10-12 — Remove duplicate Settings Status render target:** exclude `settings.status` from `Render-Menu.ps1` because Settings Home already renders the same PNG.

- [x] **2026-10-12 — Refine crafting menu presentation:** add configurable list-row styling for item selection, align station sections at top-left and station identity/status at top-right, and frame requirement icons.
  - [x] Apply the shared list-view theme to crafting choices and the requested station/header and material-icon presentation; preserve existing selection and authority behavior.
- [x] **2026-10-12 — Correct crafting list content alignment:** stretch each list button's content slot so item icons and left-justified labels start at the row's left inset in Workbench, Forge, and Inventory Craft.
- [x] **2026-10-12 — Show station levels in Workbench/Forge headers:** replace attachment-state text with each station's effective level, using level 1 as its base and adding a level for a compatible upgrade.

- [x] **2026-10-12 — Streamline verification scripts:** reduce redundant or obsolete test entrypoints while retaining required acceptance coverage; distinguish test runners from Python support tools.
  - [x] Retire the superseded standalone integrated ocean skiff journey runner; the integrated reconnect runner already covers its journey/discovery/weather checks and adds late-join/restart coverage.
  - [x] Merge ocean weather and fixed-origin assertions into the focused two-peer journey runner; remove their separate launchers.
  - [x] Combine settings runtime and no-build contract verification under `Verify-SettingsAccessibility.ps1`, retaining a contract-only mode.
  - [x] Fold the persisted-camp build fixture into the restart runner and retire its standalone wrappers.
  - [x] Merge WorldMap viewport, tile identity, private-coverage restart, and profile checks into `Verify-WorldMap.ps1`; retain separate export and co-op-awareness checks.
  - [x] Move the no-build audio cue contract into `Verify-AmbientAudio.ps1 -ContractOnly` and combine movement/ocean cue assertions in one PlayerControls run.
  - [x] Fold the skiff owner-feedback host/owner scenario into a preliminary phase of `Verify-OceanSkiffIntegratedReconnectJourney.ps1` and retire its separate launcher.
  - [x] Fold the inventory HUD-removal and empty-menu preflight into `Verify-Inventory.ps1 -ContractOnly`; update its callers and retire the standalone audit launcher.
  - [x] Move the retained PowerShell verification runners into domain categories under `Scripts/Tests/`; update path resolution, internal callers, and active documentation references.
  - [x] Consolidate the optional procedural audio-source generators into one selectable tool; keep checked-in WAV and Unreal assets as game content and document that generation is not a runtime/build step.
    - [x] Consolidate catalogue and status icon validation into one parameterized maintenance script; document that imported icon packages are runtime content while their source-art tools are optional.
  - [x] Audit remaining `Verify-*` entrypoints against current acceptance requirements, callers, and equivalent coverage; retire or consolidate only confirmed duplicates. Retained the unique construction movement runner, documented it, and retired the skiff reconnect launcher whose checks are covered by the integrated journey/reconnect runner.
  - [x] Document the non-`.ps1` support tools, their runtime requirements, and why they remain in their native formats.

- [ ] **2026-10-13 — Add three theme-appropriate tiers of weapons and tools:** add original weapons and tools across the Stone, Bronze, and Iron tiers, with item-specific stats and matching recipes; preserve the rule that upgrades improve the same item and do not grant a different item's gathering tier.
  - [x] Establish the Stone starting set with per-item combat stats and a Stone Spear identity/icon; keep harvesting access tied to each existing item identity.
  - [x] Add Bronze Knife, Axe, Pick, and Spear stats and their matching acquisition recipes; retain Bronze Axe's same-item level upgrade.
  - [x] Add Iron Knife, Axe, Pick, and Spear stats and level-one Forge recipes; make the mire bog-iron vein an Iron source for Bronze-tier mining tools or better, and preserve independent item identity instead of Bronze-to-Iron upgrades.
  - [x] Add unique imported 64×64 canonical icons for the Bronze and Iron Knife, Pick, and Spear identities; preserve the existing Stone/Axe icons and extend canonical coverage.
  - [x] Move authored carried-tool profiles, per-level stats, and acquisition/upgrade recipe requirements and costs into schema-version-5 `GameCatalogues.json`; load them into the existing server-side tool contracts.
  - [x] Split the schema-version-5 `GameCatalogues` definitions into category JSON files with an ordered manifest; preserve catalogue contents, order, validation, and runtime behavior.
  - [x] Group weapon profiles under item-family categories and their recipes under matching recipe categories; keep only the Building Hammer profile in `tools/` and preserve the prior assembly order.
  - [x] Canonicalize the starter IDs to `StoneKnife` and `StoneAxe`, and use `CampFire` for the campfire recipe/output; preserve existing starter-tool saves and icon packages without changing save schema.
  - [ ] Surface all Stone, Bronze, and Iron tool acquisitions, authored stats, and material costs in station and Inventory Craft views, with crafting routed through the server-authoritative tool progression action.

- [x] **2026-10-11 — Fix Build card fill and visible icon scale:** prevent the shared selectable-fill animation from overwriting the standalone Build menu's brown card overlay, and compensate for transparent margins in the 64×64 catalogue canvases while preserving icon proportions. Source inspection and `git diff --check` only; build/render verification deferred.

- [x] **2026-10-11 — Clear previous Render-Menu PNGs:** before directory-mode renders, remove prior renderer-named PNGs from the selected output folder after renderer preflight; preserve unrelated files and exact single-file output behavior. Lightweight source inspection only; Unreal rendering not run.

- [x] **2026-10-11 — Refine standalone Build menu icons and requirements:** center the fixed 64×64 build icons, match Inventory's warm brown card treatment, and subtly gray cards/icons with unmet requirements. Remove all visible text below the requirements icons; keep quantities in icon tooltips and detailed conditions in the panel tooltip. Lightweight source checks only; build/render verification deferred.

- [x] **2026-10-11 — Remove Escape status/weather details page:** remove its home-menu button and live text screen; keep the Escape shell to Options and Quit and preserve the active status HUD. Align the status-hotbar capture flow and current UI documentation; build and rendered verification deferred.
- [x] **2026-10-11 — Add button padding and smooth the slider thumb:** increase shared button-content padding from 7 to 9 units and replace the slider thumb source with a cleaner, smoother charcoal slate puck. Reimported the existing texture asset; rendered menu verification deferred.
- [x] **2026-10-11 — Add button padding and soften sliders:** introduce shared button-content padding without changing panel padding; use a thinner rounded slider track without the standard-mode outline and finer keyboard/controller steps. High contrast keeps its outline; render verification deferred.

- [x] **2026-10-11 — Fix Inventory menu render slot assertion:** remove content sizing from `UKalmalaThemedButton::SynchronizeProperties`, where Slate can still be constructing the parent slot; size Inventory action buttons after insertion into their vertical box. A new Inventory render was not run; lightweight inspection only.

- [x] **2026-10-11 — Stone slider thumb and content-sized buttons:** generate and import a distinct irregular slate thumb; replace the Audio tab's cycling volume buttons with live, focusable 0–100% sliders; size buttons to their content throughout shared UI layouts. Audio values keep the existing local settings keys, and slider writes are coalesced. Full rendered/package verification deferred.

- [x] **2026-10-11 — Shared stone button texture:** generate one alpha-backed, irregular beveled stone plate and apply it through the shared button theme with nine-slice scaling and distinct interaction tints. Preserve the 2170×725 master while importing its 160×53 UI-scale derivative so short, wide controls keep intact ends. High contrast retains its geometric button and focus treatment; rendered/package verification deferred.

- [x] **2026-10-11 — Generated walnut Inventory background:** create a new warm-brown wood texture, replace the Inventory source image, and reimport the existing Unreal texture package. The shared background now appears on Inventory, Crafting and Settings option groups; an isolated editor build/import and one offscreen Inventory preview succeeded. Full automation/package verification remains deferred.

- [x] **2026-10-11 — Render all menus without an ID:** make `Scripts/Render-Menu.ps1` render each supported menu when `-Menu` is omitted, preserve single-menu selection, document PNG output paths, and support Windows PowerShell 5.1. Lightweight script checks only; no Unreal menu render was run.

- [x] **2026-10-11 — Inventory/crafting sizing and background asset repair:** fill the right-anchored crafting area, widen and frame its columns; provide/load/cook original panel images; remove Inventory armor/weight and excess grid/scrollbar space. Lightweight checks only; rebuilt/editor/package visual verification deferred.

- [x] **2026-10-11 — Wood brush and inventory crafting detail layout:** correct textured image type/tint for Inventory and Settings; compact search/x, left recipe list, right icon/title/description/stats and simple icon costs, disabled Craft action; hide legacy browse/status/navigation sections. Lightweight checks only; rebuilt/runtime verification deferred.

- [x] **2026-10-11 — Inventory/crafting layout and hammer recipe:** separate top-left/top-right wood panels, remove title/intro headings, close with Tab before focus navigation, limit player crafting to Building Hammer (3 Wood + 1 Stone), preserve cooking-station scope. Lightweight checks only; full build, runtime/rendered/input/peer verification deferred.

- [x] **2026-10-11 — Standalone Build menu layout:** use the Inventory wood background and 64×64 spaced build icons; keep the main window clear below the grid, move Close into the browse row, and show material icons/counts plus placement requirements in a compact separate panel below. Update the existing Build cleanup verifier contract; runtime/rendered verification deferred.

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

**Complete:** accepted isolated source `1a411c6d2b76b3d9b4bd438b8449632fd5bac1f4`, integrated on main; final integrated-main build, full automation queue, and M14 acceptance passed 2026-10-12. [Scope and criteria](docs/04-roadmap.md#m14--expanded-world-map-rework) remain authoritative.

- [x] Render distinct original biome textures across supported zoom levels.
- [x] Preserve owner exploration fog without revealing hidden terrain through view changes or markers.
- [x] Provide owner-local marker selection and optional labels, including icon-only pins.
- [x] Relay server-validated right-click world pings globally for ten seconds, without saving them.
- [x] Remove map co-op controls/presence; keep map sharing and shared pins out of scope.
- [x] Cache bounded terrain/fog tiles across pan/zoom and reject stale async work.
- [x] Pass final editor build, 123/123 automation tests, four live map runners, viewport/scale/contrast/zoom/theme matrix and rendered/performance checks.
- [x] Complete the integrated-main milestone-final automation gate: UE 5.8 editor target rebuilt successfully after the test repairs; the full `Automation RunTests Kalmala` queue passed 123/123 with exit code 0. See [integrated-main verification and repairs](docs/evidence/m14/main-integration.md).

The accepted evidence includes 22 native PNGs, biome raster, exact commands, hashes and performance measurements. Recover the report and manifest from commit `9f11e09` with `git show 9f11e09:docs/evidence/m14/README.md` and `git show 9f11e09:docs/evidence/m14/acceptance.json`. Detailed verification and superseded retry history remain in [PROGRESS.md](PROGRESS.md) and Git history.

The M14 integrated-main verification is complete; no M14 increment remains. Map sharing remains deferred.
