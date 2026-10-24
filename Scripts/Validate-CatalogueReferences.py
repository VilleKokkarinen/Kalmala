"""No-build catalogue registration/reference and icon-ledger coverage audit.

Run Validate-UIIcons.ps1 -Category Catalogue as well to inspect PNGs/packages.
This does not load Unreal assets or certify rendered/runtime acceptance.
"""
import csv
import json
from pathlib import Path
import re


def audit(root):
    data = root / "Content/Data/GameCatalogues"
    manifest = json.loads((data.parent / "GameCatalogues.json").read_text())
    documents = {}
    groups = {}
    for group, paths in manifest.items():
        if not group.endswith("Files"):
            continue
        assert paths and len(paths) == len(set(paths)), f"Empty/duplicate {group}"
        groups[group] = paths
        for name in paths:
            path = (data / name).resolve()
            assert path.is_relative_to(data.resolve()), f"Outside catalogue: {name}"
            assert len(str(path)) < 260, f"MAX_PATH: {path}"
            documents[name] = json.loads(path.read_text(encoding="utf-8-sig"))
    unregistered = {p.relative_to(data).as_posix() for p in data.rglob("*.json")} - documents.keys()
    assert not unregistered, f"Unregistered JSON: {sorted(unregistered)}"

    definitions = [item for name in groups["itemFiles"] + groups["itemProfileFiles"]
                   for item in documents[name]["items"]]
    ids = {item["ItemId"] for item in definitions}
    assert len(ids) == len(definitions), "Duplicate ordinary item/buildable definition"
    references = []

    def visit(value, location):
        if isinstance(value, dict):
            for key, child in value.items():
                here = f"{location}/{key}"
                if key in ("ItemId", "Output"):
                    references.append((here, child))
                elif key == "ItemIds":
                    references.extend((here, item) for item in child)
                elif key == "RequiredStation":
                    references.extend((here, item) for item in
                                      (child if isinstance(child, list) else [child])
                                      if item not in ("None", ""))
                visit(child, here)
        elif isinstance(value, list):
            for index, child in enumerate(value):
                visit(child, f"{location}/{index}")

    for name, document in documents.items():
        visit(document, name)
    missing = [(where, item) for where, item in references if item not in ids]
    assert not missing, f"Unresolved references: {missing}"

    with (root / "Scripts/Data/catalogue-icon-manifest.csv").open(newline="", encoding="utf-8-sig") as stream:
        rows = list(csv.DictReader(stream))
    coverage = {}
    for row in rows:
        canonical = row["canonical_id"]
        for identity in [canonical] + row["catalogue_aliases"].split(";"):
            if identity and identity != "none":
                assert identity not in coverage or coverage[identity] == canonical, f"Ambiguous icon: {identity}"
                coverage[identity] = canonical
    missing_icons = sorted(ids - coverage.keys())
    runtime = (root / "Source/KalmalaUI/Private/KalmalaIconWidget.cpp").read_text()
    mappings = set(re.findall(r'\{TEXT\("([A-Za-z0-9]+)"\),EKalmalaIcon::', runtime))
    assert all(coverage[item] in mappings for item in ids if item in coverage), "Missing canonical runtime icon mapping"
    loader = (root / "Source/KalmalaGameplay/Private/KalmalaGameCatalogueLoader.cpp").read_text()
    for item in ids:
        if item not in coverage:
            continue
        canonical = coverage[item]
        if item != canonical:
            assert f'if (CatalogueItemId == TEXT("{item}")) return TEXT("{canonical}");' in loader, f"Missing loader alias: {item}"
    print(f"PASS: {len(documents)} registered JSON files, {len(ids)} unique item/buildable definitions, "
          f"{len(references)} resolved item/output/station references; "
          f"{len(ids) - len(missing_icons)} definitions have ledger-backed canonical runtime icon mappings.")
    assert not missing_icons, f"Missing generated icon ledger entries: {missing_icons}"


if __name__ == "__main__":
    audit(Path(__file__).resolve().parent.parent)
