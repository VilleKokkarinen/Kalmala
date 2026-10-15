# String, Bow and Wooden Arrows icon provenance

Generated on 2026-10-15 with the built-in image-generation tool. The existing
Stone original was a style reference only. Each original is 1254x1254 RGBA;
the prepared icons are 64x64 RGBA with preserved alpha. Original and prepared
artwork was visually inspected. Batch I imported and validated all three UI
Texture2D packages; the full 66-row catalogue icon file audit passed.

For each of `String`, `Bow`, and `WoodenArrow`, retained files are:

- `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`
- `Content/Kalmala/UI/Source/Icons/<ID>.png`
- `Content/Kalmala/UI/Icons/Items/<ID>.uasset`

Preparation: `python Scripts/Prepare-CatalogueIconBatch.py --batch I`.
Import: `Scripts/Import-CatalogueIconBatch.ps1 -Batch I`.
Audit: `Scripts/Validate-UIIcons.ps1 -Category Catalogue`.
The sandboxed importer created no log; the existing content-only importer
succeeded outside the sandbox. Gameplay build/render verification is deferred.

## Exact generation prompts

### String

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: a compact coil of rough flax string with a short loose end, warm pale beige twisted plant fibres, clearly thin cord rather than thick rope.

### Bow

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: one simple wooden hunting bow, slender curved light-brown wooden limbs, wrapped grip and taut thin flax bowstring, diagonally oriented lower-left to upper-right, no arrow.

### WoodenArrow

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: three simple primitive wooden arrows grouped diagonally lower-left to upper-right, sharpened wooden points and muted feather fletching, no metal heads, no bow.

## Wooden Arrow revision — sharpened wood only

On 2026-10-15 the user requested removing the broad bronze-looking heads.
The built-in image editor replaced all three front ends with continuous wooden
shafts whittled to narrow points, without separate heads or tip bindings.
The feathers, diagonal arrangement, painted style and transparency are retained.
The revised original and prepared icon were visually inspected, reimported and
validated; the 66-row icon audit passed. String and Bow asset bytes were preserved.

Exact edit prompt:

Use case: precise-object-edit. Edit the supplied WoodenArrow inventory icon. Replace ALL THREE broad attached arrowheads with plain wooden shaft tips, each whittled to a slender tapered point directly from the SAME single piece of wood as its shaft. Continuous wood grain running all the way into each tip. The shaft stays narrow and simply tapers at its end like a sharpened wooden stick or pencil. Remove all separate triangle/diamond heads, metal, bronze, copper, stone, tip sockets and bindings near the tips. Absolutely NO attached arrowheads, NO metallic sheen, NO bronze tip. Preserve the three diagonal arrows, feather fletching and bindings at the feather ends, hand-painted survival-game style, colors, dark outlines, composition and true transparent background. Only change the pointed front ends and remove their attachment bindings. Keep clean transparent padding.
