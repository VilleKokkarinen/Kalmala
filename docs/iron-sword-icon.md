# Iron Sword icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. No outside image was used as a reference.

- Original: `Content/Kalmala/UI/Source/IconOriginals/IronSword.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/IronSword.png`
- Imported texture: `Content/Kalmala/UI/Icons/Items/IronSword.uasset`
- Icon ledger: `Scripts/Data/catalogue-icon-manifest.csv`, batch N

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated IronSword item icon that matches the established small-item artwork treatment. Subject: one compact one-handed iron sword with a cool grey iron double-edged blade, simple iron crossguard and pommel, dark wooden grip wrapped in pale deer hide. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered sword set diagonally from lower-left grip to upper-right point, filling about 78 percent of a square canvas, with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, gems, bronze/gold tint, or extra objects.

The transparent 1254×1254 RGBA original was prepared as a 64×64 RGBA icon.
The batch-N importer creates `/Game/Kalmala/UI/Icons/Items/IronSword.IronSword`
as a transparent UI Texture2D. `Scripts/Validate-UIIcons.ps1 -Category
Catalogue` checks the original, prepared file, runtime mapping, and package.
