# Fresh inventory capture evidence

The inventory verifier used snapshots read before delayed screenshot capture
finished. This could reject a completed fixture whose files and final logs were
correct. Read-InventoryCapture.ps1 now polls fresh host/client logs for up to ten
seconds after the four screenshot files exist. Both peers must confirm empty
and filled states with expected capacity/counts; fatal/assert/ensure evidence
fails immediately. A missing state times out explicitly. Normal verification
still requires scrolling; the optional isolated equipment view permits fitting
without scrolling. This changes developer verification only, with no gameplay,
input, authority, replication or save change.

The prior uncommitted immediate log refresh remains preserved in the checkout;
the new bounded reader works without it. Reproduction: run the rendered
Scripts/Verify-Inventory.ps1 fixture in the documented disposable mirror and
inspect retained PNGs separately. Log confirmation does not establish text
readability.

User-requested investigation on 2026-10-03 did not resolve missing rendered text.
Mirror-only diagnostics with -norhithread, -d3d11 and per-text
ClipToBoundsAlways each still omitted tool-name/condition fragments. The last
diagnostic also passed the new capture-log checks, confirming these are separate
failures. Renderer/config changes were not adopted; HUD/crafting/equipment
visual acceptance and full parent integration remain blocked. The current
evidence does not prove a GPU backend, font atlas or batching root cause.
