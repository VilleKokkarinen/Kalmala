# Item icon provenance

Retained generation records from the former per-item and per-batch documents.
Use [the shared icon workflow](material-icons.md) for current authoring and import
instructions and `Scripts/Data/catalogue-icon-manifest.csv` for current identities
and batches. These dated records preserve exact prompts and historical check
results; their gameplay status statements are historical, not current acceptance.
Current implementation state belongs to `BACKLOG.md` and `PROGRESS.md`.

## Records
- [A8b grouped icon batch — 2026-10-22](#a8b-icon-batch)
- [A9 gem and gear icon provenance](#a9-icon-batch)
- [Bow, Bow String, Sinew and Arrow icon provenance](#archery-icons)
- [Wearable gear icon provenance](#armor-icons)
- [Badger Fur icon provenance](#badger-fur-icon)
- [Badger Meat icon provenance](#badger-meat-icon)
- [Basic Torch icon provenance](#basic-torch-icon)
- [Berry Recovery Draught icon provenance](#berry-recovery-draught-icon)
- [Bone fragment icon provenance](#bone-icon)
- [Bronze Sword icon provenance](#bronze-sword-icon)
- [Camp Bed icon provenance](#camp-bed-icon)
- [Charcoal Kiln build-menu icon provenance](#charcoal-kiln-icon)
- [Forest Chanterelle icon provenance](#forest-chanterelle-icon)
- [Garden Plot icon provenance](#garden-plot-icon)
- [Iron Sword icon provenance](#iron-sword-icon)
- [Light Bow icon provenance](#light-bow-icon)
- [Lingonberry icon provenance](#lingonberry-icon)
- [Mace inventory icons](#mace-icons)
- [Bronze, copper, tin and ore icon provenance](#material-icons)
- [Planting Trowel icon provenance](#planting-trowel-icon)
- [Rat Tail icon provenance](#rat-tail-icon)
- [Reinforced Chest icon provenance](#reinforced-chest-icon)
- [Root Stamina Draught icon provenance](#root-stamina-draught-icon)
- [Smelter build-menu icon provenance](#smelter-icon)
- [Standing Light icon provenance](#standing-light-icon)
- [Timber Beam icon provenance](#timber-beam-icon)
- [Timber Door icon provenance](#timber-door-icon)
- [Timber Stairs icon provenance](#timber-stairs-icon)
- [Wooden Club icon provenance](#wooden-club-icon)

<a id="a8b-icon-batch"></a>

## A8b grouped icon batch — 2026-10-22

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

### Exact generation prompts

Each fenced line is the complete prompt submitted for that icon.

#### `ViperFang`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated ViperFang item icon. Subject: one long, gently curved ivory viper fang, slightly yellowed at the root and tapering to a sharp dark amber tip, shown alone as a small crafting material; clearly a tooth, with a subtle hollow groove but no venom drip. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no snake, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
```

#### `GiantScale`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated GiantScale item icon. Subject: one broad, slightly curved giant snake scale, a single thick overlapping plate with dark forest-green edges, muted bronze-olive center, raised concentric ridges and a few small worn chips; sturdy and clearly reptilian, not a metal shield.
```

#### `VenomSac`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated VenomSac item icon. Subject: one small translucent amber-green venom sac, a pear-shaped leathery organ pouch with a dark green vein and a subtle liquid tint visible within; sealed and dry on the outside, no dripping venom.
```

#### `PeatHeart`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PeatHeart item icon. Subject: one compact irregular peat heart core, a dense dark umber-brown clod with layered fibrous edges, tiny moss flecks and a subdued warm amber seam through its center; earthy material, not an anatomical heart.
```

#### `WolfPelt`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfPelt item icon. Subject: one compact folded wolf pelt bundle, thick gray-brown fur with a pale cream underside and a few darker guard-hair strokes, clean trimmed edges and no visible backing skin; no head or limbs.
```

#### `WolfFang`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfFang item icon. Subject: one long tapered wolf fang, warm ivory bone with a darker root and a subtle natural curve, shown alone as a crafting material; clearly a tooth with a smooth sharp point, no blood.
```

#### `WolfMeat`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfMeat item icon. Subject: one compact piece of fresh raw wolf meat, a tidy marbled red and rose cut with a thin pale cream fat edge and a clean butchered surface; no bone, no blood, no skewer.
```

#### `LynxPelt`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated LynxPelt item icon. Subject: one neatly folded short lynx pelt, warm tawny russet fur with a few distinct dark spots and a soft pale cream underside, clean irregular silhouette with no visible backing skin; no head or limbs.
```

#### `PolarBearPelt`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PolarBearPelt item icon. Subject: one thick folded polar bear pelt, dense ivory-white fur with subtle cool gray shadows and a few soft beige guard hairs, clean trimmed edges and no visible backing skin; no claws, head or limbs.
```

#### `GoatHorn`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated GoatHorn item icon. Subject: one curved ridged goat horn, pale ivory at the tip and warm tan at the base with broad spiral growth rings, shown alone as a crafting material; no animal, no blood.
```

#### `ThunderCore`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no animal or body, no other objects, no blood, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ThunderCore item icon. Subject: one compact rugged dark slate mineral core, irregular rounded charcoal stone with a few restrained muted blue-gray mineral veins embedded through it; matte stone, not metal, no sparks or emitted light.
```

#### `ReefTooth`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ReefTooth inventory icon. Subject: one stout sea-predator tooth, weathered pale ivory with a blunt root and sharp point, subtle muted coral-orange bands and blue-gray mineral stains; clearly a single tooth, no blood.
```

#### `RatTailTonic`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated RatTailTonic inventory icon. Subject: one small corked potion vial filled with deep ruby-red lingonberry tonic, squat clear glass with a short neck, a simple dark cork and a narrow cream label with no writing; no glow or extra fruit.
```

#### `BadgerFurVest`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated BadgerFurVest inventory icon. Subject: one compact sleeveless fur vest shown front-on, layered silvery-gray and warm ivory badger fur with a dark brown leather edging and two simple ties; clean wearable garment silhouette, no mannequin.
```

#### `RoastedBadger`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated RoastedBadger inventory icon. Subject: one single roasted meat cut, a compact cooked haunch with a deep golden-brown crust, warm reddish-brown facets and a pale cut edge; appetizing cooked food, no bone, no skewer, no plate or garnish.
```

#### `FangArrows`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated FangArrows inventory icon. Subject: a small tied bundle of four straight wooden arrows laid diagonally together, each with a pale ivory fang-shaped sharp head and short dark feather fletching; crisp arrow silhouettes, no venom drip.
```

#### `ScaleCuirass`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ScaleCuirass inventory icon. Subject: one compact chest cuirass made from overlapping dark olive giant snake scales on a warm bronze-brown frame, front-facing with a clear armor silhouette and a few broad raised scale plates; no spikes or wearer.
```

#### `Antidote`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated Antidote inventory icon. Subject: one small corked potion vial filled with clear muted teal-green antidote, squat rounded glass with a short neck and a plain cream band with no writing; clean, calm colors, no glow or extra herbs.
```

#### `PeatHeartStew`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PeatHeartStew inventory icon. Subject: one sturdy dark earthenware bowl filled with thick brown peat-heart stew, visible chunky potato and onion pieces in a warm amber broth; appetizing meal icon, no spoon, no garnish or steam.
```

#### `WolfPeltBoots`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfPeltBoots inventory icon. Subject: one pair of sturdy ankle-high leather boots with compact gray-brown wolf-fur cuffs, warm dark leather soles and simple laces, shown together as a clear wearable equipment silhouette; no wearer.
```

#### `FangSpear`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated FangSpear inventory icon. Subject: one primitive straight wooden spear with a long pale ivory fang-shaped point securely lashed to the tip with dark leather cord, simple balanced one-handed weapon silhouette; no blood, no extra gear.
```

#### `WolfStew`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated WolfStew inventory icon. Subject: one sturdy dark earthenware bowl filled with rich red-brown wolf stew, visible tender meat chunks, orange carrot pieces and pale onion segments in a warm broth; appetizing meal icon, no spoon, no garnish or steam.
```

#### `LynxMantle`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated LynxMantle inventory icon. Subject: one compact short fur mantle arranged front-facing as a wearable cape, warm russet lynx fur with a few subtle dark spots and pale cream edging, a simple small bronze-brown clasp; no mannequin or wearer.
```

#### `PolarBearCoat`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated PolarBearCoat inventory icon. Subject: one heavy front-facing chest coat made from thick ivory polar bear fur, broad plush collar and shoulders over a dark brown leather body with simple closures; clear garment silhouette, no mannequin or wearer.
```

#### `HornBow`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated HornBow inventory icon. Subject: one elegant primitive longbow, warm dark densewood grip and curved pale cream horn limbs joined by a visible taut string, shown alone in a slight diagonal, clear complete weapon silhouette; no arrow, no spark or glow.
```

#### `ThunderMaul`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects. Primary request: create a new, isolated ThunderMaul inventory icon. Subject: one heavy two-handed war maul with a long dark wood handle and a broad squared charcoal-iron head, a small muted blue-gray thunderstone core inset on each visible face; rugged crafted weapon, no lightning bolts or emitted light.
```

#### `ReefHarpoon`

```text
Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated ReefHarpoon item icon. Subject: one compact spear-like reef harpoon with a light warm-brown wood shaft, a dark iron barbed head and a pale weathered reef-tooth inset near the point, simple crafted bindings and a clear one-handed weapon silhouette; no rope, no water, no fish. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no character, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.
```

<a id="a9-icon-batch"></a>

## A9 gem and gear icon provenance

Generated with the built-in image-generation tool on 2026-10-22. Each original is retained as a transparent PNG at `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`; prepared assets are 64×64 transparent RGBA PNGs at `Content/Kalmala/UI/Source/Icons/<ID>.png`; imported packages are `Content/Kalmala/UI/Icons/Items/<ID>.uasset`.

The 20 canonical identities were imported in pinned batches AL (16) and AM (4), within the importer's 16-texture limit. Every identity has a catalogue-icon manifest row, a `KalmalaIconWidget` fallback mapping, and focused `Validate-UIIcons.ps1 -Category Catalogue -Id <ID>` coverage.

A user-directed A9 data/icon batch used the proposed stats and recipe values in `alpha-item-details.md`. This provenance records icon creation only; gem acquisition sources remain disabled, and accessory effect fields remain catalogue metadata until equipment behavior is implemented.

### Exact generation prompts

#### `SmokyQuartz`

> Create original inventory icon artwork for a fantasy survival game's gem item named Smoky Quartz. One large faceted smoky quartz crystal, translucent warm gray-brown crystal with subtle pale internal inclusions, rugged natural uneven shard shape, realistic hand-painted game inventory icon, crisp readable silhouette, centered, fills about 78% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `Moonstone`

> Create original inventory icon artwork for a fantasy survival game's gem item named Moonstone. One polished oval moonstone cabochon, milky silver-white with a quiet pale blue sheen across its surface, soft layered internal glow effect kept subtle and non-emissive. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `Garnet`

> Create original inventory icon artwork for a fantasy survival game's gem item named Garnet. One deep wine-red garnet crystal, faceted but naturally irregular, rich translucent burgundy facets with a small warm highlight. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `GreenJade`

> Create original inventory icon artwork for a fantasy survival game's gem item named GreenJade. One vivid natural green jade stone, compact rough cut shard with smooth polished facets, deep emerald and celadon variation. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `Amethyst`

> Create original inventory icon artwork for a fantasy survival game's gem item named Amethyst. One small amethyst crystal cluster, saturated violet-purple quartz points with clear facets and natural pale-lilac internal bands. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `Aquamarine`

> Create original inventory icon artwork for a fantasy survival game's gem item named Aquamarine. One blue-green aquamarine crystal, clear sea-glass teal faceted shard with cool cyan and pale green internal facets. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `Tourmaline`

> Create original inventory icon artwork for a fantasy survival game's gem item named Tourmaline. One elongated dark green tourmaline crystal with natural vertical striations and a deep forest-green sheen. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `StarOpal`

> Create original inventory icon artwork for a fantasy survival game's gem item named StarOpal. One rounded rough white opal stone with subtle tiny blue, gold and rose pinfire colors inside milky translucent layers, no emitted light. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `StormSapphire`

> Create original inventory icon artwork for a fantasy survival game's gem item named StormSapphire. One deep cobalt-blue sapphire crystal, crisp angular facets with small lighter blue reflections. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `DuskDiamond`

> Create original inventory icon artwork for a fantasy survival game's gem item named DuskDiamond. One charcoal-black diamond crystal with dark violet and graphite facets, elegant and visibly distinct from blue sapphire. Realistic hand-painted 3D game inventory icon, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated object only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `QuartzNecklace`

> Create original inventory icon artwork for a fantasy survival game's crafted gear item named QuartzNecklace. A compact dark bronze necklace chain arranged in a tidy oval, holding one large smoky quartz pendant in a simple pronged bronze setting. Realistic hand-painted 3D game inventory icon, elegant but practical medieval craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, gems other than the named stone, or other objects.

#### `MoonstoneRing`

> Create original inventory icon artwork for a fantasy survival game's crafted gear item named MoonstoneRing. A small warm bronze ring band facing forward with one pale silver-blue moonstone cabochon in a plain setting. Realistic hand-painted 3D game inventory icon, elegant but practical medieval craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, gems other than the named stone, or other objects.

#### `GarnetNecklace`

> Create original inventory icon artwork for a fantasy survival game's crafted gear item named GarnetNecklace. A compact antique bronze necklace chain arranged in a tidy oval, holding one deep red garnet pendant in a simple pronged bronze setting. Realistic hand-painted 3D game inventory icon, elegant but practical medieval craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, gems other than the named stone, or other objects.

#### `JadeRing`

> Create original inventory icon artwork for a fantasy survival game's crafted gear item named JadeRing. A small warm bronze ring band facing forward with one rich green jade cabochon in a plain setting. Realistic hand-painted 3D game inventory icon, elegant but practical medieval craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, gems other than the named stone, or other objects.

#### `AmethystNecklace`

> Create original inventory icon artwork for a fantasy survival game's crafted gear item named AmethystNecklace. A compact antique bronze necklace chain arranged in a tidy oval, holding one violet amethyst pendant in a simple pronged bronze setting. Realistic hand-painted 3D game inventory icon, elegant but practical medieval craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, gems other than the named stone, or other objects.

#### `AquamarineRing`

> Create original inventory icon artwork for a fantasy survival game's A9 gear item named AquamarineRing. A small warm bronze ring band facing forward with one clear blue-green aquamarine stone in a plain setting. Realistic hand-painted 3D game inventory icon, practical Nordic-inspired fantasy craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `TourmalineNecklace`

> Create original inventory icon artwork for a fantasy survival game's A9 gear item named TourmalineNecklace. A compact antique bronze necklace chain arranged in a tidy oval, holding one dark forest-green tourmaline pendant in a simple pronged bronze setting. Realistic hand-painted 3D game inventory icon, practical Nordic-inspired fantasy craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `OpalHelm`

> Create original inventory icon artwork for a fantasy survival game's A9 gear item named OpalHelm. A practical medieval iron helmet with a rounded nasal guard and a single small milky star opal inset at the brow, dark forged metal, no horns. Realistic hand-painted 3D game inventory icon, practical Nordic-inspired fantasy craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `StormcutSword`

> Create original inventory icon artwork for a fantasy survival game's A9 gear item named StormcutSword. A one-handed straight iron sword with a practical crossguard and a small cobalt sapphire set into the pommel; plain blade with no lightning effects. Realistic hand-painted 3D game inventory icon, practical Nordic-inspired fantasy craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

#### `DuskRing`

> Create original inventory icon artwork for a fantasy survival game's A9 gear item named DuskRing. A small aged bronze ring band facing forward with one near-black diamond containing restrained violet facets in a plain setting. Realistic hand-painted 3D game inventory icon, practical Nordic-inspired fantasy craft, crisp readable silhouette, centered, fills about 72% of a square canvas. Isolated item only on a fully transparent background. No ground, cast shadow, glow, frame, UI, text, labels, logo, watermark, or other objects.

\n

<a id="archery-icons"></a>

## Bow, Bow String, Sinew and Arrow icon provenance

### Batch I — Bow, Wooden Arrow and retired String icon

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

### Batch K — Bow String and Sinew (2026-10-19)

Generated with the built-in image-generation tool as standalone transparent
1254×1254 RGBA originals. The prepared icons are centered 64×64 RGBA images
with alpha. `Prepare-CatalogueIconBatch.py --batch K` prepared them; the
content-only Unreal importer created `/Game/Kalmala/UI/Icons/Items/BowString`
and `/Game/Kalmala/UI/Icons/Items/Sinew`, and per-ID catalogue icon validation
passed for both. The originals, prepared images, packages, runtime IDs and
validation rows are retained. The full 73-row catalogue icon audit passed on
2026-10-19; rerun it during final A0 verification.

### Historical batch I exact generation prompts

#### Retired String artwork

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: a compact coil of rough flax string with a short loose end, warm pale beige twisted plant fibres, clearly thin cord rather than thick rope.

#### Bow

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: one simple wooden hunting bow, slender curved light-brown wooden limbs, wrapped grip and taut thin flax bowstring, diagonally oriented lower-left to upper-right, no arrow.

#### WoodenArrow

Use case: stylized-concept. Asset type: Kalmala survival fantasy inventory icon. Generate NEW original artwork, supplied Stone icon is ONLY a hand-painted material/rendering style reference. Warm natural materials, strong dark brown contour, tactile painterly highlights, bold silhouette legible at 64x64. Centered object with clean transparent padding filling 78% of square. True transparent background. No text, UI, border, ground, cast shadow, glow, logos or unrelated props. Subject: three simple primitive wooden arrows grouped diagonally lower-left to upper-right, sharpened wooden points and muted feather fletching, no metal heads, no bow.

### Batch K exact generation prompts

#### BowString

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a 64-pixel game item. Primary request: create one original BowString icon made from animal sinew. Subject: a compact hand-twisted coil of thin, tightly braided warm ivory and pale tan sinew cord, with one short loose end, clearly flexible thin cord rather than rope or plant fibre. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy material colors, strong clear silhouette readable at 64 pixels. Composition: one centered coil filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, bow, arrow, leaves, flax or extra objects.

#### Sinew

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a 64-pixel game item. Primary request: create one original Sinew material icon. Subject: three short, narrow, softly curved animal tendon strands laid together as a compact bundle, warm ivory and muted pale tan with subtle fibrous ends, clearly raw crafting sinew and distinct from a coiled cord. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy natural materials, strong clear silhouette readable at 64 pixels. Composition: one centered bundle filling about 78 percent of a square canvas with clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, bowstring coil, bow, arrow, hide, bone or extra objects.

### Wooden Arrow revision — sharpened wood only

On 2026-10-15 the user requested removing the broad bronze-looking heads.
The built-in image editor replaced all three front ends with continuous wooden
shafts whittled to narrow points, without separate heads or tip bindings.
The feathers, diagonal arrangement, painted style and transparency are retained.
The revised original and prepared icon were visually inspected, reimported and
validated; the 66-row icon audit passed. String and Bow asset bytes were preserved.

Exact edit prompt:

Use case: precise-object-edit. Edit the supplied WoodenArrow inventory icon. Replace ALL THREE broad attached arrowheads with plain wooden shaft tips, each whittled to a slender tapered point directly from the SAME single piece of wood as its shaft. Continuous wood grain running all the way into each tip. The shaft stays narrow and simply tapers at its end like a sharpened wooden stick or pencil. Remove all separate triangle/diamond heads, metal, bronze, copper, stone, tip sockets and bindings near the tips. Absolutely NO attached arrowheads, NO metallic sheen, NO bronze tip. Preserve the three diagonal arrows, feather fletching and bindings at the feather ends, hand-painted survival-game style, colors, dark outlines, composition and true transparent background. Only change the pointed front ends and remove their attachment bindings. Keep clean transparent padding.

### Batch Q — Stone Arrow (2026-10-21)

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

### Batch R — Bronze Arrow (2026-10-21)

Generated with the built-in image-generation tool using the Stone Arrow original
as a style reference only. The original has three wood-shaft arrows with forged
bronze heads, distinct from the Stone Arrow's pale chipped tips. It was prepared
at 64×64 and imported as `/Game/Kalmala/UI/Icons/Items/BronzeArrow`; the
canonical icon manifest and runtime mapping use `BronzeArrow`.

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Input image: style reference only; do not reproduce its stone arrowheads. Primary request: create a new, isolated BronzeArrow icon. Subject: three primitive wooden arrows grouped diagonally lower-left to upper-right, each with a small forged bronze leaf-shaped arrowhead directly fitted to its shaft, clearly warm copper-gold metal with a few broad hand-painted hammer facets; plain wooden shafts and muted feather fletching. The bronze tips must be visually distinct from pale gray stone tips and sharpened bare wood. Style: small hand-painted game inventory artwork matching the reference's tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; strong clear silhouette readable at 64 pixels. Composition: three centered arrows filling about 78 percent of a square canvas with clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, bow, stone, bindings near the arrowheads, or extra objects.

The retained original is `Content/Kalmala/UI/Source/IconOriginals/BronzeArrow.png`;
the prepared image and imported Texture2D are in batch R. The transparent original
and the 64×64 prepared icon were visually inspected. The icon is validated by
`Scripts/Validate-UIIcons.ps1 -Category Catalogue`.

<a id="armor-icons"></a>

## Wearable gear icon provenance

The six original icons were generated with the built-in image-generation tool.
The committed Bronze Axe original was supplied as a style reference. Each source
is a transparent 1254×1254 RGBA PNG. Prepared 64×64 RGBA versions are imported
as Texture2D assets in batch G; no existing image was replaced.

Shared prompt details: a crisp hand-painted survival-fantasy game inventory
icon; bold dark contour; warm directional highlights; tactile wood or hide;
readable silhouette at 64×64; centered item filling about 78% of the square;
clean transparent padding; no text, logo, border, shadow, glow, extra props, or
duplicate objects. Every prompt used `Content/Kalmala/UI/Source/IconOriginals/BronzeAxe.png`
as the style reference.

| Identity | Prompt subject | Original PNG | Prepared PNG | Texture2D |
| --- | --- | --- | --- | --- |
| `WoodShield` | Compact round ash-plank shield with a bronze rim and leather straps; front view, angled slightly, no person. | `Content/Kalmala/UI/Source/IconOriginals/WoodShield.png` | `Content/Kalmala/UI/Source/Icons/WoodShield.png` | `Content/Kalmala/UI/Icons/Items/WoodShield.uasset` |
| `BoarHideHelmet` | Open-face cap from layered brown boar hide, visible stitching and short leather brow guard; three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideHelmet.png` | `Content/Kalmala/UI/Source/Icons/BoarHideHelmet.png` | `Content/Kalmala/UI/Icons/Items/BoarHideHelmet.uasset` |
| `BoarHideCape` | Short shoulder cape from warm brown boar hide, simple clasp and stitched edging; three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideCape.png` | `Content/Kalmala/UI/Source/Icons/BoarHideCape.png` | `Content/Kalmala/UI/Icons/Items/BoarHideCape.uasset` |
| `BoarHideTunic` | Sleeveless chest tunic from layered brown boar hide, simple leather ties and handmade stitching; front three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideTunic.png` | `Content/Kalmala/UI/Source/Icons/BoarHideTunic.png` | `Content/Kalmala/UI/Icons/Items/BoarHideTunic.uasset` |
| `BoarHideLeggings` | Pair of rugged brown boar-hide leg guards joined by leather ties, shown as the garment alone; front three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideLeggings.png` | `Content/Kalmala/UI/Source/Icons/BoarHideLeggings.png` | `Content/Kalmala/UI/Icons/Items/BoarHideLeggings.uasset` |
| `BoarHideBoots` | Matched pair of sturdy ankle-high brown boar-hide boots with leather soles and rough stitching; three-quarter view, no person. | `Content/Kalmala/UI/Source/IconOriginals/BoarHideBoots.png` | `Content/Kalmala/UI/Source/Icons/BoarHideBoots.png` | `Content/Kalmala/UI/Icons/Items/BoarHideBoots.uasset` |

To prepare or reimport these assets, run:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch G
Scripts/Import-CatalogueIconBatch.ps1 -Batch G
```

The canonical IDs and texture asset names are listed in
`Scripts/Data/catalogue-icon-manifest.csv`.

Bronze Helmet was generated with the built-in image-generation tool on
2026-10-21 as a standalone transparent 1254×1254 RGBA original. Its retained
original was prepared to 64×64 RGBA and imported through the single-item S
batch; the six initial wearable icons in batch G were not replaced.

| Identity | Prompt subject | Original PNG | Prepared PNG | Texture2D |
| --- | --- | --- | --- | --- |
| `BronzeHelmet` | Single rounded hammered-bronze cap with a short brow ridge and narrow boar-hide lining; three-quarter front view, no wearer. | `Content/Kalmala/UI/Source/IconOriginals/BronzeHelmet.png` | `Content/Kalmala/UI/Source/Icons/BronzeHelmet.png` | `Content/Kalmala/UI/Icons/Items/BronzeHelmet.uasset` |
| `BronzeTunic` | Sleeveless plated-bronze chest tunic over a dark boar-hide backing; front three-quarter view, no wearer. | `Content/Kalmala/UI/Source/IconOriginals/BronzeTunic.png` | `Content/Kalmala/UI/Source/Icons/BronzeTunic.png` | `Content/Kalmala/UI/Icons/Items/BronzeTunic.uasset` |
| `BronzeLeggings` | Pair of fitted hammered-bronze leg guards over dark boar-hide backing; front three-quarter view, no wearer. | `Content/Kalmala/UI/Source/IconOriginals/BronzeLeggings.png` | `Content/Kalmala/UI/Source/Icons/BronzeLeggings.png` | `Content/Kalmala/UI/Icons/Items/BronzeLeggings.uasset` |
| `BronzeShield` | Compact round lightwood shield reinforced with a hammered-bronze rim and boss, with dark boar-hide backing; front three-quarter view. | `Content/Kalmala/UI/Source/IconOriginals/BronzeShield.png` | `Content/Kalmala/UI/Source/Icons/BronzeShield.png` | `Content/Kalmala/UI/Icons/Items/BronzeShield.uasset` |

Bronze Shield was generated with the built-in image-generation tool on
2026-10-21 as a standalone transparent RGBA original. Its retained original
was prepared to 64×64 RGBA and imported through the single-item V batch; prior
armor icons were not replaced. The prompt requested a compact round shield
with a lightwood plank core, hammered-bronze rim and boss, and dark boar-hide
backing, centered in the warm hand-painted survival-fantasy inventory style
with clean transparent padding and no text, UI, ground, shadow, glow, logo or
extra objects.

The prompt used the existing warm survival-fantasy inventory treatment: dark
contours, broad painted metal facets, warm highlights, centered silhouette,
transparent padding, and no text, props, glow, or background.

Bronze Tunic was generated with the built-in image-generation tool on
2026-10-21 as a standalone transparent 1254×1254 RGBA original. Its retained
original was prepared to 64×64 RGBA and imported through the single-item T
batch; the existing helmet and six batch-G wearable icons were not replaced.
The prompt requested one sleeveless plated-bronze chest tunic backed with dark
boar hide, centered in a warm hand-painted survival-fantasy inventory style,
with transparent padding and no wearer, text, props, or background.

Bronze Leggings was generated with the built-in image-generation tool on
2026-10-21 as a standalone transparent 1254×1254 RGBA original. Its retained
original was prepared to 64×64 RGBA and imported through the single-item U
+batch; previous armor icons were not replaced. The prompt requested one pair
+of fitted hammered-bronze leg guards over dark boar-hide backing, shown as the
+garment alone from a front three-quarter view, centered in a hand-painted
+survival-fantasy inventory style with clean transparent padding and no text,
+logo, border, shadow, glow, extra props, or duplicate objects.

<a id="badger-fur-icon"></a>

## Badger Fur icon provenance

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

### Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated BadgerFur item icon. Subject: one compact loose tuft of coarse badger fur, a small crafting-material bundle of layered silvery gray and warm ivory fibers with several bold near-black guard hairs, soft tawny-brown undertones, and a clean irregular silhouette; no skin or pelt backing. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, with clean transparent padding. Lighting/mood: soft warm highlights across the coarse fibers. Background: genuinely transparent. Constraints: original artwork; no animal, no body parts, no blood, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.

<a id="badger-meat-icon"></a>

## Badger Meat icon provenance

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

### Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated BadgerMeat item icon. Subject: one compact, fresh cut of raw game meat, a small crafting material portion with a softly angular oblong silhouette, layered deep rose-red and warm red-brown muscle facets, several restrained pale cream fat seams and subtle pink highlights; no bone, hide, fur, or pelt backing. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, with clean transparent padding. Lighting/mood: soft warm highlights across the cut surface. Background: genuinely transparent. Constraints: original artwork; no animal, no body parts, no blood, no gore, no knife, no plate, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.

<a id="basic-torch-icon"></a>

## Basic Torch icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The 1254×1254 RGBA source is prepared to a
64×64 transparent icon in batch W and imported as the `BasicTorch` Texture2D.
No existing artwork or outside reference was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/BasicTorch.png`
- `Content/Kalmala/UI/Source/Icons/BasicTorch.png`
- `Content/Kalmala/UI/Icons/Items/BasicTorch.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch W
Scripts/Import-CatalogueIconBatch.ps1 -Batch W
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated BasicTorch item icon. Subject: one simple hand-held wooden torch, a sturdy dark brown wooden shaft with a compact amber-wrapped flame bundle at the top and a small bright golden flame, unmistakably a basic carried torch; earthy warm palette with restrained amber highlights. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered object filling about 78 percent of a square canvas, shaft angled slightly from lower left to upper right, with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

\n

<a id="berry-recovery-draught-icon"></a>

## Berry Recovery Draught icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as an original
transparent inventory icon. The final artwork is one hand-painted ruby-red
glass vial with a cork and a small berry cluster. It was prepared at 64×64 for
catalogue import batch Z.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/BerryRecoveryDraught.png`
- `Content/Kalmala/UI/Source/Icons/BerryRecoveryDraught.png`
- `Content/Kalmala/UI/Icons/Items/BerryRecoveryDraught.uasset`

Workflow:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch Z
Scripts/Import-CatalogueIconBatch.ps1 -Batch Z
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The runtime mapping is `BerryRecoveryDraught` in
`Source/KalmalaUI/Private/KalmalaIconWidget.cpp`. The original prompt was:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create one isolated Berry Recovery Draught icon for an inventory grid. Subject: a small squat glass potion vial containing rich ruby-red berry infusion, with a simple cork stopper and a restrained cluster of three tiny red berries with one muted green leaf near the neck; one object only. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors, matching the existing Kalmala 64-pixel item icon treatment. Composition/framing: one centered object filling about 78 percent of a square canvas, clear readable silhouette with clean transparent padding. Materials/textures: softly beveled glass, opaque red liquid, simple cork; broad painted facets rather than fine detail. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

<a id="bone-icon"></a>

## Bone fragment icon provenance

Generated with the built-in image-generation tool on 2026-10-15. The existing
original `Stone.png` was a style reference only. No existing artwork was replaced.
The retained original is 1254x1254 RGBA with transparency; the prepared image is
64x64 RGBA. Batch H imports it as a UI Texture2D, preserving alpha.

- Original: `Content/Kalmala/UI/Source/IconOriginals/BoneFragment.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/BoneFragment.png`
- Imported: `Content/Kalmala/UI/Icons/Items/BoneFragment.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/BoneFragment.BoneFragment`

### Generation prompt

Use case: stylized-concept. Generate a NEW original Bone fragment inventory item icon for Kalmala, using the supplied Stone icon ONLY as a rendering/style reference, replacing the subject completely. Subject: one short weathered animal long-bone fragment with one recognizable rounded joint end and a jagged broken hollow end, diagonally oriented from lower-left to upper-right. Dry ivory bone, warm cream highlights, muted tan weathering in cracks and porous broken edge, dark brown contour, tactile hand-painted survival-fantasy rendering matching the reference. Bold simple silhouette readable at 64x64. Centered isolated object filling approximately 78 percent of the square with clean transparent padding. True transparent background. No stones, no skull, no blood, no flesh, no ground, no cast shadow, no glow, no text, no logos, no border, no extra props.

### Reproduction and validation

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch H
Scripts/Import-CatalogueIconBatch.ps1 -Batch H
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The original and prepared images were visually inspected. The content-only
Unreal importer loaded and validated the imported texture; all 63 ledger rows
passed the file validator. The icon mapping includes a dedicated bone vector
fallback, and the global icon automation now requires the imported BoneFragment
texture with every other mapped item. A gameplay editor build, global Unreal
automation and in-game rendering remain deferred. Existing missing material
icons and the duplicate shield recipe are separate unresolved issues.

<a id="bronze-sword-icon"></a>

## Bronze Sword icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. No external or existing image was used as a
reference.

- Original: `Content/Kalmala/UI/Source/IconOriginals/BronzeSword.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/BronzeSword.png`
- Imported texture: `Content/Kalmala/UI/Icons/Items/BronzeSword.uasset`
- Icon ledger: `Scripts/Data/catalogue-icon-manifest.csv`, batch M

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated BronzeSword item icon. Subject: one compact one-handed bronze sword, a warm bronze double-edged blade, simple bronze crossguard and pommel, dark wooden grip wrapped in pale deer hide. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered sword set diagonally from lower-left grip to upper-right point, filling about 78 percent of a square canvas, with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, gems, or extra objects.

The prepared 64×64 RGBA image was visually inspected. The batch-M importer
created `/Game/Kalmala/UI/Icons/Items/BronzeSword.BronzeSword` as a transparent
UI Texture2D.

<a id="camp-bed-icon"></a>

## Camp Bed icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original was
prepared as a centered 64×64 RGBA texture and imported as `CampBedKit` in batch
AF. The first render contained an opaque checkerboard; an image edit removed
only that backdrop. The final source has transparent pixels and was visually
checked before preparation.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/CampBedKit.png`
- `Content/Kalmala/UI/Source/Icons/CampBedKit.png`
- `Content/Kalmala/UI/Icons/Items/CampBedKit.uasset`

The icon manifest maps the `CampBed` buildable and recipe to the `CampBedKit`
texture. It depicts a placed object, never a portable inventory item or kit.

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AF
Scripts/Import-CatalogueIconBatch.ps1 -Batch AF
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id CampBedKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create an isolated Camp Bed buildable icon. Subject: one compact hand-built timber bed, sized for one person, shown from a clear three-quarter angle with the full 2 m by 1 m footprint readable; sturdy wooden frame with four short legs, layered wooden slats, and a simple pale natural linen bedroll, no pillow, no blanket folds covering the frame. Style: small hand-painted game inventory artwork matching an earthy crafted survival-game catalogue, bold dark brown contour, broad tactile painted facets, restrained highlights, warm aged timber browns and muted cream linen. Composition: one centered object filling about 78 percent of a square canvas, clean transparent padding, silhouette legible at 64 pixels. Background: genuinely transparent. Constraints: original artwork, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, person, metal fittings, headboard, extra objects, or copied references.

Edit instruction:

> Preserve the illustrated timber bed unchanged: keep the same shape, four legs, slats, linen bedroll, angle, colors, contour, scale and framing. Remove only the gray checkerboard backdrop and make everything outside the bed silhouette genuinely transparent, with clean antialiased alpha edges. Do not draw any background pattern, shadow, floor, text, or extra object.

<a id="charcoal-kiln-icon"></a>

## Charcoal Kiln build-menu icon provenance

The Charcoal Kiln construction icon was generated with the built-in image
generation tool on 2026-10-20. It is an original transparent PNG for the
construction-only `CharcoalKilnKit` identity. The existing preparation script
creates its centered 64×64 version and the icon importer creates the runtime
Texture2D in batch K.

Files:

- `Content/Kalmala/UI/Source/IconOriginals/CharcoalKilnKit.png`
- `Content/Kalmala/UI/Source/Icons/CharcoalKilnKit.png`
- `Content/Kalmala/UI/Icons/Items/CharcoalKilnKit.uasset`

The icon uses the existing batch workflow:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch K
Scripts/Import-CatalogueIconBatch.ps1 -Batch K
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id CharcoalKilnKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon for the placeable Charcoal Kiln. Primary request: create one isolated charcoal kiln icon. Subject: a compact squat stone charcoal kiln, a hand-built rounded-square masonry chamber with a small dark arched loading mouth and a short capped stone vent; make the silhouette clearly read as a kiln structure, not a forge or cooking pot. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy stone gray and soot-brown colors, readable at 64 pixels. Composition: one centered object filling about 78 percent of a square canvas, slight three-quarter view, clean transparent padding. Lighting: soft warm highlight on stone edges, no glow. Materials: rough fitted stones with a few soot marks, no loose fuel or props. Constraints: original artwork, genuinely transparent background, no text, letters, labels, UI, border, ground, cast shadow, watermark, logo, people, fire, or extra objects.

<a id="forest-chanterelle-icon"></a>

## Forest Chanterelle icon provenance

Generated with the built-in image-generation tool as an original transparent
inventory icon. The artwork shows three golden-orange chanterelle mushrooms
with wavy funnel caps, pale stems, and subtle forked ridges.

- Original: `Content/Kalmala/UI/Source/IconOriginals/ForestChanterelle.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/ForestChanterelle.png`
- Imported: `Content/Kalmala/UI/Icons/Items/ForestChanterelle.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/ForestChanterelle.ForestChanterelle`
- Batch: Y

### Generation prompt

Use case: stylized-concept. Asset type: original Kalmala survival-fantasy
inventory icon. Primary request: create a new, isolated Forest Chanterelle
item icon. Subject: a small cluster of three wild chanterelle mushrooms,
recognizable by golden-orange wavy funnel caps, pale cream stems, and subtle
forked ridges beneath the caps. Style/medium: tactile hand-painted game
inventory artwork with broad painted facets, a bold dark-brown contour,
restrained highlights, and earthy colors, matching the established Kalmala
small-item icon treatment. Composition/framing: one centered mushroom cluster
filling about 78 percent of a square canvas, clear and readable at 64 pixels,
with clean transparent padding. Constraints: original artwork; genuinely
transparent background; no text, label, UI, border, ground, soil, cast shadow,
glow, leaves, berries, basket, watermark, logo, or extra objects.

The item is hand-gathered without a tool from a deterministic, server-selected
Elderwood patch. Each accepted depletion grants exactly two Forest
Chanterelles. Eating one uses the existing non-stacking Steady Meal effect.
The world patch is the acquisition route; there is no crafting recipe,
equipment profile, upgrade cost, or attack data.

Prepare and import the retained artwork with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch Y
Scripts/Import-CatalogueIconBatch.ps1 -Batch Y
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

<a id="garden-plot-icon"></a>

## Garden Plot icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent build-menu artwork. The 1254×1254 RGBA source is prepared to a
64×64 texture and imported as `GardenPlotKit` in isolated batch AB. The image
was visually checked at 64×64. No reference image or existing artwork was
used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/GardenPlotKit.png`
- `Content/Kalmala/UI/Source/Icons/GardenPlotKit.png`
- `Content/Kalmala/UI/Icons/Items/GardenPlotKit.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AB
Scripts/Import-CatalogueIconBatch.ps1 -Batch AB
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id GardenPlotKit
```

The manifest registers this as a `buildable` icon for the `GardenPlot` recipe.
Its `GardenPlot` recipe identity resolves to the `GardenPlotKit` texture. This
is presentation for a directly placed object and does not create an inventory
item or a portable plot kit.

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create an isolated GardenPlotKit icon. Subject: a single compact square garden bed made from low, rough dark-brown timber edging around rich, freshly tilled dark soil; the bed is empty and ready for planting, with no crop or sprout. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark-brown contour, restrained painted highlights, and earthy wood-and-soil colors; clear silhouette at 64-pixel icon scale. Composition: one centered square plot seen from a slight three-quarter overhead angle, filling about 78 percent of a square canvas with clean transparent padding. Constraints: genuinely transparent background; original artwork; no reference image, plants, seeds, tools, loose stones, text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, people, or extra objects.

<a id="iron-sword-icon"></a>

## Iron Sword icon provenance

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

<a id="light-bow-icon"></a>

## Light Bow icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The Wooden Club prepared icon was used only as a
style reference; no outside or third-party image was used.

- Original: `Content/Kalmala/UI/Source/IconOriginals/LightBow.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/LightBow.png`
- Imported texture: `Content/Kalmala/UI/Icons/Items/LightBow.uasset`
- Icon ledger: `Scripts/Data/catalogue-icon-manifest.csv`, batch P

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Input image: Image 1 is a style reference only; do not reuse or depict its subject. Primary request: create a new isolated LightBow item icon that remains clearly recognizable when reduced to 64x64 pixels. Subject: one compact traditional lightwood bow with a taut dark bowstring; no arrow. Style: small hand-painted game inventory art matching the reference's bold dark brown contour, broad painted facets, simple warm highlights and earthy wood. Composition: show the whole bow in a compact diagonal arc, grip prominently at the center, thick sturdy curved limbs with a broad silhouette (wood limbs should remain clearly visible at 64px; do not make hair-thin lines), distinct taut string visibly separated from the stave. Centered object fills about 78 percent of a square canvas with clean transparent padding. Background: genuinely transparent. Constraints: original bow design; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, arrow, hand, or extra objects.

The retained original is 1254x1254 transparent RGBA. Batch P preparation produced
a centered 64x64 RGBA icon. The importer created and validated
`/Game/Kalmala/UI/Icons/Items/LightBow.LightBow`; catalogue validation checks the
original, prepared PNG, runtime mapping, and imported package.

<a id="lingonberry-icon"></a>

## Lingonberry icon provenance

Generated with the built-in image-generation tool as an original transparent
inventory icon. The artwork shows ripe red lingonberries on a leafy sprig in
the established hand-painted Kalmala small-item style.

- Original: `Content/Kalmala/UI/Source/IconOriginals/Lingonberry.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/Lingonberry.png`
- Imported: `Content/Kalmala/UI/Icons/Items/Lingonberry.uasset`
- Runtime: `/Game/Kalmala/UI/Icons/Items/Lingonberry.Lingonberry`
- Batch: X

### Generation prompt

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

<a id="mace-icons"></a>

## Mace inventory icons

Generated with the built-in imagegen tool, using transparent backgrounds. Original files are retained; prepared 64×64 RGBA images are used by the Texture2D importer. No existing art was overwritten.

### StoneMace

- Original: `Content/Kalmala/UI/Source/IconOriginals/StoneMace.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/StoneMace.png`
- Unreal asset: `Content/Kalmala/UI/Icons/Items/StoneMace.uasset`

Exact generation prompt:

Original survival fantasy inventory item icon: a rough grey stone mace head lashed securely to a short dark wooden handle with brown leather ties; a primitive blunt stone club, no metal. A single compact one-handed mace shown diagonally, head at upper left and handle at lower right, matching a realistic hand-painted northern wilderness game inventory. Warm directional lighting, earthy textured materials, crisp readable silhouette and beveled shading, generous clean transparent padding, item occupies 82% of square canvas. Transparent background, no text, no frame, no ground shadow, no extra objects, no glow. Distinct mace head visibly larger than grip, unmistakably a blunt crushing weapon.

### BronzeMace

- Original: `Content/Kalmala/UI/Source/IconOriginals/BronzeMace.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/BronzeMace.png`
- Unreal asset: `Content/Kalmala/UI/Icons/Items/BronzeMace.uasset`

Exact generation prompt:

Original survival fantasy inventory item icon: a stout warm bronze flanged mace head on a dark walnut handle with a wrapped brown leather grip; dull gold bronze highlights. A single compact one-handed mace shown diagonally, head at upper left and handle at lower right, matching a realistic hand-painted northern wilderness game inventory. Warm directional lighting, earthy textured materials, crisp readable silhouette and beveled shading, generous clean transparent padding, item occupies 82% of square canvas. Transparent background, no text, no frame, no ground shadow, no extra objects, no glow. Distinct mace head visibly larger than grip, unmistakably a blunt crushing weapon.

### IronMace

- Original: `Content/Kalmala/UI/Source/IconOriginals/IronMace.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/IronMace.png`
- Unreal asset: `Content/Kalmala/UI/Icons/Items/IronMace.uasset`

Exact generation prompt:

Original survival fantasy inventory item icon: a dark iron flanged mace head on a dark walnut handle with a wrapped brown leather grip; grey steel edge highlights. A single compact one-handed mace shown diagonally, head at upper left and handle at lower right, matching a realistic hand-painted northern wilderness game inventory. Warm directional lighting, earthy textured materials, crisp readable silhouette and beveled shading, generous clean transparent padding, item occupies 82% of square canvas. Transparent background, no text, no frame, no ground shadow, no extra objects, no glow. Distinct mace head visibly larger than grip, unmistakably a blunt crushing weapon.

<a id="material-icons"></a>

## Bronze, copper, tin and ore icon provenance

Generated with the built-in image-generation tool on 2026-10-15. The six
originals are standalone transparent RGBA inventory icons. They were prepared
at 64×64 and imported into catalogue texture batch J. Each icon uses the
Kalmala small-item treatment: a clear silhouette, dark brown contour, broad
painted facets and restrained material highlights. No existing artwork was
replaced and no outside image was used as a reference.

For each identity, retained files are:

- `Content/Kalmala/UI/Source/IconOriginals/<ID>.png`
- `Content/Kalmala/UI/Source/Icons/<ID>.png`
- `Content/Kalmala/UI/Icons/Items/<ID>.uasset`

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch J
Scripts/Import-CatalogueIconBatch.ps1 -Batch J
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

### Exact generation prompts

Each prompt used the following shared text, with the identity and subject
replaced as shown below:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated `<ID>` item icon. Subject: `<SUBJECT>`. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition: one centered object filling about 78 percent of a square canvas, with clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

| ID | Subject text |
|---|---|
| `Bronze` | a single compact forged bronze ingot, warm golden copper-brown alloy, clearly a rectangular tapered metal bar with softly beveled ends and a few broad hand-painted hammer facets |
| `Copper` | a single compact forged copper ingot, distinctive rich reddish orange metal with warm peach highlights, clearly a rectangular tapered metal bar with softly beveled ends and a few broad hand-painted hammer facets |
| `Tin` | a single compact forged tin ingot, pale cool silver-gray metal with subtle warm edge reflections, clearly a rectangular tapered metal bar with softly beveled ends and a few broad hand-painted hammer facets |
| `CopperOre` | one irregular chunk of dark stone with a few distinct muted copper-orange mineral seams embedded in it; it must read as rough ore-bearing rock, not a polished metal bar |
| `TinOre` | one irregular chunk of cool medium-gray stone with a few distinct pale silvery mineral seams embedded in it; it must read as rough ore-bearing rock, not a polished metal bar |
| `IronOre` | one irregular chunk of charcoal-gray stone with a few restrained rusty iron-red mineral seams embedded in it; it must read as rough ore-bearing rock, not an iron ingot or polished metal bar |

The six prepared icons were visually inspected at 64×64. Source, preparation,
runtime mapping, and imported-package validation are recorded with the current
equipment icon work in `docs/07-development-setup.md`. Unreal catalogue/icon
automation and in-game rendering remain unexecuted.

<a id="planting-trowel-icon"></a>

## Planting Trowel icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The generated 1254×1254 RGBA source is prepared
to a 64×64 transparent icon in batch AA and imported as the `PlantingTrowel`
Texture2D. The image was visually checked for its wooden handle, bronze neck,
and single broad planting blade. No reference image or existing artwork was
used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/PlantingTrowel.png`
- `Content/Kalmala/UI/Source/Icons/PlantingTrowel.png`
- `Content/Kalmala/UI/Icons/Items/PlantingTrowel.uasset`

Prepare, import and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AA
Scripts/Import-CatalogueIconBatch.ps1 -Batch AA
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The carried-tool profile is in `items/equipment/utility.json`; the Forge
acquisition is in `recipes/stations/forge.json`. It has no attack list or
level-up cost, and uses the existing free Workbench/Forge repair path. Planting,
harvesting, and condition use remain part of A7 plot behavior.

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new, isolated PlantingTrowel item icon. Subject: one compact hand-held garden planting trowel with a sturdy dark brown wooden handle, a short bronze-colored neck, and a single broad, gently cupped, leaf-shaped bronze blade with a rounded tip; it must read clearly as a planting trowel, with one blade and no rake tines. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette at 64-pixel inventory scale. Composition: one centered object filling about 78 percent of a square canvas, handle angled from lower left toward upper right, blade near the upper right, clean transparent padding. Background: genuinely transparent. Constraints: original artwork, no reference image, no soil, plants, leaves, text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.

<a id="rat-tail-icon"></a>

## Rat Tail icon provenance

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
in persistent escrow otherwise. The Rat-tail Tonic recipe is available at the
Cauldron; its owner-validated use restores up to 30 health over 10 seconds,
ends on death or full health, and shares the 60-second recovery cooldown with
the other draughts. The effect is session-only. Full-pack, restart, peer, and
rendered acceptance remain part of combined I4 verification.

### Exact generation prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory material icon. Primary request: create a new, isolated RatTail item icon. Subject: one curled brown rat tail, shown by itself as a small crafting material; tapered, flexible, visibly furred, with a rounded darker tip and a clean cut base, with no blood. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors; clear silhouette matching the scale and readability of a 64-pixel item icon. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding. Background: genuinely transparent. Constraints: original artwork; no rat body, no other objects, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

<a id="reinforced-chest-icon"></a>

## Reinforced Chest icon provenance

Generated with the built-in image-generation tool on 2026-10-22 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original is
prepared to a centered 64×64 RGBA texture and imported as `ReinforcedChestKit`
in isolated batch AN. No reference image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/ReinforcedChestKit.png`
- `Content/Kalmala/UI/Source/Icons/ReinforcedChestKit.png`
- `Content/Kalmala/UI/Icons/Items/ReinforcedChestKit.uasset`

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AN
Scripts/Import-CatalogueIconBatch.ps1 -Batch AN
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id ReinforcedChestKit
```

The icon manifest registers this as a `buildable` identity for the
`ReinforcedChest` recipe. It depicts a placed storage object, not a portable
inventory item or kit.

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon for a placed structure. Primary request: create a distinct ReinforcedChestKit icon that reads as a sturdy storage chest reinforced with bronze. Subject: one squat rectangular hand-built timber chest, closed, viewed from a clear three-quarter angle; visible lid, front latch, and broad bronze corner bands plus two bronze reinforcement straps across the front. Style/medium: small hand-painted game catalogue artwork matching an earthy crafted survival game, tactile broad painted facets, bold dark brown contour, restrained highlights, warm aged wood and muted bronze. Composition/framing: one centered chest filling about 78 percent of a square canvas, clean silhouette readable at 64 pixels, transparent padding. Background: genuinely transparent. Constraints: original artwork, no reference image, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, people, key, extra objects, or portable-kit packaging. Ensure transparency rather than a checkerboard pattern.

<a id="root-stamina-draught-icon"></a>

## Root Stamina Draught icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent inventory artwork. The final icon is one green-gold glass vial
with a cork, a potato and an onion tied near its neck. It was prepared at
64×64 for catalogue import batch Z; no outside image was used as a reference.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/RootStaminaDraught.png`
- `Content/Kalmala/UI/Source/Icons/RootStaminaDraught.png`
- `Content/Kalmala/UI/Icons/Items/RootStaminaDraught.uasset`

Prepare and import with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch Z
Scripts/Import-CatalogueIconBatch.ps1 -Batch Z
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

The runtime mapping is `RootStaminaDraught` in
`Source/KalmalaUI/Private/KalmalaIconWidget.cpp`. The original prompt was:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create one isolated Root Stamina Draught icon for an inventory grid. Subject: a small squat glass vial containing muted green-gold root-vegetable infusion, with a simple cork stopper and a restrained tiny potato and onion cue near the neck; one object only. Style/medium: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights and earthy material colors, matching the existing Kalmala 64-pixel item icon treatment. Composition/framing: one centered object filling about 78 percent of a square canvas, clear readable silhouette with clean transparent padding. Materials/textures: softly beveled glass, opaque pale green-gold liquid, simple cork, tiny matte root-vegetable shapes; broad painted facets rather than fine detail. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo or extra objects.

The original and 64×64 prepared PNGs were validated as transparent RGBA. The
batch importer created the matching Texture2D package and validated its
runtime mapping. In-game rendered appearance and the gameplay automation
remain for I2 milestone-final verification.

<a id="smelter-icon"></a>

## Smelter build-menu icon provenance

Generated with the built-in image-generation tool on 2026-10-20 as an original
transparent icon for the construction-only `SmelterKit` identity. The icon uses
the established small painted style and is distinguishable from the Charcoal
Kiln by its taller capped chimney and side tapping channel. Batch L prepared the
64×64 image and imported its runtime Texture2D.

Files:

- `Content/Kalmala/UI/Source/IconOriginals/SmelterKit.png`
- `Content/Kalmala/UI/Source/Icons/SmelterKit.png`
- `Content/Kalmala/UI/Icons/Items/SmelterKit.uasset`

Workflow:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch L
Scripts/Import-CatalogueIconBatch.ps1 -Batch L
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id SmelterKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon for a placeable metal Smelter. Primary request: create one isolated Smelter construction icon that is clearly distinct from a charcoal kiln. Subject: a compact squat stone smelting furnace with a broad blocky masonry base, a tall narrow capped chimney rising from the rear, and a small dark arched front firebox; add a short side tapping channel that reads as a smelter outlet, with no visible molten metal. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy cool stone gray and soot-brown colors, readable at 64 pixels. Composition: one centered object filling about 78 percent of a square canvas, slight three-quarter view, clean transparent padding. Lighting: soft neutral highlight on stone edges, no glow. Materials: rough fitted stones and dark iron outlet. Constraints: original artwork, genuinely transparent background, no text, letters, labels, UI, border, ground, cast shadow, watermark, logo, people, flames, sparks, loose ore, coal, ingots, or extra objects.

<a id="standing-light-icon"></a>

## Standing Light icon provenance

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

<a id="timber-beam-icon"></a>

## Timber Beam icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original was
prepared to a 64×64 RGBA texture and imported as `TimberBeamKit` in isolated
batch AD. No reference image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/TimberBeamKit.png`
- `Content/Kalmala/UI/Source/Icons/TimberBeamKit.png`
- `Content/Kalmala/UI/Icons/Items/TimberBeamKit.uasset`

The icon manifest registers the `TimberBeam` buildable and recipe identity to
the `TimberBeamKit` texture. This is presentation for a directly placed object,
not an inventory item or portable construction kit.

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AD
Scripts/Import-CatalogueIconBatch.ps1 -Batch AD
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id TimberBeamKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create a standalone icon for a Timber Beam buildable, a single hand-hewn squared timber beam with clear, sturdy support-beam silhouette. Subject: one 2 m long, 0.15 m square wooden structural beam, seen in a three-quarter diagonal view, with subtle end grain visible at both ends and broad carved facets. Style/medium: small hand-painted game inventory artwork, tactile broad facets, bold dark brown contour, restrained painted highlights, natural warm timber colors. Composition/framing: one centered beam filling about 78 percent of a square canvas, clear silhouette readable at 64 pixels. Materials/textures: aged warm brown wood with a few broad grain cuts, no metal fittings. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, wall, door, extra objects, or reference artwork.

<a id="timber-door-icon"></a>

## Timber Door icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original
transparent build-menu artwork. The 1254×1254 RGBA source was prepared to a
64×64 texture and imported as `TimberDoorKit` in isolated batch AC. No reference
image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/TimberDoorKit.png`
- `Content/Kalmala/UI/Source/Icons/TimberDoorKit.png`
- `Content/Kalmala/UI/Icons/Items/TimberDoorKit.uasset`

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AC
Scripts/Import-CatalogueIconBatch.ps1 -Batch AC
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id TimberDoorKit
```

The prompt requested an original stylized TimberDoorKit build-menu icon: one
closed timber door centered in a short wall segment, on a transparent
background, without text or extra objects. The manifest registers the
`TimberDoor` buildable and recipe identity to the `TimberDoorKit` texture. This
is presentation for a directly placed object, not an inventory item or
portable construction kit.

<a id="timber-stairs-icon"></a>

## Timber Stairs icon provenance

Generated with the built-in image-generation tool on 2026-10-21 as original,
transparent build-menu artwork. The retained 1254×1254 RGBA original was
prepared to a centered 64×64 RGBA texture and imported as `TimberStairsKit` in
batch AE. The artwork depicts broad timber treads and side stringers; no
reference image or existing artwork was used.

Retained files:

- `Content/Kalmala/UI/Source/IconOriginals/TimberStairsKit.png`
- `Content/Kalmala/UI/Source/Icons/TimberStairsKit.png`
- `Content/Kalmala/UI/Icons/Items/TimberStairsKit.uasset`

The icon manifest registers the `TimberStairs` buildable and recipe identity
to the `TimberStairsKit` texture. It represents a directly placed structure,
not an inventory item or portable construction kit.

Prepare, import, and validate with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AE
Scripts/Import-CatalogueIconBatch.ps1 -Batch AE
Scripts/Validate-UIIcons.ps1 -Category Catalogue -Id TimberStairsKit
```

Generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy build-menu icon. Primary request: create a standalone transparent icon for a Timber Stairs buildable. Subject: one compact timber staircase with a 2 m run, 1 m width, and 1 m rise, shown from a three-quarter angle so the stepped top profile, side stringers, and solid walkable treads are immediately readable. Style/medium: small hand-painted game inventory artwork, tactile broad facets, bold dark brown contour, restrained painted highlights, warm aged timber colors. Composition/framing: one centered staircase filling about 78 percent of a square canvas, clean silhouette readable at 64 pixels. Materials/textures: hand-hewn brown planks, broad carved wood grain marks, no metal fittings. Constraints: genuinely transparent background; original artwork; no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, ladder, rope, railings, wall, person, or extra objects.

<a id="wooden-club-icon"></a>

## Wooden Club icon provenance

Generated with the built-in imagegen tool on 2026-10-21 as original transparent
inventory artwork. No outside image was used as a reference.

- Original: `Content/Kalmala/UI/Source/IconOriginals/WoodenClub.png`
- Prepared: `Content/Kalmala/UI/Source/Icons/WoodenClub.png`
- Imported texture: `Content/Kalmala/UI/Icons/Items/WoodenClub.uasset`
- Icon ledger: `Scripts/Data/catalogue-icon-manifest.csv`, batch O

Exact generation prompt:

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon. Primary request: create a new isolated WoodenClub item icon. Subject: one simple one-handed wooden club shaped from a sturdy branch, with a naturally thick rounded striking end tapering into a short hand grip. Style: small hand-painted game inventory artwork with tactile broad facets, bold dark brown contour, restrained painted highlights, earthy brown wood, matching a readable 64-pixel item icon. Composition: one centered object shown diagonally, thick end at upper left and grip at lower right, filling about 78 percent of a square canvas with clean transparent padding. Background: genuinely transparent. Constraints: original artwork, a plainly wooden blunt weapon, no stone head, no metal, no bindings, no text, letters, labels, UI, border, ground, cast shadow, glow, watermark, logo, or extra objects.

The transparent 1254×1254 RGBA original was prepared as a centered 64×64 RGBA
icon. The batch-O importer creates
`/Game/Kalmala/UI/Icons/Items/WoodenClub.WoodenClub`; the catalogue validator
checks the original, prepared image, runtime mapping, and imported package.

<a id="a10-support-scroll-icons"></a>

## A10 reusable support-scroll icon coverage

The four existing reusable support scrolls now have distinct original inventory
icons. Mending Scroll and Hearth Shield Scroll reuse their transparent sources
generated earlier in this A10 run. Bear's Vigor Scroll and Deer Call Scroll were
generated with the first two scrolls as style references only; their emblems are
original. All four sources are 1254×1254 transparent RGBA, prepared as centered
64×64 transparent textures, and imported as canonical catalogue assets in the
isolated AP batch (`image_batch=84`).

| Item | Retained original | Prepared texture | Imported texture |
| --- | --- | --- | --- |
| `MendingScroll` | `Content/Kalmala/UI/Source/IconOriginals/MendingScroll.png` | `Content/Kalmala/UI/Source/Icons/MendingScroll.png` | `/Game/Kalmala/UI/Icons/Items/MendingScroll.MendingScroll` |
| `HearthShieldScroll` | `Content/Kalmala/UI/Source/IconOriginals/HearthShieldScroll.png` | `Content/Kalmala/UI/Source/Icons/HearthShieldScroll.png` | `/Game/Kalmala/UI/Icons/Items/HearthShieldScroll.HearthShieldScroll` |
| `BearsVigorScroll` | `Content/Kalmala/UI/Source/IconOriginals/BearsVigorScroll.png` | `Content/Kalmala/UI/Source/Icons/BearsVigorScroll.png` | `/Game/Kalmala/UI/Icons/Items/BearsVigorScroll.BearsVigorScroll` |
| `DeerCallScroll` | `Content/Kalmala/UI/Source/IconOriginals/DeerCallScroll.png` | `Content/Kalmala/UI/Source/Icons/DeerCallScroll.png` | `/Game/Kalmala/UI/Icons/Items/DeerCallScroll.DeerCallScroll` |

The runtime catalogue icon library resolves each canonical item to its own
texture. The icon ledger includes all four identities in AP; the audit rejects
shared status-icon redirects for these scrolls. Prepare, import, and validate
with:

```powershell
python Scripts/Prepare-CatalogueIconBatch.py --batch AP
Scripts/Import-CatalogueIconBatch.ps1 -Batch AP
python Scripts/Validate-CatalogueReferences.py
Scripts/Validate-UIIcons.ps1 -Category Catalogue
```

### Mending Scroll prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a reusable support scroll. Primary request: create a distinct Mending Scroll item icon. Subject: one small, unfurled parchment scroll with both ends rolled, a simple stitched green sprout emblem on the vellum, and a restrained sage-green wax tie; the scroll alone clearly reads as a carried scroll. Style/medium: hand-painted game inventory artwork matching Kalmala's earthy crafted survival catalogue, bold dark brown contour, broad tactile painted facets, restrained highlights, warm aged parchment and muted natural colors. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding, silhouette and emblem legible at 64 pixels. Background: genuinely transparent. Constraints: original artwork, no text or letter-like marks, no UI, border, ground, cast shadow, glow, watermark, logo, hands, extra objects, or copied references.

### Hearth Shield Scroll prompt

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a reusable support scroll. Primary request: create a distinct Hearth Shield Scroll item icon. Subject: one small, unfurled parchment scroll with both ends rolled, a simple embossed dark red shield-and-hearth-flame emblem on the vellum, and a restrained muted rust-red wax tie; the scroll alone clearly reads as a carried scroll. Style/medium: hand-painted game inventory artwork matching Kalmala's earthy crafted survival catalogue, bold dark brown contour, broad tactile painted facets, restrained highlights, warm aged parchment and muted natural colors. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding, silhouette and emblem legible at 64 pixels. Background: genuinely transparent. Constraints: original artwork, no text or letter-like marks, no UI, border, ground, cast shadow, glow, watermark, logo, hands, extra objects, or copied references.

### Bear's Vigor Scroll prompt

The following is a transcription of the submitted prompt content; punctuation and
word wrapping are normalized from the generation request record.

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a reusable support scroll. Inputs images 1 and 2 are existing Kalmala scrolls used as style references only: match the warm vellum, rolled silhouette, dark contour, painted facets, and restrained highlights; do not copy their emblems. Primary request: create a distinct Bear's Vigor Scroll item icon. Subject: one small, unfurled parchment scroll with both ends rolled, a simple dark umber bear paw emblem on the vellum, and a muted ochre wax tie. Style/medium: hand-painted game inventory artwork in the established Kalmala scroll style. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding, silhouette and emblem legible at 64 pixels. Background: genuinely transparent. Constraints: original artwork, no text, UI, border, ground, cast shadow, glow, watermark, logo, hands, or extra objects.

### Deer Call Scroll prompt

The following is a transcription of the submitted prompt content; punctuation and
word wrapping are normalized from the generation request record.

> Use case: stylized-concept. Asset type: original Kalmala survival-fantasy inventory icon for a reusable support scroll. Inputs images 1 and 2 are existing Kalmala scrolls used as style references only: match the warm vellum, rolled silhouette, dark contour, painted facets, and restrained highlights; do not copy their emblems. Primary request: create a distinct Deer Call Scroll item icon. Subject: one small, unfurled parchment scroll with both ends rolled, a simple branching antlers emblem on the vellum, and a forest-green wax tie. Style/medium: hand-painted game inventory artwork in the established Kalmala scroll style. Composition/framing: one centered object filling about 78 percent of a square canvas, clean transparent padding, silhouette and emblem legible at 64 pixels. Background: genuinely transparent. Constraints: original artwork, no text, UI, border, ground, cast shadow, glow, watermark, logo, hands, sound marks, or extra objects.
