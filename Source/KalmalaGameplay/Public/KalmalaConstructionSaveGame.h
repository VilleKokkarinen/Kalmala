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

UENUM()
enum class EKalmalaSmelterOre : uint8
{
    None,
    Copper,
    Tin,
    Iron
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaSmelterState
{
    GENERATED_BODY()

    static constexpr int32 InputOreCapacity = 10;
    static constexpr int32 InputCoalCapacity = 10;
    static constexpr int32 OutputCapacity = 10;
    static constexpr float JobDurationSeconds = 30.0f;

    UPROPERTY(SaveGame) TArray<EKalmalaSmelterOre> InputOreQueue;
    UPROPERTY(SaveGame) int32 InputCoal = 0;
    UPROPERTY(SaveGame) int32 OutputCopper = 0;
    UPROPERTY(SaveGame) int32 OutputTin = 0;
    UPROPERTY(SaveGame) int32 OutputIron = 0;
    UPROPERTY(SaveGame) EKalmalaSmelterOre ActiveJobOre = EKalmalaSmelterOre::None;
    UPROPERTY(SaveGame) float JobRemainingSeconds = 0.0f;

    int32 GetInputOreCount() const { return InputOreQueue.Num() + (ActiveJobOre == EKalmalaSmelterOre::None ? 0 : 1); }
    int32 GetInputCoalCount() const { return InputCoal + (ActiveJobOre == EKalmalaSmelterOre::None ? 0 : 1); }
    int32 GetOutputCount() const { return OutputCopper + OutputTin + OutputIron; }
    int32 GetOutputCount(EKalmalaSmelterOre Ore) const
    {
        switch (Ore)
        {
        case EKalmalaSmelterOre::Copper: return OutputCopper;
        case EKalmalaSmelterOre::Tin: return OutputTin;
        case EKalmalaSmelterOre::Iron: return OutputIron;
        default: return 0;
        }
    }
    bool IsValid() const
    {
        if (InputOreQueue.Num() > InputOreCapacity || GetInputOreCount() > InputOreCapacity
            || InputCoal < 0 || GetInputCoalCount() > InputCoalCapacity
            || OutputCopper < 0 || OutputTin < 0 || OutputIron < 0
            || GetOutputCount() > OutputCapacity
            || !FMath::IsFinite(JobRemainingSeconds)
            || JobRemainingSeconds < 0.0f || JobRemainingSeconds > JobDurationSeconds
            || ((JobRemainingSeconds > 0.0f) != (ActiveJobOre != EKalmalaSmelterOre::None))
            || (JobRemainingSeconds > 0.0f && GetOutputCount() >= OutputCapacity)) return false;
        for (const EKalmalaSmelterOre Ore : InputOreQueue)
            if (Ore != EKalmalaSmelterOre::Copper && Ore != EKalmalaSmelterOre::Tin
                && Ore != EKalmalaSmelterOre::Iron) return false;
        return ActiveJobOre == EKalmalaSmelterOre::None || ActiveJobOre == EKalmalaSmelterOre::Copper
            || ActiveJobOre == EKalmalaSmelterOre::Tin || ActiveJobOre == EKalmalaSmelterOre::Iron;
    }
    bool IsEmpty() const
    {
        return InputOreQueue.IsEmpty() && InputCoal == 0 && GetOutputCount() == 0
            && ActiveJobOre == EKalmalaSmelterOre::None && JobRemainingSeconds <= 0.0f;
    }
    bool TryAddOre(EKalmalaSmelterOre Ore)
    {
        if (!IsValid() || GetInputOreCount() >= InputOreCapacity
            || (Ore != EKalmalaSmelterOre::Copper && Ore != EKalmalaSmelterOre::Tin
                && Ore != EKalmalaSmelterOre::Iron)) return false;
        InputOreQueue.Add(Ore);
        return true;
    }
    bool TryAddCoal()
    {
        if (!IsValid() || GetInputCoalCount() >= InputCoalCapacity) return false;
        ++InputCoal;
        return true;
    }
    bool TryTakeOutput(EKalmalaSmelterOre Ore)
    {
        if (!IsValid()) return false;
        int32* Output = Ore == EKalmalaSmelterOre::Copper ? &OutputCopper
            : Ore == EKalmalaSmelterOre::Tin ? &OutputTin
            : Ore == EKalmalaSmelterOre::Iron ? &OutputIron : nullptr;
        if (!Output || *Output <= 0) return false;
        --*Output;
        return true;
    }
    bool Advance(float DeltaSeconds)
    {
        if (!IsValid() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return false;
        const int32 BeforeOre = InputOreQueue.Num();
        const int32 BeforeCoal = InputCoal;
        const int32 BeforeOutput = GetOutputCount();
        const EKalmalaSmelterOre BeforeJobOre = ActiveJobOre;
        const float BeforeRemaining = JobRemainingSeconds;
        float Budget = DeltaSeconds;
        while (Budget > 0.0f)
        {
            if (JobRemainingSeconds <= 0.0f)
            {
                if (InputOreQueue.IsEmpty() || InputCoal <= 0 || GetOutputCount() >= OutputCapacity) break;
                ActiveJobOre = InputOreQueue[0];
                InputOreQueue.RemoveAt(0, 1, EAllowShrinking::No);
                --InputCoal;
                JobRemainingSeconds = JobDurationSeconds;
            }
            const float Step = FMath::Min(Budget, JobRemainingSeconds);
            JobRemainingSeconds = FMath::Max(0.0f, JobRemainingSeconds - Step);
            Budget -= Step;
            if (JobRemainingSeconds <= 0.0f)
            {
                JobRemainingSeconds = 0.0f;
                if (ActiveJobOre == EKalmalaSmelterOre::Copper) ++OutputCopper;
                else if (ActiveJobOre == EKalmalaSmelterOre::Tin) ++OutputTin;
                else if (ActiveJobOre == EKalmalaSmelterOre::Iron) ++OutputIron;
                ActiveJobOre = EKalmalaSmelterOre::None;
            }
        }
        return BeforeOre != InputOreQueue.Num() || BeforeCoal != InputCoal || BeforeOutput != GetOutputCount()
            || BeforeJobOre != ActiveJobOre || !FMath::IsNearlyEqual(JobRemainingSeconds, BeforeRemaining);
    }
};

UENUM()
enum class EKalmalaGardenCropMode : uint8
{
    None,
    CarrotFood,
    CarrotSeed,
    PotatoFood,
    RutabagaFood,
    RutabagaSeed,
    OnionFood,
    OnionSeed
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaGardenPlotState
{
    GENERATED_BODY()

    static constexpr float CropDurationSeconds = 600.0f;
    static constexpr int32 HarvestYield = 3;
    UPROPERTY(SaveGame) EKalmalaGardenCropMode CropMode = EKalmalaGardenCropMode::None;
    UPROPERTY(SaveGame) float RemainingActiveSeconds = 0.0f;

    static FName GetPlantingPaymentItem(const EKalmalaGardenCropMode Mode)
    {
        switch (Mode)
        {
        case EKalmalaGardenCropMode::CarrotFood: return FName(TEXT("CarrotSeed"));
        case EKalmalaGardenCropMode::CarrotSeed: return FName(TEXT("Carrot"));
        case EKalmalaGardenCropMode::PotatoFood: return FName(TEXT("Potato"));
        case EKalmalaGardenCropMode::RutabagaFood: return FName(TEXT("RutabagaSeed"));
        case EKalmalaGardenCropMode::RutabagaSeed: return FName(TEXT("Rutabaga"));
        case EKalmalaGardenCropMode::OnionFood: return FName(TEXT("OnionSeed"));
        case EKalmalaGardenCropMode::OnionSeed: return FName(TEXT("Onion"));
        default: return NAME_None;
        }
    }
    static FName GetHarvestItem(const EKalmalaGardenCropMode Mode)
    {
        switch (Mode)
        {
        case EKalmalaGardenCropMode::CarrotFood: return FName(TEXT("Carrot"));
        case EKalmalaGardenCropMode::CarrotSeed: return FName(TEXT("CarrotSeed"));
        case EKalmalaGardenCropMode::PotatoFood: return FName(TEXT("Potato"));
        case EKalmalaGardenCropMode::RutabagaFood: return FName(TEXT("Rutabaga"));
        case EKalmalaGardenCropMode::RutabagaSeed: return FName(TEXT("RutabagaSeed"));
        case EKalmalaGardenCropMode::OnionFood: return FName(TEXT("Onion"));
        case EKalmalaGardenCropMode::OnionSeed: return FName(TEXT("OnionSeed"));
        default: return NAME_None;
        }
    }

    bool IsEmpty() const { return CropMode == EKalmalaGardenCropMode::None && RemainingActiveSeconds == 0.0f; }
    bool IsPlanted() const { return CropMode != EKalmalaGardenCropMode::None; }
    bool IsMature() const { return IsPlanted() && RemainingActiveSeconds == 0.0f; }
    bool IsValid() const
    {
        if (!FMath::IsFinite(RemainingActiveSeconds) || RemainingActiveSeconds < 0.0f
            || RemainingActiveSeconds > CropDurationSeconds) return false;
        if (CropMode == EKalmalaGardenCropMode::None) return RemainingActiveSeconds == 0.0f;
        return !GetHarvestItem(CropMode).IsNone();
    }
    bool PlantCrop(const EKalmalaGardenCropMode Mode)
    {
        if (!IsEmpty() || GetHarvestItem(Mode).IsNone()) return false;
        CropMode = Mode;
        RemainingActiveSeconds = CropDurationSeconds;
        return true;
    }
    bool Advance(const float ActiveSeconds)
    {
        if (!IsValid() || !IsPlanted() || RemainingActiveSeconds <= 0.0f
            || !FMath::IsFinite(ActiveSeconds) || ActiveSeconds <= 0.0f) return false;
        const float Before = RemainingActiveSeconds;
        RemainingActiveSeconds = FMath::Max(0.0f, RemainingActiveSeconds - ActiveSeconds);
        return RemainingActiveSeconds != Before;
    }
    bool ClearMatureCrop()
    {
        if (!IsValid() || !IsMature()) return false;
        CropMode = EKalmalaGardenCropMode::None;
        RemainingActiveSeconds = 0.0f;
        return true;
    }
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaStandingLightState
{
    GENERATED_BODY()

    static constexpr float SecondsPerWood = 600.0f;
    static constexpr float FuelCapacitySeconds = 1200.0f;
    UPROPERTY(SaveGame) float RemainingFuelSeconds = 0.0f;

    bool IsEmpty() const { return RemainingFuelSeconds == 0.0f; }
    bool IsValid() const
    {
        return FMath::IsFinite(RemainingFuelSeconds) && RemainingFuelSeconds >= 0.0f
            && RemainingFuelSeconds <= FuelCapacitySeconds;
    }
    bool TryAddWood()
    {
        if (!IsValid() || RemainingFuelSeconds > FuelCapacitySeconds - SecondsPerWood) return false;
        RemainingFuelSeconds += SecondsPerWood;
        return true;
    }
    bool Advance(const float ActiveSeconds)
    {
        if (!IsValid() || RemainingFuelSeconds <= 0.0f
            || !FMath::IsFinite(ActiveSeconds) || ActiveSeconds <= 0.0f) return false;
        const float Before = RemainingFuelSeconds;
        RemainingFuelSeconds = FMath::Max(0.0f, RemainingFuelSeconds - ActiveSeconds);
        return RemainingFuelSeconds != Before;
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
    UPROPERTY(SaveGame) FKalmalaSmelterState Smelter;
    UPROPERTY(SaveGame) FKalmalaGardenPlotState GardenPlot;
    UPROPERTY(SaveGame) FKalmalaStandingLightState StandingLight;
    UPROPERTY(SaveGame) bool bDoorOpen = false;
};

/** Schema-1 construction data model retained by in-memory gameplay tests; current slots use schema 7. */
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

/** Current bounded schema-7 server camp state, including Kiln, Smelter, Garden Plot, door, and Standing Light state. Rejects identity mismatches. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaConstructionSaveGameV2 : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 SchemaVersionValue = 7;
    static constexpr int32 MaxRecords = 128;
    static constexpr int32 MaxStationAttachments = 32;

    void InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorld);
    int32 GetSchemaVersion() const { return SchemaVersion; }
    bool MatchesWorld(const FKalmalaWorldGenerationConfig& InWorld) const;
    static bool IsValidRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddRecord(const FKalmalaConstructionSaveRecord& Record);
    bool AddStationAttachmentRecord(const FKalmalaConstructionSaveRecord& Record);
    /** Rejects deletion of processing records with contents/jobs and Garden Plots with a crop. */
    bool RemoveRecord(const FString& ConstructionId);
    bool UpdateCharcoalKilnRecord(const FString& ConstructionId, const FKalmalaCharcoalKilnState& State);
    bool UpdateSmelterRecord(const FString& ConstructionId, const FKalmalaSmelterState& State);
    bool UpdateGardenPlotRecord(const FString& ConstructionId, const FKalmalaGardenPlotState& State);
    bool UpdateStandingLightRecord(const FString& ConstructionId, const FKalmalaStandingLightState& State);
    bool ReplaceWallRecordWithDoor(const FString& ConstructionId, const FTransform& WallTransform);
    bool UpdateDoorOpenRecord(const FString& ConstructionId, const FTransform& DoorTransform, bool bOpen);
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
