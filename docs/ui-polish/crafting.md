# Crafting/construction polish candidate — blocked 2026-10-03

The uncommitted candidate in KalmalaCraftingSubsystem.cpp adds theme SlotPadding
between menu text sections and between each card heading and its state. Card
names fill the remaining horizontal heading space. Selected cards retain an
explicit AVAILABLE/UNAVAILABLE line alongside SELECTED and FOCUSED, rather than
selection hiding availability. Existing selected-result requirements, rejection
details, tooltips, actions, focus, modal handling and owner-visible inputs remain.
No gameplay, authority, RPC, save or catalogue contract changes.

The developer-only screenshot probe waits five seconds rather than two. This
did not fix incomplete rendered text and is not a gameplay timing change.

Increment verification requires affected-module compile/link, rendered
Verify-Crafting.ps1 at 1280x720/100% standard and 1024x768/150% high contrast,
both peers visually inspected, plus presentation ownership/documentation audits.
Compile and behavioral runners passed, but visual acceptance failed across three
attempts. craft-failed-client.png omits parts of UNAVAILABLE on several cards;
craft-failed-hc-client.png omits heading and instruction fragments. The host
high-contrast reference is craft-failed-hc-host.png. Preserve this candidate,
diagnose text rendering and rerun acceptance before completing this child.
Parent integration, physical controller hardware and packaged behavior remain
unverified. Existing scroll/focus navigation checks do not establish visual
readability of every detail below the captured scroll position.
