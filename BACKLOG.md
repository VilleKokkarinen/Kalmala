# Autonomous development backlog

Documentation layout: former docs 08–47 are consolidated into the core docs (00–07). See [Game features](docs/game-features.md), [architecture contracts](docs/02-technical-architecture.md#consolidated-system-contracts), and [verification index](docs/07-development-setup.md#consolidated-verification-index). This user-directed cleanup does not close any milestone.

This is the short execution queue and completion index. The roadmap and feature contracts in docs/ remain authoritative for scope and acceptance criteria.

## Roadmap status

M0–M14 and world-generation Phases 1–9 are complete under their recorded acceptance scope and limitations. M13 integrated-main acceptance passed on 2026-10-11; M14 integrated-main acceptance passed on 2026-10-12. See [M14 integration evidence](docs/evidence/m14/main-integration.md).

- [x] **2026-10-13 — Omit unused item damage channels:** remove zero-valued item DamageBonuses fields and default omissions to zero before strict loading. Lightweight checks passed; Unreal verification deferred.
- [x] **2026-10-14 — Move attacks and level-up rewards into dedicated folders:** keep shared attack lists under `attacks/` and item level gains under `progression/`, with manifest validation matching those categories.
- [x] **2026-10-14 — Group ordinary material files by type:** split the materials catalogue into `items/materials/wood.json`, `ore.json`, `metal.json`, `ingredient.json`, and `others.json`, preserving item data and loader validation.
- [x] **2026-10-14 — Split cooking and crafting ingredients:** move Frost Salt to `cooking-ingredients.json`; keep Fibre and Amber in `crafting-ingredients.json`.

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

Current first-incomplete item: under “Attack-driven tree harvesting,” execute generated-forest host/client delivery.

- [x] **2026-10-15 — Standardize JSON readability:** audit all 59 tracked JSON files and reformat seven compact or inconsistent files with readable two-space indentation and line breaks. Strict parsing and data/key-order preservation checks passed.

- [x] **2026-10-15 — Split wearable gear catalogue files:** separate ordinary gear definitions and level-one costs into boots, capes, helmets, leggings, shields, and tunics files; preserve records and register all files in the manifest. JSON/reference checks passed; Unreal verification deferred.

- [x] **2026-10-15 — Move Wood Shield to Workbench:** moved its sole crafting recipe and matching upgrade-cost station to Workbench, preserving current ingredient costs. Updated catalogue coverage; JSON/unique-ID checks passed, Unreal verification deferred.

- [x] **2026-10-15 — Remove duplicate Wood Shield recipe:** removed the uncommitted Workbench duplicate, retaining the Forge recipe and all existing balance edits. Manifest recipe JSON/unique-ID checks passed; Unreal verification deferred.

- [x] **2026-10-15 — Make arrow artwork plain sharpened wood:** replace the bronze-looking attached heads with continuous whittled shaft tips, retaining feathers/style/transparency. Updated original, prepared icon and imported texture; visual review and 66-row icon validation passed. Gameplay verification deferred.

- [x] **2026-10-15 — Add Bow, String, Wooden Arrows and two-handed wielding:** String stacks to 20, arrows to 100; Workbench Bow costs 5 Wood/2 String, arrow batch costs 5 Wood for 20. Added three original imported icons and atomic capacity-safe weapon/shield swaps, including full-pack rejection with both hands occupied. Lightweight checks/import/icon audit passed; full Unreal verification deferred. String sources and shooting are excluded by user direction.

- [x] **2026-10-15 — Add Bow item-level-up data:** reuse its Workbench acquisition cost (5 Wood, 2 String) for the level-one upgrade and add +50 durability per level. Keep damage rewards at zero while shooting is unimplemented. Data/source checks passed; Unreal verification deferred.

- [x] **2026-10-15 — Require complete item additions:** require an original generated icon, runtime mapping/icon validation, and every applicable item catalogue JSON entry with manifest registration. Explicitly record deferred or inapplicable categories. Instruction added to `AGENTS.md`.

- [x] **2026-10-15 — Move Bow and Wooden Arrows recipes to Workbench:** place both existing recipes in `recipes/stations/workbench.json`, remove the obsolete `archery.json` manifest entry and file, and retain their Workbench station requirements and costs. JSON/reference checks passed; Unreal verification deferred.

- [x] **2026-10-15 — Rename the two-handed item flag:** use `twoHanded` in equipment JSON and map it to the internal flag; absent flags mean one-handed and the retired JSON key is rejected. Loader/source inspection passed; Unreal verification deferred.

- [x] **2026-10-15 — Generate Bone fragment artwork:** generate an original transparent icon matching the existing materials, retain original/prepared PNGs, import the Texture2D in batch H, wire its shared runtime mapping and replace its test fallback exception. Import validation and the 63-entry icon audit passed; gameplay build/runtime verification deferred.

- [x] **2026-10-15 — Add Bone fragment to cape costs:** require one `BoneFragment` in Boar Hide Cape crafting and its level-one upgrade-cost record, preserving other costs and leaving drop sources deferred. Lightweight data/diff checks passed; Unreal verification deferred.
- [x] **2026-10-15 — Add Bone fragment for shields:** add `BoneFragment` as a crafting material and require one in Wood Shield crafting and its level-one upgrade-cost record. No drops or acquisition sources added. JSON/reference checks passed; Unreal verification deferred.
- [x] **2026-10-15 — Simplify wearable armor:** replace typed wearable defenses with one integer `Armor`, reducing combined Slash/Crush/Pierce/Stab damage once per hit without reducing elements. Use integer level-up rewards and schema-15 catalogue validation. Lightweight checks passed; Unreal build/automation deferred.
- [x] **2026-10-15 — Remove catalogue manifest file-count cap:** allow any non-empty category file list; retain manifest/category JSON-size, path-safety, and definition-count validation. JSON/path checks and `git diff --check` passed; Unreal build and menu render rerun deferred.
- [x] **2026-10-15 — Derive upgrade-cost identity for strict conversion:** keep upgrade JSON authored only by `ToolId`, validating it and deriving the shared struct's internal `RecipeId` before strict conversion. Data/source checks and `git diff --check` passed; Unreal build and menu render rerun deferred.
- [x] **2026-10-15 — Fix gameplay source first-header compile errors:** put each affected implementation's own header first in seven Gameplay `.cpp` files while retaining the resistance-component include. Focused include-order checks and `git diff --check` passed; Unreal build deferred.
- [x] **2026-10-15 — Fix Inventory Worn Gear sizing compile errors:** construct `FSlateChildSize` with its supported size rule and assign `Value` separately for gear slots and the balancing spacer. Focused source checks and `git diff --check` passed; Unreal build and UI automation deferred.
- [x] **2026-10-15 — Fix Stone Workbench crafting compile errors:** resolve tool labels through the equipment profile catalogue and bind each Stone-tool button to a reflected callback compatible with UMG's dynamic click event. Focused source/profile checks and `git diff --check` passed; Unreal build and UI automation deferred.
- [x] **2026-10-15 — Repair storage GUID compile errors:** reuse the existing item-GUID set while validating saved chest contents, and update the initial storage fixture deposits to pass exact carried-item GUIDs. Focused source/API checks and `git diff --check` passed; Unreal build and storage automation deferred.
- [x] **2026-10-15 — Standardize carried equipment identity as `ItemId`:** use `ItemId` across equipment code and catalogue data; advance the catalogue schema to 14; store canonical `ItemId` directly in the current schema-2 player save.
- [x] **2026-10-15 — Remove development SaveGame migrations:** current player, construction, storage, M7 sparse, and travel save paths accept only their current formats; removed compatibility conversions, and deferred migrations until release. Source searches and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Add armor items and a worn-equipment inventory:** introduce shield, chest armor, leg armor, helmet, cape, and boots, with a separate gear section in the Inventory popup.
  - [x] Author the six armor item families in catalogue JSON with defensive stats and level-up rewards; add original icon artwork, imported textures, and complete catalogue icon-manifest coverage.
  - [x] Add station-validated acquisition recipes and item level-up cost records for the new armor items, using the existing catalogue and progression contracts.
  - [x] Add a Worn Gear section to the Inventory popup with Main Hand, Head, Cape, Chest, Shield, Legs, and Boots slots; use the supplied image as a layout reference and retain Kalmala's original visual theme and artwork.
  - [x] Make gear slots a distinct, GUID-addressable part of carried inventory: equip and unequip compatible items, move replaced items into available inventory space, and keep equipped items out of the forty-cell grid. Route equipped weapons and tools such as axes and the Construction Hammer through Main Hand, with server-side slot, capacity, and transaction validation. Completed (2026-10-15): six owner-only wearable slots and Main Hand use server-validated GUID transactions. Equipped items leave the forty-cell grid; displaced/returned tools require available pack capacity. Existing physical tool records retain condition and gameplay routing. Lightweight checks passed; full Unreal verification is deferred.
    - Follow-up (2026-10-15): the Inventory displays equipped wearable names and provides owner-facing equip, replace, and return actions with server results. Equipped defenses apply to server-resolved wildlife damage; condition wear is implemented for accepted wildlife hits; Main Hand/tool routing remains.
    - Main Hand selection increment (2026-10-15): the tile now selects, replaces, or clears an exact carried weapon/tool GUID through a validated server request, reusing the current owner-private active identity for combat, harvesting, and the Construction Hammer. Clicked duplicate copies retain their GUID. Tools still occupy pack cells; distinct Main Hand storage, grid exclusion, replacement/return capacity accounting, and the separate hotbar contract remain unfinished. This selection-only increment left the child unchecked; the following location increment completes it.
    - Main Hand location increment (2026-10-15): equipped tool GUIDs are excluded from grid cells, packed-tool counts, and pack browse rows. Replacement returns the old GUID into available space; a full pack rejects return without changing selection. Crafting, grants, storage, progression and wearable capacity account for Main Hand. Existing hotbar tool use enters the same transaction; food/support use preserves the hand. Final numbered-hotbar behavior, privacy/runtime coverage and the current development save contract remain the next child.
  - [x] Define how Main Hand selection interacts with the numbered hotbar and existing carried-tool state; preserve owner privacy and current development save-format policy, and add focused catalogue, icon, inventory, and multiplayer coverage. Do not add save migrations until release. Completed (2026-10-15): compiled the final Main Hand/hotbar integration, passed 10 focused Unreal automations, live host/client privacy, and two-visit reconnect acceptance. Catalogue/icon checks passed; current save formats and owner-only fields are unchanged. Construction Hammer has no attack list and server combat rejects it without unarmed fallback.
    - Two-handed hotbar coverage increment (2026-10-15): extended `Kalmala.Gameplay.Inventory.TwoHanded` to exercise the server hotbar path for Bow capacity rejection/retry, shield-only full-pack selection, ordinary-item rejection and invalid numbers. Checks rejection layout, shield wear and retained tool GUID/level/condition. Lightweight API/fixture/data/diff/path checks passed; Unreal execution remains deferred. Child and parent remain unchecked pending executed equipment/hotbar/privacy/save acceptance and catalogue/icon gaps.
    - Current-format save coverage increment (2026-10-15): added `Kalmala.Gameplay.Inventory.EquipmentCurrentSaveFormat` for two copies of all sixteen Main Hand profiles, exact GUID/level/condition round-trip, owner/world isolation, atomic duplicate/over-cap replacement rejection, and serialized incompatible-schema/missing-GUID rejection without conversion or backfill. Lightweight source/data/API/diff/path checks passed; Unreal execution and live save/reconnect acceptance remain deferred. Save fields/caps are unchanged; child and parent remain unchecked.
    - Server hotbar coverage increment (2026-10-15): added `Kalmala.Gameplay.Inventory.MainHandHotbar`, an isolated authority-world fixture calling the hotbar path for invalid/empty numbers, duplicate-tool replacement, accepted/rejected food and support use preserving Main Hand, full-pack ordinary-selection rejection and retry after freeing capacity. Checks exact GUIDs, levels/condition and gameplay effects. Lightweight source/API/data/diff/path inspection passed; Unreal execution remains deferred. This child and parent remain unchecked pending executed equipment/hotbar/privacy/current-format save acceptance and the existing catalogue/icon gaps.
    - Contract/coverage increment (2026-10-15): numbered cells are pack locations, with no persistent Main Hand aliases; equip vacates its cell and return/replacement uses first free space. Added grid/HUD regression source cases for duplicate copies, stable numbering, synchronization and return. Updated network contract coverage for the GUID-only Main Hand RPC, index-only hotbar intent, owner-client result and owner-only inventory/gear/grid/active fields. Lightweight checks passed; Unreal tests were not executed. This child remains unchecked pending the catalogue/icon audit and live hotbar/privacy/save acceptance.
    - Equipment audit increment (2026-10-15): added focused icon automation source for all six wearable families and sixteen Main Hand profiles, including retained Stone texture aliases, distinct assets and shared-widget loading. Extended worn transactions to equip all six slots together and return every original GUID. The 62-entry icon file validator and focused equipment data/source/path checks passed; Unreal tests were not executed. Child remains unchecked for live equipment/hotbar/privacy/current-format save acceptance and the stale global icon test. Pre-existing station edits duplicate `WoodShieldRecipe` across Forge/Workbench and remain untouched.
    - Global icon-test increment (2026-10-15): replaced historical count/lifecycle assumptions with validated authored catalogue traversal, corrected Stone/Campfire asset names, and covered the deliberate Bone fragment fallback. Missing icon diagnostics now identify the item without cascading through uninitialized shape state. Lightweight checks passed for 66 mapped identities/packages and the 62-entry ledger. The broader audit found six pre-existing unmapped materials (Copper, Tin, Bronze, CopperOre, TinOre, IronOre); the global test still rejects them. Unreal execution and live hotbar/privacy/current-format save acceptance remain pending, as does resolution of the pre-existing duplicate shield recipe. Child and parent remain unchecked.
    - Material icon increment (2026-10-15): added original Bronze, Copper, Tin, CopperOre, TinOre, and IronOre artwork, unique runtime shape mappings, and imported catalogue textures in batch J. Extended the ledger and icon tooling to 72 canonical assets; source/prepared/imported texture and global catalogue coverage are now represented. The file audit and lightweight data/source checks passed; Unreal catalogue/icon automation and live equipment/hotbar/privacy/current-format save/reconnect acceptance remain unexecuted, so this child and parent remain unchecked.
    - Equipment source-audit increment (2026-10-15): expanded `Scripts/Tests/Inventory/Verify-Inventory.ps1 -ContractOnly` to require nine focused Main Hand, hotbar, grid, owner-privacy, current-save, worn-gear and icon automation registrations plus key assertion anchors. Contract-only execution, PowerShell parsing and `git diff --check` passed. This verifies authored source coverage only; Unreal compilation/automation and live peer/save acceptance remain pending, so this child and parent remain unchecked.
    - Multiplayer privacy increment (2026-10-15): extended the live inventory host/client fixture to seed nonempty Main Hand, head gear, and grid state on the host, then require that the remote client still sees all three owner-only fields empty after replication. Added a two-second propagation window and source preflight/documentation. Contract-only preflight, PowerShell parsing, path-length audit, and `git diff --check` passed; Unreal execution is deferred, so the child and parent remain unchecked.

- [x] **2026-10-15 — Use tiered wood across weapon families:** Bronze Axe, Knife, Spear, and Mace acquisition and upgrade costs use Lightwood; Iron equivalents use Densewood. Preserve other ingredients and quantities; leave Pickaxe costs unchanged. Lightweight JSON/reference checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Use tiered wood for axe costs:** require Lightwood for Bronze Axe acquisition and upgrades, and Densewood for Iron Axe acquisition and upgrades. Preserve all other ingredients and quantities. Lightweight JSON/reference checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Fix Bronze item upgrade costs:** charge Bronze bars instead of Stone for all five Bronze equipment upgrades, retaining each item Wood and Bronze quantities. Lightweight JSON/reference checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Group item level-up data:** move item upgrade-cost files into `Content/Data/GameCatalogues/item-level-up/costs/` and move level-up rewards to `item-level-up/rewards/item-stats.json`; update the manifest, loader path validation, and current documentation. JSON/path/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Add an upgrade cost for every equipment family item:** author costs for Stone, Bronze, and Iron Axe, Knife, Pick, Spear, and Mace entries using their acquisition materials; allow Starter, Crafted, and Utility profiles in upgrade validation. Lightweight JSON/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Name the Construction Hammer upgrade file:** rename `upgrades/upgrades.json` to `upgrades/construction-hammer.json` and update its manifest and documentation references without changing the upgrade record. Lightweight JSON/path checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Add weapon upgrade-cost files by family:** add empty upgrade files for axes, knives, pickaxes, spears, and maces, and preserve all existing records in the general upgrade file. Lightweight JSON/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Add Stone-tier equipment recipes to Workbench:** add level-one recipes for Stone Axe, Knife, Pick, Spear, and Mace, and expose repeatable Workbench craft actions for these GUID-bearing items. Lightweight JSON/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-15 — Keep Forge acquisitions in forge.json:** move the ten equipment acquisition records into the Forge station `recipes` array, route them by `ToolId` in the loader, and remove the obsolete Forge and empty Workbench item-recipe files and manifest group. Lightweight JSON/path/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Build stations and upgrades directly from the hammer menu:** route Workbench, Forge, Workbench Tool Rack, and Forge Anvil from `recipes/building.json` through direct world placement with raw-material costs, never through inventory item output. Catalogue/UI source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Retire Reed Fibre:** remove its catalogue item and active ID references, remove recipe costs, and redirect harvest/discovery rewards to existing items. Normalize storage by dropping retired or unknown item rows; no save-schema change. Lightweight JSON/reference and diff checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Give inventory instances GUID identity:** assign unique GUIDs to carried tool instances and inventory stacks, remove the separate carried-tool record limit, allow duplicate item IDs, and split/fill stacks according to each item's stack limit within the shared 40-cell inventory. Preserve the starter hammer in hotbar slot 1. Lightweight source and diff checks passed; full Unreal verification deferred.
- [x] **2026-10-14 — Give every item unit a GUID:** create an identity for each unit in stackable items as they are granted or crafted, preserve unit IDs through stack changes, send exact item GUIDs for storage transfers, and upgrade chest saves to schema 2 with migrated per-item IDs. Lightweight source/diff checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Group hides as materials:** move Boar Hide and Deer Hide from `items/food.json` to `items/materials/hides.json` and update the item manifest and catalogue documentation. Lightweight JSON/path and diff checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Remove the Mireling Ash item:** drop it from the item catalogue, defeat rewards, UI icon mapping, catalogue icon manifest, and its dedicated icon assets; discard legacy chest stacks without changing save schema. Lightweight source/data and diff checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Allow multiple carried Building Hammers:** make Inventory Craft allow additional hammers, preserve them in the existing carried-tool save records, enforce the existing slot cap, and show their grouped count. Lightweight source and diff checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Route Forge construction and Bronze acquisitions:** place Forge and Forge Anvil directly from raw materials through the hammer Build menu, keep Anvil placement bound to a matching Forge, and keep Bronze equipment acquisition recipes at Forge. Catalogue/record checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Give upgrade costs their own schema:** use an `upgrades` array, key upgrade-cost records only by `ToolId`, and advance the catalogue schema to 13. Lightweight JSON/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Rename Peat Amber to Amber:** change the item ID and display name, update harvesting and upgrade references, rename the icon sources/package, and migrate saved chest stacks from `PeatAmber`. Catalogue/reference checks, batch-F icon import, targeted icon validation, and `git diff --check` passed; full Unreal verification deferred.

- [x] **2026-10-14 — Group crafting recipes by station:** group station-bound recipes and equipment acquisitions under `recipes/stations/`, include Inventory as a crafting context, and keep build-menu recipes in `recipes/building.json`. Exact record preservation, station routing, manifest paths, and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Group buildable items by station:** move station and upgrade item definitions into `items/buildItems/` station files, group Storage and construction pieces in `other.json`, and allow that nested path only for ordinary items. Item preservation, manifest path validation, and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Group equipment recipes under recipes/equipment:** move the axe, knife, pickaxe, spear, and mace acquisition recipe files; allow this nested folder only for item recipes. Manifest JSON/path validation and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Group equipment profiles under items/equipment:** move the axe, knife, pickaxe, spear, mace, and Building Hammer profile files; permit only this nested item-profile folder in catalogue path validation. Manifest JSON/path validation and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Combine material catalogue files:** move natural material definitions into `items/materials.json` and remove the separate manifest file entry. The item records are preserved; lightweight JSON/path validation and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Sort catalogue records by ID:** remove manual recipe, item-profile, and item-acquisition order lists; sort ordinary items, recipes, carried profiles, acquisitions, and upgrade costs alphabetically by their IDs during loading. Preserve duplicate-ID rejection. Lightweight manifest/source checks and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Group carried equipment as item data:** route all carried profiles, acquisition recipes, and upgrade costs through item-named manifest groups; move the Building Hammer profile into `items/`. The loader still feeds the existing runtime tool catalogue. Lightweight manifest JSON/path/order validation and `git diff --check` passed; Unreal verification deferred.

- [x] **2026-10-14 — Move tool upgrades into their own catalogue folder:** move the existing costs to `upgrades/upgrades.json` and route `toolUpgradeFiles` through the loader's `upgrades/` path validation. Preserve the JSON data and runtime behavior; lightweight path/source inspection and whitespace checks passed, full Unreal verification deferred.

- [x] **2026-10-14 — Batch menu preview renders:** render a menu set in one Unreal commandlet process, preserve an isolated preview world per image, and report per-image and batch timing. PowerShell parsing and `git diff --check` passed; Unreal verification and timing comparison deferred.

- [x] **2026-10-14 — Match Forge Upgrade to Craft detail layout:** give the owner comparison the wider right detail frame, with material icon tiles and the Upgrade action in a bottom footer; retain prerequisite and availability text in the left frame. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Keep Options scrolling vertical and themed:** make Controls content fill its viewport, use vertical-only scrolling, and apply a shared bronze-on-wood scrollbar with a high-contrast treatment. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Allow unbound alternate controls:** default alternative bindings to `Not bound` except Inventory Tab; let each alternate cycle through its allowlisted keys and back to unbound, and remove other secondary keys from `DefaultInput.ini`. Lightweight source inspection and whitespace checks passed; no test, build, or render ran.

- [x] **2026-10-14 — Show all nonzero Forge Upgrade stats:** display maximum durability and every nonzero damage-bonus channel from the item's current/target levels; omit zero channels and use shared increase/decrease colors. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Add configurable alternate keyboard bindings:** show keyboard/mouse primary bindings and a matching Alternative keys group; save and apply both bindings to the same action/axis, with I primary and Tab alternate for Inventory. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Keep Forge upgrade bonus inline:** stop stretching the baseline text across the comparison row so the green bonus sits beside its opening parenthesis. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Default upgrade cost station level:** supply the implicit zero `RequiredStationLevel` before strict tool-catalogue conversion so the crafting menu renderer can load its recipe data. Lightweight JSON/source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Restore Forge upgrade icon coverage:** remove the stale axe-texture check and report the configured level icon shown by the current comparison; align the crafting verifier and setup docs. Lightweight inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Fix world-map compile shadowing:** give the acceptance-evidence pawn coordinate its own name so it no longer hides the player-centering local (C4456). Lightweight inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Center crafting and upgrade requirements:** center Craft material costs with the Craft action, and center Forge Upgrade requirements with its action while keeping the stat comparison adjacent. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Simplify the Forge Upgrade card:** show Bronze Axe with a target-level icon and slash bonus baseline/gain; color the gain through shared level-up theme colors; remove the separate axe tile and keep the button/tooltip to `Upgrade`. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Show the Inventory Tab alternative:** label the Controls choice as Tab/I and explain that clicking it switches between the keys. Existing single-key selection behavior is unchanged; lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Open Build with the active hammer:** remove Build from Control-bound keys, open the standalone Build menu with right-click only while Construction Hammer is active, and start new characters with the hammer active in hotbar slot 1. Lightweight source inspection and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-14 — Keep the expanded map centred on the player:** remove pan/recenter controls and the Recenter map binding row; keep the owning player at the view centre on open and zoom. PowerShell parser and whitespace checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Standard recipe arrays:** use recipes roots for all categories, including equipment acquisitions and upgrade costs; update loader routing. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Equipment items arrays:** rename toolProfiles roots to items in all equipment JSON and update both loader passes. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Station-only crafting/upgrade eligibility:** remove obsolete player-skill/unlock checks and requirement text, retain station/material/item constraints, and regress missing/uninitialized ledgers. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Omit default item skill levels:** remove MinimumSkillLevel from all equipment JSON and default it internally to 1. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Remove empty recipe skill requirements:** omit None/0 skill properties from all authored acquisition/upgrade JSON and default them in the loader. Lightweight checks passed; Unreal verification deferred.

- [x] **2026-10-13 — Separate axe/pick resource families:** enforce family before material tier; regress all axe/pick tiers against wood/ore sources and high item levels. Lightweight checks passed; Unreal verification deferred.

- [ ] **2026-10-13 — Attack-driven tree harvesting:** axes swing at trees, fell them into 3D trunks, and break trunks into collectible wood.
  - [x] Integrate the server-authoritative tree/trunk/drop loop into existing wood-resource actors, preserving tiers, yields and sparse depletion; replicate stages and remove tree interaction harvesting.
  - [ ] Convert decorative generated forest trees into individually attackable sources and integrate their depletion/streaming policy.
    - **Execution plan (2026-10-16):** select the first unchecked step below for each future run. Existing implementation and authored regressions are the starting point; the dated notes below are history, not extra tasks. Add coverage only for a concrete uncovered acceptance requirement or a failure discovered while executing these steps. Check this parent only after all five steps pass. These focused verification steps are explicitly queued work; broad project verification remains reserved for final tree-feature acceptance.
    - [x] **Compile and execute existing forest regressions:** use the documented temporary build/runtime mirror with short paths, normal `%LOCALAPPDATA%\UnrealBuildTool` access and outputs outside protected repository directories. Compile affected modules and run the existing tree-felling, forest reconstruction/lifecycle and M9 SecondWaveHarvestAcceptance automations. Identify exact registered test names from source; repair failures within this feature and rerun failing cases. Done when the selected queue compiles, passes and exits successfully; record commands, source commit, logs and any unrelated failures. Do not substitute more source assertions for this execution step.
    - Executed 2026-10-16 on base `30cd141` plus this run's fixture repairs: editor build and all four focused automations passed (4/4, exit 0). Fixed isolated controller registration, startup hotbar cadence and tick world context. Commands, logs, failures and limits are recorded at the top of `docs/07-development-setup.md` and `PROGRESS.md`. Generated-forest parent remains open.
    - [x] **Exercise actual population save-slot persistence:** extend the existing real-pickup fixture or focused harness to reach the normal platform save writer and startup loader in an isolated temporary project/user/save root, with the writer override disabled. First confirm all resolved output paths stay below MAX_PATH and outside repository `Saved/` and existing saves. For generated Meadow and Elderwood, collect one exact ID, restart/reload, verify it stays absent while uncollected neighbors return, and verify rejected/full-pack requests create no depletion. Require actual slot I/O and successful executed reload assertions; memory or generic temporary-file snapshots alone do not complete this step. Clean up only run-owned temporary outputs; no schema migration.
    - Executed 2026-10-16 on base `f86b731` plus this increment: normal population writer and shared startup loader passed for generated Meadow/Elderwood in fresh temporary user storage (1/1, exit 0); exact collected IDs excluded and all neighbors rebuilt twice, rejection/full-pack requests created no slot. Population slots cleaned up. Default four-test queue also passed 4/4. Commands and limitations are in the setup handoff; process restart and live peers remain outside this isolated reload step.
    - [ ] **Execute generated-forest host/client delivery:** extend an existing harvest/world peer runner with the smallest missing scenario. Verify cosmetic suppression and exact replacement actors agree on both peers; client axe attacks produce server-owned standing/trunk/drop transitions and exactly one pickup reward/depletion. Include late join during trunk/drop state and forged/out-of-range rejection. Run it and retain per-peer assertions/logs; fixture authoring alone does not complete the step.
      - Live-drop trace-range gate executed (2026-10-17), source 607a539: isolated editor build succeeded; ForestPickup passed exit 0 with nine matching patches, shared Trunk/Drop/Collected, range rejection Delta=0 LiveDrop=1, client/server Lightwood Delta=1 Expected=1 Stable=5, client Requests=3 DepletedRequests=3 and host Delta=0 Requests=3 Stable=5. Logs: C:/Users/Ville/AppData/Local/Temp/kfpeer-a3acc420/{host,client}.log. No repair required. Child remains unchecked; next forged/out-of-range peer requests. This server-local probe does not prove physical-distance rejection or RPC delivery; full feature verification deferred.
      - Authoritative pickup evidence (2026-10-17): server fixture captures the remote inventory baseline before attacks, derives expected yield independently, requires collected state and five exact-reward observation ticks, and continues rejecting excess reward while the runner is active. ForestPickup now requires matching authority/client ID, item and quantity evidence. Parser and six actual selector cases passed; compile/live execution deferred. Child remains unchecked; next execute this stricter gate, then competing pickup and forged/out-of-range rejection.
      - Post-depletion retries executed (2026-10-17): source 947a412 compiled successfully in the isolated kmforest mirror; ForestPickup passed with nine matching patches, shared exact-ID Trunk/Drop/Collected and Lightwood Delta=1 Expected=1 Requests=3 DepletedRequests=3 Stable=5. Logs: C:/Users/Ville/AppData/Local/Temp/kfpeer-8f95906f/{host,client}.log. No runtime repair needed; competing pickup and forged/out-of-range rejection remain next. Full feature verification deferred; child remains unchecked.
    - Standing delivery increment (2026-10-16): extended Verify-BiomeExpansion with opt-in ForestDeliveryOnly; compiled and executed live listen-host/client checks for all nine populated patches (162 standing replacements), matching exact IDs, owner/count, transforms within movement quantization, shape, source and zero cosmetics. Final runner passed, exit 0; logs/commands in setup. Child stays unchecked: client attacks, trunk/drop late join, pickup reward/depletion and forged/out-of-range peer rejection remain next.
    - Standing delivery acceptance hardening (2026-10-16): require nine distinct matching eighteen-tree patches instead of accepting one; deduplicate repeated host reports and include partial coverage in timeout diagnostics. PowerShell parser, six synthetic coverage cases and replay of retained successful peer logs (9/9) passed. No new Unreal/peer execution; same child stays unchecked with transition/pickup/rejection scope pending.
    - Stage observation increment (2026-10-16): opt-in observer continues after standing success and emits validated, deduplicated exact-ID Trunk/Drop observations, including first observation after late join. Standing acceptance stays standing-only. Lightweight checks passed; compilation/live transitions deferred. Same child remains unchecked; next wire client axe actions and runner stage assertions, then execute peers.
    - Stage runner increment (2026-10-16): optional ForestStages requires nine standing patches plus matching host/client Trunk and Drop for one exact tree ID/seed. Parser and six synthetic selector cases passed; no live transition run. Child stays unchecked; client axe driving, late join, pickup/depletion and rejection acceptance remain.
    - Client axe driver increment (2026-10-16): ForestStages now enables opt-in server Iron Axe/position setup after standing coverage and owner-local ordinary attack requests with stage observation holds and a sixty-request budget. Lightweight source/API/parser checks passed; no compile/live peers. Same child remains unchecked; execute and repair driver, then late join, pickup/depletion and rejection acceptance.
    - Executed client stage increment (2026-10-16): temporary editor build and live ForestStages runner passed after synchronizing the fixture axe into the inventory grid before Main Hand selection. Both peers matched nine standing patches and Trunk/Drop for the same exact tree; final runner exit 0. Commands and per-peer logs in setup/progress. Child stays unchecked; next late join during trunk/drop, then pickup/depletion and forged/out-of-range rejection.
    - Drop late-join runner increment (2026-10-16): optional ForestDropLateJoin launches a third observer only after both original peers report the same drop, then requires its exact ID/seed/drop/zero-cosmetic delivery. Parser and eight actual selector cases passed; live execution deferred. Child remains unchecked; next execute/repair drop late join, then trunk late join, pickup/depletion and rejection acceptance.
    - Executed drop late join (2026-10-16): existing temporary editor mirror matched Source/Config/Content; live ForestDropLateJoin passed exit 0 with nine matching standing patches, shared exact-ID Trunk/Drop and the third observer receiving that same Drop with zero cosmetics. Logs: C:/Users/Ville/AppData/Local/Temp/kfpeer-b782bd41/. No runtime repair or new build needed. Child remains unchecked; next trunk late join, then exactly-once pickup/depletion and forged/out-of-range rejection.
    - Trunk late-join gate increment (2026-10-16): added ForestTrunkLateJoin and an opt-in non-shipping driver hold at Trunk. Original peers and the new observer must report the same exact trunk ID/seed with zero cosmetics. Parser and seven actual selector cases passed; compilation/live execution deferred. Child remains unchecked. Next compile and execute trunk late join, then pickup/depletion and forged/out-of-range rejection.
    - Executed trunk late join (2026-10-16): compiled the changed fixture in the source-matched temporary mirror; live ForestTrunkLateJoin passed exit 0. Nine standing patches match; original peers and third observer report the exact same tree ID/seed/Trunk with zero cosmetics. Logs: C:/Users/Ville/AppData/Local/Temp/kfpeer-18e7d044/. No runtime repair needed. Child remains unchecked; next exactly-once pickup/reward/depletion and forged/out-of-range rejection.
    - Client pickup gate increment (2026-10-16): authored optional ForestPickup driver and exact-ID runner gate for three ordinary owned interaction requests, expected inventory delta stable for five ticks, and replicated collected state on host/client after shared Trunk/Drop. Parser and ten actual selector cases passed; compile/live pickup acceptance deferred. Child stays unchecked; next compile/execute/repair pickup, then competing pickup and forged/out-of-range rejection; sparse-save write count is not asserted by this mode.
    - Executed client pickup (2026-10-17): repaired the opt-in fixture camera/range setup and kept stationary movement/aim updates active. Temporary editor build and ForestPickup live runner passed, exit 0: nine standing patches, exact shared Trunk/Drop/collected ID and one Lightwood after three requests, stable for five ticks. Logs: C:/Users/Ville/AppData/Local/Temp/kfpeer-83dbb606/. Child remains unchecked; next competing pickup and forged/out-of-range rejection. Eye-position fixture camera does not establish default third-person pickup usability; sparse-save write count remains unasserted.
    - Post-depletion retry hardening (2026-10-17): pickup driver now sends three additional ordinary requests only after observing replicated collection, then requires five stable reward ticks. Runner requires DepletedRequests=3 and rejects older evidence. Parser and six actual selector cases passed; compile/live execution of the stricter gate remains deferred. Child stays unchecked; next execute this gate, then competing pickup and forged/out-of-range rejection.
    - Executed authoritative pickup evidence (2026-10-17): source 1e5ab98 compiled in the matching temporary mirror; live ForestPickup passed exit 0 with nine matching patches and exact shared Trunk/Drop/Collected. Server and client independently report Lightwood Delta=1 Expected=1 Stable=5; client confirms three initial and three post-depletion requests. Logs: C:/Users/Ville/AppData/Local/Temp/kfpeer-152a7450/. Child remains unchecked; next competing pickup and forged/out-of-range rejection. Full feature verification deferred.
    - Second collector gate (2026-10-17): ForestPickup captures the host inventory baseline, places the host within range opposite the client after exact-ID collection, and calls the existing authoritative harvest transaction three times. Each must reject; host reward must remain zero for five subsequent observations. Runner requires matching seed/tree/item and Delta=0 Requests=3 Stable=5 evidence. PowerShell parser and seven actual selector cases passed (valid accepted; missing proof, reward, request count, stability, ID and item mismatches rejected). Compilation/live execution and full verification deferred. Sequential server-local calls do not establish simultaneous contention or host RPC delivery. Production validation, authority, rewards and save formats unchanged. Delivery child remains unchecked; next compile/execute this gate, then forged/out-of-range rejection and simultaneous competition if required.
    - Live-drop range gate (2026-10-17): ForestPickup probes the authoritative interaction transaction with trace distance one centimetre beyond InteractionRange before collection. Requires rejection, unchanged live Drop state and zero remote reward; runner matches exact tree/item evidence. Parser and six actual selector cases passed. Unreal compilation/live execution deferred; child stays unchecked. This server-local probe does not prove remote RPC or physical-distance rejection. Next compile/execute the stricter gate, then forged/out-of-range peer requests.
    - Executed second collector gate (2026-10-17), source 035f714: source-matched kmforest editor build succeeded (66 actions; C:/Users/Ville/AppData/Local/Temp/kmforest/second-build.log). ForestPickup live runner passed exit 0; logs C:/Users/Ville/AppData/Local/Temp/kfpeer-ba81dc1b/{host,client}.log. Nine eighteen-tree patches matched, with shared exact-ID Trunk/Drop/Collected. Client/server Lightwood Delta=1 Expected=1 Stable=5; client Requests=3 DepletedRequests=3; host Delta=0 Requests=3 Stable=5. Zero fatal/assertion/ensure/fixture-failure markers; finally stopped peers and no UnrealEditor remained. No repair required. Delivery child remains unchecked; next forged/out-of-range rejection, with simultaneous contention and host RPC delivery still unproven. Full feature/project verification deferred.
    - [ ] **Execute streaming and restart acceptance on peers:** reuse the preceding harness to cross a patch boundary, retain a neighborhood shared by two players, evict after the last leaves, then return. Verify no duplicate actors/cosmetics, collected IDs remain absent, uncollected damaged/trunk/drop trees reset to standing on regeneration, and a new session preserves only collected depletion. Record actor counts and activation timing against existing documented budgets; fix feature-related collision, relevance or lifecycle defects and rerun.

    - [ ] **Close generated-forest integration handoff:** inspect combined implementation and executed evidence, reconcile stale current-state descriptions in game design/architecture/setup, and record the supported behavior and remaining moving-fall scope. Check this generated-forest parent only with successful compile, focused automation, real-slot reload and peer evidence from the preceding steps; leave the overall tree feature open for presentation and final acceptance.
    - Persistence guard increment (2026-10-16): reject empty harvest IDs before the population writer; extend ForestPatchLifecycle with zero-write/no-mutation checks for empty, non-authoritative and foreign-world callbacks before existing valid snapshot coverage. Lightweight checks passed; Unreal execution, production slot I/O and peers pending. Child stays unchecked.
    - Isolated disk snapshot increment (2026-10-16): generated pickup regression round-trips production callback bytes through a unique OS temporary file before patch reconstruction, with MAX_PATH/no-overwrite checks and scope-exit cleanup. Lightweight checks passed; Unreal execution, production slot I/O and peers pending. Child stays unchecked.
    - Real pickup production callback increment (2026-10-16): generated Meadow/Elderwood acceptance now binds RecordHarvestedSpawn and reconstructs patches from bytes captured at its final writer boundary. Observer no longer mutates the ledger; checks zero damage/rejected-pickup writes and exactly one successful write despite rival/repeated pickup. Lightweight checks passed; Unreal execution, disk I/O and peers pending. Child stays unchecked.
    - Production callback increment (2026-10-16): ForestPatchLifecycle now invokes a materialized node's real GameMode depletion binding, captures the final write through a private development-test override, and reconstructs from those serialized bytes. Checks exact seed slot/user, mark-before-write, prior/new depletion, undepleted candidates and no activation/eviction writes. Lightweight checks passed; disk I/O, real-pickup-to-production-callback integration and Unreal/peers remain pending. Child stays unchecked.
    - Competing pickup increment (2026-10-16): both generated biome cases now create a distinct authenticated nearby rival eligible before pickup; the losing request preserves rival pack/Main Hand and grants no wood, while existing winner/reload checks retain exactly one sparse callback. Lightweight checks passed; Unreal execution, production disk persistence and peers pending. Child stays unchecked.
    - Drop request guard increment (2026-10-16): generated Meadow/Elderwood drop acceptance rejects null/foreign actors, invalid trace/range and distant pawns after felling; preserves drop, exact pack/Main Hand, condition, rewards and depletion before successful retry. Lightweight checks passed; Unreal execution, production disk persistence and peers pending. Child stays unchecked.
    - Shared placement increment (2026-10-15): extracted `FKalmalaForestLayout` descriptors and made the existing renderer consume them. Original candidate ordinals, exact local/world positions, biome and shape are available for server materialization. Source-equivalence checks passed; attackable actors and depletion/streaming integration remain pending.
    - Stable identity increment (2026-10-15): generated descriptors now carry `forest:v1` identities derived from the full world seed, exact patch center and original candidate ordinal, independent of filtered array order. No depletion writer or actor integration yet; child remains incomplete.
    - Actor bridge increment (2026-10-15): added server reconstruction of an accepted forest candidate into the existing birch/ironheart trunk actor flow, with stable identity and sparse-depletion callback routing. No live streaming caller yet; cosmetic suppression, matching dimensions and patch ownership remain pending. Child remains incomplete.
    - Forest size increment (2026-10-15): actor reconstruction retains and replicates trunk/canopy dimensions and rebuilds standing harvest geometry from them. Legacy resource-node sizes remain the default. Primitive topology, foliage materials and fallen stages are unchanged; live spawning, cosmetic suppression and streaming/depletion ownership remain pending. Child remains incomplete.
    - Transform increment (2026-10-15): successful forest reconstruction enables actor movement replication before assigning generated position/yaw and forcing the existing network update. Legacy nodes retain their default. Source ordering/default checks passed; live peer execution, patch lifecycle, cosmetic suppression and sparse depletion integration remain pending. Child remains incomplete.
    - Cosmetic suppression increment (2026-10-15): added server-validated, replicated candidate suppression for decorative trunks, canopies and roots. Empty lists restore trees. No live caller yet; spawning/eviction and sparse depletion integration remain pending. Child remains incomplete.
    - Sparse reload increment (2026-10-15): terrain activation now filters shared forest candidates against the matching world's harvested-ID save and publishes their original ordinals through replicated cosmetic suppression. Live actor spawning/eviction and callback binding remain pending; child remains incomplete. Lightweight source checks passed; Unreal verification deferred.
    - Patch lifecycle increment (2026-10-15): activation now spawns and initializes server forest harvest actors, binds sparse depletion, and suppresses depleted or represented candidates. Patch eviction destroys its actors without harvesting; failed suppression rolls back activation. Lightweight source checks passed; Unreal integration/peer verification and lifecycle edge-case coverage remain pending, so child remains unchecked.
    - Batch reconstruction increment (2026-10-15): patch activation reuses server-generated descriptors through a private authority-guarded initializer instead of regenerating the full layout for every actor. Lightweight source checks passed; runtime lifecycle/collision/reload verification remains pending, so child stays unchecked.
    - Lifecycle regression increment (2026-10-15): added actor reconstruction coverage for authority/invalid ordinals, stable identity/source/transform, duplicate initialization rejection, destruction without depletion callbacks and standing-state reconstruction. Lightweight source checks passed; Unreal test execution and integrated GameMode/collision/peer/saved-depletion coverage remain pending, so child stays unchecked.
    - Patch relevance increment (2026-10-15): generated forest nodes inherit their terrain owner relevance so replacement actors share the audience receiving cosmetic suppression. Legacy nodes retain independent relevance. Extended lifecycle fixture settings checks; lightweight inspection passed, live peer delivery/cost and integrated lifecycle/depletion verification remain pending. Child stays unchecked.
    - Standing collision coverage increment (2026-10-15): extended the reconstruction fixture with complex procedural-mesh traces through the generated trunk above the root sphere and below the canopy, an adjacent miss, visibility-channel response, and exact hit actor checks before/after reconstruction. Lightweight source/geometry checks passed; Unreal execution, integrated depletion/eviction and peers remain pending. Child stays unchecked.
    - World collision coverage increment (2026-10-15): added simple and complex world Visibility traces selecting the exact generated actor before/after reconstruction and requiring no blocking hit after each destruction. Lightweight source checks passed; Unreal execution, integrated patch eviction/depletion and peer acceptance remain pending. Child stays unchecked.
    - Session reset coverage increment (2026-10-16): extended reconstruction coverage with explicitly seeded partial standing damage, partial trunk damage and uncollected drop state. Each eviction must avoid depletion and reconstruct the same ID at standing/full health with collision restored. Lightweight source checks passed; the fixture bypasses axe/pickup actions and GameMode eviction, and Unreal execution/integrated depletion/peer acceptance remain pending. Child stays unchecked.
    - Generated pickup coverage increment (2026-10-16): extended SecondWaveHarvestAcceptance with real Meadow/Elderwood forest candidates through equipped-axe damage, trunk/drop and pickup. Requires deterministic yield, exact condition cost, no early depletion, one stable forest sparse callback and no session callback or duplicate grant. Lightweight source/API checks passed; Unreal execution, saved reload, GameMode eviction and peers remain pending. Child stays unchecked.
    - GameMode lifecycle coverage increment (2026-10-16): added ForestPatchLifecycle using actual patch activation/empty-neighborhood eviction and an in-memory sparse-save round trip. Checks depleted-tree exclusion, complete undepleted identity reconstruction, ownership, duplicate activation and actor/bookkeeping cleanup across two activations. Lightweight source checks passed; Unreal execution, cosmetic suppression assertions, pickup callback persistence and peers remain pending. Child stays unchecked.
    - Cosmetic suppression coverage increment (2026-10-16): ForestPatchLifecycle checks the replicated ordinal list against all saved/live candidates in canonical order on activation, duplicate activation and reactivation. Lightweight checks passed; Unreal execution, rendering/peers and pickup callback persistence remain pending. Child stays unchecked.
    - Save guard coverage increment (2026-10-16): ForestPatchLifecycle covers missing/foreign saves, complete identity/suppression, bound depletion callback rejection without foreign-save contamination, and eviction. Source checks passed; successful pickup persistence and Unreal/peer execution remain pending. Child stays unchecked.
    - Shared-neighborhood coverage increment (2026-10-16): ForestPatchLifecycle uses two possessed player controllers to verify one departure retains exact shared terrain/forest actors, last departure evicts them, and neither changes sparse depletion. Lightweight checks passed; Unreal execution, successful pickup persistence and peers remain pending. Child stays unchecked.
    - Disjoint-neighborhood coverage increment (2026-10-16): ForestPatchLifecycle moves one possessed pawn four patch columns away and back, checking union size, stationary actor retention and remote eviction cleanup. Lightweight checks passed; Unreal execution, successful pickup persistence and peers remain pending. Child stays unchecked.
    - Overlapping-neighborhood coverage increment (2026-10-16): ForestPatchLifecycle moves one pawn to an adjacent patch and back, requiring the 12-patch union, three introduced patches, exact original terrain/forest retention and introduced actor destruction. Lightweight checks passed; Unreal execution, successful pickup persistence and peers remain pending. Child stays unchecked.
    - Pickup sparse-save coverage increment (2026-10-16): real axe/pickup callbacks populate a sparse ledger and round-trip it in memory, checking no pre-pickup depletion, exact generated identity/world retention and no creature/unrelated-ID contamination. Lightweight checks passed; production disk callback and Unreal/peer execution remain pending. Child stays unchecked.
    - Pickup-to-patch reload increment (2026-10-16): SecondWaveHarvestAcceptance feeds its real pickup ledger round trip into isolated GameMode activation and eviction twice, checking exact collected-ID exclusion and every uncollected neighbor. Requires multiple candidates to avoid vacuous neighbor coverage. Lightweight source checks passed; production disk callback and Unreal/peer execution remain pending. Child stays unchecked.
    - Pickup capacity rejection increment (2026-10-16): generated Meadow/Elderwood pickup acceptance fills all forty pack cells with nonstackable filler, rejects pickup without changing drop/depletion, exact grid/Main Hand, rewards or axe condition, then removes filler and retries the same drop through existing sparse reload coverage. Lightweight checks passed; Unreal execution, production disk callback and peers remain pending. Child stays unchecked.
  - [ ] Add a moving fall/log presentation and verify the integrated tree flow in Unreal and on peers.
    - [ ] **Implement deterministic moving tree fall:** animate the original procedural tree from standing into its fallen-trunk presentation using server-authored transition state and timing, preserving exact source identity, attack validation, durability, reward and depletion policy. Define collision/targeting during the transition, ensure clients and late joins derive the same pose, and keep partial fall state session-only. Use lightweight checks for this implementation increment and document the contract; no new physics/network service or save migration.
    - [ ] **Integrate moving-log targeting and peer presentation:** make the settled trunk and wood bundle readable and attackable at their presented positions, prevent transition overlap from granting extra hits/rewards, and extend the existing focused/peer harness only for the new transition behavior. Execute targeted compile/tests and visually inspect host/client fall, trunk breaking, pickup and late-join presentation; record evidence and repair feature defects.
    - [ ] **Run final tree-feature acceptance and close the parent:** review the combined changes, run all applicable verification prescribed by `docs/07-development-setup.md` (including required build, automation, integration and peer checks), repair feature-related failures and rerun required checks. Verify tiers, condition costs, collision, capacity rejection, exact-once rewards, real-slot restart depletion, streaming and moving presentation together. Record commands/results/evidence and known limitations; mark the overall tree feature complete only after required verification succeeds. Do not broaden repairs into later weapon/tool work.

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
