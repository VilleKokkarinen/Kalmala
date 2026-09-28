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

/** Bounded, versioned server camp state. It rejects identity mismatches rather than merging worlds. */
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
    friend class FKalmalaConstructionSaveGameV2Test;

    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) TArray<FKalmalaConstructionSaveRecord> Records;
};

/**
 * Schema-2 construction candidate used for migration and memory verification.
 * Normal slots continue using schema 1 until the complete M9 gate passes.
 */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaConstructionSaveGameV2 : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 SchemaVersionValue = 2;
    static constexpr int32 MaxRecords = 128;
    static constexpr int32 MaxStationAttachments = 32;
    static constexpr int32 MaxDryingLines = 5;

    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld);
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const;
    static bool IsValidRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddStationAttachmentRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddDryingLineRecord(const FKalmalaConstructionSaveRecord& Record);
    const TArray<FKalmalaConstructionSaveRecord>& GetRecords() const { return Records; }
    static bool TryMigrateSchema1(
        const UKalmalaConstructionSaveGame* Legacy,
        const FKalmalaWorldGenerationConfig& RequestedWorld,
        UObject* Outer,
        UKalmalaConstructionSaveGameV2*& OutMigrated);

private:
    friend class FKalmalaConstructionSaveGameV2Test;

    UPROPERTY(SaveGame) int32 SchemaVersion = SchemaVersionValue;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) FKalmalaM7SaveIdentity Identity;
    UPROPERTY(SaveGame) TArray<FKalmalaConstructionSaveRecord> Records;
};
