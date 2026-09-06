#pragma once

#include "KalmalaOceanSampler.h"

/** Finds existing seed terrain that emerges from open sea; it never deforms terrain or reserves content. */
struct KALMALAWORLD_API FKalmalaIslandLocator
{
    static bool FindNearest(const FKalmalaWorldGenerationConfig& Config, const FVector2D From, FVector2D& OutLocation)
    {
        if (!Config.IsValid()) return false;
        float BestDistance = TNumericLimits<float>::Max();
        for (int32 Radius = 2500; Radius <= 100000; Radius += 1250)
        {
            for (int32 Direction = 0; Direction < 16; ++Direction)
            {
                const float Angle = Direction * (2.0f * PI / 16.0f);
                const FVector2D Candidate = From + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
                const FKalmalaOceanSample Centre = FKalmalaOceanSampler::Sample(Config, Candidate);
                if (!Centre.bIsValid || Centre.TerrainHeight < 25.0f) continue;
                int32 SeaDirections = 0;
                for (int32 ShoreDirection = 0; ShoreDirection < 8; ++ShoreDirection)
                {
                    const float ShoreAngle = ShoreDirection * (2.0f * PI / 8.0f);
                    SeaDirections += FKalmalaOceanSampler::Sample(Config, Candidate + FVector2D(FMath::Cos(ShoreAngle), FMath::Sin(ShoreAngle)) * 625.0f).IsWater() ? 1 : 0;
                }
                // A majority of sampled shoreline directions being water makes this an
                // emergent islet/coastal outcrop without inventing a separate island map.
                if (SeaDirections >= 5 && Radius < BestDistance) { BestDistance = Radius; OutLocation = Candidate; }
            }
            if (BestDistance != TNumericLimits<float>::Max()) return true;
        }
        return false;
    }
};
