#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationSeeds.h"
#include "KalmalaRegionalTuning.h"
#include "KalmalaMasterMap.h"

struct FKalmalaWorldFieldSample
{
    float Elevation = 0.0f;
    float Humidity = 0.0f;
    float Temperature = 0.0f;
    float Flora = 0.0f;

    /** World seed and position metadata, not additional procedural fields. */
    uint64 WorldSeed = 0;
    FVector2D Position = FVector2D::ZeroVector;
};

/** Continuous, deterministic normalized Perlin samples for a world position. */
struct KALMALAWORLD_API FKalmalaWorldFieldSampler
{
    static FKalmalaWorldFieldSample Sample(
        const FKalmalaWorldGenerationConfig& Config,
        const FVector2D Position)
    {
        FKalmalaWorldFieldSample Result;
        Result.WorldSeed = Config.WorldSeed;
        Result.Position = Position;

        // Macro relief/climate, with small local relief and independent local flora.
        Result.Elevation = FMath::Clamp(0.5f + FKalmalaRegionalTuning::ElevationAmplitude * SampleNoise(Config, EKalmalaWorldField::Elevation, Position, FKalmalaRegionalTuning::ElevationFrequency)
            + FKalmalaRegionalTuning::LocalReliefAmplitude * SampleNoise(Config, EKalmalaWorldField::Elevation, Position, FKalmalaRegionalTuning::LocalReliefFrequency), 0.0f, 1.0f);
        Result.Humidity = FMath::Clamp(0.5f + FKalmalaRegionalTuning::ClimateAmplitude * SampleNoise(Config, EKalmalaWorldField::Humidity, Position, FKalmalaRegionalTuning::ClimateFrequency), 0.0f, 1.0f);
        Result.Temperature = FMath::Clamp(0.5f + FKalmalaRegionalTuning::ClimateAmplitude * SampleNoise(Config, EKalmalaWorldField::Temperature, Position, FKalmalaRegionalTuning::ClimateFrequency), 0.0f, 1.0f);
        Result.Flora = FMath::Clamp(0.5f + 0.5f * SampleNoise(Config, EKalmalaWorldField::Flora, Position, FKalmalaRegionalTuning::FloraFrequency), 0.0f, 1.0f);
        const double Mask = FKalmalaMasterMap::Sample(Config, Position);
        const double Coast = FMath::Clamp(Mask / 0.12, 0.0, 1.0);
        // Master mask alone determines sea-level sign. Existing Elevation
        // noise supplies interior relief, joined continuously at the coast.
        Result.Elevation = Mask > 0
                ? FKalmalaRegionalTuning::SeaElevation + Coast * Coast * (3 - 2 * Coast) * (0.10 + 0.65 * Result.Elevation)
                : FMath::Max(0.0, FKalmalaRegionalTuning::SeaElevation + Mask * 0.8);
        return Result;
    }

private:
    static float SampleNoise(const FKalmalaWorldGenerationConfig& Config, EKalmalaWorldField Field, FVector2D Position, double Frequency)
    {
        const uint64 Seed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(Config, Field);
        // Small offsets preserve local coordinate precision.
        const FVector2D Offset(double(Seed & 0xffff) / 257.0, double((Seed >> 16) & 0xffff) / 257.0);
        return FMath::PerlinNoise2D(Position * Frequency + Offset);
    }
};
