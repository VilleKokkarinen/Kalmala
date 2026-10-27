#pragma once

#include "CoreMinimal.h"

namespace KalmalaRendering
{
    enum class ERayTracingPath : uint8
    {
        Compatibility,
        Hardware
    };

    enum class ERayTracingQualityLevel : uint8
    {
        Compatibility,
        Balanced,
        High
    };

    struct KALMALAUI_API FRayTracingQualityProfile
    {
        int32 RayTracingEnable = 0;
        int32 LumenHardwareRayTracing = 0;
        int32 DynamicGlobalIllumination = 0;
        int32 ReflectionMethod = 2;
        int32 LumenLightingMode = 0;
        int32 HitLightingAllowed = 0;
        int32 ScalabilityQualityLevel = -1;
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

    KALMALAUI_API FRayTracingQualityProfile GetRayTracingQualityProfile(ERayTracingQualityLevel QualityLevel);
    KALMALAUI_API bool IsBalancedRayTracingAvailable();
    KALMALAUI_API ERayTracingQualityLevel GetSavedRayTracingQualityLevel();
    KALMALAUI_API bool SetRayTracingQualityLevel(ERayTracingQualityLevel QualityLevel);
    KALMALAUI_API bool ApplySavedRayTracingQualityLevel();
}
