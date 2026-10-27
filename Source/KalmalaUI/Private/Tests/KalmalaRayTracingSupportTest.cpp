#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaRayTracingSupport.h"
#include "Misc/AutomationTest.h"

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

#endif
