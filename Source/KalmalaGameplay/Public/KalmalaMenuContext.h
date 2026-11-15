#pragma once

#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaLocalWorldSaveListing.h"
#include "KalmalaMenuContext.generated.h"

DECLARE_MULTICAST_DELEGATE_FiveParams(FKalmalaMenuWorldSeedSelectionResult,
    uint64, uint64, bool, EKalmalaLocalWorldSaveListingAvailability, EKalmalaWorldSeedReuseDecision);

/** Identifies the dedicated local menu context to persistent local-player systems. */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaMenuPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AKalmalaMenuPlayerController();
    void RequestWorldSeedSelection(uint64 Seed, bool bConfirmedExisting, uint64 RequestToken);
    FKalmalaMenuWorldSeedSelectionResult& OnWorldSeedSelectionResult() { return WorldSeedSelectionResult; }
#if WITH_DEV_AUTOMATION_TESTS
    static bool CanProcessWorldSeedSelectionRequestForTests(bool bHasAuthority,
        bool bIsLocalController, bool bHasWorld);
#endif

protected:
    virtual void BeginPlay() override;
    virtual void SpawnDefaultHUD() override;
    virtual ASpectatorPawn* SpawnSpectatorPawn() override;

private:
#if WITH_DEV_AUTOMATION_TESTS
    void BeginWorldSeedSelectionPeerTest();
    void SendWorldSeedSelectionPeerRequests();
    void HandleWorldSeedSelectionPeerResult(uint64 RequestToken, uint64 Seed, bool bAccepted,
        EKalmalaLocalWorldSaveListingAvailability Availability, EKalmalaWorldSeedReuseDecision Decision);
    FDelegateHandle WorldSeedSelectionPeerReplyHandle;
    int32 WorldSeedSelectionPeerReplyMask = 0;
    bool bWorldSeedSelectionPeerRepliesGeneric = true;
#endif
    UFUNCTION(Server, Reliable)
    void ServerRequestWorldSeedSelection(uint64 Seed, bool bConfirmedExisting, uint64 RequestToken);

    UFUNCTION(Client, Reliable)
    void ClientWorldSeedSelectionResult(uint64 Seed, uint64 RequestToken, bool bAccepted,
        uint8 Availability, uint8 Decision);

    void ProcessWorldSeedSelectionRequest(uint64 Seed, bool bConfirmedExisting, uint64 RequestToken);
    void PublishWorldSeedSelectionResult(uint64 Seed, uint64 RequestToken, bool bAccepted,
        EKalmalaLocalWorldSaveListingAvailability Availability, EKalmalaWorldSeedReuseDecision Decision);

    FKalmalaMenuWorldSeedSelectionResult WorldSeedSelectionResult;
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
    /** Revalidates the selected identity and starts one local, unadvertised solo world. */
    bool TryStartSoloFromServer(class APlayerController* RequestingController,
        uint64 Seed, bool bConfirmedExisting);
    /** Revalidates the selected identity and starts one direct-address listen host. */
    bool TryStartListenHostFromServer(class APlayerController* RequestingController,
        uint64 Seed, bool bConfirmedExisting);
    /** Reopens the explicit menu launch gate after engine-reported travel failure. */
    void ResetGameplayLaunchAfterTravelFailure(class APlayerController* RequestingController);
#if WITH_DEV_AUTOMATION_TESTS
    int32 GetWorldSeedSaveEnumerationCountForTests() const { return WorldSeedSaveEnumerationCountForTests; }
    void ResetWorldSeedSaveEnumerationCountForTests() { WorldSeedSaveEnumerationCountForTests = 0; }
#endif
    bool HasPendingWorldSeedSelection() const { return bHasPendingWorldSeedSelection; }
    uint64 GetPendingWorldSeed() const { return PendingWorldSeed; }
    bool IsPendingWorldSeedExisting() const { return bPendingWorldSeedIsExisting; }
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

private:
    bool TryStartGameplayFromServer(class APlayerController* RequestingController,
        uint64 Seed, bool bConfirmedExisting, bool bListenHost);
#if WITH_DEV_AUTOMATION_TESTS
    int32 WorldSeedSaveEnumerationCountForTests = 0;
#endif
    bool bHasPendingWorldSeedSelection = false;
    uint64 PendingWorldSeed = 0;
    bool bPendingWorldSeedIsExisting = false;
    bool bGameplayLaunchInProgress = false;
};
