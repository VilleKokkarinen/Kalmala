#pragma once

#include "KalmalaBiomeClassifier.h"
#include "KalmalaRegionalTuning.h"
#include "KalmalaTerrainHeightSampler.h"

/**
 * Pure lake-water rules shared by local Shimmering Lakes rendering. The lake
 * surface is a seed-derived cosmetic treatment, never a client-owned physics
 * or gameplay boundary.
 */
struct KALMALAWORLD_API FKalmalaShimmeringLakeSampler
{
    static constexpr float WaterSurfaceWorldHeight = 400.0f;
    static constexpr bool bEnabled = FKalmalaRegionalTuning::bEnableAnalyticBasins;

    static bool IsWater(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        if (!bEnabled) return false;
        const auto Region = FKalmalaRegionalGeneration::Sample(Config, Position);
        return Region.BasinWeight > 0 && Region.bHasWater;
    }
};
