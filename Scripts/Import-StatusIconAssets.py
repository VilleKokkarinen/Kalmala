"""Import the complete pinned status-icon set from its prepared RGBA PNGs."""

import csv
import os
import struct
from collections import Counter

import unreal


DESTINATION = "/Game/Kalmala/UI/Icons/Status"
EXPECTED_BATCHES = {"01": 4, "02": 4, "03": 1}


def fail(message):
    unreal.log_error("KALMALA_STATUS_ICON_IMPORT_FAILED: " + message)
    raise RuntimeError(message)


repo_root = os.path.abspath(
    os.environ.get("KALMALA_STATUS_ICON_REPO_ROOT", unreal.Paths.project_dir())
)
manifest_path = os.path.join(repo_root, "docs", "status-icon-manifest.csv")
source_dir = os.path.join(
    repo_root, "Content", "Kalmala", "UI", "Source", "Icons", "Status"
)

if not os.path.isfile(manifest_path):
    fail("Status icon manifest is missing: " + manifest_path)

with open(manifest_path, "r", newline="") as manifest_file:
    reader = csv.DictReader(manifest_file)
    required_columns = {"batch", "entry_id", "icon_id", "display_name", "kind", "art_subject"}
    if not required_columns.issubset(set(reader.fieldnames or [])):
        fail("Status manifest is missing an identity or generation-batch column.")
    rows = list(reader)

batch_counts = Counter(row["batch"].strip() for row in rows)
if batch_counts != Counter(EXPECTED_BATCHES) or len(rows) != 9:
    fail("Expected the pinned 4/4/1 generation batches and nine total status icons.")

entry_ids = [row["entry_id"].strip() for row in rows]
icon_ids = [row["icon_id"].strip() for row in rows]
if not all(entry_ids) or len(set(entry_ids)) != 9:
    fail("Status manifest entry IDs must be non-empty and unique.")
if not all(icon_ids) or len(set(icon_ids)) != 9:
    fail("Status manifest image IDs must be non-empty and unique.")

tasks = []
for row, icon_id in zip(rows, icon_ids):
    if not icon_id.replace("_", "").isalnum():
        fail("Unsafe status image ID: " + icon_id)

    source_path = os.path.abspath(os.path.join(source_dir, icon_id + ".png"))
    if len(source_path) >= 260:
        fail("Prepared source path exceeds MAX_PATH: " + source_path)
    if not os.path.isfile(source_path):
        fail("Prepared source is missing for %s: %s" % (icon_id, source_path))

    with open(source_path, "rb") as source_file:
        header = source_file.read(29)
    if len(header) != 29 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
        fail("Prepared source is not a valid PNG header for " + icon_id)
    width, height, bit_depth, color_type, _, _, _ = struct.unpack(">IIBBBBB", header[16:29])
    if width != 64 or height != 64 or bit_depth != 8 or color_type != 6:
        fail("%s must be a 64x64, 8-bit RGBA PNG; found %dx%d depth=%d type=%d." % (
            icon_id, width, height, bit_depth, color_type
        ))

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source_path)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", icon_id)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    tasks.append(task)

unreal.log("KALMALA_STATUS_ICON_IMPORT_START: count=%d" % len(tasks))
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for row, icon_id in zip(rows, icon_ids):
    object_path = "%s/%s.%s" % (DESTINATION, icon_id, icon_id)
    texture = unreal.load_asset(object_path)
    if not texture or texture.get_class().get_name() != "Texture2D":
        fail("Expected Texture2D was not imported at " + object_path)

    if texture.blueprint_get_size_x() != 64 or texture.blueprint_get_size_y() != 64:
        fail(icon_id + " did not import as a 64x64 texture.")

    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("sRGB", True)
    texture.set_editor_property("compression_no_alpha", False)
    if texture.get_editor_property("lod_group") != unreal.TextureGroup.TEXTUREGROUP_UI:
        fail(icon_id + " did not retain the UI texture group.")
    if texture.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
        fail(icon_id + " did not retain the no-mipmap setting.")
    if texture.get_editor_property("never_stream") is not True:
        fail(icon_id + " did not retain the non-streaming setting.")
    if texture.get_editor_property("sRGB") is not True:
        fail(icon_id + " did not retain sRGB color.")
    if texture.get_editor_property("compression_no_alpha") is not False:
        fail(icon_id + " disabled alpha compression unexpectedly.")
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
        fail("Could not save imported texture settings for " + icon_id)

    unreal.log(
        "KALMALA_STATUS_ICON_IMPORTED: entry=%s icon_id=%s object=%s size=64x64 alpha=1 group=UI"
        % (row["entry_id"].strip(), icon_id, object_path)
    )

unreal.log("KALMALA_STATUS_ICON_IMPORT_COMPLETE: count=%d" % len(tasks))
