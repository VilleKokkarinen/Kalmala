"""Create or validate Kalmala's empty front-end map.

Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path>.
The script never changes an existing map.
"""
import unreal


MAP = "/Game/Kalmala/Maps/Menu/L_Menu"
MENU_GAME_MODE_PATH = "/Script/KalmalaGameplay.KalmalaMenuGameMode"


def validate(world, expected_game_mode):
    if not world:
        raise RuntimeError("Could not load the Home map")

    world_settings = world.get_world_settings()
    actual_game_mode = world_settings.get_editor_property("default_game_mode")
    if actual_game_mode != expected_game_mode:
        raise RuntimeError("The Home map does not select AKalmalaMenuGameMode")

    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    if actors:
        names = ", ".join(actor.get_name() for actor in actors)
        raise RuntimeError("The Home map must remain empty; found actors: " + names)


menu_game_mode = unreal.load_class(None, MENU_GAME_MODE_PATH)
if not menu_game_mode:
    raise RuntimeError("Could not load AKalmalaMenuGameMode")

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    validate(world, menu_game_mode)
    unreal.log("MENU_MAP: existing empty Home map validated; no asset was changed.")
else:
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor.new_level(MAP):
        raise RuntimeError("Could not create the Home map")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if not world:
        raise RuntimeError("Could not access the new Home map")
    world.get_world_settings().set_editor_property("default_game_mode", menu_game_mode)
    validate(world, menu_game_mode)
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP):
        raise RuntimeError("Could not save the Home map")

    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    validate(world, menu_game_mode)
    unreal.log("MENU_MAP: created and verified empty Home map with AKalmalaMenuGameMode.")
