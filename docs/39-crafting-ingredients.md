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
