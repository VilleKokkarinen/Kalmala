#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaInteractionGrid.h"
#include "KalmalaGameMode.generated.h"

/**
 * Server-authoritative rules for a Kalmala session.
 * Gameplay systems are added in later milestones.
 */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AKalmalaGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    bool CanPersistConstruction(FName KitId, const FTransform& Transform) const;
    bool PersistConstruction(class AKalmalaConstructionActor* Construction);
    bool ReadStorage(const class AKalmalaConstructionActor* Construction, TArray<FKalmalaInventoryStack>& Out) const;
    bool PersistStorage(const class AKalmalaConstructionActor* Construction, const TArray<FKalmalaInventoryStack>& Stacks);

private:
    void ActivateTerrainPatch(const FIntPoint& PatchCoordinate);
    void ActivateTerrainPatchNeighborhood(const FVector2D& WorldPosition);
    void RefreshTerrainPatchNeighborhoods();
    void ActivatePopulationKey(const FIntPoint& SpatialKey);
    void RecordHarvestedSpawn(const FString& PersistentSpawnId);
    void RecordDefeatedSpawn(const FString& PersistentSpawnId);
    void ConfigureTraversalTest();
    void DriveTraversalTest();
    void ReportWorldProfileIfReady();
    void RunReconnectVerification(APawn* ServerPawn);
    void RestorePersistedConstruction();
    void LogExposureInspection(const AActor* Occupant) const;
    void LogCampConditionInspection(const AActor* Occupant) const;
    void LogBiomeFeatureInspection(const AActor* Occupant) const;
    void UpdatePlayerExposure(float DeltaSeconds);
    void UpdateInteractionGrid();
    void DriveCampChoiceTest();
    void DriveRainVerticalSliceTest();
    void DriveCombatPeerTest();
    float CampChoiceStartTime = -1.0f;
    int32 CampChoiceStage = 0;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> CampChoicePlayers;
    TArray<float> CampChoiceBaselineWarmth;
    TArray<float> CampChoiceBaselineWetness;
    int32 RainVerticalSliceStage = 0;
    float RainVerticalSliceStageTime = 0.0f;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> RainVerticalSlicePlayers;
    TWeakObjectPtr<class AKalmalaCampfire> RainVerticalSliceFire;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceExposedFloor;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceRoofedFloor;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceFloorRoof;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceFireRoof;
    int32 CombatPeerTestStage = 0;
    bool bCombatPeerTestLogged = false;
    float CombatPeerTestStageTime = 0.0f;
    uint32 CombatPeerTestSequence = 0;
    TWeakObjectPtr<class AKalmalaCharacter> CombatPeerTestAttacker;
    TWeakObjectPtr<class AKalmalaCharacter> CombatPeerTestRemote;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> CombatPeerTestTarget;
    void InitializeWeatherCycle();
    void AdvanceWeatherCycleIfNeeded();
    void PlacePawnAtGeneratedStart(class APlayerController* PlayerController);

    class APlayerStart* GeneratedPlayerStart = nullptr;
    FKalmalaWorldGenerationConfig WorldGenerationConfig;
    TObjectPtr<class UKalmalaWorldPopulationSaveGame> PopulationSaveGame;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaConstructionSaveGame> ConstructionSaveGame;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaStorageSaveGame> StorageSaveGame;
    FVector2D TerrainPatchOrigin = FVector2D::ZeroVector;
    TSet<FIntPoint> ActiveTerrainPatchCoordinates;
    TMap<FIntPoint, TObjectPtr<class AKalmalaGeneratedTerrainPatch>> ActiveTerrainPatches;
    TSet<FIntPoint> ActivePopulationSpatialKeys;
    TSet<FIntPoint> ActiveShimmeringLakeDiscoveryKeys;
    TSet<FIntPoint> ActiveElderwoodDiscoveryKeys;
    TSet<FIntPoint> ActiveMossyMireDiscoveryKeys;
    TSet<FIntPoint> ActiveFreezingTundraDiscoveryKeys;
    TSet<FIntPoint> ActiveThunderMountainsDiscoveryKeys;
    float NextTerrainPatchActivationTime = 0.0f;
    float NextExposureUpdateTime = 0.0f;
    float NextInteractionGridUpdateTime = 0.0f;
    TMap<FIntPoint, FKalmalaInteractionCellState> ActiveInteractionCells;
    TMap<TWeakObjectPtr<class AKalmalaCharacter>, float> UnroofedRainSecondsByCharacter;
    float WorldProfileReportTime = -1.0f;
    double InitialGenerationMilliseconds = -1.0;
    bool bTraversalTestEnabled = false;
    bool bExposureInspectionEnabled = false;
    bool bExposureReplicationTestEnabled = false;
    bool bExposureReplicationCampfireSpawned = false;
    bool bCampConditionInspectionEnabled = false;
    bool bBiomeFeatureInspectionEnabled = false;
    bool bWorldProfileEnabled = false;
    bool bWorldProfileReported = false;
    FString ReconnectVerificationMode;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
    TSet<TWeakObjectPtr<APawn>> TraversalTestCompletedPawns;
    TSet<TWeakObjectPtr<APawn>> TraversalTestStartedPawns;
};
