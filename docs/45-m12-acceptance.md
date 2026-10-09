# M12 final acceptance — 2026-10-09

The accepted work is now integrated into `master`; the historical worktree and
temporary mirror named below are removed by the user-requested cleanup.
See [master integration and cleanup](46-master-cleanup.md) for the combined
120-test queue, restored M11 activity markers and final master evidence.

Status: **M12 complete.** All final verification and outstanding owner acceptance
gates passed. This record closes
the manual gates left by commit `a580e1b`; it does not start another milestone.
The retained implementation checkout is
`E:\dev\Kalmala\wt\m12-hud-feedback-rebuild`, branch
`codex/m12-hud-feedback-rebuild`. Builds and runtime scenarios use the disposable
`E:\dev\Kalmala\m12verify` mirror with matching changed source/script hashes.

## Findings and repairs

- Inventory detail images stretched across their column. Aligning the 64x64
  image slot to the left preserves its square geometry.
- The fixed-height inventory/detail area clipped equipment and selected-item
  information at large text scales. Its height is now a minimum; the existing
  outer scroll reaches the complete content. Carried-tool cards wrap within
  their actual width and show their authored tool description in repair views.
- Chest inspectors now divide navigation width equally and wrap their button
  captions, keeping both labels inside each narrow owner/chest column at 150%.
- Recipe-grid, selected-output, and upgrade images stretched with adjacent
  multiline text. Their horizontal-box slots now keep the intended vertical
  alignment and square size.
- The status overlay loaded textures but painted tiny images. Its raster slot
  now fills both axes of the scaled 64x64 box. Runtime verification measures
  all six populated images, rather than inferring their size from asset loading.
- Combined compact-screen captures exposed support cards extending outside the
  support panel and covering status icons. The panel measures its four-card row;
  status layout reserves a further 12 UI units beyond its actual right edge and
  wraps downward inside the remaining lane. Both cues receive the same global
  accessibility scale and contrast during verification.

The new Inventory capture fixture opens the subsystem-owned production menu.
Distinct server-owned packs show host Wood 7/Stone 2 and client Wood 23/Iron 4.
It checks equipment, item details, search/no-results recovery, Hearth Broth use
and a repeated action, live Wood changes to 4/20, sixteen empty slots, remote
owner privacy, and input restoration on close. Mutations use existing validated
server APIs. The fixture and service-review hooks are non-shipping and launch
gated; no RPC, save schema, gameplay balance, catalogue content, or replication
contract changed.

## Current-run verification

Run the following from the built disposable mirror; engine invocations use
normal read/write access to `%LOCALAPPDATA%\UnrealBuildTool` and isolated short
user/shader paths. `TEMP` points to the mirror's `.mvtmp` directory.

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' KalmalaEditor Win64 Development E:/dev/Kalmala/m12verify/Kalmala.uproject -WaitMutex -NoHotReload -Force -MaxParallelActions=4
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' E:/dev/Kalmala/m12verify/Kalmala.uproject -unattended -nop4 -nosplash -nosound -nullrhi -DDC-ForceMemoryCache '-ExecCmds=Automation RunTests Kalmala' '-TestExit=Automation Test Queue Empty' -forcelogflush -UserDir=E:/dev/Kalmala/m12verify/.mvtmp/aok2 -abslog=E:/dev/Kalmala/m12verify/.mvtmp/m12-last-tests.log
Scripts/Verify-InventoryMenu.ps1 -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-InventoryMenu.ps1 -Width 1024 -Height 768 -TextScale 150 -Contrast 1 -InterfaceScale 120 -ReducedMotion
Scripts/Verify-Crafting.ps1 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Crafting.ps1 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-StatusHotbar.ps1 -Width <width> -Height <height> -TextScale <scale> -Contrast <contrast>
Scripts/Verify-ConstrainedPerformance.ps1 -Profile <profile> -Project E:/dev/Kalmala/m12verify/Kalmala.uproject -OutputDirectory <new-short-directory> -TimeoutSeconds 240
```

The forced editor target build passed after repairs. The final full queue
passed **116/116 tests**, zero failures, process exit 0. The new reserved-lane
status assertions also passed in a focused two-test rerun before the full queue.
See [build log](m12/build.log) and [automation log](m12/automation.log).

Both Inventory configurations passed all eleven stages on each peer, with
painted detail-image geometry and owner privacy checked. Both Crafting
configurations passed their transaction, context, input and capture gates and
produced 32 images per peer: fourteen original Build/requirement views and top
and detail views for nine service sections. The service review deliberately
uses an **unavailable context** and does not manufacture an accepted actor or
claim a new transaction. Separate existing live/automation checks cover
accepted actions and stale-context rejection. Both configurations were refreshed
after the HUD and caption repairs, with complete runner output, peer logs and
64 PNGs each.

The final status matrix covers 1024x768, 1280x720 and 2560x1080 at 100% standard
contrast, plus 100/125/150% high contrast at each size. Each peer retains empty,
populated, expired, detail and icon-gallery images. The final helper requires
painted raster size and support/status separation, as well as minimap gap,
safe bounds, active-only icons, finite-effect timers and hidden empty state.
Earlier captures that showed tiny icons or overlapping support are superseded.

All three constrained profiles passed sequentially after the final HUD repair,
including discovery/disembark and observed host/client affinity. Each peer
retained 300 frame samples per profile. These are diagnostic scenario/capture
integrity checks on this machine; their scope is defined in
[M10 constrained performance](34-m10-constrained-performance.md).

| Profile | Frame p95, host / client (ms) | Retained evidence |
| --- | --- | --- |
| Reference | 5.33 / 5.71 | [reference](m12/performance-reference/) |
| Eight threads | 5.03 / 5.79 | [cpu8](m12/performance-cpu8/) |
| Four threads | 12.82 / 13.96 | [cpu4](m12/performance-cpu4/) |

All PowerShell scripts parse. Local input, presentation ownership, all five M5
documentation contracts, settings/accessibility, menu binding-copy, retired-panel
checks and `git diff --check` pass. All 48 catalogue icons and all nine status
icons pass manifest/image/package validation. No new image was generated or
imported in this continuation. See [static check log](m12/static.log).

## Retained visual evidence

| Views | Standard | Compact / high contrast |
| --- | --- | --- |
| Inventory, eleven stages per peer | [inv-std](m12/inv-std/) | [inv-hc](m12/inv-hc/) |
| Build and nine service sections, 32 per peer | [craft-std](m12/craft-std/) | [craft-hc](m12/craft-hc/) |
| Interaction prompts, five per peer | [prompt-std](m12/prompt-std/) | [prompt-hc](m12/prompt-hc/) |
| Status matrix, five per peer per case | [matrix](m12/status-matrix.csv) | Twelve cases, both peers |

The [evidence hash manifest](m12/evidence-sha256.csv) records all 373 retained
captures, logs and timing/profile files; 312 of those files are peer PNGs.

Visual review covers owner-distinct counts, equipment and descriptions,
square icons, reachable large-text detail fields, unavailable requirements,
construction-only browsing, action-only prompts, absent legacy panel/tutorial
card, finite timers and minimap/support separation. Both interaction-prompt
configurations were rerun on the repaired build and passed all five stages per
peer. Early standard Inventory filled frames can include engine shader warmup;
the recovered and later views retain the settled rendering as well.

## Previously completed milestone gates

The 2026-10-09 11:26 UTC progress entry and commit `a580e1b` record the passing
construction restore/remote replication, persisted two-player camp/hearth
authority, inventory transactions/privacy and two reconnect visits, camp
choices, storage conservation/persistence, settings/accessibility persistence,
minimap at 1920x1080/1024x768/3440x1440, and rendered player-controls scenarios.
Those results remain part of this milestone's verification; the UI repairs do
not alter their server, persistence or input contracts. The current full queue
revalidates the applicable inventory, construction, cooking/heat, repair,
storage, status, interaction, navigation and ownership contracts.

The user explicitly confirmed the outstanding physical keyboard and controller
check: **“Tested both; all passed.”** This is owner-reported hardware acceptance,
separate from scripted delegates and server-observed input scenarios.

## Limits preserved

`HearthBroth` is the only current catalogue item supported by the existing meal
use allowlist and has no current production recipe. The removed historical
`RoastedFieldMeat`/`SmokedFieldMeat` IDs remain legacy allowlist entries, and the
six current cooking outputs have no Eat action. This predates M12: the main
baseline `8c10ab4` has the same allowlist, and
[M9 recipes](28-m9-camp-equipment-recipes.md) document the old-item retirement.
The fixture uses Hearth Broth; this presentation milestone does not introduce
food gameplay or change that contract.

Favorite/Rank/Recent overlays are absent from this retained M12 implementation,
so the conditional badge-coexistence capture has no eligible card. Earlier M11
acceptance handoffs and main-checkout completion rows are preserved. This run
does not reconcile implementation branches or change those historical rows.

No packaging, publishing, dedicated-server build, dependency installation,
screen-reader session or new fresh-player playthrough was performed in this
continuation. Existing platform/release limitations remain documented in their
accepted milestone records. Only `BACKLOG.md` and `PROGRESS.md` are synchronized
to the dirty main checkout; implementation and retained evidence remain in the
retained worktree. No later backlog increment is selected.
