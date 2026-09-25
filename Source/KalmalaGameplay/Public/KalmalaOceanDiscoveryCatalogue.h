#pragma once

#include "CoreMinimal.h"

class UKalmalaItemCatalogue;
struct FKalmalaWorldGenerationConfig;

/** Server-derived optional sea discovery candidate. Clients only receive its presentation ID. */
struct KALMALAGAMEPLAY_API FKalmalaOceanDiscoveryDescriptor
{
    FName DiscoveryId = NAME_None;
    FIntPoint SpatialKey = FIntPoint::ZeroValue;
    FVector Location = FVector::ZeroVector;
};

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
    static constexpr int32 PlacementChanceDenominator = 8;
    static constexpr int32 PlacementCandidateBudget = 8;
    static constexpr float MinimumOceanDepthCm = 100.0f;

    static const TArray<FKalmalaOceanDiscoveryDefinition>& GetDefinitions();
    static const FKalmalaOceanDiscoveryDefinition* FindDefinition(FName DiscoveryId);
    static bool IsValidDefinition(const FKalmalaOceanDiscoveryDefinition& Definition, const UKalmalaItemCatalogue* Items);
    static bool IsValid(const UKalmalaItemCatalogue* Items);

    /** World seed and generator revision are scoped by the enclosing save identity. */
    static FString MakeStableIdentity(FName DiscoveryId, FIntPoint SpatialKey);
    static TArray<FKalmalaOceanDiscoveryDescriptor> BuildDescriptors(
        const FKalmalaWorldGenerationConfig& Config, FIntPoint SpatialKey);
    static bool IsCurrentDescriptor(
        const FKalmalaWorldGenerationConfig& Config,
        const FKalmalaOceanDiscoveryDescriptor& Descriptor);
};
