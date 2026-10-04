# Compact owner notifications

The first M11 child adds passive icon/text feedback for skill **levels**, not
each experience award. The local-player subsystem reads only its local
controller's owning pawn for detailed progression and accepted item/discovery
feedback. It never reads peer-private presentation or changes gameplay state.
Gameplay's existing server-owned ledgers and owner-only replication remain the
authority boundary.

The first complete valid six-skill snapshot is a silent baseline. Missing,
partial, duplicate or invalid snapshots cannot advance that baseline. A later
level increase displays the skill name and reached level with original shared
line art. Repeated replication and XP changes within a level do not renew or
replay feedback. Pawn replacement clears the queue and baseline; a level
rollback silently rebaselines all skills. Reconnect therefore does not announce
existing levels. These are transient session notices, with no save-schema change.

At most three rows are active. A further increase of the same active skill
updates its reached level and renews its lifetime. New skills evict the oldest
row at capacity; simultaneous increases follow canonical skill order. Expired
rows are removed and never reconstructed from unchanged snapshots.
`NotificationLifetime` in `DefaultKalmalaTheme.ini` defaults to four seconds;
theme values outside 1–10 seconds fall back to the default. Runtime queue
durations are also bounded. No entrance, exit or scrolling animation occurs,
including reduced-motion use. Text scale and high contrast use the shared theme.

The widget cannot focus or hit-test. Its provisional bottom-right anchor is
160 logical pixels above the viewport bottom, with a maximum width of 320.
It collapses while the controller ignores movement for a modal; timers continue
so closing a modal does not replay expired messages. Tiny/unavailable viewports
collapse the widget. Combined rendered HUD/modal placement acceptance remains
the final child, rather than a verified layout claim here.

## Increment verification

Build the affected editor target in the isolated short mirror. Run
`Kalmala.UI.Notifications.SkillLevels`, `Kalmala.UI.Theme.LocalPresentation`,
`Kalmala.Gameplay.Progression.SkillContract` and
`Kalmala.Gameplay.Progression.ReplicationContract` with an isolated user/log
directory and memory DDC. The notification automation covers accepted awards,
partial/invalid snapshots, coalescing, overflow, expiry, reconnect/reset,
independent owners, passive widget text and bounded theme lifetime.

Item gains and skill levels currently share the queue. Discovery notices have
passed their focused increment-level check. Real replicated notification
pixels, combined queue behavior, modal placement and the supported viewport
matrix remain later acceptance scope. Full parent verification is intentionally
pending until the combined acceptance child is complete.

## Accepted item gains

Accepted server inventory publications send a reliable client-only
`ClientAcceptedGain` receipt to the owning connection. Direct grants record the
accepted quantity. Exchanges, validated candidate commits and successful
storage withdrawals record each positive net item increase after publication;
consumption, deposits, unchanged commits, rejected/stale candidates and failed
storage writes produce no gain. This feedback adds no server mutation RPC and
cannot mint or consume inventory. Receipts do not depend on a later inventory
snapshot, so gain followed by consumption between UI refreshes still announces
the accepted gain. A same-item exchange announces its positive **net** increase,
not its gross output. Carried-tool records are a separate inventory contract and
are not newly announced by this child.

The owner stores only the latest 32 transient receipts with locally monotonic
sequence numbers. The first observed buffer silently establishes a baseline,
including already-delivered receipts on pawn attachment. Pawn replacement
clears it through the existing session reset. Refresh, expiry, modal close and
unchanged buffers never replay receipts. A UI polling gap exceeding 32 receipts
drops older notices; it cannot change inventory. There is no receipt persistence
or save-schema change. Reliable delivery follows the existing pawn's owning
connection; there is no multicast or peer detail replication.

New item receipts share the existing three-row queue with skill notices. Same
canonical item gains coalesce into a summed `Gained N Item` label and renew the
bounded lifetime; summed labels saturate at int32 maximum. Overflow evicts the
oldest row. Canonical catalogue names and original catalogue icons are used.
Late initial skill replication does not clear already accepted item feedback.

Run `Kalmala.UI.Notifications.ItemGains` and `.SkillLevels`,
`Kalmala.Gameplay.Inventory.NetworkContract`, `Kalmala.Gameplay.Crafting.Transactions`
and `Kalmala.UI.Theme.LocalPresentation` after the affected editor build.
`Scripts/Verify-Inventory.ps1` additionally requires live owning-client receipt
presence/bounds and empty remote receipt buffers, alongside existing inventory
privacy and rejected-client-mutation gates. Rendered combined notification
placement and real reconnect pixels remain final-child acceptance scope.

## Accepted discoveries

The local notification subsystem also observes the owning pawn's existing
owner-only discovery acknowledgement. Its feedback serial establishes a silent
baseline on first observation and after pawn replacement, so joining or
reconnecting with an existing acknowledgement does not replay it. A newer
`LandmarkFound` or `ScrollFound` serial adds the server-provided acknowledgement
label as a passive discovery row; `AlreadyFound` and `Unavailable` serials are
consumed without a notice. An unchanged serial does not renew the row, and
expired feedback is never reconstructed from the last replicated label.

Discovery rows share the three-row, theme-timed queue and use an original
monochrome inspection glyph. Labels are trimmed and bounded to 48 characters,
with a safe fallback for an empty accepted label. They are transient, local
presentation: no new request, claim, reward, persistence, RPC, peer query, or
saved-data field was added. Accepted discovery rewards may also produce an
existing item-gain notice for the actual inventory increase.

Run `Kalmala.UI.Notifications.Discoveries` with the affected editor build. It
covers silent initial/reconnect baselines, accepted landmark/scroll feedback,
rejected feedback silence, unchanged refresh, expiry, label bounds, passive
text and owner-local queue behavior. Combined rendered placement remains
final-child acceptance scope.
