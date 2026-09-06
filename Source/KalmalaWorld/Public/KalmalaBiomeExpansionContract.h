#pragma once

#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldPopulationLayout.h"

/** Shared pure rules for incremental land-biome slices. Only the server may turn a candidate into replicated gameplay content or a persistent discovery. */
struct KALMALAWORLD_API FKalmalaBiomeExpansionProfile
{
    float TerrainFeatureStrength = 0.0f;
    float WildlifeBudgetMultiplier = 1.0f;
    float HarvestBudgetMultiplier = 1.0f;
    float HazardBudgetMultiplier = 1.0f;
    float GroundWetnessMultiplier = 1.0f;
    float WindExposureMultiplier = 1.0f;
    float NaturalCoverMultiplier = 1.0f;
};

struct KALMALAWORLD_API FKalmalaBiomeDiscoveryCandidate
{
    EKalmalaBiome Biome = EKalmalaBiome::Meadows;
    FIntPoint SpatialKey = FIntPoint::ZeroValue;
    uint64 CandidateSeed = 0;
    FString StableId;
    FVector Location = FVector::ZeroVector;
};

struct KALMALAWORLD_API FKalmalaBiomeFeatureInspection
{
    EKalmalaBiome Biome = EKalmalaBiome::Meadows;
    bool bHasClassifierSeam = false;
    FKalmalaBiomeExpansionProfile Profile;
    FKalmalaBiomeDiscoveryCandidate DiscoveryCandidate;
};

struct KALMALAWORLD_API FKalmalaBiomeExpansionContract
{
    static constexpr float InspectionSeamDistance = 250.0f;

    static FKalmalaBiomeExpansionProfile GetProfile(const EKalmalaBiome Biome)
    {
        switch (Biome)
        {
        case EKalmalaBiome::ShimmeringLakes: return { 0.60f, 0.75f, 1.10f, 1.00f, 1.25f, 0.85f, 0.90f };
        case EKalmalaBiome::Elderwood: return { 0.70f, 1.20f, 1.15f, 0.70f, 0.95f, 0.65f, 1.35f };
        case EKalmalaBiome::MossyMire: return { 0.65f, 0.70f, 1.10f, 1.25f, 1.35f, 0.80f, 1.05f };
        case EKalmalaBiome::FreezingTundra: return { 0.55f, 0.65f, 0.70f, 0.85f, 0.80f, 1.30f, 0.55f };
        case EKalmalaBiome::ThunderMountains: return { 0.85f, 0.70f, 0.85f, 1.15f, 0.75f, 1.40f, 0.65f };
        case EKalmalaBiome::Ocean: return { 0.00f, 0.00f, 0.00f, 0.00f, 1.00f, 1.00f, 0.00f };
        case EKalmalaBiome::Meadows:
        default: return { 0.35f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f };
        }
    }

    static int32 ApplyPopulationBudget(const int32 BaseBudget, const EKalmalaWorldPopulationKind Kind, const EKalmalaBiome Biome)
    {
        const FKalmalaBiomeExpansionProfile Profile = GetProfile(Biome);
        const float Multiplier = Kind == EKalmalaWorldPopulationKind::Wildlife ? Profile.WildlifeBudgetMultiplier : Kind == EKalmalaWorldPopulationKind::HarvestNode ? Profile.HarvestBudgetMultiplier : Profile.HazardBudgetMultiplier;
        return FMath::Max(0, FMath::RoundToInt(BaseBudget * Multiplier));
    }

    static FKalmalaEnvironmentalExposureSample ApplyExposureModifiers(FKalmalaEnvironmentalExposureSample Sample, const EKalmalaBiome Biome)
    {
        const FKalmalaBiomeExpansionProfile Profile = GetProfile(Biome);
        Sample.GroundWetness = FMath::Clamp(Sample.GroundWetness * Profile.GroundWetnessMultiplier, 0.0f, 1.0f);
        Sample.WindExposure = FMath::Clamp(Sample.WindExposure * Profile.WindExposureMultiplier, 0.0f, 1.0f);
        Sample.NaturalCover = FMath::Clamp(Sample.NaturalCover * Profile.NaturalCoverMultiplier, 0.0f, 1.0f);
        return Sample;
    }

    static FKalmalaBiomeDiscoveryCandidate BuildDiscoveryCandidate(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, const EKalmalaBiome Biome)
    {
        const uint64 CandidateSeed = Mix(FKalmalaWorldPopulationLayout::DeriveSpatialSeed(Config, SpatialKey, EKalmalaWorldPopulationKind::Hazard) ^ (static_cast<uint64>(Biome) + 1ull));
        const FVector2D Origin = FVector2D(SpatialKey) * FKalmalaWorldPopulationLayout::SpatialKeySize;
        const FVector2D Position = Origin + FVector2D(static_cast<float>(CandidateSeed & 0xFFFFu) / 65535.0f, static_cast<float>((CandidateSeed >> 16) & 0xFFFFu) / 65535.0f) * FKalmalaWorldPopulationLayout::SpatialKeySize;
        FKalmalaBiomeDiscoveryCandidate Candidate;
        Candidate.Biome = Biome;
        Candidate.SpatialKey = SpatialKey;
        Candidate.CandidateSeed = CandidateSeed;
        Candidate.StableId = FString::Printf(TEXT("Discovery/%d/%d/%d/%llu"), static_cast<uint8>(Biome), SpatialKey.X, SpatialKey.Y, CandidateSeed);
        Candidate.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position));
        return Candidate;
    }

    static FKalmalaBiomeFeatureInspection Inspect(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position));
        FKalmalaBiomeFeatureInspection Inspection;
        Inspection.Biome = Biome;
        Inspection.Profile = GetProfile(Biome);
        Inspection.DiscoveryCandidate = BuildDiscoveryCandidate(Config, FKalmalaWorldPopulationLayout::GetSpatialKey(Position), Biome);
        for (const FVector2D Offset : { FVector2D(InspectionSeamDistance, 0.0f), FVector2D(-InspectionSeamDistance, 0.0f), FVector2D(0.0f, InspectionSeamDistance), FVector2D(0.0f, -InspectionSeamDistance) })
        {
            Inspection.bHasClassifierSeam |= FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position + Offset)) != Biome;
        }
        return Inspection;
    }

    /** Finds one dry, saturated lake-edge location in a spatial key. The result is a server input, not a client-visible candidate list. */
    static bool TryBuildShimmeringLakeDiscovery(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, FKalmalaBiomeDiscoveryCandidate& OutCandidate)
    {
        const FKalmalaBiomeDiscoveryCandidate BaseCandidate = BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::ShimmeringLakes);
        constexpr float ShoreProbeDistance = 350.0f;
        for (int32 Attempt = 0; Attempt < 16; ++Attempt)
        {
            const uint64 AttemptSeed = Mix(BaseCandidate.CandidateSeed ^ static_cast<uint64>(Attempt + 1) * 0x9E3779B185EBCA87ull);
            const FVector2D Origin = FVector2D(SpatialKey) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FVector2D Position = Origin + FVector2D(
                static_cast<float>(AttemptSeed & 0xFFFFu) / 65535.0f,
                static_cast<float>((AttemptSeed >> 16) & 0xFFFFu) / 65535.0f) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const bool bLakeBiome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, Position)) == EKalmalaBiome::ShimmeringLakes;
            const bool bWaterNearby = FKalmalaShimmeringLakeSampler::IsWater(Config, Position + FVector2D(ShoreProbeDistance, 0.0f))
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position - FVector2D(ShoreProbeDistance, 0.0f))
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position + FVector2D(0.0f, ShoreProbeDistance))
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position - FVector2D(0.0f, ShoreProbeDistance));
            if (bLakeBiome && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position) && bWaterNearby)
            {
                OutCandidate = BaseCandidate;
                OutCandidate.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position) + 20.0f);
                return true;
            }
        }
        return false;
    }

    /** Finds one gently sloped, lower-flora Elderwood clearing. It is a server input and does not form a trail or reserved camp. */
    static bool TryBuildElderwoodDiscovery(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, FKalmalaBiomeDiscoveryCandidate& OutCandidate)
    {
        const FKalmalaBiomeDiscoveryCandidate BaseCandidate = BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::Elderwood);
        for (int32 Attempt = 0; Attempt < 24; ++Attempt)
        {
            const uint64 AttemptSeed = Mix(BaseCandidate.CandidateSeed ^ static_cast<uint64>(Attempt + 1) * 0xD1B54A32D192ED03ull);
            const FVector2D Origin = FVector2D(SpatialKey) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FVector2D Position = Origin + FVector2D(
                static_cast<float>(AttemptSeed & 0xFFFFu) / 65535.0f,
                static_cast<float>((AttemptSeed >> 16) & 0xFFFFu) / 65535.0f) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
            if (FKalmalaBiomeClassifier::Classify(Fields) == EKalmalaBiome::Elderwood
                && Fields.Flora <= 0.76f
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.86f)
            {
                OutCandidate = BaseCandidate;
                OutCandidate.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position) + 20.0f);
                return true;
            }
        }
        return false;
    }

    /** Finds one relatively dry, gently sloped Mire hummock. It remains an optional server-owned discovery, never a crossing or route. */
    static bool TryBuildMossyMireDiscovery(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, FKalmalaBiomeDiscoveryCandidate& OutCandidate)
    {
        const FKalmalaBiomeDiscoveryCandidate BaseCandidate = BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::MossyMire);
        for (int32 Attempt = 0; Attempt < 32; ++Attempt)
        {
            const uint64 AttemptSeed = Mix(BaseCandidate.CandidateSeed ^ static_cast<uint64>(Attempt + 1) * 0x94D049BB133111EBull);
            const FVector2D Origin = FVector2D(SpatialKey) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FVector2D Position = Origin + FVector2D(
                static_cast<float>(AttemptSeed & 0xFFFFu) / 65535.0f,
                static_cast<float>((AttemptSeed >> 16) & 0xFFFFu) / 65535.0f) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
            const FKalmalaEnvironmentalExposureSample Exposure = FKalmalaEnvironmentalExposureSampler::Sample(Config, Position);
            if (FKalmalaBiomeClassifier::Classify(Fields) == EKalmalaBiome::MossyMire
                && Exposure.GroundWetness <= 0.72f
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.84f)
            {
                OutCandidate = BaseCandidate;
                OutCandidate.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position) + 20.0f);
                return true;
            }
        }
        return false;
    }

    /** Finds one exposed, gently rolling Tundra location. It stays optional and leaves enclosed shelter as player-built preparation. */
    static bool TryBuildFreezingTundraDiscovery(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey, FKalmalaBiomeDiscoveryCandidate& OutCandidate)
    {
        const FKalmalaBiomeDiscoveryCandidate BaseCandidate = BuildDiscoveryCandidate(Config, SpatialKey, EKalmalaBiome::FreezingTundra);
        for (int32 Attempt = 0; Attempt < 32; ++Attempt)
        {
            const uint64 AttemptSeed = Mix(BaseCandidate.CandidateSeed ^ static_cast<uint64>(Attempt + 1) * 0xBF58476D1CE4E5B9ull);
            const FVector2D Origin = FVector2D(SpatialKey) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FVector2D Position = Origin + FVector2D(
                static_cast<float>(AttemptSeed & 0xFFFFu) / 65535.0f,
                static_cast<float>((AttemptSeed >> 16) & 0xFFFFu) / 65535.0f) * FKalmalaWorldPopulationLayout::SpatialKeySize;
            const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
            if (FKalmalaBiomeClassifier::Classify(Fields) == EKalmalaBiome::FreezingTundra
                && Fields.Elevation >= 0.35f
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.88f)
            {
                OutCandidate = BaseCandidate;
                OutCandidate.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position) + 20.0f);
                return true;
            }
        }
        return false;
    }

private:
    static uint64 Mix(uint64 Value)
    {
        Value ^= Value >> 30; Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27; Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }
};
