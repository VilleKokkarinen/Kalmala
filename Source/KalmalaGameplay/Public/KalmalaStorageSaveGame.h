#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaStorageSaveGame.generated.h"

USTRUCT()
struct FKalmalaStorageSaveRecord
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString ConstructionId;
    UPROPERTY(SaveGame) TArray<FKalmalaInventoryStack> Stacks;
};

/** Separate server-only container; construction and player save schemas are unchanged. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaStorageSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 CurrentSchemaVersion = 1;
    static constexpr int32 MaxRecords = 128;
    void InitializeForWorld(const FKalmalaWorldGenerationConfig& Config);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& Config) const;
    static bool IsValidConstructionId(const FString& Id);
    static bool IsValidStacks(const TArray<FKalmalaInventoryStack>& Stacks);
    const FKalmalaStorageSaveRecord* FindRecord(const FString& Id) const;
    bool UpsertRecord(const FString& Id, const TArray<FKalmalaInventoryStack>& Stacks);
    static FString MakeSlotName(const FKalmalaWorldGenerationConfig& Config);
private:
    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) TArray<FKalmalaStorageSaveRecord> Records;
};
