#pragma once

#include "KalmalaWorldFieldSampler.h"

struct FKalmalaRegionalSample
{
    // Same stable order as EKalmalaBiome. Normalized and deterministic tie-breaking.
    float Weights[7] = {};
    uint8 Biome = 0;
    float Height = 0;
    float WaterLevel = 0;
    float RiverWeight = 0;
    float StreamWeight = 0;
    float BasinWeight = 0;
    bool bHasWater = false;
};

struct FKalmalaHydrologySegment
{
    FVector A = FVector::ZeroVector;
    FVector B = FVector::ZeroVector;
    float Width = 0;
    bool bStream = false;
    uint64 Id = 0;
};

/** Pure regions/shaping plus a bounded, disposable spline-only spatial cache. */
struct KALMALAWORLD_API FKalmalaRegionalGeneration
{
    static FKalmalaRegionalSample Sample(const FKalmalaWorldGenerationConfig& Config, FVector2D Position);
    static FKalmalaRegionalSample Sample(const FKalmalaWorldFieldSample& Fields);
    /** Exact current biome identity, excluding hydrology/height-only work. */
    static uint8 SampleBiome(const FKalmalaWorldFieldSample& Fields);
    static TArray<FKalmalaHydrologySegment> GetHydrology(const FKalmalaWorldGenerationConfig& Config, FIntPoint GridCell);
    static void ClearHydrologyCache();
    static bool AreStreamsEnabled(const FKalmalaWorldGenerationConfig& C)
    { return false; }
    static uint64 Seed(const FKalmalaWorldGenerationConfig& Config, uint64 Domain, FIntPoint Cell = FIntPoint::ZeroValue);
    static double Noise(const FKalmalaWorldGenerationConfig& Config, uint64 Domain, FVector2D Position, double Frequency);
private:
    static FKalmalaRegionalSample SampleInternal(const FKalmalaWorldFieldSample& Fields, bool bBiomeOnly);
};
