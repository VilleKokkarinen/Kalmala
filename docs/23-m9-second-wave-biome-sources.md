# M9 second-wave biome source map

This map completes the first child of the M9 source and loot catalogue task.
It records the four source-to-material candidates named by the M9 backlog and
keeps the existing first-wave source rewards explicit across all supported
land biomes. It does not activate runtime content.

| Supported biome | Existing M7 source → reward | M9 source parent → material |
| --- | --- | --- |
| Meadows | `meadows-birch-bark` → `Wood` | Harvestable birch trunk → `Lightwood` |
| Shimmering Lakes | `lakes-reed-cluster` → `Fibre` | No new M9 material mapped in this increment |
| Elderwood | `elderwood-resinwood` → `Wood` | Ironheart tree trunk → `Densewood` |
| Mossy Mire | `mire-bog-iron` → `Stone` | Hardened seam in a peat bank → `Peat Amber` |
| Freezing Tundra | `tundra-frostmoss` → `Fibre` | Salt deposit → `Frost Salt` |
| Thunder Mountains | `mountains-slate-vein` → `Stone` | No new M9 material mapped in this increment |

## Source boundaries

- Each M9 source parent is a distinct gatherable definition in its named
  biome. The birch trunk and ironheart trunk remain separate sources from the
  existing birch-bark and resinwood rewards; those first-wave IDs continue to
  grant `Wood`.
- The source-to-material mapping is closed: each listed M9 source yields only
  its paired material. It introduces no random material drops or client-authored
  reward choice.
- Lakes and Thunder Mountains keep their existing M7 source/reward mappings in
  this source-map increment. This does not prevent a later backlog-approved
  catalogue entry from extending either biome.
- The material labels are design names. Canonical item/source IDs,
  presentations, placement budgets, sparse depletion identities, optional
  bonus loot, and the Bronze Axe/Iron Axe field gates are separate M9 backlog
  children and remain undefined here.

## Authority and activation

This document changes no runtime or save behavior. The future implementation
must continue to derive source selection, harvest eligibility, material reward,
and depletion on the server through the existing validated harvest and
inventory paths. Clients may submit interaction intent only. Until the later
catalogue and placement children are complete, the existing M7 population
descriptor, rewards, presentation, and sparse-depletion behavior remain
unchanged.
