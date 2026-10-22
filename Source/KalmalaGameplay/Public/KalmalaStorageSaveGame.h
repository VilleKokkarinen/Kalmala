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
    UPROPERTY(SaveGame) FName KitId = NAME_None;
    UPROPERTY(SaveGame) TArray<FKalmalaInventoryStack> Stacks;
};

/** Server-only storage container; schema 3 binds each capacity to its placed buildable identity. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaStorageSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 CurrentSchemaVersion = 3;
    static constexpr int32 MaxRecords = 128;
    static constexpr int32 MaxStorageSlots = 16;
    static constexpr int32 MaxReinforcedStorageSlots = 24;
    void InitializeForWorld(const FKalmalaWorldGenerationConfig& Config);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& Config) const;
    static bool IsValidConstructionId(const FString& Id);
    static bool IsValidStacks(const TArray<FKalmalaInventoryStack>& Stacks);
    static bool IsValidStacks(const TArray<FKalmalaInventoryStack>& Stacks, int32 Capacity);
    static int32 GetCapacityForKit(FName KitId);
    const FKalmalaStorageSaveRecord* FindRecord(const FString& Id) const;
    bool UpsertRecord(const FString& Id, FName KitId, const TArray<FKalmalaInventoryStack>& Stacks);
    static FString MakeSlotName(const FKalmalaWorldGenerationConfig& Config);
private:
    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) TArray<FKalmalaStorageSaveRecord> Records;
};
