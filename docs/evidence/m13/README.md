# M13 integrated-main acceptance — 2026-10-11

M13 passes its scripted/offscreen final gate on main. This run integrates committed candidate `5ac3b05d197692a8ef6fd2f7be05234f40c60c7a` with main parent `fe7d28228522fcee0966fa06c2a828a5f17beefb`, preserving pre-existing handoff edits and the retained worktree gitlink. The merge commit containing this report has the same runtime source as tested archive tree `21a54ded2ddab92d825feba91a7de616d8001c11`; documentation and retained evidence are additional metadata.

## Results and reproduction

- Win64 Development Editor: 239 actions, exit 0, 128.89 seconds. Command: `Build.bat KalmalaEditor Win64 Development -Project=C:/Users/Ville/AppData/Local/Temp/km13end/Kalmala.uproject -WaitMutex -MaxParallelActions=4`. Engine root: `C:/Program Files/Epic Games/UE_5.8`.
- Full `Automation RunTests Kalmala`: 122 successes, zero failures, exit 0. Exact engine arguments and all live commands are in [verify.ps1](verify.ps1); results are in [results.txt](results.txt) and [tests.txt](tests.txt).
- Crafting and Inventory: 1280×720 standard and 1024×768/150% text/high contrast; compact Inventory also uses reduced motion. Owner/private reconnect passed.
- Settings: both viewport cases, five option groups, 80–120% interface scale, high contrast, reduced motion, focus/input and saved-choice reload after process restart passed.
- Status: 1280×720, 1024×768/150% text/high contrast, and 2560×1080 passed. All nine textures rendered through the status widget; owner-local geometry, timers, removal and minimap separation passed on both peers.
- Minimap: rendered host/client checks passed at 1920×1080, 1024×768 and 3440×1440. Prompt available/unavailable/modal/no-target/Grinding Stone states passed at standard and compact high contrast.
- Development and Shipping BuildCookRun exited 0. Exact UAT command is in [package.ps1](package.ps1); Development used the same arguments with `-clientconfig=Development` and `-archivedirectory=C:/Users/Ville/AppData/Local/Temp/km13dev2`. Development startup/map load and all image captures passed with a fresh profile using [smoke.ps1](smoke.ps1). Shipping direct packaged executable remained alive throughout its 30-second smoke interval.
- All 48 catalogue and nine status source/prepared/package validators, documentation/input/presentation/accessibility checks and status-runner parsing passed; see [static.txt](static.txt). Mirror max path: 214 characters; package roots: 148. Retained scenario output paths were also checked below 260.

[acceptance.json](acceptance.json) retains the exact tree, temp roots, successful scenario transcripts and limitations. [source-hashes.json](source-hashes.json) records 701 runtime project files from the tested archive. Main source/content/config/runner comparison passed for 653 files; 56 text files differ only by checkout CRLF versus archive LF. Verify final source equivalence with `git diff 21a54ded2ddab92d825feba91a7de616d8001c11 HEAD -- Source Config Content Scripts Plugins Kalmala.uproject`.

## Defects and reruns

The gallery advertised 52 identities while three fixed 16-row partitions displayed only 48. Four complete partitions now display all 52, covering 48 catalogue textures plus four support-scroll aliases. Separate read-only Mending, Bear's Vigor and Deer Call snapshots complete the six-image status fixture. No original asset was missing or reimported, and no gameplay request or effect is activated by these fixtures.

The first smoke harness looked for `-gallery.png` rather than the actual `-icons.png`; a later monolithic LogTemp-marker assertion failed despite rendered images. The successful fresh smoke uses actual captures plus engine initialization/map-load evidence, with native images visually reviewed. Editor peer checks retain explicit geometry/marker assertions. A compact crafting startup timed out while editor platform validation waited behind a concurrent build; builds and live checks were serialized, and the complete final matrix reran successfully. No failing required check remains.

## Rendered evidence and scope

Native package gallery/status captures, host/client Build, Inventory at standard/compact scale, Escape/Settings, status at three viewports, minimap and prompt samples are retained here. They were inspected for visible images, named consumers, focus cues, opaque/transparent surfaces, timer placement and clipping/scroll behavior. All complete native captures and logs remain in the manifest's temp roots.

This is scripted/offscreen acceptance, not physical device, audio, or an unscripted player journey certification. Shipping smoke establishes startup/process survival; complete packaged raster coverage is from Development. Existing M0–M12 acceptance limitations remain. M14 source integration is separate and was not performed. No source-generated Unreal directories, saved-data schemas, network service commitments or CI/release configuration changed.

The working tree also contains a separately authorized, unstaged Esc-home border change made during this run. It and its handoff notes are preserved and excluded from this commit and acceptance; its own build/render verification remains deferred.
