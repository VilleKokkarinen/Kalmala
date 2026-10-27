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
sun_component.set_editor_property('intensity', 9.0)
ensure_actor('KalmalaEnvironmentAtmosphere', unreal.SkyAtmosphere)
sky = ensure_actor('KalmalaEnvironmentSkyLight', unreal.SkyLight)
sky_component = sky.get_component_by_class(unreal.SkyLightComponent)
sky_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky_component.set_editor_property('real_time_capture', True)
sky_component.set_editor_property('intensity', 1.5)

post_process = ensure_actor('KalmalaEnvironmentPostProcess', unreal.PostProcessVolume)
post_process.set_editor_property('bEnabled', True)
post_process.set_editor_property('bUnbound', True)
post_process.set_editor_property('Priority', 0.0)
post_process.set_editor_property('BlendRadius', 0.0)
post_process.set_editor_property('BlendWeight', 1.0)

settings = post_process.get_editor_property('Settings')
for name, value in (
        ('AutoExposureMinBrightness', 0.03),
        ('bOverride_AutoExposureMinBrightness', True),
        ('AutoExposureMaxBrightness', 2.0),
        ('bOverride_AutoExposureMaxBrightness', True),
        ('AutoExposureLowPercent', 10.0),
        ('bOverride_AutoExposureLowPercent', True),
        ('AutoExposureHighPercent', 90.0),
        ('bOverride_AutoExposureHighPercent', True),
        ('AutoExposureSpeedUp', 2.0),
        ('bOverride_AutoExposureSpeedUp', True),
        ('AutoExposureSpeedDown', 2.0),
        ('bOverride_AutoExposureSpeedDown', True),
        ('ColorContrast', unreal.Vector4(1.02, 1.02, 1.02, 1.0)),
        ('bOverride_ColorContrast', True),
        ('ColorSaturation', unreal.Vector4(1.04, 1.04, 1.04, 1.0)),
        ('bOverride_ColorSaturation', True),
        ('BloomIntensity', 0.3),
        ('bOverride_BloomIntensity', True),
    ):
    settings.set_editor_property(name, value)
post_process.set_editor_property('Settings', settings)
if not unreal.EditorLoadingAndSavingUtils.save_map(world, MAP):
    raise RuntimeError('Could not save prototype environment')
unreal.log('LIGHTING_SETUP: saved global rig Sun=9.0 Sky=1.5, unbound exposure range=0.03..2.0 histogram=10..90 speed=2.0/2.0, contrast=1.02, saturation=1.04, bloom=0.30.')
unreal.log('WATER_SETUP: saved generated-world startup map lighting; no template floor or landscape added.')
