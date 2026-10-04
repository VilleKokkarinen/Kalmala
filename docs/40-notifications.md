# Compact owner notifications

The first M11 child adds passive icon/text feedback for skill **levels**, not
each experience award. The local-player subsystem reads only its local
controller's owning pawn `GetDetailedProgression()`. It never reads peer
presentation or changes progression. Gameplay's existing accepted-action
server ledger and owner-only replication remain the authority boundary.

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

Item gains and discoveries are the next two children. Real replicated level-up
pixels, combined queue behavior, modal placement and the supported viewport
matrix remain later acceptance scope. Full parent verification is intentionally
pending until those children are complete.
