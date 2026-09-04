#pragma once

#include "CoreMinimal.h"

/** Server-only math for the recoverable exposure consequence. */
struct KALMALAGAMEPLAY_API FKalmalaExposureResponse
{
    static float AdvanceWetness(const float CurrentWetness, const float Precipitation, const float GroundWetness, const float WindExposure, const float Shelter, const float FireWarmth, const float DeltaSeconds)
    {
        const float SoakingRate = FMath::Clamp(Precipitation, 0.0f, 1.0f) * (0.52f + FMath::Clamp(WindExposure, 0.0f, 1.0f) * 0.28f)
            + FMath::Clamp(GroundWetness, 0.0f, 1.0f) * 0.18f;
        const float DryingRate = FMath::Clamp(Shelter, 0.0f, 1.0f) * 0.36f + FMath::Clamp(FireWarmth, 0.0f, 1.0f) * 0.72f;
        return FMath::Clamp(CurrentWetness + (SoakingRate - DryingRate) * FMath::Max(DeltaSeconds, 0.0f), 0.0f, 100.0f);
    }

    static float AdvanceWarmth(const float CurrentWarmth, const float AmbientTemperature, const float Wetness, const float WindExposure, const float Shelter, const float FireWarmth, const float DeltaSeconds)
    {
        const float ColdPressure = FMath::Clamp((-AmbientTemperature) / 20.0f, 0.0f, 1.0f);
        const float ExposureLoss = ColdPressure * (0.34f + FMath::Clamp(Wetness, 0.0f, 100.0f) / 100.0f * 0.54f + FMath::Clamp(WindExposure, 0.0f, 1.0f) * 0.32f) * (1.0f - FMath::Clamp(Shelter, 0.0f, 1.0f) * 0.72f);
        const float Recovery = FMath::Clamp(FireWarmth, 0.0f, 1.0f) * 2.6f + FMath::Clamp(Shelter, 0.0f, 1.0f) * (Wetness <= 35.0f ? 0.28f : 0.0f);
        return FMath::Clamp(CurrentWarmth + (Recovery - ExposureLoss) * FMath::Max(DeltaSeconds, 0.0f), 0.0f, 100.0f);
    }

    static float GetTravelSpeedMultiplier(const float Warmth)
    {
        return FMath::Lerp(0.68f, 1.0f, FMath::Clamp((Warmth - 15.0f) / 35.0f, 0.0f, 1.0f));
    }
};
