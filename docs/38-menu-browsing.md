# Local menu browsing

The first M11 browsing increment extends interactive inventory inspection in
the existing crafting/build modal. The read-only gameplay HUD remains unchanged.
Only the owning pawn's supplied inventory/tool rows can become search results;
the widget never enumerates absent catalogue entries or queries a peer.

Search matches the visible item name, case-insensitively, after trimming outer
whitespace, using at most 64 characters. It does not search quantities, private
state, descriptions, canonical IDs or hidden content. The category control
cycles All, Items and Carried tools using the supplied carried-tool flag.
Sort cycles Owner order, Name and Category / name. Name comparison ignores case,
with canonical identity as a deterministic tie-breaker. Category sorting labels
Items and Carried tools and sorts names within each group. Owner order is the
original supplied order; filtering/sorting copies rows and never rearranges
inventory storage or saves.

Tab reaches search and the labelled controls; buttons accept normal activation.
While inspection is focused, Page Up / controller left shoulder cycles category,
Page Down / right shoulder cycles sort, and arrows / D-pad select visible entries.
Text editing keeps its cursor keys. Existing Escape/B closes the modal. Controls
retain their widget identity and focus during refresh; a still-visible canonical
selection survives filter/sort/data refresh. If selection disappears, the first
visible result is selected. No results clears selection, hides details and names
Clear search / All as recovery options. Empty inventory has distinct feedback.
Previous/next disable when no rows are visible; browsing never performs an item,
craft, repair, construction or transfer action.

The shared local theme, text scale and contrast style labels, buttons and search
text. Browse controls are vertically arranged inside the existing scroll view.
Filters/query/order live only on this widget; cross-menu or reopen persistence
belongs to the later remembered-menu task. Controller text entry uses Unreal's
existing editable text control; no custom virtual keyboard or binding is added.

Increment verification: compile the affected UI module in the short disposable
mirror with normal UnrealBuildTool access, then run Kalmala.UI.Inventory with
isolated UserDir/logs and the null renderer. LocalBrowsing covers search, actual
text-change delegate binding, filters, sorting, canonical selection, grouped
navigation, empty/no-results, owner refresh and unchanged source order at 150%
high contrast. The rendered Verify-Crafting probe additionally routes shoulder
keys through real Slate focus and requires category/sort, no-results and recovery
on both peers alongside the existing modal/transaction checks. Run presentation
ownership, five M5 documentation contracts and diff/path checks.

Crafting/cooking browsing and named build groups remain separate ordered children.
Full parent suite, rendered browsing-control layout at all supported scales,
physical controller text entry and packaging remain pending.

Accepted increment evidence, 2026-10-04: final affected UI compile/link passed four actions with normal UnrealBuildTool access. Focused inventory queue passed 5/5, exit0 at C:/Users/Ville/AppData/Local/Temp/kb1/final.log (before final contrast-only refinement). Initial port18583 peers crashed because UE5.8 SetWidgetStyle retains the supplied argument address in Slate; storing the style on the widget fixed it and port18584 passed. Final rendered port18585 passed after source-PNG inspection prompted search-background contrast and Clear search wrapping repairs. Both peer logs contain CategoryKey=1 SortKey=1 NoResults=1 Restored=1, FocusAndKeys=1 and Hidden=1 Restored=1. Final captures/logs are under C:/Users/Ville/AppData/Local/Temp/KalmalaCrafting-09a2058ab331410d9133d1cd7b2be721. Standard host inspection source PNG reviewed with browsing controls and complete selected details visible. Ownership/five documentation contracts and diff/path checks passed; mirror maximum209. Representative inspection does not establish full scale/contrast acceptance.
