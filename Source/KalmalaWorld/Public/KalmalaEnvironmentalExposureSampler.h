#pragma once

#include "CoreMinimal.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaTerrainHeightSampler.h"

/** Continuous, server-consumed environmental inputs for pawn exposure. */
struct KALMALAWORLD_API FKalmalaEnvironmentalExposureSample
{
    float AmbientTemperature = 0.0f;
    float GroundWetness = 0.0f;
    float WindExposure = 0.0f;
    float NaturalCover = 0.0f;
    float RidgeExposure = 0.0f;
    float ShorelineWetness = 0.0f;
    bool bIsLowWetGround = false;
    bool bIsShoreline = false;
};

/** Samples local terrain and vegetation, never authored zones or client state. */
struct KALMALAWORLD_API FKalmalaEnvironmentalExposureSampler
{
    static constexpr float ShorelineProbeDistance = 400.0f;

    static bool IsSurfaceWaterAt(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        return FKalmalaOceanSampler::Sample(Config, Position).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
    }

    static FKalmalaEnvironmentalExposureSample Sample(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
        const FVector SurfaceNormal = FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position);
        const float TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, Position);

        FKalmalaEnvironmentalExposureSample Result;
        Result.AmbientTemperature = FMath::Lerp(-20.0f, 20.0f, Fields.Temperature);
        Result.NaturalCover = FMath::Clamp(Fields.Flora, 0.0f, 1.0f);
        Result.RidgeExposure = FMath::SmoothStep(0.54f, 0.84f, Fields.Elevation);
        const float LowGroundWetness = FMath::SmoothStep(700.0f, -200.0f, TerrainHeight);

        const bool bWaterAtPosition = IsSurfaceWaterAt(Config, Position);
        const bool bWaterNearby = bWaterAtPosition
            || IsSurfaceWaterAt(Config, Position + FVector2D(ShorelineProbeDistance, 0.0f))
            || IsSurfaceWaterAt(Config, Position - FVector2D(ShorelineProbeDistance, 0.0f))
            || IsSurfaceWaterAt(Config, Position + FVector2D(0.0f, ShorelineProbeDistance))
            || IsSurfaceWaterAt(Config, Position - FVector2D(0.0f, ShorelineProbeDistance));

        Result.bIsShoreline = bWaterNearby;
        Result.ShorelineWetness = bWaterAtPosition ? 1.0f : (bWaterNearby ? 0.65f : 0.0f);
        Result.bIsLowWetGround = LowGroundWetness >= 0.5f && Fields.Humidity >= 0.5f;
        Result.GroundWetness = FMath::Clamp(Fields.Humidity * 0.55f + LowGroundWetness * 0.25f + Result.ShorelineWetness * 0.35f, 0.0f, 1.0f);
        Result.WindExposure = FMath::Clamp(0.15f + Result.RidgeExposure * 0.55f + (1.0f - SurfaceNormal.Z) * 0.35f - Result.NaturalCover * 0.50f, 0.0f, 1.0f);
        return Result;
    }
};
