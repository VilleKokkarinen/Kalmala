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

This is the first child increment, not interactive parent acceptance. The HUD
and its catalogue remain HitTestInvisible, so attaching a tooltip alone does
not give the player hover/focus access. The next child must establish selected
and focused slot access, action guidance and a suitable local interaction flow
without stealing ordinary gameplay input. Removal refresh and rendered layout
acceptance are also outstanding. No new action is introduced by this foundation.

Increment verification: compile the affected UI module in the short disposable
mirror with normal UnrealBuildTool access; run `Kalmala.UI.Inventory` with
isolated UserDir/logs and the null renderer. `ItemDetail` checks every catalogue
description, supplied/absent state, unknown identity and actual widget-tree text
binding at 150% high contrast. Run presentation ownership/documentation audits
and diff checks. Parent verification will require rendered selected/focused
access, removal/consumption, host/client privacy and the applicable full suite.
