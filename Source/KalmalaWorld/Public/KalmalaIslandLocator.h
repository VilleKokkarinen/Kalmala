#pragma once

#include "KalmalaOceanSampler.h"

/** Finds existing seed terrain that emerges from open sea; it never deforms terrain or reserves content. */
struct KALMALAWORLD_API FKalmalaIslandLocator
{
    static bool FindNearest(const FKalmalaWorldGenerationConfig& Config, const FVector2D From, FVector2D& OutLocation)
    {
        if (!Config.IsValid()) return false;
        float BestDistance = TNumericLimits<float>::Max();
        // Sample the same stable radial lattice used by the other developer
        // probes, but at a finer angular resolution than the old 16-ray
        // search. The regional terrain can create a small emergent outcrop
        // between those broad rays; missing it made this derived lookup depend
        // on an arbitrary probe alignment rather than the world identity.
        constexpr int32 CandidateDirections = 96;
        constexpr int32 ShoreDirections = 16;
        constexpr float ShoreRadius = 12500.0f;
        constexpr int32 RequiredWaterDirections = 9;
        for (int32 Radius = 2500; Radius <= 150000; Radius += 1250)
        {
            for (int32 Direction = 0; Direction < CandidateDirections; ++Direction)
            {
                const float Angle = Direction * (2.0f * PI / CandidateDirections);
                const FVector2D Candidate = From + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
                const FKalmalaOceanSample Centre = FKalmalaOceanSampler::Sample(Config, Candidate);
                if (!Centre.bIsValid || Centre.TerrainHeight < 25.0f) continue;
                int32 SeaDirections = 0;
                for (int32 ShoreDirection = 0; ShoreDirection < ShoreDirections; ++ShoreDirection)
                {
                    const float ShoreAngle = ShoreDirection * (2.0f * PI / ShoreDirections);
                    SeaDirections += FKalmalaOceanSampler::Sample(Config, Candidate + FVector2D(FMath::Cos(ShoreAngle), FMath::Sin(ShoreAngle)) * ShoreRadius).IsWater() ? 1 : 0;
                }
                // A majority of the wider shoreline ring must be sea, keeping
                // the result an emergent islet/outcrop rather than a generic
                // mainland coast and without inventing a separate island map.
                if (SeaDirections >= RequiredWaterDirections && Radius < BestDistance) { BestDistance = Radius; OutLocation = Candidate; }
            }
            if (BestDistance != TNumericLimits<float>::Max()) return true;
        }
        return false;
    }
};
