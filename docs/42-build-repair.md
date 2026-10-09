# Build repair — 2026-10-07

The main-checkout `KalmalaEditor Win64 DebugGame` failure was MSVC C4459:
`KalmalaWorldMapWidget.cpp` declared `PanelColour` locally while
`KalmalaSettingsWidget.cpp` supplied an anonymous-namespace constant with the
same name in the combined unity translation unit. Rename the map local to
`MapPanelColour`. Also rename the catalogue-row `NoPanelImage` constant to
`CatalogueNoPanelImage` to avoid its duplicate in `KalmalaCraftingSubsystem.cpp`
when both files enter unity compilation. Both changes preserve presentation.

The same Visual Studio solution rebuild attempted `DotNetPerforceLib`, which
this installed engine rejects with “Program targets are not currently supported
from this engine distribution”, and `LiveLinkHub`, whose post-build copy could
not find its DebugGame target receipt. `Kalmala.slnx` now excludes those two
utility projects from solution builds while retaining their browsing entries
and the Kalmala project build mappings. Regenerating project files may replace
these solution exclusions; recheck them after regeneration. The legacy generated
`Kalmala.sln` was not edited; use `Kalmala.slnx` or the direct target command.

Build the game project in Visual Studio with `DebugGame Editor | Win64`, or run:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaEditor Win64 DebugGame -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex -MaxParallelActions=4
```

Run UnrealBuildTool with normal read/write access to the real
`%LOCALAPPDATA%\UnrealBuildTool`. To exercise unity collisions explicitly, append
`-DisableAdaptiveUnity` to that command.

Verification used an isolated copy of the current working tree, including its
pre-existing recipe-preview changes, at `E:\dev\Kalmala\wt\bf`. Both the normal
DebugGame editor build (224 actions) and the build with `-DisableAdaptiveUnity`
(20 actions) succeeded. MSBuild's `ValidateSolutionConfiguration` target passed
for `Kalmala.slnx` with `Configuration=DebugGame Editor` and `Platform=Win64`.
XML checks confirmed the two exclusions and the enabled game-project mapping.
The entire Visual Studio solution was not rebuilt. Runtime tests, rendered
acceptance, packaging, and full milestone verification were not rerun for these
identifier and solution-selection changes. No gameplay, networking, authority,
or persistence contract changed.

## Project-file recovery — 2026-10-09

The authorized master cleanup removed `Intermediate`, while the existing IDE
solutions still referenced its generated projects. The legacy solution had
eight missing project files, including `Kalmala.vcxproj`, `EventLoopUnitTests`
and `IoStoreOnDemandTests`. The installed engine's
`EpicGames.Analytics.Generators.csproj` existed; the error was a stale solution
reference, not a missing Analytics source project.

With the owner's explicit permission to recreate generated project files, run
the installed engine's bundled .NET runtime from its `Engine/Source` directory:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe' 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll' -projectfiles '-project=E:\dev\Kalmala\Kalmala.uproject' -game -rocket -progress
```

UnrealBuildTool succeeded with normal local-cache access. Its installed-engine
solutions omit the unsupported native utilities and low-level tests, so
`DotNetPerforceLib` and `LiveLinkHub` cannot be included in solution builds.
The UE5 build exclusion and `DebugGame Editor | Win64` game mapping remain.
Both Kalmala solutions contain 59 project references with none missing; both
Automation solutions contain 53 with none missing. The generated Automation
XML solution also picks up the existing VisionOS Automation source project.

MSBuild `ValidateSolutionConfiguration` passed for `Kalmala.slnx` with
`Configuration=DebugGame Editor` and `Platform=Win64`. XML, reference, whitespace
and generated-path checks passed; the longest generated absolute path was 180
characters. Reopen `Kalmala.slnx` in Visual Studio after regeneration. No game
build, editor launch or runtime tests ran for this recovery; the removed game
binaries still need the editor build documented above.

## Inventory editor build repair — 2026-10-09

The new inventory-grid editor target failed compilation with MSVC C4458 in
`KalmalaInventoryGridWidget.cpp`: local variables and parameters named `Slot`
hid the inherited `UWidget::Slot` member. Rename the integer identifiers to
`SlotIndex`; keep the `.Slot` widget-layout accesses unchanged.

`KalmalaInventoryMenuSubsystem.cpp` also used `auto*` for
`APlayerController::GetPawn()`. In the installed UE5.8.2 headers this returns
`TObjectPtr<APawn>`, so explicitly unwrap it with `.Get()`. The reported errors
on the following expressions were parser/type cascades from that declaration.

The first clean short-path mirror build ran 239 actions and failed on those
compile errors. After applying the source fixes, the editor target was rebuilt
in that mirror with normal read/write access to `%LOCALAPPDATA%\UnrealBuildTool`:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaEditor Win64 Development '-Project=E:\dev\Kalmala\wt\bf\Kalmala.uproject' -WaitMutex -MaxParallelActions=4
```

The UE5.8.2 `KalmalaEditor Win64 Development` rebuild succeeded, compiling and
linking the nine actions dirtied by the repair. The target build was the only
build verification in this follow-up; automation, editor launch, rendered UI,
input and host/client acceptance remain deferred. The temporary mirror was
removed after the build; the command records the executed path and is not a
retained project directory.
