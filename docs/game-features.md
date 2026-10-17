# Game features

## Hammer building and bow drawing — 2026-10-18

Building Hammer is build-only: left-click places the selected piece and right-click
opens the build menu to choose a piece, including when a placement is pending.
It never starts a melee attack. Bow also uses its own controls instead of melee.

Hold left-click with a usable Bow and arrows to draw. The central crosshair expands
to a circle and shrinks continuously as the bow reaches full draw. Its authored
`DrawDuration` is currently 1.5 seconds and appears in item inspection. Release at
any point to shoot; a quick release is weaker, slower and less accurate. Holding
past full draw clamps at full power. Opening a modal menu or swapping bows cancels
without spending ammunition. A committed shot spends one arrow and one condition.

Select an arrow stack in the inventory grid and click **Prioritize this arrow
stack**. Click the preferred stack's action again to restore automatic selection.
If the preferred stack is missing or exhausted, selection falls back to an
available arrow stack. Currently Wooden Arrow is the available ammunition type.
Projectiles have gravity and server collision; walls stop them and wildlife takes
Pierce damage. Arrows expire after five seconds or on impact and are not recovered.

Lower `DrawDuration` supports faster authored bows. A bounded server draw-speed
calculation supports a future Archery multiplier; no Archery skill/progression or
additional bow tiers are added yet. The current wooden-arrow tuning spans 10–40
Pierce damage, 1000–4500 cm/s and 8–0.25 degrees of spread from release to full draw.

## Nearby item pickup — 2026-10-17

Automatic pickup is enabled by default. Walk within 1.5 metres of loose materials
or a broken-tree wood bundle to collect it when your pack has room and no wall
blocks it. Standing trees, falling trees, settled logs and other resource
sources still require their normal actions. Full packs leave drops intact.

Options > Accessibility > Automatic pickup switches this behaviour on or off
and remembers the choice locally. With it off, look at a nearby item and use
the Action key (the remappable Interact binding). The crosshair reads `Pickup
Wood` for one, or `Pickup 2 Wood` for multiple items, using the actual item name
and quantity. Manual pickup remains available with the option enabled too.

## Held equipment appearance — 2026-10-17

The character now shows the equipped Main Hand tool family: hammer, axe, knife,
pick, spear, mace or bow. Empty/unknown selections hide every held-tool part.
Each tool is anchored at the palm at the end of the right arm. Hammer, axe,
knife, pick, spear and mace shafts project forward through the hand, with the
axe cutting edge facing down; the bow stays upright at its central grip.
The grip follows the arm through gait, attack and sharpening poses.
These are original procedural silhouettes attached to the animated right arm;
Stone/Bronze/Iron variants currently share their family shape and materials.
Other peers receive only the server-selected visible family, not the item's GUID,
level, condition, pack contents or acquisition state. Bow appearance does not
implement shooting or a two-handed animation.

## Free melee swings with held items — 2026-10-17

Every usable equipped melee weapon or tool, excluding the Building Hammer and Bow,
can swing without a target nearby. Empty hands can punch the air too. The right
arm raises, strikes and returns over the existing 0.54-second attack window.
A miss completes cooldown quietly without tool wear, damage or rewards.
Broken/depleted tools still need repair before attacking.

Actual hits still require a server-selected target and visibility/range recheck.
Weapons use their authored Basic damage; items without an authored attack,
use the existing basic 25 Crush melee fallback.
Hammer placement and Bow drawing/shooting use the dedicated controls described above.
Only suitable axes can damage trees: Meadow trees require Stone Axe or better
and drop Wood; Elderwood requires Iron Axe and drops Densewood.

## Generated-forest harvesting — 2026-10-17

Generated Meadow/Elderwood trees are server-owned harvest actors with stable
seed/patch/candidate identities. Equipped axe attacks advance Standing through a server-timed
1.2-second Falling phase to settled Trunk, then a collectible wood Drop; one actor retains the identity through each stage.
The existing resource tiers, yields and accepted-hit condition cost apply.
Falling rejects overlapping hits without spending condition; replicated server time
allows elapsed-progress reconstruction. Matching cosmetic trees are suppressed to avoid duplicate presentation.
The original procedural tree hinges forward during Falling, then leaves its
full-size horizontal timber with exposed cut faces at the original tree root.
Presentation preserves the actor transform and persistent identity.
Settled timber uses its mesh for targeting; server axe reach is measured to that
timber surface, allowing hits along the log while rejecting distant attackers.
The old root sphere is disabled during settled timber and bundle phases. Breaking
the log leaves the original two-piece bundle at its midpoint on the root plane.
The visible bundle supplies targeting collision and the server measures physical
pickup reach from the same reconstructed position.

Accepted pickup grants once and records sparse depletion. Full packs retain the
live drop and receive the existing private rejection feedback. Collected IDs stay
absent after eviction and disk reload; uncollected trees return as standing trees,
so partial damage and uncollected trunks/drops are session-only. Focused peer
checks cover exact-ID stages, late joins, ordinary-camera pickup and competing
owners. Live peers also cover shared departure, final eviction and collected-ID
reactivation; separate processes cover real population-slot restart. The defined
final tree-feature gates and evidence audit passed.
[Setup evidence and limits](07-development-setup.md#final-tree-feature-evidence-audit-2026-10-17)
record the accepted boundaries. Rendered manual usability, Falling-phase late join,
adverse networking, live peer process restart and full-project/package verification
remain outside this acceptance.

## Bow and two-handed items — 2026-10-15

String stacks to 20; Wooden Arrows stack to 100. A level-one Workbench crafts
Bow for 5 Wood and 2 String, and 20 Wooden Arrows for 5 Wood. All three have
original imported inventory artwork. String has no acquisition source yet.
Bow occupies both hands; validated atomic swaps return the previous tool and
shield, requiring one free cell if both were held. Shield equip returns Bow;
unwielding requires pack capacity. Wielding state remains owner-private and
session-only. Bow level-ups reuse its 5 Wood/2 String Workbench acquisition cost
and grant +50 maximum durability per level. Damage rewards remain zero while
shooting is unimplemented.

Added gameplay and UI, in brief. [BACKLOG.md](../BACKLOG.md) tracks unfinished integration and verification.

- Solo and cooperative play with server-validated actions.
- Seeded wilderness with seven biomes, lakes, rivers, and a finite world.
- Walking, sprinting, jumping, and ocean swimming.
- Tool-based gathering with condition and biome-specific materials.
- Player-built fires, floors, walls, roofs, stations, and chests.
- Rain, wind, fog, cold, Wet effects, shelter, and fire recovery.
- Fuelled fires that smoulder in rain; exposed construction rain wear.
- Six cooking recipes and a temporary meal benefit for supported foods.
- Workbench/Forge crafting, axe upgrades, and station attachments.
- Free tool repair and Grinding Stone Repair All.
- Committed melee attacks and Mireling, boar, and deer encounters.
- Optional landmarks, discoveries, scroll rewards, and support magic.
- Mending, Hearth Shield, Bear's Vigor, and Deer Call.
- A two-seat ocean skiff and optional sea discoveries.
- Saved construction, chest contents, tool state, and approved discovery facts.
- A forty-cell inventory with ten numbered hotbar cells.
- Minimap, expanded map, local exploration fog, and private pins.
- Item/status icons, contextual details, and recipe requirements.
- Recipe Favorites, usage ranks, Recent shortcuts, and menu memory.
- Compact status icons and private action/gain/discovery notices.
- Shared themes, remappable controls, scaling, contrast, and reduced motion.
- Original procedural models, environmental audio, and action cues.

Implemented in ongoing milestone work; integrated acceptance is pending:

- Inventory/crafting companion, contextual tooltips, and opening transitions (M13).
- Reusable hotbar support scrolls, five-minute cooldown, and three-minute timed effects (M13).
- Textured map, restored fog, selectable labelled pins, temporary global pings, and cached pan/zoom tiles (M14).

Limits: ordinary inventory, equipped gear, and skill XP are not generally saved; farming remains unimplemented. Six gear items, defenses, level-up rewards, icons, Workbench/Forge acquisition recipes, and level-up costs are authored. Server-validated owner-private equip/unequip transactions move wearable item GUIDs into and out of six session-only slots; the Inventory offers equip, replacement, and return actions and displays current wearable state. Accepted wildlife hits spend one condition per wearable; depleted gear provides no defense. Main Hand now moves the selected tool outside the pack grid with capacity-safe replacement/return; the final numbered-hotbar contract remains pending; equipped defenses contribute to server-resolved wildlife damage. Settings/Build/HUD refinements still in the backlog are requested work.

## Worn Gear status — 2026-10-15

The Inventory popup displays current owner-only wearable state and supports server-validated equip, replacement, and return actions for Head, Cape, Chest, Shield, Legs, and Boots. Wearable names display remaining/maximum condition; accepted wildlife hits wear gear and depleted gear provides no defense. Equipped weapons/tools now use Main Hand outside the grid, retain physical GUID/condition, and return only when pack space permits; equipped defenses contribute to server-resolved wildlife damage. Ordinary gear remains session-only.
