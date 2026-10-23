# Standing Light icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. Updated on 2026-10-21 to replace the brazier
with a wooden torch head and visible flame. The retained 1254×1254 RGBA
original is prepared to a centered 64×64 RGBA texture and imported as
`StandingLightKit` in batch AO. The icon manifest maps the `StandingLight`
buildable and recipe to that texture. It depicts a placed object, never a
portable inventory item.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/StandingLightKit.png`
- `Content/Kalmala/UI/Source/Icons/StandingLightKit.png`
- `Content/Kalmala/UI/Icons/Items/StandingLightKit.uasset`

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AO
Scripts/Import-CatalogueIconBatch.ps1 -Batch AO
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id StandingLightKit
```

Update prompt:

> Edit the referenced Kalmala Standing Light icon into a rustic wooden torch with a real flame at its top. Keep the same centered upright standing object, recognizable timber tripod feet, hand-painted survival-fantasy game icon style, broad tactile wood facets, dark brown outlines, earthy warm palette, and clean transparent square canvas. Replace the faceted amber crystal and its forked cradle completely: make the top a simple wooden torch head, with a short bundle of dry kindling or wrapped cloth secured to the pole by dark rope, and a compact bright yellow-orange flame visibly rising from the very end. It must read clearly as burning wood/cloth at 64 pixels, not as a crystal, gem, lantern, bowl, brazier or metal cage. The upright shaft remains plain carved wood with a couple of rope lashings and a stable three-leg timber base. Preserve the existing object's overall scale and polished painted rendering; make the flame modest and crisp with no separate glow or cast shadow. Original artwork, no text, labels, UI, border, ground, watermark, logo or extra objects. Genuinely transparent background.
