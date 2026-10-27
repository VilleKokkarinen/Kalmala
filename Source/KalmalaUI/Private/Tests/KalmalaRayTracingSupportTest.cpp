#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaRayTracingSupport.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRayTracingSupportTest,
    "Kalmala.Rendering.RayTracing.UnsupportedAdaptersUseCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaRayTracingSupportTest::RunTest(const FString& Parameters)
{
    using namespace KalmalaRendering;

    FKalmalaRayTracingCapabilities SupportedCapabilities;
    SupportedCapabilities.bProjectAllowsHardwareRayTracing = true;
    SupportedCapabilities.bWindowsD3D12 = true;
    SupportedCapabilities.bShaderPlatformSupportsSm6 = true;
    SupportedCapabilities.bShaderPlatformSupportsRayTracing = true;
    SupportedCapabilities.bShaderPlatformSupportsRayTracingShaders = true;
    SupportedCapabilities.bRhiSupportsRayTracing = true;
    SupportedCapabilities.bRhiSupportsRayTracingShaders = true;

    TestTrue(TEXT("A requested hardware path is available when every project, shader-platform, and RHI capability is present."),
        ResolveRayTracingPath(true, SupportedCapabilities) == ERayTracingPath::Hardware);
    TestTrue(TEXT("The compatibility path remains selected when hardware RT is not requested."),
        ResolveRayTracingPath(false, SupportedCapabilities) == ERayTracingPath::Compatibility);

    FKalmalaRayTracingCapabilities UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bProjectAllowsHardwareRayTracing = false;
    TestTrue(TEXT("Project-disabled hardware RT resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bWindowsD3D12 = false;
    TestTrue(TEXT("A non-Windows-D3D12 RHI resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bShaderPlatformSupportsSm6 = false;
    TestTrue(TEXT("A shader platform without SM6 resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bShaderPlatformSupportsRayTracing = false;
    TestTrue(TEXT("A shader platform without ray tracing resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bShaderPlatformSupportsRayTracingShaders = false;
    TestTrue(TEXT("A shader platform without ray-tracing shaders resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bRhiSupportsRayTracing = false;
    TestTrue(TEXT("An RHI without ray tracing resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    UnsupportedCapabilities = SupportedCapabilities;
    UnsupportedCapabilities.bRhiSupportsRayTracingShaders = false;
    TestTrue(TEXT("An RHI without ray-tracing shaders resolves to compatibility."),
        ResolveRayTracingPath(true, UnsupportedCapabilities) == ERayTracingPath::Compatibility);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRayTracingQualityProfileTest,
    "Kalmala.Rendering.RayTracing.QualityProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaRayTracingQualityProfileTest::RunTest(const FString& Parameters)
{
    using namespace KalmalaRendering;

    int32 ProjectRayTracing = 0;
    const bool bHasProjectRayTracing = GConfig != nullptr
        && GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.RayTracing"),
            ProjectRayTracing, GEngineIni);
    TestTrue(TEXT("Project ray tracing support is enabled for on-demand local profiles."),
        bHasProjectRayTracing);
    TestEqual(TEXT("Project ray tracing support remains available while Compatibility starts inactive."),
        ProjectRayTracing, 1);

    int32 SkinCacheCompileShaders = 0;
    const bool bHasSkinCacheCompileShaders = GConfig != nullptr
        && GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.SkinCache.CompileShaders"),
            SkinCacheCompileShaders, GEngineIni);
    TestTrue(TEXT("Project compiles the skin-cache shaders required by runtime ray tracing."),
        bHasSkinCacheCompileShaders);
    TestTrue(TEXT("Runtime ray tracing has the required skin-cache shader support."),
        SkinCacheCompileShaders >= 1);

    int32 RayTracingOnDemand = 0;
    const bool bHasRayTracingOnDemand = GConfig != nullptr
        && GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.RayTracing.EnableOnDemand"),
            RayTracingOnDemand, GEngineIni);
    TestTrue(TEXT("Project config enables on-demand ray tracing so local profiles can toggle it."),
        bHasRayTracingOnDemand);
    TestEqual(TEXT("On-demand ray tracing is enabled for the runtime profile selector."), RayTracingOnDemand, 1);

    int32 DefaultRayTracingEnable = 1;
    const bool bHasDefaultRayTracingEnable = GConfig != nullptr
        && GConfig->GetInt(TEXT("SystemSettings"), TEXT("r.RayTracing.Enable"),
            DefaultRayTracingEnable, GEngineIni);
    TestTrue(TEXT("Project config defines the startup ray-tracing state."), bHasDefaultRayTracingEnable);
    TestEqual(TEXT("Compatibility remains the RT-off startup profile."), DefaultRayTracingEnable, 0);

    const FRayTracingQualityProfile Compatibility =
        GetRayTracingQualityProfile(ERayTracingQualityLevel::Compatibility);
    TestTrue(TEXT("Compatibility disables runtime hardware ray tracing."), Compatibility.RayTracingEnable == 0);
    TestTrue(TEXT("Compatibility disables Lumen hardware tracing."), Compatibility.LumenHardwareRayTracing == 0);
    TestTrue(TEXT("Compatibility retains the no-dynamic-GI baseline."), Compatibility.DynamicGlobalIllumination == 0);
    TestTrue(TEXT("Compatibility uses screen-space reflections."), Compatibility.ReflectionMethod == 2);
    TestTrue(TEXT("Compatibility pins Surface Cache and disables Hit Lighting."),
        Compatibility.LumenLightingMode == 0 && Compatibility.HitLightingAllowed == 0);

    const FRayTracingQualityProfile Balanced =
        GetRayTracingQualityProfile(ERayTracingQualityLevel::Balanced);
    TestTrue(TEXT("Balanced enables hardware ray tracing."), Balanced.RayTracingEnable == 1);
    TestTrue(TEXT("Balanced enables Lumen hardware tracing."), Balanced.LumenHardwareRayTracing == 1);
    TestTrue(TEXT("Balanced enables Lumen global illumination and reflections."),
        Balanced.DynamicGlobalIllumination == 1 && Balanced.ReflectionMethod == 1);
    TestTrue(TEXT("Balanced uses Surface Cache and disables Hit Lighting."),
        Balanced.LumenLightingMode == 0 && Balanced.HitLightingAllowed == 0);
    return true;
}

#endif
