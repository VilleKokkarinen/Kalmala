# Bronze, copper, tin and ore icon provenance

Generated with the built-in image-generation tool on 2026-10-15. The six
originals are standalone transparent RGBA inventory icons. They were prepared
at 64×64 and imported into catalogue texture batch J. Each icon uses the
Kalmala small-item treatment: a clear silhouette, dark brown contour, broad
painted facets and restrained material highlights. No existing artwork was
replaced and no outside image was used as a reference.

For each identity, retained files are:

- `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`
- `Content/Kalmala/UI/Source/Icons/<ID>.png`
- `Content/Kalmala/UI/Icons/Items/<ID>.uasset`

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch J
Scripts/Import-CatalogueIconBatch.ps1 -Batch J
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

## Exact generation prompts

Each prompt used the following shared text, with the identity and subject
replaced as shown below:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated `<ID>` item icon. Subject: `<SUBJECT>`. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered object filling about 78 percent of a square canvas, with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

| ID | Subject text |
|---|---|
| `Bronze` | a single compact forged bronze ingot, warm golden copper-brown alloy, clearly a rectangular tapered metal bar with softly beveled ends and a few broad hand-painted hammer facets |
| `Copper` | a single compact forged copper ingot, distinctive rich reddish orange metal with warm peach highlights, clearly a rectangular tapered metal bar with softly beveled ends and a few broad hand-painted hammer facets |
| `Tin` | a single compact forged tin ingot, pale cool silver-gray metal with subtle warm edge reflections, clearly a rectangular tapered metal bar with softly beveled ends and a few broad hand-painted hammer facets |
| `CopperOre` | one irregular chunk of dark stone with a few distinct muted copper-orange mineral seams embedded in it; it must read as rough ore-bearing rock, not a polished metal bar |
| `TinOre` | one irregular chunk of cool medium-gray stone with a few distinct pale silvery mineral seams embedded in it; it must read as rough ore-bearing rock, not a polished metal bar |
| `IronOre` | one irregular chunk of charcoal-gray stone with a few restrained rusty iron-red mineral seams embedded in it; it must read as rough ore-bearing rock, not an iron ingot or polished metal bar |

The six prepared icons were visually inspected at 64×64. Source, preparation,
runtime mapping, and imported-package validation are recorded with the current
equipment icon work in `docs/07-development-setup.md`. Unreal catalogue/icon
automation and in-game rendering remain unexecuted.
