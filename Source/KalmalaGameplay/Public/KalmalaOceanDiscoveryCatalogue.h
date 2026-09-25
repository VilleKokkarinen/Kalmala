#pragma once

#include "CoreMinimal.h"

class UKalmalaItemCatalogue;

/** One optional, route-free reward attached to an original ocean discovery kind. */
struct KALMALAGAMEPLAY_API FKalmalaOceanDiscoveryDefinition
{
    FName DiscoveryId = NAME_None;
    FName PresentationId = NAME_None;
    FString DisplayName;
    FName RewardItemId = NAME_None;
    int32 RewardQuantity = 0;
    bool bOptional = false;
};

/**
 * Bounded server-selected sea-discovery content and stable sparse identity
 * formatting. This catalogue grants nothing and carries no route or position.
 */
struct KALMALAGAMEPLAY_API FKalmalaOceanDiscoveryCatalogue
{
    static constexpr int32 StableIdentityVersion = 1;
    static constexpr int32 MaxDefinitions = 3;

    static const TArray<FKalmalaOceanDiscoveryDefinition>& GetDefinitions();
    static const FKalmalaOceanDiscoveryDefinition* FindDefinition(FName DiscoveryId);
    static bool IsValidDefinition(const FKalmalaOceanDiscoveryDefinition& Definition, const UKalmalaItemCatalogue* Items);
    static bool IsValid(const UKalmalaItemCatalogue* Items);

    /** World seed and generator revision are scoped by the enclosing save identity. */
    static FString MakeStableIdentity(FName DiscoveryId, FIntPoint SpatialKey);
};
