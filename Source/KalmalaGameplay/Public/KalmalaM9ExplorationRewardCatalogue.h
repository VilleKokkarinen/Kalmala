#pragma once

#include "CoreMinimal.h"

class UKalmalaItemCatalogue;
struct FKalmalaWorldGenerationConfig;

/** Server-derived, optional M9 land discovery. Clients only receive the presentation identity. */
struct KALMALAGAMEPLAY_API FKalmalaM9ExplorationRewardDescriptor
{
    FName CandidateId = NAME_None;
    FIntPoint SpatialKey = FIntPoint::ZeroValue;
    FVector Location = FVector::ZeroVector;
};

/** Canonical, bounded reward and observation text for one M9 land discovery. */
struct KALMALAGAMEPLAY_API FKalmalaM9ExplorationRewardDefinition
{
    FName CandidateId = NAME_None;
    FName PresentationId = NAME_None;
    FString DisplayName;
    FString ObservationText;
    FName RewardItemId = NAME_None;
    int32 RewardQuantity = 0;
    bool bOptional = false;
};

/** Pure deterministic placement and sparse identity contract for M9 land discoveries. */
struct KALMALAGAMEPLAY_API FKalmalaM9ExplorationRewardCatalogue
{
    static constexpr int32 StableIdentityVersion = 1;
    static constexpr int32 MaxDefinitions = 2;
    static constexpr float MinimumWalkableNormalZ = 0.80f;
    static constexpr float ShoreProbeDistanceCm = 350.0f;

    static const TArray<FKalmalaM9ExplorationRewardDefinition>& GetDefinitions();
    static const FKalmalaM9ExplorationRewardDefinition* FindDefinition(FName CandidateId);
    static bool IsValidDefinition(const FKalmalaM9ExplorationRewardDefinition& Definition, const UKalmalaItemCatalogue* Items);
    static bool IsValid(const UKalmalaItemCatalogue* Items);
    static FString MakeStableIdentity(FName CandidateId, FIntPoint SpatialKey);
    static TArray<FKalmalaM9ExplorationRewardDescriptor> BuildDescriptors(
        const FKalmalaWorldGenerationConfig& Config, FIntPoint SpatialKey);
    static bool IsCurrentDescriptor(
        const FKalmalaWorldGenerationConfig& Config,
        const FKalmalaM9ExplorationRewardDescriptor& Descriptor);
};
