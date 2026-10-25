#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaBiomeContentContract.h"
#include "KalmalaGemstoneContract.h"

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
    /** Server-selected biome material or creature-niche identity. */
    FName ContentId = NAME_None;
    /** Optional server-selected stable slot identity; ordinary descriptors leave this empty. */
    FString PersistentSpawnIdOverride;
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
    static constexpr float SpatialKeySizeCm = 6400.0f;
    // One optional Tin node, six land-biome Stone outcrops and one Tundra salt deposit.
    static constexpr int32 SupplementalHarvestSourceBudget = 8;

    static FIntPoint GetSpatialKey(const FVector2D WorldPosition)
    {
        return FIntPoint(
            FMath::FloorToInt(WorldPosition.X / SpatialKeySizeCm),
            FMath::FloorToInt(WorldPosition.Y / SpatialKeySizeCm));
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
        const FVector2D KeyCenter = (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f)) * SpatialKeySizeCm;
        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, KeyCenter);
        switch (Kind)
        {
        case EKalmalaWorldPopulationKind::Wildlife:
            return 1 + FMath::FloorToInt(Fields.Flora * 2.0f);
        case EKalmalaWorldPopulationKind::HarvestNode:
            return 2 + FMath::FloorToInt(Fields.Flora * 4.0f) + SupplementalHarvestSourceBudget + 1;
        case EKalmalaWorldPopulationKind::Hazard:
            return FMath::FloorToInt(Fields.Humidity * 2.0f);
        default:
            return 0;
        }
    }

    static TArray<FKalmalaWorldPopulationSpawn> BuildSpawnDescriptors(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaWorldPopulationKind Kind, const bool bApplyGemstoneReplacements = true)
    {
        TArray<FKalmalaWorldPopulationSpawn> Spawns;
        const int32 Budget = GetSpawnBudget(Config, SpatialKey, Kind);
        const int32 PrimaryBudget = Kind == EKalmalaWorldPopulationKind::HarvestNode
            ? Budget - SupplementalHarvestSourceBudget : Budget;
        Spawns.Reserve(Budget);
        const FVector2D SpatialKeyOrigin = FVector2D(SpatialKey) * SpatialKeySizeCm;
        bool bAddedMeadowCarrotPatch = false;
        bool bAddedMeadowPotatoPatch = false;
        bool bAddedMeadowLingonberryShrub = false;
        bool bAddedLakesRutabagaPatch = false;
        bool bAddedElderwoodOnionPatch = false;
        bool bAddedElderwoodChanterellePatch = false;
        bool bAddedElderwoodCopperVein = false;
        bool bAddedLakesTinGravel = false;
        // Wildlife candidates deliberately have a small, fixed retry budget. Land
        // animals require dry, safe ground; Ocean candidates require deep generated
        // water for the swimming niche. The seed remains in the sparse-delta ID.
        // Clients never choose a fallback or receive irrelevant actor descriptors.
        const int32 CandidateBudget = Kind == EKalmalaWorldPopulationKind::Wildlife ? PrimaryBudget * 4 : PrimaryBudget;
        for (int32 SpawnIndex = 0; SpawnIndex < CandidateBudget && Spawns.Num() < PrimaryBudget; ++SpawnIndex)
        {
            const uint64 CandidateSeed = Mix(DeriveSpatialSeed(Config, SpatialKey, Kind) ^ static_cast<uint64>(SpawnIndex + 1));
            const float XFraction = static_cast<float>(CandidateSeed & 0xFFFFu) / 65535.0f;
            const float YFraction = static_cast<float>((CandidateSeed >> 16) & 0xFFFFu) / 65535.0f;
            const FVector2D Position = SpatialKeyOrigin + FVector2D(XFraction, YFraction) * SpatialKeySizeCm;
            if (!FKalmalaWorldBounds::Contains(Config, Position)) continue;
            if (Kind == EKalmalaWorldPopulationKind::Wildlife && !IsSafeWildlifeLocation(Config, Position)) continue;
            const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position));
            const EKalmalaBiomeContentKind ContentKind = Kind == EKalmalaWorldPopulationKind::Wildlife
                ? EKalmalaBiomeContentKind::CreatureNiche
                : EKalmalaBiomeContentKind::GatheringSource;
            FName ContentId = Kind == EKalmalaWorldPopulationKind::Hazard
                ? NAME_None
                : FKalmalaBiomeContentContract::GetServerContentId(Biome, ContentKind);
            uint64 PersistentSeed = CandidateSeed;
            if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Biome == EKalmalaBiome::Meadows
                && IsTerrainSafeDiscoveryLocation(Config, Position))
            {
                if (!bAddedMeadowCarrotPatch)
                {
                    // Keep the original candidate position but domain-separate
                    // the source identity from its former biome-source occupant.
                    ContentId = TEXT("meadows-wild-carrot-patch");
                    PersistentSeed = Mix(CandidateSeed ^ 0xA0CA22071019ull);
                    bAddedMeadowCarrotPatch = true;
                }
                else if (!bAddedMeadowPotatoPatch)
                {
                    // Use the next eligible Meadow candidate so the two crop
                    // patches have distinct positions and sparse-save identities.
                    ContentId = TEXT("meadows-wild-potato-patch");
                    PersistentSeed = Mix(CandidateSeed ^ 0xA0B070A71019ull);
                    bAddedMeadowPotatoPatch = true;
                }
                else if (!bAddedMeadowLingonberryShrub)
                {
                    // Give forage its own stable Meadow identity after the two
                    // established wild-crop sources have been assigned.
                    ContentId = TEXT("meadows-wild-lingonberry-shrub");
                    PersistentSeed = Mix(CandidateSeed ^ 0x1A660B3A71019ull);
                    bAddedMeadowLingonberryShrub = true;
                }
            }
            else if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Biome == EKalmalaBiome::ShimmeringLakes
                && !bAddedLakesRutabagaPatch && IsTerrainSafeDiscoveryLocation(Config, Position)
                && FKalmalaEnvironmentalExposureSampler::Sample(Config, Position).bIsShoreline)
            {
                // Keep one hand-gathered crop on dry, gently sloped Lakes land
                // within the shared shoreline probe of generated lake water.
                ContentId = TEXT("lakes-wild-rutabaga-patch");
                PersistentSeed = Mix(CandidateSeed ^ 0xA0B07ABA71019ull);
                bAddedLakesRutabagaPatch = true;
            }
            else if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Biome == EKalmalaBiome::Elderwood
                && !bAddedElderwoodOnionPatch && IsTerrainSafeDiscoveryLocation(Config, Position))
            {
                // Preserve a deterministic, safe Elderwood candidate while
                // separating its sparse identity from the former biome source.
                ContentId = TEXT("elderwood-wild-onion-patch");
                PersistentSeed = Mix(CandidateSeed ^ 0xA0E10A71019ull);
                bAddedElderwoodOnionPatch = true;
            }
            else if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Biome == EKalmalaBiome::Elderwood
                && !bAddedElderwoodChanterellePatch && IsTerrainSafeDiscoveryLocation(Config, Position))
            {
                ContentId = TEXT("elderwood-wild-chanterelle-patch");
                PersistentSeed = Mix(CandidateSeed ^ 0xF0E57C4A71019ull);
                bAddedElderwoodChanterellePatch = true;
            }
            else if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Biome == EKalmalaBiome::Elderwood
                && !bAddedElderwoodCopperVein && IsTerrainSafeDiscoveryLocation(Config, Position))
            {
                ContentId = TEXT("elderwood-copper-vein");
                PersistentSeed = Mix(CandidateSeed ^ 0xC022E271019ull);
                bAddedElderwoodCopperVein = true;
            }
            else if (Kind == EKalmalaWorldPopulationKind::HarvestNode && Biome == EKalmalaBiome::ShimmeringLakes
                && !bAddedLakesTinGravel && IsTerrainSafeDiscoveryLocation(Config, Position)
                && FKalmalaEnvironmentalExposureSampler::Sample(Config, Position).bIsShoreline)
            {
                ContentId = TEXT("lakes-tin-shore-gravel");
                PersistentSeed = Mix(CandidateSeed ^ 0x71A6A271019ull);
                bAddedLakesTinGravel = true;
            }
            Spawns.Add({ Kind, SpatialKey, PersistentSeed, FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position)), ContentId });
        }

        if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
        {
            // Preserve a Tin node in shoreline keys even when their ordinary
            // harvest budget was filled by other biome sources first.
            const bool bHasTinNode = Spawns.ContainsByPredicate([](const FKalmalaWorldPopulationSpawn& Spawn)
            {
                return Spawn.ContentId == TEXT("lakes-tin-shore-gravel");
            });
            constexpr int32 LakesTinCandidateBudget = 32;
            constexpr float MinimumTinSeparation = 250.0f;
            for (int32 CandidateOffset = 0; !bHasTinNode && CandidateOffset < LakesTinCandidateBudget; ++CandidateOffset)
            {
                const int32 SpawnIndex = CandidateBudget + LakesTinCandidateBudget + CandidateOffset;
                const uint64 CandidateSeed = Mix(DeriveSpatialSeed(Config, SpatialKey, Kind) ^ static_cast<uint64>(SpawnIndex + 1));
                const FVector2D Position = SpatialKeyOrigin + FVector2D(
                    static_cast<float>(CandidateSeed & 0xFFFFu) / 65535.0f,
                    static_cast<float>((CandidateSeed >> 16) & 0xFFFFu) / 65535.0f) * SpatialKeySizeCm;
                if (!FKalmalaWorldBounds::Contains(Config, Position) || !IsTerrainSafeDiscoveryLocation(Config, Position)) continue;
                if (FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position)) != EKalmalaBiome::ShimmeringLakes
                    || !FKalmalaEnvironmentalExposureSampler::Sample(Config, Position).bIsShoreline) continue;

                const bool bTooCloseToExistingSpawn = Spawns.ContainsByPredicate([&Position, MinimumTinSeparation](const FKalmalaWorldPopulationSpawn& Spawn)
                {
                    return FVector2D::DistSquared(FVector2D(Spawn.Location.X, Spawn.Location.Y), Position)
                        < MinimumTinSeparation * MinimumTinSeparation;
                });
                if (bTooCloseToExistingSpawn) continue;

                const uint64 PersistentSeed = Mix(CandidateSeed ^ 0x71A6A271019ull);
                Spawns.Add({ Kind, SpatialKey, PersistentSeed,
                    FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position)),
                    TEXT("lakes-tin-shore-gravel") });
                break;
            }

            // Keep the existing harvest budget and source descriptors intact,
            // then add one distinct Stone outcrop for each land biome that
            // lacks a slate source in this key. Extra candidates let Meadow
            // crops and ore nodes coexist with Stone without sharing a position.
            TSet<EKalmalaBiome> BiomesWithStone;
            for (const FKalmalaWorldPopulationSpawn& Spawn : Spawns)
            {
                if (Spawn.ContentId != TEXT("mountains-slate-vein")
                    && Spawn.ContentId != TEXT("allbiomes-stone-outcrop")) continue;
                BiomesWithStone.Add(FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Spawn.Location))));
            }

            constexpr int32 LandBiomeCount = 6;
            constexpr int32 StoneOutcropCandidateBudget = 32;
            constexpr float MinimumOutcropSeparation = 250.0f;
            for (int32 CandidateOffset = 0; CandidateOffset < StoneOutcropCandidateBudget
                && BiomesWithStone.Num() < LandBiomeCount; ++CandidateOffset)
            {
                const int32 SpawnIndex = CandidateBudget + CandidateOffset;
                const uint64 CandidateSeed = Mix(DeriveSpatialSeed(Config, SpatialKey, Kind) ^ static_cast<uint64>(SpawnIndex + 1));
                const FVector2D Position = SpatialKeyOrigin + FVector2D(
                    static_cast<float>(CandidateSeed & 0xFFFFu) / 65535.0f,
                    static_cast<float>((CandidateSeed >> 16) & 0xFFFFu) / 65535.0f) * SpatialKeySizeCm;
                if (!FKalmalaWorldBounds::Contains(Config, Position) || !IsTerrainSafeDiscoveryLocation(Config, Position)) continue;

                const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position));
                if (!FKalmalaBiomeContentContract::IsFirstWaveBiome(Biome) || BiomesWithStone.Contains(Biome)) continue;

                const bool bTooCloseToExistingSpawn = Spawns.ContainsByPredicate([&Position, MinimumOutcropSeparation](const FKalmalaWorldPopulationSpawn& Spawn)
                {
                    return FVector2D::DistSquared(FVector2D(Spawn.Location.X, Spawn.Location.Y), Position)
                        < MinimumOutcropSeparation * MinimumOutcropSeparation;
                });
                if (bTooCloseToExistingSpawn) continue;

                const uint64 PersistentSeed = Mix(CandidateSeed ^ 0x510AE0C0720ull);
                Spawns.Add({ Kind, SpatialKey, PersistentSeed,
                    FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position)),
                    TEXT("allbiomes-stone-outcrop") });
                BiomesWithStone.Add(Biome);
            }
        }
        if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
        {
            // Explicitly approved A9 prerequisite: at most one actual mineral
            // deposit per key, preserving frostmoss and all ordinary candidates.
            for (int32 Attempt = 0; Attempt < 32; ++Attempt)
            {
                const uint64 Seed = Mix(DeriveSpatialSeed(Config, SpatialKey, Kind) ^ uint64(CandidateBudget + 65 + Attempt));
                const FVector2D Position = SpatialKeyOrigin + FVector2D(float(Seed & 0xFFFFu) / 65535.f,
                    float((Seed >> 16) & 0xFFFFu) / 65535.f) * SpatialKeySizeCm;
                if (!FKalmalaWorldBounds::Contains(Config, Position)
                    || FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position)) != EKalmalaBiome::FreezingTundra
                    || !IsTerrainSafeDiscoveryLocation(Config, Position)) continue;
                if (Spawns.ContainsByPredicate([&Position](const auto& Spawn)
                    { return FVector2D::DistSquared(FVector2D(Spawn.Location), Position) < 62500.f; })) continue;
                Spawns.Add({Kind, SpatialKey, Mix(Seed ^ 0xA9F20575A17ull), FVector(Position.X, Position.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(Config, Position)), TEXT("tundra-frost-salt-deposit")});
                break;
            }
        }
        if (bApplyGemstoneReplacements && Kind == EKalmalaWorldPopulationKind::HarvestNode)
        {
            // Replace only after all ordinary suitability, candidate budgets and
            // supplemental-source decisions. Keep exact position and stable seed.
            for (auto& Spawn : Spawns)
            {
                const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Config, FVector2D(Spawn.Location)));
                const FName Selected = FKalmalaGemstoneContract::SelectReplacement(Spawn.ContentId, Biome, Spawn.SpawnSeed);
                if (FKalmalaGemstoneContract::IsAcquisitionEnabled(Selected)) Spawn.ContentId = Selected;
            }
        }
        return Spawns;
    }

    /** Stable server identifier for sparse deltas within a world identity/revision save. */
    static FString GetPersistentSpawnId(const FKalmalaWorldPopulationSpawn& Spawn)
    {
        if (!Spawn.PersistentSpawnIdOverride.IsEmpty())
        {
            return Spawn.PersistentSpawnIdOverride;
        }
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
        const FVector2D SpatialKeyOrigin = FVector2D(SpatialKey) * SpatialKeySizeCm;
        constexpr int32 CandidateBudget = 8;
        for (int32 CandidateOrdinal = 0; CandidateOrdinal < CandidateBudget; ++CandidateOrdinal)
        {
            const uint64 DescriptorSeed = Mix(DeriveDiscoverySeed(Config, SpatialKey, Kind) ^ static_cast<uint64>(CandidateOrdinal + 1));
            const FVector2D Position = SpatialKeyOrigin + FVector2D(
                static_cast<float>(DescriptorSeed & 0xFFFFu) / 65535.0f,
                static_cast<float>((DescriptorSeed >> 16) & 0xFFFFu) / 65535.0f) * SpatialKeySizeCm;
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
#if WITH_DEV_AUTOMATION_TESTS
    friend class FKalmalaA10FreshStartBronzeAndIronTest;
#endif
    friend struct FKalmalaMonsterSpawnTuning;
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
            const FName RareDiscoveryId = FKalmalaBiomeContentContract::GetServerContentId(Biome, EKalmalaBiomeContentKind::RareDiscovery);
            return RareDiscoveryId.IsNone() ? FString() : RareDiscoveryId.ToString();
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

    static bool IsSafeWildlifeLocation(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(Config, Position));
        const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, Position);
        const bool bLakeWater = FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
        if (Biome == EKalmalaBiome::Ocean)
        {
            // The Reef Maw body extends 144 cm below its sea-level origin when
            // idle, so keep candidates above the seabed with a small margin.
            return Ocean.IsWater() && Ocean.WaterDepth >= 150.0f && !bLakeWater;
        }

        // Land wildlife remains on dry, gently traversable terrain. All samples
        // are deterministic functions of the immutable world identity.
        return !Ocean.IsWater() && !bLakeWater
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

struct FKalmalaMonsterSpawnTuning final
{
    static constexpr float RingMinimumDistanceCm = 32.0f * 100.0f;
    static constexpr float RingMaximumDistanceCm = 96.0f * 100.0f;
    static constexpr float PlayerExclusionDistanceCm = 36.0f * 100.0f;
    static constexpr double MaximumLandSlopeDegrees = 32.0;
    static constexpr int32 CommonSpeciesAreaCap = 4;
    static constexpr int32 RareHardEnemyAreaCap = 1;
    static constexpr int32 WolfIndividualAreaCap = 6;
    static constexpr float PopulationKeyCheckIntervalSeconds = 5.0f;
    static constexpr float PopulationRoofClearanceTraceDistanceCm = 2400.0f;
    static constexpr float CampSuppressionRadiusCm = 1800.0f;
    static constexpr float PopulationStationCylinderHalfHeightCm = 1200.0f;
    static constexpr float PopulationKeyPhaseStepSeconds = 0.25f;
    static constexpr int32 PopulationKeyPhaseCount = 20;
    static constexpr float PopulationSchedulerWakeIntervalSeconds = 0.25f;
    static constexpr int32 MaxPopulationKeysPerWake = 4;
    static constexpr int32 MaxPopulationCandidatesPerKey = 4;
    static constexpr int32 MaxPopulationCandidatesPerWake = 16;
    static constexpr uint8 PopulationAdmissionRollRange = 100;
    static constexpr uint8 PopulationAdmissionRollMaxAccepted = 24;
    static constexpr float CountRadiusCm = 96.0f * 100.0f;
    static constexpr double LivingCreatureUnloadCooldownSeconds = 30.0;
    static constexpr float PlayerKeyIntersectionRadiusCm = 96.0f * 100.0f;

    static_assert(FKalmalaWorldPopulationLayout::SpatialKeySizeCm == 64.0f * 100.0f,
        "Population spatial keys must remain 64 m wide.");
    static_assert(RingMinimumDistanceCm > 0.0f && RingMinimumDistanceCm < RingMaximumDistanceCm,
        "Monster spawn ring distances must be positive and ordered.");
    static_assert(PlayerExclusionDistanceCm > RingMinimumDistanceCm
        && PlayerExclusionDistanceCm < RingMaximumDistanceCm,
        "Player exclusion distance must remain inside the spawn ring.");
    static_assert(MaximumLandSlopeDegrees > 0.0f && MaximumLandSlopeDegrees < 90.0f,
        "Land population surfaces must use a positive slope limit below vertical.");
    static_assert(RingMaximumDistanceCm <= CountRadiusCm,
        "Monster spawn count radius must cover the outer ring.");
    static_assert(LivingCreatureUnloadCooldownSeconds == 30.0,
        "Living population slots retain the fixed 30-second session cooldown.");
    static_assert(PlayerKeyIntersectionRadiusCm > 0.0f,
        "Player population-key radius must be positive.");
    static_assert(CommonSpeciesAreaCap > 0 && RareHardEnemyAreaCap > 0 && WolfIndividualAreaCap > 0,
        "Population species area caps must be positive.");
    static_assert(PopulationKeyCheckIntervalSeconds > 0.0f,
        "Monster population checks must use a positive interval.");
    static_assert(PopulationRoofClearanceTraceDistanceCm == 24.0f * 100.0f,
        "Population roof clearance is the fixed 24 m construction-roof rule.");
    static_assert(CampSuppressionRadiusCm == 18.0f * 100.0f
        && PopulationStationCylinderHalfHeightCm == 12.0f * 100.0f,
        "Camp suppression uses an 18 m radius; Workbench and Forge use a 12 m vertical half-height.");
    static_assert(PopulationKeyPhaseStepSeconds > 0.0f && PopulationKeyPhaseCount > 0
        && PopulationKeyPhaseStepSeconds * PopulationKeyPhaseCount == PopulationKeyCheckIntervalSeconds,
        "Monster key staggering must partition one positive check interval.");
    static_assert(PopulationSchedulerWakeIntervalSeconds > 0.0f && MaxPopulationKeysPerWake > 0
        && MaxPopulationCandidatesPerKey > 0 && MaxPopulationCandidatesPerWake > 0
        && MaxPopulationKeysPerWake * MaxPopulationCandidatesPerKey <= MaxPopulationCandidatesPerWake,
        "Population scheduler wake and candidate budgets must be positive and bounded.");
    static_assert(PopulationAdmissionRollRange == 100 && PopulationAdmissionRollMaxAccepted == 24,
        "Population admission remains an unbiased 25 percent roll over 0 through 99.");

    static uint8 GetPopulationSpawnAdmissionRoll(const FKalmalaWorldGenerationConfig& Config,
        const FIntPoint SpatialKey, const FString& PersistentSpawnId, const uint64 CheckOrdinal)
    {
        constexpr uint64 SpawnRollDomain = 0x535041574E524F4Cull;
        uint64 Value = FKalmalaWorldPopulationLayout::DeriveSpatialSeed(
            Config, SpatialKey, EKalmalaWorldPopulationKind::Wildlife) ^ SpawnRollDomain;
        for (int32 CharacterIndex = 0; CharacterIndex < PersistentSpawnId.Len(); ++CharacterIndex)
        {
            Value ^= static_cast<uint32>(PersistentSpawnId[CharacterIndex]);
            Value *= 0x100000001B3ull;
        }
        Value ^= CheckOrdinal * 0x9E3779B185EBCA87ull;
        Value = FKalmalaWorldPopulationLayout::Mix(Value);

        constexpr uint64 RollRange = PopulationAdmissionRollRange;
        const uint64 RejectionThreshold = (uint64(0) - RollRange) % RollRange;
        uint64 RetryOrdinal = 0;
        while (Value < RejectionThreshold)
        {
            Value = FKalmalaWorldPopulationLayout::Mix(
                Value ^ (0x9E3779B97F4A7C15ull + RetryOrdinal++));
        }
        return static_cast<uint8>(Value % RollRange);
    }
};
