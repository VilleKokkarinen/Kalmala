#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

enum class EKalmalaWorldField : uint8
{
    Elevation,
    Humidity,
    Temperature,
    Flora
};

/** Deterministic, independent seeds for continuous world-generation fields. */
struct KALMALAWORLD_API FKalmalaWorldGenerationSeeds
{
    static uint64 DeriveFieldSeed(const FKalmalaWorldGenerationConfig& Config, const EKalmalaWorldField Field)
    {
        uint64 Value = Config.WorldSeed ^ (static_cast<uint64>(Config.GeneratorRevision) << 32)
            ^ (0x9E3779B97F4A7C15ull * (static_cast<uint64>(Field) + 1ull));
        Value ^= Value >> 30;
        Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27;
        Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }
};
