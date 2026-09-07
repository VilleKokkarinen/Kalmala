#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationSeeds.h"
#include "KalmalaRegionalTuning.h"

struct FKalmalaWorldFieldSample
{
    float Elevation = 0.0f;
    float Humidity = 0.0f;
    float Temperature = 0.0f;
    float Flora = 0.0f;

    /** Rule-version metadata, not a fifth procedural field. */
    int32 GeneratorRevision = 2;
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
        constexpr float BiomeScale = 1.0f; // Smaller = larger biomes

        FKalmalaWorldFieldSample Result;
        Result.GeneratorRevision = Config.GeneratorRevision;
        Result.WorldSeed = Config.WorldSeed;
        Result.Position = Position;

        if (Config.GeneratorRevision >= 3)
        {
            // Macro relief/climate, with small local relief and independent local flora.
            Result.Elevation = FMath::Clamp(0.5f + FKalmalaRegionalTuning::ElevationAmplitude * SampleModern(Config, EKalmalaWorldField::Elevation, Position, FKalmalaRegionalTuning::ElevationFrequency)
                + FKalmalaRegionalTuning::LocalReliefAmplitude * SampleModern(Config, EKalmalaWorldField::Elevation, Position, FKalmalaRegionalTuning::LocalReliefFrequency), 0.0f, 1.0f);
            Result.Humidity = FMath::Clamp(0.5f + FKalmalaRegionalTuning::ClimateAmplitude * SampleModern(Config, EKalmalaWorldField::Humidity, Position, FKalmalaRegionalTuning::ClimateFrequency), 0.0f, 1.0f);
            Result.Temperature = FMath::Clamp(0.5f + FKalmalaRegionalTuning::ClimateAmplitude * SampleModern(Config, EKalmalaWorldField::Temperature, Position, FKalmalaRegionalTuning::ClimateFrequency), 0.0f, 1.0f);
            Result.Flora = FMath::Clamp(0.5f + 0.5f * SampleModern(Config, EKalmalaWorldField::Flora, Position, FKalmalaRegionalTuning::FloraFrequency), 0.0f, 1.0f);
            return Result;
        }

        Result.Elevation = SampleField(
            Config,
            EKalmalaWorldField::Elevation,
            Position,
            0.00035f * BiomeScale);

        Result.Humidity = SampleField(
            Config,
            EKalmalaWorldField::Humidity,
            Position,
            0.00050f * BiomeScale);

        Result.Temperature = SampleField(
            Config,
            EKalmalaWorldField::Temperature,
            Position,
            0.00028f * BiomeScale);

        Result.Flora = SampleField(
            Config,
            EKalmalaWorldField::Flora,
            Position,
            0.00075f * BiomeScale);

        return Result;
    }

private:
    static float SampleModern(const FKalmalaWorldGenerationConfig& Config, EKalmalaWorldField Field, FVector2D Position, double Frequency)
    {
        const uint64 Seed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(Config, Field);
        // Small offsets avoid the metre-scale precision steps in legacy large offsets.
        const FVector2D Offset(double(Seed & 0xffff) / 257.0, double((Seed >> 16) & 0xffff) / 257.0);
        return FMath::PerlinNoise2D(Position * Frequency + Offset);
    }
    static float SampleField(const FKalmalaWorldGenerationConfig& Config, const EKalmalaWorldField Field, const FVector2D Position, const float Frequency)
    {
        const uint64 Seed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(Config, Field);
        const FVector2D Offset(
            static_cast<float>(Seed & 0xFFFFull) * 31.0f,
            static_cast<float>((Seed >> 16) & 0xFFFFull) * 31.0f);
        return FMath::Clamp((FMath::PerlinNoise2D(Position * Frequency + Offset) + 1.0f) * 0.5f, 0.0f, 1.0f);
    }
};
