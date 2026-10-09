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

The widget cannot focus or hit-test. Its bottom-right anchor is 280 logical
pixels above the viewport bottom, with a maximum width of 320; this clears the
centered arrival prompt at the supported 1024x768/150% layout.
It collapses while the controller ignores movement for a modal; timers continue
so closing a modal does not replay expired messages. Tiny/unavailable viewports
collapse the widget. The combined acceptance below checks both supported
layouts and the existing movement-ignore modal signal; it does not open a
gameplay menu.

## Increment verification

Build the affected editor target in the isolated short mirror. Run
`Kalmala.UI.Notifications.SkillLevels`, `Kalmala.UI.Theme.LocalPresentation`,
`Kalmala.Gameplay.Progression.SkillContract` and
`Kalmala.Gameplay.Progression.ReplicationContract` with an isolated user/log
directory and memory DDC. The notification automation covers accepted awards,
partial/invalid snapshots, coalescing, overflow, expiry, reconnect/reset,
independent owners, passive widget text and bounded theme lifetime.

Item gains and skill levels share the queue with discovery notices. Their
focused increment-level checks cover each source separately; the final
`Kalmala.UI.Notifications.CombinedPresentation` automation checks all three
sources together, no renewal on repeated snapshots, reset/reconnect baselines,
and owner-queue isolation. The rendered acceptance uses the local-only
`-KalmalaNotificationCapture` review fixture to inspect the real widget at
100% standard contrast and 150% high contrast. Its synthetic labels are
different per peer and do not mutate a pawn, claim, inventory, or replicated
field. Normal player/client state remains under the existing receipt and
authority checks.

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
`LandmarkFound`, `ScrollFound`, `AlreadyFound`, or `Unavailable` serial adds the
server-provided acknowledgement label as a passive discovery row. Empty labels
use concise outcome-specific fallbacks, and all labels are trimmed and bounded
to 48 characters. An unchanged serial does not renew the row, and expired
feedback is never reconstructed from the last replicated label.

Discovery rows share the three-row, theme-timed queue and use an original
monochrome inspection glyph. Labels are trimmed and bounded to 48 characters,
with a safe fallback for an empty accepted label. They are transient, local
presentation: no new request, claim, reward, persistence, RPC, peer query, or
saved-data field was added. Accepted discovery rewards may also produce an
existing item-gain notice for the actual inventory increase.

Run `Kalmala.UI.Notifications.Discoveries` with the affected editor build. It
covers silent initial/reconnect baselines, accepted/already-found/unavailable
acknowledgements, unchanged refresh, expiry, label bounds, passive text and
owner-local queue behavior.

## Combat and support action results

The local queue also observes the owner's existing owner-only combat and
support feedback serials. Initial attachment and pawn replacement silently
baseline both values; a newer serial adds one short text-and-icon notice for
`Hit confirmed`, `Defeated`, `Attack unavailable`, `Support accepted`, or
`Support unavailable`. Repeated snapshots do not renew a notice, serial
rollback silently rebaselines, and the combat notice contains no target name or
identity. Discovery acknowledgements remain their own existing notice source.

These events use the same three-row bound, theme lifetime, passive presentation,
and modal collapse as skill, item-gain, and discovery notices. The notification
subsystem reads only the local owning pawn. The server-owned components still
validate and publish every result; this presentation adds no request, RPC,
replicated field, save data, or gameplay mutation. The inventory HUD no longer
duplicates combat, support, or discovery result text.

`Kalmala.UI.Notifications.CombinedPresentation` checks silent action baselines,
serial deduplication, concise text, expiry, and absence of hidden target
identity. The existing rendered capture fixture continues to review the
skill/item/discovery combination; action-result rendering remains part of the
M12 milestone-final HUD matrix.

## Combined acceptance

`Scripts/Verify-Inventory.ps1 -Rendered -NotificationReview` runs two isolated
peers with a developer-only local presentation fixture. Each peer silently
baselines its current three owner sources, verifies a reset/reconnect baseline,
then paints one skill, one item, and one discovery row in the same three-row
widget. It captures combined, modal-collapsed, and restored states for both
peers. The modal stage toggles the existing local movement-ignore signal for
one bounded capture interval; it does not open a gameplay menu or change a
gameplay action. The widget remains non-focusable and hit-test-invisible, and
the notices have no entrance, exit, or scrolling animation, so their motion
path is static under reduced-motion use. Theme lifetime still controls expiry.

Run the rendered fixture at 1280x720/100%/standard contrast and
1024x768/150%/high contrast, inspect both peers' source PNGs, and run
`Scripts/Verify-InventoryReconnect.ps1`. That reconnect scenario starts two
successive clients against the same live host and requires each fresh owner
baseline to be silent for skills, accepted item receipts, and discovery state.
The synthetic capture labels validate per-peer presentation isolation; actual
inventory receipt privacy remains covered by the live inventory verifier and
discovery ownership by its existing replication contract. Full queue, ownership,
M5 documentation, PowerShell parser, diff, and path checks complete the parent
verification. The review fixture and audit flags are inert unless explicitly
passed to a non-shipping development process.
