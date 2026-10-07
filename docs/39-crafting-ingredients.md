# Crafting ingredients and requirements

The first ordered child adds read-only ingredient rows below the selected
recipe description in the existing crafting/build scroll view. Each row reuses
the canonical catalogue icon and name and states `owned N / required M`, with
explicit Enough or Missing text. Requirements describe one request, matching
the panel's batch-one action. Ordinary recipes use their listed Ingredients;
direct builds resolve BuildDirectMaterialCost rather than showing kit costs.
The hearth's additional one-of-four ignition fuel remains in its existing
description; these rows describe raw construction costs, not that alternative.

Only the owning pawn's existing inventory component supplies quantities. A
missing component reports owned pending / Waiting for pack. An absent stack
on an available component reports zero. NativeTick refresh follows accepted
inventory replication and selection/filter changes; no results clears rows.
Unchanged identity/count/scale/contrast snapshots retain widget rows to avoid
per-frame reconstruction. Rows introduce no focusable control and use shared
theme icon dimensions, spacing, typography, text scale and contrast.

No gameplay request, inventory mutation, server availability rule, RPC,
replicated field, catalogue identity or saved-data contract changes. Enough
means sufficient quantity for that ingredient only; the server independently
validates stations, tools, heat, placement, inventory capacity and transactions.
Existing description and availability text remain. Structured requirements and
combined parent acceptance are later children of this same task.

Increment verification: compile affected UI in the short disposable mirror
with normal LOCALAPPDATA/UnrealBuildTool access. Run
Kalmala.UI.Crafting.IngredientCounts, LocalBrowsing and
Kalmala.Gameplay.Crafting.Transactions with isolated UserDir/logs.
IngredientCounts exercises authoritative grants/consumption, independent owner
inventories, pending/no-selection states, every current recipe/direct-build
ingredient icon and non-mutation. Run rendered Verify-Crafting for real Slate
scroll geometry, existing owner menu navigation/modal and transaction checks;
run presentation ownership, five M5 documentation contracts and diff/path checks.
Full parent suite and combined supported-scale pixel acceptance remain pending.

Accepted child evidence, 2026-10-04: final affected UI build passed four
compile/link actions with normal UBT access. Focused queue passed3/3,
process/test exit0 at C:/Users/Ville/AppData/Local/Temp/kic3/tests.log.
Initial compilation's member-shadow warning was fixed. First test lacked
authoritative actor owners; the second exposed first-refresh widget initialization.
Both were repaired; the final queue covers those paths. Rendered Verify-Crafting
18599 passed both peers, all required captures and existing modal/transaction
checks at Temp/KalmalaCrafting-71635fa89be1455085f9745e9c35c3a9.
Host/client source detail PNGs show both ingredient icons and missing counts;
unchanged copies are retained in ingredients/host.png and client.png.
This rendered run preceded the final initialization fix affecting freshly
NewObject-created test widgets; normal menu-created widgets were initialized.
The earlier null-renderer18598 attempt failed Scrollable=0/ScrollEnd=0.0;
rendered mode supplies real geometry and passed. Ownership, five documentation
contracts and diff/path checks passed; mirror maximum209.

Limits: only standard1280x720 ingredient source pixels reviewed. Consumption
and different-owner quantities use transient authoritative widget fixtures;
no rendered consumption or physical-input acceptance claimed. Structured
requirements, combined supported scales/contrast and full parent-level queue
remain next children. No packaging or clean-HEAD verification claimed.

## Selected recipe requirements

The second child adds a themed, wrapping Requirements block after ingredient
rows. It refreshes with the same selected recipe and owner inventory as the
rest of the panel and clears on no results. It adds no focusable control.
Station alternatives use RequiredStation and canonical readable names;
station attachments add their existing matching-station and 1.25 m placement
contract. Direct builds explicitly need the owner's carried Construction Hammer
level 1 and no assembly station. Other recipes show an optional reusable pack
tool as Present, Missing or Waiting for pack; it is never consumed by this UI.

Cooking rack, cauldron and frying-pan recipes describe the lit-hearth heat
requirement even when no station is currently nearby. No new station scan or
heat query is added. The existing model's first unmet availability reason is
labelled Unavailable and updates as existing replicated/local-visible state
changes. An empty reason says no unmet requirement was reported and that the
server rechecks every request; it does not promise acceptance or placement.
Disabled recipe metadata is always explicitly unavailable. Hearth ignition
alternatives remain separate from the raw material counts.

Current FKalmalaRecipe has no skill-level or learned-recipe lock. Labels
explicitly say no recipe skill requirement / additional lock rather than
interpreting ExperienceSkill as a requirement. The separate axe-progression
actions retain their existing owner-only skill/station/tool-level details in
Tool progression; this formatter does not describe those actions as recipes.
No gameplay, transaction validation, catalogue, RPC or save changes.

Child verification: affected UI build; focused Kalmala.UI.Crafting plus
Kalmala.Gameplay.Crafting.Transactions and
Kalmala.Gameplay.Food.CookingStationHeat; rendered Verify-Crafting includes
live requirements/hammer/skill/unlock assertions in each peer's existing
presentation gate. Requirements automation checks all current catalogue
requirements, missing-station heat guidance, attachment placement, pending and
missing carried hammer, alternative stations, disabled metadata, reusable-tool
owner refresh/non-mutation and truthful empty-reason wording. The reusable-tool
branch uses a supplied presentation-only fixture; no catalogue recipe is added.
Full parent-level verification and combined supported-scale pixel acceptance
remain the final ordered child.

Requirements child evidence, 2026-10-04: UE5.8.2 affected UI build passed ten actions in short Temp/ka with normal UBT access. Focused queue passed5/5, process/test exit0 at Temp/kreq1/tests.log. Rendered Verify-Crafting18601 passed both peers, new live requirements assertions, all twenty captures, existing navigation/modal/server validation/payment/atomicity checks at Temp/KalmalaCrafting-72631bb8acbc41aebf9bb93dba0ad5df. Standard host detail source PNG inspected: heading, no-station and Present hammer text visible; remaining requirements scroll below this capture. Full requirements pixel review at supported settings remains final-child scope. Ownership, five M5 documentation contracts and diff/path checks passed; mirror maximum209, added paths below260. No required check failed. No physical-input, package, rendered station/heat transition or clean-HEAD verification claimed. Parent full queue intentionally pending.

## Combined parent acceptance procedure

The final child adds four bounded developer review views per peer: direct-floor
ingredient counts, direct-floor requirements, cooking ingredient counts and
cooking requirements. Each view selects an existing canonical recipe and
scrolls the normal menu to the relevant widget. Live assertions compare the
displayed counts to the owning pawn's inventory and check the selected hammer,
skill and heat guidance. They perform no action or inventory mutation.
Existing no-results navigation additionally requires both ingredient and
requirement text to clear. The rendered runner requires all fourteen PNGs per
peer and every review assertion;180 seconds allows the eight extra bounded
stages. Shipping menu timing/input and gameplay are unchanged.

Parent verification requires the full Kalmala queue, standard1280x720/100%
and high-contrast1024x768/150% rendered runs, owner-private inventory peers,
ownership/five M5 contracts, script parsing and diff/path checks. Review each
saved ingredient/requirement PNG for both peers. Earlier captures that merely
showed a heading do not substitute for complete section review.

## Parent accepted — 2026-10-04

The final child and ingredient/requirements parent are complete under the
existing editor-fixture scope. No production gameplay, RPC, authoritative
value, catalogue, save or input action changed in acceptance work.

- UE5.8.2 KalmalaEditor Win64 Development build passed eight actions in
  Temp/ka with normal LOCALAPPDATA/UnrealBuildTool access.
- Full Kalmala queue passed108/108, failed0, process/test exit0 at
  C:/Users/Ville/AppData/Local/Temp/kpa1/all.log.
- Rendered18602 at1280x720/100%/standard passed both peers and all28 captures:
  Temp/KalmalaCrafting-9f708b6253614164928693ebe2debe7a.
- Rendered18604 at1024x768/150%/high contrast passed both peers and all28 captures:
  Temp/KalmalaCrafting-10e933d3915348f3babe94a8ed8f25c6.
- Both runs passed existing selection/filter/no-results/focus/modal restoration,
  server rejection/payment/atomicity/final state and all new review gates.
  No-results checks require costs and requirements to clear together.
- Inventory18603 passed owner privacy, capacity, server grants/rejections and
  read-only presentation at Temp/KalmalaInventory-1334dc4ac1eb4445a8074550665c1d38.
- Ownership, all five M5 documentation contracts, PowerShell parser and
  diff/path checks passed; mirror maximum209, all added paths below260.

All sixteen saved source ingredient/requirement images were inspected: four
views × two peers × two settings. Original PNGs are retained in ingredients/
with standard-/contrast- prefixes. Direct-floor Wood/Fibre counts and icons,
hammer state, placement and no skill/unlock lock text, Cooking rack/heat,
Boar meat count/icon and first unmet reasons are complete and readable.
Long surrounding controls/details continue scrolling at enlarged text; the
fixed Close control remains available. No visual repair was required.

Limits: rendered fixtures show missing material/station states; sufficient
counts, accepted consumption refresh, separate quantities, pending states,
disabled metadata and reusable-tool branches are covered by focused tests in
the full queue. No rendered physical cooking-station/heat transition, physical
keyboard/controller playthrough, exhaustive viewport matrix, clean-HEAD or
package acceptance claimed. Other first-unmet reasons retain the existing
model contract. Earlier pending statements above are historical; this parent
is now accepted, while the later complete M11 milestone acceptance remains open.

## M12 construction detail template

Direct construction details use the existing result description once, followed
by the existing owner-local ingredient counts and a compact Build requirements
block. It names the carried Construction Hammer state, clear/dry/gently sloped
placement, and the hearth's one-of-four raw-fuel requirement and 60-second
starting duration. The current first blocker appears once when present; a
ready recipe does not add success, skill, unlock, request, or rejection prose.
The result name and material totals stay in their existing header and ingredient
rows. Catalogue descriptions and all recipe, cost, placement, authority, and
save behavior remain unchanged.

The `Kalmala.UI.Crafting.Requirements` contract covers each direct build's
placement and single-blocker copy, the hearth fuel summary, pending/present/
missing hammer states, and omission of generic boilerplate. The rendered
Verify-Crafting presentation gate checks the selected hearth copy and ensures
its old repeated costs, output label, skill list, and request prose are absent.
Build, full automation, and rendered verification remain deferred to M12's
milestone-final run.
