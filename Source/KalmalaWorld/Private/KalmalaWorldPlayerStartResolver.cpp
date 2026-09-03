#include "KalmalaWorldPlayerStartResolver.h"

#include "KalmalaBiomeClassifier.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationSeeds.h"

FTransform FKalmalaWorldPlayerStartResolver::ResolveStartTransform(const FKalmalaWorldGenerationConfig& Config)
{
    const uint64 SelectionSeed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(Config, EKalmalaWorldField::Flora);
    float BestScore = TNumericLimits<float>::Max();
    FVector2D BestLocation = FVector2D::ZeroVector;

    for (int32 CandidateIndex = 0; CandidateIndex < 24; ++CandidateIndex)
    {
        const uint64 CandidateBits = (SelectionSeed >> ((CandidateIndex % 4) * 16)) & 0xFFFFull;
        const float Angle = 2.0f * PI * FMath::Frac((static_cast<float>(CandidateBits) + CandidateIndex * 4051.0f) / 65536.0f);
        const float Radius = 1500.0f + CandidateIndex * 650.0f;
        const FVector2D CandidateLocation(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
        const FKalmalaWorldFieldSample Sample = FKalmalaWorldFieldSampler::Sample(Config, CandidateLocation);

        float Score = FMath::Abs(Sample.Elevation - 0.5f)
            + FMath::Abs(Sample.Humidity - 0.5f)
            + FMath::Abs(Sample.Temperature - 0.55f)
            + FMath::Abs(Sample.Flora - 0.5f);
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

    // Terrain height will replace this temporary prototype elevation when generated terrain arrives.
    return FTransform(FRotator::ZeroRotator, FVector(BestLocation.X, BestLocation.Y, 120.0f));
}
