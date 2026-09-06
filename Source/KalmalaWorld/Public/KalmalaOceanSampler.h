#pragma once

#include "KalmalaTerrainHeightSampler.h"

/** Derived sea depth, not a biome field or replicated gameplay state. */
struct FKalmalaOceanSample
{
    bool bIsValid = false;
    float TerrainHeight = 0.0f;
    float WaterDepth = 0.0f;
    bool IsWater() const { return bIsValid && WaterDepth > 0.0f; }
};

/** Samples the sea over the actual terrain triangle lattice. Gameplay callers
 * must use server-owned identity/position; clients may use this for presentation.
 * Does not infer ocean connectivity, inland lake depth, or active collision. */
struct KALMALAWORLD_API FKalmalaOceanSampler
{
    static FKalmalaOceanSample Sample(const FKalmalaWorldGenerationConfig& Config, FVector2D Position);
    // Explicit lattice origin for patch-level callers and verification.
    static FKalmalaOceanSample Sample(const FKalmalaWorldGenerationConfig& Config,
        FVector2D Position, FVector2D GridOrigin);
};
