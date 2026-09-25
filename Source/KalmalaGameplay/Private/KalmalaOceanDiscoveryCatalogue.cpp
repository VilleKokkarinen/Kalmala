#include "KalmalaOceanDiscoveryCatalogue.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"

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
