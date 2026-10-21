# Bow, Bow String, Sinew and Arrow icon provenance

## Batch I — Bow, Wooden Arrow and retired String icon

Generated on 2026-10-15 with the built-in image-generation tool. The existing
Stone original was a style reference only. Each original is 1254x1254 RGBA;
the prepared icons are 64x64 RGBA with preserved alpha. Original and prepared
artwork was visually inspected. Batch I imported and validated all three UI
Texture2D packages; the full 66-row catalogue icon file audit passed.

The old `String` item was replaced by bow-only `BowString` on 2026-10-19. Its
old original/prepared files and package remain in the workspace but are no
longer registered in the catalogue icon manifest or runtime mapping. For `Bow`
and `WoodenArrow`, retained files are:

- `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`
- `Content/Kalmala/UI/Source/Icons/<ID>.png`
- `Content/Kalmala/UI/Icons/Items/<ID>.uasset`

Preparation: `python Scripts/Prepare-CatalogueIconBatch.py --batch I`.
Import: `Scripts/Import-CatalogueIconBatch.ps1 -Batch I`.
Audit: `Scripts/Validate-UIIcons.ps1 -Category Catalogue`.
The sandboxed importer created no log; the existing content-only importer
succeeded outside the sandbox. Gameplay build/render verification is deferred.

## Batch K — Bow String and Sinew (2026-10-19)

Generated with the built-in image-generation tool as standalone transparent
1254×1254 RGBA originals. The prepared icons are centered 64×64 RGBA images
with alpha. `Prepare-CatalogueIconBatch.py --batch K` prepared them; the
content-only Unreal importer created `/Game/Kalmala/UI/Icons/Items/BowString`
and `/Game/Kalmala/UI/Icons/Items/Sinew`, and per-ID catalogue icon validation
passed for both. The originals, prepared images, packages, runtime IDs and
validation rows are retained. The full 73-row catalogue icon audit passed on
2026-10-19; rerun it during final A0 verification.

## Historical batch I exact generation prompts

### Retired String artwork

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: a compact coil of rough flax string with a short loose end, warm pale beige twisted plant fibres, clearly thin cord rather than thick rope.

### Bow

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: one simple wooden hunting bow, slender curved light-brown wooden limbs, wrapped grip and taut thin flax bowstring, diagonally oriented lower-left to upper-right, no arrow.

### WoodenArrow

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: three simple primitive wooden arrows grouped diagonally lower-left to upper-right, sharpened wooden points and muted feather fletching, no metal heads, no bow.

## Batch K exact generation prompts

### BowString

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a 64-pixel game item. Primary request: create one original BowString icon made from animal sinew. Subject: a compact hand-twisted coil of thin, tightly braided warm ivory and pale tan sinew cord, with one short loose end, clearly flexible thin cord rather than rope or plant fibre. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy material colors, strong clear silhouette readable at 64 pixels. Composition: one centered coil filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, bow, arrow, leaves, flax or extra objects.

### Sinew

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a 64-pixel game item. Primary request: create one original Sinew material icon. Subject: three short, narrow, softly curved animal tendon strands laid together as a compact bundle, warm ivory and muted pale tan with subtle fibrous ends, clearly raw crafting sinew and distinct from a coiled cord. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy natural materials, strong clear silhouette readable at 64 pixels. Composition: one centered bundle filling about 78 percent of a square canvas with clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, bowstring coil, bow, arrow, hide, bone or extra objects.

## Wooden Arrow revision — sharpened wood only

On 2026-10-15 the user requested removing the broad bronze-looking heads.
The built-in image editor replaced all three front ends with continuous wooden
shafts whittled to narrow points, without separate heads or tip bindings.
The feathers, diagonal arrangement, painted style and transparency are retained.
The revised original and prepared icon were visually inspected, reimported and
validated; the 66-row icon audit passed. String and Bow asset bytes were preserved.

Exact edit prompt:

Use case: precise-object-edit. Edit the supplied WoodenArrow inventory icon. Replace ALL THREE broad attached arrowheads with plain wooden shaft tips, each whittled to a slender tapered point directly from the SAME single piece of wood as its shaft. Continuous wood grain running all the way into each tip. The shaft stays narrow and simply tapers at its end like a sharpened wooden stick or pencil. Remove all separate triangle/diamond heads, metal, bronze, copper, stone, tip sockets and bindings near the tips. Absolutely NO attached arrowheads, NO metallic sheen, NO bronze tip. Preserve the three diagonal arrows, feather fletching and bindings at the feather ends, hand-painted survival-game style, colors, dark outlines, composition and true transparent background. Only change the pointed front ends and remove their attachment bindings. Keep clean transparent padding.

## Batch Q — Stone Arrow (2026-10-21)

Generated as original standalone transparent artwork with the built-in image
generation tool. The 1254×1254 RGBA original shows three wooden arrows with
chipped pale-gray stone tips and muted feather fletching. It was prepared at
64×64 and imported as `/Game/Kalmala/UI/Icons/Items/StoneArrow`; the canonical
icon manifest and runtime mapping use `StoneArrow`. The first image pass added
cord around the stone tips, so a focused edit removed only those wrappings.

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated StoneArrow icon for a game inventory. Subject: three primitive wooden arrows grouped diagonally lower-left to upper-right, each with a small clearly visible chipped pale-gray stone tip secured to the shaft, simple muted feather fletching, and plain wooden shafts. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; strong silhouette that remains readable at 64 pixels. Composition: three centered arrows filling about 78 percent of a square canvas with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, bow, metal or bronze parts, bindings near the stone tips, or extra objects.

Edit prompt:

> Use case: precise-object-edit. Edit the supplied StoneArrow inventory icon. Remove only the tan cord wrappings around the bases of all three stone tips. Each chipped stone tip should meet its wooden shaft directly with no cord, binding, socket, or metal fitting near the tip. Preserve all three arrows, their stone tips, shafts, feather fletching, composition, hand-painted style, dark outlines, earthy colors, clean transparent padding, and genuinely transparent background. Do not add anything.

The retained original is `Content/Kalmala/UI/Source/IconOriginals/StoneArrow.png`;
the prepared image and imported Texture2D are in batch Q. The transparent original
and the 64×64 prepared icon were visually inspected. The icon is validated by
`Scripts/Validate-UIIcons.ps1 -Category Catalogue`.
