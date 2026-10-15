# Game features

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

Limits: ordinary inventory, equipped gear, and skill XP are not generally saved; farming remains unimplemented. Six gear items, defenses, level-up rewards, icons, Workbench/Forge acquisition recipes, and level-up costs are authored. Server-validated owner-private equip/unequip transactions move wearable item GUIDs into and out of six session-only slots; the Inventory offers equip, replacement, and return actions and displays current wearable state. Accepted wildlife hits spend one condition per wearable; depleted gear provides no defense. Main Hand/hotbar routing remains pending; equipped defenses contribute to server-resolved wildlife damage. Settings/Build/HUD refinements still in the backlog are requested work.

## Worn Gear status — 2026-10-15

The Inventory popup displays current owner-only wearable state and supports server-validated equip, replacement, and return actions for Head, Cape, Chest, Shield, Legs, and Boots. Wearable names display remaining/maximum condition; accepted wildlife hits wear gear and depleted gear provides no defense. Equipped weapons/tools through Main Hand remain pending; equipped defenses contribute to server-resolved wildlife damage. Ordinary gear remains session-only.
