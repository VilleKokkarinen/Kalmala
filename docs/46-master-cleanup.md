# Master integration and workspace cleanup — 2026-10-09

The user requested integrating all retained work into `master` and removing
worktrees, `m12verify`, and unnecessary folders. This is explicit repository
integration/cleanup, not another backlog feature increment.

## Preserved work

Committed the existing main-checkout handoffs and `.gitignore` edit as
`216a107`. Preserved the older uncommitted M12 HUD draft as `cd5af68`, then
merged its history into master. M11's accepted `2626ba1` chain was merged as
`499464e`; the draft merge is `728131c`. The final M12 `f433528` chain is
integrated with those histories, preserving current main backlog completion
and progress entries. Branch refs remain available after their checkouts are
removed; no history was reset, rebased or force-pushed.

The merged UI keeps M11's local Favorites, usage ranks, Recent tracking,
per-menu browsing memory, and inline Forge comparison with M12's separate
Inventory/station contexts, concise result/requirement copy, canonical raster
icons and active-only status group. The M12 result view replaces the older
duplicated result presentation. Ended statuses disappear immediately as M12
requires; start/refresh cue bookkeeping and reduced-motion behavior remain
local. No server authority, request validation, save schema or balance changed.

Integration repairs:

- Restored crafting declarations removed by the overlapping merge and routed
  inspector memory to the carried-tool repair inspector, preserving the
  retirement of the persistent Inventory HUD.
- Used `GetOutputIdentity()` for build activity/ranking and preview coverage,
  including direct-material constructions whose inventory `Output` is empty.
- Kept inline upgrade comparison in the Forge Upgrade section and collapsed it
  in Build and other service sections.
- Cleared stale requirements in the no-results view.
- Retained a scoped Build Favorites capture showing distinct owners and
  Favorite/Gold Rank 1/Recent markers together with the canonical object image.
  Favorites and Recent filtering continue to respect the service/build scope.

## Verification

Verification ran in the disposable `E:\dev\Kalmala\m12verify` mirror with all
current Source/Scripts hashes matched to main. UnrealBuildTool had normal access
to its local cache. The final commands were:

```powershell
Build.bat KalmalaEditor Win64 Development E:/dev/Kalmala/m12verify/Kalmala.uproject -WaitMutex -NoHotReload -Force -MaxParallelActions=4
UnrealEditor-Cmd.exe E:/dev/Kalmala/m12verify/Kalmala.uproject -unattended -nop4 -nosplash -nosound -nullrhi -DDC-ForceMemoryCache -forcelogflush -UserDir=E:/dev/Kalmala/m12verify/.mvtmp/int-ax -abslog=E:/dev/Kalmala/m12verify/.mvtmp/int-tests-last.log '-ExecCmds=Automation RunTests Kalmala' '-TestExit=Automation Test Queue Empty'
Scripts/Verify-Crafting.ps1 -Port 18740 -Rendered -Width 1280 -Height 720 -TextScale 100 -Contrast 0
Scripts/Verify-Crafting.ps1 -Port 18741 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-StatusHotbar.ps1 -Port 18752 -Width 1024 -Height 768 -TextScale 150 -Contrast 1
Scripts/Verify-InventoryMenu.ps1 -Port 18753 -Width 1024 -Height 768 -TextScale 150 -Contrast 1 -InterfaceScale 120 -ReducedMotion
```

The final editor build passed. The final full queue passed **120/120 tests**,
zero failures, queue-empty exit 0. The earlier queue found two integration
failures in activity identity and preview coverage; those passed after repair.
The rendered helper initially found out-of-context comparison labels and stale
no-results requirements; both complete rendered configurations passed after
repair, including all service views and the activity-marker capture. The
compact HUD passed actual scaled image geometry, support/minimap separation,
active-only/finite-timer/empty-state checks. Inventory passed all eleven stages
on both owners, including privacy, meal use/repeat, live changes, empty pack
and input restoration.

All PowerShell scripts parse. M5 documentation contracts, local input,
presentation ownership, menu copy, panel retirement and settings/accessibility
checks passed. Staged diffs and committed-path MAX_PATH checks passed.
The existing DebugGame unity-name fixes and solution utility exclusions remain.
No packaging, new hardware test, publication or dedicated-server build ran.
Prior accepted diagnostic performance evidence remains in `docs/m12/`.

Final logs and representative paired screenshots are retained in
[master-int](master-int/), including [full queue](master-int/int-tests-last.log),
[build](master-int/int-build-last.log), and
[compact activity markers](master-int/compact-client-activity-markers.png).
The original M12 and M11 evidence remains in their tracked documentation folders.

## Cleanup

Removed main `Binaries`, `Intermediate`, `DerivedDataCache`, `.cache`, `.ms`,
`.vs`, the literal `$runDir`/`$user`/`$userDir` folders and the empty
`System.Management.Automation.Internal.Host.InternalHost` directory. The user
saved and closed Visual Studio to release its locked index files. Removed
ignored plugin build caches and regenerable Saved logs/build/shader/crash caches.
Preserved Saved/SaveGames, Saved/Config, Saved/Autosaves and Saved/Collections.
Generated build and IDE files will be recreated on the next build/editor use.

Worktree and verification-folder removal is finalized after the integrated
commit. No tracked project source, original art, imported asset, retained
acceptance evidence, local settings or save-game data is deleted by cleanup.
