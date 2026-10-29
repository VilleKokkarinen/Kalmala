#include "KalmalaMenuContext.h"

#include "Engine/World.h"
#include "KalmalaM7PersistenceContract.h"

void AKalmalaMenuPlayerController::SpawnDefaultHUD() {}
ASpectatorPawn* AKalmalaMenuPlayerController::SpawnSpectatorPawn() { return nullptr; }

AKalmalaMenuGameMode::AKalmalaMenuGameMode()
{
    PlayerControllerClass = AKalmalaMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    SpectatorClass = nullptr;
    HUDClass = nullptr;
}

bool AKalmalaMenuGameMode::TrySelectWorldSeedFromServer(const uint64 Seed, const bool bConfirmedExisting,
    EKalmalaLocalWorldSaveListingAvailability& OutAvailability, EKalmalaWorldSeedReuseDecision& OutDecision)
{
    OutAvailability = EKalmalaLocalWorldSaveListingAvailability::Unsupported;
    OutDecision = EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview;

    UWorld* World = GetWorld();
    if (!HasAuthority() || World == nullptr || World->GetNetMode() == NM_Client) return false;

    bHasPendingWorldSeedSelection = false;
    PendingWorldSeed = 0;
    bPendingWorldSeedIsExisting = false;

    TArray<FKalmalaLocalWorldSaveRecord> Records;
    OutAvailability = FKalmalaLocalWorldSaveListing::Enumerate(Records);
    OutDecision = FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
        OutAvailability, Records, Seed, FKalmalaM7SaveIdentity::ForWorld(Seed).GeneratorRevision);

    if (OutAvailability != EKalmalaLocalWorldSaveListingAvailability::Supported) return false;
    if (OutDecision == EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview) return false;
    if (OutDecision == EKalmalaWorldSeedReuseDecision::ConfirmExisting && !bConfirmedExisting) return false;
    if (OutDecision == EKalmalaWorldSeedReuseDecision::NoKnownRecord && bConfirmedExisting) return false;

    bHasPendingWorldSeedSelection = true;
    PendingWorldSeed = Seed;
    bPendingWorldSeedIsExisting = OutDecision == EKalmalaWorldSeedReuseDecision::ConfirmExisting;
    return true;
}

void AKalmalaMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) {}
void AKalmalaMenuGameMode::RestartPlayer(AController* NewPlayer) {}
APawn* AKalmalaMenuGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) { return nullptr; }
APawn* AKalmalaMenuGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) { return nullptr; }
