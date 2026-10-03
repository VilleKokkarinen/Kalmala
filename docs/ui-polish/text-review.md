# Source-pixel text review

Use `Scripts/Export-UITextCrop.ps1` before diagnosing missing glyphs from a
reduced screenshot preview. It copies a bounded rectangle into integer-sized
pixel blocks, records the source SHA256, rejects existing outputs and paths of
260 characters or more, and verifies that the source was not changed. It does
not interpolate, reconstruct text, alter the original screenshot, or certify
layout acceptance.

Example from the project root:

```powershell
Scripts/Export-UITextCrop.ps1 -Source docs/ui-polish/text-review/hud-Client.png `
  -Output "$env:TEMP/hud-review.png" -X 20 -Y 25 -Width 225 -Height 170 -Scale 3
```

## 2026-10-03 diagnostic increment

No persistent C++, renderer, configuration or gameplay change was made.
The current checkout UI source files matched the existing disposable `ka`
mirror by SHA256. Its UE 5.8.2 editor build succeeded up-to-date (zero actions),
with normal UnrealBuildTool user-directory access. This is not a new compile
or parent-level build gate.

Fresh rendered peer fixtures all passed their behavioral/log checks:

- Port 18550: inventory/equipment, 1280x720, 100%, standard contrast;
  `KalmalaInventory-dd649734854a4881b0ecd61409b6b58e`.
- Port 18551: normal inventory/HUD, 1024x768, 150%, high contrast;
  `KalmalaInventory-5a1b4eb57c75448aa1cda7d7698d5d3b`.
- Port 18552: crafting/build, 1024x768, 150%, high contrast;
  `KalmalaCrafting-d73b9b6f402a4ca58024dd451c6ec352`.

These evidence roots are under `C:/Users/Ville/AppData/Local/Temp`. They exercise
the preserved, uncommitted UI candidates as well as HEAD, not an isolated
proposal containing only this diagnostic helper. None of those candidates is
included in this commit or accepted by this diagnostic increment.

Source-pixel enlargement shows complete equipment names/condition lines and
HUD support labels in inspected retained and fresh captures. Some review
previews nevertheless appear to omit text. The fresh host and client MEND
rectangles `(27,55,48,19)` are byte-identical, each with 267 bright pixels.
Across `(20,25,215,170)`, both support regions have 3,738 pixels with all RGB
channels greater than 150, with zero disagreement in that binary mask.
The four retained PNGs in `text-review/` preserve this paired source/crop
evidence. The crop rectangle is `(20,25,225,170)`, enlarged 3x.

This contradicts a claim that those saved HUD PNGs omit different text on
each peer. It does not prove every historical image or view is correct, or
identify a Slate renderer defect. Crafting heading/instruction masks differ
at 36 pixels in `(220,45,570,450)`; no equivalence claim is made for that view.
Earlier blanket missing-text conclusions need source-pixel reassessment before
another engine/thread/backend repair attempt.

## Helper verification and remaining acceptance

PowerShell parsing passed. Every output pixel in the 4x retained equipment
crop matched Pillow's nearest-neighbor enlargement of the source rectangle.
Negative rectangle and existing-output cases were rejected without creating
or replacing output. Presentation ownership, all five M5 documentation
contracts, and `git diff --check` passed.

Full view-specific acceptance, high-contrast equipment capture, standard
crafting regression, scrolling/detail review, settings regression, full
parent integration, physical inputs and packaging remain pending. Reassess
the existing blocked candidates individually using original pixels and
truthful layout/focus/accessibility criteria. Do not use an enlarged crop to
claim that tiny native-size text is comfortably readable.

The helper has no networking, gameplay, input, replication, transaction or
save-state effect.
