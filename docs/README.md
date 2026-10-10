# Kalmala documentation

This folder holds the project's durable design and technical contracts, operating guides, and focused acceptance evidence. The current completion summary and execution queue live in [PROGRESS.md](../PROGRESS.md) and [BACKLOG.md](../BACKLOG.md). They keep only milestone-level history; detailed contracts and formal acceptance evidence remain in the relevant documents below.

## Core design and project guidance

| Document | Purpose |
| --- | --- |
| [00 Project brief](00-project-brief.md) | Product vision, player promise, creative constraints |
| [01 Game design](01-game-design.md) | Core loop, player systems, world and content pillars |
| [02 Technical architecture](02-technical-architecture.md) | UE5 stack, multiplayer boundaries, project conventions |
| [03 Agent operating guide](03-agent-operating-guide.md) | Rules, definition of done, handoff format |
| [04 Roadmap](04-roadmap.md) | Incremental milestones and acceptance criteria |
| [05 Decision log](05-decision-log.md) | Decisions that should not be silently reversed |
| [06 Agent prompts](06-agent-prompts.md) | Ready-to-use implementation prompts |
| [07 Development setup](07-development-setup.md) | Build, editor, and dedicated-server setup |
| [08 World generation and biomes](08-world-generation-and-biomes.md) | Original deterministic open-world generation roadmap and biome prototype parameters |

## Gameplay and acceptance contracts

| Document | Purpose |
| --- | --- |
| [09 Inventory verification](09-inventory-verification.md) | Inventory and carried-tool verification contract |
| [10 Campfire and crafting](10-campfire-and-crafting.md) | Survival-camp, construction, crafting, and cooking contracts |
| [11 Combat and support magic](11-combat-and-support-magic.md) | Combat, creature, discovery, and support-effect contracts |
| [12 Vertical-slice runbook](12-vertical-slice-runbook.md) | Tool-free 20–30 minute M5 session charter and acceptance evidence |
| [13 Onboarding and tutorial](13-onboarding-and-tutorial.md) | Route-free local prompt beats, accessibility, and authority boundaries |
| [14 Settings and accessibility](14-settings-and-accessibility.md) | Local option groups, accessibility requirements, and authority boundary |
| [15 Presentation ownership](15-presentation-ownership.md) | Project-owned visual seams, allowed sources, and runtime limits |
| [16 Audio cue contract](16-audio-cue-contract.md) | Original audio cues, readable fallbacks, and authority/privacy limits |
| [17 M7 tools and gathering](17-m7-tools-and-gathering.md) | Tool durability, harvesting, repair, and progression rules |
| [18 M7 automated acceptance](18-m7-automated-acceptance.md) | Headless M7 progression, camp recovery, food, creature, discovery, and privacy acceptance |
| [19 M8 ocean travel contract](19-m8-ocean-travel-contract.md) | Ocean travel, authority, movement, and persistence rules |
| [20 M8 ocean travel budget profile](20-m8-ocean-performance-budget.md) | Serialized travel-save size cap, measurement, and remaining M8 profiling work |
| [21 M8 finite-world origin stability](21-m8-world-origin-stability.md) | Fixed-origin peer verification during the streamed ocean crossing |
| [22 M8 integrated discovery voyage](22-m8-integrated-discovery-voyage.md) | Two-peer discovery, crosswind/calm sailing, streamed route, and safe disembark check |
| [23 M9 biome sources](23-m9-second-wave-biome-sources.md) | Source-to-material map for the second-wave biome catalogue |
| [24 M9 source catalogue](24-m9-second-wave-source-catalogue.md) | Canonical source IDs, presentation, placement, sparse identity, and fixed optional loot |
| [25 M9 harvest integration](25-m9-second-wave-harvest-integration.md) | Item catalogue, bounded server harvest rewards, and sparse depletion wiring |
| [26 M9 source acceptance](26-m9-second-wave-source-acceptance.md) | Deterministic rewards and accepted/rejected harvest transaction checks |
| [27 M9 carried-tool inventory](27-m9-carried-tool-inventory.md) | Owner-only tool records with separate server-owned level and condition |
| [28 M9 camp and equipment recipes](28-m9-camp-equipment-recipes.md) | Current camp recipes, direct material costs, and server validation |
| [29 M9 exploration rewards](29-m9-exploration-rewards.md) | Optional land discoveries, stable identities, and reward boundaries |
| [30 M9 persistence migration](30-m9-persistence-migration.md) | Versioned world/player saves, identity matching, migration, and reconnect gates |
| [31 M10 scope freeze](31-m10-scope-freeze.md) | Frozen launch gameplay/content baseline, save versions, and post-freeze change rule |
| [32 M10 camp sampling regression](32-m10-camp-sampling-regression.md) | Camp-tradeoff sampling regression |
| [33 M10 crafting verifier regression](33-m10-crafting-verifier-regression.md) | Rendered release verification corrections |
| [34 M10 constrained performance](34-m10-constrained-performance.md) | Verified CPU-affinity stress profiles, simulation limits, and release acceptance |
| [35 M11 UI theme](35-ui-theme.md) | Shared local theme, tokens, and view styling |
| [36 M11 status icons](36-status-icons.md) | Status parent, icon catalogue, and presentation rules |
| [37 Item details](37-item-details.md) | Shared inventory item details |
| [38 Menu browsing](38-menu-browsing.md) | Local menu browsing and selection behavior |
| [39 Crafting ingredients](39-crafting-ingredients.md) | Ingredient and requirement presentation |
| [40 Notifications](40-notifications.md) | Compact owner-local notifications |
| [41 Recipe activity](41-recipe-activity.md) | Local recipe/build activity contract |
| [42 Build repairs](42-build-repair.md) | Recorded project-file and editor-build repairs |
| [43 Catalogue icon manifest](43-catalogue-icon-manifest.md) | Pinned M12 item/tool icon identities, aliases, shared views, image batches, and validator |
| [44 Catalogue copy audit](44-catalogue-copy-audit.md) | M12 item, recipe, build, station, and result text audit |

## Completion evidence and current handoffs

| Document | Purpose |
| --- | --- |
| [45 M12 final acceptance](45-m12-acceptance.md) | Final UI repairs, owner Inventory and service captures, combined HUD matrix, verification results, and hardware acceptance |
| [46 Master integration and cleanup](46-master-cleanup.md) | Preserved branch histories, combined UI verification, and workspace cleanup |
| [47 Shared player inventory](47-inventory-grid.md) | Forty-cell inventory, numbered hotbar, armor/weight presentation, authority, and verification handoff |

## Contract checks

| Check | Purpose |
| --- | --- |
| [Onboarding contract check](../Scripts/Verify-OnboardingContract.ps1) | No-build validation for local tutorial prompt rules |
| [Settings contract check](../Scripts/Verify-SettingsAccessibilityContract.ps1) | No-build validation for local settings/accessibility rules |
| [Presentation ownership check](../Scripts/Verify-PresentationOwnership.ps1) | No-build audit for presentation assets and source anchors |
| [Audio cue contract check](../Scripts/Verify-AudioCueContract.ps1) | No-build validation for the M5 audio specification |
| [Local input contract check](../Scripts/Verify-LocalInputContract.ps1) | No-build validation for the existing keyboard/controller input baseline |
| [M5 documentation contract suite](../Scripts/Verify-M5DocumentationContracts.ps1) | Runs the current no-build M5 presentation/settings checks |

## Authority order

The M11 status parent, status migration, complete icon audit and their
verification are documented in [36 Status and icons](36-status-icons.md).

1. A direct current user instruction
2. This project's decision log
3. Technical architecture
4. Game design
5. Project brief

When documents conflict, resolve the conflict in this order and record a decision rather than silently choosing.
