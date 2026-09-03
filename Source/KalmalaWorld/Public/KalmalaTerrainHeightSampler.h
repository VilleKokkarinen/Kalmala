#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldFieldSampler.h"

/**
 * Converts the continuous Elevation field into the shared terrain surface used
 * by spawning and future terrain rendering/collision. It never creates terrain
 * or gameplay state on a client.
 */
struct KALMALAWORLD_API FKalmalaTerrainHeightSampler
{
    static constexpr float SeaLevelElevation = 0.22f;
    static constexpr float SeaLevelWorldHeight = 0.0f;
    static constexpr float WorldUnitsPerElevation = 2000.0f;
    static constexpr float NormalSampleDistance = 100.0f;

    static float SampleHeight(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const float Elevation = FKalmalaWorldFieldSampler::Sample(Config, Position).Elevation;
        return SeaLevelWorldHeight + (Elevation - SeaLevelElevation) * WorldUnitsPerElevation;
    }

    static FVector SampleSurfaceNormal(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const float WestHeight = SampleHeight(Config, Position - FVector2D(NormalSampleDistance, 0.0f));
        const float EastHeight = SampleHeight(Config, Position + FVector2D(NormalSampleDistance, 0.0f));
        const float SouthHeight = SampleHeight(Config, Position - FVector2D(0.0f, NormalSampleDistance));
        const float NorthHeight = SampleHeight(Config, Position + FVector2D(0.0f, NormalSampleDistance));
        return FVector(
            -(EastHeight - WestHeight) / (2.0f * NormalSampleDistance),
            -(NorthHeight - SouthHeight) / (2.0f * NormalSampleDistance),
            1.0f).GetSafeNormal();
    }
};
