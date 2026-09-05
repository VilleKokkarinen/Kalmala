"""Add lighting to the generated-world startup map without template terrain.

Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path>.
Only L_Prototype is saved. Existing actors and fixtures are preserved.
"""
import unreal

MAP = '/Game/Kalmala/Maps/Prototype/L_Prototype'
world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
if not world:
    raise RuntimeError('Could not load prototype map')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def ensure_actor(tag, actor_class):
    for actor in actors.get_all_level_actors():
        if unreal.Name(tag) in actor.tags:
            return actor
    actor = actors.spawn_actor_from_class(actor_class, unreal.Vector(0, 0, 2000))
    actor.tags = list(actor.tags) + [unreal.Name(tag)]
    actor.set_actor_label(tag)
    return actor


sun = ensure_actor('KalmalaEnvironmentSun', unreal.DirectionalLight)
sun.set_actor_rotation(unreal.Rotator(-40, -35, 0), False)
sun_component = sun.get_component_by_class(unreal.DirectionalLightComponent)
sun_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun_component.set_editor_property('atmosphere_sun_light', True)
sun_component.set_editor_property('intensity', 10.0)
ensure_actor('KalmalaEnvironmentAtmosphere', unreal.SkyAtmosphere)
sky = ensure_actor('KalmalaEnvironmentSkyLight', unreal.SkyLight)
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_component.set_editor_property('real_time_capture', True)
sky_component.set_editor_property('intensity', 1.0)
if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP):
    raise RuntimeError('Could not save prototype environment')
unreal.log('WATER_SETUP: saved generated-world startup map lighting; no template floor or landscape added.')
