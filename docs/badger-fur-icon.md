# Badger Fur icon provenance

Generated with the built-in image-generation tool on 2026-10-22, without an
external reference. The retained original is a transparent 1254×1254 RGBA
image. Batch AH prepares it as a centered transparent 64×64 RGBA PNG and
imports it as a Texture2D with the established catalogue icon pipeline.

Retained files:

- Content/Kalmala/UI/Source/IconOriginals/BadgerFur.png
- Content/Kalmala/UI/Source/Icons/BadgerFur.png
- Content/Kalmala/UI/Icons/Items/BadgerFur.uasset

Prepare and import with:

    python Scripts/Prepare-CatalogueIconBatch.py --batch AH
    Scripts/Import-CatalogueIconBatch.ps1 -Batch AH
    Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id BadgerFur

The canonical BadgerFur mapping uses the existing Hide vector fallback
variant; the imported texture supplies its visible inventory artwork. Badger
Fur has no equipment profile, attack, or upgrade record; it is a material used
by the existing Badger Fur Vest recipe. This icon task does not add or enable a
Badger drop. The Badger actor and accepted runtime reward remain prerequisites
for reachable acquisition and material completion.

## Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated BadgerFur item icon. Subject: one compact loose tuft of coarse badger fur, a small crafting-material bundle of layered silvery gray and warm ivory fibers with several bold near-black guard hairs, soft tawny-brown undertones, and a clean irregular silhouette; no skin or pelt backing. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, with clean transparent padding. Lighting/mood: soft warm highlights across the coarse fibers. Background: genuinely transparent. Constraints: original artwork; no animal, no body parts, no blood, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.
