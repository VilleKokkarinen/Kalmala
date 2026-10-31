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
