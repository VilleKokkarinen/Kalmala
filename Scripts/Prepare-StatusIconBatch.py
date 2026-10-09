"""Prepare one pinned status-icon batch as centered transparent 64x64 PNGs."""

import argparse
import csv
import re
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "docs" / "status-icon-manifest.csv"
SOURCE_DIR = ROOT / "Content" / "Kalmala" / "UI" / "Source" / "IconOriginals" / "Status"
OUTPUT_DIR = ROOT / "Content" / "Kalmala" / "UI" / "Source" / "Icons" / "Status"
CANVAS_SIZE = 64
ART_SIZE = 56


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--batch", required=True, choices=("01", "02", "03"))
    args = parser.parse_args()

    if not MANIFEST.is_file():
        raise SystemExit(f"Missing status icon manifest: {MANIFEST}")
    with MANIFEST.open("r", newline="", encoding="utf-8-sig") as handle:
        rows = [row for row in csv.DictReader(handle) if row["batch"].strip() == args.batch]

    icon_ids = [row["icon_id"].strip() for row in rows]
    if not rows or len(rows) > 4 or len(set(icon_ids)) != len(icon_ids):
        raise SystemExit(f"Batch {args.batch} must contain one to four unique icon IDs")

    for icon_id in icon_ids:
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9]*", icon_id):
            raise SystemExit(f"Invalid icon ID in batch {args.batch}: {icon_id!r}")
        source = SOURCE_DIR / f"{icon_id}.png"
        target = OUTPUT_DIR / f"{icon_id}.png"
        for path in (source, target):
            if len(str(path.resolve())) >= 260:
                raise SystemExit(f"Path must be shorter than 260 characters: {path}")
        if not source.is_file():
            raise SystemExit(f"Missing retained original for {icon_id}: {source}")

        with Image.open(source) as loaded:
            if loaded.format != "PNG" or loaded.width <= CANVAS_SIZE or loaded.height <= CANVAS_SIZE:
                raise SystemExit(f"{icon_id} original must be a PNG larger than 64x64")
            rgba = loaded.convert("RGBA")
            alpha = rgba.getchannel("A")
            if alpha.getextrema()[0] == 255:
                raise SystemExit(f"{icon_id} original has no transparent pixels")
            bounds = alpha.getbbox()
            if bounds is None:
                raise SystemExit(f"{icon_id} original has no visible artwork")

            artwork = rgba.crop(bounds)
            artwork.thumbnail((ART_SIZE, ART_SIZE), Image.Resampling.LANCZOS)
            canvas = Image.new("RGBA", (CANVAS_SIZE, CANVAS_SIZE), (0, 0, 0, 0))
            x = (CANVAS_SIZE - artwork.width) // 2
            y = (CANVAS_SIZE - artwork.height) // 2
            canvas.alpha_composite(artwork, (x, y))
            OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
            canvas.save(target, format="PNG", optimize=True)

        with Image.open(target) as prepared:
            if prepared.size != (CANVAS_SIZE, CANVAS_SIZE) or prepared.mode != "RGBA":
                raise SystemExit(f"Prepared {icon_id} must be 64x64 RGBA")
            if prepared.getchannel("A").getextrema()[0] == 255:
                raise SystemExit(f"Prepared {icon_id} lost transparency")
        print(f"Prepared {icon_id}: source={rgba.width}x{rgba.height} final=64x64 RGBA transparent=1")


if __name__ == "__main__":
    main()
