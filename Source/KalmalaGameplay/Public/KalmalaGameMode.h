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
    void LogExposureInspection(const FVector& Location) const;
    void InitializeWeatherCycle();
    void AdvanceWeatherCycleIfNeeded();

    class APlayerStart* GeneratedPlayerStart = nullptr;
    FKalmalaWorldGenerationConfig WorldGenerationConfig;
    TObjectPtr<class UKalmalaWorldPopulationSaveGame> PopulationSaveGame;
    FVector2D TerrainPatchOrigin = FVector2D::ZeroVector;
    TSet<FIntPoint> ActiveTerrainPatchCoordinates;
    TSet<FIntPoint> ActivePopulationSpatialKeys;
    float NextTerrainPatchActivationTime = 0.0f;
    bool bTraversalTestEnabled = false;
    FString ReconnectVerificationMode;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
    TSet<TWeakObjectPtr<APawn>> TraversalTestCompletedPawns;
    TSet<TWeakObjectPtr<APawn>> TraversalTestStartedPawns;
};
