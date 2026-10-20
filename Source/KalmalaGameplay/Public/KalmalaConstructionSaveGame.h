#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaConstructionSaveGame.generated.h"

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaCharcoalKilnState
{
    GENERATED_BODY()

    static constexpr int32 InputCapacity = 10;
    static constexpr int32 OutputCapacity = 10;
    static constexpr float JobDurationSeconds = 30.0f;

    UPROPERTY(SaveGame) int32 InputWood = 0;
    UPROPERTY(SaveGame) int32 OutputCoal = 0;
    /** Active-game seconds remaining for the already-reserved Wood unit. */
    UPROPERTY(SaveGame) float JobRemainingSeconds = 0.0f;

    bool IsValid() const
    {
        return InputWood >= 0 && InputWood <= InputCapacity
            && OutputCoal >= 0 && OutputCoal <= OutputCapacity
            && FMath::IsFinite(JobRemainingSeconds)
            && JobRemainingSeconds >= 0.0f && JobRemainingSeconds <= JobDurationSeconds
            && (JobRemainingSeconds <= 0.0f || OutputCoal < OutputCapacity);
    }

    bool IsEmpty() const
    {
        return InputWood == 0 && OutputCoal == 0 && JobRemainingSeconds <= 0.0f;
    }

    bool TryAddWood()
    {
        if (!IsValid() || InputWood >= InputCapacity) return false;
        ++InputWood;
        return true;
    }

    bool TryTakeCoal()
    {
        if (!IsValid() || OutputCoal <= 0) return false;
        --OutputCoal;
        return true;
    }

    bool Advance(const float DeltaSeconds)
    {
        if (!IsValid() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return false;
        const int32 BeforeInput = InputWood;
        const int32 BeforeOutput = OutputCoal;
        const float BeforeRemaining = JobRemainingSeconds;
        float Budget = DeltaSeconds;
        while (Budget > 0.0f)
        {
            if (JobRemainingSeconds <= 0.0f)
            {
                if (InputWood <= 0 || OutputCoal >= OutputCapacity) break;
                --InputWood; // Reserve one Wood when the job starts.
                JobRemainingSeconds = JobDurationSeconds;
            }
            const float Step = FMath::Min(Budget, JobRemainingSeconds);
            JobRemainingSeconds = FMath::Max(0.0f, JobRemainingSeconds - Step);
            Budget -= Step;
            if (JobRemainingSeconds <= 0.0f)
            {
                JobRemainingSeconds = 0.0f;
                if (OutputCoal >= OutputCapacity) break;
                ++OutputCoal;
            }
        }
        return InputWood != BeforeInput || OutputCoal != BeforeOutput
            || !FMath::IsNearlyEqual(JobRemainingSeconds, BeforeRemaining);
    }
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaConstructionSaveRecord
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString ConstructionId;
    UPROPERTY(SaveGame) FName KitId;
    UPROPERTY(SaveGame) FTransform Transform;
    UPROPERTY(SaveGame) FKalmalaCharcoalKilnState CharcoalKiln;
};

/** Schema-1 construction data model retained by in-memory gameplay tests; current slots use schema 3. */
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

/** Current bounded schema-3 server camp state, including kiln processing. Rejects identity mismatches. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaConstructionSaveGameV2 : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 SchemaVersionValue = 3;
    static constexpr int32 MaxRecords = 128;
    static constexpr int32 MaxStationAttachments = 32;

    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld);
    int32 GetSchemaVersion() const { return SchemaVersion; }
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const;
    static bool IsValidRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddStationAttachmentRecord(const FKalmalaConstructionSaveRecord& Record);
    /** Rejects deletion of a Kiln record while it contains Wood, Coal, or active job time. */
    bool RemoveRecord(const FString& ConstructionId);
    bool UpdateCharcoalKilnRecord(const FString& ConstructionId, const FKalmalaCharcoalKilnState& State);
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
