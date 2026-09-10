#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
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
    const TArray<FKalmalaConstructionSaveRecord>& GetRecords() const { return Records; }
private:
    UPROPERTY(SaveGame) int32 SchemaVersion = CurrentSchemaVersion;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) TArray<FKalmalaConstructionSaveRecord> Records;
};
