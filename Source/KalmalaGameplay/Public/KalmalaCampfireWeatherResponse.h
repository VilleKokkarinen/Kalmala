#pragma once

#include "CoreMinimal.h"
#include "KalmalaWeatherState.h"

/**
 * Server-only weather response for a lit campfire. The normalized output is
 * replicated by the owning actor; clients never calculate fuel state.
 */
struct KALMALAGAMEPLAY_API FKalmalaCampfireWeatherResponse
{
    static constexpr float ExtinguishWetness = 0.90f;
    static constexpr float HighlyActiveStormFuelWetnessMaximumBonus = 0.25f;

    static float GetStormFuelWetnessMultiplier(const float StormIntensity)
    {
        if (!FMath::IsFinite(StormIntensity)) return 1.0f;
        const float StormFraction = FMath::Clamp(
            (FMath::Clamp(StormIntensity, 0.0f, 1.0f) - FKalmalaWeatherState::HighlyActiveStormThreshold)
                / (1.0f - FKalmalaWeatherState::HighlyActiveStormThreshold),
            0.0f, 1.0f);
        return 1.0f + StormFraction * HighlyActiveStormFuelWetnessMaximumBonus;
    }

    static float AdvanceFuelWetness(const float CurrentWetness, const float PrecipitationIntensity, const float WindStrength, const float DeltaSeconds, const bool bLit)
    {
        const float Rain = FMath::Clamp(PrecipitationIntensity, 0.0f, 1.0f);
        const float Wind = FMath::Clamp(WindStrength, 0.0f, 1.0f);
        const float StormIntensity = Rain * Wind;
        const float RainWetnessRate = Rain * (0.035f + Wind * 0.045f) * GetStormFuelWetnessMultiplier(StormIntensity);
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
