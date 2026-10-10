# M14 final acceptance — 2026-10-10

M14 passed on source commit `1a411c6d2b76b3d9b4bd438b8449632fd5bac1f4`, branch `codex/m14-fix`. This is the isolated integrated acceptance tree. Main and M13 are not marked complete or integrated by this result.

The candidate merges the complete M14 chain through `38371b512c7d919eea58cb12e6d1fa04079aed37`, committed M13 repairs `5ac3b05d197692a8ef6fd2f7be05234f40c60c7a`, and committed main/docs baseline `fe7d28228522fcee0966fa06c2a828a5f17beefb`. Integration commit: `d5e1141cb05193f66cede3be96b6fc735d22296b`. No uncommitted M13 or main implementation files were copied.

The former compile blocker was the older committed M13 crafting UI. Integrating its committed repair allowed the full editor target to link. The existing Slate Fill-weight repair retains 0.43/0.57. Final acceptance also repaired a genuine marker-caption overlap at 150% text by measuring fonts and sizing the palette and its pointer targets together.

| Gate | Result |
| --- | --- |
| KalmalaEditor Win64 Development | PASS: initial 239-action target build; final five-action target rebuild after repairs |
| Automation RunTests Kalmala | PASS: 123 successes, zero failures, process exit 0 |
| Focused Kalmala.UI.WorldMap suite | PASS after palette and assertion repairs |
| MapAwareness | PASS: private peer positions, both-owner global relay, world bounds, cooldown, order, ten-second expiry |
| WorldMap, three standard viewports | PASS: 1024x768, 1280x720, 2560x1080, both peers |
| WorldMapTiles | PASS: identical seed-418 fingerprints despite conflicting client seed; personal coverage reload and remote fog after restart |
| WorldMapProfile | PASS: retained open, worker, tick, cache, and late-join metrics |
| Small high-contrast / 150% text | PASS: both captures inspected; distinct readable marker captions and white focus outlines |
| Wide 100/125/150% application scale | PASS: applied scale recorded, both peers captured |
| Close / maximum zoom / fitted overview | PASS: original raster treatments, clipping, fog, focus, and cache reuse |
| Changed original project theme / high-contrast override | PASS: cyan focus and BuildPanel texture probe; high contrast suppresses decoration; mirror config restored |
| Documentation/input/ownership contracts, runner parsing, diff/path checks | PASS |

The final profile reports host/client maximum map-tick work of 1.226/1.504 ms, maximum tile-worker times of 25.629/212.247 ms, and 1,001,568 cached CPU pixel bytes each. The isolated automation budget measured an 18.495 ms tile and 47.932 ms companion minimap with four tile workers. These are local development measurements, not shipping hardware budgets. Every rendered cache probe retained all ready terrain textures and the bounded cache.

[acceptance.json](acceptance.json) contains exact build/test/runner commands, pinned source/tree/blob hashes, each raw log path/hash, and all 12 passing scenario transcripts. [runtime.txt](runtime.txt), [automation.txt](automation.txt), [build.txt](build.txt), and [static.txt](static.txt) retain the relevant evidence. The 22 PNGs are unmodified native host/client captures. `biome-textures.bmp` contains actual tested samples: columns Meadows, Lakes, Elderwood, Mire, Tundra, Mountains, Ocean; rows 78.125, 312.5, 5000, 100000 cm spacing. All seven rasters remain spatially varied and distinct at each spacing.

Representative inspected captures: [standard client](map-1280x720-client.png), [150% text/high contrast](contrast-1024x768-client.png), [150% interface](scale150-2560x1080-client.png), [close zoom](close-1280x720-host.png), [fogged overview](overview-1280x720-client.png), [changed theme](theme-1280x720-host.png), and [changed-theme high contrast](theme-contrast-1024x768-client.png).

Earlier failures were retained in the disposable mirror `C:/Users/Ville/AppData/Local/Temp/km14fix`: stale blank-label and RGB assertions; the initial rejection fixture's punctuation assumption; the old zero-ping initial sample; and capture-harness member naming, PowerShell parameter collision, Unreal INI syntax, and Finnish decimal validation. All affected checks were repaired and rerun. The 150% caption overlap was found through rendered inspection, repaired, regression-tested, and recaptured. Final required gates passed; no unrelated pre-existing failure remains in this M14 acceptance result.

Source worktree generated directories stayed absent. The mirror's maximum path was 214 characters; retained runner roots were below 200, and committed paths are below 260. UnrealBuildTool used its normal writable local directory. Gameplay authority, RPCs, balance, durable schemas, world generation, and map-sharing scope were unchanged by this repair. No later milestone was started. Main M13 integration and its acceptance remain separate work.
