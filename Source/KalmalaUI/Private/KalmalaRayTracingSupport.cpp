#include "KalmalaRayTracingSupport.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "DynamicRHI.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "RenderUtils.h"
#include "RHI.h"
#include "RHIGlobals.h"
#include "Scalability.h"

namespace
{
    constexpr TCHAR RayTracingSettingsSection[] = TEXT("/Script/KalmalaUI.KalmalaSettingsWidget");
    constexpr TCHAR RayTracingQualityKey[] = TEXT("LocalRayTracingQuality");
    Scalability::FQualityLevels PreviousScalabilityQuality(false);
    bool bHasPreviousScalabilityQuality = false;

    bool ApplyRayTracingProfile(const KalmalaRendering::FRayTracingQualityProfile& Profile)
    {
        IConsoleVariable* RayTracingEnable = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing.Enable"));
        IConsoleVariable* LumenHardwareRayTracing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing"));
        IConsoleVariable* DynamicGlobalIllumination = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"));
        IConsoleVariable* ReflectionMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"));
        IConsoleVariable* LumenLightingMode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing.LightingMode"));
        IConsoleVariable* HitLightingAllowed = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing.HitLighting.Allowed"));

        if (LumenHardwareRayTracing == nullptr || DynamicGlobalIllumination == nullptr
            || ReflectionMethod == nullptr || LumenLightingMode == nullptr || HitLightingAllowed == nullptr
            || (Profile.RayTracingEnable != 0 && RayTracingEnable == nullptr))
        {
            return false;
        }

        // Keep the Epic GI preset from briefly enabling Hit Lighting.
        HitLightingAllowed->Set(Profile.HitLightingAllowed, ECVF_SetByGameOverride);
        if (Profile.ScalabilityQualityLevel >= 0)
        {
            if (!bHasPreviousScalabilityQuality)
            {
                // Keep the RT-off path's actual user-selected baseline intact.
                PreviousScalabilityQuality = Scalability::GetQualityLevels();
                bHasPreviousScalabilityQuality = true;
            }

            // Balanced uses High (2); High uses Epic (3), never Cinematic.
            // Only these two scalability groups are raised by the RT selector.
            Scalability::FQualityLevels QualityLevels = Scalability::GetQualityLevels();
            QualityLevels.GlobalIlluminationQuality = Profile.ScalabilityQualityLevel;
            QualityLevels.ReflectionQuality = Profile.ScalabilityQualityLevel;
            Scalability::SetQualityLevels(QualityLevels);
        }
        else if (bHasPreviousScalabilityQuality)
        {
            Scalability::SetQualityLevels(PreviousScalabilityQuality);
            bHasPreviousScalabilityQuality = false;
        }

        // Set up Lumen's method before enabling RT. Apply the explicit Hit
        // Lighting opt-out after scalability: UE's Epic GI preset enables it.
        LumenHardwareRayTracing->Set(Profile.LumenHardwareRayTracing, ECVF_SetByGameOverride);
        DynamicGlobalIllumination->Set(Profile.DynamicGlobalIllumination, ECVF_SetByGameOverride);
        ReflectionMethod->Set(Profile.ReflectionMethod, ECVF_SetByGameOverride);
        LumenLightingMode->Set(Profile.LumenLightingMode, ECVF_SetByGameOverride);
        HitLightingAllowed->Set(Profile.HitLightingAllowed, ECVF_SetByGameOverride);
        if (RayTracingEnable != nullptr)
        {
            RayTracingEnable->Set(Profile.RayTracingEnable, ECVF_SetByGameOverride);
        }
        return true;
    }
}

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

    FRayTracingQualityProfile GetRayTracingQualityProfile(const ERayTracingQualityLevel QualityLevel)
    {
        if (QualityLevel == ERayTracingQualityLevel::Balanced || QualityLevel == ERayTracingQualityLevel::High)
        {
            FRayTracingQualityProfile Profile;
            Profile.RayTracingEnable = 1;
            Profile.LumenHardwareRayTracing = 1;
            Profile.DynamicGlobalIllumination = 1;
            Profile.ReflectionMethod = 1;
            Profile.LumenLightingMode = 0;
            Profile.HitLightingAllowed = 0;
            Profile.ScalabilityQualityLevel = QualityLevel == ERayTracingQualityLevel::High ? 3 : 2;
            return Profile;
        }

        return FRayTracingQualityProfile();
    }

    bool IsBalancedRayTracingAvailable()
    {
        return ResolveCurrentRayTracingPath(true) == ERayTracingPath::Hardware;
    }

    ERayTracingQualityLevel GetSavedRayTracingQualityLevel()
    {
        int32 StoredQuality = static_cast<int32>(ERayTracingQualityLevel::Compatibility);
        if (GConfig != nullptr)
        {
            GConfig->GetInt(RayTracingSettingsSection, RayTracingQualityKey,
                StoredQuality, GGameUserSettingsIni);
        }
        if (StoredQuality == static_cast<int32>(ERayTracingQualityLevel::High))
        {
            return ERayTracingQualityLevel::High;
        }
        return StoredQuality == static_cast<int32>(ERayTracingQualityLevel::Balanced)
            ? ERayTracingQualityLevel::Balanced : ERayTracingQualityLevel::Compatibility;
    }

    bool SetRayTracingQualityLevel(const ERayTracingQualityLevel QualityLevel)
    {
        if (QualityLevel != ERayTracingQualityLevel::Compatibility && !IsBalancedRayTracingAvailable())
        {
            return false;
        }

        const FRayTracingQualityProfile Profile = GetRayTracingQualityProfile(QualityLevel);
        if (!ApplyRayTracingProfile(Profile))
        {
            return false;
        }

        if (GConfig != nullptr)
        {
            GConfig->SetInt(RayTracingSettingsSection, RayTracingQualityKey,
                static_cast<int32>(QualityLevel), GGameUserSettingsIni);
            GConfig->Flush(false, GGameUserSettingsIni);
        }
        return true;
    }

    bool ApplySavedRayTracingQualityLevel()
    {
        ERayTracingQualityLevel QualityLevel = GetSavedRayTracingQualityLevel();
        if (QualityLevel != ERayTracingQualityLevel::Compatibility && !IsBalancedRayTracingAvailable())
        {
            QualityLevel = ERayTracingQualityLevel::Compatibility;
        }

        return SetRayTracingQualityLevel(QualityLevel);
    }
}
