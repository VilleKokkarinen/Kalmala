#include "KalmalaWorldPlayerStartResolver.h"

#include "KalmalaBiomeClassifier.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationSeeds.h"

FTransform FKalmalaWorldPlayerStartResolver::ResolveStartTransform(const FKalmalaWorldGenerationConfig& Config)
{
    const uint64 SelectionSeed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(Config, EKalmalaWorldField::Flora);
    float BestScore = TNumericLimits<float>::Max();
    FVector2D BestLocation = FVector2D::ZeroVector;

    const int32 CandidateCount = 128;
    for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount; ++CandidateIndex)
    {
        const uint64 CandidateBits = (SelectionSeed >> ((CandidateIndex % 4) * 16)) & 0xFFFFull;
        const float Angle = 2.0f * PI * FMath::Frac((static_cast<float>(CandidateBits) + CandidateIndex * 4051.0f) / 65536.0f);
        const float Radius = 1500.0f + CandidateIndex * 240.0f;
        const FVector2D CandidateLocation(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
        const FKalmalaWorldFieldSample Sample = FKalmalaWorldFieldSampler::Sample(Config, CandidateLocation);

        float Score = FMath::Abs(Sample.Elevation - 0.5f)
            + FMath::Abs(Sample.Humidity - 0.5f)
            + FMath::Abs(Sample.Temperature - 0.55f)
            + FMath::Abs(Sample.Flora - 0.5f);
        {
            const auto Region = FKalmalaRegionalGeneration::Sample(Sample);
            if (Region.bHasWater || Region.Height < 40) continue;
        }
        if (FKalmalaBiomeClassifier::Classify(Sample) != EKalmalaBiome::Meadows)
        {
            Score += 10.0f;
        }

        if (Score < BestScore)
        {
            BestScore = Score;
            BestLocation = CandidateLocation;
        }
    }

    constexpr float PawnClearance = 136.0f;
    return FTransform(
        FRotator::ZeroRotator,
        FVector(BestLocation.X, BestLocation.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, BestLocation) + PawnClearance));
}
