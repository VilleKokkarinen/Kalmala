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
