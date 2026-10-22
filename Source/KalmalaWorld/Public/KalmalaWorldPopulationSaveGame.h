#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationSaveGame.generated.h"

USTRUCT()
struct KALMALAWORLD_API FKalmalaPendingWildlifeReward
{
    GENERATED_BODY()

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
    bool TryQueuePendingWildlifeReward(const FString& PersistentSpawnId, const FString& PlayerIdentity);
    bool RemovePendingWildlifeReward(const FString& PersistentSpawnId);
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

    /** Credited Rat rewards retained until their owner's pack can accept the full drop. */
    UPROPERTY(SaveGame)
    TArray<FKalmalaPendingWildlifeReward> PendingWildlifeRewards;
};
