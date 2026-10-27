#pragma once

#include "CoreMinimal.h"

namespace KalmalaRendering
{
    enum class ERayTracingPath : uint8
    {
        Compatibility,
        Hardware
    };

    struct KALMALAUI_API FKalmalaRayTracingCapabilities
    {
        bool bProjectAllowsHardwareRayTracing = false;
        bool bWindowsD3D12 = false;
        bool bShaderPlatformSupportsSm6 = false;
        bool bShaderPlatformSupportsRayTracing = false;
        bool bShaderPlatformSupportsRayTracingShaders = false;
        bool bRhiSupportsRayTracing = false;
        bool bRhiSupportsRayTracingShaders = false;

        bool SupportsHardwareRayTracing() const;
    };

    /** Read the initialized project's actual shader-platform and RHI support. */
    KALMALAUI_API FKalmalaRayTracingCapabilities GetCurrentRayTracingCapabilities();

    /** Fail closed to Compatibility unless hardware RT was requested and every capability is present. */
    KALMALAUI_API ERayTracingPath ResolveRayTracingPath(
        bool bHardwareRayTracingRequested,
        const FKalmalaRayTracingCapabilities& Capabilities);

    /** Runtime convenience overload; call after RHI initialization. */
    KALMALAUI_API ERayTracingPath ResolveCurrentRayTracingPath(bool bHardwareRayTracingRequested);
}
