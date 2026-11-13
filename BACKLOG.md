# Development backlog

## Active queue

- [x] Targeted world-save and Escape-menu repair (2026-10-30; user-requested
  outside the project-wide review queue).
  - [x] Write a fresh population record on world entry so untouched worlds are
    discoverable by the existing local-world list.
  - [x] Add solo/host Save world and Save and log out actions; save current
    world components and player state before host/solo logout, retaining the
    session on save failure. Clients retain server-mediated player logout.
  - Lightweight presentation checks passed; Unreal build and live
    create/save/logout/reopen acceptance remain deferred. Review tasks unchanged.

- [x] Project-wide code review — review every project-owned file and integration.
  - **Goal:** Produce a complete, traceable review of the current project against
    AGENTS.md and the brief, design, architecture, roadmap and decision log.
  - **Run instructions:** Complete the first unchecked, unblocked child in order,
    exactly one per run. Split large children into numbered file batches before
    reviewing them (target 10–20 minutes plus a lightweight check per batch).
    Update backlog/progress, commit only run-owned changes, then stop.
  - **Review-only boundary (CR01–CR19):** Do not change code, tests, scripts,
    configuration, catalogues or assets, including small fixes or verification
    repairs. Only BACKLOG.md, PROGRESS.md and the review evidence ledger/reports
    may be edited. Inspection tools must not rewrite project files.
  - **Finding handoff:** Before checking off each review child, add every
    confirmed actionable finding as an unchecked RF task under the separate
    Review findings remediation goal below; do not wait until CR18. Link its
    finding ID and evidence. Never implement a repair during a review run.
  - **Review defaults chosen for this plan:** Review and findings only. Record actionable defects with severity, file/line
    or asset reference, trigger, impact, evidence and suggested repair. Separate
    confirmed defects from questions. Queue repairs explicitly without treating
    this review as permission for restricted changes. A clean review also needs
    a recorded result; an unchecked or unread file is never implicitly covered.
  - **Coverage:** Include all project-owned tracked files, authored local files
    and relevant ignored source assets; exclude generated build/cache output.
    Inspect binary assets through their metadata, references and available
    previews/editor inspection; do not claim textual review of binary content.
    Record third-party/generated exclusions and ownership rationale explicitly.
  - [x] **CR01 — Establish the review baseline and coverage ledger.** Record Git
    revision and existing local changes; enumerate files by domain in a short
    `Temporary/Review/coverage.md` ledger. Give every file a pending/reviewed/
    excluded status and rationale, and create bounded batches for CR02–CR16.
    Store findings in `Temporary/Review/findings.md`; track exact reviewed
    revisions or hashes so later changes can be identified for re-review.
    Baseline `fc83040078cb02284d7bc0b4ce2f960d873b648a` fingerprints all 1,437
    tracked files and defines 272 bounded batches; no implementation review or
    findings are claimed in this baseline increment.
  - [x] **CR02 — Review project structure and Core.** Check module/target rules,
    project descriptor, shared types, ownership, dependencies and public APIs.
  - [x] **CR03 — Review session and server code.** Check solo/listen/dedicated
    lifecycle, identity, joins, disconnects, reconnects and cleanup.
  - [x] **CR04 — Review network trust boundaries.** Trace every client request
    to server validation, ownership, bounds, replay/rate handling and replicated
    privacy; cover item IDs, targets, quantities, damage and interactions.
  - [x] **CR05 — Review player traversal and interaction.** Check input routing,
    movement, traces, range/line-of-sight gates and state/lifetime transitions.
  - [x] **CR06 — Review inventory, equipment and progression.** Check capacities,
    transactions, crafting, upgrades, rewards and failure/rollback paths.
  - [x] **CR07 — Review construction and camp systems.** Check placement,
    support, storage and direct-material Hammer payment; verify no portable
    construction items can enter grants, loot, recipes or restored inventory.
  - [x] **CR08 — Review survival, combat and support magic.** Check damage,
    resources, status timing/stacking, death/recovery, authority and design limits.
  - [x] **CR09 — Review creatures and population.** Check AI, navigation,
    abilities, spawning/despawning, caps, loot identity and restart behavior.
  - [x] **CR10 — Review world generation and simulation.** Check determinism,
    seeds, terrain, biomes, water, streaming, weather, exposure and travel bounds.
  - [x] **CR11 — Review persistence end to end.** Check ownership, serialization,
    malformed/incompatible rejection, atomicity, rollback and restart behavior;
    apply the unreleased no-migration policy and never rewrite user data.
  - [x] **CR12 — Review UI and accessibility.** Check Home/gameplay transitions,
    input/focus/cursor ownership, settings apply/revert, HUD, menus, map, scaling,
    labels, empty/error states and exposure of private information.
  - [x] **CR13 — Review catalogues and authored content.** Check every JSON and
    manifest reference, recipes/stats/attacks, icon mappings/provenance, maps,
    materials, meshes, textures, audio and Blueprint wiring against runtime use.
  - [x] **CR14 — Review Editor tools and scripts.** Check commandlets, previews,
    import/export, validators, packaging helpers, argument/path handling,
    destructive operations, error reporting and Windows path-length limits.
  - [x] **CR15 — Review configuration, plugins and automation definitions.**
    Check config defaults/overrides, authored plugin code and descriptors,
    dependency assumptions, secrets/logging and build/release definitions.
    Inspection does not authorize dependency installation or CI/release edits.
    - [x] **CR15-001 — Review root repository configuration and toolchain files.**
      `.editorconfig`, `.gitattributes`, `.gitignore`, and `.vsconfig`; see the
      CR15-001 coverage result and RF131–RF132 review handoffs.
    - [x] **CR15-002 — Review project Config defaults.**
      `Config/DefaultEngine.ini`, `DefaultGame.ini`, `DefaultInput.ini`, and
      `DefaultKalmalaTheme.ini`; see CR15-002 evidence and RF150.
    - [x] **CR15-003 — Review VisualStudioTools plugin root files.**
      `Plugins/VisualStudioTools/.gitignore`, `build.proj`, `CODE_OF_CONDUCT.md`,
      `CODEOWNERS`, `CONTRIBUTING.md`, `LICENSE`, `README.md`, `SECURITY.md`,
      `SUPPORT.md`, and `VisualStudioTools.uplugin`; see CR15-003 review result
      and the open upstream CODEOWNERS question.
    - [x] **CR15-004 — Review VisualStudioTools pipeline definitions.**
      `Plugins/VisualStudioTools/azure-pipelines/compliance.yml`, `release.yml`,
      `Support-extra-UBT-args-in-UAT.BuildPlugin.patch`, and `TSAOptions.json`;
      see the CR15-004 review result and vendor-pipeline questions.
    - [x] **CR15-005 — Review VisualStudioTools filter configuration.**
      `Plugins/VisualStudioTools/Config/FilterPlugin.ini`; see the CR15-005
      review result.
    - [x] **CR15-006 — Review VisualStudioTools documentation.**
      `Plugins/VisualStudioTools/Docs/Marketplace_Readme.md`,
      `Troubleshooting.md`, and `Visual Studio Integration Tool Documentation.pdf`;
      see the CR15-006 review result and RF151–RF152 handoffs.
    - [x] **CR15-007 — Review VisualStudioTools documentation images.**
      `Plugins/VisualStudioTools/Docs/images/configuration_page.png`,
      `ide_support_ue.png`, `test_explorer.png`, and `test_options.png`; see
      the CR15-007 review result and RF152 caption evidence.
    - [x] **CR15-008 — Review VisualStudioTools packaging scripts and dependencies.**
      `Plugins/VisualStudioTools/Scripts/Package-Plugin.ps1`, `packages.config`,
      and `SignDetached.proj`; see the CR15-008 result and RF153–RF154.
    - [x] **CR15-009 — Review VisualStudioTools source formatting rules.**
      `Plugins/VisualStudioTools/Source/.editorconfig`; see the CR15-009 result.
    - [x] **CR15-010 — Review the Blueprint debugger helper module rules.**
      `Plugins/VisualStudioTools/Source/VisualStudioBlueprintDebuggerHelper/VisualStudioBlueprintDebuggerHelper.Build.cs`; see CR15-010 and RF155.
    - [x] **CR15-011 — Review Blueprint debugger helper module startup.**
      `Plugins/VisualStudioTools/Source/VisualStudioBlueprintDebuggerHelper/Private/VisualStudioBlueprintDebuggerHelperModule.cpp`
      and `VisualStudioBlueprintDebuggerHelperModule.h`; see the CR15-011
      coverage result.
    - [x] **CR15-012 — Review VisualStudioTools module build rules.**
      `Plugins/VisualStudioTools/Source/VisualStudioTools/VisualStudioTools.Build.cs`;
      see CR15-012 and RF156.
    - [x] **CR15-013 — Review Blueprint reference and commandlet implementation.**
      `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintAssetHelper.cpp`,
      `BlueprintAssetHelpers.h`, `BlueprintReferencesCommandlet.cpp`,
      `BlueprintReferencesCommandlet.h`, `FSmartBSTR.h`, `VisualStudioTools.cpp`,
      `VisualStudioToolsBlueprintBreakpointExtension.cpp`,
      `VisualStudioToolsBlueprintBreakpointExtension.h`,
      `VisualStudioToolsCommandlet.cpp`, `VisualStudioToolsCommandlet.h`,
      `VisualStudioToolsCommandletBase.cpp`, and `VisualStudioToolsCommandletBase.h`;
      see the CR15-013 coverage result and RF157–RF159 and RF207–RF208.
    - [x] **CR15-014 — Review test adapter commandlet implementation.**
      `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSServerCommandlet.cpp`,
      `VSServerCommandlet.h`, `VSTestAdapterCommandlet.cpp`, and
      `VSTestAdapterCommandlet.h`; see the CR15-014 coverage result and RF190–RF194.
    - [x] **CR15-015 — Review VisualStudioTools public interface.**
      `Plugins/VisualStudioTools/Source/VisualStudioTools/Public/VisualStudioTools.h`; see the CR15-015 coverage result and RF209.
  - [x] **CR16 — Review tests, documentation and all remaining files.** Check
    assertions against real contracts, missing regressions, stale commands,
    acceptance claims and provenance; reconcile every file outside CR02–CR15.
  - [x] **CR17 — Review integration and performance risks.** Trace complete
    gather/craft/build/save/reload, combat/reward and menu/join/reconnect flows;
    inspect tick/allocation/streaming costs, race/lifetime issues and failure
    propagation. Cross-reference findings instead of duplicating them.
  - [x] **CR17-001 — Review rollback feedback across durable item transactions.**
    Trace owner gain receipts through persisted harvest, carried-tool craft/refuel,
    and direct-material construction rollback. Extend RF43 with the confirmed
    shared failure path; inspect authority, save ordering, rollback, and the
    owner notification consumer. Record exact file scopes and current hashes.
  - [x] **CR17-002 — Trace gather/craft/build/save/reload continuity.** Follow
    accepted material acquisition and crafting through construction payment,
    world/player save owners, and restart restoration. Separate transient player
    inventory from persisted state and cross-reference CR06/CR07/CR11 findings.
  - [x] **CR17-003 — Trace combat and reward persistence.** Follow accepted hits
    and defeats through reward ownership, inventory/claim writes, retry/replay,
    rollback, reconnect, and restart behavior. Cross-reference the existing
    main-checkout RF97 (CR09-001d-08e) for the confirmed Boar/Deer capacity-loss
    finding; do not create a duplicate.
  - [x] **CR17-004 — Trace menu, join, logout, and reconnect integration.** Follow
    Home/world selection, host/client session transitions, save/logout, player
    identity, restoration, and owner/private presentation across transitions.
  - [x] **CR17-005 — Review cross-flow tick, allocation, streaming, race, and
    lifetime risks.** Assess bounded work from source structure, trace actor and
    timer teardown, and cross-reference domain findings; do not claim runtime
    performance without a profile.  
  - [x] **CR18 — Close coverage and prioritize findings.** Reconcile the ledger
    against the current tree, re-review changed files and eliminate unexplained
    gaps. Give each confirmed finding a concrete unchecked repair task with
    acceptance criteria and dependencies under Review findings remediation, or
    an explicit user disposition. Confirm earlier children already queued their
    findings; CR18 reconciles the list rather than delaying the initial handoff.
    - [x] **CR18-001 — Reconcile the current tree and recorded finding handoffs.**
      Compare the current tracked tree and file hashes with the CR01 inventory;
      reconcile every recorded RF finding with an unchecked remediation task or
      user disposition; record exact open batches, stale review assignments,
      missing evidence, and any evidence artifacts created after CR01. Keep all
      unreviewed or changed assignments explicitly open.
    - [x] **CR18-002 — Re-review stale previously reviewed assignments.**
      Re-review the bounded file batches identified by CR18-001 whose current
      hashes no longer match CR01, update the evidence with the reviewed hashes,
      and queue any newly confirmed findings. Preserve the CR01 fingerprints.
  - [x] **CR19 — Complete review acceptance.** Every in-scope file/batch has a
    recorded result, exclusions have reasons, all findings have dispositions,
    and a final report lists reviewed revision/local changes, coverage totals,
    evidence, unresolved defects and limits. Perform lightweight checks per
    batch; full implementation verification belongs to the remediation goal.
    Any inspection failure becomes a finding, never an in-review code repair.
    Review completion means coverage and triage complete; it does not imply
    unresolved defects are fixed or unperformed runtime checks passed. Leave
    this parent unchecked until these review acceptance criteria are satisfied.

- [ ] Review findings remediation — implement fixes recorded by CR01–CR19.
  - [x] **RF01 — Prune stale character rain timers (P3; CR03-001b-02).**
    GameMode.cpp:435 adds weak-key timers; GameMode.h:610 retains them with no
    production pruning (only test-fixture removal). Repeated pawn replacement
    or reconnects grows the map until world teardown. After CR19, remove expired
    keys while preserving live rain accumulation; validate repeated destroyed
    characters/disconnects and unchanged Wet timing. Evidence and acceptance:
    `Temporary/Review/findings.md#rf01--prune-stale-per-character-rain-timers-p3`.
  - [x] **RF02 — Check discovery privacy on the remote client
    (P2; CR03-001b-62, CR03-001b-64).** Affected files:
    `KalmalaCharacter.cpp:736-750`, `KalmalaGameMode.cpp:4992-4999, 5201-5290`,
    `Scripts/Tests/Progression/DiscoveryPeer.ps1:23-28`, and
    `Scripts/Tests/Progression/ExplorationRewards.ps1:27-41`.
    In DiscoveryPeer, the client probe checks only its locally controlled
    `Remote` pawn, not the host `Entitled` pawn replica. In the M9 fixture, the
    server validates Host and Remote state and reports `OwnerOnlyState=1`, while
    the client runner checks only world-generation identity; it never inspects
    the Host replica's feedback or reward inventory. Trigger: host feedback,
    serial, label, or inventory becomes visible to the nonowner while these
    fixtures still pass. Impact: peer tests can miss a privacy regression.
    Current progress-feedback and inventory fields use `COND_OwnerOnly`, so no
    live leak is established. After CR19, extend the Development-only client
    checks to inspect the nonlocal host replica after first, duplicate, and
    independent M9 claims, synchronized or polled through fixture completion.
    Keep the server owner, distance, forgery, duplicate, independent-player,
    and exact-reward assertions. Acceptance: the host retains expected feedback
    and reward; Remote observes no host feedback, serial, label, or private
    reward inventory; both fixtures fail under a test-only exposed-field
    regression. Focused validation: run both
    `Scripts/Tests/Progression/DiscoveryPeer.ps1` and `ExplorationRewards.ps1`
    after an Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf02--discovery-peer-privacy-probe-checks-the-local-pawn-p2`.
  - [x] **RF03 — Cover all Wolf land-surface samples (P3; CR03-001b-78).**
    `KalmalaGameMode.cpp:5928-5953` checks the pack anchor and all three actual
    member locations. `KalmalaPopulationLandSurfaceTest.cpp:39-52` claims its
    success case covers the center and all three members, but supplies only
    three booleans and calls only the generic all-of helper. The current
    production loop checks all three members; this is a test-coverage gap, not
    an observed spawn defect. After CR19, cover the anchor plus all three
    generated member positions and verify that any one failing member rejects
    the pack. Acceptance: focused automation covers all four positions and
    rejects each individual member failure through the pack placement path.
    Focused validation: run `Kalmala.Gameplay.Population.LandSurface` after an
    Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf03--wolf-land-surface-test-omits-one-required-pack-location-p3`.
  - [x] **RF04 — Exercise Reef Maw aquatic footprint validation (P3; CR03-001b-79).**
    KalmalaGameMode.cpp:6010-6102 validates the center and sampled sphere
    footprint. The dedicated test covers helper geometry, but its only
    descriptor-level call uses an out-of-bounds input at
    KalmalaPopulationAquaticSurfaceTest.cpp:99-102; it never reaches the
    sampling loop. No present spawn defect was observed. After CR19, add
    deterministic valid Reef Maw descriptors that exercise accepted full-water
    placement and rejection when an individual footprint sample fails.
    Acceptance: the focused automation proves both results through the
    descriptor-level predicate. Focused validation: run
    Kalmala.Gameplay.Population.AquaticSurface after an Editor build. Evidence
    and repair: Temporary/Review/findings.md#rf04--reef-maw-whole-body-surface-loop-lacks-descriptor-path-test-p3.
  - [x] **RF05 — Trace offset snake body tops for roof clearance (P3; CR03-001b-80).**
    Affected: KalmalaGameMode.cpp:6217-6235,6583-6636; KalmalaWildlifeSpawn.cpp:3645-3692;
    KalmalaConstructionActor.cpp:416-435; KalmalaPopulationRoofClearanceTest.cpp:130-169.
    Trigger: a snake body top is beneath a RoofKit whose footprint misses the actor origin.
    Evidence: the shared snake capsule has a -22 cm X offset and 210 cm half-height,
    but the body-top helper returns actor X/Y and the roof predicate traces only there.
    An offset roof can cover the actual capsule top while the center trace misses; if
    the roof is just above the body, collision overlap does not reject the descriptor.
    Proposed repair: trace from the actual top positions of active shapes for each
    actor descriptor, with deterministic coverage for offset snake profiles.
    Dependency: defer until CR19 review acceptance. Acceptance: a valid snake
    descriptor is rejected when a solid RoofKit covers its capsule top but not the
    actor-origin trace; no roof and roofs beyond 24 m remain eligible. Focused
    validation: run Kalmala.Gameplay.Population.RoofClearance after an Editor build.
    Evidence and repair: Temporary/Review/findings.md#rf05--snake-roof-clearance-traces-only-the-actor-center-p3.
  - [x] **RF06 — Exercise valid body-clearance descriptor composition (P3; CR03-001b-80).**
    Affected: KalmalaGameMode.cpp:6103-6169; KalmalaPopulationBodyClearanceTest.cpp:281-286;
    KalmalaPopulationPhysicalPlacementGateTest.cpp:24-49.
    Trigger: a regression in full descriptor body validation denies valid wildlife or
    skips a member/shape while helper-level overlap and placement assertions remain green.
    Evidence: the dedicated test calls DoesPopulationDescriptorPassBodyClearance only
    with out-of-bounds and nonfinite descriptors; it tests placement/profile/overlap
    helpers separately but never proves that a valid descriptor passes the composition.
    Proposed repair: add deterministic valid land descriptors that pass without blockers
    and reject a blocker intersecting an active body shape through the full predicate;
    cover the generated Wolf members as part of the same fixture.
    Dependency: defer until CR19 review acceptance. Acceptance:
    Kalmala.Gameplay.Population.BodyClearance proves valid admission and rejection
    through the descriptor-level body predicate. Focused validation: run that test
    after an Editor build. Evidence and repair:
    Temporary/Review/findings.md#rf06--valid-wildlife-body-clearance-composition-lacks-test-p3.
  - [x] **RF07 — Exercise movement-support descriptor composition (P3; CR03-001b-81).**
    Affected: `KalmalaGameMode.cpp:6241-6310`, `KalmalaWildlifeSpawn.cpp:848-916`,
    and `KalmalaPopulationPhysicalPlacementGateTest.cpp:24-76`. The production
    descriptor gate composes S20 land or S21 Reef Maw surface support with the
    existing species movement-support rules and checks each proposed actor
    position, including each generated Wolf member. No test calls this movement
    composition or the shared position-support helper; the combined placement
    test uses a NaN descriptor and exits before valid support evaluation.
    Trigger: a future regression routes a species through the wrong support
    policy, skips a proposed actor/member, or drops a documented biome, water,
    traversability, or snake-leash rule while the existing surface and
    fail-closed checks still pass. No current spawn defect was found. After CR19,
    add deterministic valid and rejected descriptor cases for land and Reef Maw
    support, including every Wolf member and the applicable species rules.
    Acceptance: focused automation proves supported descriptor admission and
    rejects unsupported actual actor positions through S22c composition.
    Focused validation: run `Kalmala.Gameplay.Population.MovementSupport` after
    an Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf07--movement-support-descriptor-composition-lacks-a-focused-test-p3`.
  - [x] **RF08 — Exercise S25 camp suppression for every Wolf member origin (P3; CR03-001b-82).**
    Affected: `KalmalaGameMode.cpp:6416-6480`,
    `KalmalaPopulationPhysicalPlacementGateTest.cpp:24-76`,
    `KalmalaPopulationStationCylinderTest.cpp:35-79`, and
    `KalmalaPopulationSphericalSuppressionTest.cpp:45-142`. S25 correctly
    derives each proposed Wolf member origin and checks it against both station
    and camp suppression. Existing tests cover the point queries, but none calls
    the final placement predicate with a valid descriptor; the combined-gate test
    uses NaN and exits before those checks. Trigger: a regression omits station
    or camp suppression from S25, or evaluates only the pack anchor, while the
    point-query and fail-closed tests remain green. No current production defect
    was found. After CR19, extend `Kalmala.Gameplay.Population.PhysicalPlacementGate`
    with a deterministic valid Wolf pack that passes without suppression sources
    and is rejected when a source overlaps each member origin while the pack
    anchor remains outside. Acceptance: the final gate rejects Workbench/Forge
    and eligible camp-source overlap at each actual Wolf member origin, retains
    the valid no-source pass, and preserves existing rejection-order assertions.
    Focused validation: run `Kalmala.Gameplay.Population.PhysicalPlacementGate`
    after an Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf08--s25-camp-suppression-lacks-wolf-member-composition-coverage-p3`.
  - [x] **RF09 — Cover lazy pruning of stale live-creature entries (P3; CR03-001b-84).**
    Affected: `KalmalaGameMode.cpp:6829-6864` and
    `KalmalaPopulationLiveCreatureRegistryTest.cpp:58-84`. The count query
    removes invalid, foreign-world, non-authoritative, defeated and dead weak
    entries, but the focused test only verifies explicit defeat/EndPlay
    unregistration before counting. Trigger: a regression removes lazy cleanup
    while lifecycle cleanup tests still pass, leaving stale weak handles in a
    long-running server registry. No production defect was found. After CR19,
    extend the focused registry test with stale invalid and defeated weak entries
    that bypass lifecycle unregistration, then assert the count excludes and
    prunes them while a healthy actor still follows current-position/inclusive
    radius rules. Acceptance: the focused automation proves lazy cleanup of both
    invalid and defeated entries without changing live count semantics.
    Focused validation: run `Kalmala.Gameplay.Population.LiveCreatureRegistry`
    after an Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf09--lazy-live-creature-registry-pruning-lacks-focused-coverage-p3`.
  - [x] **RF10 — Verify population unload against every living player (P3; CR03-001b-87).**
    Affected: `KalmalaGameMode.cpp:7232-7305`,
    `KalmalaPopulationCreationLifecycleTest.cpp:56-112`, and
    `KalmalaPopulationLivingPlayersTest.cpp:88-98`. The unload path correctly
    retains an actor when any valid living player is within the inclusive 96 m
    radius, but the lifecycle test exercises only one player. The separate
    living-player test confirms two players are collected but never composes
    that list with unload behavior. Trigger: a regression unloads an actor while
    another living player remains nearby, despite single-player radius and
    player-selection tests passing. No production defect was found. After CR19,
    extend the lifecycle automation with two living players, one near and one
    far, then move both outside and verify unload/cooldown behavior. Acceptance:
    the actor stays loaded when at least one living player is on or within 96 m
    and unloads only after every living player is outside. Focused validation:
    run `Kalmala.Gameplay.Population.CreationLifecycle` after an Editor build.
    Evidence and repair:
    `Temporary/Review/findings.md#rf10--population-unload-lacks-multiple-player-composition-coverage-p3`.
  - [x] **RF11 — Cover source-owned live actors in the local cap path (P3; CR03-001b-87).**
    Affected: `KalmalaGameMode.cpp:7348-7419`,
    `KalmalaEnvironmentalSpawnerPolicyTest.cpp:45-91`, and
    `KalmalaEnvironmentalSpawnerAdmissionTest.cpp:145-212`. The source-sample
    adapter joins live registry actors to their configured source IDs and
    combines them with pending reservations, but tests cover synthetic policy
    samples and pending reservations only; none exercises a successfully
    created source-owned actor through that adapter. Trigger: a regression
    omits or misattributes a live actor in the sampler while synthetic policy
    and pending-reservation tests still pass. The two stable slots bound current
    exposure; no production defect was found. After CR19, add a narrow configured
    admission regression that successfully creates one source-owned actor and
    proves a second same-source candidate within the inclusive 24 m radius sees
    that actor, while another source's actor is excluded. Acceptance: successful
    source registration, same-source local counting, other-source exclusion and
    existing pending-reservation behavior are covered. Focused validation: run
    `Kalmala.Gameplay.Population.EnvironmentalSpawnerAdmission` after an Editor
    build. Evidence and repair:
    `Temporary/Review/findings.md#rf11--source-owned-live-actors-lack-local-cap-adapter-coverage-p3`.
  - [x] **RF12 — Cover natural population transaction success and Wolf rollback (P3; CR03-001b-88).**
    Affected: `KalmalaGameMode.cpp:7423-7630`,
    `KalmalaPopulationCreationLifecycleTest.cpp:20-104`, and
    `KalmalaEnvironmentalSpawnerAdmissionTest.cpp:145-212`. The transaction
    reserves capacity, spawns and initializes each actor, validates stable IDs,
    registers live actors and callbacks, releases reservations, and rolls back
    partial Wolf packs. Trigger: a regression in creation, registration,
    reservation release or partial rollback passes current gate/cap fixtures.
    Evidence: the lifecycle fixture calls the transaction only with default
    failing gates and manually spawns its test actors; configured admission only
    covers rejection, and Wolf reservation tests do not create a pack through
    the transaction. No production defect was found. After CR19, extend
    `Kalmala.Gameplay.Population.CreationLifecycle` with passing natural single
    and three-member Wolf transactions and a controlled failure after one Wolf
    member is created. Acceptance: successful transactions produce the expected
    stable IDs, initialized live actors, registry entries and defeat callbacks,
    then release all reservations; partial failure leaves no actors, registry
    entries, reservations, defeated IDs or reward/save callbacks. Focused
    validation: run `Kalmala.Gameplay.Population.CreationLifecycle` after an
    Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf12--natural-population-creation-and-wolf-rollback-lack-transaction-coverage-p3`.
  - [x] **RF13 — Cover population scheduler GameMode tick integration (P3; CR03-001b-90).**
    Affected: `KalmalaGameMode.cpp:7905-8016`,
    `KalmalaPopulationSchedulerCadenceTest.cpp:12-212`, and
    `KalmalaPopulationSchedulerBudgetTest.cpp:12-69`. Trigger: a regression
    removes or misorders `UpdatePopulationKeySchedule` and
    `UpdatePopulationSchedulerWake` in the real server tick while helper-level
    cadence and wake-budget tests still pass. Evidence: `Tick` currently checks
    authority/world/configuration, refreshes terrain keys, updates the active-key
    schedule, then services the bounded wake. The focused population tests call
    cadence and wake helpers directly and never assert that GameMode `Tick`
    wires the sequence; no production defect was found. After CR19, extend the
    scheduler automation with a controlled server tick fixture that proves a
    due key is serviced through the actual entry point and that the configured
    wake/candidate bounds and no-catch-up behavior remain intact. Acceptance:
    a focused test observes schedule advancement followed by one bounded wake
    through `Tick`, with no extra wake before its deadline. Focused validation:
    run `Kalmala.Gameplay.Population.SchedulerCadence` and
    `Kalmala.Gameplay.Population.SchedulerWakeBudget` after an Editor build.
    Evidence and repair:
    `Temporary/Review/findings.md#rf13--population-scheduler-tick-integration-lacks-focused-coverage-p3`.
  - [x] **RF14 — Cover generated hazard materialization and permanent depletion (P3; CR03-001b-93).**
    Affected: `KalmalaGameMode.cpp:8460-8473`,
    `KalmalaInteractionAuthorityTest.cpp:254-264`,
    `KalmalaWorldPlayerStartResolverTest.cpp:232-279`, and
    `KalmalaFiniteWorldTest.cpp:34-35`. Trigger: a regression drops GameMode
    hazard initialization, stable-ID callback binding, persistent defeat write,
    or saved-ID filtering while the existing eligibility, descriptor-bounds,
    and generic save tests pass. Evidence: `ActivatePopulationKey` skips saved
    defeated hazard IDs, spawns each remaining `AKalmalaHazardSpawn`, assigns its
    deterministic ID, and binds the production defeat callback. Existing tests
    cover the eligibility predicate, generic save round-trip, and descriptor
    bounds separately; none exercises materialization through accepted defeat,
    save, and fresh activation suppression. No production defect was found.
    After CR19, add a focused GameMode activation test with multiple deterministic
    hazard descriptors, verify stable IDs and the production callback, persist
    one accepted server defeat through an isolated/in-memory writer, and require
    a fresh activation to omit only that ID while materializing its undefeated
    neighbor. Acceptance: actor materialization, exact-ID save mutation and
    defeated-slot suppression are covered without touching user save slots.
    Focused validation: run the new GameMode hazard-materialization test after
    an Editor build. Evidence and repair:
    `Temporary/Review/findings.md#rf14--generated-hazard-materialization-and-permanent-depletion-lack-integration-coverage-p3`.
  - [x] **RF15 — Recycle active population keys during traversal (P1; CR03-001b-63/66/94).**
    Affected: `KalmalaGameMode.cpp:5093-5184, 5364-5450, 7922, 8311, 8479-8622`;
    `KalmalaPopulationSpatialKeyUnionTest.cpp:42-133`;
    `KalmalaPopulationSchedulerCadenceTest.cpp:75-119`; and
    `KalmalaM9ExplorationRewardCatalogueTest.cpp:20-165`;
    `Scripts/Tests/Progression/ExplorationRewards.ps1:12-42`. Trigger: the
    server visits nine distinct spatial keys, then a player enters a new key.
    `ActivatePopulationKey` rejects new keys at the nine-key limit, while
    `ActivePopulationSpatialKeys` is only appended to and never evicted. Tick
    keeps requesting each living player's current key, but the candidate query
    and cadence cleanup only filter/remove scheduler state. Impact: population,
    harvest, hazard, first-wave discovery and M9 reward descriptors stop being
    materialized in all later cells; long-distance exploration loses generated
    encounters and discoveries. No runtime traversal was exercised. After CR19,
    implement bounded key turnover within the existing nine-key budget, with
    safe cleanup/re-activation of generated actors and per-key state while
    preserving living-creature unload cooldowns, sparse depletion, per-player
    discovery claims and server authority. Acceptance: a focused server test
    activates nine keys, moves into a tenth, and proves the new cell becomes
    active without exceeding nine. A deterministic eligible Thunder Mountains
    key must produce its canonical M9 reward actor through normal activation,
    without fixture-side key insertion or actor spawning. Reactivation must not
    duplicate actors or restore depleted sources or accepted claims. Focused
    validation: run the new key-turnover GameMode automation after an Editor
    build. Evidence and
    repair: `Temporary/Review/findings.md#rf15--active-population-keys-never-turn-over-p1`.
  - [x] **RF16 — Recover failed population depletion writes (P2; CR03-001b-95).**
    `KalmalaGameMode.cpp:8623-8655` ignores failed harvest and generic wildlife/hazard defeat population-slot writes after live depletion is committed. A restart before a later successful save can recreate the source or defeated spawn. Add a retryable, server-owned depletion transaction and prevent a failed harvest write from duplicating a durable player grant. Acceptance: injected write failure is retried or rolled back safely; accepted wildlife/hazard defeats remain scheduled until their stable IDs are durable; a fresh load never treats a failed write as durable. Focused validation: run new harvest/death write-failure automations and the forest restart regression after an Editor build. Evidence and repair: `Temporary/Review/findings.md#rf16--failed-population-depletion-writes-are-silently-abandoned-p2`.
  - [x] **RF17 — Keep wildlife defeat callback failures atomic (P2; CR03-001b-95/96).**
    `KalmalaGameMode.cpp:8658-8671,8820-9042` and `KalmalaWildlifeSpawn.cpp:478-583` leave partial state on two failure paths: Rat marks defeated before a reward queue can reject, while six species callbacks queue/mark before a failed slot write returns false. The actor then aborts defeat after the lethal hit has already reduced health to zero. Stage the save mutation and reward claim until the slot write succeeds, and roll back or retain an explicit retryable actor transaction on failure. Acceptance: Rat queue rejection and species slot-write failure leave live and serialized defeated IDs/claims unchanged and do not leave an unaccepted zero-health actor; a valid retry accepts exactly one owner-bound claim with its stable outcome. Focused validation: add a GameMode failure-injection regression for queue rejection and species slot-write failure, then run the six species contract automations and the Rat peer reward/reconnect regression after an Editor build. Evidence and repair: `Temporary/Review/findings.md#rf17--wildlife-defeat-callback-failures-leave-partial-actor-save-state-p2`.
  - [x] **RF18 — Cover Wolf-to-Reef Maw GameMode reward callbacks (P3; CR03-001b-96).**
    `KalmalaGameMode.cpp:8820-9045` has six species reward handlers, but their focused tests exercise the save-game claim helpers rather than the GameMode callbacks. The live peer scenarios check attacks/replication; only Rat's shared scenario checks reward escrow. A callback wiring, owner, stored-roll or defeated-ledger regression could pass current species tests. Add isolated GameMode/DefeatServer coverage for Wolf, Lynx, Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw. Acceptance: each actual species path creates one authenticated owner-bound claim with its stable member ID and persisted optional-roll outcome, and retry does not reroll or reassign. Focused validation: run the new callback automation and the six existing species contract automations after an Editor build. Evidence and repair: `Temporary/Review/findings.md#rf18--species-defeat-reward-callbacks-lack-focused-gamemode-coverage-p3`.
  - [x] **RF19 — Correct wildlife peer-runner instructions and M4 scenario claims (P3; CR03-001b-58/96, CR18-002 batch 105).**
    **Source:** CR03-001b-58/96, CR04-022, and CR18-002 batch 105; `docs/07-development-setup.md:2541-2564,2680-2783`; `docs/04-roadmap.md:38-48`; `Scripts/Tests/Acceptance/VerticalSlice.ps1:1-53`; `Scripts/Tests/Combat/EnemyPeers.ps1:4,20-22`.
    **Affected path:** Wildlife peer command examples and the M4 support/boss-scroll verification instructions.
    **Trigger:** A developer follows the standalone `*Peer.ps1` commands or treats the documented M4 `VerticalSlice.ps1` run as Mireling/boss-scroll coverage.
    **Impact:** Referenced per-species scripts do not exist, and the M4 section claims a Mireling two-peer scenario that the runner does not execute. The setup guide also retains a rendered two-player boss-reward requirement that conflicts with the current M4 roadmap acceptance.
    **Evidence:** `Scripts/Tests/Combat` contains only `EnemyPeers.ps1`. Its `-Scenario` set includes `SharedCombat`, `PeatHuskSweep`, `WolfPack`, `LynxLeap`, `PolarBearCharge`, `HornedGoatPush`, `ThunderSentinelSlam` and `ReefMawBite`, but no Mireling scenario. `VerticalSlice.ps1` invokes only `SharedCombat` (whose native flag selects the Rat fixture), `BoarCharge`, `DeerHerd`, and `PlayerScopedPersistence`; it does not invoke the development-only `KalmalaMirelingPeerTest` flag and does not inspect a boss-scroll entitlement. The setup section says this harness runs a Mireling peer scenario and that a rendered boss-reward scenario remains part of M4, while `docs/04-roadmap.md` says a rendered two-player acceptance scenario is not required. No production defect was established.
    **Proposed repair:** Replace absent standalone script paths with `EnemyPeers.ps1` and the valid scenario names while preserving authored seeds. Correct the M4 setup text to describe only the scenarios the harness actually runs and align its acceptance wording with the roadmap; if boss-scroll runtime validation is later needed, point to the dedicated recovery regression tracked by RF29 rather than this baseline harness.
    **Dependencies:** After CR19 review acceptance; no setup-doc or runner repair during CR03 or CR18.
    **Acceptance:** Every documented wildlife peer command resolves to the current runner and matching scenario, and M4 setup no longer claims the harness exercises Mireling or a rendered boss-scroll journey that it does not run or that the roadmap does not require. Boss-scroll coverage is claimed only when a dedicated boss-scroll check is present.
    **Focused validation:** Run `Scripts/Tests/Combat/EnemyPeers.ps1 -ListScenarios`, parser-check the updated setup documentation, and verify its M4 harness description matches the actual `VerticalSlice.ps1` scenario calls and roadmap acceptance.
  - [x] **RF20 — Correct Mireling gemstone claim delivery (P2; CR03-001b-97).**
    KalmalaGameMode.cpp:9104-9118 removes 12 characters from the
    mireling-gem| claim prefix although it is 13 characters. The saved
    defeated ledger contains the unprefixed spawn ID, so lookup receives
    |<spawn-id> and skips the pending Dusk Diamond claim on every retry.
    Parse the complete claim prefix and add delivery-path regression coverage.
    Acceptance: an authenticated generated Mireling with a persisted Dusk
    Diamond claim delivers exactly once when capacity exists; a full inventory
    retains the same claim and retry grants it once without rerolling.
    Focused validation: run Kalmala.Gameplay.Discovery.EnemyGemSources and
    the new server delivery regression after an Editor build. Evidence:
    Temporary/Review/findings.md#rf20--mireling-gem-claim-prefix-is-not-fully-removed-p2.
  - [x] **RF21 — M9 resource depletion is session-only (P2; CR03-001b-98).**
    **Source:** CR03-001b-98; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8350-8353, 9305-9311`,
      `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:524-538`, and
      `docs/02-technical-architecture.md:990-1004`.
    **Affected path:** Generated M9 harvest sources, server depletion recording, and reactivation of their
      population key after a fresh session.
    **Trigger:** Harvest an M9 source, then restart the server or load the same world in a fresh process and
      revisit its spatial key.
    **Impact:** M9 resource IDs are added only to the transient `SessionM9ResourceDepletionIds` set. The M9
      harvest event bypasses the ordinary callback that writes `UKalmalaWorldPopulationSaveGame`, and population
      activation checks only the session set for M9 sources. The source therefore respawns after restart and can
      grant its deterministic reward again, despite the documented world-scoped sparse depletion contract.
    **Evidence:** Full evidence: Temporary/Review/findings.md#rf21--m9-resource-depletion-is-session-only-p2.
    **Proposed repair:** Integrate the stable M9 resource ID with the validated world-scoped sparse ledger, use
      the loaded delta to suppress the source on activation, and handle write failure without granting a durable
      repeatable harvest. Add a GameMode integration regression that harvests a generated source, verifies the
      saved delta, and confirms fresh-load suppression; cover wrong-world identity and save-write failure.
    **Dependencies:** After CR19 review acceptance; no repair belongs to this CR03 review run.
    **Acceptance:** A successful M9 harvest persists exactly one `ResourceDepleted` delta under the matching
      world identity; a fresh session for that same world suppresses the source; a different seed or generator
      revision does not inherit it; and injected write failure cannot create a durable player grant while
      leaving depletion absent.
    **Focused validation:** Run the new GameMode M9 depletion persistence regression, the existing M9 source
      acceptance test, and the M7 sparse persistence contract test after an Editor build.

  - [x] **RF22 — Cover fail-closed player discovery save loading (P3; CR03-001b-99).**
    **Source:** CR03-001b-99; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9427-9488` and `9520-9585`.
    **Affected path:** Loading and writing authenticated player discovery/equipment state when a player slot
      already exists but is malformed, incompatible, belongs to another owner/world, or contains a stale M9
      descriptor.
    **Trigger:** Start or reconnect a player whose expected slot contains an invalid or incompatible
      `UKalmalaPlayerDiscoverySaveGameV2` record, then attempt to restore or persist player state.
    **Impact:** The production loader currently fails closed and preserves such data, as required. No focused
      test proves that the GameMode path returns no state, rejects subsequent persistence, and leaves the exact
      disk bytes unchanged, so a future overwrite or partial-restore regression could pass the save-model and
      valid-reconnect checks.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf22--cover-fail-closed-player-discovery-save-loading-p3.
    **Proposed repair:** Add an isolated GameMode persistence test that seeds the exact player slot with invalid
      schema, owner/world mismatch, and a stale-but-well-formed M9 claim in separate cases; call the production
      load and persist paths, and compare the slot bytes before and after each rejection.
    **Dependencies:** After CR19 review acceptance; no test repair belongs to this CR03 review run.
    **Acceptance:** Each invalid case returns no restored player state; a subsequent
      `PersistPlayerStateFromServer` remains rejected; the original slot bytes remain identical; and a valid
      current-format slot still loads through the same GameMode path.
    **Focused validation:** Run the new GameMode invalid-player-slot regression,
      `Kalmala.Gameplay.Discovery.Schema3CurrentFormat`, and
      `Scripts/Tests/Progression/Schema2CandidateReconnect.ps1` after an Editor build.

  - [x] **RF23 — Cover world save and Home-return transactions (P3; CR03-001b-100/CR17-004).**
    **Source:** CR03-001b-100; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9588-9710`.
    **Affected path:** Saving population, construction, storage, and authenticated player state before solo
      return, network-client return, or listen-host shutdown/Home travel.
    **Trigger:** A regression changes save ordering, mode/controller/pawn validation, failure retry state, or
      the listen-host client-disconnect/net-driver teardown sequence while current presentation checks remain
      green.
    **Impact:** Static inspection found the existing server-side gates and failure behavior consistent with the
      start-menu contract. This increment adds an isolated regression for world/player writes, retryable failure,
      client-only persistence, late-join admission/drain, and listen-driver completion ordering. The automation
      has not been run; the documented live peer and create/save/logout/reopen confirmation remains pending.
    **CR17-004 review evidence:** The listen host snapshots remote controllers before waiting for logout, but no `PreLogin` gate or equivalent return-window admission check prevents a late join. That client is absent from `PendingListenHostReturnClients`; after tracked clients leave, driver teardown can disconnect the late join. This race is inferred from source and was not reproduced live.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf23--cover-world-save-and-home-return-transactions-p3.
    **Proposed repair:** Add an isolated GameMode return/save regression with controllable write outcomes. Cover
      standalone and listen-host component/player writes, each failure leaving the session active and retryable,
      network-client persistence limited to the authenticated owner's record, and listen-host completion only
      after all requested remote clients log out and the listen driver is destroyed. Also gate new joins once host return begins or include every late arrival in that drain before closing the driver. Verify a successful path
      reaches Home and preserved slot data can be reopened.
    **Dependencies:** After CR19 review acceptance; no test repair belongs to this CR03 review run.
    **Acceptance:** Focused automation proves current-mode/controller authority gates, full save success before
      each return, retryable failure without premature travel, player-only client persistence, and listen-host
      disconnect/driver completion ordering. A join after return begins must be rejected cleanly or
      included in the drain before driver closure.
    **Focused validation:** Run the new GameMode world-save/Home-return automation and the existing player-save
      and presentation regressions after an Editor build; include a peer attempt to join while an existing client is returning; perform the documented live create/save/logout/reopen
      acceptance before the feature is accepted.

  - [x] **RF24 — Collision-blocked saved skiff can be silently replaced (P2; CR03-001b-101).**
    **Source:** CR03-001b-101; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9758-9799`, `9885-9924`, and
      `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:399-463`.
    **Affected path:** Restoring and updating the world-scoped moored `ocean-skiff:primary` snapshot after a
      server restart.
    **Trigger:** The current-format saved vessel passes identity, bounds, and generated-ocean footprint
      validation, but `SpawnActor` with `DontSpawnIfColliding` returns null because another actor blocks its
      saved location.
    **Impact:** `RestorePersistedOceanSkiff` logs that the save is preserved but leaves
      `bOceanTravelPersistenceWritable` true and retains the loaded `OceanTravelWorldSave`. Launch admission
      only checks for live skiff actors, so with no restored actor the player can launch a new primary skiff
      elsewhere. New skiffs default to `Moored`, and the launch callback immediately calls
      `PersistOceanTravelState`. That writer clones the cached save, and `SetVesselState` permits an update with
      the same stable vessel ID; the new location then overwrites the previously accepted safe location. The old
      moored state cannot be recovered after the obstruction is removed.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf24--collision-blocked-saved-skiff-can-be-silently-replaced-p2.
    **Proposed repair:** Keep the failed saved vessel slot reserved and non-writable after a collision-blocked
      restore, and prevent a new primary launch from replacing it until the original restores or an explicit
      recovery path safely resolves the saved state.
    **Dependencies:** After CR19 review acceptance; no implementation repair belongs to this CR03 review run.
    **Acceptance:** With a valid saved vessel and a blocker at its saved location, startup does not spawn a
      duplicate and a new launch cannot overwrite the slot. After removing the blocker, a fresh startup restores
      the original vessel ID and exact safe transform; no saved accepted state is lost.
    **Focused validation:** Add a GameMode restart regression with an isolated collision blocker and slot-byte
      comparison; run `Kalmala.World.OceanTravel.PersistenceContract`,
      `Kalmala.Gameplay.OceanTravel.SkiffRestoreContract`, and
      `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1` after an Editor build.

  - [x] **RF25 — Cover fail-closed Ocean travel world-slot loading (P3; CR03-001b-101).**
    **Source:** CR03-001b-101; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9736-9755`.
    **Affected path:** GameMode startup loading of the world-scoped schema-1 Ocean travel save.
    **Trigger:** A future regression accepts or rewrites an existing slot with an unsupported schema, wrong
      world identity, missing/invalid vessel payload, or malformed transform while the save-model and
      valid-restart checks remain green.
    **Impact:** The current loader fails closed and preserves incompatible slots, but no focused test drives
      that production loader branch or proves subsequent travel writes remain blocked and the exact original
      slot bytes stay unchanged. The model test checks wrong identities and malformed records in memory; the M8
      restart runner covers a valid saved vessel only.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf25--cover-fail-closed-ocean-travel-world-slot-loading-p3.
    **Proposed repair:** Add an isolated GameMode loader regression that seeds incompatible world-slot bytes,
      starts the production load path, attempts world/player travel persistence, and compares the original bytes
      before and after rejection; retain a valid current-format restore case through the same path.
    **Dependencies:** After CR19 review acceptance; no test repair belongs to this CR03 review run.
    **Acceptance:** Invalid schema, wrong seed/revision, and invalid vessel-payload cases restore no actor, keep
      travel persistence blocked, and leave exact slot bytes unchanged; a valid current-format world slot still
      restores through GameMode.
    **Focused validation:** Run the new GameMode invalid-slot regression,
      `Kalmala.World.OceanTravel.PersistenceContract`, `Kalmala.Gameplay.OceanTravel.SkiffRestoreContract`, and
      `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1` after an Editor build.

  - [x] **RF26 — Do not accept skiff launch or boarding after travel-save failure (P2; CR03-001b-102).**
    **Source:** CR03-001b-102; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9823-9949`, `9951-9984`, and
      `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:399-465`, `514-559`, `561-604`, `658-674`.
    **Affected path:** Launching or boarding the persisted ocean skiff, then disembarking or restoring the
      accepted vessel and seat after restart.
    **Trigger:** The world or authenticated passenger slot write fails, or the player's existing travel slot is
      invalid/incompatible and is correctly preserved without overwrite.
    **Impact:** Before this repair, `PersistOceanTravelState` returned failure
      when the world or any current occupant's player record could not be saved,
      but `OnOceanSkiffStateChanged` was `void` and only logged the failure. `TryLaunchFromServer`
      reported `SkiffLaunched` for the spawned actor, and `TryInteractFromServer` reported an assigned
      seat after the state setter's notification. With an incompatible existing player slot,
      `PersistOceanTravelPassenger` fails without overwriting it; later `ClearOceanTravelPassenger` rejects
      clearing that same record and `TryDisembarkFromServer` leaves the character attached. When a slot write
      fails instead, the accepted vessel or seat can exist only in session memory and disappear after a server
      restart.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf26--do-not-accept-skiff-launch-or-boarding-after-travel-save-failure-p2.
    **Proposed repair:** Propagate persistence results to launch and seat assignment. Preserve incompatible slot
      bytes, but reject or roll back a launch/boarding transition that cannot satisfy its required world/player
      write, with requester feedback that does not claim success. Keep the disembark slot-clear-before-detach
      guarantee.
    **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** Fault-injected world and passenger slot failures, plus an incompatible existing passenger
      slot, do not produce a reported successful but non-durable launch/seat. The incompatible slot remains
      byte-identical, the player is not left seated without a clearable record, and a valid persisted
      vessel/seat still restores exactly after restart.
    **Focused validation:** `Kalmala.Gameplay.OceanTravel.SaveFailureRollback` covers world-write failure,
      passenger-write failure, incompatible-slot preservation, boarding rollback, and valid restart restore.
      After an Editor build, run it with `Kalmala.World.OceanTravel.PersistenceContract`,
      `Kalmala.Gameplay.OceanTravel.SkiffRestoreContract`, and
      `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1`.

  - [x] **RF27 — Report unavailable world discovery saves to the owner (P3; CR03-001b-103).**
    **Source:** CR03-001b-103; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10060-10063`,
      `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9427-9517`, and
      `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:146-164`.
    **Affected path:** A nearby player's first landmark or scroll claim when the authenticated player discovery
      save cannot be loaded or created.
    **Trigger:** `ClaimDiscovery` has a valid feedback component, but `GetPlayerDiscoverySave` returns null for
      an unavailable identity, invalid/incompatible slot, or other fail-closed save condition.
    **Impact (before repair):** The claim returned false before mutation, but did not publish `Unavailable`. The
      discovery actor ignores the returned bool, so the owner received no reason or retry cue. This left a
      rejected interaction silent even though later save-write failures and Ocean discovery load failures publish
      owner feedback.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf27--report-unavailable-world-discovery-saves-to-the-owner-p3.
    **Repair:** When the save lookup fails and `Feedback` is valid, `ClaimDiscovery` now publishes owner-only
      `Unavailable` before returning. The claim remains rejected, invalid slot bytes remain preserved, and scroll
      effects are not applied.
    **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** A valid world discovery still succeeds once; an unavailable/incompatible owner save produces
      one owner-only `Unavailable` result with no claim or effect mutation; duplicate and out-of-range paths
      retain their existing behavior; invalid save bytes remain unchanged.
    **Focused validation:** Added world-discovery save-failure feedback coverage to
      `Kalmala.Gameplay.Persistence.PlayerDiscoveryFailClosedLoad`; after an Editor build, run that contract
      and `Scripts/Tests/Progression/DiscoveryPeer.ps1`.

  - [x] **RF28 — Recover Ocean discovery rewards when claim rollback fails (P2; CR03-001b-103).**
    **Source:** CR03-001b-103; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10155-10200`,
      `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-295`, `411-427`, and
      `Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:51-54`.
    **Affected path:** Committing an authenticated player's one-time Ocean discovery ledger entry and catalogue
      reward.
    **Trigger:** The candidate ledger slot write succeeds, `TryCommitStacksFromServer` rejects the reward
      candidate, and the compensating write of the prior ledger also fails. One commit rejection can occur when
      `BuildGrant` accepts the stack layout but the combined stack-plus-packed-tool slot limit enforced by
      `TryCommitStacksFromServer` is exceeded.
    **Impact (before repair):** Inventory remained unchanged while the last successful slot write could contain
      the new `DiscoveryClaimed` ID. After a restart, the handler reloaded that ID and reported `AlreadyFound`, so
      the player could not retry the reward. The handler logged failed compensation but had no durable recovery
      record. This violated M8's owner-private exact-once reward contract by durably consuming a claim without
      delivering its reward.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf28--recover-ocean-discovery-rewards-when-claim-rollback-fails-p2.
    **Repair:** The GameMode now preflights reward stacks with packed-tool capacity, persists a player-scoped
      `DiscoveryRewardPending` marker before inventory commit, and writes `DiscoveryClaimed` only after the grant
      succeeds. Inventory rejection attempts to restore the old ledger; failed compensation preserves the
      pending marker for retry after restart. A live-pawn delivery marker retries a failed completion write
      without granting twice. M7 save schema 2 rejects prior development saves without rewriting them.
    **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** A candidate exceeding combined packed-tool capacity is rejected before any claim is durably
      added and can be retried after capacity is freed. Injected inventory-commit and rollback-write failures
      leave a recoverable outcome across restart; no valid claim is replayable without its reward, and recovery
      cannot duplicate a granted reward.
    **Focused validation:** Added `Kalmala.Gameplay.OceanTravel.DiscoveryClaimRecovery` for packed-tool capacity,
      capacity retry, inventory/rollback-write failure recovery through a fresh GameMode, failed completion
      write retry, and duplicate prevention. After an Editor build, run it with
      `Kalmala.World.M7.PersistenceContract`, `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract`, and
      `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1`.

  - [ ] **RF29 — Keep Mireling boss scroll claims recoverable after player-save failure (P2; CR03-001b-104).**
    **Source:** CR03-001b-104; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10295-10340`, `12034-12051`,
      and `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-597`, `5392-5450`.
    **Affected path:** The authenticated credited attacker’s one-time world-seeded support scroll from a
      server-designated Mireling boss defeat.
    **Trigger:** `RecordMirelingDefeatReward` successfully persists the Mireling defeated ID, but
      `GetPlayerDiscoverySave` returns null or `PersistPlayerStateFromServer` fails while
      `ClaimMirelingBossScroll` records the scroll and learned effect.
    **Impact:** The player claim returns false after the defeated source has been durably consumed.
      `GrantDefeatReward` has already set `bDefeatRewardProcessed` and ignores the claim result; the actor
      remains defeated, and population activation suppresses that stable ID on reactivation. The persistent
      Mireling reward queue carries only the Dusk Diamond claim, not the boss scroll, so the original encounter
      has no retry path. A separate eligible Mireling may offer another attempt, but this is not recovery of the
      accepted encounter and is not guaranteed. When the save lookup returns null, the handler also returns
      without owner feedback.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf29--keep-mireling-boss-scroll-claims-recoverable-after-player-save-failure-p2.
    **Proposed repair:** Preserve a bounded server-owned, authenticated pending scroll entitlement or otherwise
      retain a retryable source state until the owner’s player save contains both the discovery and learned
      effect. Publish owner-only unavailable feedback on load/write failure, and make retry/recovery idempotent
      across reconnect without granting the effect twice.
    **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** With an eligible Mireling and injected player-save load/write failure, the defeated source
      retains a retryable owner-bound scroll outcome. After persistence recovers, the owner receives the
      world-canonical scroll and learned effect once, reconnect retains the entitlement, replay does not
      duplicate it, and other peers receive no private reward state.
    **Focused validation:** Add a GameMode/wildlife two-peer boss-scroll recovery regression with injected
      player-save outcomes; run it, `Kalmala.Gameplay.Discovery.PlayerScopedPersistence`, and
      `Scripts/Tests/Acceptance/VerticalSlice.ps1` after an Editor build. The vertical-slice runner is a broad
      combat/support baseline and must not be counted as boss-scroll coverage unless it is extended to assert
      that path.

  - [ ] **RF30 — Cover M9 exploration reward rollback on player-save failure (P3; CR03-001b-104).**
    **Source:** CR03-001b-104; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10208-10293`,
      `Source/KalmalaGameplay/Public/KalmalaGameMode.h:188-190`, and
      `Source/KalmalaGameplay/Private/Tests/KalmalaStormcutSwordTest.cpp:188-195`.
    **Affected path:** Granting an M9 land-discovery material reward while persisting the authenticated player's
      stable claim.
    **Trigger:** A future regression changes the inventory-first claim transaction or its refund after
      `PersistPlayerStateFromServer` rejects the candidate, while the current valid peer scenario remains green.
    **Impact:** The current handler attempts to restore the prior inventory and reports `Unavailable`; no test
      proves the real GameMode path leaves neither a reward nor a durable/session claim after the player-slot
      write fails, or that the player can retry once persistence recovers. A regression could grant material
      without a claim or leave inventory mutated while rejecting the discovery.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf30--cover-m9-exploration-reward-rollback-on-player-save-failure-p3.
    **Proposed repair:** Add an isolated GameMode claim regression using the player-save gate to reject the
      first persistence attempt, then restore the gate and retry the same canonical descriptor. Assert owner
      feedback, live inventory, saved claim and session cap at each step.
    **Dependencies:** After CR19 review acceptance; this finding is a test handoff only and does not authorize
      test changes during CR03.
    **Acceptance:** A forced player-save rejection publishes `Unavailable`, returns inventory to its exact
      pre-claim contents, writes no discovery claim, and leaves the descriptor retryable. Once the save
      succeeds, one retry grants exactly the catalogue quantity and persists one claim; a later replay grants
      nothing.
    **Focused validation:** Run the new GameMode failure/retry automation,
      `Kalmala.Gameplay.M9.ExplorationRewardCatalogue`, `Scripts/Tests/Progression/ExplorationRewards.ps1`, and
      `Kalmala.Gameplay.Discovery.PlayerScopedPersistence` after an Editor build.

  - [ ] **RF31 — Reject malformed biome baseline capture numeric arguments (P3; CR03-001b-105, CR18-002 batch 106).**
    **Source:** CR03-001b-105 and CR18-002 batch 106; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10382-10410, 10472-10479, 10668-10678`,
      `Scripts/Tests/World/BiomeBaselineCapture.ps1:9-18,73-87`, installed UE 5.8
      `Engine/Source/Runtime/Core/Private/Misc/Parse.cpp:678-687,709-718`,
      `Engine/Source/Runtime/Core/Public/Microsoft/MicrosoftPlatformString.h:154-167`, and
      `docs/valheim-like-lighting-plan.md:1224-1230`.
    **Affected path:** Direct non-Shipping `-KalmalaBiomeBaselineCapture` launches with coordinates or temporal/GPU frame settings.
    **Trigger:** A numeric command-line token contains non-numeric text or a valid numeric prefix followed by junk.
    **Impact:** Numeric `FParse::Value` overloads report that the token was found and convert through `FCString::Atof`/`Atoi` without checking full-token consumption. Malformed X/Y/view-Z can become zero or a parsed prefix; finiteness alone then allows terrain activation and local pawn relocation at the wrong site. Temporal and GPU settings can likewise accept a parsed prefix; a wholly invalid GPU frame token becomes zero, which is within the allowed range and can silently omit requested profiling. The checked-in PowerShell runner uses typed numeric parameters and range validation, protecting its normal path.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf31--reject-malformed-biome-baseline-capture-numeric-arguments-p3.
    **Proposed repair:** Strictly convert every numeric capture argument as a full token and reject malformed values before terrain activation, pawn relocation, or capture scheduling. Preserve valid signed/scientific coordinates and supported timing/frame bounds.
    **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** Direct launches reject non-numeric and trailing-junk tokens for X/Y/view-Z, temporal frame count/interval, and GPU frame count before terrain activation, pawn movement, or capture scheduling. Valid supported coordinates and frame settings retain their requested values, and malformed GPU settings cannot silently disable profiling.
    **Focused validation:** Add a narrow parser regression covering malformed and trailing-junk tokens for each numeric argument plus valid signed/scientific coordinates and supported frame boundaries. Verify rejection precedes scene changes and image/profile output; run the baseline capture runner on a valid fixed site after an Editor build.

  - [ ] **RF32 — Reject incomplete per-site lighting comparison fixtures (P3; CR03-001b-106).**
    **Source:** CR03-001b-106; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10622-10645`,
      `Scripts/Tests/Presentation/LightingComparison.ps1:101-108, 191-218`, and
      `docs/valheim-like-lighting-plan.md:602-610, 678-695`.
    **Affected path:** Isolated `DefaultLit` or `Toon` L03 lighting captures and their six per-site
      material-class summaries.
    **Trigger:** One smooth/banded proxy pair for a site is missing or misnamed while at least one smooth and
      one banded actor from another site remain in the fixture map.
    **Impact:** The GameMode's aggregate `SmoothActors`/`BandedActors` checks still pass, so the capture
      proceeds. The runner then samples the expected crop rectangle without checking that its proxy is present;
      background pixels can be written to `LightingSampleLuminance.csv` as that surface's measured result,
      weakening or misleading the material comparison.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf32--reject-incomplete-per-site-lighting-comparison-fixtures-p3.
    **Proposed repair:** Validate the exact six smooth and six banded proxy identities for the requested site
      before capture, and reject tagged comparison actors with an unrecognized variant prefix. Add a narrow
      rendered or fixture-level regression proving missing and misnamed pairs cannot be summarized as valid
      samples.
    **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** An isolated capture rejects a site missing or misnaming any one of its six pairs before
      producing the image or sample summary. A complete site captures all six selected proxies, hides the six
      opposite-variant proxies, and reports no background-only class crop.
    **Focused validation:** Add fixture tests for one missing pair and one unrecognized name, then run the
      focused `LightingComparison.ps1` capture on a valid generated map and verify all six per-site class crops
      contain their expected proxy. Run after an Editor build.

  - [ ] **RF33 — Restore the ambient-audio accepted-craft peer fixture (P2; CR03-001b-109).**
    **Source:** CR03-001b-109; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11026-11036`,
      `Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp:83-101,174-190`,
      `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:223-235,312-330`, and
      `Scripts/Tests/Audio/AmbientAudioPeers.ps1:70-73`.
    **Affected path:** The host/client accepted-crafting cue check in
      `Scripts/Tests/Audio/AmbientAudioPeers.ps1` when launched with `-KalmalaAmbientAudioTest`.
    **Trigger:** Run the audio peer fixture. GameMode grants nine Wood and directly invokes the `CookingRack`
      recipe, then expects one `CookingRackKit` in the player's inventory. Catalogue loading maps that recipe to
      the `CookingRackKit` buildable and places it in `BuildableOutput`; `CraftFromServer` rejects all
      direct-material buildables before payment or output. The fixture therefore reports a rejected craft.
      Independently, its log says `crafted rack`, while the runner only counts `crafted fuel: Accepted=1
      ResultSerial=1` and requires at least two matches.
    **Impact:** The accepted-craft fixture gate remains false and the documented peer runner times out even when
      the other cue checks pass. It cannot validate the accepted crafting cue for either owner.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf33--restore-the-ambient-audio-accepted-craft-peer-fixture-p2.
    **Proposed repair:** Use an enabled ordinary inventory recipe for the audio acceptance cue, assert its
      ordinary item result, and align the server marker with the peer runner. Keep construction identities out
      of inventory.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** A fresh host/client audio run publishes one accepted ordinary craft per owner at result
      serial 1 and one rejected craft at serial 2; the runner observes both accepted-craft markers and passes
      without any placed-object identity in inventory.
    **Focused validation:** Run `Scripts/Tests/Audio/AmbientAudioPeers.ps1 -WeatherExposureOnly` after an Editor
      build, then run the normal peer scenario to retain visible-water coverage.

  - [ ] **RF34 — Select a persistent, interactable source for the harvest restart fixture (P2; CR03-001b-111).**
    **Source:** CR03-001b-111; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11566-11638`,
      `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:139-164`,
      `Source/KalmalaWorld/Public/KalmalaBiomeContentContract.h:43,125`,
      `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:198-202,228-230,335-365,524-537`, and
      `Scripts/Tests/Survival/PersistedCampRestart.ps1:92-94`.
    **Affected path:** The `Harvest` then `Verify` process-restart check in
      `Scripts/Tests/Survival/PersistedCampRestart.ps1`.
    **Trigger:** Run the helper when the server start spatial key's first descriptor is a Meadow source.
      `RunReconnectVerification` always selects `HarvestSpawns[0]` and calls `InteractWithToolIntentFromServer`.
      A generated Meadow candidate can remain `meadows-birch-bark`, whose standing tree is not interactable
      until it becomes a wood drop, or it can be replaced with a hand-gathered M9 crop source. M9 depletion is
      recorded in `SessionM9ResourceDepletionIds`, while this reconnect helper requires
      `PopulationSaveGame->IsHarvested(PersistentSpawnId)` before it reports success. The M9 depletion set is
      not the world sparse harvested-ID ledger reloaded by `Verify`.
    **Impact:** For either selected source, the smoke test can fail to produce its required `harvested generated
      node` marker or fail to suppress that same node after restart. That leaves the documented world-population
      sparse-restart check dependent on an unrelated first descriptor rather than a source compatible with its
      persistence contract.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf34--select-a-persistent-interactable-source-for-the-harvest-restart-fixture-p2.
    **Proposed repair:** Select a descriptor whose current interaction and depletion path writes the population
      save's harvested-ID set, and fail early with a clear precondition if none exists. Keep session-only M9
      depletion on a separate verification path.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** `Harvest` accepts a supported generated node and records its persistent spawn ID in the
      population save. A new process with the same world identity reloads that ID and suppresses the node. A
      start key whose first generated descriptor is a tree or M9 source still selects an eligible persistent
      target or reports a clear setup failure before claiming test success.
    **Focused validation:** After an Editor build, run `Scripts/Tests/Survival/PersistedCampRestart.ps1` for the
      existing fixed-seed case and a case whose first start-key descriptor is a Meadow tree or M9 source;
      confirm both process logs contain the required success markers and the restarted server has no actor for
      the saved ID.

  - [ ] **RF35 — Reconcile four-player terrain neighborhoods with the 25-patch cap (P2; CR03-001b-112).**
    **Source:** CR03-001b-112;
      `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:92-96,11764-11778,11835-11878`,
      `Source/KalmalaWorld/Public/KalmalaTerrainPatchLayout.h:8-30`,
      `Source/KalmalaGameplay/Private/Tests/KalmalaForestPatchTest.cpp:199-225`,
      `docs/02-technical-architecture.md:282`, and `docs/00-project-brief.md:16`.
    **Affected path:** `RefreshTerrainPatchNeighborhoods`, called every second while players move through
      generated terrain.
    **Trigger:** Three or four pawns occupy disjoint radius-one neighborhoods inside world bounds. Each pawn
      contributes nine required patches, so three require 27 and four require 36.
      `RefreshTerrainPatchNeighborhoods` adds all coordinates to a `TSet`, then `ActivateTerrainPatch` silently
      returns after 25 active patches. There is no explicit per-pawn priority or reservation for each pawn's
      center or nearby movement patches, so at least two or eleven requested collision/render patches remain
      unmaterialized.
    **Impact:** Affected pawns can reach a neighborhood edge with no generated terrain actor or server collision
      beneath the route. The 1–4 player session contract and long-distance co-op travel can therefore encounter
      gaps once the players separate.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf35--reconcile-four-player-terrain-neighborhoods-with-the-25-patch-cap-p2.
    **Proposed repair:** Reconcile the four-player patch demand with the 25-patch budget. Define and enforce
      deterministic minimum terrain/collision coverage for every live pawn, or adjust the budget/neighborhood
      contract after validating the resulting memory and replication cost; never let `TSet` iteration decide
      which player loses nearby terrain.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** A focused three- and four-player test places pawns in disjoint neighborhoods and crosses
      patch boundaries. Every pawn retains continuous server collision and the documented local render area
      while active actors stay within the chosen documented cap; behavior is deterministic across runs and
      independent of set iteration order.
    **Focused validation:** Extend the forest-patch lifecycle test with three/four disjoint pawns and
      over-budget refreshes, then run a live four-peer traversal/streaming check after an Editor build and
      confirm no pawn enters an unmaterialized collision region.

  - [ ] **RF36 — Keep treasure failure feedback consistent with the claim transaction (P3; CR03-001b-113).**
    **Source:** CR03-001b-113; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:12007-12030`,
      `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:411-427`,
      `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:459-471`,
      `Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:51-54`, and
      `Source/KalmalaUI/Private/KalmalaNotificationSubsystem.cpp:244-246`.
    **Affected path:** `ClaimWorldTreasure` when a valid shared chest cannot fit its reward or its world-save
      claim fails.
    **Trigger:** First, `BuildGrant` can construct an item-stack candidate that fits the inventory stack limit,
      while the final `CanFitContents` check rejects it after accounting for carried tools; this return path
      does not publish the “Inventory is full; treasure remains” result used by the earlier grant-failure
      branch. Second, `TryCommitStacksFromServer` emits owner gain receipts before `SaveGameToSlot` commits the
      claimed treasure. If that save fails, the pack is rolled back but the reliable gain receipt is not
      retracted, and the notification subsystem can present the rolled-back item as acquired.
    **Impact:** A packed-tool capacity rejection gives the player no reason for the still-open chest, while a
      storage failure can show a false item-gain notice even though the pack and chest remain unchanged.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf36--keep-treasure-failure-feedback-consistent-with-the-claim-transaction-p3.
    **Proposed repair:** Publish explicit owner feedback from the combined-capacity rejection. Stage the gain
      receipt until the treasure claim is durable, or otherwise make the two-state transaction publish only
      after both sides commit while preserving rollback on failure.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** A candidate that exceeds capacity only after carried-tool slots are included leaves
      inventory, save and chest unchanged and tells the owner the treasure remains. An injected treasure-save
      failure leaves inventory and chest unchanged and emits no gain receipt. After capacity or storage
      recovers, retry grants the exact contents once and persists one claimed record.
    **Focused validation:** Add a GameMode transaction regression for the packed-tool slot boundary and save
      failure, then run the A9 peer treasure claim/retry with owner notifications observed.

  - [ ] **RF37 — Keep per-player Ocean save caches reachable through garbage collection (P2; CR03-002).**
    **Source:** CR03-002; `Source/KalmalaGameplay/Public/KalmalaGameMode.h:567,570`,
      `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9802-9821,9840-9881,9999-10043,10121-10203`, and
      [Epic's UE 5.8 Object Pointers
      documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine).
    **Affected path:** Ocean discovery claims and authenticated ocean-skiff seat persistence/restoration after
      the GameMode has cached a player's SaveGame objects.
    **Trigger:** `AKalmalaGameMode` stores loaded `UKalmalaM7PersistenceSaveGame` and
      `UKalmalaOceanTravelPersistenceSaveGame` objects in `OceanDiscoverySaves` and `OceanTravelPlayerSaves`,
      each a `TMap<FString, TObjectPtr<...>>` without `UPROPERTY`. The GameMode has no `AddReferencedObjects`
      override. A garbage-collection pass can therefore reclaim those objects while the map entries remain
      available for later lookup.
    **Impact:** `LoadOceanDiscoveryLedger` treats an existing key as already loaded, and `ClaimOceanDiscovery`
      consumes that cached entry rather than reloading it. After collection, a later valid Ocean discovery can
      fail as unavailable or encounter an invalid cached object for the remainder of the session. The travel
      player-save cache is likewise reused by seat persistence and restoration and can reject a valid saved
      association.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf37--keep-per-player-ocean-save-caches-reachable-through-garbage-collection-p2.
    **Proposed repair:** Mark both cache maps `UPROPERTY(Transient)` (matching the existing player-discovery
      cache pattern), or explicitly enumerate their values from `AddReferencedObjects`. Add a focused forced-GC
      regression for both cache paths.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation in CR03.
    **Acceptance:** After loading valid current-format per-player Ocean saves and forcing garbage collection,
      the same GameMode can accept one new discovery exactly once and can persist/restore the authenticated
      player's seat association. The claim and seat still match exact world and player identity, and no save
      schema or migration is introduced.
    **Focused validation:** Add an isolated GameMode automation that loads each cache, forces full garbage
      collection, verifies cached objects remain valid, then exercises the discovery-claim retry and
      seat-persistence/restoration paths. Run the focused automation after an Editor build.
  - [ ] **RF38 — Suppress M9 reward receipts when claim persistence fails (P3; CR04-010).**
    `KalmalaGameMode.cpp:10269-10282` commits the M9 reward inventory before
    saving the player's discovery claim. A failed player-save write rolls back
    the inventory but cannot retract the reliable owner gain receipt emitted by
    the first commit. Stage that receipt until the claim save succeeds, or make
    notification publication part of the transaction. Acceptance: an injected
    save failure leaves inventory and claim state unchanged, reports unavailable,
    and emits no item-gain receipt; after persistence recovers, one retry grants
    the exact reward and one receipt, while replay grants nothing. Focused
    validation: extend the M9 GameMode failure/retry automation to inspect owner
    receipts and run it with the M9 exploration peer scenario after an Editor
    build. Related transaction work: RF30 and RF36. Evidence:
    `Temporary/Review/findings.md#rf38--suppress-m9-reward-receipts-when-claim-persistence-fails-p3`.
  - [ ] **RF39 — Rate-limit repeated storage-close requests (P3; CR04-013).**
    `KalmalaStorageInteraction.cpp:89-92,300` lets the owning client call the
    reliable close RPC without the adjacent 200 ms request gate. Every call
    clears the storage view and asks its character for an immediate update
    even when already closed. Keep the first valid close immediate, then make repeats
    idempotent and bounded with a close-specific cadence. Acceptance: open-view
    closure clears immediately; rapid repeats do not request repeated character
    updates; closed storage rejects transfers and peer views remain empty.
    Focused validation: add a two-peer close-flood regression for open and
    closed states, then run the storage peer scenario. Evidence:
    `Temporary/Review/findings.md#rf39--rate-limit-repeated-storage-close-requests-p3`.
  - [ ] **RF40 — Make the non-axe harvest-gate regression reachable (P3; CR04-014).**
    `KalmalaCombatLoopTest.cpp:72,165,174` adds only profiles accepted by
    `IsAttackItemAllowed` to its melee loop, then checks Hammer/Bow rejection
    inside that loop. Both item types are explicitly excluded, so the negative
    tree-damage assertion never runs and no non-axe melee item is checked.
    Move the rejection check to an explicit non-axe melee fixture and verify
    tree health and tool condition stay unchanged. Focused validation: run
    `Kalmala.Gameplay.Combat.AllItemSwings` or `Kalmala.Gameplay.Combat.AxeSwing`
    after an Editor build. Evidence:
    `Temporary/Review/findings.md#rf40--make-the-non-axe-harvest-gate-regression-reachable-p3`.
  - [ ] **RF41 — Add remote support-magic authority and privacy coverage (P3; CR04-015).**
    `KalmalaDiscoveryProgressTest.cpp:70-73` checks pure gate helpers, while
    `KalmalaInventoryHotbarTest.cpp:63-66,116-127` calls the hotbar RPC wrapper
    directly in a local server-only world. `Scripts/Tests/Acceptance/VerticalSlice.ps1`
    runs support assertions locally after its three peer scenarios; no remote
    owner and observer support cast is exercised. The production component
    marks `LearnedMask`, `CooldownExpiry`, `Feedback` and `FeedbackSerial`
    owner-only, but no test checks those replication conditions. The setup
    guide also claims sequence/replay checks and a five-second cooldown, while
    current M13 architecture specifies slot-only intent and a 300-second
    cooldown. Add a focused owning-client/server/observer support regression
    and align the setup notes. Acceptance: a remote owner activates from its
    server-resolved hotbar slot; the server accepts one cast and rejects a
    repeat during cooldown without another payment; peers see only public
    active-effect state while entitlement, cooldown and feedback stay owner-only;
    the guide accurately describes the tested intent and cooldown. Focused
    validation: build the Editor target, run the support automation and peer
    scenario, and check observer replication. Evidence:
    `Temporary/Review/findings.md#rf41--add-remote-support-magic-authority-and-privacy-coverage-p3`.
  - [ ] **RF42 — Bound repeated interaction RPC work (P3; CR04-016).**
    **Source:** CR04-016; `Source/KalmalaGameplay/Public/KalmalaCharacter.h:296-298`,
      `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1687-1973`,
      `Source/KalmalaGameplay/Private/Tests/KalmalaInventoryNetworkContractTest.cpp:14-30`, and
      `Source/KalmalaGameplay/Private/Tests/KalmalaInteractionAuthorityTest.cpp:21-97`.
    **Affected path:** An owning client's reliable `ServerRequestInteract` requests against world actors.
    **Trigger:** An owning client sends repeated `ServerRequestInteract` RPCs. The handler obtains the server
      view point, performs `TraceInteractionCandidate`, and dispatches the hit without a server-side
      per-character cadence check. Inventory layout and hotbar RPCs have separate 50 ms and 100 ms gates, but
      this general interaction RPC has none.
    **Impact:** Repeated requests can make the server repeat view tracing and target dispatch work beyond normal
      button cadence. Target-specific authority and state gates still apply; no client-authoritative mutation or
      practical flood impact was established.
    **Evidence:** Full evidence: Temporary/Review/findings.md#rf42--bound-repeated-interaction-rpc-work-p3.
    **Proposed repair:** Add a server-owned per-character cadence for interaction requests before tracing,
      keeping the first valid request immediate and preserving existing target-specific checks.
    **Dependencies:** After CR19 review acceptance; this is a review handoff and does not authorize
      implementation during CR04.
    **Acceptance:** A remote owner can still perform an ordinary interaction immediately; a burst of repeated
      interaction requests results in at most one dispatch per configured cadence window, while invalid intent,
      range, authority and inventory checks remain intact.
    **Focused validation:** Add a focused listen-server owner scenario that issues rapid interaction requests,
      verifies the first accepted request and bounded subsequent dispatches, and confirms a normally paced
      interaction still succeeds.

  - [ ] **RF43 — Suppress owner gain receipts after durable rollback (P3; CR04-017/CR17-001).**
    **Source:** CR04-017 and CR17-001; `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:411-427`,
      `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:450-470`,
      `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:258-263,311-318`,
      `Source/KalmalaGameplay/Private/KalmalaGemstoneNode.cpp:71-78`,
      `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9520-9585,11930-11945`,
      `Source/KalmalaGameplay/Private/Tests/KalmalaM9SourceAcceptanceTest.cpp:1143-1150`, and
      `Source/KalmalaUI/Private/KalmalaNotificationSubsystem.cpp:242-246`.
    **Affected path:** A forest wood-drop harvest, M9 hand-gathered crop, gemstone
      claim, carried-tool craft/refuel, or direct-material construction action
      accepted by the server for an owning player, followed by durable persistence
      rejection.
    **Trigger:** `TryCommitStacksFromServer` updates stacks and calls
      `RecordAcceptedGains`, which immediately sends each positive delta through
      reliable owner RPC `ClientAcceptedGain`. The harvest handlers then call
      `PersistPlayerStateFromServer`; gemstone claim calls `PersistGemstoneState`.
      If a player save or gemstone population-slot write fails, each handler
      restores inventory, but the earlier receipt remains in the owner's buffer
      and can be shown as an item-gain notice. CR17-001 confirmed that the same
      positive-delta helper can publish compensation gains when carried-tool
      craft/refuel or direct-material construction fails at its durable write.
    **Impact:** The owner can be told that an item was gained even though the
      corresponding action failed and restored prior inventory, tools, or
      construction state. This is a feedback/transaction-consistency defect; no
      authority bypass, peer disclosure, or duplicated durable reward was
      established here.
    **Evidence:** CR04-017 evidence:
      Temporary/Review/findings.md#rf43--suppress-m9-harvest-gain-receipts-after-persistence-rollback-p3.
      CR17-001 statically corroborated the same refund-receipt path for carried-tool
      craft/refuel and direct-material construction rollback.
    **Proposed repair:** Stage each owner gain receipt until its durable action
      commits. Discard staged receipts when inventory, tool, source, or
      construction state is rolled back; never publish compensating positive
      deltas as gains. Preserve the existing server-side source, capacity, and
      persistence gates.
    **Dependencies:** After CR19 review acceptance; this is a review handoff and does not authorize
      implementation during CR04/CR17.
    **Acceptance:** Injected player-save failures for forest and hand-gathered
      M9 harvests, a gemstone-slot write failure, and carried-tool craft/refuel
      or direct-material construction write failures leave source/save,
      inventory/tool/construction state rolled back, report failure, and produce
      no owner gain receipt. A successful retry grants only the exact accepted
      reward and produces one accurate receipt. A remote owner receives the
      receipt while observers receive no private inventory or gain detail.
    **Focused validation:** Extend the isolated source/gemstone transaction
      automation and add receipt inspection for tool craft/refuel and construction
      save failures, retry, and state rollback; then run a listen-server
      remote-owner scenario with existing harvest/gemstone coverage after an
      Editor build.

  - [ ] **RF44 — Add remote Opal Helm Forge-maintenance coverage (P3; CR04-018).**
    **Source:** CR04-018; `Source/KalmalaGameplay/Private/Tests/KalmalaOpalHelmTest.cpp:13-79`;
      `Source/KalmalaGameplay/Private/KalmalaOpalHelm.cpp:59-89`;
      `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:407-415`;
      `Source/KalmalaGameplay/Public/KalmalaInventoryComponent.h:37-45`;
      `Scripts/Tests/Inventory/ItemPeers.ps1:4-48`; `Scripts/Tests/Progression/GemMiningTreasurePeer.ps1:33`;
      `docs/07-development-setup.md:4932-4935`.
    **Affected path:** An owning remote client upgrading or repairing its equipped Opal Helm at a Forge while
      the server maintains the gear level and owner-only result.
    **Trigger:** Treating the standalone `Kalmala.Gameplay.Inventory.OpalHelm` fixture as evidence for the
      client RPC route. It calls `MaintainOpalHelmFromServer` and inventory methods directly, not
      `ServerUpgradeOpalHelm` or `ServerRepairOpalHelm`. The existing item-peer scenario list has no Opal Helm
      route; the A9 peer case checks accessory stats, and the setup guide specifies an Opal inventory render for
      the maintenance controls.
    **Impact:** Regressions in remote RPC ownership routing, the crafting request cadence, owner result
      delivery, or the level update through owner-only equipped gear can go undetected. The inspected server
      path validates authority, exact equipped GUID, same-world visible Forge/range/level, material costs, and a
      200 ms request cadence; no current authority bypass was found.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf44--add-remote-opal-helm-forge-maintenance-coverage-p3.
    **Proposed repair:** Add a listen-server scenario with a remote owner and observer that invokes the actual
      upgrade and repair RPCs. Reuse the local test's catalogue/cost checks while exercising Forge level,
      range/visibility, exact GUID, insufficient-material, and 200 ms cadence rejection through the network
      path. Verify owner feedback and updated level/condition, and retain the existing observer assertion that
      equipped gear stays private.
    **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize
      implementation during CR04.
    **Acceptance:** The remote owner upgrades with the authored payment at the matching Forge level and repairs
      worn Opal Helm for free at a valid Forge. Wrong or foreign GUID, wrong Forge level, distant/occluded
      Forge, insufficient material, and too-rapid repeat requests do not change level, condition, or payment.
      The owner receives the updated gear and one owner-only result; the observer receives no private
      equipped-gear data.
    **Focused validation:** Build the Editor target; run `Kalmala.Gameplay.Inventory.OpalHelm`, the new
      three-role Opal Helm peer scenario, and the existing `Scripts/Tests/Inventory/Inventory.ps1` owner-only
      inventory check.

  - [ ] **RF45 — Verify detailed skill replication privacy with an observer (P3; CR04-020).**
    **Source:** CR04-020; `Source/KalmalaGameplay/Private/KalmalaSkillProgressionComponent.cpp:46-67`,
      `Source/KalmalaGameplay/Private/Tests/KalmalaSkillProgressionReplicationTest.cpp:11-46`, and
      `Scripts/Tests/Construction/Crafting.ps1:22-46`.
    **Affected path:** The server-owned `UKalmalaSkillProgressionComponent` on a replicated player pawn.
    **Trigger:** A regression changes the replication condition for `DetailedProgression` or stops delivering
      its detailed ledger to the owning player while retaining the aggregate `PeerPresentation`.
    **Impact:** Other relevant players could receive another player's per-skill experience, levels and unlocks,
      or the owner could lose progression detail while the public badge still looks correct. The current
      component uses `COND_OwnerOnly` for `DetailedProgression`; no existing disclosure was observed.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf45--verify-detailed-skill-replication-privacy-with-an-observer-p3.
    **Proposed repair:** Add a listen-host, remote-owner and observer scenario that earns skill experience
      through an accepted server action. Verify that the owner receives the exact detailed ledger, the observer
      receives no per-skill entries or experience, and both relevant peers receive only the aggregate badge.
    **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize
      implementation during CR04.
    **Acceptance:** In a three-role peer run, accepted progression updates the remote owner's detailed six-skill
      state; the observer has no detailed state for that pawn; both peers receive only the validated
      highest-level/unlock presentation; no client-authored skill outcome is accepted.
    **Focused validation:** Build the Editor target, run `Kalmala.Gameplay.Progression.ReplicationContract` and
      `Kalmala.Gameplay.Progression.SkillContract`, then run the new three-role skill privacy scenario and the
      existing crafting owner presentation check.

  - [ ] **RF46 — Cover remaining wildlife defeat reward callbacks (P3; CR04-021 and CR04-022).**
    **Source:** CR04-021 and CR04-022; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-583`;
      `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8677-8814`;
      `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBadgerTest.cpp:91-147`;
      `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMoonBearTest.cpp:97-120`;
      `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeGiantSnakeTest.cpp:102-179`;
      `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeDeathAdderTest.cpp:124-227`;
      `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp:160-169`;
      `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeViperTest.cpp:89-95,148-150`;
      `Scripts/Tests/Combat/EnemyPeers.ps1:4-33,156-166`; `docs/07-development-setup.md:2401-2493,2612-2660`.
    **Affected path:** Badger, Moon Bear, Giant Snake, Death Adder, Peat Husk and Viper defeat dispatch through
      `AKalmalaWildlifeSpawn::DefeatServer`, their GameMode reward callbacks, owner-bound pending claims and
      persistent defeated-spawn state.
    **Trigger:** A callback mapping or handler change omits a species reward, supplies the wrong stable spawn ID
      or authenticated owner, or persists defeat before the claim is safely queued. Focused source tests still
      pass because they call `TryQueuePending*Reward` directly.
    **Impact:** A species can be defeated without the expected owner-bound reward claim or correct persistent
      defeat record, while its helper-level contract remains green. The shared Rat peer scenario covers generic
      reward retry, persistence and peer privacy but not these six callback mappings. RF18 tracks the same test
      gap for Wolf, Lynx, Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw. The inspected server path
      validates authority and identity; no current bypass or live failure was established.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf46--cover-remaining-wildlife-defeat-reward-callbacks-p3.
    **Proposed repair:** Add isolated GameMode/`DefeatServer` automation for the six species using a test save
      writer or isolated save root. Assert stable spawn identity, authenticated owner, exact claim outcome,
      queue-before-defeat persistence order, retry idempotency and rejection of mismatched identity/world cases.
    **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize
      implementation during CR04.
    **Acceptance:** Each of the six actual species defeat dispatches creates one owner-bound claim with the
      expected persistent spawn ID and outcome, then persists the matching defeated ID. Retry is idempotent and
      preserves any optional roll; invalid world, identity or queue cases create no misplaced claim and do not
      write a false defeat record.
    **Focused validation:** Build the Editor target, run the new isolated callback automation and the six
      existing species contract automations. Use the existing Rat peer scenario for the shared
      delivery/restart/privacy path; a separate per-species peer scenario is not required for those shared
      checks.

  - [ ] **RF47 — Exercise finite-world movement bounds through remote prediction (P3; CR04-022/CR16-031).**
    `KalmalaWorldBoundaryMovementTest` currently checks an authority pawn and
    manually invokes the movement update; the peer runner does not approach
    the finite edge with a remote owner. Extend the listen-server movement
    scenario with an owner near the boundary and an observer. Drive outward
    through normal input and verify the server and owner converge within the
    radius-minus-capsule margin while tangential and vertical movement remain
    available. Acceptance: neither owner nor server crosses the radius after
    correction; outward velocity is removed, tangential/vertical components
    remain, and the observer sees the same bounded transform. Dependencies:
    after CR19 review acceptance. Focused validation: build the Editor target,
    run `Kalmala.Gameplay.Movement.WorldBoundary`, and run the extended remote
    owner/observer scenario through `Scripts/Tests/Presentation/PlayerControls.ps1`.
    Evidence: `Temporary/Review/findings.md#rf47--exercise-finite-world-movement-bounds-through-remote-prediction-p3`.

  - [ ] **RF48 — Reject ineligible melee requests before target traces (P3; CR04-023).**
    **Source:** CR04-023; Source/KalmalaGameplay/Public/KalmalaCombatComponent.h:25,47-60;
      Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:15-20,34-56,171-203; RF42 covers the separate
      ServerRequestInteract trace path.
    **Affected path:** A remote owning client's reliable ServerRequestAttack request and the server's melee
      target selection.
    **Trigger:** A client sends stale-sequence requests, or fresh requests while an action is already
      unavailable. The handler resolves the held item and calls FindServerTarget() before IsAttackRequestAllowed
      checks server authority, sequence freshness, idle/bow state and swing availability. FindServerTarget() can
      execute both a visibility line trace and a 60 cm lower sphere sweep; the attack is rejected only
      afterward. The unavailable-feedback timer limits feedback publication, not these scene queries.
    **Impact:** Rejected reliable RPCs can still consume authoritative scene-query work. A client able to send a
      burst can amplify server CPU cost. The inspected ordering does not allow the request to choose a target or
      bypass hit/damage validation; no combat authority bypass was established.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf48--reject-ineligible-melee-requests-before-target-traces-p3.
    **Proposed repair:** Run the inexpensive authority, sequence, action-phase, bow-draw and swing-availability
      checks before target selection. Only eligible requests should perform the target trace/sweep; preserve the
      existing server-side target validation and impact-time checks. Add request cadence only if the reordered
      admission still permits unbounded query work.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation during CR04.
    **Acceptance:** Stale, busy, bow-drawing, unavailable-swing and non-authority requests perform no target
      trace or sweep. A fresh eligible request performs the expected target query, and subsequent impact still
      accepts only a server-validated target with canonical damage.
    **Focused validation:** Extend the combat request contract test to assert trace count for rejected versus
      eligible requests. After an Editor build, run the combat action/item acceptance tests and a remote-owner
      burst with a server trace counter; verify ordinary attack timing and target/damage gates remain intact.

  - [ ] **RF49 — Bound repeated Home-return persistence attempts after save failure (P3; CR04-023).**
    **Source:** CR04-023; Source/KalmalaGameplay/Public/KalmalaCharacter.h:299-303;
      Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1645-1661;
      Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9520-9585,9635-9654;
      docs/02-technical-architecture.md:324-335; docs/startmenu.md:5-13.
    **Affected path:** A remote client's reliable ServerRequestReturnToHome request while the server cannot
      persist that player's current state.
    **Trigger:** TryReturnNetworkClientToHomeFromServer adds the controller to PendingNetworkHomeReturns,
      synchronously calls PersistPlayerStateFromServer, then removes the marker when persistence fails so the
      documented client retry can work. A subsequent request repeats copying the current save through
      SaveGameToMemory/LoadGameFromMemory, rebuilding and validating the candidate, and attempting
      SaveGameToSlot. There is no per-controller retry interval on this failure branch.
    **Impact:** During a persistent storage failure, a client request burst can repeatedly perform serialization
      and disk-write work on the authoritative server and send repeated failure replies, increasing host CPU/I/O
      pressure. The first successful request remains protected by the pending-return marker. No save ownership
      bypass was established, and the documented retry behavior is valid.
    **Evidence:** Full evidence:
      Temporary/Review/findings.md#rf49--bound-repeated-home-return-persistence-attempts-after-save-failure-p3.
    **Proposed repair:** Keep the first save attempt immediate and preserve eventual client retry, but apply a
      server-owned per-controller backoff to repeated attempts after failure. Return a lightweight rejection
      during the backoff without rebuilding or writing another save candidate.
    **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize
      implementation during CR04.
    **Acceptance:** The first request attempts persistence and a failed write leaves the client connected.
      Repeated requests inside the retry interval do not repeat candidate serialization or slot writes; a later
      request can retry, and success retains the existing one-return guard.
    **Focused validation:** Add a save-failure automation that counts candidate/write attempts across repeated
      requests, verifies one later retry succeeds, and confirms successful returns remain idempotently guarded.
      Then run the existing client Home-return peer path with injected failure and recovery after an Editor
      build.

  - [ ] **RF50 — Retire the remaining map-sharing RPC (P3; CR04-025).**
    **Source:** CR04-025; `Source/KalmalaGameplay/Public/KalmalaMapAwarenessComponent.h:24-71`;
      `Source/KalmalaGameplay/Private/KalmalaMapAwarenessComponent.cpp:12-15,48-62,143-172`;
      `Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:330-334,457-464`;
      `docs/02-technical-architecture.md:310`.
    **Affected path:** A remote owning client invokes `ServerSetSharingEnabled` on its replicated
      player-controller component; the server writes `bMapSharingEnabled` on that player's state and forces an
      update to peers.
    **Trigger:** A client repeatedly invokes the reliable RPC, including with the current value. The handler
      checks authority and player eligibility but has no cadence or duplicate-state gate and calls
      `ForceNetUpdate` on every request. The RPC and globally replicated flag remain in production code even
      though the current map contract excludes co-op sharing controls/status and remote pawn markers.
    **Impact:** A client can drive unnecessary server work and repeated PlayerState replication scheduling
      through a stale map-sharing endpoint. The old marker/status API still derives peer pawn positions and
      exposes a co-op status string. The current map widget only submits and draws temporary pings and does not
      consume those APIs, so no current player-visible marker/status disclosure was established. Request-flood
      impact was not measured.
    **Evidence:** Full evidence: Temporary/Review/findings.md#rf50--retire-the-remaining-map-sharing-rpc-p3.
    **Proposed repair:** Remove the obsolete sharing RPC, replicated consent flag, peer-marker/status helpers,
      and their old test expectations while retaining the independent validated ping path. If a future approved
      feature restores location sharing, define its privacy contract and server-owned request cadence before
      adding a client request surface.
    **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize
      implementation during CR04.
    **Acceptance:** Current builds expose no client-callable map-sharing RPC or globally replicated sharing
      state, and the map presents only bounded temporary pings. A remote-client check confirms no remote marker
      or co-op status is exposed while valid pings still use the existing two-second cooldown, lifetime, inbox
      bound and owner-only delivery.
    **Focused validation:** Update the map-awareness automation to remove the legacy sharing scenario and assert
      the current no-sharing contract. After an Editor build, run `Scripts/Tests/World/MapAwareness.ps1` and a
      remote-owner/observer map check for ping validation, cooldown, expiry, inbox bounds and owner-only
      delivery; statically confirm no production call sites remain for sharing APIs.

  - [ ] **RF51 — Bound repeated arrow-preference RPC work (P3; CR04-032).**
    `Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:1431`,
    `Source/KalmalaGameplay/Public/KalmalaCombatComponent.h:32,55`,
    `Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp:118-128`,
    `Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:254`, and
    `Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp:67-71`.
    **Affected path:** A remote owning client selects or clears its preferred
    arrow stack through the inventory menu; the server stores an owner-only
    preference used for bow ammunition selection.
    **Trigger:** Repeated reliable ServerPreferArrowStack calls with a valid
    owned arrow GUID, or repeated clears, are processed without a request
    cadence or duplicate-state check. Valid requests scan the server inventory
    and every accepted selection or clear calls ForceNetUpdate.
    **Evidence:** The server verifies authority and resolves a supplied unit or
    stack GUID against supported arrow inventory stacks. An invalid GUID is the
    menu's clear request and always invalidates the preference and forces an
    update. The preference replicates with COND_OwnerOnly. The existing
    Kalmala.Gameplay.Combat.BowDrawAndArrows test directly exercises the
    handler in a local authority world; it does not exercise remote RPC cadence.
    **Proposed repair:** Add a server-owned cadence for this preference RPC and
    skip replication scheduling when the resolved value is unchanged, while
    keeping the first selection or clear responsive.
    **Dependencies:** After CR19 review acceptance; this is a review handoff and
    does not authorize implementation during CR04.
    **Acceptance:** The first remote-owner selection or clear is applied
    immediately. Repeated and alternating requests inside the configured
    cadence do not repeat inventory resolution or ForceNetUpdate; a request
    after the cadence can change the preference. The server still accepts only
    a GUID belonging to a supported owned arrow stack, and the preference
    remains owner-only.
    **Focused validation:** Extend Kalmala.Gameplay.Combat.BowDrawAndArrows
    for selection, clear, duplicate and cadence behavior; after an Editor
    build, add or run a remote-owner/observer check for the actual RPC route,
    owner preference update and observer privacy.
  - [ ] **RF52 — Verify remote ownership of world-seed selection (P3; CR04-039).**
    `Source/KalmalaGameplay/Public/KalmalaMenuContext.h:18,28-35`,
    `Source/KalmalaGameplay/Private/KalmalaMenuContext.cpp:34-73,115-142`,
    `Source/KalmalaGameplay/Private/Tests/KalmalaMenuContextTest.cpp:8-19`,
    `Source/KalmalaUI/Private/Tests/KalmalaWorldSeedParserTest.cpp:8-38`, and
    `docs/startmenu.md:294-320`.
    **Affected path:** A remote menu controller invokes the reliable
    `ServerRequestWorldSeedSelection` RPC. If accepted, the GameMode enumerates
    server-local save names, resolves whether the requested seed is an existing
    current-revision identity, and stores a pending selection.
    **Trigger:** A future regression removes or bypasses the local-controller
    requirement in `ProcessWorldSeedSelectionRequest`, or otherwise lets a
    remote controller reach `TrySelectWorldSeedFromServer`. The parser and
    menu setup tests would still pass.
    **Evidence:** The server currently requires authority and a local controller
    before server-local save resolution, and rejects a remote request with a
    generic result. The selected parser test covers decimal parsing, while the
    menu context test checks setup and does not invoke the selection RPC. A
    targeted source/test/script search found no remote-controller regression
    test. The start-menu contract says only the server-local menu controller
    may select a seed. No current bypass or disclosure was observed.
    **Impact:** If the guard regresses, a remote client could infer whether a
    server-local seed is listed as a current-revision world and alter the
    GameMode pending selection.
    **Proposed repair:** Add Development-only replicated-controller coverage
    that invokes the actual reliable RPC from a remote menu controller while
    the server has both an unlisted seed and an exact current-revision saved
    seed. Assert both remote requests are rejected before save enumeration, the
    reply reveals no save classification, and pending selection is unchanged.
    Keep a local-controller case proving valid new-world and confirmed-reuse
    selections still work.
    **Dependencies:** After CR19 review acceptance; this review handoff does not
    authorize implementation during CR04.
    **Acceptance:** A remote controller cannot enumerate or distinguish
    server-local world-seed records or change pending selection, and receives
    the generic rejection. The local controller can select an unlisted seed or
    confirm an exact current-revision identity, and launch still revalidates
    the exact seed and confirmation.
    **Focused validation:** After an Editor build, run the focused
    `Kalmala.Gameplay.MenuContext.WorldSeedSelection` automation and a
    Development-only remote-controller scenario; also run
    `Kalmala.UI.WorldSeedParser` for decimal and uint64-boundary coverage.
  - [ ] **RF53 — Reconcile Automatic Pickup with the settings authority contract (P3; CR04-042).**
    `Source/KalmalaUI/Private/KalmalaSettingsSubsystem.cpp:139-166`,
    `Source/KalmalaUI/Private/KalmalaSettingsWidget.cpp:1606-1618,2258-2263`,
    `Source/KalmalaGameplay/Public/KalmalaCharacter.h:74-78`,
    `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1555-1573`,
    `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:588-607`,
    `docs/02-technical-architecture.md:2648-2663` and
    `docs/07-development-setup.md:346-365`.
    **Affected path:** The locally persisted Accessibility > Automatic pickup
    choice is sent to the owning character, whose server-side tick scans nearby
    loose materials and broken-tree bundles.
    **Trigger:** The architecture says settings send no RPC and cannot change
    inventory, although the Settings subsystem sends
    `ServerSetAutomaticPickupEnabled` and the server uses that preference to
    enable accepted pickups.
    **Evidence:** The setting persists in `GGameUserSettingsIni`; the RPC carries
    only a boolean. The server selects eligible drops and validates owner,
    same-world state, the 150 cm radius, visibility and capacity before grant.
    The preference is not replicated. Development setup documents this flow,
    but the architecture's local-settings boundary contradicts it. No current
    authority bypass was found.
    **Proposed repair:** Limit the architecture's no-RPC/no-gameplay claim to
    presentation and input preferences, and document Automatic pickup as the
    explicit local-persisted owner preference. Preserve server-owned candidate
    selection, checks and grants, and align the settings acceptance wording.
    **Dependencies:** After CR19 review acceptance; this handoff does not
    authorize implementation during CR04.
    **Acceptance:** The architecture and development setup describe the same
    Automatic pickup RPC and authority boundary; clients cannot select drops or
    grant quantities, and no replicated preference or save field is added.
    **Focused validation:** Compare both contract sections with the source and
    run `Kalmala.UI.Settings.AutomaticPickup` and
    `Kalmala.Gameplay.Pickup.AutomaticRadius` if code or test expectations change.
  - [ ] **RF54 — Reconcile the current HUD ownership contract (P3; CR04-043).**
    `Source/KalmalaUI/Public/KalmalaSupportSelectionSubsystem.h:84-85`,
    `Source/KalmalaUI/Private/KalmalaWeatherActivitySubsystem.cpp:32-38,229-232`,
    `Source/KalmalaUI/Private/KalmalaSurvivalStatusWidget.cpp:263-268`,
    `Source/KalmalaUI/Private/KalmalaStatusHotbarWidget.cpp:381-400`,
    `docs/02-technical-architecture.md:2774-2775` and
    `docs/07-development-setup.md:2898-2903,2959,3999-4002`.
    **Affected path:** The active UI ownership ledger and HUD verification
    guidance describe a full lower-left survival strip, a selected-support strip,
    and a separate weather-tier badge. Runtime keeps lower-left ocean-travel
    feedback, puts active status and Storm icons in the top-right hotbar, and
    retires the two standalone presenters.
    **Trigger:** A developer follows the current ownership or capture contract
    and expects to display or verify the retired widgets.
    **Impact:** The documented UI and acceptance path diverge from the current
    runtime layout, so future changes may target a surface that is not active.
    **Evidence:** `UKalmalaSurvivalStatusWidget::SetSnapshot` uses only
    `BuildOceanTravelText`; `UKalmalaSupportSelectionSubsystem::IsTickable`
    returns false with an M13 scroll-migration comment; and
    `UKalmalaWeatherActivitySubsystem::Tick` releases its widget without creating
    or updating it. The status hotbar supplies the active support and Storm
    entries. The architecture ownership ledger still lists all three standalone
    surfaces as current.
    **Proposed repair:** Reconcile current ownership and M11/M13 verification
    guidance with the active HUD and reusable support-scroll path. Label older
    M7/M12 presenter notes as historical or superseded where needed, while
    retaining the local weather fog/material response contract. Do not imply a
    new HUD feature.
    **Dependencies:** After CR19 review acceptance; this is a documentation
    handoff and does not authorize implementation during CR04.
    **Acceptance:** Current architecture and setup guidance describe the
    lower-left travel-only panel, the single active top-right status/weather
    hotbar, and no standalone selected-support strip or weather-tier badge.
    They retain the local fog/material response and mark older presenter
    verification as historical. No gameplay, network, or save behavior changes.
    **Focused validation:** Compare the revised current-contract sections with
    the widget lifecycle and hotbar sources. Run
    `Scripts/Tests/Acceptance/DocumentationContracts.ps1` if its assertions
    cover the revised wording; no Unreal build is needed for a docs-only repair.
  - [ ] **RF55 — Reject oversized pending population rewards on load (P3; CR04-047).**
    `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:41-48,99-116`,
    `Source/KalmalaWorld/Public/KalmalaWorldPopulationSaveGame.h:48`, and
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:647-658,7829-7833,9046-9053`.
    **Affected path:** The server loads a same-world schema-2 population save
    and retries pending wildlife rewards.
    **Trigger:** A structurally loadable slot contains more than the declared
    4,096 pending reward rows. Normal queue mutations enforce the limit, but
    `MatchesWorld` omits it, so `LoadPopulationSave` can retain the oversized
    record and GameMode copies/scans every row on a 0.5-second retry interval.
    No client-controlled path to create the over-limit record was found.
    **Impact:** Malformed or tampered local save data bypasses the intended row
    bound and causes repeated server allocation and validation work; this is not
    a demonstrated remote exploit.
    **Evidence:** `TryQueuePendingWildlifeReward` rejects additions at 4,096,
    while `MatchesWorld` validates other save caps but never checks the pending
    reward count. The loader trusts `MatchesWorld`; the authoritative retry
    loop copies the accepted array and visits every row.
    **Proposed repair:** Enforce the pending-row cap in load validation and
    preserve rejected slot bytes without rewriting them. Keep normal owner-bound
    retry and full-pack delivery behavior unchanged.
    **Dependencies:** After CR19 review acceptance; this handoff does not
    authorize implementation during CR04.
    **Acceptance:** A same-world schema-2 record with 4,096 pending rows remains
    valid; 4,097 rows are rejected before retention or delivery scans, without
    modifying the existing slot. Valid in-bound owner claims still retry.
    **Focused validation:** Extend the population-save contract automation with
    a serialized over-limit record and the 4,096/4,097 boundary; verify rejected
    slot bytes remain unchanged. No test was run during this review.
  - [ ] **RF56 — Cover the M7 sparse-delta count bound (P3; CR04-048).**
    `Source/KalmalaWorld/Private/Tests/KalmalaM7PersistenceContractTest.cpp:36-52`,
    `Source/KalmalaWorld/Public/KalmalaM7PersistenceContract.h:89`, and
    `Source/KalmalaWorld/Private/KalmalaM7PersistenceContract.cpp:110,143-155`.
    **Affected path:** The M7 identity-scoped save accepts and reloads sparse
    world/player facts under the documented 256-row bound.
    **Trigger:** A future change removes or weakens the count checks while the
    current focused contract test still passes because it adds and reloads only
    three rows.
    **Impact:** The sparse-record resource limit has no boundary regression
    guard. No current production bypass was confirmed; load and insertion paths
    currently enforce the cap.
    **Evidence:** `MaxSparseDeltas` is 256; `Matches` rejects an over-cap loaded
    array, and `AddSparseDelta` rejects additions at the cap. The test covers
    three rows, duplicate/invalid IDs, and round-trip, but not 256/257 or an
    over-cap load. Sparse IDs are server-selected.
    **Proposed repair:** Extend `Kalmala.World.M7.PersistenceContract` with the
    256/257 boundary, unchanged count on rejection, and a valid maximum-size
    round trip. Add an over-cap load case if the fixture can create one without
    writing a project save slot.
    **Dependencies:** After CR19 review acceptance; this handoff does not
    authorize implementation during CR04.
    **Acceptance:** A current M7 save with 256 valid unique sparse deltas remains
    valid; the 257th addition is rejected without changing the count; the valid
    maximum survives in-memory serialization/reload. If feasible through the
    existing fixture, an over-cap record is rejected by `Matches`.
    **Focused validation:** Run `Kalmala.World.M7.PersistenceContract` after an
    editor build. No test was run during this review.
  - [ ] **RF57 — Correct the generated population save version in the architecture table (P3; CR04-048).**
    `docs/02-technical-architecture.md:1415`,
    `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:30-43`,
    and `docs/05-decision-log.md:209`.
    **Affected path:** A developer consults the current save-version table
    before inspecting, rejecting, or testing generated population saves.
    **Trigger:** The table says `UKalmalaWorldPopulationSaveGame` uses schema 1,
    but `InitializeForWorld` writes schema 2 and `MatchesWorld` accepts only
    schema 2. The M7 sparse ledger is a separate save owner and remains schema 1.
    **Impact:** The authoritative save-version reference gives the wrong current
    format and can lead to incorrect development-save or test expectations.
    This is documentation drift; no runtime mismatch or remote exploit was found.
    **Evidence:** The architecture table says schema 1 for generated population
    deltas; the active code initializes and validates schema 2; the decision log
    records approval of population save schema 2.
    **Proposed repair:** Update the generated-population row to schema 2 and
    keep it clearly separate from the M7 sparse ledger's schema 1. Retain the
    unreleased no-migration policy and reject schema-1 population saves without
    rewriting them.
    **Dependencies:** After CR19 review acceptance; this handoff does not
    authorize implementation during CR04.
    **Acceptance:** The current-version table identifies generated population
    saves as schema 2 and M7 sparse persistence as schema 1; it agrees with
    runtime initialization/validation and the decision log without migration.
    **Focused validation:** Compare both save owners' current-version constants
    with the decision-log entry; no Unreal build is needed for a docs-only repair.
  - [ ] **RF58 — Reconcile rejected Camp Bed interactions with the local rest pose (P3; CR05-001).**
    Affected: `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1576-1589,1687-1782,1969-1971`,
    `Source/KalmalaGameplay/Private/KalmalaPlayerModelComponent.cpp:197-200,255-284`,
    `Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:118-132`,
    and `docs/02-technical-architecture.md:520`.
    **Affected path:** An owning client interacts with a Camp Bed and renders
    the local rest pose.
    **Trigger:** Local `RequestInteract` starts the pose when its selected bed passes
    `CanUse`, before the server's independent trace and interactable checks and
    the crafting `AcceptRequest` gate accept the action. A missed target or a
    server-side rejection has no pose cancellation path; the pose stops only
    after movement, health loss, or loss of local control.
    **Impact:** The local character can appear to rest even when the server did
    not accept the Rest action, and may stay posed indefinitely while stationary
    and undamaged. This is a presentation mismatch, not a gameplay authority bypass.
    **Evidence:** The client calls `StartCampBedRestPose` before sending the interaction
    RPC. The server retraces and dispatches only a current interactable target;
    the Camp Bed branch publishes acceptance only after the crafting request
    gate. `StopCampBedRestPose` is otherwise called only by model tick when its
    movement, health, or local-control condition fails. The architecture says
    the pose follows an accepted Rest action. Existing RF42 covers repeated
    interaction RPC work and is a separate issue.
    **Proposed repair:** Start the pose after the owner receives the server's
    accepted Rest result, or give each pending local pose a bounded prediction
    window that reverts on rejection or timeout. Keep movement and health as
    ways to end an accepted pose.
    **Dependencies:** After CR19 review acceptance; no fix during the review-only phase.
    **Acceptance:** An accepted server Rest action poses the local model; a
    missed trace, invalidated bed, rate-rejected action, or missing response
    leaves the original model transform restored without movement or damage.
    An accepted pose still ends on movement or health loss.
    **Focused validation:** Add a focused accepted/rejected interaction result
    check and a listen-host/remote-owner scenario. Verify the rejected and
    timed-out cases restore the original model transform. No runtime check was
    run during this review.
  - [ ] **RF59 — Keep skiff blockage details private to the helmsman (P3; CR05-002).**
    Affected: `Source/KalmalaGameplay/Public/KalmalaOceanSkiff.h:23-29,62,102-103`,
    `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:80-86,283-292`,
    and `docs/02-technical-architecture.md:1220,1226,1273`.
    **Affected path:** Relevant peers receive replication for a blocked skiff.
    **Trigger:** The server selects a specific blockage reason and the skiff
    exposes it through `GetBlockReason` and an unconditionally replicated
    property.
    **Impact:** Relevant passengers and observers can distinguish an invalid
    ocean footprint, generated-terrain collision, or sweep-limit response for
    another player's helm. The travel contract allows the blocked mode and
    movement to relevant peers, but limits transient failure feedback to the
    requester and public vessel state to movement, mode, and occupancy. This
    is excess replicated detail, not a gameplay authority bypass.
    **Evidence:** `BlockMovementAtLastSafeTransform` sets `BlockReason`;
    `GetLifetimeReplicatedProps` registers it with unconditional
    `DOREPLIFETIME`. The existing travel feedback component replicates its
    feedback owner-only.
    **Proposed repair:** Keep the specific reason server-local or deliver it
    through the existing owner-only travel feedback path. Preserve public
    blocked mode and movement replication.
    **Dependencies:** After CR19 review acceptance; no fix during the review-only phase.
    **Acceptance:** The helmsman receives the specific failure reason; relevant
    passengers and observers receive the blocked mode and movement but cannot
    read that reason.
    **Focused validation:** Exercise a blocked movement with a helmsman,
    passenger, and observer in a listen-server session. Confirm the detailed
    result reaches only the helmsman while public mode and movement still
    replicate. No runtime check was run during this review.
  - [ ] **RF60 — Repair the selected carried-tool instance (P3; CR06-001).**
    `Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:443-463,1288-1328`,
    `Source/KalmalaGameplay/Public/KalmalaCraftingComponent.h:47`,
    `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:657-693`,
    `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1996-2008,2060-2073`,
    and `docs/05-decision-log.md:66-67`.
    **Affected path:** The owner selects one duplicate carried tool and requests
    an individual repair.
    **Trigger:** The UI validates the representative copy by GUID but sends only
    its item ID. The server resolves that ID to the active matching copy first,
    then the first matching copy in carried state.
    **Impact:** Individual repair can change a different copy or report that the
    tool is already full while the selected duplicate remains worn. Repair All
    is a workaround at a valid station; authority and station checks still hold.
    **Evidence:** Inventory rows group duplicate tools by ID while retaining a
    GUID; `RepairSelectedTool` validates that GUID then sends `Row.Id`. The RPC
    accepts only an item ID, and `FindToolDurabilityFromServer(FName)` prefers
    the active same-type GUID. Duplicate GUID-addressed tool copies are allowed.
    **Proposed repair:** Send the selected GUID and validate that exact carried
    tool on the server, or make the UI explicitly present and repair the chosen
    server-resolved copy.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** With duplicates at different condition, individual repair
    changes only the selected GUID. An invalid/unowned GUID, unavailable
    station, failed persistence, or full selected copy changes no condition.
    **Focused validation:** Add a regression with an active full copy and a worn
    selected spare, then reverse conditions; verify selected-only repair and
    unchanged state on rejection. No test was run during this review.
  - [ ] **RF61 — Reconcile the current Bow feature summary (P3; CR06-001).**
    `docs/game-features.md:9-25,102-112`,
    `docs/01-game-design.md:3-25,60-81`,
    `Content/Data/GameCatalogues/recipes/stations/workbench.json:18-68,293-355`,
    `Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp:133-194`, and
    `Source/KalmalaGameplay/Private/KalmalaArrowProjectile.cpp:55-105`.
    **Affected path:** A reader uses the Bow summary to learn current materials,
    ammunition, or shooting behavior.
    **Trigger:** The summary says only Wooden Arrows are available, Bow String
    has no source, Bow costs 5 Wood plus 2 generic String, and shooting is not
    implemented, despite the current design and runtime contract.
    **Impact:** The feature reference describes obsolete costs and omits
    implemented arrow types and shooting. This is documentation drift; no
    runtime defect was found.
    **Evidence:** Current design and recipe data define 4 Sinew to 1 Bow String,
    Bow for 5 Wood plus 1 Bow String, and Wooden/Stone/Bronze Arrow recipes.
    BowCombat and ArrowProjectile implement server draw/release shots.
    **Proposed repair:** Reconcile or consolidate the summary with current design
    and catalogue contracts; label retained historical text as superseded.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** Current text matches Bow String sourcing/costs, available
    arrow types, and server draw/release shooting; historical details are
    clearly marked so they cannot be mistaken for current behavior.
    **Focused validation:** Compare the summary with current design, registered
    Workbench recipes, and shot/projectile paths. Run the documentation
    contract checker if it covers the Bow section; no Unreal build is needed.
    No test was run during this review.
  - [ ] **RF62 — Enable authored Workbench upgrades for Stone starter tools (P2; CR06-002).**
    `Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp:2210-2283`,
    `Source/KalmalaGameplay/Public/KalmalaCraftingComponent.h:50`,
    `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:376-386`,
    `Source/KalmalaGameplay/Private/KalmalaToolProgressionContract.cpp:410-417`,
    `Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolStationProgressionTest.cpp:36-37,77-100`,
    the Stone entries in `Content/Data/GameCatalogues/item-level-up/costs/weapons/`,
    `docs/02-technical-architecture.md:3278`, and `docs/07-development-setup.md:5179`.
    **Affected path:** A player with a level-one Stone Axe, Knife, Mace, Pick, or
    Spear attempts its authored level-two Workbench upgrade.
    **Trigger:** The Stone Workbench UI resolves only `FindEntry(ItemId, 1)` and
    enables acquisition only while that tool is not carried. Its request sends
    only the item ID. Both the server progression path and the shared upgrade
    builder special-case Starter profiles to resolve level one instead of the
    next entry for the carried level.
    **Impact:** All five authored Stone starter-tool upgrades are unreachable
    through the Workbench. Their documented material costs and level rewards
    cannot be earned. No authority bypass was found; station and payment checks
    remain server-side.
    **Evidence:** Upgrade-cost JSON contains Workbench level-one upgrade records
    for all five Stone IDs; the progression catalogue derives same-item level-two
    entries. Architecture says Stone upgrades use Workbench, and the setup
    contract records level-up stats for Stone Axe. `RefreshStoneCraftState`
    gates its action on `GetCarriedToolLevel(ItemId) == 0`; both
    `ProgressToolFromServer` and `BuildServerUpgrade` select the Starter
    level-one entry regardless of carried level. The focused station transaction
    test exercises Bronze Axe progression, not a Stone upgrade.
    **Proposed repair:** Separate level-one acquisition intent from same-item
    upgrade intent in the Workbench UI/request path. Offer each Stone level-two
    upgrade when its level-one tool is carried, then let the server resolve and
    validate the exact predecessor/target step while retaining station, range,
    materials, capacity, persistence, and authority checks.
    **Dependencies:** After CR19 review acceptance; no repair during the
    review-only phase.
    **Acceptance:** Each of the five carried level-one Stone tools can be
    upgraded at the required Workbench with its authored materials and receives
    the catalogued level-two stats. Missing materials, wrong station, absent or
    wrong-level predecessor, and failed persistence leave tool and inventory
    unchanged. Level-one acquisition remains available for an uncarried item;
    clients cannot choose costs or target levels.
    **Focused validation:** Extend `Kalmala.Gameplay.M9.ToolStationProgression`
    to cover all five Stone level-one-to-two transactions, exact costs, retained
    condition/identity, and rejection rollback, while preserving the acquisition
    case. Run the focused automation after the required Editor build. No test was
    run during this review.
  - [ ] **RF63 — Align campfire warmth coverage with normalized heat (P3; CR07-003).**
    Source/KalmalaGameplay/Public/KalmalaCampfireWeatherResponse.h:35-46,
    Source/KalmalaGameplay/Private/Tests/KalmalaInteractionAuthorityTest.cpp:143-149,
    Source/KalmalaGameplay/Private/KalmalaCampfire.cpp:189-192,
    docs/01-game-design.md:159, and docs/02-technical-architecture.md:719.
    **Affected path:** The helper and named weather-response automation used as
    evidence for production campfire warmth.
    **Trigger:** The automation calls the unused CalculateEffectiveWarmth helper
    and asserts rain, wind, and damp fuel reduce warmth; production heat is set
    directly from Lit state.
    **Impact:** A stale assertion can pass while current production heat behavior
    regresses, and it documents superseded behavior. No live production defect
    was confirmed; current server code matches the normalized heat contract.
    **Evidence:** Production assigns EffectiveWarmth to 1.0 for Lit and 0
    otherwise. The helper has no production caller, and its storm assertion is
    only a pure-function test. Current architecture says exactly 1 for Lit,
    zero otherwise; exposed rain transitions an active fire to zero-heat
    Smouldering.
    **Proposed repair:** Remove the unused helper and replace its assertion with
    live server-derived heat coverage for Lit, Smouldering, exposed rain, and
    accepted roof transitions. Retain separate fuel-wetting assertions.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** Focused automation verifies live Lit heat is 1.0 and
    Smouldering/Extinguished heat is zero through exposed-rain and roofed
    transitions; no assertion claims weather attenuates heat while still Lit.
  - [ ] **RF64 — Retry reusable support-scroll grants after capacity rejection (P3; CR07-003).**
    Source/KalmalaGameplay/Private/KalmalaSupportMagicComponent.cpp:41-58,
    Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:426-432,
    Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-289,
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9443-9445,9506-9514,
    10078-10080,10337-10340, Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:341-358,
    and docs/02-technical-architecture.md:638.
    **Affected path:** Reusable support scrolls granted for learned discovery or
    Mireling boss entitlements and restored from the saved learned-effect set.
    **Trigger:** A reward is learned while the owner's 40-slot pack cannot fit
    its scroll. The grant returns false, but the learning API ignores that
    result and discovery feedback still reports ScrollFound. If initial restore
    also fails for capacity, the cached save lookup returns without rerunning
    entitlement restoration after a slot is freed.
    **Impact:** The entitlement remains saved but its required hotbar scroll is
    absent, preventing normal activation until a later save-load/restore attempt.
    Feedback can report success while the item is missing. No authority bypass
    was confirmed.
    **Evidence:** BuildGrant rejects pack output capacity; LearnEffectFromServer
    ignores TryGrantFromServer's result and returns success. Discovery handlers
    publish ScrollFound after learning. GetPlayerDiscoverySave reconstructs
    scrolls on the cache-miss/load path, while a matching cached save returns
    before that loop. Hotbar activation maps the current slot item to an effect.
    RF27 covers unavailable save lookup and RF41 covers remote privacy tests,
    not this persisted-entitlement/capacity delivery case.
    **Proposed repair:** Retain the entitlement and retry the item grant when
    capacity becomes available, or keep an owner-visible pending delivery that
    retries idempotently. Report item delivery complete only when the stack is
    present; add no save field and never duplicate on retry/reconnect.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** A full-pack award or restore remains retryable without
    claiming the scroll is present; freeing one slot grants exactly one usable
    hotbar scroll in the same session. Reconnect and repeat attempts do not
    duplicate it, and all entitlement, capacity, and reward decisions remain
    server-owned.
  - [ ] **RF65 — Bound owner-only status timer replication work (P3; CR08-001).**
    Cross-reference: CR17 should reuse RF65 and must not create a duplicate.
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:432 and
    Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:99-103,343-420,457-489.
    **Affected path:** Active survival-status timers on player pawns during normal
    server simulation and replication to each owning client.
    **Trigger:** GameMode advances each player pawn's status component every
    server tick. AdvanceFromServer calls ForceNetUpdate unconditionally, including when the
    array is empty; when timers are active, it decrements RemainingSeconds
    every server tick. Statuses is owner-only.
    **Impact:** Potential avoidable per-pawn owner replication scheduling and
    status timer traffic at server-tick cadence. No net-profile measurement was
    run, so CPU and bandwidth impact remain unquantified; no authority bypass
    was identified.
    **Evidence:** AKalmalaGameMode calls AdvanceFromServer(DeltaSeconds) for
    every player pawn in its server simulation loop. The status component
    registers Statuses with COND_OwnerOnly, subtracts SafeDelta from active
    status timers in Advance, and calls GetOwner()->ForceNetUpdate()
    unconditionally at the end of AdvanceFromServer. Status effects and
    expiration remain server-calculated.
    **Proposed repair:** Avoid forcing an owner replication update on every
    server tick when status state is unchanged. Use an explicit bounded timer
    replication cadence or client-side remaining-time reconstruction from
    server-authored timestamps, while preserving prompt status transitions and
    server-authoritative effect timing.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** Status timer replication follows a documented upper cadence
    bound and does not force an actor update every server tick solely to publish
    changing remaining-time floats. Owners still see accurate status activation,
    stacking, expiry, and death/recovery transitions within that bound; gameplay
    effects and expiration remain server-authoritative and status data stays
    owner-only. A representative multiplayer profile records bounded per-owner
    update frequency, replication bytes, and server scheduling cost against a
    documented budget. No net-profile measurement was run during this review.
    **Focused validation:** Extend the focused status automation for stacking,
    poison ticks, recovery, expiry and death cleanup; verify owner/observer
    privacy in a multiplayer scenario; and capture a net profile at the
    supported representative player count to establish the cadence and
    replication-work bounds. No test was run during this review.
  - [ ] **RF66 — Keep Ocean discovery candidates inside their source spatial key (P3; CR09-001c).**
    `Source/KalmalaGameplay/Private/KalmalaOceanDiscoveryCatalogue.cpp:130-138,159-176`,
    `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:61-69`,
    `Source/KalmalaWorld/Private/Tests/KalmalaWorldPlayerStartResolverTest.cpp:190-214`,
    `Source/KalmalaGameplay/Private/Tests/KalmalaOceanDiscoveryClaimTest.cpp:24-62`,
    and `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9314-9389,10083-10154`.
    **Affected path:** The server builds optional Ocean discovery descriptors,
    groups their actors by source key, and saves authenticated claims under the
    descriptor's spatial key.
    **Trigger:** A generated 16-bit X or Y sample is `0xFFFF` and the resulting
    candidate is accepted as deep Ocean. Dividing that value by `65535.0f`
    produces exactly `1.0f`, placing the candidate on the source cell's upper
    edge while retaining the source key.
    **Impact:** The descriptor location belongs to the adjacent key under the
    shared floor-based key mapper, but its deterministic actor grouping and
    claim identity still name the source key. A neighboring cell can independently
    generate its own optional discovery. This is a rare spatial ownership and
    per-cell placement mismatch; no duplicate reward or authority bypass was
    demonstrated.
    **Evidence:** `GetSpatialKey` floors each coordinate divided by the 6,400 cm
    key size. `BuildDescriptors` divides each 16-bit sample by 65535, adds the
    full key size, and stores the input key. `IsCurrentDescriptor` accepts the
    regenerated location without checking that the location maps back to the
    stored key. `RefreshOceanDiscoveries` groups actors by the supplied key,
    and claim persistence uses that same key. The shared population-layout test
    asserts generated descriptors remain in their source key, but the Ocean
    claim test checks canonical position, water depth and forged-position
    rejection without checking key containment.
    **Proposed repair:** Map sampled coordinates into a half-open key interval
    (for example, divide by 65,536 or clamp below the upper edge) and retain
    canonical descriptor validation.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** Deterministic endpoint cases and a bounded grid of accepted
    Ocean descriptors satisfy `FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(Location)) == SpatialKey`; one
    optional descriptor per source key, stable identity, claim replay rejection
    and reward behavior remain unchanged.
    **Focused validation:** Extend `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract`
    with source-key containment, including the upper 16-bit endpoint, and run it
    after the required Editor build. No test was run during this review.
  - [ ] **RF67 — Correct the current storage save schema in architecture references (P3; CR11-001).**
    `docs/02-technical-architecture.md:664,1108,1401-1402,1416,1644` and
    `Source/KalmalaGameplay/Public/KalmalaStorageSaveGame.h:23`.
    **Affected path:** A developer reads the architecture save-version reference
    before inspecting or validating chest storage saves.
    **Trigger:** Several current architecture paragraphs and the version table
    say the storage SaveGame uses schema 2, while the runtime owner initializes
    schema 3 and MatchesWorld rejects any other version. A later paragraph
    correctly says schema 3.
    **Impact:** The authoritative format reference is contradictory and can
    produce incorrect development-save or test expectations. This is
    documentation drift; no runtime defect was found.
    **Evidence:** CurrentSchemaVersion is 3; InitializeForWorld writes it and
    MatchesWorld requires it. The cited architecture passages say schema 2,
    while the current M9 persistence paragraph at line 1644 identifies schema
    3. RF57 is the separate generated-population table mismatch.
    **Proposed repair:** Correct the stale storage references and table to schema
    3 while retaining the distinct construction schema 7, player-discovery
    schema 3, and M7 sparse-ledger schema 1.
    **Dependencies:** After CR19 review acceptance; no documentation repair
    during the review-only phase.
    **Acceptance:** Current architecture references consistently identify
    storage schema 3 and agree with the runtime constant; other save owners and
    the unreleased no-migration policy remain correctly distinguished.
    **Focused validation:** Compare every current storage-version reference
    against the runtime constant and current-format acceptance text. No build
    is needed for a documentation-only repair.
  - [ ] **RF68 — Reject out-of-bounds construction records during restore (P3; CR11-001).**
    `Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:13-20,65-92`;
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-953`;
    `Source/KalmalaWorld/Public/KalmalaWorldBounds.h:8-16`; and
    `Source/KalmalaGameplay/Private/KalmalaPlacementPreview.cpp:774`.
    **Affected path:** The authoritative server restores placed construction
    when a world starts.
    **Trigger:** A finite saved transform outside the playable world radius or
    live placement margin passes IsValidRecord. RestorePersistedConstruction
    checks that record and then uses SpawnActorDeferred with
    AlwaysSpawn; it does not repeat the world-bound placement check.
    **Impact:** A malformed current-schema save can materialize an authoritative
    placed actor beyond the 1,600,000 cm playable radius, where players cannot
    reach it and world state no longer matches placement constraints. This is a
    persistence-validation defect, not a client placement authority bypass.
    **Evidence:** The construction transform helper checks finite location and
    scale plus a near-unit quaternion but does not check world bounds;
    IsValidRecord uses that helper. FKalmalaWorldBounds defines the 1,600,000 cm
    radius, and live construction placement checks it with a 300 cm margin.
    Startup restoration validates the record then spawns with AlwaysSpawn.
    Existing RF22/RF23 cover different player/world save transaction cases;
    RF24 covers skiff restore collision and does not cover construction bounds.
    **Proposed repair:** Reject persisted construction transforms outside the
    same world margin enforced by live construction placement before spawning;
    preserve the existing save unchanged on rejection.
    **Dependencies:** After CR19 review acceptance; no repair during the review.
    **Acceptance:** Current-format construction records outside the playable
    world margin are rejected on load without spawning actors or rewriting the
    slot. In-bounds records restore as before.
    **Focused validation:** Extend the construction-save contract and restore
    coverage with finite transforms beyond the radius and within the excluded
    300 cm margin, plus a valid in-bounds control. Verify rejection leaves the
    serialized slot unchanged. No test was run during this review.
  - [ ] **RF69 — Keep rejected lethal wildlife hits from leaving active zero-health actors (P2; CR09-001d-01).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-599,689-702,1083-1097`,
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8677-8694`,
    `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:99-123`,
    `Source/KalmalaWorld/Public/KalmalaWorldPopulationSaveGame.h:48`, and
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBadgerTest.cpp:91-95`.
    **Affected path:** A server accepts a lethal wildlife hit, then a credited
    species reward handler tries to queue its durable owner-bound claim before
    publishing the defeat.
    **Trigger:** The pending wildlife-reward array is at its 4,096-row limit, or
    the same stable claim is already bound to a different player identity, when
    a queue-backed species defeat is recorded.
    **Impact:** Damage has already set Health to zero. When the reward callback
    rejects, DefeatServer returns before setting the defeated flag,
    unregistering the creature, or broadcasting defeat. The server tick gates
    wildlife behavior on authority and the defeated flag, not Health, so the
    zero-health actor continues its AI and can attack. There is no automatic
    retry of this unqueued defeat; another damaging hit is needed to re-enter
    the callback after the rejection condition clears.
    **Evidence:** ApplyValidatedDamageFromServer clamps Health to zero before
    calling DefeatServer. The actor returns immediately when a species
    Record*DefeatReward handler fails, before its accepted-defeat mutations.
    For example, RecordBadgerDefeatReward returns false when
    TryQueuePendingBadgerReward rejects, and only marks/saves the spawn after
    queue acceptance. The common queue helper rejects at 4,096 pending rows and
    rejects an existing claim owned by another identity. Tick runs the
    archetype behavior while the actor is not marked defeated, without checking
    Health. Existing Badger automation checks queue-model idempotency and owner
    identity, but does not execute this GameMode/actor rejection path. RF17 is
    the distinct Rat false-ledger ordering defect; RF18/RF46 are callback
    coverage gaps and do not record this live zero-health AI state.
    **Proposed repair:** Make lethal state and durable reward acceptance one
    recoverable server transaction. On rejection, either restore a valid
    retryable pre-lethal health state or enter an explicit inactive pending-
    defeat state. Retain the original credited identity and outcome, and retry
    server-side until the claim and defeated ID can be committed; then unregister
    and publish defeat once.
    **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
    **Acceptance:** With a full pending queue and with a conflicting stable
    claim identity, a rejected lethal hit leaves no zero-health creature running
    AI, emits no accepted defeat, and creates no wrong-owner/duplicate claim.
    After a queue slot becomes available, retry completes the same stable
    owner-bound reward and defeated ID once, then unregisters and broadcasts
    defeat once. Repeated hits/retries do not reroll or duplicate the claim.
    **Focused validation:** Add an isolated GameMode plus wildlife defeat
    regression that injects a full queue and a conflicting claim, observes
    Health/AI/defeat state, frees the queue for the valid case, and verifies
    owner identity and exact-once persistence. Run the focused automation after
    the required Editor build; no test was run during this review.
  - [ ] **RF70 — Fail closed on invalid explicit world-launch identity (P3; CR10-005).**
    `Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp:30-74`,
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:520-569`,
    `Source/KalmalaGameplay/Private/KalmalaMenuContext.cpp:115-180`, and
    `docs/startmenu.md:184-206,494-522`.
    **Affected path:** Starting a solo or host-co-op world with explicit menu
    launch identity options.
    **Trigger:** At least one KalmalaMenuLaunch, KalmalaMenuSeed, or
    KalmalaMenuRevision option is present but the set is incomplete, malformed,
    stale, incompatible with the current network mode, or rejected by the
    server's seed-selection API.
    **Impact:** GameState logs rejection and returns with its constructor seed
    10323456789; GameMode then proceeds to load or create seed-keyed population
    and construction saves using that fallback. An explicit launch can therefore
    enter and later modify a different world than the selected identity. The
    normal menu builder revalidates the identity and emits the current revision;
    this finding concerns malformed or stale explicit launch input, not a remote
    client seed-control path.
    **Evidence:** PostInitializeComponents detects any explicit menu option,
    validates the launch kind, seed, revision and net mode, then only logs and
    returns on failure without recording a rejected startup state. The default
    seed remains set. AKalmalaGameMode::BeginPlay consumes that configuration
    and proceeds to LoadPopulationSave and construction-slot selection. In the
    absent-menu-options branch, the constructor default or process WorldSeed=
    remains the documented normal behavior. The start-menu contract says the
    selected world seed/revision is revalidated before travel.
    **Proposed repair:** Preserve the no-menu-options default/command-line path
    and valid explicit menu launch. Record invalid explicit identity as a
    startup failure and stop before selecting, loading, creating, or writing any
    world save; return to the menu with an actionable local error if supported.
    **Dependencies:** After CR19 review acceptance; no repair during the
    review-only phase.
    **Acceptance:** With no menu identity options, normal default or process
    WorldSeed= selection still works. Valid explicit solo and host-co-op
    options start the requested seed. Any explicit incomplete, malformed, stale,
    or network-mode-mismatched options fail closed before gameplay/save startup
    and never load, create, or write a fallback seed's world records.
    **Focused validation:** Add standalone and listen-host launch coverage for
    valid options, each missing/malformed identity field, stale revision, and
    mode mismatch. Verify rejection occurs before GameMode save-slot selection
    or world persistence, while no-option default/command-line launches remain
    unchanged. No runtime launch was run during this review.
  - [ ] **RF71 — Cover fail-closed preservation of invalid construction save slots (P3; CR11-002).**
    `Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveGameTest.cpp:276-368`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveWriteCandidateTest.cpp:29-77`;
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:568-585,702-741`;
    `docs/07-development-setup.md:1941,4331-4403`;
    `Scripts/Tests/Construction/ConstructionPersistence.ps1`; and
    `Scripts/Tests/Survival/PersistedCampRestart.ps1`.
    **Affected path:** A server starts a world whose same-seed construction slot
    contains a deserializable but incompatible or malformed save, then a player
    attempts another construction write.
    **Trigger:** A future change treats the invalid existing save as absent or
    allows a new candidate to write after startup rejected the slot.
    **Impact:** Previously persisted construction data could be replaced by an
    empty or partial current save. Current source fails closed, but no focused
    regression protects the no-rewrite guarantee for construction slots.
    **Evidence:** The assigned model tests cover current-format in-memory
    round-trips, malformed data, zero/future versions, and candidate isolation;
    none seeds or reads an actual invalid construction slot. ConstructionPersistence.ps1
    covers valid current-format restore only. PersistedCampRestart.ps1 covers
    valid restore and no restore under a different seed, not invalid bytes in
    the same slot. GameMode startup only builds a candidate when the slot is
    absent or loaded data is non-null; an invalid candidate leaves
    ConstructionSaveGame null, and PersistConstruction rejects writes while it
    remains null. Setup requires incompatible or invalid slots to fail closed
    without rewriting. RF22 is specific to player-discovery slots and RF23
    covers world-save/Home-return transactions; neither protects this path.
    **Proposed repair:** Add an isolated GameMode persistence regression that
    seeds an invalid current construction save at the exact slot for the test
    world, starts the production loader, attempts a construction write, and
    compares the original slot bytes before and after. Keep a valid same-world
    restore control and the cross-seed rejection check.
    **Dependencies:** After CR19 review acceptance; no test repair during the
    review-only phase.
    **Acceptance:** An incompatible or malformed same-slot construction save is
    not restored or rewritten, later construction persistence remains rejected,
    and the original bytes are identical after the attempted write. A valid
    current-format save still restores through the same production path.
    **Focused validation:** Run the isolated GameMode regression with
    `Kalmala.Gameplay.Construction.Schema7CurrentFormat`,
    `Kalmala.Gameplay.Construction.Schema7WriteCandidate`, and
    `Scripts/Tests/Construction/ConstructionPersistence.ps1` after an Editor
    build. No test was run during this review.
  - [ ] **RF72 — Require Thunder Mountains coverage for the all-biome Stone fallback (P3; CR10-006).**
    `Source/KalmalaWorld/Private/Tests/KalmalaBiomeContentContractTest.cpp:118-122,199-207,239-262`;
    `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:246-286`;
    `docs/07-development-setup.md:2077-2083`; and `docs/05-decision-log.md:10`.
    **Affected path:** Deterministic regression coverage for generated Stone
    outcrops in Thunder Mountains spatial keys without a same-key slate source.
    **Trigger:** The test sets `bSawStoneOutcropMountains` for any
    `mountains-slate-vein` descriptor, and its completion predicate accepts that
    flag. The `allbiomes-stone-outcrop` branch only verifies its descriptor when
    the same flag is still false. A slate source in one key therefore can
    satisfy the test's Mountain coverage without requiring the fallback in a
    different key where no slate source exists.
    **Impact:** The contract that fills each land biome lacking a slate source
    per spatial key has no independent Mountain fallback regression assertion;
    a future change could omit Mountain fallback outcrops while other biomes
    and the Mountain slate source keep the current test green. No current
    production placement failure or gameplay authority issue was confirmed.
    **Evidence:** The test shares `bSawStoneOutcropMountains` between the
    ThunderMountains case for `allbiomes-stone-outcrop` and the independent
    `mountains-slate-vein` shortcut, then uses it in `HasCompleteSourceCoverage`.
    Production builds `BiomesWithStone` separately for each spatial key and adds
    `allbiomes-stone-outcrop` only for a land biome missing a slate source in
    that key. The setup and decision log describe this six-biome, per-key
    fallback. Static review did not execute the deterministic fixture, so this
    finding is about the missing required assertion, not a claim that the
    current fixture currently generates no Mountain fallback.
    **Proposed repair:** Track Mountain slate-source coverage separately from
    all-biome fallback coverage. Use a deterministic fixture key with a
    Thunder Mountains candidate and no same-key `mountains-slate-vein` source,
    then require and validate the generated `allbiomes-stone-outcrop`.
    **Dependencies:** After CR19 review acceptance; no repair during the
    review-only phase.
    **Acceptance:** A focused test independently proves that a selected
    Thunder Mountains key with no slate source receives exactly one
    `allbiomes-stone-outcrop` when a safe separated candidate exists, with
    stable identity, safe terrain, and 250 cm separation. A Mountain slate
    descriptor from another key cannot satisfy this assertion.
    **Focused validation:** Extend `Kalmala.World.M7.BiomeContentContract` with
    the deterministic key fixture and run it after the required Editor build.
    No test was run during this review.
  - [ ] **RF73 — Make Deer flee away from their triggering threat (P3; CR09-001d-02).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:689-775,782-793,5039-5051,5076-5120`,
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:16-25,94-103`,
    `docs/01-game-design.md:138-142`, and `docs/02-technical-architecture.md:1138-1140`.
    **Affected path:** An accepted hit puts a Deer into Flee and alerts nearby idle Deer; the server then advances each actor toward its selected destination.
    **Trigger:** A Deer is hit or alerted by combat while the stable spawn-ID-derived flee offset points in the direction of the attacker or noise source.
    **Impact:** The Deer moves toward the threat during the Flee phase instead of away, undermining the documented wary response and potentially drawing hunted or alerted animals toward the player. No authority bypass or reward duplication was found.
    **Evidence:** `ApplyValidatedDamageFromServer` sends Deer through the Flee transition and calls `AlertNearbyDeerFromServer`; the alert helper independently assigns `SpawnOrigin + GetDeterministicOffset(...)`. `GetDeterministicOffset` derives an angle only from the persistent spawn ID and receives no threat location. `AdvanceServerBehaviour` moves directly toward `BehaviourDestination` while the state is Flee. The following Investigate and Return stages occur afterward, so the toward-threat movement is not the homeward return. The design says Deer flee from noise and nearby combat; the architecture calls this bounded wary-herd flee behavior. `KalmalaWildlifeBehaviourTest` checks state transitions and bounded flee distance, but not direction.
    **Proposed repair:** Pass a trusted server-side source location through direct-hit and herd-alert transitions, then select a bounded destination away from that source. Keep a stable bounded fallback for a coincident source and retain server control of destination selection.
    **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
    **Acceptance:** Affected Deer and alerted herd members move away from the triggering combat/noise source within their ecology-specific flee distance, including all tested source bearings; coincident-source fallback remains bounded and server-owned. Later Investigate/Return behavior still returns to the spawn origin.
    **Focused validation:** Extend `Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle` with direct-hit and nearby-herd destination/movement assertions for multiple bearings, plus coincident-source fallback, authority and flee-distance bounds. Run after the required Editor build. No test was run during this review.
  - [ ] **RF74 — Restore Mireling campsite attraction and camp-pressure behavior (P2; CR09-001d-03).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1112-1135`,
    `docs/01-game-design.md:140`, `docs/02-technical-architecture.md:860,1138`,
    `docs/05-decision-log.md:30,151`, and `docs/biome-enemies.md:417-423`.
    **Affected path:** A living server Mireling selects its target and applies
    its close-range melee behavior.
    **Trigger:** A Mireling is active near a campsite while no living player is
    within its 500 cm target search, or players are nearby but the camp itself
    needs pressure.
    **Impact:** The Mireling idles without a player in range and otherwise
    approaches or attacks only a player. It never detects, approaches, or
    pressures campsites or their supplies, so the documented camp-defense loop
    is absent. No client authority bypass was found.
    **Evidence:** `UpdateMirelingScavenge` iterates only authoritative living
    `AKalmalaCharacter` actors within 500 cm, returns when none exists, and
    approaches/melees that character. A source search found no other Mireling
    campsite or supply target behavior. The active game design calls the
    Mireling a scavenger attracted to campsites that pressures camp-supply
    protection; architecture says Mirelings scavenge and threaten camps.
    The A8 base-enemy proposal retains approach/simple melee without saying
    targets are player-only, and the accepted M5 decision preserves existing
    target selection without removing the campsite role. Mireling tests cover
    spawn share, habitat, stats and cooldown, not campsite behavior.
    **Proposed repair:** Add bounded server-owned campsite-aware targeting and
    pressure while retaining the simple-melee contract. Define the precise
    supply-pressure outcome through an accepted design decision before
    implementing it; do not allow clients to choose sites or supply outcomes.
    **Dependencies:** After CR19 review acceptance; any supply effect requires
    its own accepted behavior detail. No repair during CR09 review.
    **Acceptance:** With no player in the current 500 cm search, a living
    Mireling can select an eligible campsite through server-owned bounded
    rules and apply the separately approved pressure outcome. Tests cover the
    active site, no-player case, rejected/unavailable sites and authority;
    clients cannot select a site, item or result.
    **Focused validation:** Add a Mireling behavior contract and focused
    server-side scenario for campsite target eligibility, bounded approach and
    the approved pressure result. Run after the required Editor build. No test
    was run during this review.
  - [ ] **RF75 — Reject overflowing Smelter save counters (P3; CR11-003).**
    `Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:108-130`;
    `Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:65-75,160-173,227-245`;
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-953,831-855`;
    `Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp:112-137,193-201`;
    `Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:447-483`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveGameTest.cpp:396-430`;
    `docs/07-development-setup.md:1941,4392-4403`.
    **Affected path:** A schema-7 world slot restores a Smelter with an
    overflowing Coal or output count, after which a player collects its bars.
    **Trigger:** A deserializable current-schema record contains nonnegative
    signed 32-bit counts whose aggregate overflows before the capacity check.
    **Impact:** The invalid record can pass current validation and restore with
    impossible Smelter contents. Output collection can then persist decremented
    values and grant bars beyond the ten-unit output capacity. No client request
    selects or modifies the saved counter; this is a malformed server-slot
    validation failure.
    **Evidence:** `GetInputCoalCount` adds the active-job reservation to
    `InputCoal` as `int32`; `GetOutputCount` sums three `int32` output counters.
    `IsValid` checks `InputCoal < 0` and each output `< 0`, but relies on those
    potentially overflowing sums for upper bounds. For example, `INT_MAX`
    Copper plus `INT_MAX` Tin can overflow on the Windows target and evade the
    sum comparison. Construction `IsValidRecord` calls `Smelter.IsValid`, and
    `MatchesWorld` applies `IsValidRecord` to every record. Startup restore and
    actor initialization repeat the same predicates without another counter
    bound. `CollectSmelterItemFromServer` decrements one output, persists the
    candidate through `State.IsValid` and `MatchesWorld`, then commits one bar;
    none independently caps the stored output fields. Current tests cover
    ordinary full-capacity states but no integer-boundary values. Setup requires
    invalid current slots to fail closed without rewriting. RF68 concerns
    transform world bounds; RF71 concerns the separate invalid-slot regression;
    neither covers arithmetic overflow.
    **Proposed repair:** Validate each stored counter against its capacity and
    compute reserved/aggregate counts in a wider checked type. Add boundary
    cases for INT_MAX with an active job and an overflowing output sum; require
    both record and world validation to reject them. Exercise same-slot startup
    preservation and retain valid zero/full-capacity controls.
    **Dependencies:** After CR19 review acceptance; no repair during the
    review-only phase.
    **Acceptance:** Overflowing current-schema Smelter count sets fail both
    `IsValidRecord` and `MatchesWorld` before spawning; startup preserves exact
    slot bytes. A valid ten-output boundary restores. One collection saves
    nine outputs before granting exactly one bar, and a later load retains that
    nine-output state.
    **Focused validation:** Extend `Kalmala.Gameplay.Construction.Schema7CurrentFormat`
    and the invalid-slot GameMode regression, then run
    `Scripts/Tests/Construction/ConstructionPersistence.ps1` after an Editor
    build. No test was run during this review.
  - [ ] **RF77 — Stop wildlife attacks after an accepted poison defeat (P3; CR09-001d-03).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-599,642-681,1083-1095,1138-1168`;
    `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:320-360`; and
    `Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp:228-251`.
    **Affected path:** Fang Arrow poison defeats a charging Boar during its
    authoritative actor tick.
    **Trigger:** A poison tick lethally damages a Boar with a committed
    `BoarChargeTarget`, positive charge time and a nearby living player, while
    the melee cooldown permits a hit.
    **Impact:** The accepted defeat hides and marks the Boar defeated, but the
    same server Tick continues into `UpdateBoarTerritory`, which can apply one
    additional hit from the defeated actor. This is a one-tick gameplay
    lifecycle defect; no client authority bypass was found.
    **Evidence:** `Tick` evaluates authority/`!bDefeated` once, calls
    `AdvanceFangArrowWeakPoisonFromServer`, then unconditionally advances
    behavior and calls the archetype updater inside that already-entered block.
    A lethal poison tick calls validated damage and `DefeatServer`; accepted
    defeat sets `bDefeated` and `ApplyDefeatedState` hides the actor, but the
    current call continues. `UpdateBoarTerritory` has no defeated guard,
    retains its committed target/timer through defeated-state cleanup, and
    directly calls the Character damage helper when the target is within
    180 cm. `AKalmalaCharacter::ApplyWildlifeDamageFromServer` checks source
    validity, authority, same world, damage and reach but not whether a
    wildlife source is defeated, so it accepts the hit. Existing BowDraw
    coverage advances poison and checks damage/expiry but does not combine
    lethal poison with the wildlife behavior updater; Boar behavior tests do
    not cover defeat during a committed charge. RF69 is the separate rejected
    lethal-reward path that leaves a zero-health actor active for subsequent
    ticks.
    **Proposed repair:** Recheck `bDefeated` after poison advancement and exit
    the current Tick before behavior updates; also consider rejecting
    defeated wildlife sources in the Character damage helper as defense in
    depth.
    **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
    **Acceptance:** A poison tick that defeats a charging Boar prevents further
    movement or character damage in that Tick; the defeated wildlife source is
    rejected by any shared damage helper guard, and ordinary living-Boar
    charges remain unchanged.
    **Focused validation:** Add a focused test that lethally poisons a Boar
    during a committed charge and verifies defeat occurs once with no
    post-defeat hit or movement. Run after the required Editor build. No test
    was run during this review.
  - [ ] **RF78 — Honor authored extended wildlife hit ranges in shared damage validation (P2; CR09-001d-05/06).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1957-2216,2218-2360,2715-2720,2948-2956,3096-3105`;
    `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:320-333`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePolarBearTest.cpp:68-77`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMoonBearTest.cpp:55-76`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp:83-112`;
    and `docs/biome-enemies.md:45,51,54`.
    **Affected path:** Server Polar Bear charge, Moon Bear overhead paw, and Peat Husk sweeping-arm damage to player Characters.
    **Trigger:** A valid visible Bear target is at a three-dimensional distance greater than 180 cm and at most 220 cm at impact; or a valid visible Peat Husk target is within its 260 cm horizontal reach and 120-degree forward cone but more than 180 cm away in three dimensions.
    **Impact:** Each species-specific check accepts the target, but the shared Character helper rejects the source beyond its 180 cm three-dimensional limit unless it is Thunder Sentinel. The player takes no damage inside the authored reach despite passing the attack-specific range gate. This is one server-side source-range contract mismatch; no client authority bypass was found.
    **Evidence:** Polar Bear and Moon Bear reach predicates accept 220 cm and their hit paths call `AKalmalaCharacter::ApplyWildlifeDamageFromServer`. `IsPeatHuskSweepInReach` accepts horizontal distance through 260 cm and forward dot at least 0.5; `TryApplyPeatHuskSweep` uses that predicate before calling the same Character helper. The helper rejects three-dimensional distances above 180 cm unless the source is Thunder Sentinel and its separate horizontal check allows that archetype through 300 cm. The three focused species tests assert standalone attack predicates, not the shared helper at an outer-band distance. `docs/biome-enemies.md` specifies the 220 cm Bear attacks and Peat Husk's 260 cm/120-degree sweep. Reef Maw's 180 cm three-dimensional bite matches the common limit. No live outer-band damage run was performed.
    **Proposed repair:** Reconcile the shared server damage validation with the reviewed source-specific Polar Bear/Moon Bear 220 cm and Peat Husk 260 cm-horizontal/cone contracts. Preserve the ordinary 180 cm wildlife limit, the existing Thunder Sentinel rule, attack-specific visibility/target checks, and rejected invalid/client sources; do not widen every wildlife attack.
    **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
    **Acceptance:** Server-side integrated tests prove Bear hits at 180 cm and the inclusive 220 cm boundary resolve, while beyond-220 targets reject; Peat Husk hits beyond 180 cm through the inclusive 260 cm horizontal boundary within its cone resolve, while beyond-260 and outside-cone targets reject. Invalid/client sources remain rejected, ordinary wildlife remains limited to its authored range, and Thunder Sentinel's 300 cm horizontal behavior is unchanged.
    **Focused validation:** Extend the focused server-side Bear and Peat Husk call-path coverage to exercise the shared Character damage helper at each authored boundary and just outside it, including authority rejection. Retain Reef Maw 180 cm coverage and Thunder Sentinel's existing 300 cm regression. Run after the required Editor build. No test was run during this review.
  - [ ] **RF79 — Keep CommonUI as a private KalmalaUI dependency (P3; CR12-001).**
    `Source/KalmalaUI/KalmalaUI.Build.cs:6-27`;
    `Source/KalmalaUI/Private/KalmalaMinimapSubsystem.cpp:6,113-150`;
    `Source/KalmalaUI/Public` (targeted search of public headers); and
    `Kalmala.uproject:39-42`.
    **Affected path:** UnrealBuildTool resolves dependencies for another module
    that consumes KalmalaUI.
    **Trigger:** A downstream module declares a dependency on KalmalaUI and
    receives its public dependency set.
    **Impact:** `CommonUI` is exposed transitively to every KalmalaUI consumer,
    although KalmalaUI's public headers do not include or reference its API.
    This unnecessarily couples downstream build graphs to CommonUI; no runtime,
    gameplay authority, or privacy defect was found.
    **Evidence:** `KalmalaUI.Build.cs` lists `CommonUI` in
    `PublicDependencyModuleNames`. The only source usage found is the private
    `KalmalaMinimapSubsystem.cpp`, which includes
    `Input/CommonUIActionRouterBase.h` and uses the action router and input
    configuration. A targeted search across `Source/KalmalaUI/Public` found no
    CommonUI include or symbol reference. The CommonUI plugin is enabled in the
    project, so the module's implementation dependency remains available.
    **Proposed repair:** Move `CommonUI` to `PrivateDependencyModuleNames` and
    retain the plugin setting and current minimap input-router behavior.
    **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
    **Acceptance:** KalmalaUI's CommonUI implementation still compiles, its
    public headers remain free of CommonUI API requirements, and unrelated
    downstream modules no longer receive CommonUI transitively through
    KalmalaUI.
    **Focused validation:** After the required Editor build, compile KalmalaUI
    with CommonUI private and inspect a representative consumer's dependency
    graph to confirm no transitive CommonUI requirement. No build was run during
    this review.
  - [ ] **RF80 — Restore primary focus after a listed world disappears (P3; CR12-002).**
    Source: CR12-002; `E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaHomeSubsystem.cpp:758-775`;
    read-only current-main call target `E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldListWidget.cpp:589-595,641-646`.
    **Affected path:** A local player opens details for a previously listed world seed.
    **Trigger:** That existing seed disappears from recognized local save slots after the list is shown but before World details is opened.
    **Impact:** The failed details path rebuilds the list without assigning focus to a live primary control. Keyboard/controller users may have no valid focus target and must navigate manually. No save mutation or authority/privacy defect was found.
    **Evidence:** `HandleWorldSelected` returns after `ShowWorldDetails` fails, navigates back to Worlds, and calls `ShowWorldList` without `FocusPrimary`. `ShowWorldDetails` refreshes the slots and returns false for a missing existing seed; `ShowWorldList` rebuilds the controls.
    **Proposed repair:** Restore a valid primary focus target with `FocusPrimary` after rebuilding the list on this failure path.
    **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
    **Acceptance:** If the selected existing seed disappears between list and details, the updated list remains visible and focus lands on a live enabled primary target (the picker when entries remain, otherwise New world or Back according to listing availability). Keyboard and controller navigation can continue immediately; no save is changed.
    **Focused validation:** Add a focused Home/world-list automation that removes the selected seed after listing, then asserts the rebuilt screen focuses a live primary control for populated and empty lists. After an Editor build, run the focused automation and verify keyboard/controller recovery in the rendered screen. No test was run during this review.
  - [ ] **RF81 — Include generator revision in generated population save identity (P3; CR10-010).**
    `Source/KalmalaWorld/Public/KalmalaWorldGenerationConfig.h:23-26`;
    `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:30-48`;
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:205-208,647-665`;
    `Source/KalmalaWorld/Public/KalmalaM7PersistenceContract.h:37-56`;
    `Source/KalmalaWorld/Private/KalmalaM7PersistenceContract.cpp:62-69`;
    `Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp:62-64`;
    `Source/KalmalaWorld/Private/Tests/KalmalaWorldPlayerStartResolverTest.cpp:235-281`;
    and `docs/02-technical-architecture.md:958-959`.
    **Affected path:** Loading same-seed generated-population deltas after the
    compiled world generator advances to a new revision.
    **Trigger:** A population slot written under revision 7 is opened under a
    later generator revision with the same seed. The slot name and save record
    contain no generator revision, while the save schema remains version 2.
    **Impact:** Old harvested/defeated IDs, treasure claims and gemstone state
    can be applied to a different generated layout. Changed candidates can
    reappear when their new IDs no longer match old depletion, or a new
    descriptor sharing an old stable ID can inherit stale state. This is a
    server-side persistence identity defect; no client authority bypass was
    confirmed.
    **Evidence:** The documented world-save identity includes generator
    revision. `FKalmalaM7SaveIdentity::Matches` requires equal revisions, and
    launch validation requires the current revision. By contrast,
    `FKalmalaWorldGenerationConfig::operator==` compares only `WorldSeed`;
    population slots are named only by seed; `MatchesWorld` checks schema 2 and
    that seed-only config. `LoadPopulationSave` therefore accepts a same-seed
    old-revision record. The current focused population-save test rejects a
    different seed but has no same-seed/different-revision case. The targeted
    population slot/loader code is identical in the isolated checkout and the
    current main file; main was inspected read-only.
    **Proposed repair:** Include the current generator revision in the generated
    population save identity and slot lookup, and reject mismatched revisions
    without rewriting the old slot. Keep existing data available for an
    explicitly supported future migration decision; do not silently reuse it.
    **Dependencies:** After CR19 review acceptance; no repair during the
    review-only phase.
    **Acceptance:** A population record written for a given seed and generator
    revision loads only for that exact pair. Starting the same seed under a new
    revision initializes or loads only that revision's population state and
    leaves the previous-revision slot bytes unchanged. Current-revision harvest,
    defeat, treasure, and gemstone deltas still round-trip normally.
    **Focused validation:** Extend the population-save identity test and the
    GameMode load contract to cover same-seed current/stale revisions, verify
    stale data is not retained or overwritten, and retain the current-revision
    round trip. No test was run during this review.
  - [ ] **RF82 — Add current snake attack-path regression coverage (P3; CR09-001d-07).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3132-3463`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeViperTest.cpp`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeGiantSnakeTest.cpp`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeDeathAdderTest.cpp`;
    `Scripts/Tests/Combat/EnemyPeers.ps1:4,133-147`; and
    `docs/07-development-setup.md:2461-2463,2490-2492,2612`.
    **Affected path:** Future changes to the server-side Viper, Giant Snake, or Death Adder lunge and bite path, including target/range rejection and species-specific poison application.
    **Trigger:** A change regresses `UpdateSnakeAmbush`, `BeginSnakeAttack`, or `TryApplySnakeBite` while the existing catalogue/profile/reward tests and unrelated Mosquito peer scenario continue to pass.
    **Impact:** The current suite can miss regressions in the live snake attack path, including a rejected or invalid bite damaging a player or an accepted bite failing to apply its authored effect. This is a test-coverage gap; no current snake production behavior defect was confirmed. It is distinct from RF19's runner documentation and RF46's defeat-reward callback coverage.
    **Evidence:** The production tick calls the snake state machine and its bite applies damage and poison only after target, reach, and visibility checks. The Viper, Giant Snake, and Death Adder focused tests cover spawn/profile/timing, reward identity, queueing, and persistence, but do not call the snake state-machine or bite application functions. `EnemyPeers.ps1` lists `MosquitoCounterplay` but no current snake scenario; the setup guide labels ViperPeer and GiantSnakePeer results historical. No current snake attack-path assertion was found.
    **Proposed repair:** Add a current integrated server-side snake attack regression that drives the production lunge/bite path, validates target and reach rejection, and verifies species-specific effect delivery only after accepted damage. Keep Giant Snake poison-free and retain shared poison-lifecycle and reward-callback tests at their existing owners.
    **Dependencies:** After CR19 review acceptance; no test or gameplay repair during CR09 review.
    **Acceptance:** Current automated coverage exercises Viper, Giant Snake, and Death Adder attack resolution through the production server path; invalid, out-of-range, and obstructed targets cause no damage or poison; accepted Viper and Death Adder bites apply their authored poison, while Giant Snake applies none. Existing `MosquitoCounterplay`, shared poison lifecycle, and RF46 reward coverage remain distinct and passing.
    **Focused validation:** Extend the focused wildlife automation and current peer runner with deterministic attack setup and accepted/rejected bite assertions. Run after the required Editor build. No test was run during this review.
  - [ ] **RF83 — Disable visibility-blocking collision on defeated wildlife (P2; CR09-001d-07).**
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:165-179,583-596,3724-3740,5328-5331,5356-5395`;
    `Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:171-200,228-239`;
    `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1447-1450,1457-1465`;
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6811-6827,8649-8655`; and
    `docs/02-technical-architecture.md:2772`.
    **Affected path:** Server melee target selection and character visibility traces after a wildlife actor is defeated and hidden.
    **Trigger:** A defeated wildlife actor lies between a player and a living wildlife target or an eligible visibility-traced interaction target.
    **Impact:** The hidden actor can remain the first `ECC_Visibility` hit. `FindServerTarget` returns that wildlife actor, then `IsServerTargetValid` rejects it as defeated, so the attack cannot select a living target behind it. Character view traces can likewise stop at the hidden actor. The block lasts until that actor is otherwise destroyed; this is a gameplay query obstruction, not an authority bypass.
    **Evidence:** The wildlife root sphere is `QueryOnly` and blocks `ECC_Visibility`; snake archetypes also enable a visibility-blocking query capsule. `DefeatServer` unregisters the actor and calls `ApplyDefeatedState`; the replicated state update only calls `SetActorHiddenInGame` and clears behavior/reward state. It does not disable the root or snake-body collision, and the defeat path does not destroy the actor. The combat trace uses `ECC_Visibility`, immediately returns a hit wildlife actor, and the later target validator rejects defeated actors. The Character view trace also uses `ECC_Visibility`. No current design or architecture contract specifies retaining hidden defeated wildlife as a visibility blocker; the architecture describes the procedural wildlife mesh as collision-free but does not define defeated root-collision policy. This is distinct from RF77's same-Tick post-defeat damage path.
    **Proposed repair:** Disable query collision that blocks `ECC_Visibility` when applying defeated state, on the authority and replicas, while preserving the actor's required hidden/reward/persistence lifecycle and living wildlife's existing collision behavior.
    **Dependencies:** After CR19 review acceptance; no gameplay repair during CR09 review.
    **Acceptance:** After defeat, the root and any snake-body component no longer block visibility queries on server and relevant clients; traces pass through the hidden actor and can select a living target or eligible interaction behind it. Living wildlife continues to block and receive the existing validated traces, and defeat replication, reward delivery, and restart persistence are unchanged.
    **Focused validation:** Add a component-state and server/peer trace regression with a defeated wildlife actor in front of a living target and an eligible interaction target. Verify live wildlife collision remains unchanged and defeat/restart behavior still passes. Run after the required Editor build. No runtime check was run during this review.
  - [ ] **RF84 — Restore inventory scroll fallbacks at supported viewports (P2; CR12-003).**
    Source: CR12-003; current main-checkout path
    E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:288-300,337-360,
    inspected read-only; docs/07-development-setup.md:1583-1598,5260-5284;
    docs/02-technical-architecture.md:3282-3289.
    **Affected path:** A player opens Inventory at the documented 480x320 viewport and
    tries to inspect lower equipment slots or reach following inventory actions.
    **Trigger:** The responsive panel is capped to viewport height minus 32 logical
    units, while its direct vertical content exceeds that cap and has no outer or
    inventory-row scroll fallback.
    **Impact:** Lower gear and action content extends below the visible viewport with
    no supported scroll path. This leaves part of Inventory undiscoverable at an
    accepted viewport and contradicts the M12 scroll-fallback acceptance. No gameplay
    authority or private-data exposure defect was found.
    **Evidence:** InventoryPanel mounts a plain UVerticalBox directly into the
    size-capped panel; UpdateResponsivePanelSize caps height at ViewportSize.Y - 32
    while budgeting 376 units for Worn Gear alone, before the grid, details, padding,
    or actions. The M12 setup contract requires outer-menu and inventory-row scrolling
    at 480x320. The later Worn Gear record documents the current absence of a
    ScrollBox/fallback and defers the wider accessibility/resolution matrix, so this
    is an unresolved supported-viewport gap. The current main-checkout source was
    inspected read-only and matches the CR01 fingerprint.
    **Proposed repair:** Restore the documented outer-menu and inventory-row scroll
    fallbacks while keeping the responsive panel and all existing equipment/actions.
    **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
    **Acceptance:** At 480x320, pointer, keyboard, and controller users can scroll the
    outer panel and inventory row to reveal every gear slot, detail, and action with
    the active focus target visible. At 1024x768 the standard layout remains usable.
    **Focused validation:** Extend Kalmala.UI.InventoryMenu.Selection to assert the
    outer-menu and row scroll fallbacks and complete reachable content at 480x320,
    then confirm the standard layout at 1024x768. Run after an Editor build and
    inspect the focused rendered state. No test was run during this review.
  - [ ] **RF86 — Present standalone Build requirements to keyboard and controller users (P3; CR12-003).**
    Source: CR12-003; current main-checkout paths
    E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaInventoryCraftLayout.cpp:39-60,230-277
    and E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp:581-604,1945-1948,4826-4836,
    inspected read-only; docs/04-roadmap.md:830-843.
    **Affected path:** A player reviews a build card in the standalone Build menu using
    keyboard or controller focus.
    **Trigger:** The player focuses or activates a build card and needs its description,
    required station, material identities, and owner-visible owned/required counts.
    **Impact:** The standalone view shows a title and material icons, but the description
    and material names/counts are tooltip-only, while the station row is collapsed.
    These details are not available through a keyboard/controller focus target as M11
    requires. Mouse hover is a partial workaround; no authority or privacy defect was
    found.
    **Evidence:** RefreshBuildRequirements assigns description and cost names/counts
    only to BuildRequirementsPanel and CostColumn tooltips; the displayed columns
    contain only icons and are non-focusable widgets. The selected-build title is
    visible, but not the complete requested detail. The separate required-station icon
    is a child of RequirementRow, which is added to BuildExcludedWidgets; standalone
    Build collapses those widgets. M11 requires the selected name, description,
    station icon, and material costs/counts on hover or keyboard/controller focus.
    Owner counts are read from the local inventory component.
    **Proposed repair:** Add a visible selected-build detail area driven by card focus,
    presenting the description, required-station icon and name, and material names with
    owner-visible owned/required counts. Keep pointer hover presentation and provide
    accessible labels for the focused details.
    **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
    **Acceptance:** Focusing each standalone Build card exposes its name, description,
    required station when applicable, and every material's identity and owner-visible
    owned/required count without relying on hover. Text scaling and high contrast keep
    the focused details readable; counts remain local to the owner.
    **Focused validation:** Extend a focused Build UI automation to move keyboard and
    controller focus among representative build cards and assert all detail fields,
    including station-present and station-free recipes. Run after an Editor build and
    inspect rendered focus at default and enlarged text scales. No test was run during
    this review.
  - [ ] **RF85 — Align initial land-wildlife meshes with terrain (P3; CR09-001d-08a/08b/08c/08d).**
    Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:165-173,3645-3707,3711-3742,3878-3883,3906-3914,4041-4044,4089-4197,4211-4269,4282-4310,4314-4362,4364-4489,4605-5036;
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6482-6523,6543-6581,7520-7531,7578-7596;
    Source/KalmalaGameplay/Private/Tests/KalmalaPopulationBodyClearanceTest.cpp:29-91,200-253;
    Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:112-154,163-176,188-238;
    docs/01-game-design.md:144-149; docs/02-technical-architecture.md:2772;
    docs/creature-catalogues.md:31-70,75-85,106-130,137-162,163-171; docs/biome-enemies.md:46-56,74-95,118-135; and docs/monster-spawning.md:369-378.
    **Affected path:** Generated Mireling, Boar, Deer, Rat, Badger, Moon Bear, Polar Bear, Horned Goat, Wolf, Lynx, Peat Husk, Viper, Death Adder, and Giant Snake presentations placed on generated land surfaces.
    **Trigger:** Population placement raises each actor by the vertical support of its active collision shapes, while the presentation mesh attaches at zero relative offset and its lowest geometry does not reach that ground plane.
    **Impact:** On flat ground the mesh remains about 25 cm above ground for Mireling, 17 cm for Boar, 3 cm for Deer, 44 cm for Rat, 18 cm for Badger, 14 cm for Moon Bear, 12 cm for Polar Bear, 15 cm for Horned Goat, 11 cm for Wolf at rest, 15 cm for Lynx at rest, and 29 cm for Peat Husk. Current vertical snake capsules produce gaps of about 87/106/200 cm for Viper/Death Adder/Giant Snake; after RF93 aligns their axes horizontally, the remaining 70 cm root sphere still leaves about 65/64/59 cm gaps. These low/coiled land presentations float above their authored ground habitat. This is a presentation defect; authoritative collision placement remains server-owned.
    **Evidence:** GetBodyCollisionShapesForArchetype uses a 70 cm root sphere for land archetypes other than Rat (42 cm) and adds vertical snake capsules with half-heights 92/112/210 cm (RF93). TryGetPopulationBodyVerticalSupportOffset uses the greatest active-shape support on flat ground and TryGetPopulationActorPlacementLocation adds it to sampled terrain. Meshes attach at zero relative offset. Local minima are Z -45/-53/-67 cm for Mireling/Boar/Deer, +2 cm for Rat, -52 cm for Badger, -56 cm for Moon Bear, -58 cm for Polar Bear, -55 cm for Horned Goat, -59 cm for Wolf at rest, -55 cm for Lynx at rest, and -41 cm for Peat Husk. Reproducing AddSnakeBody path/radius/cross-section vertex math gives minima near -4.7/-6.1/-10.5 cm for Viper/Death Adder/Giant Snake. Existing tests check collider profiles and terrain support but not mesh bounds against ground. Current design describes low/coiled dry-shore snakes and Peat Husk as a dry-ground root brute; architecture makes wildlife meshes presentation-only and non-colliding. Mosquito Swarm is excluded because its server actor flies 55-245 cm above sampled ground, within the documented 50-250 cm band.
    **Proposed repair:** Reconcile each reviewed land-wildlife resting mesh with sampled terrain and its intended support anchor so feet, hooves, paws, roots, and coils visibly meet the ground, preserving actor/collision authority and the collision-free presentation contract. Coordinate snake bounds with RF93.
    **Dependencies:** After CR19 review acceptance; coordinate snake ground alignment with RF93. No gameplay or presentation repair during CR09 review.
    **Acceptance:** All fourteen reviewed land species remain grounded at their authored contact surfaces on flat terrain and representative slopes without gaps or penetration, including relevant resting and warning/attack poses. Collision support matches intended body geometry; presentation meshes remain non-colliding.
    **Focused validation:** Add geometry-to-placement assertions for all fourteen reviewed meshes and inspect rendered fixtures on flat terrain and representative slopes, including warning/attack poses after the required Editor build. Verify snake collision-axis/support behavior under RF93. No rendered or runtime check ran during this review.
  - [ ] **RF87 — Preserve exterior winding on Rat tail, Goat horns, and snake body tubes (P3; CR09-001d-08b/08c/08d).**
    Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:4114-4184,4431-4470,4731-4825,5036;
    Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:112-155,188-220,238.
    **Affected path:** Rat generated tail tube, Horned Goat paired horn tubes, and Viper, Death Adder, and Giant Snake continuous body tubes, including side strips and end caps.
    **Trigger:** AddTailTriangle, AddHornTriangle, and AddSnakeTriangle reverse triangles with negative face/expected-normal dots and preserve nonnegative results, so all three helpers emit nonnegative face/normal dots for nondegenerate triangles.
    **Impact:** Generated exterior surfaces have winding opposite the engine expected face orientation while vertex normals point outward; rendering can cull or incorrectly shade them from intended exterior views.
    **Evidence:** The three helpers swap B/C for negative face/expected-normal dots and preserve other triangles, guaranteeing nonnegative dots. Rat samples measured raw dots from about -201 to -51 before reversal. AddSnakeTriangle applies the same rule to every side and cap triangle using averaged outward ring normals or end tangents. FKalmalaModelSurfaceTest requires negative at its engine reference and for every face/normal dot; explicit Viper, Giant Snake, and Death Adder fixtures exercise the failing path. Existing fixtures omit Rat and Horned Goat.
    **Proposed repair:** Make all three tube helpers preserve the engine expected exterior order relative to outward normals, and add explicit Rat tail and Horned Goat horn fixtures while retaining current snake fixtures.
    **Dependencies:** After CR19 review acceptance; no geometry repair during CR09 review.
    **Acceptance:** Rat tail, Goat horns, and all snake body side and cap triangles follow engine exterior winding and agree with outward normals. The surface-winding regression covers these five species, and other wildlife presentation remains intact.
    **Focused validation:** Extend Kalmala.Gameplay.Presentation.SurfaceWinding with explicit Rat and Horned Goat fixtures, retain Viper/Death Adder/Giant Snake coverage, run it after the required Editor build, and inspect the exterior surfaces. No test or rendered check ran during this review.
  - [ ] **RF93 — Align snake collision capsules with their slithering bodies (P2; CR09-001d-08d).**
    Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3645-3707,3711-3735;
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6482-6523,6554-6576;
    Source/KalmalaGameplay/Private/Tests/KalmalaPopulationBodyClearanceTest.cpp:229-253;
    docs/creature-catalogues.md:57-70,108-115; docs/biome-enemies.md:74-78,118-132;
    docs/monster-spawning.md:369-378.
    **Affected path:** Viper, Death Adder, and Giant Snake collision and spawn placement.
    **Trigger:** Configured capsule rotation FRotator(0,90,0) is a yaw turn that leaves its local Up axis vertical. The placement helper projects that axis onto terrain normal and uses capsule half-height.
    **Impact:** Flat-ground actor support is 92/112/210 cm for Viper/Death Adder/Giant Snake rather than horizontal body support; low coils float roughly 87/106/200 cm above terrain and collision occupies tall vertical columns. Server remains authoritative.
    **Evidence:** BuildArchetypePresentation applies the stored rotation to SnakeBodyCollision. TryGetPopulationBodyVerticalSupportOffset rotates FVector::UpVector; KalmalaPopulationBodyClearanceTest explicitly expects Giant Snake placement at least ground plus half-height. Creature docs describe low/coiled bodies and dry-shore slithering. RF05 covers roof trace origin versus actual capsule top and does not cover axis alignment.
    **Proposed repair:** Align the capsule long axis with the authored slithering body and make active collision transforms and placement support use the same orientation. Preserve server validation and existing attack/reward contracts.
    **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
    **Acceptance:** All three snake capsules follow their low/coiled bodies; flat and sloped support reflects the transformed capsule plus root sphere, collision no longer forms a tall column, and body-clearance/roof queries use the actual volume without changing server-side bite authority.
    **Focused validation:** Extend KalmalaPopulationBodyClearanceTest for transformed capsule axes and support of all three species on flat and sloped ground, retaining focused roof and snake-attack regressions after an Editor build. No test was run during this review.
  - [ ] **RF88 — Correct the stale creature-catalogue schema label (P3; CR13-002).**
    `docs/creature-catalogues.md:27`, `Content/Data/GameCatalogues.json:2`,
    `Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp:407`, and
    `docs/02-technical-architecture.md:524`.
    **Affected path:** A developer consults the creature-catalogue guide for the
    current manifest and loader schema.
    **Trigger:** The guide calls the item/tool loader “schema-15” while the
    current manifest and loader accept schema 16.
    **Impact:** The obsolete version label can misdirect future catalogue edits
    or verification. The adjacent claim that the item/tool loader does not
    consume `creatureFiles` remains accurate; no runtime defect was found.
    **Proposed repair:** Change only the stale schema label to 16, retaining the
    authoring-data boundary and the statement that creature files are not loaded
    by the item/tool loader.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** The guide states schema 16 and accurately preserves the
    current unconsumed-creature-category contract, with no runtime/scope change.
    **Focused validation:** Compare the guide with the manifest version and
    loader guard; run `git diff --check`. No Unreal build is needed.
  - [ ] **RF89 — Update stale shared attack-list guidance (P3; CR13-003).**
    `docs/07-development-setup.md:5184`; `Content/Data/GameCatalogues.json:2,71`;
    `Content/Data/GameCatalogues/attacks/attacks.json` and `bows.json`;
    `Content/Data/GameCatalogues/items/equipment/building-hammer.json`;
    `Source/KalmalaGameplay/Private/KalmalaToolCatalogue.cpp:104-111`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaDamageProfileTest.cpp:136-183`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaTwoHandedTest.cpp:46`.
    **Affected path:** A developer consults the shared attack-list setup section
    to understand the current manifest, authored Basic bonuses, and utility
    hammer attack membership.
    **Trigger:** The undated section describes schema 11, says Basic bonuses
    remain zero, and says BuildingHammer has an attack list. The current
    schema is 16; several Basic style bonuses are nonzero; ConstructionHammer
    is explicitly a no-attack utility with no assigned list.
    **Impact:** Stale guidance can misdirect catalogue edits and balance checks.
    No attack-data or runtime defect was confirmed.
    **Evidence:** Manifest line 2 declares schema 16 and its `attackFiles` at
    line 71 registers both assigned files. The attack JSON contains Axe +10
    Slash and Mace +10 Stab/+10 Crush. `building-hammer.json` defines the
    `ConstructionHammer` profile; `UKalmalaToolCatalogue::Validate` excludes
    it from required attack lists and rejects assigning one. The damage-profile
    test expects Bronze Axe Basic Slash 60 and authored Mace totals; the
    two-handed test asserts ConstructionHammer has no combat attacks. The
    registered-catalogue/reference validator passed.
    **Proposed repair:** Update only the shared attack-list section's schema
    label, Basic-bonus statement, and hammer-membership statement to match the
    current contract. Do not change gameplay or catalogue data.
    **Dependencies:** After CR19 review acceptance; no repair during CR13.
    **Acceptance:** The section accurately describes schema 16, current
    nonzero Basic style bonuses, and ConstructionHammer's no-attack status,
    with no runtime or product-scope change.
    **Focused validation:** Compare the edited section with the manifest,
    assigned attack files, ConstructionHammer profile, C++ validator and tests;
    run `git diff --check`. No Unreal build is needed.
  - [ ] **RF90 — Remove paid no-benefit Construction Hammer upgrades (P2; CR13-004).**
    `Content/Data/GameCatalogues/item-level-up/costs/construction-hammer.json`;
    `Content/Data/GameCatalogues/items/equipment/building-hammer.json`;
    `Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json`;
    `Source/KalmalaGameplay/Private/KalmalaToolCatalogue.cpp:275-297`;
    `Source/KalmalaGameplay/Private/KalmalaToolProgressionContract.cpp:84-110`;
    `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:376-446,457-463`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolProgressionCatalogueTest.cpp:351-368,543-559`;
    `docs/07-development-setup.md:5174,520-523`.
    **Affected path:** A player upgrades a carried ConstructionHammer at a Workbench.
    **Trigger:** The registered cost record charges 2 Wood + 2 Stone at level 1
    and generates levels 2–10 with doubling costs, but ConstructionHammer has
    100 durability and zero damage with no level-up reward. Build placement
    accepts every level at or above 1, so higher levels unlock no capability.
    **Impact:** The accepted transaction consumes materials and advances the
    saved level without changing the hammer's stats or construction behavior.
    The available level-two upgrade is an avoidable material sink.
    **Evidence:** `building-hammer.json` authors 100 `MaxDurability` and empty
    `DamageBonuses`; no `ConstructionHammer` group exists in `item-stats.json`,
    so `GetLevelUpReward` returns its zero-valued fallback and `GetLevelStats`
    leaves stats unchanged. `ProgressToolFromServer` validates and commits the
    upgrade costs, while construction checks only `GetCarriedToolLevel(...) >= 1`.
    The progression catalogue test asserts a level-two entry and all nine
    derived hammer upgrades; setup docs say Building Hammer rewards remain zero.
    The roadmap gives every carried tool a level path but documents no
    ConstructionHammer benefit beyond level 1.
    **Proposed repair:** Remove the unsupported ConstructionHammer upgrade-cost
    record and derived levels; retain the level-one inventory recipe and build
    flow. Update tests and docs that currently expect all nine paid upgrades.
    Do not invent rewards or replacement costs.
    **Dependencies:** After CR19 review acceptance; no repair during CR13.
    **Acceptance:** ConstructionHammer remains obtainable at level 1 and usable
    for construction, while no paid level 2–10 entries or material charges are
    offered. Existing acquisition, server authority and identity behavior stay
    intact; no unsupported rewards or costs are added.
    **Focused validation:** Check the focused progression catalogue test and
    hammer build/acquisition contracts after repair; run `git diff --check`.
    No full build is required for this review handoff.
  - [ ] **RF91 — Expose accessible status details and ongoing recovery guidance (P2; CR12-005).**
    `Source/KalmalaUI/Private/KalmalaStatusHotbarWidget.cpp:189-194,342-400`;
    `Source/KalmalaUI/Private/KalmalaSurvivalStatusWidget.cpp:79-207,258-270`;
    `docs/04-roadmap.md:414-415,437`; `docs/02-technical-architecture.md:1813`.
    **Affected path:** A local player checks active status or weather indicators and
    needs to know their meaning and how the existing effect can be resolved.
    **Trigger:** An untimed Hot, Cold, or Storm entry appears in the top-right
    status hotbar with no recovery/details view.
    **Impact:** The hotbar provides an icon, hidden entry name, and optional timer,
    but no accessible ongoing label, recovery guidance, or existing category,
    intensity/stack and source details. A player cannot make an informed response
    to active survival effects from this presentation. No authority or privacy
    defect was found.
    **Evidence:** `BuildEntries` creates Hot, Cold, and Storm with empty timer text.
    `SetSnapshot` gives each icon only the hidden `Entry.Name` and optional timer.
    The detailed `BuildStatusText` is not used by production `SetSnapshot`, which
    displays only `BuildOceanTravelText`; the M11 status/weather detail page was
    later removed while M11 retained the accessible-detail requirement. All twelve
    assigned source files match the CR01 fingerprint and current main.
    **Proposed repair:** Expose existing owner-visible status/weather category,
    intensity/stack, source, and recovery details through accessible text or a
    detail view. Label conditions without an expiry as ongoing with their current
    recovery guidance. Do not add client timing, gameplay mutation, hidden-content
    queries, or peer-state access.
    **Dependencies:** After CR19 review acceptance; no implementation during CR12.
    **Acceptance:** Active entries provide readable names and applicable existing
    category, intensity/stack, source and recovery details without hover or colour
    alone. Hot, Cold and Storm are explicitly ongoing and include truthful existing
    recovery guidance, with no fabricated countdown. Local text scale and high
    contrast remain usable; data remains limited to the owner's existing state
    and shared weather.
    **Focused validation:** Extend `Kalmala.UI.StatusHotbar.SnapshotAndLayout` and
    the rendered `Scripts/Tests/Presentation/StatusHotbar.ps1` probe to assert
    accessible detail for Hot/Cold/Storm and representative timed status entries.
    After an Editor build, inspect default and enlarged-text/high-contrast captures
    and verify details are available without capturing gameplay input. No test was
    run during this review.
  - [ ] **RF92 — Correct stale Wood Shield cost documentation (P3; CR13-005).**
    docs/02-technical-architecture.md:243-246;
    Content/Data/GameCatalogues/item-level-up/costs/armor/shields.json:4-21;
    Content/Data/GameCatalogues/recipes/stations/workbench.json:266-284.
    **Affected path:** A developer uses the architecture guide to author or
    validate Wood Shield acquisition and upgrade costs.
    **Trigger:** The Bone Fragment section describes Wood Shield crafting and
    its level-one upgrade as Forge actions with obsolete materials and
    quantities. Current catalogue data uses a Workbench recipe of 10 Wood,
    3 Boar Hide and 3 Deer Hide, and a Workbench level-one upgrade cost of
    5 Wood, 2 Boar Hide and 2 Deer Hide.
    **Impact:** Following the stale contract can lead to the wrong station,
    ingredient IDs or quantities in content edits and verification.
    **Evidence:** item-level-up/costs/armor/shields.json registers the
    WoodShield level-one cost as Workbench with 5 Wood, 2 Boar Hide and
    2 Deer Hide. The sole registered WoodShieldRecipe in
    recipes/stations/workbench.json uses 10 Wood, 3 Boar Hide and 3 Deer
    Hide. The later architecture station note at lines 179-184 confirms that
    recipe and station but does not repeat the upgrade ingredients.
    **Proposed repair:** Correct only the obsolete Wood Shield station and
    material/quantity statements in the earlier Bone Fragment section to
    match the current catalogue records. Preserve the later station note and
    do not change the authored data.
    **Dependencies:** After CR19 review acceptance; no documentation edits
    during CR13.
    **Acceptance:** The architecture's Wood Shield acquisition and level-one
    upgrade descriptions match the manifest-registered Workbench recipe and
    cost records; no runtime or catalogue behavior changes.
    **Focused validation:** Compare the corrected statements with the
    manifest, workbench.json and shields.json; run git diff --check.
    No Unreal build is required.
  - [ ] **RF94 — Clarify Opal Helm upgrade support in wearable contracts (P3; CR13-005).**
    docs/02-technical-architecture.md:533;
    docs/07-development-setup.md:5216;
    Content/Data/GameCatalogues/item-level-up/costs/armor/helmets.json:30-46;
    Source/KalmalaGameplay/Private/KalmalaOpalHelm.cpp:8-39,59-86;
    Source/KalmalaGameplay/Private/Tests/KalmalaOpalHelmTest.cpp:13-74;
    docs/a9-acceptance.md:28.
    **Affected path:** A developer or reviewer determines whether a wearable
    upgrade cost has a supported server transaction.
    **Trigger:** The architecture and setup guides say wearable upgrade costs
    are data-only and gear upgrade transactions remain pending, without
    identifying the implemented Opal Helm exception.
    **Impact:** The blanket wording can mislead implementation, review and
    acceptance work about the current Opal Helm Forge/Anvil upgrade path.
    **Evidence:** The dedicated server path reads the registered OpalHelm
    upgrade row, scales its material cost, validates the exact equipped item
    and matching Forge level, and commits payment and level through the
    server-owned inventory transaction. KalmalaOpalHelmTest checks station
    rejection, payment, level/stat change, wear and repair; the A9 acceptance
    summary records that route as passed. Wearable rows still do not create
    carried-tool progression entries, and the other gear-upgrade transactions
    remain pending.
    **Proposed repair:** Clarify the Opal Helm exception in the architecture
    and setup contracts while retaining the pending status for other wearable
    upgrades. Do not change runtime behavior or catalogue data.
    **Dependencies:** After CR19 review acceptance; no documentation edits
    during CR13.
    **Acceptance:** Both guides accurately describe the implemented
    OpalHelm-specific Forge/Anvil route and the remaining gear integration
    scope; runtime and catalogue behavior remain unchanged.
    **Focused validation:** Compare both corrected passages with the
    OpalHelm cost row, server path, focused test and A9 acceptance record;
    run git diff --check. No full build is required.
  - [ ] **RF95 — Clarify wearable item count versus slot count (P3; CR13-005).**
    docs/game-features.md:145; docs/01-game-design.md:168;
    docs/02-technical-architecture.md:533; Content/Data/GameCatalogues.json:23-28;
    Content/Data/GameCatalogues/items/equipment/armor/{boots,capes,helmets,leggings,shields,tunics}.json.
    **Affected path:** A reader uses the feature summary to understand the current
    scope of authored wearable content.
    **Trigger:** The undated feature summary says “Six gear items” and separately
    describes six session-only slots, leaving the six-item statement ambiguous
    between six item definitions and six equip categories.
    **Impact:** A reader may believe the current catalogue has only six wearable
    definitions, while the six manifest-registered armor files contain 16 unique
    wearable ItemIds across the six supported GearSlot values.
    **Evidence:** The current docs/game-features.md hash matches CR01; its line 145
    is presented in the feature summary, without a historical date on that
    statement. The manifest registers the six armor files, whose hashes match
    their CR01 coverage fingerprints and whose parsed records contain 16 unique
    ItemIds. The game-design and architecture contracts identify six wearable
    slots. The separately dated 2026-10-15 decision log describes six initial
    items, not the current total. The docs/game-features row remains assigned to
    CR16-003 and the armor profile rows remain assigned to CR13-011; neither is
    marked reviewed by this finding.
    **Proposed repair:** Clarify that the six count refers to wearable slots or
    categories, and distinguish that from the 16 current wearable item
    definitions; alternatively remove the item-count wording. Do not change
    runtime or catalogue data.
    **Dependencies:** After CR19 review acceptance; no documentation edits
    during CR13.
    **Acceptance:** The feature summary no longer implies that only six wearable
    item IDs are authored and accurately distinguishes the six slots from the
    current definitions; no runtime or catalogue behavior changes.
    **Focused validation:** Compare the corrected wording with the registered
    armor files and GearSlot contract; run git diff --check. No Unreal build is
    required.
  - [ ] **RF96 — Restore expanded-map pan and recenter navigation (P3; CR12-006).**
    `Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:205-210,1502-1509,1625-1639,1641-1686`;
    `docs/04-roadmap.md:972-973,989-990,1006-1013`;
    `docs/02-technical-architecture.md:306,308`;
    `docs/07-development-setup.md:3452-3453,3483-3487`.
    **Affected path:** A local player uses the expanded map to inspect a region or
    place a personal pin or transient ping outside the current player-centred view.
    **Trigger:** The player tries to move the map with mouse, keyboard, or controller,
    or to recenter it after navigation.
    **Impact:** The map remains centred on the owning pawn and offers zoom only, so
    players cannot inspect or mark remote map locations through the UI. This misses
    M14's pan/recenter and keyboard/controller navigation contract; no authority or
    privacy defect was found.
    **Evidence:** `Recenter` uses only `RecenterOnOwningPlayer`; `ZoomAtMapCentre`
    recentres on every zoom; `NativeOnMouseMove` only updates hover; and key handling
    routes arrows/D-pad to marker-filter or pin-style navigation, with no map pan.
    M14's roadmap and architecture require pan/recenter. The later setup notes describe
    the current no-pan behavior, but no decision-log waiver was found. The rendered
    map checks exercise zoom, player-centred view, and focus, not pan/recenter.
    **Proposed repair:** Restore a user-facing pan and recenter path for the expanded
    map while keeping view input local and preserving opaque fog, local pins, and the
    temporary server-validated ping boundary.
    **Dependencies:** After CR19 review acceptance; no implementation during CR12.
    **Acceptance:** Players can pan and explicitly recenter the expanded map with
    keyboard/controller navigation and pointer input. Pan, zoom, and recenter retain
    modal input ownership and do not reveal undiscovered terrain; pins remain local,
    pings remain temporary and server-validated, and no peer map data is exposed.
    **Focused validation:** Extend `Kalmala.UI.WorldMap.LocalPresentation` and
    `Scripts/Tests/World/WorldMap.ps1` to verify pan/recenter changes the actual view
    with keyboard, controller, and pointer input at supported zoom/resolution settings.
    Recheck focus ownership, fog/pin/ping privacy, and rendered host/client cases after
    an Editor build. No test was run during this review.
  - [ ] **RF97 — Preserve credited Boar and Deer rewards when inventory lacks capacity (P2; CR09-001d-08e).**
    Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-596,5328-5446;
    Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:426-433;
    Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-291;
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7607-7608,8649-8655;
    Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:18-20;
    docs/01-game-design.md:63-65; docs/creature-catalogues.md:39-50;
    docs/monster-spawning.md:100-117; docs/biome-enemies.md:153-162.
    **Affected path:** Credited Boar and Deer defeats, including the Deer’s optional Sinew outcome.
    **Trigger:** The attacker’s inventory has room for only some or none of the sequential BoarMeat/BoarHide or DeerMeat/DeerHide/Sinew grants.
    **Impact:** When the defeat save succeeds, the stable wildlife spawn is depleted even if a direct grant fails, and the one-shot reward guard prevents retry. A bundle can be partially granted; all remaining items are permanently lost even if capacity is later freed.
    **Evidence:** DefeatServer has no Boar/Deer Record*DefeatReward queue branch; their accepted defeat broadcasts OnDefeated and GameMode::RecordDefeatedSpawn marks the stable ID and attempts to save it. ApplyDefeatedState calls GrantDefeatReward, which sets bDefeatRewardProcessed before requesting BoarMeat and BoarHide, or DeerMeat and DeerHide plus its server-rolled Sinew, one TryGrantFromServer call at a time without checking results. TryGrantFromServer returns false when BuildGrant cannot add the quantity because pack capacity is reached, without committing a candidate. No pending owner-bound claim is created. The general monster-spawning reward-escrow contract requires complete durable claims and retries after capacity is available. Current focused tests cover the Deer Sinew roll helper, not production grant/capacity behavior. RF17, RF20, RF29, RF64 and RF69 cover distinct paths.
    **Proposed repair:** Route credited Boar and Deer bundles through an authoritative durable owner-bound reward claim, preserving the single Deer Sinew roll, and deliver the complete bundle atomically when capacity permits. Do not publish an unrecoverable reward or treat a failed grant as processed.
    **Dependencies:** After CR19 review acceptance; no gameplay repair during CR09 review.
    **Acceptance:** A credited Boar or Deer defeat with full or partial capacity preserves one stable owner-bound claim and does not publish a partial bundle. After capacity is freed or the owner reconnects/restarts, the exact complete bundle is granted once; the Deer roll is not rerolled. The stable spawn remains depleted and duplicate defeat/reward attempts cannot duplicate loot.
    **Focused validation:** Add a production-path Boar/Deer defeat regression for zero, partial, and sufficient capacity. Verify durable claim identity, whole-bundle retry, stable optional Sinew outcome, reconnect/restart retry and exact-once delivery after the required Editor build. No runtime test was run during this review.
  - [ ] **RF98 — Preserve local input overrides in settings automation (P3; CR12-008).**
    Source: CR12-008; `Source/KalmalaUI/Private/Tests/KalmalaSettingsWidgetTest.cpp:127-136`;
    read-only current-main `E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaSettingsWidget.cpp:1256-1296,1501-1517`;
    `docs/07-development-setup.md:1810`.
    **Affected path:** The settings local-presentation automation runs against a
    local profile containing custom input mappings.
    **Trigger:** `Kalmala.UI.Settings.LocalPresentation` changes Interact and
    reaches `RestoreDefaultInputBindings`, including after an assertion records failure.
    **Impact:** The test removes and flushes all local input overrides, losing
    custom keyboard, controller, alternate-key, and axis-pair mappings. This is
    a persistent local-settings side effect; no gameplay authority or peer-privacy
    defect was found.
    **Evidence:** The test asserts E, changes Interact to F, calls the default
    restore, and only checks that E is reported afterward. Assertions accumulate,
    and no snapshot restores the pre-test bindings. The setter persists to
    `GGameUserSettings.ini`; the restore routine removes override keys across
    every local input definition and flushes the file. Setup documents a temporary
    `-UserDir` for the automation command, but the test does not enforce isolation.
    **Proposed repair:** Snapshot all affected local input entries and register
    unconditional scope cleanup to restore their exact prior values; run the
    automation with a disposable `-UserDir` while preserving its default-reset assertion.
    **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
    **Acceptance:** Seed representative nondefault keyboard, alternate-key,
    controller, and axis-pair mappings. Both a normal run and a run with an
    assertion failure after mutation leave every seeded value unchanged while
    retaining coverage of the default-reset behavior.
    **Focused validation:** Run the automation with isolated settings and seeded
    bindings; compare all binding entries before and after the normal case and a
    controlled assertion-failure case. No test was run during this review.
  - [ ] **RF99 — Correct stale Iron Axe and Bow setup claims (P3; CR13-006).**
    docs/07-development-setup.md:2315; Content/Data/GameCatalogues.json;
    Content/Data/GameCatalogues/item-level-up/costs/weapons/{axes,bows}.json;
    Content/Data/GameCatalogues/recipes/stations/forge.json;
    Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json;
    docs/01-game-design.md:3-25; docs/05-decision-log.md;
    Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp;
    Source/KalmalaGameplay/Private/KalmalaArrowProjectile.cpp;
    Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp.
    **Affected path:** A developer follows the current catalogue verification
    paragraph to author or review weapon costs and bow behavior.
    **Trigger:** The schema-16 setup paragraph says Iron Axe acquisition and
    upgrade costs use Densewood and says Bow has no damage reward because
    shooting remains unimplemented. The manifest-backed Forge recipe and
    axes.json instead both use 4 Iron + 3 Lightwood for Iron Axe. Server bow
    draw/release and arrow-damage paths are implemented; the authored Bow
    damage reward remains zero.
    **Impact:** The stale material claim may lead a developer to regress the
    Iron Axe costs, and the behavior claim may make the setup guide misstate
    implemented bow shooting. The zero Bow damage reward itself is current.
    **Evidence:** The manifest registers the assigned weapon upgrade files.
    axes.json and the Forge acquisition row both use 4 Iron + 3 Lightwood.
    The 2026-10-19 decision log explicitly sets that cost for both. The updated
    Bow shot contract describes drawing, release and damage; current server
    combat/projectile code and focused Bow draw test cover those paths.
    item-stats.json retains no Bow damage gain. RF61 is scoped to the
    feature-summary wording; this finding is limited to the separate setup
    paragraph. The assigned docs/07-development-setup.md review row remains
    pending and is not marked reviewed here.
    **Proposed repair:** Correct only the setup paragraph's Iron Axe material
    and Bow implementation statements. Retain Densewood for Iron Knife,
    Spear, and Mace, retain Bow's zero damage reward, and do not change runtime
    or catalogue data.
    **Dependencies:** After CR19 review acceptance; no documentation edits
    during CR13.
    **Acceptance:** The setup paragraph states that Iron Axe acquisition and
    upgrade use 4 Iron + 3 Lightwood and accurately describes implemented Bow
    shooting while retaining its zero damage reward; other Iron weapon costs
    remain accurate and runtime/catalogue behavior is unchanged.
    **Focused validation:** Compare the corrected paragraph with the manifest,
    axes.json, bows.json, the Forge acquisition row, the Bow shot design
    contract and server implementation/test references; run git diff --check.
    No Unreal build is required.
  - [ ] **RF100 — Reconcile the knife, pickaxe, and mace reward contract (P3; CR13-007).**
    docs/02-technical-architecture.md:524,1042;
    docs/07-development-setup.md:5174;
    docs/05-decision-log.md;
    Content/Data/GameCatalogues.json:68-70;
    Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json;
    Source/KalmalaGameplay/Private/KalmalaToolCatalogue.cpp:35-57,275-303;
    Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolProgressionCatalogueTest.cpp;
    Source/KalmalaGameplay/Private/Tests/KalmalaItemCatalogueTest.cpp.
    **Affected path:** A developer reviews or changes shared level-up rewards
    using the current architecture, decision, and development-setup guidance.
    **Trigger:** The manifest-backed reward file gives Knives +50 maximum
    durability, +4 Slash and +4 Stab; Pickaxes +10 maximum durability, +3 Crush
    and +3 Pierce; and Stone/Bronze/Iron Maces +50 maximum durability and
    +5 Crush. The architecture and setup passages instead say Knives gain
    +50/+5/+5 and Pickaxes/Maces gain +50/+5 Crush/+5 Pierce; architecture also
    says the three tiered Maces currently gain zero rewards.
    **Impact:** Current data and written contracts disagree, leaving authors
    without a verified balance target and risking inconsistent stat changes.
    **Evidence:** The manifest registers item-stats.json as the reward source;
    its CR01-matching contents contain 15 groups and 41 unique, known IDs with
    bounded values. Git history shows the original file held +50/+5/+5 Knife
    and Pickaxe values and +50/+5 Crush/Pierce Mace values; commit 68a0a8d9 on
    2026-10-15 changed those existing groups to the current values. The
    schema-16 architecture summary was updated on 2026-10-23 but retains the
    earlier values, and the decision log has no entry documenting the
    2026-10-15 retune. The 2026-10-21 delegated A8b choice covers newly added
    A8b rewards and does not establish the intended legacy values. Runtime
    consumes the registered file and validates identity, type, and bounds;
    focused tests do not assert every disputed row.
    **Proposed repair:** Confirm the intended legacy reward values from an
    authoritative decision, then align the architecture and setup descriptions
    with that approved contract. Change item-stats.json only if an approved
    balance target directs it; do not invent values.
    **Dependencies:** After CR19 review acceptance and confirmation of the
    intended reward contract; no repair during CR13.
    **Acceptance:** One approved value set is recorded in the authoritative
    contract, and the manifest-backed data and both guides agree with it. No
    unsupported reward or catalogue change is introduced.
    **Focused validation:** Compare the reconciled passages and data with the
    manifest, runtime reward validation/formula, decision record, and focused
    test references; run git diff --check. No Unreal build is required.
  - [ ] **RF101 — Reconcile the PotatoSeed use description (P3; CR13-008).**
    Content/Data/GameCatalogues/items/crops.json; docs/02-technical-architecture.md:512,946-953; docs/07-development-setup.md:144-147,2002-2018; Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:225-245; Source/KalmalaGameplay/Private/Tests/KalmalaItemCatalogueTest.cpp:390; Source/KalmalaGameplay/Private/Tests/KalmalaM9SourceAcceptanceTest.cpp:937-1000.
    **Affected path:** A player or developer reads the PotatoSeed item detail or tries to use the registered seed in current Garden Plot progression.
    **Trigger:** The registered PotatoSeed definition says "Seeds to grow potatoes," but the approved Garden Plot contract uses one Potato tuber for planting and returns Potatoes; the A0 potato source grants Potatoes and explicitly grants no PotatoSeed. No production planting or acquisition route uses PotatoSeed.
    **Impact:** The player-facing item definition implies a supported use that does not exist. If the item is introduced through a future grant or test setup, it has no current planting path.
    **Evidence:** GameCatalogues.json registers crops.json. FKalmalaGardenPlotState maps PotatoFood planting payment and harvest to Potato; architecture and setup guidance say PotatoSeed remains unused and no A0 source adds it. The M9 source acceptance checks that potato harvest leaves the PotatoSeed quantity unchanged. The item-catalogue test only asserts the registered description; no production route consumes or grants PotatoSeed.
    **Proposed repair:** Reconcile the player-facing item definition and availability with the approved deferred-use contract. Do not invent a seed source or planting mode; any such gameplay route requires separate scope approval. Do not change data during CR13.
    **Dependencies:** After CR19 review acceptance; separate scope approval is required before adding a PotatoSeed source or planting behavior.
    **Acceptance:** The registered item no longer implies a currently supported acquisition/planting path unless that route is separately approved and implemented; the deferred-use contract remains explicit and no unsupported source is added.
    **Focused validation:** Compare the item definition with the manifest, Garden Plot payment/harvest helpers, A0 source and M9 acceptance; run catalogue/reference validation and git diff --check after the repair. No Unreal build is required for a text-only correction.
  - [ ] **RF102 — Reconcile architecture meal and cooking catalogue summaries (P3; CR13-008/015).**
    docs/02-technical-architecture.md:751,757,1062-1077; Content/Data/GameCatalogues.json; Content/Data/GameCatalogues/items/food.json; Content/Data/GameCatalogues/recipes/stations/{cauldron,cooking-rack,frying-pan}.json; Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:126-153; Source/KalmalaGameplay/Private/Tests/KalmalaPlayerStatusTest.cpp:166-258; Source/KalmalaGameplay/Private/Tests/KalmalaItemCatalogueTest.cpp:522-737.
    **Affected path:** A developer uses the architecture meal-use list to determine which prepared food items can invoke the server-owned Steady Meal transaction.
    **Trigger:** The architecture meal allowlist names seven outputs plus Lingonberry and ForestChanterelle, while the current server also accepts RoastedBadger, PeatHeartStew and WolfStew. Its “Current food recipes” section still calls seven rows the active cooking catalogue and omits those three meals; the adjacent Cauldron summary also omits current recipe rows.
    **Impact:** Architecture guidance understates accepted meal-use behavior and the active station recipe catalogue, which can misdirect implementation or review.
    **Evidence:** The manifest registers the food file and all three cooking-station files. The current recipes contain ten meal outputs, including the three omitted meals, plus Berry Recovery Draught, Root Stamina Draught, Rat-tail Tonic and Antidote. IsKnownFoodItem includes the three meals, status tests cover the use path, and the development setup records their recipe behavior. CR13-015 reconfirmed the mismatch against all 14 enabled Cooking Rack, Cauldron and Frying Pan rows.
    **Proposed repair:** Reconcile the architecture meal allowlist and current cooking summaries with the registered recipes and server item-use paths. Keep meal-effect foods distinct from the separate draught, tonic and Antidote transactions; do not change runtime behavior or item data.
    **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
    **Acceptance:** The architecture meal allowlist and current cooking summaries match the registered meal recipes and server allowlist; the recipe overview accounts for current station outputs and keeps draught/tonic/Antidote effects separate. No runtime or item data changes.
    **Focused validation:** Compare the revised sections with IsKnownFoodItem, all manifest-registered cooking recipes and focused status/catalogue tests; run git diff --check. No Unreal build is required.
  - [ ] **RF103 — Restore traceability for legacy crop and food icon provenance (P3; CR13-008).**
    docs/icon-provenance.md; Scripts/Data/catalogue-icon-manifest.csv; Content/Kalmala/UI/Source/IconOriginals/; Content/Kalmala/UI/Source/Icons/; Content/Kalmala/UI/Icons/Items/.
    **Affected path:** A maintainer needs to trace the origin and approved visual constraints before revising or reimporting legacy crop/food item icons.
    **Trigger:** Seventeen current crop/food identities (Carrot, Potato, Rutabaga, Onion and their four seed items; BoarMeat, DeerMeat, CookedBoarMeat, CookedDeerMeat, HearthBroth, MeatStew, RootVegetableSoup, RoastedRootVegetables and DeerRootRoast) have retained original PNG files and icon-manifest/runtime mappings, but no generation record or provenance reference in the current or historical repository icon-provenance sources. Their original PNGs contain no embedded text provenance.
    **Impact:** The artwork and mappings exist, but the repository cannot trace the prompts/source references or visual constraints for these originals, making later revisions difficult to audit. This finding concerns provenance records only; it does not identify missing artwork or mappings.
    **Evidence:** All seventeen IconOriginals PNGs and manifest rows exist; Validate-CatalogueReferences confirms canonical runtime icon mappings. The icon-provenance guide says it retains former per-item and per-batch generation records but has no entries for these identities; history search found no matching record in the consolidated or prior icon documents, and the original PNGs have no text metadata chunks.
    **Proposed repair:** Recover and retain authoritative generation records if available. If they cannot be recovered, state the provenance limitation accurately; do not invent prompts or claim a source-reference history. No icon or game-documentation edits during CR13.
    **Dependencies:** After CR19 review acceptance; any replacement art or new icon workflow requires its own approved scope.
    **Acceptance:** Each affected original has an auditable generation/provenance record or an explicit documented unknown-provenance disposition. Existing artwork and mappings remain intact unless separately authorized.
    **Focused validation:** Check each item identity against its retained original, prepared image, imported texture, icon manifest and provenance disposition; run Scripts/Validate-UIIcons.ps1 -Category Catalogue for the affected IDs and git diff --check. No Unreal gameplay build is required.
  - [ ] **RF104 — Provide captions for designed vocal audio (P3; CR12-011).**
    Source: CR12-011; `docs/01-game-design.md:149,153`;
    `Source/KalmalaUI/Public/KalmalaAmbientAudioSubsystem.h`;
    `Content/Kalmala/Audio/Source/`; source search of `Source`, `Config`, and `Content`.
    **Affected path:** A player encounters designed singing/vocal audio in the
    soundscape.
    **Trigger:** Approved vocal audio plays without its approved caption text and
    timing being presented.
    **Impact:** Players who cannot hear or rely on audio lack the subtitle support
    required from the vertical slice. No vocal asset or current caption path was
    found; no defect involving existing vocal content is alleged.
    **Evidence:** The design names distant singing and requires subtitles from the
    vertical slice. The assigned audio-subsystem header exposes ambience, status,
    movement, and accepted-result cues but no caption surface; the audio source
    directory contains no vocal/singing asset, and searches across Source, Config,
    and Content found no caption/subtitle runtime path. Caption content/timing
    acceptance for future vocal content is not defined.
    **Proposed repair:** Define and implement a readable caption path for approved
    vocal audio only after caption content and timing are approved. Do not invent
    caption text, cues, or a taxonomy during repair.
    **Dependencies:** After CR19 review acceptance and approval of caption
    content/timing; no repair during CR12 review.
    **Acceptance:** Before vocal audio ships, its approved caption text and timing
    are defined and every approved vocal cue presents them in a readable path that
    respects existing accessibility text-scale and contrast controls. No
    unapproved vocal cue or caption text is introduced.
    **Focused validation:** After the vocal asset and caption contract are approved,
    add focused automation verifying each approved cue has matching caption data
    and that the caption is presented for the cue; inspect rendered captions at
    default and enlarged text scale/high contrast. No test was run during this review.

  - [ ] **RF105 — Add the required adjustable camera-shake setting (P3; CR12-011).**
    Source: CR12-011; `docs/01-game-design.md:153`;
    `docs/02-technical-architecture.md:2434-2437`; current settings source/config search.
    **Affected path:** A local player configures the promised camera-shake accessibility
    option in Settings.
    **Trigger:** The player seeks the 0–100% camera-shake control, but Accessibility
    has no such slider. No current camera-shake effect was found.
    **Impact:** The required local adjustable camera-shake setting is unavailable.
    This finding concerns the missing settings contract only; it does not allege
    or propose a new camera-shake effect.
    **Evidence:** Game design requires adjustable camera shake from the vertical
    slice onward; the architecture retains an unchecked `Camera shake — slider
    0-100%` candidate. Searches of Source, Config, and Content found no setting,
    camera-shake API/effect, or saved local value. Existing settings documentation
    contains no camera-shake setting.
    **Proposed repair:** Add a local, focusable 0–100% Accessibility slider with a
    visible value and local persistence. Any separately approved camera-shake
    effects must consume that value; do not invent an effect or default.
    **Dependencies:** After CR19 review acceptance and any product approval needed
    for effect/default semantics; no repair during CR12 review.
    **Acceptance:** The Accessibility slider accepts only 0–100%, exposes a visible
    value to keyboard/controller users, and persists only in local settings without
    an RPC, world/player save field, or replicated property. Any approved camera-shake
    effect reads the preference; no effect is added without separate approval.
    **Focused validation:** Extend Settings accessibility automation to verify
    focusability, bounds, visible value, local persistence, and no gameplay RPC or
    replicated state; if an approved effect exists, verify it observes the setting.
    No test was run during this review.
  - [ ] **RF106 — Reconcile Smelter and material-category guidance (P3; CR13-009).**
    Content/Data/GameCatalogues/items/buildItems/other.json; Content/Data/GameCatalogues/items/materials/ore.json; Content/Data/GameCatalogues/items/materials/metal.json; docs/01-game-design.md:172; docs/02-technical-architecture.md:510; docs/07-development-setup.md:5210; Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:409-411; Source/KalmalaGameplay/Private/Tests/KalmalaA10FreshStartBronzeAndIronTest.cpp:389-470.
    **Affected path:** A developer consults the Smelter and material-category descriptions to determine current ore, bar and refining routes.
    **Trigger:** The design summary describes only Iron Ore plus Coal producing Iron, and the setup catalogue summary says Ore contains only Stone and Metal only Iron. The schema-16 catalogue registers Copper, Tin and Iron ores and bars; the current server accepts all three Smelter paths.
    **Impact:** Current authored resource and processing paths are understated, leaving conflicting guidance for implementation and review.
    **Evidence:** The manifest registers ore.json, metal.json and the Smelter buildable. KalmalaStorageInteraction.cpp maps CopperOre, TinOre and IronOre to distinct server processing modes; the A10 test exercises matching Copper, Tin and Iron bars. The architecture contract at line 510 documents all three.
    **Proposed repair:** Reconcile the game-design Smelter sentence and setup material-category list with the current registered data and architecture contract. Do not alter recipes, catalogue data or runtime behavior during CR13.
    **Dependencies:** After CR19 review acceptance; no data or gameplay changes during CR13.
    **Acceptance:** Current guidance lists the registered ore/bar categories and all three accepted Smelter outputs without changing the existing processing contract.
    **Focused validation:** Compare the corrected passages with GameCatalogues.json, ore.json, metal.json, server Smelter handling and the focused A10 test; run Scripts/Validate-CatalogueReferences.py and git diff --check. No Unreal build is required.
  - [ ] **RF107 — Clarify the Grinding Stone repair scope (P3; CR13-009).**
    Content/Data/GameCatalogues/items/buildItems/grinding-stone.json; docs/04-roadmap.md:364; docs/07-development-setup.md:4272-4274; Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:700-765.
    **Affected path:** A player reads the Grinding Stone description before using its in-world Repair All action.
    **Trigger:** The description promises repair of all worn items, while the server repair path validates the Grinding Stone and repairs only entries in the owner's carried-tool list.
    **Impact:** The description can imply wearable gear or other condition-bearing inventory items are repaired when the action does not touch those systems.
    **Evidence:** RepairAllToolsFromServer calls CommitRepairAllToolsFromServer, which passes Character->CarriedTools to BuildServerRepairAll and persists that carried-tool array. The roadmap and setup contract also define the action as repairing carried tools.
    **Proposed repair:** Narrow the Grinding Stone description to repairing all carried tools at once; do not add gear repair behavior.
    **Dependencies:** After CR19 review acceptance; no catalogue edits during CR13.
    **Acceptance:** The description accurately states the supported carried-tool repair scope and does not promise wearable or inventory-wide repair.
    **Focused validation:** Compare the corrected description with the server repair path, carried-tool validation and roadmap/setup contract; run git diff --check. No Unreal build is needed for the text-only correction.
  - [ ] **RF108 — Correct the Garden Plot farming status (P3; CR13-009).**
    docs/01-game-design.md:172; docs/game-features.md:145; docs/02-technical-architecture.md:512; Content/Data/GameCatalogues/items/buildItems/other.json; Source/KalmalaGameplay/Private/KalmalaCultivationInteraction.cpp; Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp; Source/KalmalaGameplay/Private/Tests/KalmalaPlantingTrowelTest.cpp.
    **Affected path:** A player or developer consults the current feature summary or game-design status to learn whether Garden Plot cultivation is implemented.
    **Trigger:** The design and feature summaries say farming is unimplemented, while the current server supports Garden Plot planting, active-time growth and mature-crop harvest.
    **Impact:** Current Garden Plot behavior is omitted from status guidance and is easy to mistake for deferred work.
    **Evidence:** The registered GardenPlot buildable and direct recipe are present; the architecture contract documents Planting Trowel payment, saved crop mode, active-time growth and harvest. Server cultivation handling and the focused planting test cover the flow. PotatoSeed remains intentionally unused under the separate RF101 handoff.
    **Proposed repair:** Update or date-scope both status statements to reflect the implemented A7 Garden Plot flow and retain the explicit PotatoSeed limitation. Do not change gameplay or add crop behavior during CR13.
    **Dependencies:** After CR19 review acceptance; documentation only.
    **Acceptance:** Both passages distinguish implemented Garden Plot cultivation from any broader deferred farming scope and do not imply PotatoSeed has a supported route.
    **Focused validation:** Compare the corrected status with the registered GardenPlot definition, cultivation/harvest server path, architecture contract and focused test source; run git diff --check. No Unreal build is required.
  - [ ] **RF109 — Correct the current buildable recipe count (P3; CR13-009/014).**
    docs/02-technical-architecture.md:506; docs/07-development-setup.md:113-116; Content/Data/GameCatalogues.json; Content/Data/GameCatalogues/items/buildItems/; Content/Data/GameCatalogues/recipes/building.json; Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp.
    **Affected path:** A maintainer uses architecture and setup counts to assess buildable catalogue and construction-recipe coverage.
    **Trigger:** The current architecture text says there are 19 placement recipes, while the schema-16 manifest registers all seven buildItems files, which contain 22 unique buildables; the building recipe file contains 22 unique outputs. The setup's 18-object statement is under a dated 2026-10-18 heading and may be a historical snapshot.
    **Impact:** An unqualified current count understates the registered buildable and direct-recipe coverage.
    **Evidence:** CR13-009 found seven registered buildItems files with 22 unique IDs and 22 building-recipe outputs. CR13-014 reconfirmed the 22 registered, unique direct-material outputs against the assigned file's CR01 and current-main-matching fingerprint. The catalogue reference validator passes and the loader routes direct construction outputs to BuildableOutput.
    **Proposed repair:** Update the current architecture count to 22. Retain the setup figure only if it is clearly treated as an as-of-2026-10-18 snapshot; otherwise update its current count. Do not alter recipes or buildable data.
    **Dependencies:** After CR19 review acceptance; no catalogue or runtime edits during CR13.
    **Acceptance:** No current unqualified count conflicts with the 22 registered buildables/recipes; any retained 18-object statement is clearly historical.
    **Focused validation:** Parse the manifest, buildItems and building recipe outputs; run Scripts/Validate-CatalogueReferences.py and git diff --check. No Unreal build is required.
  - [ ] **RF110 — Quote paths passed to the icon-import Editor (P3; CR14-001).**
    Scripts/Import-CatalogueIconBatch.ps1:100-108 and
    Scripts/Import-StatusIconAssets.ps1:110-118.
    **Affected path:** A maintainer imports either icon batch from a repository
    or Windows temporary directory whose absolute path contains spaces.
    **Trigger:** Both wrappers pass an array containing the project, UserDir,
    log, and Python script paths to Start-Process -ArgumentList without escaped
    quotes. The cmdlet joins array elements with spaces; embedded PowerShell
    string quotes are not forwarded as process quoting.
    **Impact:** Windows splits those paths into separate arguments, so Unreal
    receives truncated project/user/log/script paths and the icon import fails
    before its completion markers. This is a tooling argument-handling failure,
    not a gameplay authority issue.
    **Evidence:** Both argument arrays contain path-valued entries without
    quoting. Microsoft Start-Process documentation states that an argument
    array is joined with spaces and path values containing spaces must be
    surrounded with escaped double quotes.
    **Proposed repair:** Build the process command line with a quoting helper
    that correctly quotes every path-valued argument, or use a ProcessStartInfo
    API with explicit argument handling; preserve the existing flags and
    timeout/error checks.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** Catalogue and status icon imports succeed with the repo
    path and temporary path containing spaces; each expected project, script,
    user, and log path arrives intact. Existing short-path imports still pass.
    **Focused validation:** Add a narrow launch-argument capture or run each
    wrapper against a disposable spaced-path mirror and verify completion
    markers and expected package outputs after the required Editor setup. No
    import was run during this review.
  - [ ] **RF111 — Reject generated outputs that exceed MAX_PATH (P3; CR14-001/002/005).**
    Scripts/Create-MenuMap.py:9,46; Scripts/Export-WorldMaps.ps1:3,11-12,22,42;
    Scripts/Generate-AudioSources.ps1:12-26,1311-1316;
    Scripts/Setup-PrototypeEnvironment.py:8-11,68-69;
    Source/KalmalaEditor/Private/CreateLightingComparisonFixtureCommandlet.cpp:48-55,62-65,90-101,183-190;
    CreatePrototypeMapCommandlet.cpp:20-35,54-59; CreateWorldMaterialsCommandlet.cpp:87-113,204-206,243-310,314-340,459-461,471-510,520-587,597-628,782-828;
    ExportWorldMapsCommandlet.cpp:96-102,156-172,184-192; RenderWorldGenerationVisualizationCommandlet.cpp:45-57,136-142,297-307,573-575,645-694.
    **Affected path:** A developer creates the Home map, updates L_Prototype
    lighting, exports world-map PNGs, generates terrain/material assets or
    previews, or generates audio when a full destination reaches 260 characters.
    **Trigger:** Create-MenuMap saves the fixed /Game/Kalmala/Maps/Menu/L_Menu
    asset without checking its resolved .umap path. Setup-PrototypeEnvironment
    loads and saves /Game/Kalmala/Maps/Prototype/L_Prototype without checking
    its resolved .umap path. Export-WorldMaps accepts any Output directory and
    creates it without a length check before launching Unreal and reading
    Output/LastRender.txt. Generate-AudioSources derives WAV destinations from
    the checkout root and creates them without checking their expanded paths.
    The five listed Editor commandlets likewise create or save resolved project
    packages or caller-selected output files without checking the complete
    destination; ExportWorldMaps also writes a `.tmp` sibling before replacing
    each PNG. RenderCreatures and RenderMenu do normalize and reject outputs at
    260 characters, so they are not included.
    **Impact:** On Windows without long-path support, a destination at or beyond
    260 characters can fail during map load/save, directory creation, commandlet
    output, or WAV writing, sometimes after generation or in-memory map edits
    have begun. This conflicts with the project path contract and leaves failure
    behavior to the OS or Unreal tools.
    **Evidence:** The original four destination writers lack a full path bound.
    A 240-character checkout root leaves Kalmala.uproject at 257 characters
    while the Home map .umap path reaches 278. A 212-character root leaves
    Kalmala.uproject at 229 while the L_Prototype .umap path reaches 260.
    The five Editor commandlets do not check the resolved or constructed full
    paths before their output side effects. RenderCreatures and RenderMenu do
    normalize and reject outputs at 260 characters; Export-UITextCrop, both
    icon importers, and Package-And-Run also have relevant explicit path checks.
    **Proposed repair:** Resolve both map file paths, normalize the exporter
    directory and every concrete generated destination, and reject any path at
    260 characters or longer before loading/saving maps or assets, creating
    directories, writing previews, or starting Unreal/audio generation.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** Destinations below 260 remain supported. Home and
    L_Prototype map paths, configured exporter directories, fixed generated
    project packages, numeric-sample outputs, world-generation previews and
    checkout-derived WAV paths at 260 or more are rejected before loading,
    saving, creating output, or launching generation, without relying on
    Windows long-path support.
    **Focused validation:** Exercise 259/260-character boundaries for both map
    assets, exporter output, commandlet preview/sample outputs, and audio
    destinations without creating output for rejected paths; verify short
    defaults remain valid. Setup rejects before loading or saving the map. No
    generation or map setup was run during review.
  - [ ] **RF113 — Make Inventory actions reachable with keyboard and controller focus (P2; CR12-012).**
    Source: CR12-012; current main-checkout paths
    Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:391-406,604-613,1406-1412
    and Source/KalmalaUI/Private/KalmalaInventoryGridWidget.cpp:346-380, inspected
    read-only; docs/02-technical-architecture.md:2583-2618 and docs/04-roadmap.md:417,737-760.
    **Affected path:** A local player opens Inventory and needs to equip or return gear,
    eat food, repair a tool, or prefer an arrow stack with keyboard/controller input.
    **Trigger:** Inventory opens with focus on the pack grid; the player tries Tab or
    arrow/D-pad navigation to reach a visible gear or action button.
    **Impact:** Tab closes Inventory, while arrow/D-pad input is consumed inside the
    grid and does not move focus to its buttons. Keyboard and D-pad users cannot reach
    these existing actions without pointer input. No authority or peer-privacy defect
    was found.
    **Evidence:** Open focuses GridView. NativeOnPreviewKeyDown closes on Tab, and
    NavigateInventoryMenu does not route keys while GridView has focus. The grid
    consumes arrows/D-pad for cell movement and Enter/Gamepad FaceButton Bottom for
    slot moves. Gear, Eat, Repair, and Prefer Arrows are existing button controls with
    click handlers but no keyboard/D-pad focus route from the grid. The architecture
    says Tab reaches labelled buttons, and M12 requires accessible existing actions
    and keyboard/controller navigation.
    **Proposed repair:** Add a visible keyboard/controller focus path from the pack
    grid through applicable gear controls and existing action buttons, and back to the
    grid. Preserve a clear menu-close path, text-entry behavior, and the existing
    server-validated actions; focus or selection alone must not invoke an action.
    **Dependencies:** After CR19 review acceptance; no implementation during CR12.
    **Acceptance:** Keyboard and controller users can move focus from the grid to each
    applicable gear/action button, see the focused state, activate the existing
    action, and return to the grid. Tab/I close and text-entry behavior remain usable;
    closing restores input without replaying an action. No new gameplay request,
    authority path, or peer-visible state is added.
    **Focused validation:** Extend Kalmala.UI.InventoryMenu.Selection and the rendered
    Inventory acceptance to exercise keyboard/D-pad focus traversal and activation
    for gear, Eat, Repair, and Prefer Arrows controls, including focus return, visible
    focus, and close/input restoration. Run after an Editor build and inspect standard
    and enlarged-text/high-contrast host/client cases. No test was run during this review.

  - [ ] **RF114 — Correct stale current armor status in the inventory architecture (P3; CR13-011).**
    docs/02-technical-architecture.md:1479-1489; docs/01-game-design.md:168-170; docs/02-technical-architecture.md:1136; docs/07-development-setup.md:1404-1410; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:322-358; Source/KalmalaGameplay/Private/KalmalaDamageProfile.cpp:88-104; Source/KalmalaGameplay/Private/Tests/KalmalaDamageProfileTest.cpp:49-89; Content/Data/GameCatalogues/items/equipment/armor/{boots,capes,helmets,leggings,shields,tunics}.json.
    **Affected path:** A developer or reviewer uses the architecture's inventory and combat notes to assess current wearable Armor behavior.
    **Trigger:** The undated shared-inventory section says current Armor remains 0 because equipped defenses are not applied, and says Armor items/equipment rules require separate gameplay work.
    **Impact:** Readers may treat wearable defenses as unavailable or duplicate already implemented work.
    **Evidence:** Together, the six schema-16 armor files contain 16 registered profiles with positive Armor values. The server wildlife-hit path sums non-depleted equipped Armor and passes it to the damage resolver; the resolver subtracts it from physical damage. Current game-design/architecture contracts and focused damage-profile assertions describe the same behavior.
    **Proposed repair:** Update or remove only the obsolete Armor-zero and pending-equipment statements to match the current server calculation. Retain the still-applicable weight/encumbrance limitations. Do not alter runtime or catalogue data.
    **Dependencies:** After CR19 review acceptance; no runtime or catalogue edits during CR13.
    **Acceptance:** Current architecture no longer states that wearable defenses are unapplied or that Armor equipment remains unimplemented; weight/encumbrance limitations stay accurate.
    **Focused validation:** Compare the corrected passage with the game-design contract, server gear-Armor path and focused damage-profile test; run git diff --check. No Unreal build is required.
  - [ ] **RF115 — Restore the authoritative A8b wearable-value source (P3; CR13-011).**
    docs/05-decision-log.md:5,186; docs/icon-provenance.md:233; docs/alpha-item-details.md#a8b (missing); docs/07-development-setup.md:5218-5258,6120-6147,6149-6170; docs/a9-acceptance.md:215-222.
    **Affected path:** A developer or reviewer follows the decision log to verify delegated A8b wearable profile values and follows the A9 icon notes to their source contract.
    **Trigger:** The current decision log declares the detailed A8b contract in alpha-item-details.md authoritative until user adjustment, and the A9 decision/icon-provenance text cites that file, but the target is absent from the current tree.
    **Impact:** A8b profile decisions cannot be traced to their declared authority; some values are only partially repeated in dated setup histories. The A9 values remain in a separate current acceptance record, but its citations are stale.
    **Evidence:** The target does not exist and Git records its deletion at 9a8cbc0. Current docs/07 contains item-specific histories for some A8b wearables but no complete A8b value contract (including the Badger Fur Vest profile); docs/a9-acceptance.md retains the Opal Helm table.
    **Proposed repair:** Preserve the authoritative A8b wearable values in an existing current source or restore an appropriate concise contract; repoint the A9 references to docs/a9-acceptance.md and mark historical provenance clearly. Do not change catalogue values or gameplay.
    **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
    **Acceptance:** The A8b decision points to an existing complete source, and A9 value/provenance references resolve to retained documentation or are explicitly historical; no runtime or catalogue behavior changes.
    **Focused validation:** Resolve the referenced Markdown targets and compare the retained wearable contract with the registered profiles and A9 acceptance table; run git diff --check. No Unreal build is required.
  - [ ] **RF116 — Clarify the Boar Hide Cape warmth description (P3; CR13-011).**
    Content/Data/GameCatalogues/items/equipment/armor/capes.json:4-7; Source/KalmalaUI/Private/KalmalaItemDetailWidget.cpp:13-18,90; docs/04-roadmap.md:422,797-799; docs/05-decision-log.md:9; docs/07-development-setup.md:1404-1410.
    **Affected path:** A player reads the Boar Hide Cape description in the inventory item-detail presentation and chooses gear for cold conditions.
    **Trigger:** The description calls it a “warm cape,” while the approved A4 decision defers insulation/cold protection for the hide Boots/Cape and the wearable schema authors no warmth field.
    **Impact:** In this survival game, the player-facing adjective may imply a cold-protection effect the current contract explicitly defers.
    **Evidence:** The item-detail widget presents the catalogue Description. The A4 decision says to defer insulation/cold protection; the current armor schema and server calculations provide Armor/durability behavior only.
    **Proposed repair:** Remove or qualify “warm” so the description does not imply a current cold-protection mechanic. Adding such a mechanic would need separate approved scope; do not change data or gameplay during CR13.
    **Dependencies:** After CR19 review acceptance; no catalogue or gameplay edits during CR13.
    **Acceptance:** The player-facing description does not promise unsupported cold protection and remains consistent with the A4 decision; no runtime behavior changes.
    **Focused validation:** Compare the revised text with the item-detail presentation and A4 cold-protection decision; run the focused item-catalogue/detail checks when repaired and git diff --check. No Unreal build is required for this review handoff.

  - [ ] **RF117 — Keep preview caches aligned with authored files (P3; CR14-002).**
    Scripts/Render-Creatures.ps1:40-50,119-153 and
    Scripts/Render-Menu.ps1:39-49,127-164.
    **Affected path:** A developer reuses a preview cache after deleting,
    renaming, or replacing authored files in Source, Config, Content, or Plugins.
    **Trigger:** Both renderers copy an existing source file only when its
    LastWriteTimeUtc differs and never remove cached authored files no longer
    present in the current tree. The native freshness digest hashes current
    Source/Plugins and Kalmala.uproject, not the cache contents. With
    -BuildModules, the renderer builds the stale cache and then writes the digest
    of current sources, so a later render treats that cache as fresh. A content
    change with a preserved timestamp is also skipped by the copy predicate.
    **Impact:** Builds can compile deleted C++/plugin sources or render stale
    copied content; a successful build can stamp and reuse a cache that does not
    represent the current checkout.
    **Evidence:** The directory loops only copy missing or timestamp-different
    files; neither renderer prunes files absent from Source/Config/Content/
    Plugins. Each computes nativeSources from the current source tree and
    writes sourceDigest after Build.bat succeeds. The documented workflow reuses
    the cache and calls -BuildModules after native source changes.
    **Proposed repair:** Reconcile the cache-owned authored directories against
    the current source set, compare content as well as timestamps, remove stale
    authored files only within those owned directories, and stamp only after
    verifying the cache's native inputs match the current digest.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** After a seeded cache has a source removed/renamed, the next
    build contains no stale source and succeeds against the current tree; a
    same-timestamp content change is copied; the freshness stamp cannot mark a
    mismatched cache current. Unchanged renders still reuse the cache.
    **Focused validation:** Seed a disposable cache with an authored .cpp and
    content file, remove/rename the source and change another file while
    preserving its timestamp, then verify cache reconciliation and stamp
    behavior before rendering.
  - [ ] **RF118 — Keep catalogue validation active under Python optimization (P3; CR14-002).**
    Scripts/Validate-CatalogueReferences.py:20,24-28,33,55-56,65,71,74,81,85.
    **Affected path:** A maintainer runs the documented catalogue-reference
    validator with Python optimization enabled by -O or PYTHONOPTIMIZE.
    **Trigger:** Every data-integrity gate in the validator uses assert.
    Python removes assert statements in optimized mode, but the script continues
    through its audit and prints PASS, including when references, registrations,
    or icon mappings are invalid.
    **Impact:** The no-build catalogue validation can report success while
    missing broken JSON references, unregistered files, or runtime/icon
    mappings.
    **Evidence:** All structural/reference checks are assert statements, and a
    harmless python -O probe with assert False printed PASS. The documented
    command is python Scripts/Validate-CatalogueReferences.py.
    **Proposed repair:** Replace assert-based validation with explicit checks
    that raise a clear exception or return a nonzero exit status; print PASS
    only after all checks complete.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** Valid current catalogues pass in normal and optimized
    Python; a synthetic missing reference, unregistered JSON file, and missing
    icon mapping fail under both modes without printing PASS.
    **Focused validation:** Run the validator against current data and
    read-only or temporary invalid fixtures under normal Python and python -O.
  - [ ] **RF119 — Check cache containment on a directory boundary (P3; CR14-002).**
    Scripts/Render-Creatures.ps1:65-70 and Scripts/Render-Menu.ps1:79-84.
    **Affected path:** A developer chooses a documented custom cache beside the
    checkout, such as C:\dev\Kalmala-cache for sourceRoot C:\dev\Kalmala.
    **Trigger:** Both scripts reject Cache when Cache.StartsWith(sourceRoot,
    OrdinalIgnoreCase), without requiring a following directory separator.
    The sibling C:\dev\Kalmala-cache therefore matches the prefix even though
    it is not inside C:\dev\Kalmala.
    **Impact:** Valid external cache paths that share the checkout's name prefix
    fail before a render, despite the documented -Cache option.
    **Evidence:** A no-write path comparison returned StartsWith=true and
    child-boundary=false for C:\dev\Kalmala and C:\dev\Kalmala-cache.
    Development setup documents -Cache as changing the renderer cache.
    **Proposed repair:** Compare normalized paths with a directory boundary,
    rejecting the source root itself and descendants while accepting siblings
    with a shared textual prefix.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** Both renderers reject the checkout and its true descendants,
    but accept an external sibling cache named <checkout>-cache; defaults stay
    unchanged.
    **Focused validation:** Add pure path tests for exact root, child, sibling
    sharing a prefix, case variation, and a different drive without creating
    cache directories.

  - [ ] **RF120 — Reconcile stale weapon profile and combat summaries (P3; CR13-012).**
    `docs/02-technical-architecture.md:1036,1040,1042,1136`;
    `docs/07-development-setup.md:5162`;
    `Content/Data/GameCatalogues/items/equipment/weapons/{axes,knives,maces,pickaxes,spears}.json`;
    `Content/Data/GameCatalogues/attacks/attacks.json`;
    `Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json`.
    **Affected path:** A developer or reviewer uses current-looking architecture/setup summaries to understand weapon durability, profile damage channels, and effective Basic values.
    **Trigger:** Architecture reports obsolete Stone starter values, Iron Knife/Pick/Spear values, Mace durability/channel claims, and effective Basic attack examples. The undated setup Mace section also says each tier uses knife durability and Crush exclusively, while Bronze/Iron Maces author Stab bonuses.
    **Impact:** Readers can misunderstand current weapon statistics and channels or derive incorrect expected combat values.
    **Evidence:** Schema-16 profiles author current durability and `DamageBonuses`; shared attack lists add Basic bonuses, and `KalmalaDamageProfileTest.cpp` asserts resulting combat values. The setup passage is presented as current guidance. This excludes the separate Mace reward disagreement (RF100), Iron Pick cost (RF122), and Bow status statement (RF125).
    **Proposed repair:** Correct only the named profile, channel, durability, and effective Basic summaries against the current profile/attack/test contract. Keep RF100's unresolved reward target separate; do not change catalogue values or runtime behavior.
    **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
    **Acceptance:** Named architecture/setup passages distinguish profile bonuses, shared Basic bonuses, and current durability/channels accurately; RF100 stays separate; no runtime or catalogue behavior changes.
    **Focused validation:** Compare each corrected value with the schema-16 profile, shared attack list and focused damage-profile test; retain RF100's authority note; run git diff --check. No Unreal build is required.
  - [ ] **RF121 — Qualify Bronze Spear's range description (P3; CR13-012).**
    `Content/Data/GameCatalogues/items/equipment/weapons/spears.json:15`;
    `Source/KalmalaUI/Private/KalmalaItemDetailWidget.cpp:21-44`;
    `Source/KalmalaGameplay/Public/KalmalaCombatComponent.h:64`;
    `Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:174-200`.
    **Affected path:** A player reads the Bronze Spear description in carried-item details and chooses it for combat reach.
    **Trigger:** Its description calls it a “Long range weapon for keeping your enemies at a distance,” but the profile has no per-weapon reach field and server melee uses a shared 220 cm range.
    **Impact:** The wording can imply a reach advantage the current weapon profile and server combat path do not provide.
    **Evidence:** The carried-tool detail function falls back to the registered BronzeSpear profile description. `AttackRange` is shared across melee attacks; profiles have no reach field.
    **Proposed repair:** Replace or qualify the range wording; weapon-specific reach requires separate approved gameplay scope. Do not change combat behavior during CR13.
    **Dependencies:** After CR19 review acceptance; no catalogue or runtime edits during CR13.
    **Acceptance:** The player-facing description does not promise a unique reach mechanic absent from the current server contract; gameplay is unchanged.
    **Focused validation:** Compare repaired item-detail text with the profile schema and shared server range; run focused catalogue/detail checks when repaired and git diff --check. No Unreal build is required.
  - [ ] **RF122 — Correct the stale Iron Pick Forge cost in architecture (P3; CR13-012).**
    `docs/02-technical-architecture.md:1040`;
    `Content/Data/GameCatalogues/recipes/stations/forge.json:63-78`;
    `docs/05-decision-log.md:11`;
    `Content/Data/GameCatalogues.json`.
    **Affected path:** A developer follows architecture guidance to author or review the Iron Pick Forge recipe.
    **Trigger:** Architecture says 4 Iron + 2 Wood + 2 Stone; the registered Forge row and user-directed decision set Forge level 1 at 10 Wood + 10 Iron.
    **Impact:** The obsolete cost can cause incorrect implementation or regression expectations.
    **Evidence:** The schema-16 manifest registers the Forge recipe file; its IronPick row has 10 Wood + 10 Iron, matching the explicit 2026-10-20 decision-log target.
    **Proposed repair:** Correct only the architecture cost to the authorized recipe. Keep RF99's distinct setup paragraph separate; do not change recipe data.
    **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
    **Acceptance:** Architecture, decision log and registered Forge recipe state the same Iron Pick cost; no runtime or data change.
    **Focused validation:** Compare the revised sentence with `forge.json`, the manifest and decision row; run git diff --check. No Unreal build is required.
  - [ ] **RF123 — Restore provenance records for selected existing weapon, material and utility icons (P3; CR13-012/013/015).**
    `docs/icon-provenance.md`; `docs/07-development-setup.md:4819`;
    `Content/Data/GameCatalogues/recipes/stations/inventory.json:4-11`;
    `Content/Kalmala/UI/Source/IconOriginals/{StoneAxe,BronzeAxe,IronAxe,StoneKnife,BronzeKnife,IronKnife,StonePick,BronzePick,IronPick,StoneSpear,BronzeSpear,IronSpear,FrostSalt,Amber,BoarHide,DeerHide,Coal,Densewood,Lightwood,Wood,Stone,ConstructionHammer}.png`;
    `Content/Kalmala/UI/Source/Icons/`; `Content/Kalmala/UI/Icons/Items/`.
    **Affected path:** A maintainer traces the generation source for an existing weapon, material or utility icon before preparing or replacing it.
    **Trigger:** The twelve weapon, nine material and ConstructionHammer original artworks and their canonical mappings/imported assets exist, but no item-specific generation prompt/source records are available for these 22 IDs; the ConstructionHammer PNG also has no text provenance metadata.
    **Impact:** Artwork and runtime mappings remain available, but the provenance guide cannot verify or reproduce these icons' original generation sources.
    **Evidence:** Catalogue validation and the focused ConstructionHammer UI icon validator confirm the assets and mappings exist. The current provenance guide and repository search contain no item-specific ConstructionHammer record, and its original PNG has no text metadata. RF103 retains its crop/food identity scope.
    **Proposed repair:** Add original prompt/source records where recoverable and explicitly record unavailable provenance otherwise. This is not a missing-art or missing-mapping finding; do not replace or regenerate assets during CR13.
    **Dependencies:** After CR19 review acceptance; no icon or documentation edits during CR13.
    **Acceptance:** Each of the 22 existing canonical weapon/material/utility icons has a source/prompt record or explicit unavailable-provenance disposition; assets and mappings remain intact.
    **Focused validation:** Check all 22 IDs against original/prepared/imported assets and mappings and resolve each provenance entry; run catalogue reference/icon validators and git diff --check. No Unreal build is required.
  - [ ] **RF125 — Remove the stale Bow shooting status from architecture (P3; CR13-012).**
    `docs/02-technical-architecture.md:524`;
    `Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp`;
    `Source/KalmalaGameplay/Private/KalmalaArrowProjectile.cpp`;
    `Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp`;
    `Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json`.
    **Affected path:** A developer reads the architecture's current Bow progression summary when reviewing ranged combat.
    **Trigger:** It says Bow has no damage reward “while shooting remains unimplemented.” The zero damage reward remains accurate, but Bow draw/release and server projectile damage are implemented.
    **Impact:** The current status is misrepresented and may misdirect future work or make implemented behavior appear deferred.
    **Evidence:** Server Bow draw/release and projectile damage paths exist, with a focused Bow draw test; the reward catalogue contains no Bow damage gain. RF61 covers a separate feature-summary statement; RF99 covers a setup paragraph.
    **Proposed repair:** Remove the obsolete implementation-status rationale while retaining the accurate zero-damage-reward statement. Do not change shooting or reward data.
    **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
    **Acceptance:** Architecture no longer says Bow shooting is unimplemented and still states the accurate zero damage reward; runtime/data are unchanged.
    **Focused validation:** Compare the revised statement with reward data, server Bow/projectile paths and focused test; run git diff --check. No Unreal build is required.
  - [ ] **RF126 — Resolve the Stone Mace cost authority mismatch (P3; CR13-012).**
    `docs/02-technical-architecture.md:1034`;
    `Content/Data/GameCatalogues/recipes/stations/workbench.json:113-129`;
    `docs/05-decision-log.md`.
    **Affected path:** A developer or reviewer follows architecture and the Workbench recipe when assessing Stone Mace's acquisition cost.
    **Trigger:** Architecture says Workbench Stone Mace costs 1 Wood + 1 Stone; the registered `StoneMace` row costs 2 Wood + 2 Stone. No current decision-log authority for this discrepancy was found.
    **Impact:** The intended cost is unclear; changing either source without direction risks an unsupported balance decision.
    **Evidence:** The schema-16 manifest registers the Workbench file and its StoneMace row uses 2 Wood + 2 Stone at level 1. Architecture says 1 Wood + 1 Stone; the current decision log contains no StoneMace cost decision.
    **Proposed repair:** Obtain or locate the authoritative intended cost, then align architecture and recipe as directed. Do not guess the target or change data during CR13.
    **Dependencies:** Requires authoritative product direction before any data correction; no data or documentation edits during CR13.
    **Acceptance:** An explicit decision establishes the intended cost and architecture/recipe agree with it; no unsupported balance target is inferred.
    **Focused validation:** Compare both passages with the approved decision and schema-16 recipe after direction is available; run reference validation and git diff --check. No Unreal build is required.

  - [ ] **RF127 — Remap expanded-map interaction controls (P3; CR12-015).**
    Source: CR12-015; current main-checkout paths
    E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaSettingsWidget.cpp:360-398,1192-1203;
    E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldMapSubsystem.cpp:48-54;
    E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:1641-1684;
    docs/01-game-design.md:151-153; docs/02-technical-architecture.md:308;
    docs/04-roadmap.md:983-1017; docs/07-development-setup.md:3485-3501.
    **Affected path:** A local player opens the expanded map and wants to customize
    keyboard/controller controls for zoom, pin selection/placement/state, marker
    filtering, or pings through the existing local input settings.
    **Trigger:** The player needs to replace a fixed in-map shortcut; Settings exposes
    the remappable WorldMap open action, but the in-map commands are handled by
    literal key comparisons and have no mapping rows.
    **Impact:** The design's remappable-input accessibility baseline does not cover
    expanded-map operations. Players who cannot use the fixed key/controller defaults
    cannot customize those map actions through Settings. Mouse alternatives remain;
    no gameplay authority or peer-privacy defect was found.
    **Evidence:** GetLocalInputDefinitions contains WorldMap (M/N and Gamepad
    Special Right) but no in-map zoom, pin, filter, or ping actions; the local-player
    subsystem binds only WorldMap. The widget directly checks EKeys for 1-3, Q,
    Tab, P, Enter, H, Delete, Page Up/Down, and marker-filter navigation. Map input
    ownership is local; pin state and filtering stay owner-local, while pings remain
    server-validated. Game design requires remappable input
    from the vertical slice onward. Architecture and setup defer map remapping to
    later accessibility work, but the decision log contains no waiver of that
    baseline. Existing tests exercise local map actions and general settings
    rebinding but do not verify remapped map actions.
    **Proposed repair:** Expose the existing map keyboard/controller commands through
    the existing local remapping settings and dispatch them using the selected
    bindings. Keep current actions/defaults and mouse alternatives; add no gameplay
    request, network state, or save field.
    **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
    **Acceptance:** Existing map keyboard/controller commands can be rebound through
    the existing local settings path, and current binding labels are accurate in the
    corresponding Controls/Controller views. Map actions use the selected bindings
    rather than also firing from the old defaults. M/Escape open/close behavior,
    current pointer alternatives, focus ownership, local saves, and server-validated
    ping behavior remain intact.
    **Focused validation:** Add isolated settings/map automation that rebinds
    representative keyboard and controller controls for map zoom, pin operations,
    filtering, and ping; verify displayed bindings and action dispatch, and confirm
    the previous defaults no longer trigger those commands. Run after an Editor
    build with a disposable local user directory and retain map focus/privacy checks.
    No test was run during this review.

  - [ ] **RF128 — Correct placed-object scope metadata in the catalogue icon manifest (P3; CR14-003).**
    `Scripts/Data/catalogue-icon-manifest.csv:11-22`.
    **Affected path:** A maintainer uses the manifest to audit inventory icon
    coverage or determine how construction identities are exposed.
    **Trigger:** Twelve rows (`WorkbenchKit`, `ForgeKit`,
    `WorkbenchToolRackKit`, `ForgeAnvilKit`, `GrindingStoneKit`, `StorageKit`,
    `CookingRackKit`, `FryingPanKit`, `CauldronKit`, `FloorKit`, `WallKit`,
    `RoofKit`) have clean catalogue aliases registered under `items/buildItems/`,
    but classify their identity as `item`, their `inventory_scope` as `pack`,
    and list `PackInventory`, `ItemDetail`, and `Storage` among their views.
    The current architecture treats these as presentation-only buildables and
    says internal `Kit` identities represent placed objects only.
    **Impact:** The manifest misstates twelve placed-object identities as
    pack inventory. No current runtime behavior changes because import and
    validation scripts ignore these fields, but the stale metadata can mislead
    future audit or authoring and reintroduce portable-kit assumptions.
    **Evidence:** A read-only cross-reference joined all twelve aliases to
    registered `items/buildItems/*.json` definitions. `docs/02-technical-architecture.md:504-506`
    and `docs/05-decision-log.md:53` require buildables to remain separate from
    inventory. Current `Validate-UIIcons.ps1 -Category Catalogue` and
    `python Scripts/Validate-CatalogueReferences.py` pass but do not validate
    these manifest columns.
    **Proposed repair:** Update the twelve rows' identity and view metadata to
    reflect non-inventory buildables, retaining only the UI contexts that
    actually consume their canonical images.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** The twelve identities are marked as presentation-only
    buildables with no pack inventory scope, PackInventory, ItemDetail or
    Storage views; direct-material recipes and current texture IDs remain
    unchanged.
    **Focused validation:** Add a read-only manifest/catalogue cross-check for
    buildable aliases that rejects inventory scope or inventory views, then
    rerun both catalogue icon and catalogue-reference validators.
  - [ ] **RF129 — Repair references to the removed shared icon workflow (P3; CR14-003).**
    `AGENTS.md:135`; `docs/README.md:22`; `docs/icon-provenance.md:4`.
    **Affected path:** A contributor follows the project guidance to the shared
    icon authoring/import workflow before preparing or updating an icon.
    **Trigger:** These references point to `docs/material-icons.md`, which is
    absent from the tracked checkout and was deleted in the documentation
    consolidation commit `9a8cbc0f`. The README still says it owns the shared
    authoring/import steps. Parts of the workflow appear in development setup
    and the provenance file, but the advertised shared link has no target.
    **Impact:** The top-level icon workflow references are broken, making the
    supported preparation/import steps harder to find and leaving maintainers
    without the promised shared entry point.
    **Evidence:** `git ls-files` contains no `docs/material-icons.md`; `Test-Path`
    is false; `git show --name-status 9a8cbc0f` lists the deletion while the
    three current references remain. Tool requirements and validator commands
    are present in `docs/07-development-setup.md`, and per-item prompts remain
    in `docs/icon-provenance.md`.
    **Proposed repair:** Restore a concise shared workflow page or retarget all
    current references to extant setup/provenance sections that provide an
    authoring, preparation, import, and validation entry point.
    **Dependencies:** After CR19 review acceptance; no documentation repair
    during CR14.
    **Acceptance:** No active guidance points to an absent page, and the
    existing authoring/import/validation workflow remains discoverable.
    **Focused validation:** Search current guidance for `material-icons.md`,
    verify every reference resolves to a tracked file, and run a Markdown link
    check.

  - [ ] **RF130 — Remove Frost Salt's unsupported food-preservation claim (P3; CR13-013).**
    `Content/Data/GameCatalogues/items/materials/cooking-ingredients.json:4-7`;
    `Source/KalmalaUI/Private/KalmalaItemDetailWidget.cpp:13-16`;
    `Source/KalmalaGameplay/Private/KalmalaM9SourceLootContract.cpp:20`;
    `Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp:369-372`;
    `Content/Data/GameCatalogues/recipes/`; `Source/KalmalaGameplay/`.
    **Affected path:** A player inspects Frost Salt in inventory details and expects it to preserve food.
    **Trigger:** The current description says “Sprinkle some to preserve your food,” but the server supports Frost Salt acquisition from a Tundra harvest only; no registered recipe consumes it and no food-preservation, spoilage, or storage rule uses it.
    **Impact:** Player-facing copy promises a use that currently cannot occur, creating a false expectation for collecting or using the item.
    **Evidence:** `DescribeItem` exposes the catalogue description. The server source/reward contract and focused acceptance-test data establish the `tundra-frost-salt-deposit` acquisition. The registered recipe catalogue has zero FrostSalt references, and inspection/search of current gameplay code found no preservation/spoilage consumer or storage rule. This is not a missing acquisition finding.
    **Proposed repair:** Replace or qualify the unsupported preservation wording with an accurate material description, unless an explicitly approved and implemented preservation feature is supplied. Do not add gameplay behavior or invent a use during CR13.
    **Dependencies:** After CR19 review acceptance; any preservation mechanic requires separate approved gameplay scope. No catalogue or runtime edits during CR13.
    **Acceptance:** Frost Salt's player-facing description does not promise an unavailable preservation use, or the use is separately approved and implemented with its own validation; this repair does not change server authority or storage behavior.
    **Focused validation:** Compare revised text with the item-detail path, Tundra acquisition contract, registered recipes, and current food/storage implementation; run focused catalogue/detail checks when repaired and git diff --check. No Unreal build is required for the documentation/copy-only repair.

  - [ ] **RF131 — Mark binary PPM world-generation images as binary (P3; CR15-001).**
    `.gitattributes:1-12`; the five tracked `Content/Kalmala/Developer/WorldGeneration/*.ppm`
    files; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:646-650,673-677`.
    **Affected path:** Git checks out, diffs, or merges the tracked P6 PPM
    world-generation images.
    **Trigger:** Two branches update the same PPM image and Git uses its
    unspecified default merge attribute; the current attributes file marks the
    listed Unreal and common image/audio formats as binary but has no PPM rule.
    **Impact:** Git can apply text merge behavior to opaque pixel bytes, leaving
    conflict markers or a meaningless merged image. This affects developer
    visualization artifacts only; no runtime or gameplay effect was confirmed.
    **Evidence:** All five tracked PPM files have the P6 binary-image header and
    opaque pixel payloads. `git check-attr --all` returns no attributes for a
    PPM path, while `.uasset`, `.umap`, and `.png` samples receive the existing
    `binary` macro. Git documents that `binary` unsets diff, merge, and text,
    while an unspecified merge attribute uses the default text merge driver.
    **Proposed repair:** Add `*.ppm binary` to `.gitattributes`.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** Tracked and newly generated `.ppm` files report `binary`;
    checkout and merge operations never normalize their pixel bytes or attempt
    line-based merges.
    **Focused validation:** Run `git check-attr --all` against all five files
    and verify `binary: set` with `diff`, `merge`, and `text` unset. No PPM test
    or merge simulation was run during this review.
  - [ ] **RF132 — Include inline C++ headers in shared naming conventions (P3; CR15-001).**
    `.editorconfig:1`; `Source/KalmalaGameplay/Private/KalmalaA8bFangSpearPeerVerification.inl:1,25,91`;
    `Source/KalmalaGameplay/Private/KalmalaA8bWolfPeltBootsPeerVerification.cpp:500`.
    **Affected path:** A developer edits or runs Visual Studio's optional C++
    naming-convention linter on the tracked `.inl` helper.
    **Trigger:** The root EditorConfig section is `[*.{cpp,h}]`, and no nearer
    EditorConfig exists for the `.inl` file. Its declarations are C++ and the
    file is included from the related `.cpp` verification fixture.
    **Impact:** The project's Unreal naming settings are not supplied for this
    inline helper, so editor naming diagnostics can differ from the rest of
    the C++ source. The helper is non-Shipping verification code; no build or
    gameplay failure was confirmed. Visual Studio's naming check is opt-in.
    **Evidence:** The only C++-like tracked extension outside `.cpp` and `.h`
    under `Source/` and `Plugins/` is this `.inl`; repository search found no
    deeper `.editorconfig` covering it. Microsoft's C++ naming-linter guidance
    says the check reads naming conventions from `.editorconfig` and must be
    enabled in C/C++ Code Style options
    ([documentation](https://learn.microsoft.com/en-us/cpp/ide/lnt-naming-convention?view=msvc-170)).
    **Proposed repair:** Extend the root C++ glob to include `.inl`.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** The shared Unreal naming rules resolve for `.inl` as they do
    for `.cpp` and `.h` files, without changing test or gameplay behavior.
    **Focused validation:** With the Visual Studio naming-convention check
    enabled, open or analyze the `.inl` helper and confirm its EditorConfig
    naming settings are active. No linter run occurred during this review.
  - [ ] **RF133 — Align README dedicated-server build guidance (P3; CR16-001).**
    `README.md:22-24`, `docs/05-decision-log.md:163-164`, and
    `docs/07-development-setup.md:2337-2345`.
    Evidence and acceptance: [RF133 finding](Temporary/Review/findings.md#rf133--align-readme-dedicated-server-build-guidance-p3).
    **Affected path:** A developer follows the README build guidance while
    using the installed Epic Games Launcher UE 5.8.2 distribution.
    **Trigger:** The README names the Launcher installation and then instructs
    developers to build both editor and dedicated-server targets, while the
    current setup guide says that distribution lacks server support and says
    not to attempt the target build.
    **Impact:** The README directs developers toward an unsupported target and
    conflicts with the accepted engine limitation. This is documentation drift;
    no runtime defect was found.
    **Evidence:** The setup guide states the Launcher engine does not include
    dedicated-server support and requires a UE 5.8 source build or other
    server-capable distribution. The decision log records that dedicated-server
    builds are deferred while the Launcher engine is used.
    **Proposed repair:** Clarify that the installed engine supports the editor
    workflow and that dedicated-server builds require a server-capable UE 5.8
    installation, linking the setup guide's conditional instructions.
    **Dependencies:** After CR19 review acceptance; no repair during the
    review-only phase.
    **Acceptance:** README setup guidance agrees with the decision log and
    development setup: it does not direct Launcher-engine users to build the
    server target, and it preserves the future server-capable-engine path.
    **Focused validation:** Compare the revised README statement with
    `docs/05-decision-log.md` and the dedicated-server section in
    `docs/07-development-setup.md`. Run the documentation contract checker if
    it covers this statement; no Unreal build is needed for a docs-only fix.
  - [ ] **RF134 — Cook the string-loaded audio packages (P2; CR13-016).**
    `Config/DefaultGame.ini:8-13`; `Source/KalmalaUI/Private/KalmalaAmbientAudioSubsystem.cpp:33-54,198-1196`; `Source/KalmalaUI/Public/KalmalaAmbientAudioSubsystem.h`; `Content/Kalmala/Audio/`.
    **Affected path:** Players using a packaged build that expects local ambient loops and owner-local feedback cues.
    **Trigger:** The runtime loads audio packages with `LoadObject<USoundWave>` from string path constants. The SoundWave fields are transient runtime pointers; packaging explicitly cooks UI directories and the menu map but no `/Game/Kalmala/Audio` directory.
    **Impact:** Audio packages with no cook-visible asset reference may be omitted, leaving runtime loads unresolved and the local audio layer silent in packaged builds.
    **Evidence:** The eight CR13-016 packages match CR01/current main and their SoundWave paths and imported-source MD5s are valid. `DefaultGame.ini` includes `/Game/Kalmala/UI` cook paths but none for audio. The runtime loads use C++ string constants, and the setup guide states existing static/peer checks do not verify packaged playback. A packaged cook was not run during review.
    **Proposed repair:** Add `/Game/Kalmala/Audio` to the explicit cook set or provide equivalent cook-visible asset references. Do not change audio behavior or assets during CR13.
    **Dependencies:** After CR19 review acceptance; no packaging configuration or runtime edits during CR13.
    **Acceptance:** A packaged cook contains the runtime audio packages and packaged runtime checks resolve ambient beds and local feedback cues with the existing non-audio fallbacks intact.
    **Focused validation:** Inspect the cook manifest/package contents for `/Game/Kalmala/Audio`, run focused packaged audio coverage, and run git diff --check.
  - [ ] **RF135 — Reconcile duplicate support-effect audio imports (P3; CR13-020, CR13-021).**
    `Content/Kalmala/Audio/{SupportMendingCue,SupportHearthShieldCue,SupportBearsVigorCue,SupportHearthShieldExpiryCue,SupportBearsVigorExpiryCue,SupportDeerCallCue}.uasset`; corresponding files under `Content/Kalmala/Audio/Source/`; and the six matching source WAVs.
    **Affected path:** A maintainer updates or reimports one of the six effect-specific support sounds.
    **Trigger:** Each support sound exists as both a root SoundWave at `/Game/Kalmala/Audio/<name>` and another SoundWave at `/Game/Kalmala/Audio/Source/<name>`. Runtime uses only the root packages. Each root package's `RelativeFilename` is `../../../Audio/<name>.wav`, which resolves to a missing project-root `Audio` path; each `/Source` package instead records the matching `<name>.wav` source. Both import MD5 records match the checked-in `Source/<name>.wav`.
    **Impact:** The root runtime packages retain a stale reimport source location, and the duplicate `/Source` packages create unused asset-browser entries that can receive a reimport by mistake. The existing root packages and runtime playback paths remain valid.
    **Evidence:** All six root and `/Source` SoundWave packages and WAV sources match CR01 and current main. Each `/Source` package has the expected `/Game/Kalmala/Audio/Source/<name>` identity and an import MD5 matching its WAV; no other content package references these `/Source` identities. CR13-021 confirmed the four companion `/Source` packages and matching imports. The root package import records resolve to absent paths while retaining matching source MD5s. Runtime path constants point to the root identities.
    **Proposed repair:** Reimport the checked-in WAVs into the existing `/Game/Kalmala/Audio` packages to refresh their source records. Resolve whether the unused `/Game/Kalmala/Audio/Source` packages should be retained; do not remove assets without explicit user direction.
    **Dependencies:** After CR19 review acceptance. Any deletion or relocation of the extra packages requires explicit user direction; no audio asset edits during CR13.
    **Acceptance:** Runtime package identities remain unchanged, their recorded import paths resolve to the checked-in source WAVs with matching MD5s, and the `/Source` packages are either explicitly retained for a documented use or removed with authorization.
    **Focused validation:** Inspect all six root and `/Source` package paths, import filenames, hashes, and package references; run the audio validator and git diff --check. No Unreal build is needed for the metadata/source cleanup.
  - [ ] **RF136 — Refresh the stale biome-classification preview palette (P3; CR13-023).**
    `Content/Kalmala/Developer/WorldGeneration/BiomeClassification.ppm`; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:30-41`; `docs/biome-height-rework.md:282`.
    **Affected path:** A developer reviews the tracked biome-classification preview to understand current generated biome regions.
    **Trigger:** The PPM uses outdated colors for Mossy Mire, Freezing Tundra, and Thunder Mountains; the current commandlet uses a newer palette.
    **Impact:** The developer preview's hues no longer match the current commandlet palette, which can mislead visual inspection. No runtime or gameplay effect was found.
    **Evidence:** The PPM contains `(76,113,55)`, `(213,236,238)`, and `(104,98,112)` for those classes. The current commandlet uses `(113,79,55)`, `(153,153,151)`, and `(240,240,240)`, changed in commit `e441d0b8`; the tracked PPM history still points to the original commandlet-output commit `cb5175f1`. The architecture note says the image uses the commandlet palette.
    **Proposed repair:** Re-render `BiomeClassification.ppm` from the current commandlet using its original defaults (`Seed=10323456789`, `Size=256`, `Extent=20000` cm) and replace only the stale classification image. Do not change PPM assets during CR13.
    **Dependencies:** After CR19 review acceptance; no generated-content edits during CR13.
    **Acceptance:** Every represented biome in the tracked image uses the current `GetBiomeColor` palette, and the image is reproducible from the current commandlet at the original seed, size, and extent.
    **Focused validation:** After the editor build, render to a short temporary directory with `-run=RenderWorldGenerationVisualization -Seed=10323456789 -Size=256 -Extent=20000`, verify the seven RGB colors match the current palette, and run git diff --check. The broader regional-generation peer suite is not required for this artifact-only repair.
  - [ ] **RF140 — Resolve M1 fixture maps inside the active project (P3; CR14-005).**
    `Source/KalmalaEditor/Private/SetupM1TestMapCommandlet.cpp:20-21,55-64`;
    `Source/KalmalaEditor/Public/SetupM1TestMapCommandlet.h:6-14`;
    `Kalmala.uproject:28-30`; `Source/KalmalaEditor.Target.cs:11`.
    **Affected path:** A developer invokes `SetupM1TestMap` from an Editor
    project checkout other than `E:/dev/Kalmala`.
    **Trigger:** The reflected `UCommandlet` is reachable through normal
    `-run=SetupM1TestMap` dispatch, but `Main` loads the absolute
    `E:/dev/Kalmala/Content/Kalmala/Maps/Prototype/L_Prototype.umap` path and
    saves it when any tagged M1 fixture actor is absent. No guard ties the path
    to the active project.
    **Impact:** A commandlet launched for a worktree or another project can
    inspect or modify the main checkout's map instead of its active project's
    map, undermining worktree isolation and making the tool machine-path bound.
    **Evidence:** The class is a public `UCLASS` derived from `UCommandlet` in
    the registered Editor module. Its `Main` ignores the active project's
    content root and passes the hard-coded path to `LoadPackage` and
    `SavePackage`; no documented script invocation constrains that dispatch.
    **Proposed repair:** Resolve `/Game/Kalmala/Maps/Prototype/L_Prototype`
    from the active project and reject any resolved map path outside that
    project's canonical `Content` directory before loading it.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** An invocation from a disposable checkout reads and writes
    only that checkout's map; a missing or out-of-project map path fails before
    loading or saving, and the normal fixture setup remains idempotent.
    **Focused validation:** Run the commandlet against a short disposable
    project copy with one fixture absent; verify only its map changes and the
    main-checkout map hash remains unchanged. No commandlet was run during
    review.
  - [ ] **RF141 — Bound visualization extent before grid enumeration (P3; CR14-005).**
    `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:319-335,654-657`;
    `Source/KalmalaWorld/Public/KalmalaRegionalTuning.h:80`;
    `docs/01-game-design.md:123`; `docs/07-development-setup.md:3671,3677`.
    **Affected path:** A developer renders a world-generation visualization
    with a positive `-Extent` larger than the documented playable-world view.
    **Trigger:** The commandlet accepts any positive `WorldExtent`. Unless
    `-SkipSplineOverlay` is present, it enumerates every hydrology grid cell in
    the square from `-Extent/2` through `+Extent/2`; `GridCell` is 6,000 cm, so
    work grows quadratically with the supplied extent. A 32,000,000 cm extent
    yields about 28.5 million cell iterations, compared with about 285,000 at
    the documented 3,200,000 cm full-width extent.
    **Impact:** A valid-looking command line can leave the Editor commandlet
    doing an unbounded amount of hydrology work and make the process
    unresponsive. No larger `-Extent` use is documented; the 128 km atlas is a
    separate fixed image, while the playable world radius remains 16 km.
    **Evidence:** `Main` checks only `WorldExtent <= 0`; lines 654-657 derive
    both loop bounds directly from the value and call `GetHydrology` for every
    cell unless the optional skip flag is set. The development guide documents
    3,200,000 cm and describes the overlay as a costly whole-area enumeration;
    the game design fixes the world at a 16 km radius.
    **Proposed repair:** Require a finite extent no greater than twice
    `FKalmalaWorldBounds::Radius` (3,200,000 cm) before allocating images or
    creating output directories.
    **Dependencies:** After CR19 review acceptance; no repair during CR14.
    **Acceptance:** The documented extent and smaller previews remain valid;
    values above 3,200,000 cm and non-finite values reject before sampling or
    output creation, with the overlay both enabled and skipped.
    **Focused validation:** Add argument-boundary checks for zero, the
    documented maximum, one unit above it, and non-finite input; verify the
    rejection path creates no output. No visualization was run during review.
  - [ ] **RF150 — Include the gameplay map in packaged cooks (P1; CR15-002).**
    `Config/DefaultGame.ini:14`; `Config/DefaultEngine.ini:3-4`;
    `Source/KalmalaGameplay/Private/KalmalaMenuContext.cpp:179-180`;
    `Scripts/Package-And-Run.ps1:11,176-191,201-228`.
    **Affected path:** A packaged player starts Home and selects a local world
    for solo or listen-host play.
    **Trigger:** The normal cook uses the project map list, which names only
    `/Game/Kalmala/Maps/Menu/L_Menu`; the Home world-start path opens
    `/Game/Kalmala/Maps/Prototype/L_Prototype`. The packaging script's `$Map`
    value is passed only to the post-cook launch, not to `BuildCookRun`.
    **Impact:** The normal packaged Home flow targets a map that is not selected
    for cooking, so solo/host world start can fail before gameplay loads.
    **Evidence:** `DefaultEngine.ini` sets Home as `GameDefaultMap` and Prototype
    only as `EditorStartupMap`. `DefaultGame.ini` has a single `MapsToCook` entry
    for Home and no always-cooked `/Game/Kalmala/Maps` directory. The menu code
    calls `OpenLevel` with the Prototype package path. `BuildCookRun` in the
    project package script has no map-list argument; its Prototype `$Map` is
    added later to the packaged executable's launch arguments. Epic documents
    `MapsToCook` as the map list used when no other map list is specified on the
    command line ([Project Packaging Settings API](https://dev.epicgames.com/documentation/unreal-engine/API/Developer/DeveloperToolSettings/UProjectPackagingSettings));
    the package script's default client flow and the Home/game round-trip are
    also still named as SM15 validation requirements in `docs/startmenu.md:435-440`.
    The package was not built during this review.
    **Proposed repair:** Add `/Game/Kalmala/Maps/Prototype/L_Prototype` to the
    `MapsToCook` list in `DefaultGame.ini`.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** A standard packaged build contains both Home and Prototype;
    launching without a map override opens Home, and solo and listen-host world
    selection load Prototype successfully.
    **Focused validation:** Package with `Scripts/Package-And-Run.ps1 -Mode Dev`
    and verify both the script's Prototype smoke launch and a fresh no-map
    launch followed by Home solo/listen-host selection. No package or runtime
    validation was run during this review.
  - [ ] **RF151 — Repair VisualStudioTools troubleshooting links and heading (P3; CR15-006).**
    `Plugins/VisualStudioTools/Docs/Troubleshooting.md:43,45,51` and
    `Plugins/VisualStudioTools/README.md:17,60`; see RF151 in the review ledger.
    Update the two README fragments to the generated heading anchors and
    normalize the Task Center error heading punctuation. Implement after CR19
    review acceptance, then verify both destinations and the rendered heading.
  - [ ] **RF152 — Correct VisualStudioTools Marketplace guide copy and captions (P3; CR15-006/007).**
    `Plugins/VisualStudioTools/Docs/Marketplace_Readme.md:20,43,46,49`,
    `Docs/images/test_explorer.png`, and `Visual Studio Integration Tool
    Documentation.pdf:2-4`; see RF152 in the review ledger. Fix the Vault
    instruction, emphasis markers, the Test Explorer caption, and Figure 4
    numbering, then update the included PDF to match. Implement after CR19
    review acceptance, then inspect the rendered Markdown and PDF pages.
  - [ ] **RF153 — Anchor VisualStudioTools packaging paths to the script location (P3; CR15-008).**
    `Plugins/VisualStudioTools/Scripts/Package-Plugin.ps1:16-17,21,27,29,38-39`,
    `Plugins/VisualStudioTools/build.proj:1,14`, and
    `Scripts/SignDetached.proj:2`; see RF153 in the review ledger. Resolve the
    project, Config, and publish paths from the plugin/script location, or fail
    with a clear precondition. Implement after CR19; verify calls from the
    plugin root, Scripts folder, and an unrelated working directory.
  - [ ] **RF154 — Stop packaging after a failed Unreal plugin build (P3; CR15-008).**
    `Plugins/VisualStudioTools/Scripts/Package-Plugin.ps1:17,21-24,27,35-43`
    and `Plugins/VisualStudioTools/build.proj:14-19`; see RF154 in the review
    ledger. Check MSBuild's exit code and archive operations, clean temporary
    output on failures, and print success only for a verified package. Implement
    after CR19; verify native failure, missing output, archive failure, and
    successful packaging with controlled fixtures.
  - [ ] **RF155 — Remove unrelated direct module dependencies from the Blueprint debugger helper (P3; CR15-010).**
    `Plugins/VisualStudioTools/Source/VisualStudioBlueprintDebuggerHelper/VisualStudioBlueprintDebuggerHelper.Build.cs:10-28`;
    see RF155 in the review ledger. Its private sources directly use a small
    subset of the 18 declared dependencies; pare the list back to dependencies
    justified by the helper's own headers and API calls. Implement after CR19;
    validate the supported Win64 Editor plugin builds and preserve the helper's
    private module boundary.+
  - [ ] **RF156 — Align primary VisualStudioTools module dependencies with source (P3; CR15-012).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/VisualStudioTools.Build.cs:55-75`;
    see RF156 in the review ledger. The private source does not use the listed
    `ApplicationCore`, `ToolMenus`, or `MainFrame` modules, and it uses GraphEditor
    without declaring that dependency. Remove the unused entries and add the
    actual GraphEditor dependency. Implement after CR19 review acceptance;
    validate supported Win64 Editor builds with the module's custom no-unity and
    no-PCH settings.
  - [ ] **RF157 — Guard failed Blueprint sync-load handles before release (P2; CR15-013).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintAssetHelper.cpp:77-87`;
    see RF157 in the review ledger. The scope-exit cleanup dereferences the handle
    before the invalid-handle branch can skip the failed asset. Make cleanup safe
    for failed loads; verify missing/invalid generated-class paths are skipped.
  - [ ] **RF158 — Validate the optional Blueprint-reference symbol (P2; CR15-013).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintReferencesCommandlet.cpp:191-211`;
    see RF158 in the review ledger. Omission of the help-documented optional
    `-symbol` leaves `ParamVals.Find` null and crashes before its required-argument
    message. Validate null and empty values and align the help/usage contract.
  - [ ] **RF159 — Preserve Blueprint references with duplicate generated-class names (P2; CR15-013).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintReferencesCommandlet.cpp:97-116,141-153`;
    see RF159 in the review ledger. Keying matches by generated class short name
    lets same-named assets in different packages overwrite one another. Preserve
    results by object path or an array; validate with two matching same-named
    Blueprints in different folders.
  - [ ] **RF207 — Keep empty native-class filters from scanning every Blueprint (P2; CR15-013).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VisualStudioToolsCommandlet.cpp:354-413,459-487`;
    see RF207 in the review ledger. When path lookup finds no native classes, the
    asset filter retains only the Blueprint class constraint and scans all
    Blueprints. Return an empty result or a clear filter error; verify a path with
    no native modules does not trigger a project-wide scan.
  - [ ] **RF208 — Make COM moniker enumeration cleanup safe and complete (P2; CR15-013).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VisualStudioToolsBlueprintBreakpointExtension.cpp:119-185`;
    see RF208 in the review ledger. Initialize and conditionally release the bind
    context, free each successful display-name allocation, and release the ROT if
    enumeration fails. Validate success and each COM failure path without leaks
    or invalid releases.
+  - [ ] **RF160 — Align current world-generation revision and distance guidance (P3; CR16-002).**
    `docs/04-roadmap.md:49`, `docs/05-decision-log.md:158,172`,
    `docs/07-development-setup.md:3671`, `docs/01-game-design.md:123`,
    `docs/02-technical-architecture.md:793,830,845`, the preview commandlet
    argument parser, and `KalmalaRegionalTuning.h:91-92`. The roadmap promises
    compatibility for revisions 1–6 and calls revision 8 a debug-stream
    identity; the decision row preserves older layouts/schemas and gives Mire
    a 2 km minimum; setup passes `-Revision=7`. The current architecture has one
    generator layout and a 3 km Mire minimum; the commandlet does not parse a
    Revision argument. Reconcile these contracts, label superseded historical
    decisions, and distinguish the save-identity revision from a generator
    selector. Evidence and acceptance: [RF160 finding](Temporary/Review/findings.md#rf160--align-current-world-generation-revision-and-distance-guidance-p3).
  - [ ] **RF190 — Return the test adapter's result over the named pipe (P2; CR15-014).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSServerCommandlet.cpp:41-89`
    and `VSTestAdapterCommandlet.cpp:88-167,277-287`; see RF190. The server
    discards the adapter's nonzero result and sends its default success response.
    Propagate failures and test successful and failing requests.
  - [ ] **RF191 — Close the Visual Studio named-pipe handle after each request (P3; CR15-014).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSServerCommandlet.cpp:41-91`;
    see RF191. Each successful `CreateFile` handle is abandoned when request
    processing returns. Close it on every path and verify repeated requests do
    not grow the server's open-handle count.
  - [ ] **RF192 — Fail test runs when the requested test list cannot be read (P2; CR15-014).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.cpp:25-56,88-107,167`;
    see RF192. A missing list file leaves no tests, but the empty loop retains
    `AllSuccessful=true` and returns zero. Propagate the read failure and verify
    it cannot produce a successful empty run.
  - [ ] **RF193 — Align Test Adapter filter help with the applied default (P2; CR15-014).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.cpp:184-276`;
    see RF193. The help-listed default and the actual default mask select
    different categories, so Stress/Negative tests are omitted from the default
    selection. Make no-argument and explicitly documented defaults equivalent.
  - [ ] **RF194 — Handle `-help` as a commandlet switch (P3; CR15-014).**
    `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.cpp:202-214,277-287`;
    see RF194. Unreal puts bare `-help` in `Switches`, but the adapter only checks
    `ParamVals`, so help does not take its early success exit. Check the switch
    collection and verify help exits without running a requested operation.
  - [ ] **RF209 — Make the VisualStudioTools log category's public visibility consistent (P3; CR15-015).**
    Plugins/VisualStudioTools/Source/VisualStudioTools/Public/VisualStudioTools.h:5-7
    and Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VisualStudioTools.cpp:4,9.
    The Public header declares the category without VISUALSTUDIOTOOLS_API while
    the module defines it privately.
    Keep it private if no external C++ contract is intended, or export it if
    dependent modules are expected to use it; validate with a modular Win64
    Editor consumer.
  - [ ] **RF161 — Reconcile M13/M14 acceptance summaries (P3; CR16-002, extended CR16-003).**
    `docs/01-game-design.md:174`, `docs/02-technical-architecture.md:773`,
    `docs/game-features.md:139-143`, `docs/README.md:16`, and
    `BACKLOG.md:1083-1084`. The design says M14's isolated acceptance did not
    integrate into main; the architecture and game-features summaries say
    integrated acceptance is open; and the README leaves M14 acceptance scope
    as remaining in the backlog. The active backlog records both integrated-main
    gates as passed. Update the summaries or identify them as dated historical
    states. Evidence
    and acceptance: [RF161 finding](Temporary/Review/findings.md#rf161--reconcile-m13m14-acceptance-summaries-p3).  - [ ] **RF162 — Remove the superseded wetness-to-warmth effect (P3; CR16-002).**
    `docs/01-game-design.md:130,155` and
    `docs/02-technical-architecture.md:668-672`. The earlier world-simulation
    table says soaked wetness makes players lose warmth; the later accepted Wet
    override replaces warmth-derived Wet penalties and continuous player
    wetness/warmth values. Update or label the stale row. Evidence and
    acceptance: [RF162 finding](Temporary/Review/findings.md#rf162--remove-the-superseded-wetness-to-warmth-effect-p3).
  - [ ] **RF163 — Clarify the A9/I5 status in its acceptance handoff (P3; CR16-002).**
    `docs/a9-acceptance.md:3,9,36-44` and
    `PROGRESS.md:9584-9609,19121-19129`. The opening says A9/I5 is complete,
    while the next paragraph and later history say I4/I5 remain open; later A10
    progress also describes broader A9/I5 acceptance as open. Label the dated
    snapshots and distinguish the final I5 result from subsequent audit limits
    or reopened gates. Evidence and acceptance: [RF163 finding](Temporary/Review/findings.md#rf163--clarify-the-a9i5-status-in-its-acceptance-handoff-p3).
  - **Dependency:** Blocked until CR19 review acceptance is complete. Review
    completion queues work here and never starts a repair in the same run.
  - [ ] **RF164 — Reconcile the start-menu activation summary (P3; CR16-003).**
    `docs/startmenu.md:15-17,435-463`, `Config/DefaultEngine.ini:4`, and
    `BACKLOG.md:977-978,1054-1057` disagree about whether packaged startup is
    routed to Home. Update the current summary, identify earlier implementation
    notes as historical, and distinguish checked SM14.5 routing from pending
    SM15 acceptance. Evidence and acceptance:
    [RF164 finding](Temporary/Review/findings.md#rf164--reconcile-the-start-menu-activation-summary-p3).
  - [ ] **RF165 — Restore or retarget the shared item-icon workflow link (P3; CR16-003).**
    `AGENTS.md:135`, `docs/README.md:22-25`, and
    `docs/icon-provenance.md:4-8` link to missing `docs/material-icons.md`.
    Restore a maintained guide or retarget the references to an existing guide
    that covers the current item-icon pipeline. Evidence and acceptance:
    [RF165 finding](Temporary/Review/findings.md#rf165--restore-or-retarget-the-shared-item-icon-workflow-link-p3).
  - [ ] **RF166 — Update creature-catalogue implementation status and schema wording (P3; CR16-003).**
    `docs/creature-catalogues.md:21-30` calls the item/tool loader schema-15
    and places behavior covered by completed Monster S01–S35 verification in
    later engine work. The manifest and loader use schema 16; creature files
    remain authoring-only. Align the summary with verified implementation while
    preserving the data-loading boundary. Evidence and acceptance:
    [RF166 finding](Temporary/Review/findings.md#rf166--update-creature-catalogue-implementation-status-and-schema-wording-p3).
  - [ ] **RF167 — Repair missing M13/M14 evidence links (P3; CR16-003).**
    `docs/README.md:16` and `BACKLOG.md:1083-1084` link to absent M13/M14
    evidence files while claiming passed integrated-main acceptance. Restore
    verifiable records or retarget the links and qualify unsupported retention
    claims. Evidence and acceptance:
    [RF167 finding](Temporary/Review/findings.md#rf167--repair-missing-m13m14-evidence-links-p3).
  - [ ] **RF168 — Use a registered food automation path in acceptance (P2; CR16-005).**
    `Scripts/Tests/Acceptance/AutomatedAcceptance.ps1:18-31,74-76` requires a
    successful `Kalmala.Gameplay.Food.CampfireProcessing` result, but the current
    registered food test is `Kalmala.Gameplay.Food.CookingStationHeat` in
    `Source/KalmalaGameplay/Private/Tests/KalmalaCookingHeatContractTest.cpp:18-20`.
    Replace the stale path with the intended registered test and keep the list
    check aligned with source registrations. Evidence and acceptance:
    [RF168 finding](Temporary/Review/findings.md#rf168-use-a-registered-food-automation-path-in-acceptance-p2).
  - [ ] **RF169 — Apply the selected Editor path to every acceptance scenario (P3; CR16-005).**
    `Scripts/Tests/Acceptance/AutomatedAcceptance.ps1:4,85-116` accepts `-Editor`
    but uses it only for its automation process and `EnemyPeers.ps1`; other
    child scenarios either hardcode the default UE path or expose an `Editor`
    parameter that is not passed. Propagate the selected executable through all
    child scenarios or reject unsupported overrides before starting. Evidence
    and acceptance: [RF169 finding](Temporary/Review/findings.md#rf169-apply-the-selected-editor-path-to-every-acceptance-scenario-p3).
  - [ ] **RF180 — Rebuild the owner inventory grid after carried-tool restore (P3; CR17-002).**
    `KalmalaCharacter.cpp:423-447`, `KalmalaToolLifecycleContract.cpp:187-207`,
    `KalmalaGameMode.cpp:9427-9512,10843-10931`,
    `KalmalaInventoryGrid.cpp:11-27,132-177,349-361`, and
    `KalmalaInventoryComponent.cpp:410-415`. On PostLogin, Character BeginPlay
    first creates fresh carried-tool GUIDs and synchronizes the owner grid. The
    authenticated player save then replaces `CarriedTools` with saved tool
    records but leaves the grid's old GUIDs and active-instance reference in
    place. Slot resolution rejects those stale GUIDs until a later grid sync.
    Repair by rebuilding the authoritative grid after tool restoration and
    clearing any stale active instance before replication. Acceptance: an
    authenticated PostLogin restore immediately maps every saved tool to a
    unique resolvable slot and leaves Main Hand either bound to a current tool
    GUID or cleared; the owner can select/use the restored tools before another
    inventory transaction, and observers receive no private tool state.
    Focused validation: add a PostLogin save-restore regression with differing
    saved tool IDs/GUIDs and inspect owner slot mapping before any hotbar or
    stack transaction; run it with the existing inventory owner-privacy check
    after an Editor build. Dependency: CR19 review acceptance. Evidence:
    `Temporary/Review/findings.md#rf180--rebuild-owner-inventory-grid-after-carried-tool-restore-p3`.
  - **Dependency:** Blocked until CR19 review acceptance is complete. Review
    completion queues work here and never starts a repair in the same run.
  - [ ] **RF170 — Require an exact world seed in peer acceptance (P2; CR16-007, extended CR16-008, CR16-009, CR16-011, CR16-012, CR16-013, CR16-014, CR16-016, and CR16-017).**
    `EnemyPeers.ps1` double-escapes its digit boundary, and the reviewed
    Construction and Inventory peer scripts use substring checks for
    `Seed=418`; InteractionPrompt, PlayerControls, and SettingsAccessibility
    also accept it as a substring. World `MapAwareness.ps1` also accepts
    `Seed=418` as a substring without another exact-seed marker; `WorldProfile.ps1`
    has the same substring-only client check. The progression DiscoveryPeer,
    ExplorationRewards, and Schema2CandidateReconnect gates do too, as do
    the Survival ExposureRecoveryPeers and PersistedCampRestart gates. All five
    Travel peer scripts use the same substring check. These checks can accept a
    different numeric seed such as `4180` (or `418` when `41` is requested).
    Require a complete numeric seed token in all reviewed
    peer runners. Evidence and acceptance:
    [RF170 finding](Temporary/Review/findings.md#rf170-require-an-exact-world-seed-in-peer-acceptance-p2).
  - [ ] **RF171 — Enable reduced motion in the Crafting acceptance runner (P3; CR16-008).**
    `docs/07-development-setup.md:4468-4469` passes `-ReducedMotion` to
    `Scripts/Tests/Construction/Crafting.ps1`, but the script neither declares
    that option nor adds `-KalmalaUIDeveloperReducedMotion=` to its host/client
    launches. Add and verify a supported reduced-motion option for both peers.
    Evidence and acceptance:
    [RF171 finding](Temporary/Review/findings.md#rf171-enable-reduced-motion-in-the-crafting-acceptance-runner-p3).
  - [ ] **RF172 — Align inventory notification baseline expectations (P2; CR16-009).**
    `Inventory.ps1` and `InventoryReconnect.ps1` require `Sources=5`, but the
    current notification fixture emits `Sources=4` for both baseline markers.
    Align the runner predicates with the intended fixture contract while
    retaining the silent, zero-row assertions. Evidence and acceptance:
    [RF172 finding](Temporary/Review/findings.md#rf172-align-inventory-notification-baseline-expectations-p2).
  - [ ] **RF173 — Bound constrained-performance evidence paths (P3; CR16-010).**
    `ConstrainedPerformance.ps1` accepts any fresh `-OutputDirectory`, then
    passes a nested `Peers` path to a runner that appends `Host`/`Client` shader
    directories without checking MAX_PATH. Reject roots whose derived outputs
    reach 260 characters before creating them. Evidence and acceptance:
    [RF173 finding](Temporary/Review/findings.md#rf173-bound-constrained-performance-evidence-paths-p3).
  - [ ] **RF174 — Reconcile map pin-entry help with the M12 input-label rule (P3; CR16-011).**
    MenuInputCopy.ps1 lines 31-32 removes the visible “Enter saves · Escape cancels”
    prompt from its copy audit, while KalmalaWorldMapWidget.cpp lines 1312-1313
    draws it during pin-label entry. The architecture documents this map
    exception, but the M12 roadmap and setup contract say key/button names are
    limited to Options. Resolve the scope and make the source, contracts, and
    audit agree. Evidence and acceptance:
    [RF174 finding](Temporary/Review/findings.md#rf174-reconcile-map-pin-entry-help-with-the-m12-input-label-rule-p3).
  - [ ] **RF175 — Match complete input-key values in the baseline check (P2; CR16-011).**
    LocalInputContract.ps1 lines 62-74 tests Key=<expected> without a token
    boundary, so the required WorldMap M can be satisfied by MouseX. Compare
    complete active mapping keys. Evidence and acceptance:
    [RF175 finding](Temporary/Review/findings.md#rf175-match-complete-input-key-values-in-the-baseline-check-p2).
  - [ ] **RF176 — Report the SettingsAccessibility capture count accurately (P3; CR16-011).**
    SettingsAccessibility.ps1 lines 202-209 requires sixteen host/client images,
    but its final PASS message says fourteen. Derive the reported count from the
    checked capture set or update it to sixteen. Evidence and acceptance:
    [RF176 finding](Temporary/Review/findings.md#rf176-report-the-settingsaccessibility-capture-count-accurately-p3).
  - [ ] **RF181 — Bound generated test evidence paths (P3; CR16-012, extended CR16-013, CR16-014, CR16-015, CR16-016, and CR16-017).**
    Progression, Survival, Travel, BiomeExpansion, and World runners build
    evidence roots from `%TEMP%`; some accept custom output roots. They append
    logs, shader/data directories, screenshots, map exports, and CSVs without
    checking derived paths. Preflight full paths and reject any at or above
    260 characters before creating output. Evidence and acceptance:
    [RF181 finding](Temporary/Review/findings.md#rf181-bound-generated-test-evidence-paths-p3).
  - [ ] **RF182 — Correct stale Prototype game-default guidance (P3; CR13-025).**
    `docs/07-development-setup.md:2124`; `docs/startmenu.md:435-438`;
    `Config/DefaultEngine.ini:3-4`.
    **Affected path:** A developer follows the setup guide to determine which map opens by default in the editor and packaged game.
    **Trigger:** The setup guide says `L_Prototype` is both the editor startup and game default, while `DefaultEngine.ini` assigns `L_Prototype` only to `EditorStartupMap` and assigns `L_Menu` to `GameDefaultMap`; the start-menu guide documents the same packaged Home default.
    **Impact:** Developers may expect the prototype world to launch first in a packaged build and validate the wrong startup flow.
    **Evidence:** Current configuration and the start-menu contract agree that packaged startup uses `L_Menu`, but the setup guide still calls Prototype the game default. This is separate from the map's valid M1 fixture/environment content and from RF150's still-unverified packaged travel acceptance.
    **Proposed repair:** Update the setup guide to identify Prototype as the editor startup only and point to the start-menu guide for packaged startup. Do not change runtime or map configuration.
    **Dependencies:** After CR19 review acceptance; no game-document edits during CR13.
    **Acceptance:** Setup guidance matches `DefaultEngine.ini` and the start-menu contract; no runtime or configuration behavior changes.
    **Focused validation:** Compare both documentation passages with the configured map values and run git diff --check.
  - [ ] **RF184 — Restore project-local source paths for imported UI textures (P3; CR13-026/027/028/029/030/031/032/033).**
    `Content/Kalmala/UI/{OptionsPanel,StoneButton,StoneSliderThumb}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{Amber,Amethyst,AmethystNecklace,Antidote,Aquamarine,AquamarineRing,BadgerFur,BadgerFurVest}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{BadgerMeat,BasicTorch,BearsVigorScroll,BerryRecoveryDraught,BoarHide,BoarHideBoots,BoarHideCape,BoarHideHelmet}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{BoarHideLeggings,BoarHideTunic,BoarMeat,BoneFragment,Bow,BowString,Bronze,BronzeArrow}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{BronzeAxe,BronzeHelmet,BronzeKnife,BronzeLeggings,BronzeMace,BronzePick,BronzeShield,BronzeSpear}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{BronzeSword,BronzeTunic,CampBedKit,CampfireKit,Carrot,CarrotSeed,CauldronKit,CharcoalKilnKit}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{Coal,ConstructionHammer,CookedBoarMeat,CookedDeerMeat,CookingRackKit,Copper,CopperOre,DeerCallScroll}.uasset`;
    `Content/Kalmala/UI/Icons/Items/{DeerHide,DeerMeat,DeerRootRoast,Densewood,DuskDiamond,DuskRing,FangArrows,FangSpear}.uasset`;
    `Content/Kalmala/UI/Source/{OptionsPanel,StoneButton,StoneSliderThumb}.png`;
    `Content/Kalmala/UI/Source/Icons/{Amber,Amethyst,AmethystNecklace,Antidote,Aquamarine,AquamarineRing,BadgerFur,BadgerFurVest}.png`;
    `Content/Kalmala/UI/Source/Icons/{BadgerMeat,BasicTorch,BearsVigorScroll,BerryRecoveryDraught,BoarHide,BoarHideBoots,BoarHideCape,BoarHideHelmet}.png`;
    `Content/Kalmala/UI/Source/Icons/{BoarHideLeggings,BoarHideTunic,BoarMeat,BoneFragment,Bow,BowString,Bronze,BronzeArrow}.png`;
    `Content/Kalmala/UI/Source/Icons/{BronzeAxe,BronzeHelmet,BronzeKnife,BronzeLeggings,BronzeMace,BronzePick,BronzeShield,BronzeSpear}.png`;
    `Content/Kalmala/UI/Source/Icons/{BronzeSword,BronzeTunic,CampBedKit,CampfireKit,Carrot,CarrotSeed,CauldronKit,CharcoalKilnKit}.png`;
    `Content/Kalmala/UI/Source/Icons/{Coal,ConstructionHammer,CookedBoarMeat,CookedDeerMeat,CookingRackKit,Copper,CopperOre,DeerCallScroll}.png`;
    `Content/Kalmala/UI/Source/Icons/{DeerHide,DeerMeat,DeerRootRoast,Densewood,DuskDiamond,DuskRing,FangArrows,FangSpear}.png`;
    `docs/07-development-setup.md:1629-1703`.
    **Affected path:** A maintainer reimports a shared UI texture while editing the project in another checkout or clean mirror.
    **Trigger:** `OptionsPanel.uasset` records `../../../Import/OptionsPanel.png`, which is missing in both this worktree and current main, while `Source/OptionsPanel.png` exists. Twenty-eight packages record absolute paths under the main checkout, including BronzeSword, BronzeTunic, CampBedKit, CharcoalKilnKit, Copper and CopperOre. Six item icons point into a removed `.wt/a9` checkout; BadgerMeat, Antidote, BadgerFur, BadgerFurVest, DuskDiamond and DuskRing point into that removed checkout. Six item icons point into a removed `.wt/a8b` checkout; FangArrows and FangSpear join BadgerMeat, Antidote, BadgerFur and BadgerFurVest. BearsVigorScroll and DeerCallScroll point into a removed `a10-acceptance` worktree; BoarHide, BoarMeat, BronzeAxe, CampfireKit, Carrot, CarrotSeed, CauldronKit, Coal, ConstructionHammer, CookedBoarMeat, CookedDeerMeat, CookingRackKit, DeerHide, DeerMeat, DeerRootRoast and Densewood point into a removed `wt/m12-hud-feedback-rebuild` worktree.
    **Impact:** The textures render from their packages, but source reimport is tied to a missing path or another checkout, weakening reproducibility and risking edits from the wrong checkout.
    **Evidence:** All fifty-nine embedded source MD5 values match their checked-in project-local PNGs. Current-main and CR01 package hashes match. The setup guide identifies the canonical project-local UI source inputs; focused catalogue icon validators confirm the fifty-six affected catalogue icon originals, prepared PNGs, mappings and packages are present.
    **Proposed repair:** Reimport all fifty-nine textures from their existing project-local source PNGs so each package records a source path that resolves within the project. Preserve package identities, source pixels and existing texture settings; do not alter assets during CR13.
    **Dependencies:** After CR19 review acceptance; no UI asset edits during CR13.
    **Acceptance:** All fifty-nine affected package records resolve to their project-local source PNGs with matching MD5 values; package object paths, pixels and settings remain unchanged, and reimport works from a disposable short-path mirror.
    **Focused validation:** Inspect recorded paths and source MD5s after reimport; run the relevant UI icon validators and git diff --check. Full Unreal/menu verification is not needed for the metadata-only repair.
  - [ ] **RF183 — Require cross-world restore completion before accepting rejection (P2; CR16-013).**
    PersistedCampRestart.ps1:95-99 sleeps for twelve seconds, stops the seed
    419 server if it is still running, and accepts the negative case whenever
    its log lacks Construction restored:. Require server readiness and the
    fixture's explicit completed zero-construction/zero-storage result before
    accepting the mismatch, and fail on timeout or peer errors. Evidence and
    acceptance: [RF183 finding](Temporary/Review/findings.md#rf183-require-cross-world-restore-completion-before-accepting-rejection-p2).
  - [ ] **RF195 — Keep successful peer checks from timing out at the deadline (P3; CR16-014, extended CR16-015, CR16-016, and CR16-017).**
    `OceanTravel.ps1`, `Swimming.ps1`, three `BiomeExpansion.ps1` loops, and
    the host/client completion loops in `MapAwareness.ps1`, `Minimap.ps1`, and
    `WorldMap.ps1`, and the startup/profile loops in `WorldProfile.ps1` can
    throw based on the wall clock after observing success.
    Track observed completion and time out only when a required marker is
    still missing. Evidence and acceptance:
    [RF195 finding](Temporary/Review/findings.md#rf195-keep-successful-peer-checks-from-timing-out-at-the-deadline-p3).
  - [ ] **RF205 — Coalesce timer-driven construction save writes (P2; CR03-001b-07).**
    `Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp:40-44,143-180`,
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:819-918`,
    `Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:160-173`,
    `Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:362`,
    and `docs/02-technical-architecture.md:666`.
    **Affected path:** Server timer updates for active Charcoal Kilns, Smelters,
    Garden Plots, and Standing Lights in the shared construction-save slot.
    **Trigger:** Each active actor advances its timer at a one-second tick
    interval and calls its GameMode persistence method. Each call clones and
    validates the complete construction save and synchronously writes the same
    world slot; up to 128 saved records are permitted.
    **Impact:** With multiple active timers, the server repeats full-world
    validation/serialization and slot writes once per actor per second. At the
    record cap this can produce up to 128 synchronous full-slot writes per
    second and add synchronous work to the server game thread. The player-visible
    hitch size has not been measured.
    **Evidence:** See the [RF205 review entry](PROGRESS.md).
    ConstructionActor sets `TickInterval` to 1 second. Active kiln,
    smelter, growing-plot, and fueled-light branches advance state then call
    separate `Persist*State` methods. Each method builds a separate candidate,
    revalidates it, and calls `SaveGameToSlot`; `MatchesWorld` walks every
    record and `MaxRecords` is 128. All methods target the same world slot.
    **Proposed repair:** Batch timer-only state changes into one server-owned
    construction-save candidate and at most one slot write per world per
    one-second service interval. Leave planting, fueling, collection and harvest
    on their existing transaction paths, including their payment, compensation,
    and rollback ordering; preserve the no-offline-progress rule.
    **Dependencies:** After CR19 review acceptance; no repair during review.
    **Acceptance:** With timer updates active across the 128-record bound,
    timer-only updates perform at most one full slot write per world per service
    interval and persist every due record before actor replication. Explicit
    planting, fueling, collection and harvest transactions retain their current
    payment, save, apply, compensation and rollback ordering outside the timer
    batch. A failed timer batch publishes none of its actor-state updates. No
    offline time is applied.
    **Focused validation:** Add a controllable writer/candidate test that counts
    writes for multiple active timer actors, verifies all updated records in the
    successful save, and confirms a rejected write leaves their live states
    unchanged. Profile server tick and save duration at the record cap after
    an Editor build; no runtime performance measurement was run during review.
  - **Dependency:** Blocked until CR19 review acceptance is complete. Review
    completion queues work here and never starts a repair in the same run.
  - [ ] **RF206 — Verify requested metadata in rendered biome baseline captures (P2; CR16-015).**
    `BiomeBaselineCapture.ps1` checks image shape/diversity and only a subset
    of the runtime metadata. Compare logged lighting variant, requested pose,
    temporal and GPU frame settings, and comparison flag with each request
    before accepting the image. Evidence and acceptance:
    [RF206 finding](Temporary/Review/findings.md#rf206-verify-requested-metadata-in-rendered-biome-baseline-captures-p2).
  - [ ] **RF210 — Match numeric biome profile outputs to their input identities (P2; CR16-015, extended CR16-016).**
    Hill, Floor, Mire, Rolling, and Wood profile runners do not all match each
    output `Seed`, `Site`, `Xcm`, and `Ycm` to its corresponding fixed input
    row. Compare every output identity tuple before summarizing results and
    verify the fixed groups and coordinates. Evidence and acceptance:
    [RF210 finding](Temporary/Review/findings.md#rf210-match-numeric-biome-profile-outputs-to-their-input-identities-p2).
  - [ ] **RF211 — Align current chest-storage schema references (P3; CR03-001b-08).**
    `docs/02-technical-architecture.md:545,1108,1416,1644`,
    `Source/KalmalaGameplay/Public/KalmalaStorageSaveGame.h:23`, and
    `Source/KalmalaGameplay/Private/KalmalaStorageSaveGame.cpp:50-59`.
    **Affected path:** Contributors using the architecture contract to inspect
    current chest-save compatibility, record identity, and verification.
    **Trigger:** The M2, compatibility, and current-save sections call storage
    schema 2 current, while the current code requires schema 3 and a later
    architecture section also identifies schema 3.
    **Impact:** The source-of-truth document gives conflicting save versions and
    may lead reviews or fixtures to assume schema-2 data is accepted. Runtime
    code currently rejects it; no gameplay defect was established.
    **Evidence:** `UKalmalaStorageSaveGame::CurrentSchemaVersion` is 3 and
    `MatchesWorld` requires that version. Architecture lines 545, 1108, and 1416
    still state schema 2, while line 1644 records schema 3 and rejection of
    older development schemas.
    **Proposed repair:** Align present-tense storage schema references and the
    version table to schema 3. Label schema-2 descriptions as historical and
    state that pre-release incompatible saves are rejected without rewriting.
    **Dependencies:** After CR19 review acceptance; no documentation repair
    during CR03 or CR18.
    **Acceptance:** Every current storage schema statement and version-table
    entry agrees with the implementation. Any retained schema-2 description is
    clearly historical and says it is unsupported; no migration is introduced.
    **Focused validation:** Search the architecture document for storage schema
    references and compare each present-tense claim with the schema constant and
    `MatchesWorld` guard. No runtime or save-compatibility test ran during review.
  - [ ] **RF212 — Reject combined combat peer fixture selectors (P3; CR03-001b-12).**
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1241-1264,1307-1310`.
    **Affected path:** Non-Shipping combat peer fixtures launched with more
    than one `Kalmala*PeerTest` command-line flag.
    **Trigger:** A caller combines otherwise recognized scenarios, such as
    `KalmalaBoarPeerTest` and `KalmalaDeerPeerTest`.
    **Impact:** The admission gate accepts some ambiguous combinations. The
    fixture can report/select Boar by its priority chain while selecting the
    Deer-specific StoneKnife and enabling both scenario branches, so a manual
    peer run may exercise or report a composite scenario rather than the
    requested isolated case. The supported `EnemyPeers.ps1` runner supplies a
    single flag; no production gameplay impact was found.
    **Evidence:** The gate requires any recognized flag and rejects only
    selected conflicts, not every multi-flag combination. Boar precedes Deer
    in `VerificationName` and `ExpectedArchetype`, while `ServerWeaponId`
    chooses StoneKnife whenever the Deer flag is present.
    **Proposed repair:** Require exactly one recognized scenario flag before
    stage setup, or define and test an explicit supported combination matrix.
    Emit a clear failure for unsupported combinations.
    **Dependencies:** After CR19 review acceptance; no fixture repair during
    CR03 or CR18.
    **Acceptance:** Every supported single-flag scenario remains admitted;
    every unsupported multi-flag combination is rejected before participant
    setup with an actionable diagnostic. Existing runner scenarios retain
    their current behavior.
    **Focused validation:** Add a narrow admission check for each supported
    flag and representative multi-flag combinations, then run the affected
    `Scripts/Tests/Combat/EnemyPeers.ps1` scenarios after an Editor build.
    No runtime test ran during review.
  - [ ] **RF213 — Search for a target in the CombatFeedback fixture (P3; CR03-001b-13).**
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1264,1336-1339,1374-1385`,
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:940-978`, and
    `Scripts/Tests/Combat/EnemyPeers.ps1:11,37-39`.
    **Affected path:** The non-Shipping `CombatFeedback` peer scenario.
    **Trigger:** The runner selects `KalmalaCombatPeerTest`, which falls back to
    Mireling as `ExpectedArchetype`, but the bounded search gate omits
    `bCombatPeerTest`. If the host's starting spatial key has no Mireling
    descriptor, the fixture skips nearby-key search and immediately reaches the
    missing-descriptor terminal failure. This can occur for a Meadow-only key;
    Meadows descriptors are Boar, Deer, or Rat, while Mireling is reserved for
    Mossy Mire. The runner also permits a `-WorldSeed` override.
    **Impact:** The fixture can fail before producing the hit/defeat cues that
    `CombatFeedback` is intended to verify. No production gameplay impact was
    found.
    **Evidence:** `ExpectedArchetype` defaults to Mireling for this flag. The
    search condition includes the separate Mireling flag and other species or
    reward flags, but not `bCombatPeerTest`; `ExpectedDescriptor == nullptr`
    then sets stage 99. The biome selector only creates Mirelings in Mossy Mire.
    **Proposed repair:** Give `CombatFeedback` a consistent target-selection
    path: either search a bounded set of keys for the required Mireling or select
    a target archetype that the scenario can find in its configured area. Keep
    the fixture on ordinary generated descriptors.
    **Dependencies:** After CR19 review acceptance; no fixture repair during
    CR03 or CR18.
    **Acceptance:** The default `CombatFeedback` seed and a representative
    alternate seed with no Mireling in the start key reach the intended hit,
    defeat, unavailable-text, and remote-privacy checks through a bounded,
    deterministic target search. No test-only wildlife actor is introduced.
    **Focused validation:** Add narrow coverage for `CombatFeedback` target
    selection when its start key lacks a Mireling, then run
    `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario CombatFeedback` after an
    Editor build with the default and alternate seed. No runtime test ran during
    review.
  - [ ] **RF216 — Reclassify legacy buildable icon manifest rows (P3; CR13-031).**
    `Scripts/Data/catalogue-icon-manifest.csv:11-22`;
    `Content/Data/GameCatalogues/items/buildItems/`;
    `Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp:83-107`.
    **Affected path:** A maintainer or UI audit uses the catalogue icon manifest to
    decide which views may display a placed-object identity.
    **Trigger:** WorkbenchKit, ForgeKit, WorkbenchToolRackKit, ForgeAnvilKit,
    GrindingStoneKit, StorageKit, CookingRackKit, FryingPanKit, CauldronKit,
    FloorKit, WallKit and RoofKit remain marked `identity_kind=item` with
    `inventory_scope=pack` and PackInventory, ItemDetail and Storage views. The
    current catalogue loader maps their authored buildable aliases to stable
    placed-object IDs, and AGENTS.md forbids portable construction items.
    CampfireKit, CharcoalKilnKit and CampBedKit show the intended construction/
    buildable-only manifest treatment. `Validate-CatalogueReferences.py` checks
    icon alias coverage but does not validate this role/view metadata.
    **Impact:** The manifest advertises portable inventory and storage use for
    placed objects, misleading future authoring or UI checks about the no-kit
    contract. No current portable item or grant was confirmed.
    **Evidence:** All twelve manifest rows have the stale kind, scope and views;
    the corresponding catalogue identities live in `items/buildItems/` and the
    runtime alias table maps them to their stable placed-object IDs.
    **Proposed repair:** Reclassify these rows as buildable/construction with
    `inventory_scope=none` and remove PackInventory, ItemDetail and Storage views.
    Preserve valid recipe/build/preview views, stable IDs and assets; do not
    create portable items or change runtime behavior.
    **Dependencies:** After CR19 review acceptance; no manifest edits during
    CR13.
    **Acceptance:** Every placed-object row in the icon manifest has a
    buildable/construction kind and no pack, item-detail or storage scope, while
    keeping its valid construction/preview icon views and all existing IDs.
    **Focused validation:** Compare all manifest identities and view scopes with
    registered item/buildable data and runtime aliases; run the catalogue/icon
    validators and `git diff --check`. No Unreal build is required.
  - [ ] **RF231 — Require all world profile metrics in the acceptance gate (P3; CR16-017).**
    `WorldProfile.ps1` only requires generation text, successful save
    serialization, and two players; its wildcard can accept a report missing
    or malformed memory, actor, terrain, population, and save-size metrics.
    Require each documented metric field with a valid numeric value, without
    adding performance thresholds. Evidence and acceptance:
    [RF231 finding](Temporary/Review/findings.md#rf231-require-all-world-profile-metrics-in-the-acceptance-gate-p3).
  - [ ] **RF232 — Make the Iron material icon read as a forged bar (P3; CR13-036).**
    `Content/Data/GameCatalogues/items/materials/metal.json:4-7`;
    `Content/Kalmala/UI/Source/IconOriginals/Iron.png`;
    `Content/Kalmala/UI/Source/Icons/Iron.png`;
    `Content/Kalmala/UI/Icons/Items/Iron.uasset`; `docs/icon-provenance.md`.
    **Affected path:** A player identifies refined Iron in the inventory or crafting UI.
    **Trigger:** The item is named `Iron bar` and described as a forged bar, but its icon is a rough, irregular rock-like mass similar to the separate Iron Ore icon.
    **Impact:** The image weakens the distinction between refined bar and ore at the 64×64 inventory size; missing prompt provenance also prevents reliable reproduction of the intended appearance.
    **Evidence:** `metal.json` defines a forged Iron bar while `ore.json` defines IronOre as iron-rich rock. The CR13-036 original and prepared art were visually inspected; the Iron icon reads as an irregular rock. The canonical package and runtime mapping validate, so this is a semantic art/provenance issue rather than missing coverage. The current provenance guide has no dedicated Iron-bar prompt record.
    **Proposed repair:** After CR19, revise the original Iron artwork to read as a forged bar, retain its prompt or explicitly record unavailable provenance, regenerate the prepared PNG, and reimport through the existing icon pipeline. Preserve IDs, recipes and behavior; do not edit assets during CR13.
    **Dependencies:** After CR19 review acceptance; no asset or documentation changes during CR13.
    **Acceptance:** Iron reads as a forged bar and is clearly distinguishable from IronOre at 64×64; source/prompt provenance is traceable or explicitly recorded as unavailable; catalogue IDs and runtime mapping remain intact.
    **Focused validation:** Run `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id Iron` and `python Scripts/Validate-CatalogueReferences.py`; visually compare the prepared Iron/IronOre icons at 64×64. No Unreal build is required.

  - [ ] **RF233 — Restore provenance records for active placed-object icons (P3; CR13-035/057).**
    Content/Kalmala/UI/Source/IconOriginals/{FloorKit,ForgeAnvilKit,ForgeKit,FryingPanKit,GrindingStoneKit}.png;
    Content/Kalmala/UI/Source/Icons/; Content/Kalmala/UI/Icons/Items/;
    Scripts/Data/catalogue-icon-manifest.csv; docs/icon-provenance.md.
    **Affected path:** A maintainer verifies or regenerates one of the placed-object
    textures for Floor, ForgeAnvil, Forge, FryingPan, or Grinding Stone.
    **Trigger:** These five active buildable icon textures have retained original
    and prepared PNGs and canonical runtime mappings, but no item-specific
    generation/source record appears in the current provenance/setup
    documentation. The originals identify generated media through C2PA markers
    but contain no readable prompt text. CR13-035 confirms the same gap for the
    GrindingStone buildable and its `GrindingStoneKit` texture: the original
    contains C2PA generator metadata but no text prompt, and the provenance guide
    has no item-specific source record. RF184 separately covers its stale import
    path, while RF216 covers its stale inventory/pack manifest views.
    **Impact:** The buildables and their icons remain usable, but the authored
    source and intended visual constraints cannot be audited or reproduced from
    the repository.
    **Evidence:** CR13-057 originals match their CR01 coverage fingerprints;
    focused icon validation resolves FloorKit, ForgeAnvilKit, ForgeKit, and
    FryingPanKit to the expected Texture2D paths. Repository documentation and
    icon-provenance/setup history searches found no item-specific source prompts.
    Each original has a caBX chunk and readable ChatGPT/gpt-image/OpenAI markers,
    with no PNG text chunks or readable prompt strings. RF184 separately covers
    six stale package source paths; RF216 separately covers the buildable
    manifest role/view mismatch.
    [Evidence and disposition](Temporary/Review/findings.md#rf233-finding-missing-prompt-provenance-for-five-placed-object-icon-textures).
    **Proposed repair:** Recover and record authoritative original prompts or
    source notes where available; otherwise record provenance as unavailable.
    Do not invent prompts, regenerate icons, or change buildable data, recipes,
    IDs, or direct-material placement during CR13.
    **Dependencies:** After CR19 review acceptance; no icon or documentation
    edits during CR13.
    **Acceptance:** Each of the five placed-object icon identities has an
    authoritative source record or explicit unavailable-provenance disposition;
    existing textures, mappings, buildable definitions, and direct-material
    recipes remain intact.
    **Focused validation:** Compare each source record with its original,
    prepared PNG, imported package, manifest mapping, and buildable definition;
    run the five focused UI icon validators and
    python -B Scripts/Validate-CatalogueReferences.py. No Unreal build is required.

  - [ ] **RF234 — Regenerate the stale world-generation scalar previews (P3; CR13-023).**
    `Content/Kalmala/Developer/WorldGeneration/{Elevation,Flora,Humidity,Temperature}.ppm`;
    `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp`;
    `Source/KalmalaWorld/Public/KalmalaWorldFieldSampler.h`,
    `KalmalaWorldGenerationSeeds.h`, and `KalmalaRegionalTuning.h`.
    **Affected path:** A developer uses the tracked scalar PPMs to inspect current world fields.
    **Trigger:** The four previews were added with the original commandlet at `cb5175f1` and predate the current field equations and seed derivation.
    **Impact:** The maps can misrepresent present elevation, humidity, temperature, and flora distributions during manual generation review.
    **Evidence:** All four PPM files still match CR01 and current `HEAD`, and their last asset change is `cb5175f1` (2026-09-02). That version of the sampler used direct normalized Perlin fields with frequencies 0.00035/0.00050/0.00028/0.00075 and seed offsets multiplied by 31. The current sampler uses different tuning and frequencies and adjusts Elevation from the land-water master mask; `DeriveFieldSeed` also changed after the image commit. The current commandlet writes all four sampled channels through `AddFieldPixel`, so the tracked scalar maps are historical output. No runtime code references these PPMs.
    [Evidence and disposition](Temporary/Review/findings.md#rf234-refresh-stale-world-generation-scalar-previews).
    **Proposed repair:** After CR19 review acceptance, re-render only these four scalar PPMs from the current commandlet using its default seed, dimensions, and extent. Do not change PPM assets during CR13.
    **Dependencies:** After CR19 acceptance; no generated-content changes during CR13.
    **Acceptance:** Each tracked scalar preview matches current commandlet output for the documented defaults and retains the expected grayscale channel.
    **Focused validation:** Build the Editor, render to a short temporary directory, compare the four generated PPMs with the tracked assets, visually inspect them, and run `git diff --check`. Do not run broad regional-generation peers for this artifact-only repair.

  - [ ] **RF235 — Align the Standing Light description with its torch icon (P3; CR13-065).**
    `Content/Data/GameCatalogues/items/buildItems/other.json:58-62`;
    `Content/Kalmala/UI/Source/IconOriginals/StandingLightKit.png`;
    `Scripts/Data/catalogue-icon-manifest.csv`; `docs/icon-provenance.md#standing-light-icon`.
    **Affected path:** A player reads the Standing Light description in the construction UI.
    **Trigger:** The catalogue calls the object a timber brazier, while the retained icon and its current prompt specify a freestanding wooden torch and explicitly exclude brazier/bowl imagery.
    **Impact:** The description and icon communicate different fixtures, weakening the placed object's identity in the construction UI.
    **Evidence:** CR13-065 confirmed the original PNG matches its CR01 and HEAD fingerprint. Its tripod, upright timber shaft, rope-bound kindling and flame match the `StandingLight` buildable mapping and the retained prompt. The registered catalogue description still says “A freestanding timber brazier that burns Wood for a small cosmetic light.”
    **Proposed repair:** After CR19 acceptance, describe the object as a freestanding timber torch. Preserve its buildable identity, direct-material recipe, fuel behavior and construction-only semantics.
    **Dependencies:** After CR19 review acceptance; no catalogue or art changes during CR13.
    **Acceptance:** The construction description calls the object a torch, consistent with its retained icon and prompt, while the existing buildable ID, recipe, fuel behavior and buildable-only role remain unchanged.
    **Focused validation:** Parse the registered catalogues and references; compare the `StandingLight` buildable, direct-material recipe and icon alias; run `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id StandingLightKit`, `python -B Scripts/Validate-CatalogueReferences.py`, and `git diff --check`. No Unreal build is required.
  - [ ] **RF236 — Thicken the Light Bow icon limbs to meet its recorded prompt (P3; CR13-082).**
    `Content/Kalmala/UI/Source/IconOriginals/LightBow.png`;
    `Content/Kalmala/UI/Source/Icons/LightBow.png`;
    `Content/Kalmala/UI/Icons/Items/LightBow.uasset`;
    `docs/icon-provenance.md#light-bow-icon`;
    `Scripts/Data/catalogue-icon-manifest.csv`;
    `Source/KalmalaUI/Private/KalmalaIconWidget.cpp`.
    **Affected path:** A player scans the Light Bow in carried-tool, detail,
    repair, or recipe-result UI at its prepared 64×64 size.
    **Trigger:** The prepared bow remains identifiable, but its curved wooden
    stave and limbs read as a narrow stroke at 64×64 instead of the broad,
    sturdy silhouette required by its retained prompt.
    **Impact:** The icon is less legible at inventory scale and does not meet
    its recorded visual target; no gameplay or mapping defect was found.
    **Evidence:** CR13-082 confirmed that the original, prepared PNG, and
    imported package match their CR01 fingerprints. The source is a 1254×1254
    RGBA image and the prepared icon is 64×64 RGBA. The prompt explicitly
    requires thick sturdy limbs, clearly visible wood at 64px, and no hair-thin
    lines. Visual inspection of the prepared 64×64 icon shows a very slender
    stave despite the recognizable bow shape. The registered catalogue uses
    `LightBow`; the manifest and canonical runtime alias map it to the Bow
    icon. Its package source path resolves to the project-local prepared PNG.
    **Proposed repair:** After CR19 review acceptance, revise the source artwork
    to give the wood limbs a clearly broader silhouette at 64×64 while keeping
    the compact arc, central grip, separated taut string, no-arrow constraint,
    established style, and existing item identity. Reprepare and reimport the
    icon without changing gameplay or runtime mapping.
    **Dependencies:** After CR19 acceptance; no art or package edits during
    CR13.
    **Acceptance:** The prepared 64×64 icon has clearly visible sturdy wooden
    limbs and a broad recognizable bow silhouette, while the string remains
    visibly separate; the LightBow ID, manifest role, and runtime mapping stay
    unchanged.
    **Focused validation:** Visually inspect the original and prepared icon at
    native 64×64 size; check dimensions, transparency, and LightBow package
    identity; run `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id
    LightBow`, the catalogue-reference validator, and `git diff --check`. No
    Unreal build is required.
  - [ ] **RF237 — Attribute snake bite observations to the selected target (P3; CR03-001b-19).**
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1388-1394,1610-1622,1679-1700`;
    `Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:353-377`;
    `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3299-3320`.
    **Affected path:** The non-Shipping Viper and Giant Snake peer-verification
    flags retained by `AKalmalaGameMode::DriveCombatPeerTest`.
    **Trigger:** During stages 1–2, any weak-poison status latches the Viper
    observation, and any attacker health below 100 latches the Giant Snake bite
    observation. Either branch then disables ticking on the selected target,
    without tying the event to that actor.
    **Evidence:** Setup activates the selected key's wildlife population and
    retains only the expected target; it does not isolate or silence other
    spawned wildlife in that key.
    Viper detection checks only the attacker's shared WeakPoison entry. Giant
    Snake setup restores health but does not clear pre-existing poison, whose
    server tick can reduce health; another active attacker can also do so. The
    success result later prints the latched boolean as `Bite=1`. No accepted
    bite source, damage type, or target spawn ID is checked before latching.
    **Impact:** A retained species fixture can report a successful selected
    snake bite and stop that target even when unrelated damage/status caused the
    signal. No production gameplay impact was found.
    **Proposed repair:** Tie observation to a server-accepted snake bite from
    the selected target's persistent spawn ID. Do not infer bite source from
    aggregate player health or status; retain the current species and privacy
    assertions.
    **Dependencies:** After CR19 review acceptance; no fixture repair during
    CR03 or CR18.
    **Acceptance:** Only an accepted bite from the selected Viper or Giant Snake
    can latch its bite result or pause that target. Pre-existing poison, a
    sibling creature's attack, and unrelated health loss do not produce a pass.
    A valid selected-target bite still reaches the existing success path.
    **Focused validation:** Add a narrow development regression that injects
    pre-existing poison or non-target damage while the selected snake is unable
    to attack and proves the fixture does not pass; then enable a selected-target
    bite and prove only that event passes. Run the focused poison and snake
    contracts after an Editor build. The current consolidated peer runner does
    not include these legacy species flags; record any runner change separately.
  - [ ] **RF238 — Check every observer in retained Badger/Moon Bear escrow and retry modes (P3; CR03-001b-22,24).**
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1818-1861,1887-1892`.
    **Affected path:** Development-only Badger and Moon Bear pending-reward full-pack escrow and retry predicates.
    **CR03-001b-24 source:** KalmalaGameMode.cpp lines 1971–2017.
    **Trigger:** With an owner, the selected remote peer, and another observer, the retry delivers the expected reward to the owner but also grants Bone Fragment or Badger Fur/Meat to the observer. The generic `bExpectedReward` branch checks only `RemoteInventory`; it does not require any reward to be absent from the other peers, yet the success log reports `OwnerOnly=1`.
    **Evidence:** The fixture enumerates all other player characters and accumulates `OtherPeerBoneFragment`, but uses that count only in the Rat/Viper/Giant Snake branches. It does not accumulate other-peer Badger Fur/Meat at all. The Badger/Moon Bear branch checks the selected Remote inventory and then reaches the same `Passed=1 ... OwnerOnly=1` result without checking the remaining observers. The current consolidated runner uses Rat `SharedCombat`; these legacy modes have no current runner scenario.
    **Impact:** A retained Development fixture can report owner-only retry success while an additional non-owner peer holds a duplicate reward. No production reward leak was established.
    **Proposed repair:** Aggregate each expected reward item across every non-owner inventory and require all such totals to remain zero during escrow and retry; keep the owner's exact reward counts and owner-bound claim check. If the legacy modes remain callable, do not emit `OwnerOnly=1` until every observer is checked.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **CR03-001b-24 evidence:** The full-pack escrow predicate enumerates other peers but only checks their totals when bTwoItemRewardPeerTest is true, which excludes Badger and Moon Bear. It does not accumulate other-peer Badger Fur/Meat. A third peer can hold Bone Fragment or a relevant Badger reward while the owner and selected remote remain clean.
    **Expanded acceptance:** A relevant reward injected into either non-owner during full-pack escrow or after retry makes the fixture fail before the corresponding pass marker. The owner receives the exact expected reward only after space is freed. The existing focused validation applies to both phases.
    **Acceptance:** For Badger and Moon Bear with an owner and two non-owner peers, normal retry grants the exact reward only to the owner. An injected duplicate Bone Fragment, Badger Fur, or Badger Meat on either non-owner makes the fixture fail. Existing Rat `SharedCombat` assertions remain intact.
    **Focused validation:** Add a narrow Development regression for both retained modes that injects a duplicate reward into the second observer during retry and verifies no pass marker is emitted. Run it after an Editor build; no per-species live reward runner is required.
  - [ ] **RF239 — Require accepted attacks before retained reward fixture completion (P3; CR03-001b-23).**
    `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1240,1756-1765,1910-1959,1887-1892`.
    **Affected path:** Retained Development Badger and Moon Bear pending-reward retry modes.
    **Trigger:** One of the four attack requests is rejected while the fixture's request counter still advances to four. The Badger/Moon Bear path then skips any accepted-action-serial check, applies fixture-only damage or direct defeat, finds the owner claim, and can later emit a success marker with the literal `ActionSerial=4`.
    **Evidence:** `CombatPeerTestSequence` increments before the void server RPC. `ServerRequestAttack_Implementation` returns before incrementing `ActionSerial` when the request is not allowed. The stage-two gate checks the attempt count and Idle phase; only Viper/Giant Snake require four action serials here, while Rat fails unless its natural single-hit defeat occurred. Badger/Moon Bear can reach the bounded server-damage/`DefeatServer` fallback and owner-claim lookup without accepted-action evidence. Their retained retry success output does not recheck the serial and prints four as a constant.
    **Impact:** A retained Development fixture can report its four-attack reward setup passed even when one or more player attack requests were rejected. This does not establish a production combat or reward defect; the current runner exercises the Rat path.
    **Proposed repair:** Before fixture-only defeat or owner-claim success, require the actual server `ActionSerial` to equal `ExpectedAttackCount` for Badger and Moon Bear as well as the already-checked snake modes. Keep Rat's natural-defeat gate and report the actual serial in the result.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** In both retained modes, four accepted attacks allow the existing bounded fixture defeat and reward retry. If an attack request is rejected but the attempt counter still reaches four, the fixture fails before fixture-only damage/claim success and emits no pass marker. Rat and snake assertions remain intact.
    **Focused validation:** Add a narrow Development regression for Badger and Moon Bear that forces one attack rejection during the four-request sequence and proves the fixture fails before reward setup; verify four accepted requests still reach the current completion path. Run after an Editor build. The current consolidated runner does not include these legacy species modes.
  - [ ] **RF240 — Check every peer in active Boar/Deer combat reward acceptance (P3; CR03-001b-26).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1288-1290,2057-2086; Scripts/Tests/Combat/EnemyPeers.ps1:50-69,218-221,276-282.
    **Affected path:** The current BoarCharge and DeerHerd peer scenarios' final combat reward predicates.
    **Trigger:** The host has the expected reward and the selected first remote has zero relevant items, while the second remote has a duplicate BoarMeat, BoarHide, BoneFragment, DeerMeat, DeerHide, or DeerSinew. The final bRewardValid check does not inspect that second remote, so the fixture can still emit Passed=1.
    **Evidence:** CombatPeerTestAttacker is the local server player and CombatPeerTestRemote is only the first remote actor. Boar/Deer stage-zero setup does not require the second remote; the minimum-two-remote gate applies only to bTwoItemRewardPeerTest. The final predicate checks attacker counts and zero counts in RemoteInventory, with no additional-player inventory aggregation. EnemyPeers starts a client and then a separately named observer for both scenarios. Its case checks combat/replication outcomes, not observer inventory, and a final server pass marker is not invalidated if it was emitted before observer arrival.
    **Impact:** An active Development combat scenario can pass its reward predicate while another non-owner holds a duplicate reward. No production reward leak was established.
    **Proposed repair:** Require the two non-owner peers used by the current BoarCharge/DeerHerd runner before fixture progression, then aggregate the applicable Boar or Deer reward IDs over every non-owner inventory and require zero totals before the final pass marker. Preserve exact attacker quantities, action-serial gating, and the Deer herd-alert check.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** Existing BoarCharge and DeerHerd runs wait for two non-owner peers, then pass only when the host has the exact expected rewards and both remotes have zero relevant items. A relevant reward injected into the second remote, or a missing second remote, prevents Passed=1; the existing attack, health-replication and Deer herd assertions remain intact.
    **Focused validation:** Add narrow Development fixtures for both scenarios that confirm no pass marker is emitted before two non-owner peers are present, inject each relevant reward into the second remote, and verify failure. Run after an Editor build using the current EnemyPeers runner.
  - [ ] **RF241 — Verify Mosquito dodge cooldown after the active roll ends (P3; CR03-001b-30).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2312-2337; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:136-143,1273-1311,1331-1342; Scripts/Tests/Combat/EnemyPeers.ps1:133-143.
    **Affected path:** The current MosquitoCounterplay fixture's accepted-roll and rejected-retry sequence.
    **Trigger:** The fixture receives one accepted roll, immediately asks the owner client to send another request, then counts one rejection as the cooldown result while the first roll can still be active.
    **Evidence:** ServerRequestDodgeRoll_Implementation starts a 0.35-second active roll and a 1-second cooldown. The admission helper checks !bAlreadyRolling before checking that the cooldown has expired. The second request is sent without waiting for the active roll to end, and both rejection paths increment the same counter. Stage four and the runner only require one rejected request; they do not distinguish an active-roll rejection from a cooldown rejection.
    **Impact:** MosquitoCounterplay can pass without proving that the cooldown gate rejects a retry. No production dodge defect was established.
    **Proposed repair:** Send the cooldown probe after the 0.35-second roll window but before the 1-second cooldown expires, with the other request preconditions valid, and record or assert that the cooldown gate is the rejection reason.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** The fixture records one accepted roll, waits until the active roll has ended while cooldown remains, then sends the owner request. It fails if the request is accepted or rejected by another gate and passes only on an expired-roll/positive-cooldown rejection; the existing mosquito escape and flame assertions remain intact.
    **Focused validation:** Add a narrow Development regression for the expired-roll/positive-cooldown case and a control case with cooldown expired and all other gates valid. Confirm the peer scenario cannot pass when the cooldown gate is bypassed. Run after an Editor build using EnemyPeers MosquitoCounterplay.
  - [ ] **RF242 — Verify the full two-second Mosquito repel window (P3; CR03-001b-31).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2350-2353,2416-2420; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:146-151,1314-1318; Source/KalmalaGameplay/Private/Tests/KalmalaDodgeRollTest.cpp:41-42; Scripts/Tests/Combat/EnemyPeers.ps1:133-139.
    **Affected path:** The current MosquitoCounterplay fixture's dodge escape-window verification.
    **Trigger:** The implementation shortens the applied swarm-repel interval (for example, to 1.5 seconds) while keeping the documented/configured constant at 2 seconds.
    **Evidence:** Stage five requires repelling only after at least 1.25 seconds. Stage six does not check expiry until at least 2.05 seconds, when it expects repelling to have ended. A 1.5-second interval satisfies both samples. The focused dodge automation checks the constant equals 2.0, while the peer fixture samples the applied state only at those coarse bounds; the runner only requires RollExpired=1.
    **Impact:** The peer fixture can pass while the server ends the documented two-second Mosquito escape window early. No current production timing defect was established.
    **Proposed repair:** Observe the authoritative repel state near the two-second boundary and again just after expiry, using the server acceptance/expiry timestamp and a defined tolerance so a materially shortened interval fails.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** The test fails when the applied repel window is materially shorter than two seconds, confirms it remains active near the end of the documented window, and confirms it is inactive shortly after expiry. Existing roll, cooldown and held-flame checks remain intact.
    **Focused validation:** Add a narrow timed Development regression that verifies the applied server expiry against the accepted-roll timestamp, including a shortened-window control that must fail. Run after an Editor build and retain the MosquitoCounterplay peer check.
  - [ ] **RF243 — Verify Mosquito bites around a paid, unlit campfire (P3; CR03-001b-33).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2463-2484,2485-2503; Source/KalmalaGameplay/Private/KalmalaCampfire.cpp:69-72,205-214; Source/KalmalaGameplay/Private/Tests/KalmalaHearthStateTest.cpp:24-30; Scripts/Tests/Combat/EnemyPeers.ps1:133-139; docs/biome-enemies.md:562-570.
    **Affected path:** The current non-Shipping MosquitoCounterplay fixture's unlit placed-campfire assertion.
    **Trigger:** The campfire is spawned and checked for no flame eligibility before `InitializePaidFromServer`; the fixture then waits for a bite while it is still unfuelled and initializes and lights it consecutively after the bite.
    **Evidence:** Stage seven spawns the campfire and checks only its default unlit/flame-ineligible state. Stage eight repeats the checks and waits for a bite before `InitializePaidFromServer`, then immediately calls `TryLightFromServer`. The flame query requires authority, lit state, positive finite fuel and positive finite warmth. Initialization sets owner and fuel without lighting the campfire, so the fixture can observe bites in a valid paid-unlit state before ignition.
    **Impact:** The Mosquito peer scenario can report `CampfireUnlitNoProtection=1` without proving that a fueled, extinguished placed campfire permits bites. No production campfire or mosquito defect was established.
    **Proposed repair:** Initialize/pay the campfire while keeping it extinguished, assert positive fuel and no flame eligibility, then require a selected-swarm bite before lighting it. Preserve the later lit-fire no-bite assertion.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** The peer fixture cannot pass its unlit-campfire check until it observes a selected-swarm bite while the campfire is owned, fueled, extinguished and flame-ineligible. Skipping initialization or lighting before the bite fails; the existing lit-fire repel assertion remains intact.
    **Focused validation:** Add a narrow Development regression or peer-fixture check for the paid, extinguished state, including an unfuelled control and the lit-fire transition. Run after an Editor build with `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario MosquitoCounterplay`.
  - [ ] **RF244 — Require weak poison on the bitten owner peer (P3; CR03-001b-35).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2136-2144,2530-2551; Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:99-103,493-507; Scripts/Tests/Combat/EnemyPeers.ps1:133-152,276-283; docs/07-development-setup.md:2592-2607.
    **Affected path:** The current non-Shipping MosquitoCounterplay peer privacy acceptance.
    **Trigger:** The bitten-owner client has no replicated weak-poison status, while the observer client logs one; the two results are exclusive, so the runner's XOR passes.
    **Evidence:** The GameMode assigns the first remote pawn as bitten Host and the second as observer. The runner starts the client and waits for its join before starting the observer. The server checks that the bitten Host has at least eight seconds of weak poison. However, the final runner predicate accepts either `$clientPoison` or `$observerPoison` through XOR, so observer-only delivery passes even though setup requires the owner to receive poison and the observer not to receive it.
    **Impact:** The peer scenario can pass if poison is missing from its intended owner or appears only on the observer, leaving owner-only delivery unverified. No production privacy leak was established.
    **Proposed repair:** Require poison in the designated bitten-owner client log and reject any poison in the observer log. Include explicit role or pawn identity in the observation if needed to keep the association unambiguous.
    **Dependencies:** After CR19 review acceptance; no runner repair during CR03 or CR18.
    **Acceptance:** Only owner-client poison with no observer poison passes. Owner-only absence, observer-only delivery, delivery to both peers, or delivery to neither peer fails; public-health checks and the server poison-duration assertion remain intact.
    **Focused validation:** Add a narrow check for all four owner/observer Boolean combinations, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario MosquitoCounterplay` after an Editor build.
  - [ ] **RF245 — Select Peat Husk fixtures from the activated population budget (P3; CR03-001b-36).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2612-2661,8320-8380; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:102-130; Source/KalmalaWorld/Public/KalmalaBiomeExpansionContract.h:43-62.
    **Affected path:** The non-Shipping PeatHuskPeerTest stage-zero descriptor selection and activation.
    **Trigger:** A Mossy Mire key has multiple generated wildlife descriptors, its 70% budget truncates the tail, and the first undefeated Peat Husk descriptor is in that discarded tail.
    **Evidence:** Stage zero searches the complete BuildSpawnDescriptors result and stores the first matching Peat Husk ID. ActivatePopulationKey rebuilds that array, then truncates it to ApplyPopulationBudget before spawning; the Mossy Mire wildlife multiplier is 0.70. The later fixture stage searches for the stored ID and times out if it was discarded. The Mosquito peer selector already applies the same biome budget before searching.
    **Impact:** The Development peer fixture can stop at an actor that cannot spawn and later report a missing-target failure, even when a later key in the bounded search has a retained eligible Peat Husk. No production spawn or gameplay defect was established.
    **Proposed repair:** Apply the activation path's biome budget truncation to the descriptor array before selecting the Peat Husk, keeping saved-defeat and hard-enemy-distance checks against the retained descriptors.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** The fixture stores only the ID of a descriptor retained by activation. A trimmed-tail candidate does not stop the bounded search; it continues until a retained undefeated eligible Peat Husk is found or all keys are exhausted, then reports no candidate at stage zero if necessary. No discarded ID can produce a stage-one timeout.
    **Focused validation:** Add a narrow Development regression with a trimmed-tail candidate in an earlier key and a retained eligible candidate in a later key, plus a no-retained-candidate case. Then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario PeatHuskSweep -WorldSeed 445 after an Editor build. No test was run during this review.
  - [ ] **RF246 — Tie Peat Husk telegraph observations to the selected actor (P3; CR03-001b-37).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2791-2823; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:5143-5152,5318-5323; Scripts/Tests/Combat/EnemyPeers.ps1:156-166.
    **Affected path:** The PeatHuskSweep peer scenario's replicated-telegraph acceptance.
    **Trigger:** Another Peat Husk actor emits a telegraph marker to both clients while the selected fixture target's telegraph is absent from one or both client logs.
    **Evidence:** The server fixture stores the selected Target weak reference and logs its SpawnId on success, but client-side OnRep handlers emit an identical marker with no spawn ID for any Peat Husk that enters Telegraph. The runner checks only for that generic marker in each remote log and does not correlate it with the server's selected SpawnId.
    **Impact:** PeatHuskSweep can pass without proving that both remote clients observed the selected target's replicated telegraph. No production replication defect was established.
    **Proposed repair:** Include the persistent spawn ID in each client telegraph marker, parse the selected fixture ID, and require that same ID in both client logs.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** Only the selected target's telegraph observation on both designated remote clients satisfies the peer cue gate; a marker from any other Peat Husk does not.
    **Focused validation:** Inject or fixture a second Peat Husk telegraph while suppressing the selected target's marker on one client and verify failure; verify matching selected-ID markers on both clients pass. Run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario PeatHuskSweep -WorldSeed 445 after an Editor build.
  - [ ] **RF247 — Continue fixture search when a Peat Husk has no dry approach (P3; CR03-001b-37).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2612-2649,2735-2770; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:837-840; Scripts/Tests/Combat/EnemyPeers.ps1:156-166.
    **Affected path:** The PeatHuskPeerTest candidate-to-host placement setup.
    **Trigger:** The first activation-retained Peat Husk has no direction among the 16 tested angles with three dry Mossy Mire samples along the 120 cm approach, while a later bounded-search key has another activation-retained candidate that does.
    **Evidence:** Stage zero selects by archetype, start distance, stable ID and saved-defeat status without testing the fixture's approach geometry. Stage one tests the 16 directions only after selecting and activating that target; if none passes, it terminates the fixture instead of searching another candidate.
    **Impact:** The Development peer fixture can fail on an unsuitable first spawn even when another activation-retained Peat Husk in a later key can support the sweep. No production spawn or attack defect was established.
    **Proposed repair:** After activation-budget filtering, require a viable fixture approach while selecting candidates, or continue the bounded search when the first target has no supported approach. Report no suitable fixture only after exhausting eligible candidates.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** An activation-retained candidate with no valid sampled dry approach does not stop the search when a later activation-retained candidate has one; the fixture arms only after selecting and placing a target with a validated approach. If none qualifies, it reports no suitable candidate without an activation timeout.
    **Focused validation:** Add a narrow Development regression with an earlier no-approach Peat Husk and a later valid approach, plus a no-valid-approach case. Then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario PeatHuskSweep -WorldSeed 445 after an Editor build.
  - [ ] **RF248 — Select Wolf peer fixtures only from activation-admitted packs (P3; CR03-001b-39).**
    Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2897-2930,2963-2987,3068-3095,8308-8337,8390-8407; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:85-122; Source/KalmalaWorld/Public/KalmalaBiomeExpansionContract.h:43-62; Source/KalmalaGameplay/Public/KalmalaWolfPackContract.h:15-20; Scripts/Tests/Combat/EnemyPeers.ps1:71-84.
    **Affected path:** The non-Shipping WolfPack peer fixture's stage-zero selection and population-key activation.
    **Trigger:** The first complete undefeated Wolf pack with a valid host approach lies beyond the 65% Tundra descriptor prefix, or earlier wildlife slots consume enough of the per-key actor budget that the selected pack cannot fit, while another activation-admitted pack exists in the bounded search.
    **Evidence:** Stage zero searches the full BuildSpawnDescriptors result, checks the three member IDs/defeat states and host approach, then stores that pack without applying Tundra's population budget or simulating actor-budget admission. ActivatePopulationKey rebuilds the array, truncates Tundra wildlife with ApplyPopulationBudget (multiplier 0.65), and skips a Wolf pack if previously spawned actors leave fewer than three actor slots. The fixture then waits for the selected member IDs and reports a missing-member failure after 20 seconds instead of searching for another pack. No production Wolf spawn defect was established.
    **Impact:** The Development peer fixture can select members that activation will not spawn and fail despite another complete eligible pack being available. This is the WolfPack selector path; RF245 records the separate Peat Husk selector case.
    **Proposed repair:** Select only a pack whose descriptor survives the same Tundra prefix and whose three actors fit the activation actor budget after preceding eligible slots; continue the bounded search when a candidate would be omitted or skipped. Report no suitable pack at stage zero only after exhausting eligible candidates.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** The fixture never stores member IDs for a pack that activation omits or skips. A tail or over-budget candidate does not stop the bounded search when a later activation-admitted pack exists; if none qualifies, the fixture fails clearly at stage zero rather than timing out on absent members.
    **Focused validation:** Add narrow Development regressions for a tail Wolf pack before a later retained pack, a retained pack skipped because earlier actors consume its three-member budget before a later admissible pack, and no admissible pack. Then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario WolfPack -WorldSeed 445 after an Editor build.
  - [ ] **RF249 — Place Wolf peer fixture pawns above sampled terrain (P3; CR03-001b-40).**
    **Source:** CR18-002, CR03-001b-40; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2989-3003`; neighboring fixture helpers at `KalmalaGameMode.cpp:1492-1501,2104-2115`; normal pawn placement at `KalmalaGameMode.cpp:11754-11761`; `Source/KalmalaGameplay/Public/KalmalaCharacter.h:52-52`; Wolf bite validation at `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1674-1696`.
    **Affected path:** The non-Shipping WolfPack peer fixture's listen-host and observer placement before pack activation.
    **Trigger:** The fixture reaches stage zero with a listen host and two remote observers, finds an undefeated pack, and calls `PlaceOnTerrain` for each pawn.
    **Evidence:** `AKalmalaCharacter` derives from `ACharacter`, whose actor location is the capsule center. `PlaceOnTerrain` replaces Z with sampled terrain height and teleports with sweep disabled, then stops and disables character movement; it never adds capsule half-height or verifies a grounded/collision-safe final location. Neighboring fixture helpers add `GetSimpleCollisionHalfHeight() + 1.0f`, and normal pawn placement adds capsule height plus clearance. Host approach eligibility is computed from sampled surface locations with a 170 cm margin, while `TryApplyWolfBite` checks the final actor-to-actor 3D distance against 180 cm; any height correction must preserve that actual bite range.
    **Impact:** The Development fixture can leave host and observer capsules intersecting the ground, making placement and the subsequent Wolf bite/peer acceptance unreliable. No production gameplay defect or live fixture failure was established.
    **Proposed repair:** Place each pawn using sampled ground height plus capsule half-height and a small clearance (or a validated ground placement query), verify the resulting location, and re-evaluate the host's actual 3D distance to the selected Wolf after placement.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** Host and observers finish with capsule bottoms above sampled terrain, including on the steepest accepted fixture approach; placement success is confirmed; the placed host remains within the Wolf's actual 180 cm 3D bite boundary; and both observers remain outside the 3 m acquisition range in world bounds.
    **Focused validation:** Add a narrow Development regression for flat and steep accepted terrain that checks capsule clearance, final location and post-placement 3D bite reach, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario WolfPack -WorldSeed 445` after an Editor build.
  - [ ] **RF250 — Validate Lynx, Polar Bear, Horned Goat, and Thunder Sentinel fixture capsule placement on accepted Tundra and Mountain slopes (P3; CR03-001b-42/46/50/52/54).**
    **Source:** CR18-002, CR03-001b-42/46/50/52/54; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:3198-3212,3248-3257,3553-3578,3791,3843,3862,3929-3954,4079-4082,4128,4174,4190-4193,4284-4325,4476,4524,4553`; `Source/KalmalaGameplay/Public/KalmalaCharacter.h:52`; capsule use at `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:793-795`; peer contracts at `docs/07-development-setup.md:2711-2780`; composite acceptance at `Scripts/Tests/Combat/EnemyPeers.ps1:1-5,86-129,169-170`.
    **Affected path:** The non-Shipping LynxLeap, PolarBearCharge, HornedGoatPush, and ThunderSentinelSlam fixture placement of the listen host and observers.
    **Trigger:** A fixture places a pawn on a dry Tundra or Thunder Mountains sample whose center normal passes the `Z >= 0.82` gate while terrain rises across the capsule footprint.
    **Evidence:** All four fixtures accept Tundra or Mountain surface normals down to Z 0.82. Their `PlaceOnTerrain` helpers raise the capsule center by `GetSimpleCollisionHalfHeight() + 1 cm` above a center-point terrain sample, teleport with sweep disabled, and do not verify final capsule clearance. The Goat and Sentinel path helpers sample their centerlines at intervals of at most 80 cm, but do not query the capsule's horizontal extent; direct host and observer placement calls use the same unvalidated helper. On an uphill slope across a finite-radius capsule, center-point clearance does not prove the full capsule is clear. No specific seed or live collision failure was established.
    **Impact:** Any of the four Development fixtures can start with a host or observer capsule intersecting sloped terrain, blocking or occluding the selected attack approach or observer placement and making peer acceptance unreliable. No production gameplay defect was established.
    **Relationship to existing findings:** RF249 covers the separate Wolf fixture helper that omits capsule half-height even on flat terrain. RF250 covers the Lynx, Polar Bear, Horned Goat, and Thunder Sentinel helpers that add center-point clearance but do not validate the full capsule. RF19 already covers stale standalone peer-script commands, including HornedGoatPeer.ps1; RF251 covers already-active-key handling for LynxLeap, PolarBearCharge, HornedGoatPush and ThunderSentinelSlam, RF252 covers selected-attack outcome attribution for all four peer fixtures, and RF253 covers the Goat observer-distance assertion gap.
    **Proposed repair:** Validate the complete capsule against terrain with a collision-aware placement query or sweep and confirm the final location. Skip unsafe host ring/approach candidates before activation where possible, and fail before stage-two arming if final host or observer placement is not collision-clear. Ensure the final placed host remains within each selected attack's actual reach.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** None of the four fixtures proceeds to attack/peer acceptance after any pawn placement overlaps terrain; hosts and observers finish collision-clear on flat ground and the steepest accepted Tundra or Mountain slope; unsafe host ring/approach candidates are skipped or rejected before activation, invalid final placements fail before stage-two arming, and the placed host remains within each fixture's documented attack reach.
    **Focused validation:** Add narrow Development regressions for all four fixtures covering flat terrain, a slope near normal-Z 0.82, and obstructed capsule placement. Confirm final placement and actual host-to-target distance, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario LynxLeap -WorldSeed 445`, `-Scenario PolarBearCharge -WorldSeed 445`, `-Scenario HornedGoatPush -WorldSeed 445`, and `-Scenario ThunderSentinelSlam -WorldSeed 445` after an Editor build.
  - [ ] **RF251 — Handle already-active keys before arming Lynx, Polar Bear, Horned Goat, and Thunder Sentinel peer fixtures (P3; CR03-001b-44/47/48/51/52/54/55/56).**
    **Source:** CR18-002, CR03-001b-44/47/48/51/52/54/55/56; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:3428-3459,3580-3609,3628-3777,3791-3831,3986-4116,4129-4159,4356,4369-4464,4477-4509,7880-7923,8308-8313,8366-8380,8618-8620`; `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:454-465`; `Scripts/Tests/Combat/EnemyPeers.ps1:86-113,169-170`; `docs/07-development-setup.md:2764-2770`.
    **Affected path:** The non-Shipping LynxLeap, PolarBearCharge, HornedGoatPush, and ThunderSentinelSlam fixtures' stage-zero selected-target activation and stage-one selected-actor wait.
    **Trigger:** A fixture waits for its two observers while normal Tick activates the host's current key, then its bounded selector chooses a descriptor in that already-active key but no live actor with that persistent ID exists. The Sentinel selector does not exclude active keys, and its key-capacity rejection applies only to new keys.
    **Evidence:** Tick calls peer-fixture drivers before periodic player-key activation. Stage zero returns while fewer than two observers are present; the Sentinel observer wait is at line 4356. During that wait, the ordinary path can activate the host key using the default `bAllowLegacyWildlifeForVerification=false`, marking it active without legacy wildlife materialization. The Sentinel selector at lines 4369-4464 does not skip active keys; its check at 4458-4463 allows an already-active key even when the key budget is full. The later `ActivatePopulationKey(key, true)` call returns at the `ActivePopulationSpatialKeys.Contains` guard before the verification-only wildlife branch, and stage one waits up to 20 seconds for the exact persistent ID. The ordinary scheduler may materialize that descriptor later, but its five-second cadence, seeded admission roll and other gates do not guarantee it before timeout. The Lynx, Polar Bear and Goat fixtures have the same active-key false-failure path. No specific seed or live failure was established.
    **Impact:** Any of the four Development peer fixtures can wait 20 seconds and report that its selected wildlife actor did not materialize after direct fixture activation became a no-op. No production wildlife spawning defect was established.
    **Relationship to existing findings:** RF245 and RF248 cover descriptor-budget/ordered-admission mismatches in other peer selectors. RF250 covers pawn placement in the four peer fixtures; RF252 covers selected-attack outcome attribution in all four peer fixtures; RF253 is the separate Goat observer-distance assertion gap. RF251 concerns a selected target missing after an already-active-key activation no-op.
    **Proposed repair:** Before staging, accept a matching live selected actor already present; otherwise avoid selecting an already-active key that lacks the selected actor, continue the bounded search to an activatable candidate, or use an explicit fixture-only activation path with a verified result. Do not publish stage one after a no-op activation without a guaranteed target.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** Regressions pre-activate a candidate key through the ordinary path while the selected Lynx, Polar Bear, Goat, or Sentinel is absent. Each fixture either finds a different key it can activate or fails clearly before staging; none proceeds into the 20-second wait on a target whose key activation was a no-op. A matching live selected actor in an already-active key remains eligible.
    **Focused validation:** Add narrow Development regressions for the pre-activated-key and matching-live-actor cases, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario LynxLeap -WorldSeed 445`, `-Scenario PolarBearCharge -WorldSeed 445`, `-Scenario HornedGoatPush -WorldSeed 445`, and `-Scenario ThunderSentinelSlam -WorldSeed 445` after an Editor build.
  - [ ] **RF252 — Require selected-attack outcome attribution for Lynx, Polar Bear, Horned Goat, Thunder Sentinel, and Reef Maw peer acceptance (P3; CR03-001b-45/49/53/57/61).**
    **Source:** CR18-002, CR03-001b-45/49/53/57/61; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:3412-3551,3779-3927,4118-4275,4582-4628,4933-4978,8308-8621`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1734-1867,1920-1925,1957-2194,2362-2448,2479-2511,2558-2666,2715-2720,2777-2956`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:320-359`; `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:61-69`; `Scripts/Tests/Combat/EnemyPeers.ps1:86-129,169-209`; `docs/07-development-setup.md:2744-2759,2769-2795`.
    **Affected path:** The non-Shipping LynxLeap, PolarBearCharge, HornedGoatPush, ThunderSentinelSlam, and ReefMawBite fixtures' server-side success predicates.
    **Trigger:** A fixture activates a wildlife key, the selected actor emits enough cues and movement evidence to reach Recovery but its attack misses, and another admitted wildlife actor supplies the host damage or other aggregate outcomes used for acceptance. HornedGoatPush additionally measures host-wide displacement and stagger without associating either with the selected Goat.
    **Evidence:** The Lynx fixture captures its host-health baseline before staging and activation; the Polar Bear fixture sets its baseline to 300 before activation. New-key activation materializes the admitted wildlife descriptors for the whole 6,400 cm key rather than isolating the selected target. Those fixtures latch the selected actor's cues but accept any positive total host-health loss; Lynx enters Recovery after both a hit and a miss, and Polar Bear proceeds to Recovery after its charge distance/time expires even if `TryApplyPolarBearCharge` returned false. HornedGoatPush likewise activates the key with `ActivatePopulationKey(..., true)`; stage two latches Telegraph/Recovery from only the selected Goat but samples aggregate host health, total 2D displacement, and the host-wide stagger flag. `UpdateHornedGoatAttack` ignores the return from `TryApplyHornedGoatCharge` and publishes Recovery after Telegraph even when the selected hit is rejected. The fixture succeeds on any positive health delta, at least 50 cm displacement and a 0.65–0.95 s stagger; the runner also checks the selected SpawnId and its telegraph on both clients but does not associate damage or effects with that attacker. Another admitted wildlife actor can therefore supply the measured host outcomes while the selected attack misses.  For ThunderSentinelSlam, key activation materializes the selected key's admitted wildlife descriptors; UpdateThunderSentinelAttack publishes Lunge and ignores the result of TryApplyThunderSentinelSlam. The peer fixture latches that selected Sentinel's cues and step, but measures only total host-health loss and accepts 299–301 damage. The runner pairs the selected SpawnId's cues with aggregate damage without a source-specific receipt. Another actor, including a second Sentinel possible under RF254's cap bypass, can therefore supply the measured host damage while the selected Sentinel misses. For ReefMawBite, stage two latches the selected SpawnId's Telegraph/Lunge/Recovery cues and stores its distance on first observed Lunge, but accepts only aggregate 50-point host-health loss. UpdateReefMawAttack ignores TryApplyReefMawBite's return and proceeds to Recovery after a miss; the runner matches selected-SpawnId client cues with aggregate damage and has no source-specific receipt. Verification materialization creates all admitted descriptors in the whole key, and RF254 documents that it can bypass Reef Maw's one-per-96-m cap. Another admitted Maw that reaches the host can therefore supply measured damage while the selected Maw supplies cues. No specific seed or live false pass was established.
    **Impact:** Any of the five Development fixtures can report success for the selected attack when another wildlife actor supplied the aggregate host outcome. No production combat defect was established.
    **Relationship to existing findings:** RF251 is the distinct false-failure case where activation of an already-active key does not materialize the selected target; RF254 concerns a separate area-cap bypass that can allow a second Sentinel or Reef Maw. RF250 concerns capsule placement and RF253 observer-distance assertions; RF255 concerns runtime resistance-profile acceptance. RF256 concerns Recovery duration. This finding concerns false success from host-wide outcomes paired with selected-actor cues.
    **Proposed repair:** Gate acceptance on a server-side hit receipt keyed to the selected attacker and Host, and associate the expected damage, displacement and stagger with that hit; alternatively isolate the host from every other wildlife source during the acceptance window. Keep aggregate health, movement, stagger and replicated cue observations as supporting checks.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** In all five fixtures, a selected attack miss plus another actor supplying the measured host outcomes cannot report Passed. A selected Lynx leap, Polar Bear charge, Horned Goat push, Thunder Sentinel slam, or Reef Maw bite with server-confirmed applied damage and its required movement/effects, cue evidence, distance gate and existing peer observations can pass.
    **Focused validation:** Add Development regressions for each fixture where the selected attack misses while another admitted wildlife source damages the host (and, for HornedGoatPush, supplies displacement/stagger), and verify each fixture fails. Add corresponding selected-attack hit cases. Then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario LynxLeap -WorldSeed 445`, `-Scenario PolarBearCharge -WorldSeed 445`, `-Scenario HornedGoatPush -WorldSeed 445`, `-Scenario ThunderSentinelSlam -WorldSeed 445`, and `-Scenario ReefMawBite -WorldSeed 445` after an Editor build.
  - [ ] **RF253 — Assert Horned Goat and Reef Maw observer placement before peer acceptance (P3; CR03-001b-52/60).**
    **Source:** CR18-002, CR03-001b-52/60; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4177-4219,4764-4779,4859-4915`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:5287-5304`; `Scripts/Tests/Combat/EnemyPeers.ps1:116-129,190-209`; `docs/07-development-setup.md:2744-2759,2780-2793`.
    **Affected path:** HornedGoatPush and ReefMawBite stage-one remote-observer placement and replicated-cue acceptance.
    **Trigger:** One or both observers are not at their intended fixture locations, or a Reef Maw observer is not in the selected deep-Ocean lane, but can still receive the selected actor's replicated cue.
    **Evidence:** HornedGoatPush calls `PlaceOnTerrain` and marks observers placed without reading their final positions; the runner checks selected-Goat cues but not observer distances. ReefMawBite selects deep-Ocean lane positions for the host and observers, calls `PlaceSwimmingCharacter`, and later checks only that each observer is unattached and has `IsSwimmingInGeneratedOcean()` movement mode. That helper teleports and sets the movement mode, while the mode predicate reports only the movement enum; stage one does not check the final observer coordinates or resample Ocean depth/lake/path. The server log records only `Observers=2`. The client `OnRep_ReefMawAttackCue` logs the selected SpawnId's cue without checking observer location, and the composite runner checks these logs and host distance but no observer positions or depth. Thus both scenarios can pass without proving the intended observer placement or, for Reef Maw, the selected deep-Ocean lane. No specific seed or runtime false pass was established.
    **Impact:** HornedGoatPush can pass without proving its documented 10 m and 15 m observer spacing, and ReefMawBite can pass without proving both observers occupied the intended deep-Ocean positions when they received the cues. No production replication defect was established.
    **Relationship to existing findings:** RF250 concerns capsule clearance at fixture placements; RF251 concerns already-active-key target materialization; RF252 concerns attack outcome attribution; RF254 concerns species-area caps. RF253 concerns peer-observer placement and cue acceptance.
    **Proposed repair:** After placement, record server-authoritative final observer positions. For HornedGoatPush, require the documented 1000 cm and 1500 cm distances within tolerance. For ReefMawBite, verify each observer remains at its selected Ocean lane position and the actual position passes the deep non-lake Ocean predicate. Have the composite runner assert these scenario-specific measurements in addition to matching each selected SpawnId's cues.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** Stage two is not armed and the scenario cannot pass unless the Goat peers meet their documented 10 m and 15 m observer bands and both Reef Maw peers remain at their assigned deep-Ocean lane positions. The two remote logs in each scenario must still contain the selected actor's cues, and the Goat host remains within the existing 180 cm reach check.
    **Focused validation:** Add Development regressions that move an observer outside its Goat slot or Reef Maw lane while keeping it close enough to receive the cue; each runner must reject the case. Retain intended placements, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario HornedGoatPush -WorldSeed 445` and `-Scenario ReefMawBite -WorldSeed 445` after an Editor build.
  - [ ] **RF254 — Respect Reef Maw and Thunder Sentinel area caps in peer-fixture admission (P3; CR03-001b-55/56/59).**
    **Source:** CR18-002, CR03-001b-55/56/59; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4392-4419,4458-4509,4711-4810,6867-6890,6971-7001,7182-7184,7534-7539,8308-8313,8366-8458,8618-8620`; `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:451-468`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationAreaCapsTest.cpp:52-55`; `docs/monster-spawning.md:230-244`; `docs/07-development-setup.md:2764-2768,2780-2790`; `Scripts/Tests/Combat/EnemyPeers.ps1:169-170,190-192`.
    **Affected path:** The non-Shipping ThunderSentinelSlam and ReefMawBite descriptor selectors and their verification-only population-key materialization.
    **Trigger:** A generated Sentinel or Reef Maw candidate otherwise passes its biome/descriptor, start-distance, defeat, per-key actor-budget and fixture-specific terrain/lane checks while another living or reserved same-species actor is already within the documented 96 m species-cap radius.
    **Evidence:** Both selectors track the candidate key's `WildlifeActorBudget` but do not call `DoesPopulationDescriptorPassCaps` or count nearby live/pending actors of the selected species. The Reef Maw selector uses the full Ocean descriptor list, applies the hard-enemy start exclusion and undefeated filter, then accepts a descriptor with a safe sampled Ocean lane; it does not check the Reef Maw area cap. Normal admission calls the cap gate, whose area-cap value is one for both Thunder Sentinel and Reef Maw and whose count includes live actors and pending reservations within 96 m. For a new key, `ActivatePopulationKey(key, true)` directly materializes the verification descriptors using the key-local budget without that area-cap gate. Thus either fixture can stage and accept an additional same-species actor even when ordinary admission rejects it. No specific seed or runtime over-cap case was established.
    **Impact:** ThunderSentinelSlam or ReefMawBite can report peer acceptance with an actor that normal population admission would reject, so the fixture does not prove the documented one-per-species area cap. No production population-cap defect was established.
    **Relationship to existing findings:** RF245 and RF248 concern descriptor-prefix and per-key actor-budget mismatches; this finding concerns the distinct live/pending species-area cap. RF251 concerns false failure after an already-active key makes direct activation a no-op; RF250 concerns pawn capsule placement.
    **Proposed repair:** Apply the normal species-area admission check to both fixture selectors and ensure verification-only materialization cannot bypass it for the selected actor. If the cap is full, continue the bounded search or fail before staging rather than spawning an over-cap target.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** With a living or reserved same-species actor inside a candidate's 96 m area, neither fixture materializes a second actor or reports Passed using one. Each either selects a candidate whose normal area-cap check passes or fails clearly before stage one. Under-cap cases retain the documented generated peer acceptance.
    **Focused validation:** Add Development regressions with an existing live Sentinel and Reef Maw inside 96 m of generated candidates and verify neither fixture materializes or accepts a second same-species actor; also retain under-cap passing cases. Then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario ThunderSentinelSlam -WorldSeed 445` and `-Scenario ReefMawBite -WorldSeed 445` after an Editor build.
  - [ ] **RF255 — Assert spawned Reef Maw and Thunder Sentinel resistance profiles in peer acceptance (P3; CR03-001b-56/60).**
    **Source:** CR18-002, CR03-001b-56/60; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4511-4515,4843-4846`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:161-163,297-300,326-330,689-697`; `Source/KalmalaGameplay/Public/KalmalaResistanceComponent.h:10-21`; `Source/KalmalaGameplay/Private/KalmalaResistanceComponent.cpp:9-13`; `docs/07-development-setup.md:2765-2773,2788-2793`; `Scripts/Tests/Combat/EnemyPeers.ps1:169-186,190-209`.
    **Affected path:** ThunderSentinelSlam's stage-one 75-Armor check and ReefMawBite's stage-one 2-Armor check for their materialized wildlife actors.
    **Trigger:** The archetype Armor mapping remains at the expected 75 or 2 while the selected actor's `UKalmalaResistanceComponent::Profile.Armor` is absent or differs because runtime component setup regressed.
    **Evidence:** Both fixtures check the selected actor's archetype and health, but their Armor conditions call static `GetArmorForArchetype(Target->GetArchetype())` instead of reading that actor's resistance component. Each scenario measures damage dealt by the wildlife actor to the host; neither applies a hit to the selected actor, so the outcome does not exercise its own Armor. Spawn initialization separately assigns the mapping into `Defenses->Profile.Armor`, and server damage resolution consumes the component profile. The component is a default subobject with a public profile, so the actual runtime value is directly checkable. A regression in component creation or assignment can leave the mapping at the documented value while either fixture's Armor check passes. The current spawn initialization makes the assignment; no live failure or production defect was established.
    **Impact:** Either Development peer case can pass without proving its generated wildlife actor received the documented runtime Armor profile. No current gameplay defect was established.
    **Relationship to existing findings:** RF250 concerns fixture pawn placement; RF251 and RF254 concern key activation and population-cap fidelity; RF253 concerns observer placement. RF255 concerns runtime resistance-profile acceptance.
    **Proposed repair:** Check the materialized target's resistance-component profile and fail before stage two if the component is missing or Armor differs from the expected archetype value: 75 for Thunder Sentinel or 2 for Reef Maw. Where practical, also exercise server-side resolution with a narrow known-damage profile check.
    **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
    **Acceptance:** A generated Sentinel or Reef Maw whose archetype mapping is correct but whose runtime resistance profile is missing or differs from its expected Armor value cannot arm its peer fixture; initialized 75-Armor and 2-Armor targets remain eligible.
    **Focused validation:** Add Development regressions that change or omit runtime component Armor while leaving each archetype mapping correct and verify both fixtures reject the mismatch; retain initialized-profile cases, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario ThunderSentinelSlam -WorldSeed 445` and `-Scenario ReefMawBite -WorldSeed 445` after an Editor build.
  - [ ] **RF256 — Assert the Thunder Sentinel's documented recovery duration in peer acceptance (P3; CR03-001b-57).**
    **Source:** CR18-002, CR03-001b-57; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4592-4612,4617-4624; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:2633-2666; Source/KalmalaGameplay/Public/KalmalaWildlifeSpawn.h:256; docs/07-development-setup.md:2771-2773; Scripts/Tests/Combat/EnemyPeers.ps1:174-186.
    **Affected path:** ThunderSentinelSlam's server-side Recovery cue acceptance and composite peer predicate.
    **Trigger:** The selected Sentinel exposes Recovery for at least one sampled frame but exits it before the documented 1.2 seconds.
    **Evidence:** Stage two only latches bThunderSentinelPeerTestSawRecovery when the cue equals Recovery; success requires that boolean, and the server log emits Recovery=1. The composite runner requires that flag but reads no duration. Runtime behavior currently assigns ThunderSentinelRecoverySeconds (1.2 seconds) on entry and clears Recovery when its timer expires, but the fixture never measures entry-to-exit time. A shortened duration lasting at least one sample can therefore satisfy the acceptance check. No current runtime duration defect was established.
    **Impact:** The generated Sentinel peer case can pass after a Recovery shorter than the documented duration. No current gameplay defect was established.
    **Relationship to existing findings:** RF252 concerns attribution of the selected Sentinel's damage; RF255 concerns whether the materialized actor has the expected runtime Armor profile. RF256 concerns only the temporal duration of its Recovery cue.
    **Proposed repair:** Measure the server-observed Recovery interval or authoritative timer, include it in the success log and require it to meet the documented 1.2-second duration with an explicit sampling tolerance.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** The fixture cannot report Passed when the selected Sentinel remains in Recovery for less than the documented duration, while the authored 1.2-second Recovery passes within documented sampling tolerance.
    **Focused validation:** Add a Development regression with Recovery shortened below 1.2 seconds and verify the fixture rejects it; retain the authored-duration pass and run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario ThunderSentinelSlam -WorldSeed 445 after an Editor build.
  - [ ] **RF257 — Exercise normal interaction paths in the Discovery, Ocean Discovery, and M9 peer fixtures (P3; CR03-001b-62/64, CR18-002 batch 104).**
    **Source:** CR18-002, CR03-001b-62/64 and batch 104; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4980-5091,5207-5268,10085-10153`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1576-1598,1687-1716,1968-1972`; `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:142-161`; `Source/KalmalaGameplay/Private/KalmalaOceanDiscoveryDisembarkPeerTest.cpp:424,469-472,790-791`; `Scripts/Tests/Progression/DiscoveryPeer.ps1:8-35`; `Scripts/Tests/Progression/ExplorationRewards.ps1:12-42`; `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1`; `docs/07-development-setup.md:2495-2508,4164-4180,4361-4391`.
    **Affected path:** DiscoveryPeer, Ocean discovery disembark/reconnect, and ExplorationRewards peer-fixture distant rejection, entitled claim, duplicate/replay claim and peer acceptance.
    **Trigger:** The ordinary owning-pawn `ServerRequestInteract` trace or discovery-actor dispatch stops reaching world, Ocean, or M9 discovery actors while direct server calls still succeed.
    **Evidence:** `DriveDiscoveryPeerTest` checks distance with direct `CanInteract_Implementation(Remote)` and `ClaimDiscovery(Remote, Descriptor)` calls, then invokes `Actor->Interact_Implementation(Entitled)` directly for the first and duplicate claims. In the M9 fixture, `bDistantRejected` combines `!Actor->CanInteract_Implementation(Remote) && !ClaimM9ExplorationReward(Remote, Descriptor)`; when the expected actor range check rejects the 1,000 cm Remote, C++ short-circuiting skips the GameMode claim call. M9 accepted/replay interactions also call `Interact_Implementation` directly; forged and replay helper checks call `ClaimM9ExplorationReward` directly. In the Ocean peer fixture, replay, both seated owners' accepted claims, and the integrated-journey claim call `GameMode->ClaimOceanDiscovery` directly; neither the fixture nor its runner sends an ordinary pawn interaction request. `AKalmalaDiscoveryActor::Interact_Implementation` does dispatch an Ocean descriptor to that handler, and normal play reaches the actor through `RequestInteract` -> `ServerRequestInteract` -> server trace -> `Execute_CanInteract`/`Execute_Interact`. The setup guide explicitly says focused checks do not replace a live client interaction trace in the later journey acceptance. No current production interaction failure was established.
    **Impact:** None of the three peer scenarios proves that a player can claim through the ordinary networked interaction route; a routing or server-trace regression could escape while direct handler or actor calls continue to succeed. No production defect was established.
    **Relationship to existing findings:** RF02 separately covers the missing Remote-client observation of the nonlocal Host's private feedback and reward inventory. Current fields remain owner-only; this finding concerns request/target dispatch. RF265 separately covers Ocean actor refresh materialization and cleanup, and RF28 covers the Ocean claim transaction rollback path. The M9 distant expression also does not independently exercise the GameMode claim method's distance guard, but the actor's own range rejection is expected and remains server-authoritative.
    **Proposed repair:** Have all three fixtures submit ordinary payload-free interaction intents through the owning pawn's `ServerRequestInteract` path and retain server-side trace/target selection. Keep direct descriptor-forgery and duplicate/replay helper checks as supplemental assertions; separate the M9 helper-distance assertion so short-circuiting cannot skip it. Keep independent privacy assertions.
    **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
    **Acceptance:** DiscoveryPeer, Ocean discovery, and ExplorationRewards cannot pass unless owning players send ordinary interaction requests without a target, discovery ID, reward or identity payload, the server selects actors through normal trace/dispatch, and accepted, distant-rejected and duplicate/replay outcomes are observed. Both Ocean owners receive exactly one owner-scoped reward through interaction; the M9 direct-claim distance check is independently exercised, and the privacy check remains independently required.
    **Focused validation:** Add a Development regression that disables or bypasses normal discovery dispatch and verify all three peer scenarios fail; then run `Scripts/Tests/Progression/DiscoveryPeer.ps1`, `Scripts/Tests/Progression/ExplorationRewards.ps1`, and the Ocean discovery/reconnect journey after an Editor build.  - [ ] **RF258 — Assert the Y tie-break in due population-key ordering (P3; CR03-001b-68).**
    **Source:** CR18-002, CR03-001b-68; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5364-5425,5469-5488`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerCadenceTest.cpp:26-29,91-100`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerBudgetTest.cpp:22-60`; `docs/monster-spawning.md:145-165`.
    **Affected path:** S06 lexicographic due-key ordering and S07 selection of at most four keys for each scheduler wake.
    **Trigger:** Multiple simultaneously due active keys share X but have different Y values, and insertion or hash iteration order differs from ascending Y order.
    **Evidence:** `GetDuePopulationSpatialKeys` currently compares X and then Y, so the implementation provides the documented lexicographic order. The cadence fixture adds loaded keys `(1,0)` then `(0,0)`, but `GetPopulationSpatialKeyCandidates` sorts the candidate union before schedule update; its due assertion only distinguishes X values. The scheduler-budget test uses six `(Index,0)` keys and also checks X order. No focused automation supplies same-X keys with different Y values. If the Y tie-break is removed, both tests can continue to pass while equivalent-X keys are left in TSet-derived order. With more due keys than the four-key wake cap, that can change which keys are serviced. No current production defect was established.
    **Impact:** The focused scheduler checks can miss a regression that stops honoring the Y tie-break for keys sharing X, allowing the bounded wake to choose a different set of due keys.
    **Proposed repair:** Extend the cadence or scheduler-budget automation with simultaneously due keys sharing X and having distinct Y values, insert them in reverse Y order, and assert ascending-Y output and the stable bounded wake selection.
    **Dependencies:** After CR19 review acceptance; no scheduler test or runtime behavior change during CR03 or CR18.
    **Acceptance:** A focused Development regression fails when the Y tie-break is removed or reversed, passes with same-X keys inserted in an order different from lexicographic Y order, and preserves the existing X-order and wake-budget assertions.
    **Focused validation:** Run `Kalmala.Gameplay.Population.SchedulerCadence` and `Kalmala.Gameplay.Population.SchedulerWakeBudget` after an Editor build.
  - [ ] **RF259 — Cover scheduler wake attempt-gate-to-transaction integration (P3; CR03-001b-70, CR03-001b-74).**
    **Source:** CR18-002, CR03-001b-70, CR03-001b-74; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5510-5575,5793-5818`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerBudgetTest.cpp:8-69`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRingCandidateSelectorTest.cpp:12-175`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPlayerDistanceGateTest.cpp:7-123`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSpawnGateEvaluatorTest.cpp:7-178`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationCreationLifecycleTest.cpp:12-118`; `docs/monster-spawning.md:163-166,270-281`.
    **Affected path:** A due natural-population key through the bounded server wake, descriptor selection, ordered attempt gates and successful server creation transaction.
    **Trigger:** A regression disconnects or misorders an attempt gate, omits ring membership from the per-attempt distance result, or stops submitting a fully eligible scheduled attempt to the transaction, while scheduler-budget, gate-helper and direct transaction rejection tests continue to pass.
    **Evidence:** The current wake builds descriptors, selects bounded candidates, applies player-distance, habitat, time, weather, progression, cap and physical-placement gates, then submits attempts whose rejection reason remains `None`. The scheduler-budget automation checks work counts, key and candidate limits, ordering and hitch behavior; the ring-selector automation checks candidate/ordinal behavior; the gate-evaluator automation checks helper ordering; and the creation-lifecycle automation calls `TryCreatePopulationSpawnTransaction` only with default failing gates before manually creating its lifecycle fixtures. No focused test drives one fully eligible attempt from the normal scheduler wake through all gates and verifies successful actor creation/registration. Existing RF12 covers transaction internals and RF13 covers Tick-to-scheduler wiring; neither covers this wake-to-transaction handoff. The selector fixture retains an accepted-roll descriptor outside the ring, but no test passes it through the distance gate and asserts `PlayerDistance`; because wake dispatches by rejection reason, removing the ring conjunct could leave it without distance rejection and eligible for dispatch if later gates pass. No production defect was established.
    **Impact:** A wiring regression could leave eligible natural population attempts uncreated while the scheduler, gate and transaction helper checks still pass.
    **Relationship to existing findings:** RF12 covers successful transaction internals and partial Wolf rollback; RF13 covers real GameMode Tick sequence. RF259 covers the distinct bounded-wake attempt-gate-to-transaction integration.
    **Proposed repair:** Add a Development integration fixture that drives a deterministic eligible descriptor through the scheduler wake and all ordered gates, then verifies the created actor and live registration. Include a deterministic accepted-roll, out-of-ring descriptor retained by selection; verify the distance gate records `PlayerDistance` and the wake creates no actor for that attempt. Force one other gate to reject in a companion case and verify no actor is created.
    **Dependencies:** After CR19 review acceptance; no scheduler pipeline or runtime behavior change during CR03 or CR18.
    **Acceptance:** A fully eligible due attempt reaches successful actor creation and registration through `UpdatePopulationSchedulerWake`; the retained accepted-roll, out-of-ring attempt receives `PlayerDistance` and creates no actor; a forced gate rejection creates no actor; existing wake and candidate budgets remain respected.
    **Focused validation:** Run the new scheduler-admission integration regression together with `Kalmala.Gameplay.Population.SchedulerWakeBudget` after an Editor build.
  - [ ] **RF260 — Verify horizontal spawn-ring behavior across elevation (P3; CR03-001b-72).**
    **Source:** CR18-002, CR03-001b-72; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5698-5736`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRingCandidateSelectorTest.cpp:24-67,83-91`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPlayerDistanceGateTest.cpp:44-54,79-116`; `docs/monster-spawning.md:168-190`.
    **Affected path:** S08 inclusive horizontal 32–96 m population ring membership for wildlife descriptors.
    **Trigger:** A future change computes ring membership in 3D while the player and candidate share an elevation in existing tests, so all current boundary and multi-player assertions still pass.
    **Evidence:** IsDescriptorInSpawnRing currently computes squared distance from X/Y only, matching S08. The ring-selector fixture places its player and every candidate at Z=0; the player-distance fixture likewise uses Z=0 for both players and candidates. Neither test would detect a 3D-distance regression. No production defect was established.
    **Impact:** Ring membership could depend on terrain elevation, incorrectly admitting a horizontally too-close descriptor or rejecting an in-ring descriptor at a different height. No current production defect was found.
    **Relationship to existing findings:** RF259 covers scheduler wake-to-transaction integration; this finding covers the separate horizontal-versus-3D ring predicate contract.
    **Proposed repair:** Extend the Development ring-selector regression with same-world candidates whose horizontal distances are inside and outside the ring while their vertical offsets would move their 3D distances across the ring thresholds.
    **Dependencies:** After CR19 review acceptance; no ring behavior or test changes during CR03 or CR18.
    **Acceptance:** The regression fails if Z contributes to ring distance; an in-ring horizontal candidate remains selected despite a large vertical offset, and a horizontally too-close candidate remains excluded despite a vertical offset that would put its 3D distance above 32 m. Existing inclusive 32 m and 96 m boundary assertions remain intact.
    **Focused validation:** Run `Kalmala.Gameplay.Population.RingCandidateSelector` and `Kalmala.Gameplay.Population.PlayerDistanceGate` after an Editor build.
  - [ ] **RF261 — Verify horizontal player exclusion across elevation (P3; CR03-001b-75).**
    **Source:** CR18-002, CR03-001b-75; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5820-5854`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPlayerDistanceGateTest.cpp:44-54,79-123`; `docs/monster-spawning.md:183-190`.
    **Affected path:** S09's strict 36 m horizontal exclusion against every current-world living player, composed with S08 ring membership.
    **Trigger:** A regression changes `IsDescriptorOutsidePlayerExclusion` to 3D distance while player and candidate Z values remain equal in the existing player-distance fixture.
    **Evidence:** The production predicate correctly compares X/Y only, as required by S09. The focused fixture places both players and all descriptors at Z=0; its 35 m rejection uses a candidate at `(6500,0,0)` and a second player at `(10000,0,0)`, while the first player at `(0,0,0)` keeps the candidate inside S08's 32–96 m horizontal ring. A 3D-distance regression would still reject this fixture. No production defect was found.
    **Impact:** A 3D-distance regression could admit a wildlife spawn less than 36 m from a player horizontally when their vertical separation raises 3D distance above the threshold. The current implementation remains horizontal.
    **Relationship to existing findings:** RF260 covers elevation-independent S08 ring membership at the 32 m and 96 m thresholds; RF261 covers the separate S09 all-player 36 m exclusion.
    **Proposed repair:** Extend `Kalmala.Gameplay.Population.PlayerDistanceGate` with a descriptor 35 m horizontally from the second player and a vertical offset large enough to exceed 36 m in 3D, while remaining in the horizontal S08 ring from the first player. Assert that S09 still rejects it and records `PlayerDistance`.
    **Dependencies:** After CR19 review acceptance; no distance behavior or test changes during CR03 or CR18.
    **Acceptance:** The focused test fails if Z contributes to S09 exclusion; the elevated 35 m horizontal candidate is rejected with `PlayerDistance`, an exactly 36 m horizontal candidate still passes, and S08 ring membership remains a separate horizontal condition.
    **Focused validation:** Run `Kalmala.Gameplay.Population.PlayerDistanceGate` after an Editor build.
  - [ ] **RF262 — Cover species-habitat rejection and hard-enemy classification through GameMode (P3; CR03-001b-76/77).**
    **Source:** CR18-002, CR03-001b-76/77; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:169-185,5856-5898`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:460-467`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRingCandidateSelectorTest.cpp:132-150`; `Source/KalmalaGameplay/Private/Tests/KalmalaEnvironmentalSpawnerAdmissionTest.cpp:178-191`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSpawnGateEvaluatorTest.cpp:34-57`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeGiantSnakeTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMoonBearTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePolarBearTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeThunderSentinelTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeReefMawTest.cpp:21-44,97-99`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeThunderSentinelTest.cpp:100-106`; `docs/monster-spawning.md:278-284`; `docs/biome-enemies.md:304-317,347-381`.
    **Affected path:** Seed/biome-derived archetype mapping, hard-enemy classification, generated-start distance evaluation, ordered `SpeciesHabitat` rejection, and candidate filtering through `ApplyPopulationSpeciesHabitatGate`.
    **Trigger:** A regression omits one hard-enemy archetype from the GameMode classifier or bypasses/hardcodes the population species-habitat result while the generic gate-order test, species-selection fixtures and direct boolean-policy helper tests continue to pass.
    **Evidence:** The production predicate correctly maps each descriptor from the generation configuration, classifies Giant Snake, Moon Bear, Peat Husk, Polar Bear, Thunder Sentinel and Reef Maw as hard enemies, measures horizontal distance from the generated start, and delegates the finite inclusive 15000 cm boundary to the shared helper. The generic evaluator uses synthetic booleans; species fixtures test seed/biome selection but do not exercise the GameMode classifier; the configured-source case asserts only a passing result before cap rejection; and the ring-selector fixture reaches this wrapper with an empty candidate list while preserving an earlier roll failure. The two focused helper tests pass `bHardEnemy` as true explicitly, so they do not prove the GameMode classifies every hard species. No production defect was found.
    **Impact:** A classifier or wrapper wiring regression could admit a hard enemy within the documented 150 m generated-start exclusion. The current predicate and classifier remain correct.
    **Relationship to existing findings:** RF259 covers the broader scheduler-to-transaction path; RF261 covers the separate S09 player-distance exclusion. RF262 combines the focused GameMode classifier and population-result gaps because one data-driven wrapper fixture can cover both.
    **Proposed repair:** Add focused Development automation with deterministic seed/biome descriptors for each of the six hard-enemy archetypes and a generated start. Drive each descriptor through `ApplyPopulationSpeciesHabitatGate` and assert that each inside-150 m attempt receives `SpeciesHabitat` and is removed from candidates. Retain a passing ordinary-species case, an exact-150 m hard-enemy boundary case, and a companion earlier-failure attempt to prove first-rejection preservation.
    **Dependencies:** After CR19 review acceptance; no gate behavior or test changes during CR03 or CR18.
    **Acceptance:** The focused regression detects both a bypassed/hardcoded population species-habitat result and omission of any of the six hard archetypes from classification; each near-start hard enemy is rejected through the GameMode wrapper and filtered from candidates. It retains ordinary-species admission, the inclusive 150 m boundary, first-rejection ordering, and existing direct helper assertions.
    **Focused validation:** Run the new `Kalmala.Gameplay.Population.SpeciesHabitatGate` automation after an Editor build.
  - [ ] **RF263 — Cover horizontal circle semantics in population caps (P3; CR03-001b-85).**
    **Source:** CR18-002, CR03-001b-85; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6929-6941,6947-6965`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationAreaCapsTest.cpp:65-92`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationWolfReservationTest.cpp:43-89`; `docs/monster-spawning.md:230-233`.
    **Affected path:** S11 same-species area-cap counts for live actor positions and pending reservations within the inclusive 96 m horizontal radius.
    **Trigger:** A regression includes Z in cap distance or substitutes an axis-aligned radius while current actor, reservation and candidate fixtures remain level and near an axis.
    **Evidence:** Both production loops correctly use finite X/Y and compare `DeltaX * DeltaX + DeltaY * DeltaY` to the squared radius. The area-cap fixture checks current movement and the exact/outside radius only along X; its other live actors remain at Z=0. The Wolf reservation fixture uses Z=0 and axis-aligned pack members. No focused cap test places a same-XY actor or reservation at a different elevation or checks a diagonal point outside the horizontal circle but inside its bounding square. The existing tests would therefore miss a 3D-distance or square-distance regression. No production defect was found.
    **Impact:** A 3D-distance regression could stop charging an elevated same-species actor or reservation and admit excess wildlife in the same horizontal area; a square-distance regression could charge a diagonal actor outside the documented circle and reject otherwise admissible wildlife.
    **Relationship to existing findings:** RF260 and RF261 cover elevation independence in the separate S08/S09 player-distance gates. RF254 concerns peer-fixture admission bypassing the species-area gate. RF263 concerns test coverage of S11's shared cap-count geometry.
    **Proposed repair:** Extend `Kalmala.Gameplay.Population.AreaCaps` and `Kalmala.Gameplay.Population.WolfPackReservations` with elevated live/reserved members at the candidate's same X/Y and diagonal members beyond the circular radius but within its square bounds. Assert that elevation does not remove cap charges and that out-of-circle diagonal members do not consume nearby species capacity.
    **Dependencies:** After CR19 review acceptance; no cap behavior or test changes during CR03 or CR18.
    **Acceptance:** Focused automation fails if Z contributes to S11 distance or the radius becomes axis-aligned, proves the same-XY elevated live and reserved members count, excludes the diagonal out-of-circle members, and retains existing boundary, movement, reservation and budget assertions.
    **Focused validation:** Run `Kalmala.Gameplay.Population.AreaCaps` and `Kalmala.Gameplay.Population.WolfPackReservations` after an Editor build.
  - [ ] **RF264 — Verify both seat references on the remote skiff replica (P3; CR03-001b-92).**
    **Source:** CR18-002, CR03-001b-92; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8194-8195,8255-8259`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:687-712`; `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:83-84`; `Scripts/Tests/Travel/OceanSkiffJourney.ps1:46-50`; `docs/02-technical-architecture.md:1226`; `docs/07-development-setup.md:2255-2264`.
    **Affected path:** Replication of both skiff occupant references and attached player pawns to the remote client's view during the two-peer ocean crossing.
    **Trigger:** A regression prevents the remote client from receiving the passenger occupant reference or the listen-host passenger's skiff attachment while the server state and locally controlled helm replica remain correct.
    **Evidence:** The skiff registers `HelmOccupant` and `PassengerOccupant` for replication, and the server journey fixture checks both occupants and attachments. The character probe runs only for locally controlled characters. `OceanSkiffJourney.ps1` requires the listen host's authoritative passenger travel plus the remote client's local helm attach/travel/stop, but does not assert that the remote client's skiff replica points to the host passenger or that the host passenger replica remains attached. The existing focused skiff test has no occupant-reference assertions. No production defect was established.
    **Impact:** A remote-only replication regression for passenger occupancy or attachment could pass the current crossing runner while that client presents incomplete two-seat state. Production currently registers both references for replication.
    **Relationship to existing findings:** RF59 concerns public blockage-detail replication; it does not cover the two seat references or passenger attachment. This finding is limited to remote-replica regression coverage.
    **Proposed repair:** Extend the non-shipping client journey probe and runner to assert that the remote client sees the server-selected helm and passenger occupant references and that the host passenger replica is attached to the same skiff during travel.
    **Dependencies:** After CR19 review acceptance; no journey fixture or replication changes during CR03 or CR18.
    **Acceptance:** The two-peer journey regression fails if either occupant reference or the host passenger attachment is missing on the remote client, while retaining server seat/attachment checks and the remote helm's attach, underway, and moored assertions.
    **Focused validation:** Run `Scripts/Tests/Travel/OceanSkiffJourney.ps1` after an Editor build.
  - [ ] **RF265 — Cover Ocean discovery refresh materialization and cleanup (P3; CR03-001b-98).**
    **Source:** CR18-002, CR03-001b-98; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7910,9314-9391`; `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:95-113,142-160`; `Source/KalmalaGameplay/Private/KalmalaOceanDiscoveryDisembarkPeerTest.cpp:260-275,706-791`; `Source/KalmalaGameplay/Private/Tests/KalmalaOceanDiscoveryClaimTest.cpp:24-80`; and `docs/04-roadmap.md:330`.
    **Affected path:** Server tick refresh of canonical Ocean discovery actors for the desired nearby spatial keys in the two-player M8 experience.
    **Trigger:** The refresh call, key selection, actor spawn/initialization, or stale-key cleanup regresses while the catalogue and direct claim helper continue to pass.
    **Evidence:** GameMode Tick calls `RefreshOceanDiscoveries`, which derives desired keys from player positions, removes actors for departed keys, builds canonical descriptors, and spawns initialized `AKalmalaDiscoveryActor` instances. The catalogue and claim contract tests cover deterministic descriptors and sparse-claim behavior. The Ocean peer fixture independently builds a descriptor through the catalogue and calls `ClaimOceanDiscovery` directly; it never asserts that refresh materialized or removed a world actor, so it can pass if the marker lifecycle fails. M8 acceptance requires players to find an optional sea discovery. No production defect was established. RF257 concerns ordinary interaction dispatch in the DiscoveryPeer and M9 fixtures, not Ocean actor refresh/materialization.
    **Impact:** A regression could leave Ocean discovery markers absent or stale in normal play while existing claim tests still pass.
    **Proposed repair:** Extend the Ocean GameMode peer fixture to observe the production tick-driven refresh, assert the expected canonical marker is materialized for an active player neighborhood and removed after the neighborhood is left, and retain its existing claim and disembark checks.
    **Dependencies:** After CR19 review acceptance; no refresh, actor, fixture, or runner changes during CR03 or CR18.
    **Acceptance:** The two-player seed-418 regression fails if the normal GameMode refresh does not materialize the expected canonical discovery marker for an active spatial key or fails to remove stale markers after both players leave that neighborhood. The existing descriptor, owner-claim, and disembark checks remain in place.
    **Focused validation:** Run the Ocean discovery catalogue and claim contract automations and `Scripts/Tests/Travel/OceanSkiffDiscoveryDisembark.ps1` after an Editor build.
  - [ ] **RF266 — Correct weather-transition screenshot cadence in evidence (P3; CR03-001b-107, CR18-002 batch 108).**
    **Source:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10715-10720,10778-10789`, `Scripts/Tests/World/BiomeBaselineCapture.ps1:11-15,30-43`, `Source/KalmalaUI/Private/KalmalaWeatherActivitySubsystem.cpp:194-202`, `docs/valheim-like-lighting-plan.md:1538`, and the L06e record in `PROGRESS.md`.
    **Affected path:** The fixed-view weather-transition evidence and its summary in the lighting plan.
    **Trigger:** Readers interpret the four onset/release images as samples taken every 0.5 seconds.
    **Impact:** The reported temporal evidence is denser and more regular than the fixture produced, overstating what the fixed images establish about transition smoothness.
    **Evidence:** The runner requires `TemporalFrames=1` for `WeatherTransition`; its 0.5-second interval is not used for that mode. GameMode schedules one-shot screenshots at 5.00, 11.25, 11.75, and 17.75 seconds. The local material-response logger runs from the subsystem update and emits only when the rounded response decile changes, not on a fixed 0.5-second interval. The lighting-plan sentence therefore mislabels the screenshot cadence.
    **Proposed repair:** Describe the four fixed screenshot times relative to fixture start and distinguish them from the variable response-change log samples. Keep conclusions about visible popping limited to those four selected views.
    **Dependencies:** After CR19 review acceptance; this evidence correction is a handoff only and does not authorize edits during CR03 or CR18.
    **Acceptance:** The lighting plan states the four scheduled screenshot times, explains that the default temporal interval does not govern `WeatherTransition`, and does not imply fixed-rate response telemetry.
    **Focused validation:** Compare the corrected evidence text with the GameMode one-shot timers, runner constraints, and available L06e records; if quantitative cadence claims remain, support them with timestamped samples at that cadence.
  - [ ] **RF267 — Exclude traversal, reconnect, and ocean-travel test hooks from Shipping builds (P2; CR03-001b-111/001c; CR18-002 batches 112/115).**
    **Source:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:631,636-640,669-696,11374-11647`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:450-482,759-887,889-950`; `Source/KalmalaGameplay/Private/KalmalaCharacterMovementComponent.cpp:118-126`; `docs/07-development-setup.md:2181-2183,2347-2353`.
    **Affected path:** Shipping listen/dedicated servers and player pawns launched with developer-only traversal, reconnect, or ocean-travel verification switches.
    **Trigger:** A Shipping process receives `-KalmalaTraversalTest`, any nonempty `-KalmalaReconnectVerification=...` value, or `-KalmalaOceanTravelTest`.
    **Evidence:** ConfigureTraversalTest and RunReconnectVerification parse and execute the traversal/reconnect switches without a Shipping compile guard. BeginPlay invokes traversal setup and spawns a verification pawn outside its `#if !UE_BUILD_SHIPPING` regions. Character BeginPlay/Tick derives the traversal target and applies autonomous host-pawn movement without a Shipping guard; RunReconnectVerification can persist a generated harvest to the world population save and calls RequestExit after verification or unknown-mode handling. Separately, Character BeginPlay parses `-KalmalaOceanTravelTest` and unconditionally changes each spawned character's `ECC_Pawn` and `ECC_WorldStatic` responses to Ignore. The GameMode fixture spawner is compiled out in Shipping, but that collision change still runs when the flag is present. The setup guide describes the ocean-travel fixture and the other switches as development verification.
    **Impact:** A stale launch option can steer a listen-host character, spawn a verification pawn, mutate a persistent world population record, and terminate a Shipping server. The ocean-travel flag can also disable pawn and generated-terrain collision for characters in a Shipping process.
    **Proposed repair:** Compile out or otherwise disable traversal, reconnect, and ocean-travel flag parsing, activation, and their movement, collision, save, and exit side effects in Shipping builds; keep the documented Development behavior.
    **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18.
    **Acceptance:** A Shipping launch with any of these flags follows normal gameplay, does not steer a pawn, change pawn/terrain collision, spawn a verification pawn, mutate a population save, or exit; documented Development traversal, reconnect, and ocean-travel checks remain functional.
    **Focused validation:** Build Development and Shipping targets. Run the documented traversal check, `Scripts/Tests/Survival/PersistedCampRestart.ps1`, and `Scripts/Tests/Travel/OceanTravel.ps1` in Development; launch Shipping with each flag using isolated user data and verify normal movement/collision, no test markers, no save mutation, and no forced exit.
  - [ ] **RF268 — Retry treasure preparation after an initial save failure (P3; CR03-001b-113).**
    **Source:** CR03-001b-113; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11972-11994`,
      `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:79-93,156-163`, and
      `docs/02-technical-architecture.md:3498`.
    **Affected path:** A9 treasure attached to a canonical point-of-interest actor when its first
      population-save write fails.
    **Trigger:** `PrepareWorldTreasure` returns null if its candidate state cannot be durably saved.
      `InitializeServer` sets `bWorldTreasure` only after a non-null result, then completes ordinary
      discovery setup. Later interaction calls `ClaimWorldTreasure` only when `bWorldTreasure` is true;
      no other path retries preparation for that initialized actor.
    **Impact:** A valid point of interest remains without its shared treasure while this actor remains
      initialized, even if storage recovers. This occurs before the claim-time feedback and rollback
      cases in RF36.
    **Evidence:** The only `PrepareWorldTreasure` call sites are discovery initialization and
      `ClaimWorldTreasure`; the latter is reached from the actor only behind the `bWorldTreasure` gate.
      The initialization write-failure branch returns null without setting that flag or scheduling a
      retry.
    **Proposed repair:** Keep initial preparation retryable after a failed population-save write, while
      exposing the chest and allowing claims only after the deterministic treasure record is durable.
    **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not
      authorize implementation during CR03 or CR18. RF36 remains the separate claim-time capacity and
      inventory-receipt handoff.
    **Acceptance:** Inject a failed first preparation write for an eligible canonical point of interest,
      then restore storage and retry preparation without restarting the server. The same deterministic
      treasure record is saved once, the chest becomes claimable only after that write, and no reroll or
      duplicate record occurs.
    **Focused validation:** Add a narrow discovery/GameMode write-failure retry test and run the A9
      treasure peer scenario after an Editor build.
  - [ ] **RF269 — Avoid weak object pointers as GameMode set elements (P3; CR03-002; CR18-002 batch 116).**
    **Source:** `Source/KalmalaGameplay/Public/KalmalaGameMode.h:573-574,590,627-628`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10821-10830,11665-11715`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:199-211`; [Epic UE 5.8 Object Pointers](https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine).
    **Affected path:** Development traversal tracking and the GameMode's pending-return and live-creature registries.
    **Trigger:** A controller, pawn, or wildlife actor becomes invalid while its `TWeakObjectPtr` remains a `TSet` element. In particular, the traversal started/completed sets are not cleared when a pawn logs out; `DriveTraversalTest` retains those entries for the life of the GameMode.
    **Evidence:** Unreal Engine 5.8 documents that `TWeakObjectPtr` is not supported as a `TSet` element. The UE 5.8 `FWeakObjectPtr` equality treats two pointers whose `Get()` returns null as equal regardless of their original objects, while its hash remains derived from their object index and serial number. The traversal sets add pawns and have no matching cleanup in `Logout`; repeated pawn destruction can leave stale keys in a container whose key equality/hash behavior is unsupported. The live-creature registry removes actors from `EndPlay`, and the pending-return sets remove controllers during `Logout`; those ordinary cleanup paths were not independently shown to fail.
    **Impact:** Repeated disconnect/respawn during a Development traversal run can retain stale tracking keys and make membership operations on invalid weak keys unsupported, reducing confidence in traversal-test tracking. No production gameplay failure was reproduced.
    **Proposed repair:** Use a supported stable object-key design for set membership and remove traversal entries at pawn logout, while keeping weak pointers only as validated values or transient references.
    **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18.
    **Acceptance:** A pawn destroyed during traversal leaves no stale tracking entry; a rejoining pawn is tracked independently. All GameMode weak-pointer registries use supported key semantics, with live-creature counts and listen-host return behavior preserved.
    **Focused validation:** Build Development and add a focused GameMode automation that destroys/logs out a tracked pawn, exercises cleanup and rejoin, and verifies independent membership and live-creature counts. Inspect each remaining container against the UE 5.8 object-pointer contract.
  - [ ] **RF270 — Bind generated harvest tool requests to the active Main Hand item (P3; CR04-010; CR18-002 batch 117).**
    **Source:** Source/KalmalaGameplay/Public/KalmalaCharacter.h:296-297; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1485-1552,1687-1716,2060-2068,2149-2219; Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:335-365; Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp:492-538; docs/02-technical-architecture.md:1464-1465.
    **Affected path:** Reliable ServerRequestInteract requests for generated non-tree, non-gemstone, non-hand-gatherable tool harvest sources.
    **Trigger:** An owning client submits the ID of a suitable carried tool while a different Main Hand item is active.
    **Evidence:** Normal local interaction sends the active Main Hand item as harvest intent, and the architecture contract says the active tool supplies that intent. The server independently traces its own view and selects the target, but forwards ClientItemId and ClientAction to the harvest node. Tool-gated sources call CommitToolHarvestFromServer, which looks up durability by the submitted ID; when that ID is not active, FindToolDurabilityFromServer searches the carried-tool list. Server validation derives the source and reward from the node and checks tool definition, action, source suitability, skill, durability, capacity, persistence, range and node availability, but neither the character commit path nor ApplyServerUse binds the submitted ID or instance GUID to Inventory->GetActiveItem() and GetActiveInstanceGuid(). A modified client can therefore use a stowed suitable tool.
    **Impact:** Harvesting can consume durability from and receive the server-selected yield for a stowed tool that the player did not select in Main Hand. The tool must still be carried, suitable, durable, and meet server skill checks; no arbitrary target or reward input was found.
    **Proposed repair:** Resolve the selected server-side Main Hand item and instance GUID for this interaction and reject a tool-harvest intent that does not match them before committing reward, durability, depletion or persistence.
    **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18. RF42 covers the separate repeated-interaction cadence gap.
    **Acceptance:** With a qualifying higher-tier tool stowed and a different item active in Main Hand, a request naming the stowed tool is rejected without changing inventory, tool durability, harvest state or saved state. A request using the active suitable tool still succeeds through the existing server transaction.
    **Focused validation:** Add a focused owning-client/listen-server scenario that submits a modified ServerRequestInteract payload naming a stowed tool. Verify no reward, durability, node-depletion or save change; then verify the active-tool path and its existing persistence rollback behavior.
  - [ ] **RF271 — Keep world-map verification hooks out of Shipping (P3; CR04-035; CR18-002 batch 129).**
    **Source:** `Source/KalmalaUI/Private/KalmalaWorldMapSubsystem.cpp:27-33`; `Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:213-263,876-880,900-905,1340-1362`; `Source/KalmalaGameplay/Private/KalmalaMapAwarenessComponent.cpp:77-118`; `Scripts/Tests/World/WorldMap.ps1:42-46`; `docs/07-development-setup.md:3450-3455`.
    **Affected path:** A Shipping game process with a local player and possessed pawn launched with map-verification flags.
    **Trigger:** `-KalmalaWorldMapVerification` automatically opens the map after local exploration is recorded. Adding `-KalmalaWorldMapAcceptanceEvidence` runs acceptance evidence that commits synthetic pins and submits a test ping. The documented Development runner uses these flags.
    **Evidence:** The subsystem and widget parse and execute these hooks without a Shipping guard. The evidence routine creates two pins through `CommitPinPlacement` and `PersistPins`, then calls the ordinary map-ping request. The ping still passes server authority, requester, finite/in-world position, and cooldown checks before acceptance and temporary relay.
    **Impact:** A Shipping client started with explicit verification flags can open test UI, persist synthetic pins in its local world map save, and submit a test ping that may be relayed to connected peers. No authority bypass is established.
    **Proposed repair:** Compile out or disable verification flag parsing, automatic test activation, and synthetic pin/ping injection in Shipping while preserving normal manual map, personal-pin, and ping behavior.
    **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18.
    **Acceptance:** Shipping with the verification/evidence flags does not open test UI, modify the personal pin save, or submit a test ping. Normal manual map, pin, and ping use remains functional, and the documented Development acceptance runner still passes.
    **Focused validation:** Build Development and Shipping. Run `Scripts/Tests/World/WorldMap.ps1 -AcceptanceEvidence` in Development; launch Shipping with isolated user data and each verification flag, verify no test activation, pin-save change, or peer ping, then smoke-test ordinary manual map and ping interactions.

  - [ ] **RF272 — Pin the approved Dodge Roll regression contract (P3; CR16-022).**
    `KalmalaDodgeRollTest.cpp` feeds the current cost constant into its own
    helper expectation; the peer gate does not check stamina payment or launch
    movement. Pin approved values and cover the server action's payment, launch
    and timing.
    Evidence and acceptance:
    [RF272 finding](Temporary/Review/findings.md#rf272-pin-the-approved-dodge-roll-contract-in-regression-tests-p3).

  - [ ] **RF273 — Exercise the server-only campfire relight action in the client-role test (P3; CR16-023).**
    The hearth client-role case calls `AdvanceFromServer`, not the explicit
    `TryLightFromServer` action; the static lighting-policy check does not cover
    actor authority or state mutation. Call the action with an eligible, paid,
    dry, extinguished campfire under `ROLE_SimulatedProxy` and assert rejection
    without state or fuel changes; retain the authority-role positive control.
    Evidence and acceptance:
    [RF273 finding](Temporary/Review/findings.md#rf273-exercise-the-server-only-campfire-relight-action-in-the-client-role-test-p3).

  - [ ] **RF274 — Pin the approved M9 exploration candidate rewards in the catalogue test (P3; CR16-024).**
    The reward-catalogue test checks its definitions against the same table it
    reads them from and asserts only uniqueness plus a general quantity bound.
    Pin the two architecture-approved candidate/presentation/reward mappings
    and quantities independently.
    Evidence and acceptance:
    [RF274 finding](Temporary/Review/findings.md#rf274-pin-the-approved-m9-exploration-candidate-rewards-in-the-catalogue-test-p3).

  - [ ] **RF275 — Pin the documented M9 source-loot hash input in regression tests (P3; CR16-024).**
    The source-loot test validates the FNV primitive and repeatability, but no
    expected output for a complete documented `m9-loot-v1` key. Add independent
    vectors that pin the domain tag, seed, revision, source and spawn identity.
    Evidence and acceptance:
    [RF275 finding](Temporary/Review/findings.md#rf275-pin-the-documented-m9-source-loot-hash-input-in-regression-tests-p3).

  - [ ] **RF276 — Compare full tool-progression outputs after rejection (P3; CR16-025).**
    The progression test snapshots candidate outputs but checks only array
    lengths after rejected requests. Compare all tool and inventory fields so
    same-length mutations fail.
    Evidence and acceptance:
    [RF276 finding](Temporary/Review/findings.md#rf276-compare-full-tool-progression-outputs-after-rejection-p3).

  - [ ] **RF277 — Test Workbench exclusion from camp-sphere suppression (P3; CR16-027).**
    The spherical-suppression test creates a Forge but labels the assertion as
    covering both Forge and Workbench. Add an initialized Workbench negative
    control for the S24 sphere and make the assertion diagnostic match its fixture.
    Evidence and acceptance:
    [RF277 finding](Temporary/Review/findings.md#rf277-test-workbench-exclusion-from-camp-sphere-suppression-p3).

  - [ ] **RF278 — Reconcile current storage architecture contracts (P2; CR16-028).**
    The architecture still calls the normal Chest the sole storage construction
    and schema 2 current, while later contracts and tests specify Reinforced Chest
    and schema 3. Evidence and acceptance:
    [RF278 finding](Temporary/Review/findings.md#rf278-reconcile-current-storage-architecture-contracts-p2).

  - [ ] **RF279 — Pin resource source families independently in the gate test (P3; CR16-028).**
    The expected family is derived from the source action returned by production,
    so misclassified source IDs can agree with the test oracle. Add independent
    expected actions for all twelve sources. Evidence and acceptance:
    [RF279 finding](Temporary/Review/findings.md#rf279-pin-resource-source-families-independently-in-the-gate-test-p3).

  - [ ] **RF280 — Verify rain pauses Standing Light fuel at the actor boundary (P3; CR16-028).**
    Current tests cover fuel arithmetic and rain-based flame eligibility separately,
    but not whether a rainy actor tick leaves live and saved fuel unchanged.
    Evidence and acceptance:
    [RF280 finding](Temporary/Review/findings.md#rf280-verify-rain-pauses-standing-light-fuel-at-the-actor-boundary-p3).

  - [ ] **RF281 — Require wildlife reward claims to survive reload assertions (P3; CR16-029/030).**
    Badger, Death Adder, Moon Bear, Rat and Peat Husk tests can skip reloaded
    claim checks when a valid reload returns an empty queue. Assert the expected
    count (one claim, or two for Peat Husk) before checking persisted claims.
    Evidence and acceptance:
    [RF281 finding](Temporary/Review/findings.md#rf281-require-wildlife-reward-claims-to-survive-reload-assertions-p3).

  - [ ] **RF282 — Pin the documented tree-fall duration independently (P3; CR16-029).**
    The midpoint test derives elapsed time from `TreeFallSeconds`, so duration
    drift can pass. Independently pin 1.2 seconds and sample at 0.6 seconds.
    Evidence and acceptance:
    [RF282 finding](Temporary/Review/findings.md#rf282-pin-the-documented-tree-fall-duration-independently-p3).

  - [ ] **RF283 — Check every authored wildlife model in the winding regression (P3; CR16-029).**
    The actual-mesh winding test omits eight authored wildlife models. Add them
    to the fixtures and apply the existing mesh checks to each.
    Evidence and acceptance:
    [RF283 finding](Temporary/Review/findings.md#rf283-check-every-authored-wildlife-model-in-the-winding-regression-p3).

  - [ ] **RF284 — Verify Rat claim cleanup on the reloaded save (P3; CR16-030).**
    The Rat test validates a reloaded claim but removes the claim from the
    pre-serialization object. Perform delivery cleanup and the empty-queue
    assertion on the reloaded save.
    Evidence and acceptance:
    [RF284 finding](Temporary/Review/findings.md#rf284-verify-rat-claim-cleanup-on-the-reloaded-save-p3).

  - [ ] **RF285 — Pin wildlife optional-drop odds at the roll seam (P3; CR16-030).**
    Wildlife claim tests feed preselected outcomes into queue helpers without
    pinning the documented server roll thresholds. Exercise threshold boundaries
    independently and verify the result stored in each durable claim.
    Evidence and acceptance:
    [RF285 finding](Temporary/Review/findings.md#rf285-pin-wildlife-optional-drop-odds-at-the-roll-seam-p3).

  - [ ] **RF286 — Pin the Wolf pack formation radius independently (P3; CR16-030).**
    The formation test compares each offset with the same production radius
    constant used by the helper. Assert the documented 240 cm value separately.
    Evidence and acceptance:
    [RF286 finding](Temporary/Review/findings.md#rf286-pin-the-wolf-pack-formation-radius-independently-p3).

  - [ ] **RF287 — Pin mosquito fire-repelling radii independently (P3; CR16-030).**
    Boundary tests pass production radius constants back into the helper, so
    radius drift can pass. Assert 300 cm and 500 cm independently and test just
    outside each boundary.
    Evidence and acceptance:
    [RF287 finding](Temporary/Review/findings.md#rf287-pin-mosquito-fire-repelling-radii-independently-p3).
  - [ ] **RF288 — Restore placed Campfires after world reload (P2; CR17-001).**
    Affected files: `KalmalaCraftingComponent.cpp`, `KalmalaGameMode.cpp`,
    `KalmalaCampfire.h`, and `PersistedCampRestart.ps1`. Trigger: place a
    Campfire, save/reopen the same world, and inspect the restored camp. The
    Build-menu path creates a paid `AKalmalaCampfire` but does not add a world
    save record; host save writes the population, construction, storage and
    player save owners, while construction restore creates generic actors.
    Add a validated world-owned Campfire record and restore path. Preserve direct
    material payment, create no portable Campfire item, and roll back placement
    and payment if the record cannot be saved. Define required fire-state fields
    from the approved contract instead of inventing new behavior.
    Dependencies: after CR19 review acceptance. Acceptance: after same-world
    host save/reload, each accepted Campfire is restored once as a functional
    actor at its saved transform; another world does not load it; and a failed
    write neither reports success nor consumes materials. The restart regression
    also asserts that no Campfire inventory item exists. Focused validation:
    extend the Campfire restart and write-failure coverage, then run the
    construction save, Crafting, and `PersistedCampRestart.ps1` regressions.
    Evidence:
    [RF288 finding](Temporary/Review/findings.md#rf288-restore-placed-campfires-after-world-reload-p2).
  - [ ] **RF289 — Preserve Boar and Deer drops when the pack is full (P2; CR09-001).**
    `KalmalaWildlifeSpawn.cpp:5398-5434` separately grants each Boar/Deer
    drop and ignores failed `TryGrantFromServer` results. `bDefeatRewardProcessed`
    is set before capacity checks, then the actor is defeated and hidden, so a
    full pack loses the bundle and partial space can produce partial rewards.
    Add stable owner-bound claims, persist Deer's one-time Sinew outcome, and
    atomically deliver the complete awarded bundle when capacity returns.
    Acceptance: full-pack and partial-capacity attempts preserve all rewards;
    retry after space is freed or after same-world reload delivers the exact
    bundle once to the credited owner, without rerolling Sinew or allowing a
    second owner to claim it. Focused validation: add a GameMode/wildlife claim
    regression and run the Boar/Deer peer scenarios plus wildlife claim
    automation after an Editor build. Dependencies: after CR19 review acceptance.
    Evidence:
    [RF289 finding](Temporary/Review/findings.md#rf289-preserve-boar-and-deer-drops-when-the-pack-is-full-p2).
  - [ ] **RF290 — Abort gameplay after rejecting menu launch identity (P2; CR10-005).**
    `KalmalaWorldGenerationGameState.cpp:30-74` logs rejected menu launch
    options but leaves its default seed active. `KalmalaGameMode.cpp:520-621`
    then loads or creates world saves and activates gameplay for that seed.
    Carry explicit rejection state into GameMode and abort before save
    side-effects, player-start creation, or terrain activation. Preserve valid
    menu launches and marker-free `WorldSeed=` startup. Acceptance: malformed,
    incomplete, stale-revision, or mode-mismatched menu identity performs no
    world save load/create or gameplay activation; valid Solo/HostCoop uses the
    selected seed. Focused validation: GameState identity coverage and a
    GameMode no-side-effects startup regression, plus valid Solo/HostCoop and
    command-line cases. Dependencies: after CR19 review acceptance.
    Evidence:
    [RF290 finding](Temporary/Review/findings.md#rf290-abort-gameplay-after-rejecting-menu-launch-identity-p2).
  - [ ] **RF291 — Pin rejection of unknown M7 sparse-delta kinds (P3; CR10-006).**
    `KalmalaM7PersistenceContractTest` accepts each current delta kind but does
    not exercise a casted unknown enum value. Acceptance: the focused contract
    test proves `AddSparseDelta` rejects an unknown kind without changing the
    accepted sparse-delta count. Focused validation: run the M7 persistence
    contract automation after an Editor build. Dependencies: after CR19 review
    acceptance. Evidence:
    [RF291 finding](Temporary/Review/findings.md#rf291--pin-rejection-of-unknown-m7-sparse-delta-kinds-p3).
  - [ ] **RF292 — Cover every approved gemstone replacement rate (P3; CR10-006).**
    `KalmalaGemstoneTest` measures Quartz and Jade replacement only for Tin.
    Acceptance: deterministic assertions pin the approved replacement rate
    and eligible source/biome for Copper Quartz, Tin Quartz/Jade, Iron Amethyst,
    slate Tourmaline, and Frost Salt Star Opal, including wrong-biome rejection.
    Focused validation: run the gemstone contract automation after an Editor
    build. Dependencies: after CR19 review acceptance. Evidence:
    [RF292 finding](Temporary/Review/findings.md#rf292--cover-every-approved-gemstone-replacement-rate-p3).
  - [ ] **RF293 — Assert the island locator's surrounding sea contract (P3; CR10-006).**
    `KalmalaIslandLocatorTest` checks repeatability and a dry center but does not
    independently sample the shoreline ring required by the locator. Acceptance:
    the focused test confirms the returned center is valid land and at least
    9 of 16 samples at 1,250 m are sea. Focused validation: run the island
    locator automation after an Editor build. Dependencies: after CR19 review
    acceptance. Evidence:
    [RF293 finding](Temporary/Review/findings.md#rf293--assert-the-island-locators-surrounding-sea-contract-p3).
  - [ ] **RF294 — Pin terrain snow-mask alpha coverage (P3; CR10-007).**
    `KalmalaTerrainGroundBlendTest` covers only RGB ground weights and calls the
    helper with default zero Tundra/Mountain weights, so it does not pin the
    documented snow-mask alpha. Add expected alpha checks for Tundra, the
    Mountain height transition, and surface-normal attenuation. Focused
    validation: run the terrain ground-blend automation after an Editor build.
    Dependencies: after CR19 review acceptance. Evidence:
    [RF294 finding](Temporary/Review/findings.md#rf294--pin-terrain-snow-mask-alpha-coverage-p3).
  - [ ] **RF295 — Pin M8 vessel-save bounds (P3; CR10-007).**
    The travel persistence test accepts the maximum vessel ID but does not reject
    length 129, finite safe-location Z beyond ±100,000 cm, or finite heading
    beyond ±180 degrees. Add boundary rejection assertions while retaining
    accepted limits. Focused validation: run the Ocean travel persistence
    contract automation after an Editor build. Dependencies: after CR19 review
    acceptance. Evidence:
    [RF295 finding](Temporary/Review/findings.md#rf295--pin-m8-vessel-save-bounds-p3).
  - [ ] **RF296 — Reject out-of-world construction save transforms (P3; CR11-001).**
    Schema-7 construction validation accepts any finite location and restore
    spawns each accepted record without checking the finite-world radius.
    Reject incompatible out-of-bounds save data without rewriting it, using
    the established placement margin. Focused validation: exercise valid edge
    and finite outside-world transforms and prove the rejected slot is
    preserved. Dependencies: after CR19 review acceptance. Evidence:
    [RF296 finding](Temporary/Review/findings.md#rf296--reject-out-of-world-construction-save-transforms-p3).
  - [ ] **RF297 — Bound Smelter saved counters before arithmetic (P3; CR11-003).**
    `FKalmalaSmelterState::GetInputCoalCount` and `GetOutputCount` add signed
    32-bit values before enforcing their capacities. Reject malformed current
    schema-7 counters without rewriting the slot. Acceptance: maximum valid
    input/output boundaries still pass, while an `INT32_MAX` active-job coal
    count and an overflowing combined output count fail `MatchesWorld` before
    restore. Focused validation: extend construction-save contract coverage
    for both cases and verify the rejected slot remains unchanged. Dependencies:
    after CR19 review acceptance. Evidence:
    [RF297 finding](Temporary/Review/findings.md#rf297--bound-smelter-saved-counters-before-arithmetic-p3).

  - [ ] **RF298 — Apply text scale to the accessibility feedback overlay (P2; CR12-002).**
    The optional nearby-hearth/construction feedback text stays at a fixed 16-point size.
    Apply the shared local text-scale setting and keep the overlay readable at supported
    scales and viewports. Focused validation: exercise all five text-scale presets and
    check wrapping and clipping after an Editor build. Dependencies: after CR19 review
    acceptance. Evidence:
    [RF298 finding](Temporary/Review/findings.md#rf298--apply-text-scale-to-the-accessibility-feedback-overlay-p2).
  - [ ] **RF299 — Keep UI paths behind world-facing interfaces (P3; CR12-002).**
    Ambient audio, interaction prompts, and Home launch cleanup call concrete world
    actors or GameMode methods from the UI module. Route those reads and requests through
    components, interfaces, gameplay messages, or subsystem APIs. Focused validation:
    add a source-boundary check and exercise prompt, ambience, and Home launch flows after
    an Editor build. Dependencies: after CR19 review acceptance. Evidence:
    [RF299 finding](Temporary/Review/findings.md#rf299--keep-ui-paths-behind-world-facing-interfaces-p3).

  - [ ] **RF300 — Restore valid list markers in review records (P3; CR16 handoff).**
    `BACKLOG.md` prefixes RF160 with a literal `+`, joins RF162 to RF161's final
    paragraph, and joins RF258 to RF257's focused-validation line. `PROGRESS.md`
    has seven report bullets prefixed with literal `+` characters. These render
    as ordinary text or merged paragraphs instead of independent list entries.
    Restore all three backlog task markers and seven progress markers while
    preserving the existing text.
    **Dependencies:** After CR19 review acceptance; this is a bookkeeping repair.
    **Acceptance:** RF160, RF162, and RF258 render as separate backlog tasks, and all
    seven affected progress entries render as list items with their text intact.
    **Focused validation:** Inspect the exact diff, run an available Markdown
    parser or renderer, and run `git diff --check`; no gameplay test applies.
  - [ ] **RF301 — Reapply accessibility styling to editable address and seed inputs (P2; CR12-003/006)**
    **Source:** CR12-003 and CR12-006.
    **Affected path:** `KalmalaJoinAddressWidget.cpp:79,112-139` and `KalmalaWorldListWidget.cpp:308,480-505`.
    **Trigger:** text scale or contrast changes while either join-address or world-list widget stays open.
    **Impact:** the editable fields keep their initial font and colors while nearby controls refresh.
    **Evidence:** each editable style is assigned once during widget construction; runtime `RefreshTheme` does not reapply it; no focused test covers either field.
    **Proposed repair:** rebuild each editable style in `RefreshTheme` from the current text-scale and contrast settings.
    **Dependencies:** After CR19 review acceptance; no repair during CR01–CR19.
    **Acceptance:** both editable fields track every supported text-scale and contrast change without changing focus, entered text or commit behavior.
    **Focused validation:** add focused refresh coverage for both widgets and exercise scale/contrast changes in a rendered menu after an Editor build. Evidence: `Temporary/Review/findings.md#rf301--reapply-accessibility-styling-to-editable-address-and-seed-inputs-p2`.

  - **Queue rules:** Review agents append concrete unchecked RF01, RF02, ...
    children as findings arise; do not invent defects or placeholder fixes.
    Each child includes finding/source CR ID, severity, affected files, trigger,
    evidence, proposed change, dependencies, acceptance criteria and focused
    validation. Order eligible repairs by severity and dependencies, retaining
    stable IDs. Record explicitly accepted/deferred findings with reasons.
  - **Implementation runs:** After review completion, complete the first
    unchecked, unblocked repair child, exactly one per run. Split oversized
    repairs before implementation; preserve repository permission boundaries.
    Perform applicable lightweight checks, update backlog/progress, commit
    only run-owned changes and stop. No review finding bypasses this queue.
  - **Final acceptance:** Once every queued repair is implemented or explicitly
    dispositioned, run the applicable milestone-final verification prescribed
    by docs/07-development-setup.md; record commands, results and limitations.
    Repair verification failures within this goal and rerun required checks.
    Keep this parent unchecked until required verification passes. If review
    finds no actionable defects, record that evidence and close this goal as
    having no repairs required, without claiming unperformed checks passed.

Completed goal records are retained under Completed post-roadmap work.

## Completed roadmap

| Milestone | Completed scope / acceptance |
| --- | --- |
| M0 — Bootstrap | Project setup, prototype map, packaged build and dedicated-server availability recorded. |
| M1 — Networked traversal and interaction | Replicated movement and server-validated interaction. |
| World generation, Phases 1–9 | Deterministic generation, survival, biome expansion, ocean travel and expanded maps. |
| M2 — Survival camp loop | Gathering, crafting, construction and camp persistence. |
| M3 — Elemental world prototype | Server-owned weather, wetness, exposure and recovery. |
| M4 — Combat and support magic | Combat, creature encounters, discoveries and support effects. |
| M5 — Vertical-slice finish | Complete under recorded acceptance scope. |
| M6 — Production hardening | Complete under recorded supported-session scope and limits. |
| M7 — Survival progression and gameplay UI | Complete. |
| M8 — Ocean and long-distance travel | Complete; dry-shore acceptance waived. |
| M9 — Expanded biome content | Complete under recorded scope. |
| M10 — Release completion and launch validation | Complete under recorded scope and limits. |
| M11 — User experience and visual UI | Final acceptance passed 2026-10-06. |
| M12 — Inventory menu and cleaner HUD | Final acceptance passed 2026-10-09. |
| M13 — Revalidated UI on main | Integrated-main acceptance passed 2026-10-11: 122/122 tests, rendered peers, input/privacy/reconnect and Development/Shipping smoke. [Evidence](docs/evidence/m13/README.md). |
| M14 — Expanded world map | Integrated-main acceptance passed 2026-10-12: editor build, 123/123 tests, live map runners, rendered/performance matrix. [Evidence](docs/evidence/m14/main-integration.md). |

## Scope and history

The [roadmap](docs/04-roadmap.md) and [feature contracts](docs/game-features.md)
remain authoritative for scope; the [verification index](docs/07-development-setup.md#consolidated-verification-index)
records project checks. Detailed implementation history and retired checklists
remain in [PROGRESS.md](PROGRESS.md) and Git history. No acceptance status or
product scope changes as part of this cleanup.
