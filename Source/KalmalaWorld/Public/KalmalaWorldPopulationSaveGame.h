#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationSaveGame.generated.h"

/** Versioned sparse server save data for generated population deltas. */
UCLASS()
class KALMALAWORLD_API UKalmalaWorldPopulationSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const;
    bool IsHarvested(const FString& PersistentSpawnId) const;
    void MarkHarvested(const FString& PersistentSpawnId);

private:
    UPROPERTY(SaveGame)
    int32 SchemaVersion = 1;

    UPROPERTY(SaveGame)
    FKalmalaWorldGenerationConfig WorldConfig;

    UPROPERTY(SaveGame)
    TSet<FString> HarvestedSpawnIds;
};
