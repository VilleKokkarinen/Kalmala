# Shared player inventory and numbered hotbar

The owner's 2026-10-09 request supersedes the earlier sixteen-slot pack and
separate carried-tool presentation. The player has one ten-column, four-row
inventory. Materials, meals, kits and tools all occupy this same forty-cell
space; tool condition/level records are metadata for those cells, with no
additional toolbelt capacity. Existing station and chest inspection panels
read the same holdings and do not create another player inventory.

The upper row is labelled `1 2 3 4 5 6 7 8 9 0`. Its cells are the hotbar.
Moving or swapping an item into a numbered cell assigns that number directly;
moving it into a lower row removes its hotbar entry. Exhausting a stack leaves
its cell empty and preserves every other cell's position. New holdings enter
the first free cell. Existing starting tools occupy ordinary cells. Tool
upgrades retain the replaced tool's cell.

Tab / I opens the panel at the upper left. Drag between cells to move or swap.
Arrows / D-pad navigate all forty cells, including empty cells; Enter / A picks
up and places, and Escape / B cancels a pending keyboard move before closing
the panel. The selected item retains its shared description, food-use action,
and applicable free selected-tool repair. Search, category, sorting, and the
separate tool section are removed from this player panel. Its outer scroll
retains access to descriptions/actions on small viewports and at larger text
scales. The grid remains ten by four while fitting the panel width.

Normal gameplay shows only occupied numbered cells in a compact upper-left
row, preserving their original numbers in `1` through `9`, then `0` order.
For example, occupied cells 1, 4 and 0 render three entries labelled 1, 4, 0.
The row hides during inventory and other movement-blocking modals, and when
all ten cells are empty. Original catalogue icons, quantities, condition bars,
and an active-item underline remain visible. Number keys select the server's
item in that cell; supported food uses the existing validated meal transaction.
An active tool supplies subsequent harvest intent, which the server still
independently validates. The M13 support-scroll integration supersedes the
earlier F1–F4/Q support-effect selection: a reusable scroll item occupies an
ordinary inventory cell, can be assigned to a numbered hotbar cell, and is
activated from that cell. Hotbar use sends the cell index only; the server
derives the current item and its allowlisted effect, checks learned entitlement
and existing activation gates, then routes through the support-magic authority
path. Scrolls are not consumed; the existing server cooldown controls reuse.
Each accepted support cast starts the shared, server-owned five-minute cooldown.
Hearth Shield, Bear's Vigor and Deer Call last three minutes; Mending is
instantaneous. Deer Call's bounded wildlife influence also lasts three minutes.
Only one support effect may be active per caster at a time. The current
controller D-pad/Q support-magic bindings are retired when this integration is
complete.

Below the grid, show one compact line such as `Armor 0 · Weight 8/300`.
Current armor remains 0 because the game has no armor-equipment records. Carry
weight includes stack quantities and tools. Current prototype defaults are
1 kg per item unit, 2 kg per tool, and a 300 kg displayed capacity.
`FKalmalaItemDefinition::WeightKg` is an optional validated catalogue value
and capacity is a component class default. These are presentation metrics; this
increment introduces no encumbrance penalty or weight-based transaction
rejection. Armor items, equipment rules, and final weight balance require
separate gameplay work.

## M13 inventory crafting companion

The 2026-10-10 UI request extends Inventory's existing Tab / I modal. Opening
it shows the same 10x4 inventory on the left and the standard player crafting
list and selected-recipe details on the right. The right panel reads the
existing owner-visible recipe and ingredient presentation; crafting continues
through the existing server-validated action. Station-specific service views
and the separate Build menu keep their current contexts. After implementation,
the presentation rules below supersede the selected-detail and scroll
presentation described earlier in this handoff; the grid and authority
contracts remain in effect.

On open, the inventory panel slides down from above and the crafting panel
slides in from the right. Both use the existing options-opening duration and
easing by default, and both appear immediately when opening animation is
disabled or the owner's Reduced motion preference is enabled. The panels are
one owner-local modal: they share existing close, cursor/input restoration, and
keyboard/controller focus behavior. Layout, focus, reduced motion, transition
start/interruption/completion, and recipe-action visibility are part of M13's
rendered host/client acceptance at supported viewport and accessibility
settings.

Keep the inventory's always-visible summary to the compact armor/weight line.
Do not show the slot-count line, a `Selected:` heading, control instructions,
or a persistent full-width item-detail card. Hovering an occupied cell opens a
nearby, dark, opaque detail tooltip with the item name, description and any
existing authored stats arranged as tidy, separate lines. Keyboard/controller
focus or selection on an occupied cell shows the same contextual details so
the information remains reachable without a pointer. Existing item actions
remain available only for applicable items and retain their current
server-validated path.

One original Kalmala wood texture spans the complete combined Inventory and
Crafting backplate, including the narrow inset and gap around and between the
panes. The Inventory pane and Crafting pane each use an opaque solid theme
fill. Inventory cells and crafting cards use opaque, slightly darker brown
rounded surfaces with a few UI units of gap and icon inset. Keep canonical
icons, quantities, condition bars, focus states and selected states legible
over those fills. Existing high-contrast behavior suppresses decorative wood
art and uses opaque black accessibility surfaces, white text and borders.

This is a UI presentation change only. It adds no recipe, crafting outcome,
RPC, replicated field, gameplay authority, or saved-data field. Tooltip stats
are limited to existing catalogue/tool data; this task adds no combat-stat
fields such as Pierce, Poison or Knockback.

## Authority and compatibility

The server reconciles trusted item stacks and tool records into one runtime
cell array, retaining valid positions and removing depleted identities. Cell
positions and active identity replicate with `COND_OwnerOnly`. A move RPC
carries only source/target indices and their expected identities: bounds,
current server ownership, stale contents, and request cadence are checked
before swapping. It cannot grant, delete, split, duplicate, repair, or author
quantities. Hotbar intent carries only an index in 0–9; the server derives its
identity and validates food through the existing transaction path.

Live grant, exchange, storage withdrawal and tool progression account for
both tools and stacks within forty cells. World chest capacity remains sixteen
stacks; its persistence validation and schema stay unchanged. No saved-data
field or version changes. Grid arrangement and active selection are session
state; reconnects rebuild arrangement from available holdings. Existing item
and tool persistence limitations still apply.

## Verification handoff

This is one user-directed development increment after the completed roadmap,
not milestone-final verification. Lightweight source/API inspection, input and
presentation-ownership audits, retired-panel audit, changed PowerShell parser
checks, whitespace and MAX_PATH checks were performed. The UE5.8.2
`KalmalaEditor Win64 Development` build now passes after the follow-up repair
recorded in `docs/42-build-repair.md`. Automation, rendered UI, physical input,
and multiplayer execution remain deferred.

New automation covers shared capacity, stable gaps, depleted hotbar cells,
duplicate/overflow rejection and compressed number order. Selection and RPC
contract coverage were updated for the simpler panel. The prepared rendered
`Scripts/Verify-InventoryMenu.ps1` now captures filled/detail views, assignment
to key 0, removal into row two, meal use/repeat, live quantity refresh, depleted
materials with tools retained, and the normal HUD. These updated Unreal tests
and captures have not been executed. After an authorized verification run,
execute the inventory gameplay/UI automations and this helper at
1280x720/100%/contrast0 and 1024x768/150%/contrast1, then inspect both peers'
captures and physical drag/controller input before claiming rendered acceptance.
