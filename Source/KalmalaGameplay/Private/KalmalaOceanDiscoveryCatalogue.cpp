#include "KalmalaOceanDiscoveryCatalogue.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldPopulationLayout.h"

namespace
{
    uint64 MixOceanDiscoverySeed(uint64 Value)
    {
        Value ^= Value >> 30;
        Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27;
        Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }

    uint64 DeriveOceanDiscoverySeed(const FKalmalaWorldGenerationConfig& Config, const FIntPoint SpatialKey)
    {
        uint64 Value = Config.WorldSeed ^ 0x0CEA4D15C0A71E5Bull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.X)) * 0x9E3779B185EBCA87ull;
        Value ^= static_cast<uint64>(static_cast<uint32>(SpatialKey.Y)) * 0xC2B2AE3D27D4EB4Full;
        return MixOceanDiscoverySeed(Value);
    }
}

const TArray<FKalmalaOceanDiscoveryDefinition>& FKalmalaOceanDiscoveryCatalogue::GetDefinitions()
{
    static const TArray<FKalmalaOceanDiscoveryDefinition> Definitions =
    {
        { TEXT("ocean-driftwood-cache"), TEXT("driftwood-cache-marker"), TEXT("Driftwood cache"), TEXT("Wood"), 2, true },
        { TEXT("ocean-shellbank-shoal"), TEXT("shellbank-shoal-marker"), TEXT("Shellbank shoal"), TEXT("Fibre"), 2, true },
        { TEXT("ocean-stormmark-islet"), TEXT("stormmark-islet-marker"), TEXT("Stormmark islet"), TEXT("Stone"), 1, true }
    };
    return Definitions;
}

const FKalmalaOceanDiscoveryDefinition* FKalmalaOceanDiscoveryCatalogue::FindDefinition(const FName DiscoveryId)
{
    if (DiscoveryId.IsNone())
    {
        return nullptr;
    }

    return GetDefinitions().FindByPredicate([DiscoveryId](const FKalmalaOceanDiscoveryDefinition& Definition)
    {
        return Definition.DiscoveryId == DiscoveryId;
    });
}

bool FKalmalaOceanDiscoveryCatalogue::IsValidDefinition(
    const FKalmalaOceanDiscoveryDefinition& Definition,
    const UKalmalaItemCatalogue* Items)
{
    const FKalmalaOceanDiscoveryDefinition* Canonical = FindDefinition(Definition.DiscoveryId);
    return Canonical != nullptr
        && Definition.PresentationId == Canonical->PresentationId
        && Definition.DisplayName == Canonical->DisplayName
        && Definition.RewardItemId == Canonical->RewardItemId
        && Definition.RewardQuantity == Canonical->RewardQuantity
        && Definition.bOptional
        && Items != nullptr
        && Items->IsValidStack(Definition.RewardItemId, Definition.RewardQuantity);
}

bool FKalmalaOceanDiscoveryCatalogue::IsValid(const UKalmalaItemCatalogue* Items)
{
    const TArray<FKalmalaOceanDiscoveryDefinition>& Definitions = GetDefinitions();
    if (Items == nullptr || Definitions.Num() != MaxDefinitions)
    {
        return false;
    }

    TSet<FName> DiscoveryIds;
    TSet<FName> PresentationIds;
    for (const FKalmalaOceanDiscoveryDefinition& Definition : Definitions)
    {
        if (!IsValidDefinition(Definition, Items)
            || DiscoveryIds.Contains(Definition.DiscoveryId)
            || PresentationIds.Contains(Definition.PresentationId))
        {
            return false;
        }

        DiscoveryIds.Add(Definition.DiscoveryId);
        PresentationIds.Add(Definition.PresentationId);
    }

    return true;
}

FString FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(const FName DiscoveryId, const FIntPoint SpatialKey)
{
    if (FindDefinition(DiscoveryId) == nullptr)
    {
        return FString();
    }

    const FString StableId = FString::Printf(
        TEXT("ocean-discovery:%d:%s:%d,%d"),
        StableIdentityVersion,
        *DiscoveryId.ToString(),
        SpatialKey.X,
        SpatialKey.Y);

    return FKalmalaM7SparseDelta::IsValidStableId(StableId) ? StableId : FString();
}

TArray<FKalmalaOceanDiscoveryDescriptor> FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(
    const FKalmalaWorldGenerationConfig& Config,
    const FIntPoint SpatialKey)
{
    TArray<FKalmalaOceanDiscoveryDescriptor> Descriptors;
    const TArray<FKalmalaOceanDiscoveryDefinition>& Definitions = GetDefinitions();
    if (!Config.IsValid() || Definitions.IsEmpty())
    {
        return Descriptors;
    }

    const uint64 CellSeed = DeriveOceanDiscoverySeed(Config, SpatialKey);
    if (CellSeed % PlacementChanceDenominator != 0)
    {
        return Descriptors;
    }

    const int32 DefinitionIndex = static_cast<int32>((CellSeed >> 8) % static_cast<uint64>(Definitions.Num()));
    const FKalmalaOceanDiscoveryDefinition& Definition = Definitions[DefinitionIndex];
    const FVector2D CellOrigin = FVector2D(SpatialKey) * FKalmalaWorldPopulationLayout::SpatialKeySize;
    for (int32 CandidateIndex = 0; CandidateIndex < PlacementCandidateBudget; ++CandidateIndex)
    {
        const uint64 CandidateSeed = MixOceanDiscoverySeed(
            CellSeed ^ (static_cast<uint64>(CandidateIndex + 1) * 0x165667B19E3779F9ull));
        const float XFraction = static_cast<float>(CandidateSeed & 0xFFFFu) / 65535.0f;
        const float YFraction = static_cast<float>((CandidateSeed >> 16) & 0xFFFFu) / 65535.0f;
        const FVector2D Position = CellOrigin + FVector2D(XFraction, YFraction) * FKalmalaWorldPopulationLayout::SpatialKeySize;
        if (!FKalmalaWorldBounds::Contains(Config, Position))
        {
            continue;
        }

        const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, Position);
        if (!Ocean.IsWater() || Ocean.WaterDepth < MinimumOceanDepthCm)
        {
            continue;
        }

        FKalmalaOceanDiscoveryDescriptor& Descriptor = Descriptors.AddDefaulted_GetRef();
        Descriptor.DiscoveryId = Definition.DiscoveryId;
        Descriptor.SpatialKey = SpatialKey;
        Descriptor.Location = FVector(Position.X, Position.Y, Ocean.TerrainHeight + Ocean.WaterDepth + 20.0f);
        break;
    }

    return Descriptors;
}

bool FKalmalaOceanDiscoveryCatalogue::IsCurrentDescriptor(
    const FKalmalaWorldGenerationConfig& Config,
    const FKalmalaOceanDiscoveryDescriptor& Descriptor)
{
    if (Descriptor.DiscoveryId.IsNone() || Descriptor.Location.ContainsNaN()
        || MakeStableIdentity(Descriptor.DiscoveryId, Descriptor.SpatialKey).IsEmpty())
    {
        return false;
    }

    const TArray<FKalmalaOceanDiscoveryDescriptor> Expected = BuildDescriptors(Config, Descriptor.SpatialKey);
    return Expected.ContainsByPredicate([&Descriptor](const FKalmalaOceanDiscoveryDescriptor& Candidate)
    {
        return Candidate.DiscoveryId == Descriptor.DiscoveryId
            && Candidate.SpatialKey == Descriptor.SpatialKey
            && Candidate.Location.Equals(Descriptor.Location, 1.0f);
    });
}
