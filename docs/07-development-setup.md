# Development setup

## M11 skill notification child

See `docs/40-notifications.md`. Build the affected editor target in the short
isolated mirror, then run `Kalmala.UI.Notifications.SkillLevels` plus
`Kalmala.UI.Theme.LocalPresentation`, `Kalmala.Gameplay.Progression.SkillContract`
and `Kalmala.Gameplay.Progression.ReplicationContract` with isolated user/log
paths, memory DDC and `-TestExit="Automation Test Queue Empty"`. This is targeted
increment verification; combined rendered placement and full parent checks
remain pending until the combined acceptance child is complete.

## M11 ingredient/requirements parent acceptance

Follow `39-crafting-ingredients.md`: build the short isolated editor mirror
with normal UnrealBuildTool access, run the full `Automation RunTests Kalmala`
queue with isolated UserDir/logs and queue-empty TestExit, then rendered
`Verify-Crafting.ps1` at1280x720/100%/contrast0 and1024x768/150%/contrast1.
The runner now requires fourteen source captures per peer, including
build-costs/build-requirements/cook-costs/cook-requirements and their successful
developer review assertions. Rendered timeout is180 seconds for the bounded
extra capture stages; no gameplay timing changes. Inspect both peers' source
PNGs for complete readable counts and requirements through the normal scroll
view. Run mirror Verify-Inventory for owner privacy/transaction regression,
ownership, five M5 contracts, script parser and diff/path checks. Retain accepted
captures and exact results/limits in docs/39 and PROGRESS before parent closeout.

## M11 combined menu browsing acceptance

Build `KalmalaEditor Win64 Development` in the short disposable mirror with
normal `%LOCALAPPDATA%/UnrealBuildTool` access and `-MaxParallelActions=4`.
Close verification peers before rebuilding so their loaded UI DLL is released.
Run the full `Automation RunTests Kalmala` queue with isolated UserDir/logs,
null renderer, memory DDC and the queue-empty TestExit gate. Require every
test to succeed and process/test exit 0.

Run mirror `Scripts/Verify-Crafting.ps1 -Rendered` at 1280x720/100%/contrast0
and 1024x768/150%/contrast1 with separate unused ports. Require both peers'
inventory, recipe and build browsing, focus/navigation, modal restoration,
server rejection/payment and final-state checks. Each peer must produce the
original top/details/feedback/inspection captures plus Cooking, Structural
pieces, Stations, Camp utilities, no-results and grouped inventory captures.
The added developer-only review stages scroll to the browsing controls before
capture; high text scale retains the normal scrollable header and close control.
Review saved source PNGs for readable controls, group names, selected/unavailable
cards, no-results recovery, and non-overlapping inventory headings/cards.

Run mirror `Scripts/Verify-Inventory.ps1` for owner-only state and transaction
regression, then presentation ownership, all five M5 documentation contracts,
PowerShell parser and diff/path checks. Preserve source capture evidence and
exact roots/results in `38-menu-browsing.md` and PROGRESS.md. This completes the
browsing parent only; physical keyboard/controller text entry, exhaustive
viewport combinations and packaged acceptance are not certified by the fixture.

## M11 build browsing increment

Follow `38-menu-browsing.md`: compile affected UI in the short disposable mirror
with normal UnrealBuildTool access, run Kalmala.UI.Crafting.LocalBrowsing with
isolated UserDir/logs/null renderer/memory DDC, and require Success/test exit0.
Run rendered Verify-Crafting at standard1280x720; both peers must report Build
browsing Groups=1 SelectionKept=1 CategoryKey=1 NoResults=1 alongside existing
recipe/inventory/modal/authority checks. Run ownership, five M5 documentation
contracts, script parser and diff/path checks. Parent integration stays pending
until the final browsing acceptance child.

## M11 recipe browsing increment

Follow `38-menu-browsing.md`: compile affected UI in the short disposable mirror
with normal UnrealBuildTool access. Run `Kalmala.UI.Crafting.LocalBrowsing` with
isolated UserDir/logs, null renderer and memory DDC; require Result={Success}
and test exit0. Run rendered `Scripts/Verify-Crafting.ps1` at 1280x720/100%
standard contrast; both peers must report Recipe browsing SelectionKept,
Category, NoResults, Restored and SearchFocus all1 alongside existing navigation/modal,
transaction and capture checks. Run M5 documentation contracts and diff/path
checks. This is child-level verification; build browsing and parent integration
remain pending.

## M11 inventory browsing increment

Follow `38-menu-browsing.md`: compile affected `KalmalaUI` sources in the short
disposable mirror with normal `%LOCALAPPDATA%/UnrealBuildTool` access, then run
`Automation RunTests Kalmala.UI.Inventory` with isolated `-UserDir`, `-abslog`,
`-nullrhi`, `-DDC-ForceMemoryCache` and `-TestExit="Automation Test Queue Empty"`.
Require `LocalBrowsing` and existing inventory tests to pass. Run rendered
`Scripts/Verify-Crafting.ps1` at 1280x720/100% standard contrast; both peer logs
must include `Inventory browsing: CategoryKey=1 SortKey=1 NoResults=1 Restored=1`
and the existing inspection/focus/modal/authority checks. Run presentation
ownership and M5 documentation contracts plus `git diff --check`. This is
increment-level verification; full parent integration waits for recipe/build
browsing and the final ordered acceptance child.

## M12 Inventory browsing increment

After an affected UI compile in the short disposable mirror, run
`Automation RunTests Kalmala.UI.InventoryMenu.Selection` with isolated
UserDir/logs, null renderer, memory DDC and the queue-empty TestExit gate.
Require the menu test to cover bounded trimmed display-name search, item/tool
filters, deterministic copied-row sorting, canonical selection through search
and live owner refresh, safe removal/no-results fallback, D-pad and shoulder
navigation, retained query/selection/scroll state, and focusable controls at
150% text scale/high contrast. It also checks the panel shrinks with a 480x320
viewport, returns to its standard size at 1024x768, and retains outer-menu and
inventory-row scroll fallbacks. Run the M5 documentation contracts and
`git diff --check`.
Do not run the full rendered matrix for this child; the M12 final verification
must inspect host/client captures at standard settings and at 1024x768/150%/
high contrast, including menu resize, scroll, and no-results recovery.

## M11 Escape options opening animation

In a disposable project mirror, build `KalmalaEditor Win64 Development` with
normal `%LOCALAPPDATA%/UnrealBuildTool` access and `-MaxParallelActions=4`, then
run the full `Automation RunTests Kalmala` queue. Require
`Kalmala.UI.Theme.LocalPresentation` to pass its duration/travel/easing bounds,
easing, invalid-value fallback, and theme-disabled instant-motion assertions.

Run `Scripts/Verify-SettingsAccessibility.ps1` from the mirror. Both host and
client logs must contain successful `OpeningStart`, `OpeningResize`,
`OpeningCloseReopen`, `OpeningResizeRestore`, and `OpeningAnimation` stages. The
probe changes each isolated window from 1280x720 to 1600x900 and back while the
panel is moving, interrupts and reopens it mid-transition, and checks the
intermediate/final positions, center anchoring, immediate focus, modal input
suppression/restoration, keyboard/controller mappings, gameplay stability, and
the ten existing view captures. The temporary viewport change is restored
before captures complete. Then run `Scripts/Verify-SettingsAccessibilityContract.ps1`,
`Scripts/Verify-M5DocumentationContracts.ps1`,
`Scripts/Verify-PresentationOwnership.ps1`, and `git diff --check`.

The probes use rendered offscreen editor peers and isolated settings profiles;
they do not establish physical controller hardware behavior or package cooking.

## M11 Escape options backgrounds

Prepare a disposable project mirror with the original
`Content/Kalmala/UI/Source/OptionsPanel.png`, project/plugin source and
content, excluding generated directories. First run the forced
`KalmalaEditor Win64 Development` build with normal
`%LOCALAPPDATA%/UnrealBuildTool` access and `-MaxParallelActions=4`; the clean
mirror needs its project modules before `ImportAssets` can start. Then import
the PNG as `/Game/Kalmala/UI/OptionsPanel`:

```powershell
$mirror = 'C:\temp\kopt'
$userDir = Join-Path $mirror 'ImportUser'
$ddc = Join-Path $mirror 'ImportDDC'
New-Item -ItemType Directory -Force -Path $userDir, $ddc | Out-Null
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  "$mirror\Kalmala.uproject" -run=ImportAssets `
  "-source=$mirror\Content\Kalmala\UI\Source\OptionsPanel.png" `
  -dest=/Game/Kalmala/UI -nosourcecontrol -unattended -nop4 -nosound -nullrhi `
  -NoZenAutoLaunch -DDC=NoZenLocalFallback `
  "-LocalDataCachePath=$ddc" "-UserDir=$userDir" `
  "-abslog=$mirror\ImportAssets.log" -forcelogflush
```

Use a mirror/temp root and output paths that keep every absolute Windows path
below 260 characters.

Run the full `Automation RunTests Kalmala` queue, then
`Scripts/Verify-SettingsAccessibility.ps1`,
`Scripts/Verify-SettingsAccessibilityContract.ps1`,
`Scripts/Verify-M5DocumentationContracts.ps1`,
`Scripts/Verify-PresentationOwnership.ps1`, and `git diff --check`. The
host/client settings probe visits the Escape shell and Video, Audio, Controls,
and Settings pages; require every configured panel image to resolve, high
contrast to suppress it, and existing values, focus, text scale, keyboard and
controller bindings, modal ownership, and local persistence checks to pass.
Retain the runner output and inspect all fourteen PNGs: standard-contrast
Escape, Video, Settings, reduced-motion Settings, and restored HUD captures,
plus high-contrast Controls and Audio captures on both peers. Use the two
viewport runs described in the Local Settings tab section; physical input and
packaged asset inclusion remain separate checks.

## M11 inventory/build backgrounds and slot grids

For the inventory/build visual parent, prepare a disposable project mirror
with project/plugin source and content, excluding generated directories. Use
normal `%LOCALAPPDATA%/UnrealBuildTool` access and `-MaxParallelActions=4` for
the forced `KalmalaEditor Win64 Development` build. Import the two original
PNG sources from `Content/Kalmala/UI/Source/` into `/Game/Kalmala/UI` with
Unreal's `ImportAssets` commandlet before runtime verification.

Run the full `Automation RunTests Kalmala` queue. Then run the rendered
two-peer checks from the mirror:

```powershell
Scripts/Verify-Inventory.ps1 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Inventory.ps1 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-Crafting.ps1 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Crafting.ps1 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
```

Inventory verification requires 16 empty slots, a populated stack with its
count, carried-tool condition, owner-only state, and positive scroll extent
for the populated view. Build verification requires canonical recipe/build
icons, selected/focused/unavailable labels, positive scroll extent, and
selection/restoration through focused keyboard and D-pad key events. Inspect
the screenshots for both original backgrounds and the high-contrast fallback.
The checks cover representative editor rendering; they do not verify physical
input devices or cooked/package asset inclusion. Retain selected captures and
the exact runner/test results in `PROGRESS.md` and
`docs/ui-inventory-build/`.

## M11 status parent and complete icon verification

Follow `36-status-icons.md`: build the isolated editor mirror after the three
user-requested implementations, then run the full Kalmala automation queue,
the new rendered `Scripts/Verify-StatusHotbar.ps1` viewport/text-scale matrix,
and existing rendered crafting, settings/accessibility and owner inventory
peer regressions. The hotbar probe is explicitly presentation-only and records
actual owner snapshots before supplying its six-effect fixture. Inspect PNGs;
retain exact results and unverified scope in PROGRESS.md.

## M11 theme foundation verification

Theme keys and the restart/load workflow are documented in `35-ui-theme.md`.
Build a disposable project mirror (including existing project plugin source,
excluding generated directories) to keep generated outputs out of the checkout.
The theme foundation children use these focused tests after compilation:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' '<mirror>\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='<temporary-user-directory>' -abslog='<temporary-log-file>' -ExecCmds="Automation RunTests Kalmala.UI.Theme.LocalPresentation+Kalmala.UI.SurvivalStatus.LocalPresentation+Kalmala.UI.WeatherActivity.LocalPresentation+Kalmala.UI.Settings.LocalPresentation+Kalmala.UI.Inventory.PreparedFoodDetails; Quit" -TestExit="Automation Test Queue Empty"
```

Require all five `Result={Success}` results. Run
`Scripts/Verify-M5DocumentationContracts.ps1` and `git diff --check` as well.
These are increment-level checks; they do not complete the theme parent or
verify rendered menu propagation, viewport layout, or packaged theme loading.

For the final theme child / parent integration gate, build the isolated editor
mirror with normal `%LOCALAPPDATA%/UnrealBuildTool` access and
`-MaxParallelActions=4`. Run the full `Automation RunTests Kalmala` queue with
the isolated headless flags above; require every completed test to succeed and
test exit code 0. From the mirror, run `Scripts/Verify-Inventory.ps1`,
`Scripts/Verify-Crafting.ps1 -Rendered`, `Scripts/Verify-SettingsAccessibility.ps1`,
and `Scripts/Verify-WorldMap.ps1` with unused ports. The map runner covers
1024x768, 1280x720, and 2560x1080; settings covers 150% text/high contrast.
Inspect retained PNGs as well as runner assertions. Repeat rendered map and
build checks with changed theme values in the disposable mirror, and with
150% text/high contrast in its local settings. Restore mirror config afterward.
This gate verifies representative editor presentation and existing authority /
modal input contracts; it does not establish a new package or physical-input
playthrough. Evidence and remaining M11 scope are recorded in `35-ui-theme.md`.

## M11 shared hover and focus feedback

After an isolated editor build with normal `%LOCALAPPDATA%/UnrealBuildTool`
access, run the full `Automation RunTests Kalmala` queue and require exit 0.
`Kalmala.UI.Theme.LocalPresentation` covers themed pointer/focus/selection and
disabled states, high-contrast outlines, transition settings, and immediate
reduced-motion presentation. Run `Scripts/Verify-SettingsAccessibility.ps1`,
`Scripts/Verify-Crafting.ps1 -Rendered`, and `Scripts/Verify-Inventory.ps1
-Rendered` from the mirror with unused ports. Require host/client focus,
selection, modal restoration, owner presentation, and existing transaction
checks to pass; inspect the retained standard and high-contrast captures for
active-tab, focused-control, selected-card, and unavailable-card cues. Run the
ownership and M5 documentation audits, `git diff --check`, changed-script
PowerShell parsing, and the 260-character path audit. Theme configuration can
disable interaction motion with `AnimateInteractionStates=False`; the local
Reduced motion setting below overrides theme animation when enabled.

## M12 HUD and interaction prompt binding text

The owner-local prompt uses the existing short visibility trace as its only
interaction candidate. Build the isolated editor mirror with normal
`%LOCALAPPDATA%/UnrealBuildTool` access, then run the full
`Automation RunTests Kalmala` queue with a unique `-UserDir`, `-abslog`,
`-nullrhi`, `-DDC-ForceMemoryCache`, and queue-empty `-TestExit`. The focused
`Kalmala.UI.InteractionPrompt.Presentation` test covers target/action text,
absence of keyboard/controller labels, unavailable reason, missing-target,
modal, and live remapping cases. Remapped labels remain available in Options;
they must not appear in the prompt. The full queue also retains
`Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation` for server range and
authority behavior.

Run `Scripts/Verify-InteractionPrompt.ps1` from the mirror at 1280x720/100%
standard contrast and 1024x768/150% high contrast, with separate unused ports.
It renders the available and unavailable action plus modal/no-target clearing
on both host and client; inspect the retained source PNGs for readable action
text without binding labels. Run rendered
`Scripts/Verify-Crafting.ps1` at both settings for live modal suppression and
existing interaction/transaction regressions, and
`Scripts/Verify-SettingsAccessibility.ps1` for the local Controls remapping
path. Finish with `Scripts/Verify-LocalInputContract.ps1`,
`Scripts/Verify-PresentationOwnership.ps1`,
`Scripts/Verify-M5DocumentationContracts.ps1`, changed-script PowerShell
parsing, `git diff --check`, and the 260-character path audit. This verifies
editor-rendered presentation and existing server validation; it does not claim
physical keyboard/controller hardware or packaged-build acceptance.

## M12 player-facing input-binding text

Gameplay prompts, inventory and service menus, the expanded map, status/detail
views, and the Settings home screen do not print key/button names or control
combinations. The map keeps its symbol legend, category state, and concise
share/ping action names. Options > Controls is the only player-facing view that
shows current bindings; all existing input paths and remapping behavior remain.
Run `Scripts/Verify-MenuInputCopy.ps1` for the narrow source audit of visible
copy, binding-label resolution, retained action names, and keyboard/controller
navigation seams. After an affected UI build, run the focused map, status,
inventory, and crafting automations; host/client rendering, text scaling, and
physical input remain in M12 milestone-final verification.

## M11 ingredient-count child

Follow `39-crafting-ingredients.md`: compile affected UI in the short disposable
mirror with normal UnrealBuildTool access. Run
`Kalmala.UI.Crafting+Kalmala.Gameplay.Crafting.Transactions` with isolated
UserDir/logs, null renderer and TestExit queue-empty; require success and exit0.
Run mirror `Scripts/Verify-Crafting.ps1 -Rendered` for actual Slate geometry
and existing navigation/modal/transaction gates, plus ownership, five M5
documentation contracts and diff/path checks. Parent/full integration and
combined scale/contrast pixel acceptance wait for remaining ordered children.

For the requirements child, also run
`Kalmala.Gameplay.Food.CookingStationHeat` in that focused queue. The
`Kalmala.UI.Crafting.Requirements` automation is included by the UI prefix;
rendered Verify-Crafting additionally asserts the selected result, ingredients,
supported batch quantity, live station/heat requirements and one current blocker;
direct construction retains its carried-hammer and placement checks. Generic
recipe detail copy omits skill/unlock boilerplate. See docs/39.

## Baseline

- **Engine:** Installed Unreal Engine 5.8.2 build at `C:\Program Files\Epic Games\UE_5.8`.
- **Project:** `E:\dev\Kalmala\Kalmala.uproject`.
- **IDE:** Visual Studio 2026 with the Game development with C++ workload, MSVC tools, and a Windows SDK.

### M7 persistence gate

The first M7 save contract is isolated in
`UKalmalaM7PersistenceSaveGame`. It is schema 1, matches an exact world seed
and generator revision 7 plus explicit world/player scope, and accepts at most
256 server-selected sparse resource, creature, or discovery identities. Schema
0 requires an explicit migration review; future schemas, invalid identities,
duplicate IDs, over-cap entries, and path-like IDs fail closed. This contract
does not extend existing save schemas or add progression, tool, food, or loot
state. The focused round-trip/rejection test is
`Kalmala.World.M7.PersistenceContract`.

After the editor build, run it with the documented temporary-user pattern:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7PersistenceUser' -abslog='C:\temp\KalmalaM7Persistence.log' -ExecCmds="Automation RunTests Kalmala.World.M7.PersistenceContract; Quit" -TestExit="Automation Test Queue Empty"
```

### M9 progression and discovery migration gate

The accepted schema-2 design extends the existing construction and
player-discovery save owners; it leaves the M7 sparse ledger and storage schema
unchanged. Read `docs/30-m9-persistence-migration.md` for exact seed/revision/
scope matching, caps, and the schema-1 migration rules. Normal schema-2 writes
are enabled after the focused round-trip, migration, rejection, and host/client
reconnect checks passed. Construction and player writers preserve validated
schema-1 facts, validate complete server-owned candidates before saving, and
leave invalid existing slots untouched. The player writer also re-derives live
M9 claim identities and saves the owner's bounded tool records without adding
replicated state.

### M8 travel-save contract

`UKalmalaOceanTravelPersistenceSaveGame` is a separate schema-1 gate over the
existing M7 world/player identity. World scope accepts one stable skiff ID and
one bounded last-safe transform; player scope accepts one authenticated
player's vessel ID and assigned seat. The current skiff carries no cargo, and
the contract contains no velocity, steering, or live actor references. Schema
zero requires an explicit migration decision; future schemas, identity
mismatches, malformed vessel IDs, out-of-bounds transforms, invalid seats,
and cross-world or orphaned player-to-vessel associations fail closed. The
server game mode saves a generated-ocean-validated skiff snapshot while
moored, reloads it at startup, restores authenticated seats on login, and
preloads that player's sparse ocean-discovery ledger. Seat collisions and
failed disembark-save clears fail closed; inventory and cargo are not saved.

After the forced editor build, run the in-memory and local-slot round-trip and
rejection contracts with the M7 identity/save checks:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM8TravelSaveUser' -abslog='C:\temp\KalmalaM8TravelSave.log' -ExecCmds="Automation RunTests Kalmala.World.OceanTravel.PersistenceContract+Kalmala.Gameplay.OceanTravel.SkiffRestoreContract+Kalmala.World.M7.PersistenceContract; Quit" -TestExit="Automation Test Queue Empty"
```

### M7 skill progression contract

`FKalmalaSkillProgressionLedger` provides a transient server-owned contract for
the six allowlisted skills: Gathering, Woodcutting, Mining, Crafting, Cooking,
and Survival. Only an accepted server action may award up to 25 experience;
total experience is capped at 1,000, levels are derived from 100-experience
bands, and unlock tiers derive at levels 2, 5, and 10. The attached
`UKalmalaSkillProgressionComponent` replicates detailed state only to the owning
player and a derived highest-level/unlock badge to relevant peers. It has no
client RPC or setter for progression outcomes, and it does not add save fields.

After the editor build, run the focused authority and derivation test:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7SkillUser' -abslog='C:\temp\KalmalaM7Skill.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.Progression.SkillContract; Quit" -TestExit="Automation Test Queue Empty"
```

Run the owner/privacy and original ledger checks together when changing the
replication seam:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7CombinedUser' -abslog='C:\temp\KalmalaM7Combined.log' -ExecCmds="Automation RunTests Kalmala.World.M7.PersistenceContract+Kalmala.Gameplay.Progression.ReplicationContract+Kalmala.Gameplay.Progression.SkillContract; Quit" -TestExit="Automation Test Queue Empty"
```

### M7 first-wave biome content identity

`FKalmalaBiomeContentContract` defines one original gathering-source ID,
creature-niche ID, and optional rare-discovery-source ID for each of the six
first-wave land biomes. The server derives the matching gathering or niche ID
when it builds a population descriptor and replicates the selected identity
on the relevant harvest or wildlife actor. A valid gathering source builds an
original collision-free procedural presentation from that replicated ID on
both peers; it does not change the existing reward, depletion, or save path.
Each valid point-of-interest rare source also maps to a distinct original
collision-free procedural presentation. The source identity is replicated only
with the already materialized relevant discovery actor; clients do not receive
candidate lists or author definitions, positions, claims, rewards, or routes.
Wildlife then derives a bounded
ecological profile from that replicated niche: flee pressure remains between
180 cm and 300 cm and a sparse original vertex-colour accent helps nearby peers
read habitat identity. Existing spatial/seed persistent IDs, depletion rules,
archetype combat behaviour, inventory rewards, and save schemas remain
unchanged. Point-of-interest descriptors use the optional rare source ID;
clients do not submit or enumerate these identities.

After the editor build, run the focused deterministic catalogue check:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7BiomeContentUser' -abslog='C:\temp\KalmalaM7BiomeContent.log' -ExecCmds="Automation RunTests Kalmala.World.M7.BiomeContentContract; Quit" -TestExit="Automation Test Queue Empty"
```

The wildlife authority regression covers the six niche profiles and their
bounded flee responses through `Kalmala.Gameplay.WildlifeBehaviour.ServerOwnedCycle`.

### M7 first-wave tool gathering

Valid generated gathering sources select a bounded transient tool/action on
the client; the existing server interaction RPC still carries no target or
outcome. The server retraces, validates the source/tool/action/skill/condition,
preflights the catalogue reward against inventory capacity, and commits pack,
condition, node depletion, and the existing sparse callback as one accepted
action. Owner-only condition replication and the current no-persistence limit
are specified in `17-m7-tools-and-gathering.md`. Run its focused automation,
then `Scripts/Verify-InventoryReconnect.ps1` for the host/client inventory and
tool transaction fixture. The Camp crafting panel provides one repair action
per tool at a visible Joiner's workbench; the server derives the matching
gathered material cost from current condition and publishes payment and repair
together. Run `Kalmala.Gameplay.Crafting.NetworkContract` and the same
reconnect script to check the payload boundary, validated station, atomic
material failure/success, and owner-only condition. The same reconnect fixture
wears the Field Hatchet through accepted server harvests and verifies its
replacement, then wears the Stone Pick through server-selected mining harvests
and verifies its replacement, then gathers Fibre with the Reed Knife and
verifies its replacement: zero-condition and visible-workbench gates, exact
material payments, no mutation on intact-tool, missing-station, or missing-
material attempts, owner-only condition restoration, post-payment Crafting
experience, and duplicate rejection. Recipe identity and batch remain the
only crafting RPC fields.

## First build

For M10 lower-resource diagnostics on the available high-end PC, see
[`34-m10-constrained-performance.md`](34-m10-constrained-performance.md).
`Scripts/Verify-ConstrainedPerformance.ps1` runs the existing rendered two-peer
fixture with Potato/Low/Med/High/Ultra resource and quality presets or the
original reference/eight-thread/four-thread CPU-only profiles. It verifies
peer masks and requested rendering settings and preserves isolated evidence. Use a built
disposable project mirror. These are CPU-contention diagnostics; they do not
establish representative target-hardware or numerical budget acceptance.

Open PowerShell and run:

```powershell
& 'C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe' /projectfiles 'E:\dev\Kalmala\Kalmala.uproject'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaEditor Win64 Development -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex
```

Open the project in the editor with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'E:\dev\Kalmala\Kalmala.uproject'
```

The first editor launch must create the prototype map at `/Game/Kalmala/Maps/Prototype/L_Prototype`. Do not set it as the default map until it exists.

The existing `L_Prototype` is now the configured editor startup and game default map. It contains the M1 fixtures and tagged sun/sky lighting, with no template floor or landscape underneath the generated terrain. `Scripts/Setup-PrototypeEnvironment.py` reproducibly adds that lighting through Unreal's Python commandlet and saves only this map. Run it while other editor/test processes are closed to avoid a map file lock. Press Play in `L_Prototype`; an already-open `/Engine/Maps/Templates/OpenWorld` has its own landscape that intersects the generated world and can show checkerboard patches in depressions. Restart the editor to use the configured startup map, or open `L_Prototype` explicitly before Play.

### M5 performance/startup baseline

The 2026-09-22 supported-Windows profile passed without a bounded actor,
memory, worker, or raster regression. The forced `KalmalaEditor Win64
Development` build succeeded with UnrealBuildTool access to
`%LOCALAPPDATA%\UnrealBuildTool`; a temporary Windows Development package
cooked, staged, and archived successfully. The archived packaged listen server
reached network readiness in 12,025.8 ms. This is a local Development/null-RHI
startup baseline, not a shipping frame-time target.

`Scripts/Verify-WorldProfile.ps1` recorded 179.99 ms initial generation,
1,760.48 MiB used physical memory, 46 actors, 28 replicated actors, 9 terrain
patches, 1 active population key, and 2,215 serialized save bytes before the
two-peer late-join check. `Scripts/Verify-PlayerControls.ps1` passed normal
generated-world movement and replicated remote movement. `Scripts/Verify-CampChoices.ps1`
passed two freely chosen camp sites with 31 matched server exposure snapshots
per pawn and normal fire recovery. `Scripts/Verify-WorldMapProfile.ps1`
recorded host/client map profiles; the client measured 826.540 ms open,
760.878 ms total worker time, 203.300 ms maximum worker time, 16.343 ms total
game-thread time, 12 ready tiles, and 202,800 cached CPU bytes. The focused
minimap/map automations passed, including 18.635 ms per tile, 48.963 ms for
the four-tile minimap case, and the 278,784-byte cache ceiling.

These profiles remain diagnostic evidence only. They do not establish audible
quality, hardware/controller input, packaged persistence, long-session memory
growth, shipping GPU frame time, or the final tool-free co-op acceptance.

## Bounded generated-world profile

After an editor build, run `Scripts/Verify-WorldProfile.ps1`. It starts a seed-418 listen server using the current generator revision, then joins a conflicting-seed client and waits for the existing replicated immutable identity. The server logs initial generated-world setup time, process physical-memory snapshot, total and replicated actor counts, active terrain patches/population keys, and sparse population-save bytes serialized to memory. It does not mutate the save, change the 25-patch budget, adjust density, or accept client-selected world data. The runner succeeds only after the late-joining client reports `Seed=418` and the server reports two players plus successful save serialization.

The 2026-09-23 M7 seed-418 profile measured 103.13 ms initial generation,
1,776.77 MB used physical memory, 47 actors, 29 replicated actors, 9 terrain
patches, 1 active population key, and 2,215 serialized population-save bytes;
the conflicting-seed client joined with the server identity and save
serialization succeeded. Compared with the recorded revision-4 sample, this is
one additional total/replicated actor and 16.29 MB more process memory, with
generation time lower by 76.86 ms and the patch, population-key, and save-byte
counts unchanged. These are single-run development measurements, not a
long-session memory-growth result or a shipping memory limit. The existing
25-patch world budget remains in force, and the independent M7 save contract
rejects more than 256 sparse identities.

## Water surface regression check

`Kalmala.World.Water.ClippedSurface` checks partial-cell coverage, winding, flat levels, shallow shore treatment, seeded closed basins, rejection of sea-connected/unbounded/unseeded basins, deterministic meshes, and nonempty matching patch-edge intersections. Lake-biome fields seed enclosed terrain basins rather than clipping floating sheets at humidity/temperature boundaries. Basins exceeding 8,192 wet lattice vertices are conservatively omitted. The minimap uses the same visible water decision. Terrain/collision, server wetland rules, saved-data schemas are unchanged. Run `Scripts/Verify-PlayerControls.ps1 -Rendered` for rendered host/client traversal and screenshots after building. Restart the editor to load the repaired native module.

## Ocean-depth regression

After an editor build, run headless automation with `-unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -ExecCmds="Automation RunTests Kalmala.World.Water+Kalmala.World.Ocean.IslandLocator+Kalmala.UI.Minimap.LocalPresentation" -TestExit="Automation Test Queue Empty"`, directing `-UserDir` and `-abslog` into a unique temporary directory. `Kalmala.World.Water.OceanDepth` verifies depth against both collision-triangle planes, negative coordinates, equivalent adjacent-patch origins, same-identity reproduction, different-seed variation, and actual clipped coastline vertices; the fixture must contain wet sea floor, dry land, and mixed coastal triangles. `Kalmala.World.Ocean.IslandLocator` verifies a revision-4 seed resolves a repeatable naturally emergent island using the same terrain triangles. These checks do not verify a full host/client ocean crossing or extended streaming.

## Generated-ocean swimming regression

After an editor build, run `Scripts/Verify-Swimming.ps1`. It starts a memory-only listen server with seed 418 and a conflicting-seed client with seed 999. Each owning pawn walks to the nearest deterministic sea-depth fixture and must enter the generated-ocean custom movement mode; the server log proves authoritative entry and the client log proves prediction from the server-replicated identity. Entry requires at least 100 cm depth and return-to-land uses a 75 cm hysteresis threshold. The test adds no water volume, RPC, client depth input, island, boat, or streaming change.

## Long-distance ocean travel regression

After an editor build, run `Scripts/Verify-OceanTravel.ps1`. It starts a current-generator seed-418 listen server and a conflicting-seed client. The server resolves the existing nearest emergent-island endpoint once, spawns a non-shipping replicated deep-water ribbon from a nearby entry point to that endpoint, and both peers adopt only that server-owned fixture descriptor. Each locally controlled pawn crosses the spawned water through predicted Character Movement and must log ocean entry, island arrival, and a complete duplicate-free terrain neighborhood. The fixture's temporary world-static collision relaxation and faster traversal cap exist only under `-KalmalaOceanTravelTest`; normal swimming, terrain collision, island lookup, client intent, save data, and production replication contracts remain unchanged.

## M8 skiff launch and occupancy contract

After an editor build, run `Kalmala.Gameplay.OceanTravel.SkiffAuthorityContract` and `Kalmala.Gameplay.OceanTravel.SkiffSteeringContract` with isolated temporary `-UserDir`, `-abslog`, `-DDC-ForceMemoryCache`, and `-TestExit="Automation Test Queue Empty"`. The launch test checks the server/deep-water/generated-terrain/world-bound gates, one-skiff session cap, helm-first and passenger-second seat selection, and stopped safe-exit gate. The steering test checks helm-only authority, finite bounded axes, increasing sequences, the 10 Hz rate limit, 0.5-second expiry, acceleration, and forward/reverse speed caps. These focused contracts do not prove a live client interaction, rendered skiff presentation, or collision movement over a representative coastline.

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM8SkiffUser' -abslog='C:\temp\KalmalaM8Skiff.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.OceanTravel.SkiffAuthorityContract+Kalmala.Gameplay.OceanTravel.SkiffSteeringContract; Quit" -TestExit="Automation Test Queue Empty"
```

Add `Kalmala.Gameplay.OceanTravel.SkiffCoastlineAccess` and
`Kalmala.World.Water.OceanDepth` to that automation prefix when validating
coasts. The coastline test follows 32 radial mainland coast samples from the
resolved start for seeds 418, 999, and 1337; it checks shallow-water rejection,
the production nine-point hull-depth footprint, a qualifying server launch
sample, and one of the server's eight 190 cm safe-exit surface candidates. It
requires launch candidates between 100 and 250 cm of sampled ocean depth and
within 300 m of the dry shoreline. `OceanDepth` validates the sampled depth
against the generated collision-triangle planes and clipped coastal mesh.
These deterministic checks do not instantiate a skiff, execute a visibility
interaction trace, query live pawn overlap, or exercise the skiff's actual world
collision sweep. Dry-shore-specific acceptance was waived on 2026-09-26; the
M8 journey accepts a safe stopped disembark through the server's qualifying
deep-water fallback.

## M8 late-join and restart/reconnect verification

After a forced editor build, create a short disposable project mirror and run
`Scripts\Verify-OceanSkiffReconnect.ps1 -Port 18171 -Project '<mirror>\Kalmala.uproject'`.
The runner starts a seed-418 listen host, its returning owner, and a third late
joiner using isolated user directories. It verifies the late peer sees one
moored vessel with the original helm/passenger occupants, remains unattached,
and receives no private discovery reward. It then stops all peers, restarts the
host with its original profile, reconnects the owner with the owner's original
profile, and checks that the same vessel and both authenticated seats restore.
The returning owner retries the same discovery; the server must report
`AlreadyFound` and leave the fresh inventory at zero. A second late join checks
that restart preserved the same public vessel state and private reward boundary.

This non-shipping fixture assigns stable local test-provider identities so the
headless null network driver can exercise the existing authenticated save-key
path across separate processes. It does not integrate an external online
identity provider. Player travel restore is server-owned and retries after a
controller's pawn and PlayerState are available; repeated restore calls are
idempotent for a peer already attached to its saved seat. The fixture adds no
gameplay RPC, replicated gameplay field, or save-schema change. Keep the live
logs under the runner's temporary output directory for diagnosis.

## M8 coast feedback presentation

After the forced editor build, run
`Kalmala.Gameplay.OceanTravel.FeedbackAuthority` and
`Kalmala.UI.SurvivalStatus.LocalPresentation` with the isolated headless
automation flags above. The first checks server-only feedback selection and
the launch denial mapping for shallow water, hull footprint, world edge,
session capacity, and invalid coast hits. The UI test checks one-metre
open-ocean access guidance, all-nine-sample hull depth, safe-stop text, and
generated-terrain collision recovery guidance.

Then run `Scripts\Verify-OceanSkiffFeedback.ps1 -Port 18166`. Its listen-host
and conflicting-seed client receive distinct server-published owner messages;
both local HUD text paths must show their own result, while the client copy of
the host pawn must retain no feedback. This headless check verifies owner-only
replication and message text, not a rendered viewport or live launch trace.

## M8 ocean weather navigation pressure

After a forced editor build, run
`Kalmala.Gameplay.OceanTravel.SkiffWeatherPressure` and
`Kalmala.UI.SurvivalStatus.LocalPresentation` with the isolated headless
automation flags above. The skiff test checks that head or following wind adds
no yaw, crosswind direction changes the drift sign, pressure scales with
server wind and vessel speed, helm counter-steering works, and the combined
steering rate retains its 35 degrees/second cap. The local status test checks
that the crosswind guidance appears aboard a skiff and clears when replicated
wind subsides. The focused tests do not replace the remaining host/client
weather-pressure and recovery agreement check.

Then run `Scripts\Verify-OceanSkiffWeather.ps1 -Port 18167`. The development-
only host/client fixture selects a full crosswind state and then calm weather
on the server. Both locally controlled peers must derive the same 4 degrees/s
pressure and -3 degrees/s counter-steered rate from the production skiff helper
at 350 cm/s, then derive zero wind pressure after the calm state replicates.
The client also attempts to replace the accepted crosswind with a forged calm
state; its server-only setter must leave the replicated weather unchanged.
This check covers accepted weather replication, derived pressure/recovery, and
client authority rejection, but does not move or render a live skiff.

## M8 integrated skiff crossing

After a forced editor build, run `Scripts\Verify-OceanSkiffJourney.ps1 -Port 18169`.
The development-only seed-418 listen host and conflicting-seed client create a
server launch from an active generated terrain patch, occupy the helm and
passenger seats, and sail 2.4 km with the existing bounded steering rules. The
fixture also selects crosswind and calm weather, requires both local peers to
observe replicated seats, underway movement, and the final moored state, and
checks that terrain-patch coordinates transition while active patches remain
at or below 25. The generated launch hit and seat assignment are driven by the
server fixture; the remote client owning the helm sends bounded steering through
the existing server RPC. The fixture does not test physical input or a
client-originated launch/boarding request. The finite 16 km world does not configure origin rebasing,
so the fixture verifies patch streaming and records `OriginShift=inactive`.
Discovery claims, disembark, late join, restart/reconnect, and long-session
resource budgets remain separate M8 acceptance work.

Status (2026-09-25): The forced editor build and runner syntax checks pass, but
the live crossing is blocked. Two direct server-steering attempts rejected
validated helm input; the listen-host and remote-client owner-RPC runs each
launched and occupied both seats but neither peer reported underway travel or
a stop after about two minutes. The latest run passed host/client weather-state
checks. See `PROGRESS.md`; do not count the crossing as verified until the
    steering path produces movement in this fixture.

Status (2026-09-26): The blocker is resolved. The fixture now gates neutral
movement-axis callbacks while it drives helm input, and holds the verified calm
state after weather cycle 7002 for the full route. The forced editor build,
focused skiff authority/steering contracts, PowerShell parser, and live
seed-418 host/client run passed. Both peers observed underway travel and the
moored stop at 242,511 cm across a terrain-patch transition with 9 active
patches. `OriginShift=inactive`; this run does not verify physical input,
rendered travel, or origin rebasing. Retained logs:
`C:\Users\Ville\AppData\Local\Temp\KalmalaOceanSkiffJourney-384596c99aeb413dbdad675876741e54`.

## M8 owner-scoped discovery and safe disembark peer check

After a forced editor build, run
`Scripts\Verify-OceanSkiffDiscoveryDisembark.ps1 -Port 18170`. The development-
only seed-418 listen host and conflicting-seed client use a canonical
deep-ocean discovery descriptor and isolated user directories. The server
places both players in the helm/passenger seats, accepts that optional claim
once for each authenticated player, and requires the exact catalogue reward
and owner-only discovery feedback. It then verifies the already-moored skiff
can safely disembark both players, clears both persisted seat associations,
and leaves the vessel moored and empty. The remote owner must receive only its
own reward, discovery acknowledgement, and disembark result while observing
the replicated empty seats. The fixture enters in `Moored` mode; it does not
prove sailing, deceleration from underway travel, a client interaction trace,
or a rendered presentation. Those remain separate M8 journey acceptance
checks.

## Item catalogue verification

`Content/Data/GameCatalogues.json` is the schema-version-4 source for both item and recipe definitions. No property or value in the file contains `Kit`; clean buildable IDs and each recipe's `RequiredStation` array map to stable runtime construction identities, preserving construction and save compatibility. Keep the `Data` directory in `DirectoriesToAlwaysStageAsUFS` under `[/Script/UnrealEd.ProjectPackagingSettings]`; the loader reads it through Unreal's file layer so it works from both the project and a packaged UFS container. The loader validates schema version, array bounds, required item descriptions and display names, recipe references, duplicate IDs, quantities, stack limits, and station references before exposing either catalogue. Hearth refuelling separately validates raw fuel materials. Missing or invalid data leaves the catalogues empty, and server transactions fail closed.

After building the editor, run `Kalmala.Gameplay.Inventory.Catalogue` with the headless automation flags above. It checks that all definitions in the schema-version-4 item and recipe arrays load, no JSON property or value contains "Kit", no retired item or recipe is present, clean buildable aliases map to stable runtime IDs, every item has a bounded description and a player-facing name without "Kit", exact and exceeded stack limits, empty/unknown IDs, zero/negative/extreme quantities, overflow-safe additions, full stacks, and malformed/duplicate definitions. It also checks station-array structure and the removed recipe fields; crop and seed definitions; all six current food recipe IDs, outputs, stations, batch limits, output stack limits, retired food/smoke aliases, and the recipe-free HearthBroth item. These are catalogue definitions only; planting, growth, harvesting, and food-use behavior remain separate concerns. The crafting-menu presentation check verifies descriptions for the selected output and browsed item. This is a pure contract check; live inventory replication and harvest-grant verification remain covered by their separate checks.

The catalogue and cooking contracts also cover Iron, the five-Iron Forge recipe
for a placeable Frying pan, the three root-vegetable outputs, station-filtered
Cooking rack/cauldron/pan recipes, and cooking heat at both station and player.
Looking at the three stations shows the remappable Interact prompt; the server
validates E and sends the selected station to the owner-local recipe GUI. Iron
currently has no configured gathering source.

## Player inventory verification

Run `Scripts/Verify-Inventory.ps1` after an editor build for the inventory component increment. A development-only `-KalmalaInventoryTest` fixture grants ten wood and consumes three on each server pawn, rejects unknown/overflow grants and invalid/insufficient consumption, and verifies removal of an exhausted stone stack. The runner requires two successful server results, seven wood on the remote owner, rejected client-local mutation calls, empty remote contents after owner replication, and matching immutable world identity. `Verify-InventoryPanelRemoval.ps1` separately audits the retired HUD source and keeps the on-demand Inventory menu and its empty-state regression present. Separate temporary user directories keep the scenario out of project-generated data. This headless check verifies authority/privacy, not rendered layout; rendered harvest feedback and reconnect persistence remain subsequent tasks.

The inventory runner also requires `Harvest inventory: Passed=1` for both server pawns. Its development-only fixture exercises twelve isolated initialized nodes covering Wood, Stone, and Fibre, uninitialized-node and distant rejection, full-stack rejection without depletion, successful retry after capacity is freed, duplicate rejection, and exactly one sparse-save callback per accepted grant. It restores the original seven-wood inventory before owner replication checks and destroys its temporary actors; it writes no world-save slot. Run `Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion` and `Kalmala.Gameplay.Inventory.Catalogue` with the headless flags above for the pure authority and malformed quantity gates. This does not yet exercise a client's actual harvest RPC, inventory reconnect restoration, or simultaneous competing player input.

## Workbench and storage verification

After the editor build, run `Kalmala.Gameplay.Storage` with the headless flags and unique temporary user/log paths above. It covers bounded storage serialization, identity mismatch, invalid stacks and quantities, transfer conservation/capacity, RPC payloads, and owner-only chest snapshots. Then run `Scripts/Verify-Storage.ps1` for paid workbench/chest placement, kit assembly, actual owner deposit/withdraw RPCs, distance/obstruction rejection, write-failure conservation, private peer snapshots, and a same-user-directory restart retaining the exact two chest IDs and contents. The runner writes only temporary user/save/log data. See `10-campfire-and-crafting.md` for controls and limits; this fixture does not close the later complete M2 camp acceptance gate.

## Dedicated-server build

The Epic Games Launcher engine distribution does not include dedicated-server support. The `KalmalaServer` target remains in the project, but building it requires a UE 5.8 source build or another UE 5.8 distribution with server support. Do not attempt the command below with the installed Launcher engine.

With a server-capable engine, build the target with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaServer Win64 Development -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex
```

## Generated-world traversal smoke test

For the developer-only two-player traversal verification, launch a listen server and a client with `-KalmalaTraversalTest`. Both peers derive the lake target from the server-selected world identity; locally controlled pawns move through normal Character Movement input, while the server remains authoritative. The test only disables pawn-to-pawn capsule blocking and only while the switch is present. A passing run logs that both server pawns reached the Shimmering Lakes target and that the client observed replicated movement.

## Generated harvest reconnect smoke test

Run the prototype map twice as a listen server with `-KalmalaReconnectVerification=Harvest` and then `-KalmalaReconnectVerification=Verify`. The first process activates a deterministic harvest node, uses the normal server interaction path to harvest it, saves its sparse delta, and exits. The restarted server uses the same immutable world identity and succeeds only when that node is suppressed. `WildlifeDefeat` and `WildlifeVerify` run the equivalent server-only path for one deterministic wildlife spawn. This developer-only switch is server-local and accepts no client-supplied node identifier or outcome.

## Deer peer and restart regression

After an editor build, run `Scripts/Verify-DeerPeer.ps1`. It starts a seed-418
listen server and conflicting-seed client (999) with isolated temporary user
directories. The server derives a bounded Deer descriptor and a second existing
Deer descriptor from its 3x3 population neighborhood, positions that normal
generated companion within the production herd-noise radius, and uses only the
ordinary target-free attack sequence. The runner requires deterministic
descriptor reproduction, combat-triggered herd alert, four validated attacks,
sparse defeat persistence, and owner-only DeerMeat/DeerHide. The remote peer
must see the server world identity, rejected target-free intent, shared action
state, and relevant defeat. A same-host-user-directory `WildlifeDeerVerify`
restart proves the identical server-derived Deer remains suppressed. The fixture
adds no client-selected herd, noise, target, damage, reward, ID, RPC,
replication property, or save schema.

## Discovery peer privacy regression

After an editor build, run `Scripts/Verify-DiscoveryPeer.ps1`. It starts a
seed-418 listen server and a conflicting-seed client (999) with isolated
temporary user directories. The server derives the same bounded descriptor
twice, confirms a different seed does not retain its canonical ID, materializes
only that normal server descriptor, and uses the existing payload-free
interaction path. The scenario rejects a distant remote claimant, records one
entitled-player acknowledgement, and rejects the duplicate without creating a
second discovery. The client must receive the server identity while retaining
no owner-only feedback for the other player's undiscovered content. This
development-only fixture sends no descriptor, target, reward, identity, or
progression payload from client to server and does not expose a discovery list.

## Support-magic foundation regression

After an editor build, run `Kalmala.Gameplay.Discovery.PlayerScopedPersistence`
with the normal headless automation flags. It verifies canonical scroll-to-effect
mapping, entitled-player learned-effect persistence through memory
serialization, immutable-world identity rejection, and the authority,
entitlement, sequence, cooldown, and stamina gates. Runtime activation accepts
only the enum/sequence intent. It also covers the Deer Call empty-set gate.
The regression also checks all four allowlisted effects against malformed
effect values, client-role activation, and replay/zero-sequence input; verifies
every learned token survives matching-world reconnect serialization; and audits
that every support effect is non-damaging. Active-effect fields remain ordinary
replicated presentation state; a rendered live-peer cast remains part of the
later M4 vertical slice.
Live Deer Call activation must remain server-owned: it may influence at most
three existing idle Deer within 900 cm, must reject before stamina payment when
none are eligible, and must not create wildlife or alter damage, harvest, loot,
or saved progression.

The M5 balance pass uses an 18-stamina base for every support activation. The
server runs that base through `UKalmalaPlayerStatusComponent` before checking
or charging stamina, so an active Wet status costs 20.7; the client supplies
neither value. The focused status and discovery automations cover the shared
cost calculation and tuned base constant. Every support effect also uses a
five-second server-owned cooldown after a successful activation; the client
supplies neither cost nor timing. Hearth Shield now keeps its bounded 40-point
absorption for ten seconds; the focused discovery regression covers that
server-owned duration constant. Bear's Vigor now keeps its existing 1.4x
strength and 140-point stamina cap for ten seconds; the same focused discovery
regression covers its server-owned duration constant. Live support authority
and reconnect checks remain in the M4 harness.

## Mireling boss scroll verification

The focused discovery regression also checks the bounded stable-ID gate and
world-seed-only optional boss scroll identity. Runtime defeat uses the
existing server Mireling callback: it validates the defeated actor and
authenticated attacker, persists the one canonical scroll plus learned effect
before owner-only feedback, and gives no reward to remote peers. The reward
does not create a route, reveal a discovery catalogue, or accept client
location/target/effect input. A rendered live two-peer boss-reward scenario
remains part of the M4 vertical slice.

## M4 vertical-slice harness

After an editor build, run `Scripts/Verify-M4VerticalSlice.ps1`. The harness
executes the existing isolated two-peer Mireling, boar, and deer scenarios on
separate ports, retaining each server/client/restart log under one evidence
directory, then runs `Kalmala.Gameplay.Discovery.PlayerScopedPersistence` for
all four learned support effects. This establishes the route-free creature,
owner-only reward, defeat-persistence, non-damaging support, and matching-world
learning-persistence baseline without adding a route, client-selected target,
reward, or save input. The Mireling fixture disables movement on its
server-controlled attacker after placement so gravity cannot invalidate the
production three-dimensional melee-range proof. A single rendered live run with four support casts,
late-join/reconnect presentation, and a persisted return state remains the
final M4 acceptance step.

## Exposure inspection

Launch a listen server with `-KalmalaExposureInspection` to log server-sampled terrain and field inputs plus the active replicated weather values and provisional exposure state. Once a player joins, the output includes continuous low-ground wetness, deterministic lake-adjacency shoreline wetness, ridge/slope wind exposure, Flora-derived natural cover, and server-traced roof/windbreak shelter inputs. Player-built collision geometry must carry `KalmalaShelterRoof` or `KalmalaShelterWindbreak`; authored volumes and client trace results are ignored. The weather cycle is selected and advanced only by the server. Every second, the server replicates actual wetness, warmth, and the resulting 69–100% Character Movement travel multiplier; shelter dries and restores warmth slowly when dry, while a nearby lit fire accelerates recovery.

## Exposure replication smoke test

Launch a headless listen server and then join one headless client with `-KalmalaExposureReplicationTest`. The server spawns a temporary lit campfire and initializes each normal player pawn to a wet, low-warmth state. Its server log records weather, sampled shelter, fire warmth, and the resulting exposure values; the client log records the replicated weather, campfire, and exposure values. The developer-only path has no client RPCs for weather, exposure, or campfire mutation and adds no persistent gameplay content.

## Two-player camp-choice scenario

After the editor build, run `Scripts/Verify-CampChoices.ps1` from PowerShell. It starts a hidden listen server (seed 418) and a conflicting-seed client (999) on port 17841, with logs and user data in a unique temporary directory. Both use `-KalmalaCampChoiceTest`; this developer-only scenario requires two normal possessed pawns including a remote player. It compares separated low/high-cover dry-land fixtures within the generated starting neighborhood, logs ground wetness, water distance, and harvest availability, and samples twelve seconds without fires followed by twenty seconds with normally interacted server-owned fires. Both players must dry, warm, and improve travel speed. The runner checks matching world identity, weather, and at least ten exact exposure snapshots per replicated player ID, then stops only its own processes. It fails on timeout or scenario assertions and retains logs for diagnosis.

The fixtures represent two optional camp choices, not runtime camp recommendations: no route, camp marker, authored shelter, or persistent content is added. This is automated systems verification in cold, dry starting weather, not a human usability test or validation of inventory, construction, rain preparation, or rendered feedback. Those systems retain their existing scope and limitations.

## Camp-condition inspection

Launch a listen server with `-KalmalaCampConditionInspection` and join a player. The server logs the freely chosen position's continuous natural cover, ground wetness, bounded nearest-water distance, and nearby deterministic harvest-node count. It only explains local tradeoffs; it neither creates nor marks a camp location, and clients cannot invoke or alter it.

## Biome-colour debug overlay

Launch the game with `-KalmalaBiomeDebug` to replace the generated terrain's normal material with a project-owned vertex-colour debug material. It uses the same continuous four-field biome classifier as terrain generation: Meadows are green, Shimmering Lakes cyan, Elderwood dark green, Mossy Mire olive, Freezing Tundra pale blue, Thunder Mountains grey, and Ocean blue. The material is developer-only, is built independently on each peer from the replicated world identity and patch descriptor, and changes no terrain, collision, gameplay state, or replication.

## Companion minimap presentation

Movement-triggered raster generation is asynchronous. Run `Kalmala.UI.Minimap.AsyncRefresh` after rebuilding to check nonblocking refresh timing, full-resolution worker equivalence, stationary reuse, movement coalescing, obsolete zoom/identity rejection, and reinitialization. `Kalmala.UI.Minimap.GenerationPerformance` retains the exact revision-3/4 fingerprints and reports total raster time separately from the game-thread refresh cost. Use `Scripts/Verify-Minimap.ps1 -Rendered` for current-world HUD/identity checks and `Scripts/Verify-PlayerControls.ps1 -Rendered` for host/client movement. Both scripts retain revision 1 as their default legacy fixture. Restart any existing editor after the native module build.

The local `UKalmalaMinimapSubsystem` creates a 208-unit `UKalmalaMinimapWidget` for each local player once its controller is available. The widget is anchored to the top-right viewport corner with 12 UI units of top and right inset, scaled proportionally by the viewport DPI curve. The existing weather badge remains below it with a 24-unit clear gap. It refreshes local seed-derived terrain/water samples around that player's pawn and draws only samples inside a circular radius, together with a centred marker rotated to the pawn's facing yaw. `MouseWheelAxis` changes only the local session's sampled radius, clamped from 2,500 to 10,000 cm in 750 cm steps; CommonUI's normal-game-input gate leaves wheel input to any modal UI. It reads no world actors, population, landmarks, or gameplay state beyond the already-replicated world identity and owning pawn transform.

Run `Scripts/Verify-Minimap.ps1` after an editor build for the two-peer identity check. It starts a memory-only hidden listen server with seed 418 and a conflicting-seed client with seed 999, then confirms that the client receives the server world identity. The focused `Kalmala.UI.Minimap.LocalPresentation` automation checks the two player-centred local views from that same identity, circular clipping, min/max zoom, modal input gating, the 12-unit top/right viewport inset at 4:3/75%, 16:9/100%, and ultrawide/125% UI scales, and separation from the weather badge. These checks are local presentation only and create no replicated, gameplay, or save mutation.

The compact active-status group reads the minimap widget's configured viewport slot and shares its top offset; its right edge sits 12 UI units left of the minimap's actual left edge. Both use the same top-right anchor, so Unreal applies the viewport DPI scale uniformly. `Kalmala.UI.StatusHotbar.SnapshotAndLayout` checks the actual slot relationship and wrapping fit across 4:3, 16:9, and ultrawide viewports at 75%, 100%, and 125% DPI scales. It also checks known live owner statuses, the existing Hot/Cold exposure qualifiers, Storm-only weather at the shared 0.65 storm threshold, interval expiry, and a fully empty result when all active conditions clear. Normal/active weather and fog-only highly active weather do not create an entry. If the minimap has not been created yet, the status group temporarily uses the documented default 208-unit map footprint and corrects itself on the next local refresh.

The M12 status-image follow-on pins the nine supported entry/icon identities in `docs/status-icon-manifest.csv` with fixed 4/4/1 generation batches. All three batches have retained originals and prepared transparent 64x64 RGBA PNGs under `Content/Kalmala/UI/Source/IconOriginals/Status` and `Content/Kalmala/UI/Source/Icons/Status`. Prepare a batch with `python Scripts/Prepare-StatusIconBatch.py --batch <Batch>` and validate its IDs with `Scripts/Validate-StatusIcon.ps1 -Id <IconId>`. `Scripts/Import-StatusIconAssets.ps1` imports the nine prepared PNGs in an isolated UE 5.8 content-only project, verifies 64x64 Texture2D assets with source alpha, sRGB, UI texture group, no mipmaps, and non-streaming settings, then copies only those packages to `Content/Kalmala/UI/Icons/Status`. `Scripts/Validate-StatusIconSet.ps1` checks manifest, PNG, entry-map and package coverage; `Kalmala.UI.StatusHotbar.StatusIconCoverage` checks runtime texture paths and loads after an affected editor build. The local `FKalmalaStatusIconLibrary` keeps these IDs separate from catalogue icons. The hotbar now renders each raster image at 64x64 without visible names, retaining accessible names in a visually hidden text label; a centred m:ss appears only beneath finite Wet/meal/support entries. Hot, Cold, Storm, and weather detail have no countdown. Timing remains based on the existing owner snapshot and synchronized server time, without local countdown mutation. See `36-status-icons.md` for the full identity and image review contract. These image assets do not change status ownership or gameplay.

## M12 final HUD, inventory, station, and interaction captures

After the final M12 editor build and automation queue, run
`Scripts/Verify-StatusHotbar.ps1` for each viewport/text-scale pair in the
matrix in `36-status-icons.md`. The helper checks the read-only host/client
snapshot, empty/populated/expired states, active six-icon fixture, icon-only
visual contract, finite versus untimed timer assertions, and the group's actual
12-unit top/minimap separation, left safe inset, and lower viewport bound. It
also checks that the widget source still creates raster images, visually hidden
accessible names and centred timers, and that the automation source retains the
finite/untimed assertions; the final automation queue executes those tests.
Inspect both peers' `empty`, `populated`, `expired`, `details`, and `icons` PNGs;
the populated captures must show no visible status names, real raster icons,
centred timers only for finite Wet, meal, and support effects, and safe wrapping.

Add these prepared host/client views to the same final capture handoff at both
standard 1280x720/100%/standard-contrast and compact 1024x768/150%/high-contrast
settings:

```powershell
Scripts/Verify-Inventory.ps1 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Inventory.ps1 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-Crafting.ps1 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Crafting.ps1 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-InteractionPrompt.ps1 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-InteractionPrompt.ps1 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-InventoryReconnect.ps1
```

Run the inventory peer checks for owner privacy and transactions, and make the
manual Inventory captures prescribed by the preceding pack-grid, selection,
equipment, and food sections for each peer's distinct, empty, and populated
state. Inspect the crafting runner's Build, Workbench/Forge, Cooking Rack/
Cauldron/Frying Pan, repair/upgrade, chest and Campfire/Grinding Stone captures,
plus every available/unavailable/modal/no-target/Repair All interaction-prompt
capture from both peers. Require each runner's owner-privacy, server
acceptance/rejection, station-context, stale-target and input-restoration
markers. These views supplement the complete M12 milestone-final authority and
accessibility checks; they do not replace the prescribed build, automation,
reconnect, or other verification.

### Rendered minimap regression check

The Phase 5 visibility repair replaces the sparse terrain dots with a filled, circular 129x129 texture containing original patterns for Meadows, Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, Thunder Mountains, and Ocean. Each pattern stays anchored in world space. The minimap now belongs to each local player rather than only the first game-instance controller. Its 208-unit size and 12-unit top/right inset scale with Unreal's UI DPI curve. Size/position must be set before the top-right anchor: `SetPositionInViewport` resets it to top-left in UE 5.8 and previously put the map off-screen.

After building, run `Scripts/Verify-Minimap.ps1 -Rendered -Width 1920 -Height 1080`. Repeat with `-Width 1024 -Height 768` and `-Width 3440 -Height 1440` for 16:9, 4:3 and ultrawide aspect ratios and automatic DPI scales. The runner checks actual paint geometry, the proportional 12-unit top/right insets, and full texture sample count on both peers, server identity replication, and the bound input delegate with minimum/maximum zoom, CommonUI Menu ownership, and zoom resumption. The focused automation checks that the weather badge remains below the minimap. It captures `host.png` and `client.png` in the printed temporary log directory for visual inspection. Hidden offscreen captures verify the Slate HUD; the world background may be black and these images do not validate terrain rendering. The configured `CommonGameViewportClient` is required for CommonUI routing. Without `-Rendered`, this script remains an identity-only smoke test.

`Kalmala.UI.Minimap.LocalPresentation` now also exercises the production viewport setters, checks filled/transparent raster coverage, distinct deterministic texture patterns for all seven biomes, and sea-level ocean treatment. These checks replace the previous assumption that mathematical placement assertions alone proved on-screen visibility. The launch-gated input scenario temporarily changes local input configuration and restores it and zoom; it sends no RPC or gameplay request. A normal launch requires no minimap flag. Restart the editor/game to load rebuilt C++ modules and viewport configuration; an older packaged executable needs a separate rebuild/package.

## Local settings menu

Press Escape during normal play to open the local Settings menu; press Escape again to close it and restore game input. The menu's main screen provides Options and Quit. Options contains Video, Audio, Controls, and Settings tabs; Video immediately applies and saves resolution, V-Sync, window mode, and render-distance quality through Unreal `GameUserSettings`. Run `Kalmala.UI.Settings.LocalPresentation` after an editor build for the render-distance bounds seam. This is local presentation/preferences only: no setting, menu action, or quit request is sent to the server.

The Audio tab now cycles local master volume in 25% steps and provides
mute/restore, with text showing the current level and action. It saves these
preferences in the local `GameUserSettings` config and applies the engine's
primary output-volume multiplier at local-player initialization. The focused
`Kalmala.UI.Settings.LocalPresentation` automation checks bounds, config
persistence, mute, and restore; run it after an editor build with the standard
temporary `-UserDir`, `-abslog`, `-DDC-ForceMemoryCache`, and
`-TestExit="Automation Test Queue Empty"` flags. Audio-category levels and input
remapping, text scale, contrast, and the local colour-independent feedback
mode are implemented; full rendered settings coverage remains the accessibility
follow-up surface.

### Local audio category controls

Since the master-only increment above, the Audio tab now includes focusable
Ambient, Music, and Interaction/Combat Feedback buttons. Each displays its
current value and cycles through 0%, 25%, 50%, 75%, and 100%; values save in the local
`GameUserSettings` config. Ambient scales the local wind, rain, water, fire,
and biome loops, while Interaction/Combat Feedback scales owner-local
movement, status, crafting/gathering, discovery, combat, and support cues. The
Music value is retained for a future music path; no music track currently
plays. These settings remain process-local and do not change gameplay state.

Run `Kalmala.UI.Settings.LocalPresentation` after an editor build to check
category bounds and local config round-trips alongside master mute/restore.
`Scripts/Verify-SettingsAccessibilityContract.ps1` and
`Scripts/Verify-AudioCueContract.ps1` check the written option and mix
contracts without launching Unreal. These checks do not render the Audio tab,
simulate physical controller input, or establish audible quality/device mix.

### Local Controls tab

The Controls tab exposes the existing movement, look, interact, attack, jump,
sprint, settings, map, recenter, craft, and support-activation intents. Each
row shows the current keyboard and controller label and cycles through a small
allowlisted set of alternatives; movement rows retain positive/negative axis
pairs. Restore default controls clears only the local overrides. The runtime
applies overrides to the owning `UPlayerInput` and stores them in the local
`GameUserSettings` configuration, so `Config/DefaultInput.ini` and server
validation remain unchanged. Escape remains an always-available modal close
path even if the alternate Settings key is changed. The settings automation
checks bounded labels, local persistence, rejection of an out-of-set key, and
restore defaults.

### Local Settings tab

The Settings tab cycles local text scale through 100%, 125%, and 150%, and
cycles between Standard and High contrast. Changes rebuild the current modal
immediately, use auto-wrapped text, and retain a larger bounded panel so the
Controls list remains scrollable at the largest scale. Contrast updates the
modal backdrop, panel, button surfaces, text, and focusable state controls as
one local palette; every value remains visible as text. Values persist in the
local `GameUserSettings` configuration and send no RPC or gameplay request.
The `Kalmala.UI.Settings.LocalPresentation` automation checks bounded choices
and config round-trips.

The same tab provides a local colour-independent feedback choice between
`Text only` and `Text + markers`. Marker mode adds bracketed text markers for
nearby hearth and construction context in an owner-only overlay that follows
the scaled viewport in a center-right safe area. Combat/support action results
and discovery acknowledgements use the separate bounded notification queue;
the support selection strip has explicit text in either feedback mode. These
surfaces read only the existing local pawn and accepted replicated results,
follow the local contrast palette, and send no request.
`Kalmala.UI.Settings.LocalPresentation` checks its bounded mode and config
round-trip. The focused automation remains a null-RHI contract check; use the
rendered peer probe below for the live modal.

Whole-interface scale is a separate 80/90/100/110/120% choice, defaulting to
100%, layered over the project's UI scale. It reflows the game HUD and menus;
the bounded Escape panel follows the viewport and keeps a 16-unit margin.
Reduced motion defaults off and removes the opening slide, themed button/card
highlight transitions, and animated scrolling. Focus, selection, text, and
actions remain immediately visible. Both preferences are stored in the local
`GameUserSettings` config and are restored when the same local profile restarts.
`Kalmala.UI.Settings.LocalPresentation` checks scaling bounds/application and
local persistence; `Kalmala.UI.Theme.LocalPresentation` checks the static
reduced-motion path.

### Rendered settings and accessibility regression

For parent-level settings acceptance, build an isolated UE 5.8.2 editor mirror
with normal `%LOCALAPPDATA%\UnrealBuildTool` access, then run the full
`Automation RunTests Kalmala` queue. Run the rendered settings probe at both
viewport sizes, using the extreme local scales on the smaller view:

```powershell
Scripts/Verify-SettingsAccessibility.ps1 -Width 1280 -Height 720 -HostInterfaceScale 90 -ClientInterfaceScale 110 -Port 18461
Scripts/Verify-SettingsAccessibility.ps1 -Width 1024 -Height 768 -HostInterfaceScale 80 -ClientInterfaceScale 120 -Port 18462
```

Choose another unused port if either example port is occupied.

Require local settings to survive a fresh-process restart, menu focus and
modal input to remain correct, both motion paths to behave as selected, and
the Escape panel to fit. Inspect the retained standard/high-contrast menu and
HUD captures from both viewports. Run
`Scripts/Verify-SettingsAccessibilityContract.ps1`, the ownership and M5
documentation audits, changed-script PowerShell parsing, `git diff --check`,
and the 260-character path audit. The remaining physical controller and
packaged-build checks are outside this parent increment.

`Scripts/Verify-SettingsAccessibility.ps1` starts isolated host/client profiles.
It starts a seed-418 listen server and conflicting-seed client, opens the live
Escape shell and Video, Audio, Controls, and Settings tabs, and captures each
at the selected 1280x720 or 1024x768 viewport. Escape, Video, Settings,
reduced-motion Settings, and restored HUD use standard contrast; Controls and
Audio also retain high-contrast captures. The development-only probe writes
bounded local audio, text, contrast, colour-independent feedback, interface
scale, reduced motion, and keyboard/controller remapping values to each peer's
`GameUserSettings.ini`, applies those mappings to only the owning `UPlayerInput`,
verifies focus ownership and the Escape mapping, then closes the modal. It
compares pawn health, transform, and the replicated server world identity before
and after local changes. A fresh pair of processes reuses each local user
directory to verify saved scale and motion choices. The printed temporary
directory retains host/client logs and fourteen PNG captures per viewport run.
Inspect the rendered modal, HUD, and backgrounds; this verifies the two
representative viewport sizes, not physical controller hardware, audible
quality, or packaged persistence.

## M5 onboarding contract check

`Scripts/Verify-OnboardingContract.ps1` is a no-build check for the retained
local tutorial design in `docs/13-onboarding-and-tutorial.md`. It requires the
ten route-free prompt beats, no key/button legends in prompt examples,
colour-independent text/icon guidance, visible-context triggers, server
authority boundaries, hidden-content privacy rules, and protection of prompt
history from the gameplay save schema. `Scripts/Verify-TutorialRouteFree.ps1` additionally checks that
the M12 runtime presenter is disabled. These checks do not launch Unreal or
claim packaged two-player prompt flow has passed.

## M5 settings and accessibility contract check

`Scripts/Verify-SettingsAccessibilityContract.ps1` is a no-build check for
`docs/14-settings-and-accessibility.md`. It validates the existing Video,
Audio, Controls, and Settings groups, text-scale and contrast requirements,
bounded control remapping and restore defaults, keyboard/controller focus
access, text-scale and contrast requirements, the bounded colour-independent
feedback mode and owner-only markers, reversible local storage, no-RPC
boundaries, and the remaining runtime verification limits. It does not launch
Unreal or claim that the full option set is functional.

## M5 presentation ownership check

`Scripts/Verify-PresentationOwnership.ps1` is a no-build audit for
`docs/15-presentation-ownership.md`. It confirms the seven project-owned world
materials, the procedural player/wildlife/environment/hearth sources, the local
UI texture/feedback sources, the four code-drawn support-effect glyphs, and the
absence of known engine-basic-shape, Starter Content, Marketplace, or third-
party asset paths. It does not prove material loading, visual readability,
audio, animation, packaging, or host/client screenshots; those still require
the relevant Unreal verification.

After an editor build, run `Scripts/Verify-DeerPeer.ps1 -Rendered` for a
positioned host capture of the original deer silhouette plus the bounded
host/client combat, reward-privacy, and defeat-persistence checks. The
development-only capture camera is requested by the server fixture after it
positions the existing target and herd mate; normal gameplay camera and
authority are unchanged. Use `-Project <path-to-uproject>` to run an isolated
build copy. The capture reviews one 1280×720 lighting/view direction and does
not establish readability at other distances or during other lighting.

## M5 rendered Mireling silhouette review

Run `Scripts/Verify-MirelingPeer.ps1 -Rendered` after an editor build to capture
the original Mireling in a bounded listen-host/client encounter. The transient
host camera is positioned after the fixture arranges the generated actor; the
runner also checks server combat, rejected client target-free attacks,
owner-only reward, and defeat persistence after a same-world restart. Use
`-Project <path-to-uproject>` for an isolated build copy. The 1280×720 capture
shows the low hunch, reaching arms, and split crown in close view; the dark body
planes merge somewhat, and other distances and lighting remain unreviewed.

## M5 audio cue contract check

`Scripts/Verify-AudioCueContract.ps1` is a no-build check for
`docs/16-audio-cue-contract.md`. It requires the eight original cue groups,
readable text/shape fallbacks, local mix scope, project-owned asset rule,
server-authority/privacy boundaries, silence fallback, and no new RPC/save
field. It does not create or play sound assets, test mixing/spatialization, or
claim packaged two-player audio verification.

For local ambience, `Scripts/Generate-WildernessWind.ps1`,
`Scripts/Generate-WaterAmbience.ps1`, `Scripts/Generate-FireAmbience.ps1`,
`Scripts/Generate-BiomeAmbience.ps1`, and
`Scripts/Generate-WeatherExposureAudio.ps1`,
`Scripts/Generate-SupportFeedbackAudio.ps1`,
`Scripts/Generate-CombatResultAudio.ps1`, and
`Scripts/Generate-InteractionFeedbackAudio.ps1`,
`Scripts/Generate-DiscoveryAcknowledgementAudio.ps1`,
`Scripts/Generate-SupportEffectAudio.ps1`,
`Scripts/Generate-MovementTraversalAudio.ps1`, and
`Scripts/Generate-WaterTraversalAudio.ps1` recreate original mono sources
under `Content/Kalmala/Audio/Source`, including the loop-seamed `RainBed.wav`
and the one-shot `WetStatusCue.wav`, `SupportAcceptedCue.wav`,
four support activation cues, two timed-effect expiry cues,
`CombatResultCue.wav`, `InteractionAcceptedCue.wav`,
`InteractionRejectedCue.wav`, `DiscoveryAcknowledgedCue.wav`,
`MovementFootfallCue.wav`, `MovementJumpCue.wav`, `MovementLandingCue.wav`,
`GeneratedOceanEntryCue.wav`, and `GeneratedOceanExitCue.wav`. Import the
twenty-two audio assets to
`/Game/Kalmala/Audio` with Unreal's
`ImportAssets` commandlet, then run `Scripts/Verify-AmbientAudio.ps1`,
`Scripts/Verify-AmbientAudioPeers.ps1`, `Scripts/Verify-CombatPeer.ps1`,
`Scripts/Verify-DiscoveryPeer.ps1`, `Scripts/Verify-PlayerControls.ps1
-MovementAudio -OceanTraversalAudio`, and the forced editor build. This
development-only audio mode injects one local sample transition pair and
requires each host/client owner to submit one entry and one exit cue; it never
sets a movement mode or sends audio over the network. Pass
`-WeatherExposureOnly` to the peer runner for a focused weather/Wet cue check;
the default mode still requires visible-water activation. The first
verifier checks source format, imported assets, local-player-only subsystem
contract, water/hearth visibility gates, local biome sampling, replicated
precipitation and wind, the owner's replicated Wet status, local support,
combat, discovery, and movement cues, generated-ocean entry/exit transitions,
owner-only crafting result cues, and accepted
owner-only inventory increases. The peer runner checks
independent host/client sampling, local loop creation, visible water and
server-lit hearth context, each peer's sampled-biome bed, and weather/exposure
cues, plus accepted Hearth Shield and Bear's Vigor activation and expiry cues,
an accepted Fuel craft, an actual transient harvest, and a rejected recipe
result for each pawn. Crafting text,
result serial/outcome and detailed inventory remain owner-only. The combat peer runner separately confirms that
only the locally owning pawn hears the confirmed Hit/Defeat cue; Unavailable
remains text-only. The discovery peer runner confirms only the entitled local
owner submits the landmark/scroll acknowledgment cue; duplicate, unavailable,
and remote undiscovered results remain text-only. The support grant/cast and weather changes exist only under
`-KalmalaAmbientAudioTest`. Test weather is light enough to keep the
test hearth lit, and a test-only server hook preserves Wet during the status-cue
check. The test support casts still use the server component and wait for its
existing timed states to end. These checks do not claim audible device mix or packaged
verification.

`Scripts/Verify-PlayerControls.ps1 -MovementAudio` drives the existing local
walk/sprint/jump/landing fixture on a listen host and connected client. It
requires each owning player to submit its own distance-paced footfall, upward
jump, and landing cues while retaining the existing rendered model and bound
input checks. It does not establish audible quality, hardware mixing, or
packaged playback.

## M5 local input contract check

`Scripts/Verify-LocalInputContract.ps1` is a no-build check for the current
keyboard/mouse and controller baseline in `Config/DefaultInput.ini`, as
documented in `docs/14-settings-and-accessibility.md`. It validates the
movement/look/map axes and Interact, Attack, Jump, Sprint, SettingsMenu,
WorldMap, WorldMapRecenter, InventoryMenu, and CraftMenu action bindings,
including the default Tab/I Inventory keys. It does not launch Unreal or prove
runtime input routing.

## M12 inventory modal input increment

Run `Scripts/Verify-LocalInputContract.ps1` to check the default Tab/I action.
Inspect the owner-local Inventory shell's gameplay-only open gate, Escape close
through the existing SettingsMenu action, editable-text focus guard, gamepad B
close, and restoration of the prior cursor plus movement/look ignore state.
Crafting and the map already refuse to open while a modal ignores movement;
the Settings handler first closes Inventory. No gameplay request, inventory
mutation, network state, or save state is added. Full Unreal and rendered
host/client checks remain deferred to M12 milestone-final verification.

## M12 inventory pack-grid increment

The Inventory menu reuses `UKalmalaCatalogueRowsWidget` to draw the fixed
16-slot pack grid, including the existing catalogue icons, display names, item
counts, empty cells, and filled-slot count. Its data source is only the local
player controller's pawn inventory component; that component replicates pack
stacks with `COND_OwnerOnly`. An absent pawn/component is reported as waiting,
while a valid zero-stack pack explicitly reports that it is empty. The menu
refreshes on open and while visible so accepted owner inventory changes appear
without a gameplay request. The persistent HUD pack display was removed by the
later M12 panel-removal increment; the modal Inventory menu is now the only pack
grid surface. This increment adds no RPC,
mutation, replicated field, or saved-data field.

For M12 acceptance, open Inventory independently for the host and client with
different owner pack contents, then with an empty pack. Confirm each menu shows
only that owner's canonical rows and counts, all sixteen slots in the empty
case, and live owner updates while open. Capture standard and high-contrast
views at the supported text scales as part of milestone-final verification.

## M12 inventory selection increment

The Inventory menu retains selection by canonical ID while owner rows refresh.
Previous/next buttons and arrow/D-pad input update the selected-slot outline,
non-colour selected label, and shared `UKalmalaItemDetailWidget` with the
canonical item icon/description and visible count. If the selected stack
disappears, the first remaining owner row becomes selected; when no rows remain,
selection clears and the detail panel hides. Selection and detail data are local
to each menu instance and read only the owning pawn's existing owner-only
inventory component.

After an affected UI build, run the focused automation test
`Kalmala.UI.InventoryMenu.Selection`. It checks selected detail/count updates,
owner-instance separation, fallback after removal, and empty-pack clearing.
The milestone-final rendered host/client pass must additionally confirm that
each peer sees its own selected item and that the controls/detail fit standard
and high-contrast supported text scales. This increment adds no RPC, gameplay
action, replicated field, or saved-data field.

## M12 carried-tool Inventory increment

Inventory also reads at most the six canonical records in the owning pawn's
existing owner-only `CarriedTools` array. The shared catalogue rows widget keeps
these equipment cards separate from the sixteen pack slots. Each card and the
selected detail panel display the tool's authored level, current/max condition,
and ready/damaged/broken state; malformed or unknown records cannot fabricate a
usable state. Selection remains local to the menu instance.

For a valid damaged or broken selected tool, `Repair selected tool` sends only
that canonical tool ID through the existing `UKalmalaCraftingComponent` repair
request. The server continues to look up its carried record, validate the
visible same-world Workbench or Forge within 250 cm, and publish the existing
owner-only result. The menu adds no client-supplied condition, station, cost,
RPC, replicated field, or save data. Run
`Kalmala.UI.InventoryMenu.Selection` after an affected build to check mixed
pack/tool selection, tool-level/condition display, owner-instance privacy,
fallback, and empty clearing. The final rendered host/client check also reviews
the equipment rows, repair action and response at supported scales/contrast.

## M12 carried-food Inventory increment

When one of the three supported meal items is selected in the owner's pack,
Inventory shows its existing Steady Meal effect and the live owner-visible
availability: the current serving count, active meal time remaining, pending
request, or last owner-only server result. `Eat one serving` is enabled only
when owner inventory/status and the existing crafting component are present,
the selected allowlisted item has a serving, and no meal or food request is
active. It calls `ServerConsumeFood` with only that item ID. The action has no
station/range gate. The existing server transaction revalidates the allowlist,
pack quantity and free meal slot, atomically consumes one serving, applies the
120-second 10%-lower stamina-use effect, and publishes the owner-only result;
rejected and repeated requests consume nothing and cannot stack, refresh, or
replace an active meal. No new RPC, gameplay state, replication field, or save
data is introduced. `Kalmala.UI.InventoryMenu.Selection` also checks the
supported-food effect text, visible Eat action, and fail-closed disabled state
when its owner components are absent; run it after an affected UI build.

For the M12 final host/client acceptance, exercise each supported item through
Inventory while away from stations, verify the count changes only after server
acceptance, and inspect the owner timer/effect and result. Confirm zero count,
missing owner data, an active meal, a stale/replayed request, and a rejected
request disable or reject use without consumption or effect refresh. Verify
that the other peer cannot see the owner's pack or action result. Rendered
scale/contrast and broad regressions remain in the milestone-final matrix.

## M12 HUD feedback decoupling increment

The selected support effect appears in a separate passive owner-local strip,
using the character's local selection and the owner's learned-effect state.
Input still uses the existing 1–4/D-pad selection actions and Q/controller
activation action. The transient notification queue now also observes the
owner-only combat and support result serials, plus concise discovery
found/already-found/unavailable acknowledgements from the existing owner
source. The Inventory menu must not supply these results or own the support
glyph selection. No new gameplay request or authority path is introduced.

Run `Scripts/Verify-LocalInputContract.ps1`,
`Scripts/Verify-PresentationOwnership.ps1`,
`Scripts/Verify-M5DocumentationContracts.ps1`, and `git diff --check` after an
affected editor compile. `Kalmala.UI.SupportSelection.LocalHudCue` checks the
selection label for learned, unavailable, and no-selection cases.
`Kalmala.UI.Notifications.CombinedPresentation` checks silent combat/support
baselines, new serial feedback, deduplication, bounded expiry, and that combat
text contains no target identity. Rendered strip positioning, action notices,
modal behavior, and host/client privacy remain in M12 milestone-final review.

## M12 persistent left-panel removal increment

The always-visible `UKalmalaInventorySubsystem`/`UKalmalaInventoryWidget` pack
panel is retired. Its pack/tool rows, duplicate support glyphs, build/craft
shortcut, prepared-food banner, HUD-suppression hook and screenshot fixture no
longer exist. Inventory remains available through its owner-local modal; it is
collapsed until opened and continues to read only that owner's pack/tools.
Support selection remains on its separate top-centre strip and combat/support/
discovery results remain in transient owner notifications.

The focused `Kalmala.UI.InventoryMenu.Selection` automation covers normal
collapsed startup, owner-specific rows, selection fallback, no-results recovery,
and the full sixteen-cell empty pack with stale details hidden. The
`Kalmala.UI.Inventory.PreparedFoodDetails` automation now reads the actual
Inventory menu. `Scripts/Verify-InventoryPanelRemoval.ps1` checks that retired
inventory runtime classes, help text, crafting suppression and old capture
expectations stay absent, then confirms the tutorial presenter cannot mount the
bottom card; the inventory and reconnect host/client scripts run this check as a
preflight. After an affected editor build, run the focused menu automations,
`Scripts/Verify-PresentationOwnership.ps1`, and `Scripts/Verify-Inventory.ps1`.
Rendered host/client layout and privacy remain in M12 milestone-final review.

## M5 documentation contract suite

Run `Scripts/Verify-M5DocumentationContracts.ps1` to execute the onboarding,
settings/accessibility, presentation ownership, audio-cue, and local-input
checks together. This is a no-build consistency gate for the current M5
contracts; it does not replace Unreal runtime, rendered, packaged, or
two-player verification.

## Expanded world map

The map now renders continuous opaque terrain tiles; circular clipping applies only to the companion minimap. Both views sample the same generated terrain/water colours at matching world coordinates. Exploration is collected every 0.5 seconds during ordinary gameplay, including before the first M press and while the map is closed, so travelled areas accumulate on the main map. Unvisited areas remain fogged. Tile drawing is clipped to the map panel so edge tiles cannot leak into the controls. Existing coverage saves and the original map palette remain compatible.

`Kalmala.UI.WorldMap.LocalPresentation` checks opaque square-tile corners, both tile-join directions, matching minimap/world-map RGB, and connected remembered walking coverage. The rendered `Scripts/Verify-WorldMap.ps1` fixture additionally requires both peers to record coverage while closed with zero terrain tiles before accepting the usual input/identity/paint screenshots. Run `Scripts/Verify-Minimap.ps1 -Rendered` to inspect the persistent companion HUD crop during gameplay.

The expanded map uses full-stretch viewport anchors with zero offsets. In UE 5.8, `SetDesiredSizeInViewport` becomes right/bottom margins under stretch anchors; setting a fixed 1920x1080 size collapses the surface at common viewport sizes and leaves only a marker near the top-left. Keep those margins zero and derive tile aspect ratio from the same DPI-scaled Slate geometry used for painting and pointer input. `Scripts/Verify-WorldMap.ps1` now requires actual full-player-viewport paint geometry, completed terrain tiles, and a fog texture on both peers before accepting screenshots at all three resolutions. Its zoom-bound probes restore the normal opening zoom before capture.

Press `M` to open the local expanded map. It occupies the viewport with cached 10,000 cm world-space terrain/water tiles, generated asynchronously from the replicated world identity and owning pawn, so it never reveals population, discoveries, or other server-only data. Drag with the left mouse button to pan, use the mouse wheel to zoom at the pointer, and press `R` to recenter on the owning player. Press `M` or Escape to close it; Escape closes the map before opening Settings. A view or identity change immediately drops obsolete local tile handles before requesting at most 64 centre-prioritized tiles; stale workers are epoch-rejected and never block the game thread. A 6,500 cm circle around only the owning pawn's already replicated transform is clear; revealed 500 cm cells persist in a bounded 8,192-cell local save keyed by seed and local-player index. Remembered personal coverage has a translucent sea-glass teal tint; a contrasting lichen-ember tint is reserved for future explicitly opted-in shared coverage, which is not yet loaded or rendered. The local save is independent of generated-world sparse saves and identity mismatches are rejected. Pan and zoom cannot change the reveal source or radius. A pale facing triangle shows only the owning pawn at its true world-to-map position (centred after recentering); world-aligned reference lines use a local 2,500, 5,000, or 10,000 cm scale and provide no route or destination. Run `Kalmala.UI.WorldMap.LocalPresentation`, `Kalmala.UI.WorldMap.PerformanceBudget`, and `Kalmala.UI.Minimap.LocalPresentation` after an editor build to verify deterministic tile edges, a 278,784-byte maximum CPU tile-pixel payload, a 250 ms single-tile worker budget, and a 1.5 s full-minimap worker budget while four map tiles are running. `Scripts/Verify-WorldMapTiles.ps1` starts a current-generator seed-418 host and conflicting-seed client, checks the client receives the immutable server identity, and compares their completed visible-tile count and deterministic pixel fingerprint. It then restarts both peers with their own retained local user directories and requires matching-identity personal coverage to report `Loaded=1` while a remote probe remains opaque. `Scripts/Verify-WorldMap.ps1` renders the same authoritative host/client pair at 1024x768, 1280x720, and 2560x1080, requiring both peers' open/input/zoom/pan/recenter trace, the client identity receipt, and one screenshot per peer per resolution. The runtime logs that fingerprint only after polling ready workers; it never waits on a game-thread future. These are development hardware guardrails rather than shipping frame-time targets; screen-reader integration and input remapping remain later accessibility increments.

Run `Scripts/Verify-WorldMapProfile.ps1` before increasing the map tile density or range. It starts the same current-generator seed-418 host and conflicting-seed late-joining client, opens/pans/zooms/recentres each local map through the existing verification path, and requires both peers to report open latency, aggregate/maximum tile-worker time, aggregate/maximum local map-tick time, ready-tile count, and bounded CPU tile-cache bytes. The client must first receive the server identity; this is a profile-only local UI run with no world mutation, RPC, or new replication.

Shift-click the expanded map to begin a personal pin: type a 1–32-character label, choose `1` Cairn, `2` Lantern, or `3` Thread, and press Enter to place. Click a visible pin to mark it complete, Ctrl-click to hide it, and right-click to remove it. Every marker displays textual style, label, completion, and visibility information in addition to its visual treatment. `Tab` selects pins (including hidden ones), `Enter` toggles completion, `H` toggles visibility, `Delete` removes, and `P` begins placement at the map centre; arrow keys pan, Page Up/Down zoom, and `R` recentres. Gamepad face buttons select, place/toggle, visibility/cancel, and recenter; left trigger removes while D-pad/shoulders pan/zoom. Pins persist locally in an independent version-1 slot keyed by immutable seed and local-player index; only the newest 256 finite, validated annotations are retained, and a mismatched world slot is discarded. They never create an RPC, discovery claim, route, or gameplay instruction. `Kalmala.UI.WorldMap.LocalPresentation` verifies memory serialization/reload, mismatch rejection, and non-colour keyboard pin state actions alongside the existing local rendering checks.

Co-op awareness is off by default and returns to private after reconnecting. With the map open, press `C` or the gamepad Menu button to opt in; only connected, mutually opted-in non-spectators appear, and their markers/pings remain hidden outside your own current or remembered coverage. Middle-click a map point or press `Q`/right-stick click for a map-centre ping. The server accepts only finite locations within 65 m of the sender, limits requests to one every two seconds, relays to eligible nearby opted-in recipients through owner-only inboxes, and expires pings after six seconds. Opting out immediately removes ineligible pings. This shares neither exploration coverage nor personal pins; shared cartography remains unavailable until the M2 construction/persistence prerequisite exists.

### M11 expanded-map background

`WorldMapPanelImage` selects the static, project-owned image at
`/Game/Kalmala/UI/WorldMapPanel.WorldMapPanel`; import its original source at
`Content/Kalmala/UI/Source/WorldMapPanel.png` into `/Game/Kalmala/UI`. The
full-screen background is drawn beneath the existing live map layers. It must
not carry geographic information or replace terrain/fog rendering. Empty or
missing paths retain the geometric panel brush, and high contrast suppresses
the decorative image. Verify the theme fallback assertions, then run
`Scripts/Verify-WorldMap.ps1` for host/client input and rendered map checks at
1024x768, 1280x720, and 2560x1080 plus `Scripts/Verify-WorldMapTiles.ps1` for
matching seed identity and private saved coverage. Inspect captures for map
readability; these tests do not establish packaged asset cooking.

### M11 expanded-map legend and marker filters

The local legend identifies the owning-player facing triangle, personal-pin
diamonds, co-op-player diamonds, and temporary-ping crosses. Filter counts are
the markers already eligible in the current map view: locally shown pins, or
owner-visible co-op entries that also pass the current/remembered personal-fog
check. Counts remain stable when a category is filtered so they continue to
describe what is available if drawing is re-enabled. The owning-player marker
is always shown. Mouse clicks toggle a legend row; `F` or left-stick click
focuses the filter list, arrows/D-pad select a category, Enter/A toggles it,
and Escape/B leaves filter focus. Tab/pad-X pin selection still reaches
personally hidden and locally filtered pins; the selected-pin text reports
when its marker is filtered.

The in-game map keeps this symbol/category legend and concise share/ping action
names but omits keyboard/controller help text. Current binding labels are shown
in Options > Controls only; map focus, selection, pan, zoom, recenter, pin, and
sharing inputs remain available.

Visibility is a transient local widget preference. It suppresses marker
painting only: it does not request map data, change fog/exploration, mutate or
remove pins, alter co-op consent/pings, or add an RPC or save field. After an
isolated editor build, run the full `Automation RunTests Kalmala` queue and
`Scripts/Verify-WorldMap.ps1` at 1024x768, 1280x720, and 2560x1080. Require each
peer to log `World map marker filters: PointerTargets=1 Keyboard=1 Controller=1 PinsPreserved=1`; inspect the host/client captures for legend
layout, checkbox state, text scaling, and keyboard focus. Run
`Scripts/Verify-WorldMapTiles.ps1` to retain current seed/fog and private
coverage checks. The focused `Kalmala.UI.WorldMap.LocalPresentation` test
covers category defaults, pointer target mapping, keyboard/controller focus,
filtered hit-testing, unchanged eligibility counts, and pin preservation.
These editor/rendered checks do not establish physical-controller or packaged
acceptance.

## Biome feature inspection

`Kalmala.World.Biomes.TerrainSelection` verifies current field/classifier agreement, Flora independence, seed variation and coverage. `Kalmala.World.BiomeExpansion.IntegratedScenario` searches eligible inner and outer distances for deterministic discovery candidates. Every launch and test runs the current generator with `-WorldSeed` as its sole world identity setting.

Launch a listen server with `-KalmalaBiomeFeatureInspection` and join a player to log the server-sampled biome-expansion profile, a nearby classifier-seam flag, and a stable but non-materialized discovery candidate. The switch only inspects deterministic inputs from the replicated world identity; it does not spawn, save, reveal, or route toward content, and clients cannot request it.

## Discovery progression verification

After an editor build, run `Kalmala.Gameplay.Discovery.PlayerScopedPersistence`
headlessly with the standard isolated `-UserDir`, `-abslog`,
`-DDC-ForceMemoryCache`, and `-TestExit="Automation Test Queue Empty"` flags.
It checks an identity-scoped discovery record serializes and restores for the
same player/world, rejects another player, rejects a different world seed, and
cannot record the same canonical discovery twice. This focused test does not
replace the later two-peer privacy, range, duplicate-request, or descriptor
variation scenario.

## Shimmering Lakes slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.ShimmeringLakesSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a dry Shimmering Lakes discovery location, verifies adjacent seed-derived water and the wet-shore exposure tradeoff, and confirms that the stable discovery ID reproduces. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active lake key and requires neither water physics nor a boat.

## Elderwood slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.ElderwoodSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a gently sloped, lower-flora Elderwood clearing discovery, verifies the compact-canopy exposure tradeoff and stable ID reproduction, and does not create a trail or reserve a camp. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Elderwood key; field-driven canopy and root presentation are local cosmetic meshes derived from the replicated world identity and patch descriptor.

## Mossy Mire slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.MossyMireSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a gently sloped, relatively dry Mire hummock discovery, verifies increased wet-ground preparation pressure and stable ID reproduction, and does not create a crossing, route, or reserved camp. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Mire key; the server applies the bounded Mire footing drag through the existing replicated travel-speed state, while clients receive only that normal replicated state.

## Freezing Tundra slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.FreezingTundraSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for an exposed, gently rolling Tundra discovery, verifies stronger wind pressure, bounded sparse natural cover, and stable ID reproduction, and does not create an authored ridge, route, camp, or travel gate. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Tundra key; the server applies the existing profile through normal replicated exposure state while enclosed roof/windbreak shelter remains player-built counterplay.

## Thunder Mountains slice verification

After an editor build, run the focused `Kalmala.World.BiomeExpansion.ThunderMountainsSlice` headless automation with `-DDC-ForceMemoryCache`. It searches deterministic spatial keys for a high, steep-but-traversable Mountain overlook discovery, verifies stronger weather-driven wind pressure, bounded cover, and stable ID reproduction, and does not create a designed passage, authored ridge, reserved shelter, or precision gate. Runtime activation remains server-only: it creates at most one normal validated harvest discovery per active Mountain key; the existing server weather and player-built roof/windbreak shelter provide storm and lightning-safe-enclosure counterplay through ordinary replicated exposure state.

## Integrated biome-expansion scenario

After an editor build, run `Scripts/Verify-BiomeExpansion.ps1`. It runs `Kalmala.World.BiomeExpansion.IntegratedScenario`, which compares same-seed host/client terrain, classification, exposure inputs, seams, stable optional-discovery IDs, and roof/windbreak counterplay across Shimmering Lakes, Elderwood, Mossy Mire, Freezing Tundra, and Thunder Mountains. It then runs the existing conflicting-seed two-player camp scenario to verify replicated world identity, weather, exposure state, and recovery from two freely selected camps. This is verification only: it does not activate remote biome content, add paths, or let a client choose a server location or outcome.

## Basic player controls

Restart the editor after building the native modules, then play `L_Prototype`. The third-person character uses the original faceted wanderer silhouette with a tapered mantle, hood, face guard, and walking/airborne poses. Press Space to jump once; hold either Shift key to sprint at 1.5 times the current walking speed. Releasing Shift restores walking speed, including the existing exposure penalty. Sprint has no stamina cost in this prototype.

After an editor build, run `Scripts/Verify-PlayerControls.ps1 -Rendered`. It launches a hidden listen server and client, exercises the bound jump/sprint/release delegates through normal movement prediction, and checks server-observed remote sprint, upward jump, release, landing, matching world identity, and nine collision-free model parts. It retains host/client screenshots and logs in its printed temporary directory and stops its own processes. Physical keyboard input is not simulated by this test. Run the focused `Kalmala.Gameplay.Movement.SprintSavedMoves` headless automation to verify compressed flags, release, move-combination boundaries, and saved-move clearing.

## Boar encounter verification

After an editor build, run `Scripts/Verify-BoarPeer.ps1 -Port <unused-port>`. It starts a seed-418 listen server and a conflicting-seed client with separate temporary user directories. The fixture derives a bounded stable boar descriptor entirely on the server, verifies its normal territorial charge damages the host, rejects the remote player's normal target-free attack sequence, then exercises four server-selected player-combat windups in the ordinary trace range. It requires relevant host/client action, health, and defeat replication, owner-only `BoarMeat`/`BoarHide`, and an identity-matched restart where the same server-derived boar remains absent. It adds no gameplay RPC, client target/damage/reward payload, descriptor replication, or save-schema change.

## Regional generation verification (Phase 7)

### Generation performance regression

After an editor build, run `Kalmala.UI.Minimap.GenerationPerformance`. It measures moving 129x129 rasters at 25/50/100 m zoom radii, checks repeated hashes and compares batched output with independent terrain/water/biome queries for seeds 418 and 419. Timings exclude engine startup.

Sampling shares per-position warp and region calculations and reuses collision vertices within each raster. Run regional/water/island/minimap regressions and `Scripts/Verify-Minimap.ps1` after shared sampling changes.

Build `KalmalaEditor Win64 Development -WaitMutex -NoHotReload -MaxParallelActions=4`, then run `Scripts/Verify-RegionalGeneration.ps1`. The runner explicitly rejects failed automation results even when Unreal returns exit code zero, generates two seed-418 previews and one seed-419 preview, hashes every PPM for reproduction, checks biome/hydrology/height variation, and launches a current-generator seed-418 host with a conflicting-seed client. It uses a unique temporary user/cache/output directory and retains all logs and images. `-SkipPeers` is available for local analysis only; full Phase 7 acceptance requires the peer run.

`Kalmala.World.Regional.Integrated` samples a 16 km square for seeds 418/419, verifies seven-biome coverage, boundary density below 0.18, fewer than 2% tiny isolated components relative to sample count, continuous weights/heights, Flora independence, deterministic spline rebuild and water patch edges. This broad 100 m lattice can undersample narrow regions.

`Scripts/Verify-Minimap.ps1` compares 81 host/client positions across a 20 km square, including biome weights, terrain, water and river signals. It creates no RPC or saved inspection data.

## Source-control rules

- Commit `Config/`, `Source/`, `.uproject`, and `.uasset`/`.umap` content assets.
- Never commit generated `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/` folders.
- Unreal assets are marked as binary in `.gitattributes`; resolve asset conflicts in the editor, not through text merging.

## Fuelled hearth and crafting verification

After the editor build, run `Kalmala.Gameplay.Crafting`, `Kalmala.Gameplay.Inventory` and `Kalmala.Gameplay.Campfire` with the headless temporary-user/log flags above, then `Scripts/Verify-Crafting.ps1 -Rendered`. Rendered runs need access to Unreal's shader working directory as well as the build-tool cache. The runner requires exact inventory conservation for both players, rejection of real forged/malformed RPCs and insufficient placement, two matching dry/rain-extinguished fire states, paid placement and fuel/protection gates, local menu input restoration, and retained host/client screenshots. Also run `Scripts/Verify-InventoryReconnect.ps1` and `Scripts/Verify-CampChoices.ps1`. Full contract and test limits: `10-campfire-and-crafting.md`.

## Master-map generation verification

For routine map tuning, use the fast PNG exporter below. The older full visualization commandlet remains useful for height, water, weights and spline diagnostics.

Build the editor, then run headless automation with the temporary user/log flags above and:
`Automation RunTests Kalmala.World.Regional.MasterMap+Kalmala.World.Regional.FiniteWorld+Kalmala.World.Regional.Integrated+Kalmala.World.Water+Kalmala.UI.Minimap.GenerationPerformance`.
Check every requested result reports Success; engine exit code alone is insufficient.

The master-map test covers independent master seed variation, repeatable crop/rotation, containment, coastline agreement, biome coverage, exact distance boundaries, zero forbidden weights, continuous gates and dry central Meadows starts. The exporter verification additionally gives Lakes and Mire identical ranges and requires identical wetland weights at every sampled pixel.

Run `Scripts/Verify-Minimap.ps1` for a conflicting-seed client and matching 81-position host/client terrain, biome-weight, water and hydrology fingerprints across a 20 km square (inside the playable circle). This proves replicated identity and sampler agreement across inner and outer biome distances; it does not establish a full-world human traversal playtest.

The visualization commandlet accepts `-Revision=7 -Seed=418 -Size=256 -Extent=3200000 -SkipSplineOverlay -Output=<temporary-directory>`. Use `-NoZenAutoLaunch -DDC=NoZenLocalFallback -LocalDataCachePath=<temporary-DDC>` for commandlets so a restricted disk-cache fallback error does not force failure. It writes the existing field/biome/weight/height images plus:

- `MasterLandWater.ppm`: full independently seeded atlas, with the selected circular crop outlined.
- `LandWaterCrop.ppm`: rotated base crop, masked to the playable circle.
- `Tuning.txt`: master seed, atlas scale, crop centre, rotation and distance limits.

The main biome preview also masks the exterior. `-SkipSplineOverlay` skips only the costly whole-area line-overlay enumeration; each pixel still samples actual hydrology and terrain. At whole-world resolution, small lakes and channels may be subpixel. Render the same seed twice and compare every PPM hash, then render seed 419 and require crop, biome and height variation. No preview image becomes authoritative world data. Restart an open editor or watcher after rebuilding native modules.

## Fast world-map PNG export

Run from the repository root:

```powershell
# Export once using the compiled generator:
./Scripts/Export-WorldMaps.ps1

# Recommended for tuning: keep one headless exporter warm.
./Scripts/Export-WorldMaps.ps1 -Watch

# Only after C++ source changes, or if this commandlet is not built yet:
./Scripts/Export-WorldMaps.ps1 -Build -Watch
```

Edit and save `Scripts/WorldMapPreview.params` while watching. Each stable edit regenerates these ordinary PNG files under the ignored `.cache/WorldMaps/` directory:

| File | Contents |
| --- | --- |
| `MasterLandWater.png` | The full 128 km master land/water atlas, without a crop outline. |
| `LandWaterCrop.png` | The game-seeded rotated crop at the current 16 km playable radius. |
| `Biomes.png` | The same crop classified by the production biome rules, including lake basins and distance eligibility. |
| `LastRender.txt` | Requested and effective parameters, crop transform, palette and timings from the last successful export. |

Use `-Parameters <path>` for an alternate parameter file and `-Output <directory>` for a separate experiment. One exporter should own each output directory. Ctrl+C stops watch mode and its owned child process. Logs and headless engine cache/user data go to a unique temporary directory; the script prints its location. The script does not load a gameplay map, spawn terrain/population, run shaders, or join a session.

The first export still pays headless Unreal startup time. Saving parameters in watch mode avoids that startup entirely. Measured on 2026-09-12: the default 512×512 three-PNG export took 0.326 s sampling / 0.350 s total, excluding startup; the first measured process took 30.27 s including startup while other verification processes were running. These are local measurements, not a hardware-independent guarantee. Use a smaller `Size` while iterating, then increase it for export.

### Preview parameters

The file uses one `Name=value` per line, with optional blank lines and full-line `#` comments. Distances are kilometres; seeds are unsigned decimal integers. Omitted settings revert to production defaults on every reload. Unknown/duplicate keys, non-finite values and out-of-range settings are rejected. Invalid edits leave previous PNGs intact and print a warning; fix and save the file to resume.

| Parameter | Default | Meaning / allowed values |
| --- | --- | --- |
| `WorldSeed` | 418 | Game seed selecting crop, rotation and regional fields; unsigned 64-bit integer. |
| `Size` | 512 | Square PNG dimension, 64–2048 pixels. |
| `MasterSeed` | Production master seed | Independent atlas seed; omit to follow the compiled default. |
| `MasterWavelengthKm` | 3 | Main land/water wavelength, 0.1–32 km. |
| `LandThreshold` | 0 | -0.5–0.5; increasing it creates less land. |
| `BiomeScaleKm` | 0.6 | Regional scale, 0.1–16 km; larger values make broader regions. |
| `WarpStrengthKm` | 0.08 | Regional edge displacement, 0–2 km. |
| `StarterRadiusKm` | 0.35 | Inclusive Meadows/Ocean-only radius. |
| `ElderwoodMinimumKm` | 0.75 | Forest eligibility minimum. |
| `LakesMinimumKm` / `LakesMaximumKm` | 0.35 / 3 | Starter wetland distance range. |
| `MireMinimumKm` / `MireMaximumKm` | 3 / 16 | Outer wetland distance range. |
| `WetlandHumidityMinimum` / `WetlandHumidityFull` | 0.48 / 0.65 | Shared humidity ramp. |
| `WetlandElevationFull` / `WetlandElevationMaximum` | 0.43 / 0.58 | Shared lowland suitability ramp. |
| `WetlandTemperatureMinimum` / `WetlandTemperatureFull` | 0.18 / 0.32 | Shared temperature ramp. |
| `TundraMinimumKm` | 4 | Tundra eligibility minimum. |
| `MeadowsMaximumKm` | 4 | Meadows cutoff; must exceed the starter radius and be at most 16 km. |
| `EligibilityBlendKm` | 0.05 | Smooth eligibility ramp; 0.001 km through the Meadows maximum. |

The starter, Elderwood, Mire and Tundra radii must be nonnegative, ordered and at most 16 km. Lakes follow the starter exclusion. Atlas dimensions and playable radius remain fixed to the current contract. Broad land thresholds can leave little or no land; such an experiment is a visual preview, not validation of a playable spawn.

These parameters are **preview-only** and restricted to the headless commandlet sampling thread. Each render restores defaults and invalidates derived caches. Promote selected values into compiled production constants and rebuild all peers. There is no generator compatibility selector; the current build always defines the layout.

The fast path shares the production regional classifier and wetland suitability/distance rules. It skips bowl shaping, rivers and other height work because these do not change labels. PNGs display biome identity, not final water depth. Use `RenderWorldGenerationVisualization` for hydrology and height.

### Export verification

After building, run `Scripts/Verify-WorldMapExport.ps1`. In one warm headless process it verifies PNG signatures, every visible fast/full biome pixel at 64×64, changed master/region tuning, restoration of default output hashes, and preservation of the last good PNGs after malformed input. It keeps its parameter edits and outputs in a temporary directory; the user's parameter file is untouched.

Run `Kalmala.World.Regional`, `Kalmala.UI.Minimap.GenerationPerformance`, `Scripts/Verify-WorldMapExport.ps1` and `Scripts/Verify-Minimap.ps1` after shared sampling changes. The exporter has no gameplay authority or persistence path.
## M3 wetness and rain override (2026-09-14)

`Kalmala.Gameplay.Status.WetModifiers` adds a spawned-pawn regression for the tuned 0.92 walking/sprint/swim speed, duplicate-status non-stacking, expiry recovery, and 1.15 stamina-cost calculation. It is included by the `Kalmala.Gameplay.Status.Wet` prefix. The M5 balance pass keeps the 120-second duration, 10-second rain trigger, and campfire recovery unchanged while reducing the travel/stamina tax. This test does not prove live network agreement or stamina drain; there is no stamina consumer yet. Legacy exposure speed assertions are historical and no longer represent the M3 movement contract.

This section supersedes earlier references to continuous player wetness, warmth-derived wetness penalties, fuel wetness extinguishing, or rain damage below. `Wet` is only a server-owned player debuff with a reusable parameter definition: default maximum duration 120 seconds, unroofed-rain trigger 10 uninterrupted seconds, movement multiplier 0.92, and stamina-use multiplier 1.15. Standing in server-confirmed water applies it immediately. Rain applies it only when the server finds no accepted roof above the pawn. Reapplication cannot exceed 120 seconds; a nearby lit campfire with nonzero authoritative heat removes it. Surface moisture remains a grid material/fire input and is never a second player wetness system. The player-facing result is a replicated status-container entry with a finite remaining duration; it has no client mutation path.

Roofs are rain-immune. Unroofed floors, walls, workbenches, and storage take slow server-owned rain wear, clamped at 50% health; an accepted overhead roof prevents that wear. A fuelled campfire is normal in dry weather. Rain without a roof makes it `Smouldering`, with zero heat; roof protection automatically returns it to `Lit`. Clients submit no wetness, duration, roof, health, weather, fire, or relight state.

After an editor build, run `Kalmala.Gameplay.InteractionGrid.SurfaceMoisture+Kalmala.Gameplay.Status.Wet` headlessly with the standard temporary `-UserDir`, `-abslog`, `-DDC-ForceMemoryCache`, and `-TestExit="Automation Test Queue Empty"` flags. The grid test verifies the 200 cm floor-key transform, deterministic cell centre, bounded rain/drying moisture, malformed-input recovery, and the explicit 1,024-cell server budget. The Wet test verifies bounded status creation, server-time expiry, capped refresh, and malformed/duplicate entry rejection. These focused tests do not claim grid replication/persistence, Wet movement or stamina modifiers, campfire removal, fire spread, or construction response.

### M3 Wet stamina verification

The sprint stamina implementation supersedes earlier statements that sprint has no stamina cost. Run the headless prefix Kalmala.Gameplay.Status.Wet plus Kalmala.Gameplay.Movement.SprintSavedMoves after building. WetStamina checks actual dry/Wet consumption, zero/max clamping, exhaustion gating, recovery hysteresis, stationary intent, and malformed elapsed time. Run Scripts/Verify-PlayerControls.ps1 -WetStamina on an unused port for normal bound sprint/jump/release in a listen-server session with a conflicting-seed client. The server fixture maintains Wet; both server pawns and the remote owner must report stamina below 99 and matching 0.92-adjusted sprint speed. The remote owner also attempts the local stamina update and must observe no mutation. This fixture does not test natural rain/water triggers, expiry, or latency at exhaustion. The original controls scenario remains available without the switch.

The Kalmala.Gameplay.Status.Wet prefix now includes WetCampfire. It spawns and lights an actual paid-initialized hearth and checks missing/unlit/zero-heat/radius-edge/exhausted-source rejection, client-role rejection without mutation, successful nearby heat removal, default movement/stamina restoration, idempotence, reapplication, and ordinary expiry. It exercises the server component seam with real actors; live replication of campfire removal and the combined rain/roof/smoulder scenario remain part of the later M3 acceptance task.

### Construction rain-wear verification

After the editor build, run Kalmala.Gameplay.Construction headlessly with the usual temporary UserDir/log flags. RainWear uses actual construction collision to check all four eligible kits, proportional rain, clamped intensity, 50-health floor, roof immunity, roof reach, untagged first-hit rejection, protection without healing, malformed inputs, unknown kits and client-role mutation rejection. Existing shelter and save tests remain included in that prefix. This is actor/physics and authority coverage; live health replication and the full rain/roof/hearth loop remain in the later M3 acceptance scenario.

### Three-state hearth verification

Run Kalmala.Gameplay.Hearth+Kalmala.Gameplay.Status.Wet headlessly after building. Hearth.RainState exercises real paid-initialized actors: no self-lighting, dry/windy Lit, threshold Smouldering, zero light/heat, normal fuel consumption, client mutation rejection, actual roof collision and loss of protection, dry reignition, invalid input rejection, fuel exhaustion and Wet removal only after heat returns. Scripts/Verify-Crafting.ps1 now requires enum State=1 and State=2 on both server and client in its existing paid crafting scenario. Earlier rain-extinguishing descriptions are superseded by Smouldering; later combined M3 acceptance still owns the complete live roof/rain loop.

## M3 rain authority regression

After a forced editor build, run `Kalmala.Gameplay.Hearth+Kalmala.Gameplay.Construction+Kalmala.Gameplay.Status.Wet+Kalmala.Gameplay.Crafting.NetworkContract+Kalmala.Gameplay.Storage` with the temporary user/log and headless flags above. Hearth.AuthorityContract initializes runtime replication metadata, checks no server RPC on state owners, verifies ordinary lifetime replication for health/hearth/protection fields, and guards the existing three-field construction schema. The other prefixes cover client-role mutation rejection, real roof collision, malformed weather, status effects and bounded identity-scoped save serialization. This focused audit does not replace combined live M3 acceptance.

### Construction feedback rendering

After building, run Scripts/Verify-Crafting.ps1 -Rendered. The paid floor fixture now advances rain wear through the authoritative actor seam to the 50-health floor. Both peer captures require the local nearby-construction text to report Health: 50.0 / 100 and the rain-wear limit. Inspect host.png and client.png for readable status and recovery cues. This covers floor health replication and rendered feedback, not all construction kits, camera occlusion cases, aspect ratios or the complete natural-weather M3 scenario.


## Local tutorial prompt smoke test

Run `Scripts/Verify-TutorialRouteFree.ps1` for a no-build source audit of the retained route-free prompt design and its disabled runtime presenter. It checks that the bottom gameplay card cannot mount, while the historical arrival/context rules still avoid routes, quest flow, hidden-actor scans, RPCs, and gameplay-save state. `Verify-PlayerControls.ps1` runs this audit before its fresh-pawn host/client scenario; `Verify-InventoryPanelRemoval.ps1` includes it in the reconnect/HUD-absence preflight.

After a forced editor build, run `Scripts/Verify-PlayerControls.ps1 -Rendered -Port <unused-port>` from the same isolated project copy. Its fresh-pawn host/client path verifies local jump/sprint input and server-observed remote movement, with the tutorial absence audit as a preflight. Inspect both 1280×720 captures for the absence of the bottom card while checking essential status/action notifications remain visible. The fixture uses a development-only movement-test flag. It does not simulate physical controller input or replace the final packaged 20–30 minute no-developer-tools acceptance in `docs/12-vertical-slice-runbook.md`.

### M3 rain vertical-slice verification

After a forced editor build, run `Scripts/Verify-RainVerticalSlice.ps1` on an unused port. Its development-only listen-server fixture activates the deterministic seed-418 coverage domain, locates water using the authoritative ocean/lake samplers, and freezes two ordinary peer pawns while it drives the production weather, status, roof-trace, rain-wear, and hearth update paths. The client joins with seed 999 and must receive seed 418. The server then requires immediate water Wet, heat-based removal, Highly Active rain Wet by 8.5 seconds (7.5-second trigger), an exposed floor at the 50-health cap, rain-immune roofed floor and roof, a Smouldering fire, and roof-driven Lit recovery; the remote owner independently reads the replicated final construction IDs, hearth state, roof state, and no-Wet status. The fixture adds no gameplay RPC, save record, client-supplied mutation, or normal-play grant, and uses separate temporary user directories.

### M7 weather hazards and stamina recovery

`Kalmala.World.WeatherCycle.Determinism` verifies seeded fog, normalized
storm intensity, and the calm/active/highly-active thresholds.
`Kalmala.Gameplay.Exposure.RecoverableTravelPenalty` covers bounded
heat/cold summaries plus the reversible low-warmth stamina-recovery curve.
Kalmala.Gameplay.Exposure.WeatherHazardResponse checks that only Highly Active
weather shortens the normal 10-second unroofed-rain Wet trigger to 7.5 seconds. It also checks the shared 0.65 storm threshold, the bounded 25% fuel-wetting increase, and roof/windbreak mitigation; Kalmala.Gameplay.Hearth.RainState exercises the production roof trace against storm wetting.
`Kalmala.Gameplay.Exposure.ColdStaminaRecoveryAuthority` checks the live server
stamina path at cold and recovered warmth, and rejects mutation from a
simulated client copy. After the forced editor build, run these tests with an
isolated temporary user directory. Then run `Scripts/Verify-CampChoices.ps1`
to compare replicated weather tier/fog and both players' heat/cold exposure
snapshots against server logs. `Kalmala.UI.WeatherActivity.LocalPresentation`
verifies that each replicated tier maps to distinct shape-plus-text feedback
and that its owner-local badge remains anchored below the minimap; it does not
replace rendered viewport inspection.

The focused command is:

    & 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7WeatherUser' -abslog='C:\temp\KalmalaM7Weather.log' -ExecCmds="Automation RunTests Kalmala.World.WeatherCycle.Determinism+Kalmala.Gameplay.Exposure.RecoverableTravelPenalty+Kalmala.Gameplay.Exposure.WeatherHazardResponse+Kalmala.Gameplay.Exposure.ColdStaminaRecoveryAuthority+Kalmala.Gameplay.Hearth.WeatherMutation+Kalmala.Gameplay.Hearth.RainState+Kalmala.UI.WeatherActivity.LocalPresentation; Quit" -TestExit="Automation Test Queue Empty"
    Scripts\Verify-CampChoices.ps1 -Port 17842
    Scripts\Verify-RainVerticalSlice.ps1 -Port 18119

### M7 local survival status presentation

After the editor build, run `Kalmala.UI.SurvivalStatus.LocalPresentation` with
the standard `-unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache`,
isolated `-UserDir`, unique `-abslog`, and
`-TestExit="Automation Test Queue Empty"` flags. The test covers shape-plus-
text labels, server-derived Wet/food timers, weather interval/intensity,
heat/cold context, active support expiry/magnitude, recovery/source guidance,
the empty state, and the local lower-left viewport slot. If
`UnrealEditor-Cmd.exe` exits during its all-platform SDK preflight, run the same
arguments with `UnrealEditor.exe` and wait for process completion; the editor
automation log must show `Result={Success}`. This is a presentation-only
contract check and does not replace rendered readability review.
### M7 first food transaction

The original M7 acceptance used roasted field-meat IDs. That content was
superseded by the current schema-4 catalogue. For current recipe IDs,
ingredients, stations, and outputs, use the catalogue contract in
`docs/28-m9-camp-equipment-recipes.md` and the focused current-catalogue tests
listed below.

The Cooking Rack, Cauldron, and Frying Pan use the existing paid construction path and schema-1 construction record; none adds a save field, private inventory, or persistent fuel authority. The server resolves each required station and derives heat needs from its cooking identity. A food recipe requires a usable Lit hearth with finite positive heat within 250 cm of both player and station. Looking at a rack, cauldron, frying pan, or Workbench shows a remappable Interact prompt; the default E interaction is rerun by the server, which replicates the selected station identity only to its owner. The shared themed shell opens the Cooking Rack, Cauldron, and Frying Pan Cook sections and the Workbench Craft section. Workbench Craft filters to recipes requiring that station or producing its matching Tool Rack attachment, plus the Bronze Axe tool operation; it reports material requirements, effective level, and current Tool Rack state. The exact accepted actor and stable construction ID bind the section, and the shell closes if that actor is destroyed, leaves range, or the owning pawn changes. The older CraftMenu remains available. The filter and shell do not authorize crafting: the server independently validates each recipe/tool request against the visible same-world station, required levels, private inventory, and atomic material exchange. `Kalmala.Gameplay.Food.CookingStationHeat` asserts the exact accepted Cooking Rack context and Workbench interaction identity; the M12 `Verify-Crafting.ps1` presentation check also audits Workbench Craft's limited outputs and Bronze Axe prerequisites on both peers. Each recipe exchange consumes only the listed ingredients; the hearth burns its fuel by elapsed server time at the ordinary one-fuel-second-per-second rate. A serving batch creates no raw-fuel item debit or hidden timer. The owner food request carries only the allowlisted food item ID. The server checks its private inventory and one-meal slot before consuming an accepted meal and publishing the existing 120-second server status that multiplies stamina use by 0.90. Duplicate use and alternate-food replacement are rejected without consuming food or changing the active timer; server status time expires the effect and restores ordinary stamina costs. `HearthBroth` remains an item without a production recipe; food recipes and outputs are enumerated in `docs/28-m9-camp-equipment-recipes.md`. Food remains optional, with no hunger drain or travel requirement. Food and the effect remain transient; no save schema changed.

Each successful prepared-food recipe transaction awards one fixed 10 Cooking experience through the existing server-owned skill component, regardless of its serving batch. The award happens only after the atomic private-pack exchange succeeds. Rejected stations, heat, quantities, inputs, or output capacity award no experience; client-side calls cannot reach the award path. The award is transient. `Kalmala.Gameplay.Food.CookingStationHeat` checks the station-local heat rule, no recipe fuel debit, and time-based fire burn; `Kalmala.Gameplay.Crafting.Transactions` checks recipe batches and raw-fuel selection for hearth refuelling.

The recipes do not require a Cooking level, and accepted cooking awards remain post-transaction.

The `Kalmala.Gameplay.Inventory.Catalogue` check above owns the six current food
recipe IDs, outputs, stations, batch/output bounds, and retired aliases. After
the forced editor build, run the six focused cooking, meal-use, crafting,
network, and construction contracts:
`Kalmala.Gameplay.Food.CookingStationHeat+Kalmala.Gameplay.Status.SteadyMeal+Kalmala.Gameplay.Crafting.Transactions+Kalmala.Gameplay.Crafting.NetworkContract+Kalmala.Gameplay.Construction.LocalPreview+Kalmala.Gameplay.Construction.SaveContract`.
CookingStationHeat covers the live
rack, cauldron, and pan transactions including stew, local hearth heat, and
fuel behavior. SteadyMeal checks inventory consumption, duplicate/client
rejection, and the active timer. NetworkContract checks that food use sends
only the item identity. The crafting transaction test checks raw fuel
selection for hearth refuelling separately from recipe ingredient exchanges:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir='C:\temp\KalmalaM7FoodUser' -abslog='C:\temp\KalmalaM7Food.log' -ExecCmds="Automation RunTests Kalmala.Gameplay.Food.CookingStationHeat+Kalmala.Gameplay.Status.SteadyMeal+Kalmala.Gameplay.Crafting.Transactions+Kalmala.Gameplay.Crafting.NetworkContract+Kalmala.Gameplay.Construction.LocalPreview+Kalmala.Gameplay.Construction.SaveContract; Quit" -TestExit="Automation Test Queue Empty"
```

## Windows Development package smoke

Create a disposable mirror of the current workspace, excluding .git,
Binaries, Intermediate, Saved, and DerivedDataCache. Put the archive and fresh
smoke profile outside that mirror. With UE 5.8.2, package the project using
both -pak and -iostore so cooked assets are staged in local containers; omitting
them can produce an archive that points at the temporary Zen store without
including local game content.

    & "C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$projectMirror/Kalmala.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -package -archive -pak -iostore "-archivedirectory=$archiveRoot/Archive" -unattended -utf8output

Smoke-launch Archive/Windows/Kalmala.exe from its containing directory with a
new empty UserDir, null rendering, and a unique absolute log path. Keep the
process alive for at least 20 seconds and require the log to contain
“Game Engine Initialized” and “Load map complete /Game/Kalmala/Maps/Prototype/L_Prototype”.
The wrapper starts the actual game under Kalmala/Binaries/Win64; check that
process before stopping the smoke run. Null rendering verifies startup and map
loading only; it does not cover the player-facing co-op walkthrough.

## Native Windows control for packaged player validation

Read the installed `computer-use:computer-use` skill and its guidance before
driving the game. Use the deferred `mcp__node_repl__js` tool and initialize its
persistent JavaScript session with:

```javascript
if (!globalThis.sky) {
  const { sky } = await import("@oai/sky");
  globalThis.sky = sky;
}
globalThis.nativeWindows = await sky.list_windows();
nodeRepl.write(JSON.stringify(nativeWindows, null, 2));
```

Select exactly one returned game window per peer, carrying its returned `id`
and `app` into `get_window`. Activate it, observe `get_window_state`, inspect
the screenshot, then perform one normal input and refresh. For example,
`sky.press_key({ window: gameState.window, key: "Escape" })` opens Settings.
Use only the supported API and current observations; do not invent handles or
replace normal player input with developer commands or gameplay fixtures.

The packaged root `Kalmala.exe` is a bootstrap wrapper. Its actual window
belongs to `Kalmala/Binaries/Win64/Kalmala.exe`. A native launch of the wrapper
can report no targetable window before the child appears; refresh
`sky.list_windows()` after startup and select the returned child window.
A hidden/minimized shell launch may also need restoration before capture.
If user input invalidates activation, reobserve before retrying activation.
Never act on a screenshot showing another application.

`cua.getState()` belongs to a separate browser-oriented control surface in
this runtime. Its `apps: []`, and absent `cua.computer` methods, do not prove
that native Windows control is unavailable. Check `@oai/sky` before declaring
that blocker. On 2026-10-02 native discovery, game screenshot capture, Escape
opening Settings, and clicking Quit were verified against the existing M10
Development archive. This is a control preflight only: full fresh-profile
co-op, progression, reconnect, and long-distance travel are still required.
The runbook's native-surface skip conditions and dedicated-server capability
restriction remain unchanged.

### Windowed peers on the native-control desktop (2026-10-02)

For this environment's shell launch path, start the packaged executable outside
the shell sandbox using the approved `require_escalated` execution path.
Sandboxed launches had real window handles but were absent from native
discovery; outside-sandbox launches exposed both peers to `sky.list_windows()`.
Windowed mode alone did not fix discovery. Never construct native window
objects from shell-reported handles.

Launch `Kalmala/Binaries/Win64/Kalmala.exe` with
`/Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=19864 -windowed
-ResX=960 -ResY=540 -WinX=20 -WinY=40 -WorldSeed=418`, a fresh host
`-UserDir`, and an absolute log. Launch the client with `127.0.0.1:19864
-windowed -ResX=960 -ResY=540 -WinX=1010 -WinY=40 -WorldSeed=999`, a
separate fresh `-UserDir`, and its own log. Pick an unused port for later runs.
These are startup/session arguments, not developer gameplay fixtures.
Select each peer from returned native windows as above.

This path confirmed joining and shared terrain/two-player presentation on one
PC. Host B opened and closed crafting. It has not passed the walkthrough:
W, Shift+W, and Space taps produced no observable movement, including after
viewport focus. The documented `sky.press_key` takes a key/chord and window;
it exposes no duration or key-down/key-up API. Sustained walking, sprinting,
and vessel steering need a supported input path or human input. Joining logs,
screenshots, and working menus do not pass progression, reconnect, or travel.

## M8 sea-discovery placement and claim contract

After the forced editor build, run
`Kalmala.Gameplay.OceanTravel.DiscoveryCatalogue` with the isolated headless
automation flags above. The contract checks the three canonical optional
discovery definitions, reward mappings against the server item catalogue,
stable sparse IDs derived from discovery kind and spatial key, and rejection
of forged definitions or unknown discovery IDs.

Also run `Kalmala.Gameplay.OceanTravel.DiscoveryClaimContract`. It scans the
seed-418 deterministic layout for all three optional kinds, checks qualifying
100 cm master-ocean water, verifies the server rejects altered IDs and
positions, and round-trips a player-scoped M7 `DiscoveryClaimed` delta to
confirm replay rejection after load. It checks catalogue reward construction
without mutating a live inventory. These focused checks and the editor build do
not replace a live client interaction trace, owner feedback inspection, or
rendered presentation review in the later two-player journey acceptance.

## M8 combined-voyage late-join and restart verification

After a forced `KalmalaEditor Win64 Development` build, run
`Scripts/Verify-OceanSkiffIntegratedReconnectJourney.ps1 -Port 18181`.
The isolated seed-418 host and returning helm owner sail the existing 2.4 km
development route through the server-selected crosswind and calm intervals. A
third peer joins underway; the host verifies one vessel, the original seats,
no late-player attachment, and no private reward leak. The two original owners
must each retain their accepted catalogue reward through the moored stop.

The runner then stops and restarts the host using the same save profile and
reconnects the returning owner with the same development-only authenticated
identity. It checks the exact moored stop, one restored vessel, both saved
seats, and the resumed client replica. The server briefly moves the test owner
to the discovery's validated interaction range to attempt a duplicate claim,
then restores the saved helm position; the ledger must report `AlreadyFound`,
with no second inventory grant and the helm seat intact. A new late peer joins
after restart and must remain unattached without private reward state.

Inventory is intentionally transient under the current M8 save contract, so
the restart check proves the claim ledger prevents a duplicate reward rather
than restoring the original inventory quantity. This fixture uses stable
development test-provider identities, not an external authentication
provider. It complements the separate safe-disembark evidence and does not
claim player-facing, rendered, or physical-input acceptance. Dry-shore-specific
acceptance was waived on 2026-09-26; the integrated journey accepts a safe
stopped disembark through the qualifying deep-water fallback. It adds no
production RPC or saved-data field.

## M9 tool stations and progression

The Forge is a paid construction kit handled by the existing placement and
schema-1 construction-save paths. The Workbench and Forge each derive base
level 1 from their server-accepted kit identity. Paid Workbench tool-rack and
Forge-anvil kits are crafted at the matching visible same-world station within
250 cm, then placed within 125 cm of a compatible station by server-derived
placement. The server derives effective level from initialized matching
construction actors and caps it at level 2; duplicate, distant, wrong-family,
and unusable-station attachments are rejected. Tool upgrade requests contain
only a tool ID, and placement requests contain only a kit ID. Attachments and
their level bonus remain session-only until the M9 save migration contract is
approved; they do not extend the current construction schema.

After the forced editor build, run these focused automations with isolated
user and log directories: Kalmala.Gameplay.M9.ToolStationProgression,
Kalmala.Gameplay.M9.ToolProgressionCatalogue,
Kalmala.Gameplay.Crafting.Transactions,
Kalmala.Gameplay.Crafting.NetworkContract,
Kalmala.Gameplay.Construction.LocalPreview, and
Kalmala.Gameplay.Construction.SaveContract.

ToolStationProgression covers Bronze Axe creation, exact station family and
level checks, Iron Axe prerequisite/replacement, full condition, paid costs,
and unchanged candidates after rejection; it also verifies the RPC accepts only
the target tool ID. The catalogue check covers the paid Forge, Workbench rack,
and Forge anvil recipes; base/effective station levels; compatible families;
placement range; duplicate rejection; and the kit-only placement RPC payload.

### M9 free selected-tool repair

The owner-only crafting panel offers one selected-tool action for each of the
three starting tools and both axe tiers. The owning client sends only the tool
ID. The server reads the carried record and requires a visible same-world
Workbench or Forge within 250 cm; damaged and zero-condition tools restore to
their authored maximum without inventory changes or Crafting experience.
Material-paid repair and broken-tool replacement recipes are retired.

`Kalmala.Gameplay.Tools.LifecycleContract` checks free repair for all five
tool definitions, including zero condition and tool-level preservation, plus
authority, station, unknown-tool, full, and invalid-condition rejection.
`Kalmala.Gameplay.Crafting.Transactions` confirms the old replacement recipes
are absent. `Scripts/Verify-InventoryReconnect.ps1` checks live Workbench
range/rejection, repair with an empty pack, unchanged Crafting experience,
retired replacement requests, and owner-only condition across two visits.
After the forced editor build, run
`Kalmala.Gameplay.M9.GrindingStoneRepairAll` with an isolated user and log
directory. It checks the paid Workbench recipe, generic construction save and
placement allowlists, repair of every damaged/broken carried tool, unchanged
full tools and levels, and whole-action rejection for client authority,
missing Grinding Stone validation, malformed records, duplicates, and an
oversized list. A repeated Repair All against already repaired tools is an
accepted no-op whose candidate preserves every tool ID, level, and condition.
In-world use is server-only: the remappable Interact action (default E) targets
a visible accepted Grinding Stone within 250 cm; the server reads and repairs
the owner's current carried list, with no client-supplied IDs/conditions, cost,
or Crafting XP.
The accepted transaction sends a parameterless cosmetic multicast for the
1.2-second procedural sharpening pose; no gameplay or saved state is carried.
Run `Kalmala.Gameplay.Tools.SharpeningPresentation` with the same isolated
editor automation setup to check its bounded three-stroke pose and return to
the current gait. This pose test does not replace rendered in-world review.
`Kalmala.UI.InteractionPrompt.Presentation` checks the action-only “Repair all”
prompt, absence of key legends, and stable text when Interact is remapped.
`Scripts/Verify-InteractionPrompt.ps1 -Rendered` captures the Grinding Stone
prompt state on both host and client. In the M12 `Scripts/Verify-Crafting.ps1`
fixture, the server sends one Interact request at a fresh visible Grinding
Stone and requires exactly one accepted action result, unchanged full tools and
pack, unchanged station/menu context, and the concise already-full feedback.
These interaction and rendering fixtures prepare part of the later M12
host/client acceptance; they do not replace a live damaged-tool repair review.

### M9 owner-local tool progression feedback

The former combined M9 Build-panel text probe was retired when M12 removed the
legacy progression panel from standalone Build. Tool progression remains in
its Workbench Craft and Forge Upgrade contexts. `Scripts/Verify-Crafting.ps1`
requires both peer markers for the Workbench Bronze Axe prerequisites and
passive Tool Rack status, plus Forge Upgrade requirements/comparison and the
passive Anvil status. These checks inspect the current station-scoped
presentation; they add no server request, gameplay authority, replicated
field, or save schema.

### M9 Construction Hammer and direct builds

The server creates a level-one carried Construction Hammer for each new
character. Its owner can open the local build and craft menu with the remappable
`CraftMenu` binding; other clients cannot supply hammer state or placement
costs. Hearth rings consume 5 Stone, 3 Wood, and one raw Wood, Lightwood,
Densewood, or Coal directly;
floors, walls, and roofs are paid directly with Wood and Fibre. Their internal
placement identities remain stable, and floor/wall/roof schema-one camp saves
keep restoring the same actors. The selected raw-fuel item starts the hearth
with its existing 60-second payment; crafting no longer creates a CampfireKit
item.

After a forced editor build, run
`Kalmala.Gameplay.Tools.CarriedToolInventoryContract`,
`Kalmala.Gameplay.Crafting.Transactions+Kalmala.Gameplay.Crafting.NetworkContract`,
`Kalmala.Gameplay.Construction.LocalPreview+Kalmala.Gameplay.Construction.SaveContract+Kalmala.Gameplay.Construction.ShelterPieces+Kalmala.Gameplay.Construction.ShelterSampling`,
`Kalmala.Gameplay.M9.RaisedStorage`,
`Kalmala.Gameplay.Food.CookingStationHeat`, and
`Kalmala.Gameplay.Storage.SaveContract+Kalmala.Gameplay.Storage.Transfers+Kalmala.Gameplay.Storage.NetworkContract`
with isolated `-UserDir`, `-abslog`, `-DDC-ForceMemoryCache`, and
`-TestExit="Automation Test Queue Empty"` arguments. Require every requested
test result to report success. The crafting test checks hearth and structural
raw costs, mixed raw-fuel payment, insufficient-material rejection, and
refusal to craft direct-build items;
the network test checks that construction requests carry only the buildable
identity.

Then run `Scripts/Verify-Crafting.ps1 -Port <unused-port>`,
`Scripts/Verify-ConstructionPersistence.ps1 -Port <unused-port>`, and the
single-session `Scripts/Verify-PersistedCampHearth.ps1 -Port <unused-port>`
fixture described in `10-campfire-and-crafting.md`. The crafting runner checks
the owner-local menu, two-peer input restoration, hearth raw-material and
ignition payment, rejection gates, and exact final inventories. The construction
runner confirms that two raw-material floor builds replicate with
server-generated identities, those same identities restore after restart, and
subsequent paid builds replicate as two new records. The gathered-camp fixture
checks direct hearth construction alongside the full camp's material budget.
Use the scripts' `-Rendered` option for the crafting run when visually reviewing
the menu; a null-renderer peer run proves menu data and input flow but does not
inspect layout or the in-hand model visually. These checks add no RPC fields,
replicated gameplay field, or saved-data schema.

### M9 retired camp variants and raw-material catalogue

The raised chest has no item, recipe, placement-preview, or storage identity in
current gameplay. The normal Chest remains the only storage construction, and
the current schema-4 catalogue has no Smoke Frame or legacy smoke recipes. Run
`Kalmala.Gameplay.M9.RaisedStorage` to assert the retired chest identity and
surviving Chest recipe, `Kalmala.Gameplay.Inventory.Catalogue` for retired
food and smoke aliases, and `Kalmala.Gameplay.Food.CookingStationHeat` plus
`Kalmala.Gameplay.Status.SteadyMeal` for current food transactions, with
`Kalmala.Gameplay.Crafting.Transactions`,
`Kalmala.Gameplay.Construction.LocalPreview`, and
`Kalmala.Gameplay.Storage.SaveContract`. The storage test also checks that old
schema-one chest contents convert Fuel to Wood and ConstructionSupply to its
equivalent Wood and Fibre quantities. No save schema is extended.

### M9 optional land exploration rewards

After the forced editor build, run
`Kalmala.Gameplay.M9.ExplorationRewardCatalogue` with isolated user and log
directories. The test checks the bounded two-candidate catalogue, canonical
rewards, sparse identities, deterministic placement in each accepted biome,
walkable land and shoreline/ridge conditions, forged descriptor rejection,
and seed variation. For example:

```powershell
$testRoot = Join-Path $env:TEMP ('KalmalaM9ExplorationRewards-' + [guid]::NewGuid().ToString('N'))
$userDir = Join-Path $testRoot 'User'
$logFile = Join-Path $testRoot 'catalogue.log'
New-Item -ItemType Directory -Path $userDir | Out-Null
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\dev\Kalmala\Kalmala.uproject' -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir="$userDir" -abslog="$logFile" -ExecCmds="Automation RunTests Kalmala.Gameplay.M9.ExplorationRewardCatalogue; Quit" -TestExit="Automation Test Queue Empty"
```

Require `Result={Success}` for `ExplorationRewardCatalogue`. If the command-line
editor stops at the existing LinuxArm64/VisionOS SDK preflight, use
`UnrealEditor.exe` with the same project and automation arguments. Then run
`Scripts/Verify-M9ExplorationRewards.ps1 -Port <unused-port>` from the same
project mirror. This script resolves `Kalmala.uproject` relative to its own
`Scripts` directory, so use the copied script inside the mirror when verifying
a mirror build. The host/client fixture requires the server to accept the
exact reward for each player while rejecting distant, forged, duplicate, and
replayed claims, and checks that owner-only feedback and inventory state do not
leak to the other peer. Claims now persist in the authenticated player's
schema-2 save; reward materials remain session-only because ordinary inventory
is not saved. The fixture uses the null renderer, so it does not review the
discovery's visual presentation in-world.

### M9 schema-2 world construction migration coverage

After the forced editor build, run
`Kalmala.Gameplay.Construction.Schema2Migration` with isolated `-UserDir`,
`-abslog`, `-DDC-ForceMemoryCache`, and
`-TestExit="Automation Test Queue Empty"` arguments. The in-memory schema-2
candidate round-trips established construction records and station
attachments; migrates matching schema-1 construction records while
binding seed/revision/world scope; and rejects mismatched, unsupported,
malformed, duplicate, and over-cap legacy data without changing its source
bytes. It also checks exact schema-2 identity, malformed/duplicate current
records, and the 32-attachment limit. Normal construction writes use the
schema-2 container. On first write, a matching schema-1 save is migrated and
merged with the new server-accepted record; every record is revalidated before
the slot is replaced. Invalid or mismatched existing slots remain untouched.
Normal player-discovery writes use the schema-2 container. First writes migrate
matching schema-1 discovery/effect facts, revalidate and merge current server
M9 claims, and include the owner's complete carried-tool list. Invalid or
mismatched existing player slots stay untouched and block restore and writes.

### M9 schema-2 player discovery and tool migration coverage

After the forced `KalmalaEditor Win64 Development` build, run
`Kalmala.Gameplay.Discovery.Schema2Migration` with isolated `-UserDir`,
`-abslog`, `-DDC-ForceMemoryCache`, and
`-TestExit="Automation Test Queue Empty"` arguments. The memory round-trip
checks existing first-wave discoveries, learned effects, separate M9 land
claims, and canonical carried-tool IDs, levels, and condition. The schema-1
migration preserves valid discovery/effect facts, binds the requested seed,
revision 7, player scope, and authenticated identity, and starts the absent M9
claims and tool records empty. Rejection checks cover wrong seed/player,
invalid identity, schema zero/future versions, malformed facts, duplicate M9
claims and tools, invalid tool levels/condition, and each discovery, claim, and
tool bound plus one; failed migrations preserve the original source bytes.
Use `UnrealEditor-Cmd.exe` first; if it stops at the existing LinuxArm64 or
VisionOS SDK preflight, use the `UnrealEditor.exe` fallback with the same
project and automation arguments. Normal player-discovery slots use schema 2;
first-write migration and live claim/tool merging are covered by the M9
cross-system acceptance fixture.

### M9 schema-2 world/player candidate host-client reconnect

After the forced editor build and both focused migration automations, run the
two-client restart fixture against the isolated project mirror:

```powershell
& '.\Scripts\Verify-M9Schema2CandidateReconnect.ps1' -Port 19725 -ProjectPath "$projectMirror/Kalmala.uproject"
```

The fixture writes a dedicated world candidate and a schema-1 player fixture
to isolated user directories. It also uses a dedicated player-candidate slot
for owner-privacy checks. After the host restarts, it requires the normal player
slot's migrated discovery/effect facts, re-derived M9 claim, and complete tool
records to reload with exact identity. Adding the saved claim again must fail
without changing the claim set. Each client must receive only its own carried-
tool details. Normal construction and player slots must both report schema 2.
This fixture adds no replicated fields;
normal construction restart coverage is provided separately by
`Scripts/Verify-ConstructionPersistence.ps1` and the focused write-candidate
automation.

This null-renderer authority and replication check uses deterministic
test-provider identities; it does not verify a production online identity
provider or rendered UI. It seeds a normal schema-1 player slot, verifies
discovery/effect migration, writes a current re-derived M9 claim and tool state,
then checks those facts after host restart and owner reconnect.

## M11 item-gain notification child

After the affected isolated editor build, run Kalmala.UI.Notifications plus
Kalmala.Gameplay.Inventory.NetworkContract, Kalmala.Gameplay.Crafting.Transactions
and Kalmala.UI.Theme.LocalPresentation with memory DDC and isolated user/log
paths. Run the mirror's Scripts/Verify-Inventory.ps1 for live owner receipt
delivery and remote privacy. Full parent and combined renderer checks remain
pending until the combined acceptance child is complete.

## M11 discovery-notification child

After the affected isolated editor build, run
`Kalmala.UI.Notifications.Discoveries` with the standard memory DDC and
isolated user/log paths. It covers silent initial/reconnect baselines,
accepted landmark/scroll feedback, rejected feedback silence, refresh/expiry
deduplication, bounded labels, passive text and owner-local queue behavior.
Combined source placement, modal/reduced-motion behavior and the supported
viewport matrix remain in the notification parent's final acceptance child.

## M11 combined notification parent acceptance

Build `KalmalaEditor Win64 Development` from a short disposable mirror with
normal `%LOCALAPPDATA%/UnrealBuildTool` access and `-MaxParallelActions=4`.
Run the complete `Automation RunTests Kalmala` queue with an isolated user
directory, log, memory DDC, and `-TestExit="Automation Test Queue Empty"`;
require every requested test and process exit to pass, including
`Kalmala.UI.Notifications.CombinedPresentation`.

Run the two-peer rendered review at both supported acceptance settings:

```powershell
Scripts/Verify-Inventory.ps1 -Rendered -NotificationReview -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Inventory.ps1 -Rendered -NotificationReview -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-InventoryReconnect.ps1
```

Both rendered peers must report silent owner baselines, a complete three-source
queue, readable passive text at the requested scale/contrast, modal collapse,
and visible restoration. Retain and inspect combined/modal/restored source PNGs
for both peers at both settings. The reconnect verifier uses two client visits
to the same live host and requires a silent skill, item-receipt, and discovery
baseline on each visit. The local synthetic review labels are presentation-only;
the live inventory run remains the receipt-privacy and transaction check.

Then run `Scripts/Verify-PresentationOwnership.ps1`,
`Scripts/Verify-M5DocumentationContracts.ps1`, the PowerShell parser for the
changed scripts, `git diff --check`, and the 260-character path audit. Check
only the notification parent and final child after all required verification
passes. This parent check does not complete the wider M11 acceptance matrix,
physical input, package validation, or other M11 parents.

## M12 Workbench Repair section

The Workbench context shell opens Craft by default and lets the owner switch to
Repair without closing the accepted station context. Repair shows only that
owner's carried repairable tools, with their authored level and current
condition, through a separate selection widget from the Craft recipe state.
The free repair request reuses `ServerRepairTool` and submits only the selected
tool ID. The server reads the owner's current tool record, validates a visible
same-world Workbench or Forge within 250 cm, persists the repaired state, and
publishes the result only to the owner. Full tools and invalid client-side
selections do not dispatch a request; the server remains the authority for
station, tool, and persistence rejection.

`Verify-Crafting.ps1` expects a `Workbench Repair scope` result from both host
and client. It checks owner-pawn sourcing, tool-only rows, visible condition,
selection independence from Craft, hidden unrelated controls, and that an
invalid/stale context dispatches no repair request. `Verify-PresentationOwnership.ps1`
also checks the owner-only carried-tool and repair-result replication contracts
and the ID-only server route. The existing
`Kalmala.Gameplay.Tools.LifecycleContract` covers server repair authority,
unknown/full/invalid tools, missing stations, and rejected-state preservation.
The rendered menu and physical controller review remain part of M12's final
acceptance.

## M12 Forge Craft section

An accepted Forge interaction opens the shared station shell in Craft. Its
recipe filter includes only recipes requiring the Forge and recipes producing
its matching attachment, including Frying Pan production and Forge Anvil.
Ingredient rows and selected recipe requirements retain the catalogue costs,
station requirement, and current availability. A status line reads the
effective level and Anvil attachment state from the accepted Forge actor. Craft
requests continue through the existing recipe-ID/batch RPC without a
client-selected station or costs; the server revalidates station
visibility/range, recipe identity, inventory and material exchange. The Craft
section hides Workbench-only Bronze Axe operations and
unrelated build, food, repair, and storage controls.

`Verify-Crafting.ps1` expects a `Forge Craft scope` marker from both host and
client. Its prepared source assertion covers the Frying Pan's five-Iron Forge
recipe, matching Forge Anvil attachment, station-only recipe filter, accepted
interaction routing, and focused Craft presentation. The focused
`Kalmala.Gameplay.Food.CookingStationHeatContract` also checks the owner's exact
accepted Forge actor/kit/ID and interaction serial. Full runtime host/client
execution and rendered inspection remain in M12's final acceptance.

## M12 Forge Upgrade section

The Forge menu opens Craft by default and lets the owner switch to Upgrade
without closing the accepted station context. Upgrade shows the carried
Bronze Axe's level and condition against the level-two Iron Axe at its authored
starting condition; it lists Forge level 2, Crafting level 5 and the second-tier
unlock, the Bronze Axe level-one prerequisite, and every material cost with the
owner's current counts. It presents the first unmet requirement and enables the
existing Upgrade action only when the owner-visible progression is ready. The
action calls `ServerProgressTool` with only `IronAxe`; the server still chooses
a visible same-world Forge, validates the exact authored prerequisites, commits
the material/tool exchange, and persists the owner state. A stale or invalid
station context dispatches no upgrade request. Forge level/Anvil status remains
bound to the accepted actor; the progression and inventory state remain
owner-only.

`Verify-Crafting.ps1` requires a `Forge Upgrade scope` result from both host
and client. The focused marker checks target comparison, authored station/skill
requirements, all catalogue costs, unavailable-state presentation, section
routing and scope, plus no request from stale context. The existing
`Kalmala.Gameplay.M9.ToolStationProgression` covers server acceptance/rejection,
the Bronze-to-Iron exchange, costs, and unchanged candidates after rejection.
Run the full host/client and rendered checks only in M12 milestone-final
verification.

## M12 Forge Repair section

The Forge shell adds a Repair section beside Craft and Upgrade. It reuses the
owner-only carried-tool inspector and condition rows, with selection independent
of both the recipe index and the Upgrade presentation. A repair request contains
only the selected tool ID and uses the existing `ServerRepairTool` path; the
server still reads the owner's current tool record and accepts only a visible
same-world Workbench or Forge within 250 cm. Invalid or stale Forge context
dispatches no request. The accepted Forge context remains bound to its exact
actor and stable construction ID for UI lifetime, but the existing server
repair rule may resolve another qualifying nearby station.

`Verify-Crafting.ps1` requires `Forge Repair scope` from both host and client.
Its prepared marker covers owner-only tool rows and condition, Repair tab
routing, selection independence from Forge Upgrade and Craft, hidden unrelated
operations, unavailable-context feedback, and no request after context expiry.
`Verify-PresentationOwnership.ps1` checks that the client routes only a tool ID
and that repair results remain owner-only. Existing
`Kalmala.Gameplay.Tools.LifecycleContract` retains server authority, station,
unknown/full/invalid-tool, and rejected-state coverage. Runtime and rendered
host/client execution remain in M12 final verification.

## M12 Cooking Rack menu

The Cooking Rack shell shows only Cooked Boar Meat and Cooked Deer Meat. The
selected food uses the shared result description, live owned/required ingredient
counts, the existing one-batch-per-press and five-batch request limit, and a
live hearth-heat summary derived from the existing recipe-availability path.
Losing the exact accepted rack context disables Cook and sends no recipe request.
The owning client submits the existing recipe identity and batch-one request;
the server still selects a visible same-world Cooking Rack, validates positive
lit-hearth heat at both the player and station, checks materials/output capacity,
and commits the existing exchange.

`Verify-Crafting.ps1` expects `Cooking Rack scope: Recipes=1 Ingredients=1
Quantity=1 Description=1 Heat=1 NoUnrelated=1 StaleNoRequest=1 UiScope=1` on
both peers. Its focused UI marker covers the two catalogue recipes, their
ingredient counts and quantity limits, result description, live availability
presentation, hidden unrelated operations, and no request from an invalid
context. `Verify-PresentationOwnership.ps1` checks the local availability read
and existing ID/batch request route. The existing
`Kalmala.Gameplay.Food.CookingStationHeat` automation covers server acceptance
and missing-heat rejection for the accepted Cooking Rack. Run these checks with
the full rendered menu matrix during M12 milestone-final verification.

## M12 Cauldron menu

The Cauldron opens the shared station shell in Cook with only Meat Stew and Root
Vegetable Soup. Each selection shows its owner-visible ingredient counts,
existing one-batch-per-press and five-batch request limit, selected result
description, and live hearth-heat availability. Expired station context disables
Cook and sends no recipe request; the server's existing recipe-ID request and
station, heat, cost, capacity, and exchange validation remain authoritative.

`Verify-Crafting.ps1` expects `Cauldron scope: Recipes=1 Ingredients=1
Quantity=1 Description=1 Heat=1 NoUnrelated=1 StaleNoRequest=1 UiScope=1` on
both peers. The focused UI marker checks the two catalogue recipes and their
authored ingredient costs, quantity limits, selected descriptions, heat requirement/status, hidden unrelated
actions, and stale-context request suppression. `Verify-PresentationOwnership.ps1`
checks the Cauldron recipe filter, owner-local availability read, and existing
ID/batch request route. Run the prepared checks with the full rendered menu
matrix during M12 milestone-final verification.

## M12 Frying Pan menu

The Frying Pan opens the shared station shell in Cook with only Roasted Root
Vegetables and Deer and Rutabaga Roast. Each selection shows its owner-visible
ingredient counts, existing one-batch-per-press and five-batch request limit,
selected result description, and live hearth-heat availability. Expired station
context disables Cook and sends no recipe request. The narrow UI fixture also
keeps the three interactions distinct: `FryingPanRecipe` makes the pan at a
Forge for five Iron, the Construction Hammer Build catalogue places it, and the
placed pan cooks the two food recipes.

`Verify-Crafting.ps1` expects `Frying Pan scope: Recipes=1 Ingredients=1
Quantity=1 Description=1 Heat=1 NoUnrelated=1 StaleNoRequest=1 UiScope=1` on
both peers. The marker checks the authored ingredients and quantity limits,
result descriptions, heat presentation, exact pan recipe filter, Forge
production/build-placement separation, hidden unrelated actions, and stale
context request suppression. `Verify-PresentationOwnership.ps1` checks the
owner-local availability read and existing recipe-ID/batch request route. Run
the prepared checks with the full rendered menu matrix during M12 milestone-final
verification.

## M12 Chest storage menu

An accepted Chest interaction opens the shared station shell in Store. Its pack
selector reads only the local owner's current pack; its chest selector reads the
existing owner-only `StorageView`. Both show item descriptions, icons, and
counts. The summary reports occupied slots out of 16 for each destination and
disables Store or Take when the selected item, stack, or destination capacity
cannot accept one item. The UI submits only the selected item ID through the
existing deposit/withdraw RPCs. On the server, each request refreshes and
revalidates the exact active chest, visibility/range, current inventory, stack
limits, and persistence before publishing inventory changes. Closing or losing
the accepted chest context clears the private snapshot; no RPC or save schema
is added.

`Verify-Crafting.ps1 -Port <unused-port>` requires
`Chest scope: OwnerPack=1 OwnerChest=1 Capacity=1 Route=1 StaleNoRequest=1
UiScope=1` on both peers. Its focused marker checks the owner-local lists,
capacity feedback, StorageKit interaction route, hidden unrelated actions, and
no dispatch from a stale context. `Verify-PresentationOwnership.ps1` checks the
owner-only replication conditions and selected-ID transfer routes.
`Scripts/Verify-Storage.ps1 -Port <unused-port>` covers the live two-peer
deposit/withdraw transactions, capacity/conservation, private snapshots, and
restart persistence. Run those checks with the rendered Chest menu review in
M12 milestone-final verification; the focused crafting marker alone does not
replace the live transfer fixture.

## M12 Campfire direct refuelling

Looking at a placed Campfire shows **Add fuel** for the remappable Interact
action. The server interaction must consume exactly one raw item in the
existing Wood, Lightwood, Densewood, Coal priority, add 60 seconds up to the
300-second cap, and leave the hearth state unchanged. Its visible-target trace,
same-world access, and 250 cm range remain required. Lighting stays on the
separate existing Build-menu action until the goal-7 cleanup child.

`Scripts/Verify-Crafting.ps1 -Port <unused-port>` requires the server marker
`Campfire interaction: AddedOne=1 NoLighting=1 FullRejected=1
NoFuelRejected=1 RangeRejected=1`. The fixture reaches the target through the
normal server Interact trace and checks priority payment, no-lighting behavior,
full-capacity and no-fuel preservation, and out-of-range rejection.
`Kalmala.UI.InteractionPrompt.Presentation` covers the action-only prompt and
explicit no-fuel/full reasons; `Verify-PresentationOwnership.ps1` checks the
existing server-owned raw-fuel route and owner-local prompt source. Run the
host/client runtime and rendered prompt review with the M12 milestone-final
verification.

## M12 Build catalogue scope

The standalone Construction Hammer menu defaults to the supported placeable
catalogue and cycles only All builds, Structural pieces, Stations and Camp
utilities. Bootstrap construction remains available. Station-required item
production and station attachments remain in their matching Workbench/Forge
service section; their placeable outputs stay in Build for preview/placement,
but production is disabled there. The existing selected-output ingredient
counts, direct-material cost substitution, local preview and server placement
routes remain intact.

The `Kalmala.UI.Crafting.LocalBrowsing` automation checks that the default Build
grid equals the supported placement outputs, includes the bootstrap/station/
attachment recipes, excludes non-placeable outputs, and rejects Build-menu
production for service-only recipes. `Verify-Crafting.ps1 -Rendered` reviews
All builds and its named groups plus bootstrap and station-kit costs/requirements
on both peers. Workbench/Forge scope markers continue to check their production
lists. Run the affected UI build, full queue, host/client rendered matrix,
presentation ownership and documentation checks during M12 milestone-final
verification; this child does not claim those runtime results.

## M12 Build context cleanup

Standalone Build retains placeable browsing, material details, local preview,
placement, latest action feedback, and the explicit **Light hearth** action.
The old pack inspector, food controls, raw-fuel button, repair/upgrade panels,
and chest lists are removed from Build. Inventory/food use lives in Inventory;
repair and upgrade live in Workbench/Forge contexts; storage lives in Chest.
Campfire Interact replaces the raw-fuel button, but it only refuels, so the
separate Light action remains until an equivalent relight route is integrated.

After an affected UI build, `Verify-Crafting.ps1` requires
`Build context cleanup: Header=1 ObsoleteHidden=1 Placement=1 Relight=1 Status=1
StorageShell=1` on both peers, alongside Workbench `PassiveRack=1` and Forge
`PassiveAnvil=1`. Its rendered matrix captures the cleaned standalone Build
menu and verifies the clean view after category/no-results recovery. Run the
full queue, host/client rendered matrix, ownership and documentation checks
during M12 milestone-final verification; this child does not claim those
runtime results.

## M12 catalogue icon integration

The shared recipe/build grid, selected-output preview and Forge Upgrade target
use imported canonical textures through `UKalmalaIconWidget`. Each row retains
its own player-facing name, availability, selection/focus and structured
requirements. Run `Kalmala.UI.CatalogueIcons.CompleteCoverage` after an affected
editor build to verify imported texture paths and loads for all current item,
tool and recipe-output identities. `Scripts/Verify-Crafting.ps1` also requires
`Catalogue icons: Grid=1 Selected=1 Upgrade=1` from both peers; the grid flag
checks each visible recipe output against its canonical loaded texture.

The canonical icon count remains 48: 41 normal item definitions, six carried
tools, and the Campfire construction identity. CampfireKit remains in the
shared icon map and recipe/build/preview views but is not a pack or storage
item. The status icon gallery includes unique recipe/build identities so the
construction-only Campfire texture is included in the same count.

During M12 milestone-final verification, use
`Scripts/Verify-Crafting.ps1 -Rendered` for Build categories and station
craft/upgrade contexts, and inspect the selected-output and unavailable views
for readable image, name, count, focus and requirement overlays. Review host and
client captures. Once Favorite/Rank/Recent markers are present, render a card
showing all applicable markers at once and confirm none overlap each other or
the object image. Keep badges as independent UI overlays; unknown IDs must keep
the honest fallback and no supported manifest identity may use it. This
increment prepares these assertions; it does not perform the M12 rendered
acceptance or full verification.

## M12 direct construction descriptor split

Direct-material recipes keep their stable construction identities in the
runtime `BuildableOutput` field; `Output` is reserved for actual inventory
results. The catalogue loader maps the legacy Campfire `HearthRing` reference
to `CampfireKit` in that construction field. `Kalmala.Gameplay.Inventory.Catalogue`
checks that the descriptor validates with no Campfire item entry, that normal
recipe scaling rejects direct construction output, and that the legacy source
reference still resolves to the stable construction identity.
`Kalmala.Gameplay.Crafting` and `Kalmala.UI.Crafting.LocalBrowsing` retain
raw-cost, Build-menu, icon, and placement selection coverage. Run the focused
automations after an affected editor build; confirm paid placement and
failed-placement conservation in the M12 milestone-final
`Verify-Crafting.ps1` host/client run.

## M12 Campfire item retirement

`GameCatalogues.json` has 41 normal item definitions after removing the
HearthRing inventory row. The Campfire recipe still uses source output
`HearthRing`, which the loader maps to `BuildableOutput=CampfireKit`; recipe
and placed-result text say Campfire. CampfireKit remains an internal stable
construction identity and icon key, never an inventory output. Keep the
existing 5 Stone + 3 Wood direct cost and one priority-selected raw fuel item.

`Kalmala.Gameplay.Inventory.Catalogue` asserts the item is absent while the
recipe descriptor validates. `Kalmala.Gameplay.Construction.Schema2Migration`
retains a CampfireKit save-record round trip. `Kalmala.Gameplay.Crafting`
continues to cover successful payment and overlap rejection without payment;
the placement path still allocates before the atomic inventory exchange and
destroys the deferred actor if payment fails. No save schema or construction
identity changes. The generic saved-construction path stores the stable kit ID
and restores supported records through `AKalmalaConstructionActor`. Live
`AKalmalaCampfire` gameplay actors remain outside that generic construction
save, as before.

At M12 milestone-final verification, run the applicable automation queue and
the rendered host/client `Scripts/Verify-Crafting.ps1` matrix. This child only
prepares the contracts; it does not run a build, runtime scenario, or rendered
acceptance.
