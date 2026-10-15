#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaConstructionSaveGame.generated.h"

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaConstructionSaveRecord
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString ConstructionId;
    UPROPERTY(SaveGame) FName KitId;
    UPROPERTY(SaveGame) FTransform Transform;
};

/** Schema-1 construction data model retained by in-memory gameplay tests; current slots use schema 2. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaConstructionSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 CurrentSchemaVersion = 1;
    static constexpr int32 MaxRecords = 128;
    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const;
    static bool IsValidRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddRecord(const FKalmalaConstructionSaveRecord& Record);
    bool RemoveRecord(const FString& ConstructionId);
    int32 GetSchemaVersion() const { return SchemaVersion; }
    const FKalmalaWorldGenerationConfig& GetWorldConfig() const { return WorldConfig; }
    const TArray<FKalmalaConstructionSaveRecord>& GetRecords() const { return Records; }
private:
    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) TArray<FKalmalaConstructionSaveRecord> Records;
};

/** Current bounded schema-2 server camp state. Rejects identity mismatches. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaConstructionSaveGameV2 : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 SchemaVersionValue = 2;
    static constexpr int32 MaxRecords = 128;
    static constexpr int32 MaxStationAttachments = 32;

    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld);
    int32 GetSchemaVersion() const { return SchemaVersion; }
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const;
    static bool IsValidRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddStationAttachmentRecord(const FKalmalaConstructionSaveRecord& Record);
    const TArray<FKalmalaConstructionSaveRecord>& GetRecords() const { return Records; }
    static bool TryBuildWriteCandidate(
        class USaveGame* Existing,
        const FKalmalaWorldGenerationConfig& RequestedWorld,
        UObject* Outer,
        UKalmalaConstructionSaveGameV2*& OutCandidate);
private:
    friend class FKalmalaConstructionSaveGameV2Test;

    UPROPERTY(SaveGame) int32 SchemaVersion = SchemaVersionValue;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) FKalmalaM7SaveIdentity Identity;
    UPROPERTY(SaveGame) TArray<FKalmalaConstructionSaveRecord> Records;
};
