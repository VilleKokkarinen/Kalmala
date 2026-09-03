#pragma once

#include "KalmalaBiomeClassifier.h"
#include "KalmalaTerrainHeightSampler.h"

/**
 * Pure lake-water rules shared by local Shimmering Lakes rendering. The lake
 * surface is a seed-derived cosmetic treatment, never a client-owned physics
 * or gameplay boundary.
 */
struct KALMALAWORLD_API FKalmalaShimmeringLakeSampler
{
    static constexpr float WaterSurfaceWorldHeight = 400.0f;

    static bool IsWater(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const FKalmalaWorldFieldSample FieldSample = FKalmalaWorldFieldSampler::Sample(Config, Position);
        return FKalmalaBiomeClassifier::Classify(FieldSample) == EKalmalaBiome::ShimmeringLakes
            && FKalmalaTerrainHeightSampler::SampleHeight(Config, Position) <= WaterSurfaceWorldHeight;
    }
};
