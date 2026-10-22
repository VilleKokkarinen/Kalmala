# A8b grouped icon batch — 2026-10-22

Generated with the built-in image-generation tool without external references. These are original standalone inventory icons in the approved hand-painted Kalmala style. Every original is a transparent 1254×1254 RGBA PNG, retained under `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`. Preparation produces a centered transparent 64×64 RGBA PNG at `Content/Kalmala/UI/Source/Icons/<ID>.png`; imported Texture2D assets are at `Content/Kalmala/UI/Icons/Items/<ID>.uasset`.

The 27 remaining A8b identities use import batch AJ for the first 16 manifest rows and AK for the last 11. Both use the standard 16-texture maximum; no limit was raised. Commands:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AJ
python Scripts/Prepare-CatalogueIconBatch.py --batch AK
Scripts/Import-CatalogueIconBatch.ps1 -Batch AJ
Scripts/Import-CatalogueIconBatch.ps1 -Batch AK
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id <ID>
```

The manifest registers each canonical ID and its item/recipe views. The runtime lookup now includes a canonical-ID vector fallback mapping for each item; the imported texture supplies its visible artwork. These icon tasks do not add or enable acquisition. All A8b material/source rows and their dependent outputs remain incomplete until the named enemy and accepted server-side reward exist in runtime. Item recipes and use behavior were already authored in the earlier A8b catalogue increment; this batch changes no effects, costs, acquisition or save/network behavior.

## Exact generation prompts

Each fenced line is the complete prompt submitted for that icon.

### `ViperFang`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated ViperFang item icon. Subject: one long, gently curved ivory viper fang, slightly yellowed at the root and tapering to a sharp dark amber tip, shown alone as a small crafting material; clearly a tooth, with a subtle hollow groove but no venom drip. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no snake, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
```

### `GiantScale`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated GiantScale item icon. Subject: one broad, slightly curved giant snake scale, a single thick overlapping plate with dark forest-green edges, muted bronze-olive center, raised concentric ridges and a few small worn chips; sturdy and clearly reptilian, not a metal shield.
```

### `VenomSac`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated VenomSac item icon. Subject: one small translucent amber-green venom sac, a pear-shaped leathery organ pouch with a dark green vein and a subtle liquid tint visible within; sealed and dry on the outside, no dripping venom.
```

### `PeatHeart`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PeatHeart item icon. Subject: one compact irregular peat heart core, a dense dark umber-brown clod with layered fibrous edges, tiny moss flecks and a subdued warm amber seam through its center; earthy material, not an anatomical heart.
```

### `WolfPelt`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfPelt item icon. Subject: one compact folded wolf pelt bundle, thick gray-brown fur with a pale cream underside and a few darker guard-hair strokes, clean trimmed edges and no visible backing skin; no head or limbs.
```

### `WolfFang`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfFang item icon. Subject: one long tapered wolf fang, warm ivory bone with a darker root and a subtle natural curve, shown alone as a crafting material; clearly a tooth with a smooth sharp point, no blood.
```

### `WolfMeat`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfMeat item icon. Subject: one compact piece of fresh raw wolf meat, a tidy marbled red and rose cut with a thin pale cream fat edge and a clean butchered surface; no bone, no blood, no skewer.
```

### `LynxPelt`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated LynxPelt item icon. Subject: one neatly folded short lynx pelt, warm tawny russet fur with a few distinct dark spots and a soft pale cream underside, clean irregular silhouette with no visible backing skin; no head or limbs.
```

### `PolarBearPelt`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PolarBearPelt item icon. Subject: one thick folded polar bear pelt, dense ivory-white fur with subtle cool gray shadows and a few soft beige guard hairs, clean trimmed edges and no visible backing skin; no claws, head or limbs.
```

### `GoatHorn`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated GoatHorn item icon. Subject: one curved ridged goat horn, pale ivory at the tip and warm tan at the base with broad spiral growth rings, shown alone as a crafting material; no animal, no blood.
```

### `ThunderCore`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ThunderCore item icon. Subject: one compact rugged dark slate mineral core, irregular rounded charcoal stone with a few restrained muted blue-gray mineral veins embedded through it; matte stone, not metal, no sparks or emitted light.
```

### `ReefTooth`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ReefTooth inventory icon. Subject: one stout sea-predator tooth, weathered pale ivory with a blunt root and sharp point, subtle muted coral-orange bands and blue-gray mineral stains; clearly a single tooth, no blood.
```

### `RatTailTonic`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated RatTailTonic inventory icon. Subject: one small corked potion vial filled with deep ruby-red lingonberry tonic, squat clear glass with a short neck, a simple dark cork and a narrow cream label with no writing; no glow or extra fruit.
```

### `BadgerFurVest`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated BadgerFurVest inventory icon. Subject: one compact sleeveless fur vest shown front-on, layered silvery-gray and warm ivory badger fur with a dark brown leather edging and two simple ties; clean wearable garment silhouette, no mannequin.
```

### `RoastedBadger`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated RoastedBadger inventory icon. Subject: one single roasted meat cut, a compact cooked haunch with a deep golden-brown crust, warm reddish-brown facets and a pale cut edge; appetizing cooked food, no bone, no skewer, no plate or garnish.
```

### `FangArrows`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated FangArrows inventory icon. Subject: a small tied bundle of four straight wooden arrows laid diagonally together, each with a pale ivory fang-shaped sharp head and short dark feather fletching; crisp arrow silhouettes, no venom drip.
```

### `ScaleCuirass`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ScaleCuirass inventory icon. Subject: one compact chest cuirass made from overlapping dark olive giant snake scales on a warm bronze-brown frame, front-facing with a clear armor silhouette and a few broad raised scale plates; no spikes or wearer.
```

### `Antidote`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated Antidote inventory icon. Subject: one small corked potion vial filled with clear muted teal-green antidote, squat rounded glass with a short neck and a plain cream band with no writing; clean, calm colors, no glow or extra herbs.
```

### `PeatHeartStew`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PeatHeartStew inventory icon. Subject: one sturdy dark earthenware bowl filled with thick brown peat-heart stew, visible chunky potato and onion pieces in a warm amber broth; appetizing meal icon, no spoon, no garnish or steam.
```

### `WolfPeltBoots`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfPeltBoots inventory icon. Subject: one pair of sturdy ankle-high leather boots with compact gray-brown wolf-fur cuffs, warm dark leather soles and simple laces, shown together as a clear wearable equipment silhouette; no wearer.
```

### `FangSpear`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated FangSpear inventory icon. Subject: one primitive straight wooden spear with a long pale ivory fang-shaped point securely lashed to the tip with dark leather cord, simple balanced one-handed weapon silhouette; no blood, no extra gear.
```

### `WolfStew`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfStew inventory icon. Subject: one sturdy dark earthenware bowl filled with rich red-brown wolf stew, visible tender meat chunks, orange carrot pieces and pale onion segments in a warm broth; appetizing meal icon, no spoon, no garnish or steam.
```

### `LynxMantle`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated LynxMantle inventory icon. Subject: one compact short fur mantle arranged front-facing as a wearable cape, warm russet lynx fur with a few subtle dark spots and pale cream edging, a simple small bronze-brown clasp; no mannequin or wearer.
```

### `PolarBearCoat`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PolarBearCoat inventory icon. Subject: one heavy front-facing chest coat made from thick ivory polar bear fur, broad plush collar and shoulders over a dark brown leather body with simple closures; clear garment silhouette, no mannequin or wearer.
```

### `HornBow`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated HornBow inventory icon. Subject: one elegant primitive longbow, warm dark densewood grip and curved pale cream horn limbs joined by a visible taut string, shown alone in a slight diagonal, clear complete weapon silhouette; no arrow, no spark or glow.
```

### `ThunderMaul`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ThunderMaul inventory icon. Subject: one heavy two-handed war maul with a long dark wood handle and a broad squared charcoal-iron head, a small muted blue-gray thunderstone core inset on each visible face; rugged crafted weapon, no lightning bolts or emitted light.
```

### `ReefHarpoon`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated ReefHarpoon item icon. Subject: one compact spear-like reef harpoon with a light warm-brown wood shaft, a dark iron barbed head and a pale weathered reef-tooth inset near the point, simple crafted bindings and a clear one-handed weapon silhouette; no rope, no water, no fish. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
```
