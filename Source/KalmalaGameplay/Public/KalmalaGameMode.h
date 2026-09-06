#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KalmalaWorldGenerationConfig.h"
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

private:
    void ActivateTerrainPatch(const FIntPoint& PatchCoordinate);
    void ActivateTerrainPatchNeighborhood(const FVector2D& WorldPosition);
    void ActivatePopulationKey(const FIntPoint& SpatialKey);
    void RecordHarvestedSpawn(const FString& PersistentSpawnId);
    void RecordDefeatedSpawn(const FString& PersistentSpawnId);
    void ConfigureTraversalTest();
    void DriveTraversalTest();
    void RunReconnectVerification(APawn* ServerPawn);
    void LogExposureInspection(const AActor* Occupant) const;
    void LogCampConditionInspection(const AActor* Occupant) const;
    void LogBiomeFeatureInspection(const AActor* Occupant) const;
    void UpdatePlayerExposure(float DeltaSeconds);
    void DriveCampChoiceTest();
    float CampChoiceStartTime = -1.0f;
    int32 CampChoiceStage = 0;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> CampChoicePlayers;
    TArray<float> CampChoiceBaselineWarmth;
    TArray<float> CampChoiceBaselineWetness;
    void InitializeWeatherCycle();
    void AdvanceWeatherCycleIfNeeded();
    void PlacePawnAtGeneratedStart(class APlayerController* PlayerController);

    class APlayerStart* GeneratedPlayerStart = nullptr;
    FKalmalaWorldGenerationConfig WorldGenerationConfig;
    TObjectPtr<class UKalmalaWorldPopulationSaveGame> PopulationSaveGame;
    FVector2D TerrainPatchOrigin = FVector2D::ZeroVector;
    TSet<FIntPoint> ActiveTerrainPatchCoordinates;
    TSet<FIntPoint> ActivePopulationSpatialKeys;
    TSet<FIntPoint> ActiveShimmeringLakeDiscoveryKeys;
    TSet<FIntPoint> ActiveElderwoodDiscoveryKeys;
    TSet<FIntPoint> ActiveMossyMireDiscoveryKeys;
    TSet<FIntPoint> ActiveFreezingTundraDiscoveryKeys;
    TSet<FIntPoint> ActiveThunderMountainsDiscoveryKeys;
    float NextTerrainPatchActivationTime = 0.0f;
    float NextExposureUpdateTime = 0.0f;
    bool bTraversalTestEnabled = false;
    bool bExposureInspectionEnabled = false;
    bool bExposureReplicationTestEnabled = false;
    bool bExposureReplicationCampfireSpawned = false;
    bool bCampConditionInspectionEnabled = false;
    bool bBiomeFeatureInspectionEnabled = false;
    FString ReconnectVerificationMode;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
    TSet<TWeakObjectPtr<APawn>> TraversalTestCompletedPawns;
    TSet<TWeakObjectPtr<APawn>> TraversalTestStartedPawns;
};
