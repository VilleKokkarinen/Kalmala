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
    void ConfigureTraversalTest();
    void DriveTraversalTest();

    class APlayerStart* GeneratedPlayerStart = nullptr;
    FKalmalaWorldGenerationConfig WorldGenerationConfig;
    FVector2D TerrainPatchOrigin = FVector2D::ZeroVector;
    TSet<FIntPoint> ActiveTerrainPatchCoordinates;
    TSet<FIntPoint> ActivePopulationSpatialKeys;
    float NextTerrainPatchActivationTime = 0.0f;
    bool bTraversalTestEnabled = false;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
    TSet<TWeakObjectPtr<APawn>> TraversalTestCompletedPawns;
    TSet<TWeakObjectPtr<APawn>> TraversalTestStartedPawns;
};
