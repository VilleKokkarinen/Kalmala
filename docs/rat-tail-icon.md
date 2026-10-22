# Rat Tail icon provenance

Generated with the built-in image-generation tool on 2026-10-22, without an
external reference. The retained original is a transparent 1254×1254 RGBA
image. Batch AG prepares it as a centered transparent 64×64 RGBA PNG and
imports it as a Texture2D with the established catalogue icon pipeline.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/RatTail.png`
- `Content/Kalmala/UI/Source/Icons/RatTail.png`
- `Content/Kalmala/UI/Icons/Items/RatTail.uasset`

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AG
Scripts/Import-CatalogueIconBatch.ps1 -Batch AG
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id RatTail
```

The catalogue mapping uses the RatTail canonical ID and the String vector
fallback. The imported texture supplies the visible inventory artwork. RatTail
has no equipment profile, attack, or upgrade record. It is used by the existing
Rat-tail Tonic recipe. The icon does not add or activate a Rat drop: Rat and its
accepted server-side defeat reward are absent from runtime, so the material and
tonic remain incomplete and unreachable.

## Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated RatTail item icon. Subject: one curled brown rat tail, shown by itself as a small crafting material; tapered, flexible, visibly furred, with a rounded darker tip and a clean cut base, with no blood. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no rat body, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
