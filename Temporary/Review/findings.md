# Project-wide review findings

- **Baseline revision:** `fc83040078cb02284d7bc0b4ce2f960d873b648a` (`master`; captured 2026-10-30T17:17:26Z).
- **Current state:** CR01–CR19 review acceptance is complete on master: 386/386 review batches, including the 115 subdivided CR03 ranges, and all 1,437 CR01 tracked-file fingerprints have recorded outcomes. The reconciled register contains 254 actionable findings with corresponding open backlog handoffs. RF160, RF162, and RF258 retain malformed list placement covered by RF300; none is represented as fixed by this review. Runtime and implementation verification remain with the remediation goal.
- **Rule:** Add each confirmed actionable finding as a stable RF ID with source CR ID, severity, exact file/line or asset reference, trigger, impact, evidence, proposed repair, dependencies, acceptance criteria, and focused validation. Record questions separately and do not implement repairs during review runs.

## Reviewed batch results

### CR02-001 — Project and solution descriptors

- **Reviewed revision:** `ebcd01d1f7a9921c9308905a7d63eaeb12148b9c`; the three current SHA-256 values match the CR01 baseline.
- **Review evidence:** Both `.slnx` files parsed as XML; the `.uproject` parsed as JSON. All 53 Automation solution references and 59 game solution references exist, including all three local generated project references. The game solution has the documented `DebugGame Editor | Win64` mapping. All six manifest modules have source directories and matching `.Build.cs` files.
- **Outcome:** No confirmed actionable defect. The solution files are generated for UE 5.8; the current recovery guidance documents their local generated references and installed-engine limitations. No project-file regeneration or build was attempted.
- **Questions:** None.

### CR02-002 — Game and Editor target rules

- **Reviewed revision:** `cfbaccd6931c448f7600014de55d80705d95b189`; both SHA-256 fingerprints match the CR01 baseline.
- **Evidence:** Read both target constructors in full. Game selects `TargetType.Game` and the four runtime modules Core/Gameplay/World/UI; Editor selects `TargetType.Editor` and adds KalmalaEditor. Each class/constructor matches its target filename. Every named module is registered in the project descriptor and has a matching Build.cs. Neither target explicitly adds the ServerOnly module; Game does not add the Editor module. Both use V7 build settings and Unreal5_8 include order, present in the installed UE 5.8 TargetRules.cs (lines 170 and 235). Compared module intent with architecture module boundaries and development-setup project-file recovery guidance.
- **Result:** No confirmed actionable finding or open question within these two target rules.
- **Limits:** Static review only; no UBT invocation, regeneration, compilation, test suite or runtime validation. Module dependency internals and dedicated-server target remain assigned to their later batches; this result does not close CR04.

### CR02-003 — Core module rules

- **Reviewed revision:** `9bbd3e5f90e440e2426111a32ae48e34e3bd825b`; SHA-256 `834ef4b8ff32ab4c7239c336e8e1bcb47446d02ed5afda74362a8f5323f38335` matches baseline.
- **Evidence:** Read the complete KalmalaCore.Build.cs. Module class and constructor match the filename and forward Target to ModuleRules; explicit/shared PCH policy is set. Its only dependencies are public Core, CoreUObject, Engine and GameplayTags; each has a matching installed engine Runtime Build.cs. No project-module reverse dependency, editor dependency, platform-specific branch, custom build step or external library is introduced. Checked descriptor runtime registration, architecture module intent and inbound Core references from Gameplay, World, UI and Server rules. Core currently has only the build rule and default module implementation, with no public header surface; unused forward-looking engine dependencies alone are not a confirmed defect.
- **Result:** No confirmed actionable finding or open question in this rule file.
- **Limits:** Static inspection and focused source checks only. No rules compilation, UBT invocation, build, regeneration or runtime tests. The module implementation remains in CR02-004 and CR04 remains pending.

### CR02-004 — Core module implementation and CR02 closure

- **Reviewed revision:** `cfc40b44be8cb9883a109539084ad5c1fe762b19`; SHA-256 `209a7c16c1799ff6efac7383f1074f5f686dcb039b585e5ef798aee6eaeab2ee` matches baseline.
- **Evidence:** Read the entire three-line implementation. The sole include is Core's Modules/ModuleManager.h and the sole statement registers FDefaultModuleImpl as KalmalaCore. Installed engine ModuleManager.h defines that implementation as an empty IModuleInterface subclass (line 884); it introduces no custom initialization, delegates, allocations, retained objects or teardown. The name matches the build-rule class, unique Runtime descriptor entry and target references. Source search found exactly one KalmalaCore registration. No public headers or shared types exist in the Core directory to review.
- **Result:** No confirmed actionable defect or open question. All four assigned CR02 batches are reviewed; CR02 is complete as a bounded review child. Other modules' rule/API internals and the server target retain their ledger assignments; cross-cutting CR04 remains pending. This is not overall project acceptance.
- **Checks / limits:** Baseline SHA-256, exact registration-only source shape and unique runtime descriptor assertions passed. Engine implementation inspected locally. No compilation, UBT invocation, regeneration, tests or runtime validation.

### CR03-001a — GameMode opening helpers (lines 1–240)

- **Reviewed revision:** `e3b42016f211986e48e5c261573af415c4a7d8d9`; current whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches baseline, including pre-existing local edits. Only lines 1–240 are covered by this result.
- **Batch adjustment:** Original CR03-001 contains a 12,053-line GameMode plus four other files. Split it before review: 001a opening helper namespace; 001b remaining GameMode (must be subdivided into bounded ranges before review); 001c the other four files. GameMode remains partially reviewed, never whole-file reviewed.
- **Evidence:** Read includes, constants and complete KalmalaGameMode helper namespace. Wildlife selection first resolves biome/archetype, applies water/shore/slope checks for restricted species, and filters through the same eligibility helper; compared habitat intent with monster-spawning.md. Session review centers on six slot-name helpers and GetAuthenticatedPlayerIdentity (lines 205–239): slots include world seed and, for ocean records, generator revision; player slots use a fixed-width CRC of owner identity. Identity resolution rejects missing PlayerState, invalid network ID/shared ID and values above the 128-character contract bound; it prefixes provider type and never accepts a menu display name. Compared against startmenu.md identity restrictions and architecture ownership guidance.
- **Result:** No confirmed actionable defect within this range. Duplicate includes are harmless and are not queued as defects.
- **Follow-through / limits:** Slot hashes are routing keys, not proof of ownership. Collision handling, full owner/revision matching and failure behavior in load/write callers need the later GameMode/persistence review; no collision-safety or end-to-end authentication claim is made here. Actual provider authentication is outside this helper. No UBT, build, runtime or peer tests ran. CR03 and CR04 remain pending.
- **Lightweight checks:** Baseline hash and focused identity null/validity/length guard assertions passed; save-slot format expressions and source contract bound inspected.

### CR03-001b-01 — GameMode route search and grid update (lines 241–398)

- **Reviewed revision:** `42180cf4989f35319ffe92b35cf3c2ebb496b495`; whole-file SHA-256 remains `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325`, matching baseline including existing local edits.
- **Scope:** First bounded subdivision of CR03-001b: complete KalmalaOceanJourneyPeer namespace and UpdateInteractionGrid. Remaining range starts at UpdatePlayerExposure on line 399 and stays pending.
- **Evidence:** Route search uses bounded 72-ray, radial, 20-bisection and 24-heading loops; outputs are assigned only after footprint sampling along the route and a different start/end terrain patch. Inspected the skiff footprint validator's configuration, finite heading/center and world-boundary gates. This sampling helper does not establish continuous route clearance or runtime journey acceptance.
- **Grid evidence:** Authority and GameState guard precede mutation. Lit hearths and eligible possessed pawns are sorted by server actor name, hearth neighborhoods precede pawn neighborhoods, a set removes duplicate keys, and the 1,024-cell cap bounds admitted work. Previous active states are moved and reused only for retained keys; absent keys expire, new keys receive server-derived baseline, and moisture advances from authoritative rain. Compared the exact priority/cap/expiry policy with architecture's interaction-grid contract and inspected helper constants and finite-value moisture handling. No client target, camera or map request drives this activation.
- **Result / limits:** No confirmed actionable defect in this range. No compile, Unreal, runtime or peer validation. Source/name ordering and sampled navigation are inspected behavior, not measured performance or end-to-end acceptance. Helper implementations read as context retain their own review assignments; CR03 and CR04 remain open.
- **Lightweight checks:** Baseline SHA-256, next function boundary, authority-before-mutation, deduplication order, hearth-before-pawn ordering and transient-state reset assertions passed.

### CR03-001b-02 — Player exposure and GameMode defaults (lines 399–519)

- **Reviewed revision:** `d8b9d467b7ba498dbd9c9b54d536bda808c1ad0e`; whole-file baseline SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` unchanged. Scope includes complete UpdatePlayerExposure and constructor; BeginPlay starts at line 520.
- **Evidence:** Exposure reads server weather, biome, shelter and pawn position; advances construction rain wear and held-torch fuel; water refreshes Wet immediately, unroofed rain accumulates a bounded trigger, dry/roof conditions reset it, and authoritative campfire removal follows refresh. Compared with design's Wet/rain override and architecture exposure contract. Inspected Tick's authority/config/world guard (7824) and one-second constant argument (7898) as caller context, and status/character authority seams. Constructor selects seamless travel, ticking, character/controller/player-state and generation GameState defaults.
- **Finding:** RF01 (P3), stale rain-timer map entries across character lifetimes; see confirmed findings and queued remediation. No repair performed.
- **Checks / limits:** Whole-file baseline hash and exact next function boundary passed. Searched every rain-map reference and inspected Logout as cleanup context. No runtime memory measurement, build, UBT, tests or peer validation. Other helper and caller review assignments remain pending; no end-to-end exposure balance claim.

### CR03-001b-03 — GameMode BeginPlay (lines 520–646)

- **Reviewed revision:** `b8b3bbf11d224b5eaf6c93b3896f6e9b0f452530`; whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches baseline including preserved local edits. Complete BeginPlay reviewed; next function LoadPopulationSave begins at line 647.
- **Evidence:** Super precedes authority and missing generation-state guards. Server world config is copied before ocean/weather/population initialization. Construction distinguishes missing saves from failed reads before TryBuildWriteCandidate, whose current-class/world matching and duplicate-candidate branches were inspected as context. Storage rejects failed casts/world mismatch and creates a fresh instance only for absent slots. Neither rejection branch writes the rejected save. Save pointers are retained by transient UPROPERTY fields.
- **Startup ordering:** Server creates a seed-derived PlayerStart; only successful spawn enters terrain activation, test-fixture call, construction/skiff restoration and existing-controller placement. Inspected fixture Shipping exclusion/command-line gate, construction null guard and placement authority/start guards as context. Weather/schema fixtures in this function are Shipping-excluded. Compared with architecture server ownership, unreleased save rejection policy, and startmenu's separate world checkpoints contract.
- **Result:** No new confirmed actionable defect in this function. RF01 remains open. Called loaders, restoration bodies, config lifecycle, spawn-failure recovery and reconnect fixtures retain later review scope; this result does not certify those paths or atomic save behavior.
- **Lightweight checks / limits:** Baseline hash, next function boundary, authority-before-load, failed-read candidate gate and terrain-before-player-placement assertions passed. No UBT/build/regeneration, tests, editor or runtime validation.

### CR03-001b-04 — Population loading and construction persistence (lines 647–744)

- **Reviewed revision:** `647565982f8f13b031ae3b26b6e4d89cf339cb6d`; whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches baseline including existing local edits. Covers LoadPopulationSave, SpawnOceanTravelTestFixture, CanPersistConstruction and PersistConstruction in full; next starts at CanReplaceWallWithDoor, line 745.
- **Population evidence:** Existing slot read/cast/world-match failure returns with null live pointer without rewriting rejected data. Absent slots initialize a new record and attempt its first write; write failure logs an error while retaining the live record for later saving, matching startmenu's initial-world-record contract. Only current caller is authority-gated BeginPlay. Read MatchesWorld as context for schema/config validation; complete malformed-record validation remains in its own review assignment.
- **Fixture evidence:** Entire spawn helper is Shipping-excluded and requires authority, world, explicit command flag and generated start. Failed island lookup/spawn returns with diagnostics; constant width/depth feed fixture initialization. Fixture internals remain separately pending.
- **Construction evidence:** Admission checks authority, available matching save, valid record/transform, overall count and station-attachment count. Persistence constructs a new record and candidate, routes attachment additions separately, revalidates and saves before publishing the candidate pointer. Inspected candidate-builder rejection and the crafting call site's server-generated actor/ID and direct-material exchange with failure rollback as context. This is no portable construction grant. Full transaction/cross-world misuse review remains assigned to construction and CR04.
- **Result / checks / limits:** No new confirmed defect; RF01 remains open. Baseline hash, exact boundary, rejected-load return and save-before-publish assertions passed. No build, UBT, tests, editor, runtime save-failure injection or peer validation. Candidate/record/crafting internals read as context do not close their own batches.

## Confirmed actionable findings

### RF01 — Prune stale per-character rain timers (P3)

- **Source:** CR03-001b-02; reviewed revision `d8b9d467b7ba498dbd9c9b54d536bda808c1ad0e` with baseline-matching local GameMode bytes.
- **Evidence:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:435` calls FindOrAdd for each possessed character every exposure step. `Source/KalmalaGameplay/Public/KalmalaGameMode.h:610` stores a TMap of weak character keys to floats. Repository-wide search finds only two Remove calls, at GameMode lines 1152 and 1163 in rain test setup; no production prune/reset/removal. Logout (10817 onward) persists the pawn and calls Super without clearing the timer. Weak keys cease keeping objects alive but do not automatically remove TMap entries.
- **Trigger / impact:** A long-lived server world repeatedly replaces characters or accepts reconnects. Each distinct destroyed character can leave an invalid-key timer entry until GameMode destruction, causing unbounded small server-memory growth. No live memory measurement or data-loss claim.
- **Repair / dependencies:** After CR19 review acceptance, prune invalid weak keys during the exposure step and clear obsolete timers on relevant ownership/lifetime transitions where needed. Preserve current live characters' accumulated rain time. No dependency beyond the review-remediation gate.
- **Acceptance / focused validation:** Repeatedly add/remove character lifetimes and run exposure maintenance; expired keys disappear, current timers retain values and wet-trigger behavior is unchanged. Inspect cleanup with destroyed pawns and disconnects; run a narrow lifecycle regression when implementing.

### CR03-001b-05 — Wall replacement and door persistence (lines 745–798)

- **Reviewed revision:** `09e342f5`; whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches baseline including existing edits. Split the remaining range before review; next batch begins at PersistCharcoalKilnState, line 799.
- **Evidence:** Read all three functions. Admission requires authority, valid same-world authoritative wall, exact saved ID/type/transform and matching world. Replacement modifies an isolated candidate, validates and writes before publishing and changing the live actor. Inspected the actor conversion: its only rejection guards are already satisfied by admission on this synchronous path. Door persistence rejects unchanged state and validates exact saved identity/transform through UpdateDoorOpenRecord before writing and publishing.
- **Caller context:** Crafting pays direct materials and restores inventory on replacement failure. Door toggle checks usable range and occupied closing clearance before persistence and publishes replicated state after success. Compared with architecture's 8-Wood replacement, retained identity/transform and schema-7 open-state contract. These contextual reads do not close the construction/crafting or CR04 assignments.
- **Result / checks / limits:** No new confirmed defect; RF01 remains open. Baseline hash, exact next boundary and save-before-publication checks passed. No code changed; no build, tests, editor, save-failure injection or peer validation. Full verification deferred; CR03 remains incomplete.

### CR03-001b-06 - Kiln and Smelter persistence (lines 799-862)

- **Reviewed revision:** `9d0886d74f08d31897564f3b4c64ce91e58dd445`; GameMode whole-file SHA-256 matches baseline including preserved local edits. Split remaining range; next starts at line 863.
- **Evidence:** Read both persistence functions completely. Each rejects non-authority, invalid/non-authoritative or foreign-world actors, wrong buildable type, invalid state and mismatched save world. Exact construction ID/type and transform must match the saved record. Candidate creation, record update, complete-world validation and synchronous slot write precede publication of the new save owner; failures return without publishing.
- **Context:** Inspected record updater validation/rollback, authoritative actor processing tick and storage load/collection callers. Tick and interactions apply live state only after persistence succeeds; collection saves decreased output before inventory commit and attempts restoration on commit rejection. Compared with architecture's active-time processing and schema-7 contract. These reads do not close helper, crafting, persistence or CR04 coverage.
- **Result / checks / limits:** No new confirmed defect in this range; RF01 remains open. Baseline hash, exact function boundaries, authority/identity guards and save-before-publication assertions passed. No code changed, build, UBT, tests, editor, failure injection or runtime validation. Full verification deferred; CR03 remains incomplete. No end-to-end atomicity or runtime performance claim.

### CR03-001b-07 - Garden Plot and Standing Light persistence (lines 863-926)

- **Reviewed revision:** `d8ab9744f19c552a6a104969293b1371578f2b15`; GameMode whole-file SHA-256 matches baseline including preserved local edits. Split remaining range before review; next starts at line 927.
- **Evidence:** Read both complete functions. Each requires authority, a valid authoritative same-world actor of the exact buildable type, valid state and matching world save. Saved ID/type and transform must match. Isolated candidate creation, record update, complete-world validation and successful synchronous slot write precede publication; rejection leaves the save owner unchanged.
- **Context:** Inspected both record updaters and their rollback on failed validation, actor tick publication after successful persistence, and planting payment/persistence rollback. Compared with architecture schema-7 crop active-time and light fuel contract. These contextual reads do not close construction, cultivation, persistence or CR04 batches.
- **Result / checks / limits:** No new confirmed defect in this range; RF01 remains queued. Baseline hash, exact boundaries, authority/identity guards and save-before-publication assertions passed. No build, UBT, tests, editor, runtime or failure injection; full verification deferred. No end-to-end atomicity or performance claim. CR03 remains incomplete.


### CR03-001b-08 - Terrain activity, construction restoration and storage (lines 927-987)

- **Reviewed revision:** `10c0b8547517ea82f501ad7508119dda645decb0`; whole-file SHA-256 matches baseline `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325`, including preserved local edits. Subdivided remaining range before review; next begins at LogExposureInspection, line 988.
- **Evidence:** Read all four functions completely. Terrain membership rejects non-authority/non-finite positions and uses the shared patch layout. Restoration requires authority and a matching validated save, validates each record, handles allocation failure and initializes deferred actors before FinishSpawning. It creates placed actors directly, without granting portable items.
- **Storage evidence:** ReadStorage requires authoritative same-world chest, matching storage/construction worlds and exact placed ID/type/transform; absent contents return an empty successful result. PersistStorage repeats admission via ReadStorage, duplicates the save, validates through UpsertRecord and writes before publishing. Inspected storage world/stack validators and UpsertRecord capacity/identity checks, shared terrain coordinate helper, and server transfer caller as context. These do not close those files' own review assignments.
- **Result / checks / limits:** No new confirmed defect in this range; RF01 remains queued. Baseline hash, function boundary and save-before-publication assertions passed. No build, UBT, tests, editor or runtime/failure injection; full verification deferred. No end-to-end transaction or restore-success guarantee. CR03 remains incomplete.

### CR03-001b-09 - Developer inspection helpers (lines 988-1025)

- **Reviewed revision:** `f64721132f6c7bba9d51776d660568324e9f826d`; whole-file SHA-256 matches baseline `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325`, including preserved local edits. Subdivided remaining range before review; weather initialization starts at line 1026.
- **Evidence:** Read all three complete helpers. Null occupants return; exposure additionally requires generation GameState. Position/config-derived terrain, exposure, camp and biome samples feed logs only. Shelter uses the server world and weather direction. No inventory grant, spawn, discovery reveal or persistence mutation occurs in these helpers. Inspected their sole call sites in player initialization and command-line enable flags in BeginPlay as context; GameMode owns these diagnostics on the server.
- **Contract:** Compared architecture camp-condition inspection and setup Exposure inspection guidance. Logged Wetness=0/Warmth=100/Mitigation=None are provisional diagnostic constants, not live character state; setup explicitly calls this provisional exposure state. No confirmed defect assigned for that documented limitation. Sampler internals and full initialization lifecycle retain their own review assignments.
- **Result / checks / limits:** No new confirmed actionable finding; RF01 remains queued. Baseline hash, exact boundaries and three null guards/log calls passed. No code changed; no build, UBT, tests, editor or runtime validation. Full verification deferred; CR03 and CR04 remain incomplete. No live exposure or sampling-performance acceptance claim.

### CR03-001b-10 - Weather cycle (lines 1026-1058)

- **Revision:** `2ef40a9bb414905c51aad037652625347b79b8ff`; whole-file SHA-256 matches baseline including preserved local edits. Split remaining range before review; next begins at rain fixture line 1059.
- **Evidence:** Read both complete functions. Initialization asserts authority/GameState and derives index zero at server time. Advancement handles missing GameState and catches up elapsed intervals at scheduled boundaries before publishing server-derived states. Reserved ocean journey fixture freeze requires a non-Shipping server command-line flag. Inspected DeriveState positive 120-240 second duration, GameState setter authority/state validation before replication, and BeginPlay/Tick callers. Compared architecture weather-cycle restart/clock/replication contract. Contextual reads do not close helper or CR04 coverage.
- **Result/checks/limits:** No new confirmed finding; RF01 remains queued. Baseline, corrected exact boundary and initialization/advancement assertions passed (initial boundary assertion was off by one; no files changed on that attempt). No build, UBT, tests, editor or runtime validation; full verification deferred. No long-session precision, catch-up performance or peer acceptance claim. CR03 remains incomplete.


### CR03-001b-11 - Rain acceptance fixture (lines 1059-1222)

- **Revision:** bec792155fbde70f875f6a4443da75a456b60392; whole-file SHA-256 matches baseline including preserved local edits. Split remaining range before review; next starts at DriveCombatPeerTest line 1224.
- **Evidence:** Read the complete fixture: non-Shipping command-line gate, two-pawn collection, bounded deterministic water search, direct placed-actor spawn with initial validity checks, staged water Wet/heat recovery/rain wear/smoulder/roof recovery assertions and terminal failure logging. Weather changes use the server setter; no client payload or portable construction grant is introduced. Inspected GameMode Tick caller, weak fixture fields and acceptance runner host/client seed, log and timeout checks as context. Compared design/architecture rain and hearth contracts and setup RainVerticalSlice guidance. Context reads do not close their own assignments.
- **Result/checks/limits:** No new confirmed finding for the controlled two-peer fixture; RF01 remains queued. Baseline, exact boundaries, non-Shipping flag and two-player gate assertions passed. Fixture assumes spawned actors survive subsequent stages; disconnect/destruction robustness and runtime timing were not verified. No build, UBT, tests, editor or runtime validation; full verification deferred. CR03 remains incomplete.


## CR03-001b-12 — Combat fixture setup

Reviewed GameMode lines 1224-1332 at `77708f54075a6b31a75657be9cc2247972b06e66` with baseline-matching local bytes. No new confirmed actionable finding. Scenario gates, participant wait and server equipment rejection paths inspected; later stages remain pending. RF01 remains queued. Static review only; full verification deferred. Evidence: coverage.md CR03-001b-12.


## CR03-001b-13 - Combat fixture descriptor search

Reviewed GameMode lines 1333-1386 at c516da969e5258e113816ffacd00049899722fc1 with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Bounded search, archetype selection and missing-descriptor failure inspected. ID/count comparison does not prove full descriptor equality; later acceptance remains pending. Static checks only; full verification deferred. See coverage.md CR03-001b-13.


## CR03-001b-14 - Combat target activation and herd selection

Reviewed GameMode lines 1387-1426 at 855390b96018a7851e917c0b2bbb65ffec815fe2 with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Global herd selection is followed by deliberate controlled positioning. Target identity, bounded fallback and failure guards inspected; no runtime or overall population-bound claim. Full verification deferred. See coverage.md CR03-001b-14.


## CR03-001b-15 - Pending-reward fixture setup

Reviewed GameMode 1427-1482 at aedeba9f34441456aa60ffdbc566352a077d7ec2 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected coat identity/equip, buildable exclusions and exact full-pack guard. No runtime reward/retry/privacy acceptance claimed; full verification deferred. See coverage.md CR03-001b-15.


## CR03-001b-16 - Combat fixture positioning

Reviewed GameMode 1484-1587 at `130c0d6a` with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Finite dry-approach search, failure exit, grounded placements and observer exclusion inspected. Sampled dryness and unswept fixture teleports do not establish production collision safety. Full verification deferred; see coverage.md CR03-001b-16.


## CR03-001b-17 - Combat fixture health reset and herd placement

Reviewed GameMode 1588-1630 at eaad9379 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Bounded healing and terminal full-health validation inspected; advancing poison also advances other statuses and may apply one tick. Controlled default-health assumptions remain. Full verification deferred; see coverage.md CR03-001b-17.


## CR03-001b-18 - Screenshot setup and stage-zero completion

Reviewed GameMode 1632-1661 at f39a829a34ec30c9d3cfd33f5b2b7f89e49015ae with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Non-Shipping trusted command-line capture, optional transient camera and stage publication inspected. Capture success/framing and peer delivery remain unverified. Full verification deferred; see coverage.md CR03-001b-18.


## CR03-001b-19 - Fixture lifetime and snake observation

Reviewed GameMode 1663-1701 at 7b3f0f5f with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Weak actor/component guards and latched snake observations inspected. Giant Snake health loss is an indirect bite signal, not independent proof of damage source/type. Full verification deferred; see coverage.md CR03-001b-19.


## CR03-001b-20 - Target placement helper

Reviewed GameMode 1702-1723 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Server-derived placement, dry-shore rejection and caller failure exits inspected. Unswept relocation and unchecked relocation result remain runtime limitations, not proven failures. Full verification deferred; see coverage.md CR03-001b-20.


## CR03-001b-21 - Encounter pressure and attack sequencing

Reviewed GameMode 1725-1786 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Bounded pressure grace, failure exits, attack sequence cap and companion lifetime guard inspected. Sampled health/pressure does not prove historical non-mutation or damage source; runtime timing remains unverified. Full verification deferred; see coverage.md CR03-001b-21.


## CR03-001b-22 - Pending-reward retry observation

Reviewed GameMode 1788-1908 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Claim absence, exact reward counts, peer exclusion, defeat/save-object predicates and terminal timeout inspected. Server-side quantities do not prove client privacy; in-memory saved defeat does not prove disk durability. Full verification deferred; see coverage.md CR03-001b-22.


## CR03-001b-23 - Post-combat defeat and owner claim lookup

Reviewed GameMode 1910-1970 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Accepted-serial gate, Rat natural defeat requirement, bounded fixture damage and authenticated owner claim lookup inspected. Direct defeat fallback is reward fixture setup, not natural lethality evidence. Full verification deferred; see coverage.md CR03-001b-23.


## CR03-001b-24 - Full-pack escrow predicates

Reviewed GameMode 1971-2026 with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Inspected full-capacity, zero-reward, owner-claim and other-peer predicates plus terminal failure. Server observations do not prove replication privacy, unique claim count or durable storage. Full verification deferred; see coverage.md CR03-001b-24.


## CR03-001b-25 - Reward retry cell freeing

Reviewed GameMode 2028-2055 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected bounded snapshot consumption, reward exclusions, failure exit and stage publication. Capacity preparation does not prove retry delivery or durable storage. Full verification deferred; see coverage.md CR03-001b-25.

## CR03-001b-26 - Combat fixture final acceptance

Reviewed GameMode 2057-2086 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected saved-defeat, reward, herd and terminal predicates. Server observations and in-memory save membership do not prove privacy or restart durability. Full verification deferred; see coverage.md CR03-001b-26.

## CR03-001b-27 - Mosquito fixture setup

Reviewed GameMode 2088-2211 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected server/Shipping gates, peer selection, placement helpers and bounded generated-descriptor selection. Runtime placement, external timeout, status replication and later stages remain unverified. Full verification deferred; see coverage.md CR03-001b-27.

## CR03-001b-28 - Mosquito fixture activation

Reviewed GameMode 2213-2258 at 02ff769a6d62906bbbc4098d2e38a0ae770cad4a with baseline-matching bytes. No new confirmed actionable finding; RF01 remains queued. Pawn lifetime, selected stable ID, activation timeout, archetype and stage publication inspected. Placement success and peer replication remain unverified. Full verification deferred; see coverage.md CR03-001b-28.


## CR03-001b-29 - Mosquito encounter observation

Reviewed GameMode 2260-2288 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Lifetime guards, bite/health/status predicate, stage publication and thirty-second timeout inspected. Aggregate observations do not prove damage attribution, exact duration or peer privacy. Full verification deferred; see coverage.md CR03-001b-29.


## CR03-001b-30 - Mosquito dodge request sequencing

Reviewed GameMode 2290-2339 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Lifetime, bite comparison, request counts and timeouts inspected with character RPC context. Rejection reason and live timing remain unverified. Full verification deferred; see coverage.md CR03-001b-30.

## CR03-001b-31 - Roll window and held torch setup

Reviewed GameMode 2341-2406 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected bite/count/window gates, server material grants and craft, GUID selection, flame eligibility and contact/stage publication. Clean-loadout identity and runtime timing/persistence/privacy remain limitations. Full verification deferred; see coverage.md CR03-001b-31.

## CR03-001b-32 - Held torch protection and unequip

Reviewed GameMode 2408-2447 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected bite/flame/window gates, server Main Hand clearing, flame-state update and contact/stage publication. Continuous protection, resumed bites, runtime timing, persistence and peer privacy remain unverified. Full verification deferred; see coverage.md CR03-001b-32.

## CR03-001b-33 - Resumed bites and unlit campfire setup

Reviewed GameMode 2449-2484 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected resumed-bite timeout, spawn/default flame checks and contact/stage publication. Exact reacquisition delay, collision safety, later flame behavior and cleanup remain unverified. Full verification deferred; see coverage.md CR03-001b-33.

## CR03-001b-34 - Unlit campfire bite and ignition

Reviewed GameMode 2485-2515 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected weak-reference/unlit gate, resumed bite timeout, paid fixture ignition and contact/stage publication. Continuous protection state, lit behavior, persistence and privacy remain unverified. Full verification deferred; see coverage.md CR03-001b-34.

## CR03-001b-35 - Lit campfire final acceptance

Reviewed GameMode 2517-2555 with baseline-matching bytes. No new confirmed actionable GameMode finding; RF01 queued. Flame/bite/time gates, cumulative evidence, null-status rejection and terminal ordering inspected. Privacy, timing, persistence and cleanup remain limitations. Question for CR14/CR16: reconcile setup's absent MosquitoSwarmPeer.ps1 reference against current runner layout. Full verification deferred; see coverage.md CR03-001b-35.

## CR03-001b-36 - Peat Husk selection and activation

Reviewed GameMode 2557-2665 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Inspected authority/Shipping gates, peer waits, bounded descriptor search, exclusion/saved-defeat filters and activation budget/order. Frame cost, distinct connections, actual activation and replication unverified. Full verification deferred; see coverage.md CR03-001b-36.

## CR03-001b-37 - Peat Husk activation and placement

Reviewed GameMode 2667-2799 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Host/target lifetime, profile, placement/search bounds, observers and stage publication inspected. Collision-safe arrival, continuous path, live exclusion position, replication and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-37.

## CR03-001b-38 - Peat Husk sweep acceptance

Reviewed GameMode 2801-2837 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Pointer guards, latched cues, hit/damage predicates, success and timeout termination inspected. Exact timing/order, damage attribution, continued observers, replication and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-38.

## CR03-001b-39 - Wolf pack and approach selection

Reviewed GameMode 2839-2987 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Authority/Shipping gates, waits, bounded pack/member selection, terrain/reach filters and population budget inspected. Frame cost, collision/path safety, formation and actual activation remain unverified. Full verification deferred; see coverage.md CR03-001b-39.

## CR03-001b-40 - Wolf placement and activation

Reviewed GameMode 2989-3065 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Placement helper, finite health, bounded observers, array precondition and activation/publication inspected. Direct terrain Z, unchecked teleport and leash wording remain explicit fixture limitations; collision-safe arrival, targeting, replication and cleanup unverified. Full verification deferred; see coverage.md CR03-001b-40.

## CR03-001b-41 - Wolf formation and attack acceptance

Reviewed GameMode 3067-3185 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Member identity/count/profile/formation gates, array preconditions, attack cues, hit/damage acceptance and timeouts inspected. XY-only formation, cue timing/order, damage attribution, observers, replication and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-41.

## CR03-001b-42 - Lynx gates and terrain helpers

Reviewed GameMode 3187-3260 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Authority/Shipping gates, placement clearance, peer timeouts and dry-surface predicate inspected. Collision-safe placement, distinct connections, cleanup and later predicate use unverified. Full verification deferred; see coverage.md CR03-001b-42.

## CR03-001b-43 - Lynx descriptor budget and approach selection

Reviewed GameMode 3262-3410 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Search bounds, descriptor/actor budgets, pack accounting, dry approach/ring/path and reach inspected. Activation parity, rejected-candidate accounting, collision/navigation and frame cost remain explicit review limits. Full verification deferred; see coverage.md CR03-001b-43.

## CR03-001b-44 - Lynx staging and activation

Reviewed GameMode 3412-3516 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Health, ring staging, activation/identity/profile, bounded observer placement and stage publication inspected. Actual reach, teleport success, visibility/replication, connectivity and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-44.

## CR03-001b-45 - Lynx leap acceptance

Reviewed GameMode 3518-3551 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Lifetime guard, cue latches, net-damage acceptance and success/timeout termination inspected. No per-hit attribution or exact timing/order proof; client replication, observer connectivity and cleanup unverified. Full verification deferred; see coverage.md CR03-001b-45.

## CR03-001b-46 - Polar Bear gates and terrain helpers

Reviewed GameMode 3553-3626 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Authority/Shipping gates, placement clearance, peer waits/timeouts and dry predicate inspected. Collision-safe arrival, topology, later caller enforcement, reserve and cleanup remain unverified. Full verification deferred; see coverage.md CR03-001b-46.

## CR03-001b-47 - Polar Bear descriptor and approach selection

Reviewed GameMode 3628-3777 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Search bounds, budgets/pack accounting, hard exclusion, saved defeat, dry path/ring and charge reach inspected. Activation parity, rejected-candidate accounting, collision/navigation and frame cost remain limitations. Full verification deferred; see coverage.md CR03-001b-47.

## CR03-001b-48 - Polar Bear reserve staging and activation

Reviewed GameMode 3779-3889 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Reserve, host/target identity/profile/exclusion, activation timeout, observers and stage publication inspected. Live position/reach, teleport, reserve restoration, connectivity, replication and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-48.

## CR03-001b-49 - Polar Bear charge acceptance

Reviewed GameMode 3891-3927 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Lifetime guard, four cue latches, net-damage acceptance and success/timeout termination inspected. Sequence/timing, per-hit attribution, reserve restoration, observers, replication and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-49.

## CR03-001b-50 - Horned Goat gates and mountain helpers

Reviewed GameMode 3929-4020 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Authority/Shipping gates, terrain clearance, Walking restoration, sampled dry/height path and peer timeouts inspected. Collision safety, later helper use, stagger/push, topology and cleanup unverified. Full verification deferred; see coverage.md CR03-001b-50.

## CR03-001b-51 - Horned Goat descriptor and lane selection

Reviewed GameMode 4021-4116 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Mountain key/budget, archetype/exclusion/defeat/count filters, bounded lane search and failure gates inspected. Sampled terrain versus exact contact/collision safety, activation parity, push and replication remain limitations. Full verification deferred; see coverage.md CR03-001b-51.

## CR03-001b-52 - Horned Goat staging and activation

Reviewed GameMode 4118-4220 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Fixture health/resets, actor lifetime/profile, contact baseline, bounded dry observers and publication inspected. Actual reach/collision, Armor application, visibility/replication, restoration and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-52.

## CR03-001b-53 - Horned Goat push and stagger acceptance

Reviewed GameMode 4222-4275 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Lifetime/cue/stagger latches, duration bounds, net damage/displacement and terminal timeout inspected. Sampling delay, exact cue order, effect attribution/bounds, replication, restoration and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-53.

## CR03-001b-54 - Thunder Sentinel gates and mountain helpers

Reviewed GameMode 4277-4368 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Authority/Shipping gates, terrain clearance/Walking restoration, dry sampled path and peer timeouts inspected. Collision safety, helper callers, actual step, topology, reserve and cleanup unverified. Full verification deferred; see coverage.md CR03-001b-54.

## CR03-001b-55 - Thunder Sentinel descriptor and lane selection

Reviewed GameMode 4369-4464 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Mountain/budget, archetype/exclusion/defeat/count gates, bounded paths and rejection inspected. Live-position evidence, collision safety, activation parity, frame cost and actual step/slam unverified. Full verification deferred; see coverage.md CR03-001b-55.

## CR03-001b-56 - Thunder Sentinel staging and activation

Reviewed GameMode 4466-4580 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Reserve/reset, lifetime/profile/exclusion, actual XY reach gate, dry observers and publication inspected. Live start exclusion, Armor application, teleport, restoration, connectivity/replication and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-56.

## CR03-001b-57 - Thunder Sentinel slam acceptance

Reviewed GameMode 4582-4628 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Lifetime/cue gates, sampled step, damage tolerance, health and terminal timeout inspected. Step trajectory, recovery duration, per-hit attribution, replication, restoration and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-57.

## CR03-001b-58 - Reef Maw gates and ocean helpers

Reviewed GameMode 4630-4710 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Authority/Shipping gates, ocean habitat/path predicate, swimming helper/component/mode and peer timeouts inspected. Partial placement mutation, teleport/collision, actual depth/attachment, topology and cleanup remain limitations. Full verification deferred; see coverage.md CR03-001b-58.

## CR03-001b-59 - Reef Maw descriptor and ocean lane selection

Reviewed GameMode 4711-4810 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Ocean budget/filter/count gates, bounded three-peer path selection, failure gates and activation inspected. Sampled versus live depth/exclusion, collision/navigation, accounting parity, frame cost and replication remain limitations. Full verification deferred; see coverage.md CR03-001b-59.

## CR03-001b-60 - Reef Maw activation and swimmer staging

Reviewed GameMode 4812-4931 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Actor lifetime/profile, three-position/two-observer bounds, swimming/attachment, actual reach and publication inspected. Live habitat/exclusion, Armor application, collision safety, continued connectivity, replication and restoration remain limitations. Full verification deferred; see coverage.md CR03-001b-60.

## CR03-001b-61 - Reef Maw bite cue and damage observation

Reviewed GameMode 4933-4978 with baseline-matching bytes. No new confirmed actionable finding; RF01 queued. Target lifetime, cue presence latches, sampled bite distance, aggregate damage/range/survival gates and timeout termination inspected. This block does not attribute damage to the Maw or prove client cue replication/rendering; runtime timing, unrelated damage interference and fixture cleanup remain unverified. Full verification deferred; see coverage.md CR03-001b-61.

## CR03-001b-62 - Discovery descriptor, privacy and duplicate-claim fixture

Reviewed GameMode 4980-5091 with baseline-matching bytes. Confirmed test-coverage finding RF02: the client privacy probe reads the remote client's own pawn instead of the entitled host pawn replica, so host feedback leakage can pass the runner's client gate. No current runtime leak found; discovery feedback fields are presently owner-only. The server stage-one actor wait is bounded by the outer 90-second runner, which retains logs. No runtime execution; see coverage.md CR03-001b-62.

## RF02 — Discovery peer privacy probe checks the local pawn (P2)

- **Source review:** CR03-001b-62 and CR03-001b-64.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:736-750`, `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4992-4999, 5201-5290`, `Scripts/Tests/Progression/DiscoveryPeer.ps1:23-28`, and `Scripts/Tests/Progression/ExplorationRewards.ps1:27-41`.
- **Trigger:** A host's discovery feedback, serial, label, or reward inventory is accidentally replicated to a remote client while the peer checks still pass.
- **Evidence:** In DiscoveryPeer, GameMode assigns the listen-server pawn to Entitled and the other player to Remote. The client helper executes only for `IsLocallyControlled()` and inspects that local pawn once at four seconds; it never checks the nonlocal host replica, and the runner accepts the one-time marker independently of claim completion. In the M9 fixture, the server checks Host and Remote components and prints `OwnerOnlyState=1`, but the runner's client gate checks only world-generation identity. It never checks the Host pawn's replicated discovery feedback or reward inventory on the Remote client. Current discovery-feedback and inventory fields use `COND_OwnerOnly`, so no live leak is established.
- **Impact:** A regression exposing another player's private discovery feedback or reward inventory can escape the peer coverage.
- **Proposed repair:** Extend the Development-only client probe and both peer runners to inspect the nonlocal Host replica after the relevant claims, synchronized or polled through fixture completion. Keep the server-side distance, forgery, duplicate, independent-player, owner-feedback, and exact-reward assertions.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** DiscoveryPeer and M9 peer checks confirm the Host retains expected feedback and reward while Remote sees no Host feedback, serial, label, or private reward inventory. A test-only exposed-field regression must fail.
- **Focused validation:** Run `Scripts/Tests/Progression/DiscoveryPeer.ps1` and `Scripts/Tests/Progression/ExplorationRewards.ps1` after an Editor build; verify a test-only exposed-field regression fails for the non-owner.
- **Additional evidence — CR04-011:** `KalmalaOceanDiscoveryDisembarkPeerTest.cpp:498-609` and `KalmalaOceanIntegratedReconnectPeerTest.cpp:81-170` have the same late-join privacy-probe gap. Their server-side late-join checks inspect the joining player's own inventory and discovery feedback; the late-join client checks also inspect only its locally controlled pawn. Neither path checks the original host/helm pawn replicas on that client after the owners hold discovery feedback and reward inventory. The `RewardLeak=0` markers therefore cannot catch a regression that exposes an original owner's state. Those production fields remain `COND_OwnerOnly`, so no live leak was observed. This is evidence for RF02's existing remote-client privacy repair, not a separate finding.

## CR03-001b-63 - M9 exploration reward peer setup and descriptor staging

Reviewed GameMode 5093-5184 against the baseline-matching source. No new confirmed actionable finding. The Development-only fixture selects a canonical server-built descriptor and calls an actor initializer that checks authority. The one-tick bounded scan cost, ordinary population activation/collision behavior, peer replication, teleport safety, and cleanup after the later stages were not exercised. Full verification deferred; see coverage.md CR03-001b-63.

## CR03-001b-64 - M9 exploration reward claims and peer privacy checks

Reviewed GameMode 5187-5296. Confirmed additional test-coverage evidence for RF02: the server checks owner state, but the M9 peer runner does not inspect the Host's replicated feedback or reward inventory on the Remote client. Current fields use `COND_OwnerOnly`; no live leak is established. Full verification deferred; see coverage.md CR03-001b-64.

## CR03-001b-65 - Living player pawn source and deterministic ordering

Reviewed GameMode 5298-5362 against the baseline-matching source and `docs/monster-spawning.md` S04. No new confirmed actionable finding; authority, eligibility filters, deterministic ordering, and pointer deduplication match the documented contract. Runtime lifecycle transitions were not exercised. Full verification deferred; see coverage.md CR03-001b-65.

## CR03-001b-66 - Loaded population-key union geometry and ordering

Reviewed GameMode 5364-5425 against `docs/monster-spawning.md` S05 and the focused automation source. No new confirmed actionable finding; the query returns the deterministic inclusive horizontal intersection of living-player radii with already active spatial keys and leaves activation state unchanged. The automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-66.

## CR03-001b-67 - Per-key cadence initialization and inactive scheduler-state cleanup

Reviewed GameMode 5427-5467 against `docs/monster-spawning.md` S06 and the cadence automation source. No new confirmed actionable finding; authority guards, stable per-key initial phases, and removal of inactive due/cursor/ordinal state match the documented cadence. The focused automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-67.

## CR03-001b-68 - Due population-key filtering and deterministic ordering

Reviewed GameMode 5469-5490 against `docs/monster-spawning.md` S06 and the cadence automation source. No new confirmed actionable finding; due-key eligibility is based on active scheduler membership and elapsed due time, and results are lexicographically ordered independently of TSet iteration. The focused automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-68.

## CR03-001b-69 - Server-authoritative due-key service and cadence advance

Reviewed GameMode 5492-5508 against `docs/monster-spawning.md` S06 and the cadence automation source. No new confirmed actionable finding; service requires an authoritative active key with an elapsed due time, then advances its due time by the five-second interval. The focused automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-69.

## CR03-001b-70 - Server-authoritative bounded population scheduler wake and attempt gates

Reviewed GameMode 5510-5575 against `docs/monster-spawning.md` S06/S07, scheduler tuning assertions, and the focused budget automation source. No new confirmed actionable finding; due work is re-based and bounded to four keys/sixteen candidates, serviced keys receive one ordinal, and only attempts surviving the ordered gates reach the server transaction. The focused automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-70.

## CR03-001b-71 - Configured environmental-spawner candidate admission

Reviewed GameMode 5577-5696 against `docs/monster-spawning.md` S08/S14/S28/S29, the environmental-spawner policy helpers, and focused policy/admission automation source. No confirmed actionable finding; the current server-side seam enforces explicit finite-slot configuration, due-time and global/source budgets, stable depleted-slot identity, candidate radius/key consistency, ordered gates, and transaction rechecks. No production call site or runtime source actor/registry exists. **Open question:** confirm a future authoritative scheduler advances `Check.CheckOrdinal` once per serviced check; this function uses but does not advance it, so otherwise identical fixed candidates in the same slot/key reuse the seeded roll. This is an integration question, not a confirmed active defect. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-71.

## CR03-001b-72 - Authoritative horizontal 32–96 m spawn-ring predicate

Reviewed GameMode 5698-5736 against `docs/monster-spawning.md` S08/S09, the ring selector and player-distance gate call paths, and focused boundary automation sources. No new confirmed actionable finding; the predicate enforces inclusive horizontal 32–96 m membership for at least one valid same-world living player, while S09's 36 m all-player exclusion remains a separate later gate. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-72.

## CR03-001b-73 - Bounded authoritative population ring candidate selection

Reviewed GameMode 5738-5791 against `docs/monster-spawning.md` S08/S14, its scheduler caller and later gate/creation path, and the focused ring-selector automation source. No new confirmed actionable finding; the selector bounds work, rotates its per-key cursor on every examined descriptor, retains each attempt and seeded-roll result, and filters candidates through the authoritative ring predicate. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-73.

## CR03-001b-74 - Server-authoritative population ring and all-player distance gate

Reviewed GameMode 5793-5818 against `docs/monster-spawning.md` S09/S14/S15, the ring/exclusion predicates and gate evaluator, and focused player-distance automation source. No new confirmed actionable finding; every retained attempt requires the inclusive ring and at least 36 m from every valid current-world living player, while earlier scheduler or roll failures remain the recorded reason. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-74.

## CR03-001b-75 - Server-authoritative horizontal player-exclusion predicate

Reviewed GameMode 5820-5854 against `docs/monster-spawning.md` S09/S15, both call sites and focused player-distance automation source. No new confirmed actionable finding; the predicate rejects any valid current-world player within 36 m using horizontal distance, while exactly 36 m passes and ring membership remains a separate requirement. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-75.

## CR03-001b-76 - Species-habitat gate result and candidate filtering

Reviewed GameMode 5856-5870 against `docs/monster-spawning.md` S13–S15, `docs/biome-enemies.md`, the shared species/habitat predicate, both callers and focused automation sources. No new confirmed actionable finding; every retained attempt receives the habitat result without replacing an earlier rejection, and candidates use the same predicate for filtering. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-76.

## CR03-001b-77 - Seeded species derivation and hard-enemy start exclusion

Reviewed GameMode 5872-5898 against S15, the species rules, mapping/classification helpers, all output-parameter callers, and focused hard-enemy start-exclusion assertions. No new confirmed actionable finding: the shared gate is authoritative and requires valid world/configuration, and the documented 150 m exclusion is applied to all six hard-enemy archetypes with an inclusive boundary. Automation was inspected but not run. Full verification deferred; see coverage.md CR03-001b-77.

## CR03-001b-78 - Authoritative dry-land surface gate and Wolf formation sampling

Reviewed GameMode 5900-6008 against S20/S22c, the three-member Wolf contract, final placement wiring, and `KalmalaPopulationLandSurfaceTest`. The production path samples the anchor and all three generated member positions and rejects any failing surface. Confirmed RF03: the test's positive array has three booleans although its assertion describes four positions; it tests only the generic all-of helper and does not exercise descriptor-level member enumeration. No current spawn defect was found. Automation was inspected but not run; full verification is deferred.

## RF03 - Wolf land-surface test omits one required pack location (P3)

- **Source review:** CR03-001b-78.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5928-5953`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationLandSurfaceTest.cpp:39-52`, and `Source/KalmalaGameplay/Public/KalmalaWolfPackContract.h:15`.
- **Trigger:** A future regression omits a generated Wolf member's surface result while the focused land-surface automation remains green.
- **Evidence:** `MemberCount` is three, and `DoesPopulationDescriptorPassLandSurface` samples the pack anchor plus each of those three member positions. The test's success array contains only three booleans despite claiming to cover the center and all three members. It passes that array directly to the generic all-of helper; its descriptor-level call only checks an out-of-bounds non-Wolf descriptor. The current production loop checks all three members, so no present spawn defect or live bad placement was observed.
- **Impact:** The focused regression test can miss a future omission of one required Wolf surface check.
- **Proposed repair:** Cover four positions (anchor plus all three generated members) and exercise the pack composition path. Add a focused rejection assertion for each member's failed surface.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** The land-surface automation verifies all four contributions and rejects the pack when any one member's surface fails through the pack placement path.
- **Focused validation:** Run `Kalmala.Gameplay.Population.LandSurface` after an Editor build.

## RF04 - Reef Maw whole-body surface loop lacks descriptor-path test (P3)

- **Source review:** CR03-001b-79.
- **Affected files:** Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6010-6101, Source/KalmalaGameplay/Private/Tests/KalmalaPopulationAquaticSurfaceTest.cpp:93-102, and Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPhysicalPlacementGateTest.cpp:24-49.
- **Trigger:** A regression omits or misclassifies a center/footprint sample in DoesPopulationDescriptorPassAquaticSurface while its isolated sphere-geometry helper remains correct.
- **Evidence:** Production loops over the sampled Reef Maw sphere footprint and combines the center swimming check with per-sample Ocean and terrain-clearance checks. The aquatic test directly covers the geometry helper, but its only descriptor-level call uses an out-of-bounds position, so it returns before species evaluation and the footprint loop. The combined physical-placement test uses a NaN descriptor and likewise does not exercise valid aquatic placement. No test currently calls the descriptor-level predicate with valid in-bounds generated Ocean data.
- **Impact:** Focused automation can pass if Reef Maw full-body water placement stops rejecting an invalid center or footprint sample; an unsuitable natural spawn could then be admitted.
- **Proposed repair:** Add deterministic valid Reef Maw descriptors that pass the full-body check, plus cases where the center or a footprint sample fails its Ocean, depth or terrain requirement. Exercise the descriptor-level predicate or final placement composition while retaining direct helper math coverage.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** Kalmala.Gameplay.Population.AquaticSurface covers valid descriptor-level admission and rejects a failing center and footprint sample through the full policy path.
- **Focused validation:** Run Kalmala.Gameplay.Population.AquaticSurface after an Editor build.

## RF05 - Snake roof clearance traces only the actor center (P3)

- **Source review:** CR03-001b-80.
- **Affected files:** Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6217-6235 and 6583-6636; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3645-3692; Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp:416-435; Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRoofClearanceTest.cpp:130-169.
- **Trigger:** A snake spawns with its uppermost capsule point beneath a solid RoofKit that does not cover the actor-origin XY.
- **Evidence:** DoesPopulationDescriptorPassRoofClearance gets one BodyTop and traces upward from that point. TryGetPopulationBodyTop computes the maximum active-shape Z but returns ActorLocation.X/Y. The actual Viper, Giant Snake and Death Adder capsule profile has RelativeLocation (-22, 0, 0), radius 16/38/21 cm and half-height 92/210/112 cm. With zero actor rotation, the actual capsule top is offset from the actor origin, but the trace is not. The RoofKit collision half-extents are 132 x 132 x 16 cm, so it can cover the actual top while excluding the actor origin. If its lower face is just above the body top, the separate body-overlap check finds no collision and the combined S25 placement gate can admit the spawn. The existing roof automation tests centered traces and body-top Z, not this offset descriptor case.
- **Impact:** A snake can spawn beneath a solid construction roof at a roof edge, contrary to S22b.
- **Proposed repair:** Preserve the actual world-space top positions for each active collision shape and trace from them for each descriptor, including each Wolf member. Add a deterministic RoofKit fixture over an offset snake capsule top but outside the actor-origin trace.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** A valid snake descriptor is rejected when a RoofKit covers the capsule top but misses actor-origin XY; no roof and a roof beyond 24 m remain allowed. Wolf member roof checks remain per member.
- **Focused validation:** Run Kalmala.Gameplay.Population.RoofClearance after an Editor build.

## RF06 - Valid wildlife body-clearance composition lacks a test (P3)

- **Source review:** CR03-001b-80.
- **Affected files:** Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6103-6169; Source/KalmalaGameplay/Private/Tests/KalmalaPopulationBodyClearanceTest.cpp:281-286; Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPhysicalPlacementGateTest.cpp:24-49.
- **Trigger:** A regression in full descriptor body validation denies valid wildlife or skips an actor/member shape while helper-level support and overlap assertions remain green.
- **Evidence:** KalmalaPopulationBodyClearanceTest calls DoesPopulationDescriptorPassBodyClearance only for an out-of-bounds descriptor and a nonfinite descriptor. It tests the profile, placement, bounds and overlap helpers separately but never asserts that a valid descriptor passes the composition or that a blocker fails through it. The physical-placement test supplies a nonfinite descriptor and stops before a valid body-clearance decision.
- **Impact:** The focused automation can miss a fail-closed regression that suppresses wildlife or a composition regression that omits required body-shape checks.
- **Proposed repair:** Add deterministic valid land descriptors that pass without blockers and reject a blocker intersecting an active body shape through the full predicate; cover all generated Wolf member descriptors.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** Kalmala.Gameplay.Population.BodyClearance proves valid admission and body-overlap rejection through DoesPopulationDescriptorPassBodyClearance, including Wolf member placement.
- **Focused validation:** Run Kalmala.Gameplay.Population.BodyClearance after an Editor build.

## CR03-001b-81 - Population movement support and station/camp spacing

Reviewed GameMode 6241-6415 against S22c/S23/S24/S25 and the focused station, camp, land-surface, aquatic-surface and physical-placement test sources. The production S22c path routes land and Reef Maw descriptors to their documented S20/S21 checks, constructs all three Wolf member descriptors, and evaluates existing species movement support at each proposed actor position. Station and camp source eligibility and inclusive geometry match the documented S23/S24 policy. No current production defect was found. Confirmed RF07: no test calls the descriptor movement-support composition or shared position-support helper, and the combined placement fixture uses a NaN descriptor that exits before valid movement support. Static inspection only; automation and runtime behavior were not exercised. Full verification deferred.

## RF07 - Movement-support descriptor composition lacks a focused test (P3)

- **Source review:** CR03-001b-81.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6241-6310`, `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:848-916`, and `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPhysicalPlacementGateTest.cpp:24-76`.
- **Trigger:** A future regression routes a species through the wrong S20/S21 support path, omits an actual actor or Wolf member position, or drops a documented biome, water, traversability or snake-leash constraint while surface and fail-closed coverage remains green.
- **Evidence:** `DoesPopulationDescriptorPassMovementSupport` composes S20 land or S21 Reef Maw surface support and then calls `IsPopulationSpawnPositionSupported` for each proposed actor location; Wolf descriptors are expanded to all three members. A repository-wide search found no test calls to the descriptor movement gate or shared support helper. The physical-placement gate test uses a NaN location and returns before valid support evaluation. Existing S20/S21 tests exercise surface predicates separately, not the S22c descriptor composition. The current production path follows the documented policy; no live placement defect was observed.
- **Impact:** Focused automation can miss a regression that admits unsupported natural wildlife or rejects supported wildlife through the server-side movement gate.
- **Proposed repair:** Add deterministic valid and rejected land and Reef Maw descriptor cases through the movement-support gate or final placement composition. Cover the species-specific biome, water, traversability and snake-leash rules and verify every actual Wolf member location.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** `Kalmala.Gameplay.Population.MovementSupport` proves supported descriptor admission and rejects unsupported actual actor positions through S22c composition, including the aquatic route and all generated Wolf members.
- **Focused validation:** Run `Kalmala.Gameplay.Population.MovementSupport` after an Editor build.

## CR03-001b-82 - Physical placement composition and support transforms

Reviewed GameMode 6416-6582 against S22a/S25, the server creation transaction, shared body profiles, and the body-clearance, station-cylinder, spherical-suppression and physical-placement tests. The final placement predicate composes movement, body and roof clearance, then evaluates station and camp suppression at each proposed actor origin; Wolf packs expand to all three deterministic members. The support-offset formula matches the sphere/capsule collision geometry and zero-rotation creation path, with focused assertions for sloped support, capsule offset, Mosquito height and Reef Maw depth. No production defect was found. Confirmed RF08 (P3): the final S25 predicate has no valid-descriptor test; the existing combined-gate fixture uses NaN and never reaches component or source-suppression evaluation. Static inspection only; full verification deferred.

## RF08 - S25 camp suppression lacks Wolf-member composition coverage (P3)

- **Source review:** CR03-001b-82.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6416-6480`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPhysicalPlacementGateTest.cpp:24-76`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationStationCylinderTest.cpp:35-79`, and `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSphericalSuppressionTest.cpp:45-142`.
- **Trigger:** A future regression omits station/camp suppression from the final S25 result or checks only the Wolf pack descriptor/anchor while all point-query tests remain green.
- **Evidence:** The production S25 predicate builds all three deterministic Wolf member descriptors, derives each proposed actor origin and evaluates both the station cylinder and eligible camp sphere for each one. Existing station and camp tests directly exercise the point predicates. `KalmalaPopulationPhysicalPlacementGateTest` is the only test call to `DoesPopulationDescriptorPassPhysicalPlacement`, and its descriptor contains NaN, so it returns before the component and source-suppression checks. No valid final-gate result is asserted. Current production composition matches S25; no live spawn defect was observed.
- **Impact:** Focused automation can miss a future server-side regression that admits a Wolf member within a suppressed station or camp volume.
- **Proposed repair:** Extend the physical-placement gate automation with a deterministic valid Wolf pack that passes without suppression sources. Add a Workbench/Forge and an eligible Camp Bed/source at each member origin in turn while keeping the pack anchor outside the source field, and assert rejection through the final S25 predicate.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** `Kalmala.Gameplay.Population.PhysicalPlacementGate` proves the valid no-source Wolf pass and rejects station and eligible camp-source overlap at every actual member origin while preserving earlier rejection-order coverage.
- **Focused validation:** Run `Kalmala.Gameplay.Population.PhysicalPlacementGate` after an Editor build.

## CR03-001b-83 - Body tops, roof intersection, collision overlap and bounds

Reviewed GameMode 6583-6771 against the shared active collision profiles, S22a/S22b/S25, and the focused body-clearance and roof-clearance tests. Body-top calculation, RoofKit query filtering, sphere/capsule overlap transforms and horizontal world-bound extents match their contracts; focused tests cover these helpers. No new production defect or distinct test gap was found. The actor-origin XY used with maximum body-top Z is already recorded as RF05 from CR03-001b-80. Static inspection only; full verification deferred.

## CR03-001b-84 - Population time gate and live-creature registry

Reviewed GameMode 6772-6895 against S10/S16 and the gate/registry tests. Time remains unrestricted for all current species, unknown archetypes fail closed, and the live count uses current actor positions with inclusive horizontal radius and server-side lazy pruning. No production defect was found. Confirmed RF09 (P3): the registry test covers explicit defeat and EndPlay unregistration, but does not exercise the count query's lazy removal of invalid or defeated weak entries. Static inspection only; full verification deferred.

## RF09 - Lazy live-creature registry pruning lacks focused coverage (P3)

- **Source review:** CR03-001b-84.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6829-6864` and `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationLiveCreatureRegistryTest.cpp:58-84`.
- **Trigger:** A regression removes the lazy cleanup in `CountLivePopulationCreaturesWithinRadius` while the explicit defeat and EndPlay unregistration tests still pass.
- **Evidence:** The count query removes invalid, foreign-world, non-authoritative, defeated and dead weak entries. The focused registry test verifies duplicate registration, inclusive current-position counts, roaming, and immediate removal on defeat/EndPlay; those lifecycle paths remove entries before a later count can exercise the query's pruning branch. No test injects a stale invalid or defeated weak entry and verifies lazy cleanup.
- **Impact:** A missed lifecycle removal can leave stale weak handles in the long-running server registry, increasing the registry size and the work of later count queries; invalid entries are currently skipped rather than counted.
- **Proposed repair:** Extend `Kalmala.Gameplay.Population.LiveCreatureRegistry` with invalid and defeated weak entries inserted without lifecycle unregistration, then query the count and assert both entries are removed while healthy live actors still count at their current positions.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** The focused automation proves the count query prunes invalid and defeated weak entries without changing inclusive-radius/current-position count semantics.
- **Focused validation:** Run `Kalmala.Gameplay.Population.LiveCreatureRegistry` after an Editor build.

## CR03-001b-85 - Population area-cap counting and admission

Reviewed GameMode 6896-7047 against S11/S19, area-cap and Wolf-reservation automation, and the gate evaluator. The implementation counts valid living actors and pending reservations against world, home-key and aggregate budgets, and current same-species positions against inclusive area caps. Current species limits and rejection order match the contracts; focused tests cover boundaries, movement, budgets and reservations. No production defect or distinct test gap was found. Static inspection only; full verification deferred.

## CR03-001b-86 - Population time, weather, progression, and cap gates

Reviewed GameMode 7048-7198 against S13/S16-S19 and the configured-source S27-S29 policy. All 17 current archetypes pass the intentionally unrestricted time, weather and progression policies; unknown archetypes fail closed. Natural and configured-source paths apply gates in order, and shared/source-local cap checks include pending reservations and whole-pack Wolf capacity. Focused evaluator, cap and configured-admission tests cover the relevant policies and rejection behavior. No production defect or distinct test gap was found. Static inspection only; full verification deferred.

## CR03-001b-87 - Physical-placement dispatch, unload/reactivation, and source caps

Reviewed GameMode 7199-7422 against S10/S26 and S27-S29. Physical-placement dispatch preserves rejection order and filters failed candidates. Unload and reactivation match the inclusive 96 m player radius and session-only 30 s cooldown contract; source-local cap sampling combines mapped live actors and pending reservations and applies the inclusive 24 m limit. No production defect found. Confirmed RF10 (P3): lifecycle coverage uses one player and does not verify unload composition with multiple living players. Confirmed RF11 (P3): source-cap policy and reservation tests do not exercise the successful source-owned live-actor mapping path. Static inspection only; full verification deferred.

## RF10 - Population unload lacks multiple-player composition coverage (P3)

- **Source review:** CR03-001b-87.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7232-7305`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationCreationLifecycleTest.cpp:56-112`, and `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationLivingPlayersTest.cpp:88-98`.
- **Trigger:** A regression unloads a roaming actor while at least one other living player remains within its inclusive 96 m horizontal radius, despite the existing one-player boundary and living-player selection tests passing.
- **Evidence:** `UpdatePopulationCreatureUnloads` retains the actor when any valid same-world authoritative living player is within range. `KalmalaPopulationCreationLifecycleTest` verifies exact-radius retention, 1 cm outside unload, cooldown and reload with one player. `KalmalaPopulationLivingPlayersTest` verifies that two living players are returned but does not pass them through the unload behavior. No production defect was found.
- **Impact:** A regression in the multi-player composition could unload a creature still within one player's neighborhood. Current implementation correctly checks all supplied players.
- **Proposed repair:** Extend `Kalmala.Gameplay.Population.CreationLifecycle` with two living players, one near and one far, and verify the actor stays loaded; then move both outside and verify unload and cooldown behavior.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** The focused automation proves the actor remains loaded when any living player is at or within 96 m, and unloads only after all living players move outside.
- **Focused validation:** Run `Kalmala.Gameplay.Population.CreationLifecycle` after an Editor build.

## RF11 - Source-owned live actors lack local-cap adapter coverage (P3)

- **Source review:** CR03-001b-87.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7348-7419`, `Source/KalmalaGameplay/Private/Tests/KalmalaEnvironmentalSpawnerPolicyTest.cpp:45-91`, and `Source/KalmalaGameplay/Private/Tests/KalmalaEnvironmentalSpawnerAdmissionTest.cpp:145-212`.
- **Trigger:** A regression omits or assigns a live actor to the wrong environmental source in `EnvironmentalSpawnerSourceIdBySpawnId` or `GetEnvironmentalSpawnerLivingSamples`, while synthetic-sample policy checks and pending-reservation admission checks continue to pass.
- **Evidence:** The sampler joins living registry actors through their registered source IDs and appends matching pending reservations before applying S29's local cap. Policy tests exercise synthetic same-source, other-source, dead, outside-radius and invalid samples; admission tests exercise pending reservations and rejection behavior, but no test creates a configured-source actor successfully and verifies that the resulting live actor is sampled for later admission. No production defect was found. Two stable source slots and shared/pending caps bound the untested path.
- **Impact:** If live ownership sampling regresses, a same-source actor can be omitted or misattributed and the local cap can make the wrong admission decision. Current code and source limits show no defect.
- **Proposed repair:** Extend `Kalmala.Gameplay.Population.EnvironmentalSpawnerAdmission` through successful configured-source actor creation, then assert a later same-source candidate within the inclusive 24 m radius counts that actor while an actor from another source is excluded.
- **Dependency:** Defer implementation until CR19 review acceptance.
- **Acceptance:** The focused test covers successful live source registration, same-source local counting, other-source exclusion and existing pending-reservation semantics.
- **Focused validation:** Run `Kalmala.Gameplay.Population.EnvironmentalSpawnerAdmission` after an Editor build.

## CR03-001b-88 - Natural population spawn transaction

Reviewed GameMode 7423-7631 against S26/S29, the natural and configured call sites, wildlife initialization/registry callbacks, and focused population automation sources. The server transaction rechecks caps and final placement, reserves all proposed capacity before spawning, initializes and validates stable actor identity/state, cleans up partial failures, releases reservations, and only then clears cooldown and publishes source ownership. No production defect was found. Confirmed RF12 (P3): no test exercises a successful natural population transaction or partial Wolf-pack creation rollback. Static inspection only; full verification deferred.

## CR03-001b-89 - Population progression/cap admission and reservations

Reviewed GameMode 7632-7819 against S11/S12/S19/S26/S29 and the focused gate-evaluator, area-cap, Wolf-reservation and environmental-admission tests. The server-only progression gate resolves known species and fails unknown archetypes closed. Non-Wolf admission counts live actors and pending reservations; Wolf cap evaluation atomically trial-reserves its three deterministic member locations and releases that token. The shared reservation transaction validates inputs, accounts for each provisional member and source-local pending capacity, and removes the complete reservation on any failure. No production defect or distinct focused-test gap was found. Static source/document/test-source inspection only; full verification deferred.

## RF12 - Natural population creation and Wolf rollback lack transaction coverage (P3)

- **Source review:** CR03-001b-88.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7423-7630`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationCreationLifecycleTest.cpp:20-104`, and `Source/KalmalaGameplay/Private/Tests/KalmalaEnvironmentalSpawnerAdmissionTest.cpp:145-212`.
- **Trigger:** A regression skips or corrupts actor initialization, stable-ID validation, live registration, defeat-callback binding, reservation release or partial Wolf-pack rollback while the existing gate, cap, rejection and manual actor-lifecycle tests still pass.
- **Evidence:** The creation-lifecycle automation calls `TryCreatePopulationSpawnTransaction` only with default failing gate results, then manually spawns and initializes a Rat for unload/reload checks. Configured-source admission tests cover disabled, full-local-cap and defeated-slot rejection, not a successful transaction. Wolf-reservation automation tests capacity policy without creating a pack through this transaction. No focused test reaches successful `TryCreatePopulationSpawnTransaction` execution or a failure after one Wolf member has been created. Static review found no production defect.
- **Impact:** Natural admission could regress by failing to create or correctly register an eligible actor, leaking capacity, or leaving a partial pack; the current transaction contains rollback logic, but these outcomes lack a focused regression guard.
- **Proposed repair:** Extend `Kalmala.Gameplay.Population.CreationLifecycle` with a fully passing natural single-actor transaction and a successful three-member Wolf transaction. Add a controlled later-member failure (using a test-only spawn hook if needed) to exercise rollback after partial creation.
- **Dependency:** Defer implementation until CR19 review acceptance; no repair belongs in this review run.
- **Acceptance:** Successful single and Wolf attempts produce the expected initialized stable IDs, live registry entries and defeat callbacks, then release all pending reservations. Injected partial Wolf failure leaves no new actors or registry entries, pending reservations, defeated IDs or reward/save callbacks.
- **Focused validation:** Run `Kalmala.Gameplay.Population.CreationLifecycle` after an Editor build.

## CR03-001b-90 - GameMode tick and population scheduler service

Reviewed GameMode 7820-7931 against the S06/S07 server scheduler contract, constructor tick enablement, the population scheduler helpers and focused cadence/budget automation. The authoritative tick guard precedes server services; terrain-key activation is followed by unload update, cadence refresh and bounded wake service. The current code matches the documented order and intervals; no production defect was found. Confirmed RF13 (P3): focused tests exercise cadence and wake helpers directly but do not assert their integration through the real GameMode `Tick`. Static source/document/test-source inspection only; full verification deferred.

## RF13 - Population scheduler Tick integration lacks focused coverage (P3)

- **Source review:** CR03-001b-90.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7820-7930`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerCadenceTest.cpp:12-130`, and `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerBudgetTest.cpp:12-69`.
- **Trigger:** A regression removes or misorders `UpdatePopulationKeySchedule` and `UpdatePopulationSchedulerWake` in the real GameMode tick while the focused helper tests continue to pass.
- **Evidence:** `AKalmalaGameMode` enables actor ticking. Its `Tick` validates authority, world and generation configuration, refreshes terrain/key activation, then calls `UpdatePopulationKeySchedule` before `UpdatePopulationSchedulerWake`. The cadence test calls the schedule helper directly and the budget test calls the wake helper directly; neither invokes or asserts the GameMode tick integration. Other test files that tick worlds do not assert population scheduler state. The existing runtime order matches the contract; no production defect was found.
- **Impact:** A future wiring regression could stop or delay natural wildlife scheduler service while direct cadence and budget tests still pass.
- **Proposed repair:** Extend the scheduler automation with a controlled server fixture that advances the real GameMode tick and checks schedule-to-wake integration, deadline behavior and the existing bounded budgets.
- **Dependency:** Defer implementation until CR19 review acceptance; no repair belongs in this review run.
- **Acceptance:** A focused test observes a due key being scheduled and serviced through actual `Tick`, verifies one wake remains within the key/candidate limits, and proves no extra wake occurs before the next deadline.
- **Focused validation:** Run `Kalmala.Gameplay.Population.SchedulerCadence` and `Kalmala.Gameplay.Population.SchedulerWakeBudget` after an Editor build.

## CR03-001b-91 - Ocean travel feedback and weather peer fixtures

Reviewed GameMode 7932-8087 against the owner-only travel-feedback contract, server-authoritative weather and skiff movement, the ocean travel automation sources, and M8 peer-validation guidance. The listen-server fixture publishes distinct host/remote feedback, checks server values and serials, and the integrated runner requires both owners' local messages plus no cross-owner feedback on the remote client. The weather fixture selects a server crosswind and calm state; both peers verify replicated pressure, and the remote client attempts an unauthorized local weather mutation that must leave the accepted state unchanged. The travel runners require these results and reject a client-received forged cycle. No production defect or distinct focused-test gap was found. Static inspection only; full verification deferred.

## CR03-001b-92 - Ocean journey fixture and world-profile readiness

Reviewed GameMode 8088-8307 against the M8 server-authoritative travel contract, the generated route/streaming limits, the bounded world-profile procedure, and both peer-runner acceptance checks. The fixture validates a generated 2.4 km ocean corridor and launch collision, assigns helm/passenger seats on the server, accepts bounded helm steering through the existing RPC, and requires a moored in-bounds stop across a patch transition with bounded active patches and observed weather states. The profile snapshot is scheduled only after two controllers are present, waits three seconds, and serializes the existing population save to memory; the runner requires the client to adopt seed 418 and the server to report two players and serialization success. No production defect or distinct focused-test gap was found. Static inspection only; full verification deferred.

## CR03-001b-93 - Active-key wildlife, harvest, and hazard materialization

Reviewed GameMode 8308-8478 against the active-key population, harvest/depletion, wildlife scheduler, and permanent hazard-defeat contracts. Server activation derives stable descriptors, applies biome budgets and the salt/opals exception, skips saved or session-depleted sources, binds the existing callbacks, keeps direct wildlife activation behind the non-shipping verification flag, and cleans partial Wolf packs. Hazard actors restore their stable identity and production defeat callback, and defeated IDs are checked before materialization. No production defect was found. Confirmed RF14 (P3): tests cover hazard defeat eligibility and generic save serialization separately, but not the GameMode materialization → accepted defeat/save callback → reactivation suppression chain. `docs/02-technical-architecture.md:614` retains future-tense wording for the wildlife/hazard ledger path; the current `docs/monster-spawning.md` S02/S26 mapping describes implementation, so this is recorded as a documentation question, not a behavior defect. Static inspection only; full verification deferred.

## RF14 - Generated hazard materialization and permanent depletion lack integration coverage (P3)

- **Source review:** CR03-001b-93.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8460-8473`, `Source/KalmalaGameplay/Private/Tests/KalmalaInteractionAuthorityTest.cpp:254-264`, `Source/KalmalaWorld/Private/Tests/KalmalaWorldPlayerStartResolverTest.cpp:232-279`, and `Source/KalmalaWorld/Private/Tests/KalmalaFiniteWorldTest.cpp:34-35`.
- **Trigger:** A regression drops the GameMode's generated-hazard initialization, stable-ID callback binding, persistent defeat write, or saved-ID filter while the independent actor-eligibility, descriptor-bounds, and save-container checks continue to pass.
- **Evidence:** `ActivatePopulationKey` skips hazard descriptors found in the population save's defeated set, spawns `AKalmalaHazardSpawn`, initializes its deterministic ID, and binds `OnDefeated` to `RecordDefeatedSpawn`. `Kalmala.Gameplay.HazardSpawn.ServerOnlyDefeat` tests only the eligibility predicate. `Kalmala.World.PopulationSaveGame.SparseDeltas` round-trips a literal hazard ID, and `Kalmala.World.FiniteWorld` checks that hazard descriptors stay in bounds. None invokes GameMode activation, follows a generated actor's accepted defeat through the bound save callback, or verifies that the defeated slot remains absent after reactivation. The code matches the documented permanent-depletion contract; no production defect was observed.
- **Impact:** A future wiring regression could let a defeated generated hazard return after reconnect/restart, or fail to persist an accepted defeat, without failing the current focused checks.
- **Proposed repair:** After CR19 acceptance, add a focused GameMode key-activation automation with a deterministic key containing multiple hazard descriptors. Verify stable actor IDs and the bound production callback; use an isolated/in-memory writer seam to assert an accepted server defeat writes only its exact ID; then activate a fresh GameMode with that save and require the defeated hazard to remain absent while an undefeated neighbor materializes. Keep the fixture out of user save slots.
- **Dependency:** Defer implementation until CR19 review acceptance; no repair belongs in this review run.
- **Acceptance:** The focused automation covers expected hazard actor materialization, stable descriptor identity, production `OnDefeated` binding and exact sparse save mutation, and fresh activation suppression of only the defeated ID.
- **Focused validation:** Run the new focused GameMode hazard-materialization test after an Editor build.

## CR03-001b-94 - Active-key discovery and M9 reward materialization

Reviewed GameMode 8479-8622 against the first-wave discovery, M9 land-reward, active-key, persistence and authority contracts. Generated POI/scroll actors and biome-specific harvest discovery nodes use server-derived descriptors; nodes skip saved depletion and bind the production callback. The M9 catalogue supplies at most one deterministic candidate per eligible key, and the replicated actor accepts only server initialization. Claim handling revalidates the canonical descriptor and active cell, authenticated owner, range, saved/session duplicate state and exact reward transaction. Existing catalogue and peer checks cover descriptor validity, placement, forged/distant/replayed claims, independent owners and private feedback. The peer fixture manually creates the marker, so it does not cover activation integration.

Confirmed RF15 (P1): `ActivePopulationSpatialKeys` is capped at nine in `ActivatePopulationKey`, only appended to after materialization, and never removed. `Tick` keeps requesting the current key for each living player, but unseen cells are rejected after the ninth unique key. `GetPopulationSpatialKeyCandidates` iterates only the existing set, while `UpdatePopulationKeySchedule` removes only scheduler/due/cursor/ordinal entries for keys outside the player neighborhood. Therefore moving through the world eventually stops materializing population, harvest, hazard, POI/scroll, biome discovery and M9 reward actors in new cells. The focused key-union test proves only candidate-query stability; cadence tests prove scheduler-state cleanup; the M9 peer fixture manually inserts its key. No test covers rollover or new-cell activation after the cap. This conflicts with the current nine-active-key budget's use as a loaded-cell bound and prevents long-distance world exploration from continuing to receive generated content. No runtime traversal was exercised.

## RF15 - Active population keys never turn over (P1)

- **Source review:** CR03-001b-94.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5365-5450, 7922, 8311, 8619`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSpatialKeyUnionTest.cpp:42-133`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerCadenceTest.cpp:75-119`; `Scripts/Tests/Progression/ExplorationRewards.ps1:12-42`.
- **Trigger:** A server session visits nine distinct population spatial keys and a living player enters a tenth.
- **Evidence:** `ActivatePopulationKey` returns for any new key once `ActivePopulationSpatialKeys.Num()` reaches `MaxActivePopulationSpatialKeys` (9), and successful activation only adds the key. Repository-wide references show no removal from this set. The normal server tick keeps calling activation for each living pawn's current key. `GetPopulationSpatialKeyCandidates` filters only keys already in the set, and `UpdatePopulationKeySchedule` prunes the scheduler's due/cursor/ordinal maps, not the active-key set. The key-union test asserts that its query neither adds nor removes loaded keys; cadence tests cover scheduler entries only. The M9 peer fixture manually inserts its cell and spawns the actor, so it bypasses production activation.
- **Impact:** Once the ninth unique cell is reached, later cells cannot create wildlife, harvest, hazards, first-wave discoveries or M9 rewards. A small number of spatial transitions permanently disables generated content beyond those initial cells for the rest of the server session.
- **Proposed repair:** After CR19 review acceptance, add bounded active-key turnover within the existing nine-key budget. Evict keys outside the supported player neighborhood and clean up/recreate their generated actors and per-key state coherently. Preserve permanent sparse depletion, M9 per-player claims, living-creature unload cooldowns and server authority; keep actor and reservation counts bounded.
- **Dependency:** Defer implementation until CR19 review acceptance; no repair belongs in this review run.
- **Acceptance:** A focused server test activates nine keys, moves into a tenth, and verifies the new cell becomes active without exceeding nine. Returning to a prior cell must not duplicate actors or restore depleted sources/accepted claims; scheduler and living-creature unload behavior must remain bounded and correct.
- **Focused validation:** Run the new key-turnover GameMode automation after an Editor build.

## CR03-001b-95 - Spawn depletion and Rat-to-Peat Husk reward callbacks

Reviewed GameMode.cpp:8623-8819 against S02's permanent-depletion/reward-escrow rules, the world-population sparse-save contract, the M7 persistence and multiplayer gates, and the Rat/Peat Husk regression procedures. The normal callback path validates authority and world identity; reward handlers queue owner-bound claims and persist stable defeated IDs, while Peat Husk keeps its 25% Peat Heart outcome in the stable claim identity. Two failure paths are confirmed: depletion callbacks do not react to a failed population-slot write, and the Rat handler mutates the defeated ledger before reward-queue acceptance. No other production defect was found in this slice. Static inspection only; full verification deferred.

## RF16 - Failed population depletion writes are silently abandoned (P2)

- **Source:** CR03-001b-95; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8623-8655`.
- **Affected path:** The harvest and generic defeat callbacks update the in-memory population save and invoke `SaveGameToSlot`, but neither propagates nor retries a failed write. `RecordHarvestedSpawn` explicitly discards its `bSaved` result. `RecordDefeatedSpawn` discards the call result.
- **Trigger:** The population slot write fails after a server harvest has committed its inventory/player-state transaction or after an accepted hazard defeat broadcasts `OnDefeated`.
- **Impact:** The live session keeps the source depleted, but disk can retain the earlier population save. A restart before another successful population save may recreate the harvested source or defeated hazard, enabling repeated resource/reward acquisition or encounter replay.
- **Evidence:** `AKalmalaHarvestNode::CommitHarvestedStateFromServer` sets `bHarvested` and broadcasts `OnHarvested` after the accepted grant; GameMode binds this callback to `RecordHarvestedSpawn`. Hazard defeat broadcasts after server acceptance and GameMode binds `RecordDefeatedSpawn`. Both callbacks mutate the live save before writing and have no callback-level retry/rollback. The forest test writer seam returns no success value, so it cannot cover a failed write. This conflicts with `docs/monster-spawning.md` S02 permanent depletion and the sparse-save contract in `docs/02-technical-architecture.md`.
- **Proposed repair:** Make the persistence result part of a recoverable depletion transaction. Preserve a retryable server-owned delta on write failure and prevent the harvesting grant from becoming a second durable acquisition; ensure accepted defeats remain queued for durable persistence.
- **Dependencies:** After CR19 review acceptance; no repair is authorized during CR03 review.
- **Acceptance:** A focused failure-injection regression proves a failed harvest depletion cannot create a durable grant/depletion mismatch and is retried or rolled back safely; an accepted hazard defeat remains scheduled for persistence until its stable ID is durably saved. Reload checks confirm successful depletion stays absent and a failed write does not silently claim durable success.
- **Focused validation:** Run the new harvest/death persistence-failure automations and the existing forest restart regression after an Editor build; full milestone verification remains governed by `docs/07-development-setup.md`.

## RF17 - Rat reward-queue rejection leaves a false defeated ledger entry (P2)

- **Source:** CR03-001b-95; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8658-8671` and `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-491`.
- **Affected path:** `RecordRatDefeatReward` calls `MarkDefeated` before `TryQueuePendingWildlifeReward`. The queue can reject at its 4096-claim limit or when an existing stable claim belongs to a different player identity.
- **Trigger:** A valid Rat defeat callback reaches a queue rejection. The callback returns false; `DefeatServer` then exits before setting `bDefeated` or broadcasting the accepted defeat, but `PopulationSaveGame` already contains the spawn ID in `DefeatedSpawnIds`.
- **Impact:** The unaccepted live Rat remains at zero health while the shared save reports it defeated. If the actor unloads or a later unrelated save serializes that object, activation can suppress a Rat whose defeat and reward claim never completed.
- **Evidence:** In the selected handler, `MarkDefeated` precedes the queue call and its false return. `TryQueuePendingWildlifeReward` can fail on capacity or owner mismatch. `AKalmalaWildlifeSpawn::DefeatServer` returns immediately when the callback fails, before accepting the defeat. The neighboring callbacks in this slice queue before marking. The existing Rat save-model test covers queue idempotency, owner identity and serialization but does not exercise this GameMode rejection path.
- **Proposed repair:** Validate/queue the claim before mutating the defeated ledger, preferably on a candidate save that is published only after the complete callback succeeds. A rejected queue must leave the original save unchanged.
- **Dependencies:** After CR19 review acceptance; no repair is authorized during CR03 review.
- **Acceptance:** A focused GameMode regression fills/rejects the pending queue and verifies that the callback fails without adding the Rat ID to the live or serialized defeated set; a valid retry queues exactly one owner-bound claim and accepts the defeat.
- **Focused validation:** Run the new Rat GameMode callback regression and the existing Rat peer reward/reconnect regression after an Editor build.

## CR03-001b-96 - Wolf-to-Reef Maw reward callbacks

Reviewed GameMode.cpp:8820-9045 against S02's reward-escrow contract, the population sparse-save rules, the M7 creature/reward authority boundary, and the six species contracts. Each handler checks authority/world/save identity, preserves the one-time optional outcome in a stable owner-bound claim where applicable, marks the stable spawn defeated, and returns the slot-write result before wildlife defeat broadcasts. The shared actor dispatcher maps each reviewed archetype to its matching GameMode handler. No production defect was observed. Confirmed RF18 (P3): the species tests cover save/claim helpers but do not invoke these GameMode callbacks, and the live peer scenarios do not assert these species' reward claims. Confirmed RF19 (P3): the six live peer commands in development setup refer to absent per-species scripts; the current runner is EnemyPeers.ps1 with named scenarios.

## RF18 - Species defeat reward callbacks lack focused GameMode coverage (P3)

- **Source:** CR03-001b-96; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8820-9045` and `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-583`.
- **Affected path:** Wolf, Lynx, Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw defeat dispatch, authenticated owner claim creation, stable defeated-ID write and stored optional-roll outcomes.
- **Trigger:** A callback mapping or handler refactor changes a species' claim ID, owner, roll preservation, defeated mark or save order.
- **Impact:** The current source tests can still pass while production omits a species reward, assigns its claim incorrectly, or fails to preserve its stable outcome. Rat's shared end-to-end escrow case does not cover these six callbacks.
- **Evidence:** `KalmalaWildlifeWolfTest.cpp`, `KalmalaWildlifeLynxTest.cpp`, `KalmalaWildlifePolarBearTest.cpp`, `KalmalaWildlifeHornedGoatTest.cpp`, `KalmalaWildlifeThunderSentinelTest.cpp` and `KalmalaWildlifeReefMawTest.cpp` exercise the save-game claim constructors/queue helpers and reward candidates; no test calls the corresponding `Record*DefeatReward` method or `AKalmalaWildlifeSpawn::DefeatServer`. `Scripts/Tests/Combat/EnemyPeers.ps1` checks their nonlethal attack/replication scenarios; its shared reward/persistence scenario is Rat-only. The current handler code matches the documented S02 owner-bound escrow intent.
- **Proposed repair:** Add isolated GameMode/DefeatServer automation coverage for all six species, using a test save writer or isolated save root so no user slot is touched. Assert exact stable identity, authenticated owner, claim outcome and defeat-save ordering.
- **Dependencies:** After CR19 review acceptance; no repair is authorized during CR03 review.
- **Acceptance:** Each actual species dispatch creates one owner-bound claim with the exact persistent member ID, persists the matching defeated ID, and retains its optional roll on retry; rejected identity/world/queue cases create no misplaced claim.
- **Focused validation:** Run the new callback automation and the six existing species contract automations after an Editor build.

## RF19 - Wildlife peer-test docs reference missing scripts (P3)

- **Source:** CR03-001b-96; CR04-022; `docs/07-development-setup.md:2386-2407,2653-2660,2680-2783`.
- **Affected path:** The Rat shared-combat, Peat Husk sweep, Wolf, Lynx, Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw live multi-peer command examples.
- **Trigger:** A developer follows any of the eight documented standalone `*Peer.ps1` commands.
- **Impact:** The referenced file does not exist, so the documented live acceptance command fails before launching its peer scenario.
- **Evidence:** `Scripts/Tests/Combat` contains only `EnemyPeers.ps1`. Its `-Scenario` set includes `SharedCombat`, `PeatHuskSweep`, `WolfPack`, `LynxLeap`, `PolarBearCharge`, `HornedGoatPush`, `ThunderSentinelSlam` and `ReefMawBite`. The current Rat and Peat Husk setup sections also point at absent `RatPeer.ps1` and `PeatHuskPeer.ps1` files; the existing composite runner contains matching scenarios for both.
- **Proposed repair:** Replace all eight standalone script paths with `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario <matching-name>` and preserve each scenario's authored world seed.
- **Dependencies:** After CR19 review acceptance; this is an audit handoff only and does not authorize documentation repair during CR04.
- **Acceptance:** All eight setup commands reference the existing runner and a valid matching scenario while retaining their authored seed.
- **Focused validation:** Run `Scripts/Tests/Combat/EnemyPeers.ps1 -ListScenarios` and parser-check the updated setup instructions.

## RF20 - Mireling gem claim prefix is not fully removed (P2)

- **Source:** CR03-001b-97; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9104-9118, 12034-12050.
- **Affected path:** A generated Mireling's optional Dusk Diamond claim, queued for the authenticated credited attacker and retried from the authoritative GameMode tick.
- **Trigger:** RecordMirelingDefeatReward queues mireling-gem|<PersistentSpawnId> and saves the defeated ledger under the bare spawn ID. DeliverPendingWildlifeRewards recognizes the prefix but calls RightChop(12). The prefix mireling-gem| is 13 characters, so the resulting lookup key is |<PersistentSpawnId>. PopulationSaveGame->IsDefeated checks for the bare ID, fails, and the loop skips delivery on every 0.5-second retry.
- **Impact:** A rolled Dusk Diamond is never granted, even when the authenticated owner is present and has enough inventory capacity. The pending claim remains stuck rather than following the documented capacity-retry behavior.
- **Evidence:** The selected delivery guard and claim producer establish the mismatched IDs. docs/02-technical-architecture.md A9 requires the selected gem to be held in the owner's pending claim and retried; docs/monster-spawning.md S02 requires owner-bound durable claims. KalmalaA9EnemySourcesTest verifies deterministic gem source selection but no test exercises DeliverPendingWildlifeRewards or this claim ID.
- **Proposed repair:** Parse/remove the complete prefix (or use a typed claim-ID parser) before checking the defeated set, and add a focused server delivery regression that starts from the Mireling producer's saved claim.
- **Dependencies:** After CR19 review acceptance; no repair is authorized during CR03 review.
- **Acceptance:** A generated Mireling with a persisted successful Dusk Diamond roll delivers the gem exactly once to its authenticated owner when capacity exists. A full inventory retains the original owner-bound claim and stored outcome; after capacity is restored, one retry grants the gem and durably consumes the claim.
- **Focused validation:** Run Kalmala.Gameplay.Discovery.EnemyGemSources and the new GameMode delivery regression after an Editor build.

## RF21 - M9 resource depletion is session-only (P2)

- **Source:** CR03-001b-98; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8350-8353, 9305-9311`, `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:524-538`, and `docs/02-technical-architecture.md:990-1004`.
- **Affected path:** Generated M9 harvest sources, server depletion recording, and reactivation of their population key after a fresh session.
- **Trigger:** Harvest an M9 source, then restart the server or load the same world in a fresh process and revisit its spatial key.
- **Impact:** M9 resource IDs are added only to the transient `SessionM9ResourceDepletionIds` set. The M9 harvest event bypasses the ordinary callback that writes `UKalmalaWorldPopulationSaveGame`, and population activation checks only the session set for M9 sources. The source therefore respawns after restart and can grant its deterministic reward again, despite the documented world-scoped sparse depletion contract.
- **Evidence:** `RecordM9ResourceDepleted` contains no save mutation or writer call; production sources contain no `ResourceDepleted` sparse-delta read/write. `CommitHarvestedStateFromServer` broadcasts `OnM9ResourceDepleted` instead of `OnHarvested` for M9 sources. The M9 source acceptance test captures the event through a lambda and only enables its real-slot path for generated forest cases, so it cannot detect the persistence gap. The architecture contract requires a `ResourceDepleted` sparse delta scoped to exact world seed and generator revision.
- **Proposed repair:** Integrate the stable M9 resource ID with the validated world-scoped sparse ledger, use the loaded delta to suppress the source on activation, and handle write failure without granting a durable repeatable harvest. Add a GameMode integration regression that harvests a generated source, verifies the saved delta, and confirms fresh-load suppression; cover wrong-world identity and save-write failure.
- **Dependencies:** After CR19 review acceptance; no repair belongs to this CR03 review run.
- **Acceptance:** A successful M9 harvest persists exactly one `ResourceDepleted` delta under the matching world identity; a fresh session for that same world suppresses the source; a different seed or generator revision does not inherit it; and injected write failure cannot create a durable player grant while leaving depletion absent.
- **Focused validation:** Run the new GameMode M9 depletion persistence regression, the existing M9 source acceptance test, and the M7 sparse persistence contract test after an Editor build.

## CR03-001b-99 - Player discovery save lookup and server player-state persistence

- **Source review:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9427-9587`.
- **Evidence:** The authenticated server path refreshes the authoritative world identity before caching or loading player data, checks exact current save type/world/scope/owner and current M9 claim descriptors, and preserves invalid slots while blocking later writes for that identity. Player writes validate a cloned candidate, merge server-owned claims and current carried-tool records, save it first, and replace the cache only after disk success. Current model tests cover schema-3 data and malformed identities in memory; the reconnect fixture covers valid GameMode persistence/reload and owner privacy; the Stormcut test covers action rollback when the player save gate fails.
- **Result:** No production defect confirmed. Confirmed RF22 (P3), a focused test gap for the invalid/incompatible existing player-slot branch and its no-rewrite guarantee.
- **Scope/limits:** Static inspection only; no disk slot, save failure, PostLogin restore or reconnect was exercised. No gameplay implementation changed.

## RF22 - Cover fail-closed player discovery save loading (P3)

- **Source:** CR03-001b-99; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9427-9488` and `9520-9585`.
- **Affected path:** Loading and writing authenticated player discovery/equipment state when a player slot already exists but is malformed, incompatible, belongs to another owner/world, or contains a stale M9 descriptor.
- **Trigger:** Start or reconnect a player whose expected slot contains an invalid or incompatible `UKalmalaPlayerDiscoverySaveGameV2` record, then attempt to restore or persist player state.
- **Impact:** The production loader currently fails closed and preserves such data, as required. No focused test proves that the GameMode path returns no state, rejects subsequent persistence, and leaves the exact disk bytes unchanged, so a future overwrite or partial-restore regression could pass the save-model and valid-reconnect checks.
- **Evidence:** `KalmalaPlayerDiscoverySaveGameTest.cpp` checks malformed/over-cap data and identity mismatches through `MatchesPlayer` in memory. `Scripts/Tests/Progression/Schema2CandidateReconnect.ps1` exercises valid current saves and restart restore, not an invalid pre-existing slot. The source loader's invalid branch adds the identity to `RejectedPlayerDiscoverySaveIdentities` and returns without rewriting the slot.
- **Proposed repair:** Add an isolated GameMode persistence test that seeds the exact player slot with invalid schema, owner/world mismatch, and a stale-but-well-formed M9 claim in separate cases; call the production load and persist paths, and compare the slot bytes before and after each rejection.
- **Dependencies:** After CR19 review acceptance; no test repair belongs to this CR03 review run.
- **Acceptance:** Each invalid case returns no restored player state; a subsequent `PersistPlayerStateFromServer` remains rejected; the original slot bytes remain identical; and a valid current-format slot still loads through the same GameMode path.
- **Focused validation:** Run the new GameMode invalid-player-slot regression, `Kalmala.Gameplay.Discovery.Schema3CurrentFormat`, and `Scripts/Tests/Progression/Schema2CandidateReconnect.ps1` after an Editor build.

## RF23 - Cover world save and Home-return transactions (P3)

- **Source:** CR03-001b-100; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9588-9710`.
- **Affected path:** Saving population, construction, storage, and authenticated player state before solo return, network-client return, or listen-host shutdown/Home travel.
- **Trigger:** A regression changes save ordering, mode/controller/pawn validation, failure retry state, or the listen-host client-disconnect/net-driver teardown sequence while current presentation checks remain green.
- **Impact:** Static inspection found the current server-side gates and failure behavior consistent with the start-menu contract. No focused automation establishes that world slots and player records are all attempted, a failed write retains a retryable session, a network client saves only its own player record, or a listen host waits for remote logout and driver teardown before returning Home. The documented live create/save/logout/reopen confirmation remains pending, so these paths lack a regression gate.
- **Evidence:** Exact API and player-facing save/return searches found only production declarations/callers and no focused automation. `docs/startmenu.md` defines separate world component checkpoints, save-before-return ordering, client ownership and host teardown; it explicitly leaves build and live create/save/logout/reopen confirmation pending. `docs/07-development-setup.md` has no targeted Home-return acceptance runner.
- **Proposed repair:** Add an isolated GameMode return/save regression with controllable write outcomes. Cover standalone and listen-host component/player writes, each failure leaving the session active and retryable, network-client persistence limited to the authenticated owner's record, and listen-host completion only after all requested remote clients log out and the listen driver is destroyed. Verify a successful path reaches Home and preserved slot data can be reopened.
- **Dependencies:** After CR19 review acceptance; no test repair belongs to this CR03 review run.
- **Acceptance:** Focused automation proves current-mode/controller authority gates, full save success before each return, retryable failure without premature travel, player-only client persistence, and listen-host disconnect/driver completion ordering.
- **Focused validation:** Run the new GameMode world-save/Home-return automation and the existing player-save and presentation regressions after an Editor build; perform the documented live create/save/logout/reopen acceptance before the feature is accepted.

## RF24 - Collision-blocked saved skiff can be silently replaced (P2)

- **Source:** CR03-001b-101; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9758-9799`, `9885-9924`, and `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:399-463`.
- **Affected path:** Restoring and updating the world-scoped moored `ocean-skiff:primary` snapshot after a server restart.
- **Trigger:** The current-format saved vessel passes identity, bounds, and generated-ocean footprint validation, but `SpawnActor` with `DontSpawnIfColliding` returns null because another actor blocks its saved location.
- **Impact:** `RestorePersistedOceanSkiff` logs that the save is preserved but leaves `bOceanTravelPersistenceWritable` true and retains the loaded `OceanTravelWorldSave`. Launch admission only checks for live skiff actors, so with no restored actor the player can launch a new primary skiff elsewhere. New skiffs default to `Moored`, and the launch callback immediately calls `PersistOceanTravelState`. That writer clones the cached save, and `SetVesselState` permits an update with the same stable vessel ID; the new location then overwrites the previously accepted safe location. The old moored state cannot be recovered after the obstruction is removed.
- **Evidence:** The collision-failure branch at `KalmalaGameMode.cpp:9786-9794` does not disable writes or reserve the saved session slot. `AKalmalaOceanSkiff::TryLaunchFromServer` at lines 399-463 admits a new skiff when no actor exists and persists it through the server GameMode. `PersistOceanTravelState` at `KalmalaGameMode.cpp:9885-9924` writes the same world slot; `UKalmalaOceanTravelPersistenceSaveGame::SetVesselState` accepts replacing the transform when the vessel ID is unchanged. M8 requires persisted accepted state to survive reconnect/restart, and the restore contract explicitly requires actor spawn to pass collision handling.
- **Proposed repair:** Keep the failed saved vessel slot reserved and non-writable after a collision-blocked restore, and prevent a new primary launch from replacing it until the original restores or an explicit recovery path safely resolves the saved state.
- **Dependencies:** After CR19 review acceptance; no implementation repair belongs to this CR03 review run.
- **Acceptance:** With a valid saved vessel and a blocker at its saved location, startup does not spawn a duplicate and a new launch cannot overwrite the slot. After removing the blocker, a fresh startup restores the original vessel ID and exact safe transform; no saved accepted state is lost.
- **Focused validation:** Add a GameMode restart regression with an isolated collision blocker and slot-byte comparison; run `Kalmala.World.OceanTravel.PersistenceContract`, `Kalmala.Gameplay.OceanTravel.SkiffRestoreContract`, and `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1` after an Editor build.

## RF25 - Cover fail-closed Ocean travel world-slot loading (P3)

- **Source:** CR03-001b-101; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9736-9755`.
- **Affected path:** GameMode startup loading of the world-scoped schema-1 Ocean travel save.
- **Trigger:** A future regression accepts or rewrites an existing slot with an unsupported schema, wrong world identity, missing/invalid vessel payload, or malformed transform while the save-model and valid-restart checks remain green.
- **Impact:** The current loader fails closed and preserves incompatible slots, but no focused test drives that production loader branch or proves subsequent travel writes remain blocked and the exact original slot bytes stay unchanged. The model test checks wrong identities and malformed records in memory; the M8 restart runner covers a valid saved vessel only.
- **Evidence:** `KalmalaOceanTravelPersistenceContractTest.cpp` covers schema decisions, identity mismatch, malformed bounds, an empty world payload, and valid local-slot round trips. `OceanSkiffIntegratedReconnectJourney.ps1` and its peer fixture cover successful restart restoration. No test or script references `LoadOceanTravelWorldSave`; the M8 setup guide records model and valid restart coverage without an invalid existing-slot preservation case.
- **Proposed repair:** Add an isolated GameMode loader regression that seeds incompatible world-slot bytes, starts the production load path, attempts world/player travel persistence, and compares the original bytes before and after rejection; retain a valid current-format restore case through the same path.
- **Dependencies:** After CR19 review acceptance; no test repair belongs to this CR03 review run.
- **Acceptance:** Invalid schema, wrong seed/revision, and invalid vessel-payload cases restore no actor, keep travel persistence blocked, and leave exact slot bytes unchanged; a valid current-format world slot still restores through GameMode.
- **Focused validation:** Run the new GameMode invalid-slot regression, `Kalmala.World.OceanTravel.PersistenceContract`, `Kalmala.Gameplay.OceanTravel.SkiffRestoreContract`, and `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1` after an Editor build.

## RF26 - Do not accept skiff launch or boarding after travel-save failure (P2)

- **Source:** CR03-001b-102; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9823-9949`, `9951-9984`, and `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:399-465`, `514-559`, `561-604`, `658-674`.
- **Affected path:** Launching or boarding the persisted ocean skiff, then disembarking or restoring the accepted vessel and seat after restart.
- **Trigger:** The world or authenticated passenger slot write fails, or the player's existing travel slot is invalid/incompatible and is correctly preserved without overwrite.
- **Impact:** `PersistOceanTravelState` returns failure when the world or any current occupant's player record cannot be saved, but `OnOceanSkiffStateChanged` is `void` and only logs the failure. `TryLaunchFromServer` still reports `SkiffLaunched` for the spawned actor, and `TryInteractFromServer` still reports an assigned seat after the state setter's notification. With an incompatible existing player slot, `PersistOceanTravelPassenger` fails without overwriting it; later `ClearOceanTravelPassenger` rejects clearing that same record and `TryDisembarkFromServer` leaves the character attached. When a slot write fails instead, the accepted vessel or seat can exist only in session memory and disappear after a server restart.
- **Evidence:** `KalmalaGameMode.cpp:9823-9884` returns false for invalid/incompatible passenger saves or candidate/save failure. `KalmalaGameMode.cpp:9885-9949` writes the world snapshot before passenger records and reduces failures to a warning through the void callback. `KalmalaOceanSkiff.cpp:399-465` reports launch success based on actor spawn, not persistence; lines 514-559 assigns and reports a seat without a persistence result. The disembark path clears the seat slot before detaching and returns early when that clear fails. The M8 contract requires authenticated seat references and restart preservation; the existing model and integrated journey cover valid serialization/restart only, with no GameMode-level write-failure regression.
- **Proposed repair:** Propagate persistence results to launch and seat assignment. Preserve incompatible slot bytes, but reject or roll back a launch/boarding transition that cannot satisfy its required world/player write, with requester feedback that does not claim success. Keep the disembark slot-clear-before-detach guarantee.
- **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize implementation in CR03.
- **Acceptance:** Fault-injected world and passenger slot failures, plus an incompatible existing passenger slot, do not produce a reported successful but non-durable launch/seat. The incompatible slot remains byte-identical, the player is not left seated without a clearable record, and a valid persisted vessel/seat still restores exactly after restart.
- **Focused validation:** Add isolated GameMode/skiff automation for world-write failure, passenger-write failure, incompatible-slot preservation, and boarding rollback; run `Kalmala.World.OceanTravel.PersistenceContract`, `Kalmala.Gameplay.OceanTravel.SkiffRestoreContract`, and `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1` after an Editor build.

## RF27 - Report unavailable world discovery saves to the owner (P3)

- **Source:** CR03-001b-103; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10060-10063`, `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9427-9517`, and `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:146-164`.
- **Affected path:** A nearby player's first landmark or scroll claim when the authenticated player discovery save cannot be loaded or created.
- **Trigger:** `ClaimDiscovery` has a valid feedback component, but `GetPlayerDiscoverySave` returns null for an unavailable identity, invalid/incompatible slot, or other fail-closed save condition.
- **Impact:** The claim returns false before mutation, but it does not publish `Unavailable`. The discovery actor ignores the returned bool, so the owner receives no reason or retry cue. This leaves a rejected interaction silent even though later save-write failures and Ocean discovery load failures publish owner feedback.
- **Evidence:** `ClaimDiscovery` obtains `Feedback`, calls `GetPlayerDiscoverySave`, then returns immediately when `Save == nullptr || Feedback == nullptr`; the subsequent `PersistPlayerStateFromServer` failure path does publish `Unavailable`. `AKalmalaDiscoveryActor::Interact_Implementation` invokes `ClaimDiscovery` and discards its result. The existing `DiscoveryPeer.ps1` fixture covers valid owner success, distance rejection, owner privacy and duplicate feedback, but does not induce a failed player-save lookup. RF22 separately records the missing invalid-slot preservation regression.
- **Proposed repair:** When the save lookup fails and `Feedback` is valid, publish owner-only `Unavailable` before returning. Keep the claim rejected, preserve invalid slot bytes and avoid applying scroll effects.
- **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize implementation in CR03.
- **Acceptance:** A valid world discovery still succeeds once; an unavailable/incompatible owner save produces one owner-only `Unavailable` result with no claim or effect mutation; duplicate and out-of-range paths retain their existing behavior; invalid save bytes remain unchanged.
- **Focused validation:** Add a GameMode world-discovery save-failure feedback regression and run `Scripts/Tests/Progression/DiscoveryPeer.ps1` plus the player discovery save contract after an Editor build.

## RF28 - Recover Ocean discovery rewards when claim rollback fails (P2)

- **Source:** CR03-001b-103; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10155-10200`, `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-295`, `411-427`, and `Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:51-54`.
- **Affected path:** Committing an authenticated player's one-time Ocean discovery ledger entry and catalogue reward.
- **Trigger:** The candidate ledger slot write succeeds, `TryCommitStacksFromServer` rejects the reward candidate, and the compensating write of the prior ledger also fails. One commit rejection can occur when `BuildGrant` accepts the stack layout but the combined stack-plus-packed-tool slot limit enforced by `TryCommitStacksFromServer` is exceeded.
- **Impact:** Inventory remains unchanged while the last successful slot write can contain the new `DiscoveryClaimed` ID. After a restart, the handler reloads that ID and reports `AlreadyFound`, so the player cannot retry the reward. The handler logs the failed compensation but has no durable recovery record. This violates M8's owner-private exact-once reward contract by durably consuming a claim without delivering its reward.
- **Evidence:** `ClaimOceanDiscovery` saves `CandidateSave` with the new delta before calling `TryCommitStacksFromServer`; on commit failure it writes the old `Save`, logs if that write fails, and returns `Unavailable`. `BuildGrant` validates stack state and item quantity but does not apply the packed-tool combined-capacity check. `TryCommitStacksFromServer` applies `CanFitContents(Candidate, CountPackedTools(...))` before changing inventory, so it can reject a candidate after the ledger write. The Ocean discovery model test checks a full ordinary stack pack before a ledger write, and the integrated M8 runner checks successful claim/restart/replay; neither injects this rejection plus rollback-write failure.
- **Proposed repair:** Preflight the exact combined inventory capacity before writing the ledger, and make the claim/reward transaction recoverable if inventory commit or compensating persistence fails. Add controllable save and inventory outcomes to the GameMode regression.
- **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize implementation in CR03.
- **Acceptance:** A candidate exceeding combined packed-tool capacity is rejected before any claim is durably added and can be retried after capacity is freed. Injected inventory-commit and rollback-write failures leave a recoverable outcome across restart; no valid claim is replayable without its reward, and recovery cannot duplicate a granted reward.
- **Focused validation:** Add a GameMode Ocean claim transaction regression covering packed-tool capacity and both failure outcomes; run `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract` and `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1` after an Editor build.

## RF29 - Keep Mireling boss scroll claims recoverable after player-save failure (P2)

- **Source:** CR03-001b-104; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10295-10340`, `12034-12051`, and `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-597`, `5392-5450`.
- **Affected path:** The authenticated credited attacker’s one-time world-seeded support scroll from a server-designated Mireling boss defeat.
- **Trigger:** `RecordMirelingDefeatReward` successfully persists the Mireling defeated ID, but `GetPlayerDiscoverySave` returns null or `PersistPlayerStateFromServer` fails while `ClaimMirelingBossScroll` records the scroll and learned effect.
- **Impact:** The player claim returns false after the defeated source has been durably consumed. `GrantDefeatReward` has already set `bDefeatRewardProcessed` and ignores the claim result; the actor remains defeated, and population activation suppresses that stable ID on reactivation. The persistent Mireling reward queue carries only the Dusk Diamond claim, not the boss scroll, so the original encounter has no retry path. A separate eligible Mireling may offer another attempt, but this is not recovery of the accepted encounter and is not guaranteed. When the save lookup returns null, the handler also returns without owner feedback.
- **Evidence:** `RecordMirelingDefeatReward` queues only `mireling-gem|<PersistentSpawnId>` plus its gem outcome, marks the spawn defeated, and saves the population record. `AKalmalaWildlifeSpawn::DefeatServer` sets `bDefeated` and enters `ApplyDefeatedState`; `GrantDefeatReward` processes once and discards the `ClaimMirelingBossScroll` result. That handler writes the scroll discovery and canonical learned effect through `PersistPlayerStateFromServer`, but does not add a pending reward when the write fails. Architecture states that boss scrolls commit authenticated entitlement before acknowledgement and failed saves leave the encounter/discovery recoverable. The setup guide retains a two-peer boss-reward scenario for M4, but the in-tree discovery test checks only model/identity helpers and the checked-in combat runner has no Mireling boss-scroll scenario.
- **Proposed repair:** Preserve a bounded server-owned, authenticated pending scroll entitlement or otherwise retain a retryable source state until the owner’s player save contains both the discovery and learned effect. Publish owner-only unavailable feedback on load/write failure, and make retry/recovery idempotent across reconnect without granting the effect twice.
- **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize implementation in CR03.
- **Acceptance:** With an eligible Mireling and injected player-save load/write failure, the defeated source retains a retryable owner-bound scroll outcome. After persistence recovers, the owner receives the world-canonical scroll and learned effect once, reconnect retains the entitlement, replay does not duplicate it, and other peers receive no private reward state.
- **Focused validation:** Add a GameMode/wildlife two-peer boss-scroll recovery regression with injected player-save outcomes; run `Kalmala.Gameplay.Discovery.PlayerScopedPersistence` and the M4 combat/boss-reward acceptance after an Editor build.

## RF30 - Cover M9 exploration reward rollback on player-save failure (P3)

- **Source:** CR03-001b-104; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10208-10293`, `Source/KalmalaGameplay/Public/KalmalaGameMode.h:188-190`, and `Source/KalmalaGameplay/Private/Tests/KalmalaStormcutSwordTest.cpp:188-195`.
- **Affected path:** Granting an M9 land-discovery material reward while persisting the authenticated player's stable claim.
- **Trigger:** A future regression changes the inventory-first claim transaction or its refund after `PersistPlayerStateFromServer` rejects the candidate, while the current valid peer scenario remains green.
- **Impact:** The current handler attempts to restore the prior inventory and reports `Unavailable`; no test proves the real GameMode path leaves neither a reward nor a durable/session claim after the player-slot write fails, or that the player can retry once persistence recovers. A regression could grant material without a claim or leave inventory mutated while rejecting the discovery.
- **Evidence:** `ClaimM9ExplorationReward` commits `CandidateStacks` before `PersistPlayerStateFromServer`, then attempts `TryCommitStacksFromServer(CandidateStacks, ExistingStacks)` on save failure; the session claim is added only after persistence succeeds. The existing `PlayerStateSaveGateForTests` can force this write boundary to fail, and `KalmalaStormcutSwordTest` uses it for a different transactional action. `KalmalaM9ExplorationRewardPeerTest` covers success, distance, forgery, replay and independent owners; `KalmalaDiscoveryProgressTest` covers player-save model behavior. Neither drives this handler’s failed-write/refund/retry path. Setup documentation explicitly says failed saves reject the land claim and inventory transaction.
- **Proposed repair:** Add an isolated GameMode claim regression using the player-save gate to reject the first persistence attempt, then restore the gate and retry the same canonical descriptor. Assert owner feedback, live inventory, saved claim and session cap at each step.
- **Dependencies:** After CR19 review acceptance; this finding is a test handoff only and does not authorize test changes during CR03.
- **Acceptance:** A forced player-save rejection publishes `Unavailable`, returns inventory to its exact pre-claim contents, writes no discovery claim, and leaves the descriptor retryable. Once the save succeeds, one retry grants exactly the catalogue quantity and persists one claim; a later replay grants nothing.
- **Focused validation:** Run the new GameMode failure/retry automation, `Kalmala.Gameplay.M9.ExplorationRewardCatalogue`, `Scripts/Tests/Progression/ExplorationRewards.ps1`, and `Kalmala.Gameplay.Discovery.PlayerScopedPersistence` after an Editor build.

## RF31 - Reject malformed biome baseline capture coordinates (P3)

- **Source:** CR03-001b-105; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10382-10393, 10472-10479`, `Scripts/Tests/World/BiomeBaselineCapture.ps1:15-18`, and the installed UE 5.8 `Engine/Source/Runtime/Core/Private/Misc/Parse.cpp:678-686` plus `Engine/Source/Runtime/Core/Public/Microsoft/MicrosoftPlatformString.h:164-167`.
- **Affected path:** Direct non-Shipping process launches using `-KalmalaBiomeBaselineCapture` and the required `KalmalaBiomeBaselineXcm`, `Ycm`, or `ViewZcm` arguments.
- **Trigger:** A required coordinate token is present but contains non-numeric text.
- **Impact:** The float `FParse::Value` overload reports that the token was found, then converts it through `FCString::Atof` without validating full numeric consumption. Invalid text converts to zero; the GameMode checks only finiteness, so it accepts the value and activates terrain/relocates the local pawn at a wrong coordinate. This can generate a misleading fixed-site baseline capture. The checked-in PowerShell runner uses mandatory `[double]` parameters and rejects malformed coordinates before launching, so its normal path is protected.
- **Evidence:** `TryCaptureBiomeBaselineView` combines `FParse::Value` results for required coordinates and only rejects missing arguments, an empty site/output path, or non-finite float values; it does not validate full numeric-token consumption. The installed engine implementation shows the parser returns true after extracting a token and calls `FCString::Atof`; the Windows TCHAR conversion delegates to `_tstof`, which yields zero for a non-numeric token when no end pointer is checked. Zero passes `FMath::IsFinite` and flows into `ActivateTerrainPatchNeighborhood` and the pawn `SetActorLocation` call.
- **Proposed repair:** Parse each required coordinate with a strict full-token conversion and reject malformed values before terrain activation or pawn relocation. Keep valid signed decimal and exponent forms supported by the capture runner.
- **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize implementation in CR03.
- **Acceptance:** Direct capture launches reject malformed X, Y, or view-Z tokens with an explicit error and produce no screenshot or scene relocation; valid numeric tokens retain their exact requested coordinates.
- **Focused validation:** Add a narrow capture-argument regression for invalid X/Y/view-Z tokens and valid signed/scientific values; verify rejection occurs before terrain activation, pawn movement, or screenshot output. Run the baseline capture runner on a valid fixed site after an Editor build.

## RF32 - Reject incomplete per-site lighting comparison fixtures (P3)

- **Source:** CR03-001b-106; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10622-10645`, `Scripts/Tests/Presentation/LightingComparison.ps1:101-108, 191-218`, and `docs/valheim-like-lighting-plan.md:602-610, 678-695`.
- **Affected path:** Isolated `DefaultLit` or `Toon` L03 lighting captures and their six per-site material-class summaries.
- **Trigger:** One smooth/banded proxy pair for a site is missing or misnamed while at least one smooth and one banded actor from another site remain in the fixture map.
- **Impact:** The GameMode's aggregate `SmoothActors`/`BandedActors` checks still pass, so the capture proceeds. The runner then samples the expected crop rectangle without checking that its proxy is present; background pixels can be written to `LightingSampleLuminance.csv` as that surface's measured result, weakening or misleading the material comparison.
- **Evidence:** The fixture contract defines six classes—ground, rock, wood, canopy, creature, and construction—at each site. The GameMode counts recognized smooth/banded actors globally and errors only when either global count is zero. `LightingComparison.ps1` iterates all six classes for each variant and `Get-RegionPixelSummary` rejects only a zero-pixel rectangle; a rendered rectangle always contains pixels even when the proxy is absent. It does not assert foreground occupancy or per-site actor counts.
- **Proposed repair:** Validate the exact six smooth and six banded proxy identities for the requested site before capture, and reject tagged comparison actors with an unrecognized variant prefix. Add a narrow rendered or fixture-level regression proving missing and misnamed pairs cannot be summarized as valid samples.
- **Dependencies:** After CR19 review acceptance; this finding is review handoff only and does not authorize implementation in CR03.
- **Acceptance:** An isolated capture rejects a site missing or misnaming any one of its six pairs before producing the image or sample summary. A complete site captures all six selected proxies, hides the six opposite-variant proxies, and reports no background-only class crop.
- **Focused validation:** Add fixture tests for one missing pair and one unrecognized name, then run the focused `LightingComparison.ps1` capture on a valid generated map and verify all six per-site class crops contain their expected proxy. Run after an Editor build.

## RF33 - Restore the ambient-audio accepted-craft peer fixture (P2)

- **Source:** CR03-001b-109; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11026-11036`, `Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp:83-101,174-190`, `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:223-235,312-330`, and `Scripts/Tests/Audio/AmbientAudioPeers.ps1:70-73`.
- **Affected path:** The host/client accepted-crafting cue check in `Scripts/Tests/Audio/AmbientAudioPeers.ps1` when launched with `-KalmalaAmbientAudioTest`.
- **Trigger:** Run the audio peer fixture. GameMode grants nine Wood and directly invokes the `CookingRack` recipe, then expects one `CookingRackKit` in the player's inventory. Catalogue loading maps that recipe to the `CookingRackKit` buildable and places it in `BuildableOutput`; `CraftFromServer` rejects all direct-material buildables before payment or output. The fixture therefore reports a rejected craft. Independently, its log says `crafted rack`, while the runner only counts `crafted fuel: Accepted=1 ResultSerial=1` and requires at least two matches.
- **Impact:** The accepted-craft fixture gate remains false and the documented peer runner times out even when the other cue checks pass. It cannot validate the accepted crafting cue for either owner.
- **Evidence:** `KalmalaGameCatalogueLoader.cpp` converts the source `CookingRack` output to the internal `CookingRackKit` ID and then stores supported buildables in `BuildableOutput` with empty inventory `Output`. `UKalmalaRecipeCatalogue::IsDirectMaterialBuildable` recognizes supported placed identities, and `CraftFromServer` returns false with the Building Hammer instruction before inventory payment. `ServerCraft_Implementation` publishes that rejection and increments the result serial; the PostLogin fixture checks for an accepted result and kit quantity anyway. The runner waits for two successful `crafted fuel` log markers, whereas the only matching GameMode fixture log is named `crafted rack` and reports `Accepted=0`. The architecture and development setup specify direct-material Building Hammer construction and list this peer runner as the accepted-crafting audio check.
- **Proposed repair:** Use an enabled ordinary inventory recipe for the audio acceptance cue, assert its ordinary item result, and align the server marker with the peer runner. Keep construction identities out of inventory.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation in CR03.
- **Acceptance:** A fresh host/client audio run publishes one accepted ordinary craft per owner at result serial 1 and one rejected craft at serial 2; the runner observes both accepted-craft markers and passes without any placed-object identity in inventory.
- **Focused validation:** Run `Scripts/Tests/Audio/AmbientAudioPeers.ps1 -WeatherExposureOnly` after an Editor build, then run the normal peer scenario to retain visible-water coverage.

### CR03-001b-111 — Traversal test configuration and reconnect verification (lines 11374–11648)

- **Reviewed revision:** `2bfc71d0c7515e366cd5b4393c3f3c83a61d837e`; whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches the CR01 baseline including existing local edits.
- **Scope:** `ConfigureTraversalTest` target selection and `RunReconnectVerification` harvest/wildlife restart fixtures.
- **Evidence:** The traversal selector uses the generated start and world identity, a bounded 250 m grid, and sampled dry-route checks. The companion character search reads the replicated world-generation config and independently derives the same target on each peer. The reconnect helper validates that an exact wildlife spawn ID is accompanied by both spatial-key coordinates, then derives the requested server descriptor before activation. Cross-checked its harvest assumptions against population descriptor generation and depletion callbacks; RF34 records the confirmed mismatch.
- **Result / limits:** No traversal-target defect was confirmed. RF34 is the one confirmed actionable finding in the reconnect fixture. Static source, documentation, and runner inspection only; no compile, Unreal, runtime, or peer validation. CR03 remains open.
- **Lightweight checks:** Whole-file SHA-256, selected function boundaries, traversal parameter/reference search, reconnect mode/identity pairing, and runner log assertions were inspected. `git diff --check` passed after this evidence update.

## RF34 — Select a persistent, interactable source for the harvest restart fixture (P2)

- **Source:** CR03-001b-111; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11566-11638`, `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:139-164`, `Source/KalmalaWorld/Public/KalmalaBiomeContentContract.h:43,125`, `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:198-202,228-230,335-365,524-537`, and `Scripts/Tests/Survival/PersistedCampRestart.ps1:92-94`.
- **Affected path:** The `Harvest` then `Verify` process-restart check in `Scripts/Tests/Survival/PersistedCampRestart.ps1`.
- **Trigger:** Run the helper when the server start spatial key's first descriptor is a Meadow source. `RunReconnectVerification` always selects `HarvestSpawns[0]` and calls `InteractWithToolIntentFromServer`. A generated Meadow candidate can remain `meadows-birch-bark`, whose standing tree is not interactable until it becomes a wood drop, or it can be replaced with a hand-gathered M9 crop source. M9 depletion is recorded in `SessionM9ResourceDepletionIds`, while this reconnect helper requires `PopulationSaveGame->IsHarvested(PersistentSpawnId)` before it reports success. The M9 depletion set is not the world sparse harvested-ID ledger reloaded by `Verify`.
- **Impact:** For either selected source, the smoke test can fail to produce its required `harvested generated node` marker or fail to suppress that same node after restart. That leaves the documented world-population sparse-restart check dependent on an unrelated first descriptor rather than a source compatible with its persistence contract.
- **Evidence:** The Meadow layout can assign its first eligible candidate to `meadows-wild-carrot-patch` and leaves the biome-default `meadows-birch-bark` otherwise. The harvest node rejects interaction with an unfelled tree; accepted M9 depletion broadcasts only the M9 callback, and GameMode stores it in the session-only resource set. The reconnect verifier unconditionally chooses descriptor zero and checks the separate `PopulationSaveGame` harvested set in both modes. The PowerShell runner requires the durable-harvest success markers.
- **Proposed repair:** Select a descriptor whose current interaction and depletion path writes the population save's harvested-ID set, and fail early with a clear precondition if none exists. Keep session-only M9 depletion on a separate verification path.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation in CR03.
- **Acceptance:** `Harvest` accepts a supported generated node and records its persistent spawn ID in the population save. A new process with the same world identity reloads that ID and suppresses the node. A start key whose first generated descriptor is a tree or M9 source still selects an eligible persistent target or reports a clear setup failure before claiming test success.
- **Focused validation:** After an Editor build, run `Scripts/Tests/Survival/PersistedCampRestart.ps1` for the existing fixed-seed case and a case whose first start-key descriptor is a Meadow tree or M9 source; confirm both process logs contain the required success markers and the restarted server has no actor for the saved ID.

### CR03-001b-112 — Traversal drive, generated starts, and terrain patch lifecycle (lines 11649–11880)

- **Reviewed revision:** `19cbf2e88b7a05334b70fe9c664659ffe87a3218`; whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches the CR01 baseline including existing local edits.
- **Scope:** Traversal server observations, generated-start pawn placement, terrain-patch creation, neighborhood refresh, forest actor ownership, and eviction.
- **Evidence:** The GameMode performs traversal diagnostics and terrain activation on the server; generated starts use sampled terrain height plus the pawn collision half-height and clearance after initial patch activation. Patch creation tracks only successful actors and suppresses matching cosmetic forest candidates; failed suppression rolls back spawned actors, and eviction destroys transient forest nodes without depletion callbacks. The one-second refresh requests a radius-one 3×3 patch neighborhood for every pawn, but the global 25-patch cap can be exceeded by three or four disjoint neighborhoods; RF35 records the resulting uncovered requests.
- **Result / limits:** RF35 is the one confirmed actionable finding in this range. The remaining inspected start-placement, traversal-observation, and patch ownership/eviction paths align with the documented server authority and sparse-identity contracts. Static source, documentation, and test inspection only; no compile, Unreal, runtime, or peer validation. CR03 remains open.
- **Lightweight checks:** Whole-file SHA-256, source boundaries, patch size/radius/cap arithmetic, server authority guards, and the existing two-player forest-patch lifecycle assertions were inspected. `git diff --check` passed after the evidence update.

## RF35 — Reconcile four-player terrain neighborhoods with the 25-patch cap (P2)

- **Source:** CR03-001b-112; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:92-96,11764-11778,11835-11878`, `Source/KalmalaWorld/Public/KalmalaTerrainPatchLayout.h:8-30`, `Source/KalmalaGameplay/Private/Tests/KalmalaForestPatchTest.cpp:199-225`, `docs/02-technical-architecture.md:282`, and `docs/00-project-brief.md:16`.
- **Affected path:** `RefreshTerrainPatchNeighborhoods`, called every second while players move through generated terrain.
- **Trigger:** Three or four pawns occupy disjoint radius-one neighborhoods inside world bounds. Each pawn contributes nine required patches, so three require 27 and four require 36. `RefreshTerrainPatchNeighborhoods` adds all coordinates to a `TSet`, then `ActivateTerrainPatch` silently returns after 25 active patches. There is no explicit per-pawn priority or reservation for each pawn's center or nearby movement patches, so at least two or eleven requested collision/render patches remain unmaterialized.
- **Impact:** Affected pawns can reach a neighborhood edge with no generated terrain actor or server collision beneath the route. The 1–4 player session contract and long-distance co-op travel can therefore encounter gaps once the players separate.
- **Evidence:** `PlayerTerrainPatchRadius` is 1 and each patch is 30 m across; the hard active cap is 25. The project brief supports four players. `KalmalaForestPatchTest.cpp` verifies two disjoint players retain two complete 3×3 neighborhoods (18 patches), but has no three- or four-player over-cap case. Architecture documents the 25-patch budget without defining a safe overflow policy.
- **Proposed repair:** Reconcile the four-player patch demand with the 25-patch budget. Define and enforce deterministic minimum terrain/collision coverage for every live pawn, or adjust the budget/neighborhood contract after validating the resulting memory and replication cost; never let `TSet` iteration decide which player loses nearby terrain.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation in CR03.
- **Acceptance:** A focused three- and four-player test places pawns in disjoint neighborhoods and crosses patch boundaries. Every pawn retains continuous server collision and the documented local render area while active actors stay within the chosen documented cap; behavior is deterministic across runs and independent of set iteration order.
- **Focused validation:** Extend the forest-patch lifecycle test with three/four disjoint pawns and over-budget refreshes, then run a live four-peer traversal/streaming check after an Editor build and confirm no pawn enters an unmaterialized collision region.

### CR03-001b-113 — Player-start selection, gem state, and treasure rewards (lines 11881–12053)

- **Reviewed revision:** `faa4a29d510efca012189c6e3d8cf6538569dd46`; whole-file SHA-256 `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325` matches the CR01 baseline including existing local edits.
- **Scope:** Non-Shipping peer-test start selection, server gemstone state reads/writes, generated enemy gem selection, canonical world-treasure preparation/claim, and Mireling defeat reward persistence.
- **Evidence:** Peer starts are created only for remote controllers in the named non-Shipping fixtures and reuse a tagged start on respawn. Gemstone reads/writes require authority and a matching population save; writes enforce expected hit/outcome transitions and restore the in-memory row if saving fails. Enemy gem selection matches the supplied stable ID to a generated wildlife descriptor, then checks biome and species before applying the deterministic contract. Treasure creation validates an existing canonical POI in an allowed biome and persists the deterministic loot once, including empty results. Claims validate the live same-world pawn and range, build and capacity-check the full inventory candidate, then persist the consumed record; a failed save rolls back the pack, and the discovery actor only publishes claimed state after success. RF20 already records the Mireling pending-claim prefix delivery defect; it is cross-referenced rather than duplicated here.
- **Result / limits:** RF36 is the one new confirmed actionable finding in this range. Capacity rejection can be silent when a candidate fits the item-stack array but not the combined carried-tool slots; a later treasure-save failure can also leave an owner gain receipt for an inventory change that is rolled back. Remaining inspected paths match the documented server-authority, deterministic source, empty-treasure, and durable-claim contracts. Static source/document/test inspection only; no compile, Unreal, runtime, peer, or injected save-failure validation. CR03 remains open.
- **Lightweight checks:** Reviewed exact source boundaries and callers, current-file SHA-256, A9 gemstone/treasure contracts and acceptance, inventory slot checks, owner gain-receipt publication, and existing serialization/peer fixtures. `git diff --check` passed after the evidence update.

## RF36 — Keep treasure failure feedback consistent with the claim transaction (P3)

- **Source:** CR03-001b-113; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:12007-12030`, `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:411-427`, `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:459-471`, `Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:51-54`, and `Source/KalmalaUI/Private/KalmalaNotificationSubsystem.cpp:244-246`.
- **Affected path:** `ClaimWorldTreasure` when a valid shared chest cannot fit its reward or its world-save claim fails.
- **Trigger:** First, `BuildGrant` can construct an item-stack candidate that fits the inventory stack limit, while the final `CanFitContents` check rejects it after accounting for carried tools; this return path does not publish the “Inventory is full; treasure remains” result used by the earlier grant-failure branch. Second, `TryCommitStacksFromServer` emits owner gain receipts before `SaveGameToSlot` commits the claimed treasure. If that save fails, the pack is rolled back but the reliable gain receipt is not retracted, and the notification subsystem can present the rolled-back item as acquired.
- **Impact:** A packed-tool capacity rejection gives the player no reason for the still-open chest, while a storage failure can show a false item-gain notice even though the pack and chest remain unchanged.
- **Evidence:** `BuildGrant` does not include packed-tool count; `CanFitContents` applies that combined slot limit. The final capacity guard returns without feedback. Inventory commit calls `RecordAcceptedGains`/`ClientAcceptedGain`, which the owner notification subsystem observes; rollback only removes inventory contents and does not cancel the receipt. Existing A9 peer coverage checks a successful treasure claim, and no focused GameMode test covers the packed-tool capacity or treasure-save-failure branches.
- **Proposed repair:** Publish explicit owner feedback from the combined-capacity rejection. Stage the gain receipt until the treasure claim is durable, or otherwise make the two-state transaction publish only after both sides commit while preserving rollback on failure.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation in CR03.
- **Acceptance:** A candidate that exceeds capacity only after carried-tool slots are included leaves inventory, save and chest unchanged and tells the owner the treasure remains. An injected treasure-save failure leaves inventory and chest unchanged and emits no gain receipt. After capacity or storage recovers, retry grants the exact contents once and persists one claimed record.
- **Focused validation:** Add a GameMode transaction regression for the packed-tool slot boundary and save failure, then run the A9 peer treasure claim/retry with owner notifications observed.

## RF37 — Keep per-player Ocean save caches reachable through garbage collection (P2)

- **Source:** CR03-002; `Source/KalmalaGameplay/Public/KalmalaGameMode.h:567,570`, `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9802-9821,9840-9881,9999-10043,10121-10203`, and [Epic's UE 5.8 Object Pointers documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine).
- **Affected path:** Ocean discovery claims and authenticated ocean-skiff seat persistence/restoration after the GameMode has cached a player's SaveGame objects.
- **Trigger:** `AKalmalaGameMode` stores loaded `UKalmalaM7PersistenceSaveGame` and `UKalmalaOceanTravelPersistenceSaveGame` objects in `OceanDiscoverySaves` and `OceanTravelPlayerSaves`, each a `TMap<FString, TObjectPtr<...>>` without `UPROPERTY`. The GameMode has no `AddReferencedObjects` override. A garbage-collection pass can therefore reclaim those objects while the map entries remain available for later lookup.
- **Impact:** `LoadOceanDiscoveryLedger` treats an existing key as already loaded, and `ClaimOceanDiscovery` consumes that cached entry rather than reloading it. After collection, a later valid Ocean discovery can fail as unavailable or encounter an invalid cached object for the remainder of the session. The travel player-save cache is likewise reused by seat persistence and restoration and can reject a valid saved association.
- **Evidence:** `PostLogin` loads the Ocean discovery ledger and calls `RestoreOceanTravelForPlayer`, which caches loaded objects. `LoadOceanDiscoveryLedger` returns when `OceanDiscoverySaves.Contains(PlayerIdentity)`; the claim path reuses `OceanDiscoverySaves.Find`. `PersistOceanTravelPassenger` and `RestoreOceanTravelForPlayer` reuse `OceanTravelPlayerSaves.Find`. The neighboring GameMode SaveGame caches that must stay alive are explicitly `UPROPERTY(Transient)`. UE 5.8 documentation describes `TObjectPtr` as a strong reference affecting garbage collection when it is marked `UPROPERTY`; an unreflected member map does not expose its values to the UObject reference graph.
- **Proposed repair:** Mark both cache maps `UPROPERTY(Transient)` (matching the existing player-discovery cache pattern), or explicitly enumerate their values from `AddReferencedObjects`. Add a focused forced-GC regression for both cache paths.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation in CR03.
- **Acceptance:** After loading valid current-format per-player Ocean saves and forcing garbage collection, the same GameMode can accept one new discovery exactly once and can persist/restore the authenticated player's seat association. The claim and seat still match exact world and player identity, and no save schema or migration is introduced.
- **Focused validation:** Add an isolated GameMode automation that loads each cache, forces full garbage collection, verifies cached objects remain valid, then exercises the discovery-claim retry and seat-persistence/restoration paths. Run the focused automation after an Editor build.

## RF38 — Suppress M9 reward receipts when claim persistence fails (P3)

- **Source:** CR04-010; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10208-10282`, `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:411-427`, `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:450-476`, `Source/KalmalaGameplay/Public/KalmalaInventoryComponent.h:155-157`, and `Source/KalmalaUI/Private/KalmalaNotificationSubsystem.cpp:244-246`.
- **Affected path:** An authenticated player's M9 land-discovery reward when the inventory grant succeeds but the player's discovery save cannot be written.
- **Trigger:** `ClaimM9ExplorationReward` calls `TryCommitStacksFromServer` before `PersistPlayerStateFromServer`. The inventory commit sends a reliable `ClientAcceptedGain` receipt to the owner. If player persistence fails, the handler restores the previous inventory and publishes unavailable feedback, but the already-sent receipt remains in the owner's local gain queue.
- **Impact:** The owner can receive an item-gain notice for material that is absent from inventory and was never durably claimed. This is an owner-facing transaction inconsistency; no cross-player disclosure or reward duplication was established.
- **Evidence:** `TryCommitStacksFromServer` replaces stacks and calls `RecordAcceptedGains` before returning success. `RecordAcceptedGains` invokes the client reliable receipt RPC for positive deltas. `ClaimM9ExplorationReward` subsequently calls `PersistPlayerStateFromServer`; its failure branch commits the old stacks back but does not retract the receipt. The notification subsystem observes gain receipts. RF30 covers the missing failure/retry transaction assertion, and RF36 covers the analogous receipt timing in `ClaimWorldTreasure`; neither establishes this M9 receipt behavior.
- **Proposed repair:** Stage owner gain receipts until the M9 claim save succeeds, or otherwise publish inventory and receipt state as one transaction while retaining the existing exact inventory rollback.
- **Dependencies:** After CR19 review acceptance; this is a review handoff and does not authorize implementation during CR04.
- **Acceptance:** An injected player-save rejection leaves inventory and the saved/session claim unchanged, publishes unavailable feedback, and sends no item-gain receipt. After persistence recovers, retrying the same canonical descriptor grants the exact catalogue reward with one owner receipt; replay grants no reward or receipt.
- **Focused validation:** Extend an isolated M9 GameMode failure/retry automation to inspect inventory, saved/session claim state, owner feedback, and gain receipts at every step. Run it alongside the M9 exploration peer scenario after an Editor build.

## RF39 — Rate-limit repeated storage-close requests (P3)

- **Source:** CR04-013; `Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:89-92,300`, `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:143-151`, and `Source/KalmalaGameplay/Public/KalmalaCraftingComponent.h:55-58`.
- **Affected path:** An owning client closing, or repeatedly requesting closure of, its private chest-storage view.
- **Trigger:** `ServerCloseStorage` is a reliable server RPC but does not call the component's `AcceptRequest` gate used by open, deposit and withdraw. Each request invokes `ClearStorageView`, which resets state and calls `ForceNetUpdate` on the owning character even if no storage view is open and the view is already empty.
- **Impact:** A client can bypass the normal 200 ms request cadence and repeatedly request immediate character updates, creating avoidable server and network work for relevant peers. The path does not grant items or expose another player's storage; no runtime flood was exercised.
- **Evidence:** `AcceptRequest` rejects requests inside its 0.2-second window. The adjacent storage open/deposit/withdraw handlers call it; `ServerCloseStorage_Implementation` calls only `ClearStorageView`. `ClearStorageView` has no unchanged-state early return and always calls `GetOwner()->ForceNetUpdate()` after clearing. The storage snapshot remains `COND_OwnerOnly`.
- **Proposed repair:** Preserve immediate clearing on the first valid close, then make repeated close calls idempotent and subject to a close-specific cadence that cannot leave an open view uncleared because of an unrelated request gate.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** Closing an open view immediately clears the owner-only snapshot and active chest. Repeated or rapid close requests remain bounded and do not repeatedly force character updates; closed storage cannot be used for transfers and peers still receive no chest contents.
- **Focused validation:** Add a focused two-peer storage RPC regression that floods close requests in both open and already-closed states, verifies the first close clears promptly and repeats are bounded/idempotent, then runs the storage peer scenario and checks that simulated peers retain empty views.

## RF40 — Make the non-axe harvest-gate regression reachable (P3)

- **Source:** CR04-014; `Source/KalmalaGameplay/Private/Tests/KalmalaCombatLoopTest.cpp:27-34,72,165,174`.
- **Affected path:** `Kalmala.Gameplay.Combat.AllItemSwings` coverage of the server axe-only tree-damage gate.
- **Trigger:** The fixture builds `Items` only from tool profiles for which `IsAttackItemAllowed` returns true (line 72), then checks for `ConstructionHammer` or `Bow` inside that filtered loop (line 165). The test separately asserts that those same items are excluded from `IsAttackItemAllowed` (lines 27-34), so the branch containing the tree-damage rejection assertion (line 174) is unreachable.
- **Impact:** The stated negative axe-gate assertion never executes, and no non-axe melee profile is checked against `ApplyAxeDamageFromServer`. A regression that lets a non-axe melee item damage a tree would pass this intended check. This is a test-coverage defect; no production exploit was established by this finding.
- **Evidence:** Static source inspection confirms the mutually exclusive filter and branch conditions. The other test references to `ApplyAxeDamageFromServer` cover accepted axe hits, falling-phase rejection, and range rejection, not rejection of a non-axe melee caller.
- **Proposed repair:** Move the rejection assertion out of the filtered attackable-item loop. Select an otherwise valid non-axe melee item such as `StoneKnife`, issue the server tree-damage operation against a valid tree, and assert that both tree health and tool condition remain unchanged.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** A named automation explicitly executes a valid non-axe melee caller against an eligible tree and confirms rejection with no health or condition mutation, while the accepted axe case continues to pass.
- **Focused validation:** Extend `Kalmala.Gameplay.Combat.AllItemSwings` or `Kalmala.Gameplay.Combat.AxeSwing`; build the Editor target and run the named automation.

## RF41 — Add remote support-magic authority and privacy coverage (P3)

- **Source:** CR04-015; `Source/KalmalaGameplay/Private/Tests/KalmalaDiscoveryProgressTest.cpp:70-73`, `Source/KalmalaGameplay/Private/Tests/KalmalaInventoryHotbarTest.cpp:63-66,116-127`, `Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:341-358`, `Source/KalmalaGameplay/Private/KalmalaSupportMagicComponent.cpp:271`, `Scripts/Tests/Acceptance/VerticalSlice.ps1:10-40`, and `docs/07-development-setup.md:2509-2538`.
- **Affected path:** A remote owning client activates a learned support effect from its numbered hotbar while an observer receives only the public active-effect presentation.
- **Trigger:** Treating the current local helper and hotbar fixtures as evidence for remote RPC ownership and owner-only replication, or relying on the M4 harness and setup guide claims without verifying the actual peer route.
- **Impact:** A regression in remote hotbar activation or observer privacy can pass the current support checks. The setup guide also overstates current evidence: its named foundation test has no sequence input or replay assertion, and its five-second cooldown text conflicts with the later M13 300-second slot-only contract. No current production authority or privacy exploit was established.
- **Evidence:** `KalmalaDiscoveryProgressTest` calls `IsActivationAllowed` directly and tests save-memory identity; it does not create a network world. `KalmalaInventoryHotbarTest` calls `ServerUseHotbarSlot` from a standalone authority world, so it verifies the local server path and cooldown but not a remote caller. `VerticalSlice.ps1` launches peer scenarios for combat, boar and deer, then runs support assertions through local Unreal automation. The server RPC takes only a slot index, and `KalmalaSupportMagicComponent::GetLifetimeReplicatedProps` currently marks learned entitlement, cooldown and feedback fields `COND_OwnerOnly`; no support test asserts those peer conditions.
- **Proposed repair:** Add a focused listen-host, remote-owner and observer support scenario that activates through the slot-only server RPC, verifies accepted and cooldown-rejected casts with one payment, and checks that only public active-effect state reaches the observer while entitlement, cooldown and feedback remain owner-only. Update the development setup to describe slot-only intent, the current 300-second cooldown, and checks the harness actually performs.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** The remote owner can activate an entitled effect through its current server-owned hotbar cell; a repeat during cooldown is rejected without further stamina payment; owner feedback arrives once; observers receive the public active effect but none of the owner-only fields; the guide matches the current M13 contract and executed evidence.
- **Focused validation:** Build the Editor target, run the focused support automation and the new peer scenario with a listen host, remote owner and observer, then verify the peer markers and replication conditions.

## RF42 — Bound repeated interaction RPC work (P3)

- **Source:** CR04-016; `Source/KalmalaGameplay/Public/KalmalaCharacter.h:296-298`, `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1687-1973`, `Source/KalmalaGameplay/Private/Tests/KalmalaInventoryNetworkContractTest.cpp:14-30`, and `Source/KalmalaGameplay/Private/Tests/KalmalaInteractionAuthorityTest.cpp:21-97`.
- **Affected path:** An owning client's reliable `ServerRequestInteract` requests against world actors.
- **Trigger:** An owning client sends repeated `ServerRequestInteract` RPCs. The handler obtains the server view point, performs `TraceInteractionCandidate`, and dispatches the hit without a server-side per-character cadence check. Inventory layout and hotbar RPCs have separate 50 ms and 100 ms gates, but this general interaction RPC has none.
- **Impact:** Repeated requests can make the server repeat view tracing and target dispatch work beyond normal button cadence. Target-specific authority and state gates still apply; no client-authoritative mutation or practical flood impact was established.
- **Evidence:** The RPC is declared `UFUNCTION(Server, Reliable)`. `ServerRequestInteract_Implementation` contains no interaction request timer before the view trace or dispatch. The assigned network contract checks the two bounded intent parameters, while the interaction authority fixture's cadence assertion covers automatic pickup scanning rather than this RPC. No peer flood was run.
- **Proposed repair:** Add a server-owned per-character cadence for interaction requests before tracing, keeping the first valid request immediate and preserving existing target-specific checks.
- **Dependencies:** After CR19 review acceptance; this is a review handoff and does not authorize implementation during CR04.
- **Acceptance:** A remote owner can still perform an ordinary interaction immediately; a burst of repeated interaction requests results in at most one dispatch per configured cadence window, while invalid intent, range, authority and inventory checks remain intact.
- **Focused validation:** Add a focused listen-server owner scenario that issues rapid interaction requests, verifies the first accepted request and bounded subsequent dispatches, and confirms a normally paced interaction still succeeds.

## RF43 — Suppress M9 harvest gain receipts after persistence rollback (P3)

- **Source:** CR04-017; `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:411-427`, `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:450-470`, `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:258-263,311-318`, `Source/KalmalaGameplay/Private/KalmalaGemstoneNode.cpp:71-78`, `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9520-9585,11930-11945`, `Source/KalmalaGameplay/Private/Tests/KalmalaM9SourceAcceptanceTest.cpp:1143-1150`, and `Source/KalmalaUI/Private/KalmalaNotificationSubsystem.cpp:242-246`.
- **Affected path:** A forest wood-drop harvest, M9 hand-gathered crop, or gemstone claim accepted by the server for an owning player, followed by player/world persistence rejection.
- **Trigger:** `TryCommitStacksFromServer` updates stacks and calls `RecordAcceptedGains`, which immediately sends each positive delta through reliable owner RPC `ClientAcceptedGain`. The harvest handlers then call `PersistPlayerStateFromServer`; gemstone claim calls `PersistGemstoneState`. If a player save or gemstone population-slot write fails, each handler restores inventory, but the earlier receipt remains in the owner's buffer and can be shown as an item-gain notice.
- **Impact:** The owner can be told that an item was gained even though the corresponding M9 action returned failure and restored the prior inventory. This is a feedback/transaction-consistency defect; no authority bypass, peer disclosure, or duplicated durable reward was established here.
- **Evidence:** Inventory commit records gains before returning. The tree and crop handlers commit before persistence and call `TryCommitStacksFromServer` with the prior stacks on failure. `PersistPlayerStateFromServer` can return false for invalid/mismatched state or a failed `SaveGameToSlot`; `PersistGemstoneState` restores its prior save row and returns false when its slot write fails. The M9 source acceptance test sets the wild-onion pawn's PlayerState to null, then checks inventory rollback and that depletion did not occur, but does not inspect `GetGainReceipts` or a remote owner. RF16 separately covers failed durable population depletion, RF36 covers treasure claims, and RF38 covers M9 discovery rewards; this finding is limited to M9 harvest and gemstone feedback.
- **Proposed repair:** Defer or stage the owner gain receipt until the associated player or world persistence step succeeds, and discard the staged receipt when inventory is rolled back. Preserve the existing server-side source, capacity, and persistence gates.
- **Dependencies:** After CR19 review acceptance; this is a review handoff and does not authorize implementation during CR04.
- **Acceptance:** Injected player-save failures for forest and hand-gathered M9 harvests, and a gemstone-slot write failure, leave inventory and source/save state unchanged and produce no owner gain receipt. A retry after persistence recovers grants only the exact accepted reward and produces one receipt. A remote owner receives the receipt while observers receive no private inventory or gain detail.
- **Focused validation:** Extend the isolated source/gemstone transaction automation to inspect receipt state at rollback and retry; then run a listen-server remote-owner scenario with the existing harvest/gemstone coverage after an Editor build.

## RF44 — Add remote Opal Helm Forge-maintenance coverage (P3)

- **Source:** CR04-018; `Source/KalmalaGameplay/Private/Tests/KalmalaOpalHelmTest.cpp:13-79`; `Source/KalmalaGameplay/Private/KalmalaOpalHelm.cpp:59-89`; `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:407-415`; `Source/KalmalaGameplay/Public/KalmalaInventoryComponent.h:37-45`; `Scripts/Tests/Inventory/ItemPeers.ps1:4-48`; `Scripts/Tests/Progression/GemMiningTreasurePeer.ps1:33`; `docs/07-development-setup.md:4932-4935`.
- **Affected path:** An owning remote client upgrading or repairing its equipped Opal Helm at a Forge while the server maintains the gear level and owner-only result.
- **Trigger:** Treating the standalone `Kalmala.Gameplay.Inventory.OpalHelm` fixture as evidence for the client RPC route. It calls `MaintainOpalHelmFromServer` and inventory methods directly, not `ServerUpgradeOpalHelm` or `ServerRepairOpalHelm`. The existing item-peer scenario list has no Opal Helm route; the A9 peer case checks accessory stats, and the setup guide specifies an Opal inventory render for the maintenance controls.
- **Impact:** Regressions in remote RPC ownership routing, the crafting request cadence, owner result delivery, or the level update through owner-only equipped gear can go undetected. The inspected server path validates authority, exact equipped GUID, same-world visible Forge/range/level, material costs, and a 200 ms request cadence; no current authority bypass was found.
- **Evidence:** The local automation directly invokes the server helper in its success and rejection cases. The two reliable server RPC implementations each call `AcceptRequest` before dispatching maintenance. `EquippedGear` carries `Level` and is replicated with `COND_OwnerOnly`; generic inventory peer coverage hides gear from observers but does not send Opal maintenance requests or observe a level change. No Opal maintenance scenario was found in the peer scripts.
- **Proposed repair:** Add a listen-server scenario with a remote owner and observer that invokes the actual upgrade and repair RPCs. Reuse the local test's catalogue/cost checks while exercising Forge level, range/visibility, exact GUID, insufficient-material, and 200 ms cadence rejection through the network path. Verify owner feedback and updated level/condition, and retain the existing observer assertion that equipped gear stays private.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** The remote owner upgrades with the authored payment at the matching Forge level and repairs worn Opal Helm for free at a valid Forge. Wrong or foreign GUID, wrong Forge level, distant/occluded Forge, insufficient material, and too-rapid repeat requests do not change level, condition, or payment. The owner receives the updated gear and one owner-only result; the observer receives no private equipped-gear data.
- **Focused validation:** Build the Editor target; run `Kalmala.Gameplay.Inventory.OpalHelm`, the new three-role Opal Helm peer scenario, and the existing `Scripts/Tests/Inventory/Inventory.ps1` owner-only inventory check.

## RF45 — Verify detailed skill replication privacy with an observer (P3)

- **Source:** CR04-020; `Source/KalmalaGameplay/Private/KalmalaSkillProgressionComponent.cpp:46-67`, `Source/KalmalaGameplay/Private/Tests/KalmalaSkillProgressionReplicationTest.cpp:11-46`, and `Scripts/Tests/Construction/Crafting.ps1:22-46`.
- **Affected path:** The server-owned `UKalmalaSkillProgressionComponent` on a replicated player pawn.
- **Trigger:** A regression changes the replication condition for `DetailedProgression` or stops delivering its detailed ledger to the owning player while retaining the aggregate `PeerPresentation`.
- **Impact:** Other relevant players could receive another player's per-skill experience, levels and unlocks, or the owner could lose progression detail while the public badge still looks correct. The current component uses `COND_OwnerOnly` for `DetailedProgression`; no existing disclosure was observed.
- **Evidence:** `KalmalaSkillProgressionReplicationTest` calls `BuildPeerPresentation` with an in-memory ledger and validates derived values; it does not instantiate replicated peers or inspect which connection receives `DetailedProgression`. The crafting peer runner starts a listen host and one remote client and its acceptance predicates do not check skill fields or include a separate observer. The component publishes detailed state plus a derived badge, then declares the detailed property owner-only and the badge relevant to peers.
- **Proposed repair:** Add a listen-host, remote-owner and observer scenario that earns skill experience through an accepted server action. Verify that the owner receives the exact detailed ledger, the observer receives no per-skill entries or experience, and both relevant peers receive only the aggregate badge.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** In a three-role peer run, accepted progression updates the remote owner's detailed six-skill state; the observer has no detailed state for that pawn; both peers receive only the validated highest-level/unlock presentation; no client-authored skill outcome is accepted.
- **Focused validation:** Build the Editor target, run `Kalmala.Gameplay.Progression.ReplicationContract` and `Kalmala.Gameplay.Progression.SkillContract`, then run the new three-role skill privacy scenario and the existing crafting owner presentation check.

## RF46 — Cover remaining wildlife defeat reward callbacks (P3)

- **Source:** CR04-021 and CR04-022; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-583`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8677-8814`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBadgerTest.cpp:91-147`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMoonBearTest.cpp:97-120`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeGiantSnakeTest.cpp:102-179`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeDeathAdderTest.cpp:124-227`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp:160-169`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeViperTest.cpp:89-95,148-150`; `Scripts/Tests/Combat/EnemyPeers.ps1:4-33,156-166`; `docs/07-development-setup.md:2401-2493,2612-2660`.
- **Affected path:** Badger, Moon Bear, Giant Snake, Death Adder, Peat Husk and Viper defeat dispatch through `AKalmalaWildlifeSpawn::DefeatServer`, their GameMode reward callbacks, owner-bound pending claims and persistent defeated-spawn state.
- **Trigger:** A callback mapping or handler change omits a species reward, supplies the wrong stable spawn ID or authenticated owner, or persists defeat before the claim is safely queued. Focused source tests still pass because they call `TryQueuePending*Reward` directly.
- **Impact:** A species can be defeated without the expected owner-bound reward claim or correct persistent defeat record, while its helper-level contract remains green. The shared Rat peer scenario covers generic reward retry, persistence and peer privacy but not these six callback mappings. RF18 tracks the same test gap for Wolf, Lynx, Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw. The inspected server path validates authority and identity; no current bypass or live failure was established.
- **Evidence:** The six selected species tests exercise save-game queue helpers, including identity, capacity, retry, optional outcome and serialization, but none calls the corresponding `Record*DefeatReward` method or `AKalmalaWildlifeSpawn::DefeatServer`. Production `DefeatServer` dispatches all six callbacks and each handler queues the species claim before marking the spawn defeated. The existing peer scenarios exercise species attacks; the Rat shared scenario remains the only live end-to-end reward, persistence and privacy check, and it does not exercise these callback mappings. The development guide calls for combined I4 acceptance of Badger/Moon Bear and describes the Rat scenario as the shared case; neither supplies focused regression coverage for these six callback paths.
- **Proposed repair:** Add isolated GameMode/`DefeatServer` automation for the six species using a test save writer or isolated save root. Assert stable spawn identity, authenticated owner, exact claim outcome, queue-before-defeat persistence order, retry idempotency and rejection of mismatched identity/world cases.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** Each of the six actual species defeat dispatches creates one owner-bound claim with the expected persistent spawn ID and outcome, then persists the matching defeated ID. Retry is idempotent and preserves any optional roll; invalid world, identity or queue cases create no misplaced claim and do not write a false defeat record.
- **Focused validation:** Build the Editor target, run the new isolated callback automation and the six existing species contract automations. Use the existing Rat peer scenario for the shared delivery/restart/privacy path; a separate per-species peer scenario is not required for those shared checks.

## RF47 — Exercise finite-world movement bounds through remote prediction (P3)

- **Source:** CR04-022; `Source/KalmalaGameplay/Private/Tests/KalmalaWorldBoundaryMovementTest.cpp:9-31`; `Source/KalmalaGameplay/Private/KalmalaCharacterMovementComponent.cpp:167-182`; `Scripts/Tests/Presentation/PlayerControls.ps1:28-44,66-81`; `docs/02-technical-architecture.md:845-849`.
- **Affected path:** A remote owning client moving a character toward the circular 16 km world edge, including the server's accepted Character Movement result and owner prediction/correction.
- **Trigger:** A change constrains the local authority pawn but stops applying the same radius-minus-capsule clamp to autonomous owner prediction, or allows owner movement and server correction to diverge at the boundary.
- **Impact:** A remote player may briefly display outside the finite world or fail to converge to the server-clamped position. The inspected component currently runs the clamp on authority and non-simulated owner prediction, while simulated proxies follow replication; no current bypass was observed.
- **Evidence:** `KalmalaWorldBoundaryMovementTest` creates one transient world and authority pawn, teleports it beyond the edge, and manually calls `OnMovementUpdated`; it checks the capsule margin and radial, tangential and vertical velocity components but creates no client/server movement session. The existing `PlayerControls.ps1` peer run checks remote jump, sprint, release, landing and world identity without staging a pawn near the world edge or asserting radius. Architecture explicitly requires the finite radius clamp on both authority and owner prediction.
- **Proposed repair:** Extend a listen-server movement scenario with a remote owner staged near the boundary and an observer. Drive the pawn outward through normal movement input, then verify the accepted server transform and owner prediction converge within the radius-minus-capsule margin while tangential and vertical movement remain available.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** Normal remote movement cannot leave the finite-world radius on either owner or server after correction. Outward velocity is removed at the boundary; tangential and vertical components remain unchanged, and the observer receives the same accepted bounded transform.
- **Focused validation:** Build the Editor target, run `Kalmala.Gameplay.Movement.WorldBoundary`, and run the extended remote-owner/observer movement scenario through `Scripts/Tests/Presentation/PlayerControls.ps1`.

## RF48 — Reject ineligible melee requests before target traces (P3)

- **Source:** CR04-023; Source/KalmalaGameplay/Public/KalmalaCombatComponent.h:25,47-60; Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:15-20,34-56,171-203; RF42 covers the separate ServerRequestInteract trace path.
- **Affected path:** A remote owning client's reliable ServerRequestAttack request and the server's melee target selection.
- **Trigger:** A client sends stale-sequence requests, or fresh requests while an action is already unavailable. The handler resolves the held item and calls FindServerTarget() before IsAttackRequestAllowed checks server authority, sequence freshness, idle/bow state and swing availability. FindServerTarget() can execute both a visibility line trace and a 60 cm lower sphere sweep; the attack is rejected only afterward. The unavailable-feedback timer limits feedback publication, not these scene queries.
- **Impact:** Rejected reliable RPCs can still consume authoritative scene-query work. A client able to send a burst can amplify server CPU cost. The inspected ordering does not allow the request to choose a target or bypass hit/damage validation; no combat authority bypass was established.
- **Evidence:** ServerRequestAttack is declared Server, Reliable. Its implementation obtains Target = FindServerTarget() before computing and checking bAllowed; IsAttackRequestAllowed requires authority, a fresh nonzero sequence, an idle action and a valid swing. FindServerTarget issues the line trace and, absent a valid first target, the lower sweep. Existing combat fixtures exercise accepted and rejected outcomes but were not run as remote request bursts. Static inspection only; no trace counter or network flood was measured.
- **Proposed repair:** Run the inexpensive authority, sequence, action-phase, bow-draw and swing-availability checks before target selection. Only eligible requests should perform the target trace/sweep; preserve the existing server-side target validation and impact-time checks. Add request cadence only if the reordered admission still permits unbounded query work.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** Stale, busy, bow-drawing, unavailable-swing and non-authority requests perform no target trace or sweep. A fresh eligible request performs the expected target query, and subsequent impact still accepts only a server-validated target with canonical damage.
- **Focused validation:** Extend the combat request contract test to assert trace count for rejected versus eligible requests. After an Editor build, run the combat action/item acceptance tests and a remote-owner burst with a server trace counter; verify ordinary attack timing and target/damage gates remain intact.

## RF49 — Bound repeated Home-return persistence attempts after save failure (P3)

- **Source:** CR04-023; Source/KalmalaGameplay/Public/KalmalaCharacter.h:299-303; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1645-1661; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9520-9585,9635-9654; docs/02-technical-architecture.md:324-335; docs/startmenu.md:5-13.
- **Affected path:** A remote client's reliable ServerRequestReturnToHome request while the server cannot persist that player's current state.
- **Trigger:** TryReturnNetworkClientToHomeFromServer adds the controller to PendingNetworkHomeReturns, synchronously calls PersistPlayerStateFromServer, then removes the marker when persistence fails so the documented client retry can work. A subsequent request repeats copying the current save through SaveGameToMemory/LoadGameFromMemory, rebuilding and validating the candidate, and attempting SaveGameToSlot. There is no per-controller retry interval on this failure branch.
- **Impact:** During a persistent storage failure, a client request burst can repeatedly perform serialization and disk-write work on the authoritative server and send repeated failure replies, increasing host CPU/I/O pressure. The first successful request remains protected by the pending-return marker. No save ownership bypass was established, and the documented retry behavior is valid.
- **Evidence:** The reliable server RPC calls ProcessReturnToHomeRequest for every invocation. The GameMode clears the pending marker on a failed player save. PersistPlayerStateFromServer serializes a candidate and calls SaveGameToSlot; its test gate and the platform write can fail. The Home-return contract explicitly leaves a rejected client connected and able to retry. No request cadence or failure-injection burst was run; this is a static path finding.
- **Proposed repair:** Keep the first save attempt immediate and preserve eventual client retry, but apply a server-owned per-controller backoff to repeated attempts after failure. Return a lightweight rejection during the backoff without rebuilding or writing another save candidate.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** The first request attempts persistence and a failed write leaves the client connected. Repeated requests inside the retry interval do not repeat candidate serialization or slot writes; a later request can retry, and success retains the existing one-return guard.
- **Focused validation:** Add a save-failure automation that counts candidate/write attempts across repeated requests, verifies one later retry succeeds, and confirms successful returns remain idempotently guarded. Then run the existing client Home-return peer path with injected failure and recovery after an Editor build.

## RF50 — Retire the remaining map-sharing RPC (P3)

- **Source:** CR04-025; `Source/KalmalaGameplay/Public/KalmalaMapAwarenessComponent.h:24-71`; `Source/KalmalaGameplay/Private/KalmalaMapAwarenessComponent.cpp:12-15,48-62,143-172`; `Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:330-334,457-464`; `docs/02-technical-architecture.md:310`.
- **Affected path:** A remote owning client invokes `ServerSetSharingEnabled` on its replicated player-controller component; the server writes `bMapSharingEnabled` on that player's state and forces an update to peers.
- **Trigger:** A client repeatedly invokes the reliable RPC, including with the current value. The handler checks authority and player eligibility but has no cadence or duplicate-state gate and calls `ForceNetUpdate` on every request. The RPC and globally replicated flag remain in production code even though the current map contract excludes co-op sharing controls/status and remote pawn markers.
- **Impact:** A client can drive unnecessary server work and repeated PlayerState replication scheduling through a stale map-sharing endpoint. The old marker/status API still derives peer pawn positions and exposes a co-op status string. The current map widget only submits and draws temporary pings and does not consume those APIs, so no current player-visible marker/status disclosure was established. Request-flood impact was not measured.
- **Evidence:** `AKalmalaMapPlayerState::GetLifetimeReplicatedProps` uses unconditional `DOREPLIFETIME` for `bMapSharingEnabled`. `SetSharingEnabled` invokes the Server, Reliable RPC; its implementation accepts the client boolean subject to active/spectator/pawn eligibility, assigns the shared flag and always calls `ForceNetUpdate`. `GetPeerMarkers` enumerates sharing-enabled peers and their current pawn locations; `GetStatusText` reports sharing state and peer count. UI call-site search found only `RequestPing` and `GetVisiblePings` in the expanded-map widget; test-only self-toggles are behind the non-Shipping verification flag. The architecture contract says the map has no sharing opt-in/status or remote pawn positions, with temporary pings as its only peer-visible action.
- **Proposed repair:** Remove the obsolete sharing RPC, replicated consent flag, peer-marker/status helpers, and their old test expectations while retaining the independent validated ping path. If a future approved feature restores location sharing, define its privacy contract and server-owned request cadence before adding a client request surface.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** Current builds expose no client-callable map-sharing RPC or globally replicated sharing state, and the map presents only bounded temporary pings. A remote-client check confirms no remote marker or co-op status is exposed while valid pings still use the existing two-second cooldown, lifetime, inbox bound and owner-only delivery.
- **Focused validation:** Update the map-awareness automation to remove the legacy sharing scenario and assert the current no-sharing contract. After an Editor build, run `Scripts/Tests/World/MapAwareness.ps1` and a remote-owner/observer map check for ping validation, cooldown, expiry, inbox bounds and owner-only delivery; statically confirm no production call sites remain for sharing APIs.

## RF51 — Bound repeated arrow-stack preference requests (P3)

- **Source:** CR04-032; `Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:1431`, `Source/KalmalaGameplay/Public/KalmalaCombatComponent.h:32,55`, `Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp:118-128`, `Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:254`, and `Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp:67-71`.
- **Affected path:** A remote owning client selects or clears its preferred arrow stack from the inventory menu; the server stores an owner-only preference used to choose bow ammunition.
- **Trigger:** The reliable ServerPreferArrowStack handler has no request cadence or duplicate-state check. A valid GUID scans the server inventory stacks and unit GUIDs; each valid selection calls ForceNetUpdate. An invalid GUID is the clear intent and also calls ForceNetUpdate on every request, including when the preference is already empty.
- **Impact:** A remote owner can cause repeated authoritative inventory lookup and actor replication scheduling for a preference that is already current. The property is owner-only and the request does not spend ammunition or change shot results by itself. No authority bypass or measured flood impact was established.
- **Evidence:** KalmalaInventoryMenuWidget::PreferSelectedArrows sends the selected stack or unit GUID, or an invalid GUID to clear. ServerPreferArrowStack_Implementation checks authority, resolves valid GUIDs against supported arrow stacks actually in the owner's inventory, and updates PreferredArrowStack; it has no time gate or same-value early return. The property is replicated with COND_OwnerOnly. Kalmala.Gameplay.Combat.BowDrawAndArrows calls the RPC wrapper in a local authority test for selection, forged GUID rejection and clear, but has no remote owner, observer, cadence or repeated-request assertion. No live request burst was run.
- **Proposed repair:** Add a server-owned cadence before lookup or clear and avoid ForceNetUpdate when the resolved preference is unchanged. Preserve an immediate first selection/clear and the existing exact inventory ownership check.
- **Dependencies:** After CR19 review acceptance; this finding is a review handoff and does not authorize implementation during CR04.
- **Acceptance:** The remote owner can select and clear a supported owned arrow stack immediately. Repeated or alternating calls inside the configured cadence cause no repeated inventory resolution or replication scheduling; later requests can update the preference. Invalid/foreign GUIDs cannot select another player's stack, and observers receive no preference.
- **Focused validation:** Extend Kalmala.Gameplay.Combat.BowDrawAndArrows for repeated and unchanged requests. After an Editor build, exercise the actual RPC with a remote owner and observer, checking ordinary selection responsiveness, bounded repeat work and owner-only replication.

## RF52 — Verify remote ownership of world-seed selection (P3)

- **Source:** CR04-039; Source/KalmalaGameplay/Public/KalmalaMenuContext.h:18,28-35; Source/KalmalaGameplay/Private/KalmalaMenuContext.cpp:34-73,115-142; Source/KalmalaGameplay/Private/Tests/KalmalaMenuContextTest.cpp:8-19; Source/KalmalaUI/Private/Tests/KalmalaWorldSeedParserTest.cpp:8-38; docs/startmenu.md:294-320.
- **Affected path:** A remote menu controller invokes the reliable ServerRequestWorldSeedSelection RPC. If accepted, the GameMode enumerates server-local save names, resolves whether the requested seed is an existing current-revision identity, and stores a pending selection.
- **Trigger:** A future regression removes or bypasses the local-controller requirement in ProcessWorldSeedSelectionRequest, or otherwise allows a remote controller to reach TrySelectWorldSeedFromServer. The existing parser and UI tests would still pass.
- **Impact:** A remote client could infer whether a server-local seed is listed as a current-revision world and alter the GameMode's pending selection. No current bypass or disclosure was observed; production currently requires authority and IsLocalController before the server-local resolver runs.
- **Evidence:** KalmalaWorldSeedParserTest exercises decimal parsing only. KalmalaMenuContextTest verifies menu class setup but does not invoke the request RPC. A targeted search across gameplay/UI tests and Scripts/Tests found no test invoking ServerRequestWorldSeedSelection or ProcessWorldSeedSelectionRequest. The server path currently returns a generic rejected result to a nonlocal controller; accepted decisions expose the NoKnownRecord versus ConfirmExisting classification. docs/startmenu.md says only the server-local menu controller may select a seed and other clients cannot change the host selection.
- **Proposed repair:** Add Development-only replicated-controller coverage that invokes the actual reliable RPC from a remote menu controller while the server has both an unlisted seed and an exact current-revision saved seed. Assert both remote requests are rejected before save enumeration, the reply does not reveal the server's save classification, and the server's pending selection is unchanged. Keep a local-controller case proving valid new-world and confirmed-reuse selections still work.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR04.
- **Acceptance:** A remote controller cannot enumerate or distinguish server-local world-seed records and cannot change pending selection; its request receives the generic rejection. The local menu controller can still select an unlisted seed or confirm an exact current-revision identity, and the launch route continues to revalidate the exact seed and confirmation.
- **Focused validation:** After an Editor build, run a focused Kalmala.Gameplay.MenuContext.WorldSeedSelection automation and the Development-only remote-controller scenario; also run Kalmala.UI.WorldSeedParser to retain decimal and uint64-boundary coverage.

## RF53 — Reconcile Automatic Pickup with the settings authority contract (P3)

- **Source:** CR04-042; `Source/KalmalaUI/Private/KalmalaSettingsSubsystem.cpp:139-166`; `Source/KalmalaUI/Private/KalmalaSettingsWidget.cpp:1606-1618,2258-2263`; `Source/KalmalaGameplay/Public/KalmalaCharacter.h:74-78`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1555-1573`; `Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:588-607`; `docs/02-technical-architecture.md:2648-2663`; `docs/07-development-setup.md:346-365`.
- **Affected path:** The locally persisted Accessibility > Automatic pickup choice is synchronized to the owning character, whose server-side tick scans nearby loose materials and broken-tree bundles.
- **Trigger:** A developer relies on the architecture section's blanket statement that settings send no RPC and cannot change inventory, although the current Settings subsystem sends `ServerSetAutomaticPickupEnabled` when the local choice or pawn changes and the server uses it to enable accepted inventory pickups.
- **Impact:** The source-of-truth contract conflicts with the shipped option and documented pickup behavior. Future changes may incorrectly remove the setting or apply the no-RPC rule to it. No current authority bypass was found: the server chooses eligible drops and validates range, visibility and capacity before granting inventory.
- **Evidence:** `IsAutomaticPickupEnabled` and `SetAutomaticPickupEnabled` read/write only `GGameUserSettingsIni`; `UKalmalaSettingsSubsystem::Tick` sends only the resulting boolean to the local character. The server implementation checks authority, and `UpdateAutomaticPickupFromServer` delegates to `TryAutomaticPickupFromServer`, which checks same-world ownership, the 150 cm radius, visibility and inventory capacity before the existing server grant. The preference itself is not replicated. `docs/07-development-setup.md` documents this behavior, while the architecture's local-storage section says settings send no RPC and cannot affect inventory. Existing focused coverage is named `Kalmala.UI.Settings.AutomaticPickup` and `Kalmala.Gameplay.Pickup.AutomaticRadius`.
- **Proposed repair:** Scope the architecture's no-RPC/no-gameplay claim to presentation and input preferences, and document Automatic pickup as the explicit local-persisted, owner-submitted gameplay preference. State that the RPC carries no target/item/quantity and that the server retains candidate selection, range, visibility, capacity and grants; align the settings acceptance wording without changing the feature.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during CR04.
- **Acceptance:** `docs/02-technical-architecture.md` and `docs/07-development-setup.md` describe the same Automatic pickup ownership and RPC boundary. The architecture no longer says this option cannot affect inventory, and still makes clear that clients cannot choose drops or grant quantities. No new replicated preference or saved-game field is introduced.
- **Focused validation:** After the contract update, statically compare the two documentation sections with the implementation. Run `Kalmala.UI.Settings.AutomaticPickup` and `Kalmala.Gameplay.Pickup.AutomaticRadius` if the repair changes code or test expectations; no source change is required to reconcile the current wording.

## RF54 — Reconcile the current HUD ownership contract (P3)

- **Source:** CR04-043; `Source/KalmalaUI/Public/KalmalaSupportSelectionSubsystem.h:84-85`; `Source/KalmalaUI/Private/KalmalaWeatherActivitySubsystem.cpp:32-38,229-232`; `Source/KalmalaUI/Private/KalmalaSurvivalStatusWidget.cpp:263-268`; `Source/KalmalaUI/Private/KalmalaStatusHotbarWidget.cpp:381-400`; `docs/02-technical-architecture.md:2774-2775`; `docs/07-development-setup.md:2898-2903,2959,3999-4002`.
- **Affected path:** The current UI ownership ledger and HUD verification guidance describe a full lower-left survival strip, a selected-support strip, and a separate weather-tier badge. Runtime keeps lower-left ocean-travel feedback, places active statuses and a Storm icon in the top-right hotbar, and retires the two standalone presenters.
- **Trigger:** A developer follows the current ownership or capture contract and expects to display or verify one of the retired widgets.
- **Impact:** The documented UI and acceptance path diverge from the current runtime layout, so future changes may target or test a surface that is not active.
- **Evidence:** `UKalmalaSurvivalStatusWidget::SetSnapshot` builds visible text only with `BuildOceanTravelText`; `UKalmalaSupportSelectionSubsystem::IsTickable` returns false and its comment identifies the M13 reusable-scroll migration; `UKalmalaWeatherActivitySubsystem::Tick` calls `ReleaseController`, and the subsystem contains no widget creation or update. The active status hotbar builds support-effect and high-storm entries. The architecture ownership ledger still describes the lower-left survival strip, selected-support strip, and separate weather badge as current. `PresentationOwnership.ps1` labels the support selector as retired. The M13 architecture already documents support-scroll activation.
- **Proposed repair:** Reconcile current ownership and M11/M13 verification guidance with the active HUD and reusable support-scroll path. Label older M7/M12 presenter notes as historical or superseded where needed, while retaining the local weather fog/material response contract. Do not imply a new HUD feature.
- **Dependencies:** After CR19 review acceptance; this is a documentation handoff only and does not authorize implementation during CR04.
- **Acceptance:** Current architecture and setup guidance describe the lower-left travel-only panel, the single active top-right status/weather hotbar, and no standalone selected-support strip or weather-tier badge. They retain the local fog/material response and mark older presenter verification as historical. No gameplay, network, or save behavior changes.
- **Focused validation:** Compare the revised current-contract sections with the widget lifecycle and hotbar sources. Run `Scripts/Tests/Acceptance/DocumentationContracts.ps1` if its assertions cover the revised wording; no Unreal build is needed for a docs-only repair.

## RF55 — Reject oversized pending population rewards on load (P3)

- **Source:** CR04-047; `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:41-48,99-116`; `Source/KalmalaWorld/Public/KalmalaWorldPopulationSaveGame.h:48`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:647-658,7829-7833,9046-9053`.
- **Affected path:** A server loads a same-world schema-2 population save and retries its pending wildlife rewards.
- **Trigger:** A structurally loadable slot contains more than 4,096 pending reward rows. `TryQueuePendingWildlifeReward` enforces the declared `MaxPendingWildlifeRewards` during normal mutations, but `MatchesWorld` omits the same count check. `LoadPopulationSave` accepts any record for which `MatchesWorld` succeeds, and GameMode copies and scans the pending rows on its 0.5-second retry interval.
- **Impact:** The intended row bound is bypassed for loaded data, allowing an oversized local save to cause repeated server allocations and validation work. No client-controlled path to create an over-limit record was found; this is a malformed or tampered save-boundary defect, not a demonstrated remote exploit.
- **Evidence:** `MatchesWorld` checks schema/world identity, gemstone and treasure caps, and gem-item mappings, but never checks `PendingWildlifeRewards.Num()`. The normal queue helper returns false once the 4,096-row cap is reached. `LoadPopulationSave` preserves and rejects records only when `MatchesWorld` returns false; the authoritative retry loop copies the accepted array and visits every row.
- **Proposed repair:** Reject pending reward arrays larger than `MaxPendingWildlifeRewards` in the complete load-validation contract, preserving the existing slot without rewriting it. Keep normal owner-bound retry and full-pack delivery behavior unchanged.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during CR04.
- **Acceptance:** A same-world schema-2 record with 4,096 pending rows remains valid; a structurally loadable record with 4,097 rows is rejected before GameMode retains it or performs reward scans. Rejected slot bytes remain untouched, and valid in-bound owner claims still retry normally.
- **Focused validation:** Extend the population-save contract automation with a serialized over-limit record and the 4,096/4,097 boundary; statically verify that `LoadPopulationSave` leaves a rejected existing slot untouched. No test was run during this review.

## RF56 — Cover the M7 sparse-delta count bound (P3)

- **Source:** CR04-048; `Source/KalmalaWorld/Private/Tests/KalmalaM7PersistenceContractTest.cpp:36-52`; `Source/KalmalaWorld/Public/KalmalaM7PersistenceContract.h:89`; `Source/KalmalaWorld/Private/KalmalaM7PersistenceContract.cpp:110,143-155`.
- **Affected path:** The M7 identity-scoped save accepts and reloads sparse world/player facts under the documented 256-row bound.
- **Trigger:** A future change removes or weakens the count checks while the current focused contract test still passes because it adds and reloads only three rows.
- **Impact:** The M7 save's sparse-record resource limit has no boundary regression guard. No current production bypass was confirmed; `Matches` rejects over-cap loaded data and `AddSparseDelta` rejects additions at the cap.
- **Evidence:** `MaxSparseDeltas` is 256. `Matches` rejects a stored array over that count, and `AddSparseDelta` rejects once the array already contains 256 rows. The test exercises three accepted kinds, duplicate/invalid IDs, and a three-row in-memory round trip, but has no 256/257 boundary or over-cap load case. The sparse IDs are documented as server-selected.
- **Proposed repair:** Extend `Kalmala.World.M7.PersistenceContract` with the 256/257 insertion boundary, unchanged count after rejection, and a valid maximum-size round trip. Add a malformed over-cap load case if the fixture can create one without writing a project save slot.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during CR04.
- **Acceptance:** A current M7 save with 256 valid unique sparse deltas remains valid; the 257th addition is rejected without changing the count; the valid maximum survives in-memory serialization/reload. If an over-cap record can be constructed through the existing fixture, `Matches` rejects it.
- **Focused validation:** Run `Kalmala.World.M7.PersistenceContract` after an editor build. No test was run during this review.

## RF57 — Correct the generated population save version in the architecture table (P3)

- **Source:** CR04-048; `docs/02-technical-architecture.md:1415`; `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:30-43`; `docs/05-decision-log.md:209`.
- **Affected path:** A developer consults the current save-version table before inspecting, rejecting, or testing generated population saves.
- **Trigger:** The architecture table says `UKalmalaWorldPopulationSaveGame` uses save schema 1, but `InitializeForWorld` writes schema 2 and `MatchesWorld` accepts only schema 2. The adjacent M7 sparse persistence ledger is a different save owner and correctly remains schema 1.
- **Impact:** The authoritative save-version reference gives the wrong current format and can lead to incorrect development-save or test expectations. This is documentation drift; no runtime mismatch or remote exploit was found.
- **Evidence:** The version table at architecture line 1415 says schema 1 for generated population deltas. The active population save code initializes and validates schema 2; the decision log explicitly records approval of population save schema 2 while preserving the separate M7 schema-1 ledger.
- **Proposed repair:** Update the generated-population row to schema 2 and keep it clearly separate from the M7 sparse ledger's schema 1. Retain the unreleased no-migration policy and reject existing schema-1 population saves without rewriting them.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during CR04.
- **Acceptance:** The current-version table identifies generated population saves as schema 2 and M7 sparse persistence as schema 1; wording agrees with runtime initialization/validation and the decision log without introducing compatibility migration behavior.
- **Focused validation:** Compare the corrected table against both save owners' current-version constants and the decision-log entry; no Unreal build is needed for a documentation-only repair.

## RF58 — Reconcile rejected Camp Bed interactions with the local rest pose (P3)

- **Source:** CR05-001; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1576-1589,1687-1782,1969-1971`; `Source/KalmalaGameplay/Private/KalmalaPlayerModelComponent.cpp:197-200,255-284`; `Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:118-132`; and `docs/02-technical-architecture.md:520`.
- **Affected path:** An owning client interacts with a Camp Bed and renders the local rest pose.
- **Trigger:** Local `RequestInteract` starts the pose when its selected bed passes `CanUse`, before the server's independent trace and interactable checks and the crafting `AcceptRequest` gate accept the action. A missed target or a server-side rejection has no pose cancellation path; the pose stops only after movement, health loss, or loss of local control.
- **Impact:** The local character can appear to rest even when the server did not accept the Rest action, and may stay posed indefinitely while stationary and undamaged. This is a presentation mismatch, not a gameplay authority bypass.
- **Evidence:** The client calls `StartCampBedRestPose` before sending the interaction RPC. The server retraces and dispatches only a current interactable target; the Camp Bed branch publishes acceptance only after the crafting request gate. `StopCampBedRestPose` is otherwise called only by model tick when its movement, health, or local-control condition fails. The architecture says the pose follows an accepted Rest action. Existing RF42 covers repeated interaction RPC work and is a separate issue.
- **Proposed repair:** Start the pose after the owner receives the server's accepted Rest result, or give each pending local pose a bounded prediction window that reverts on rejection or timeout. Keep movement and health as ways to end an accepted pose.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** An accepted server Rest action poses the local model; a missed trace, invalidated bed, rate-rejected action, or missing response leaves the original model transform restored without movement or damage. An accepted pose still ends on movement or health loss.
- **Focused validation:** Add a focused accepted/rejected interaction result check and a listen-host/remote-owner scenario. Verify the rejected and timed-out cases restore the original model transform. No runtime check was run during this review.

## RF59 — Keep skiff blockage details private to the helmsman (P3)

- **Source:** CR05-002; `Source/KalmalaGameplay/Public/KalmalaOceanSkiff.h:23-29,62,102-103`; `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:80-86,283-292`; and `docs/02-technical-architecture.md:1220,1226,1273`.
- **Affected path:** Relevant peers receive replication for a blocked skiff.
- **Trigger:** The server sets a specific blockage reason after invalid-water or collision rejection. The public header exposes `GetBlockReason`, and `BlockReason` is a replicated property registered with unconditional `DOREPLIFETIME`.
- **Impact:** Relevant passengers and observers can distinguish an invalid ocean footprint, generated-terrain collision, or sweep-limit response for another player's helm. The architecture allows blocked mode and movement to relevant peers, but sends failure detail only to the requester and lists movement, mode, and occupancy as the public replicated vessel state. This is excess replicated detail, not a gameplay authority bypass.
- **Evidence:** `BlockMovementAtLastSafeTransform` assigns the reason and forces an update; `GetLifetimeReplicatedProps` registers `BlockReason` without an owner condition. The existing ocean-travel feedback component uses `COND_OwnerOnly` for feedback and its serial.
- **Proposed repair:** Keep the detailed reason server-local or deliver it through the existing owner-only travel feedback path, while preserving public blocked-mode and movement replication.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during CR05.
- **Acceptance:** The helmsman receives the specific failure reason; relevant passengers and observers receive blocked mode and movement but cannot read the detailed reason.
- **Focused validation:** Exercise blocked movement with a helmsman, passenger, and observer in a listen-server session. Confirm the detailed result reaches only the helmsman while public mode and movement still replicate. No runtime check was run during this review.

## RF60 — Repair the selected carried-tool instance (P3)

- **Source:** CR06-001; `Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:443-463,1288-1328`; `Source/KalmalaGameplay/Public/KalmalaCraftingComponent.h:47`; `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:657-693`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1996-2008,2060-2073`; and `docs/05-decision-log.md:66-67`.
- **Affected path:** An owner selects one carried copy from a group of duplicate tools and requests an individual repair.
- **Trigger:** The inventory row retains a specific copy GUID and the client checks that copy's condition, but `RepairSelectedTool` sends only the shared item ID. The server resolves that ID by preferring the active copy of the same type, then the first matching carried copy.
- **Impact:** The server may repair a different copy than the owner selected, or report that the tool is already full while the selected duplicate remains worn. `Repair All` is a workaround at a valid station. No authority bypass was found; station and server checks remain enforced.
- **Evidence:** Inventory tool rows group duplicates by canonical item ID and retain a representative GUID. The selected-tool handler uses that GUID to validate the row, then calls `ServerRepairTool(Row.Id)`. The RPC accepts only `FName ItemId`; its server handler calls `FindToolDurabilityFromServer(CanonicalId)`, which first returns the active matching GUID and otherwise the first matching item ID. The project explicitly permits duplicate physical tool copies with separate GUIDs.
- **Proposed repair:** Pass the selected instance GUID to the server and validate that exact carried tool before repairing it, or make the UI explicitly present and repair the server-selected copy.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** With duplicate tools at different condition, individual repair changes only the selected GUID's condition. A wrong or unowned GUID, unavailable station, persistence failure, or already-full selected copy leaves all tool conditions unchanged.
- **Focused validation:** Add a server-side regression with an active full copy and a worn selected spare, then reverse their conditions. Verify only the selected copy changes and rejection changes neither. No test was run during this review.

## RF61 — Reconcile the current Bow feature summary (P3)

- **Source:** CR06-001; `docs/game-features.md:9-25,102-112`; `docs/01-game-design.md:3-25,60-81`; `Content/Data/GameCatalogues/recipes/stations/workbench.json:18-68,293-355`; `Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp:133-194`; and `Source/KalmalaGameplay/Private/KalmalaArrowProjectile.cpp:55-105`.
- **Affected path:** A developer or player consults the Bow summary for current acquisition materials, ammunition, and shot behavior.
- **Trigger:** The feature summary says Wooden Arrows are the only current ammunition, Bow String has no source, Bow costs 5 Wood plus 2 generic String, and shooting is unimplemented. Its earlier summary also says only Wooden Arrow is available.
- **Impact:** The current feature reference describes unavailable materials and omits implemented Stone/Bronze arrows and server shooting, which can mislead players and future changes. This is documentation drift; no runtime defect was found.
- **Evidence:** The current game-design contract describes 4 Sinew crafting one Bow String, Bow acquisition using 5 Wood plus 1 Bow String, Workbench recipes for Wooden/Stone/Bronze Arrows, and server shooting. The registered recipe data and Bow/projectile implementation confirm those paths. The feature summary contains no subsequent correction.
- **Proposed repair:** Reconcile or consolidate the Bow summary with the current game-design and catalogue contracts; label any retained historical description as superseded.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** The current feature summary matches current Bow String sourcing and costs, available arrow types, and implemented draw/release shooting. Historical details are clearly labeled and cannot be mistaken for current behavior.
- **Focused validation:** Compare the edited summary with the Bow design section, registered Workbench recipes, and server shot/projectile paths. Run the documentation contract checker if it covers this section; no Unreal build is needed for a docs-only correction. No test was run during this review.

## RF62 — Enable authored Workbench upgrades for Stone starter tools (P2)

- **Source:** CR06-002; `Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp:2210-2283`; `Source/KalmalaGameplay/Public/KalmalaCraftingComponent.h:50`; `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:376-386`; `Source/KalmalaGameplay/Private/KalmalaToolProgressionContract.cpp:410-417`; `Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolStationProgressionTest.cpp:36-37,77-100`; Stone entries under `Content/Data/GameCatalogues/item-level-up/costs/weapons/`; `docs/02-technical-architecture.md:3278`; and `docs/07-development-setup.md:5179`.
- **Affected path:** A player with a level-one Stone Axe, Knife, Mace, Pick, or Spear attempts its authored level-two Workbench upgrade.
- **Trigger:** The Stone Workbench UI resolves only `FindEntry(ItemId, 1)` and enables acquisition only while that tool is not carried. Its request sends only the item ID. Both the server progression path and the shared upgrade builder special-case Starter profiles to resolve level one instead of the next entry for the carried level.
- **Impact:** All five authored Stone starter-tool upgrades are unreachable through the Workbench. Their documented material costs and level rewards cannot be earned. No authority bypass was found; station and payment checks remain server-side.
- **Evidence:** Upgrade-cost JSON contains Workbench level-one upgrade records for all five Stone IDs; the progression catalogue derives same-item level-two entries. Architecture says Stone upgrades use Workbench, and the setup contract records level-up stats for Stone Axe. `RefreshStoneCraftState` gates its action on `GetCarriedToolLevel(ItemId) == 0`; both `ProgressToolFromServer` and `BuildServerUpgrade` select the Starter level-one entry regardless of carried level. The focused station transaction test exercises Bronze Axe progression, not a Stone upgrade.
- **Proposed repair:** Separate level-one acquisition intent from same-item upgrade intent in the Workbench UI/request path. Offer each Stone level-two upgrade when its level-one tool is carried, then let the server resolve and validate the exact predecessor/target step while retaining station, range, materials, capacity, persistence, and authority checks.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Each of the five carried level-one Stone tools can be upgraded at the required Workbench with its authored materials and receives the catalogued level-two stats. Missing materials, wrong station, absent or wrong-level predecessor, and failed persistence leave tool and inventory unchanged. Level-one acquisition remains available for an uncarried item; clients cannot choose costs or target levels.
- **Focused validation:** Extend `Kalmala.Gameplay.M9.ToolStationProgression` to cover all five Stone level-one-to-two transactions, exact costs, retained condition/identity, and rejection rollback, while preserving the acquisition case. Run the focused automation after the required Editor build. No test was run during this review.

## RF184 extension — CR13-027 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `Amber.uasset`, `Amethyst.uasset`, `AmethystNecklace.uasset`, `Antidote.uasset`, `Aquamarine.uasset`, `AquamarineRing.uasset`, `BadgerFur.uasset` and `BadgerFurVest.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting Amethyst, AmethystNecklace, Aquamarine and AquamarineRing uses recorded source paths in the removed `E:/dev/Kalmala/.wt/a9/` worktree. Antidote, BadgerFur and BadgerFurVest point into the removed `.wt/a8b/` worktree. Amber records an absolute source path under `E:/dev/Kalmala/`, which resolves only in this checkout.
- **Impact:** The imported textures remain usable, but seven source paths do not resolve and the Amber path is tied to one checkout; reimport is not portable to a clean mirror or another checkout.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 fingerprints and the current main-checkout files. The exact encoded source paths were inspected; seven are absent and Amber's path exists only in the main checkout. The earlier byte-identical CR13-027 parser verified the expected object paths, `Texture2D` class and embedded source `FileMD5` matches. All eight original PNGs were visually inspected and match their identities; `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for each ID, and `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. The icon manifest and current catalogue/runtime mappings agree for all eight identities. Amber's missing exact prompt remains under RF123; the other seven have provenance records.
- **Disposition:** Keep these eight source-path defects in the existing RF184 repair scope. After CR19 review acceptance, reimport from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-028 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `BadgerMeat.uasset`, `BasicTorch.uasset`, `BearsVigorScroll.uasset`, `BerryRecoveryDraught.uasset`, `BoarHide.uasset`, `BoarHideBoots.uasset`, `BoarHideCape.uasset` and `BoarHideHelmet.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** BadgerMeat points into the removed `E:/dev/Kalmala/.wt/a8b/` worktree. BearsVigorScroll's relative path resolves to the absent `E:/dev/Users/Ville/.codex/worktrees/a10-acceptance/` location. BoarHide points into the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/` worktree. BasicTorch, BerryRecoveryDraught, BoarHideBoots, BoarHideCape and BoarHideHelmet record absolute source paths under the current main checkout.
- **Impact:** The imported textures remain usable, but three source paths do not resolve and the other five are tied to one checkout; reimport is not portable to a clean mirror or another checkout.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 fingerprints and current `HEAD`. Each package has its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` class. The earlier byte-identical CR13-028 parser verified embedded source `FileMD5` matches; all eight original and prepared images remain unchanged. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs, and `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. The icon manifest, current catalogue/runtime mapping and visually inspected originals agree on these identities. The BoarHide prompt-provenance gap is already covered by RF123; no duplicate task is needed.
- **Disposition:** Keep these eight source-path defects in the existing RF184 repair scope. After CR19 review acceptance, reimport from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-029 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `BoarHideLeggings.uasset`, `BoarHideTunic.uasset`, `BoarMeat.uasset`, `BoneFragment.uasset`, `Bow.uasset`, `BowString.uasset`, `Bronze.uasset` and `BronzeArrow.uasset` under `Content/Kalmala/UI/Icons/Items/`, with matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Seven packages record absolute source paths under the main `E:/dev/Kalmala` checkout. `BoarMeat.uasset` records `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/Content/Kalmala/UI/Source/Icons/BoarMeat.png`, which no longer exists.
- **Impact:** Imported textures remain usable, but all eight reimport records depend on a particular checkout; BoarMeat path is stale and the other seven are checkout-bound.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 fingerprints and current `HEAD`. The prior parser package identities, `Texture2D` classes and embedded source MD5 results remain applicable to these byte-identical packages. All eight embedded source MD5s match the current project-local prepared PNGs; seven recorded paths resolve and BoarMeat path does not. The eight original PNGs were visually inspected and match the documented subjects; manifest/runtime mappings and catalogue definitions agree. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs, and `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. The BoarMeat prompt-provenance gap is already covered by RF103.
- **Disposition:** Keep these eight source-path defects in the existing RF184 repair scope. After CR19 review acceptance, reimport from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-030 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `BronzeAxe.uasset`, `BronzeHelmet.uasset`, `BronzeKnife.uasset`, `BronzeLeggings.uasset`, `BronzeMace.uasset`, `BronzePick.uasset`, `BronzeShield.uasset` and `BronzeSpear.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** `BronzeAxe.uasset` records `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/Content/Kalmala/UI/Source/Icons/BronzeAxe.png`, which no longer exists. The other seven packages record absolute source paths under the main `E:/dev/Kalmala` checkout.
- **Impact:** Imported textures remain usable, but the BronzeAxe source path is stale and the other seven reimport paths depend on this checkout; reimport is not portable to a clean mirror or another checkout.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 fingerprints and current `HEAD`. Prior byte-identical parser evidence confirms the expected package identities, `Texture2D` classes and embedded source `FileMD5` values; all eight embedded MD5s match the project-local prepared PNGs. BronzeAxe's encoded source path is absent and the other seven paths resolve. All eight original PNGs were visually inspected and match their icon identities; the manifest, catalogue and runtime mappings agree. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs, and `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. Exact prompt records for BronzeAxe, BronzeKnife, BronzePick and BronzeSpear remain covered by RF123.
- **Disposition:** Keep these eight source-path defects in the existing RF184 repair scope. After CR19 review acceptance, reimport from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-031 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `BronzeSword.uasset`, `BronzeTunic.uasset`, `CampBedKit.uasset`, `CampfireKit.uasset`, `Carrot.uasset`, `CarrotSeed.uasset`, `CauldronKit.uasset` and `CharcoalKilnKit.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** `CampfireKit.uasset`, `Carrot.uasset`, `CarrotSeed.uasset` and `CauldronKit.uasset` record source paths under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/` worktree. BronzeSword, BronzeTunic, CampBedKit and CharcoalKilnKit record absolute source paths under the main `E:/dev/Kalmala` checkout.
- **Impact:** The imported textures remain usable, but four source paths are stale and four depend on this checkout; reimport is not portable to a clean mirror or another checkout.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 fingerprints and current `HEAD`. The package records contain the expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` marker; all eight embedded source `FileMD5` values match their current project-local prepared PNGs. The four main-checkout paths resolve and the four worktree paths do not. All eight original images and prepared 64×64 icons were visually inspected and match their catalogue identities. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. The placed-object identities remain buildable-only in catalogue/runtime data. CauldronKit's manifest role mismatch is already queued under RF216; Carrot/CarrotSeed prompt gaps are covered by RF103; Campfire/Cauldron prompt retention is an open question under CR13-054. No portable item or grant was found, and no duplicate finding was added.
- **Disposition:** Keep these eight source-path defects in the existing RF184 repair scope. After CR19 review acceptance, reimport from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-032 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `Coal.uasset`, `ConstructionHammer.uasset`, `CookedBoarMeat.uasset`, `CookedDeerMeat.uasset`, `CookingRackKit.uasset`, `Copper.uasset`, `CopperOre.uasset` and `DeerCallScroll.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Coal, ConstructionHammer, both cooked-meat icons and CookingRackKit record source paths under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/` worktree. DeerCallScroll records a source path into the removed `a10-acceptance` worktree. Copper and CopperOre record absolute paths under the main `E:/dev/Kalmala` checkout.
- **Impact:** Imported textures remain usable, but six source paths are stale and two reimport paths depend on the current checkout; none is portable to a clean mirror or another checkout.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints and current `HEAD`. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` marker, and every embedded source `FileMD5` matches its checked-in prepared PNG. The six worktree paths do not resolve; the Copper and CopperOre paths resolve only under the main checkout. All eight original and prepared icons were visually inspected and match their subjects. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight identities; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. Catalogue and runtime checks confirm CookingRack is a direct-material buildable output mapped to the placed-object identity `CookingRackKit`; the catalogue does not define a portable CookingRackKit item, and the ambient-audio fixture's rejected kit-result expectation is already tracked under RF33. RF216 already covers its stale manifest inventory views. Coal and ConstructionHammer provenance gaps remain under RF123; cooked-meat provenance remains under RF103; Copper and CopperOre have exact prompt records, and DeerCallScroll's prompt and canonical scroll mapping are documented. No duplicate finding was added.
- **Disposition:** Extend RF184 from 43 to 51 active affected textures. After CR19 acceptance, reimport these eight packages from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-033 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `DeerHide.uasset`, `DeerMeat.uasset`, `DeerRootRoast.uasset`, `Densewood.uasset`, `DuskDiamond.uasset`, `DuskRing.uasset`, `FangArrows.uasset` and `FangSpear.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** DeerHide, DeerMeat, DeerRootRoast and Densewood record source paths under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/` worktree. DuskDiamond and DuskRing point into the removed `E:/dev/Kalmala/.wt/a9/` worktree; FangArrows and FangSpear point into the removed `E:/dev/Kalmala/.wt/a8b/` worktree. None of the eight recorded paths resolves in the current checkout.
- **Impact:** The imported textures remain usable, but their reimport records point to missing worktree files rather than the checked-in project-local prepared PNGs.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 fingerprints and current `HEAD`. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` marker; each embedded source `FileMD5` matches its prepared PNG. All eight encoded paths are absent. The eight original and prepared icons were visually inspected and match their subjects. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. The catalogue and runtime mappings agree with the icon manifest and authored item data: DeerHide and DeerMeat are creature drops, DeerRootRoast is a FryingPan recipe output, Densewood is a material, DuskRing uses DuskDiamond in its recipe, and FangArrows/FangSpear recipes and combat descriptions match their documented identities. Exact prompt records for DuskDiamond, DuskRing, FangArrows and FangSpear are present; DeerHide and Densewood provenance gaps remain under RF123, and DeerMeat and DeerRootRoast gaps remain under RF103. No duplicate finding was added.
- **Disposition:** Extend RF184 from 51 to 59 active affected textures. After CR19 acceptance, reimport these eight packages from the checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-034 catalogue icon source records

- **Related finding:** RF184 in `BACKLOG.md`.
- **Affected packages:** The active `FieldHatchet.uasset`, `FloorKit.uasset`, `ForestChanterelle.uasset`, `ForgeAnvilKit.uasset`, `ForgeKit.uasset`, `FrostSalt.uasset` and `FryingPanKit.uasset` packages under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`. The retired `Fibre.uasset` is excluded.
- **Trigger:** FieldHatchet, FloorKit, ForgeAnvilKit, ForgeKit, FrostSalt and FryingPanKit record paths under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild/` worktree. ForestChanterelle records an absolute path under `E:/dev/Kalmala/`; it resolves in this checkout but is checkout-bound and does not resolve in another checkout or clean mirror. Fibre also records the removed worktree path, but is a retired, intentionally unlisted package.
- **Impact:** Seven active imported textures remain usable, but six have stale source paths and ForestChanterelle's reimport path is tied to this checkout, leaving all seven non-portable.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints and current `HEAD`. All eight packages contain their expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity, `Texture2D` marker and embedded source `FileMD5` matching the prepared PNG. The six stale paths do not resolve; ForestChanterelle's absolute main-checkout path does. The eight original and prepared icons were visually inspected and match their subjects. `Scripts/Validate-UIIcons.ps1 -Category Catalogue` passed for all 152 identities and confirms the retired Fibre and String packages remain unlisted. `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. FieldHatchet maps to the canonical StoneAxe identity. FloorKit, ForgeAnvilKit, ForgeKit and FryingPanKit remain placed-object/buildable identities with direct-material construction recipes; their stale manifest views remain under RF216 and missing icon prompts under RF233. ForestChanterelle's project-local prompt and mapping are documented. FieldHatchet and FrostSalt provenance gaps remain under RF123; FrostSalt's unsupported food-preservation description remains under RF130. Fibre has no current catalogue item, recipe, manifest row or runtime mapping and is excluded from the active repair scope.
- **Disposition:** Correct the CR13-034 RF184 extension to include seven active packages, bringing the affected-texture count from 59 to 66. After CR19 acceptance, reimport the six stale packages and reimport ForestChanterelle from its project-local prepared PNG while preserving package identities, source pixels and texture settings; do not change packages during CR13.

## RF184 extension — CR13-035 catalogue icon source records

- **Related finding:** RF184 in `BACKLOG.md`.
- **Affected packages:** `GardenPlotKit.uasset`, `Garnet.uasset`, `GarnetNecklace.uasset`, `GiantScale.uasset`, `GoatHorn.uasset`, `GreenJade.uasset`, `GrindingStoneKit.uasset`, and `HearthBroth.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting these eight textures uses a recorded `RelativeFilename` from the temporary `i3` import mirror, the removed `.wt/a9` or `.wt/a8b` worktrees, or the removed `wt/m12-hud-feedback-rebuild` worktree.
- **Impact:** The imported textures remain usable, but source reimport depends on missing paths and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints and current `HEAD`. Each package contains the expected `/Game/Kalmala/UI/Icons/Items/<asset>` object path and `Texture2D` marker. The embedded source `FileMD5` equals the matching checked-in prepared PNG's MD5 for every package; all eight originals are 1254×1254 RGBA and all eight prepared PNGs are transparent 64×64 RGBA. The originals and prepared icons were visually inspected. All eight import paths are stale: GardenPlotKit resolves through a removed temporary `i3` mirror, Garnet/GarnetNecklace/GreenJade through `.wt/a9`, GiantScale/GoatHorn through `.wt/a8b`, and GrindingStoneKit/HearthBroth through `wt/m12-hud-feedback-rebuild`. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs, and `python -B Scripts/Validate-CatalogueReferences.py` passed for all 152 registered definitions and runtime icon mappings. The `GardenPlotKit` original has a documented buildable provenance record; the A9 prompts cover Garnet, GarnetNecklace and GreenJade; the A8b prompts cover GiantScale and GoatHorn. All eight original PNGs carry C2PA metadata identifying ChatGPT/gpt-image trained algorithmic media. HearthBroth's missing prompt-level record remains within RF103. `GrindingStoneKit` is a placed-object identity; its stale pack/storage manifest views remain under RF216 and its missing item-specific prompt/source record is added to RF233.
- **Disposition:** Extend RF184 from 66 to 74 active affected textures. Reimport these packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

## RF184 extension — CR13-036 catalogue icon source records

- **Related finding:** RF184 in `BACKLOG.md`.
- **Affected packages:** `HearthShieldScroll.uasset`, `HornBow.uasset`, `Iron.uasset`, and `IronAxe.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting the four packages uses recorded source paths under the removed `a10-acceptance` worktree, the removed `.wt/a8b` worktree, or the removed `wt/m12-hud-feedback-rebuild` worktree.
- **Impact:** The imported textures remain usable, but source reimport depends on missing paths and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package SHA-256 values match their CR01 coverage fingerprints. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<asset>` object path and `Texture2D` class. Every embedded source `FileMD5` matches its checked-in prepared PNG, and all eight original and prepared PNGs exist. The four paths listed above fail the current-checkout existence check; IronKnife, IronMace, IronOre, and IronPick resolve to project-local prepared PNGs. All eight focused `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` checks passed, as did `python Scripts/Validate-CatalogueReferences.py` for 152 registered definitions and canonical runtime icon mappings.
- **Disposition:** Extend RF184 from 73 to 77 active affected textures. Reimport these packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

## RF133 — Align README dedicated-server build guidance (P3)

- **Source:** CR16-001; `README.md:22-24`; `docs/05-decision-log.md:163-164`; and `docs/07-development-setup.md:2337-2345`.
- **Affected path:** A developer follows the README build guidance while using the installed Epic Games Launcher UE 5.8.2 distribution.
- **Trigger:** README identifies the Launcher engine installation, then instructs developers to build the editor and dedicated-server targets. The setup guide explicitly says the Launcher distribution does not support dedicated-server builds and says not to attempt that target with the installed engine.
- **Impact:** The README directs developers toward an unsupported target and conflicts with the accepted engine limitation. This is documentation drift; no runtime defect was found.
- **Evidence:** The dedicated-server section in `docs/07-development-setup.md` says a UE 5.8 source build or other server-capable distribution is required and marks the Launcher target build unsupported. The decision log records dedicated-server builds as deferred while the Epic Games Launcher engine is in use. The roadmap likewise makes dedicated-server acceptance conditional on server-capable engine availability.
- **Proposed repair:** Clarify that the installed engine supports the editor workflow and that dedicated-server builds require a server-capable UE 5.8 installation, linking the setup guide's conditional instructions.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** README setup guidance agrees with the decision log and development setup: it does not direct Launcher-engine users to build the server target, and it preserves the future server-capable-engine path.
- **Focused validation:** Compare the revised README statement with the decision log and dedicated-server section in the setup guide. Run the documentation contract checker if it covers this statement; no Unreal build is needed for a docs-only fix. No test was run during this review.

## RF160 — Align current world-generation revision and distance guidance (P3)

- **Source:** CR16-002 and CR10-006; `docs/04-roadmap.md:49`; `docs/05-decision-log.md:158,172`; `docs/07-development-setup.md:3671`; `docs/01-game-design.md:123`; `docs/02-technical-architecture.md:793,830,845`; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:328-330`; `Source/KalmalaWorld/Public/KalmalaRegionalTuning.h:91-92`; and `Source/KalmalaWorld/Private/Tests/KalmalaMasterMapTest.cpp:72`.
- **Affected path:** A developer plans generation previews, biome-boundary checks, or compatibility expectations from the roadmap, decision log, or setup guide.
- **Trigger:** The roadmap says revisions 1–6 remain compatible and revision 8 is an opt-in debug-stream identity; the September 11 decision row says preserve older layouts/schemas, assigns Mire a 2 km minimum, and repeats the revision-8 identity. The visualization example passes `-Revision=7`. The current consolidated architecture says generation compatibility versions/selectors and alternate stream-debug identity are removed and assigns Mire a 3–16 km range. The commandlet reads Seed, Size, and Extent but has no Revision parser; the live Mire minimum is 300,000 cm (3 km). Separately, `KalmalaMasterMapTest` labels its Mire assertion as a 2 km minimum while checking the 3 km tuning constant.
- **Impact:** A preview silently ignores the documented revision argument, and developers receive conflicting layout-compatibility and Mire-boundary expectations; the test's failure message also reports the wrong boundary when the assertion fails. No runtime generator defect was found in this review.
- **Proposed repair:** Align current roadmap, decision-log and setup guidance with the consolidated generator contract. Clearly label superseded historical revision decisions; remove the unsupported preview argument; state the 3 km Mire minimum; and update the test diagnostic to name the boundary it checks. Distinguish the remaining revision-7 save identity from a generator-layout selector.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Current guidance consistently states there is one generator layout and no command-line compatibility selector, uses the 3 km Mire minimum, and describes revision 7 only in the persistence-identity context where applicable. The Mire test diagnostic names its enforced 3 km minimum. Historical decisions remain identifiable as historical rather than current compatibility promises.
- **Focused validation:** Compare the updated docs with the commandlet's parsed arguments, the compiled regional tuning constants, and the consolidated architecture section. Run a documentation contract checker if it covers these claims; no Unreal build is needed for a docs-only repair. No test was run during this review.

## RF161 — Reconcile M13/M14 acceptance summaries (P3)

- **Source:** CR16-002 and CR16-003; `docs/01-game-design.md:174`; `docs/02-technical-architecture.md:773`; `docs/game-features.md:139-143`; `docs/README.md:16`; and `BACKLOG.md:1083-1084`.
- **Affected path:** A developer or reviewer checks whether M13/M14 has integrated-main acceptance and decides whether to repeat work or rely on its recorded gate.
- **Trigger:** The design summary says M14's separate isolated acceptance did not integrate its source; the architecture summary says M13 and integrated M14 acceptance are still open; `game-features.md` says M13/M14 integrated acceptance is pending; and the README describes M14 acceptance scope as remaining in the backlog. The active backlog records M13 integrated-main acceptance passed on 2026-10-11 and M14 integrated-main acceptance passed on 2026-10-12.
- **Impact:** The project documents disagree about the integration and acceptance state of two milestones, weakening the handoff and potentially prompting the wrong verification decision. No runtime behavior was found to be affected.
- **Proposed repair:** Update the design, architecture, feature summary and README to match the active M13/M14 records, or label older status text as historical. Distinguish M14's earlier isolated acceptance from integrated-main acceptance and point readers to verifiable current evidence.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Current M13/M14 status summaries agree with the active backlog and available verifiable evidence; older isolated results are dated and distinguished from integrated-main acceptance. Missing evidence targets are handled by RF167.
- **Focused validation:** Compare the revised status with the M13/M14 backlog rows and linked evidence. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF162 — Remove the superseded wetness-to-warmth effect (P3)

- **Source:** CR16-002; `docs/01-game-design.md:130,155`; `docs/02-technical-architecture.md:668-672`.
- **Affected path:** A designer or implementer uses the world-simulation table to determine whether player Wet reduces warmth.
- **Trigger:** The design table says soaked wetness makes players lose warmth. The later M3 override explicitly supersedes warmth-derived Wet penalties and replaces the old continuous player wetness/warmth values with the server-owned Wet status, whose listed effects are movement and stamina costs. Cold-biome warmth pressure is described separately.
- **Impact:** The earlier table can be read as an additional warmth penalty and can reintroduce the retired second player-moisture system. No current runtime defect was established.
- **Proposed repair:** Update the earlier Wetness row to match the accepted Wet-status and separate cold-pressure contracts, or label it as superseded historical design text.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The design summary does not attribute warmth loss to Wet and agrees with the later design override and consolidated architecture while preserving separate cold/warmth behavior.
- **Focused validation:** Compare the updated row with both Wet overrides and the current status/cold-pressure contracts. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF163 — Clarify the A9/I5 status in its acceptance handoff (P3)

- **Source:** CR16-002; `docs/a9-acceptance.md:3,9,36-44`; `PROGRESS.md:9584-9609,19121-19129`.
- **Affected path:** A reviewer uses the A9 handoff to determine whether the integrated I5 gate passed and which acceptance limitations remain.
- **Trigger:** The opening summary says A9 and combined I5 are complete. The next source-decision paragraph says the independent verification increment does not close I4 or I5. Later dated history retains open-gate statements; later A10 progress also says full A9/I5 acceptance remains open.
- **Impact:** The handoff does not distinguish a verified historical snapshot, the final integrated I5 result, and later broader A9/A10 audit limitations, so readers cannot determine which gate the open statements refer to.
- **Proposed repair:** Reconcile the current A9/I5 statement with the dated verification records. Preserve historical snapshots, but label each as-of state and identify any later finding or reopened gate separately; do not overstate current acceptance.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The handoff clearly identifies the tested commit and its I5 result, dates superseded open-gate statements, and distinguishes completed I5 from any later broader audit or remediation still open.
- **Focused validation:** Compare the revised handoff with the final I5 progress record and subsequent A10/review entries. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF164 — Reconcile the start-menu activation summary (P3)

- **Source:** CR16-003; `docs/startmenu.md:15-17,435-463`; `Config/DefaultEngine.ini:4`; and `BACKLOG.md:977-978,1054-1057`.
- **Affected path:** A developer follows the start-menu contract to determine whether packaged startup currently enters Home and whether final acceptance remains open.
- **Trigger:** The document says the front-end flow is not active and later says normal startup is unchanged with no menu world wired, while its SM03 text says packaged startup uses `L_Menu`, `GameDefaultMap` points to that map, and backlog child SM14.5 is checked. The plan also says to leave normal startup unchanged until SM14, while SM15 final acceptance remains unchecked.
- **Impact:** Current wiring, earlier implementation-stage statements, and remaining acceptance scope are mixed together, making the packaged launch contract difficult to determine. No runtime defect or change to actual startup behavior is asserted by this finding.
- **Proposed repair:** Update the current summary to match the checked SM14.5 route, label statements that described earlier implementation stages as historical, and distinguish current launch wiring from the still-pending SM15 integrated acceptance.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The document consistently describes packaged startup routing, editor startup, and the pending SM15 acceptance gate without presenting earlier stage notes as current behavior.
- **Focused validation:** Compare the updated text with `Config/DefaultEngine.ini`, SM14.5/SM15 backlog state, and the final acceptance criteria. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF165 — Restore or retarget the shared item-icon workflow link (P3)

- **Source:** CR16-003; `AGENTS.md:135`; `docs/README.md:22-25`; `docs/icon-provenance.md:4-8`; and `docs/07-development-setup.md`.
- **Affected path:** An agent or developer follows the mandatory item-icon authoring/import workflow before adding a gameplay item.
- **Trigger:** The rules, documentation index, and provenance guide link to `docs/material-icons.md`, but that file is absent from the checkout and tracked docs. The setup guide's historical batch commands do not provide the linked shared workflow as a maintained guide.
- **Impact:** The mandatory icon-generation and integration workflow cannot be reached through its canonical reference, making a required item-addition step difficult to follow.
- **Proposed repair:** Restore a concise maintained workflow guide at the referenced path or retarget all current references to an existing authoritative guide that covers the complete current process.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every current mandatory workflow reference resolves to an existing guide that covers icon preparation, provenance, import, runtime mapping, and catalogue validation.
- **Focused validation:** Check all references to the shared workflow and verify that the target explains the current icon pipeline. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF166 — Update creature-catalogue implementation status and schema wording (P3)

- **Source:** CR16-003; `docs/creature-catalogues.md:21-30`; `Content/Data/GameCatalogues.json:2,75`; `Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp:406-407`; and the Monster S01–S35 verification handoff in `PROGRESS.md`.
- **Affected path:** A designer or implementer reads the creature catalogue guide to determine whether creature behavior remains future work and which schema the runtime item/tool loader accepts.
- **Trigger:** The guide places defensive triggers, swarm behavior, AI movement, spawning/pack rules, and reward safety in a later engine implementation, and calls the loader schema-15. Monster S01–S35 final verification is recorded complete; the manifest and loader use schema 16. Creature catalogue files remain authoring data and are not parsed by the item/tool loader, so the repair must preserve that distinction.
- **Impact:** The guide can make completed behavior appear unimplemented and gives a stale schema version, while also risking confusion between authoring data and runtime loader support.
- **Proposed repair:** Describe verified S01–S35 behavior as implemented, identify only genuinely outstanding design decisions as deferred, and state that the schema-16 item/tool loader does not consume the `creatureFiles` category.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The current guide agrees with the completed S01–S35 handoff, schema-16 manifest/loader, and authoring-only creature-data contract without claiming that creature JSON is runtime-loaded.
- **Focused validation:** Compare the revised summary with the creature manifest, loader category handling, relevant current gameplay code, and S01–S35 final-verification record. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF167 — Repair missing M13/M14 evidence links (P3)

- **Source:** CR16-003; `docs/README.md:16`; `BACKLOG.md:1083-1084`; and the referenced `docs/evidence/m13/README.md` and `docs/evidence/m14/main-integration.md` paths.
- **Affected path:** A reviewer follows the M13/M14 integrated-main acceptance record to inspect the claimed verification evidence.
- **Trigger:** Both evidence targets are absent from the checkout and Git index, while the README calls M13 evidence retained and the backlog links both targets for passed integrated-main acceptance.
- **Impact:** The acceptance records cannot be independently followed through their cited evidence paths, weakening the verification handoff.
- **Proposed repair:** Restore the evidence records if retained elsewhere or retarget the links to existing verifiable records; revise any claim that evidence is retained if no such record is available.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every M13/M14 evidence link resolves to a retained record that supports the listed acceptance result, or the acceptance row clearly describes the available evidence and remaining limitation.
- **Focused validation:** Resolve every updated evidence link and compare the target contents with the listed M13/M14 acceptance claims. No Unreal build is needed for a documentation-only repair. No test was run during this review.

## RF168 — Use a registered food automation path in acceptance (P2)

- **Source:** CR16-005; `Scripts/Tests/Acceptance/AutomatedAcceptance.ps1:18-31,74-76`; and `Source/KalmalaGameplay/Private/Tests/KalmalaCookingHeatContractTest.cpp:18-20`.
- **Affected path:** A developer runs the M7 automated acceptance wrapper and expects its headless automation queue to complete.
- **Trigger:** The wrapper requests `Kalmala.Gameplay.Food.CampfireProcessing`, but no current source test registers that path. The current food contract registers `Kalmala.Gameplay.Food.CookingStationHeat`. The wrapper later requires a `Result={Success}` record for every configured path.
- **Impact:** The acceptance wrapper cannot satisfy its own per-test success check with the current source registrations, so the gate fails even when all registered automation tests pass.
- **Proposed repair:** Replace the stale path with the intended current registered food test and verify every configured automation path against current registrations.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every name configured by `AutomatedAcceptance.ps1` resolves to a registered automation test; the food entry matches the intended current test and the success loop accepts its result.
- **Focused validation:** Compare the configured list with current `IMPLEMENT_SIMPLE_AUTOMATION_TEST` registrations and run the focused automation queue when the remediation is implemented. No test was run during this review.

## RF169 — Apply the selected Editor path to every acceptance scenario (P3)

- **Source:** CR16-005; `Scripts/Tests/Acceptance/AutomatedAcceptance.ps1:4,85-116`; `Scripts/Tests/Inventory/InventoryReconnect.ps1:1,12-13`; `Scripts/Tests/Construction/Crafting.ps1:1,9-10`; `Scripts/Tests/Survival/ExposureRecoveryPeers.ps1:1`; and `Scripts/Tests/Progression/DiscoveryPeer.ps1:1,8`.
- **Affected path:** A developer invokes the acceptance wrapper with `-Editor` pointing to a valid nondefault Unreal installation.
- **Trigger:** The wrapper uses the selected path for its automation executable and passes it to `EnemyPeers.ps1`, but omits it for `ExposureRecoveryPeers.ps1`; the inventory, crafting, and discovery scripts also launch their own hardcoded default executable and do not expose an override.
- **Impact:** The wrapper's `-Editor` option does not select one engine installation for the complete run. On a machine where the default path is missing or differs from the requested executable, the initial automation can start with one installation and later scenarios fail to launch or use another.
- **Proposed repair:** Thread the selected executable through all child scenarios, adding child parameters where needed, or reject a nondefault override before running a partial suite.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every process launched by one acceptance run uses the selected Unreal installation, or an unsupported override fails before any scenario starts with a clear diagnostic.
- **Focused validation:** Statically trace the selected executable into every child process launch and run a narrow wrapper smoke check with a nondefault executable path when the remediation is implemented. No test was run during this review.

## RF170 — Require an exact world seed in peer acceptance (P2)

- **Source:** CR16-007, extended CR16-008, CR16-009, CR16-011, CR16-012, CR16-013, CR16-014, CR16-016, and CR16-017; `Scripts/Tests/Combat/EnemyPeers.ps1:219-220`; all five scripts under `Scripts/Tests/Construction`; `Scripts/Tests/Inventory/Inventory.ps1:251`; `InventoryMenu.ps1:60`; `InventoryReconnect.ps1:46`; `ItemPeers.ps1:127-128`; `Scripts/Tests/Presentation/InteractionPrompt.ps1:52`; `PlayerControls.ps1:66`; `SettingsAccessibility.ps1:260`; `Scripts/Tests/Progression/DiscoveryPeer.ps1:32`; `ExplorationRewards.ps1:36`; `Schema2CandidateReconnect.ps1:82`; `Scripts/Tests/Survival/ExposureRecoveryPeers.ps1:37`; `PersistedCampRestart.ps1:59`; `Scripts/Tests/Travel/OceanSkiffDiscoveryDisembark.ps1:129`; `OceanSkiffIntegratedReconnectJourney.ps1:139,150,162,183,197`; `OceanSkiffJourney.ps1:64`; `OceanTravel.ps1:39`; `Swimming.ps1:39`; `Scripts/Tests/World/MapAwareness.ps1:36`; `Scripts/Tests/World/WorldProfile.ps1:33`; `Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp:121-123`; and `docs/02-technical-architecture.md:280,795`.
- **Affected path:** A developer runs an enemy, construction, inventory, presentation, progression, Survival, Travel, map-awareness, or bounded world-profile peer scenario and expects each remote client to use the server's immutable generated-world seed.
- **Trigger:** `EnemyPeers.ps1` builds its seed check with `'(?!\\d)'`. PowerShell single-quoted strings preserve both backslashes, so the .NET regex looks for a literal backslash followed by `d` instead of rejecting a following digit. The reviewed Construction, Inventory, Presentation, progression, Survival, and Travel runners accept `Seed=418` as a substring, which also accepts `Seed=4180`. `MapAwareness.ps1` and `WorldProfile.ps1` make the same substring-only identity check and their other completion gates do not establish an exact numeric seed token.
- **Impact:** A peer scenario can pass its shared identity gate even though a client reports a different numeric world seed, weakening the same-world multiplayer acceptance check.
- **Proposed repair:** Make all peer-runner checks match a complete numeric seed token and reject any following digit.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every peer runner accepts the exact server seed and rejects a different numeric seed, including expected `41` vs. `418` and expected `418` vs. `4180`.
- **Focused validation:** Add exact-versus-prefix assertions for every changed peer gate and parser-check the runners; when implemented, run `-ListScenarios` where supported and controlled enemy, construction, inventory, presentation, and progression peer scenarios. No test was run during this review.

## RF171 — Enable reduced motion in the Crafting acceptance runner (P3)

- **Source:** CR16-008; `Scripts/Tests/Construction/Crafting.ps1:1-2,16-19,25,33`; `docs/07-development-setup.md:4465-4474,4498-4503`; and `Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp:5622-5625`.
- **Affected path:** A developer runs the documented second rendered Crafting/Favorites check at 1024x768, 150% text scale, high contrast, and reduced motion.
- **Trigger:** The setup guide passes `-ReducedMotion`, but `Crafting.ps1` does not declare or consume that option and builds host/client arguments with only text-scale and contrast settings. The runtime developer setting is applied through `-KalmalaUIDeveloperReducedMotion=`; the runner never passes it.
- **Impact:** The documented run does not enable reduced motion for either peer, so it cannot establish the reduced-motion presentation condition that the setup procedure claims to check.
- **Proposed repair:** Add a supported reduced-motion option to the runner and pass the corresponding developer setting to both peers; assert the resulting state in the evidence gate.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The documented reduced-motion invocation enables the setting for host and client, and the acceptance output confirms the state for both peers.
- **Focused validation:** Compare the parameter and launch-argument paths, parser-check the script, then run the documented rendered mode and inspect its retained host/client evidence. No test was run during this review.

## RF172 — Align inventory notification baseline expectations (P2)

- **Source:** CR16-009; `Scripts/Tests/Inventory/Inventory.ps1:234-240`; `Scripts/Tests/Inventory/InventoryReconnect.ps1:42`; and `Source/KalmalaUI/Private/KalmalaNotificationSubsystem.cpp:275,329`.
- **Affected path:** A developer runs the documented inventory baseline, rendered notification review, or two-visit reconnect acceptance.
- **Trigger:** Both inventory runners require `Notification owner baseline: ... Sources=5`; `Inventory.ps1 -NotificationReview` also requires the reconnect baseline with `Sources=5`. The current notification subsystem logs `Sources=4` for both markers. The exact predicates cannot match: Inventory.ps1 fails its post-scenario owner-baseline check (and its notification-review reconnect check also mismatches), while InventoryReconnect never satisfies its loop completion condition and times out.
- **Impact:** The documented inventory acceptance scenarios fail despite successful underlying inventory and notification fixtures, blocking review of those behaviors.
- **Evidence:** The two scripts contain literal `Sources=5` expectations. The current owner-baseline and reconnect-baseline `UE_LOG` format strings in `KalmalaNotificationSubsystem.cpp` emit literal `Sources=4`; no alternate matching source marker exists in the reviewed project sources. No live scenario was run.
- **Proposed repair:** Align the Inventory runner's owner and reconnect baseline predicates with the current fixture marker contract, preserving checks that the baseline is silent and has zero rows. If the intended source count is actually five, update the fixture and its semantics consistently instead.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The documented `Inventory.ps1`, rendered `-NotificationReview`, and `InventoryReconnect.ps1` runs accept the current intended source count, still require `Silent=1` and `Rows=0`, and pass only when their other peer predicates are satisfied.
- **Focused validation:** Compare every runner's required baseline string with the registered fixture log format, then run the focused inventory reconnect and rendered notification scenarios after the remediation. No test was run during this review.

## RF173 — Bound constrained-performance evidence paths (P3)

- **Source:** CR16-010; `Scripts/Tests/Performance/ConstrainedPerformance.ps1:4,6,41-45,95`; `Scripts/Tests/Travel/OceanSkiffDiscoveryDisembark.ps1:19-27`; and `docs/07-development-setup.md:4979`.
- **Affected path:** A developer runs a constrained-performance profile with a custom `-OutputDirectory` for the retained profile JSON, scenario transcript, host/client logs, shader working directories, and frame-time CSVs.
- **Trigger:** The wrapper converts the supplied output root to a full path and creates it without checking its length, then passes `<root>\Peers` to the nested runner. That runner appends `\Host\ShaderWorkingDir` and `\Client\ShaderWorkingDir` and creates those directories without a MAX_PATH check. The suffix from the wrapper root through the host shader directory is 28 characters; a valid 232-character root therefore reaches 260 characters. Other generated paths beneath the peers can be longer still.
- **Impact:** A custom root can pass the wrapper's initial path handling but fail when the nested runner or Unreal tries to create shader/config/capture paths under traditional Windows path limits. The run leaves a partial evidence directory and depends on long-path support that the project rules prohibit assuming.
- **Evidence:** `ConstrainedPerformance.ps1` accepts arbitrary `OutputDirectory` values and has no path-length predicate before `New-Item`; it passes `Join-Path $output 'Peers'` to the nested runner. The nested runner creates the host/client shader directories with no length check. The documented procedure requires a fresh output directory but sets no maximum path length. The default root measured 143 characters in this environment, yielding a 171-character direct shader path; no scenario was run.
- **Proposed repair:** Resolve the output root and validate every derived wrapper and peer path needed for directory creation and Unreal-generated files before creating any evidence directory. Reject overlong custom roots with the exact offending path and a short-root instruction.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A custom root whose derived output path would reach 260 characters is rejected before any directory or peer process is created; supported roots keep all required paths below 260 and retain the same evidence layout.
- **Focused validation:** Add boundary checks for the longest derived path immediately below and at 260 characters; parser-check both runners and verify the diagnostic occurs before output creation. No test was run during this review.

## RF174 — Reconcile map pin-entry help with the M12 input-label rule (P3)

- **Source:** CR16-011; `Scripts/Tests/Presentation/MenuInputCopy.ps1:31-32`; `Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:1309-1313`; `docs/02-technical-architecture.md:308`; `docs/04-roadmap.md:731-742`; and `docs/07-development-setup.md:1879-1891`.
- **Affected path:** A player enters a label for a personal map pin and sees save/cancel key names, while reviewers use the M12 input-label contract and copy audit to check where key names appear.
- **Trigger:** The widget draws `Optional label · Enter saves · Escape cancels`; the copy audit removes that exact text before checking its forbidden copy list. The architecture documents this behavior, while the M12 roadmap and setup text say input-binding/key names appear only in Options and that the expanded map omits them.
- **Impact:** The current source and audit permit a key-name prompt in the map, but other current contracts forbid it. Acceptance does not establish a consistent rule for this player-facing prompt.
- **Evidence:** `MenuInputCopy.ps1` replaces the key-name substring with `Optional label` before joining the world-map source into its audit corpus. `KalmalaWorldMapWidget.cpp` passes the original text to `DrawMapLabel` when `bPinLabelEntry` is true. The architecture lists Enter as the save action, but M12 roadmap/setup wording sets an Options-only boundary. No rendered scenario was run.
- **Proposed repair:** Resolve whether pin-label entry is an explicit exception to the M12 key-name rule. Align the prompt, roadmap/setup contract, and static audit; if the exception remains, document and assert that narrow scope rather than silently removing it from the audit.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** The architecture, M12 roadmap/setup wording, map prompt, and `MenuInputCopy.ps1` agree on the pin-label key-name behavior, and the test explicitly checks the resolved rule.
- **Focused validation:** Compare the prompt text and all relevant contract statements, then run the copy audit and inspect the rendered pin-label prompt after the accepted change. No test was run during this review.

## RF175 — Match complete input-key values in the baseline check (P2)

- **Source:** CR16-011; `Scripts/Tests/Presentation/LocalInputContract.ps1:62-74`; `Config/DefaultInput.ini:114`; and `docs/07-development-setup.md:1871-1875,3244-3248`.
- **Affected path:** A developer checks that the default local WorldMap action remains bound to `M` and that the other required keyboard/controller input mappings remain present.
- **Trigger:** The generated action pattern ends at `Key=M` without requiring the end of the key token. A mapping line `ActionName="WorldMap",Key=MouseX` therefore satisfies the required `M` test. The current default config has `Key=M`.
- **Impact:** The no-build input contract can accept a changed or removed required mapping when another key value starts with the expected key name, weakening the default-controls regression check.
- **Evidence:** Reconstructed the WorldMap pattern from the script and evaluated it against `ActionName="WorldMap",Key=MouseX`; the regex matched. The current config line maps WorldMap to `M`. No test script or game scenario was run.
- **Proposed repair:** Parse active `AxisMappings` and `ActionMappings` entries and compare the complete `Key` value against each required key, or otherwise enforce a key-token boundary.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The current WorldMap `M` and all required mappings pass, while replacing `M` with `MouseX` fails; prefix collisions in required axis/controller values are also rejected.
- **Focused validation:** Add a narrow config-fixture or parser assertion for exact key-token matching, then run `LocalInputContract.ps1` against the normal and altered fixtures. No test was run during this review.

## RF176 — Report the SettingsAccessibility capture count accurately (P3)

- **Source:** CR16-011; `Scripts/Tests/Presentation/SettingsAccessibility.ps1:202-209,337`; and `docs/02-technical-architecture.md:2682-2697`.
- **Affected path:** A developer uses the final PASS output to locate and understand the retained SettingsAccessibility evidence.
- **Trigger:** The script lists and validates eight PNG paths for each of two peers (sixteen total), but its final PASS message says fourteen PNG captures were retained.
- **Impact:** Successful-run output understates the actual evidence count and conflicts with the contract's sixteen host/client images, making the handoff summary inaccurate.
- **Evidence:** Counted the capture array entries and compared them with the final output literal and architecture acceptance statement. No scenario was run.
- **Proposed repair:** Derive the reported number from the validated capture array or change the literal to sixteen.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The PASS output count matches the number of distinct PNGs the runner verifies for the two peers.
- **Focused validation:** Statically count the capture paths and verify the output derives or reports the same count; parser-check the runner after the change. No scenario was run during this review.

## RF181 — Bound generated test evidence paths (P3)

- **Source:** CR16-012, extended CR16-013, CR16-014, CR16-015, CR16-016, and CR16-017; `Scripts/Tests/Progression/DiscoveryPeer.ps1:1,8-14`; `ExplorationRewards.ps1:1,8-16`; `Schema2CandidateReconnect.ps1:12-13,59-82`; `Scripts/Tests/Survival/ExposureRecoveryPeers.ps1:10-18`; `PersistedCampRestart.ps1:8-21,32,41,75-77,96`; `Scripts/Tests/Travel/OceanSkiffDiscoveryDisembark.ps1:19-27,49-51,145-146`; `OceanSkiffIntegratedReconnectJourney.ps1:14-15,33-35`; `OceanSkiffJourney.ps1:12-24`; `OceanTravel.ps1:9-12`; `Swimming.ps1:9-12`; `Scripts/Tests/World/BiomeExpansion.ps1:341-351`; `MapAwareness.ps1:9-17`; `Minimap.ps1:13-25`; `RegionalGeneration.ps1:10-16`; `WorldMap.ps1:19,36-42,55-63`; `WorldMapExport.ps1:7-10,26-27,50`; `WorldProfile.ps1:9-17`; and `docs/07-development-setup.md:2177-2183,2208-2236,2867,4332-4344,4381,4400`.
- **Affected path:** A developer runs Progression, Survival, Travel, BiomeExpansion, or World map/export acceptance under a long temporary root or supplies a long custom output root, expecting the runners to retain logs, isolated Unreal user data, shader working directories, screenshots, exported maps, and optional frame-time CSVs.
- **Trigger:** The three reviewed Progression peer runners, `ExposureRecoveryPeers.ps1`, the five Travel runners, integrated `BiomeExpansion.ps1`, and six World runners append generated names under `%TEMP%` or custom roots. They do not check all resulting absolute paths before `New-Item`, process launch, or output writing. `OceanSkiffDiscoveryDisembark.ps1` is also the nested runner in RF173; RF181 covers its direct/default-root paths.
- **Impact:** A valid temporary or custom root below 260 characters can produce a required path at or above 260 characters, causing directory creation, Unreal launch, or evidence writing to fail after partial setup. At a valid 200-character `%TEMP%` root, the WorldProfile server log measures 264 characters. Other World path examples remain 264 characters for the MapAwareness server log, 259 for the Minimap server log, 270 for the RegionalGeneration `User-automation` directory, 268 for the WorldMap `host.png` in the `2560x1080` case, and 275 for the WorldMapExport `MasterLandWater.png`; the Minimap log reaches 260 characters with a 201-character root. Previously recorded Progression, Survival, Travel, and BiomeExpansion examples remain as above. This environment's `%TEMP%` is 92 characters; measurements are static path arithmetic only, and no overlong directory was created.
- **Evidence:** Compared path construction with the existing preflight in `GemMiningTreasurePeer.ps1:7-9` and full derived-path validation in `FreshStartBronzeAndIron.ps1:32-46`. The seventeen affected runners do not preflight all derived output paths before creating directories, launching processes, or writing evidence. Lengths are static string arithmetic only. RF173 separately records the ConstrainedPerformance wrapper's custom-root path through the DiscoveryDisembark runner.
- **Proposed repair:** Resolve each output root and preflight every derived log, user-data, screenshot, image, and report path, including required Unreal-generated descendants, before creating the evidence directory or starting a process; reject paths at or above 260 characters with the exact offending path and a short-root instruction.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** Roots producing any required path at or above 260 characters are rejected before directory creation or process launch; supported roots keep all logs, user directories, and required generated descendants below the limit across the seventeen runners.
- **Focused validation:** Check roots producing longest derived paths just below and exactly at 260 characters for all seventeen scripts, then parser-check the runners and confirm rejection occurs before `New-Item`/`Start-Process`. No runner or test was executed during this review.

## RF183 — Require cross-world restore completion before accepting rejection (P2)

- **Source:** CR16-013; `Scripts/Tests/Survival/PersistedCampRestart.ps1:95-100`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-956`; `Source/KalmalaGameplay/Private/KalmalaCraftingVerification.cpp:590-615`; and `docs/07-development-setup.md:4330-4344`.
- **Affected path:** The final seed-419 cross-world construction-reuse rejection check in `PersistedCampRestart.ps1`.
- **Trigger:** The runner starts the server and sleeps for a fixed twelve seconds, then kills it if it is still running. It accepts the negative case whenever `cross-world.log` does not contain `Construction restored:`; it does not wait for listen readiness, inspect process/log failures, or require the persisted-camp verification fixture to report completion. If startup or fixture execution has not reached restoration by the fixed delay, the absence of the marker is treated as a successful rejection.
- **Impact:** The combined persistence runner can print PASS without proving the seed-419 server completed the mismatched-world restore check. A startup delay, missing player, or early fixture failure can therefore make the documented cross-world save rejection appear verified when it was never observed.
- **Evidence:** `RestorePersistedConstruction` returns without emitting restoration records when the saved world identity does not match. The restore fixture reports its server result only after a character is available and verification elapsed time exceeds three seconds, logging `Persisted camp restore server: Passed=... Constructions=... StorageWood=...`. The runner's cross-world branch checks only for the absence of construction-record log lines after an unconditional twelve-second sleep, so it cannot distinguish the intended completed zero-construction outcome from an incomplete fixture. The setup guide requires the combined scenario to reject cross-world reuse.
- **Proposed repair:** Wait for server readiness and a bounded explicit completed result from the seed-419 restore fixture; require the reported zero constructions and zero storage result and no restored-record lines. Fail on timeout, process exit before the result, fatal/assertion/error markers, or an unexpected restore.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** The runner reports PASS only after a seed-419 server has reached the fixture's completed restore result with zero constructions and no storage, and it fails when that result is absent, delayed past the deadline, or accompanied by process/log errors or any restored construction.
- **Focused validation:** After an Editor build, run the combined fixture and confirm that the same-identity restart still restores all ten expected construction IDs while the seed-419 phase emits its explicit completed zero-construction/zero-storage result. Exercise a delayed or absent completion marker and verify the runner fails instead of printing PASS. No runner or test was executed during this review.

## RF195 — Keep successful peer checks from timing out at the deadline (P3)

- **Source:** CR16-014, extended CR16-015, CR16-016, and CR16-017; `Scripts/Tests/Travel/OceanTravel.ps1:27-39`; `Swimming.ps1:26-39`; `Scripts/Tests/World/BiomeExpansion.ps1:89-95,263-267,351-359`; `MapAwareness.ps1:18-35`; `Minimap.ps1:26-45`; `WorldMap.ps1:56-62,64-95,125-145`; `WorldProfile.ps1:18-24,27-36`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1107-1110,1170-1181,1235-1241`; and `docs/07-development-setup.md:2177-2183`.
- **Affected path:** The final success/timeout gates for the host/client OceanTravel and Swimming peers, BiomeExpansion host-readiness, forest-delivery, and integrated-automation loops, plus MapAwareness, Minimap, WorldMap, and WorldProfile host-readiness and peer/profile-completion loops.
- **Trigger:** These scripts break from verification loops when host/client success markers or required presentation state are present, then unconditionally throw if the current clock is at or beyond the deadline. If processing crosses the deadline after a loop observes success but before its post-loop clock check, a completed operation is reported as timed out. WorldMap repeats this pattern during host restart and saved-coverage checks.
- **Impact:** A completed travel, swimming, map-awareness, minimap, world-map, world-profile, host-startup, forest-delivery, or integrated automation check can be reported as timed out at the deadline boundary, producing a false failure and unnecessary reruns.
- **Evidence:** OceanTravel and Swimming break after both peer predicates pass but then check only the clock. BiomeExpansion's host readiness, forest-delivery, and integrated automation loops do the same after their success markers. MapAwareness and Minimap have host-readiness and peer success loops with post-loop clock-only gates; WorldMap repeats the pattern for viewport host readiness, peer presentation, restarted-host readiness, and saved fog coverage. WorldProfile checks for listen readiness or both profile/identity markers inside its loops, then performs the same clock-only timeout checks after the loop. The relevant markers are in the reviewed runner checks; no runtime scenario was executed.
- **Proposed repair:** Track completion or test the final success predicates after each loop, and raise the timeout only when a required completion marker remains missing.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** Each loop passes when it observes all required completion markers before its deadline even if the post-loop clock reaches the deadline; it times out only when a required marker is missing at the end of the allowed window.
- **Focused validation:** Exercise successful completion just before the deadline while advancing the clock past the deadline before the post-loop gate for the Travel and BiomeExpansion loops; confirm success remains a pass. Confirm missing markers still fail at timeout. No runner or test was executed during this review.

## RF206 — Verify requested metadata in rendered biome baseline captures (P2)

- **Source:** CR16-015 and CR16-033; `Scripts/Tests/World/BiomeBaselineCapture.ps1:156-191`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10806-10811`; `Scripts/Tests/World/BiomeBaseline418419.csv`; `docs/biome-height-rework.md:424-452`; `docs/valheim-like-lighting-plan.md:704,766`; and the four assigned FireDusk/ForestShade raw GPU CSVs under `TestResults/L03c`.
- **Affected path:** Acceptance of fixed-site, rendered biome baseline capture images.
- **Trigger:** The runner checks PNG dimensions and pixel diversity, and checks only `Seed`, `Site`, and `Condition` in the runtime log (with separate quality and sky-fill checks). It does not compare the logged `LightingVariant`, requested `Xcm`, `Ycm`, and `ViewZcm`, temporal frame count/interval, GPU frame count, or comparison flag with the capture request. For GPU captures it accepts any positive `GPUTime` sample count, even if the CSV contains fewer samples than the requested `GpuCaptureFrames` value.
- **Impact:** A capture made with a different lighting variant, view pose, temporal/GPU sampling setup, or comparison mode can still be accepted as evidence for the requested baseline. A truncated GPU CSV can also produce a successful summary from fewer samples than the documented capture count.
- **Evidence:** The runner enumerates requested capture values and validates image appearance, but its metadata predicate covers only the three listed identity/condition fields. `TryCaptureBiomeBaselineView` logs the omitted request metadata, so it is available for explicit comparison. In the GPU path, the runner returns the count as `Frames` but rejects only when the number of positive `GPUTime` samples is zero; it does not compare that count with `GpuCaptureFrames`. The four CR16-033 raw captures each currently contain 120 positive samples and their means match the documented FireDusk/ForestShade values, so no incomplete artifact was observed. The older RF31 finding concerns malformed direct coordinate tokens and does not cover this runner's metadata/sample-count acceptance gap.
- **Proposed repair:** Compare each applicable logged request field against its fixed CSV/parameter values before accepting the image, including lighting variant, pose, frame counts/interval, and comparison flag. Require the count of positive GPU CSV `GPUTime` rows to equal the requested `GpuCaptureFrames` value.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** A capture with any requested lighting, pose, or frame-setting mismatch is rejected with the site and mismatched field identified; a GPU CSV with fewer or more positive `GPUTime` samples than requested is rejected; valid baseline requests with the exact expected sample count still pass.
- **Focused validation:** Alter each logged/requested metadata field independently and truncate or extend a copied GPU CSV's positive `GPUTime` rows; confirm the runner rejects each mismatch, then verify fixed-site captures with matching metadata and sample counts. No runner or test was executed during this review.

## RF210 — Match numeric biome profile outputs to their input identities (P2)

- **Source:** CR16-015, extended CR16-016; `Scripts/Tests/World/BiomeHillProfiles.ps1:22-27,81-103`; `BiomeFloorProfiles.ps1:218-231` and its output-profile grouping checks; `BiomeMireProfiles.ps1:31-67,160-171,305-314`; `BiomeRollingProfiles.ps1:38-59,186-245`; `BiomeWoodProfiles.ps1:33-77,165-200`; the Hill, Floor, Mire, Rolling, and Wood input fixtures; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:136-239`; and `docs/biome-height-rework.md:714-754,843-895`.
- **Affected path:** Numeric Hill, Floor, Mire, Rolling, and Wood biome profile sweeps and their acceptance summaries.
- **Trigger:** Hill and Floor do not match output identity fields row-by-row to their fixtures. Mire compares each trial's identities with the nominal output but does not compare nominal output identities with the input fixture. Rolling and Wood validate the fixed inputs, output row counts, numeric fields, and selected controls/grouping, but do not compare every output `Seed`, `Site`, `Xcm`, and `Ycm` tuple with its corresponding input row.
- **Impact:** Stale or incorrectly keyed commandlet output can retain the expected row count, finite measurements, and plausible grouping/control results while representing different seeds, sites, or coordinates; the runner can summarize it as the requested fixed profile sweep. Mire's cross-trial identity check does not catch a consistently mis-keyed set of outputs.
- **Evidence:** The five fixtures define fixed profile rows and coordinates: 88 Hill, 400 Floor, 252 Mire, 264 Rolling, and 66 Wood samples. Their runners contain the output checks cited above, but not a complete per-row input/output identity comparison. The commandlet currently copies identity fields from parsed input rows, but the runners do not enforce that output contract if the exporter regresses or emits stale data.
- **Proposed repair:** Compare every output `Seed`, `Site`, `Xcm`, and `Ycm` value to its corresponding input row before aggregating results, using exact seed/site checks and the established coordinate tolerance; validate each fixture's fixed groups, seeds, and coordinates.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** Any missing, reordered, duplicated, or mismatched output identity is rejected before a profile summary is printed; all five profile outputs match their corresponding fixed input rows.
- **Focused validation:** Exercise output with a changed seed, site, X, or Y and with reordered or duplicated rows for every fixture; confirm each mismatch fails, then check correctly keyed profile output. No runner or test was executed during this review.

## RF231 — Require all world profile metrics in the acceptance gate (P3)

- **Source:** CR16-017; `Scripts/Tests/World/WorldProfile.ps1:30-37`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8300-8305`; `docs/07-development-setup.md:2155-2159`; and `docs/02-technical-architecture.md:884`.
- **Affected path:** A developer runs the bounded generated-world profile and treats its PASS result as evidence that generation time, memory, actor, terrain/population, serialized-save, and late-join metrics were captured.
- **Trigger:** The runner's server-log regex requires `InitialGenerationMs=`, `SaveSerialized=1`, and `LateJoinPlayers=2`, but its `.*` wildcard does not require `UsedPhysicalMB`, `AvailablePhysicalMB`, `Actors`, `ReplicatedActors`, `TerrainPatches`, `PopulationKeys`, or `SaveBytes`. It also does not validate `InitialGenerationMs` or the other metrics as numeric values. The current GameMode log emits every field, but a partial or malformed report can still satisfy the runner's gate.
- **Impact:** A profile with omitted or invalid memory, actor-count, terrain/population, or save-size metrics can print PASS and appear complete, weakening the diagnostic evidence used to assess generated-world load and late-join cost.
- **Evidence:** The setup guide lists the server's time, physical-memory, actor, terrain-patch, population-key, and serialized-save-byte metrics. `KalmalaGameMode.cpp` emits those named values on one line. `WorldProfile.ps1` checks only the generation field name, save-serialization flag, and player count; its wildcard permits the other fields to be absent or malformed. No log fixture or runtime scenario was executed during this review.
- **Proposed repair:** Require every documented metric field in the server report and validate finite non-negative decimal values for time/memory and non-negative integer values for counts/bytes. Retain the existing exact save-serialization and two-player gates; do not add performance thresholds absent from the diagnostic contract.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A missing or malformed metric field prevents PASS with the field identified; a valid complete report with successful serialization and two players still passes. No new performance threshold is introduced.
- **Focused validation:** Check a complete valid report, then remove or corrupt each metric field independently and confirm the runner rejects it; parser-check the runner after repair. No runner or test was executed during this review.

## RF232 — Make the Iron material icon read as a forged bar (P3; CR13-036)

- **Source:** CR13-036.
- **Affected files:** `Content/Data/GameCatalogues/items/materials/metal.json`; `Content/Kalmala/UI/Source/IconOriginals/Iron.png`; `Content/Kalmala/UI/Source/Icons/Iron.png`; `Content/Kalmala/UI/Icons/Items/Iron.uasset`; `docs/icon-provenance.md`.
- **Affected path:** A player identifies refined Iron in the inventory and crafting interfaces.
- **Trigger:** The item is named `Iron bar` and described as a forged bar, but its canonical icon depicts a rough, irregular rock-like mass similar to the separate Iron Ore icon.
- **Impact:** The icon weakens the distinction between the refined bar and ore, making a core crafting material harder to recognize at the inventory's 64×64 display size. The current provenance guide also lacks an Iron-bar prompt record, so the intended appearance cannot be reproduced reliably.
- **Evidence:** `metal.json` defines the `Iron` display name as `Iron bar` and describes it as “A forged bar of pure iron”; `ore.json` describes `IronOre` as a chunk of iron-rich rock. The 1254×1254 original and prepared 64×64 icon were visually inspected; the art reads as an irregular rock rather than a forged bar. The package hash, canonical object path, `Texture2D` marker, embedded source MD5 and runtime icon mapping are valid, so this is a semantic art/provenance issue, not a missing asset or mapping. `docs/icon-provenance.md` contains the IronOre prompt but no dedicated Iron-bar prompt record.
- **Proposed repair:** After CR19 review acceptance, revise the original Iron artwork to read clearly as a forged bar at 64×64, retain a recoverable generation prompt or explicitly record unavailable provenance, regenerate the prepared PNG, and reimport the canonical texture through the existing icon pipeline. Preserve item IDs, recipes and runtime behavior; do not edit assets during CR13.
- **Dependencies:** After CR19 review acceptance; no icon or documentation edits during CR13.
- **Acceptance:** The Iron texture reads as a forged bar and is readily distinguishable from IronOre at 64×64; its original prompt/source is traceable or explicitly recorded as unavailable; catalogue IDs and runtime mapping remain unchanged.
- **Focused validation:** Run `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id Iron` and `python Scripts/Validate-CatalogueReferences.py`; visually compare the prepared Iron and IronOre icons at 64×64. No Unreal build is required for the metadata/art repair.

## RF184 extension — CR13-037 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `Content/Kalmala/UI/Icons/Items/JadeRing.uasset`, `Lightwood.uasset`, `LynxMantle.uasset` and `LynxPelt.uasset`, with matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting JadeRing uses a recorded source path under the removed `.wt/a9` checkout; Lightwood points into `wt/m12-hud-feedback-rebuild`; LynxMantle and LynxPelt point into the removed `.wt/a8b` checkout.
- **Impact:** These imported textures remain usable, but source reimport depends on paths missing from the current checkout and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package SHA-256 values match their CR01 coverage fingerprints. Each package contains the expected object path formed from its asset ID and has a `Texture2D` class marker. Every embedded source `FileMD5` matches its checked-in prepared PNG, and all eight original and prepared PNGs exist and match their CR01 fingerprints. The four paths listed above fail the current-checkout existence check; IronSpear, IronSword, LightBow and Lingonberry resolve to project-local prepared PNGs. All eight focused `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` checks passed, as did `python -B Scripts/Validate-CatalogueReferences.py` for 152 registered definitions and canonical runtime icon mappings. The item-specific prompt gaps for IronSpear and Lightwood remain covered by RF123; the other six prompts are recorded in `docs/icon-provenance.md`.
- **Disposition:** Extend RF184 from 77 to 81 active affected textures. Reimport these four packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

## RF184 extension — CR13-038 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `MeatStew.uasset`, `MendingScroll.uasset`, `Moonstone.uasset`, `MoonstoneRing.uasset`, `Onion.uasset`, `OnionSeed.uasset`, `OpalHelm.uasset` and `PeatHeart.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting MeatStew, Onion and OnionSeed uses paths under the removed `wt/m12-hud-feedback-rebuild` worktree; MendingScroll references the removed A10 acceptance worktree; Moonstone, MoonstoneRing and OpalHelm point into `.wt/a9`; PeatHeart points into `.wt/a8b`. All eight recorded source paths fail the current-checkout existence check.
- **Impact:** The imported textures remain usable, but reimport depends on missing source paths and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Every package contains its expected `/Game/Kalmala/UI/Icons/Items/<asset>` object path and `Texture2D` marker. Each embedded source `FileMD5` matches its checked-in prepared PNG, and all eight original and prepared PNGs exist. All eight focused `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` checks passed; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 definitions, 776 resolved references and 152 canonical runtime icon mappings. Visual review of the original and prepared icons found each image readable and consistent with its catalogue identity. MeatStew, Onion and OnionSeed prompt gaps remain covered by RF103; item prompts are retained for Moonstone, MoonstoneRing, OpalHelm and PeatHeart. MendingScroll's A10/AP batch and source are documented, but its exact generation prompt is not retained in the current guide; that remains an open provenance question recorded below.
- **Disposition:** Extend RF184 from 81 to 89 active affected textures. Reimport these eight packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

### CR13-038 open question — MendingScroll prompt retention

The A10 reusable-scroll section documents that MendingScroll reuses an earlier transparent source and was imported in isolated AP batch 84, but does not retain its exact generation prompt. Repository history contains no earlier prompt record.

- **Question:** Was the original MendingScroll prompt preserved outside the repository? If it is unavailable, later provenance work should state that explicitly instead of reconstructing it from the image.

## RF184 extension — CR13-039 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `PeatHeartStew.uasset`, `PlantingTrowel.uasset`, `PolarBearCoat.uasset`, `PolarBearPelt.uasset`, `Potato.uasset`, `PotatoSeed.uasset`, `QuartzNecklace.uasset` and `RatTail.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting PeatHeartStew, PolarBearCoat, PolarBearPelt and RatTail uses paths under the removed `.wt/a8b` checkout; Potato and PotatoSeed point into the removed `wt/m12-hud-feedback-rebuild` checkout; QuartzNecklace points into `.wt/a9`. All seven paths fail the current-checkout existence check. PlantingTrowel resolves to the prepared PNG in this checkout.
- **Impact:** The seven imported textures remain usable, but reimport depends on missing source paths and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Every package contains its expected `/Game/Kalmala/UI/Icons/Items/<asset>` object path and `Texture2D` marker. Each embedded source `FileMD5` matches its checked-in prepared PNG, and all eight original and prepared PNGs exist. Seven recorded source paths fail the current-checkout existence check; PlantingTrowel resolves locally. All eight focused `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` checks passed; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 definitions, 776 resolved references and 152 canonical runtime icon mappings. Visual review found each original and prepared icon readable and consistent with its catalogue identity. Potato and PotatoSeed prompt gaps remain covered by RF103; item-specific prompts for PeatHeartStew, PlantingTrowel, PolarBearCoat, PolarBearPelt, QuartzNecklace and RatTail are retained in `docs/icon-provenance.md`.
- **Disposition:** Extend RF184 from 89 to 96 active affected textures. Reimport these seven packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

## RF184 extension — CR13-040 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `RatTailTonic.uasset`, `ReedKnife.uasset`, `ReefHarpoon.uasset`, `ReefTooth.uasset`, `ReinforcedChestKit.uasset`, `RoastedBadger.uasset`, `RoastedRootVegetables.uasset` and `RoofKit.uasset` under `Content/Kalmala/UI/Icons/Items/`, with matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting RatTailTonic, ReefHarpoon, ReefTooth and RoastedBadger uses paths under the removed `.wt/a8b` checkout. ReedKnife, RoastedRootVegetables and RoofKit use paths under the removed `wt/m12-hud-feedback-rebuild` checkout. ReinforcedChestKit resolves to its project-local prepared PNG. Seven recorded source paths fail the current-checkout existence check.
- **Impact:** The seven imported textures remain usable, but source reimport depends on missing checkout paths and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Every package contains the expected `/Game/Kalmala/UI/Icons/Items/<asset>` package path and `Texture2D` class marker. Each embedded source `FileMD5` matches its checked-in prepared PNG, and all eight original and prepared PNGs exist. Seven recorded source paths are missing; ReinforcedChestKit resolves locally. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for the eight registered identities, using canonical `StoneKnife` for the legacy `ReedKnife` texture mapping. `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique definitions, 776 resolved references and 152 canonical runtime icon mappings. All eight originals and prepared PNGs were visually inspected and match their catalogue identities. Exact prompt records are present for RatTailTonic, ReefHarpoon, ReefTooth, ReinforcedChestKit and RoastedBadger; the StoneKnife/ReedKnife prompt gap remains under RF123, and RoastedRootVegetables remains under RF103. RoofKit's buildable manifest misclassification remains under RF216.
- **Disposition:** Extend RF184 from 96 to 103 active affected textures. Reimport these seven packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

### CR13-040 open question — RoofKit prompt retention

The current icon provenance guide and its Git history contain no RoofKit-specific generation prompt. Its original PNG carries a C2PA claim identifying ChatGPT/gpt-image trained algorithmic media; a readable scan did not surface prompt text.

- **Question:** Was the original RoofKit prompt retained outside the repository? If it is unavailable, later provenance work should say so rather than reconstructing it from the image.

## RF184 extension — CR13-041 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `RootStaminaDraught.uasset`, `RootVegetableSoup.uasset`, `Rutabaga.uasset`, `RutabagaSeed.uasset`, `ScaleCuirass.uasset`, `Sinew.uasset`, `SmelterKit.uasset` and `SmokyQuartz.uasset` under `Content/Kalmala/UI/Icons/Items/`, with matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Reimporting RootVegetableSoup, Rutabaga and RutabagaSeed uses paths under the removed `wt/m12-hud-feedback-rebuild` checkout; ScaleCuirass points into the removed `.wt/a8b` checkout, and SmokyQuartz into the removed `.wt/a9` checkout. RootStaminaDraught, Sinew and SmelterKit resolve to project-local prepared PNGs. Five recorded source paths fail the current-checkout existence check.
- **Impact:** The five imported textures remain usable, but source reimport depends on missing checkout paths and cannot reliably use the checked-in project-local prepared PNGs from another checkout or clean mirror.
- **Evidence:** All eight package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Every package contains the expected `/Game/Kalmala/UI/Icons/Items/<asset>` package path and `Texture2D` class marker. Each embedded source `FileMD5` matches its checked-in prepared PNG, and all eight original and prepared PNGs exist. Five recorded source paths are missing; the other three resolve locally. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs, and `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique definitions, 776 resolved references and 152 canonical runtime icon mappings. Visual inspection of the eight original and prepared icons found them readable and consistent with their catalogue identities. Prompt records are present for RootStaminaDraught, ScaleCuirass, Sinew, SmelterKit and SmokyQuartz; RootVegetableSoup, Rutabaga and RutabagaSeed prompt gaps remain under RF103. The SmelterKit manifest row is construction-only and the icon depicts a placed smelter; no portable construction item was identified.
- **Disposition:** Extend RF184 from 103 to 108 active affected textures. Reimport these five packages only from the checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

## RF184 extension — CR13-042 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `StandingLightKit.uasset`, `StarOpal.uasset`, `Stone.uasset`, `StoneMace.uasset`, `StonePick.uasset`, `StoneSpear.uasset` and `StorageKit.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** StandingLightKit points into the absent `.wt/standing-light` worktree; StarOpal points into absent `.wt/a9`; Stone, StonePick and StorageKit point into the removed `wt/m12-hud-feedback-rebuild` worktree. StoneMace and StoneSpear use absolute paths under the main checkout, so their import records are tied to this checkout. StoneArrow's `../../Source/Icons/StoneArrow.png` path resolves to its project-local prepared PNG and is not affected.
- **Impact:** These seven imported textures remain usable, but their recorded source paths are missing or checkout-specific and cannot reliably support reimport in another checkout or clean mirror.
- **Evidence:** All eight package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<asset>` identity and `Texture2D` marker; each embedded `FileMD5` matches its checked-in prepared PNG. Five source paths fail the current-checkout existence check, two resolve only through absolute main-checkout paths, and StoneArrow resolves through a project-relative path. The eight focused `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` checks passed; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 definitions, 776 resolved references and 152 canonical runtime icon mappings. All eight original and prepared icons were visually inspected and match their catalogue identities. Prompt records are present for StandingLightKit, StarOpal, StoneArrow and StoneMace; the Stone, StonePick and StoneSpear prompt gaps remain under RF123. StorageKit's manifest role/view mismatch remains under RF216; its registered data is the direct-material Chest buildable, with no portable kit identified.
- **Disposition:** Extend RF184 from 108 to 115 active affected textures. Reimport these seven packages only from checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

### CR13-042 open question — StorageKit prompt retention

The current `docs/icon-provenance.md` has no StorageKit-specific entry. The pre-consolidation `docs/43-catalogue-icon-manifest.md` Batch 05 record describes the art subject and shared style constraints but does not retain its exact image prompt. The original PNG contains C2PA metadata, but no exact prompt is recorded in the checked-in documentation.

- **Question:** Was the exact StorageKit generation prompt preserved outside the repository? If it is unavailable, later provenance work should record that rather than reconstructing it from the image.

## RF184 extension — CR13-043 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `StormcutSword.uasset`, `StormSapphire.uasset`, `ThunderCore.uasset` and `ThunderMaul.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** StormcutSword and StormSapphire record source PNGs under the missing `.wt/a9` checkout; ThunderCore and ThunderMaul point into the missing `.wt/a8b` checkout. All four source paths fail the current-checkout existence check.
- **Impact:** The imported textures remain usable, but source reimport depends on missing worktree files and cannot reliably use the checked-in project-local prepared PNGs in another checkout or a clean mirror.
- **Evidence:** All 24 SHA-256 values for this batch’s eight packages, eight originals and eight prepared PNGs match the CR01 coverage fingerprints, including the preserved retired `String` package. Every package contains its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` marker, and each embedded `FileMD5` matches its prepared PNG. Seven active manifest identities passed `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>`; `String` is retired, has no current catalogue/recipe definition or runtime mapping, and is excluded from the manifest. Among the seven active identities, the three Timber package source paths resolve project-locally; the four affected paths above are missing. `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. All eight original and prepared icons were visually inspected and match their documented subjects. Storm Sapphire’s 2% Thunder Mountains treasure source, Thunder Core’s 25% Thunder Sentinel reward, the Stormcut Sword and Thunder Maul recipe/progression data, and the three Timber buildable-only direct-material recipes match the current design and catalogue contracts. Timber source paths are project-relative and resolve locally.
- **Disposition:** Extend RF184 from 115 to 119 active affected textures. Reimport only the four listed packages from their checked-in project-local prepared PNGs after CR19 acceptance; do not change packages during CR13.

## RF184 extension — CR13-044 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `Tin.uasset`, `TinOre.uasset`, `Tourmaline.uasset`, `TourmalineNecklace.uasset`, `VenomSac.uasset`, `ViperFang.uasset`, `WallKit.uasset` and `WolfFang.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** `Tin` and `TinOre` record absolute source paths under the main checkout. `Tourmaline` and `TourmalineNecklace` point into the removed `.wt/a9` worktree; `VenomSac`, `ViperFang` and `WolfFang` point into the removed `.wt/a8b` worktree; and `WallKit` points into the removed `wt/m12-hud-feedback-rebuild` worktree. The six latter paths do not resolve in this checkout.
- **Impact:** All eight imported textures remain usable, but reimport depends on paths tied to this checkout or missing worktrees rather than stable project-local prepared sources.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` class, and its embedded `FileMD5` matches the corresponding checked-in prepared PNG. The six listed missing paths fail the current-checkout existence check; the two Tin paths resolve only through absolute main-checkout paths. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. All eight original and prepared icons were visually inspected and match their subjects. Catalogue cross-checks confirmed Tin/Tin Ore and smelting/forge use, Tourmaline and its necklace's authored healing-received bonus, the Viper/Death Adder/Wolf drop entries and crafting references, and Wall's direct-material buildable output. `WallKit` remains a placed-object icon; its stale pack/item/storage manifest scope is already covered by RF216 and was not duplicated. Exact prompt records are present for the other seven identities; the WallKit-specific prompt is not retained in the current guide or the pre-consolidation batch record.
- **Disposition:** Extend RF184 from 119 to 127 active affected textures. After CR19 acceptance, reimport these eight packages from their checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

### CR13-044 open question — WallKit prompt retention

The current `docs/icon-provenance.md` has no WallKit-specific prompt. The pre-consolidation `docs/43-catalogue-icon-manifest.md` Batch 06 record describes the wall subject and shared style constraints, but not the exact prompt. `WallKit.png` carries C2PA metadata identifying ChatGPT/gpt-image trained algorithmic media, but the metadata contains no prompt text.

- **Question:** Was the exact WallKit generation prompt preserved outside the repository? If it is unavailable, later provenance work should record that rather than reconstructing it from the image.


## RF184 extension — CR13-045 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `WolfMeat.uasset`, `WolfPelt.uasset`, `WolfPeltBoots.uasset`, `WolfStew.uasset`, `Wood.uasset`, `WoodenArrow.uasset` and `WoodShield.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** WolfMeat, WolfPelt, WolfPeltBoots and WolfStew point into the removed `.wt/a8b` worktree. Wood points into the removed `wt/m12-hud-feedback-rebuild` worktree. WoodenArrow and WoodShield record absolute source paths under the main checkout. The seven affected paths are missing or tied to this checkout. WoodenClub uses `../../Source/Icons/WoodenClub.png`, which resolves to its project-local prepared PNG and is not affected.
- **Impact:** The seven affected imported textures remain usable, but reimport depends on missing worktree files or a path tied to the main checkout rather than stable project-local sources.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints, including the current WoodenClub package bytes, whose pre-existing worktree change is also present at CR01. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` class; every embedded `FileMD5` matches its checked-in prepared PNG. Five paths fail the current-checkout existence check and two resolve only through absolute main-checkout paths; WoodenClub's relative path resolves locally. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for all eight IDs; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. All eight original and prepared icons were visually inspected and match their identities. Cross-checks confirmed the Wolf's guaranteed one-each Wolf Pelt and Wolf Meat drops, Wolf Pelt Boots' authored equipment/recipe data, Wolf Stew's Cauldron recipe, Wooden Arrows' Workbench output, WoodenClub's Workbench profile/recipe, and Wood Shield's current Workbench acquisition and upgrade data. RF92 already tracks the stale Wood Shield architecture text; RF123 already tracks the missing Wood icon prompt, and neither was duplicated. Exact prompt records are present for the four Wolf identities, WoodenArrow and WoodenClub. The WoodShield guide retains a shared style description and subject row but no exact prompt; its C2PA metadata identifies ChatGPT/gpt-image trained algorithmic media without prompt text.
- **Disposition:** Extend RF184 from 127 to 134 active affected textures. After CR19 acceptance, reimport these seven packages from their checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

### CR13-045 open question — WoodShield prompt retention

The current `docs/icon-provenance.md` and pre-consolidation `docs/armor-icons.md` retain shared style constraints and a WoodShield subject description, but not the exact generation prompt. The original PNG's C2PA metadata identifies its generator and media type but contains no prompt text.

- **Question:** Was the exact WoodShield generation prompt preserved outside the repository? If it is unavailable, later provenance work should record that rather than reconstructing it from the image.


## RF184 extension — CR13-046 catalogue icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `WorkbenchKit.uasset` and `WorkbenchToolRackKit.uasset` under `Content/Kalmala/UI/Icons/Items/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/`.
- **Trigger:** Both packages record absolute source PNG paths under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild` worktree. Neither source path resolves in the current checkout.
- **Impact:** The imported textures remain usable, but reimport is tied to a missing worktree path instead of the checked-in project-local prepared PNGs.
- **Evidence:** All six package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Each package contains its expected `/Game/Kalmala/UI/Icons/Items/<ID>` identity and `Texture2D` class; its embedded source `FileMD5` matches the checked-in prepared PNG. Both recorded paths are missing. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` passed for both IDs; `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references and 152 canonical runtime icon mappings. The originals and prepared icons were visually inspected and match their placed-object subjects. The registered `items/buildItems/workbench.json` contains only the `Workbench` and `WorkbenchToolRack` buildable definitions; `recipes/building.json` defines direct-material Buildable outputs (9 Wood + 2 Stone for Workbench; 6 Wood for the Tool Rack), with no portable kit output. The 2026-10-14 decision and current architecture specify direct Building Hammer placement. The stale icon-manifest `item`/pack/storage views for these identities are already covered by RF216 and are not duplicated.
- **Disposition:** Extend RF184 from 134 to 136 active affected textures. After CR19 acceptance, reimport these packages from their checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change packages during CR13.

### CR13-046 open question — Workbench icon prompt retention

The current `docs/icon-provenance.md` has no item-specific WorkbenchKit or WorkbenchToolRackKit prompt. The pre-consolidation `docs/43-catalogue-icon-manifest.md` Batch 03/04 records their subjects and shared style constraints but not the exact prompts. Their original PNG C2PA metadata identifies ChatGPT/gpt-image trained algorithmic media and contains no readable prompt text.

- **Question:** Were the exact WorkbenchKit and WorkbenchToolRackKit generation prompts preserved outside the repository? If unavailable, later provenance work should record that rather than reconstructing either prompt from the image.


## RF184 extension — CR13-047 status icon source records

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected packages:** `BearsVigor.uasset`, `Cold.uasset`, `DeerCall.uasset`, `HearthShield.uasset`, `Heat.uasset`, `Mending.uasset`, `SteadyMeal.uasset` and `Storm.uasset` under `Content/Kalmala/UI/Icons/Status/`, with their matching prepared PNGs under `Content/Kalmala/UI/Source/Icons/Status/`.
- **Trigger:** All eight packages record absolute source PNG paths under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild` worktree. None resolves in the current checkout.
- **Impact:** The imported status textures remain usable, but source reimport is tied to missing worktree files instead of the checked-in project-local prepared PNGs.
- **Evidence:** All 24 package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. Each package contains its expected `/Game/Kalmala/UI/Icons/Status/<ID>` identity and `Texture2D` class; its embedded source `FileMD5` matches the prepared PNG. All eight recorded source paths are absent. The eight focused `Scripts/Validate-UIIcons.ps1 -Category Status -Id <ID>` checks passed; the original and prepared PNGs were visually inspected and match their status-manifest subjects. `Scripts/Data/status-icon-manifest.csv`, `Source/KalmalaUI/Private/KalmalaStatusIconLibrary.cpp` and the static `Kalmala.UI.StatusHotbar.StatusIconCoverage` assertions in `Source/KalmalaUI/Private/Tests/KalmalaStatusHotbarTest.cpp` agree on the eight reviewed entry-to-icon mappings and target package paths; the reviewed identities remain unique within the nine-icon set. `docs/07-development-setup.md` describes these textures as client-side presentation of owner status snapshots; no status ownership or gameplay behavior changed.
- **Disposition:** Extend RF184 from 136 to 144 active affected textures. After CR19 acceptance, reimport these packages from their checked-in project-local prepared PNGs while preserving package identities, source pixels and texture settings; do not change assets during CR13.

### CR13-047 open question — status icon prompt retention

The current `docs/icon-provenance.md` has no per-icon generation record for these status icons, and `Scripts/Data/status-icon-manifest.csv` records subjects and batches but no exact prompt text. The original PNG C2PA metadata identifies ChatGPT/gpt-image trained algorithmic media; a readable scan found no prompt text.

- **Question:** Were the exact generation prompts for these eight status icons preserved outside the repository? If unavailable, later provenance work should record that rather than reconstructing prompts from the artwork.


## RF184 extension — CR13-048 Wet status icon source record

- **Related finding:** `RF184` in `BACKLOG.md`.
- **Affected package:** `Wet.uasset` under `Content/Kalmala/UI/Icons/Status/`, with its matching prepared PNG under `Content/Kalmala/UI/Source/Icons/Status/`.
- **Trigger:** The package records an absolute source PNG path under the removed `E:/dev/Kalmala/wt/m12-hud-feedback-rebuild` worktree; that path does not resolve in the current checkout.
- **Impact:** The imported Wet texture remains usable, but source reimport is tied to a missing worktree file instead of the checked-in project-local prepared PNG.
- **Evidence:** The package, original-PNG and prepared-PNG SHA-256 values match their CR01 coverage fingerprints. The package contains its expected `/Game/Kalmala/UI/Icons/Status/Wet` identity and `Texture2D` class; its embedded source `FileMD5` matches the prepared PNG. The recorded source path is absent. `Scripts/Validate-UIIcons.ps1 -Category Status -Id Wet` passed; the original and prepared PNGs were visually inspected and match the `Wet` manifest subject. `Scripts/Data/status-icon-manifest.csv`, `Source/KalmalaUI/Private/KalmalaStatusIconLibrary.cpp` and the static `Kalmala.UI.StatusHotbar.StatusIconCoverage` assertions in `Source/KalmalaUI/Private/Tests/KalmalaStatusHotbarTest.cpp` agree on the Wet-to-Wet mapping and `/Game/Kalmala/UI/Icons/Status/Wet.Wet` package path. `docs/07-development-setup.md` describes these textures as client-side presentation of owner status snapshots; no status ownership or gameplay behavior changed.
- **Disposition:** Extend RF184 from 144 to 145 active affected textures. After CR19 acceptance, reimport this package from its checked-in project-local prepared PNG while preserving package identity, source pixels and texture settings; do not change assets during CR13.

### CR13-048 open question — Wet status icon prompt retention

The current `docs/icon-provenance.md` has no per-icon generation record for Wet, and `Scripts/Data/status-icon-manifest.csv` records its subject and batch but no exact prompt text. The original PNG C2PA metadata identifies ChatGPT/gpt-image trained algorithmic media; a readable scan found no prompt text.

- **Question:** Was the exact Wet status icon generation prompt preserved outside the repository? If unavailable, later provenance work should record that rather than reconstructing the prompt from the artwork.


### CR13-049 open question — UI source artwork prompt retention

The M11 setup guidance records the intended roles and dimensions for the BuildPanel, InventoryPanel, OptionsDivider, OptionsPanel, StoneButtonMaster, StoneSliderThumb and WorldMapPanel artwork, but neither it nor the current `docs/icon-provenance.md` retains their exact generation prompts. The seven original artworks carry C2PA metadata identifying ChatGPT/gpt-image trained algorithmic media; a readable scan found no prompt text. `StoneButton.png` is the documented 160x53 derivative of StoneButtonMaster, not a separate generated original.

- **Question:** Were the exact generation prompts for these seven original UI images preserved outside the repository? If unavailable, later provenance work should record that rather than reconstructing prompts from the images.

### CR13-054 open question — Campfire and Cauldron icon prompt retention

The current `docs/icon-provenance.md` retains no exact generation prompt for `CampfireKit` or `CauldronKit`, and the current catalogue icon manifest records only their subjects and batches. Each original has a `caBX` C2PA PNG chunk with readable ChatGPT, gpt-image and OpenAI markers; a readable-byte scan found no prompt text. Visual inspection confirms the originals and prepared 64×64 icons match the campfire and hearth-cauldron identities. The crop/food prompt gaps for `Carrot` and `CarrotSeed` remain covered by RF103. The `CauldronKit` item/pack and inventory-view manifest mismatch is already tracked by RF216; its authored buildable definition and direct-material building recipe remain intact.

- **Question:** Were the exact Campfire and Cauldron icon generation prompts preserved outside the repository? If unavailable, later provenance work should record that rather than reconstructing prompts from the images.

### CR13-055 open question — Cooking Rack icon prompt retention

The current `docs/icon-provenance.md` contains no exact generation record for `CookingRackKit`; its manifest row retains only a subject and batch. The original has a `caBX` C2PA PNG chunk with readable ChatGPT, gpt-image and OpenAI markers, while a readable-byte scan found no prompt text. Visual inspection confirms that the original and prepared 64×64 icon depict the placed timber Cooking Rack. The stale item/pack and inventory-view manifest classification is already tracked by RF216; the registered buildable definition and direct-material recipe remain intact.

- **Question:** Was the exact Cooking Rack icon generation prompt preserved outside the repository? If unavailable, later provenance work should record that rather than reconstructing the prompt from the image.

## RF233 finding: missing prompt provenance for five placed-object icon textures

The active buildable definitions Floor, ForgeAnvil, Forge, and FryingPan resolve to the existing FloorKit, ForgeAnvilKit, ForgeKit, and FryingPanKit UI textures. Their current catalogue icon manifest rows are stale about inventory/pack views; RF216 covers that separate issue. The six active CR13-057 packages with obsolete import paths are already covered by RF184. CR13-035 adds the GrindingStone buildable and its GrindingStoneKit UI texture to this provenance scope; RF184 covers its stale source path, and RF216 covers its manifest-role/view mismatch.

- **Trigger:** A maintainer needs to audit or reproduce the source artwork for one of these five build-menu icons.
- **Impact:** The textures and buildable mappings work, but the original generation prompts and visual constraints cannot be verified from the repository.
- **Evidence:** CR13-057 source and prepared PNG hashes match their CR01 coverage fingerprints. Each original has a C2PA caBX chunk with readable ChatGPT/gpt-image/OpenAI generator markers, but no PNG text chunk or readable prompt. The current icon-provenance guide and repository documentation search contain no item-specific record; history searches across the guide and development-setup record also found none. Focused UI icon validation passes for all four IDs. The images visually match their mapped placed-object subjects.
- **CR13-035 extension:** The GrindingStone buildable uses the `GrindingStoneKit` texture mapping. Its original PNG matches its CR01 hash, contains a C2PA `caBX` generator record with ChatGPT/gpt-image markers, and has no text metadata or readable prompt. `docs/icon-provenance.md` has no item-specific GrindingStoneKit record. Focused UI icon validation passes, the 64×64 icon visibly matches the Grinding Stone, and the registered definition remains buildable-only with a direct-material recipe of 6 Wood and 4 Stone. No portable kit output or inventory definition was found.
- **Disposition:** Queue RF233 for post-CR19 provenance recovery or an explicit unavailable-provenance record for all five identities. Do not invent prompts or change any art, catalogue item, recipe, or placement behavior during CR13. These suffix-Kit textures represent placed objects; no portable construction items are introduced.

## RF102 — Reconcile architecture meal and cooking catalogue summaries (P3)

- **Source:** CR13-008/015; `docs/02-technical-architecture.md:751,757,1062-1077`; the registered food and Cooking Rack, Cauldron, and Frying Pan catalogues; `KalmalaPlayerStatusComponent.cpp`; focused status and catalogue tests.
- **Affected path:** A developer uses the architecture meal-use list to determine which prepared foods can invoke the server-owned Steady Meal transaction.
- **Trigger:** The architecture lists seven cooking outputs plus Lingonberry and ForestChanterelle for meal use, and describes seven rows as the active cooking catalogue. It omits RoastedBadger, PeatHeartStew, and WolfStew, which the server accepts, and omits current Cauldron recipes from its summary.
- **Impact:** Architecture guidance understates accepted meal-use behavior and the active station recipe catalogue, which can misdirect implementation or review.
- **Evidence:** The manifest registers the food catalogue and all three cooking-station files. CR13-015 confirmed all 14 assigned cooking rows are enabled and cross-checked the row set: ten meal outputs include the three omitted meals, alongside Berry Recovery Draught, Root Stamina Draught, Rat-tail Tonic, and Antidote. `IsKnownFoodItem` includes the three meals; focused status/catalogue tests cover their recipes and use path. The non-meal transactions have separate effects and remain distinct from meal effects.
- **Proposed repair:** After CR19 review acceptance, reconcile the architecture meal allowlist and current cooking summaries with registered recipes and server item-use paths. Keep meal-effect foods distinct from draught, tonic, and Antidote transactions; do not change runtime behavior or item data.
- **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
- **Acceptance:** The architecture meal allowlist and current cooking summaries match registered meal recipes and the server allowlist; the recipe overview accounts for current station outputs and keeps draught/tonic/Antidote effects separate. No runtime or item data changes.
- **Focused validation:** Compare the revised sections with `IsKnownFoodItem`, all manifest-registered cooking recipes, and focused status/catalogue tests; run `git diff --check`. No Unreal build is required.

## CR13-009 — build-item catalogue review

- **Reviewed inputs:** The seven assigned `items/buildItems/*.json` files. Their current SHA-256 fingerprints match the CR01 ledger, and all seven remain registered in `itemFiles`.
- **Evidence:** Parsed 22 buildable definitions with unique `ItemId`s. The 22 unique outputs in `recipes/building.json` match those identities one-to-one, and each recipe has positive direct-material inputs. The prior review also checked names, descriptions, material references, attachment stations, Smelter processing, Garden Plot cultivation, Grinding Stone repair scope, icon files/mappings and the catalogue reference validator.
- **Outcome:** No structural, reference or portable-construction-item defect was confirmed in the assigned data. Existing actionable documentation findings RF106–RF109 remain open in `BACKLOG.md`; no duplicate finding was added.
- **Additional RF106 evidence:** The current architecture material summary at `docs/02-technical-architecture.md:3276` also says Ore contains only Stone and Metal only Iron, despite registered Copper, Tin and Iron ores/bars. The current setup summary is at line 5215. Include this architecture passage when RF106 is repaired after CR19 acceptance.
- **Limits:** This reconciliation relies on the exact matching assigned-file fingerprints and the prior source/data review; the full catalogue validator was not rerun. No Unreal build, tests, live interaction, Editor preview, icon render or reimport ran.

### RF106 — Smelter and material-category guidance (P3; CR13-009)

- **Affected paths:** `docs/01-game-design.md:172`, `docs/02-technical-architecture.md:3276`, `docs/07-development-setup.md:5215`, the Smelter buildable and ore/metal catalogues, plus the server Smelter handler.
- **Trigger and impact:** Current summaries say only Iron Ore is smelted and list only Stone in Ore and Iron in Metal, understating the registered Copper, Tin and Iron routes and giving maintainers conflicting category guidance.
- **Evidence:** The manifest registers all three ores and bars; the server maps CopperOre, TinOre and IronOre to distinct Smelter modes. The focused A10 source covers Copper, Tin and Iron bars. The architecture construction contract describes all three processing routes elsewhere.
- **Proposed repair and dependency:** Reconcile the stale design, architecture-material and setup summaries after CR19 review acceptance. Do not change recipes, catalogue data or runtime behavior during CR13.
- **Acceptance and validation:** Current guidance lists the registered ore/bar categories and accepted Smelter outputs. Compare the repaired text with the manifest, ore/metal catalogues, server handler and A10 test; run `Scripts/Validate-CatalogueReferences.py` and `git diff --check`. No Unreal build is required.

### RF107 — Grinding Stone repair scope (P3; CR13-009)

- **Affected paths:** `items/buildItems/grinding-stone.json`, `docs/04-roadmap.md:364`, `docs/07-development-setup.md`, and the server repair transaction.
- **Trigger and impact:** The item description promises repair of all worn items, while the server action repairs only eligible entries in the owner's carried-tool list; players may infer that wearable or other condition-bearing inventory items are repaired.
- **Evidence:** The server repair transaction passes the character's `CarriedTools` into the repair contract and persists that carried-tool array. The roadmap describes the action as repairing the server-owned carried-tool list.
- **Proposed repair and dependency:** Narrow the description to all carried tools at once after CR19 review acceptance; do not add gear repair behavior.
- **Acceptance and validation:** The description matches the carried-tool-only server behavior; compare the text with the server transaction and roadmap/setup contract, then run `git diff --check`. No Unreal build is required.

### RF108 — Garden Plot farming status (P3; CR13-009)

- **Affected paths:** `docs/01-game-design.md:172`, `docs/game-features.md:145`, `items/buildItems/other.json`, and the server cultivation/harvest path.
- **Trigger and impact:** The design and feature summaries say farming is unimplemented even though Garden Plot planting, active-time growth and mature-crop harvest are supported, making current scope appear deferred.
- **Evidence:** The registered GardenPlot definition and direct-material recipe exist; the architecture documents the Planting Trowel flow, and current server cultivation and harvest handlers accept GardenPlot records. PotatoSeed remains intentionally unused under RF101.
- **Proposed repair and dependency:** After CR19 acceptance, distinguish implemented Garden Plot cultivation from broader deferred farming scope and keep the PotatoSeed limitation explicit. Do not add gameplay or crop behavior during CR13.
- **Acceptance and validation:** Both summaries reflect the implemented A7 flow without implying a supported PotatoSeed route; compare them with the definition, server handlers, architecture and focused planting test, then run `git diff --check`.

### RF109 — Current buildable recipe count (P3; CR13-009/014)

- **Affected paths:** `docs/02-technical-architecture.md:506`, the seven registered build-item files, `recipes/building.json` and the catalogue loader.
- **Trigger and impact:** The unqualified architecture count says 19 placement recipes, but the current manifest has 22 unique buildables and the building recipe catalogue has 22 unique outputs; the stale count understates current direct-recipe coverage.
- **Evidence:** All seven build-item files are registered; the 22 unique buildable IDs match the 22 building-recipe outputs one-to-one. CR13-014 separately reconfirmed this against its CR01-matching recipe fingerprint and the loader routes construction outputs to `BuildableOutput`. The setup's 18-object figure is under an as-of-2026-10-18 heading and may remain a historical snapshot.
- **Proposed repair and dependency:** Update the unqualified architecture count to 22 after CR19 acceptance; retain the setup count only as a clearly dated snapshot or update it. Do not change recipe/buildable data during CR13.
- **Acceptance and validation:** No current unqualified count conflicts with the 22 registered buildables/recipes. Reparse the manifest, seven build-item files and building outputs; run `Scripts/Validate-CatalogueReferences.py` and `git diff --check`. No Unreal build is required.

## CR13-011 — wearable armor profile review

- **Reviewed inputs:** `boots.json`, `capes.json`, `helmets.json`, `leggings.json`, `shields.json` and `tunics.json` under `items/equipment/armor/`. All six current SHA-256 fingerprints match CR01 and all six files remain registered.
- **Evidence:** Parsed 16 unique wearable profiles across the six supported slots and cross-checked slot/stat bounds, acquisition and progression references, current descriptions, icon mappings and provenance. The prior review's catalogue-reference validator and focused UI icon validation passed for all assigned IDs. No structural or reference defect was confirmed.
- **Outcome:** Existing findings RF114–RF116 remain open in `BACKLOG.md`; this report records their current evidence below. RF95 remains the separate CR13-005/CR16-003 count-versus-slot handoff; this profile batch adds no duplicate.
- **Limits:** The prior review and this reconciliation are static. No Unreal build, automation tests, live equipment transaction, damage session, Editor preview or rendered icon inspection ran.

### RF114 — stale current armor status in architecture (P3; CR13-011)

- **Affected paths:** `docs/02-technical-architecture.md:1479-1489`, current armor profiles and the server damage path.
- **Trigger and impact:** The undated architecture section says equipped Armor remains zero and that equipment rules need separate gameplay work, which misstates the implemented wearable-defense path.
- **Evidence:** The six registered armor files contain 16 profiles, including positive Armor values. The server sums non-depleted equipped profile Armor before calling the damage resolver; the resolver subtracts it from physical damage. Current design guidance and focused damage-profile assertions describe this behavior.
- **Proposed repair and dependency:** After CR19 review acceptance, update or remove only the obsolete Armor-zero and pending-equipment statements; retain the weight/encumbrance limitations. Do not alter runtime or catalogue data during CR13.
- **Acceptance and validation:** Architecture no longer says wearable defenses are unapplied or equipment is unimplemented. Compare with the current game-design text, server calculation and focused damage test; run `git diff --check`. No Unreal build is needed.

### RF115 — missing authoritative A8b wearable-value source (P3; CR13-011)

- **Affected paths:** `docs/05-decision-log.md:5,186`, `docs/icon-provenance.md:233`, `docs/alpha-item-details.md#a8b` (missing), A8b setup histories and `docs/a9-acceptance.md:215-222`.
- **Trigger and impact:** The decision log and A9 provenance text identify `alpha-item-details.md` as the authoritative A8b contract, but that file is absent. Dated setup histories only partly preserve the values, so reviewers cannot trace the full delegated armor profile contract.
- **Evidence:** `docs/alpha-item-details.md` does not exist. Current setup histories omit a complete contract including Badger Fur Vest, while `docs/a9-acceptance.md` retains the Opal Helm table and the stale source citations remain.
- **Proposed repair and dependency:** After CR19 acceptance, preserve the complete A8b values in an existing current source or restore an appropriate concise contract, and repoint A9 references to retained records. Do not change catalogue values or gameplay during CR13.
- **Acceptance and validation:** The A8b decision points to an existing complete source, and A9 references resolve or are explicitly historical. Resolve Markdown targets and compare retained values against the profiles and A9 acceptance table; run `git diff --check`.

### RF116 — Boar Hide Cape warmth description (P3; CR13-011)

- **Affected paths:** `items/equipment/armor/capes.json:4-7`, item-detail presentation, the A4 decision and roadmap/setup guidance.
- **Trigger and impact:** The player-facing description calls the Boar Hide Cape “warm” even though the approved A4 contract defers cold insulation, implying an unsupported survival effect.
- **Evidence:** The item-detail widget displays the catalogue description. The A4 decision defers insulation/cold protection for the hide Boots and Cape, and the armor profile/runtime schema supplies Armor and condition behavior without a warmth effect.
- **Proposed repair and dependency:** After CR19 acceptance, remove or qualify “warm” so the description does not promise cold protection. Adding warmth would require separately approved scope; no catalogue or gameplay changes during CR13.
- **Acceptance and validation:** The displayed description no longer implies unsupported cold protection. Compare with item-detail presentation and the A4 decision; run the focused catalogue/detail checks and `git diff --check`. No Unreal build is required for a text-only correction.

## RF120 — Reconcile stale weapon profile and combat summaries (P3)

- **Source:** CR13-012; docs/02-technical-architecture.md:1036,1040,1042,1136; docs/07-development-setup.md:5162; schema-16 weapon profiles, attacks/attacks.json, item-level-up/rewards/item-stats.json, and KalmalaDamageProfileTest.cpp.
- **Affected path:** A developer or reviewer uses current-looking architecture/setup summaries to understand weapon durability, profile damage channels, and effective Basic values.
- **Trigger:** Architecture reports obsolete Stone starter values, Iron Knife/Pick/Spear values, Mace durability/channel claims, and effective Basic attack examples. The Mace setup passage says each tier uses knife durability and Crush exclusively, while Bronze/Iron Maces author Stab bonuses.
- **Impact:** Readers can misunderstand current weapon statistics and channels or derive incorrect expected combat values.
- **Evidence:** Schema-16 profiles author current durability and DamageBonuses; shared attack lists add Basic bonuses, and KalmalaDamageProfileTest.cpp asserts resulting combat values. The setup passage is presented as current guidance. This excludes the separate Mace reward disagreement (RF100), Iron Pick cost (RF122), and Bow status statement (RF125).
- **Proposed repair:** Correct only the named profile, channel, durability, and effective Basic summaries against current profiles, attacks, and tests. Keep RF100's unresolved reward target separate; do not change catalogue values or runtime behavior.
- **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
- **Acceptance:** The named architecture/setup passages accurately distinguish profile bonuses, shared Basic bonuses, and current durability/channels while keeping RF100 separate; no runtime or catalogue behavior changes.
- **Focused validation:** Compare each corrected value with the schema-16 profile, shared attack list, and focused damage-profile test; retain RF100's authority note and run git diff --check. No Unreal build is required.

## RF121 — Qualify Bronze Spear's range description (P3)

- **Source:** CR13-012; spears.json:15; KalmalaItemDetailWidget.cpp:21-44; KalmalaCombatComponent.h:64; KalmalaCombatComponent.cpp:174-200.
- **Affected path:** A player reads Bronze Spear's carried-item details and chooses it based on combat reach.
- **Trigger:** Its description calls it a "Long range weapon for keeping your enemies at a distance," but profiles have no per-weapon reach field and server melee uses a shared 220 cm range.
- **Impact:** The wording can imply a reach advantage absent from the current profile and server combat contract.
- **Evidence:** The carried-tool detail function falls back to the registered BronzeSpear profile description. Melee traces/sweeps use the shared AttackRange constant.
- **Proposed repair:** Replace or qualify the range wording. Weapon-specific reach requires separate approved gameplay scope; no combat change during CR13.
- **Dependencies:** After CR19 review acceptance; no catalogue/runtime edits during CR13.
- **Acceptance:** The player-facing description does not promise a unique reach mechanic absent from the server contract; gameplay remains unchanged.
- **Focused validation:** Compare revised text with the item-detail path, profile schema, and shared server range; run focused catalogue/detail checks when repaired and git diff --check.

## RF122 — Correct stale Iron Pick Forge cost in architecture (P3)

- **Source:** CR13-012; docs/02-technical-architecture.md:1040; Content/Data/GameCatalogues/recipes/stations/forge.json:63-78; docs/05-decision-log.md:11; schema-16 GameCatalogues.json.
- **Affected path:** A developer follows architecture guidance to author or review the Iron Pick Forge recipe.
- **Trigger:** Architecture says 4 Iron + 2 Wood + 2 Stone; the registered Forge row and user-directed decision set Forge level 1 at 10 Wood + 10 Iron.
- **Impact:** The obsolete cost may produce incorrect implementation or regression expectations.
- **Evidence:** The manifest registers the Forge recipe file; its IronPick row uses 10 Wood and 10 Iron, matching the 2026-10-20 decision-log target.
- **Proposed repair:** Update only the architecture cost to the authorized recipe. Keep RF99's distinct setup paragraph separate; do not change data.
- **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
- **Acceptance:** Architecture, decision log, and registered Forge recipe state the same Iron Pick cost; runtime/data remain unchanged.
- **Focused validation:** Compare the corrected sentence with forge.json, the manifest, and decision row; run git diff --check.

## RF123 — Restore provenance records for selected existing weapon, material and utility icons (P3)

- **Source:** CR13-012/013/015; `docs/icon-provenance.md`; the 12 Stone/Bronze/Iron Axe, Knife, Pick, and Spear identities, nine material identities FrostSalt, Amber, BoarHide, DeerHide, Coal, Densewood, Lightwood, Wood, and Stone, plus ConstructionHammer; their original/prepared/imported icons and canonical mappings.
- **Affected path:** A maintainer traces the generation source for an existing weapon, material, or utility icon before preparing or replacing it.
- **Trigger:** The 22 original artworks and canonical mappings/imported assets exist, but item-specific generation prompt/source records are unavailable for these identities.
- **Impact:** Artwork and runtime mappings remain available, but the provenance guide cannot verify or reproduce the icons' original generation sources.
- **Evidence:** Earlier CR13-012/013 checks recorded the 12 weapon and nine material provenance gaps. CR13-015's focused `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id ConstructionHammer` check confirmed the original, prepared 64x64 image, and imported target. `docs/icon-provenance.md` has no item-specific ConstructionHammer record, and its original PNG contains no text metadata chunks. This is not a missing-art or missing-mapping finding.
- **Proposed repair:** Add original prompt/source records where recoverable and explicitly record unavailable provenance otherwise. Do not replace or regenerate assets during CR13.
- **Dependencies:** After CR19 review acceptance; no icon or documentation edits during CR13.
- **Acceptance:** Each of the 22 existing canonical weapon/material/utility icons has a source/prompt record or explicit unavailable-provenance disposition; assets and mappings remain intact.
- **Focused validation:** Resolve all assigned IDs to original/prepared/imported files and mappings and to provenance entries; run catalogue reference/icon validators and `git diff --check`. No Unreal build is required.

## RF125 — Remove stale Bow shooting status from architecture (P3)

- **Source:** CR13-012; docs/02-technical-architecture.md:524; KalmalaBowCombat.cpp; KalmalaArrowProjectile.cpp; KalmalaBowDrawTest.cpp; item-level-up/rewards/item-stats.json.
- **Affected path:** A developer follows the current Bow progression summary when reviewing ranged combat.
- **Trigger:** Architecture says Bow has no damage reward "while shooting remains unimplemented." Bow's zero damage reward remains accurate, but shooting is implemented.
- **Impact:** Current feature status is misrepresented and may misdirect future work or make implemented behavior appear deferred.
- **Evidence:** Server Bow draw/release and projectile-damage paths exist, with a focused Bow draw test; the reward catalogue contains no Bow damage gain. RF61 is a separate feature-summary occurrence and RF99 is the setup paragraph.
- **Proposed repair:** Remove the obsolete implementation-status rationale while retaining the accurate zero-damage-reward statement. Do not change shooting or reward data.
- **Dependencies:** After CR19 review acceptance; no documentation edits during CR13.
- **Acceptance:** Architecture no longer says Bow shooting is unimplemented and still states the zero damage reward accurately; runtime/data are unchanged.
- **Focused validation:** Compare the revised statement with reward data, server Bow/projectile paths, and the focused test; run git diff --check.

## RF126 — Resolve Stone Mace cost authority mismatch (P3)

- **Source:** CR13-012; docs/02-technical-architecture.md:1034; Content/Data/GameCatalogues/recipes/stations/workbench.json:113-129; docs/05-decision-log.md.
- **Affected path:** A developer or reviewer follows architecture and the Workbench recipe when assessing Stone Mace progression cost.
- **Trigger:** Architecture says the Workbench Stone Mace recipe costs 1 Wood + 1 Stone; the registered StoneMace row costs 2 Wood + 2 Stone. No current decision-log authority for this discrepancy was found.
- **Impact:** The intended cost is unclear; changing either source without direction risks an unsupported balance decision.
- **Evidence:** The schema-16 manifest registers the Workbench file and its StoneMace row uses 2 Wood and 2 Stone at level 1. Architecture says 1 Wood and 1 Stone; the current decision log contains no StoneMace cost decision.
- **Proposed repair:** Obtain or locate the authoritative intended cost, then align architecture and recipe as directed. Do not guess the target or change data during CR13.
- **Dependencies:** Requires authoritative product direction before any data correction; no data or documentation edits during CR13.
- **Acceptance:** An explicit decision establishes the intended cost and architecture/recipe agree with it; no unsupported balance target is inferred.
- **Focused validation:** Compare both passages with the approved decision and schema-16 recipe after direction is available; run reference validation and git diff --check.

## RF130 — Remove Frost Salt's unsupported food-preservation claim (P3)

- **Source:** CR13-013; `Content/Data/GameCatalogues/items/materials/cooking-ingredients.json:4-7`; `Source/KalmalaUI/Private/KalmalaItemDetailWidget.cpp:13-16`; `Source/KalmalaGameplay/Private/KalmalaM9SourceLootContract.cpp:20`; `Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp:369-372`; registered recipe catalogues and gameplay/UI sources.
- **Affected path:** A player inspects Frost Salt in carried-item details and expects it to preserve food.
- **Trigger:** The catalogue description says “Sprinkle some to preserve your food,” but Frost Salt currently has a Tundra harvest source and no registered recipe or runtime preservation, spoilage, or food-storage consumer.
- **Impact:** Player-facing copy promises an unavailable use and can mislead players about why they should collect or use the item.
- **Evidence:** `DescribeItem` presents the registered catalogue description. The server source/reward contract maps `tundra-frost-salt-deposit` to FrostSalt, and the gathering contract selects FrostSalt for that source. No registered recipe references FrostSalt, and a focused gameplay/UI source search found no food-preservation, spoilage, or food-storage rule. This is not a missing-acquisition finding.
- **Proposed repair:** Replace or qualify the unsupported preservation wording with an accurate material description, unless an explicitly approved and implemented preservation feature is supplied. Do not add gameplay behavior or invent a use during CR13.
- **Dependencies:** After CR19 review acceptance; any preservation mechanic requires separate approved gameplay scope. No catalogue or runtime edits during CR13.
- **Acceptance:** Frost Salt's player-facing description does not promise an unavailable preservation use, or the use is separately approved and implemented with its own validation; server authority and storage behavior remain unchanged.
- **Focused validation:** Compare revised text with the item-detail path, Tundra acquisition contract, registered recipes, and current food/storage implementation; run focused catalogue/detail checks when repaired and git diff --check. No Unreal build is required.

## RF134 — Cook the string-loaded audio packages (P2; CR13-016)

- **Source:** CR13-016; `Config/DefaultGame.ini:8-14`; `Source/KalmalaUI/Private/KalmalaAmbientAudioSubsystem.cpp:33-54,198-1196`; `Source/KalmalaUI/Public/KalmalaAmbientAudioSubsystem.h`; `Content/Kalmala/Audio/`.
- **Affected path:** Players using a packaged build that expects local ambient loops and owner-local feedback cues.
- **Trigger:** The ambient subsystem resolves audio packages with `LoadObject<USoundWave>` from C++ string paths. Its SoundWave pointers are transient, and the current `DefaultGame.ini` cooks `/Game/Kalmala/UI` and lists Menu/Prototype maps but has no `/Game/Kalmala/Audio` cook rule. The existing working-tree Prototype map addition does not add audio content.
- **Impact:** Audio packages without a cook-visible asset reference may be omitted, leaving runtime loads unresolved and the local audio layer silent in packaged builds.
- **Evidence:** All eight CR13-016 package hashes match CR01 and current main. Each package contains its expected `/Game/Kalmala/Audio/<name>` identity and `/Script/Engine.SoundWave` class marker; its import record points to `Source/<name>.wav` and the recorded MD5 matches the checked-in WAV. Runtime uses string path constants, and the header stores the waves in transient pointers. The architecture/setup guides say existing static and peer checks do not establish packaged playback. No packaged cook was run.
- **Proposed repair:** Add `/Game/Kalmala/Audio` to the explicit cook set or provide equivalent cook-visible asset references. Do not change audio behavior or assets during CR13.
- **Dependencies:** After CR19 review acceptance; no packaging configuration or runtime edits during CR13.
- **Acceptance:** A packaged cook contains the runtime audio packages and packaged runtime checks resolve ambient beds and local feedback cues with existing non-audio fallbacks intact.
- **Focused validation:** Inspect the cook manifest/package contents for `/Game/Kalmala/Audio`, run focused packaged audio coverage, and run `git diff --check`.

## CR13-012 — Weapon profile catalogue review

- **Reviewed inputs:** axes.json, bows.json, knives.json, maces.json, pickaxes.json, spears.json, and swords.json under items/equipment/weapons/. All seven current SHA-256 fingerprints match CR01 and all seven files remain registered in itemProfileFiles.
- **Evidence:** Parsed 25 unique profiles; checked identity/stat constraints and cross-referenced acquisition, level-one cost, reward, and shared attack data. No structural or reference defect was confirmed. Server combat uses the shared 220 cm melee reach and combines profile DamageBonuses with shared Basic attack bonuses; Bow shooting and projectile damage are server-owned. The Bronze Spear wording, stale summaries, Iron Pick cost description, missing weapon-icon provenance, and Stone Mace cost mismatch are tracked by RF120-RF123, RF125, and RF126.
- **Result:** The seven CR13-012 files and aggregate batch are reviewed. CR13-013 is next; CR13 remains unchecked. All six related RF tasks are already unchecked in BACKLOG. No gameplay, test, script, configuration, catalogue, asset, or game documentation was modified.
- **Checks / limits:** Current-main checks confirmed all seven CR01 hashes and registrations, parsed 25 unique ItemIds, and reran Scripts/Validate-CatalogueReferences.py successfully for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references, and 152 ledger-backed icon mappings. Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id passed individually for all 25 weapon identities, including the current WoodenClub package. Relevant project brief, game design, architecture, roadmap, decision log, setup guidance, feature summary, acceptance references, and icon provenance were reviewed. No tests, Unreal build, runtime session, Editor preview, or rendered icon inspection ran; full verification remains deferred and this is not milestone-final verification.
- **Observable impact:** The main-checkout ledger now records CR13-012 coverage and its six finding handoffs; gameplay, data, assets, and player behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Melee reach/damage and Bow projectile resolution remain server-owned; no runtime or network code changed.
- **Known limitations / remaining scope:** Static inspection does not execute Unreal catalogue loading or combat. Icon validation was structural, not visual. RF123 provenance scope later expanded beyond the weapon identities; RF126 still needs authoritative direction before any cost correction. Later CR13 batches remain pending.

## CR13-013 — Material catalogue review

- **Reviewed inputs:** `cooking-ingredients.json`, `crafting-ingredients.json`, `gems.json`, `hides.json`, `metal.json`, `ore.json`, `others.json`, and `wood.json` under `Content/Data/GameCatalogues/items/materials/`. All eight SHA-256 fingerprints match CR01 and the current main checkout; all remain registered in the schema-16 manifest.
- **Evidence:** Parsed 42 unique material ItemIds and checked manifest registration and references to recipes, source loot, Smelter processing, crafting, and item-detail presentation. Ten reserved-gem descriptions match their registered A9 gear recipes. No additional material identity, schema, reference, or recipe defect was confirmed. RF130 tracks Frost Salt's unsupported preservation wording while its server-owned Tundra source exists. RF123 now includes nine material icon provenance gaps; none of their originals has PNG text metadata or an item-specific provenance record. RF106's existing material-category documentation scope was not duplicated.
- **Result:** The eight CR13-013 file rows and aggregate batch are reviewed. CR13-014 is next; CR13 remains unchecked. RF123 and RF130 are already queued as unchecked tasks in BACKLOG. No gameplay, test, script, configuration, catalogue, asset, or game documentation was modified.
- **Checks / limits:** `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references, and 152 ledger-backed canonical runtime icon mappings. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id` passed individually for all 42 material identities. Current-main parsing confirmed 42 unique IDs, all eight assigned hashes, and manifest registration. No tests, Unreal build, runtime session, Editor preview, or rendered icon inspection ran; full verification remains deferred and this is not milestone-final verification.
- **Observable impact:** The main-checkout ledger now records CR13-013 material coverage and findings; runtime, catalogue data, assets, and player behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Material harvesting, source rewards, recipes, and storage retain existing shared/server-owned paths; no runtime or network code changed.
- **Known limitations / remaining scope:** Static checks do not execute Unreal catalogue loading, harvesting, recipes, or storage. Icon validation was structural, not visual. The nine material provenance records remain open under RF123; CR13 remains incomplete.

## CR13-014 — Building recipe catalogue review

- **Reviewed input:** `Content/Data/GameCatalogues/recipes/building.json`. Its SHA-256 fingerprint matches CR01 and current main; the schema-16 manifest registers it in `recipeFiles`.
- **Evidence:** Parsed 22 unique RecipeIds and 22 unique direct-material outputs. The output set matches the 22 buildable identities across all seven registered `items/buildItems/` files; all 34 ingredient entries resolve to registered material definitions, and no portable `Kit` value appears in the catalogue identities. Loader normalization routes these outputs into runtime `BuildableOutput`; recipe validation enforces distinct buildable outputs with no inventory Output or station requirement, and the server crafting/placement path requires the carried Construction Hammer and derives the material exchange from the registered recipe. No structural, reference, or portable-construction-item defect was confirmed. The separate architecture count mismatch remains covered by RF109.
- **Result:** The CR13-014 aggregate and assigned file row are reviewed. CR13-015 is next; CR13 remains unchecked. RF109 is already an unchecked remediation task in BACKLOG and the findings ledger; no duplicate was added. No gameplay, test, script, configuration, catalogue, asset, or game documentation was modified.
- **Checks / limits:** Current-main parsing confirmed the CR01 fingerprint, manifest registration, 22 unique recipe IDs/outputs, one-to-one buildable coverage, and 34 material references. `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references, and 152 ledger-backed canonical runtime icon mappings. Loader, recipe-validation, server crafting/placement, and `KalmalaDirectBuildTest.cpp` source were inspected; tests were not run. No Unreal build, runtime placement, or Editor session ran; full verification remains deferred and this is not milestone-final verification.
- **Observable impact:** The main-checkout ledger now records CR13-014 recipe coverage and RF109's confirming evidence; runtime, catalogue data, assets, and player behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Recipe identity, Hammer ownership, direct material cost, and placement validation remain server-derived; no runtime or network code changed.
- **Known limitations / remaining scope:** Static parsing, reference validation, and source inspection do not execute Unreal catalogue loading or live placement/payment. CR13 remains incomplete.

## CR13-015 — Station recipe catalogue review

- **Reviewed inputs:** `a9-gear.json`, `cauldron.json`, `cooking-rack.json`, `forge.json`, `frying-pan.json`, `inventory.json`, and `workbench.json` under `Content/Data/GameCatalogues/recipes/stations/`. All seven SHA-256 fingerprints match CR01 and current main; all seven remain registered in the schema-16 manifest.
- **Evidence:** Current-main parsing found 72 unique recipe IDs and result identities: 42 ordinary outputs and 30 level-one equipment acquisitions. All 152 ingredient entries have positive quantities; ordinary recipes are enabled with positive output/batch bounds, acquisition station/level forms are valid, and no result is a portable `Kit`. The catalogue reference validator resolved all registered references. The prior isolated handoff's A9 gem-gear acceptance cross-check remains applicable because all seven assigned recipe hashes match. Server crafting checks authority, recipe identity/enabled state, batch, station and inventory exchange; focused tests were inspected in the handoff but not run. RF102's stale architecture meal/cooking summary is confirmed against the 14 enabled cooking rows and server food allowlist. RF123 now includes the separately confirmed ConstructionHammer provenance gap. Existing RF122/RF126 cost discrepancies remain separate.
- **Result:** The CR13-015 aggregate and seven assigned file rows are reviewed. RF102/RF123 remain existing unchecked remediation tasks in `BACKLOG.md`; no duplicate or new recipe-data finding was added. CR13-016 is next and CR13 remains incomplete. No gameplay, test, script, configuration, catalogue, asset, or game documentation was modified.
- **Checks / limits:** `python -B Scripts/Validate-CatalogueReferences.py` passed for 61 registered JSON files, 152 unique item/buildable definitions, 776 resolved references, and 152 ledger-backed canonical runtime icon mappings. `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id ConstructionHammer` passed. Focused parsing confirmed all seven CR01 hashes, manifest registrations, row identities and recipe/acquisition invariants. The ConstructionHammer original PNG has no `tEXt`, `zTXt`, or `iTXt` chunks. Source contracts were inspected; no tests, Unreal build, runtime crafting session, Editor load, or visual icon inspection ran. Full verification remains deferred; this is not milestone-final verification.
- **Observable impact:** The review ledger now records CR13-015 coverage and its RF102/RF123 evidence; runtime, catalogue content, assets, and player behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Recipe identity, station checks, material payment, outputs, and accepted crafting transactions remain server-validated; no runtime or network code changed.
- **Known limitations / remaining scope:** Static parsing and validators do not execute Unreal catalogue loading or live crafting. Icon validation was structural, not visual. RF102 and RF123 remain open for post-CR19 remediation; CR13 remains incomplete.

## CR13-016 — Audio package review

- **Reviewed inputs:** `BiomeBed.uasset`, `CombatResultCue.uasset`, `DiscoveryAcknowledgedCue.uasset`, `FireBed.uasset`, `GeneratedOceanEntryCue.uasset`, `GeneratedOceanExitCue.uasset`, `InteractionAcceptedCue.uasset`, and `InteractionRejectedCue.uasset` under `Content/Kalmala/Audio/`. All eight current SHA-256 values match CR01.
- **Evidence:** Each package has the Unreal package magic, expected `/Game/Kalmala/Audio/<name>` identity, `SoundWave` class marker, and matching embedded import records. Every import path is `Source/<name>.wav`, and each stored MD5 matches the checked-in source WAV. All eight sources are mono PCM16 at 22.05 kHz, non-silent, and have no clipped samples; the BiomeBed and FireBed first/last samples match. All eight runtime path constants resolve through `LoadObject<USoundWave>` in the local audio subsystem. The current worktree `DefaultGame.ini` has no `/Game/Kalmala/Audio` cook rule; its pre-existing Prototype map entry does not affect this. RF134 is the existing cook-inclusion finding; no additional asset or runtime defect was confirmed.
- **Result:** The CR13-016 aggregate and only its eight assigned package rows are reviewed. RF134 remains an unchecked remediation task in `BACKLOG.md`; no duplicate finding or Backlog checkbox change was needed. CR13-017 is next and CR13 remains incomplete. No code, tests, scripts, configuration, audio assets, or game documentation were modified.
- **Checks / limits:** Focused read-only parsing confirmed all eight CR01 hashes, package identities/classes, import paths/MD5s, and WAV format/content. Static source inspection confirmed local string-load references and current cook configuration. No tests, Unreal build, cook, runtime session, Editor load, or audio audition ran. Full verification remains deferred; this is not milestone-final verification.
- **Observable impact:** The review ledger records CR13-016 package coverage and RF134 evidence; runtime, configuration, audio assets, and player behavior are unchanged.
- **Networking/authority:** These cues are local presentation driven by local context and owner-only result state; audio itself is not replicated. No authority, privacy, or persistence change was identified.
- **Known limitations / remaining scope:** Static metadata and source inspection do not prove Unreal package loading, audible quality, device mixing, spatialization, or packaged playback. RF134 remains open for post-CR19 remediation; CR13 remains incomplete.

## CR13-017 — Audio package review

- **Reviewed inputs:** MovementFootfallCue.uasset, MovementJumpCue.uasset, MovementLandingCue.uasset, RainBed.uasset, SupportAcceptedCue.uasset, SupportBearsVigorCue.uasset, SupportBearsVigorExpiryCue.uasset, and SupportDeerCallCue.uasset under Content/Kalmala/Audio/. Each current package SHA-256 matches its CR01 coverage fingerprint and the current main HEAD.
- **Evidence:** All eight packages have the Unreal package magic, expected /Game/Kalmala/Audio/<name> identity, SoundWave class marker, and one unique embedded import record. Each import record stores the FileMD5 of its canonical checked-in mono PCM16 22.05 kHz WAV; five RelativeFilename values resolve to Source/<name>.wav, while the three stale root paths noted below do not. All source MD5 records match and samples are nonzero and unclipped. Movement clips are 0.12 s, 0.18 s, and 0.16 s; RainBed is an 8 s loop; SupportAcceptedCue is 0.8 s; the three selected support-effect clips are 0.42 s. The RainBed generator builds a one-second tail-to-head crossfade and runtime enables looping while accepted precipitation is active. The local subsystem maps movement cues to grounded footfall, jump, and landing transitions; support activation uses accepted owner-local feedback and the existing active effect, while Bear's Vigor expiry follows the existing effect-state transition.
- **Existing finding cross-check:** SupportBearsVigorCue, SupportBearsVigorExpiryCue, and SupportDeerCallCue store a RelativeFilename under ../../../Audio/ that resolves to a missing project-root Audio path, although each stored FileMD5 matches its checked-in Source/<name>.wav. RF135 already covers these stale root import paths and the duplicate /Source packages; no duplicate finding was added. Current DefaultGame.ini still has no /Game/Kalmala/Audio cook rule; RF134 already covers the string-loaded package cook gap. Its pre-existing Prototype map entry does not cook audio.
- **Result:** CR13-017 and only its eight assigned package rows are now reconciled as reviewed in the main-checkout coverage ledger. RF134 and RF135 remain the existing unchecked remediation tasks. No Backlog checkbox changed; CR13 remains open and CR13-018 is next. Source WAV rows remain assigned to later CR13 batches.
- **Checks / limits:** A read-only parser confirmed all eight CR01/current-HEAD hashes, package identity/class markers, import records and MD5 links, WAV format, duration, nonzero samples, and clipping counts. Static inspection checked runtime path constants, local movement submissions, rain looping/gating, and owner-local support cue gates. The earlier isolated CR13-017 progress entry was checked against these current-main results. No tests, Unreal build, cook, runtime session, Editor load, or audio audition ran; this is not milestone-final verification and full verification remains deferred.
- **Observable impact:** Review evidence and the eight package coverage statuses now agree with the current main checkout; runtime, configuration, audio assets, and player behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Audio remains local presentation; movement samples the local controller, and support cues consume the existing owner-local accepted feedback/effect state. No authority, privacy, or persistence path changed.
- **Known limitations / remaining scope:** Static metadata and source inspection do not prove Unreal package loading, audible quality, device mixing, spatialization, or packaged playback. The three stale import paths remain queued under RF135, the audio cook gap remains queued under RF134, and CR13 remains incomplete.

## CR13-018 — Audio package review

- **Reviewed inputs:** SupportHearthShieldCue.uasset, SupportHearthShieldExpiryCue.uasset, SupportMendingCue.uasset, WaterBed.uasset, WetStatusCue.uasset, and WindBed.uasset under Content/Kalmala/Audio/. All six current package SHA-256 values match their CR01 coverage fingerprints and current main HEAD.
- **Evidence:** All packages have the Unreal package magic, expected /Game/Kalmala/Audio/<name> identity, SoundWave class marker, and a unique import record. Each recorded FileMD5 matches its checked-in mono PCM16 22.05 kHz source WAV; all sources contain nonzero samples and no clipped samples. Support clips are 0.42 s, WaterBed and WindBed are 8 s, and WetStatusCue is 0.8 s. WaterBed and WindBed generation use a one-second crossfade. The runtime explicitly sets both beds to loop; WaterBed is loaded only after local visible-water sampling finds accepted sea or lake surface, while WindBed starts in normal local play and follows accepted replicated wind strength. WetStatusCue follows the local pawn's existing server-owned Wet status transition. Support activation cues use the existing accepted owner-local feedback/effect, and Hearth Shield expiry follows its existing effect-state transition.
- **Existing finding cross-check:** SupportHearthShieldCue, SupportHearthShieldExpiryCue, and SupportMendingCue record RelativeFilename under ../../../Audio/, which resolves to a missing project-root Audio path. Their stored FileMD5 values nevertheless match the canonical checked-in Source/<name>.wav files. RF135 already covers these stale root imports and duplicate /Source packages; no duplicate finding was added. Current DefaultGame.ini still contains no /Game/Kalmala/Audio cook rule; RF134 covers the shared string-loaded package cook gap.
- **Result:** CR13-018 and only its six assigned package rows are now reconciled as reviewed in the main-checkout coverage ledger. RF134 and RF135 remain the existing unchecked remediation tasks. No Backlog checkbox changed; CR13 remains open and CR13-019 is next. The assigned source WAVs remain pending in their later batches.
- **Checks / limits:** A read-only parser confirmed all six CR01/current-HEAD hashes, package identity/class markers, import records and MD5 matches, WAV format/duration, nonzero samples, and clipping counts. Static inspection checked SoundWave loop metadata and runtime overrides, local visible-water sampling, accepted replicated wind, Wet-status playback, and support activation/expiry paths. The earlier isolated CR13-018 report was cross-checked against current-main files. No tests were rerun during this main-checkout reconciliation; no Unreal build, cook, runtime session, Editor load, or audio audition ran; this is not milestone-final verification and full verification remains deferred.
- **Observable impact:** Main-checkout review evidence and the six CR13-018 coverage rows now agree; runtime, configuration, audio assets, and player behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Ambient audio remains local; wind/Wet/support cues read existing server-owned or owner-local state, and water audio uses local visible context. No authority, privacy, or persistence path changed.
- **Known limitations / remaining scope:** Static inspection does not prove Unreal package loading, audible quality, device mixing, spatialization, or packaged playback. The three stale support import paths and shared audio cook gap remain queued under RF135/RF134. Later audio source and authored-content batches remain pending.

## CR13-019 — Audio source WAV review

- **Reviewed inputs:** `BiomeBed.wav`, `CombatResultCue.wav`, `DiscoveryAcknowledgedCue.wav`, `FireBed.wav`, `GeneratedOceanEntryCue.wav`, `GeneratedOceanExitCue.wav`, `InteractionAcceptedCue.wav`, and `InteractionRejectedCue.wav` under `Content/Kalmala/Audio/Source/`. All eight SHA-256 values match their CR01 coverage fingerprints and current `HEAD`; none has a worktree diff.
- **Evidence:** A read-only WAV parser found all eight to be mono PCM16 at 22.05 kHz, with nonzero samples and no full-scale clipping. `BiomeBed` and `FireBed` are eight-second beds whose first and last PCM samples match exactly. The six one-shots range from 0.24 to 0.50 seconds. The in-repository generator groups and the architecture cue contract match the source names and runtime paths. The documented triggers remain local ambience, owner-only accepted combat/discovery/interaction feedback, and local generated-ocean movement transitions. The matching root SoundWave package identities and source MD5 links were already checked in CR13-016. `DefaultGame.ini` has UI cook paths but no audio cook path; existing RF134 records that packaged-build risk.
- **Result:** Updated the CR13-019 aggregate and only its eight assigned source rows. No additional actionable source defect was confirmed; RF134 remains the existing unchecked packaging finding. CR13 remains open and CR13-020 is next. No Backlog checkbox changed.
- **Checks / limits:** Re-read the review instructions and relevant project/design/architecture/roadmap/decision/setup context; statically inspected generator mappings, runtime path constants and gates, the existing package-review evidence, and cook configuration. No tests, Unreal build/cook, runtime session, Editor load, or audio audition ran. Full verification is deferred; this is not milestone-final verification.
- **Observable impact:** Review coverage and its evidence are recorded; runtime, configuration, audio sources, and player-visible behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Audio remains local presentation over existing local or owner-only state; the server retains authority over combat, discoveries, interactions, and movement. No authority, privacy, or persistence path changed.
- **Known limitations / remaining scope:** Static source and metadata checks do not establish audible quality, Unreal loading, mixing, spatialization, or packaged playback. RF134 remains queued for post-CR19 remediation; later CR13 source and authored-content batches remain pending.

## CR13-020 — Audio source and import review

- **Reviewed inputs:** `MovementFootfallCue.wav`, `MovementJumpCue.wav`, `MovementLandingCue.wav`, `RainBed.wav`, `SupportAcceptedCue.wav`, `SupportBearsVigorCue.wav`, `SupportBearsVigorCue.uasset`, and `SupportBearsVigorExpiryCue.uasset` under `Content/Kalmala/Audio/Source/`. All eight SHA-256 values match their CR01 fingerprints and current `HEAD`; no assigned file has a worktree diff.
- **Evidence:** All six assigned WAVs decode as mono PCM16 at 22.05 kHz with nonzero samples and no clipping. Footfall, Jump, Landing, and BearsVigor are 0.12, 0.18, 0.16, and 0.42 seconds; SupportAccepted is 0.80 seconds; RainBed is eight seconds. The rain generator applies its documented one-second tail-to-head crossfade. Its endpoint jump is 9,122 PCM units, below the internal adjacent-sample 99th percentile (12,594) and maximum (14,754). Both assigned `.uasset` files have the Unreal package magic, expected `/Game/Kalmala/Audio/Source/<name>` package identity, `SoundWave` class marker, and an `AssetImportData` record whose source filename and FileMD5 match the canonical WAV. Searches found those `/Source` package identities only in the packages themselves; runtime constants load the root `/Game/Kalmala/Audio/` counterparts. RF135 already covers the duplicate support packages and their root import-source reconciliation.
- **Runtime cross-check:** The local subsystem submits movement clips on grounded footfall distance/cooldown, ground-to-fall jump, and fall-to-ground landing; RainBed follows accepted replicated precipitation in local presentation. Support activation plays only for a new accepted owner-only feedback serial and uses the specific BearsVigor clip, with SupportAccepted as fallback when a specific cue cannot load. The expiry cue follows the existing owner-visible active-effect transition. No new authority or hidden-state path was found.
- **Result:** Updated the CR13-020 aggregate and only its eight assigned rows. RF135 remains the existing unchecked finding; no duplicate finding was added. CR13 remains open and CR13-021 is next. No Backlog checkbox changed.
- **Checks / limits:** Re-read repository instructions and the relevant project/design/architecture/roadmap/decision/setup context. A read-only parser checked CR01/HEAD hashes, WAV properties, SoundWave package identity/import records, and RainBed seam statistics; static inspection checked generator groups, runtime paths/gates, and package references. No tests, Unreal build/cook, runtime session, Editor load, or audio audition ran. Full verification is deferred; this is not milestone-final verification.
- **Observable impact:** Review coverage and RF135 corroboration are recorded; source assets, runtime behavior, packaging, and player-visible behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Movement and ambience audio remain local presentation; support cues read existing accepted owner-only state while gameplay effects remain server-authoritative. No network or persistence behavior changed.
- **Known limitations / remaining scope:** Static checks do not establish audible quality, Unreal loading, device mixing, spatialization, or packaged playback. RF134 and RF135 remain queued; later CR13 source and authored-content batches remain pending.

## CR13-021 — Support-effect source review

- **Reviewed inputs:** `SupportBearsVigorExpiryCue.wav`, `SupportDeerCallCue.uasset`, `SupportDeerCallCue.wav`, `SupportHearthShieldCue.uasset`, `SupportHearthShieldCue.wav`, `SupportHearthShieldExpiryCue.uasset`, `SupportHearthShieldExpiryCue.wav`, and `SupportMendingCue.uasset` under `Content/Kalmala/Audio/Source/`. All eight SHA-256 values match their CR01 fingerprints and current `HEAD`; no assigned file has a worktree diff.
- **Evidence:** The four assigned WAVs are mono PCM16 at 22.05 kHz, 0.42 seconds each, non-silent, and unclipped. All four assigned packages have the Unreal package magic, expected `/Game/Kalmala/Audio/Source/<name>` identity, `SoundWave` class marker, and an `AssetImportData` source filename/FileMD5 matching the corresponding checked-in WAV. Each assigned `/Source` identity appears only in its own package; runtime constants load the root `/Game/Kalmala/Audio/` counterparts. The four root packages record `../../../Audio/<name>.wav`, which resolves to a missing project-root `Audio/` path, while their source MD5s still match. RF135 already tracks these stale root import paths and unused duplicate packages.
- **Runtime cross-check:** The local subsystem selects Mending, Hearth Shield, and Deer Call clips only for a new accepted owner-only support feedback serial and the existing active effect. Mending and Deer Call are immediate effects; Hearth Shield expiry follows the existing owner-visible shield/strength state transition. No new audio authority, private-state, or timer path was found.
- **Result:** Updated the CR13-021 aggregate and only its eight assigned rows. RF135 remains the existing unchecked finding; no duplicate finding was added. CR13 remains open and CR13-022 is next. No Backlog checkbox changed.
- **Checks / limits:** Re-read repository instructions and relevant brief/design/architecture/roadmap/decision/setup context. A read-only parser checked CR01/HEAD hashes, WAV properties, package identities/import MD5s, root import-path resolution, and package references; static inspection checked generator groups, runtime constants/gates, and RF135. No tests, Unreal build/cook, runtime session, Editor load, or audio audition ran. Full verification is deferred; this is not milestone-final verification.
- **Observable impact:** Main-checkout review coverage and the RF135 corroboration are recorded; assets, runtime behavior, configuration, and player-visible behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Support feedback remains local presentation of existing owner-only accepted state; gameplay effects remain server-authoritative. No network or persistence behavior changed.
- **Known limitations / remaining scope:** Static checks do not establish audible quality, Unreal loading, mixing, spatialization, or packaged playback. RF134 and RF135 remain queued; later CR13 source and authored-content batches remain pending.

## CR13-022 - Audio source WAV review

- **Reviewed inputs:** `SupportMendingCue.wav`, `WaterBed.wav`, `WetStatusCue.wav`, and `WindBed.wav` under `Content/Kalmala/Audio/Source/`. All four SHA-256 values match their CR01 coverage fingerprints and current `HEAD`; none has a worktree diff.
- **Evidence:** All four are mono PCM16 at 22.05 kHz, non-silent, and unclipped. SupportMendingCue is 0.42 s and WetStatusCue is 0.80 s; WaterBed and WindBed are eight-second loops. The generator uses a one-second tail-to-head crossfade for both loop beds. Their measured last-to-first sample jumps (873 and 1,504 PCM units) are below each bed's internal adjacent-sample 99th percentile (9,715 and 3,716, respectively). The corresponding root SoundWave package import/source evidence was already checked in CR13-018.
- **Runtime cross-check:** WindBed loops during normal local play and follows the existing weather state's WindStrength. WaterBed loops only after bounded local samples find sea or visible inland-lake water with an unobstructed trace from the local view. WetStatusCue plays on the owning pawn's existing server-owned Wet-status transition. SupportMendingCue is selected only for a new accepted owner-only support feedback serial and the existing active Mending effect, with the documented generic accepted-cue fallback if the specific asset cannot load. No new authority, privacy, or persistence path was found.
- **Existing finding cross-check:** The string-loaded audio cook gap remains covered by RF134. SupportMending's stale root import and duplicate `/Source` package are already included in RF135; no duplicate finding was added.
- **Result:** Updated the CR13-022 aggregate and only its four assigned WAV rows. No additional actionable defect was confirmed. No Backlog checkbox changed; CR13 remains open and CR13-023 is next.
- **Checks / limits:** Read-only parsing checked all four CR01/current-HEAD fingerprints, WAV metadata/sample presence/clipping, and loop seam statistics. Static inspection checked generator grouping/crossfade behavior, runtime asset paths and local cue gates, architecture/setup contracts, and existing RF134/RF135 evidence. No tests, Unreal build/cook, runtime session, Editor load, or audio audition ran. Full verification is deferred; this is not milestone-final verification.
- **Observable impact:** Main-checkout coverage and the CR13-022 review result now agree; audio assets, runtime behavior, configuration, and player-visible behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. Audio remains local presentation over local context and existing server-owned or owner-only state; gameplay authority remains server-side. No network or persistence behavior changed.
- **Known limitations / remaining scope:** Static metadata and source inspection do not establish audible quality, Unreal package loading, mixing, spatialization, or packaged playback. RF134 and RF135 remain queued and CR13 is incomplete.


## RF136 Finding: Refresh stale biome-classification preview palette

- **Source:** CR13-023; `Content/Kalmala/Developer/WorldGeneration/BiomeClassification.ppm`; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:30-41`; `docs/biome-height-rework.md`.
- **Severity:** P3. The mismatch affects a developer preview only; no runtime or gameplay effect was found.
- **Trigger:** A developer uses the tracked biome-classification PPM to understand current generated biome regions.
- **Impact:** The preview can mislead manual inspection because three class colors no longer match the current commandlet palette.
- **Evidence:** The P6 image contains `(76,113,55)`, `(213,236,238)`, and `(104,98,112)` for Mossy Mire, Freezing Tundra, and Thunder Mountains. Current `GetBiomeColor` uses `(113,79,55)`, `(153,153,151)`, and `(240,240,240)`, changed in `e441d0b8`; the PPM history points to `cb5175f1`. The architecture note states that the image uses the commandlet palette. This is already queued as unchecked RF136 in the pre-existing worktree backlog edit; no duplicate ID was created.
- **Proposed repair:** After CR19 review acceptance, re-render only `BiomeClassification.ppm` using the current commandlet defaults (`Seed=10323456789`, `Size=256`, `Extent=20000` cm). Do not change PPM assets during CR13.
- **Dependencies:** After CR19 acceptance; no generated-content changes during CR13.
- **Acceptance:** Every represented biome uses the current commandlet palette, and the PPM is reproducible at the original defaults.
- **Focused validation:** After an Editor build, render to a short temporary directory, verify all represented RGB colors, compare the output with the tracked image, and run `git diff --check`.

## RF234 Refresh stale world-generation scalar previews

- **Source:** CR13-023; `Content/Kalmala/Developer/WorldGeneration/{Elevation,Flora,Humidity,Temperature}.ppm`; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp`; `Source/KalmalaWorld/Public/KalmalaWorldFieldSampler.h`, `KalmalaWorldGenerationSeeds.h`, and `KalmalaRegionalTuning.h`.
- **Severity:** P3. These are developer previews; no runtime consumer or gameplay effect was found.
- **Trigger:** A developer uses the tracked scalar PPMs to inspect current world fields.
- **Impact:** The maps display distributions from earlier field equations and can mislead manual review of current world generation.
- **Evidence:** All four files match their CR01 hashes and current `HEAD`, with their last asset changes in `cb5175f1` on 2026-09-02. The sampler in that commit used direct normalized Perlin values at frequencies 0.00035/0.00050/0.00028/0.00075 and seed offsets multiplied by 31. Current code uses a new regional tuning set and changed field formulas/frequencies; Elevation is further adjusted by the master land-water mask, and `DeriveFieldSeed` changed after the image commit. The visualization commandlet sends each current sampled channel through `AddFieldPixel`, while the tracked scalar files retain the older output. Static reference searches found no runtime PPM load.
- **Proposed repair:** After CR19 review acceptance, regenerate only the four scalar PPMs with the current commandlet's original default seed, size, and extent. Do not edit them during CR13.
- **Dependencies:** After CR19 acceptance; no generated-content changes during CR13.
- **Acceptance:** The four tracked PPMs match output from the current commandlet at the documented defaults and remain grayscale maps of their respective fields.
- **Focused validation:** After an Editor build, write a fresh four-file render to a short temporary directory, compare pixel payloads and hashes with the tracked files, inspect the previews, and run `git diff --check`. Broad peer generation tests are not needed for this artifact-only repair.

## CR13-023 World-generation PPM review

- **Reviewed inputs:** `BiomeClassification.ppm`, `Elevation.ppm`, `Flora.ppm`, `Humidity.ppm`, and `Temperature.ppm` under `Content/Kalmala/Developer/WorldGeneration/`. All five SHA-256 values match their CR01 coverage fingerprints and current `HEAD`; none has a worktree diff.
- **Evidence:** Each image is a valid 256x256 P6 RGB PPM with a 196,608-byte pixel payload. Elevation, Flora, Humidity, and Temperature are nonconstant grayscale maps. Their unique-shade counts/ranges are 188/[16,209], 214/[0,222], 184/[36,219], and 190/[39,228]. Visual inspection found coherent continuous patterns and no image-structure corruption. BiomeClassification has seven represented colors; four match the current palette while three are legacy values: `(76,113,55)` (686 pixels), `(213,236,238)` (3,934), and `(104,98,112)` (548). It contains none of the current replacement colors for those three biomes.
- **Reference cross-check:** The Editor commandlet writes these five default-path files from the four `FKalmalaWorldFieldSampler` channels and `FKalmalaBiomeClassifier`. The design/architecture and decision log retain the continuous field and revision-7 biome contracts; the setup and biome-height notes describe commandlet output and its palette. Searches found no runtime load of these PPMs. Git history confirms the five files were added with the visualization commandlet in `cb5175f1`; the current scalar sampler, tuning, and field-seed derivation changed after that commit. RF136 covers the palette mismatch, and RF234 covers the stale scalar distributions.
- **Result:** Updated the CR13-023 aggregate and its five assigned rows, recorded both finding evidence entries, and added unchecked RF234 to the review findings remediation queue. RF136 is already present as unchecked in the pre-existing `BACKLOG.md` worktree edits; those edits were preserved. CR13 remains open and CR13-024 is next. No backlog review checkbox changed.
- **Checks / limits:** Re-read `AGENTS.md`, the CR13 plan, relevant `PROGRESS.md` history, review ledgers, and project brief/design/architecture/roadmap/decision-log/setup/biome-height context. A read-only parser verified the five CR01/current-HEAD hashes, P6 headers, dimensions, exact payload lengths, palette counts, and grayscale ranges. Static source/history inspection confirmed the changed field equations, seed derivation, default commandlet path, and current RGB palette. Visually inspected all five images after converting them to temporary PNG previews outside the repository; those previews were removed. `git diff --check` reported one pre-existing trailing-space line at `BACKLOG.md:167`; the staged CR13-023 changes passed `git diff --cached --check`. No tests, Unreal build, commandlet, automation, peer session, or packaged verification ran.
- **Full verification:** Deferred; this is not milestone-final verification. Fresh commandlet comparison awaits the post-review RF136/RF234 remediation.
- **Observable impact:** The review now records that the five developer previews are historical and queues targeted post-CR19 repairs. No runtime, configuration, or player-visible behavior changed.
- **Networking/authority:** Review bookkeeping only. These files are developer diagnostics, not runtime network state or authority inputs; no gameplay authority or privacy behavior changed.
- **Known limitations / remaining scope:** Scalar PPMs were not pixel-compared against a fresh current commandlet render; no Editor build or runtime session ran. RF136 and RF234 remain open, and CR13 remains incomplete.

## CR13-024 Menu map review

- **Reviewed input:** `Content/Kalmala/Maps/Menu/L_Menu.umap`. Its SHA-256 is `2ac43cac0e7b146981f703267d1093026050cf54d489ce92519643523befa6a2`, matching the CR01 fingerprint and current `HEAD`; the worktree has no map diff.
- **Package inspection:** The 6,592-byte file begins with the Unreal package tag `C1 83 2A 9E`. Readable package names include `L_Menu`, `WorldSettings`, `KalmalaMenuGameMode`, and `PersistentLevel`.
- **Saved-map evidence:** The earlier SM14.1 Editor run created and reloaded this map with `AKalmalaMenuGameMode`, verified zero level actors, and reported zero Map Check errors and warnings. That run confirmed its copied map matched the validated mirror. The current artifact still matches its review-baseline bytes. `Scripts/Create-MenuMap.py` also contains a read-only existing-map branch that checks the GameMode and rejects any actor list.
- **Contract cross-check:** `docs/startmenu.md` SM03 specifies this dedicated empty Home world. Current `DefaultEngine.ini` selects it for packaged startup, keeps `L_Prototype` as editor startup, and retains `KalmalaGameMode` as the global default. The current `DefaultGame.ini` working copy includes the Home map in cook registration; its existing Prototype cook entry was already a user change and was preserved. `AKalmalaMenuGameMode` selects `AKalmalaMenuPlayerController`, clears pawn/spectator/HUD classes, and declines default pawn spawning; the existing menu-context automation asserts those defaults but was not run during this review.
- **Result:** Reconciled the prior isolated CR13-024 report with current main, updated its aggregate and file coverage rows, and confirmed no new actionable finding. CR13 remains open; CR13-025 is next. No backlog checkbox was changed.
- **Checks / limits:** Re-read `AGENTS.md`, the CR13 review instructions, current review ledgers and relevant project brief/design/architecture/roadmap/decision/setup/start-menu context. Compared the current map SHA-256 with CR01 and checked it has no worktree diff. Inspected package header/name strings, the map-authoring validator, menu GameMode/controller defaults, startup/cook configuration, prior SM14.1 Editor validation, and CR13-024 history. No current Unreal Editor load, build, cook, automation, packaged launch, or gameplay session ran.
- **Full verification:** Deferred; this is not milestone-final verification.
- **Observable impact:** Main-checkout review coverage now records the unchanged Home map as reviewed. The map and runtime behavior are unchanged.
- **Networking/authority:** Review bookkeeping only. No runtime or network code changed; the menu context does not spawn a gameplay pawn.
- **Known limitations / remaining scope:** This reconciliation did not perform a fresh Editor reload or packaged startup. SM15 packaged launch and menu/game round-trip acceptance remain outstanding; CR13 remains incomplete.

## RF182 Finding: Correct stale Prototype game-default guidance

- **Source:** CR13-025; `docs/07-development-setup.md:2124`, `docs/startmenu.md` SM03, and `Config/DefaultEngine.ini`.
- **Severity:** P3. This is a developer-guidance mismatch; runtime map selection remains configured.
- **Trigger:** A developer follows the setup guide's statement that `L_Prototype` is both the editor startup and game default.
- **Impact:** The guide can lead developers to expect the prototype world at packaged startup and validate the wrong launch flow.
- **Evidence:** Current `DefaultEngine.ini` sets `EditorStartupMap` to `/Game/Kalmala/Maps/Prototype/L_Prototype` and `GameDefaultMap` to `/Game/Kalmala/Maps/Menu/L_Menu`, while retaining `KalmalaGameMode` as the global default. `docs/startmenu.md` SM03 describes the same editor/packaged split. The current working copy of `DefaultGame.ini` adds both Home and Prototype to `MapsToCook`; cook inclusion does not change `GameDefaultMap`. Solo/host menu travel opens Prototype after launch. RF150 still covers packaged-cook and Home-to-game acceptance, which were not run in this review.
- **Proposed repair:** After CR19 review acceptance, revise the setup sentence to identify Prototype as editor startup only and point packaged startup guidance to the start-menu contract. Do not change runtime or map configuration.
- **Dependencies:** After CR19 acceptance; no game-document edits during CR13. RF182 already exists as an unchecked task in the pre-existing worktree `BACKLOG.md` edits; this review did not add a duplicate task.
- **Focused validation:** Compare the setup guide with `DefaultEngine.ini` and the SM03 contract, then run `git diff --check` after the repair.

## CR13-025 Prototype map review

- **Reviewed input:** `Content/Kalmala/Maps/Prototype/L_Prototype.umap`. SHA-256 `fc01494705ef760139b851cbcbaca2af67f01e9368c689101eba3678ef1a11a6` matches the CR01 fingerprint and current `HEAD`; the worktree has no map diff.
- **Package inspection:** The 30,894-byte package begins with the Unreal package tag `C1 83 2A 9E`. A raw string scan finds the map name, two PlayerStart object names, `KalmalaInteractionTestActor`, M1 fixture tags, the tagged environment sun/atmosphere/sky, and the `KalmalaEnvironmentPostProcess`/`PostProcessVolume` names. This is string-level reference evidence, not a fresh package deserialization or actor-property readback.
- **Map contract:** The M1 roadmap calls for a two-player test map. `SetupM1TestMapCommandlet` creates the two tagged PlayerStarts and an interaction target; `Setup-PrototypeEnvironment.py` ensures the tagged sun, sky, atmosphere and global unbound post-process volume. The later L01f lighting record documents that volume; the earlier L01a “no post-process actor” scan predates L01f. L01h records a UE 5.8 PIE spawn/camera spot-check on this map, but no current Editor load was performed here.
- **Open question, not a confirmed finding:** The M1 interaction target is in the playable Prototype map. Its class is a replicated 50 cm `BlockAll` sphere with no visual component and no editor-only marker, and the setup commandlet places it near the PlayerStarts. The roadmap explicitly calls this an M1 test map, so intent to retain or exclude this fixture from packaged gameplay is not established by current design evidence. No RF task was added without that product intent.
- **Result:** Reconciled the earlier isolated CR13-025 report with current main, updated the aggregate and file coverage rows, and recorded RF182 evidence. No additional actionable map-content finding was confirmed. CR13 remains open; CR13-026 is next. `BACKLOG.md` was not changed by this run.
- **Checks / limits:** Re-read `AGENTS.md`, the project-wide review instructions, relevant progress/review records, and project brief/design/architecture/roadmap/decision/setup/start-menu/lighting context. Confirmed the map hash against CR01/current `HEAD` and no worktree diff. Inspected the package tag and raw names, setup commandlets, M1 interaction actor implementation, current startup/cook settings, RF150/RF182, and prior map/editor evidence. No current Editor load, build, test, cook, automation, packaged launch, or gameplay session ran.
- **Full verification:** Deferred; this is not milestone-final verification.
- **Observable impact:** Main-checkout coverage now records the Prototype map review and RF182 evidence; the map and runtime behavior were not changed.
- **Networking/authority:** Review bookkeeping only. No gameplay or network code changed; the M1 fixture's interaction count remains server-authoritative.
- **Known limitations / remaining scope:** Static package strings do not prove current Unreal deserialization or exact actor properties. No packaged cook or Home-to-game launch ran; RF150 remains open. The packaged intent for the M1 test target remains an open question, and CR13 is incomplete.

<a id="rf235-standing-light-description-and-torch-icon-mismatch"></a>

## RF235 — Standing Light description and torch icon mismatch

- **Source:** CR13-065; `Content/Data/GameCatalogues/items/buildItems/other.json:58-62`; `Content/Kalmala/UI/Source/IconOriginals/StandingLightKit.png`; `Scripts/Data/catalogue-icon-manifest.csv`; `docs/icon-provenance.md#standing-light-icon`.
- **Severity:** P3. This is a player-facing construction description and presentation mismatch; no gameplay or authority defect was found.
- **Trigger:** A player views the Standing Light buildable description alongside its construction icon.
- **Impact:** The description identifies a brazier, but the icon presents a timber torch, so the two sources communicate different fixtures.
- **Evidence:** The registered description is “A freestanding timber brazier that burns Wood for a small cosmetic light.” The current `StandingLightKit` original shows an upright torch with rope-bound burning kindling on a timber tripod. The retained provenance says the artwork was updated to replace the brazier with a wooden torch; its prompt explicitly excludes a bowl or brazier. The icon manifest maps this texture to the `StandingLight` buildable and construction views. The source PNG matches its CR01 and HEAD SHA-256 fingerprint.
- **Proposed repair:** After CR19 acceptance, change the description to call it a freestanding timber torch. Preserve the buildable ID, direct-material recipe, fuel behavior, and construction-only identity.
- **Dependencies:** After CR19 review acceptance; no catalogue or art changes during CR13.
- **Acceptance:** The catalogue description matches the retained torch artwork and prompt while buildable identity, recipe, fuel behavior, and construction-only semantics remain unchanged.
- **Focused validation:** Parse registered catalogue JSON and references, check the `StandingLight` icon alias and direct-material recipe, run the focused catalogue icon and reference validators, and run `git diff --check`. No Unreal build is required.

## RF272 — Pin the approved Dodge Roll contract in regression tests (P3)

- **Source:** CR16-022; `Source/KalmalaGameplay/Private/Tests/KalmalaDodgeRollTest.cpp:14-42`; `Source/KalmalaGameplay/Public/KalmalaCharacter.h:96-100`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1273-1327`; `Scripts/Tests/Combat/EnemyPeers.ps1:133-146`; and `docs/01-game-design.md:93-95`.
- **Affected path:** `Kalmala.Gameplay.Movement.DodgeRollAuthorityAndCost` and the `MosquitoCounterplay` peer scenario.
- **Trigger:** An approved Dodge Roll value changes, or the server action stops applying the expected stamina payment or forward launch while preserving its accepted-request and mosquito-counterplay signals.
- **Impact:** The isolated test supplies `DodgeRollStaminaCost` to the eligibility helper as the expected cost, so it does not pin the approved 25-stamina value. The peer scenario confirms one accepted request, one rejected repeat and the two-second mosquito escape window, but does not assert the stamina delta, forward movement/speed, 0.35-second active duration or one-second cooldown. Those behavior or tuning regressions can evade the current test gates.
- **Evidence:** The game-design contract specifies 25 stamina before active modifiers, a 620 cm/s forward roll lasting 0.35 seconds, and a 1-second cooldown. The runtime constants currently match those values. `KalmalaDodgeRollTest.cpp` passes the configured cost constant in its helper cases and checks only the 2-second repel constant numerically. The peer runner's marker gates request counts and counterplay state; it does not parse roll movement or stamina. No mismatch in current runtime behavior was established.
- **Proposed repair:** Pin the approved numeric contract with independent expected values and add a focused server-action assertion for actual stamina payment, forward launch and roll/cooldown timing. Keep the owner-client peer counterplay scenario as the network-path check.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** Changing an approved cost, speed, duration or cooldown, or omitting the server-side cost/launch effect, causes focused regression coverage to fail; current documented values and behavior pass.
- **Focused validation:** Exercise the focused test with each approved constant changed independently and with cost/launch side effects suppressed; confirm each mutation fails while the current action passes. No test was executed during this review.

## RF273 — Exercise the server-only campfire relight action in the client-role test (P3)

- **Source:** CR16-023; `Source/KalmalaGameplay/Private/Tests/KalmalaHearthStateTest.cpp:42-48`; `Source/KalmalaGameplay/Private/Tests/KalmalaInteractionAuthorityTest.cpp:139-141,237`; `Source/KalmalaGameplay/Private/KalmalaCampfire.cpp:125-137`; and `docs/02-technical-architecture.md:569`.
- **Affected path:** `AKalmalaCampfire::TryLightFromServer`.
- **Trigger:** A regression removes or weakens the campfire actor's authority guard in the explicit relight action while the automatic state-advance guard and static `IsLightingAllowed` policy continue to pass.
- **Impact:** The test labeled “Client cannot reignite” does not call `TryLightFromServer`; it only verifies simulated-proxy rejection from `AdvanceFromServer`. A change to the explicit actor action's authority check can therefore evade this client-role regression test. No current runtime authority flaw was found.
- **Evidence:** `KalmalaCampfire::TryLightFromServer` currently checks campfire authority, character validity/authority, interaction permission, extinguished state, positive finite fuel, and dry-fuel policy before setting the hearth Lit. The hearth test switches the campfire to `ROLE_SimulatedProxy`, invokes `AdvanceFromServer`, then restores authority. The interaction authority test invokes the pure `IsLightingAllowed(false, 0)` policy, which does not cover actor authority or state mutation. Architecture states that only a validated server interaction may light dry fuel.
- **Proposed repair:** In the simulated-proxy case, prepare a valid character and an eligible, paid, dry, extinguished campfire, call `TryLightFromServer`, and assert rejection with hearth state and fuel unchanged. Keep the existing authority-role relight case as the positive control.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The client-role actor rejects the explicit relight action without changing state or fuel, while the otherwise equivalent authority-role case can relight the campfire.
- **Focused validation:** Run the focused hearth/interaction automation with the campfire role switched to `ROLE_SimulatedProxy` for the explicit action and confirm it fails if that authority guard is removed; verify the authority positive control still passes. No test was executed during this review.

## RF274 — Pin the approved M9 exploration candidate rewards in the catalogue test (P3)

- **Source:** CR16-024; `Source/KalmalaGameplay/Private/Tests/KalmalaM9ExplorationRewardCatalogueTest.cpp:25-68`; `Source/KalmalaGameplay/Private/KalmalaM9ExplorationRewardCatalogue.cpp:44-54`; `docs/02-technical-architecture.md:1283-1284`; and `docs/07-development-setup.md:4364-4368`.
- **Affected path:** The canonical M9 land-discovery definition table and `Kalmala.Gameplay.M9.ExplorationRewardCatalogue`.
- **Trigger:** A candidate ID, presentation identity, reward item or quantity changes while the definitions remain internally valid and use distinct existing materials.
- **Impact:** The test gets its expectations from `GetDefinitions()` and calls `IsValidDefinition()`, which finds the same definition in that same table. It checks uniqueness and the generic one-to-two reward bound, so the approved Rillstone/Leeward candidate-to-reward mapping can drift without failing the catalogue test. No current runtime mismatch was found.
- **Evidence:** The architecture table specifies `lakes-rillworn-marker` / Three-Run Rillstone → Stone x2 and `mountains-leeward-grain` / Leeward Grain → Wood x2. Current production definitions match. The test asserts exactly two definitions, self-consistency, valid catalogue grants and distinct candidate/presentation/reward IDs, but does not compare those fields with independent expected values; setup documentation describes the test as checking canonical rewards.
- **Proposed repair:** Add independent expected rows for the two candidate IDs, presentation IDs, reward item IDs and quantities, then compare the catalogue definitions exactly while retaining the placement, bounds and sparse-identity checks.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** Missing, extra, renamed or mismatched candidate/presentation/reward IDs and changed reward quantities fail the focused catalogue regression; the current two approved rows pass.
- **Focused validation:** Mutate each canonical candidate or presentation ID, swap the two reward item IDs, and change either quantity independently; confirm each mutation fails while the current architecture-aligned definitions pass. No test was executed during this review.

## RF275 — Pin the documented M9 source-loot hash input in regression tests (P3)

- **Source:** CR16-024; `Source/KalmalaGameplay/Private/Tests/KalmalaM9SourceLootContractTest.cpp:83-124`; `Source/KalmalaGameplay/Private/KalmalaM9SourceLootContract.cpp:37-53`; and `docs/02-technical-architecture.md:1012-1017`.
- **Affected path:** `FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity` and the canonical M9 source bonus-yield calculation.
- **Trigger:** The loot key's domain tag, world seed, generator revision, source ID or population spawn ID is omitted, reordered or formatted incorrectly while the hash function and repeated-query behavior remain deterministic.
- **Impact:** The test checks a standard FNV-1a string vector, repeats identical queries, bounds results to one or two, and searches world seeds for both outcomes. It does not pin any expected hash or yield for a complete documented `m9-loot-v1` input, so an incorrect key composition can pass those checks. No current runtime mismatch was found.
- **Evidence:** The architecture specifies the exact key `m9-loot-v1|<world-seed>|<generator-revision>|<source-id>|<population-spawn-id>` and awards the bonus only when its FNV-1a 64-bit hash is divisible by four. Production code currently constructs that complete key and applies the documented divisor. The test's `HashAscii("hello")` vector covers the primitive but not composition.
- **Proposed repair:** Add independent known hash/yield vectors for complete documented inputs, including different seeds and source/population identities, so the domain tag, field order and every required field are covered.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The current complete key produces the independently specified hash and one/two-unit result; changing or omitting any key component or changing the bonus divisor fails the focused regression.
- **Focused validation:** Run the source-loot contract test with the current vectors, then confirm mutations to the domain tag, generator revision, source ID, population spawn ID, field order or bonus divisor each fail. No test was executed during this review.

## RF276 — Compare full tool-progression outputs after rejection (P3)

- **Source:** CR16-025; `Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolStationProgressionTest.cpp:206-220`; `Source/KalmalaGameplay/Private/KalmalaToolProgressionContract.cpp:372-488`; and `docs/07-development-setup.md:4603-4605`.
- **Affected path:** Rejected calls to `FKalmalaToolProgressionContract::BuildServerUpgrade` and its `OutTools` / `OutInventory` candidate outputs.
- **Trigger:** A failure path mutates an existing tool or inventory stack while preserving the output array length.
- **Impact:** The focused progression regression can pass despite same-length changes to an item ID, level, durability, instance GUID, stack quantity, or stack identity after a rejected request.
- **Evidence:** `RejectWithoutMutation` copies the pre-call output arrays, but its two assertions compare only `Num()`. Production currently builds local candidates and assigns `OutTools` and `OutInventory` only after all validation and payment checks succeed, so no runtime mutation was established. The setup guide explicitly says this automation covers unchanged candidates after rejection.
- **Proposed repair:** Compare every field of both output arrays with the pre-call copies after each rejected progression request; keep the existing rejection reasons and accepted-path assertions.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** Each rejection case detects any change to tool identity, level, durability, GUID, inventory item, quantity, or unit GUID even when array lengths remain unchanged; successful acquisitions and upgrades still pass.
- **Focused validation:** Run `Kalmala.Gameplay.M9.ToolStationProgression` after an Editor build, then verify a same-length candidate mutation makes the regression fail. No test was executed during this review.

## RF277 — Test Workbench exclusion from camp-sphere suppression (P3)

- **Source:** CR16-027; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSphericalSuppressionTest.cpp:123-128`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6381-6413`; and `docs/monster-spawning.md:380-392`.
- **Affected path:** S24 camp-source classification in `AKalmalaGameMode::DoesPopulationPointOverlapCampSphere`.
- **Trigger:** A regression adds WorkbenchKit as an S24 spherical source, or otherwise conflates the S23 station cylinder with the separate S24 sphere, while the current Forge-only negative case continues to pass.
- **Impact:** Wildlife could be rejected in the extra 12–18 m vertical band around a Workbench even though S23 defines only an 18 m horizontal by 12 m vertical cylinder and S24 excludes unlisted stations. Current production correctly admits only Camp Bed, eligible Standing Light, and lit fuelled Campfire sources to the sphere query; no runtime defect was found.
- **Evidence:** The S24 test creates an initialized `ForgeKit` actor and calls the sphere query, but the assertion text says “Workbench and Forge stations” without creating a Workbench fixture. The documented S24 contract excludes unlisted stations; S23 handles Workbench/Forge separately. The production helper's construction loop currently checks only `CampBedKit` and eligible `StandingLightKit`, then checks Campfire actors in a separate loop.
- **Proposed repair:** Add a distinct initialized `WorkbenchKit` fixture at a point outside all existing camp sources and assert that the direct S24 sphere query returns false. Keep the Forge negative and current eligible-source positive controls; rename or split the existing assertion so its diagnostic matches the fixture.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** `Kalmala.Gameplay.Population.SphericalCampSuppression` independently proves WorkbenchKit and ForgeKit are excluded from S24 while the existing Camp Bed, eligible Standing Light, and eligible Campfire positive controls continue to pass.
- **Focused validation:** Run `Kalmala.Gameplay.Population.SphericalCampSuppression` after an Editor build. No test was executed during this review.

## RF278 — Reconcile current storage architecture contracts (P2)

- **Source:** CR16-028; `docs/02-technical-architecture.md:664,1640-1646`; `Source/KalmalaGameplay/Private/Tests/KalmalaReinforcedChestTest.cpp:10-62`; and `Source/KalmalaGameplay/Private/Tests/KalmalaStorageTest.cpp:10-151`.
- **Affected path:** Current implementation guidance for supported storage buildables, capacities, and saved storage schema.
- **Trigger:** A developer follows the unqualified paragraph that says the normal Chest is the sole storage construction and current storage schema is 2.
- **Impact:** The architecture source of truth directs new work toward a retired single-chest/schema-2 contract even though the current supported design includes Reinforced Chest and storage schema 3.
- **Evidence:** The paragraph at line 664 states “sole storage construction” and “Current storage schema 2.” The later current contract at lines 1640-1646 specifies sixteen stacks for standard Chest, twenty-four for Reinforced Chest, and storage schema 3. The assigned Reinforced Chest and Storage tests exercise that newer contract. No runtime defect was found.
- **Proposed repair:** Replace the stale current-tense storage claims with the standard and Reinforced Chest capacities and schema-3 identity-validation contract. Retain the still-current food-processing, hearth-fuel, and construction-save facts in that paragraph.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The architecture describes both supported storage constructions and current schema 3 consistently, with no unqualified current-tense claim that the normal Chest is the sole storage construction or that storage schema 2 is current.
- **Focused validation:** After the documentation repair, compare its storage summary with the current schema and capacity assertions in the assigned tests and search for contradictory current-tense storage claims. No documentation was changed during this review.

## RF279 — Pin resource source families independently in the gate test (P3)

- **Source:** CR16-028; `Source/KalmalaGameplay/Private/Tests/KalmalaResourceFamilyGateTest.cpp:10-42`; `Source/KalmalaGameplay/Private/Tests/KalmalaToolLifecycleContractTest.cpp:29-116`; `Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp:358-412`; and `docs/02-technical-architecture.md:3270`.
- **Affected path:** Mapping of each server resource source ID to Woodcutting, Mining, or Gathering before tool-family validation.
- **Trigger:** A source ID is assigned to the wrong action while the family gate continues to use the returned action consistently.
- **Impact:** A tree or ore source could become usable by the wrong tool family while the current family test still passes its expected-family comparison.
- **Evidence:** For each source, `KalmalaResourceFamilyGateTest` sets `bFamilyMatches` from `Definition->Action == Selection.Action`, where `Selection.Action` is returned by production `BuildServerSelection`, then uses that value as the expected outcome. The test therefore verifies agreement with production's classification, not whether all twelve source IDs have their documented action. Other assigned selection assertions cover only part of the source set. The architecture states axes apply to Woodcutting resources, picks to Mining resources, and knives to Gathering sources. Current production mapping matches that contract; no runtime defect was found.
- **Proposed repair:** Add an independent expected-action table for the twelve source IDs and assert each `BuildServerSelection` result against it before retaining the existing six-tool tier/family matrix and altered-action rejection checks.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** Every listed source has an independently asserted action (and corresponding skill family), and the existing cross-family rejection matrix still covers all six axe/pick identities.
- **Focused validation:** Run `Kalmala.Gameplay.Harvest.ResourceFamilyGates` and `Kalmala.Gameplay.Tools.LifecycleContract` after an Editor build. No test was executed during this review.

## RF280 — Verify rain pauses Standing Light fuel at the actor boundary (P3)

- **Source:** CR16-028; `Source/KalmalaGameplay/Private/Tests/KalmalaStandingLightTest.cpp:42-55`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSphericalSuppressionTest.cpp:76-87`; `Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp:143-155`; and `docs/05-decision-log.md:7`.
- **Affected path:** Server Standing Light tick, rain exposure check, and fuel persistence.
- **Trigger:** The actor tick regresses to advance or persist fuel while the light is exposed to rain.
- **Impact:** A rain-exposed light can consume its stored Wood despite the approved rain-pause behavior, shortening its usable light time.
- **Evidence:** The decision log specifies a cosmetic light with Wood fuel and rain pause. Production currently returns from `Tick` before advancing or persisting fuel when `bRainBlocked` is true. `StandingLightTest` directly exercises the state helper's active-time arithmetic, while the spherical-suppression test checks only that a rain-exposed light is not an eligible flame source; neither verifies actor fuel remains unchanged through a rainy tick and resumes afterward. No runtime defect was found.
- **Proposed repair:** Add an authoritative actor-level regression with an initialized fuelled Standing Light and controlled weather, asserting exposed-rain ticks leave live and persisted fuel unchanged and that a subsequent dry or roofed tick consumes the expected active time.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The focused automation fails if rain-exposed ticks change live or saved fuel, and passes when dry/roofed active ticks consume the expected fuel while the rain-blocked presentation remains unlit.
- **Focused validation:** Run the Standing Light contract and focused rain integration automation after an Editor build. No test was executed during this review.

## RF281 — Require wildlife reward claims to survive reload assertions (P3)

- **Source:** CR16-029 and CR16-030; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBadgerTest.cpp:132-149`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeDeathAdderTest.cpp:213-227`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMoonBearTest.cpp:104-124`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeRatTest.cpp:81-96`; and `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp:249-282`.
- **Affected path:** Badger, Death Adder, Moon Bear, Rat and Peat Husk pending reward claims after population-save serialization and reload.
- **Trigger:** A valid reload loses one or more pending reward rows while returning a non-null population save.
- **Impact:** The tests skip the post-reload identity, owner and idempotency checks when the claim list is empty, so a reward-persistence regression can pass without reporting the lost claim.
- **Evidence:** All five tests check only that the reloaded pointer is non-null before guarding subsequent claim checks with `!Reloaded->GetPendingWildlifeRewards().IsEmpty()`. Their pre-save queues contain one claim for Badger, Death Adder, Moon Bear and Rat, and two for Peat Husk. Current production behavior was not shown to lose claims; the finding is a false-pass gap in the regression tests.
- **Proposed repair:** Assert the exact reloaded queue size (one for Badger, Death Adder, Moon Bear and Rat; two for Peat Husk) before indexing or searching rows. Retain the defeated identity, claim ID, credited owner and applicable same-owner retry / other-owner rejection assertions.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** All five focused tests fail if a valid reload drops or duplicates a pending row and verify the expected identity, owner binding and idempotent retry when it survives.
- **Focused validation:** Run the five species contract automations after an Editor build, then verify that empty and undersized reloaded queues fail their exact-count assertions. No automation was run during this review.

## RF282 — Pin the documented tree-fall duration independently (P3)

- **Source:** CR16-029; `Source/KalmalaGameplay/Private/Tests/KalmalaTreeFellingTest.cpp:23-24,44-45`; `Source/KalmalaGameplay/Public/KalmalaHarvestNode.h:60`; `docs/01-game-design.md:182`; and `docs/02-technical-architecture.md:3264`.
- **Affected path:** The server-timed Falling phase and late-peer progress reconstruction for felled trees.
- **Trigger:** `TreeFallSeconds` changes while the test and implementation continue to agree on half of that changed duration.
- **Impact:** The automation would pass if the Falling phase drifted from the documented 1.2 seconds, allowing the gameplay timing contract and both design documents to diverge without a regression failure.
- **Evidence:** Both half-progress checks calculate elapsed time using `AKalmalaHarvestNode::TreeFallSeconds * 0.5`; the only exact 1.2-second value is the production constant itself. The design and architecture specify a 1.2-second Falling phase. Current production matches that duration; no runtime defect was found.
- **Proposed repair:** Assert the duration constant against the independently documented 1.2-second value and exercise half progress at 0.6 seconds, so the test oracle does not derive the expected timing from production.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The tree-felling automation fails if the duration changes from 1.2 seconds, while still verifying the expected midpoint pose, clamping and phase transitions.
- **Focused validation:** Run `Kalmala.Gameplay.Harvest.TreeFelling` after an Editor build and verify that changing the production duration alone makes its independent duration assertion fail. No test was executed during this review.

## RF283 — Check every authored wildlife model in the winding regression (P3)

- **Source:** CR16-029; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:112-205`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:4089,4199,4314,4364,4496,4558,4660,4899`; and `docs/07-development-setup.md:421-425`.
- **Affected path:** Procedural wildlife model triangle winding, vertex normals and degenerate-face validation.
- **Trigger:** A species-specific model branch changes index order, normals or mesh construction while the existing checked species continue to pass.
- **Impact:** One-sided materials can cull an affected model's exterior or expose its interior, and invalid or degenerate geometry can ship without the actual-mesh regression detecting it.
- **Evidence:** `SurfaceWinding` applies its checks to the player and Boar, Deer, Mireling, Moon Bear, Viper, Giant Snake, Death Adder, Peat Husk and Wolf. `BuildArchetypePresentation` also has authored model branches for Rat, Badger, Polar Bear, Horned Goat, Thunder Sentinel, Reef Maw, Lynx and Mosquito Swarm, none of which are constructed by that regression. The setup guide likewise describes only the original three creature models. No current model defect was established.
- **Proposed repair:** Add actual presentation fixtures for Rat, Badger, Polar Bear, Horned Goat, Thunder Sentinel, Reef Maw, Lynx and Mosquito Swarm, then apply the existing section, winding, finite-normal and non-degenerate-face checks to each.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** `Kalmala.Gameplay.Presentation.SurfaceWinding` creates and checks all eight currently omitted authored models alongside its existing player and wildlife fixtures.
- **Focused validation:** Run `Kalmala.Gameplay.Presentation.SurfaceWinding` after an Editor build and confirm each model fixture produces a procedural section that passes the existing checks. No automation was run during this review.

## RF284 — Verify Rat claim cleanup on the reloaded save (P3)

- **Source:** CR16-030; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeRatTest.cpp:81-100`.
- **Affected path:** Rat pending reward cleanup following population-save serialization and reload.
- **Trigger:** Delivery removes the pending row from the original in-memory population object while a reloaded save still retains the row.
- **Impact:** The test reports successful delivery cleanup without establishing that the durable/reloaded escrow row was cleared.
- **Evidence:** The test reloads the serialized save into `Reloaded` and inspects its defeated identity and claim fields at lines 87-96. It then calls `Population->RemovePendingWildlifeReward` and checks `Population` at lines 97-100, so the cleanup assertions target the pre-serialization object. No production cleanup defect was established.
- **Proposed repair:** Model delivery on `Reloaded`, remove the expected claim there, then assert that its queue is empty; retain an independent assertion for the original object's state only if needed to test that contract.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The Rat save test fails if delivery clears only the original object while leaving the reloaded claim pending, and passes when cleanup clears the reloaded escrow row.
- **Focused validation:** Run the Rat wildlife reward persistence automation and verify it fails when reloaded-claim removal is omitted. No automation was run during this review.

## RF285 — Pin wildlife optional-drop odds at the roll seam (P3)

- **Source:** CR16-030; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp:162-169`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeWolfTest.cpp:190-236`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeThunderSentinelTest.cpp:109-156`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeReefMawTest.cpp:102-148`; and the Lynx reward test from CR16-029 at `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeLynxTest.cpp:101-151`. Production roll sites are `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8809,8853,8897,8991,9035`; documented odds are in `docs/creature-catalogues.md:127-149,158` and `docs/biome-enemies.md:171-179`.
- **Affected path:** Server-side optional Peat Heart, Wolf Fang, Lynx Pelt, Thunder Core and Reef Tooth outcomes persisted in owner-bound wildlife reward claims.
- **Trigger:** A production random-roll threshold changes or the roll outcome stops being copied into the durable claim while the queue helper continues to accept manually supplied `true` and `false` outcomes.
- **Impact:** The documented optional-drop odds or persisted outcome can regress without the claim-construction tests detecting it.
- **Evidence:** Current production uses `FMath::FRand()` thresholds of 25% for Peat Heart, Wolf Fang and Thunder Core and 50% for Lynx Pelt and Reef Tooth. Assigned wildlife tests exercise claim serialization and manually supply both outcomes to queue/model helpers; they do not pin the production thresholds or deterministically test their boundary and propagation into the claim. Existing RF18/RF46 describe callback integration gaps and were not duplicated; this finding is limited to odds and stored outcome coverage. No current runtime defect was established.
- **Proposed repair:** Extract or expose a deterministic probability predicate/roll seam that can be tested with controlled samples. Pin each documented threshold, exercise samples on either side of the boundary, and verify the resulting optional item flag is retained through the durable owner claim.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** Focused wildlife reward tests fail if any of the five documented thresholds changes or if a selected optional outcome is lost before claim persistence; tests cover both roll outcomes deterministically without flaky randomness.
- **Focused validation:** Run Peat Husk, Wolf, Lynx, Thunder Sentinel and Reef Maw reward contract automations after an Editor build, including deterministic below/at/above-threshold samples. No automation was run during this review.

## RF286 — Pin the Wolf pack formation radius independently (P3)

- **Source:** CR16-030; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeWolfTest.cpp:105-115`; `Source/KalmalaGameplay/Public/KalmalaWolfPackContract.h:16,23-31`; and `docs/creature-catalogues.md:140`.
- **Affected path:** Seed-rotated three-member Wolf pack formation offsets.
- **Trigger:** `FormationRadiusCentimetres` changes while both `GetMemberOffset` and the test continue to use the changed constant.
- **Impact:** The test can pass when pack spacing drifts from the documented 240 cm radius.
- **Evidence:** The test compares each offset size directly with `FKalmalaWolfPackContract::FormationRadiusCentimetres`; the production helper multiplies its unit vector by that same constant. The creature catalogue independently specifies a 240 cm radius. No current production defect was found.
- **Proposed repair:** Add an independent assertion that the formation-radius constant is 240 cm and retain the per-member offset geometry checks.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The Wolf pack test fails if the documented radius changes from 240 cm even when helper output and the production constant change together.
- **Focused validation:** Run the Wolf pack contract automation and verify that changing the production radius alone fails the independent 240 cm assertion. No automation was run during this review.

## RF287 — Pin mosquito fire-repelling radii independently (P3)

- **Source:** CR16-030; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMosquitoTest.cpp:84-98`; `Source/KalmalaGameplay/Public/KalmalaWildlifeSpawn.h:183-184`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3395-3417`; `docs/05-decision-log.md:38`; and `docs/biome-enemies.md:561-562,597-598`.
- **Affected path:** Server mosquito repulsion around eligible torch and campfire flames.
- **Trigger:** A production avoidance-radius constant drifts while the test supplies that same current constant to the predicate.
- **Impact:** The test can continue to pass although the approved 300 cm torch or 500 cm campfire exclusion radius has changed.
- **Evidence:** The test checks repulsion at 300 cm and 500 cm but passes `MosquitoTorchAvoidanceCentimetres` and `MosquitoCampfireAvoidanceCentimetres` as the predicate's radius arguments. Production uses these constants for the corresponding active flame sources. Current constants are 300 cm and 500 cm, matching the decision and biome contracts. No runtime defect was established.
- **Proposed repair:** Independently assert the constants against 300 cm and 500 cm, and check that active flames just outside each radius do not repel; retain authority and inactive-flame controls.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR16.
- **Acceptance:** The Mosquito test fails when either approved radius changes even if the helper and the passed test argument remain aligned, and pins the just-outside boundary for both source types.
- **Focused validation:** Run the Mosquito wildlife contract automation and verify that changing either production radius alone fails its independent constant assertion. No automation was run during this review.

### CR17-001 — Gathered resources to camp save/reload integration

- **Reviewed revision:** `codex/cr16-chat` at `fa71290f`.
- **Scope:** Server-authoritative generated harvest, crafting, direct-material Campfire placement, world save owners, and same-world restore.
- **Evidence:** `UKalmalaCraftingComponent::PlaceConstructionFromServer` dispatches `CampfireKit` to `PlaceFromServer` (`Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:504-528`). `PlaceFromServer` validates the recipe plus raw ignition fuel, spawns `AKalmalaCampfire`, commits payment and initializes the actor, but does not write a world record (`.../KalmalaCraftingComponent.cpp:457-501`). `AKalmalaGameMode::TrySaveWorldFromLocalHost` writes population, construction, storage and player records (`Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9588-9614`); `RestorePersistedConstruction` restores each construction record as `AKalmalaConstructionActor` and never creates a Campfire actor (`.../KalmalaGameMode.cpp:935-956`). `AKalmalaCampfire` owns replicated fuel and hearth state (`Source/KalmalaGameplay/Public/KalmalaCampfire.h:26,46-55,76-88`). The roadmap says existing saved Campfires must restore correctly (`docs/04-roadmap.md:648-661`). The combined restart runner checks construction IDs and storage after restart but has no Campfire restoration assertion (`Scripts/Tests/Survival/PersistedCampRestart.ps1:83-87`). Its lower-level CampfireKit save-model round trip does not cover the runtime placement or restore path (`Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveGameTest.cpp:75-80`).
- **Result:** Confirmed RF288. Existing RF16, RF23, RF273 and RF183 were reviewed for overlap; they cover other depletion, save-return test, relight-authority, and cross-world-runner concerns. No duplicate Campfire persistence finding was present.
- **Limits:** Static inspection only; no test, build, runtime save/reload or injected write failure was run. The roadmap does not define whether remaining fuel and lit/smouldering state must persist, so no such behavior is assumed here.

<a id="rf298--apply-text-scale-to-the-accessibility-feedback-overlay-p2"></a>

## RF298 — Apply text scale to the accessibility feedback overlay (P2)

- **Source:** CR12-002; `Source/KalmalaUI/Private/KalmalaAccessibilityFeedbackSubsystem.cpp:31-48`; `docs/02-technical-architecture.md:2297-2298,2627-2629`.
- **Severity:** P2. A local accessibility preference is ignored by the feedback overlay.
- **Trigger:** A player uses nearby-hearth or construction feedback while the selected text-scale preference is not 100%.
- **Impact:** This short-lived feedback remains fixed at 16 points and may be too small or too large for the player's selected scale.
- **Evidence:** `NativeOnInitialized` assigns a fixed 16-point font. `SetFeedbackText` updates text and contrast but does not call the shared text-scale helper or read the configured text-scale percentage. The architecture requires local text-scale settings to affect presentation.
- **Proposed repair:** Apply the shared local text-scale setting to the feedback text and preserve safe wrapping and bounds at the supported presets.
- **Dependencies:** After CR19 review acceptance; no implementation during CR12 review.
- **Acceptance:** The 50%, 75%, 100%, 125%, and 150% presets scale the feedback text, and the overlay remains readable without clipping in supported viewports.
- **Focused validation:** After an Editor build, exercise all five presets and check wrapping and clipping in the relevant viewport sizes.

<a id="rf299--keep-ui-paths-behind-world-facing-interfaces-p3"></a>

## RF299 — Keep UI paths behind world-facing interfaces (P3)

- **Source:** CR12-002; `docs/02-technical-architecture.md:559`; `Source/KalmalaUI/Private/KalmalaHomeSubsystem.cpp:623-624,806-814`; `Source/KalmalaUI/Private/KalmalaAmbientAudioSubsystem.cpp:180,1068-1090`; `Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp:294-456`.
- **Severity:** P3. Several UI flows depend directly on concrete world classes and GameMode APIs, contrary to the documented module boundary.
- **Trigger:** Local ambience updates, interaction prompts, or Home launch/failure cleanup query or call world actors/GameMode from the UI module.
- **Impact:** UI behavior is coupled to concrete world implementations and is harder to test or evolve behind the documented interface.
- **Evidence:** Ambient-audio and Home paths cast or call concrete world-facing types from the UI module; crafting UI code also obtains a GameMode and invokes gameplay-facing functions. No player-visible failure or authority bypass was confirmed. Home launch remains server-validated.
- **Proposed repair:** Route world snapshots and requests through components, interfaces, gameplay messages, or subsystem APIs while retaining server-side validation and owner-visible state.
- **Dependencies:** After CR19 review acceptance; no implementation during CR12 review.
- **Acceptance:** The affected UI flows use documented world-facing seams, and server validation and owner-only state behavior remain unchanged.
- **Focused validation:** Add a source-boundary check and exercise prompt, ambience, and Home launch flows after an Editor build.

<a id="rf300--restore-valid-list-markers-in-review-records-p3"></a>

## RF300 — Restore valid list markers in review records (P3)

- **Source:** CR16 master handoff review; `BACKLOG.md` and `PROGRESS.md` in the current master working tree.
- **Severity:** P3. Several review-record list markers are malformed.
- **Trigger:** A reader opens the affected backlog tasks or CR09 progress entry in a Markdown renderer.
- **Impact:** RF160 and RF258 do not render as independent backlog tasks; RF162 is joined to RF161, and RF258 is joined to RF257's focused-validation text. Seven progress bullets also render as ordinary text. The intended task/report structure is not reliably visible.
- **Evidence:** `BACKLOG.md` prefixes RF160 with a literal `+`, places the RF162 task heading on the same line as RF161's final sentence, and places RF258 on the same line as RF257's focused-validation sentence. `PROGRESS.md` has seven consecutive CR09 report lines prefixed with literal `+` characters.
- **Proposed repair:** Remove the stray plus characters, put RF160, RF162, and RF258 on their own list lines, restore the seven progress list markers, and preserve the existing text.
- **Dependencies:** After CR19 review acceptance; no gameplay or runtime repair is involved.
- **Acceptance:** RF160, RF162, and RF258 render as separate backlog tasks, and the seven affected progress entries render as independent bullets with their text intact.
- **Focused validation:** Inspect the exact diff, render or parse the Markdown, and run `git diff --check`. No runtime test applies.

## RF01 — Prune stale character rain timers (P3)

- **Source:** CR03-001b-02; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:435` and `KalmalaGameMode.h:610`.
- **Affected path:** the per-character rain-timer map and its Wet accumulation path.
- **Trigger:** player-pawn replacement, destruction, or repeated reconnects leave expired weak keys behind.
- **Impact:** production retains stale timer entries until world teardown, so repeated pawn churn grows the map unnecessarily.
- **Evidence:** the rain timer map is keyed by weak character pointers and is pruned only by test-fixture cleanup; the production update path has no stale-key removal.
- **Proposed repair:** prune expired character keys during production rain-timer maintenance while preserving accumulation for live characters.
- **Dependencies:** After CR19 review acceptance; no gameplay code changed during review.
- **Acceptance:** repeated destroyed-character and disconnect cycles do not grow stale timer entries, and live-character Wet timing remains unchanged.
- **Focused validation:** exercise repeated pawn replacement/destruction and disconnect cleanup, then compare live Wet accumulation timing with the existing behavior.

## RF63 — Align campfire warmth coverage with normalized heat (P3)

- **Source:** CR07-003; Source/KalmalaGameplay/Public/KalmalaCampfireWeatherResponse.h:35-46; Source/KalmalaGameplay/Private/Tests/KalmalaInteractionAuthorityTest.cpp:143-149; Source/KalmalaGameplay/Private/KalmalaCampfire.cpp:189-192; docs/01-game-design.md:159; docs/02-technical-architecture.md:719.
- **Affected path:** The weather-response helper and ServerWeatherResponse automation compared with the server-published EffectiveWarmth for a lit campfire.
- **Trigger:** A maintainer relies on the current named automation as a regression check for production warmth. It calls CalculateEffectiveWarmth directly and asserts rain, wind, and damp fuel reduce its output; no production caller uses that helper.
- **Impact:** The test can remain green while the live fire heat contract changes or regresses, and its assertion describes a superseded behavior. No current production defect was established: the server sets EffectiveWarmth to exactly 1.0 for Lit and 0 otherwise, matching the current design and architecture.
- **Evidence:** CalculateEffectiveWarmth is declared in the public weather-response header and referenced only by the two assertions in KalmalaInteractionAuthorityTest. AKalmalaCampfire::AdvanceFromServer sets EffectiveWarmth from IsLit, and the architecture says heat is exactly 1 for Lit and 0 otherwise while exposed rain transitions an active fire to zero-heat Smouldering. The helper-level test does not instantiate the fire or inspect its server-published state.
- **Proposed repair:** Remove the unused legacy warmth helper and replace its assertions with focused coverage of the live Lit/Smouldering heat contract across rain and accepted roof transitions. Keep fuel wetting and fire-state transition checks separate.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and authorizes no repair during CR07.
- **Acceptance:** Focused automation asserts live server-derived heat is 1.0 when Lit and zero when Smouldering or Extinguished, including the exposed-rain and roof-protected transitions; no test continues to claim that wind or rain attenuates heat while a fire remains Lit.
- **Focused validation:** Build the Editor target and run Kalmala.Gameplay.Campfire.ServerWeatherResponse plus the relevant campfire exposure scenario. No runtime check was run during this review.

## RF64 — Retry reusable support-scroll grants after inventory capacity rejection (P3)

- **Source:** CR07-003; Source/KalmalaGameplay/Private/KalmalaSupportMagicComponent.cpp:41-58; Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:426-432; Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-289; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9443-9445,9506-9514,10078-10080,10337-10340; Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:341-358; docs/02-technical-architecture.md:638.
- **Affected path:** Reusable support scroll items granted for a learned discovery or Mireling boss entitlement, and restored from the authenticated player's saved learned-effect entitlement.
- **Trigger:** The owner claims a scroll while the 40-slot inventory has no room for the item. BuildGrant returns false at pack output capacity, but LearnEffectFromServer ignores the grant result, returns success, and the discovery handlers publish ScrollFound after persisting the learned entitlement. If a restore attempt also meets a full pack, later cached GetPlayerDiscoverySave calls return before the entitlement restoration loop, so freeing inventory space does not retry the grant during that session.
- **Impact:** The player retains the server-persisted entitlement but has no reusable scroll in a numbered hotbar cell, so the learned effect cannot be activated through normal gameplay until another save-load/restore attempt succeeds. Owner feedback can say the scroll was found while the required inventory item is absent. This is a delivery/retry gap; the entitlement and activation rules remain server-authoritative.
- **Evidence:** TryGrantFromServer returns the result of BuildGrant and stack commit; BuildGrant reports Pack output capacity reached when AddQuantityToStacks cannot fit the stack. LearnEffectFromServer marks the effect learned, ignores TryGrantFromServer's boolean, then returns true. ClaimMirelingBossScroll and the world-discovery claim publish ScrollFound after LearnEffectFromServer without checking inventory delivery. GetPlayerDiscoverySave restores learned effects only after the cache-miss/load path; its matching cached-save branch returns immediately. ServerUseHotbarSlot resolves support activation from the current inventory slot's scroll item ID. The architecture requires reconstructing reusable scrolls from the existing entitlement when inventory initializes. RF27 concerns unavailable discovery-save lookup, while RF41 concerns remote authority/privacy test coverage; neither covers successful entitlement persistence followed by capacity-rejected scroll delivery.
- **Proposed repair:** Preserve the saved entitlement while keeping a failed scroll grant retryable when pack capacity becomes available, or expose an owner-visible pending delivery that retries idempotently. Report scroll delivery as complete only when the ordinary scroll stack exists; do not duplicate the stack across retries or reconnects and do not add a save field.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and authorizes no repair during CR07.
- **Acceptance:** With a full pack, claiming or restoring a learned scroll preserves the entitlement and records a retryable pending delivery without claiming the item is present. After capacity is freed, exactly one reusable scroll appears and activates through its hotbar slot in the same session; retries and reconnect restoration never duplicate it. Save failure still fails closed, and all decisions remain server-owned.
- **Focused validation:** Add an isolated server regression that fills the pack, exercises discovery and Mireling scroll entitlement restoration, frees capacity, and verifies one usable scroll plus idempotent reconnect; run the focused support and inventory automation after an Editor build. No test was run during this review.

## RF65 — Bound owner-only status timer replication work (P3)

- **Source:** CR08-001; cross-reference for CR17 (reuse this finding; do not duplicate); Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:432; Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:99-103,343-420,457-489.
- **Affected path:** Active survival-status timers on player pawns during normal server simulation and replication to each owning client.
- **Trigger:** GameMode advances each player pawn's status component every server tick. AdvanceFromServer calls ForceNetUpdate unconditionally, including when the array is empty; when timers are active, it decrements RemainingSeconds every server tick. Statuses is owner-only.
- **Impact:** Potential avoidable per-pawn owner replication scheduling and status timer traffic at server-tick cadence. No net-profile measurement was run, so CPU and bandwidth impact remain unquantified. No authority bypass was identified.
- **Evidence:** AKalmalaGameMode calls AdvanceFromServer(DeltaSeconds) for every player pawn in its server simulation loop. The status component registers Statuses with COND_OwnerOnly, subtracts SafeDelta from active status timers in Advance, and calls GetOwner()->ForceNetUpdate() unconditionally at the end of AdvanceFromServer. Status effects and expiration remain server-calculated.
- **Proposed repair:** Avoid forcing an owner replication update on every server tick when status state is unchanged. Use an explicit bounded timer replication cadence or client-side remaining-time reconstruction from server-authored timestamps, while preserving prompt status transitions and server-authoritative effect timing.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during review.
- **Acceptance:** Status timer replication follows a documented upper cadence bound and does not force an actor update every server tick solely to publish changing remaining-time floats. Owners still see accurate status activation, stacking, expiry, and death/recovery transitions within that bound; gameplay effects and expiration remain server-authoritative and status data stays owner-only. A representative multiplayer profile records bounded per-owner update frequency, replication bytes, and server scheduling cost against a documented budget. No net-profile measurement was run during this review.
- **Focused validation:** Extend the focused status automation for stacking, poison ticks, recovery, expiry and death cleanup; verify owner/observer privacy in a multiplayer scenario; and capture a net profile at the supported representative player count to establish the cadence and replication-work bounds. No test was run during this review.

## RF66 — Keep Ocean discovery candidates inside their source spatial key (P3)

- **Source:** CR09-001c; `Source/KalmalaGameplay/Private/KalmalaOceanDiscoveryCatalogue.cpp:130-138,159-176`; shared mapper `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:61-69`; related invariant test `Source/KalmalaWorld/Private/Tests/KalmalaWorldPlayerStartResolverTest.cpp:190-214`; Ocean tests `Source/KalmalaGameplay/Private/Tests/KalmalaOceanDiscoveryClaimTest.cpp:24-62`; and server activation/claim callers in `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9314-9389,10083-10154`.
- **Affected path:** The server builds optional Ocean discovery descriptors, groups materialized actors by source key, and stores authenticated claim identity under the descriptor's spatial key.
- **Trigger:** A generated 16-bit X or Y sample is `0xFFFF` and that candidate is accepted as deep Ocean. Dividing the value by `65535.0f` produces exactly `1.0f`; adding the full 6,400 cm key size places the candidate on the source cell's upper boundary, while the descriptor keeps the input key.
- **Impact:** `GetSpatialKey` maps the boundary location to the adjacent key, but the deterministic actor grouping and claim identity still name the source key. A neighboring cell can independently generate its own optional discovery. This is a rare spatial ownership/per-cell placement mismatch. No duplicate reward or authority bypass was demonstrated.
- **Evidence:** The shared mapper floors each coordinate divided by `SpatialKeySizeCm`. Ocean `BuildDescriptors` divides each 16-bit component by 65,535, adds the resulting fraction times the full key size, and stores the input key. `IsCurrentDescriptor` accepts the canonical regenerated position and key without checking that the position maps back to that key. `RefreshOceanDiscoveries` groups spawned actors by the supplied key, and `ClaimOceanDiscovery` derives its stable sparse identity from the same key. The shared population-layout test asserts that descriptors remain in their spatial key. Ocean catalogue/claim tests check canonical regeneration, qualifying water, identity and forged-position rejection but omit this containment invariant.
- **Proposed repair:** Map sampled coordinates into a half-open key interval (for example, divide by 65,536 or clamp below the upper edge) and retain canonical descriptor validation.
- **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
- **Acceptance:** Deterministic endpoint cases and a bounded grid of accepted Ocean descriptors satisfy `FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(Location)) == SpatialKey`; one optional descriptor per source key, stable identity, claim replay rejection and reward behavior remain unchanged.
- **Focused validation:** Extend `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract` with source-key containment, including the upper 16-bit endpoint, and run it after the required Editor build. No test was run during this review.

## RF67 — Correct the current storage save schema in architecture references (P3)

- **Source:** CR11-001; docs/02-technical-architecture.md:664,1108,1401-1402,1416,1644; Source/KalmalaGameplay/Public/KalmalaStorageSaveGame.h:23; Source/KalmalaGameplay/Private/KalmalaStorageSaveGame.cpp:4-6,50-58.
- **Affected path:** A developer reads the architecture save-version reference before inspecting or validating chest storage saves.
- **Trigger:** Several current architecture paragraphs and the version table say the storage SaveGame uses schema 2, while the runtime owner initializes schema 3 and MatchesWorld rejects any other version. A later paragraph correctly says schema 3.
- **Impact:** The authoritative format reference is contradictory and can produce incorrect development-save or test expectations. This is documentation drift; no runtime defect was found.
- **Evidence:** CurrentSchemaVersion is 3; InitializeForWorld writes it and MatchesWorld requires it. The cited architecture passages say schema 2, while the current M9 persistence paragraph at line 1644 identifies schema 3. RF57 is the separate generated-population table mismatch.
- **Proposed repair:** Correct the stale storage references and table to schema 3 while retaining the distinct construction schema 7, player-discovery schema 3, and M7 sparse-ledger schema 1.
- **Dependencies:** After CR19 review acceptance; no documentation repair during the review-only phase.
- **Acceptance:** Current architecture references consistently identify storage schema 3 and agree with the runtime constant; other save owners and the unreleased no-migration policy remain correctly distinguished.
- **Focused validation:** Compare every current storage-version reference against the runtime constant and current-format acceptance text. No build is needed for a documentation-only repair.

## RF68 — Reject out-of-bounds construction records during restore (P3)

- **Source:** CR11-001; Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:13-20,65-92; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-953; Source/KalmalaWorld/Public/KalmalaWorldBounds.h:8-16; Source/KalmalaGameplay/Private/KalmalaPlacementPreview.cpp:774.
- **Affected path:** The authoritative server restores placed construction when a world starts.
- **Trigger:** A finite saved transform outside the playable world radius or live placement margin passes IsValidRecord. RestorePersistedConstruction checks that record and then uses SpawnActorDeferred with AlwaysSpawn; it does not repeat the world-bound placement check.
- **Impact:** A malformed current-schema save can materialize an authoritative placed actor beyond the 1,600,000 cm playable radius, where players cannot reach it and world state no longer matches placement constraints. This is a persistence-validation defect, not a client placement authority bypass.
- **Evidence:** The construction transform helper checks finite location and scale plus a near-unit quaternion but does not check world bounds; IsValidRecord uses that helper. FKalmalaWorldBounds defines the 1,600,000 cm radius, and live construction placement checks it with a 300 cm margin. Startup restoration validates the record then spawns with AlwaysSpawn. Existing RF22/RF23 cover different player/world save transaction cases; RF24 covers skiff restore collision and does not cover construction bounds.
- **Proposed repair:** Reject persisted construction transforms outside the same world margin enforced by live construction placement before spawning; preserve the existing save unchanged on rejection.
- **Dependencies:** After CR19 review acceptance; no repair during the review.
- **Acceptance:** Current-format construction records outside the playable world margin are rejected on load without spawning actors or rewriting the slot. In-bounds records restore as before.
- **Focused validation:** Extend the construction-save contract and restore coverage with finite transforms beyond the radius and within the excluded 300 cm margin, plus a valid in-bounds control. Verify rejection leaves the serialized slot unchanged. No test was run during this review.

## RF69 — Keep rejected lethal wildlife hits from leaving active zero-health actors (P2)

- **Source:** CR09-001d-01; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-599,689-702,1083-1097; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8677-8694; Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:99-123; Source/KalmalaWorld/Public/KalmalaWorldPopulationSaveGame.h:48; and Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBadgerTest.cpp:91-95.
- **Affected path:** A server accepts a lethal wildlife hit, then a credited species reward handler tries to queue its durable owner-bound claim before publishing the defeat.
- **Trigger:** The pending wildlife-reward array is at its 4,096-row limit, or the same stable claim is already bound to a different player identity, when a queue-backed species defeat is recorded.
- **Impact:** Damage has already set Health to zero. When the reward callback rejects, DefeatServer returns before setting the defeated flag, unregistering the creature, or broadcasting defeat. The server tick gates wildlife behavior on authority and the defeated flag, not Health, so the zero-health actor continues its AI and can attack. There is no automatic retry of this unqueued defeat; another damaging hit is needed to re-enter the callback after the rejection condition clears.
- **Evidence:** ApplyValidatedDamageFromServer clamps Health to zero before calling DefeatServer. The actor returns immediately when a species Record*DefeatReward handler fails, before its accepted-defeat mutations. For example, RecordBadgerDefeatReward returns false when TryQueuePendingBadgerReward rejects, and only marks/saves the spawn after queue acceptance. The common queue helper rejects at 4,096 pending rows and rejects an existing claim owned by another identity. Tick runs the archetype behavior while the actor is not marked defeated, without checking Health. Existing Badger automation checks queue-model idempotency and owner identity, but does not execute this GameMode/actor rejection path. RF17 is the distinct Rat false-ledger ordering defect; RF18/RF46 are callback coverage gaps and do not record this live zero-health AI state.
- **Proposed repair:** Make lethal state and durable reward acceptance one recoverable server transaction. On rejection, either restore a valid retryable pre-lethal health state or enter an explicit inactive pending-defeat state. Retain the original credited identity and outcome, and retry server-side until the claim and defeated ID can be committed; then unregister and publish defeat once.
- **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
- **Acceptance:** With a full pending queue and with a conflicting stable claim identity, a rejected lethal hit leaves no zero-health creature running AI, emits no accepted defeat, and creates no wrong-owner/duplicate claim. After a queue slot becomes available, retry completes the same stable owner-bound reward and defeated ID once, then unregisters and broadcasts defeat once. Repeated hits/retries do not reroll or duplicate the claim.
- **Focused validation:** Add an isolated GameMode plus wildlife defeat regression that injects a full queue and a conflicting claim, observes Health/AI/defeat state, frees the queue for the valid case, and verifies owner identity and exact-once persistence. Run the focused automation after the required Editor build. No test was run during this review.

## RF70 — Fail closed on invalid explicit world-launch identity (P3)

- **Source:** CR10-005; Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp:30-74; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:520-569; Source/KalmalaGameplay/Private/KalmalaMenuContext.cpp:115-180; and docs/startmenu.md:184-206,494-522.
- **Affected path:** Starting a solo or host-co-op world with explicit menu launch identity options.
- **Trigger:** One or more KalmalaMenuLaunch, KalmalaMenuSeed, or KalmalaMenuRevision options are present but incomplete, malformed, stale, incompatible with the current network mode, or rejected by the server's seed-selection API.
- **Impact:** The GameState logs rejection and returns with constructor seed 10323456789; GameMode still proceeds to select seed-keyed world slots and start gameplay. The user can enter and later modify a different world than the explicit launch intended. The normal menu revalidates and emits the selected seed/current revision; no remote client seed-control path was found.
- **Evidence:** PostInitializeComponents detects any explicit menu identity option, validates the options, then logs and returns on failure without recording a rejected startup state. The constructor's default seed remains active. AKalmalaGameMode::BeginPlay consumes that config and continues through population save loading and construction-slot selection. With no menu options, the documented default or process WorldSeed= path remains intentional. The start-menu contract requires the selected world seed and revision to be revalidated before travel.
- **Proposed repair:** Preserve the no-menu-options default/command-line path and valid explicit launches. Record invalid explicit identity as a startup failure and stop before selecting, loading, creating, or writing any world save; return to the menu with a local error if supported.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** No menu identity options retain the normal default or process WorldSeed= behavior. Valid explicit solo and host-co-op options start the requested seed. Any explicit incomplete, malformed, stale, or network-mode-mismatched options fail closed before gameplay/save startup and never load, create, or write a fallback seed's world records.
- **Focused validation:** Cover valid standalone/listen-host options, missing or malformed identity fields, stale revision, and mode mismatch. Verify invalid launches stop before GameMode save-slot selection or persistence while no-option defaults remain unchanged. No runtime launch was run during this review.

## RF71 — Cover fail-closed preservation of invalid construction save slots (P3)

- **Source:** CR11-002; Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveGameTest.cpp:276-368; Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveWriteCandidateTest.cpp:29-77; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:568-585,702-741; docs/07-development-setup.md:1941,4331-4403; Scripts/Tests/Construction/ConstructionPersistence.ps1; Scripts/Tests/Survival/PersistedCampRestart.ps1.
- **Affected path:** A server starts a world whose same-seed construction slot contains a deserializable but incompatible or malformed save, then a player attempts another construction write.
- **Trigger:** A future change treats the invalid existing save as absent or allows a new candidate to write after startup rejected the slot.
- **Impact:** Previously persisted construction data could be replaced by an empty or partial current save. Current source fails closed, but no focused regression protects the no-rewrite guarantee for construction slots.
- **Evidence:** The assigned model tests cover current-format in-memory round-trips, malformed data, zero/future versions, and candidate isolation; none seeds or reads an actual invalid construction slot. ConstructionPersistence.ps1 covers valid current-format restore only. PersistedCampRestart.ps1 covers valid restore and no restore under a different seed, not invalid bytes in the same slot. GameMode startup only builds a candidate when the slot is absent or loaded data is non-null; an invalid candidate leaves ConstructionSaveGame null, and PersistConstruction rejects writes while it remains null. Setup requires incompatible or invalid slots to fail closed without rewriting. RF22 is specific to player-discovery slots and RF23 covers world-save/Home-return transactions; neither protects this path.
- **Proposed repair:** Add an isolated GameMode persistence regression that seeds an invalid current construction save at the exact slot for the test world, starts the production loader, attempts a construction write, and compares the original slot bytes before and after. Keep a valid same-world restore control and the cross-seed rejection check.
- **Dependencies:** After CR19 review acceptance; no test repair during the review-only phase.
- **Acceptance:** An incompatible or malformed same-slot construction save is not restored or rewritten, later construction persistence remains rejected, and the original bytes are identical after the attempted write. A valid current-format save still restores through the same production path.
- **Focused validation:** Run the isolated GameMode regression with Kalmala.Gameplay.Construction.Schema7CurrentFormat, Kalmala.Gameplay.Construction.Schema7WriteCandidate, and Scripts/Tests/Construction/ConstructionPersistence.ps1 after an Editor build. No test was run during this review.

## RF72 — Require Thunder Mountains coverage for the all-biome Stone fallback (P3)

- **Source:** CR10-006; Source/KalmalaWorld/Private/Tests/KalmalaBiomeContentContractTest.cpp:118-122,199-207,239-262; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:246-286; docs/07-development-setup.md:2077-2083; docs/05-decision-log.md:10.
- **Affected path:** Deterministic regression coverage for generated Stone outcrops in Thunder Mountains spatial keys without a same-key slate source.
- **Trigger:** The test sets bSawStoneOutcropMountains for any mountains-slate-vein descriptor and its completion predicate accepts that flag. The allbiomes-stone-outcrop branch only verifies its descriptor when the same flag is still false. A slate source in one key can therefore satisfy Mountain coverage without requiring the fallback in another key that lacks slate.
- **Impact:** The contract that fills each land biome lacking a slate source per spatial key has no independent Mountain fallback regression assertion. A future change could omit Mountain fallback outcrops while other biomes and the Mountain slate source keep the current test green. No current production placement failure or gameplay authority issue was confirmed.
- **Evidence:** The test shares bSawStoneOutcropMountains between the Thunder Mountains case for allbiomes-stone-outcrop and the independent mountains-slate-vein shortcut, then uses it in HasCompleteSourceCoverage. Production builds BiomesWithStone separately for each spatial key and adds allbiomes-stone-outcrop only for a land biome missing a slate source in that key. The setup and decision log describe this six-biome, per-key fallback. Static review did not execute the deterministic fixture, so this finding concerns the missing required assertion, not a claim that the current fixture currently generates no Mountain fallback.
- **Proposed repair:** Track Mountain slate-source coverage separately from all-biome fallback coverage. Use a deterministic fixture key with a Thunder Mountains candidate and no same-key mountains-slate-vein source, then require and validate the generated allbiomes-stone-outcrop.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A focused test independently proves that a selected Thunder Mountains key with no slate source receives exactly one allbiomes-stone-outcrop when a safe separated candidate exists, with stable identity, safe terrain, and 250 cm separation. A Mountain slate descriptor from another key cannot satisfy this assertion.
- **Focused validation:** Extend Kalmala.World.M7.BiomeContentContract with the deterministic key fixture and run it after the required Editor build. No test was run during this review.

## RF73 — Make Deer flee away from their triggering threat (P3)

- **Source:** CR09-001d-02; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:689-775,782-793,5039-5051,5076-5120`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:16-25,94-103`; `docs/01-game-design.md:138-142`; and `docs/02-technical-architecture.md:1138-1140`.
- **Affected path:** An accepted hit transitions a Deer to Flee and alerts nearby idle Deer; the server advances each selected Deer toward its destination.
- **Trigger:** A Deer is hit or alerted by combat while its stable spawn-ID-derived flee offset points toward the attacker or noise source.
- **Impact:** The Deer moves toward the threat during the Flee phase instead of away, undermining the documented wary response and potentially drawing hunted or alerted animals toward the player. No authority bypass or reward duplication was found.
- **Evidence:** `ApplyValidatedDamageFromServer` sends Deer through the Flee transition and calls `AlertNearbyDeerFromServer`; the alert helper independently assigns `SpawnOrigin + GetDeterministicOffset(...)`. `GetDeterministicOffset` derives an angle only from the persistent spawn ID and receives no threat location. `AdvanceServerBehaviour` moves directly toward `BehaviourDestination` while the state is Flee. The following Investigate and Return stages occur afterward, so the toward-threat movement is not the homeward return. The design says Deer flee from noise and nearby combat; the architecture calls this bounded wary-herd flee behavior. `KalmalaWildlifeBehaviourTest` checks state transitions and bounded flee distance, but not direction.
- **Proposed repair:** Pass a trusted server-side source location through direct-hit and herd-alert transitions, then select a bounded destination away from that source. Keep a stable bounded fallback for a coincident source and retain server control of destination selection.
- **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
- **Acceptance:** Affected Deer and alerted herd members move away from the triggering combat/noise source within their ecology-specific flee distance, including all tested source bearings; coincident-source fallback remains bounded and server-owned. Later Investigate/Return behavior still returns to the spawn origin.
- **Focused validation:** Extend `Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle` with direct-hit and nearby-herd destination/movement assertions for multiple bearings, plus coincident-source fallback, authority and flee-distance bounds. Run after the required Editor build. No test was run during this review.

## RF74 — Restore Mireling campsite attraction and camp-pressure behavior (P2)

- **Source:** CR09-001d-03; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1112-1135`; `docs/01-game-design.md:140`; `docs/02-technical-architecture.md:860,1138`; `docs/05-decision-log.md:30,151`; and `docs/biome-enemies.md:417-423`.
- **Affected path:** A living server Mireling selects its target and applies its close-range melee behavior.
- **Trigger:** A Mireling is active near a campsite while no living player is within its 500 cm target search, or players are nearby but the camp itself needs pressure.
- **Impact:** The Mireling idles without a player in range and otherwise approaches or attacks only a player. It never detects, approaches, or pressures campsites or their supplies, so the documented camp-defense loop is absent. No client authority bypass was found.
- **Evidence:** `UpdateMirelingScavenge` iterates only authoritative living `AKalmalaCharacter` actors within 500 cm, returns when none exists, and approaches/melees that character. A source search found no other Mireling campsite or supply target behavior. The active game design calls the Mireling a scavenger attracted to campsites that pressures camp-supply protection; architecture says Mirelings scavenge and threaten camps. The A8 base-enemy proposal retains approach/simple melee without saying targets are player-only, and the accepted M5 decision preserves existing target selection without removing the campsite role. Mireling tests cover spawn share, habitat, stats and cooldown, not campsite behavior.
- **Proposed repair:** Add bounded server-owned campsite-aware targeting and pressure while retaining the simple-melee contract. Define the precise supply-pressure outcome through an accepted design decision before implementing it; do not allow clients to choose sites or supply outcomes.
- **Dependencies:** After CR19 review acceptance; any supply effect requires its own accepted behavior detail. No repair during CR09 review.
- **Acceptance:** With no player in the current 500 cm search, a living Mireling can select an eligible campsite through server-owned bounded rules and apply the separately approved pressure outcome. Tests cover the active site, no-player case, rejected/unavailable sites and authority; clients cannot select a site, item or result.
- **Focused validation:** Add a Mireling behavior contract and focused server-side scenario for campsite target eligibility, bounded approach and the approved pressure result. Run after the required Editor build. No test was run during this review.

## RF75 — Reject overflowing Smelter save counters (P3)

- **Source:** CR11-003; Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:108-130; Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:65-75,160-173,227-245; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-953,831-855; Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp:112-137,193-201; Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:447-483; Source/KalmalaGameplay/Private/Tests/KalmalaConstructionSaveGameTest.cpp:396-430; docs/07-development-setup.md:1941,4392-4403.
- **Affected path:** A schema-7 world slot restores a Smelter with an overflowing Coal or output count, after which a player collects its bars.
- **Trigger:** A deserializable current-schema record contains nonnegative signed 32-bit counters whose aggregate overflows before the capacity check.
- **Impact:** The invalid record can pass current validation and restore with impossible Smelter contents. Output collection can then persist decremented values and grant bars beyond the ten-unit output capacity. No client request selects or modifies the saved counter; this is malformed server-slot validation.
- **Evidence:** GetInputCoalCount adds the active-job reservation to InputCoal as int32; GetOutputCount sums three int32 output counters. IsValid checks InputCoal < 0 and each output < 0, but relies on the potentially overflowing sums for upper bounds. For example, INT_MAX Coal with a valid active job, or INT_MAX Copper plus INT_MAX Tin, can overflow on the Windows target and evade the sum comparison. Construction IsValidRecord calls Smelter.IsValid, and MatchesWorld applies IsValidRecord to every record. Startup restore and actor initialization repeat the same predicates without another counter bound. CollectSmelterItemFromServer decrements one output, persists the candidate through State.IsValid and MatchesWorld, then commits one bar; none independently caps the stored output fields. Current tests cover ordinary full-capacity states but no integer-boundary values. Setup requires invalid current slots to fail closed without rewriting. RF68 concerns transform world bounds; RF71 concerns the separate invalid-slot regression gap.
- **Proposed repair:** Validate each stored counter against its capacity and compute reserved/aggregate counts in a wider checked type. Add boundary cases for INT_MAX with an active job and an overflowing output sum; require both record and world validation to reject them. Exercise same-slot startup preservation and retain valid zero/full-capacity controls.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Overflowing current-schema Smelter count sets fail both IsValidRecord and MatchesWorld before spawning; startup preserves exact slot bytes. A valid ten-output boundary restores. One collection saves nine outputs before granting exactly one bar, and a later load retains that nine-output state.
- **Focused validation:** Extend Kalmala.Gameplay.Construction.Schema7CurrentFormat and the invalid-slot GameMode regression, then run Scripts/Tests/Construction/ConstructionPersistence.ps1 after an Editor build. No test was run during this review.

## RF77 — Stop wildlife attacks after an accepted poison defeat (P3)

- **Source:** CR09-001d-03; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-599,642-681,1083-1095,1138-1168`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:320-360`; and `Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp:228-251`.
- **Affected path:** Fang Arrow poison defeats a charging Boar during its authoritative actor tick.
- **Trigger:** A poison tick lethally damages a Boar with a committed `BoarChargeTarget`, positive charge time and a nearby living player, while the melee cooldown permits a hit.
- **Impact:** The accepted defeat hides and marks the Boar defeated, but the same server Tick continues into `UpdateBoarTerritory`, which can apply one additional hit from the defeated actor. This is a one-tick gameplay lifecycle defect; no client authority bypass was found.
- **Evidence:** `Tick` evaluates authority/`!bDefeated` once, calls `AdvanceFangArrowWeakPoisonFromServer`, then unconditionally advances behavior and calls the archetype updater inside that already-entered block. A lethal poison tick calls validated damage and `DefeatServer`; accepted defeat sets `bDefeated` and `ApplyDefeatedState` hides the actor, but the current call continues. `UpdateBoarTerritory` has no defeated guard, retains its committed target/timer through defeated-state cleanup, and directly calls the Character damage helper when the target is within 180 cm. `AKalmalaCharacter::ApplyWildlifeDamageFromServer` checks source validity, authority, same world, damage and reach but not whether a wildlife source is defeated, so it accepts the hit. Existing BowDraw coverage advances poison and checks damage/expiry but does not combine lethal poison with the wildlife behavior updater; Boar behavior tests do not cover defeat during a committed charge. RF69 is the separate rejected lethal-reward path that leaves an actor active across later ticks.
- **Proposed repair:** Recheck `bDefeated` after poison advancement and exit the current Tick before behavior updates; also consider rejecting defeated wildlife sources in the Character damage helper as defense in depth.
- **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
- **Acceptance:** A poison tick that defeats a charging Boar prevents further movement or character damage in that Tick; the defeated wildlife source is rejected by any shared damage helper guard, and ordinary living-Boar charges remain unchanged.
- **Focused validation:** Add a focused test that lethally poisons a Boar during a committed charge and verifies defeat occurs once with no post-defeat hit or movement. Run after the required Editor build. No test was run during this review.

## RF78 — Honor authored extended wildlife hit ranges in shared damage validation (P2)

- **Source:** CR09-001d-05/06; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1957-2216,2218-2360,2715-2720,2948-2956,3096-3105`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:320-333`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePolarBearTest.cpp:68-77`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeMoonBearTest.cpp:55-76`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifePeatHuskTest.cpp:83-112`; and `docs/biome-enemies.md:45,51,54`.
- **Affected path:** Authoritative Polar Bear charge, Moon Bear overhead paw, and Peat Husk sweeping-arm damage to a player Character.
- **Trigger:** A valid visible Bear target is at a three-dimensional distance greater than 180 cm and at most 220 cm when the hit is applied, or a valid visible Peat Husk target has horizontal distance at most 260 cm and a forward dot of at least 0.5 while its three-dimensional distance exceeds 180 cm.
- **Impact:** The species-specific range predicate accepts the target, but `AKalmalaCharacter::ApplyWildlifeDamageFromServer` rejects the non-Thunder-Sentinel source beyond 180 cm in three dimensions. The player takes no damage inside the authored Bear reach or Peat Husk sweep area despite passing the attack-specific range gate. This is a bounded server-side combat contract mismatch; no client authority bypass was found.
- **Evidence:** `IsPolarBearChargeInReach` and `IsMoonBearPawInReach` accept through 220 cm, and `TryApplyPolarBearCharge`/`TryApplyMoonBearPaw` pass accepted hits to the shared Character helper. `IsPeatHuskSweepInReach` accepts horizontal distance through 260 cm when the forward dot is at least 0.5 (the 120-degree cone); `TryApplyPeatHuskSweep` then calls that same helper. At `KalmalaCharacter.cpp:325-333`, the helper rejects three-dimensional distances greater than 180 cm unless the source archetype is Thunder Sentinel; its explicit Sentinel exception accepts up to 300 cm horizontally. The Bear and Peat Husk focused tests cover their standalone predicates, not the integrated helper call at outer-band distances. `docs/biome-enemies.md` specifies 220 cm Bear attacks and a 260 cm/120-degree Peat Husk sweep. Reef Maw's 180 cm three-dimensional bite remains within the shared rule. No live outer-band scenario was run.
- **Proposed repair:** Reconcile the shared server validation with the reviewed Polar Bear/Moon Bear 220 cm and Peat Husk 260 cm-horizontal/cone reach contracts through validated source-specific rules. Preserve the ordinary 180 cm limit, Thunder Sentinel's existing exception, and each attack's target/visibility checks; do not widen all wildlife attacks.
- **Dependencies:** After CR19 review acceptance; no repair during CR09 review.
- **Acceptance:** Integrated server-side coverage accepts Bear hits at 180 cm and the inclusive 220 cm boundary, and rejects beyond 220 cm; accepts Peat Husk hits beyond 180 cm through the inclusive 260 cm horizontal boundary within its cone, and rejects beyond 260 cm or outside the cone. Invalid/client sources remain rejected, ordinary wildlife keeps its authored maximum reach, Reef Maw remains at 180 cm, and Thunder Sentinel retains its 300 cm horizontal exception.
- **Focused validation:** Extend the focused server-side Polar Bear, Moon Bear, and Peat Husk hit call-path coverage to exercise the shared Character helper at each authored boundary and just outside it, including authority rejection. Retain the Reef Maw 180 cm and Thunder Sentinel 300 cm regressions. Run after the required Editor build. No test was run during this review.

## RF79 — Keep CommonUI as a private KalmalaUI dependency (P3)

- **Source:** CR12-001; `Source/KalmalaUI/KalmalaUI.Build.cs:6-27`; `Source/KalmalaUI/Private/KalmalaMinimapSubsystem.cpp:6,113-150`; targeted search of `Source/KalmalaUI/Public`; and `Kalmala.uproject:39-42`.
- **Affected path:** UnrealBuildTool resolves dependencies for another module that consumes KalmalaUI.
- **Trigger:** A downstream module declares a dependency on KalmalaUI and receives its public dependency set.
- **Impact:** CommonUI is exposed transitively to every KalmalaUI consumer, although KalmalaUI's public headers do not include or reference its API. This unnecessarily couples downstream build graphs to CommonUI; no runtime, gameplay authority, or privacy defect was found.
- **Evidence:** `KalmalaUI.Build.cs` lists `CommonUI` in `PublicDependencyModuleNames`. The only source usage found is the private `KalmalaMinimapSubsystem.cpp`, which includes `Input/CommonUIActionRouterBase.h` and uses the action router and input configuration. A targeted search across `Source/KalmalaUI/Public` found no CommonUI include or symbol reference. The CommonUI plugin is enabled in the project, so the module's implementation dependency remains available.
- **Proposed repair:** Move `CommonUI` to `PrivateDependencyModuleNames` and retain the plugin setting and current minimap input-router behavior.
- **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
- **Acceptance:** KalmalaUI's CommonUI implementation still compiles, its public headers remain free of CommonUI API requirements, and unrelated downstream modules no longer receive CommonUI transitively through KalmalaUI.
- **Focused validation:** After the required Editor build, compile KalmalaUI with CommonUI private and inspect a representative consumer's dependency graph to confirm no transitive CommonUI requirement. No build was run during this review.

## RF80 — Restore primary focus after a listed world disappears (P3)

- **Source:** CR12-002; current main-checkout path E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaHomeSubsystem.cpp:758-775, inspected read-only; cross-reference E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldListWidget.cpp:589-595, 641-646, inspected read-only.
- **Affected path:** A local player opens details for a previously listed world seed.
- **Trigger:** An existing seed disappears from recognized local save slots after Your worlds is displayed but before the player opens World details.
- **Impact:** ShowWorldDetails refreshes the records and fails. HandleWorldSelected returns to Worlds and rebuilds the picker, but does not assign focus to a live control. Keyboard/controller users may lose their current focus target and need to navigate manually. No save mutation, gameplay authority defect, or private-data exposure was found.
- **Evidence:** The current main-checkout HandleWorldSelected failure branch calls ShowWorldList and returns before its normal FocusPrimary call. ShowWorldDetails refreshes slot records and returns false when the existing seed is absent; ShowWorldList rebuilds the list controls. FocusPrimary selects a live picker, New world, or Back target based on current listing state.
- **Proposed repair:** After rebuilding the list in this failure branch, call FocusPrimary so focus returns to a valid enabled primary control.
- **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
- **Acceptance:** If the selected existing seed disappears between listing and details, the updated list remains visible and focus lands on a live enabled primary target (the picker when entries remain, otherwise New world or Back according to listing availability). Keyboard and controller navigation can continue immediately, and no save is changed.
- **Focused validation:** Add a focused Home/world-list automation that removes the selected seed after listing and asserts that the rebuilt screen focuses a live primary control for populated and empty lists. After an Editor build, run the focused automation and verify keyboard/controller recovery in the rendered screen. No test was run during this review.

## RF81 — Include generator revision in generated population save identity (P3)

- **Source:** CR10-010; `Source/KalmalaWorld/Public/KalmalaWorldGenerationConfig.h:23-26`; `Source/KalmalaWorld/Private/KalmalaWorldPopulationSaveGame.cpp:30-48`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:205-208,647-665`; `Source/KalmalaWorld/Public/KalmalaM7PersistenceContract.h:37-56`; `Source/KalmalaWorld/Private/KalmalaM7PersistenceContract.cpp:62-69`; `Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp:62-64`; `Source/KalmalaWorld/Private/Tests/KalmalaWorldPlayerStartResolverTest.cpp:235-281`; and `docs/02-technical-architecture.md:958-959`.
- **Affected path:** Loading same-seed generated-population deltas after the compiled world generator advances to a new revision.
- **Trigger:** A population slot written under revision 7 is opened under a later generator revision with the same seed. The slot name and save record contain no generator revision, while the population save schema remains version 2.
- **Impact:** Old harvested/defeated IDs, treasure claims and gemstone state can be applied to a different generated layout. Changed candidates can reappear when their new IDs no longer match old depletion, or a new descriptor sharing an old stable ID can inherit stale state. This is a server-side persistence identity defect; no client authority bypass was confirmed.
- **Evidence:** The documented world-save identity includes generator revision. `FKalmalaM7SaveIdentity::Matches` requires equal revisions, and launch validation requires the current revision. By contrast, `FKalmalaWorldGenerationConfig::operator==` compares only `WorldSeed`; population slots are named only by seed; `MatchesWorld` checks schema 2 and that seed-only config. `LoadPopulationSave` therefore accepts a same-seed old-revision record. The current focused population-save test rejects a different seed but has no same-seed/different-revision case. The targeted population slot/loader code is identical in the isolated checkout and the current main file; main was inspected read-only.
- **Proposed repair:** Include the current generator revision in the generated population save identity and slot lookup, and reject mismatched revisions without rewriting the old slot. Keep existing data available for an explicitly supported future migration decision; do not silently reuse it.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A population record written for a given seed and generator revision loads only for that exact pair. Starting the same seed under a new revision initializes or loads only that revision's population state and leaves the previous-revision slot bytes unchanged. Current-revision harvest, defeat, treasure, and gemstone deltas still round-trip normally.
- **Focused validation:** Extend the population-save identity test and the GameMode load contract to cover same-seed current/stale revisions, verify stale data is not retained or overwritten, and retain the current-revision round trip. No test was run during this review.

## RF82 — Add current snake attack-path regression coverage (P3)

- **Source:** CR09-001d-07; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3132-3463`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeViperTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeGiantSnakeTest.cpp`; `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeDeathAdderTest.cpp`; `Scripts/Tests/Combat/EnemyPeers.ps1:4,133-147`; and `docs/07-development-setup.md:2461-2463,2490-2492,2612`.
- **Affected path:** Future changes to authoritative Viper, Giant Snake, and Death Adder lunge/bite behavior, including target and range rejection and accepted species-specific poison.
- **Trigger:** A production attack-path regression occurs while existing profile, spawn, reward, persistence, and Mosquito peer checks continue passing.
- **Impact:** The current test suite can miss regressions in live snake attack resolution, including damage from a rejected bite or loss of an effect from an accepted bite. This is a coverage gap; no current snake production behavior defect was confirmed. RF19 covers peer-runner documentation, and RF46 covers remaining defeat-reward callbacks; neither covers this attack path.
- **Evidence:** `AKalmalaWildlifeSpawn::Tick` dispatches Viper, Giant Snake, and Death Adder to `UpdateSnakeAmbush`; `BeginSnakeAttack` and `TryApplySnakeBite` contain the live lunge, target, reach, visibility, damage, and poison path. The focused Viper/Giant Snake/Death Adder tests cover roster/profile/timing, reward identity, queueing, and persistence, but do not call the production state-machine or bite-application functions. The peer runner lists `MosquitoCounterplay` but no current snake scenario. The setup guide explicitly labels ViperPeer and GiantSnakePeer runs historical. No current snake attack-path assertion was found.
- **Proposed repair:** Add integrated server-side regression coverage that drives the production snake lunge/bite path and checks valid and rejected targets, reach, and species-specific accepted-hit effects. Keep Giant Snake poison-free; retain shared poison lifecycle and reward-callback coverage at their existing owners.
- **Dependencies:** After CR19 review acceptance; no test or gameplay repair during CR09 review.
- **Acceptance:** Current automation exercises Viper, Giant Snake, and Death Adder resolution through the server attack path. Invalid, out-of-range, and obstructed targets cause no damage or poison; accepted Viper and Death Adder bites apply their authored poison, while Giant Snake applies none. Existing MosquitoCounterplay, shared poison lifecycle, and RF46 callback coverage remain distinct.
- **Focused validation:** Extend focused wildlife automation and the current peer runner with deterministic accepted/rejected attack assertions. Run after the required Editor build. No test was run during this review.

## RF83 — Disable visibility-blocking collision on defeated wildlife (P2)

- **Source:** CR09-001d-07; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:165-179,583-596,3724-3740,5328-5331,5356-5395`; `Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:171-200,228-239`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1447-1450,1457-1465`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6811-6827,8649-8655`; and `docs/02-technical-architecture.md:2772`.
- **Affected path:** Server melee target selection and character visibility traces after a wildlife actor is defeated and hidden.
- **Trigger:** A defeated wildlife actor lies between a player and a living wildlife target or an eligible visibility-traced interaction target.
- **Impact:** The hidden actor can remain the first `ECC_Visibility` hit. `FindServerTarget` returns that wildlife actor, after which `IsServerTargetValid` rejects it as defeated; the attack cannot select a living target behind it. Character view traces can likewise stop at the hidden actor. The obstruction lasts until the actor is otherwise destroyed. This is a gameplay query obstruction, not an authority bypass, and is distinct from RF77's same-Tick post-defeat damage path.
- **Evidence:** The root sphere is query-only and blocks `ECC_Visibility`; snake archetypes enable an additional visibility-blocking query capsule. `DefeatServer` unregisters the actor and applies defeated state; `ApplyDefeatedState` calls `SetActorHiddenInGame` and clears behavior/reward state but does not disable root or snake-body collision. The defeat path does not destroy the actor. The combat component uses an `ECC_Visibility` line trace and returns a hit wildlife actor before the later target validator rejects defeated actors. Character view traces also use `ECC_Visibility`. No current design or architecture contract specifies retaining hidden defeated wildlife as a visibility blocker. The architecture describes the procedural mesh as collision-free but does not define the defeated root-collision policy.
- **Proposed repair:** Disable visibility-blocking query collision as part of defeated-state application on the authority and replicas, while preserving the actor's required hidden/reward/persistence lifecycle and the existing collision behavior of living wildlife.
- **Dependencies:** After CR19 review acceptance; no gameplay repair during CR09 review.
- **Acceptance:** After defeat, the root and any snake-body component no longer block visibility queries on the server or relevant clients. Traces pass through the hidden actor and can select a living target or eligible interaction behind it. Living wildlife retains its existing validated collision behavior; defeat replication, reward delivery, and restart persistence are unchanged.
- **Focused validation:** Add component-state and server/peer trace coverage with defeated wildlife in front of a living target and an eligible interaction target. Verify living collision remains unchanged and defeat/restart behavior still passes. Run after the required Editor build. No runtime check was run during this review.

## RF84 — Restore inventory scroll fallbacks at supported viewports (P2)

- **Source:** CR12-003; current main-checkout path E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:288-300,337-360, inspected read-only; docs/07-development-setup.md:1583-1598,5260-5284; docs/02-technical-architecture.md:3282-3289.
- **Affected path:** A player opens Inventory at the documented 480x320 viewport and tries to inspect lower equipment slots or reach following inventory actions.
- **Trigger:** The responsive panel is capped to viewport height minus 32 logical units, while its direct vertical content exceeds that cap and has no outer or inventory-row scroll fallback.
- **Impact:** Lower gear and action content extends below the visible viewport with no supported scroll path. This leaves part of Inventory undiscoverable at an accepted viewport and contradicts the M12 scroll-fallback acceptance. No gameplay authority or private-data exposure defect was found.
- **Evidence:** InventoryPanel mounts a plain UVerticalBox directly into the size-capped panel; UpdateResponsivePanelSize caps height at ViewportSize.Y - 32 while budgeting 376 units for Worn Gear alone, before the grid, details, padding, or actions. The M12 setup contract requires outer-menu and inventory-row scrolling at 480x320. The later Worn Gear record documents the current absence of a ScrollBox/fallback and defers the wider accessibility/resolution matrix, so this is an unresolved supported-viewport gap. The current main-checkout source was inspected read-only and matches the CR01 fingerprint.
- **Proposed repair:** Restore the documented outer-menu and inventory-row scroll fallbacks while keeping the responsive panel and all existing equipment/actions.
- **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
- **Acceptance:** At 480x320, pointer, keyboard, and controller users can scroll the outer panel and inventory row to reveal every gear slot, detail, and action with the active focus target visible. At 1024x768 the standard layout remains usable.
- **Focused validation:** Extend Kalmala.UI.InventoryMenu.Selection to assert the outer-menu and row scroll fallbacks and complete reachable content at 480x320, then confirm the standard layout at 1024x768. Run after an Editor build and inspect the focused rendered state. No test was run during this review.

## RF85 — Align initial land-wildlife meshes with terrain (P3)

- **Source:** CR09-001d-08a/08b/08c/08d; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:165-173,3645-3707,3711-3742,3878-3883,3906-3914,4041-4044,4089-4197,4211-4269,4282-4310,4314-4362,4364-4489,4605-5036; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6482-6523,6543-6581,7520-7531,7578-7596; Source/KalmalaGameplay/Private/Tests/KalmalaPopulationBodyClearanceTest.cpp:29-91,200-253; Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:112-154,163-176,188-238; docs/01-game-design.md:144-149; docs/02-technical-architecture.md:2772; docs/creature-catalogues.md:31-70,75-85,106-130,137-162,163-171; docs/biome-enemies.md:46-56,74-95,118-135; and docs/monster-spawning.md:369-378.
- **Affected path:** Generated Mireling, Boar, Deer, Rat, Badger, Moon Bear, Polar Bear, Horned Goat, Wolf, Lynx, Peat Husk, Viper, Death Adder, and Giant Snake presentations placed on generated land surfaces.
- **Trigger:** Population placement raises each actor by the vertical support of its active collision shapes, while the presentation mesh attaches at zero relative offset and its lowest geometry does not reach that ground plane.
- **Impact:** On flat ground the mesh remains about 25 cm above ground for Mireling, 17 cm for Boar, 3 cm for Deer, 44 cm for Rat, 18 cm for Badger, 14 cm for Moon Bear, 12 cm for Polar Bear, 15 cm for Horned Goat, 11 cm for Wolf at rest, 15 cm for Lynx at rest, and 29 cm for Peat Husk. Because the snake capsules currently remain vertical, production mesh gaps are about 87 cm for Viper, 106 cm for Death Adder, and 200 cm for Giant Snake; after RF93 aligns the capsules to their slithering bodies, the existing 70 cm root-sphere support still leaves about 65/64/59 cm gaps. These low/coiled land presentations float above their authored ground habitat. This is a presentation alignment defect; authoritative collision placement remains server-owned.
- **Evidence:** GetBodyCollisionShapesForArchetype uses a 70 cm root sphere for land archetypes other than Rat (42 cm) and adds vertical snake capsules with half-heights 92/112/210 cm (see RF93). TryGetPopulationBodyVerticalSupportOffset uses the greatest active-shape support on flat ground and TryGetPopulationActorPlacementLocation adds it to sampled terrain. Meshes attach at zero relative offset. Local minima are Z -45/-53/-67 cm for Mireling/Boar/Deer, +2 cm for Rat, -52 cm for Badger, -56 cm for Moon Bear, -58 cm for Polar Bear, -55 cm for Horned Goat, -59 cm for Wolf at rest, -55 cm for Lynx at rest, and -41 cm for Peat Husk. Reproducing AddSnakeBody path/radius/cross-section vertex math gives minima near -4.7/-6.1/-10.5 cm for Viper/Death Adder/Giant Snake. Existing tests check collider profiles and terrain support but do not compare mesh bounds with ground. Current design describes low/coiled dry-shore snakes and Peat Husk as a dry-ground root brute; architecture makes wildlife meshes presentation-only and non-colliding. Mosquito Swarm is excluded because its server actor intentionally flies 55-245 cm above sampled ground, within its documented 50-250 cm band.
- **Proposed repair:** Reconcile each reviewed land-wildlife resting mesh with sampled terrain and its intended support anchor so feet, hooves, paws, roots, and coiled bodies visibly meet the ground, while preserving actor/collision authority and the collision-free presentation contract. Coordinate snake bounds with RF93.
- **Dependencies:** After CR19 review acceptance; coordinate snake ground alignment with RF93. No repair during CR09 review.
- **Acceptance:** All fourteen reviewed land species remain visibly grounded at their authored contact surfaces on flat terrain and representative slopes, without gaps or penetration, including relevant resting and warning/attack poses. Collision support matches intended body geometry, and meshes remain non-colliding.
- **Focused validation:** Add geometry-to-placement bound assertions for all fourteen reviewed meshes and inspect rendered host fixtures on flat terrain and representative accepted slopes, including relevant warning/attack poses after the required Editor build. Verify snake collision-axis/support behavior under RF93. No rendered or runtime check was run during this review.

## RF86 — Present standalone Build requirements to keyboard and controller users (P3)

- **Source:** CR12-003; current main-checkout paths E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaInventoryCraftLayout.cpp:39-60,230-277 and E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaCraftingSubsystem.cpp:581-604,1945-1948,4826-4836, inspected read-only; docs/04-roadmap.md:830-843.
- **Affected path:** A player reviews a build card in the standalone Build menu using keyboard or controller focus.
- **Trigger:** The player focuses or activates a build card and needs its description, required station, material identities, and owner-visible owned/required counts.
- **Impact:** The standalone view shows a title and material icons, but the description and material names/counts are tooltip-only, while the station row is collapsed. These details are not available through a keyboard/controller focus target as M11 requires. Mouse hover is a partial workaround; no authority or privacy defect was found.
- **Evidence:** RefreshBuildRequirements assigns description and cost names/counts only to BuildRequirementsPanel and CostColumn tooltips; the displayed columns contain only icons and are non-focusable widgets. The selected-build title is visible, but not the complete requested detail. The separate required-station icon is a child of RequirementRow, which is added to BuildExcludedWidgets; standalone Build collapses those widgets. M11 requires the selected name, description, station icon, and material costs/counts on hover or keyboard/controller focus. Owner counts are read from the local inventory component.
- **Proposed repair:** Add a visible selected-build detail area driven by card focus, presenting the description, required-station icon and name, and material names with owner-visible owned/required counts. Keep pointer hover presentation and provide accessible labels for the focused details.
- **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
- **Acceptance:** Focusing each standalone Build card exposes its name, description, required station when applicable, and every material's identity and owner-visible owned/required count without relying on hover. Text scaling and high contrast keep the focused details readable; counts remain local to the owner.
- **Focused validation:** Extend a focused Build UI automation to move keyboard and controller focus among representative build cards and assert all detail fields, including station-present and station-free recipes. Run after an Editor build and inspect rendered focus at default and enlarged text scales. No test was run during this review.

## RF87 — Preserve exterior winding on Rat tail, Goat horns, and snake body tubes (P3)

- **Source:** CR09-001d-08b/08c/08d; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:4114-4184,4431-4470,4731-4825,5036; Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:112-155,188-220,238.
- **Affected path:** Rat generated tail tube, Horned Goat paired horn tubes, and Viper, Death Adder, and Giant Snake continuous body tubes, including side strips and end caps.
- **Trigger:** AddTailTriangle, AddHornTriangle, and AddSnakeTriangle reverse triangles when their face/expected-normal dot is negative, then preserve nonnegative results. All three helpers therefore emit nonnegative face/normal dots for nondegenerate triangles.
- **Impact:** These generated exterior surfaces have winding opposite the engine expected face orientation while their vertex normals point outward; rendering can cull or incorrectly shade them from intended exterior views.
- **Evidence:** The three helpers swap B/C for negative face/expected-normal dots and preserve other triangles, guaranteeing nonnegative dots. Rat samples measured raw dots from about -201 to -51 before reversal. AddSnakeTriangle applies the same rule to every side and cap triangle using averaged outward ring normals or end tangents. FKalmalaModelSurfaceTest requires a negative engine reference sign at line 125 and a negative face/vertex-normal dot at line 148; its explicit Viper, Giant Snake, and Death Adder fixtures at lines 188-220 exercise this path. Existing fixtures omit Rat and Horned Goat.
- **Proposed repair:** Make all three tube helpers preserve the engine expected exterior order relative to outward normals, and add explicit Rat tail and Horned Goat horn fixtures while retaining the current snake fixtures.
- **Dependencies:** After CR19 review acceptance; no geometry repair during CR09 review.
- **Acceptance:** Rat tail, Goat horns, and all snake body side and cap triangles satisfy the engine exterior winding convention and agree with their outward normals. The surface-winding regression covers these five species, and existing wildlife presentation remains intact.
- **Focused validation:** Extend Kalmala.Gameplay.Presentation.SurfaceWinding with explicit Rat and Horned Goat fixtures, retain coverage of Viper/Death Adder/Giant Snake, run it after the required Editor build, and inspect the exterior surfaces. No test or rendered check was run during this review.

## RF88 — Correct the stale creature-catalogue schema label (P3)

- **Source:** CR13-002; `docs/creature-catalogues.md:27`; `Content/Data/GameCatalogues.json:2`; `Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp:407`; `docs/02-technical-architecture.md:524`.
- **Affected path:** A developer consults the creature-catalogue guide for the current manifest and item/tool loader contract.
- **Trigger:** The guide says the C++ item/tool loader is “schema-15,” while the current manifest is schema 16 and the loader rejects every other version. The guide's separate statement that the item/tool loader does not consume `creatureFiles` remains accurate.
- **Impact:** The stale schema label can misdirect catalogue edits and verification. No runtime defect or creature-data mismatch was confirmed.
- **Evidence:** The manifest at line 2 declares schema 16; `LoadCataloguesFromJson` at line 407 requires `SchemaVersion == 16.0`; architecture line 524 describes the current schema-version-16 catalogues. Source search found no `creatureFiles` consumer in the C++ item/tool catalogue loader. The creature file remains authoring data validated by `Scripts/Validate-CreatureCatalogues.py`.
- **Proposed repair:** Update only the schema label to 16 and retain the explicit authoring-data/no-item-tool-loader-consumption boundary.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize documentation edits during CR13.
- **Acceptance:** `docs/creature-catalogues.md` names schema 16 and still states accurately that `creatureFiles` is not consumed by the item/tool loader. No runtime, save, network, or product-scope changes are introduced.
- **Focused validation:** Compare the edited paragraph with the manifest, loader version guard, and architecture contract; run `git diff --check`. No Unreal build is required.

## RF89 — Update stale shared attack-list guidance (P3)

- **Source:** CR13-003; `docs/07-development-setup.md:5184` (Shared attack lists); `Content/Data/GameCatalogues.json:2,71-74`; `Content/Data/GameCatalogues/attacks/attacks.json`; `Content/Data/GameCatalogues/attacks/bows.json`; `Content/Data/GameCatalogues/items/equipment/building-hammer.json`; `Source/KalmalaGameplay/Private/KalmalaToolCatalogue.cpp:104-111`; `Source/KalmalaGameplay/Private/Tests/KalmalaDamageProfileTest.cpp:136-183`; `Source/KalmalaGameplay/Private/Tests/KalmalaTwoHandedTest.cpp:46`; `docs/02-technical-architecture.md:524`.
- **Affected path:** A developer follows the setup guide to interpret the current shared attack-list catalogue and ConstructionHammer behavior.
- **Trigger:** The undated section presents schema 11, all Basic bonuses as zero, and BuildingHammer as an attack-list member. The current manifest is schema 16; authored attack data contains nonzero Basic bonuses; ConstructionHammer has no attack list.
- **Impact:** The stale claims can misdirect catalogue edits, attack-balance review, and expectations for the utility hammer. No attack-data or runtime defect was confirmed.
- **Evidence:** The manifest declares schema 16 and registers both attack files. The authored files contain Axe +10 Slash and Mace +10 Stab/+10 Crush Basic bonuses. `building-hammer.json` defines ItemId `ConstructionHammer` with zero profile damage; the C++ validator treats it as a no-attack utility and rejects attack-list membership. `KalmalaDamageProfileTest.cpp` expects Bronze Axe Basic Slash 60 and mace totals including style bonuses; `KalmalaTwoHandedTest.cpp` asserts ConstructionHammer has no combat attacks. `Scripts/Validate-CatalogueReferences.py` passed for all registered catalogue JSON and references.
- **Proposed repair:** Correct only the setup section's schema label, Basic-bonus statement, and hammer-membership statement. Preserve the current JSON, runtime behavior, and scope.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize documentation edits during CR13.
- **Acceptance:** The section accurately reflects the schema-version-16 manifest, current Basic attack bonuses, and ConstructionHammer no-attack contract, with no runtime changes.
- **Focused validation:** Compare the corrected section to the manifest, both attack files, profile, C++ validator and regression tests; run `git diff --check`. No Unreal build is required.

## RF90 — Remove paid no-benefit Construction Hammer upgrades (P2)

- **Source:** CR13-004; `Content/Data/GameCatalogues/item-level-up/costs/construction-hammer.json`; `Content/Data/GameCatalogues.json:52-61`; `Content/Data/GameCatalogues/items/equipment/building-hammer.json:4-9`; `Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json`; `Source/KalmalaGameplay/Private/KalmalaToolCatalogue.cpp:275-297`; `Source/KalmalaGameplay/Private/KalmalaToolProgressionContract.cpp:84-110`; `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:376-446,457-463`; `Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolProgressionCatalogueTest.cpp:351-368,543-559`; `docs/07-development-setup.md:5174,520-523`; `docs/04-roadmap.md:350-368`.
- **Affected path:** A player upgrades the carried ConstructionHammer at a Workbench and pays materials for a level that has no gameplay effect.
- **Trigger:** The upgrade record costs 2 Wood + 2 Stone at source level 1 and produces derived levels 2–10 with doubling costs. The ConstructionHammer profile has 100 durability and zero damage, it has no level-up reward entry, and construction only checks that hammer level is at least 1.
- **Impact:** The accepted level-two transaction consumes 2 Wood + 2 Stone and persists level 2 without increasing durability, damage, or construction capability. Additional generated levels have the same zero-effect outcome. This is a material sink in the player-facing progression flow.
- **Evidence:** `GetLevelUpReward` returns a zero-valued fallback when no item group exists; `GetLevelStats` computes unchanged base stats when rewards are zero. The server progression path accepts the validated same-item upgrade, checks its Workbench/material requirements, and commits it. `PlaceFromServer` and build-menu eligibility check only `GetCarriedToolLevel("ConstructionHammer") >= 1`. The progression catalogue test asserts a level-two entry and nine derived levels; current setup docs explicitly say Building Hammer rewards remain zero. Roadmap M9 asks every carried tool to have a level path but specifies no ConstructionHammer reward or capability beyond level one.
- **Proposed repair:** Remove the ConstructionHammer upgrade-cost record and derived levels while retaining its enabled, station-free level-one inventory recipe and construction flow. Update the focused tests and setup notes that currently expect nine upgrades. Do not invent rewards or replacement costs.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize catalogue, runtime, test, or documentation edits during CR13.
- **Acceptance:** The ConstructionHammer remains obtainable at level 1 and continues to build; no paid level 2–10 progression entries or material charges are exposed. Its acquisition, server authority, and identity behavior remain intact, and no unapproved reward/cost is introduced.
- **Focused validation:** After repair, verify the focused progression catalogue contract, inventory hammer acquisition, and level-one build authorization; run `git diff --check`. No full project build is required for this narrow correction.

## RF91 — Expose status details and ongoing recovery guidance accessibly (P2)

- **Source:** CR12-005; `Source/KalmalaUI/Private/KalmalaStatusHotbarWidget.cpp:189-194,342-400`; `Source/KalmalaUI/Private/KalmalaSurvivalStatusWidget.cpp:79-207,258-270`; `docs/04-roadmap.md:414-415,437`; `docs/02-technical-architecture.md:1813`.
- **Affected path:** A local player checks active status or weather indicators and needs to know their meaning and how the existing effect can be resolved.
- **Trigger:** An untimed Hot, Cold, or Storm entry appears in the top-right status hotbar with no status detail view.
- **Impact:** The hotbar provides an icon, hidden entry name, and optional timer, but no accessible ongoing label, recovery guidance, or existing category, intensity/stack, and source details. A player cannot make an informed response to active survival effects from this presentation. No authority or privacy defect was found.
- **Evidence:** `BuildEntries` creates Hot, Cold, and Storm with empty timer text. `SetSnapshot` gives each icon only the hidden `Entry.Name` and optional timer. The detailed `BuildStatusText` is not used by production `SetSnapshot`, which displays only `BuildOceanTravelText`; the M11 status/weather detail page was later removed while M11 retained the accessible-detail requirement. All twelve assigned source files match the CR01 fingerprint and current main.
- **Proposed repair:** Expose existing owner-visible status/weather category, intensity/stack, source, and recovery details through accessible text or a detail view. Label conditions without an expiry as ongoing with their current recovery guidance. Do not add client timing, gameplay mutation, hidden-content queries, or peer-state access.
- **Dependencies:** After CR19 review acceptance; no implementation during CR12.
- **Acceptance:** Active entries provide readable names and applicable existing category, intensity/stack, source, and recovery details without hover or colour alone. Hot, Cold, and Storm are explicitly ongoing and include truthful existing recovery guidance, with no fabricated countdown. Local text scale and high contrast remain usable; data remains limited to the owner's existing state and shared weather.
- **Focused validation:** Extend `Kalmala.UI.StatusHotbar.SnapshotAndLayout` and the rendered `Scripts/Tests/Presentation/StatusHotbar.ps1` probe to assert accessible detail for Hot/Cold/Storm and representative timed status entries. After an Editor build, inspect default and enlarged-text/high-contrast captures and verify details are available without capturing gameplay input. No test was run during this review.

## RF92 — Correct stale Wood Shield cost documentation (P3)

- Source: CR13-005; docs/02-technical-architecture.md:243-246; Content/Data/GameCatalogues/item-level-up/costs/armor/shields.json:4-21; Content/Data/GameCatalogues/recipes/stations/workbench.json:266-284; docs/02-technical-architecture.md:177-184.
- Affected path: A developer uses the architecture guide to author or validate Wood Shield acquisition and upgrade costs.
- Trigger: The earlier Bone Fragment section describes Wood Shield crafting and its level-one upgrade as Forge actions with obsolete materials and quantities. Current catalogue data uses a Workbench recipe of 10 Wood, 3 Boar Hide and 3 Deer Hide, and a Workbench level-one upgrade cost of 5 Wood, 2 Boar Hide and 2 Deer Hide.
- Impact: Following the stale contract can lead to the wrong station, ingredient IDs or quantities in content edits and verification.
- Evidence: The manifest registers the shields cost file. Its WoodShield level-one row uses Workbench with 5 Wood, 2 Boar Hide and 2 Deer Hide. The sole registered WoodShieldRecipe in workbench.json uses 10 Wood, 3 Boar Hide and 3 Deer Hide. The later architecture station note confirms that recipe and station but does not repeat the upgrade ingredients. Current assigned hashes match CR01 in the review branch and main.
- Proposed repair: Correct only the obsolete Wood Shield station and material/quantity statements in the earlier Bone Fragment section to match the current catalogue records. Preserve the later station note and do not change authored data.
- Dependencies: After CR19 review acceptance; no documentation edits during CR13.
- Acceptance: The architecture's Wood Shield acquisition and level-one upgrade descriptions match the manifest-registered Workbench recipe and cost records; no runtime or catalogue behavior changes.
- Focused validation: Compare the corrected statements with the manifest, workbench.json and shields.json; run git diff --check. No Unreal build is required.

## RF93 — Align snake collision capsules with their slithering bodies (P2)

- **Source:** CR09-001d-08d; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3645-3707,3711-3735,4731-4825; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6482-6523,6554-6576; Source/KalmalaGameplay/Private/Tests/KalmalaPopulationBodyClearanceTest.cpp:229-253; docs/creature-catalogues.md:57-70,108-115; docs/biome-enemies.md:74-78,118-132; and docs/monster-spawning.md:369-378.
- **Affected path:** Spawn placement and query collision for Viper, Death Adder, and Giant Snake.
- **Trigger:** Each snake collision capsule uses FRotator(0, 90, 0) as its relative rotation. Unreal's FRotator order makes this a yaw turn about the vertical axis, so the capsule local Up axis remains vertical rather than aligning its long axis with the snake's horizontal coil.
- **Impact:** The placement support helper treats the capsules as vertical and raises actor anchors by their half-heights: 92 cm, 112 cm, and 210 cm respectively, instead of using their horizontal body support. The low generated coils are consequently suspended about 87 cm, 106 cm, and 200 cm above flat ground; the collision volumes occupy tall vertical columns instead of following the authored slithering body. This can misplace encounter collision and other shape-based placement queries. Gameplay authority remains server-side.
- **Evidence:** BuildArchetypePresentation applies the stored capsule rotation to the active SnakeBodyCollision; TryGetPopulationBodyVerticalSupportOffset rotates FVector::UpVector and projects it onto the ground normal, so yaw-only rotation produces the full capsule half-height on level ground. The body-clearance test explicitly asserts that Giant Snake placement is at least GroundHeight plus HalfHeightCentimetres, confirming current vertical behavior instead of checking a horizontal axis. The creature catalogue calls these species low/coiled and the design requires dry-shore slithering. RF05 concerns roof tracing from actor center versus actual collision top; it does not address this capsule-axis or support error.
- **Proposed repair:** Align each snake capsule long axis with its authored horizontal body and make active collision transforms and terrain support calculations use the same orientation. Preserve server validation, current attack/reward contracts, and the original mesh silhouette.
- **Dependencies:** After CR19 review acceptance; no collision repair during CR09 review.
- **Acceptance:** Viper, Death Adder, and Giant Snake collision axes follow their low/coiled bodies, and flat-ground support reflects the transformed horizontal capsule plus root sphere rather than capsule half-height. Their meshes remain near the authored contact plane, body-clearance and roof queries account for the actual transformed volume, and server-side bite validation and authority remain intact.
- **Focused validation:** Extend KalmalaPopulationBodyClearanceTest to assert transformed capsule axes for all three species, expected support on flat and sloped ground, and placement above terrain without a tall vertical collision column. Retain focused body-clearance, roof, and snake-attack regressions after the required Editor build. No test was run during this review.

## RF94 — Clarify Opal Helm upgrade support in wearable contracts (P3)

- Source: CR13-005; docs/02-technical-architecture.md:533; docs/07-development-setup.md:5216; Content/Data/GameCatalogues/item-level-up/costs/armor/helmets.json:30-46; Source/KalmalaGameplay/Private/KalmalaOpalHelm.cpp:8-39,59-86; Source/KalmalaGameplay/Private/Tests/KalmalaOpalHelmTest.cpp:13-74; docs/a9-acceptance.md:28.
- Affected path: A developer or reviewer determines whether a wearable upgrade cost has a supported server transaction.
- Trigger: The architecture and setup guides say wearable upgrade costs are data-only and gear upgrade transactions remain pending, without identifying the implemented Opal Helm exception.
- Impact: The blanket wording can mislead implementation, review and acceptance work about the current Opal Helm Forge/Anvil upgrade path.
- Evidence: The dedicated server path reads the registered OpalHelm upgrade row, scales its material cost, validates the exact equipped item and matching Forge level, and commits payment and level through the server-owned inventory transaction. KalmalaOpalHelmTest checks station rejection, payment, level/stat change, wear and repair; the A9 acceptance summary records that route as passed. Wearable rows still do not create carried-tool progression entries, and other gear-upgrade transactions remain pending.
- Proposed repair: Clarify the Opal Helm exception in the architecture and setup contracts while retaining the pending status for other wearable upgrades. Do not change runtime behavior or catalogue data.
- Dependencies: After CR19 review acceptance; no documentation edits during CR13.
- Acceptance: Both guides accurately describe the implemented OpalHelm-specific Forge/Anvil route and the remaining gear integration scope; runtime and catalogue behavior remain unchanged.
- Focused validation: Compare both corrected passages with the OpalHelm cost row, server path, focused test and A9 acceptance record; run git diff --check. No full build is required.

## RF95 — Clarify wearable item count versus slot count (P3)

- Source: CR13-005; docs/game-features.md:145; docs/01-game-design.md:168; docs/02-technical-architecture.md:533; Content/Data/GameCatalogues.json:23-28; Content/Data/GameCatalogues/items/equipment/armor/{boots,capes,helmets,leggings,shields,tunics}.json; docs/05-decision-log.md:58.
- Affected path: A reader uses the feature summary to understand the current scope of authored wearable content.
- Trigger: The undated feature summary says “Six gear items” and separately describes six session-only slots, leaving the six-item statement ambiguous between six item definitions and six equip categories.
- Impact: A reader may believe the current catalogue has only six wearable definitions, while the six manifest-registered armor files contain 16 unique wearable ItemIds across the six supported GearSlot values.
- Evidence: The current docs/game-features.md SHA-256 matches CR01, and line 145 appears in the feature summary without a historical date. The manifest registers the six armor files; their SHA-256 values match their CR01 coverage fingerprints, and the parsed records contain 16 unique ItemIds. The game-design and architecture contracts identify six wearable slots. The separately dated 2026-10-15 decision log describes six initial items, not the current total. The docs/game-features row remains assigned to CR16-003 and the armor profile rows remain assigned to CR13-011; neither is marked reviewed by this finding.
- Proposed repair: Clarify that the six count refers to wearable slots or categories, and distinguish that from the 16 current wearable item definitions; alternatively remove the item-count wording. Do not change runtime or catalogue data.
- Dependencies: After CR19 review acceptance; no documentation edits during CR13.
- Acceptance: The feature summary no longer implies that only six wearable item IDs are authored and accurately distinguishes the six slots from the current definitions; no runtime or catalogue behavior changes.
- Focused validation: Compare the corrected wording with the registered armor files and GearSlot contract; run git diff --check. No Unreal build is required.

## RF96 — Restore expanded-map pan and recenter navigation (P3)

- **Source:** CR12-006; current main-checkout paths `E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:205-210,1502-1509,1625-1639,1641-1686`, inspected read-only; `docs/04-roadmap.md:972-973,989-990,1006-1013`; `docs/02-technical-architecture.md:306,308`; `docs/07-development-setup.md:3452-3453,3483-3487`.
- **Affected path:** A local player uses the expanded map to inspect an area or place a personal pin or temporary ping outside the current player-centred view.
- **Trigger:** The player attempts to navigate the map with mouse, keyboard, or controller, or to recenter after moving the view.
- **Impact:** The map remains centred on the owning pawn and supports zoom only, so players cannot inspect or mark remote map locations through the UI. This misses M14's pan/recenter and keyboard/controller navigation contract. No authority or privacy defect was found.
- **Evidence:** `Recenter` only calls `RecenterOnOwningPlayer`; `ZoomAtMapCentre` calls it again on every zoom. `NativeOnMouseMove` only updates hover, and `NativeOnKeyDown` routes arrows/D-pad to marker-filter or pin-style navigation without a map-pan path. The M14 roadmap and architecture require pan/recenter. The later M14 setup notes describe the current no-pan behavior, but no decision-log waiver was found; the rendered map checks cover zoom, player-centred view, and focus without pan/recenter. All eight current main source hashes match the CR01 fingerprints. The main-local `KalmalaWorldListWidget.cpp` edits and relevant `docs/startmenu.md`/`docs/07-development-setup.md` edits were inspected read-only and not copied.
- **Proposed repair:** Restore a user-facing pan and recenter path for the expanded map while keeping view input local and preserving opaque fog, local pins, and the temporary server-validated ping boundary.
- **Dependencies:** After CR19 review acceptance; no implementation during CR12.
- **Acceptance:** Players can pan and explicitly recenter the expanded map with keyboard/controller navigation and pointer input. Pan, zoom, and recenter retain modal input ownership and do not reveal undiscovered terrain; pins remain local, pings remain temporary and server-validated, and no peer map data is exposed.
- **Focused validation:** Extend `Kalmala.UI.WorldMap.LocalPresentation` and `Scripts/Tests/World/WorldMap.ps1` to verify that pan/recenter changes the actual view with keyboard, controller, and pointer input at supported zoom/resolution settings. Recheck focus ownership, fog/pin/ping privacy, and rendered host/client cases after an Editor build. No test was run during this review.

## RF97 — Preserve credited Boar and Deer rewards when inventory lacks capacity (P2)

- **Source:** CR09-001d-08e; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:478-596,5328-5446; Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:426-433; Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-291; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7607-7608,8649-8655; Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeBehaviourTest.cpp:18-20; docs/01-game-design.md:63-65; docs/creature-catalogues.md:39-50; docs/monster-spawning.md:100-117; and docs/biome-enemies.md:153-162.
- **Affected path:** Credited Boar and Deer defeats, including the optional server-rolled Deer Sinew reward.
- **Trigger:** The attacker's inventory has room for only some or none of the sequential BoarMeat/BoarHide or DeerMeat/DeerHide/Sinew grants.
- **Impact:** When the defeat save succeeds, the stable wildlife spawn is depleted even when a direct inventory grant fails. The one-shot reward guard prevents retry, and the bundle can be partially granted. Items that did not fit are permanently lost after capacity is later freed.
- **Evidence:** DefeatServer has no Boar/Deer Record*DefeatReward branch; their accepted defeat broadcasts OnDefeated and the bound GameMode::RecordDefeatedSpawn handler marks the stable ID and attempts to save it. ApplyDefeatedState calls GrantDefeatReward. That function sets bDefeatRewardProcessed before obtaining the inventory result, then calls TryGrantFromServer separately for BoarMeat and BoarHide, or DeerMeat and DeerHide plus its Sinew roll, without checking return values or persisting a pending claim. TryGrantFromServer returns false when the candidate BuildGrant fails; BuildGrant reports Pack output capacity reached without committing the candidate. The general monster-spawning contract requires durable owner-bound wildlife claims and retries when capacity is available. Existing focused coverage tests the Sinew roll helper but does not execute this production grant path with constrained capacity. RF17, RF20, RF29, RF64 and RF69 concern distinct reward paths and are not duplicates.
- **Proposed repair:** Route credited Boar and Deer rewards through an authoritative durable owner-bound claim, preserve the single Deer Sinew outcome, and deliver the complete bundle atomically once capacity permits. A failed grant must remain recoverable.
- **Dependencies:** After CR19 review acceptance; no gameplay repair during CR09 review.
- **Acceptance:** For credited Boar and Deer defeats, full or partial inventory capacity preserves one stable owner-bound claim and grants no partial bundle. Freeing capacity or reconnecting/restarting delivers the exact complete bundle once, without rerolling Deer Sinew; the defeated spawn remains depleted and duplicate defeat/reward attempts cannot duplicate loot.
- **Focused validation:** Add a production-path Boar/Deer defeat regression covering zero, partial and sufficient capacity, durable claim identity, whole-bundle retry, the stored Sinew outcome, reconnect/restart and exact-once delivery. Run after the required Editor build. No runtime test was run during this review.

## RF98 — Preserve local input overrides in settings automation (P3)

- **Source:** CR12-008; `Source/KalmalaUI/Private/Tests/KalmalaSettingsWidgetTest.cpp:127-136`; current main-checkout `E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaSettingsWidget.cpp:1256-1296,1501-1517`, inspected read-only; `docs/07-development-setup.md:1810`.
- **Affected path:** A local profile contains custom keyboard, controller, alternate-key, or axis-pair mappings before `Kalmala.UI.Settings.LocalPresentation` runs.
- **Trigger:** Normal execution reaches `RestoreDefaultInputBindings` after changing Interact to F, or an assertion records failure after that mutation and execution continues through the same restore path.
- **Impact:** The automation permanently removes and flushes all local input override keys, losing custom mappings that may be needed for accessibility. This is a local settings side effect; no gameplay authority or peer-privacy defect was found.
- **Evidence:** The test checks the default E binding, changes Interact to F, calls `RestoreDefaultInputBindings`, and then checks E; assertion failures are accumulated, and no cleanup restores the pre-test bindings. `SetLocalInputBinding` writes to `GGameUserSettings.ini`; the restore routine removes the primary/alternate keyboard, controller, and axis-pair override keys for every local input definition, flushes the file, and applies defaults. The setup command specifies a temporary `-UserDir`, but the automation test itself does not enforce isolation.
- **Proposed repair:** Snapshot all affected local input entries and restore their exact prior values with unconditional scope cleanup; run the automation with a disposable `-UserDir` while retaining its default-reset assertion.
- **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
- **Acceptance:** With representative nondefault keyboard, alternate-key, controller, and axis-pair mappings seeded, both the normal test path and a controlled assertion-failure path leave all seeded values unchanged while retaining coverage of the default-reset behavior.
- **Focused validation:** Run the automation with isolated settings and seeded bindings, compare every binding entry before and after both paths, and verify the reset assertion. No test was run during this review.

## RF99 — Correct stale Iron Axe and Bow setup claims (P3)

- Source: CR13-006; docs/07-development-setup.md:2315; Content/Data/GameCatalogues.json; Content/Data/GameCatalogues/item-level-up/costs/weapons/{axes,bows}.json; Content/Data/GameCatalogues/recipes/stations/forge.json; Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json; docs/01-game-design.md:3-25; docs/05-decision-log.md; Source/KalmalaGameplay/Private/KalmalaBowCombat.cpp; Source/KalmalaGameplay/Private/KalmalaArrowProjectile.cpp; Source/KalmalaGameplay/Private/Tests/KalmalaBowDrawTest.cpp.
- Affected path: A developer follows the current catalogue verification paragraph to author or review weapon costs and Bow behavior.
- Trigger: The schema-16 setup paragraph says Iron Axe acquisition and upgrade use Densewood and says Bow has no damage reward because shooting remains unimplemented. The manifest-backed Forge acquisition and axes.json both instead use 4 Iron + 3 Lightwood. Current server Bow draw/release and arrow-damage paths are implemented.
- Impact: The stale material statement can lead to an incorrect Iron Axe cost or regression; the Bow statement falsely describes current behavior. Bow's authored zero damage reward remains accurate.
- Evidence: axes.json and the Forge acquisition row both author 4 Iron + 3 Lightwood, matching the 2026-10-19 decision. The updated Bow shot design contract, server combat/projectile sources and focused Bow draw test cover draw, release and projectile damage. item-stats.json retains no Bow damage gain. RF61 is scoped to the feature-summary wording; this finding concerns the separate setup paragraph. The setup-doc coverage row remains pending and is not marked reviewed here.
- Proposed repair: Correct only the setup paragraph's Iron Axe material and Bow implementation statements. Retain Densewood for Iron Knife, Spear and Mace, retain Bow's zero damage reward, and do not change runtime or catalogue data.
- Dependencies: After CR19 review acceptance; no documentation edits during CR13.
- Acceptance: The setup paragraph states Iron Axe acquisition and upgrade use 4 Iron + 3 Lightwood and accurately describes implemented Bow shooting while retaining its zero damage reward; other Iron weapon costs remain accurate and runtime/catalogue behavior is unchanged.
- Focused validation: Compare the corrected paragraph with the manifest, assigned axes/bows costs, Forge acquisition, Bow shot design contract and server implementation/test references; run git diff --check. No Unreal build is required.

## RF100 — Reconcile the knife, pickaxe, and mace reward contract (P3)

- Source: CR13-007; docs/02-technical-architecture.md:524,1042; docs/07-development-setup.md:5174; docs/05-decision-log.md; Content/Data/GameCatalogues.json:68-70; Content/Data/GameCatalogues/item-level-up/rewards/item-stats.json; Source/KalmalaGameplay/Private/KalmalaToolCatalogue.cpp:35-57,275-303; Source/KalmalaGameplay/Private/Tests/KalmalaM9ToolProgressionCatalogueTest.cpp; Source/KalmalaGameplay/Private/Tests/KalmalaItemCatalogueTest.cpp.
- Affected path: A developer reviews or changes shared level-up rewards using the current architecture, decision, and development-setup guidance.
- Trigger: The manifest-backed reward file gives Knives +50 maximum durability, +4 Slash and +4 Stab; Pickaxes +10 maximum durability, +3 Crush and +3 Pierce; and Stone/Bronze/Iron Maces +50 maximum durability and +5 Crush. The architecture and setup passages instead say Knives gain +50/+5/+5 and Pickaxes/Maces gain +50/+5 Crush/+5 Pierce; architecture also says the three tiered Maces currently gain zero rewards.
- Impact: Current data and written contracts disagree, leaving authors without a verified balance target and risking inconsistent stat changes.
- Evidence: The manifest registers item-stats.json as the reward source; its CR01-matching contents contain 15 groups and 41 unique, known IDs with bounded values. Git history shows the original file held +50/+5/+5 Knife and Pickaxe values and +50/+5 Crush/Pierce Mace values; commit 68a0a8d9 on 2026-10-15 changed those existing groups to the current values. The schema-16 architecture summary was updated on 2026-10-23 but retains the earlier values, and the decision log has no entry documenting the 2026-10-15 retune. The 2026-10-21 delegated A8b choice covers newly added A8b rewards and does not establish the intended legacy values. Runtime consumes the registered file and validates identity, type, and bounds; focused tests do not assert every disputed row.
- Proposed repair: Confirm the intended legacy reward values from an authoritative decision, then align the architecture and setup descriptions with that approved contract. Change item-stats.json only if an approved balance target directs it; do not invent values.
- Dependencies: After CR19 review acceptance and confirmation of the intended reward contract; no repair during CR13.
- Acceptance: One approved value set is recorded in the authoritative contract, and the manifest-backed data and both guides agree with it. No unsupported reward or catalogue change is introduced.
- Focused validation: Compare the reconciled passages and data with the manifest, runtime reward validation/formula, decision record, and focused test references; run git diff --check. No Unreal build is required.

## RF101 — Reconcile the PotatoSeed use description (P3)

- Source: CR13-008; Content/Data/GameCatalogues/items/crops.json; docs/02-technical-architecture.md:512,946-953; docs/07-development-setup.md:144-147,2002-2018; Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:225-245; Source/KalmalaGameplay/Private/Tests/KalmalaItemCatalogueTest.cpp:390; Source/KalmalaGameplay/Private/Tests/KalmalaM9SourceAcceptanceTest.cpp:937-1000.
- Affected path: A player or developer reads the PotatoSeed item detail or tries to use the registered seed in current Garden Plot progression.
- Trigger: The registered PotatoSeed definition says "Seeds to grow potatoes," but the approved Garden Plot contract uses one Potato tuber for planting and returns Potatoes; the A0 potato source grants Potatoes and explicitly grants no PotatoSeed. No production planting or acquisition route uses PotatoSeed.
- Impact: The player-facing item definition implies a supported use that does not exist. If the item is introduced through a future grant or test setup, it has no current planting path.
- Evidence: GameCatalogues.json registers crops.json. FKalmalaGardenPlotState maps PotatoFood planting payment and harvest to Potato; architecture and setup guidance say PotatoSeed remains unused and no A0 source adds it. The M9 source acceptance checks that potato harvest leaves the PotatoSeed quantity unchanged. The item-catalogue test only asserts the registered description; no production route consumes or grants PotatoSeed.
- Proposed repair: Reconcile the player-facing item definition and availability with the approved deferred-use contract. Do not invent a seed source or planting mode; any such gameplay route requires separate scope approval. Do not change data during CR13.
- Dependencies: After CR19 review acceptance; separate scope approval is required before adding a PotatoSeed source or planting behavior.
- Acceptance: The registered item no longer implies a currently supported acquisition/planting path unless that route is separately approved and implemented; the deferred-use contract remains explicit and no unsupported source is added.
- Focused validation: Compare the item definition with the manifest, Garden Plot payment/harvest helpers, A0 source and M9 acceptance; run catalogue/reference validation and git diff --check after the repair. No Unreal build is required for a text-only correction.

## RF103 — Restore traceability for legacy crop and food icon provenance (P3)

- Source: CR13-008; docs/icon-provenance.md; Scripts/Data/catalogue-icon-manifest.csv; Content/Kalmala/UI/Source/IconOriginals/; Content/Kalmala/UI/Source/Icons/; Content/Kalmala/UI/Icons/Items/.
- Affected path: A maintainer needs to trace the origin and approved visual constraints before revising or reimporting legacy crop/food item icons.
- Trigger: Seventeen current crop/food identities (Carrot, Potato, Rutabaga, Onion and their four seed items; BoarMeat, DeerMeat, CookedBoarMeat, CookedDeerMeat, HearthBroth, MeatStew, RootVegetableSoup, RoastedRootVegetables and DeerRootRoast) have retained original PNG files and icon-manifest/runtime mappings, but no generation record or provenance reference in the current or historical repository icon-provenance sources. Their original PNGs contain no embedded text provenance.
- Impact: The artwork and mappings exist, but the repository cannot trace the prompts/source references or visual constraints for these originals, making later revisions difficult to audit. This finding concerns provenance records only; it does not identify missing artwork or mappings.
- Evidence: All seventeen IconOriginals PNGs and manifest rows exist; Validate-CatalogueReferences confirms canonical runtime icon mappings. The icon-provenance guide says it retains former per-item and per-batch generation records but has no entries for these identities; history search found no matching record in the consolidated or prior icon documents, and the original PNGs have no text metadata chunks.
- Proposed repair: Recover and retain authoritative generation records if available. If they cannot be recovered, state the provenance limitation accurately; do not invent prompts or claim a source-reference history. No icon or game-documentation edits during CR13.
- Dependencies: After CR19 review acceptance; any replacement art or new icon workflow requires its own approved scope.
- Acceptance: Each affected original has an auditable generation/provenance record or an explicit documented unknown-provenance disposition. Existing artwork and mappings remain intact unless separately authorized.
- Focused validation: Check each item identity against its retained original, prepared image, imported texture, icon manifest and provenance disposition; run Scripts/Validate-UIIcons.ps1 -Category Catalogue for the affected IDs and git diff --check. No Unreal gameplay build is required.

## RF104 — Provide captions for designed vocal audio (P3)

- **Source:** CR12-011; `docs/01-game-design.md:149,153`; `Source/KalmalaUI/Public/KalmalaAmbientAudioSubsystem.h`; `Content/Kalmala/Audio/Source/`; searches of `Source`, `Config`, and `Content`.
- **Affected path:** A player encounters designed singing/vocal audio in the soundscape.
- **Trigger:** Approved vocal audio plays without its approved caption text and timing being presented.
- **Impact:** Players who cannot hear or rely on audio lack the subtitle support required from the vertical slice. No vocal asset or current caption path was found, so this does not allege a failure involving existing vocal content.
- **Evidence:** The design names distant singing and requires subtitles from the vertical slice. The assigned audio-subsystem header exposes ambience, status, movement, and accepted-result cues but no caption surface. `Content/Kalmala/Audio/Source` contains no vocal/singing asset, and searches across Source, Config, and Content found no caption/subtitle runtime path. Caption content/timing acceptance for future vocal content is not defined.
- **Proposed repair:** Define and implement a readable caption path for approved vocal audio only after caption content and timing are approved. Do not invent caption text, cues, or a taxonomy during repair.
- **Dependencies:** After CR19 review acceptance and approval of caption content/timing; no repair during CR12 review.
- **Acceptance:** Before vocal audio ships, its approved caption text and timing are defined and every approved vocal cue presents them in a readable path that respects existing accessibility text-scale and contrast controls. No unapproved vocal cue or caption text is introduced.
- **Focused validation:** After the vocal asset and caption contract are approved, add focused automation verifying each approved cue has matching caption data and that the caption is presented for the cue; inspect rendered captions at default and enlarged text scale/high contrast. No test was run during this review.

## RF105 — Add the required adjustable camera-shake setting (P3)

- **Source:** CR12-011; `docs/01-game-design.md:153`; `docs/02-technical-architecture.md:2434-2437`; searches of current settings source and Config.
- **Affected path:** A local player configures the promised camera-shake accessibility option in Settings.
- **Trigger:** The player seeks the 0–100% camera-shake control, but Accessibility has no such slider. No current camera-shake effect was found.
- **Impact:** The required local adjustable camera-shake setting is unavailable. This finding concerns the missing settings contract only; it does not allege or propose a new camera-shake effect.
- **Evidence:** Game design requires adjustable camera shake from the vertical slice onward; the architecture retains an unchecked `Camera shake — slider 0-100%` candidate. Searches of Source, Config, and Content found no setting, camera-shake API/effect, or saved local value. Existing settings documentation contains no camera-shake setting.
- **Proposed repair:** Add a local, focusable 0–100% Accessibility slider with a visible value and local persistence. Any separately approved camera-shake effects must consume that value; do not invent an effect or default.
- **Dependencies:** After CR19 review acceptance and any product approval needed for effect/default semantics; no repair during CR12 review.
- **Acceptance:** The Accessibility slider accepts only 0–100%, exposes a visible value to keyboard/controller users, and persists only in local settings without an RPC, world/player save field, or replicated property. Any approved camera-shake effect reads the preference; no effect is added without separate approval.
- **Focused validation:** Extend Settings accessibility automation to verify focusability, bounds, visible value, local persistence, and no gameplay RPC or replicated state; if an approved effect exists, verify it observes the setting. No test was run during this review.

## RF106 — Reconcile Smelter and material-category guidance (P3)

- Source: CR13-009; Content/Data/GameCatalogues/items/buildItems/other.json; Content/Data/GameCatalogues/items/materials/ore.json; Content/Data/GameCatalogues/items/materials/metal.json; docs/01-game-design.md:172; docs/02-technical-architecture.md:510; docs/07-development-setup.md:5210; Source/KalmalaGameplay/Private/KalmalaStorageInteraction.cpp:409-411; Source/KalmalaGameplay/Private/Tests/KalmalaA10FreshStartBronzeAndIronTest.cpp:389-470.
- Affected path: A developer consults the Smelter and material-category descriptions to determine current ore, bar and refining routes.
- Trigger: The design summary describes only Iron Ore plus Coal producing Iron, and the setup catalogue summary says Ore contains only Stone and Metal only Iron. The schema-16 catalogue registers Copper, Tin and Iron ores and bars; the current server accepts all three Smelter paths.
- Impact: Current authored resource and processing paths are understated, leaving conflicting guidance for implementation and review.
- Evidence: The manifest registers ore.json, metal.json and the Smelter buildable. KalmalaStorageInteraction.cpp maps CopperOre, TinOre and IronOre to distinct server processing modes; the A10 test exercises matching Copper, Tin and Iron bars. The architecture contract at line 510 documents all three.
- Proposed repair: Reconcile the game-design Smelter sentence and setup material-category list with the current registered data and architecture contract. Do not alter recipes, catalogue data or runtime behavior during CR13.
- Dependencies: After CR19 review acceptance; no data or gameplay changes during CR13.
- Acceptance: Current guidance lists the registered ore/bar categories and all three accepted Smelter outputs without changing the existing processing contract.
- Focused validation: Compare the corrected passages with GameCatalogues.json, ore.json, metal.json, server Smelter handling and the focused A10 test; run Scripts/Validate-CatalogueReferences.py and git diff --check. No Unreal build is required.

## RF107 — Clarify the Grinding Stone repair scope (P3)

- Source: CR13-009; Content/Data/GameCatalogues/items/buildItems/grinding-stone.json; docs/04-roadmap.md:364; docs/07-development-setup.md:4272-4274; Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:700-765.
- Affected path: A player reads the Grinding Stone description before using its in-world Repair All action.
- Trigger: The description promises repair of all worn items, while the server repair path validates the Grinding Stone and repairs only entries in the owner's carried-tool list.
- Impact: The description can imply wearable gear or other condition-bearing inventory items are repaired when the action does not touch those systems.
- Evidence: RepairAllToolsFromServer calls CommitRepairAllToolsFromServer, which passes Character->CarriedTools to BuildServerRepairAll and persists that carried-tool array. The roadmap and setup contract also define the action as repairing carried tools.
- Proposed repair: Narrow the Grinding Stone description to repairing all carried tools at once; do not add gear repair behavior.
- Dependencies: After CR19 review acceptance; no catalogue edits during CR13.
- Acceptance: The description accurately states the supported carried-tool repair scope and does not promise wearable or inventory-wide repair.
- Focused validation: Compare the corrected description with the server repair path, carried-tool validation and roadmap/setup contract; run git diff --check. No Unreal build is needed for the text-only correction.

## RF108 — Correct the Garden Plot farming status (P3)

- Source: CR13-009; docs/01-game-design.md:172; docs/game-features.md:145; docs/02-technical-architecture.md:512; Content/Data/GameCatalogues/items/buildItems/other.json; Source/KalmalaGameplay/Private/KalmalaCultivationInteraction.cpp; Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp; Source/KalmalaGameplay/Private/Tests/KalmalaPlantingTrowelTest.cpp.
- Affected path: A player or developer consults the current feature summary or game-design status to learn whether Garden Plot cultivation is implemented.
- Trigger: The design and feature summaries say farming is unimplemented, while the current server supports Garden Plot planting, active-time growth and mature-crop harvest.
- Impact: Current Garden Plot behavior is omitted from status guidance and is easy to mistake for deferred work.
- Evidence: The registered GardenPlot buildable and direct recipe are present; the architecture contract documents Planting Trowel payment, saved crop mode, active-time growth and harvest. Server cultivation handling and the focused planting test cover the flow. PotatoSeed remains intentionally unused under the separate RF101 handoff.
- Proposed repair: Update or date-scope both status statements to reflect the implemented A7 Garden Plot flow and retain the explicit PotatoSeed limitation. Do not change gameplay or add crop behavior during CR13.
- Dependencies: After CR19 review acceptance; documentation only.
- Acceptance: Both passages distinguish implemented Garden Plot cultivation from any broader deferred farming scope and do not imply PotatoSeed has a supported route.
- Focused validation: Compare the corrected status with the registered GardenPlot definition, cultivation/harvest server path, architecture contract and focused test source; run git diff --check. No Unreal build is required.

## RF109 — Correct the current buildable recipe count (P3)

- Source: CR13-009; docs/02-technical-architecture.md:506; docs/07-development-setup.md:113-116; Content/Data/GameCatalogues.json; Content/Data/GameCatalogues/items/buildItems/; Content/Data/GameCatalogues/recipes/building.json; Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp.
- Affected path: A maintainer uses architecture and setup counts to assess buildable catalogue and construction-recipe coverage.
- Trigger: The current architecture text says there are 19 placement recipes, while the schema-16 manifest registers all seven buildItems files, which contain 22 unique buildables; the building recipe file contains 22 unique outputs. The setup's 18-object statement is under a dated 2026-10-18 heading and may be a historical snapshot.
- Impact: An unqualified current count understates the registered buildable and direct-recipe coverage.
- Evidence: All seven assigned buildItems files are registered; they contain 22 unique IDs, and recipes/building.json has 22 outputs. The catalogue reference validator passes and the loader routes direct construction outputs to BuildableOutput.
- Proposed repair: Update the current architecture count to 22. Retain the setup figure only if it is clearly treated as an as-of-2026-10-18 snapshot; otherwise update its current count. Do not alter recipes or buildable data.
- Dependencies: After CR19 review acceptance; no catalogue or runtime edits during CR13.
- Acceptance: No current unqualified count conflicts with the 22 registered buildables/recipes; any retained 18-object statement is clearly historical.
- Focused validation: Parse the manifest, buildItems and building recipe outputs; run Scripts/Validate-CatalogueReferences.py and git diff --check. No Unreal build is required.

## RF110 — Quote paths passed to the icon-import Editor (P3)

- **Source:** CR14-001; Scripts/Import-CatalogueIconBatch.ps1:100-108; Scripts/Import-StatusIconAssets.ps1:110-118.
- **Affected path:** A maintainer runs either icon import when the repository or Windows temporary directory contains spaces.
- **Trigger:** Both PowerShell wrappers pass a string array with project, UserDir, log, and Python script paths to Start-Process -ArgumentList without escaped quotes.
- **Impact:** Start-Process joins array entries into one command line, and Windows splits unquoted path values containing spaces. Unreal receives truncated or extra arguments, so the import cannot reach its expected completion markers. No gameplay authority or runtime behavior is affected.
- **Evidence:** The path-valued array entries at the cited lines contain spaces when the repository or temp path does, but have no escaped quote characters. The official Microsoft documentation says Start-Process joins argument arrays with spaces and requires escaped double quotes around values containing spaces: https://learn.microsoft.com/powershell/module/microsoft.powershell.management/start-process?view=powershell-5.1#-argumentlist. No import was executed.
- **Proposed repair:** Use correctly escaped quoting for every path-valued argument or another explicit argument API; preserve the current flags, timeouts, and completion checks.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** Both import wrappers pass each project, script, UserDir, and log path intact when the repository and temp roots contain spaces, while their current short-path flows still succeed.
- **Focused validation:** Capture child argv or run both wrappers from a disposable path containing spaces and verify expected markers and package outputs after the required Editor setup.

## RF111 — Reject generated outputs that exceed MAX_PATH (P3)

- **Source:** CR14-001/CR14-002/CR14-005; Scripts/Create-MenuMap.py:9,46; Scripts/Export-WorldMaps.ps1:3,11-12,22,42; Scripts/Generate-AudioSources.ps1:12-26,1311-1316; Scripts/Setup-PrototypeEnvironment.py:8-11,68-69; `Source/KalmalaEditor/Private/CreateLightingComparisonFixtureCommandlet.cpp:48-55,62-65,90-101,183-190`; `CreatePrototypeMapCommandlet.cpp:20-35,54-59`; `CreateWorldMaterialsCommandlet.cpp:87-113,204-206,243-310,314-340,459-461,471-510,520-587,597-628,782-828`; `ExportWorldMapsCommandlet.cpp:96-102,156-172,184-192`; `RenderWorldGenerationVisualizationCommandlet.cpp:45-57,136-142,297-307,573-575,645-694`.
- **Affected path:** A developer creates the Home/prototype maps, updates L_Prototype lighting, generates world-material or test assets, exports world-map PNGs, renders world-generation previews or numeric samples, or generates audio when a full destination reaches 260 characters.
- **Trigger:** Create-MenuMap saves the fixed /Game/Kalmala/Maps/Menu/L_Menu asset without checking its resolved .umap path. Setup-PrototypeEnvironment loads and saves /Game/Kalmala/Maps/Prototype/L_Prototype without checking its resolved .umap path. Export-WorldMaps accepts any Output directory and creates it without a length check before launching Unreal and reading Output/LastRender.txt. Generate-AudioSources derives WAV destinations from the checkout root and creates them without checking their expanded paths. The five listed Editor commandlets also resolve fixed project package paths or accept caller-selected destinations without checking the full generated path before directory creation, package mutation/save, or file writes; ExportWorldMaps creates a temporary `.tmp` sibling for each PNG. RenderCreaturesCommandlet and RenderMenuCommandlet normalize their output paths and reject lengths of 260 or more.
- **Impact:** On Windows without long-path support, a destination at or beyond 260 characters can fail during map load/save, directory creation, commandlet output, or WAV writing, sometimes after generation or in-memory map edits have begun. This conflicts with the explicit project path contract and leaves error behavior to the OS or Unreal tools.
- **Evidence:** The original four destination writers lack a full path bound. A 240-character checkout root leaves Kalmala.uproject at 257 characters while the Home map `.umap` path reaches 278. A 212-character root leaves Kalmala.uproject at 229 while the L_Prototype `.umap` path reaches 260. The five new commandlets either resolve package names directly to filenames or write to an arbitrary Output path, with no corresponding 260-character guard before output-directory creation, map/asset save, or preview write. By comparison, RenderCreatures and RenderMenu convert outputs to absolute paths and reject `Len() >= 260` before creating their output directory. Export-UITextCrop, both icon importers, and Package-And-Run also have relevant explicit path checks.
- **Proposed repair:** Resolve both map file paths, normalize the exporter and preview directories and every concrete generated destination (including temporary `.tmp` files), and reject any path at 260 characters or longer before loading/saving maps or assets, creating directories, writing previews, or starting Unreal/audio generation.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** Destinations below 260 remain supported. Home/prototype map paths, generated material and fixture packages, configured exporter directories and their temporary files, numeric-sample and world-generation preview outputs, and checkout-derived WAV paths at 260 or more reject before loading, saving, or creating output/launching generation, without relying on Windows long-path support.
- **Focused validation:** Exercise 259/260-character boundaries for fixed project assets, exporter and preview outputs, temporary PNG siblings, sample outputs, and audio destinations without creating output for rejected paths; verify short defaults remain valid. No generation or map setup was run during review.

## RF113 — Make Inventory actions reachable with keyboard and controller focus (P2)

- **Source:** CR12-012; current main-checkout E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaInventoryMenuWidget.cpp:391-406,604-613,1406-1412 and E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaInventoryGridWidget.cpp:346-380, inspected read-only; docs/02-technical-architecture.md:2583-2618; docs/04-roadmap.md:417,737-760.
- **Affected path:** A local player opens Inventory and needs to equip or return gear, eat food, repair a tool, or prefer an arrow stack with keyboard/controller input.
- **Trigger:** Inventory opens with keyboard focus on the pack grid; the player tries Tab or arrow/D-pad navigation to reach a visible gear or action button.
- **Impact:** Tab closes Inventory, while arrow/D-pad input is consumed inside the grid and does not move focus to its buttons. Keyboard and D-pad users cannot reach these existing actions without pointer input. No authority or peer-privacy defect was found.
- **Evidence:** Open selects GridView as the focus target and explicitly sets user and keyboard focus. NativeOnPreviewKeyDown closes on Tab; NavigateInventoryMenu declines routing while GridView has focus. UKalmalaInventoryGridWidget::NativeOnKeyDown consumes arrow/D-pad for cell movement and Enter/Gamepad FaceButton Bottom for slot moves. The existing gear, Eat, Repair, and Prefer Arrows controls are button click handlers with no keyboard/D-pad focus route from the grid. The architecture says Tab reaches labelled buttons, and the M12 roadmap requires accessible existing actions and keyboard/controller navigation.
- **Proposed repair:** Add a visible keyboard/controller focus path from the pack grid through applicable gear controls and existing action buttons, and back to the grid. Preserve a clear menu-close path, text-entry behavior, and the existing server-validated actions; focus or selection alone must not invoke an action.
- **Dependencies:** After CR19 review acceptance; no implementation during CR12.
- **Acceptance:** Keyboard and controller users can move focus from the grid to each applicable gear/action button, see the focused state, activate the existing action, and return to the grid. Tab/I close and text-entry behavior remain usable; closing restores input without replaying an action. No new gameplay request, authority path, or peer-visible state is added.
- **Focused validation:** Extend Kalmala.UI.InventoryMenu.Selection and the rendered Inventory acceptance to exercise keyboard/D-pad focus traversal and activation for gear, Eat, Repair, and Prefer Arrows controls, including focus return, visible focus, and close/input restoration. Run after an Editor build and inspect standard and enlarged-text/high-contrast host/client cases. No test was run during this review.

## RF114 — Correct stale current armor status in the inventory architecture (P3)

- Source: CR13-011; docs/02-technical-architecture.md:1479-1489; docs/01-game-design.md:168-170; docs/02-technical-architecture.md:1136; docs/07-development-setup.md:1404-1410; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:322-358; Source/KalmalaGameplay/Private/KalmalaDamageProfile.cpp:88-104; Source/KalmalaGameplay/Private/Tests/KalmalaDamageProfileTest.cpp:49-89; Content/Data/GameCatalogues/items/equipment/armor/{boots,capes,helmets,leggings,shields,tunics}.json.
- Affected path: A developer or reviewer uses the architecture's inventory and combat notes to assess current wearable Armor behavior.
- Trigger: The undated shared-inventory section says current Armor remains 0 because equipped defenses are not applied, and says Armor items/equipment rules require separate gameplay work.
- Impact: Readers may treat wearable defenses as unavailable or duplicate already implemented work.
- Evidence: Together, the six schema-16 armor files contain 16 registered profiles with positive Armor values. The server wildlife-hit path sums non-depleted equipped Armor and passes it to the damage resolver; the resolver subtracts it from physical damage. Current game-design/architecture contracts and focused damage-profile assertions describe the same behavior.
- Proposed repair: Update or remove only the obsolete Armor-zero and pending-equipment statements to match the current server calculation. Retain the still-applicable weight/encumbrance limitations. Do not alter runtime or catalogue data.
- Dependencies: After CR19 review acceptance; no runtime or catalogue edits during CR13.
- Acceptance: Current architecture no longer states that wearable defenses are unapplied or that Armor equipment remains unimplemented; weight/encumbrance limitations stay accurate.
- Focused validation: Compare the corrected passage with the game-design contract, server gear-Armor path and focused damage-profile test; run git diff --check. No Unreal build is required.

## RF115 — Restore the authoritative A8b wearable-value source (P3)

- Source: CR13-011; docs/05-decision-log.md:5,186; docs/icon-provenance.md:233; docs/alpha-item-details.md#a8b (missing); docs/07-development-setup.md:5218-5258,6120-6147,6149-6170; docs/a9-acceptance.md:215-222.
- Affected path: A developer or reviewer follows the decision log to verify delegated A8b wearable profile values and follows the A9 icon notes to their source contract.
- Trigger: The current decision log declares the detailed A8b contract in alpha-item-details.md authoritative until user adjustment, and the A9 decision/icon-provenance text cites that file, but the target is absent from the current tree.
- Impact: A8b profile decisions cannot be traced to their declared authority; some values are only partially repeated in dated setup histories. The A9 values remain in a separate current acceptance record, but its citations are stale.
- Evidence: The target does not exist and Git records its deletion at 9a8cbc0. Current docs/07 contains item-specific histories for some A8b wearables but no complete A8b value contract (including the Badger Fur Vest profile); docs/a9-acceptance.md retains the Opal Helm table.
- Proposed repair: Preserve the authoritative A8b wearable values in an existing current source or restore an appropriate concise contract; repoint the A9 references to docs/a9-acceptance.md and mark historical provenance clearly. Do not change catalogue values or gameplay.
- Dependencies: After CR19 review acceptance; no documentation edits during CR13.
- Acceptance: The A8b decision points to an existing complete source, and A9 value/provenance references resolve to retained documentation or are explicitly historical; no runtime or catalogue behavior changes.
- Focused validation: Resolve the referenced Markdown targets and compare the retained wearable contract with the registered profiles and A9 acceptance table; run git diff --check. No Unreal build is required.

## RF116 — Clarify the Boar Hide Cape warmth description (P3)

- Source: CR13-011; Content/Data/GameCatalogues/items/equipment/armor/capes.json:4-7; Source/KalmalaUI/Private/KalmalaItemDetailWidget.cpp:13-18,90; docs/04-roadmap.md:422,797-799; docs/05-decision-log.md:9; docs/07-development-setup.md:1404-1410.
- Affected path: A player reads the Boar Hide Cape description in the inventory item-detail presentation and chooses gear for cold conditions.
- Trigger: The description calls it a “warm cape,” while the approved A4 decision defers insulation/cold protection for the hide Boots/Cape and the wearable schema authors no warmth field.
- Impact: In this survival game, the player-facing adjective may imply a cold-protection effect the current contract explicitly defers.
- Evidence: The item-detail widget presents the catalogue Description. The A4 decision says to defer insulation/cold protection; the current armor schema and server calculations provide Armor/durability behavior only.
- Proposed repair: Remove or qualify “warm” so the description does not imply a current cold-protection mechanic. Adding such a mechanic would need separate approved scope; do not change data or gameplay during CR13.
- Dependencies: After CR19 review acceptance; no catalogue or gameplay edits during CR13.
- Acceptance: The player-facing description does not promise unsupported cold protection and remains consistent with the A4 decision; no runtime behavior changes.
- Focused validation: Compare the revised text with the item-detail presentation and A4 cold-protection decision; run the focused item-catalogue/detail checks when repaired and git diff --check. No Unreal build is required for this review handoff.

## RF117 — Keep preview caches aligned with authored files (P3)

- **Source:** CR14-002; Scripts/Render-Creatures.ps1:40-50,119-153; Scripts/Render-Menu.ps1:39-49,127-164.
- **Affected path:** A developer reuses a preview cache after deleting, renaming, or replacing authored files in Source, Config, Content, or Plugins.
- **Trigger:** Both renderers copy an existing source file only when its LastWriteTimeUtc differs and never remove cached authored files no longer present in the current tree. The native freshness digest hashes current Source/Plugins and Kalmala.uproject, not the cache contents. With -BuildModules, the renderer builds the stale cache and then writes the digest of current sources, so later renders treat that cache as fresh. A content change with a preserved timestamp is also skipped by the copy predicate.
- **Impact:** Builds can compile deleted C++/plugin sources or render stale copied content; a successful build can stamp and reuse a cache that does not represent the current checkout.
- **Evidence:** The directory loops only copy missing or timestamp-different files; neither renderer prunes files absent from Source/Config/Content/Plugins. Each computes nativeSources from the current source tree and writes sourceDigest after Build.bat succeeds. The documented workflow reuses the cache and calls -BuildModules after native source changes.
- **Proposed repair:** Reconcile the cache-owned authored directories against the current source set, compare content as well as timestamps, remove stale authored files only within those owned directories, and stamp only after verifying the cache's native inputs match the current digest.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** After a seeded cache has a source removed/renamed, the next build contains no stale source and succeeds against the current tree; a same-timestamp content change is copied; the freshness stamp cannot mark a mismatched cache current. Unchanged renders still reuse the cache.
- **Focused validation:** Seed a disposable cache with an authored .cpp and content file, remove/rename the source and change another file while preserving its timestamp, then verify cache reconciliation and stamp behavior before rendering.

## RF118 — Keep catalogue validation active under Python optimization (P3)

- **Source:** CR14-002; Scripts/Validate-CatalogueReferences.py:20,24-28,33,55-56,65,71,74,81,85.
- **Affected path:** A maintainer runs the documented catalogue-reference validator with Python optimization enabled by -O or PYTHONOPTIMIZE.
- **Trigger:** Every data-integrity gate in the validator uses assert. Python removes assert statements in optimized mode, but the script continues through its audit and prints PASS, including when references, registrations, or icon mappings are invalid.
- **Impact:** The no-build catalogue validation can report success while missing broken JSON references, unregistered files, or runtime/icon mappings.
- **Evidence:** All structural/reference checks are assert statements, and a harmless python -O probe with assert False printed PASS. The documented command is python Scripts/Validate-CatalogueReferences.py.
- **Proposed repair:** Replace assert-based validation with explicit checks that raise a clear exception or return a nonzero exit status; print PASS only after all checks complete.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** Valid current catalogues pass in normal and optimized Python; a synthetic missing reference, unregistered JSON file, and missing icon mapping fail under both modes without printing PASS.
- **Focused validation:** Run the validator against current data and read-only or temporary invalid fixtures under normal Python and python -O.

## RF119 — Check cache containment on a directory boundary (P3)

- **Source:** CR14-002; Scripts/Render-Creatures.ps1:65-70; Scripts/Render-Menu.ps1:79-84.
- **Affected path:** A developer chooses a documented custom cache beside the checkout, such as C:\dev\Kalmala-cache for sourceRoot C:\dev\Kalmala.
- **Trigger:** Both scripts reject Cache when Cache.StartsWith(sourceRoot, OrdinalIgnoreCase), without requiring a following directory separator. The sibling C:\dev\Kalmala-cache therefore matches the prefix even though it is not inside C:\dev\Kalmala.
- **Impact:** Valid external cache paths that share the checkout's name prefix fail before a render, despite the documented -Cache option.
- **Evidence:** A no-write path comparison returned StartsWith=true and child-boundary=false for C:\dev\Kalmala and C:\dev\Kalmala-cache. Development setup documents -Cache as changing the renderer cache.
- **Proposed repair:** Compare normalized paths with a directory boundary, rejecting the source root itself and descendants while accepting siblings with a shared textual prefix.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** Both renderers reject the checkout and its true descendants, but accept an external sibling cache named <checkout>-cache; defaults stay unchanged.
- **Focused validation:** Add pure path tests for exact root, child, sibling sharing a prefix, case variation, and a different drive without creating cache directories.

## RF127 — Remap expanded-map interaction controls (P3)

- **Source:** CR12-015; current main-checkout E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaSettingsWidget.cpp:360-398,1192-1203, E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldMapSubsystem.cpp:48-54, and E:/dev/Kalmala/Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:1641-1684, inspected read-only; docs/01-game-design.md:151-153; docs/02-technical-architecture.md:308; docs/04-roadmap.md:983-1017; docs/07-development-setup.md:3485-3501.
- **Affected path:** A local player opens the expanded map and wants to customize keyboard/controller controls for zoom, pin selection/placement/state, marker filtering, or pings through the existing local input settings.
- **Trigger:** The player needs to replace a fixed in-map shortcut; Settings exposes the remappable WorldMap open action, but the in-map commands are handled by literal key comparisons and have no mapping rows.
- **Impact:** The design's remappable-input accessibility baseline does not cover expanded-map operations. Players who cannot use the fixed key/controller defaults cannot customize those map actions through Settings. Mouse alternatives remain; no gameplay authority or peer-privacy defect was found.
- **Evidence:** GetLocalInputDefinitions contains WorldMap (M/N and Gamepad Special Right) but no in-map zoom, pin, filter, or ping actions; the local-player subsystem binds only WorldMap. The widget directly checks EKeys for 1-3, Q, Tab, P, Enter, H, Delete, Page Up/Down, and marker-filter navigation. Map input ownership is local; pin state and filtering stay owner-local, while pings remain server-validated. Game design requires remappable input from the vertical slice onward. Architecture and setup defer map remapping to later accessibility work, but the decision log contains no waiver of that baseline. Existing tests exercise local map actions and general settings rebinding but do not verify remapped map actions.
- **Proposed repair:** Expose the existing map keyboard/controller commands through the existing local remapping settings and dispatch them using the selected bindings. Keep current actions/defaults and mouse alternatives; add no gameplay request, network state, or save field.
- **Dependencies:** After CR19 review acceptance; no repair during CR12 review.
- **Acceptance:** Existing map keyboard/controller commands can be rebound through the existing local settings path, and current binding labels are accurate in the corresponding Controls/Controller views. Map actions use the selected bindings rather than also firing from the old defaults. M/Escape open/close behavior, current pointer alternatives, focus ownership, local saves, and server-validated ping behavior remain intact.
- **Focused validation:** Add isolated settings/map automation that rebinds representative keyboard and controller controls for map zoom, pin operations, filtering, and ping; verify displayed bindings and action dispatch, and confirm the previous defaults no longer trigger those commands. Run after an Editor build with a disposable local user directory and retain map focus/privacy checks. No test was run during this review.

## RF128 — Correct placed-object scope metadata in the catalogue icon manifest (P3)

- **Source:** CR14-003; `Scripts/Data/catalogue-icon-manifest.csv:11-22`.
- **Affected path:** A maintainer uses the manifest to audit inventory icon coverage or determine how construction identities are exposed.
- **Trigger:** Twelve rows (`WorkbenchKit`, `ForgeKit`, `WorkbenchToolRackKit`, `ForgeAnvilKit`, `GrindingStoneKit`, `StorageKit`, `CookingRackKit`, `FryingPanKit`, `CauldronKit`, `FloorKit`, `WallKit`, and `RoofKit`) have clean catalogue aliases registered under `items/buildItems/` but classify their identity as `item`, set `inventory_scope=pack`, and list `PackInventory`, `ItemDetail`, and `Storage` views. Current architecture treats these as presentation-only buildables and says internal `Kit` identities represent placed objects only.
- **Impact:** The manifest misstates twelve placed-object identities as pack inventory. Current runtime behavior is unaffected because import and validation scripts ignore these metadata columns, but the stale data can mislead future audits or authoring and reintroduce portable-kit assumptions.
- **Evidence:** A read-only cross-reference joined all twelve aliases to registered `items/buildItems/*.json` definitions. `docs/02-technical-architecture.md:504-506` and `docs/05-decision-log.md:53` require buildables to remain separate from inventory. `Validate-UIIcons.ps1 -Category Catalogue` and `python Scripts/Validate-CatalogueReferences.py` pass but do not validate these manifest columns.
- **Proposed repair:** Update the twelve rows' identity and view metadata to reflect non-inventory buildables, retaining only UI contexts that actually consume their canonical images.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** The twelve identities are marked as presentation-only buildables with no pack inventory scope, `PackInventory`, `ItemDetail`, or `Storage` views; direct-material recipes and current texture IDs remain unchanged.
- **Focused validation:** Add a read-only manifest/catalogue cross-check for buildable aliases that rejects inventory scope or inventory views, then rerun both catalogue icon and catalogue-reference validators.

## RF129 — Repair references to the removed shared icon workflow (P3)

- **Source:** CR14-003; `AGENTS.md:135`; `docs/README.md:22`; `docs/icon-provenance.md:4`.
- **Affected path:** A contributor follows project guidance to the shared icon authoring/import workflow before preparing or updating an icon.
- **Trigger:** These references point to `docs/material-icons.md`, which is absent from the tracked checkout and was deleted in the documentation consolidation commit `9a8cbc0f`. The README still says it owns the shared authoring/import steps. Parts of the workflow appear in development setup and the provenance file, but the advertised shared link has no target.
- **Impact:** The top-level icon workflow references are broken, making supported preparation/import steps harder to find and leaving maintainers without the promised shared entry point.
- **Evidence:** `git ls-files` contains no `docs/material-icons.md`; `Test-Path` is false; `git show --name-status 9a8cbc0f` lists the deletion while the three current references remain. Tool requirements and validator commands are present in `docs/07-development-setup.md`, and per-item prompts remain in `docs/icon-provenance.md`.
- **Proposed repair:** Restore a concise shared workflow page or retarget all current references to extant setup/provenance sections that provide an authoring, preparation, import, and validation entry point.
- **Dependencies:** After CR19 review acceptance; no documentation repair during CR14.
- **Acceptance:** No active guidance points to an absent page, and the existing authoring/import/validation workflow remains discoverable.
- **Focused validation:** Search current guidance for `material-icons.md`, verify every reference resolves to a tracked file, and run a Markdown link check.

## RF131 — Mark binary PPM world-generation images as binary (P3)

- **Source:** CR15-001; `.gitattributes:1-12`; `Content/Kalmala/Developer/WorldGeneration/{BiomeClassification,Elevation,Flora,Humidity,Temperature}.ppm`; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:646-650,673-677`.
- **Affected path:** Git checks out, diffs, or merges the tracked P6 PPM world-generation images.
- **Trigger:** Two branches update the same PPM image and Git uses its unspecified default merge attribute. The project attributes mark `.uasset`, `.umap`, PNG/JPEG, FBX, and common audio as binary but omit `.ppm`.
- **Impact:** Git can apply text merge behavior to opaque pixel bytes, leaving conflict markers or a meaningless merged image. These are developer visualization artifacts; no runtime or gameplay effect was confirmed.
- **Evidence:** All five tracked PPM files have a P6 header and opaque binary pixel data. `git check-attr --all` returns no attributes for a PPM path, while representative Unreal packages and PNGs receive the existing `binary` macro. Git documents that the macro unsets `diff`, `merge`, and `text`; when `merge` is unspecified, Git uses its default text merge driver ([Git attributes documentation](https://git-scm.com/docs/gitattributes)). The commandlet writes current world-generation PPM outputs. Static inspection only; no synthetic merge was run.
- **Proposed repair:** Add `*.ppm binary` to `.gitattributes`.
- **Dependencies:** After CR19 review acceptance; no repair during review.
- **Acceptance:** Current and newly generated PPM files report `binary` with `diff`, `merge`, and `text` unset, preventing text normalization and line-based merge behavior.
- **Focused validation:** Run `git check-attr --all` against all five tracked PPM files and confirm the binary macro attributes. No test was run during this review.

## RF132 — Include inline C++ headers in shared naming conventions (P3)

- **Source:** CR15-001; `.editorconfig:1`; `Source/KalmalaGameplay/Private/KalmalaA8bFangSpearPeerVerification.inl:1,25,91`; `Source/KalmalaGameplay/Private/KalmalaA8bWolfPeltBootsPeerVerification.cpp:500`; `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:1153`.
- **Affected path:** A developer edits or runs Visual Studio's optional C++ naming-convention linter on the tracked `.inl` helper.
- **Trigger:** The root EditorConfig section `[*.{cpp,h}]` does not match `.inl`, and there is no nearer EditorConfig for this file. The file contains C++ declarations and is included by a related `.cpp` verification fixture.
- **Impact:** The project's Unreal naming settings are not supplied for this inline helper, so editor naming diagnostics can differ from the rest of the C++ source. The helper is non-Shipping verification code; no build or gameplay failure was confirmed. Visual Studio's naming check is opt-in.
- **Evidence:** This is the only tracked C++-like extension outside `.cpp` and `.h` under `Source/` and `Plugins/`; no deeper `.editorconfig` covers it. Microsoft's C++ naming-linter documentation says the check reads conventions from `.editorconfig` and must be enabled in C/C++ Code Style options ([documentation](https://learn.microsoft.com/en-us/cpp/ide/lnt-naming-convention?view=msvc-170)).
- **Proposed repair:** Extend the root C++ EditorConfig glob to include `.inl`.
- **Dependencies:** After CR19 review acceptance; no repair during review.
- **Acceptance:** The shared Unreal naming rules resolve for `.inl` as for `.cpp` and `.h`, with no test or gameplay behavior change.
- **Focused validation:** With Visual Studio's naming-convention check enabled, open or analyze the `.inl` helper and confirm its EditorConfig naming settings are active. No linter run occurred during this review.

## RF135 — Reconcile duplicate support-effect audio imports (P3)

- **Source:** CR13-020 and CR13-021; six support-effect root and `/Source` SoundWave packages and their source WAVs.
- **Affected path:** `Content/Kalmala/Audio/` support-effect SoundWaves and `Content/Kalmala/Audio/Source/` duplicates.
- **Trigger:** a maintainer reimports or updates one of the six support-effect sounds.
- **Impact:** runtime root packages retain stale source paths, while unused `/Source` packages create duplicate asset-browser entries that may receive an incorrect reimport. Existing runtime playback identities remain valid.
- **Evidence:** all six root and `/Source` package hashes match CR01 and master; source MD5s match the checked-in WAVs. Root import paths resolve to absent project-root locations, while `/Source` packages point to their matching WAVs and are not referenced by other content packages.
- **Proposed repair:** reimport the checked-in WAVs into the existing runtime package identities, then decide whether unused `/Source` packages have a documented purpose; do not delete assets without authorization.
- **Dependencies:** After CR19 review acceptance; deletion or relocation requires explicit direction.
- **Acceptance:** runtime package identities remain unchanged, import paths resolve to matching project WAVs, and `/Source` packages are either documented or removed with authorization.
- **Focused validation:** inspect all six package paths, import filenames, hashes, and package references; run the audio validator and `git diff --check`.

## RF140 — Resolve M1 fixture maps inside the active project (P3)

- **Source:** CR14-005; `Source/KalmalaEditor/Private/SetupM1TestMapCommandlet.cpp:20-21,55-64`; `Source/KalmalaEditor/Public/SetupM1TestMapCommandlet.h:6-14`; `Kalmala.uproject:28-30`; `Source/KalmalaEditor.Target.cs:11`.
- **Affected path:** A developer invokes `SetupM1TestMap` from an Editor project checkout other than `E:/dev/Kalmala`.
- **Trigger:** The public reflected `UCommandlet` is reachable through normal `-run=SetupM1TestMap` dispatch, but `Main` loads the absolute `E:/dev/Kalmala/Content/Kalmala/Maps/Prototype/L_Prototype.umap` path and saves it when any tagged M1 fixture actor is absent. No guard ties the path to the active project.
- **Impact:** A commandlet launched for a worktree or another project can inspect or modify the main checkout's map instead of its active project's map, undermining worktree isolation and making the tool machine-path bound.
- **Evidence:** `USetupM1TestMapCommandlet` is declared as a `UCLASS` derived from `UCommandlet` in the public header. `KalmalaEditor` is registered as an Editor module and included by the Editor target. `Main` ignores the active project's content root and passes the hard-coded path to `LoadPackage` and `SavePackage`; no documented script invocation constrains that dispatch.
- **Proposed repair:** Resolve `/Game/Kalmala/Maps/Prototype/L_Prototype` from the active project and reject any resolved map path outside that project's canonical `Content` directory before loading it.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** An invocation from a disposable checkout reads and writes only that checkout's map; a missing or out-of-project map path fails before loading or saving, and the normal fixture setup remains idempotent.
- **Focused validation:** Run the commandlet against a short disposable project copy with one fixture absent; verify only its map changes and the main-checkout map hash remains unchanged. No commandlet was run during review.

## RF141 — Bound visualization extent before grid enumeration (P3)

- **Source:** CR14-005; `Source/KalmalaEditor/Private/RenderWorldGenerationVisualizationCommandlet.cpp:319-335,654-657`; `Source/KalmalaWorld/Public/KalmalaRegionalTuning.h:80`; `docs/01-game-design.md:123`; `docs/07-development-setup.md:3671,3677`.
- **Affected path:** A developer renders a world-generation visualization with a positive `-Extent` larger than the documented playable-world view.
- **Trigger:** The commandlet accepts any positive `WorldExtent`. Unless `-SkipSplineOverlay` is present, it enumerates every hydrology grid cell in the square from `-Extent/2` through `+Extent/2`; `GridCell` is 6,000 cm, so work grows quadratically with the supplied extent. A 32,000,000 cm extent yields about 28.5 million cell iterations, compared with about 285,000 at the documented 3,200,000 cm full-width extent.
- **Impact:** A valid-looking command line can leave the Editor commandlet doing an unbounded amount of hydrology work and make the process unresponsive. No larger `-Extent` use is documented; the 128 km atlas is a separate fixed image, while the playable world radius remains 16 km.
- **Evidence:** `Main` checks only `WorldExtent <= 0`; lines 654-657 derive both loop bounds directly from the value and call `GetHydrology` for every cell unless the optional skip flag is set. The development guide documents 3,200,000 cm and describes the overlay as a costly whole-area enumeration; the game design fixes the world at a 16 km radius. A read-only search of documented `Extent` invocations found only 400,000 cm and 3,200,000 cm.
- **Proposed repair:** Require a finite extent no greater than twice `FKalmalaWorldBounds::Radius` (3,200,000 cm) before allocating images or creating output directories.
- **Dependencies:** After CR19 review acceptance; no repair during CR14.
- **Acceptance:** The documented extent and smaller previews remain valid; values above 3,200,000 cm and non-finite values reject before sampling or output creation, with the overlay both enabled and skipped.
- **Focused validation:** Add argument-boundary checks for zero, the documented maximum, one unit above it, and non-finite input; verify the rejection path creates no output. No visualization was run during review.

## RF150 — Include the gameplay map in packaged cooks (P1)

- **Source:** CR15-002; `Config/DefaultGame.ini:14`; `Config/DefaultEngine.ini:3-4`; `Source/KalmalaGameplay/Private/KalmalaMenuContext.cpp:179-180`; `Scripts/Package-And-Run.ps1:11,176-191,201-228`.
- **Affected path:** A packaged player starts Home and selects a local world for solo or listen-host play.
- **Trigger:** The normal cook uses the project map list, which names only `/Game/Kalmala/Maps/Menu/L_Menu`; the Home world-start path opens `/Game/Kalmala/Maps/Prototype/L_Prototype`. The packaging script's `$Map` value is passed only to the post-cook launch, not to `BuildCookRun`.
- **Impact:** The normal packaged Home flow targets a map that is not selected for cooking, so solo/host world start can fail before gameplay loads.
- **Evidence:** `DefaultEngine.ini` sets Home as `GameDefaultMap` and Prototype only as `EditorStartupMap`. `DefaultGame.ini` has a single `MapsToCook` entry for Home and no always-cooked `/Game/Kalmala/Maps` directory. The menu code calls `OpenLevel` with the Prototype package path. `BuildCookRun` in the project package script has no map-list argument; its Prototype `$Map` is added later to the packaged executable's launch arguments. Epic documents `MapsToCook` as the map list used when no other map list is specified on the command line ([Project Packaging Settings API](https://dev.epicgames.com/documentation/unreal-engine/API/Developer/DeveloperToolSettings/UProjectPackagingSettings)); the package script's default client flow and the Home/game round-trip are also still named as SM15 validation requirements in `docs/startmenu.md:435-440`. The package was not built during this review.
- **Proposed repair:** Add `/Game/Kalmala/Maps/Prototype/L_Prototype` to the `MapsToCook` list in `DefaultGame.ini`.
- **Dependencies:** After CR19 review acceptance; no repair during review.
- **Acceptance:** A standard packaged build contains both Home and Prototype; launching without a map override opens Home, and solo and listen-host world selection load Prototype successfully.
- **Focused validation:** Package with `Scripts/Package-And-Run.ps1 -Mode Dev` and verify both the script's Prototype smoke launch and a fresh no-map launch followed by Home solo/listen-host selection. No package or runtime validation was run during this review.

## RF151 — Repair VisualStudioTools troubleshooting links and heading (P3)

- **Source:** CR15-006; `Plugins/VisualStudioTools/Docs/Troubleshooting.md:43,45,51`; `Plugins/VisualStudioTools/README.md:17,60`.
- **Affected path:** A user follows either troubleshooting link to the plugin README or searches for the Task Center error heading.
- **Trigger:** The troubleshooting links use fragments `#building-and-installing` and `#optional-enabling-the-plugin`, but the linked README headings generate `#building-and-installing-the-plugin` and `#enabling-the-plugin-optional`. The Task Center heading starts with a double quote and ends with a single quote.
- **Impact:** Both navigation links miss their intended sections, and the malformed heading is harder to scan and search accurately.
- **Evidence:** Compared the link targets with the current README headings and GitHub's documented heading-anchor rules ([GitHub heading-anchor documentation](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)).
- **Proposed repair:** Update both fragments to match the generated anchors and balance or remove the error-message quotation marks.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** Both links open the intended README sections on GitHub, and the error heading has consistent punctuation without changing the reported error text.
- **Focused validation:** Render or inspect the Markdown anchors against the two README headings and check the heading punctuation. No repair or runtime validation was run during this review.

## RF152 — Correct VisualStudioTools Marketplace guide copy and captions (P3)

- **Source:** CR15-006 and CR15-007; `Plugins/VisualStudioTools/Docs/Marketplace_Readme.md:20,43,46,49`; `Plugins/VisualStudioTools/Docs/images/test_explorer.png`; and `Plugins/VisualStudioTools/Docs/Visual Studio Integration Tool Documentation.pdf:2-4`.
- **Affected path:** A reader follows the plugin installation guide and identifies its setup and test screenshots in Markdown or the included PDF.
- **Trigger:** The Vault instruction says “go Library -> Vault in the and locate”; the Figure 2 and Figure 4 captions have unclosed emphasis markers; the Figure 4 image is captioned “Figure 3”; and the Figure 3 caption describes a Blueprint rescan menu although its image shows discovered tests in Visual Studio Test Explorer. The PDF reproduces the defects present in the guide it prints.
- **Impact:** The installation sentence is confusing, emphasis markers render literally, figure numbering is duplicated, and one caption misidentifies its screenshot.
- **Evidence:** Read the complete Markdown, extracted the four-page PDF text, and rendered and visually inspected every PDF page and all four referenced PNGs. The Test Explorer image displays discovered tests and status counts; it does not show a rescan menu. All Markdown image targets resolve to tracked files. The PDF metadata identifies a 2024 GitHub page print of `Marketplace_Readme.md` at upstream commit `a0e69279fb4d41c4c5f99e8d407b2ab7b93664b9`.
- **Proposed repair:** Correct the Vault sentence, close the two emphasis markers, accurately caption the Test Explorer screenshot, label the final screenshot Figure 4, and update the included PDF to match the corrected guide.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** The Markdown is grammatical, all four screenshot captions match their images and are correctly numbered with no literal emphasis markers, and the included PDF matches and renders cleanly.
- **Focused validation:** Render the Markdown and the updated PDF pages containing the guide screenshots; confirm the sentence, image descriptions, numbering, emphasis, and layout. No repair or PDF regeneration was performed during this review.

## RF153 — Anchor VisualStudioTools packaging paths to the script location (P3)

- **Source:** CR15-008; `Plugins/VisualStudioTools/Scripts/Package-Plugin.ps1:16-17,21,27,29,38-39`; `Plugins/VisualStudioTools/build.proj:1,14`; and `Plugins/VisualStudioTools/Scripts/SignDetached.proj:2`.
- **Affected path:** A developer runs the helper to create a versioned VisualStudioTools ZIP.
- **Trigger:** The script invokes `msbuild` without naming a project and resolves `Config` and `publish` relative to the caller's current directory. MSBuild searches the current directory for a project when none is named; the plugin root has `build.proj`, while `Scripts` has the unrelated `SignDetached.proj` ([MSBuild command-line reference](https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference?view=visualstudio)).
- **Impact:** Running the helper from `Scripts` or another directory can select the wrong MSBuild project, fail to find the intended Config folder, or publish the ZIP in an unintended location. The signing target may be invoked instead of plugin packaging.
- **Evidence:** The three paths are relative and the script never sets or validates the working directory. Repository search found no tracked caller or usage guide that requires invocation from the plugin root. The release pipeline names `Scripts/SignDetached.proj` explicitly, demonstrating that the signing project has its own intended invocation.
- **Proposed repair:** Resolve the plugin root from `$PSScriptRoot`, pass its `build.proj` explicitly to MSBuild, and anchor Config and publish paths there; alternatively, reject any working directory other than the plugin root with a clear message.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** Calls from the plugin root, `Scripts`, and an unrelated directory either build the same plugin with the same intended inputs/output path or fail immediately with the documented precondition. The signing target is never selected accidentally.
- **Focused validation:** Use a controlled MSBuild stub from those three directories and confirm the explicit project, Config, and publish paths. No package build was run during this review.

## RF154 — Stop VisualStudioTools packaging after a failed build (P3)

- **Source:** CR15-008; `Plugins/VisualStudioTools/Scripts/Package-Plugin.ps1:17,21-24,27,35-43`; and `Plugins/VisualStudioTools/build.proj:14-19`.
- **Affected path:** A developer builds and archives the plugin with `Package-Plugin.ps1`.
- **Trigger:** The script starts native `msbuild` but never checks `$LASTEXITCODE` or otherwise verifies its success before reading and rewriting the descriptor, copying Config, creating the ZIP, and printing `Done`. Under PowerShell's default handling, a native nonzero exit is stored in `$LASTEXITCODE` and does not create an error record or invoke `catch`/`trap` by itself ([PowerShell error handling](https://learn.microsoft.com/en-us/PowerShell/module/microsoft.PowerShell.core/about/about_error_handling?view=powershell-5.1)).
- **Impact:** A failed UAT build can be followed by archive steps over partial output, and the helper can reach its success path without proving that a complete plugin package was produced. A terminating failure before the final cleanup can also leave the temporary staging directory behind.
- **Evidence:** There is no exit-code check, terminating error policy, output-manifest check, or `try/finally`; cleanup at line 41 and the `Done` message at line 43 follow the unchecked archive operation.
- **Proposed repair:** Check and preserve the MSBuild exit code immediately, make package cmdlet failures fatal, verify the output ZIP and required descriptor before reporting success, and clean the generated temp directory in `finally`.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** Any nonzero build result, missing descriptor, copy failure, or archive failure stops the helper with a failing exit and no success message or newly published package; temporary staging is cleaned. Success is reported only for a verified package.
- **Focused validation:** Use a controlled MSBuild stub for success and nonzero exit, plus missing-output and archive-failure cases. Confirm exit status, success messaging, package presence, and cleanup. No package build was run during this review.

## RF155 — Remove unrelated direct dependencies from the Blueprint debugger helper (P3)

- **Source:** CR15-010; `Plugins/VisualStudioTools/Source/VisualStudioBlueprintDebuggerHelper/VisualStudioBlueprintDebuggerHelper.Build.cs:10-28`; `Plugins/VisualStudioTools/Source/VisualStudioBlueprintDebuggerHelper/Private/VisualStudioBlueprintDebuggerHelperModule.cpp:4-25`; and `Plugins/VisualStudioTools/Source/VisualStudioBlueprintDebuggerHelper/Private/VisualStudioBlueprintDebuggerHelperModule.h:5-13`.
- **Affected path:** A developer builds or packages the VisualStudioTools Blueprint debugger helper module for Win64 Editor.
- **Trigger:** The helper lists 18 `PrivateDependencyModuleNames`, copied from the primary plugin module, although the helper's own private header and implementation directly use Core, CoreUObject, Engine, and UnrealEd header/API families. Entries such as AssetRegistry, Json/JsonUtilities, Slate, ToolMenus, MainFrame, VisualStudioDTE, EditorStyle, and Projects have no matching helper-source include or API call. Epic documents these lists as module dependencies and says private dependencies are for modules used by private code ([Unreal Engine Modules](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules)).
- **Impact:** The helper's direct build graph is coupled to unrelated editor, UI, asset, serialization, and Visual Studio integration modules. This makes the helper less isolated and can pull additional modules into its compile/link dependency chain without supporting its current implementation.
- **Evidence:** The helper contains only one private `.cpp` and one private `.h`; include and API-reference inspection found no usage for the extra direct dependencies. The plugin descriptor already constrains this helper to Win64 Editor. The published upstream Build.cs uses the same broad list, but no helper-specific need for those entries was found in this vendored source.
- **Proposed repair:** Keep only direct module dependencies justified by the helper's private includes and API usage; leave public dependencies absent while the interface remains private.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** The Build.cs dependency list reflects only the helper's actual direct module needs, and the supported Win64 Editor plugin builds still compile and link the helper without changing the main module's independent dependencies.
- **Focused validation:** Build the plugin for its supported Unreal versions after trimming the list, and confirm UBT resolves the helper and primary module independently. No build was run during this review.

## RF156 — Align primary VisualStudioTools module dependencies with source (P3)

- **Source:** CR15-012; `Plugins/VisualStudioTools/Source/VisualStudioTools/VisualStudioTools.Build.cs:55-75`; `VisualStudioToolsBlueprintBreakpointExtension.h:8`; and `VisualStudioToolsBlueprintBreakpointExtension.cpp:13,36-55`.
- **Affected path:** A developer builds or packages the VisualStudioTools primary module for Win64 Editor.
- **Trigger:** UBT resolves the primary module's source and link dependencies.
- **Impact:** The source uses GraphEditor without declaring that module in this module's Build.cs, relying on a transitive private engine dependency. The rule also directly adds `ApplicationCore`, `MainFrame`, and `ToolMenus`, although the primary module has no corresponding include or API use. This leaves the build graph both incomplete and unnecessarily coupled, making module builds more sensitive to transitive dependency changes and pulling in unrelated direct edges.
- **Evidence:** Searched every primary-module private header and implementation. `GraphEditorModule.h` and `FGraphEditorModule` are directly used by the breakpoint extension, but GraphEditor is absent from the 17 private dependencies. In installed UE 5.8.2, GraphEditor appears in UnrealEd's private dependency and private include-path lists, not its public dependencies. `ApplicationCore`, `MainFrame`, and `ToolMenus` appear only in this Build.cs. The only public module header includes `CoreMinimal.h`, matching the sole public dependency `Core`. Epic's [Unreal Engine Modules guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules) explains that Build.cs dependency lists make modules available to source and recommends private dependencies for private source use.
- **Proposed repair:** Remove the unused `ApplicationCore`, `MainFrame`, and `ToolMenus` entries and add `GraphEditor` to the primary module's private dependencies.
- **Dependencies:** After CR19 review acceptance; this handoff does not authorize implementation during the review-only phase.
- **Acceptance:** The primary module's Build.cs declares the modules its private source directly uses without unrelated entries, and the supported Win64 Editor plugin build resolves and links the breakpoint extension.
- **Focused validation:** Build the plugin for supported Win64 Editor targets with the custom no-unity/no-PCH build settings and confirm GraphEditor resolves through the explicit dependency. No build was run during this review.

## RF157 — Guard failed Blueprint sync-load handles before release (P2)

- **Source:** CR15-013; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintAssetHelper.cpp:77-87`.
- **Affected path:** A Blueprint reference or index commandlet processes an asset-registry row whose generated-class path cannot be loaded.
- **Trigger:** `RequestSyncLoad` returns an invalid handle. The helper registers scope-exit cleanup before checking `Handle.IsValid()`, then the invalid-handle branch logs and continues.
- **Impact:** The cleanup dereferences a null handle, crashing the commandlet/editor process instead of skipping the failed asset as the warning says.
- **Evidence:** Read the complete helper loop. Its `ON_SCOPE_EXIT` body calls `Handle->ReleaseHandle()` at line 81 before the validity guard at line 84. Installed UE 5.8.2 `StreamableManager.cpp:2024-2040` returns `nullptr` for invalid short package names or requests containing no valid assets, so the guarded failure path is reachable.
- **Proposed repair:** Register cleanup only after confirming the handle is valid, or make cleanup check validity before releasing it.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Missing or invalid generated-class paths are logged and skipped without a crash, while valid handles are released after processing.
- **Focused validation:** Exercise a failed generated-class load and a valid Blueprint load through the helper; confirm the failed asset is skipped and the valid handle is released. No commandlet or build was run during review.

## RF158 — Validate the optional Blueprint-reference symbol (P2)

- **Source:** CR15-013; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintReferencesCommandlet.cpp:191-211`.
- **Affected path:** A developer invokes `VsBlueprintReferences` without `-symbol`.
- **Trigger:** Constructor help labels the symbol parameter optional, but `Run` obtains it with `ParamVals.Find` and dereferences the returned pointer without checking for null.
- **Impact:** Omitting the documented optional parameter causes an access violation before the commandlet can print its missing-parameter message or return an error.
- **Evidence:** The constructor at lines 191-194 describes the parameter as optional, while lines 206-211 call `ReferencesSymbol->IsEmpty()` before validating the `Find` result. The empty-value branch already intends to print help and fail cleanly.
- **Proposed repair:** Check both pointer presence and string emptiness before dereferencing, and align the required/optional wording in help and usage.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Omitted and empty symbol values return a documented nonzero error with help; a valid fully qualified symbol proceeds to search.
- **Focused validation:** Invoke the commandlet with the symbol omitted, empty, malformed, and valid. No commandlet run or build was performed during review.

## RF159 — Preserve references to same-named Blueprints (P2)

- **Source:** CR15-013; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/BlueprintReferencesCommandlet.cpp:97-116,141-153`.
- **Affected path:** A Blueprint reference search returns matching assets that share a basename but live in different packages.
- **Trigger:** Confirmed matches are stored in a `TMap` keyed by `BlueprintGeneratedClass->GetName()`, which contains only the generated class's short name; `Add` replaces an existing value when that key is already present.
- **Impact:** One of the matching Blueprint assets is silently omitted from the JSON array, so the commandlet reports incomplete reference results and an undercount.
- **Evidence:** `GetConfirmedAssets` adds each match under `BlueprintClassName->GetName()` at line 115. The serializer emits one entry per map element and includes package file path only after the collision has already occurred. Unreal assets in different packages can share the same object/class short name.
- **Proposed repair:** Preserve each asset using its full object path as the key, or store matches in an array and retain the short name only as display data.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Two matching Blueprints with the same basename in different folders both appear in the output with distinct package paths, and the reported count matches the returned entries.
- **Focused validation:** Run a reference search against two same-named matching Blueprints in separate packages and inspect the serialized JSON. No such fixture or commandlet run was performed during review.

## RF180 — Rebuild the owner inventory grid after carried-tool restore (P3)

- **Source:** CR17-002; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:423-447`; `Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp:187-207`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:9427-9512,10843-10931`; `Source/KalmalaGameplay/Private/KalmalaInventoryGrid.cpp:11-27,132-177,349-361`; and `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:410-415`.
- **Affected path:** An authenticated player's carried tools are restored from a valid schema-3 player-discovery save during PostLogin.
- **Trigger:** `AKalmalaCharacter::BeginPlay` creates fresh default carried-tool instance GUIDs and synchronizes the inventory grid. After the pawn has started, PostLogin loads the player's save, replaces `CarriedTools` with the saved tool records/GUIDs, and calls `ForceNetUpdate` without rebuilding the grid or rebinding the active-tool instance GUID.
- **Impact:** The owner-only grid can retain the startup GUIDs after the carried-tool records have been replaced. `GetSlotInstanceGuid` checks grid GUIDs against current stacks and carried tools, so stale slot GUIDs fail resolution until a later grid synchronization. The active-tool reference can also point at an instance no longer present. A later hotbar use or stack transaction synchronizes the grid, so this is a temporary restore inconsistency rather than evidence of permanent tool loss.
- **Evidence:** Character startup populates and synchronizes the default tool records; the PostLogin restore path replaces those records from the authenticated save and forces replication but does not call the grid synchronization path. Grid resolution rejects GUIDs absent from current inventory state, and the grid fields are owner-only. Existing slot-use and inventory transaction paths synchronize later, which bounds the defect's duration. The save persists carried-tool IDs, levels, condition, and instance GUIDs, while ordinary carried material stacks and cell positions remain session-only by design.
- **Proposed repair:** After applying saved carried-tool records, rebuild the authoritative owner grid immediately and clear or rebind the active-tool GUID against the restored records before replication.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A PostLogin restore with nondefault saved tool IDs/GUIDs immediately maps each restored tool to one unique resolvable slot and leaves Main Hand bound to a current GUID or cleared, before any later hotbar or stack transaction. The owner can select and use restored tools, and observers receive no private tool state.
- **Focused validation:** Add a PostLogin save-restore regression with differing saved tool IDs/GUIDs, assert owner slot mapping and active-tool validity before any hotbar or stack transaction, and run it with the existing inventory owner-privacy check after an Editor build.

### CR17-001 — Durable inventory rollback feedback

- **Reviewed revision:** `codex/cr17-chat` at `4e81f982cfd36f414d65a9bed45f8b528cce859c`; nine focused source/test files match their CR01 SHA-256 fingerprints. `KalmalaGameMode.cpp` differs from CR01 (`ca3e52a4160f04e0323996fd7ba6ff3813d34e6635665d704df119a6c07079ea` versus `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325`), so only the recorded construction/player persistence ranges were reviewed in the current file.
- **Evidence:** Followed server RPC and authority checks through direct-material cost validation, inventory payment, candidate save write, actor teardown, and stack rollback. Confirmed the rollback commit is indistinguishable from an accepted gain to `RecordAcceptedGains`; the owner receipt queue presents it as “You gained”. The same helper sequence appears in carried-tool craft/refuel persistence failures and in the existing RF43 harvest/gemstone cases.
- **Result:** Confirmed an extension of RF43, not a separate finding. Added the RF43 remediation handoff and recorded the CR17-001 file ranges in `Temporary/Review/coverage.md`. No source, test, script, configuration, catalogue, asset, or game documentation changed.
- **Authority:** Placement, payment, persistence, rollback, and receipt creation remain server-controlled; the receipt is client-only to the owning player. No authority bypass or observer exposure was found in this path.
- **Checks / limits:** Recomputed the ten selected file hashes against CR01; nine matched and the changed GameMode hash is recorded above. Static call-path and receipt-consumer inspection only; `git diff --check` passed after evidence updates. No test, build, automation, failure injection, live owner/observer session, or runtime profiling was run. Full verification remains deferred.
- **Remaining scope:** This increment does not review the complete successful gather/craft/build/save/reload flow, combat rewards, menu/join/reconnect, or tick/allocation/streaming/lifetime behavior. CR17-002 is next.

### CR17-002 — Gather, craft, build, save, and reload continuity

- **Reviewed revision:** `codex/cr17-chat` at `ec94a3e6183d1b09624f43c141d4d49fe2fe63c5`. Eleven of the twelve focused source files match their CR01 SHA-256 fingerprints. `KalmalaGameMode.cpp` differs from CR01 (`ca3e52a4160f04e0323996fd7ba6ff3813d34e6635665d704df119a6c07079ea` versus `9866c16cd248f54eeff0226c9ac5281a3af5a05aa226158bc94f11a5dee3c325`); only the ranges listed for CR17-002 in `Temporary/Review/coverage.md` were reviewed at the current hash.
- **Evidence:** Traced accepted forest/crop gathering through the inventory transaction and the separate world-population depletion/save path; RF16 and RF21 already record relevant population-save and session-only depletion limitations. Normal recipe costs/results pass through the inventory transaction, while ordinary pack stacks and grid cell positions are session-only. Carried-tool recipes persist the authenticated player's carried-tool records. Direct-material construction charges through the inventory transaction and writes validated construction records to the world save, which are restored at startup. The schema-3 player save owns authenticated-player discovery facts and carried-tool IDs, levels, condition, and GUIDs; it does not persist general pack stacks. Cross-referenced CR06-001/002, CR07-002, RF16, RF21, and RF43 without duplicating their findings. Confirmed RF180 for the gap between PostLogin carried-tool restoration and immediate owner-grid synchronization.
- **Result:** CR17-002 is reviewed and RF180 is queued as an unchecked post-CR19 handoff. The focused batch and file ranges are recorded in `Temporary/Review/coverage.md`. No gameplay source, test, script, configuration, catalogue, asset, or game documentation changed.
- **Authority:** Gathering, crafting/payment, construction placement, persistence, and restoration are server-controlled in the reviewed paths. Carried-tool/grid fields remain owner-only. No client authority bypass or observer disclosure was identified.
- **Checks / limits:** Recomputed all twelve focused file hashes; eleven match CR01 and the changed GameMode hash is recorded above. Reviewed source call paths and persistence ownership statically and ran `git diff --check` after evidence updates. No tests, builds, automation, restart scenario, owner/observer session, failure injection, or runtime profiling ran. Full verification remains deferred.
- **Remaining scope:** CR17-003 combat/reward persistence, CR17-004 menu/join/logout/reconnect, and CR17-005 tick/allocation/streaming/race/lifetime risks remain pending. CR11-001/002/003 were not reviewed in this increment and remain pending; this review only cross-references their area. The project-wide review remains incomplete.
- **Observable impact:** CR17-002 now has traceable persistence and restore evidence, and RF180 records a bounded PostLogin grid-sync inconsistency for later remediation after CR19. Runtime behavior is unchanged.
- **Networking/authority:** Review evidence only; no runtime state changed. Server-side payment, save writes, and restore remain authoritative; the affected grid state is private to the owner.
- **Known limitations / remaining scope:** The restore mismatch was inferred from static call-path inspection and was not exercised in a live reconnect or multiplayer scenario. Only the recorded ranges of the changed GameMode file were reviewed. No performance profile was run.
- **Next eligible task:** CR17-003, trace combat and reward persistence.

## RF184 — Restore project-local source paths for imported UI textures (P3)

- **Source:** CR13-026 through CR13-033; 59 imported UI texture packages and their checked-in project-local source PNGs.
- **Affected path:** `Content/Kalmala/UI/` imported panel/button textures and item icons, plus their `Content/Kalmala/UI/Source/` PNGs.
- **Trigger:** a maintainer reimports a shared UI texture from another checkout or a clean project mirror.
- **Impact:** the textures render, but source reimport depends on missing paths or absolute paths in other worktrees, reducing reproducibility and risking edits from the wrong checkout.
- **Evidence:** 59 embedded source MD5 values match project-local PNGs; current-main and CR01 package hashes match. The package records include one missing `Import/OptionsPanel.png` path, absolute main-checkout paths, and paths into removed worktrees. Focused validators confirm the affected catalogue icon originals, prepared PNGs, mappings, and packages exist.
- **Proposed repair:** reimport all 59 textures from their existing project-local PNGs while preserving package identities, pixels, and settings.
- **Dependencies:** After CR19 review acceptance; no asset edits during CR13.
- **Acceptance:** all 59 package records resolve to their project-local source PNGs with matching MD5s and reimport works from a disposable short-path mirror.
- **Focused validation:** inspect package paths and source MD5s after reimport; run relevant UI icon validators and `git diff --check`; broad Unreal/menu verification is not needed for this metadata repair.

## RF190 — Return the test adapter's result over the named pipe (P2)

- **Source:** CR15-014; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSServerCommandlet.cpp:41-89` and `VSTestAdapterCommandlet.cpp:88-167,277-287`.
- **Affected path:** Visual Studio sends a test discovery or test-run request to the persistent `VSServer` named-pipe commandlet.
- **Trigger:** The server initializes the response to `"0"`, invokes `VSTestAdapterCommandlet::Main`, stores its return in `subCommandletResult`, but never uses that value. The exception handler also leaves/sets the response to `"0"`; only an unrecognized request changes it to `"1"`.
- **Impact:** The pipe response reports command success even when the adapter returns failure, such as when it cannot create its output file or a test run reports failure. The caller cannot distinguish successful adapter execution from a failed subcommand using the response.
- **Evidence:** `VSTestAdapterCommandlet::Main` returns the list/run result directly. `ListTests` and `RunTests` return 1 on output-open failure, and `RunTests` returns 1 when any requested test fails. `VSServerCommandlet` writes its `result` string to the pipe and uses `"1"` for an unknown request, but discards the adapter's result.
- **Proposed repair:** Define the pipe response contract and propagate the adapter's execution/setup status; keep per-test outcomes in the results file if the protocol distinguishes request failure from test failure.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A successful adapter request returns the documented success response; output/setup failures return a failure response, and failed test cases remain represented accurately in the results file.
- **Focused validation:** Exercise successful discovery, output-file failure, and a failing test through the persistent server; verify both the pipe response and results file. No server or Visual Studio session was run during review.

## RF191 — Close each Visual Studio named-pipe handle (P3)

- **Source:** CR15-014; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSServerCommandlet.cpp:41-91,105-110`.
- **Affected path:** The long-running `VSServer` handles successive Visual Studio requests.
- **Trigger:** Each successful `CreateFile` call stores a pipe handle in `HPipe`, but the request function returns without calling `CloseHandle` on either the normal or error path.
- **Impact:** The persistent server retains one kernel handle for each successfully opened request pipe. Repeated test discovery or execution calls accumulate handles for the lifetime of the Unreal process.
- **Evidence:** Read the complete request function and its loop caller; no cleanup path closes `HPipe`. Microsoft's [CloseHandle documentation](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle) says applications should close each handle they open when finished with it.
- **Proposed repair:** Use scoped handle ownership or an equivalent cleanup path that closes every successfully opened pipe handle after the response, including when state inspection or I/O fails.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every successful pipe open has exactly one close on all non-process-exit paths, and repeated requests do not grow the server's open-handle count.
- **Focused validation:** Run many request cycles against a controlled named-pipe peer and compare the server's process handle count before and after. No pipe session was run during review.

## RF192 — Fail when a requested test list cannot be read (P2)

- **Source:** CR15-014; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.cpp:25-56,88-107,167`.
- **Affected path:** Visual Studio asks the test adapter to run tests from a list file that is missing or unreadable.
- **Trigger:** `ReadTestsFromFile` logs an open failure and returns without signaling failure. `RunTests` then sees an empty `TestInfos`, leaves `AllSuccessful` true, executes no tests, and returns zero.
- **Impact:** The adapter reports a successful run even though it ran none of the requested tests, producing a false-success outcome and an empty results file.
- **Evidence:** At lines 31-35, an unreadable list causes an early return from a `void` function. Lines 97-107 continue with the empty array and `AllSuccessful = true`; the zero-iteration loop reaches the success return at line 167.
- **Proposed repair:** Return and propagate a read status from `ReadTestsFromFile`, and fail the run when the requested list cannot be opened or resolved according to the chosen empty-list contract.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** A missing or unreadable list produces a nonzero commandlet result and no success response; valid requested tests still run and report their individual outcomes.
- **Focused validation:** Run with a missing list file, a valid list file, and a list with no currently registered tests; verify exit/pipe status and result records. No commandlet run was performed during review.

## RF193 — Align Test Adapter filter help with the applied default (P2)

- **Source:** CR15-014; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.cpp:184-276`; installed UE 5.8.2 `Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h:127-149` and `Private/Misc/AutomationTest.cpp:857-871`.
- **Affected path:** Visual Studio discovers or runs tests without an explicit `-filters` argument, or supplies the documented default list.
- **Trigger:** Help advertises `application+smoke+product+perf+stress+negative`, but the no-argument mask is Product, Smoke, Perf, and Engine. The explicit-list parser has an `engine` branch but no `application` branch, so passing the documented string removes Engine and enables Stress/Negative; the no-argument and documented defaults therefore differ.
- **Impact:** Stress and Negative tests are omitted from default discovery/run despite the help saying they are included; using the documented default string explicitly selects a different set. Unreal's `GetValidTestNames` filters by intersection with the requested filter mask.
- **Evidence:** Lines 184-185 state the documented defaults; lines 216-217 construct a mask without Stress or Negative and with Engine; lines 230-273 toggle Engine/Stress/Negative based on the supplied string. Installed Unreal defines the supported filter bits and applies the requested mask when returning valid tests.
- **Proposed repair:** Make the default mask and help text describe the same supported filter categories; define how the `application` label maps, or replace it with the intended Unreal filter name.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Running without `-filters` and passing the documented default select the same categories, including the intended Stress and Negative behavior; each supported category can be deliberately included or excluded.
- **Focused validation:** Compare discovered test names across no-filter and explicit-default calls using one test per filter category. No commandlet or test run was performed during review.

## RF194 — Handle `-help` as a commandlet switch (P3)

- **Source:** CR15-014; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VSTestAdapterCommandlet.cpp:202-214,277-287`; installed UE 5.8.2 `Engine/Source/Runtime/Engine/Classes/Commandlets/Commandlet.h:142-159`.
- **Affected path:** A developer or Visual Studio invokes the test adapter with the documented help switch.
- **Trigger:** Unreal's commandlet parser places bare `-help` in `Switches`; `Main` checks only `ParamVals.Contains(HelpParam)`.
- **Impact:** The bare help switch does not take the early success exit. With no operation it falls through to the final help print and returns 1; when combined with list/run parameters, the requested operation proceeds instead of help exiting immediately.
- **Evidence:** The adapter parses both `Switches` and `ParamVals` at lines 204-209 but checks only the parameter map at line 210. The engine parser moves only key/value switches into `Params` and leaves bare switches in `Switches`. The adjacent shared commandlet base correctly checks `Switches.Contains(HelpSwitch)`.
- **Proposed repair:** Recognize the bare help switch in `Switches` (and support key/value form only if intentionally required) before setting filters or dispatching an operation.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Bare `-help` prints help, exits zero, and does not list or run tests, even if other operation parameters are present.
- **Focused validation:** Invoke with `-help` alone and alongside list/run parameters; verify output, exit status, and that no tests or files are touched. No commandlet run was performed during review.

## RF205 — Coalesce timer-driven construction save writes (P2)

- **Source:** CR18-002, CR03-001b-06/07; `Source/KalmalaGameplay/Private/KalmalaConstructionActor.cpp:40-44,143-180`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:819-918`; `Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:160-173`; `Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:362-363`; and `docs/02-technical-architecture.md:666`.
- **Affected path:** Server timer updates for active Charcoal Kilns, Smelters, Garden Plots, and Standing Lights in one world's construction-save slot.
- **Trigger:** Construction actors tick at a one-second interval. Each active actor advances its timer and calls its own GameMode persistence method. Each call clones and validates the complete construction save, then synchronously writes the same slot. The current save permits up to 128 records.
- **Impact:** With multiple active timer actors, the server repeats full-world validation/serialization and synchronous slot writes once per actor per second. At the record cap this can produce up to 128 full-slot writes per second and add synchronous work to the server game thread. The player-visible hitch size has not been measured.
- **Evidence:** `AKalmalaConstructionActor` sets `PrimaryActorTick.TickInterval` to `1.0f`; active kiln, smelter, growing-plot, and fueled-light Tick branches advance state before calling separate `Persist*State` methods. Each method builds a new candidate and calls `SaveGameToSlot`; `MatchesWorld` iterates every record and `MaxRecords` is 128. All methods construct the same world-scoped slot name. No existing review finding covers this repeated timer-driven full-save path.
- **Proposed repair:** Batch timer-only state changes into one server-owned construction-save candidate and at most one slot write per world per one-second service interval. Leave planting, fueling, collection and harvest on their existing transaction paths, including their payment, compensation, and rollback ordering; do not add offline progress.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize implementation during CR03 or CR18.
- **Acceptance:** With timer updates active across the 128-record bound, timer-only updates perform at most one full slot write per world per service interval and persist every due record before actor replication. Explicit planting, fueling, collection and harvest transactions retain their current payment, save, apply, compensation and rollback ordering outside the timer batch. A failed timer batch publishes none of its actor-state updates. No offline time is applied.
- **Focused validation:** Add a controllable writer/candidate test that counts writes for multiple active timer actors, verifies every due record in the successful save, and confirms a rejected write leaves live actor states unchanged. Profile server tick and save duration at the record cap after an Editor build. No runtime performance measurement was run during review.

## RF207 — Keep empty native-class filters from scanning every Blueprint (P2)

- **Source:** CR15-013; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VisualStudioToolsCommandlet.cpp:354-413,459-487`.
- **Affected path:** A developer runs the VisualStudioTools indexing commandlet with a filter path that resolves to no native modules/classes.
- **Trigger:** `GetNativeClassesByPath` leaves `FilterBaseClasses` empty. `RunAssetScan` still creates the recursive Blueprint class filter, but transforms no entries into `TagsAndValues`; the Asset Registry query therefore has no native-parent restriction.
- **Impact:** A scoped scan unexpectedly loads and indexes every Blueprint in the project, potentially incurring the broad scan cost the `-full` option warns can be very slow and returning results outside the requested path.
- **Evidence:** The commandlet help at lines 432-433 promises that `-filter` scans Blueprints derived from native classes under the provided path. Lines 459-470 can produce an empty class list; lines 380-390 then add no native-parent tags, and lines 410-415 query every asset matching only the Blueprint class filter.
- **Proposed repair:** When a scoped scan resolves no native classes, serialize an empty index or return a clear filter error instead of querying the registry without parent tags.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** An explicit/default scoped path with no matching native classes does not enumerate or load unrelated Blueprints; `-full` retains its documented full-scan behavior.
- **Focused validation:** Run against a project/path with no matching native modules and confirm the output is empty or the commandlet reports a clear error without processing unrelated assets. No commandlet run or build was performed during review.

## RF208 — Make COM moniker enumeration cleanup safe and complete (P2)

- **Source:** CR15-013; `Plugins/VisualStudioTools/Source/VisualStudioTools/Private/VisualStudioToolsBlueprintBreakpointExtension.cpp:119-185`.
- **Affected path:** A developer invokes the Visual Studio breakpoint menu while the extension enumerates the Running Object Table.
- **Trigger:** If `CreateBindCtx` fails, the uninitialized `BindContext` pointer is still released at line 174. On successful `GetDisplayName` calls, the returned `OutName` allocation is never freed. If `EnumRunning` fails, the acquired `RunningObjectTable` reference is not released.
- **Impact:** A bind-context creation failure can cause an invalid release/crash; successful enumeration leaks one COM-allocator string per moniker, and enumeration failure leaks a COM interface reference.
- **Evidence:** Read the complete function and traced every acquisition/release branch. `BindContext` and `OutName` are uninitialized at declaration; the bind context release is unconditional. The error branch after `EnumRunning` logs and returns without releasing the ROT. Microsoft's [IMoniker::GetDisplayName documentation](https://learn.microsoft.com/en-us/windows/win32/api/objidl/nf-objidl-imoniker-getdisplayname) states that the caller must free the returned string with the COM task allocator.
- **Proposed repair:** Initialize output pointers, release only successfully acquired interfaces, and free each successful display-name result with the COM task allocator (or remove the unused display-name call if it is not needed).
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** Every successful COM allocation/reference is released exactly once on success and failure paths; a failed bind-context creation cannot release an invalid pointer.
- **Focused validation:** Exercise successful enumeration and injected failures from bind-context creation and ROT enumeration; confirm no invalid release or retained COM allocation/reference. No Visual Studio session or failure injection was run during review.

## RF209 — Make the VisualStudioTools log category's public visibility consistent (P3)

- **Source:** CR15-015; Plugins/VisualStudioTools/Source/VisualStudioTools/Public/VisualStudioTools.h:5-7, Private/VisualStudioTools.cpp:4,9, VisualStudioTools.Build.cs:48-53, and VisualStudioTools.uplugin:20-28.
- **Affected path:** A downstream C++ module declares a dependency on VisualStudioTools, includes its Public header, and references LogVisualStudioTools in a modular Win64 Editor build.
- **Trigger:** The Public header declares LogVisualStudioTools with DECLARE_LOG_CATEGORY_EXTERN but no VISUALSTUDIOTOOLS_API; its definition is in the plugin's private .cpp. Installed UE 5.8.2 LogCategory.h:133-135 expands the declaration to an ordinary external variable declaration; it does not add a module export specifier. Epic documents that Public headers are visible to dependent modules and that module API specifiers export symbols across module DLLs ([module structure](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules), [module API specifiers](https://dev.epicgames.com/documentation/unreal-engine/module-api-specifiers-in-unreal-engine)).
- **Impact:** A dependent module can include the advertised header but cannot link the category symbol across the plugin DLL boundary. No external C++ consumer exists in this repository, and the plugin documentation describes Visual Studio integration through Unreal commandlets, so no current in-tree consumer failure was observed.
- **Evidence:** The header contains only CoreMinimal.h and the bare category declaration. VisualStudioTools.cpp defines the category; all repository references are private source files in the same module. The public dependency on Core matches the header include, and the descriptor restricts the module to Win64 Editor. The sibling debugger-helper module declares its log category in a Private header. No plugin public function, class, or data contract is otherwise exposed.
- **Proposed repair:** If this category is supported for downstream module use, add VISUALSTUDIOTOOLS_API to the declaration and document that contract. If it is internal logging only, move the declaration to a Private header and keep it out of the Public interface.
- **Dependencies:** After CR19 review acceptance; no repair during the review-only phase.
- **Acceptance:** The category's visibility matches the intended module contract. If public, a dependent module can compile and link against it in a modular Win64 Editor build; if internal, no Public header advertises it and the plugin's own module still builds.
- **Focused validation:** Build a small dependent Editor module that includes the header and logs through the category when the public contract is retained; otherwise build the plugin after moving the declaration to Private and confirm no public-header reference remains. No build was run during review.

## RF211 — Align current chest-storage schema references (P3)

- **Source:** CR18-002, CR03-001b-08; `docs/02-technical-architecture.md:545,1108,1416,1644`; `Source/KalmalaGameplay/Public/KalmalaStorageSaveGame.h:23`; and `Source/KalmalaGameplay/Private/KalmalaStorageSaveGame.cpp:50-59`.
- **Affected path:** Contributors and reviewers relying on the architecture document for current chest-save compatibility, record identity, and verification.
- **Trigger:** The M2, compatibility, and current-save sections state that storage schema 2 is current, while the implementation requires schema 3 and a later architecture section also identifies schema 3.
- **Impact:** The architecture gives conflicting save versions and can lead reviews or fixtures to assume schema-2 data is accepted. Runtime `MatchesWorld` rejects it; no gameplay defect was established.
- **Evidence:** `UKalmalaStorageSaveGame::CurrentSchemaVersion` is 3 and `MatchesWorld` requires that value. Architecture lines 545, 1108, and 1416 state schema 2; line 1644 says schema 3 records the placed buildable identity and older development schemas are rejected without rewrite.
- **Proposed repair:** Align present-tense storage schema references and the version table to schema 3. Label schema-2 descriptions as historical and state that pre-release incompatible saves are rejected without rewriting.
- **Dependencies:** After CR19 review acceptance; this is a review handoff only and does not authorize documentation repair during CR03 or CR18.
- **Acceptance:** Every current storage schema statement and version-table entry agrees with the implementation. Any retained schema-2 description is clearly historical and says it is unsupported; no migration is introduced.
- **Focused validation:** Search the architecture document for storage schema references and compare each present-tense claim with the schema constant and `MatchesWorld` guard. No runtime or save-compatibility test ran during this review.

## RF212 — Reject combined combat peer fixture selectors (P3)

- **Source:** CR18-002, CR03-001b-12; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1241-1264,1307-1310`.
- **Affected path:** Non-Shipping combat peer fixtures invoked with multiple `Kalmala*PeerTest` flags.
- **Trigger / impact:** A manual invocation combines otherwise recognized scenarios, for example `KalmalaBoarPeerTest` and `KalmalaDeerPeerTest`. The gate accepts this combination, the priority chain labels and selects the Boar archetype, and the Deer flag selects StoneKnife; the resulting fixture can exercise a composite scenario and misrepresent which isolated case was run. The supported `EnemyPeers.ps1` runner supplies one flag per scenario, so its configured runs are unaffected. No production gameplay impact was found.
- **Evidence:** The admission condition requires at least one recognized flag and rejects selected conflicts, but not every combination. `VerificationName` and `ExpectedArchetype` prefer Boar over Deer, whereas `ServerWeaponId` selects StoneKnife whenever `bDeerPeerTest` is true. The fixture is Shipping-excluded.
- **Proposed repair:** Require exactly one scenario flag before participant setup, or define and test an explicit combination matrix; log a clear rejection for unsupported combinations.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** Each supported single flag remains admitted, while unsupported multi-flag combinations terminate before setup with a clear diagnostic; configured `EnemyPeers.ps1` scenarios keep their existing behavior.
- **Focused validation:** Add a narrow selector/admission check for each supported flag and representative combinations, then run the relevant `Scripts/Tests/Combat/EnemyPeers.ps1` scenarios after an Editor build. No runtime test ran during review.

## RF213 — Search for a target in the CombatFeedback fixture (P3)

- **Source:** CR18-002, CR03-001b-13; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1264,1336-1339,1374-1385`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:940-978`; and `Scripts/Tests/Combat/EnemyPeers.ps1:11,37-39`.
- **Affected path:** The non-Shipping `CombatFeedback` peer scenario.
- **Trigger / impact:** The runner passes `KalmalaCombatPeerTest`, which defaults `ExpectedArchetype` to Mireling, but the bounded descriptor-search condition omits `bCombatPeerTest`. If the host's current spatial key has no Mireling, the search is skipped and the null expected descriptor terminates the fixture at stage 99. Meadow-only keys contain Boar, Deer, or Rat descriptors, while Mireling is reserved for dry Mossy Mire; the runner also permits a `-WorldSeed` override. The supported peer test can therefore fail before it checks hit/defeat feedback. No production gameplay impact was found.
- **Evidence:** `ExpectedArchetype` falls through to Mireling for the `CombatFeedback` flag. The search condition covers `bMirelingPeerTest`, Boar, Deer and pending-reward flags, but not `bCombatPeerTest`; after this block, a null `ExpectedDescriptor` logs failure and sets stage 99. `TryGetSpawnArchetypeForBiome` assigns Meadow slots to Boar/Deer/Rat and reserves Mireling for Mossy Mire. The runner maps `CombatFeedback` to `KalmalaCombatPeerTest` and supports an optional world-seed override.
- **Proposed repair:** Give `CombatFeedback` a consistent target-selection path: search a bounded set of keys for the required Mireling or choose a target archetype available in its configured area. Continue using ordinary generated descriptors.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** The default `CombatFeedback` seed and a representative alternate seed whose start key lacks a Mireling reach hit, defeat, unavailable-text, and remote-privacy checks through a bounded deterministic search. No test-only wildlife actor is created.
- **Focused validation:** Add narrow coverage for target selection when the start key lacks a Mireling, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario CombatFeedback` after an Editor build with the default and alternate seed. No runtime test ran during review.

## RF216 — Reclassify legacy buildable icon manifest rows (P3)

- **Source:** CR13-031; `Scripts/Data/catalogue-icon-manifest.csv:11-22`, the build-item catalogue, and the catalogue loader.
- **Affected path:** icon-manifest metadata for twelve placed-object identities.
- **Trigger:** a maintainer or UI audit uses manifest rows to decide which views may display a placed-object identity.
- **Impact:** rows advertise pack inventory, item-detail, and storage use for placed objects, misleading future authoring despite no current portable item or grant being confirmed.
- **Evidence:** twelve rows are marked `identity_kind=item`, `inventory_scope=pack`, and include PackInventory, ItemDetail, or Storage views. Catalogue identities and runtime aliases resolve to stable placed-object IDs; the manifest already shows buildable-only treatment for CampfireKit, CharcoalKilnKit, and CampBedKit.
- **Proposed repair:** reclassify the twelve rows as buildable/construction with no inventory scope and remove pack, item-detail, and storage views. Preserve valid recipe/build/preview views, IDs, and assets; do not create portable items.
- **Dependencies:** After CR19 review acceptance; no manifest edits during CR13.
- **Acceptance:** every placed-object row uses buildable/construction metadata with no pack, item-detail, or storage scope while retaining valid construction/preview views and existing IDs.
- **Focused validation:** compare manifest identities and view scopes with buildable catalogue data and runtime aliases; run catalogue/icon validators and `git diff --check`.

## RF236 — Thicken the Light Bow icon limbs to meet its recorded prompt

- **Source:** CR13-082; `Content/Kalmala/UI/Source/IconOriginals/LightBow.png`; `Content/Kalmala/UI/Source/Icons/LightBow.png`; `Content/Kalmala/UI/Icons/Items/LightBow.uasset`; `docs/icon-provenance.md#light-bow-icon`; `Scripts/Data/catalogue-icon-manifest.csv`; `Source/KalmalaUI/Private/KalmalaIconWidget.cpp`.
- **Severity:** P3. The icon is recognizable, but its prepared silhouette does not meet the authored small-scale visual target.
- **Trigger:** A player views the prepared LightBow icon at 64×64 in carried-tool, detail, repair, or recipe-result UI.
- **Impact:** The stave and wooden limbs read as a narrow stroke at inventory scale, reducing glance legibility and conflicting with the retained prompt's explicit requirement for thick sturdy limbs and a broad silhouette.
- **Evidence:** CR13-082 confirmed the original, prepared PNG, and imported package match their CR01 fingerprints. The original is 1254×1254 RGBA and the prepared PNG is 64×64 RGBA. The exact prompt at `docs/icon-provenance.md#light-bow-icon` requires limbs clearly visible at 64px and says not to make hair-thin lines. Visual inspection at native size shows a very slender stave, although the overall curved bow remains recognizable. The registered catalogue contains `LightBow`; its manifest row and canonical runtime alias map it to the Bow icon. The package's source path resolves to the project-local prepared PNG.
- **Proposed repair:** After CR19 review acceptance, revise the source artwork to give the wood limbs a clearly broader silhouette at 64×64 while retaining the compact arc, central grip, separated taut string, no-arrow constraint, established style, and existing item identity. Reprepare and reimport without changing gameplay or runtime mapping.
- **Dependencies:** After CR19 acceptance; no art or package edits during CR13.
- **Acceptance:** The prepared 64×64 icon has clearly visible sturdy wooden limbs and a broad recognizable bow silhouette, with the string visibly separate; the LightBow ID, manifest role, and runtime mapping remain unchanged.
- **Focused validation:** Visually inspect the original and prepared icon at native 64×64 size; check dimensions, transparency, and LightBow package identity; run `Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id LightBow`, the catalogue-reference validator, and `git diff --check`. No Unreal build is required.

## RF237 — Attribute snake bite observations to the selected target (P3; CR03-001b-19)

- **Source:** CR18-002, CR03-001b-19; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1388-1394,1610-1622,1679-1700; Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:353-377; and Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:3299-3320.
- **Affected path:** The retained non-Shipping Viper and Giant Snake peer-verification flags.
- **Trigger:** In stages 1–2, the fixture latches Viper success from any active WeakPoison status on the attacker and Giant Snake success from any attacker health below 100. It then disables the selected target's actor tick, without isolating it from other spawned wildlife in the activated key. Giant Snake setup heals the attacker but leaves prior poison statuses running; any poison tick or another active actor's hit can supply the health-loss signal.
- **Impact:** The species fixture may log Bite=1 and stop the selected target before that target has produced the asserted bite. This weakens the test's ability to catch a broken Viper/Giant Snake attack. No production gameplay impact was found.
- **Evidence:** The setup calls ActivatePopulationKey(SpatialKey, true), whose wildlife branch creates actors for eligible key descriptors, and stores a weak pointer only for the expected spawn. The Viper observation checks HasStatus(WeakPoisonStatusId) without source identity; TryApplySnakeBite applies weak poison after accepted damage for any Viper. The GameMode advances player statuses each tick, and the Giant Snake observer checks only Attacker->GetHealth() < 100.0f; the preceding Giant Snake reset path restores health but does not expire poison. The observation code disables only the selected Target and later emits the boolean as Bite. None of these paths associates the event with the target's persistent spawn ID.
- **Proposed repair:** Emit or retain a server-owned accepted-bite result tied to the selected target's persistent spawn ID and use that exact event for the fixture assertion. Do not treat aggregate player status or health change as proof of the selected snake's attack.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** Only a server-accepted bite from the selected Viper or Giant Snake can set its bite-observed state or stop the target. Existing poison, a sibling actor's damage/status effect, or unrelated health loss leaves the bite assertion false; a valid bite from the selected target reaches the existing success path.
- **Focused validation:** Add a narrow Development regression where the selected target cannot attack while the attacker has residual poison or receives non-target damage, and assert that no bite is recorded. Then allow the selected target's accepted bite and assert it is recorded. Run the focused poison and snake contracts after an Editor build; if a multiplayer peer case is added, retain it in the current runner and verify owner/observer results. No runtime verification ran during review.

## RF238 — Check every observer in retained Badger/Moon Bear escrow and retry modes (P3; CR03-001b-22,24)

- **Source:** CR18-002, CR03-001b-22; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1818-1861,1887-1892`.
- **Affected path:** The retained non-Shipping Badger and Moon Bear pending-reward full-pack escrow and retry fixture modes.
- **Additional source:** CR03-001b-24; GameMode lines 1971–2017.
- **Trigger:** During retry with at least two non-owner peers, the owner receives the expected reward and another observer also receives Bone Fragment or (for Badger) Fur/Meat. The generic `bExpectedReward` branch checks only the selected `RemoteInventory`; unlike Rat/Viper/Giant Snake branches, it ignores all `OtherPeer` quantities and the success result still prints `OwnerOnly=1`.
- **Impact:** A Development peer fixture can falsely report owner-only reward delivery while a non-selected observer holds a duplicate. No production reward leak was established. The current consolidated runner uses the Rat `SharedCombat` path, which has observer checks; Badger and Moon Bear live reward modes are retired from that runner.
- **Evidence:** The function enumerates non-owner player characters and aggregates `OtherPeerBoneFragment`, `OtherPeerRatTail`, `OtherPeerViperFang`, and `OtherPeerGiantScale`. `bExpectedReward` requires those observer totals to be zero only in the Rat/Viper/Giant Snake branches. The Badger/Moon Bear branch checks `RemoteBone`, `RemoteFur`, and `RemoteMeat` but not any other peer; the generic success log then asserts `OwnerOnly=1`.
- **Proposed repair:** Aggregate the expected reward item set across every non-owner inventory and require zero observer totals on both retained branches before reporting owner-only success. Preserve the existing exact owner counts, pending-claim absence, and saved-defeat checks.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** With an owner and two non-owner peers, successful Badger and Moon Bear retries grant the exact expected items only to the owner. Injecting Bone Fragment or the relevant Badger Fur/Meat into either non-owner makes the fixture fail before any owner-only pass marker.
- **Focused validation:** Add a narrow Development regression for both modes that injects an unexpected reward into the second observer during retry and verifies failure; run it after an Editor build. The current `EnemyPeers.ps1` runner does not include these retired species modes.


### CR03-001b-24 full-pack escrow evidence for RF238

The escrow predicate (GameMode lines 1973–2017) enumerates non-owner inventories, but enforces their reward totals only under bTwoItemRewardPeerTest. That flag is false for Badger and Moon Bear. The loop does not collect Badger Fur/Meat from any observer. With an owner, selected remote, and third peer, the third peer can hold Bone Fragment (Moon Bear or Badger) or Badger Fur/Meat while escrow passes the checked zero-quantity conditions for the owner and selected remote. The full-pack escrow predicate passes and the fixture proceeds to retry. This is the same missing-observer assertion as the B22 retry branch, so it remains RF238 rather than a duplicate.

For both retained modes, acceptance now covers the full-pack escrow point and the later retry point: a relevant reward in either non-owner inventory must fail before the corresponding pass marker. No production leak was established; the current runner uses the Rat path.

## RF239 — Require accepted attacks before retained reward fixture completion (P3; CR03-001b-23)

- **Source:** CR18-002, CR03-001b-23; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1240,1756-1765,1910-1959,1887-1892`; `Source/KalmalaGameplay/Private/KalmalaCombatComponent.cpp:34-56`.
- **Affected path:** The retained non-Shipping Badger and Moon Bear pending-reward retry fixture modes.
- **Trigger:** At least one attack request is rejected, but the fixture request counter increments to four. The Badger/Moon Bear flow proceeds to fixture-only defeat and owner-claim lookup without checking the actual accepted action serial.
- **Impact:** A Development fixture can report its four-attack reward setup passed after fewer than four server-accepted attacks. This is a fixture false pass only; no production combat or reward defect was established. The active Rat SharedCombat path requires natural defeat, and the Viper/Giant Snake branches require exact action serials.
- **Evidence:** `ExpectedAttackCount` is four except for the Rat path. The stage-two loop calls `ServerRequestAttack(++CombatPeerTestSequence)`, so the fixture counter advances before the void RPC returns. `ServerRequestAttack_Implementation` exits on a rejected request before incrementing `ActionSerial`. The stage-two completion gate waits for the expected fixture request count and Idle phase, but its exact serial check applies only to Viper/Giant Snake. Rat separately fails if its single accepted hit did not defeat the target. Badger/Moon Bear can then apply up to four fixture damage hits and directly call `DefeatServer` before checking the correctly derived scenario claim ID and authenticated owner. The later retry success result reports the literal `ActionSerial=4` without checking the action component's value.
- **Proposed repair:** Require `AttackerCombat->GetActionSerial() == ExpectedAttackCount` before fixture-only defeat and owner-claim success for both retained modes, retaining Rat's natural-defeat guard and the snake checks. Print the actual serial rather than a constant.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** With Badger and Moon Bear modes, four accepted requests still reach the bounded fixture defeat and exact owner-claim/retry path. If one request is rejected while the attempt counter reaches four, the fixture stops before fixture defeat or claim success and emits no pass marker. Existing Rat and snake gates remain intact.
- **Focused validation:** Add a narrow Development regression for each retained mode that forces a rejected request during the four-attempt sequence and confirms failure before reward setup, then confirms four accepted requests reach the current completion path. Run after an Editor build; the current consolidated runner does not include these legacy modes.

## RF240 — Check every peer in active Boar/Deer combat reward acceptance (P3; CR03-001b-26)

- **Source:** CR18-002, CR03-001b-26; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:1288-1290,2057-2086; Scripts/Tests/Combat/EnemyPeers.ps1:50-69,218-221,276-282.
- **Affected path:** The current non-Shipping BoarCharge and DeerHerd peer scenarios' final combat reward predicates.
- **Trigger:** The host receives the expected Boar or Deer reward and the selected first remote has none, but the second remote holds a duplicate relevant item. The final bRewardValid condition inspects only AttackerInventory and RemoteInventory, so this extra reward does not block Passed=1.
- **Impact:** An active Development combat scenario can pass its reward predicate while a non-owner peer holds a duplicate. This is a fixture false pass; no production reward leak was established.
- **Evidence:** Stage-zero setup assigns LocalServerPlayer as CombatPeerTestAttacker and only the first remote actor as CombatPeerTestRemote. It requires a remote, but its two-remote minimum is gated only by bTwoItemRewardPeerTest, which is false for Boar and Deer. The Boar branch checks BoarMeat, BoarHide, BoneFragment on the attacker and zero of those IDs on RemoteInventory. The Deer branch similarly checks DeerMeat, DeerHide, BoneFragment, optional Sinew on the attacker, and zero on RemoteInventory. Neither branch inspects any additional player inventory. EnemyPeers sets Clients=2 for BoarCharge and DeerHerd, starts a client and then a separate observer. Its scenario predicates check the server Passed=1 output and combat/replication results, not the observer inventory; that later observer identity check does not invalidate a pass marker emitted before the observer arrived. RF46 covers reward-callback dispatch/persistence tests, while RF238/RF239 cover distinct Badger/Moon Bear pending-reward paths; they do not cover this final Boar/Deer peer assertion.
- **Proposed repair:** Require the two non-owner peers used by the current BoarCharge/DeerHerd runner before fixture progression, then aggregate each scenario's expected reward IDs across every non-owner inventory and require all totals to be zero before emitting the final pass marker. Preserve the exact attacker reward counts, action-serial gate and Deer herd-alert gate.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** Existing BoarCharge and DeerHerd runs wait for two non-owner peers, then pass only when the host has exact expected rewards and both remotes have zero relevant items. A relevant reward injected into the second remote, or a missing second remote, prevents Passed=1. Existing combat, health-replication, action-serial and Deer herd assertions remain intact.
- **Focused validation:** Add narrow Development fixtures for both scenarios that confirm no pass marker is emitted before two non-owner peers are present, inject each relevant reward into the second remote, and verify failure. Run after an Editor build using the current EnemyPeers runner.

## RF241 — Verify Mosquito dodge cooldown after the active roll ends (P3; CR03-001b-30)

- **Source:** CR18-002, CR03-001b-30; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2312-2337; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:136-143,1273-1311,1331-1342; Scripts/Tests/Combat/EnemyPeers.ps1:133-143; docs/biome-enemies.md:594-596.
- **Affected path:** The current non-Shipping MosquitoCounterplay fixture's dodge cooldown acceptance.
- **Trigger:** After the first roll is accepted, the fixture sends a second owner-client request immediately and advances when any one rejection is counted.
- **Impact:** The peer scenario may pass without exercising the cooldown gate. No production dodge defect was established.
- **Evidence:** The accepted server request sets `DodgeRollSecondsRemaining` to the documented 0.35 seconds, sets `DodgeRollCooldownRemaining` to 1 second, and marks the roll active. The admission helper rejects `bAlreadyRolling` before evaluating cooldown. GameMode stage three immediately issues the next client request after observing one accepted request; stage four only checks the aggregate rejection counter. The server increments that counter for all rejection paths, and the current EnemyPeers acceptance checks only `DodgeAccepted=1 DodgeRejected=1`. No inspected assertion ties the rejection to an expired roll with positive cooldown remaining.
- **Proposed repair:** Wait until the active roll ends while the cooldown remains positive before sending the probe, ensure the other admission conditions are valid, and record or assert the cooldown-specific rejection.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** One accepted roll is followed by a request after the active-roll window but before cooldown expiry. The fixture fails if that request is accepted or rejected by another gate, and passes only when positive cooldown is the blocking condition. Existing mosquito escape and flame checks remain intact.
- **Focused validation:** Add a narrow Development regression for expired-roll/positive-cooldown rejection and for the otherwise-valid cooldown-expired control case. Confirm the peer scenario fails if cooldown enforcement is bypassed. Run after an Editor build with `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario MosquitoCounterplay`.

## RF242 — Verify the full two-second Mosquito repel window (P3; CR03-001b-31)

- **Source:** CR18-002, CR03-001b-31; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2350-2353,2416-2420; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:146-151,1314-1318; Source/KalmalaGameplay/Private/Tests/KalmalaDodgeRollTest.cpp:41-42; Scripts/Tests/Combat/EnemyPeers.ps1:133-139; docs/biome-enemies.md:552-555,594-596.
- **Affected path:** The current non-Shipping MosquitoCounterplay fixture's server-side swarm-repel timer verification.
- **Trigger:** The applied repel deadline is materially earlier than two seconds (for example 1.5 seconds) while the configured contract constant remains 2 seconds.
- **Impact:** The fixture may pass after the swarm stops being repelled before the documented two-second window. No current production timing defect was established.
- **Evidence:** The accepted dodge sets `MosquitoRollRepelUntilTime` from the server world time plus `DodgeRollMosquitoRepelSeconds`. Stage five waits until at least 1.25 seconds after `MosquitoPeerTestAcceptedRollTime`, then requires the repelling query to be true. Stage six waits until at least 2.05 seconds after that timestamp, then requires the query to be false. There is no positive observation near the two-second boundary; a 1.5-second applied window can satisfy both checks. `KalmalaDodgeRollTest` checks that the configured constant equals 2.0 but does not test timed behavior near expiry, and the peer runner checks only `RollExpired=1`. This is distinct from RF241, which concerns the reason for the immediate retry rejection.
- **Proposed repair:** Verify the repelling query near the full two-second boundary and just after expiry using the authoritative server acceptance/expiry timestamp with a defined timing tolerance.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** A materially shortened window fails; the query remains true near the documented two-second duration and is false shortly after expiry. Existing accepted-roll, cooldown and held-flame assertions remain intact.
- **Focused validation:** Add a narrow timed Development regression for the server-applied deadline, including a shortened-window control that must fail. Run after an Editor build and retain the MosquitoCounterplay peer check.

## RF243 — Verify Mosquito bites around a paid, unlit campfire (P3; CR03-001b-33)

- **Source:** CR18-002, CR03-001b-33; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2463-2484,2485-2503; Source/KalmalaGameplay/Private/KalmalaCampfire.cpp:69-72,205-214; Source/KalmalaGameplay/Private/Tests/KalmalaHearthStateTest.cpp:24-30; Scripts/Tests/Combat/EnemyPeers.ps1:133-139; docs/biome-enemies.md:562-570.
- **Affected path:** The current non-Shipping MosquitoCounterplay fixture's unlit placed-campfire assertion.
- **Trigger:** The campfire is spawned and checked for no flame eligibility before `InitializePaidFromServer`; the fixture then waits for a bite while it is still unfuelled and initializes and lights it consecutively after the bite.
- **Impact:** MosquitoCounterplay can report `CampfireUnlitNoProtection=1` without proving that a valid, fueled, extinguished placed campfire permits bites. No production campfire or mosquito defect was established.
- **Evidence:** Stage seven's new campfire is only spawned before `IsLit()` and `IsFlameEligibleForMosquitoesFromServer()` are checked. Stage eight repeats those checks and waits for the bite before calling `InitializePaidFromServer`, then immediately calls `TryLightFromServer`. The flame query requires authority, lit state, positive finite fuel and positive finite warmth; the default uninitialized campfire has no paid fuel. `InitializePaidFromServer` sets owner and fuel without lighting it, so a valid paid-but-unlit state can be held for a bite observation. The hearth-state test covers fuel remaining while unlit but does not connect that state to Mosquito bites. No other current Mosquito check covers this paid-unlit state.
- **Proposed repair:** Initialize/pay the campfire while keeping it extinguished, assert positive fuel, `!IsLit()` and no flame eligibility, then require a selected-swarm bite while that paid-unlit state remains. Preserve the subsequent ignition and lit-fire no-bite checks.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** The peer fixture cannot emit `CampfireUnlitNoProtection=1` until it has observed a selected-swarm bite with the campfire owned, fueled, extinguished and flame-ineligible. A control that skips initialization or lights the fire before the bite fails; the existing lit-fire repel assertion remains intact.
- **Focused validation:** Add a narrow Development regression or peer-fixture check for a paid, extinguished campfire beside the selected swarm, including an unfuelled control and the existing lit-fire transition. Run after an Editor build with `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario MosquitoCounterplay`. No test was run during this review.

## RF244 — Require weak poison on the bitten owner peer (P3; CR03-001b-35)

- **Source:** CR18-002, CR03-001b-35; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2136-2144,2530-2551; Source/KalmalaGameplay/Private/KalmalaPlayerStatusComponent.cpp:99-103,493-507; Scripts/Tests/Combat/EnemyPeers.ps1:133-152,276-283; docs/07-development-setup.md:2592-2607.
- **Affected path:** The current non-Shipping MosquitoCounterplay peer privacy acceptance.
- **Trigger:** The bitten-owner client has no replicated weak-poison status, while the observer client logs one; the two results are exclusive, so the runner's XOR passes.
- **Impact:** The peer scenario can pass if poison is missing from its intended owner or appears only on the observer, so it does not fully verify owner-only status replication. No production privacy leak was established.
- **Evidence:** GameMode stage zero assigns `Peers[0]` as the bitten Host and `Peers[1]` as observer. The runner starts `client`, waits for its first remote join, then starts `observer`, documenting stable owner/observer ordering. The server completion predicate confirms the bitten Host has at least eight seconds of weak poison. `OnRep_Statuses` logs any replicated weak poison on a non-authority character but does not encode the designated role. The runner computes `$clientPoison` and `$observerPoison`, then accepts `$clientPoison -xor $observerPoison`; therefore `client=false, observer=true` passes. The component currently uses `COND_OwnerOnly`, while development setup requires the owner client to see poison and the other peer not to receive it.
- **Proposed repair:** Make the peer assertion role-specific: require poison in the designated bitten-owner client log and reject any poison in the observer log. Include explicit role or pawn identity in the observation if needed to keep the association unambiguous.
- **Dependencies:** After CR19 review acceptance; no runner repair during CR03 or CR18.
- **Acceptance:** Only owner-client poison with no observer poison passes. Owner-only absence, observer-only delivery, delivery to both peers, or delivery to neither peer fails; public-health checks and the server's poison-duration assertion remain intact.
- **Focused validation:** Add a narrow check for all four owner/observer Boolean combinations, then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario MosquitoCounterplay` after an Editor build. No test was run during this review.

## RF245 — Select Peat Husk fixtures from the activated population budget (P3; CR03-001b-36)

- **Source:** CR18-002, CR03-001b-36; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2612-2661,8320-8380; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:102-130; Source/KalmalaWorld/Public/KalmalaBiomeExpansionContract.h:43-62.
- **Affected path:** The non-Shipping PeatHuskPeerTest stage-zero descriptor selection and activation.
- **Trigger:** A Mossy Mire spatial key has multiple generated wildlife descriptors, activation truncates its 70% budget, and the first undefeated Peat Husk candidate is in the discarded tail.
- **Impact:** The Development peer fixture records an ID for a descriptor that activation will not spawn; stage one then times out waiting for that exact ID, even when a later key in the bounded search has a retained eligible Peat Husk. No production spawn defect was established.
- **Evidence:** Stage zero calls BuildSpawnDescriptors and searches all returned descriptors, applying archetype, 150 m hard-enemy start exclusion, nonempty stable ID and saved-defeat filters, but no biome budget truncation. The first raw match ends the spatial-key search. ActivatePopulationKey rebuilds the same descriptors and truncates wildlife to ApplyPopulationBudget for a Mossy Mire key; its profile multiplier is 0.70 and SetNum drops the array tail. Stage one later searches ActiveWildlifeSpawns by the stored persistent ID and fails after fifteen seconds if absent. The Mosquito selector in the same GameMode applies that same biome truncation before searching.
- **Proposed repair:** Apply the activation path's biome budget truncation to descriptors before selecting the Peat Husk, then run the existing distance and saved-defeat predicates on the retained array.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** A selected fixture ID always belongs to the descriptor prefix retained by activation. A trimmed-tail candidate does not stop the bounded search; it continues until a retained undefeated eligible Peat Husk is found or all keys are exhausted, then reports no candidate at stage zero if necessary. No discarded ID can cause a stage-one timeout.
- **Focused validation:** Add a narrow Development regression with a trimmed-tail candidate in an earlier key and a retained eligible candidate in a later key, plus a no-retained-candidate case. Then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario PeatHuskSweep -WorldSeed 445` after an Editor build. No test was run during this review.

## RF246 — Tie Peat Husk telegraph observations to the selected actor (P3; CR03-001b-37)

- **Source:** CR18-002, CR03-001b-37; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2791-2823; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:5143-5152,5318-5323; Scripts/Tests/Combat/EnemyPeers.ps1:156-166; docs/07-development-setup.md:2653-2660.
- **Affected path:** The PeatHuskSweep peer scenario's replicated-telegraph acceptance.
- **Trigger:** Another Peat Husk actor emits a telegraph marker to both clients while the selected fixture target's telegraph is absent from one or both client logs.
- **Impact:** PeatHuskSweep can pass without proving that both remote clients observed the selected target's replicated telegraph. No production replication defect was established.
- **Evidence:** The server fixture stores the selected Target weak reference, gates its server pass on that target's Telegraph and Recovery, and includes the selected persistent SpawnId in the success log. The OnRep handlers log the same generic client marker for any Peat Husk entering Telegraph and do not include an actor ID. The runner's server regex does not capture the SpawnId, and its two remote checks only search for the generic marker. Thus an unrelated Peat Husk cue can satisfy the client evidence while the selected actor's cue is missing on a peer.
- **Proposed repair:** Include the persistent spawn ID in each client telegraph marker, parse the selected fixture ID, and require that same ID in both client logs.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** Only the selected target's telegraph observation on both designated remote clients satisfies the peer cue gate; a marker from any other Peat Husk does not.
- **Focused validation:** Inject or fixture a second Peat Husk telegraph while suppressing the selected target's marker on one client and verify failure; verify matching selected-ID markers on both clients pass. Run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario PeatHuskSweep -WorldSeed 445` after an Editor build. No test was run during this review.

## RF247 — Continue fixture search when a Peat Husk has no dry approach (P3; CR03-001b-37)

- **Source:** CR18-002, CR03-001b-37; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2612-2649,2735-2770; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:837-840; Scripts/Tests/Combat/EnemyPeers.ps1:156-166.
- **Affected path:** The PeatHuskPeerTest candidate-to-host placement setup.
- **Trigger:** The first activation-retained Peat Husk has no direction among the 16 tested angles with three dry Mossy Mire samples along the 120 cm approach, while a later bounded-search key has another activation-retained candidate that does.
- **Impact:** The Development peer fixture can fail on an unsuitable first spawn even when another activation-retained Peat Husk in a later key can support the sweep. No production spawn or attack defect was established.
- **Evidence:** Stage zero selects by archetype, hard-enemy start distance, stable ID and saved-defeat status without testing whether the fixture can place the host. Stage one searches 16 radial directions with three dry Mossy Mire samples each. If none passes, it sets terminal failure and returns; it does not revisit descriptor candidates or search other spatial keys. Thus a later usable candidate cannot rescue the fixture.
- **Proposed repair:** After activation-budget filtering, require a viable fixture approach while selecting candidates, or continue the bounded search when the first target has no supported approach. Report no suitable fixture only after exhausting eligible candidates.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** An activation-retained candidate with no valid sampled dry approach does not stop the search when a later activation-retained candidate has one; the fixture arms only after selecting and placing a target with a validated approach. If none qualifies, it reports no suitable candidate without an activation timeout.
- **Focused validation:** Add a narrow Development regression with an earlier no-approach Peat Husk and a later valid approach, plus a no-valid-approach case. Then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario PeatHuskSweep -WorldSeed 445` after an Editor build. No test was run during this review.

## RF248 — Select Wolf peer fixtures only from activation-admitted packs (P3; CR03-001b-39)

- **Source:** CR18-002, CR03-001b-39; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2897-2930,2963-2987,3068-3095,8308-8337,8390-8407`; `Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:85-122`; `Source/KalmalaWorld/Public/KalmalaBiomeExpansionContract.h:43-62`; `Source/KalmalaGameplay/Public/KalmalaWolfPackContract.h:15-20`; `Scripts/Tests/Combat/EnemyPeers.ps1:71-84`; `docs/biome-enemies.md:352-357,442-449`.
- **Affected path:** The non-Shipping WolfPack peer fixture's stage-zero pack selection and normal population-key activation.
- **Trigger:** The first complete undefeated pack with a valid host approach is in the descriptor tail removed by the 65% Tundra wildlife budget, or its three actors do not fit after earlier eligible wildlife actors consume the per-key budget, while a later activation-admitted pack exists.
- **Impact:** The fixture can store member IDs for a pack that activation will not materialize, then fail after its 20-second member wait despite another suitable pack being available. This does not establish a production Wolf spawning defect.
- **Evidence:** Stage zero scans the complete `BuildSpawnDescriptors` result, validates all three member IDs/defeat states and a host approach, and stores the first matching pack without applying the Tundra population prefix or actor-budget admission order. `ActivatePopulationKey` rebuilds descriptors and applies `ApplyPopulationBudget` with Freezing Tundra's wildlife multiplier of 0.65. Its actor budget has a three-member floor, but it skips a pack when `SpawnedWildlifeActorCount + MemberCount` exceeds the budget after earlier spawns. Stage one searches for the stored member IDs and reports a missing-member failure after 20 seconds. The bounded selector does not continue to a later viable pack after choosing an omitted/skipped one.
- **Relationship to existing findings:** RF245 addresses the analogous but separate Peat Husk selector; RF248 is limited to WolfPack's three-member selection and actor admission. RF19 already covers the missing standalone `WolfPeer.ps1` documentation path.
- **Proposed repair:** Select only packs that survive the exact descriptor prefix and can fit the actor budget under activation's ordered admission rules. Continue the bounded search after omitted or over-budget candidates, and report no suitable pack before activation if none qualifies.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** The fixture never waits for IDs belonging to a descriptor tail or pack that activation skips. A later activation-admitted pack is selected when an earlier candidate is omitted or over budget; an exhausted search reports no suitable pack at stage zero rather than timing out.
- **Focused validation:** Add narrow Development regressions for a tail Wolf pack before a retained pack, a retained pack skipped after earlier actors consume its three-member budget before a later admissible pack, and no admissible pack. Then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario WolfPack -WorldSeed 445` after an Editor build. No test was run during this review.

## RF249 — Place Wolf peer fixture pawns above sampled terrain (P3; CR03-001b-40)

- **Source:** CR18-002, CR03-001b-40; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:2989-3003; neighboring fixture helpers at KalmalaGameMode.cpp:1492-1501,2104-2115; normal pawn placement at KalmalaGameMode.cpp:11754-11761; Source/KalmalaGameplay/Public/KalmalaCharacter.h:52; Wolf bite validation at Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1674-1696; docs/biome-enemies.md:448-454; docs/07-development-setup.md:2679-2689.
- **Affected path:** The non-Shipping WolfPack peer fixture's listen-host and observer placement before pack activation.
- **Trigger:** The fixture reaches stage zero with a listen host and two observers, finds an undefeated pack, and calls PlaceOnTerrain for each pawn.
- **Impact:** The Development fixture can leave host and observer capsules intersecting the ground, making placement and subsequent Wolf bite/peer acceptance unreliable. No production gameplay defect or live fixture failure was established.
- **Evidence:** AKalmalaCharacter derives from ACharacter, so actor location is the capsule center. PlaceOnTerrain overwrites Z with sampled terrain height and calls SetActorLocation with sweep disabled, then stops and disables movement; it does not add capsule half-height or verify a grounded final location. Neighboring fixture helpers add GetSimpleCollisionHalfHeight() + 1.0f, and normal pawn placement adds capsule height plus clearance. Stage-zero host approach selection measures sampled surface locations and keeps a 10 cm margin below 180 cm; the actual TryApplyWolfBite gate measures final actor-to-actor 3D distance against the full 180 cm boundary. No static proof shows that correcting pawn height will make a selected approach exceed bite reach, but the final range must be rechecked after placement.
- **Relationship to existing findings:** RF248 concerns which Wolf pack is admitted by activation; RF249 concerns the fixture pawn placement after selection. RF19 covers the absent standalone WolfPeer.ps1 path. No issue was found with the observer error text beyond its wording.
- **Proposed repair:** Place pawns at sampled ground height plus capsule half-height and a small clearance, or use a validated ground-placement query. Confirm the final location and recompute the host's actual 3D distance to the selected Wolf after placement.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** Host and observers finish with capsule bottoms above sampled terrain, including on the steepest accepted fixture approach; placement success is confirmed; the host remains within the Wolf's actual 180 cm 3D bite boundary; and both observers remain outside the 3 m acquisition range and in world bounds.
- **Focused validation:** Add a narrow Development regression for flat and steep accepted terrain that checks capsule clearance, final location and post-placement 3D bite reach, then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario WolfPack -WorldSeed 445 after an Editor build. No test was run during this review.

## RF250 — Validate Lynx, Polar Bear, Horned Goat, and Thunder Sentinel fixture capsule placement on accepted Tundra and Mountain slopes (P3; CR03-001b-42/46/50/52/54)

- **Source:** CR18-002, CR03-001b-42/46/50/52/54; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:3198-3212,3248-3257,3553-3578,3791,3843,3862,3929-3954,4079-4082,4128,4174,4190-4193,4284-4325,4476,4524,4553; Source/KalmalaGameplay/Public/KalmalaCharacter.h:52; capsule use at Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:793-795; docs/07-development-setup.md:2711-2780; Scripts/Tests/Combat/EnemyPeers.ps1:1-5,86-129,169-170.
- **Affected path:** The non-Shipping LynxLeap, PolarBearCharge, HornedGoatPush, and ThunderSentinelSlam fixture placement of the listen host and observers.
- **Trigger:** A fixture places a pawn on a dry Tundra or Thunder Mountains sample whose center normal passes the Z >= 0.82 gate while terrain rises across the capsule footprint.
- **Impact:** Any of the four Development fixtures can start with a host or observer capsule intersecting sloped terrain, blocking or occluding the selected attack approach or observer placement and making peer acceptance unreliable. No production gameplay defect or live collision failure was established.
- **Evidence:** All four helpers accept Tundra or Mountain normals down to Z 0.82 and set the capsule center to a center-point terrain sample plus `GetSimpleCollisionHalfHeight() + 1 cm`. They teleport with sweep disabled and do not verify final capsule clearance. The Goat and Sentinel lane helpers sample their centerlines at intervals of at most 80 cm but do not query the capsule's horizontal extent; direct host and observer placement call sites use the same `PlaceOnTerrain` helper. On an uphill slope across a finite-radius capsule, center-point clearance does not prove the full capsule is clear. No specific seed or runtime collision was evaluated.
- **Relationship to existing findings:** RF249 concerns the Wolf fixture helper that omits capsule half-height even on flat terrain. RF250 covers the Lynx, Polar Bear, Horned Goat, and Thunder Sentinel helpers that add center-point clearance but do not validate the full capsule. RF19 covers stale standalone peer-script commands, including HornedGoatPeer.ps1; RF251 concerns existing-key activation for LynxLeap, PolarBearCharge, HornedGoatPush and ThunderSentinelSlam, RF252 concerns selected-attack outcome attribution across all four fixtures, and RF253 concerns Goat observer-distance acceptance.
- **Proposed repair:** Validate the complete capsule against terrain with a collision-aware placement query or sweep and confirm the final location. Skip unsafe host ring/approach candidates before activation where possible, and fail before stage-two arming if final host or observer placement is not collision-clear. Ensure final host placement remains within each selected attack's actual reach.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** None of the four fixtures proceeds to attack/peer acceptance after any pawn placement overlaps terrain; hosts and observers finish collision-clear on flat ground and the steepest accepted Tundra or Mountain slope; unsafe host ring/approach candidates are skipped or rejected before activation, invalid final placements fail before stage-two arming, and the placed host remains within each fixture's documented attack reach.
- **Focused validation:** Add narrow Development regressions for all four fixtures covering flat terrain, a slope near normal-Z 0.82 and obstructed capsule placement. Confirm final placement and actual host-to-target distance, then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario LynxLeap -WorldSeed 445, -Scenario PolarBearCharge -WorldSeed 445, -Scenario HornedGoatPush -WorldSeed 445 and -Scenario ThunderSentinelSlam -WorldSeed 445 after an Editor build. No tests were run during this review.

## RF251 — Handle already-active keys before arming Lynx, Polar Bear, Horned Goat, and Thunder Sentinel peer fixtures (P3; CR03-001b-44/47/48/51/52/54/55/56)

- **Source:** CR18-002, CR03-001b-44/47/48/51/52/54/55/56; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:3428-3459,3580-3609,3628-3777,3791-3831,3986-4116,4129-4159,4356,4369-4464,4477-4509,7880-7923,8308-8313,8366-8380,8618-8620; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:454-465; Scripts/Tests/Combat/EnemyPeers.ps1:86-113,169-170; docs/07-development-setup.md:2764-2770.
- **Affected path:** The non-Shipping LynxLeap, PolarBearCharge, HornedGoatPush, and ThunderSentinelSlam fixtures' stage-zero selected-target activation and stage-one selected-actor wait.
- **Trigger:** A fixture waits for remote observers while normal Tick activates the host's current key, then its bounded selector chooses a descriptor from that already-active key even though no live actor with that persistent ID exists. The Sentinel selector permits active keys and rejects the full key budget only for new keys.
- **Impact:** Any of the four Development peer fixtures can wait 20 seconds and report that its selected wildlife actor did not materialize after direct fixture activation became a no-op. No production wildlife spawning defect was established.
- **Evidence:** Tick calls peer-fixture drivers before periodic player-key activation. Each affected stage zero returns while fewer than two observers are present; the Sentinel wait is at line 4356. During that wait, ordinary activation can mark the host key active with `bAllowLegacyWildlifeForVerification=false`, which skips legacy wildlife materialization. The Sentinel selector at lines 4369-4464 does not skip active keys, and its active-key-capacity guard at 4458-4463 permits an already-active key even at capacity. Its later `ActivatePopulationKey(key, true)` call returns at the `ActivePopulationSpatialKeys.Contains` guard before the verification-only wildlife branch; stage one then waits for the exact persistent ID. The ordinary scheduler's cadence and admission gates do not guarantee that exact descriptor before timeout. The same false-failure path was previously confirmed for LynxLeap, PolarBearCharge and HornedGoatPush. No particular seed or live failure was established.
- **Relationship to existing findings:** RF245 and RF248 cover descriptor-budget/ordered-admission mismatches in other selectors. RF250 concerns fixture capsule placement; RF252 concerns selected-attack outcome attribution across all four peer fixtures; RF253 concerns Goat observer distances. This finding concerns false failure after a fixture activation call becomes a no-op for an already-active key.
- **Proposed repair:** Before staging, accept a matching live selected actor already present; otherwise avoid selecting an already-active key that lacks the selected actor, continue the bounded search to an activatable candidate, or use an explicit fixture-only activation path with a verified result. Do not publish stage one after a no-op activation without a guaranteed target.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** For all four fixtures, pre-activating a candidate key while the selected actor is absent either leads to another activatable candidate or a clear failure before staging; none waits 20 seconds on a target whose key activation was a no-op. A matching live selected actor in an already-active key remains eligible.
- **Focused validation:** Add Development regressions for the pre-activated-key and matching-live-actor cases, then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario LynxLeap -WorldSeed 445, -Scenario PolarBearCharge -WorldSeed 445, -Scenario HornedGoatPush -WorldSeed 445 and -Scenario ThunderSentinelSlam -WorldSeed 445 after an Editor build. No test was run during this review.

## RF252 — Require selected-attack outcome attribution for Lynx, Polar Bear, Horned Goat, and Thunder Sentinel peer acceptance (P3; CR03-001b-45/49/53/57)

- **Source:** CR18-002, CR03-001b-45/49/53/57; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:3412-3551,3779-3927,4118-4275,4582-4628,8308-8621; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:1734-1867,1920-1925,1957-2194,2362-2448,2479-2511,2558-2666,2715-2720; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:320-359; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:61-69; Scripts/Tests/Combat/EnemyPeers.ps1:86-129,169-186; docs/07-development-setup.md:2744-2759,2769-2777.
- **Affected path:** The non-Shipping LynxLeap, PolarBearCharge, HornedGoatPush, and ThunderSentinelSlam fixtures' server-side success predicates.
- **Trigger:** A fixture activates a wildlife key, the selected actor emits enough cues and movement evidence to reach Recovery but its attack misses, and another admitted wildlife actor supplies the host damage or other aggregate outcomes used for acceptance. HornedGoatPush additionally measures host-wide displacement and stagger without associating either with the selected Goat.
- **Impact:** Any of the four Development fixtures can report success for the selected attack when another wildlife actor supplied the aggregate host outcome. No production combat defect or live false pass was established.
- **Evidence:** The Lynx fixture captures its host-health baseline before staging and activation; the Polar Bear fixture sets its baseline to 300 before activation. New-key activation materializes the admitted wildlife descriptors for the whole 6,400 cm key rather than isolating the selected target. Those fixtures latch selected-actor cues but accept aggregate host-health loss; Lynx enters Recovery after both a hit and a miss, and Polar Bear proceeds to Recovery after its charge distance/time expires even if `TryApplyPolarBearCharge` returned false. HornedGoatPush likewise activates the key with `ActivatePopulationKey(..., true)`; stage two latches Telegraph/Recovery from only the selected Goat but samples aggregate host health, total 2D displacement and host-wide stagger. `UpdateHornedGoatAttack` ignores the return from `TryApplyHornedGoatCharge` and publishes Recovery after Telegraph even when the selected hit is rejected. For ThunderSentinelSlam, key activation materializes the selected key's admitted wildlife descriptors; `UpdateThunderSentinelAttack` publishes Lunge and ignores the result of `TryApplyThunderSentinelSlam`. The peer fixture latches that selected Sentinel's cues and step, but measures only total host-health loss and accepts 299–301 damage. The runner pairs the selected SpawnId's cues with aggregate damage without a source-specific receipt. Another actor, including a second Sentinel possible under RF254's cap bypass, can therefore supply the measured host damage while the selected Sentinel misses. No specific seed or runtime false pass was established.
- **Relationship to existing findings:** RF251 is the distinct false-failure case where activation of an already-active key does not materialize the selected target; RF254 concerns a separate area-cap bypass that can allow a second Sentinel. RF250 concerns capsule placement and RF253 observer-distance assertions. RF256 concerns Recovery duration; this finding concerns false success from host-wide outcomes paired with selected-actor cues.
- **Proposed repair:** Gate acceptance on a server-side hit receipt keyed to the selected attacker and Host, and associate expected damage and any required movement/stagger with that hit; alternatively isolate the host from every other wildlife source during the acceptance window. Keep aggregate health, movement, stagger and replicated cue observations as supporting checks.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** In all four fixtures, a selected attack miss plus another actor supplying the measured host outcomes cannot report Passed. A selected Lynx leap, Polar Bear charge, Horned Goat push, or Thunder Sentinel slam with server-confirmed applied damage and its required movement/effects, cue evidence, distance gate and existing peer observations can pass.
- **Focused validation:** Add Development regressions for each fixture where the selected attack misses while another admitted wildlife source supplies host damage (and, for HornedGoatPush, displacement/stagger), and verify each fixture fails. Add corresponding selected-attack hit cases. Then run `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario LynxLeap -WorldSeed 445`, `-Scenario PolarBearCharge -WorldSeed 445`, `-Scenario HornedGoatPush -WorldSeed 445`, and `-Scenario ThunderSentinelSlam -WorldSeed 445` after an Editor build.

## RF253 — Assert Horned Goat and Reef Maw observer placement before peer acceptance (P3; CR03-001b-52/60)

- **Source:** CR18-002, CR03-001b-52/60; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4177-4219,4764-4779,4859-4915; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:5287-5304; Scripts/Tests/Combat/EnemyPeers.ps1:116-129,190-209; docs/07-development-setup.md:2744-2759,2780-2793.
- **Affected path:** HornedGoatPush and ReefMawBite stage-one remote-observer placement and replicated-cue acceptance.
- **Trigger:** One or both observers are not at their intended fixture locations, or a Reef Maw observer is not in the selected deep-Ocean lane, but can still receive the selected actor's replicated cue.
- **Impact:** HornedGoatPush can pass without proving its documented 10 m and 15 m observer spacing, and ReefMawBite can pass without proving both observers occupied the intended deep-Ocean positions when they received the cues. No production replication defect or runtime false pass was established.
- **Evidence:** HornedGoatPush calls PlaceOnTerrain and marks observers placed without reading final locations; the composite runner checks selected-Goat cues but not observer distances. ReefMawBite selects deep-Ocean lane positions for the host and observers, calls PlaceSwimmingCharacter, and later checks only that observers are unattached and have IsSwimmingInGeneratedOcean movement mode. That helper teleports and sets the movement mode, while the predicate reports only the movement enum; stage one does not check final observer coordinates or resample Ocean depth/lake/path. The server log records only Observers=2. On clients, OnRep_ReefMawAttackCue logs the selected SpawnId's replicated cue without checking observer location, and the composite runner checks these logs and host distance but no observer positions or depth. Thus both scenarios can pass without demonstrating the intended observer placement or, for Reef Maw, the selected deep-Ocean lane. No specific seed or runtime false pass was established.
- **Relationship to existing findings:** RF250 concerns capsule clearance at fixture placements; RF251 concerns already-active-key target materialization; RF252 concerns attack outcome attribution; RF254 concerns species-area caps. RF253 concerns peer-observer placement and cue acceptance across both scenarios.
- **Proposed repair:** After placement, record server-authoritative final observer positions. For HornedGoatPush, require the documented 1000 cm and 1500 cm distances within tolerance. For ReefMawBite, verify each observer remains at its selected Ocean lane position and the actual position passes the deep non-lake Ocean predicate. Have the composite runner assert these scenario-specific measurements in addition to matching each selected SpawnId's cues.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** Stage two is not armed and neither scenario can pass unless Goat observers meet the documented 10 m and 15 m bands and Reef Maw observers remain at their assigned deep-Ocean lane positions. Both remote logs in each scenario must still contain the selected actor's cues, and the Goat host remains within the existing 180 cm reach check.
- **Focused validation:** Add Development regressions that move an observer outside its Goat slot or Reef Maw lane while keeping it close enough to receive the cue; each runner must reject the case. Retain intended placements, then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario HornedGoatPush -WorldSeed 445 and -Scenario ReefMawBite -WorldSeed 445 after an Editor build. No test was run during this review.

## RF254 — Respect Reef Maw and Thunder Sentinel area caps in peer-fixture admission (P3; CR03-001b-55/56/59)

- **Source:** CR18-002, CR03-001b-55/56/59; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4392-4419,4458-4509,4711-4810,6867-6890,6971-7001,7182-7184,7534-7539,8308-8313,8366-8458,8618-8620; Source/KalmalaWorld/Public/KalmalaWorldPopulationLayout.h:451-468; Source/KalmalaGameplay/Private/Tests/KalmalaPopulationAreaCapsTest.cpp:52-55; docs/monster-spawning.md:230-244; docs/07-development-setup.md:2764-2768,2780-2790; Scripts/Tests/Combat/EnemyPeers.ps1:169-170,190-192.
- **Affected path:** The non-Shipping ThunderSentinelSlam and ReefMawBite descriptor selectors and their verification-only population-key materialization.
- **Trigger:** A generated Sentinel or Reef Maw candidate otherwise passes its biome/descriptor, start-distance, defeat, per-key actor-budget and fixture-specific terrain/lane checks while another living or reserved same-species actor is already within the documented 96 m species-cap radius.
- **Impact:** ThunderSentinelSlam or ReefMawBite can report peer acceptance with an actor that normal population admission would reject, so the fixtures do not prove the documented one-per-species area cap. No production population-cap defect was established.
- **Evidence:** Both selectors track the candidate key's `WildlifeActorBudget` but do not call `DoesPopulationDescriptorPassCaps` or count nearby live/pending actors of the selected species. The Reef Maw selector uses the full Ocean descriptor list, applies the hard-enemy start exclusion and undefeated filter, then accepts a descriptor with a safe sampled Ocean lane; it does not check the Reef Maw area cap. Normal admission calls the cap gate, whose area-cap value is one for both Thunder Sentinel and Reef Maw and whose count includes live actors and pending reservations within 96 m. For a new key, `ActivatePopulationKey(key, true)` directly materializes verification descriptors using the key-local budget without that area-cap gate. Thus either fixture can stage and accept an additional same-species actor even when ordinary admission rejects it. No specific seed or runtime over-cap case was established.
- **Relationship to existing findings:** RF245 and RF248 concern descriptor-prefix and per-key actor-budget mismatches; RF251 concerns false failure after an already-active key makes direct activation a no-op; RF250 concerns pawn capsule placement. This finding covers the separate live/pending species-area-cap gap for both fixtures.
- **Proposed repair:** Apply the normal species-area admission check to both fixture selectors and ensure verification-only materialization cannot bypass it for the selected actor. If the cap is full, continue the bounded search or fail before staging rather than spawning an over-cap target.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** With a living or reserved same-species actor inside a candidate's 96 m area, neither fixture materializes a second actor or reports Passed using one. Each either selects a candidate whose normal area-cap check passes or fails clearly before stage one. Under-cap cases retain the documented generated peer acceptance.
- **Focused validation:** Add Development regressions with existing live Sentinels and Reef Maws inside 96 m of generated candidates and verify neither fixture materializes or accepts a second same-species actor; also retain under-cap passing cases. Then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario ThunderSentinelSlam -WorldSeed 445 and -Scenario ReefMawBite -WorldSeed 445 after an Editor build. No test was run during this review.

## RF255 — Assert spawned Reef Maw and Thunder Sentinel resistance profiles in peer acceptance (P3; CR03-001b-56/60)

- **Source:** CR18-002, CR03-001b-56/60; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4511-4515,4843-4846; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:161-163,297-300,326-330,689-697; Source/KalmalaGameplay/Public/KalmalaResistanceComponent.h:10-21; Source/KalmalaGameplay/Private/KalmalaResistanceComponent.cpp:9-13; docs/07-development-setup.md:2765-2773,2788-2793; Scripts/Tests/Combat/EnemyPeers.ps1:169-186,190-209.
- **Affected path:** ThunderSentinelSlam's stage-one 75-Armor check and ReefMawBite's stage-one 2-Armor check for their materialized wildlife actors.
- **Trigger:** The archetype Armor mapping remains at the expected 75 or 2 while the selected actor's resistance-component profile is absent or differs because runtime component setup regressed.
- **Impact:** Either Development peer case can pass without proving its generated wildlife actor received the documented runtime Armor profile. No current production defect was established.
- **Evidence:** Both fixtures check the selected actor's archetype and health, but their Armor conditions call static `GetArmorForArchetype(Target->GetArchetype())` instead of reading that actor's resistance component. Each scenario measures damage dealt by the wildlife actor to the host; neither applies a hit to the selected actor, so the outcome does not exercise its own Armor. Spawn initialization separately assigns the mapping into `Defenses->Profile.Armor`, and server damage resolution consumes the component profile. The component is a default subobject and exposes its profile, so the actual runtime value is directly checkable. A regression in component creation or assignment can leave the mapping at the documented value while either fixture's Armor check passes. The current spawn initialization makes the assignment; no live failure or production defect was established.
- **Relationship to existing findings:** RF250 concerns fixture pawn placement; RF251 and RF254 concern key activation and population-cap fidelity; RF253 concerns observer placement. RF255 concerns runtime resistance-profile acceptance.
- **Proposed repair:** Check the materialized target's resistance-component profile and fail before stage two if the component is missing or Armor differs from the expected archetype value: 75 for Thunder Sentinel or 2 for Reef Maw. Where practical, also exercise server-side resolution with a narrow known-damage profile check.
- **Dependencies:** After CR19 review acceptance; no fixture repair during CR03 or CR18.
- **Acceptance:** A generated Sentinel or Reef Maw whose archetype mapping is correct but whose runtime resistance profile is missing or differs from its expected Armor value cannot arm its peer fixture; initialized 75-Armor and 2-Armor targets remain eligible.
- **Focused validation:** Add Development regressions that change or omit runtime component Armor while leaving each archetype mapping correct and verify both fixtures reject the mismatch; retain initialized-profile cases, then run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario ThunderSentinelSlam -WorldSeed 445 and -Scenario ReefMawBite -WorldSeed 445 after an Editor build. No test was run during this review.

## RF256 — Assert the Thunder Sentinel's documented recovery duration in peer acceptance (P3; CR03-001b-57)

- **Source:** CR18-002, CR03-001b-57; Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4592-4612,4617-4624; Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:2633-2666; Source/KalmalaGameplay/Public/KalmalaWildlifeSpawn.h:256; docs/07-development-setup.md:2771-2773; Scripts/Tests/Combat/EnemyPeers.ps1:174-186.
- **Affected path:** ThunderSentinelSlam's server-side Recovery cue acceptance and composite peer predicate.
- **Trigger:** The selected Sentinel exposes Recovery for at least one sampled frame but exits it before the documented 1.2 seconds.
- **Impact:** The generated Sentinel peer case can pass after a Recovery shorter than the documented duration. No current gameplay defect was established.
- **Evidence:** Stage two only latches `bThunderSentinelPeerTestSawRecovery` when the cue equals Recovery; success requires that boolean, and the server log emits `Recovery=1`. The composite runner requires that flag but reads no duration. Runtime behavior currently assigns `ThunderSentinelRecoverySeconds` (1.2 seconds) on entry and clears Recovery when its timer expires, but the fixture never measures entry-to-exit time. A shortened duration lasting at least one sample can therefore satisfy the acceptance check. No current runtime duration defect was established.
- **Relationship to existing findings:** RF252 concerns attribution of the selected Sentinel's damage; RF255 concerns whether the materialized actor has the expected runtime Armor profile. RF256 concerns only the temporal duration of its Recovery cue.
- **Proposed repair:** Measure the server-observed Recovery interval or authoritative timer, include it in the success log and require it to meet the documented 1.2-second duration with an explicit sampling tolerance.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** The fixture cannot report Passed when the selected Sentinel remains in Recovery for less than the documented duration, while the authored 1.2-second Recovery passes within documented sampling tolerance.
- **Focused validation:** Add a Development regression with Recovery shortened below 1.2 seconds and verify the fixture rejects it; retain the authored-duration pass and run Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario ThunderSentinelSlam -WorldSeed 445 after an Editor build. No test was run during this review.

## RF257 — Exercise normal interaction paths in the Discovery, Ocean Discovery, and M9 peer fixtures (P3; CR03-001b-62/64, CR18-002 batch 104)

- **Source:** CR18-002, CR03-001b-62/64 and batch 104; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:4980-5091,5207-5268,10085-10153`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1576-1598,1687-1716,1968-1972`; `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:142-161`; `Source/KalmalaGameplay/Private/KalmalaOceanDiscoveryDisembarkPeerTest.cpp:424,469-472,790-791`; `Scripts/Tests/Progression/DiscoveryPeer.ps1:8-35`; `Scripts/Tests/Progression/ExplorationRewards.ps1:12-42`; `Scripts/Tests/Travel/OceanSkiffIntegratedReconnectJourney.ps1`; `docs/07-development-setup.md:2495-2508,4164-4180,4361-4391`.
- **Affected path:** DiscoveryPeer, Ocean discovery disembark/reconnect, and ExplorationRewards peer-fixture distance rejection, entitled claim, duplicate/replay claim, and peer acceptance.
- **Trigger:** The ordinary owning-pawn `ServerRequestInteract` trace or discovery-actor dispatch stops reaching world, Ocean, or M9 discovery actors while direct server calls still succeed.
- **Evidence:** DiscoveryPeer checks distance with direct `CanInteract_Implementation(Remote)` and `ClaimDiscovery(Remote, Descriptor)` calls, then invokes `Actor->Interact_Implementation(Entitled)` directly for initial and duplicate claims. The M9 fixture combines `!Actor->CanInteract_Implementation(Remote) && !ClaimM9ExplorationReward(Remote, Descriptor)`; when the expected range check rejects the distant player, short-circuiting skips the GameMode claim call. M9 accepted/replay interactions also call `Interact_Implementation` directly. The Ocean fixture and runner call `ClaimOceanDiscovery` directly; neither submits an ordinary pawn interaction request. Production routes normal play through `RequestInteract`, `ServerRequestInteract`, server tracing, and interactable dispatch. No current production interaction failure was established.
- **Impact:** None of the three peer scenarios proves that a player can claim through the ordinary networked interaction route; a routing or server-trace regression could escape while direct handler or actor calls continue to succeed. No production defect was established.
- **Relationship to existing findings:** RF02 separately covers the missing Remote-client observation of the nonlocal Host's private feedback and reward inventory. Current fields remain owner-only; this finding concerns request/target dispatch. RF265 separately covers Ocean actor refresh materialization and cleanup, and RF28 covers the Ocean claim transaction rollback path.
- **Proposed repair:** Have all three fixtures submit ordinary payload-free interaction intents through the owning pawn's `ServerRequestInteract` path and retain server-side trace/target selection. Keep direct descriptor-forgery and duplicate/replay checks as supplemental assertions; separate the M9 helper-distance assertion so short-circuiting cannot skip it. Keep independent privacy assertions.
- **Dependencies:** After CR19 review acceptance; no fixture or runner repair during CR03 or CR18.
- **Acceptance:** DiscoveryPeer, Ocean discovery, and ExplorationRewards cannot pass unless owning players send ordinary interaction requests without target, discovery ID, reward, or identity payloads, the server selects actors through normal trace/dispatch, and accepted, distant-rejected, and duplicate/replay outcomes are observed. Both Ocean owners receive exactly one owner-scoped reward through interaction; the M9 direct-claim distance check is independently exercised, and the privacy check remains independently required.
- **Focused validation:** Add a Development regression that disables or bypasses normal discovery dispatch and verify all three peer scenarios fail; then run `Scripts/Tests/Progression/DiscoveryPeer.ps1`, `Scripts/Tests/Progression/ExplorationRewards.ps1`, and the Ocean discovery/reconnect journey after an Editor build.

## RF258 — Assert the Y tie-break in due population-key ordering (P3; CR03-001b-68)

- **Source:** CR18-002, CR03-001b-68; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5364-5425,5469-5488`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerCadenceTest.cpp:26-29,91-100`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerBudgetTest.cpp:22-60`; `docs/monster-spawning.md:145-165`.
- **Affected path:** S06 lexicographic due-key ordering and S07 selection of at most four keys for each scheduler wake.
- **Trigger:** Multiple simultaneously due active keys share X but have different Y values, and insertion or hash iteration order differs from ascending Y order.
- **Evidence:** `GetDuePopulationSpatialKeys` currently compares X and then Y, so the implementation provides the documented lexicographic order. The cadence fixture adds loaded keys `(1,0)` then `(0,0)`, but `GetPopulationSpatialKeyCandidates` sorts the candidate union before schedule update; its due assertion only distinguishes X values. The scheduler-budget test uses six `(Index,0)` keys and also checks X order. No focused automation supplies same-X keys with different Y values. If the Y tie-break is removed, both tests can continue to pass while equivalent-X keys are left in TSet-derived order. With more due keys than the four-key wake cap, that can change which keys are serviced. No current production defect was established.
- **Impact:** The focused scheduler checks can miss a regression that stops honoring the Y tie-break for keys sharing X, allowing the bounded wake to choose a different set of due keys.
- **Relationship to existing findings:** RF258 concerns scheduler key-order test coverage only; it does not identify a current ordering defect or change to S06/S07 behavior.
- **Proposed repair:** Extend the cadence or scheduler-budget automation with simultaneously due keys sharing X and having distinct Y values, insert them in reverse Y order, and assert ascending-Y output and the stable bounded wake selection.
- **Dependencies:** After CR19 review acceptance; no scheduler test or runtime behavior change during CR03 or CR18.
- **Acceptance:** A focused Development regression fails when the Y tie-break is removed or reversed, passes with same-X keys inserted in an order different from lexicographic Y order, and preserves the existing X-order and wake-budget assertions.
- **Focused validation:** Run `Kalmala.Gameplay.Population.SchedulerCadence` and `Kalmala.Gameplay.Population.SchedulerWakeBudget` after an Editor build. No test was run during this review.

## RF259 — Scheduler wake attempt gates lack transaction integration coverage (P3; CR03-001b-70, CR03-001b-74)

- **Source:** CR18-002, CR03-001b-70, CR03-001b-74; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5510-5575,5793-5818`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSchedulerBudgetTest.cpp:8-69`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRingCandidateSelectorTest.cpp:12-175`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPlayerDistanceGateTest.cpp:7-123`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSpawnGateEvaluatorTest.cpp:7-178`; `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationCreationLifecycleTest.cpp:12-118`; `docs/monster-spawning.md:163-166,270-281`.
- **Affected path:** A due natural-population key through the bounded server wake, descriptor selection, ordered attempt gates and successful server creation transaction.
- **Trigger:** A regression disconnects or misorders an attempt gate, or stops submitting a fully eligible scheduled attempt to the transaction, while scheduler-budget, gate-helper and direct transaction rejection tests continue to pass.
- **Evidence:** The current wake builds descriptors, selects bounded candidates, applies player-distance, habitat, time, weather, progression, cap and physical-placement gates, then submits attempts whose rejection reason remains `None`. The scheduler-budget automation checks work counts, key and candidate limits, ordering and hitch behavior; the ring-selector automation checks candidate/ordinal behavior; the gate-evaluator automation checks helper ordering; and the creation-lifecycle automation calls `TryCreatePopulationSpawnTransaction` only with default failing gates before manually creating its lifecycle fixtures. No focused test drives one fully eligible attempt from the normal scheduler wake through all gates and verifies successful actor creation/registration. The selector fixture also retains an accepted-roll descriptor outside the ring, but no test feeds that attempt through the distance gate and asserts `PlayerDistance`; since the wake dispatches by rejection reason instead of candidate-list membership, a regression that removes the ring conjunct could leave the attempt without a distance rejection and eligible for dispatch if later gates pass. Existing RF12 covers transaction internals and RF13 covers Tick-to-scheduler wiring; neither covers this wake-to-transaction handoff. No production defect was established.
- **Impact:** A wiring regression could leave eligible natural population attempts uncreated while the scheduler, gate and transaction helper checks still pass.
- **Relationship to existing findings:** RF12 covers successful transaction internals and partial Wolf rollback; RF13 covers real GameMode Tick sequence. RF259 covers the distinct bounded-wake attempt-gate-to-transaction integration.
- **Proposed repair:** Add a Development integration fixture that drives a deterministic eligible descriptor through the scheduler wake and all ordered gates, then verifies the created actor and live registration. Include a deterministic accepted-roll, out-of-ring descriptor retained by selection; verify the distance gate records `PlayerDistance` and the wake creates no actor for that attempt. Force one other gate to reject in a companion case and verify no actor is created.
- **Dependencies:** After CR19 review acceptance; no scheduler pipeline or runtime behavior change during CR03 or CR18.
- **Acceptance:** A fully eligible due attempt reaches successful actor creation and registration through `UpdatePopulationSchedulerWake`; the retained accepted-roll, out-of-ring attempt receives `PlayerDistance` and creates no actor; a forced gate rejection creates no actor; existing wake and candidate budgets remain respected.
- **Focused validation:** Run the new scheduler-admission integration regression together with `Kalmala.Gameplay.Population.SchedulerWakeBudget` after an Editor build. No test was run during this review.

## RF260 — Horizontal spawn-ring coverage does not vary elevation (P3)

- **Source review:** CR18-002, CR03-001b-72.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5698-5736`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRingCandidateSelectorTest.cpp:24-67,83-91`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPlayerDistanceGateTest.cpp:44-54,79-116`, and `docs/monster-spawning.md:168-190`.
- **Trigger:** A regression changes `IsDescriptorInSpawnRing` from horizontal X/Y distance to 3D distance while the player and candidate remain at the same Z in existing tests.
- **Evidence:** The production predicate correctly calculates `DeltaX` and `DeltaY` only, as required by S08. The ring-selector test covers the inclusive 32 m and 96 m boundaries and the one-centimetre outside case, but its player and every descriptor use Z=0. The player-distance fixture also places both players and all candidates at Z=0. Neither test would detect a 3D-distance regression. No production defect was found.
- **Impact:** A regression could admit a candidate horizontally within 32 m because vertical separation increases its 3D distance, or reject a candidate horizontally inside the 32–96 m ring because elevation pushes its 3D distance outside the upper bound. The current implementation remains horizontal.
- **Relationship to existing findings:** RF259 concerns the broader scheduler wake-to-transaction integration; RF260 concerns the distinct S08 horizontal geometry invariant.
- **Proposed repair:** Add Development ring-selector cases with same-world players and candidates at different Z values while preserving chosen X/Y distances. Include a horizontal in-ring candidate whose vertical offset moves its 3D distance beyond 96 m and a horizontally too-close candidate whose vertical offset moves its 3D distance above 32 m.
- **Dependency:** Defer test implementation until CR19 review acceptance; do not change ring behavior during CR03 or CR18.
- **Acceptance:** Focused automation fails if Z contributes to ring membership, confirms horizontal-only inclusion and exclusion across the two vertical-offset cases, and retains the existing inclusive 32 m and 96 m checks.
- **Focused validation:** Run `Kalmala.Gameplay.Population.RingCandidateSelector` and `Kalmala.Gameplay.Population.PlayerDistanceGate` after an Editor build. No test was run during this review.

## RF261 — Horizontal player-exclusion coverage does not vary elevation (P3; CR03-001b-75)

- **Source review:** CR18-002, CR03-001b-75.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5820-5854`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationPlayerDistanceGateTest.cpp:44-54,79-123`, and `docs/monster-spawning.md:183-190`.
- **Trigger:** A regression changes `IsDescriptorOutsidePlayerExclusion` from horizontal X/Y distance to 3D distance while all players and candidates remain at the same elevation in the existing fixture.
- **Evidence:** The production predicate correctly checks only X/Y and rejects when any valid current-world player's horizontal distance is strictly less than 36 m. The player-distance fixture places both players and all candidate descriptors at Z=0. Its descriptor at `(6500,0,0)` is 35 m horizontally from the second player at `(10000,0,0)`, yet remains 65 m from the selecting player at `(0,0,0)`, so it is valid under S08's ring; neither existing assertion would detect changing the S09 exclusion to 3D distance. No production defect was established.
- **Impact:** A 3D-distance regression could allow a spawn within 36 m horizontally of a player when vertical separation raises its 3D distance above the threshold. The current predicate remains horizontal.
- **Relationship to existing findings:** RF260 covers the separate S08 horizontal 32–96 m ring predicate. RF261 covers S09's all-player 36 m exclusion.
- **Proposed repair:** Extend `Kalmala.Gameplay.Population.PlayerDistanceGate` with a candidate 35 m horizontally from the second player, a vertical offset sufficient to place its 3D distance above 36 m, and X/Y coordinates still inside the ring around the first player. Assert that the S09 gate records `PlayerDistance` and rejects the candidate.
- **Dependency:** Defer test implementation until CR19 review acceptance; do not change exclusion behavior during CR03 or CR18.
- **Acceptance:** Focused automation fails if Z contributes to S09 exclusion, confirms rejection of the elevated 35 m horizontal case, and retains acceptance at exactly 36 m plus the separate horizontal S08 ring checks.
- **Focused validation:** Run `Kalmala.Gameplay.Population.PlayerDistanceGate` after an Editor build. No test was run during this review.

## RF262 — Population species-habitat rejection lacks GameMode fixture coverage (P3; CR03-001b-76)

- **Source review:** CR18-002, CR03-001b-76.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:5856-5898`, `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:460-467`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationRingCandidateSelectorTest.cpp:132-150`, `Source/KalmalaGameplay/Private/Tests/KalmalaEnvironmentalSpawnerAdmissionTest.cpp:178-191`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationSpawnGateEvaluatorTest.cpp:34-57`, `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeReefMawTest.cpp:97-99`, `Source/KalmalaGameplay/Private/Tests/KalmalaWildlifeThunderSentinelTest.cpp:100-106`, and `docs/monster-spawning.md:282-283`.
- **Trigger:** A regression bypasses or hardcodes the GameMode species-habitat result while the generic gate-order test, direct hard-enemy helper boundary tests, and positive configured-source case continue to pass.
- **Evidence:** `ApplyPopulationSpeciesHabitatGate` correctly evaluates each retained attempt through the shared predicate, recomputes its ordered rejection reason, and filters candidates through the same predicate. The gate-evaluator test supplies synthetic booleans; the configured-source fixture checks a passing habitat result before a later cap rejection; and the ring-selector fixture calls the wrapper after its candidate list is empty while checking only preservation of an earlier seeded-roll failure. Reef Maw and Thunder Sentinel fixtures test the hard-enemy helper directly at 14999 and 15000 cm, but no focused population test supplies a generated start and drives a hard-enemy descriptor inside the 150 m exclusion through the GameMode wrapper. No production defect was established.
- **Impact:** A wiring regression could admit a hard enemy within the documented 150 m generated-start exclusion. The current implementation remains correct.
- **Relationship to existing findings:** RF259 concerns full scheduler wake-to-transaction integration; RF261 concerns the distinct S09 player-distance gate. RF262 covers the focused S15 species-habitat result path.
- **Proposed repair:** Add a Development fixture that supplies a generated start and a deterministic hard-enemy descriptor inside its 150 m exclusion to `ApplyPopulationSpeciesHabitatGate`. Assert that the result is false, the rejection reason is `SpeciesHabitat`, and the descriptor is removed from candidates. Include a passing case and a companion attempt with an earlier rejection to verify ordering remains intact.
- **Dependency:** Defer test implementation until CR19 review acceptance; do not change species-habitat behavior during CR03 or CR18.
- **Acceptance:** Focused automation detects a bypassed or hardcoded species-habitat result, proves near-start hard-enemy rejection through the GameMode wrapper and candidate filtering, and retains the passing, earlier-rejection, and direct 14999/15000 cm helper-boundary checks.
- **Focused validation:** Run the new `Kalmala.Gameplay.Population.SpeciesHabitatGate` automation after an Editor build. No test was run during this review.

## RF263 — Horizontal species-area cap geometry lacks elevated and diagonal coverage (P3; CR03-001b-85)

- **Source review:** CR18-002, CR03-001b-85.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:6929-6941,6947-6965`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationAreaCapsTest.cpp:65-92`, `Source/KalmalaGameplay/Private/Tests/KalmalaPopulationWolfReservationTest.cpp:43-89`, and `docs/monster-spawning.md:230-233`.
- **Trigger:** A regression changes either live-actor or reservation cap distance from horizontal Euclidean X/Y to 3D or axis-aligned distance while current cap fixtures remain at Z=0 and near an axis.
- **Evidence:** Both current count loops correctly compare squared X/Y distance against the inclusive radius. The area-cap fixture checks current-position movement and exact/outside radius along X; other live members have Z=0. The Wolf reservation fixture uses Z=0 and axis-aligned pack members. No cap-count fixture places a same-XY actor/reservation at another elevation or a diagonal outside the circular radius but inside the square bounds. Existing tests therefore do not detect either 3D-distance or square-distance drift. No production defect was found.
- **Impact:** A 3D-distance regression could stop charging an elevated same-species actor or reservation and admit excess wildlife in that horizontal area. An axis-aligned regression could charge an actor outside the documented circle and reject an otherwise admissible spawn.
- **Relationship to existing findings:** RF260/RF261 cover the separate horizontal S08/S09 player-distance gates; RF254 concerns verification fixtures bypassing the species-area gate. RF263 covers S11 cap-count geometry only.
- **Proposed repair:** Extend the area-cap and Wolf-reservation automations with same-XY elevated live/reserved members and diagonal members beyond the circular radius but inside its bounding square. Assert the former remain charged and the latter do not consume nearby species capacity.
- **Dependency:** Defer test implementation until CR19 review acceptance; do not change S11 behavior during CR03 or CR18.
- **Acceptance:** Focused tests fail when Z contributes to cap distance or the circle becomes axis-aligned, prove both live and reserved elevation-independent counts and diagonal exclusion, and retain the existing inclusive-boundary, movement, reservation and budget checks.
- **Focused validation:** Run `Kalmala.Gameplay.Population.AreaCaps` and `Kalmala.Gameplay.Population.WolfPackReservations` after an Editor build. No test was run during this review.

## RF264 — Verify both seat references on the remote skiff replica (P3; CR03-001b-92)

- **Source review:** CR18-002, CR03-001b-92.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:8194-8195,8255-8259`, `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:687-712`, `Source/KalmalaGameplay/Private/KalmalaOceanSkiff.cpp:83-84`, `Source/KalmalaGameplay/Private/Tests/KalmalaOceanSkiffTest.cpp`, `Scripts/Tests/Travel/OceanSkiffJourney.ps1:46-50`, `docs/02-technical-architecture.md:1226`, and `docs/07-development-setup.md:2255-2264`.
- **Trigger:** A regression prevents the remote client from receiving the passenger occupant reference or the listen-host passenger's skiff attachment while server state and the locally controlled helm replica remain correct.
- **Evidence:** Production registers both skiff occupant references for replication. The GameMode fixture checks both seat references and both attachments on the server. The character journey probe runs only for locally controlled characters; the runner checks the listen server's authoritative passenger travel and the remote client's local helm attach/travel/stop. It does not assert that the remote client's skiff replica identifies the host passenger or that the host passenger replica remains attached. The focused `KalmalaOceanSkiffTest` contains no occupant-reference assertions. The M8 technical contract requires both references to replicate to relevant peers. No production defect was established.
- **Impact:** A remote-only regression in passenger occupancy or attachment replication could pass the journey runner while that client's two-seat presentation is incomplete. The current production replication registration is present.
- **Relationship to existing findings:** RF59 concerns public blockage-detail replication and does not cover seat occupancy or passenger attachment. RF264 is a separate remote-replica regression coverage gap.
- **Proposed repair:** Extend the non-shipping client journey probe and runner to assert that the remote client sees the server-selected helm and passenger occupant references and that the host passenger replica remains attached to the same skiff during travel.
- **Dependency:** Defer the test repair until CR19 review acceptance; do not change the journey fixture or replication behavior during CR03 or CR18.
- **Acceptance:** The two-peer journey regression fails if either occupant reference or the host passenger attachment is missing on the remote client, while preserving the server seat/attachment checks and the remote helm attach, underway and moored assertions.
- **Focused validation:** Run `Scripts/Tests/Travel/OceanSkiffJourney.ps1` after an Editor build.

## RF265 — Cover Ocean discovery refresh materialization and cleanup (P3)

- **Source:** CR18-002, CR03-001b-98; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:7910,9314-9391`; `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:95-113,142-160`; `Source/KalmalaGameplay/Private/KalmalaOceanDiscoveryDisembarkPeerTest.cpp:260-275,706-791`; `Source/KalmalaGameplay/Private/Tests/KalmalaOceanDiscoveryClaimTest.cpp:24-80`; and `docs/04-roadmap.md:330`.
- **Affected path:** The authoritative GameMode refresh that materializes canonical Ocean discovery actors for active player neighborhoods and destroys actors after those keys leave the active set.
- **Trigger:** The server refresh stops selecting, spawning, initializing, or removing Ocean discovery actors while catalogue and direct claim tests continue to pass.
- **Impact:** Players may not find a sea discovery marker, or may retain a stale marker, despite the optional discovery being required by M8 acceptance.
- **Evidence:** GameMode Tick calls `RefreshOceanDiscoveries`; the method derives desired keys from player pawn locations, destroys actors for departed keys, and spawns/initializes `AKalmalaDiscoveryActor` for descriptors from `FKalmalaOceanDiscoveryCatalogue`. `KalmalaOceanDiscoveryClaimTest` exercises catalogue descriptors and sparse-claim behavior. `KalmalaOceanDiscoveryDisembarkPeerTest` independently builds a descriptor from the catalogue and calls `GameMode->ClaimOceanDiscovery` directly; it neither observes a materialized Ocean actor nor checks refresh cleanup, so it passes even if this GameMode lifecycle is broken. The current M8 roadmap acceptance requires players to find an optional sea discovery. No production defect was established.
- **Relationship to existing findings:** RF257 covers normal interaction dispatch in the generic DiscoveryPeer and M9 fixtures; it does not cover the Ocean actor refresh lifecycle. RF28 covers claim rollback failure and is also separate.
- **Proposed repair:** Extend the Ocean GameMode peer fixture to observe the production tick-driven refresh, assert canonical marker creation for an active player neighborhood and stale-marker removal after leaving it, while retaining its existing claim and disembark checks.
- **Dependencies:** After CR19 review acceptance; do not change refresh, actor, fixture, or runner behavior during CR03 or CR18.
- **Acceptance:** A two-player seed-418 regression fails when normal GameMode refresh does not materialize the expected canonical discovery marker for an active spatial key or fails to remove stale markers after both players leave that neighborhood. Existing descriptor, owner-claim, and disembark assertions remain.
- **Focused validation:** Run `Kalmala.Gameplay.OceanTravel.DiscoveryCatalogue`, `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract`, and `Scripts/Tests/Travel/OceanSkiffDiscoveryDisembark.ps1` after an Editor build.

## RF266 - Correct weather-transition screenshot cadence in evidence (P3)

- **Source review:** CR18-002, CR03-001b-107.
- **Affected files:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10715-10720,10778-10789`; `Scripts/Tests/World/BiomeBaselineCapture.ps1:11-15,30-43`; `Source/KalmalaUI/Private/KalmalaWeatherActivitySubsystem.cpp:194-202`; `docs/valheim-like-lighting-plan.md:1538`; and the L06e weather record in `PROGRESS.md`.
- **Trigger:** A reader treats the four fixed weather images as samples at 0.5-second intervals.
- **Evidence:** `WeatherTransition` requires `TemporalFrames=1`. The runner's `TemporalIntervalSeconds=0.5` default is not used to generate WeatherTransition images: its frame-expansion loop excludes that condition. GameMode requests screenshots at 5.00, 11.25, 11.75, and 17.75 seconds after fixture start. The precipitation-material response update runs per subsystem tick and logs only when its rounded decile changes; it has no fixed 0.5-second sampling timer. The L06e lighting-plan sentence conflates the four fixed screenshots with a 0.5-second cadence. No production weather defect was established.
- **Impact:** The lighting evidence implies a denser, more regular temporal observation than was produced, which can overstate what the four fixed images demonstrate about transition smoothness.
- **Relationship to existing findings:** RF32 covers incomplete per-site L03 comparison fixtures and does not cover the L06e weather-capture cadence statement.
- **Proposed repair:** Correct the lighting-plan evidence to give the four scheduled screenshot times relative to fixture start and describe response-change logs separately. Limit the no-pop conclusion to the selected fixed views.
- **Dependency:** Defer evidence correction until CR19 review acceptance; do not edit the game documentation during CR03 or CR18.
- **Acceptance:** The lighting plan identifies all four screenshot times, says the default temporal interval does not govern WeatherTransition, and makes no fixed-cadence claim for response telemetry.
- **Focused validation:** Check the revised text against the GameMode timers, runner constraints, and available L06e records. Retain any quantitative cadence statement only if supported by timestamped samples collected at that cadence.

## RF267 — Exclude traversal, reconnect, and ocean-travel test hooks from Shipping builds (P2; CR03-001b-111/001c; CR18-002 batches 112/115)

- **Source:** `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:631,636-640,669-696,11374-11647`; `Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:450-482,759-887,889-950`; `Source/KalmalaGameplay/Private/KalmalaCharacterMovementComponent.cpp:118-126`; `docs/07-development-setup.md:2181-2183,2347-2353`.
- **Affected path:** Shipping listen/dedicated servers and player pawns launched with developer-only traversal, reconnect, or ocean-travel verification switches.
- **Trigger:** A Shipping process receives `-KalmalaTraversalTest`, any nonempty `-KalmalaReconnectVerification=...` value, or `-KalmalaOceanTravelTest`.
- **Evidence:** Traversal/reconnect switch parsing and activation lack Shipping compile guards; BeginPlay can configure traversal and spawn a verification pawn outside guarded regions. Character BeginPlay/Tick derives and applies traversal movement without a Shipping guard. Reconnect verification can persist a generated harvest and call `RequestExit`. Separately, Character BeginPlay parses `-KalmalaOceanTravelTest` and unconditionally changes spawned-character `ECC_Pawn` and `ECC_WorldStatic` responses to Ignore. The GameMode fixture spawner is compiled out in Shipping, but the collision change remains active when the flag is present. The setup guide describes these switches as development verification.
- **Impact:** A stale launch option can steer a listen-host character, spawn a verification pawn, mutate a persistent population record, and terminate a Shipping server. The ocean-travel flag can also disable pawn and generated-terrain collision.
- **Proposed repair:** Compile out or otherwise disable traversal, reconnect, and ocean-travel flag parsing, activation, and their movement, collision, save, and exit side effects in Shipping builds; retain the documented Development behavior.
- **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18.
- **Acceptance:** A Shipping launch with any of these flags follows normal gameplay, does not steer a pawn, change pawn/terrain collision, spawn a verification pawn, mutate a population save, or exit; documented Development checks remain functional.
- **Focused validation:** Build Development and Shipping targets. Run the documented traversal check, `Scripts/Tests/Survival/PersistedCampRestart.ps1`, and `Scripts/Tests/Travel/OceanTravel.ps1` in Development; launch Shipping with each flag using isolated user data and verify normal movement/collision, no test markers, no save mutation, and no forced exit.

## RF268 — Retry treasure preparation after an initial save failure (P3; CR03-001b-113)

- **Source:** CR03-001b-113; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:11972-11994`; `Source/KalmalaGameplay/Private/KalmalaDiscoveryActor.cpp:79-93,156-163`; `docs/02-technical-architecture.md:3498`.
- **Affected path:** A9 treasure attached to a canonical point-of-interest actor when its first population-save write fails.
- **Trigger:** `PrepareWorldTreasure` returns null if its candidate state cannot be durably saved. `InitializeServer` sets `bWorldTreasure` only after a non-null result, then completes ordinary discovery setup. Later interaction calls `ClaimWorldTreasure` only when `bWorldTreasure` is true; no other path retries preparation for that initialized actor.
- **Impact:** A valid point of interest remains without its shared treasure while this actor remains initialized, even if storage recovers. This occurs before the claim-time feedback and rollback cases in RF36.
- **Evidence:** The only `PrepareWorldTreasure` call sites are discovery initialization and `ClaimWorldTreasure`; the latter is reached from the actor only behind the `bWorldTreasure` gate. The initialization write-failure branch returns null without setting that flag or scheduling a retry.
- **Proposed repair:** Keep initial preparation retryable after a failed population-save write, while exposing the chest and allowing claims only after the deterministic treasure record is durable.
- **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18. RF36 remains the separate claim-time capacity and inventory-receipt handoff.
- **Acceptance:** Inject a failed first preparation write for an eligible canonical point of interest, then restore storage and retry preparation without restarting the server. The same deterministic treasure record is saved once, the chest becomes claimable only after that write, and no reroll or duplicate record occurs.
- **Focused validation:** Add a narrow discovery/GameMode write-failure retry test and run the A9 treasure peer scenario after an Editor build.

## RF269 — Avoid weak object pointers as GameMode set elements (P3)

- **Source:** CR03-002; `Source/KalmalaGameplay/Public/KalmalaGameMode.h:573-574,590,627-628`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:10821-10830,11665-11715`; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:199-211`; [Epic UE 5.8 Object Pointers](https://dev.epicgames.com/documentation/en-us/unreal-engine/object-pointers-in-unreal-engine).
- **Affected path:** Development traversal tracking and the GameMode’s pending-return and live-creature registries.
- **Trigger:** A UObject becomes invalid while its `TWeakObjectPtr` remains a `TSet` element. `DriveTraversalTest` adds pawns to its started/completed sets; unlike pending-return sets, those traversal entries are not removed in `Logout`.
- **Evidence:** Epic’s UE 5.8 documentation states that `TWeakObjectPtr` is not supported as an element in a `TSet`. The installed UE 5.8 `FWeakObjectPtr` equality treats two pointers whose `Get()` returns null as equal regardless of the original objects, while its hash is derived from the object index and serial number. Thus invalidation changes equality for keys held by the set and can violate the hash/equality contract. The live-creature set removes actors from `EndPlay`, and pending-return sets remove controllers during `Logout`; no failure in those paths was reproduced.
- **Impact:** Repeated pawn destruction during a Development traversal run can retain stale set entries and make membership behavior for invalid weak keys unsupported, reducing confidence in traversal-test tracking. No production gameplay failure was demonstrated.
- **Proposed repair:** Use supported stable object-key semantics for set membership and remove traversal entries at pawn logout; keep weak pointers only as validated values or transient references.
- **Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18.
- **Acceptance:** A pawn destroyed during traversal leaves no stale tracking entry, and a rejoining pawn is tracked independently. All GameMode weak-pointer registries use supported key semantics while live-creature counts and listen-host return behavior remain correct.
- **Focused validation:** Build Development and add a focused GameMode automation that destroys/logs out a tracked pawn, exercises cleanup and rejoin, and verifies independent membership and live-creature counts. Inspect each remaining container against the UE 5.8 object-pointer contract.

## RF270 — Bind generated harvest tool requests to the active Main Hand item (P3)

- **Source:** CR04-010; Source/KalmalaGameplay/Public/KalmalaCharacter.h:296-297; Source/KalmalaGameplay/Private/KalmalaCharacter.cpp:1485-1552,1687-1716,2060-2068,2149-2219; Source/KalmalaGameplay/Private/KalmalaHarvestNode.cpp:335-365; Source/KalmalaGameplay/Private/KalmalaToolLifecycleContract.cpp:492-538; docs/02-technical-architecture.md:1464-1465.
- **Affected path:** Reliable ServerRequestInteract requests for generated non-tree, non-gemstone, non-hand-gatherable tool harvest sources.
- **Trigger:** A modified owning client names a suitable carried tool that is not the server-selected Main Hand item.
- **Evidence:** Normal local interaction sends Main Hand selection as harvest intent. The RPC receives the client item ID/action, then server-traces and chooses the current target. Tool-gated harvest sources pass those client fields to CommitToolHarvestFromServer. FindToolDurabilityFromServer uses the active instance only when the requested ID matches; otherwise it searches carried tools. The tool lifecycle validates the node-derived source and reward, action, tool suitability, skill, durability, inventory capacity and server persistence, but not whether the submitted item or instance GUID is currently active. The server therefore accepts a stowed suitable tool. Trees, gemstones and hand-gatherable patches use separate paths and are outside this finding.
- **Impact:** A player can harvest with a suitable stowed tool and spend its durability without selecting it in Main Hand. Server target, reward, range, skill, inventory and persistence checks remain in place; no arbitrary reward or target input was found.
- **Proposed repair:** Bind the tool-harvest commit to the server-resolved active item and instance GUID and reject mismatches before any reward, durability, depletion or persistence change.
- **Dependencies:** After CR19 review acceptance; handoff only during CR03/CR18. RF42 is the separate repeated-interaction cadence gap.
- **Acceptance:** A modified request naming a qualifying stowed tool is rejected while another item is active in Main Hand, with inventory, durability, harvest state and saved state unchanged. A valid active-tool request still succeeds through the existing server transaction.
- **Focused validation:** Add an owning-client/listen-server scenario that submits the forged stowed-tool ID, checks for no reward/durability/depletion/save changes, then confirms a valid active-tool harvest and the existing persistence rollback behavior.

## RF271 — Keep world-map verification hooks out of Shipping (P3)

**Source:** `Source/KalmalaUI/Private/KalmalaWorldMapSubsystem.cpp:27-33`; `Source/KalmalaUI/Private/KalmalaWorldMapWidget.cpp:213-263,876-880,900-905,1340-1362`; `Source/KalmalaGameplay/Private/KalmalaMapAwarenessComponent.cpp:77-118`; `Scripts/Tests/World/WorldMap.ps1:42-46`; `docs/07-development-setup.md:3450-3455`.

**Affected path:** A Shipping game process with a local player and a possessed pawn, launched with the world-map verification flags.

**Trigger:** The process receives `-KalmalaWorldMapVerification`; with `-KalmalaWorldMapAcceptanceEvidence`, the verification routine also runs the acceptance side effects. The documented Development runner supplies these flags for map evidence.

**Evidence:** The map subsystem parses the verification flag during ordinary local-player ticking and opens the map after closed-map exploration is recorded. Neither the subsystem nor widget source has a `UE_BUILD_SHIPPING` guard around these hooks. The acceptance-evidence branch creates two synthetic pins via `CommitPinPlacement`, which writes the owner-local pin save, then calls `SendMapPing`. The server still checks authority, requester eligibility, finite and in-world coordinates, and the ping cooldown before relaying an accepted ping; this is not an authority bypass.

**Impact:** With explicit launch flags, a Shipping client can automatically open the test map, persist synthetic personal pins, and submit a test ping that may be accepted and shown to connected peers for the normal temporary lifetime.

**Proposed repair:** Compile out or disable developer verification flag parsing, automatic test activation, and synthetic pin/ping injection in Shipping while retaining ordinary manual map, personal-pin, and ping behavior.

**Dependencies:** After CR19 review acceptance; this finding is a handoff only and does not authorize implementation during CR03 or CR18.

**Acceptance:** A Shipping launch with the map verification and evidence flags does not open test UI, write synthetic pins, or send a test ping. Normal manual map, pin, and ping interactions continue to work, and the documented Development acceptance runner still produces its evidence.

**Focused validation:** Build Development and Shipping targets. Run `Scripts/Tests/World/WorldMap.ps1 -AcceptanceEvidence` in Development; launch Shipping with each verification flag using isolated user data and confirm no test UI activation, pin-save change, or peer ping. Smoke-test normal manual map and ping behavior.

## RF288 — Restore placed Campfires after world reload (P2)

- **Source:** CR17-001; `Source/KalmalaGameplay/Private/KalmalaCraftingComponent.cpp:457-501,504-528`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-956,9588-9614`; `Source/KalmalaGameplay/Public/KalmalaCampfire.h:26,46-55,76-88`; `Scripts/Tests/Survival/PersistedCampRestart.ps1:83-87`; and `docs/04-roadmap.md:648-661`.
- **Affected path:** A host or solo player places a Campfire from Build, saves or returns Home, then reopens the same world.
- **Trigger:** `PlaceConstructionFromServer` routes `CampfireKit` through `PlaceFromServer`; the paid Campfire actor is accepted, then the world is saved and reloaded.
- **Impact:** The paid hearth is absent after reload. If a saved `CampfireKit` construction row exists, the current restore loop creates a generic `AKalmalaConstructionActor`, not the functional Campfire actor with heat and fuel behavior.
- **Evidence:** `PlaceFromServer` validates direct material and ignition-fuel cost, spawns an `AKalmalaCampfire`, commits payment, initializes it and reports success without adding a world save record. `TrySaveWorldFromLocalHost` saves only the population, construction, storage and per-player save owners. `RestorePersistedConstruction` restores only saved construction rows as `AKalmalaConstructionActor`; it never scans or recreates Campfire actors. The Campfire stores fuel/hearth state as actor fields, but no current save owner includes them. The roadmap explicitly requires existing saved Campfires to restore correctly. `PersistedCampRestart.ps1` verifies construction IDs and storage after restart but omits Campfire presence/function. The existing CampfireKit record-model round trip does not exercise runtime placement or restore.
- **Proposed repair:** Add a validated world-owned save/restore record for accepted Campfire placements and restore them as functional server-owned `AKalmalaCampfire` actors. Keep the direct-material Build contract and create no portable Campfire item. Define any required fuel and hearth-state persistence from the approved contract. A failed record write must reject the placement and return its materials.
- **Dependencies:** After CR19 review acceptance; this review finding does not authorize implementation during CR17.
- **Acceptance:** A successfully paid Campfire is restored once at its saved transform as a functional Campfire after same-world reload; another world identity does not load it. A failed write leaves no acknowledged placed hearth and does not consume its materials. Inventory contains no Campfire item.
- **Focused validation:** Extend the GameMode Campfire save/restore scenario with same-world restart, exact actor count/transform, cross-world rejection and an injected save-write failure; run the construction save, Crafting and `Scripts/Tests/Survival/PersistedCampRestart.ps1` regressions after an Editor build. No test was run during this review.

## RF289 — Preserve Boar and Deer drops when the pack is full (P2)

- **Source:** CR09-001; `Source/KalmalaGameplay/Private/KalmalaWildlifeSpawn.cpp:5398-5434`; `Source/KalmalaGameplay/Private/KalmalaInventoryComponent.cpp:426-433`; `Source/KalmalaGameplay/Private/KalmalaInventoryTransactions.cpp:278-294`; and `docs/biome-enemies.md:396-399`.
- **Affected path:** Guaranteed Boar Meat/Hide and Deer Meat/Hide rewards, plus Deer Sinew when its 50% roll succeeds.
- **Trigger:** A credited Boar or Deer is defeated while the owner's pack is full or has room for fewer distinct drops than the defeat produces.
- **Impact:** The actor becomes permanently defeated, but each reward is granted by a separate single-item inventory transaction whose result is ignored. A full pack loses the whole bundle; a pack with one free cell may receive only the first item. No pending claim remains for delivery when capacity returns, and a failed Sinew grant cannot reuse its original roll.
- **Evidence:** `GrantDefeatReward` sets `bDefeatRewardProcessed` before checking capacity, then issues separate `TryGrantFromServer` calls for each Boar/Deer drop without checking their Boolean results. `TryGrantFromServer` uses `BuildGrant` for one item and commits only if that item fits, so multiple calls are not an all-or-nothing bundle. Defeat state is then applied and the actor is hidden; no retry path exists. The current wildlife contract requires stable credited-death/loot claims and says to preserve unclaimed rewards on full packs. RF16 concerns failed population-slot writes after depletion and does not cover inventory-capacity failure.
- **Proposed repair:** Record a stable owner-bound Boar/Deer reward claim before accepting defeat, persist Deer Sinew's one-time server roll with it, and deliver the complete awarded bundle atomically when it fits. Keep the claim pending through a full pack and restart; mark it consumed only after the inventory commit succeeds.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR09.
- **Acceptance:** A full-pack Boar/Deer defeat preserves the exact owner-bound claim without changing inventory. With room for fewer items than the guaranteed bundle, delivery remains all-or-nothing. After space becomes available and after a same-world restart, retry delivers each guaranteed item and the stored optional Deer Sinew outcome exactly once to the credited owner; another owner cannot claim it.
- **Focused validation:** Add a GameMode/wildlife defeat regression for full-pack claim creation, partial-capacity rejection, exact retry, owner binding and reload persistence; run it with the Boar and Deer peer scenarios and the relevant wildlife claim automation after an Editor build. No automation was run during this review.

## RF290 — Abort gameplay after rejecting menu launch identity (P2)

- **Source:** CR10-005; `Source/KalmalaWorld/Private/KalmalaWorldGenerationGameState.cpp:30-32,44-74`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:520-621,647-667`; `docs/startmenu.md:308-320`.
- **Affected path:** Solo or listen-host travel from the menu to gameplay with a malformed, partial, stale-revision, or network-mode-mismatched `KalmalaMenuLaunch`, `KalmalaMenuSeed`, or `KalmalaMenuRevision` option set.
- **Trigger:** The authoritative GameState detects menu identity options but fails validation. It logs that the options were rejected and returns without recording rejection or replacing the constructor's default seed (`10323456789`). GameMode then copies that default identity, loads world state for it, creates a population save if one is absent, and proceeds to create a player start and activate terrain.
- **Impact:** A failed menu launch can silently enter the default-seed world instead of rejecting travel. If that seed already has saves, the session can load and later modify the wrong world; otherwise it can create a misleading default-world population save before gameplay begins.
- **Evidence:** `PostInitializeComponents` validates launch kind against standalone/listen-server mode, parses complete decimal seed/revision values, and matches the current generator revision. Its invalid branch only logs before returning. The GameState constructor initializes the default seed. `AKalmalaGameMode::BeginPlay` checks only that the GameState exists, copies its configuration, and continues into `LoadOceanTravelWorldSave`, `LoadPopulationSave`, seed-derived save-slot lookup, player-start creation and terrain activation. `LoadPopulationSave` initializes and writes an empty population save when the selected slot does not exist. The start-menu contract describes the complete options and accepted seed as applying before gameplay begins; it also preserves the command-line path when no menu marker exists. RF52 covers remote ownership/privacy regression coverage in the menu selection RPC and does not cover this gameplay-side rejection path.
- **Proposed repair:** Expose an explicit rejected-menu-launch state from the authoritative GameState and make GameMode stop before any world save load/create, player-start creation, or terrain activation when a menu marker was present but rejected. Provide a clear travel failure or return-to-menu path. Preserve valid Solo/HostCoop options and the existing command-line behavior when no menu marker exists.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR10.
- **Acceptance:** Any malformed, incomplete, stale-revision, or network-mode-mismatched menu identity fails before world saves are loaded or created and before a pawn start or terrain is activated. Valid complete Solo and HostCoop options use exactly the selected seed. With no menu marker, the command-line `WorldSeed=` behavior remains unchanged.
- **Focused validation:** Add focused GameState parsing/acceptance coverage and a GameMode startup regression proving invalid menu markers cause no save-slot load/create, generated start, or terrain activation. Cover valid Solo, valid HostCoop, and marker-free command-line seed startup. Run the focused automation and menu travel acceptance after an Editor build; no test or runtime scenario was run during this review.

## RF291 — Pin rejection of unknown M7 sparse-delta kinds (P3)

- **Source:** CR10-006; `Source/KalmalaWorld/Private/Tests/KalmalaM7PersistenceContractTest.cpp:33-44`; `Source/KalmalaWorld/Public/KalmalaM7PersistenceContract.h:15-22`; and `Source/KalmalaWorld/Private/KalmalaM7PersistenceContract.cpp:93-99,143-159`.
- **Affected path:** Adding generated resource, creature, or discovery facts to a world-scoped M7 sparse save.
- **Trigger:** A future or malformed sparse-delta kind value outside the three declared enum values is passed to `AddSparseDelta`, or validation changes so such a value is admitted.
- **Impact:** The implementation currently rejects unknown kinds, but the focused contract test only exercises the three supported values. A regression could let a corrupt or unrecognized row enter sparse state without the focused test detecting it.
- **Evidence:** The test adds one `ResourceDepleted`, `CreatureDefeated`, and `DiscoveryClaimed` value and checks duplicate/ID rejection. `FKalmalaM7SparseDelta::IsValid` explicitly allowlists those three kinds, and `AddSparseDelta` calls it before insertion; no selected test passes a casted unknown kind.
- **Proposed repair:** Add a focused assertion that a casted out-of-range `EKalmalaM7SparseDeltaKind` is rejected and leaves the accepted delta count unchanged.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize a test change during CR10.
- **Acceptance:** The M7 persistence contract test proves an unknown kind cannot be inserted and cannot mutate the sparse-delta count.
- **Focused validation:** Run `Kalmala.World.M7.PersistenceContract` after an Editor build; no automation or build was run during this review.

## RF292 — Cover every approved gemstone replacement rate (P3)

- **Source:** CR10-006; `Source/KalmalaWorld/Private/Tests/KalmalaGemstoneTest.cpp:30-41`; `Source/KalmalaWorld/Private/Tests/KalmalaBiomeContentContractTest.cpp:230`; `Source/KalmalaWorld/Public/KalmalaGemstoneContract.h:14-27`; and `docs/a9-acceptance.md:99-105`.
- **Affected path:** Seeded resource generation choosing whether an eligible mineral slot becomes a dedicated gemstone rock.
- **Trigger:** A replacement rate or source/biome gate for Copper, Tin, Iron, slate, or Frost Salt changes while the current focused assertions remain green.
- **Impact:** An approved rarity or eligibility rule could drift unnoticed, producing too many/few gemstone nodes or placing one in an unsupported biome. No current runtime rate defect was confirmed.
- **Evidence:** `KalmalaGemstoneTest` samples 10,000 seeds and checks only Tin's 5% Quartz and 2% Jade results, plus rejection of that Tin source in Meadows. The content contract test checks generated replacement identity against the selector but not source rates. The approved contract sets Copper Quartz 5%, Tin Quartz/Jade 5%/2%, Iron Amethyst 3%, slate Tourmaline 2%, and Frost Salt Star Opal 1%, with exclusive source/biome mappings.
- **Proposed repair:** Extend deterministic contract coverage across each approved source and its eligible biome, pinning each replacement percentage, mutually exclusive outcomes, and rejection in wrong biomes.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize a test change during CR10.
- **Acceptance:** Focused assertions detect changes to all five approved replacement rates, Tin's mutually exclusive Quartz/Jade outcomes, and source/biome eligibility.
- **Focused validation:** Run the gemstone contract automation after an Editor build; no automation or build was run during this review.

## RF293 — Assert the island locator's surrounding sea contract (P3)

- **Source:** CR10-006; `Source/KalmalaWorld/Private/Tests/KalmalaIslandLocatorTest.cpp:14-20`; `Source/KalmalaWorld/Public/KalmalaIslandLocator.h:12-34`; and `Scripts/Tests/Travel/OceanTravel.ps1:33-34`.
- **Affected path:** Resolving the seeded emergent-island endpoint used by long-distance ocean travel.
- **Trigger:** `FindNearest` regresses to returning any valid dry terrain point without enforcing its surrounding-water condition.
- **Impact:** The focused locator test could still pass for a mainland/coastal point because it checks repeatability and center elevation only. OceanTravel's peer gate checks arrival and a terrain audit but does not independently assert the endpoint's water ring, so the test could label a non-island endpoint as an island. No current locator defect was found.
- **Evidence:** The production locator requires at least 9 water samples among 16 directions at a 125,000 cm radius. The test checks the returned center is valid and at least 25 cm above sea level but does not sample that ring. The peer runner waits for island-arrival and terrain-audit markers without asserting the shoreline ratio.
- **Proposed repair:** Independently sample the returned center and the same 16-point, 125,000 cm shoreline ring in the locator automation; assert valid dry center terrain and at least 9 water samples.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize a test change during CR10.
- **Acceptance:** The focused test fails if the result is a dry center without at least 9 of 16 surrounding water samples, while preserving same-seed repeatability.
- **Focused validation:** Run `Kalmala.World.Ocean.IslandLocator` and `Scripts/Tests/Travel/OceanTravel.ps1` after an Editor build; no automation, build, or travel scenario was run during this review.

## RF294 — Pin terrain snow-mask alpha coverage (P3)

- **Source:** CR10-007; `Source/KalmalaWorld/Private/Tests/KalmalaTerrainGroundBlendTest.cpp:8-37`; `Source/KalmalaWorld/Public/KalmalaTerrainGroundBlend.h:14-42`; `Source/KalmalaWorld/Private/KalmalaGeneratedTerrainPatch.cpp:404-411`; and `docs/02-technical-architecture.md:2780-2788`.
- **Affected path:** Vertex-color alpha controlling generated-terrain snow coverage on Tundra and high Thunder Mountains surfaces.
- **Trigger:** The snow-mask formula or terrain vertex-color wiring regresses while the existing test continues to pass because it checks only RGB ground blend channels.
- **Impact:** Snow can disappear from cold biomes or cover the wrong heights/slopes without the focused ground-blend test detecting the visual contract regression. No current production defect was found.
- **Evidence:** The header documents alpha as the Tundra/high-Mountain snow mask and computes it from Tundra weight, highland-gated Mountain weight, and final normal Z. Generated terrain passes the final biome weights, surface normal, and height. The architecture describes the same continuous mask and its material use. The test checks RGB weights and determinism only; its calls use default zero biome weights and it never reads alpha.
- **Proposed repair:** Add independent alpha assertions for full Tundra coverage, Mountain coverage below and across the documented height transition, and normal-Z attenuation; keep the existing RGB assertions.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize a test change during CR10.
- **Acceptance:** `Kalmala.World.TerrainGroundBlend.SlopeMoistureWeights` fails if alpha is dropped, applied below the Mountain threshold, loses its smooth height transition, or ignores Tundra weight and surface-normal attenuation.
- **Focused validation:** Run the terrain ground-blend automation after an Editor build. No automation or build was run during this review.

## RF295 — Pin M8 vessel-save bounds (P3)

- **Source:** CR10-007; `Source/KalmalaWorld/Private/Tests/KalmalaOceanTravelPersistenceContractTest.cpp:121-161`; `Source/KalmalaWorld/Public/KalmalaOceanTravelPersistenceContract.h:20-36`; and `Source/KalmalaWorld/Private/KalmalaOceanTravelPersistenceContract.cpp:6-25`.
- **Affected path:** Validation of the schema-1 world-scoped saved skiff identity and safe transform.
- **Trigger:** A finite vessel Z beyond the documented ±100,000 cm limit, a finite heading beyond ±180 degrees, or a vessel ID one character beyond its 128-character limit is admitted after a validation regression.
- **Impact:** Invalid or oversized vessel state can enter a persistent travel record despite the documented bounds. No current production defect was found.
- **Evidence:** Production validation currently rejects all three cases, and the focused test covers XY radius overflow, non-finite position, non-finite heading, and an accepted maximum-length identity. It does not check finite vertical overflow, finite heading overflow, or maximum-length-plus-one identity rejection. The existing implementation therefore matches the contract, but those boundary regressions are not pinned.
- **Proposed repair:** Add focused rejection assertions for safe-location Z just beyond both limits, heading just beyond both limits, and a correctly prefixed vessel ID of length 129; retain the current accepted-boundary and non-finite cases.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize a test change during CR10.
- **Acceptance:** The persistence contract automation proves accepted boundary values remain valid and rejects values just outside each documented vertical, heading, and vessel-ID bound.
- **Focused validation:** Run the Ocean travel persistence contract automation after an Editor build. No automation or build was run during this review.

## RF296 — Reject out-of-world construction save transforms (P3)

- **Source:** CR11-001; `Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:13-19,65-75,160-172`; `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-951`; `Source/KalmalaWorld/Public/KalmalaWorldBounds.h:8-12`; and `docs/02-technical-architecture.md:845-849`.
- **Affected path:** Loading and restoring schema-7 world construction records.
- **Trigger:** A matching-world schema-7 slot contains an otherwise valid supported construction record with a finite location outside the 16 km world, such as `(1600100, 0, 0)` cm.
- **Impact:** `MatchesWorld` accepts the record and server startup restores the construction beyond the playable world boundary. The actor is unreachable through normal movement and violates the finite-world construction contract.
- **Evidence:** `IsValidConstructionTransform` checks finite location, scale, and normalized rotation but no world bound. `UKalmalaConstructionSaveGameV2::IsValidRecord` and `MatchesWorld` therefore admit finite out-of-world locations. `RestorePersistedConstruction` repeats only `IsValidRecord` before directly spawning each accepted record. Normal placement separately checks the 300 cm radial margin through `FKalmalaPlacementPreview::Evaluate`; the save-load path does not. No existing finding covers this save-boundary defect.
- **Proposed repair:** Reject the complete current-schema construction save when any record's XY location falls outside the same accepted placement margin. Keep incompatible save bytes untouched under the unreleased no-migration policy; do not partially restore the file.
- **Dependencies:** After CR19 review acceptance; this finding does not authorize implementation during CR11.
- **Acceptance:** A same-world schema-7 save with a valid in-bounds record is accepted, while one with a finite out-of-world transform is rejected before restoration and remains unchanged on disk. Boundary cases pin the accepted 300 cm edge margin.
- **Focused validation:** Extend the construction save and restore automation with inside-margin and finite outside-world records, including a slot-byte preservation assertion; run it after an Editor build. No test or build was run during this review.

## RF297 — Bound Smelter saved counters before arithmetic (P3)

- **Source:** CR11-003; `Source/KalmalaGameplay/Public/KalmalaConstructionSaveGame.h:108-130`; `Source/KalmalaGameplay/Private/KalmalaConstructionSaveGame.cpp:65-71,160-170`; and `Source/KalmalaGameplay/Private/KalmalaGameMode.cpp:935-954`.
- **Affected path:** Schema-7 Smelter save validation and server restoration.
- **Trigger:** A matching-world save contains an active job with `InputCoal == MAX_int32`, or nonnegative Copper/Tin/Iron output counters whose `int32` sum overflows (for example, Copper and Tin both `MAX_int32`).
- **Impact:** `GetInputCoalCount` adds the active-job reservation to `InputCoal`, and `GetOutputCount` sums three output counters before capacity validation. Signed overflow makes validation undefined and can wrap an oversized value below the capacity checks. If accepted, the server restores the invalid counters into the placed Smelter, breaking its documented input/output bounds and later capacity decisions.
- **Evidence:** `FKalmalaSmelterState::IsValid` rejects negative counters and compares `GetInputCoalCount()`/`GetOutputCount()` with their caps, but it does not bound `InputCoal` before adding the active reservation or widen the output sum. `UKalmalaConstructionSaveGameV2::IsValidRecord` relies on this method; `MatchesWorld` invokes `IsValidRecord`, and `RestorePersistedConstruction` checks `MatchesWorld` before calling `InitializeFromServer` with the saved state. Existing Smelter tests cover valid capacity edges and ordinary over-cap insertion, not integer-limit serialized counters.
- **Proposed repair:** Bound each saved counter before arithmetic and use a widened or otherwise overflow-safe total for reserved coal and combined outputs. Keep the current schema and reject the complete malformed slot without rewriting it; do not add a migration.
- **Dependencies:** After CR19 review acceptance; CR11 review does not authorize implementation.
- **Acceptance:** Valid input/output boundary states remain accepted. `MAX_int32` reserved-coal and overflowing combined-output records fail current save validation before restore, and their original slot bytes remain unchanged.
- **Focused validation:** Extend the construction save contract with both integer-limit records and a valid-boundary control, then run it after an Editor build. No test or build was run during this review.

## RF301 — Reapply accessibility styling to editable address and seed inputs (P2)

- **Source:** CR12-003 and CR12-006; `Source/KalmalaUI/Private/KalmalaJoinAddressWidget.cpp:79,112-139`; `Source/KalmalaUI/Private/KalmalaWorldListWidget.cpp:308,480-505`.
- **Affected path:** the editable address field in `UKalmalaJoinAddressWidget` and editable world-seed field in `UKalmalaWorldListWidget`.
- **Trigger:** change text scale or contrast while either widget remains open. `NativeTick` detects the preference change and calls `RefreshTheme`, but the editable field's style is only built and applied during widget construction.
- **Impact:** the fields retain their previous font size and contrast colors while neighboring text and controls reflect the new accessibility preference, making the input controls inconsistent and potentially harder to read.
- **Evidence:** the join-address builder calls `AddressInput->SetWidgetStyle(InputStyle)` once before `RefreshTheme`; its refresh updates panels, labels and buttons but not `AddressInput`. The world-list builder similarly styles `SeedInput` once, while its runtime refresh updates `SeedInputLabel` and other controls but not `SeedInput`. No focused test covers these two editable-field refresh paths.
- **Proposed repair:** reapply each editable box style inside `RefreshTheme` using the current `LastTextScale` and `LastContrast`, following the existing text-color, foreground and high-contrast background rules.
- **Dependencies:** After CR19 review acceptance; this review did not modify UI code.
- **Acceptance:** while each widget remains open, all supported text-scale settings and both contrast modes update the editable field's font and colors along with adjacent controls; focus, input contents and commit behavior remain intact.
- **Focused validation:** add a narrow automation or source-boundary regression for both widgets, then exercise supported scale/contrast changes in the rendered menu after an Editor build.
