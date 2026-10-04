# Shared inventory item details

The M11 foundation is `UKalmalaItemDetailWidget`, attached as the detail tooltip
of populated `UKalmalaCatalogueRowsWidget` cards. It uses the canonical item
description and assigned icon (64 pixels versus the card's 28/32 pixels), plus
the caller's existing visible count or tool condition text. Empty cards retain
their empty-slot explanation. Unknown identities say Description unavailable.
No weight field exists in the current item definition; no weight is invented.

The panel applies the shared local theme, text scale and high-contrast mode.
It exposes no mutation, RPC, private peer query, or saved selection. Reusing
SetItem replaces the displayed description/state rather than appending old data.

The HUD remains HitTestInvisible. Interactive details are available through
the existing build/craft modal: focus and activate Inspect inventory (Tab and
Enter, or controller navigation and A), which focuses the inspection panel and
scrolls it into view. Previous/next controls also accept pointer activation.
While inspection or its controls have focus, arrows and D-pad select the
previous/next owner-supplied slot with wrapping. A > marker and bold name identify
selection; the shared detail panel shows that slot's description, icon and
supplied state. Tab continues through other modal controls; Escape/B closes
the existing modal and restores its normal gameplay input. No new gameplay
binding is added, and modal craft/build shortcuts yield to focused inspection.

Rows come only from the owning pawn's existing inventory stacks and carried
tool snapshot. Canonical selection survives a refreshed row order. The panel
explains existing meal, repair, construction and recipe/storage controls rather
than dispatching an action or inventing weight/statistics. Comprehensive
removal/consumption, rendered layout/contrast and privacy acceptance remain in
the final child; the parent is still incomplete.

Increment verification: compile the affected UI module in the short disposable
mirror with normal UnrealBuildTool access; run `Kalmala.UI.Inventory` with
isolated UserDir/logs and the null renderer. `ItemDetail` checks every catalogue
description, supplied/absent state, unknown identity and actual widget-tree text
binding at 150% high contrast. Run presentation ownership/documentation audits
and diff checks. Parent verification will require rendered selected/focused
access, removal/consumption, host/client privacy and the applicable full suite.

Interaction-child verification (2026-10-04): seven-action affected UI
compile/link in the short ka mirror passed with normal UnrealBuildTool access.
Kalmala.UI.Inventory passed 4/4 including InspectionNavigation (150% high
contrast, canonical selection and supplied tool guidance), exit 0; log
C:/Users/Ville/AppData/Local/Temp/ki2/pass.log. Rendered Verify-Crafting at
port 18571 passed real Slate inspection focus, keyboard Right/controller Left
selection and modal restoration on both peers, plus the existing recipe-grid
navigation and authoritative transaction checks. Evidence root:
C:/Users/Ville/AppData/Local/Temp/KalmalaCrafting-1b869647b1aa4583a1222af6398e92d6.
The interrupted run's null-renderer failure was a zero scroll extent; this
rendered run reports Scrollable=1 on both peers. The captures are existing
crafting review stages, not item-detail visual acceptance. Full suite, physical
input hardware, package verification and parent-level integration were not run.

Final dependency repair: staged review found an older uncommitted BuildToolDetail helper reference. Replaced it with bounded condition formatting within this increment (invalid state says Condition unavailable). Final four-action UI compile/link passed; repeated rendered peers at port 18572 passed, evidence C:/Users/Ville/AppData/Local/Temp/KalmalaCrafting-2f78cb0e94dd4d559384a6e763c0e564. No older helper or equipment hunk adopted. Final rendered artifact paths max 188 characters.

Acceptance attempt 2026-10-04 (uncommitted): added selected meal count decrement,
final-item removal/fallback, stale-action removal and independent-owner widget
assertions. Added required peer -inspection.png captures and a geometry check.
Inspector/shared-detail roots now build before Slate construction; TakeWidget
before first refresh is tested. Detail titles wrap. No gameplay/RPC/save change.

Final UE5.8.2 build passed ten actions with normal UnrealBuildTool user-directory
access. Full Kalmala queue passed 104/104, failed=0, process/test exit=0 at
C:/Users/Ville/AppData/Local/Temp/ki3/last.log. Network inventory privacy port18577
passed; root KalmalaInventory-810a378450c340fab3f92a5614105357. Ownership, five M5
documentation contracts, crafting script parsing and diff checks passed.
Consumption/removal coverage uses supplied-row widget fixtures, not a rendered
meal-input playthrough. Production rows query only the owning pawn snapshots.

Three rendered attempts failed visual acceptance despite passing peer runners:
1. Ports18573/74: inspection PNGs show repair/progression. Roots
KalmalaCrafting-121900cb68c34baf963c71d768250406 and
KalmalaCrafting-8c20aa584a7a439fb43fbd905da630b7.
2. Ports18575/76: geometry scroll still misses the detail surface. Roots
KalmalaCrafting-20e9694dfe8d4c5ba47b78d581fcb0e8 and
KalmalaCrafting-c1d39827ff46436fb550a0cfda946bf1.
3. Ports18578/79 after root construction repair: standard peers now show slots,
selection and icon, but detail condition/action text is clipped below the view;
1024x768 150% high-contrast inspection PNGs still show progression. Roots
KalmalaCrafting-47be1d59de89459faddcf3745f7333a2 and
KalmalaCrafting-eaf5fd1a91b54a40a8055a02551e06f5.

All roots are under C:/Users/Ville/AppData/Local/Temp. Both final peers' original
inspection PNGs were inspected. Scrolled=1/nonzero geometry cannot establish
visible readable text. Three-attempt stop reached; child BLOCKED, no commit or
parent acceptance. Next repair must reliably expose the full selected detail
card at both scales. Physical input/package/clean-HEAD integration unverified.

Resumed acceptance 2026-10-04T08:02:17.2467829Z — parent complete

User requested continuation of the same blocked child. Inspection is the last
scroll child; the focus button and capture probe now use deferred ScrollToEnd,
which resolves content extent after layout. Inspector styling follows the
menu-wide style pass. Complete selected detail is reachable through the existing
modal and its arrow/D-pad selection path; no new input binding or action added.

Final parent-level verification: UE5.8.2 KalmalaEditor Win64 Development build
passed four actions with normal UnrealBuildTool access; full Kalmala queue
passed104/104, failed0, process/test exit0 at
C:/Users/Ville/AppData/Local/Temp/ki4/all.log. Rendered Verify-Crafting passed
port18580 (1280x720 100% standard) and18581 (1024x768 150% high contrast), including
focus/key navigation, modal restoration, server transactions and four capture
stages. Exact roots under Temp:
KalmalaCrafting-8174d20fb50a4bfb8329b21f159133b6 and
KalmalaCrafting-6bcb6ad8ebe74ac383d789ee2332019b.
Final Verify-Inventory port18582 passed owner/remote privacy and read-only state;
root KalmalaInventory-043222dc457a4b62b8be49a9c5484786. Ownership, all five M5
contracts, script parsing and diff/path checks passed.

Accepted source PNGs are in item-detail/std-host.png, std-client.png,
hc-host.png and hc-client.png. Both standard cards fit icon, description fallback,
level/condition and existing repair guidance above Close. The high-contrast
cards fit the same data and instruction/slot controls. Lossless crops use
x224,y435,w567,h246,scale2: crop-host.png and crop-client.png have identical
RGB bytes and SHA256 7565A847917CEBAC358D19B81C3350DA4F92796AAA11C0CC0D1172ABE8849461.
Native detail masks have6593 bright pixels each and zero differences. A blank
client preview is contradicted by those saved pixels; no renderer fix claimed.

Earlier three-attempt blocker is resolved in this resumed run; parent/child
checked only after all final checks. Removal/consumption and independent-owner
panel refresh are widget automation; canonical catalogue descriptions and
fallbacks are automated. Reed Knife has no catalogue description, so its truthful
fallback is retained. No weight/statistics invented. Physical input, packaging,
exhaustive viewports and clean-HEAD integration are not claimed; mirror includes
preserved earlier uncommitted UI candidates. Main checkout, no handoff sync.
