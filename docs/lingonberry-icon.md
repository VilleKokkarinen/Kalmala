# Lingonberry icon provenance

Generated with the built-in image-generation tool as an original transparent
inventory icon. The artwork shows ripe red lingonberries on a leafy sprig in
the established hand-painted Kalmala small-item style.

- Original: `Content/Kalmala/UI/Source/IconOriginals/Lingonberry.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/Lingonberry.png`
- Imported: `Content/Kalmala/UI/Icons/Items/Lingonberry.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/Lingonberry.Lingonberry`
- Batch: X

## Generation prompt

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy
inventory icon. Primary request: create a new, isolated Lingonberry item icon.
Subject: a small cluster of ripe wild lingonberries still attached to a short
leafy sprig, with five rounded deep-crimson berries and several dark green oval
leaves. Style: tactile hand-painted game inventory artwork, broad painted
facets, bold dark-brown contour, restrained highlights, earthy color.
Composition: one centered cluster filling about 78 percent of a square canvas,
readable at 64 pixels, with clean transparent padding. Background: genuinely
transparent. Constraints: original artwork; no text, label, UI, border, ground,
cast shadow, glow, pot, basket, watermarks, logo, or extra objects.

The item is gathered without a tool from a server-selected Meadow shrub source;
each accepted depletion grants exactly three Lingonberries. Its server-owned
food use reuses the existing one-slot Steady Meal effect. It has no equipment,
attack, upgrade, or acquisition-recipe record; its world source is the
acquisition route.

Prepare and import the retained artwork with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch X
Scripts/Import-CatalogueIconBatch.ps1 -Batch X
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```
