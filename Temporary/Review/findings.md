# Project-wide review findings

- **Baseline revision:** `fc83040078cb02284d7bc0b4ce2f960d873b648a` (`master`; captured 2026-10-30T17:17:26Z).
- **Current state:** All four CR02 batches have been reviewed with no confirmed actionable finding. CR03 reviewed batches through CR03-001b-96 have queued RF01-RF19; later CR03 batches and the remaining review children are pending. No overall project-clean conclusion is implied.
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

- **Source:** CR03-001b-96; `docs/07-development-setup.md:2680-2783`.
- **Affected path:** The Wolf, Lynx, Polar Bear, Horned Goat, Thunder Sentinel and Reef Maw live multi-peer command examples.
- **Trigger:** A developer follows one of the six documented per-species `*Peer.ps1` commands.
- **Impact:** The referenced file does not exist, so the documented live acceptance command fails before launching its peer scenario.
- **Evidence:** `Get-ChildItem Scripts/Tests/Combat` shows only `EnemyPeers.ps1`. That current runner supports `WolfPack`, `LynxLeap`, `PolarBearCharge`, `HornedGoatPush`, `ThunderSentinelSlam` and `ReefMawBite` via its `-Scenario` parameter; the six setup-doc paths do not resolve.
- **Proposed repair:** Replace each per-species script path with `Scripts/Tests/Combat/EnemyPeers.ps1 -Scenario <matching-name>` and retain the documented seed.
- **Dependencies:** After CR19 review acceptance; no documentation repair is authorized during CR03 review.
- **Acceptance:** All six setup commands reference the existing runner and a valid matching scenario while retaining seed 445.
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
