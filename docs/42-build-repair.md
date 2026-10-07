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
