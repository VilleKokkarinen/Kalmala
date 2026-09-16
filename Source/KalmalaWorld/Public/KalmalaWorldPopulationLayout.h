#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaBiomeClassifier.h"

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

/**
 * Server-only optional discovery domains. These are descriptor inputs, not
 * replicated content types: a client never receives candidates in order to
 * query, reserve, or route toward an undiscovered location.
 */
enum class EKalmalaWorldDiscoveryKind : uint8
{
    PointOfInterest,
    Scroll
};

struct KALMALAWORLD_API FKalmalaWorldPopulationSpawn
{
    EKalmalaWorldPopulationKind Kind = EKalmalaWorldPopulationKind::Wildlife;
    FIntPoint SpatialKey = FIntPoint::ZeroValue;
    uint64 SpawnSeed = 0;
    FVector Location = FVector::ZeroVector;
};

struct KALMALAWORLD_API FKalmalaWorldDiscoveryDescriptor
{
    EKalmalaWorldDiscoveryKind Kind = EKalmalaWorldDiscoveryKind::PointOfInterest;
    FIntPoint SpatialKey = FIntPoint::ZeroValue;
    uint64 DescriptorSeed = 0;
    int32 Ordinal = 0;
    FString DefinitionId;
    FVector Location = FVector::ZeroVector;
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
        uint64 Value = Config.WorldSeed;
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

    static TArray<FKalmalaWorldPopulationSpawn> BuildSpawnDescriptors(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaWorldPopulationKind Kind)
    {
        TArray<FKalmalaWorldPopulationSpawn> Spawns;
        const int32 Budget = GetSpawnBudget(Config, SpatialKey, Kind);
        Spawns.Reserve(Budget);
        const FVector2D SpatialKeyOrigin = FVector2D(SpatialKey) * SpatialKeySize;
        // Wildlife candidates deliberately have a small, fixed retry budget.  The
        // server derives all candidates, rejects water and unsafe slopes, and
        // keeps the candidate seed in the sparse-delta ID.  Clients never choose
        // a fallback location or receive descriptors for actors outside relevancy.
        const int32 CandidateBudget = Kind == EKalmalaWorldPopulationKind::Wildlife ? Budget * 4 : Budget;
        for (int32 SpawnIndex = 0; SpawnIndex < CandidateBudget && Spawns.Num() < Budget; ++SpawnIndex)
        {
            const uint64 SpawnSeed = Mix(DeriveSpatialSeed(Config, SpatialKey, Kind) ^ static_cast<uint64>(SpawnIndex + 1));
            const float XFraction = static_cast<float>(SpawnSeed & 0xFFFFu) / 65535.0f;
            const float YFraction = static_cast<float>((SpawnSeed >> 16) & 0xFFFFu) / 65535.0f;
            const FVector2D Position = SpatialKeyOrigin + FVector2D(XFraction, YFraction) * SpatialKeySize;
            if (!FKalmalaWorldBounds::Contains(Config, Position)) continue;
            if (Kind == EKalmalaWorldPopulationKind::Wildlife && !IsTerrainSafeWildlifeLocation(Config, Position)) continue;
            Spawns.Add({ Kind, SpatialKey, SpawnSeed, FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position)) });
        }
        return Spawns;
    }

    /** Stable server identifier for sparse deltas within a world identity/revision save. */
    static FString GetPersistentSpawnId(const FKalmalaWorldPopulationSpawn& Spawn)
    {
        return FString::Printf(TEXT("%d/%d/%d/%llu"), static_cast<uint8>(Spawn.Kind), Spawn.SpatialKey.X, Spawn.SpatialKey.Y, Spawn.SpawnSeed);
    }

    /**
     * Builds at most one optional descriptor of each discovery kind per
     * invisible spatial key. Candidates use a fixed retry bound and only land
     * in a biome suitable for the selected definition; rejected candidates do
     * not create a fallback chosen by an actor or client.
     */
    static TArray<FKalmalaWorldDiscoveryDescriptor> BuildDiscoveryDescriptors(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaWorldDiscoveryKind Kind)
    {
        TArray<FKalmalaWorldDiscoveryDescriptor> Descriptors;
        const FVector2D SpatialKeyOrigin = FVector2D(SpatialKey) * SpatialKeySize;
        constexpr int32 CandidateBudget = 8;
        for (int32 CandidateOrdinal = 0; CandidateOrdinal < CandidateBudget; ++CandidateOrdinal)
        {
            const uint64 DescriptorSeed = Mix(DeriveDiscoverySeed(Config, SpatialKey, Kind) ^ static_cast<uint64>(CandidateOrdinal + 1));
            const FVector2D Position = SpatialKeyOrigin + FVector2D(
                static_cast<float>(DescriptorSeed & 0xFFFFu) / 65535.0f,
                static_cast<float>((DescriptorSeed >> 16) & 0xFFFFu) / 65535.0f) * SpatialKeySize;
            if (!FKalmalaWorldBounds::Contains(Config, Position) || !IsTerrainSafeDiscoveryLocation(Config, Position)) continue;

            const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position));
            const FString DefinitionId = GetDiscoveryDefinition(Kind, Biome, DescriptorSeed);
            if (DefinitionId.IsEmpty()) continue;

            FKalmalaWorldDiscoveryDescriptor& Descriptor = Descriptors.AddDefaulted_GetRef();
            Descriptor.Kind = Kind;
            Descriptor.SpatialKey = SpatialKey;
            Descriptor.DescriptorSeed = DescriptorSeed;
            Descriptor.Ordinal = CandidateOrdinal;
            Descriptor.DefinitionId = DefinitionId;
            Descriptor.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position));
            break;
        }
        return Descriptors;
    }

    /** Canonical bounded server ID; the matching world identity remains the save-container key. */
    static FString GetPersistentDiscoveryId(const FKalmalaWorldDiscoveryDescriptor& Descriptor)
    {
        const TCHAR* KindToken = Descriptor.Kind == EKalmalaWorldDiscoveryKind::PointOfInterest ? TEXT("Poi") : TEXT("Scroll");
        return FString::Printf(TEXT("%s:1:%s:%d,%d:%d"), KindToken, *Descriptor.DefinitionId, Descriptor.SpatialKey.X, Descriptor.SpatialKey.Y, Descriptor.Ordinal);
    }

private:
    static uint64 DeriveDiscoverySeed(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaWorldDiscoveryKind Kind)
    {
        uint64 Value = Config.WorldSeed ^ 0xD15C0A71E5ull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.X)) * 0xA24BAED4963EE407ull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.Y)) * 0x9FB21C651E98DF25ull;
        Value ^= (static_cast<uint64>(Kind) + 1ull) * 0xDB4F0B9175AE2165ull;
        return Mix(Value);
    }

    static FString GetDiscoveryDefinition(const EKalmalaWorldDiscoveryKind Kind, const EKalmalaBiome Biome, const uint64 DescriptorSeed)
    {
        if (Kind == EKalmalaWorldDiscoveryKind::PointOfInterest)
        {
            switch (Biome)
            {
            case EKalmalaBiome::Meadows: return TEXT("meadow-stone");
            case EKalmalaBiome::ShimmeringLakes: return TEXT("lake-stone");
            case EKalmalaBiome::Elderwood: return TEXT("root-hollow");
            case EKalmalaBiome::MossyMire: return TEXT("mire-stone");
            case EKalmalaBiome::FreezingTundra: return TEXT("ice-spring");
            case EKalmalaBiome::ThunderMountains: return TEXT("storm-overlook");
            default: return FString();
            }
        }

        switch (Biome)
        {
        case EKalmalaBiome::Meadows: return TEXT("mending");
        case EKalmalaBiome::Elderwood: return TEXT("mending");
        case EKalmalaBiome::MossyMire: return TEXT("deer-call");
        case EKalmalaBiome::FreezingTundra: return TEXT("bears-vigor");
        case EKalmalaBiome::ThunderMountains: return TEXT("hearth-shield");
        case EKalmalaBiome::ShimmeringLakes: return DescriptorSeed % 2ull == 0ull ? TEXT("mending") : TEXT("hearth-shield");
        default: return FString();
        }
    }

    static bool IsTerrainSafeDiscoveryLocation(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        return !FKalmalaOceanSampler::Sample(Config, Position).IsWater()
            && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position)
            && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.88f;
    }

    static bool IsTerrainSafeWildlifeLocation(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        // Wildlife remains on dry, gently traversable generated terrain.  Both
        // water samplers and the collision-height normal are deterministic
        // functions of the immutable world identity.
        return !FKalmalaOceanSampler::Sample(Config, Position).IsWater()
            && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position)
            && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.82f;
    }

    static uint64 Mix(uint64 Value)
    {
        Value ^= Value >> 30;
        Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27;
        Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }
};
