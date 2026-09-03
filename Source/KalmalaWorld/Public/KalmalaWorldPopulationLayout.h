#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationConfig.h"

/**
 * Server-only deterministic population inputs. Spatial keys are invisible
 * simulation partitions, not terrain, biome, or gameplay-area boundaries.
 */
enum class EKalmalaWorldPopulationKind : uint8
{
    Wildlife,
    HarvestNode,
    Hazard
};

struct KALMALAWORLD_API FKalmalaWorldPopulationLayout
{
    static constexpr float SpatialKeySize = 6000.0f;

    static FIntPoint GetSpatialKey(const FVector2D WorldPosition)
    {
        return FIntPoint(
            FMath::FloorToInt(WorldPosition.X / SpatialKeySize),
            FMath::FloorToInt(WorldPosition.Y / SpatialKeySize));
    }

    static uint64 DeriveSpatialSeed(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaWorldPopulationKind Kind)
    {
        uint64 Value = Config.WorldSeed ^ (static_cast<uint64>(Config.GeneratorRevision) << 32);
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.X)) * 0x9E3779B185EBCA87ull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.Y)) * 0xC2B2AE3D27D4EB4Full;
        Value ^= (static_cast<uint64>(Kind) + 1ull) * 0x165667B19E3779F9ull;
        Value ^= Value >> 30;
        Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27;
        Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }

    static int32 GetSpawnBudget(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaWorldPopulationKind Kind)
    {
        const FVector2D KeyCenter = (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f)) * SpatialKeySize;
        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, KeyCenter);
        switch (Kind)
        {
        case EKalmalaWorldPopulationKind::Wildlife:
            return 1 + FMath::FloorToInt(Fields.Flora * 2.0f);
        case EKalmalaWorldPopulationKind::HarvestNode:
            return 2 + FMath::FloorToInt(Fields.Flora * 4.0f);
        case EKalmalaWorldPopulationKind::Hazard:
            return FMath::FloorToInt(Fields.Humidity * 2.0f);
        default:
            return 0;
        }
    }
};
