#pragma once

#include "CoreMinimal.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaWorldPopulationLayout.h"

/** Server-consumed local tradeoffs at a freely chosen camp position. Never reserves or directs a camp. */
struct KALMALAWORLD_API FKalmalaCampConditionSample
{
    float NaturalCover = 0.0f;
    float GroundWetness = 0.0f;
    float WaterDistance = 0.0f;
    float NearbyResourceScore = 0.0f;
    int32 NearbyHarvestNodeCount = 0;
};

struct KALMALAWORLD_API FKalmalaCampConditionSampler
{
    static constexpr float WaterSearchRadius = 3200.0f;
    static constexpr float WaterSearchStep = 200.0f;
    static constexpr float ResourceSearchRadius = 2400.0f;

    static FKalmalaCampConditionSample Sample(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const FKalmalaEnvironmentalExposureSample Environment = FKalmalaEnvironmentalExposureSampler::Sample(Config, Position);
        FKalmalaCampConditionSample Result;
        Result.NaturalCover = Environment.NaturalCover;
        Result.GroundWetness = Environment.GroundWetness;
        Result.WaterDistance = FindNearestWaterDistance(Config, Position);

        const FIntPoint CenterKey = FKalmalaWorldPopulationLayout::GetSpatialKey(Position);
        for (int32 KeyY = CenterKey.Y - 1; KeyY <= CenterKey.Y + 1; ++KeyY)
        {
            for (int32 KeyX = CenterKey.X - 1; KeyX <= CenterKey.X + 1; ++KeyX)
            {
                for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, FIntPoint(KeyX, KeyY), EKalmalaWorldPopulationKind::HarvestNode))
                {
                    if (FVector2D::Distance(Position, FVector2D(Spawn.Location)) <= ResourceSearchRadius)
                    {
                        ++Result.NearbyHarvestNodeCount;
                    }
                }
            }
        }
        Result.NearbyResourceScore = FMath::Clamp(static_cast<float>(Result.NearbyHarvestNodeCount) / 4.0f, 0.0f, 1.0f);
        return Result;
    }

private:
    static float FindNearestWaterDistance(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        if (FKalmalaShimmeringLakeSampler::IsWater(Config, Position))
        {
            return 0.0f;
        }
        static const FVector2D Directions[] = { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1), FVector2D(0.70710678f, 0.70710678f), FVector2D(0.70710678f, -0.70710678f), FVector2D(-0.70710678f, 0.70710678f), FVector2D(-0.70710678f, -0.70710678f) };
        for (float Distance = WaterSearchStep; Distance <= WaterSearchRadius; Distance += WaterSearchStep)
        {
            for (const FVector2D Direction : Directions)
            {
                if (FKalmalaShimmeringLakeSampler::IsWater(Config, Position + Direction * Distance)) return Distance;
            }
        }
        return WaterSearchRadius;
    }
};
