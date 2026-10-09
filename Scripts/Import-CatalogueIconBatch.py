"""Import one pinned catalogue texture batch from the canonical icon manifest.

Run inside Unreal Editor with PythonScriptPlugin enabled and
KALMALA_ICON_IMPORT_BATCH set to A, B, or C. The manifest remains the only
source of canonical IDs and aliases; aliases resolve to the same canonical
texture object and never create duplicate assets.
"""

import csv
import os
import struct

import unreal


DESTINATION = "/Game/Kalmala/UI/Icons/Items"
MAX_BATCH_SIZE = 16


def fail(message):
    unreal.log_error("KALMALA_ICON_IMPORT_FAILED: " + message)
    raise RuntimeError(message)


batch = os.environ.get("KALMALA_ICON_IMPORT_BATCH", "").strip().upper()
if batch not in ("A", "B", "C"):
    fail("Set KALMALA_ICON_IMPORT_BATCH to one pinned batch: A, B, or C.")

repo_root = os.path.abspath(
    os.environ.get("KALMALA_ICON_REPO_ROOT", unreal.Paths.project_dir())
)
manifest_path = os.path.join(repo_root, "docs", "catalogue-icon-manifest.csv")
source_dir = os.path.join(
    repo_root, "Content", "Kalmala", "UI", "Source", "Icons"
)

if not os.path.isfile(manifest_path):
    fail("Canonical icon manifest is missing: " + manifest_path)

with open(manifest_path, "r", newline="") as manifest_file:
    reader = csv.DictReader(manifest_file)
    required_columns = {
        "import_batch",
        "canonical_id",
        "catalogue_aliases",
    }
    if not required_columns.issubset(set(reader.fieldnames or [])):
        fail("Manifest is missing import identity or alias columns.")
    rows = [row for row in reader if row["import_batch"].strip().upper() == batch]

if not rows or len(rows) > MAX_BATCH_SIZE:
    fail("Batch %s has %d rows; expected between 1 and %d." % (
        batch, len(rows), MAX_BATCH_SIZE
    ))

canonical_ids = [row["canonical_id"].strip() for row in rows]
if len(set(canonical_ids)) != len(canonical_ids):
    fail("Batch %s contains duplicate canonical IDs." % batch)

alias_assignments = {}
tasks = []
for row, canonical_id in zip(rows, canonical_ids):
    if not canonical_id or not canonical_id.replace("_", "").isalnum():
        fail("Batch %s contains an invalid canonical ID." % batch)

    alias_text = row["catalogue_aliases"].strip()
    aliases = [] if alias_text.lower() in ("", "none") else [
        alias.strip() for alias in alias_text.split(";") if alias.strip()
    ]
    for alias in aliases:
        previous_id = alias_assignments.get(alias)
        if previous_id and previous_id != canonical_id:
            fail("Alias %s maps to both %s and %s." % (
                alias, previous_id, canonical_id
            ))
        alias_assignments[alias] = canonical_id

    source_path = os.path.abspath(os.path.join(source_dir, canonical_id + ".png"))
    if not os.path.isfile(source_path):
        fail("Prepared source is missing for %s: %s" % (canonical_id, source_path))

    with open(source_path, "rb") as source_file:
        header = source_file.read(29)
    if len(header) != 29 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        fail("Prepared source is not a valid PNG header for %s." % canonical_id)
    width, height, bit_depth, color_type, _, _, _ = struct.unpack(">IIBBBBB", header[16:29])
    if width != 64 or height != 64 or bit_depth != 8 or color_type != 6:
        fail("%s must be a 64x64, 8-bit RGBA PNG; found %dx%d depth=%d type=%d." % (
            canonical_id, width, height, bit_depth, color_type
        ))

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source_path)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", canonical_id)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

unreal.log("KALMALA_ICON_IMPORT_START: batch=%s count=%d" % (batch, len(tasks)))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.import_asset_tasks(tasks)

for row, canonical_id in zip(rows, canonical_ids):
    object_path = "%s/%s.%s" % (DESTINATION, canonical_id, canonical_id)
    texture = unreal.load_asset(object_path)
    if not texture or texture.get_class().get_name() != "Texture2D":
        fail("Expected Texture2D was not imported at " + object_path)

    actual_width = texture.blueprint_get_size_x()
    actual_height = texture.blueprint_get_size_y()
    if actual_width != 64 or actual_height != 64:
        fail("%s imported at %dx%d instead of 64x64." % (
            canonical_id, actual_width, actual_height
        ))

    # Small catalogue textures are loaded on demand by UI, keep their source
    # alpha and avoid world-texture streaming/mipmap defaults.
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("sRGB", True)
    texture.set_editor_property("compression_no_alpha", False)
    if texture.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        fail("%s did not retain the UI texture group." % canonical_id)
    if texture.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
        fail("%s did not retain the no-mipmap UI setting." % canonical_id)
    if texture.get_editor_property("never_stream") is not True:
        fail("%s did not retain the non-streaming UI setting." % canonical_id)
    if texture.get_editor_property("sRGB") is not True:
        fail("%s did not retain sRGB color." % canonical_id)
    if texture.get_editor_property("compression_no_alpha") is not False:
        fail("%s imported with alpha compression disabled." % canonical_id)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        fail("Could not save imported texture settings for %s." % canonical_id)

    aliases = row["catalogue_aliases"].strip()
    unreal.log(
        "KALMALA_ICON_IMPORTED: batch=%s canonical=%s object=%s size=64x64 alpha=1 group=UI aliases=%s"
        % (batch, canonical_id, object_path, aliases or "none")
    )

unreal.log("KALMALA_ICON_IMPORT_COMPLETE: batch=%s count=%d" % (batch, len(tasks)))
