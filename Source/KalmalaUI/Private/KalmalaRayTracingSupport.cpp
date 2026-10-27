#include "KalmalaRayTracingSupport.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "DynamicRHI.h"
#include "RenderUtils.h"
#include "RHI.h"
#include "RHIGlobals.h"

namespace KalmalaRendering
{
    bool FKalmalaRayTracingCapabilities::SupportsHardwareRayTracing() const
    {
        return bProjectAllowsHardwareRayTracing
            && bWindowsD3D12
            && bShaderPlatformSupportsSm6
            && bShaderPlatformSupportsRayTracing
            && bShaderPlatformSupportsRayTracingShaders
            && bRhiSupportsRayTracing
            && bRhiSupportsRayTracingShaders;
    }

    FKalmalaRayTracingCapabilities GetCurrentRayTracingCapabilities()
    {
        FKalmalaRayTracingCapabilities Capabilities;
        if (!GIsRHIInitialized)
        {
            return Capabilities;
        }

        const auto ShaderPlatform = GMaxRHIShaderPlatform;
        Capabilities.bProjectAllowsHardwareRayTracing = IsRayTracingAllowed();
        Capabilities.bShaderPlatformSupportsSm6 = IsFeatureLevelSupported(ShaderPlatform, ERHIFeatureLevel::SM6);
        Capabilities.bShaderPlatformSupportsRayTracing = RHISupportsRayTracing(ShaderPlatform);
        Capabilities.bShaderPlatformSupportsRayTracingShaders = RHISupportsRayTracingShaders(ShaderPlatform);
        Capabilities.bRhiSupportsRayTracing = GRHISupportsRayTracing;
        Capabilities.bRhiSupportsRayTracingShaders = GRHISupportsRayTracingShaders;
#if PLATFORM_WINDOWS
        Capabilities.bWindowsD3D12 = RHIGetInterfaceType() == ERHIInterfaceType::D3D12;
#endif
        return Capabilities;
    }

    ERayTracingPath ResolveRayTracingPath(
        const bool bHardwareRayTracingRequested,
        const FKalmalaRayTracingCapabilities& Capabilities)
    {
        return bHardwareRayTracingRequested && Capabilities.SupportsHardwareRayTracing()
            ? ERayTracingPath::Hardware
            : ERayTracingPath::Compatibility;
    }

    ERayTracingPath ResolveCurrentRayTracingPath(const bool bHardwareRayTracingRequested)
    {
        return ResolveRayTracingPath(bHardwareRayTracingRequested, GetCurrentRayTracingCapabilities());
    }
}
