#include "KalmalaM9ExplorationRewardCatalogue.h"

#include "KalmalaBiomeClassifier.h"
#include "KalmalaBiomeExpansionContract.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationLayout.h"

namespace
{
    uint64 MixM9ExplorationSeed(uint64 Value)
    {
        Value ^= Value >> 30;
        Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27;
        Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }

    uint64 DeriveM9ExplorationSeed(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey)
    {
        uint64 Value = Config.WorldSeed ^ 0x4D394C414E445245ull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.X)) * 0x9E3779B185EBCA87ull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.Y)) * 0xC2B2AE3D27D4EB4Full;
        return MixM9ExplorationSeed(Value);
    }

    bool IsSafeLandPosition(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position)
    {
        return FKalmalaWorldBounds::Contains(Config, Position)
            && !FKalmalaOceanSampler::Sample(Config, Position).IsWater()
            && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position)
            && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z
                >= FKalmalaM9ExplorationRewardCatalogue::MinimumWalkableNormalZ;
    }
}

const TArray<FKalmalaM9ExplorationRewardDefinition>& FKalmalaM9ExplorationRewardCatalogue::GetDefinitions()
{
    static const TArray<FKalmalaM9ExplorationRewardDefinition> Definitions =
    {
        { TEXT("lakes-rillworn-marker"), TEXT("three-run-rillstone-marker"), TEXT("Three-Run Rillstone"),
            TEXT("Rillstone: three channels meet"), TEXT("Stone"), 2, true },
        { TEXT("mountains-leeward-grain"), TEXT("leeward-grain-marker"), TEXT("Leeward Grain"),
            TEXT("Leeward Grain: fibre rests in lee"), TEXT("Fibre"), 2, true }
    };
    return Definitions;
}

const FKalmalaM9ExplorationRewardDefinition* FKalmalaM9ExplorationRewardCatalogue::FindDefinition(const FName CandidateId)
{
    if (CandidateId.IsNone()) return nullptr;
    return GetDefinitions().FindByPredicate([CandidateId](const FKalmalaM9ExplorationRewardDefinition& Definition)
    {
        return Definition.CandidateId == CandidateId;
    });
}

bool FKalmalaM9ExplorationRewardCatalogue::IsValidDefinition(
    const FKalmalaM9ExplorationRewardDefinition& Definition, const UKalmalaItemCatalogue* Items)
{
    const FKalmalaM9ExplorationRewardDefinition* Canonical = FindDefinition(Definition.CandidateId);
    return Canonical != nullptr
        && Definition.PresentationId == Canonical->PresentationId
        && Definition.DisplayName == Canonical->DisplayName
        && Definition.ObservationText == Canonical->ObservationText
        && Definition.RewardItemId == Canonical->RewardItemId
        && Definition.RewardQuantity == Canonical->RewardQuantity
        && Definition.bOptional
        && Items != nullptr
        && Items->IsValidStack(Definition.RewardItemId, Definition.RewardQuantity);
}

bool FKalmalaM9ExplorationRewardCatalogue::IsValid(const UKalmalaItemCatalogue* Items)
{
    const TArray<FKalmalaM9ExplorationRewardDefinition>& Definitions = GetDefinitions();
    if (Items == nullptr || Definitions.Num() != MaxDefinitions) return false;

    TSet<FName> CandidateIds;
    TSet<FName> PresentationIds;
    for (const FKalmalaM9ExplorationRewardDefinition& Definition : Definitions)
    {
        if (!IsValidDefinition(Definition, Items)
            || CandidateIds.Contains(Definition.CandidateId)
            || PresentationIds.Contains(Definition.PresentationId)) return false;
        CandidateIds.Add(Definition.CandidateId);
        PresentationIds.Add(Definition.PresentationId);
    }
    return true;
}

FString FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(const FName CandidateId, const FIntPoint SpatialKey)
{
    if (FindDefinition(CandidateId) == nullptr) return FString();
    const FString StableId = FString::Printf(TEXT("land-discovery:m9:%d:%s:%d,%d"),
        StableIdentityVersion, *CandidateId.ToString(), SpatialKey.X, SpatialKey.Y);
    return FKalmalaM7SparseDelta::IsValidStableId(StableId) ? StableId : FString();
}

TArray<FKalmalaM9ExplorationRewardDescriptor> FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(
    const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey)
{
    TArray<FKalmalaM9ExplorationRewardDescriptor> Descriptors;
    if (!Config.IsValid()) return Descriptors;

    const FVector2D KeyCenter = (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f))
        * FKalmalaWorldPopulationLayout::SpatialKeySize;
    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, KeyCenter));
    const FKalmalaM9ExplorationRewardDefinition* Definition = nullptr;
    FKalmalaBiomeDiscoveryCandidate BaseCandidate;
    if (KeyBiome == EKalmalaBiome::ShimmeringLakes)
    {
        Definition = FindDefinition(TEXT("lakes-rillworn-marker"));
        if (Definition == nullptr || !FKalmalaBiomeExpansionContract::TryBuildShimmeringLakeDiscovery(Config, SpatialKey, BaseCandidate))
            return Descriptors;
    }
    else if (KeyBiome == EKalmalaBiome::ThunderMountains)
    {
        Definition = FindDefinition(TEXT("mountains-leeward-grain"));
        if (Definition == nullptr || !FKalmalaBiomeExpansionContract::TryBuildThunderMountainsDiscovery(Config, SpatialKey, BaseCandidate))
            return Descriptors;
    }
    else
    {
        return Descriptors;
    }

    // These cells may already contain a first-wave harvest discovery. Offset
    // the M9 marker from that canonical point while retaining its local cue.
    const uint64 CellSeed = DeriveM9ExplorationSeed(Config, SpatialKey);
    const float StartAngle = static_cast<float>((CellSeed >> 12) % 360ull);
    for (int32 OffsetIndex = 0; OffsetIndex < 24; ++OffsetIndex)
    {
        const float Radius = 180.0f + static_cast<float>(OffsetIndex / 8) * 130.0f;
        const float AngleRadians = FMath::DegreesToRadians(StartAngle + (OffsetIndex % 8) * 45.0f);
        const FVector2D Position = FVector2D(BaseCandidate.Location)
            + FVector2D(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)) * Radius;
        if (!IsSafeLandPosition(Config, Position)) continue;

        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
        if (FKalmalaBiomeClassifier::Classify(Fields) != KeyBiome) continue;
        if (KeyBiome == EKalmalaBiome::ShimmeringLakes)
        {
            const bool bWaterNearby =
                FKalmalaShimmeringLakeSampler::IsWater(Config, Position + FVector2D(ShoreProbeDistanceCm, 0.0f))
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position - FVector2D(ShoreProbeDistanceCm, 0.0f))
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position + FVector2D(0.0f, ShoreProbeDistanceCm))
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position - FVector2D(0.0f, ShoreProbeDistanceCm));
            if (!bWaterNearby) continue;
        }
        else if (Fields.Elevation < 0.80f)
        {
            continue;
        }

        FKalmalaM9ExplorationRewardDescriptor& Descriptor = Descriptors.AddDefaulted_GetRef();
        Descriptor.CandidateId = Definition->CandidateId;
        Descriptor.SpatialKey = SpatialKey;
        Descriptor.Location = FVector(Position.X, Position.Y, FKalmalaTerrainHeightSampler::SampleHeight(Config, Position) + 20.0f);
        break;
    }
    return Descriptors;
}

bool FKalmalaM9ExplorationRewardCatalogue::IsCurrentDescriptor(
    const FKalmalaWorldGenerationConfig& Config, const FKalmalaM9ExplorationRewardDescriptor& Descriptor)
{
    if (Descriptor.CandidateId.IsNone() || Descriptor.Location.ContainsNaN()
        || MakeStableIdentity(Descriptor.CandidateId, Descriptor.SpatialKey).IsEmpty()) return false;

    const TArray<FKalmalaM9ExplorationRewardDescriptor> Expected = BuildDescriptors(Config, Descriptor.SpatialKey);
    return Expected.ContainsByPredicate([&Descriptor](const FKalmalaM9ExplorationRewardDescriptor& Candidate)
    {
        return Candidate.CandidateId == Descriptor.CandidateId
            && Candidate.SpatialKey == Descriptor.SpatialKey
            && Candidate.Location.Equals(Descriptor.Location, 1.0f);
    });
}
