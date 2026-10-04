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
