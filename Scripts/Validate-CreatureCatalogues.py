"""Validate the authored creature extension (no Unreal/runtime activation)."""

import json
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "Content/Data/GameCatalogues"
CHANNELS = {"Slash", "Stab", "Crush", "Pierce", "Fire", "Water", "Holy", "Poison"}
EXPECTED = {
    "Boar", "Deer", "Mireling", "Rat", "Badger", "MoonBear", "Viper",
    "GiantSnake", "DeathAdder", "MosquitoSwarm", "PeatHusk", "Wolf", "Lynx",
    "PolarBear", "HornedGoat", "ThunderSentinel", "ReefMaw",
}
BIOMES = {
    "Meadows", "Elderwood", "ShimmeringLakes", "MossyMire",
    "FreezingTundra", "ThunderMountains", "Ocean",
}
# These intentionally unresolved item references belong to A8b/A9. They must
# never be interpreted as current, grantable inventory definitions.
FUTURE_DROPS = {
    "RatTail", "BadgerFur", "BadgerMeat", "Moonstone", "ViperFang",
    "GiantScale", "DuskDiamond", "VenomSac", "PeatHeart", "WolfPelt",
    "WolfFang", "WolfMeat", "LynxPelt", "PolarBearPelt", "GoatHorn",
    "ThunderCore", "ReefTooth",
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def number(value, minimum=0):
    return type(value) in (int, float) and math.isfinite(value) and value >= minimum


def read(path):
    require(len(str(path.resolve())) < 260, f"MAX_PATH exceeded: {path}")
    return json.loads(path.read_text(encoding="utf-8-sig"))


def category_files(manifest, key):
    paths = manifest[key]
    require(isinstance(paths, list) and paths, f"Empty category: {key}")
    require(len(set(paths)) == len(paths), f"Duplicate path in {key}")
    for relative in paths:
        path = (DATA / relative).resolve()
        require(path.is_relative_to(DATA.resolve()), f"Path escapes catalogue: {relative}")
        yield read(path)


def main():
    manifest = read(ROOT / "Content/Data/GameCatalogues.json")
    # Also parse existing categories so manifest references cannot silently break.
    for key in manifest:
        if key.endswith("Files"):
            list(category_files(manifest, key))
    items = {
        item["ItemId"]
        for document in category_files(manifest, "itemFiles")
        for item in document["items"]
    }
    creatures = [
        creature
        for document in category_files(manifest, "creatureFiles")
        for creature in document["creatures"]
    ]
    ids = [creature["CreatureId"] for creature in creatures]
    require(len(ids) == len(set(name.casefold() for name in ids)), "Duplicate creature ID")
    require(set(ids) == EXPECTED, "Incomplete or unexpected creature roster")
    require(manifest["creatureFiles"] == ["creatures.json"], "Expected one creature catalogue")
    require("creatureRuleFiles" not in manifest, "Separate creature rules are no longer used")
    profiles = {}
    allowed_fields = {
        "CreatureId", "DisplayName", "Health", "Armor", "Biomes", "Attacks", "Drops",
        "DefensiveTriggerRangeCm", "BodyLengthCm", "Swarm",
    }
    required_fields = {"CreatureId", "DisplayName", "Health", "Armor", "Biomes", "Attacks", "Drops"}
    for creature in creatures:
        cid = creature["CreatureId"]
        require(required_fields <= creature.keys() <= allowed_fields, f"Unexpected or missing creature fields: {cid}")
        require(isinstance(creature["DisplayName"], str) and creature["DisplayName"], f"Invalid name: {cid}")
        require(number(creature["Health"], 1) and number(creature["Armor"]), f"Invalid stats: {cid}")
        require(creature["Biomes"] and set(creature["Biomes"]) <= BIOMES, f"Invalid biomes: {cid}")
        profiles[cid] = creature
        attacks = creature["Attacks"]
        require(len({a["AttackId"] for a in attacks}) == len(attacks), f"Duplicate attacks: {cid}")
        for attack in attacks:
            require(set(attack) <= {
                "AttackId", "Damage", "WindupSeconds", "RecoverySeconds", "Poison",
                "ApproachSeconds", "ContactIntervalSeconds", "ContactRangeCm", "MaxContactDamagePerSecond",
            }, f"Unexpected attack fields: {cid}")
            damage = attack["Damage"]
            require(damage and set(damage) <= CHANNELS and all(number(v, .001) for v in damage.values()), f"Invalid damage: {cid}")
            if "Poison" in attack:
                poison = attack["Poison"]
                require(set(poison) == {"DamagePerTick", "DurationSeconds", "TickIntervalSeconds", "FirstTickDelaySeconds"},
                        f"Invalid poison fields: {cid}")
                require(all(number(value, .001) for value in poison.values()), f"Invalid inline poison: {cid}")
            for field, value in attack.items():
                if field.endswith("Seconds") or field.endswith("Cm") or field == "MaxContactDamagePerSecond":
                    require(number(value, .001), f"Invalid attack tuning: {cid}/{field}")
        for field in ("DefensiveTriggerRangeCm", "BodyLengthCm"):
            if field in creature:
                require(number(creature[field], .001), f"Invalid body/trigger tuning: {cid}/{field}")
        if "Swarm" in creature:
            swarm = creature["Swarm"]
            require(set(swarm) == {
                "CosmeticInsectCount", "MinGroundClearanceCm", "MaxGroundClearanceCm",
                "MaxAttackingSwarmsPerPlayer", "RollReacquireDelaySeconds", "HeldTorchAvoidanceRadiusCm",
                "PlacedFlameLightAvoidanceRadiusCm", "CampfireAvoidanceRadiusCm",
                "RequiresAuthoritativeActiveFlame", "RequiresLineOfSightToFlame", "CatchUpDamage",
            }, f"Unexpected swarm fields: {cid}")
            for field, value in swarm.items():
                require(type(value) is bool if field.startswith("Requires") or field == "CatchUpDamage"
                        else number(value, .001), f"Invalid swarm tuning: {cid}/{field}")
        for drop in creature["Drops"]:
            require(set(drop) == {"ItemId", "Quantity", "Chance"}, f"Unexpected drop fields: {cid}")
            require(type(drop["Quantity"]) is int and drop["Quantity"] > 0, f"Invalid quantity: {cid}")
            require(number(drop["Chance"]) and 0 < drop["Chance"] <= 1, f"Invalid chance: {cid}")
            require(drop["ItemId"] in items | FUTURE_DROPS, f"Unknown drop: {cid}/{drop['ItemId']}")
    require(not profiles["Deer"]["Attacks"], "Deer cannot attack")
    require(not profiles["MosquitoSwarm"]["Drops"], "Swarm has no loot")
    for item, source in (("Moonstone", "MoonBear"), ("DuskDiamond", "Mireling")):
        sources = {cid for cid, p in profiles.items() if any(d["ItemId"] == item for d in p["Drops"])}
        require(sources == {source}, f"Incorrect exclusive gem source: {item}")
    print("PASS: 17 unique flat species profiles, one Wolf, stats, attacks, drop references, inline poison and biome identities; all manifest JSON parses.")


if __name__ == "__main__":
    main()
