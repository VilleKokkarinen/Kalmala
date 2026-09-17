# Kalmala documentation

This folder is the project’s durable operating manual. An agent must read the files relevant to its task before changing the project.

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
| [12 Vertical-slice runbook](12-vertical-slice-runbook.md) | Tool-free 20–30 minute M5 session charter and acceptance evidence |
| [13 Onboarding and tutorial](13-onboarding-and-tutorial.md) | Route-free local prompt beats, accessibility, and authority boundaries |
| [Onboarding contract check](../Scripts/Verify-OnboardingContract.ps1) | No-build validation for local tutorial prompt rules |
| [14 Settings and accessibility](14-settings-and-accessibility.md) | Local option groups, accessibility requirements, and authority boundary |
| [Settings contract check](../Scripts/Verify-SettingsAccessibilityContract.ps1) | No-build validation for local settings/accessibility rules |
| [15 Presentation ownership](15-presentation-ownership.md) | Project-owned visual seams, allowed sources, and runtime limits |
| [Presentation ownership check](../Scripts/Verify-PresentationOwnership.ps1) | No-build audit for presentation assets and source anchors |
| [16 Audio cue contract](16-audio-cue-contract.md) | Original audio cues, readable fallbacks, and authority/privacy limits |
| [Audio cue contract check](../Scripts/Verify-AudioCueContract.ps1) | No-build validation for the M5 audio specification |
| [Local input contract check](../Scripts/Verify-LocalInputContract.ps1) | No-build validation for the existing keyboard/controller input baseline |
| [M5 documentation contract suite](../Scripts/Verify-M5DocumentationContracts.ps1) | Runs all current no-build M5 presentation/settings checks together |

## Authority order

1. A direct current user instruction
2. This project’s decision log
3. Technical architecture
4. Game design
5. Project brief

When documents conflict, resolve the conflict in this order and record a decision rather than silently choosing.
