# Project-wide review findings

- **Baseline revision:** `fc83040078cb02284d7bc0b4ce2f960d873b648a` (`master`; captured 2026-10-30T17:17:26Z).
- **Current state:** All four CR02 batches have been reviewed with no confirmed actionable finding. Remaining review batches are pending; no overall project-clean conclusion is implied.
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

## Confirmed actionable findings

None recorded through CR03-001a; later batches remain pending.
