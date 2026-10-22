#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationSaveGame.generated.h"

USTRUCT()
struct KALMALAWORLD_API FKalmalaPendingWildlifeReward
{
    GENERATED_BODY()

    // Rat claims keep the raw ID; named species prefix stable IDs by reward type.
    // Peat Husk IDs also retain its one-time Peat Heart roll outcome.
    UPROPERTY(SaveGame) FString PersistentSpawnId;
    UPROPERTY(SaveGame) FString PlayerIdentity;
};

/** Versioned sparse server save data for generated population deltas. */
UCLASS()
class KALMALAWORLD_API UKalmalaWorldPopulationSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 MaxPendingWildlifeRewards = 4096;
    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const;
    bool IsHarvested(const FString& PersistentSpawnId) const;
    void MarkHarvested(const FString& PersistentSpawnId);
    bool IsDefeated(const FString& PersistentSpawnId) const;
    void MarkDefeated(const FString& PersistentSpawnId);
    bool TryQueuePendingWildlifeReward(const FString& RewardClaimId, const FString& PlayerIdentity);
    bool TryQueuePendingBadgerReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeBadgerPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetBadgerSpawnIdFromPendingRewardClaimId(const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingMoonBearReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeMoonBearPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetMoonBearSpawnIdFromPendingRewardClaimId(const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingViperReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeViperPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetViperSpawnIdFromPendingRewardClaimId(const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingGiantSnakeReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeGiantSnakePendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetGiantSnakeSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingDeathAdderReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeDeathAdderPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetDeathAdderSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingPeatHuskReward(
        const FString& PersistentSpawnId, const FString& PlayerIdentity, bool bHasPeatHeart);
    static FString MakePeatHuskPendingRewardClaimId(const FString& PersistentSpawnId, bool bHasPeatHeart);
    static bool TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasPeatHeart);
    bool TryQueuePendingWolfReward(
        const FString& PersistentSpawnId, const FString& PlayerIdentity, bool bHasWolfFang);
    static FString MakeWolfPendingRewardClaimId(const FString& PersistentSpawnId, bool bHasWolfFang);
    static bool TryGetWolfSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasWolfFang);
    bool TryQueuePendingLynxReward(
        const FString& PersistentSpawnId, const FString& PlayerIdentity, bool bHasLynxPelt);
    static FString MakeLynxPendingRewardClaimId(const FString& PersistentSpawnId, bool bHasLynxPelt);
    static bool TryGetLynxSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId, bool& OutHasLynxPelt);
    bool TryQueuePendingPolarBearReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakePolarBearPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetPolarBearSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingHornedGoatReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeHornedGoatPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetHornedGoatSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingThunderSentinelReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeThunderSentinelPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId);
    bool TryQueuePendingReefMawReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    static FString MakeReefMawPendingRewardClaimId(const FString& PersistentSpawnId);
    static bool TryGetReefMawSpawnIdFromPendingRewardClaimId(
        const FString& ClaimId, FString& OutPersistentSpawnId);
    bool RemovePendingWildlifeReward(const FString& RewardClaimId);
    const TArray<FKalmalaPendingWildlifeReward>& GetPendingWildlifeRewards() const { return PendingWildlifeRewards; }

private:
    UPROPERTY(SaveGame)
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame)
    FKalmalaWorldGenerationConfig WorldConfig;

    UPROPERTY(SaveGame)
    TSet<FString> HarvestedSpawnIds;

    /** Server-authored deltas for generated wildlife or hazards removed by gameplay. */
    UPROPERTY(SaveGame)
    TSet<FString> DefeatedSpawnIds;

    /** Credited wildlife rewards retained until their owner's pack can accept the full drop. */
    UPROPERTY(SaveGame)
    TArray<FKalmalaPendingWildlifeReward> PendingWildlifeRewards;
};
