# Project-wide review findings

- **Baseline revision:** `fc83040078cb02284d7bc0b4ce2f960d873b648a` (`master`; captured 2026-10-30T17:17:26Z).
- **Current state:** All four CR02 batches have been reviewed with no confirmed actionable finding. CR03 partial exposure review found RF01 (P3). Remaining review batches are pending; no overall project-clean conclusion is implied.
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
