# Badger Meat icon provenance

Generated with the built-in image-generation tool on 2026-10-22, without an
external reference. The retained original is a transparent 1254×1254 RGBA
image. Batch AI prepares it as a centered transparent 64×64 RGBA PNG and
imports it as a Texture2D with the established catalogue icon pipeline.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/BadgerMeat.png`
- `Content/Kalmala/UI/Source/Icons/BadgerMeat.png`
- `Content/Kalmala/UI/Icons/Items/BadgerMeat.uasset`

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AI
Scripts/Import-CatalogueIconBatch.ps1 -Batch AI
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id BadgerMeat
```

The canonical `BadgerMeat` mapping uses the existing Meat fallback variant;
the imported texture supplies its visible inventory artwork. Badger Meat has
no equipment profile, attack, or upgrade record; it is a material used by the
Roasted Badger recipe. This icon task does not add or enable a Badger drop.
The Badger actor and accepted runtime reward remain prerequisites for
reachable acquisition and material completion.

## Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated BadgerMeat item icon. Subject: one compact, fresh cut of raw game meat, a small crafting material portion with a softly angular oblong silhouette, layered deep rose-red and warm red-brown muscle facets, several restrained pale cream fat seams and subtle pink highlights; no bone, hide, fur, or pelt backing. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, with clean transparent padding. Lighting/mood: soft warm highlights across the cut surface. Background: genuinely transparent. Constraints: original artwork; no animal, no body parts, no blood, no gore, no knife, no plate, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.
