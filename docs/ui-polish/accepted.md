# M11 UI-polish acceptance — 2026-10-03

Historical M11 visual acceptance record. The arrival card shown in these
captures was removed from current gameplay by M12; the screenshots remain as
evidence of the earlier accepted presentation.

The user explicitly requested finishing the four blocked children together.
This review supersedes the earlier blanket missing-glyph blocker annotations
for HUD, crafting/construction and equipment. The source PNGs and lossless
integer crops contain the labels that some reduced review previews omit.
No Slate renderer, thread or graphics-backend defect was established or fixed.

## Improvements and verification

- HUD: existing themed/scaled support glyphs and explicit labels accepted in
  normal standard and 150% high-contrast owner views. Fixed a real 1024x768
  overlap by moving the arrival card clear of the 340-logical-unit left HUD;
  larger viewports retain their centred placement.
- Crafting/construction: existing card spacing, fill alignment and separate
  availability/selection accepted. Row action labels now wrap inside their
  own buttons at 150% text scale. Corrected obsolete station-kit, retired-chest
  and session-only attachment messages to match current raw-material and save
  contracts. No transaction or persistence behavior changed.
- Equipment: existing two-column cards with bold names, separate level,
  condition and READY/DAMAGED/BROKEN labels accepted. Unknown state remains
  unavailable. Full/damaged/broken/invalid states pass automation; the rendered
  fixture shows intact tools only. EquipmentView is developer-only and does
  not certify physical-device access to the full HUD's scrolled region.
- Settings: previous two-line Settings labels remain readable at 150%.
  Fixed a verifier error: its fixed (1100,360) backdrop sample was inside a
  button in a 1600x900 screenshot. The outer-gutter sample uses 97.5% of the
  captured width and 50% of height; all ten earlier captures pass it, including
  the capture falsely rejected at RGB 93,111,121.

Verify-Crafting now retains six PNGs per rendered run: top grid, selected
requirements, and repair/progression feedback on both peers. It requires both
review-scroll stages to succeed in addition to existing grid navigation,
keyboard/D-pad focus restoration, HUD suppression/restoration, server gates,
payment/atomicity, privacy and matching-fire checks. Scrolling is a read-only
developer capture probe; it does not trigger actions or change gameplay input.

## Fresh evidence

All roots below are under C:/Users/Ville/AppData/Local/Temp.

| Check | Port | Root | Result |
| --- | --- | --- | --- |
| Normal HUD/inventory, 1280x720, 100%, standard | 18560 | KalmalaInventory-4ae3ccc9a84e44cda0fbecf0a0dafc2f | PASS |
| HUD after overlap fix, 1024x768, 150%, high contrast | 18565 | KalmalaInventory-fef00d58f8a143df98bf2af550f8e5b6 | PASS |
| Equipment, 1024x768, 150%, high contrast | 18563 | KalmalaInventory-3a7d4bddcd494a6c8feaf217186e42e8 | PASS |
| Equipment, 1280x720, 100%, standard | 18566 | KalmalaInventory-3441905a828a4783a269e2d4a65a697f | PASS |
| Final crafting top/details/feedback, standard | 18574 | KalmalaCrafting-e5e61a16ac2740b0a69aa11f5e5f7a48 | PASS |
| Final crafting top/details/feedback, high contrast | 18575 | KalmalaCrafting-5436f788602542b0a5b1937ee43430de | PASS |
| Settings, all five views and interrupted opening/resize | 18572 | KalmalaSettingsAccessibility-76cad8dc47fe45ec92197629bcfcd3d7 | PASS |
| Status empty/six-effect/removal/details/icon gallery | 18573 | kh-62e29365 | PASS |
| Map input/privacy/modal regression, 1024x768 | 18576 | KalmalaWorldMap-08aaacecbbad4312b7c123b0db6c3984 | PASS |

Both peers' top/detail/feedback views were inspected. Representative original
PNG evidence and lossless crops are retained in accepted/. In the final repair
label rectangle (225,440,565,108), each peer has 8,429 pixels whose RGB channels
all exceed 150, with zero mask disagreement. Standard equipment rectangle
(26,305,195,145) has 849/850 such pixels and one disagreement; this is not a
claim of whole-image equivalence. Crops confirm complete labels but cannot
make native text larger. Native tool text remains compact; use text scaling.

## Acceptance scope

Parent-level acceptance applies to the current workspace, including preserved
earlier uncommitted polish candidates. Only this turn's new fixes, verification
probes and handoff are committed; earlier changes are not silently staged.
Do not interpret this acceptance record as a clean-HEAD or packaged release.

Editor build and full automation results are recorded in PROGRESS.md. No new
gameplay authority, RPC, replicated field, save schema, item ID or transaction
was introduced. Physical keyboard/controller hardware, packaging/cooking and
every viewport/aspect ratio remain outside this editor verification. This
closes only the existing UI-polish parent, not the remaining M11 features.
