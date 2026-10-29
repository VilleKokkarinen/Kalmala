#pragma once

#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaLocalWorldSaveListing.h"
#include "KalmalaMenuContext.generated.h"

/** Identifies the dedicated local menu context to persistent local-player systems. */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaMenuPlayerController : public APlayerController
{
    GENERATED_BODY()
protected:
    virtual void SpawnDefaultHUD() override;
    virtual ASpectatorPawn* SpawnSpectatorPawn() override;
};

/** Lightweight front-end mode. It never starts the gameplay world or spawns a player pawn. */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AKalmalaMenuGameMode();
    /** Revalidates a seed against server-local slots before accepting new-world or confirmed reuse. */
    bool TrySelectWorldSeedFromServer(uint64 Seed, bool bConfirmedExisting,
        EKalmalaLocalWorldSaveListingAvailability& OutAvailability,
        EKalmalaWorldSeedReuseDecision& OutDecision);
    bool HasPendingWorldSeedSelection() const { return bHasPendingWorldSeedSelection; }
    uint64 GetPendingWorldSeed() const { return PendingWorldSeed; }
    bool IsPendingWorldSeedExisting() const { return bPendingWorldSeedIsExisting; }
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

private:
    bool bHasPendingWorldSeedSelection = false;
    uint64 PendingWorldSeed = 0;
    bool bPendingWorldSeedIsExisting = false;
};
