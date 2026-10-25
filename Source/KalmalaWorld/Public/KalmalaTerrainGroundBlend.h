#pragma once

#include "CoreMinimal.h"

/** Deterministic cosmetic weights for generated terrain ground materials. */
struct KALMALAWORLD_API FKalmalaTerrainGroundBlend
{
    static constexpr float SoilMoistureStart = 0.40f;
    static constexpr float SoilMoistureFull = 0.72f;
    static constexpr float RockSlopeStart = 0.08f;
    static constexpr float RockSlopeFull = 0.30f;

    /** Vertex color RGB carries normalized grass, soil and rock weights. */
    static FLinearColor SampleWeights(const float SurfaceNormalZ, const float Moisture)
    {
        const float NormalZ = FMath::IsFinite(SurfaceNormalZ)
            ? FMath::Clamp(SurfaceNormalZ, 0.0f, 1.0f) : 1.0f;
        const float NormalizedMoisture = FMath::IsFinite(Moisture)
            ? FMath::Clamp(Moisture, 0.0f, 1.0f) : 0.5f;
        const float Slope = 1.0f - NormalZ;
        const float RockWeight = SmoothStep(RockSlopeStart, RockSlopeFull, Slope);
        const float SoilMoistureWeight = SmoothStep(SoilMoistureStart, SoilMoistureFull, NormalizedMoisture);
        const float LowlandWeight = 1.0f - RockWeight;
        return FLinearColor(
            LowlandWeight * (1.0f - SoilMoistureWeight),
            LowlandWeight * SoilMoistureWeight,
            RockWeight,
            1.0f);
    }

private:
    static float SmoothStep(const float Start, const float End, const float Value)
    {
        const float T = FMath::Clamp((Value - Start) / (End - Start), 0.0f, 1.0f);
        return T * T * (3.0f - 2.0f * T);
    }
};
