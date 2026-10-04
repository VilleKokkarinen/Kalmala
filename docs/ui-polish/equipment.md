# Carried-tool presentation candidate

Equipment currently consists of owner-only carried tools, not armour slots.
The candidate uses two wider columns, bold readable tool names, theme spacing,
and separate level, condition, and explicit READY / DAMAGED / BROKEN lines.
Unknown or invalid condition data displays Tool state unavailable rather than
fabricating a maximum or a usable state. Canonical icons and tooltip details
remain shared with the existing catalogue view. No new action is supplied.

The existing row cache includes detail text, text scale and contrast, so a
replicated condition change or local accessibility change rebuilds the view.
The widget remains non-focusable and hit-test-invisible. No RPC, gameplay
validation, inventory transaction, or save schema changes.

## Targeted verification

Build the affected UI in a disposable mirror with normal UnrealBuildTool
user-directory access. Run Kalmala.UI.Inventory.ToolPresentation together with
Kalmala.UI.Inventory.PreparedFoodDetails and Kalmala.UI.Theme.LocalPresentation.
The new test checks full, damaged, broken and invalid/missing condition labels.
It does not emulate renderer layout; widget integration is checked by peers.

Run the existing inventory fixture with -Rendered -EquipmentView at
1280x720/100%/standard and 1024x768/150%/high contrast. EquipmentView is an
explicit non-shipping developer view: it hides the upper pack/support text
and scrolls to the carried-tool region without altering the owner's state.
It allows the region to fit without scrolling, and waits five seconds per
capture. Existing authority, capacity, privacy and read-only checks still run.
Inspect both peers' filled screenshots; a behavioral PASS is insufficient
when tool names or condition text are missing. This isolated view does not
establish the whole dense HUD's layout or normal input scrolling access.

Broader HUD/crafting rendering defects, parent integration, physical input
hardware and packaged behavior remain outside this candidate's acceptance.

## Outcome: blocked

Focused tests pass. Wider high-contrast tool regions were readable on both
peers, but standard client labels/condition fragments and the final five-second
standard host Reed knife label were incomplete. The latter used only this
run's proposed changes plus HEAD in the disposable mirror. Four equipment
images retain final standard failures and readable high-contrast references.
They are evidence, not acceptance; no renderer root-cause fix is claimed.

The final exact-proposal runner also reads stale logs before the delayed
captures finish, failing its capture-log assertion. Fresh retained logs show
expected empty/filled states. The pre-existing uncommitted log-refresh fix was
preserved in the checkout and excluded from the exact proposal. Resolve this
verification dependency without silently committing prior work as this run's.
