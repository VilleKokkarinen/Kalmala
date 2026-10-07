# Crafting/cooking modal and left HUD

> Superseded by the M12 left-panel removal: the persistent inventory HUD and
> its crafting suppression/restoration hooks no longer exist. This page records
> the earlier overlap acceptance fixture as historical evidence.

Opening the owner's construction/crafting or cooking-station menu collapses
the left inventory/support HUD. Closing the menu restores its previous role as
a hit-test-invisible HUD. The hidden HUD continues reading owner-visible state,
so returning to gameplay displays current inventory and support feedback.
Suppression is retained if the HUD widget is created/recreated while the menu
is open. Crafting teardown calls the ordinary close path to restore it.

This prevents the centered menu covering the left HUD at narrow viewports and
large text scale. Minimap and top-right status presentation remain visible.
The change affects local visibility only: no RPC, server transaction, inventory
state, input binding or persistence schema changes.

The existing rendered crafting fixture now requires suppression while open
and restoration on close as part of its presentation result. Both peers log
`Crafting HUD overlap: Hidden=1 Restored=1`. Verify at 1280x720/100% standard
and 1024x768/150% high contrast, including keyboard/D-pad navigation, scroll
extent, modal/input restoration, and existing authority/payment rejection gates.
Review both peers' captures for absence of the left HUD behind the menu.

This is a focused overlap fix. It does not complete the broader HUD/crafting
polish children or establish that every previously reported glyph fragment has
the same cause. Their uncommitted candidates and failure evidence are preserved.
Cooking uses the shared open/close path, but a separate rendered cooking-station
walkthrough, physical controllers, packaged rendering and parent acceptance
remain pending. See PROGRESS.md for exact results and retained images.
