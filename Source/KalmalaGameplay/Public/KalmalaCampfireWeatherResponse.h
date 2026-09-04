#pragma once

#include "CoreMinimal.h"

/**
 * Server-only weather response for a lit campfire. The normalized output is
 * replicated by the owning actor; clients never calculate fuel state.
 */
struct KALMALAGAMEPLAY_API FKalmalaCampfireWeatherResponse
{
    static constexpr float ExtinguishWetness = 0.90f;

    static float AdvanceFuelWetness(const float CurrentWetness, const float PrecipitationIntensity, const float WindStrength, const float DeltaSeconds, const bool bLit)
    {
        const float RainWetnessRate = FMath::Clamp(PrecipitationIntensity, 0.0f, 1.0f) * (0.035f + FMath::Clamp(WindStrength, 0.0f, 1.0f) * 0.045f);
        const float DryingRate = PrecipitationIntensity <= KINDA_SMALL_NUMBER ? (bLit ? 0.055f : 0.012f) : 0.0f;
        return FMath::Clamp(CurrentWetness + (RainWetnessRate - DryingRate) * FMath::Max(DeltaSeconds, 0.0f), 0.0f, 1.0f);
    }

    static float CalculateEffectiveWarmth(const bool bLit, const float FuelWetness, const float PrecipitationIntensity, const float WindStrength)
    {
        if (!bLit || FuelWetness >= ExtinguishWetness)
        {
            return 0.0f;
        }

        const float DryFuelFactor = 1.0f - FMath::Clamp(FuelWetness, 0.0f, 1.0f);
        const float RainFactor = 1.0f - FMath::Clamp(PrecipitationIntensity, 0.0f, 1.0f) * 0.45f;
        const float WindFactor = 1.0f - FMath::Clamp(WindStrength, 0.0f, 1.0f) * 0.30f;
        return FMath::Clamp(DryFuelFactor * RainFactor * WindFactor, 0.0f, 1.0f);
    }

    static bool ShouldRemainLit(const bool bLit, const float FuelWetness)
    {
        return bLit && FuelWetness < ExtinguishWetness;
    }
};
