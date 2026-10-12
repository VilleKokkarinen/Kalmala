# M14 main integration verification — 2026-10-12

The accepted source candidate `codex/m14-fix` (tip `4bb141e`) was integrated into `master`, whose pre-merge tip was `619263c`. The isolated-candidate acceptance remains recorded in [README.md](README.md) and [acceptance.json](acceptance.json); this report records checks against the integrated main source.

## Build and automation

The UE 5.8 Win64 Development editor build passed all 241 actions from the isolated merged-source mirror `C:\Users\Ville\AppData\Local\Temp\m14-b8673a84`. Command: `Build.bat KalmalaEditor Win64 Development -Project=C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\Kalmala.uproject -WaitMutex -MaxParallelActions=4`. The final build log is `C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\build-main.log`; UnrealBuildTool used its normal writable local directory.

The first integrated-main command-line `Automation RunTests Kalmala` queue completed with **120 successes and 3 failures** (123 tests total). The three failures were:

- `Kalmala.UI.CatalogueIcons.CompleteCoverage`: expected “Crafted recipe uses a canonical inventory identity” (`KalmalaStatusHotbarTest.cpp:360`).
- `Kalmala.UI.Crafting.LocalBrowsing`: expected the current crafting-browsing acceptance assertion (`KalmalaRecipeBrowseTest.cpp:18`).
- `Kalmala.UI.Theme.LocalPresentation`: its 9-slice edge and button-padding expectations failed (`KalmalaUIThemeTest.cpp:39,161`).

The failing test files and relevant supporting UI files were byte-identical to pre-merge `master`; none were changed by M14. The integrated M14 automation cases `Kalmala.UI.WorldMap.BiomeTextures`, `Kalmala.UI.WorldMap.LocalPresentation`, and `Kalmala.UI.WorldMap.PerformanceBudget` passed. The first full-queue log is `C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\automation-main.log`.

## Final integrated-main verification — 2026-10-12

The three initial failures were repaired, then the Development editor target was rebuilt and the full automation queue rerun in the isolated mirror. The build command `C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat KalmalaEditor Win64 Development -Project=C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\Kalmala.uproject -WaitMutex -MaxParallelActions=4` succeeded after recompiling the changed UI test sources and linking the editor target. UnrealBuildTool used its normal writable local directory.

The focused command `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\Kalmala.uproject -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir=C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\test-focused-final -abslog=C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\automation-focused-final.log -ExecCmds="Automation RunTests Kalmala.UI.CatalogueIcons.CompleteCoverage+Kalmala.UI.Crafting.LocalBrowsing+Kalmala.UI.Theme.LocalPresentation; Quit" -TestExit="Automation Test Queue Empty"` passed all three requested tests. The final full command `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\Kalmala.uproject -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -UserDir=C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\test-final-main -abslog=C:\Users\Ville\AppData\Local\Temp\m14-b8673a84\automation-final-main.log -ExecCmds="Automation RunTests Kalmala; Quit" -TestExit="Automation Test Queue Empty"` completed with **123 successes, 0 failures**, and editor exit code 0. The log recorded 123 test starts and 123 successes.

Repairs:

- `Kalmala.UI.CatalogueIcons.CompleteCoverage`: the Building Hammer recipe outputs `ConstructionHammer`, a canonical carried-tool identity intentionally absent from the inventory item catalogue. The assertion now verifies that recipe output against the construction-hammer tool definition while preserving the regular item checks for other crafted recipes.
- `Kalmala.UI.Crafting.LocalBrowsing`: the layout fixture created a native `UUserWidget` without a registered local-player controller, so Unreal skipped `NativeOnInitialized` and the test inspected an empty widget tree. The fixture now creates a transient world, registers its controller and local player, and creates the widget through the normal player-context path. Its test-only layout probe also returns the specific failed conditions.
- `Kalmala.UI.Theme.LocalPresentation`: the expected default button padding was stale at 8 after the configured default became 9. The 9-slice test now checks the imported 160×53 source size; `GetSizeX/Y()` returned zero in the null-RHI run before platform resource data was initialized, while the imported dimensions and 0.16/0.18 slice margins were correct.

The repairs update automation expectations and setup only; no unrelated UI behavior, gameplay authority, networking, or save schema changed. With the final full queue passing, **M14 integrated-main milestone verification is complete**. Map sharing remains deferred.

## M14 live and rendered checks

All commands below used the corrected consolidated `Scripts/Tests/World/Verify-WorldMap.ps1` on the built mirror, with `-AcceptanceEvidence`. Every completed invocation passed host/client checks for local pins and optional labels, unchanged private fog, ready-tile reuse within the bounded cache, and personal-coverage reload after restart.

| Scenario | Command arguments | Result |
| --- | --- | --- |
| Standard map matrix | `-BasePort 20444` (all resolutions) | PASS: 1024×768, 1280×720, 2560×1080; matching peer fingerprints |
| Fitted overview | `-BasePort 20449 -Resolution 1280x720 -Overview` | PASS: `Fits=1 Reveal=0` on both peers |
| Large text and contrast | `-BasePort 20450 -Resolution 1024x768 -TextScale 150 -Contrast 1` | PASS |
| Application scale 125% | `-BasePort 20451 -Resolution 2560x1080 -ApplicationScale 1.25` | PASS |
| Application scale 150% | `-BasePort 20452 -Resolution 2560x1080 -ApplicationScale 1.5` | PASS |
| Close zoom | `-BasePort 20453 -Resolution 1280x720 -CaptureZoom 2500` | PASS |
| Maximum zoom | `-BasePort 20454 -Resolution 1280x720 -CaptureZoom 3500000` | PASS |
| Changed project theme | `-BasePort 20455 -Resolution 1280x720` | PASS |
| Changed theme, high contrast | `-BasePort 20456 -Resolution 1024x768 -TextScale 150 -Contrast 1` | PASS |

The two theme captures used a temporary override in the isolated mirror (`ButtonFocused=(R=0.15,G=0.60,B=0.78,A=1)` and `WorldMapPanelImage=/Game/Kalmala/UI/BuildPanel.BuildPanel`). The original theme file was restored and its SHA-256 matched its pre-test value `2F12850848A156BCC3A99204F67DDCB63CCBB9C9C0433E2CD6EE24A894B798BC`.

`Scripts/Tests/World/Verify-MapAwareness.ps1 -Port 20442` passed private player-position checks, global ping delivery without consent, malformed/out-of-world rejection, per-sender cooldown, host/client ordering, opt-out independence, and ten-second expiry. `Scripts/Tests/World/Verify-WorldMapExport.ps1` also passed PNG integrity, export/tuning variation, invalid-input preservation, and restored-default checks.

The first rendered attempts exposed two stale runner assumptions: `$resolution` shadowed the validated `-Resolution` parameter, and the profile matcher expected a period immediately after `CacheBytes` despite M14's optional `CachedTiles` field. Both were fixed in the integrated runner and parser-checked. The complete matrix above passed with those fixes. One overview restart timed out while another rendered run was active; its serial rerun passed.

## Retained captures and status

The integrated host/client screenshots reviewed during verification are retained here:

| Capture | SHA-256 |
| --- | --- |
| [Standard 1280×720 client](main/standard-1280x720-client.png) | `0D2A15EB6E9F33F8A71B029982831C5574E54A6BAB227A622FA70DE6FAD2B647` |
| [Fitted overview](main/overview-1280x720-client.png) | `F542CDDF0D63D766D08836828A4905B0F52799A7E644E8CE851F1B975BBADAAF` |
| [High contrast, 150% text](main/high-contrast-150-text-1024x768-client.png) | `903A832A56FFF9587A40943679719F80080E0691E4DE06294D66CDA6378FE1EF` |
| [150% interface scale](main/scale150-2560x1080-host.png) | `8E01A15931E98D1A392460D0CBF9F8888BB71A28DB15330CCB557497A4D40542` |
| [Close zoom](main/closezoom-1280x720-host.png) | `F204828B223E1A7E2A43A6134A69AFE0B25D8935C08894E64D504753C9B5355B` |
| [Maximum zoom](main/maxzoom-1280x720-host.png) | `A451CB14374D91C25442E0EFD264BAA86F035A5BA65B4CE8DCD5BEDD1EC83BBF` |
| [Changed theme](main/changed-theme-1280x720-host.png) | `F7CC1B498431F8165E412E33F76CC43AFCE3DAF1F5CC28BDAF5C5C1CBEEA0588` |
| [Changed theme, high contrast](main/changed-theme-contrast-1024x768-client.png) | `6EEAA0F8AF14EC331BED5CF72F6E2627BE0C9F1896859106C404C85A1F218725` |

The M14 source is integrated and its map-specific build, automation, network, exporter, rendering, privacy, cache, viewport, zoom, and theme checks passed. The first integrated-main queue's three failures are resolved by the rerun above; final integrated-main automation passed 123/123. **M14 is complete.** No saved-data schema or gameplay authority changed; pings remain server-validated and transient, while exploration and pins remain owner-local.
