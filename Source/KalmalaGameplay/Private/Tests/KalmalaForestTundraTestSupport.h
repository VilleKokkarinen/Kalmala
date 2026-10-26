#pragma once

#include "KalmalaBiomeClassifier.h"
#include "KalmalaForestLayout.h"
#include "KalmalaWorldFieldSampler.h"

namespace KalmalaForestTundraTestSupport
{
    inline bool FindPatch(const FKalmalaWorldGenerationConfig& Config, const int32 MinimumTrees,
        FVector2D& OutCenter, TArray<FKalmalaForestTreeDescriptor>& OutTrees)
    {
        constexpr int32 AngleSteps = 72;
        constexpr double MinimumRadius = 400000.0;
        constexpr double MaximumRadius = 1500000.0;
        constexpr double RadiusStep = 25000.0;
        for (double Radius = MinimumRadius; Radius <= MaximumRadius; Radius += RadiusStep)
        {
            for (int32 AngleIndex = 0; AngleIndex < AngleSteps; ++AngleIndex)
            {
                const double Angle = 2.0 * PI * AngleIndex / AngleSteps;
                const FVector2D Center(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
                if (FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Config, Center)) != EKalmalaBiome::FreezingTundra)
                {
                    continue;
                }

                TArray<FKalmalaForestTreeDescriptor> TundraTrees;
                for (const FKalmalaForestTreeDescriptor& Tree : FKalmalaForestLayout::Generate(Config, Center))
                {
                    if (Tree.TreeKind == EKalmalaForestTreeKind::FreezingTundraBirch)
                    {
                        TundraTrees.Add(Tree);
                    }
                }
                if (TundraTrees.Num() < MinimumTrees) continue;
                OutCenter = Center;
                OutTrees = MoveTemp(TundraTrees);
                return true;
            }
        }
        return false;
    }
}
