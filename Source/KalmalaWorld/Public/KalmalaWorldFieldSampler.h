#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationSeeds.h"

struct FKalmalaWorldFieldSample
{
    float Elevation = 0.0f;
    float Humidity = 0.0f;
    float Temperature = 0.0f;
    float Flora = 0.0f;
};

/** Continuous, deterministic normalized Perlin samples for a world position. */
struct KALMALAWORLD_API FKalmalaWorldFieldSampler
{
    static FKalmalaWorldFieldSample Sample(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        FKalmalaWorldFieldSample Result;
        Result.Elevation = SampleField(Config, EKalmalaWorldField::Elevation, Position, 0.00035f);
        Result.Humidity = SampleField(Config, EKalmalaWorldField::Humidity, Position, 0.00050f);
        Result.Temperature = SampleField(Config, EKalmalaWorldField::Temperature, Position, 0.00028f);
        Result.Flora = SampleField(Config, EKalmalaWorldField::Flora, Position, 0.00075f);
        return Result;
    }

private:
    static float SampleField(const FKalmalaWorldGenerationConfig& Config, const EKalmalaWorldField Field, const FVector2D Position, const float Frequency)
    {
        const uint64 Seed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(Config, Field);
        const FVector2D Offset(
            static_cast<float>(Seed & 0xFFFFull) * 31.0f,
            static_cast<float>((Seed >> 16) & 0xFFFFull) * 31.0f);
        return FMath::Clamp((FMath::PerlinNoise2D(Position * Frequency + Offset) + 1.0f) * 0.5f, 0.0f, 1.0f);
    }
};
