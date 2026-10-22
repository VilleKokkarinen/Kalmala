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
is registered in the crafting-ingredients catalogue and has no equipment
profile, attack, or upgrade record. Its accepted server-side Rat reward queues
one Rat Tail with the baseline Bone Fragment under the defeated spawn ID and
authenticated owner. The pair is delivered atomically when both fit and stays
in persistent escrow otherwise. The Rat-tail Tonic recipe is registered, but
its consumable behavior remains a separate A8b task. Full-pack, restart, peer,
and rendered acceptance remain part of combined I4 verification.

## Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated RatTail item icon. Subject: one curled brown rat tail, shown by itself as a small crafting material; tapered, flexible, visibly furred, with a rounded darker tip and a clean cut base, with no blood. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no rat body, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
