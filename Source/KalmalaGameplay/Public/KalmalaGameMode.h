#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaInteractionGrid.h"
#include "KalmalaGameMode.generated.h"

struct FKalmalaOceanDiscoveryDescriptor;
struct FKalmalaM9ExplorationRewardDescriptor;
enum class EKalmalaOceanSkiffSeat : uint8;

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
    bool ClaimDiscovery(class AKalmalaCharacter* Interactor, const struct FKalmalaWorldDiscoveryDescriptor& Descriptor);
    bool ClaimOceanDiscovery(class AKalmalaCharacter* Interactor, const FKalmalaOceanDiscoveryDescriptor& Descriptor);
    bool ClaimM9ExplorationReward(class AKalmalaCharacter* Interactor, const FKalmalaM9ExplorationRewardDescriptor& Descriptor);
    bool ClaimMirelingBossScroll(class AKalmalaCharacter* Attacker, const FString& PersistentSpawnId);
    static bool IsMirelingBossRewardId(const FString& PersistentSpawnId);
    static FString GetMirelingBossScrollDefinition(uint64 WorldSeed);
    static FString GetMirelingBossScrollId(uint64 WorldSeed);
    void OnOceanSkiffStateChanged(class AKalmalaOceanSkiff* Skiff);
    bool ClearOceanTravelPassenger(class AKalmalaCharacter* Interactor, const FString& VesselId);

private:
    void ActivateTerrainPatch(const FIntPoint& PatchCoordinate);
    void ActivateTerrainPatchNeighborhood(const FVector2D& WorldPosition);
    void RefreshTerrainPatchNeighborhoods();
    void ActivatePopulationKey(const FIntPoint& SpatialKey);
    void RefreshOceanDiscoveries();
    void RecordHarvestedSpawn(const FString& PersistentSpawnId);
    void RecordM9ResourceDepleted(const FString& StableResourceId);
    void RecordDefeatedSpawn(const FString& PersistentSpawnId);
    class UKalmalaPlayerDiscoverySaveGame* GetPlayerDiscoverySave(class AKalmalaCharacter* Interactor, FString& OutIdentity);
    bool IsCurrentDiscoveryDescriptor(const struct FKalmalaWorldDiscoveryDescriptor& Descriptor) const;
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
    void DriveDiscoveryPeerTest();
    void DriveM9ExplorationRewardPeerTest();
    void DriveOceanTravelFeedbackTest();
    void DriveOceanWeatherPeerTest();
    void DriveOceanJourneyPeerTest();
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
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> CombatPeerTestHerdMate;
    bool bCombatPeerTestHerdAlertObserved = false;
    int32 DiscoveryPeerTestStage = 0;
    float DiscoveryPeerTestStageTime = 0.0f;
    bool bDiscoveryPeerTestLogged = false;
    FKalmalaWorldDiscoveryDescriptor DiscoveryPeerTestDescriptor;
    TWeakObjectPtr<class AKalmalaCharacter> DiscoveryPeerTestEntitled;
    TWeakObjectPtr<class AKalmalaCharacter> DiscoveryPeerTestRemote;
    TWeakObjectPtr<class AKalmalaDiscoveryActor> DiscoveryPeerTestActor;
    int32 M9ExplorationRewardPeerTestStage = 0;
    float M9ExplorationRewardPeerTestStageTime = 0.0f;
    FKalmalaM9ExplorationRewardDescriptor M9ExplorationRewardPeerTestDescriptor;
    TWeakObjectPtr<class AKalmalaCharacter> M9ExplorationRewardPeerTestHost;
    TWeakObjectPtr<class AKalmalaCharacter> M9ExplorationRewardPeerTestRemote;
    TWeakObjectPtr<class AKalmalaDiscoveryActor> M9ExplorationRewardPeerTestActor;
    int32 M9ExplorationRewardPeerTestHostBaseline = 0;
    int32 M9ExplorationRewardPeerTestRemoteBaseline = 0;
    int32 OceanTravelFeedbackTestStage = 0;
    float OceanTravelFeedbackTestStageTime = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> OceanTravelFeedbackTestHost;
    TWeakObjectPtr<class AKalmalaCharacter> OceanTravelFeedbackTestRemote;
    int32 OceanWeatherPeerTestStage = 0;
    float OceanWeatherPeerTestStageTime = 0.0f;
    int32 OceanJourneyPeerTestStage = 0;
    float OceanJourneyPeerTestStartTime = 0.0f;
    float OceanJourneyPeerTestStageTime = 0.0f;
    FVector2D OceanJourneyPeerTestLaunch = FVector2D::ZeroVector;
    FVector2D OceanJourneyPeerTestTarget = FVector2D::ZeroVector;
    float OceanJourneyPeerTestYaw = 0.0f;
    bool bOceanJourneyPeerTestSawCrosswind = false;
    bool bOceanJourneyPeerTestSawCalm = false;
    TWeakObjectPtr<class AKalmalaCharacter> OceanJourneyPeerTestHelm;
    TWeakObjectPtr<class AKalmalaCharacter> OceanJourneyPeerTestPassenger;
    TWeakObjectPtr<class AKalmalaOceanSkiff> OceanJourneyPeerTestSkiff;
    void InitializeWeatherCycle();
    void AdvanceWeatherCycleIfNeeded();
    void PlacePawnAtGeneratedStart(class APlayerController* PlayerController);
    void SpawnOceanTravelTestFixture();
    void LoadOceanTravelWorldSave();
    void RestorePersistedOceanSkiff();
    void RestoreOceanTravelForPlayer(class APlayerController* PlayerController);
    void LoadOceanDiscoveryLedger(class AKalmalaCharacter* Interactor);
    bool PersistOceanTravelState(class AKalmalaOceanSkiff* Skiff);
    bool PersistOceanTravelPassenger(class AKalmalaCharacter* Interactor,
        const FString& VesselId, EKalmalaOceanSkiffSeat Seat);

    class APlayerStart* GeneratedPlayerStart = nullptr;
    FKalmalaWorldGenerationConfig WorldGenerationConfig;
    UPROPERTY(Transient) TObjectPtr<class AKalmalaOceanTravelTestFixture> OceanTravelTestFixture;
    TObjectPtr<class UKalmalaWorldPopulationSaveGame> PopulationSaveGame;
    TSet<FString> SessionM9ResourceDepletionIds;
    TMap<FString, TObjectPtr<class UKalmalaPlayerDiscoverySaveGame>> PlayerDiscoverySaves;
    TMap<FString, TObjectPtr<class UKalmalaM7PersistenceSaveGame>> OceanDiscoverySaves;
    TMap<FString, TSet<FString>> SessionM9ExplorationClaims;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaOceanTravelPersistenceSaveGame> OceanTravelWorldSave;
    TMap<FString, TObjectPtr<class UKalmalaOceanTravelPersistenceSaveGame>> OceanTravelPlayerSaves;
    TWeakObjectPtr<class AKalmalaOceanSkiff> RestoredOceanSkiff;
    TArray<TWeakObjectPtr<APlayerController>> PendingOceanTravelRestores;
    bool bOceanTravelPersistenceWritable = false;
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
    TMap<FIntPoint, TArray<TWeakObjectPtr<class AKalmalaDiscoveryActor>>> ActiveOceanDiscoveryActors;
    float NextTerrainPatchActivationTime = 0.0f;
    float NextOceanDiscoveryRefreshTime = 0.0f;
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
