#include "KalmalaMenuContext.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "KalmalaM7PersistenceContract.h"

void AKalmalaMenuPlayerController::SpawnDefaultHUD() {}
ASpectatorPawn* AKalmalaMenuPlayerController::SpawnSpectatorPawn() { return nullptr; }

void AKalmalaMenuPlayerController::RequestWorldSeedSelection(const uint64 Seed,
    const bool bConfirmedExisting, const uint64 RequestToken)
{
    if (HasAuthority())
    {
        ProcessWorldSeedSelectionRequest(Seed, bConfirmedExisting, RequestToken);
        return;
    }
    ServerRequestWorldSeedSelection(Seed, bConfirmedExisting, RequestToken);
}

void AKalmalaMenuPlayerController::ServerRequestWorldSeedSelection_Implementation(const uint64 Seed,
    const bool bConfirmedExisting, const uint64 RequestToken)
{
    ProcessWorldSeedSelectionRequest(Seed, bConfirmedExisting, RequestToken);
}

void AKalmalaMenuPlayerController::ProcessWorldSeedSelectionRequest(const uint64 Seed,
    const bool bConfirmedExisting, const uint64 RequestToken)
{
    EKalmalaLocalWorldSaveListingAvailability Availability = EKalmalaLocalWorldSaveListingAvailability::Unsupported;
    EKalmalaWorldSeedReuseDecision Decision = EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview;
    bool bAccepted = false;

    if (HasAuthority() && IsLocalController() && GetWorld() != nullptr)
    {
        if (AKalmalaMenuGameMode* MenuGameMode = GetWorld()->GetAuthGameMode<AKalmalaMenuGameMode>())
        {
            bAccepted = MenuGameMode->TrySelectWorldSeedFromServer(
                Seed, bConfirmedExisting, Availability, Decision);
        }
    }

    if (IsLocalController())
    {
        PublishWorldSeedSelectionResult(Seed, RequestToken, bAccepted, Availability, Decision);
        return;
    }

    ClientWorldSeedSelectionResult(Seed, RequestToken, bAccepted,
        static_cast<uint8>(Availability), static_cast<uint8>(Decision));
}

void AKalmalaMenuPlayerController::ClientWorldSeedSelectionResult_Implementation(const uint64 Seed,
    const uint64 RequestToken, const bool bAccepted, const uint8 Availability, const uint8 Decision)
{
    if (Availability > static_cast<uint8>(EKalmalaLocalWorldSaveListingAvailability::Unsupported)
        || Decision > static_cast<uint8>(EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview))
    {
        PublishWorldSeedSelectionResult(Seed, RequestToken, false,
            EKalmalaLocalWorldSaveListingAvailability::Unsupported,
            EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview);
        return;
    }

    const auto ParsedAvailability = static_cast<EKalmalaLocalWorldSaveListingAvailability>(Availability);
    const auto ParsedDecision = static_cast<EKalmalaWorldSeedReuseDecision>(Decision);
    const bool bValidAcceptedResult = bAccepted
        && ParsedAvailability == EKalmalaLocalWorldSaveListingAvailability::Supported
        && (ParsedDecision == EKalmalaWorldSeedReuseDecision::NoKnownRecord
            || ParsedDecision == EKalmalaWorldSeedReuseDecision::ConfirmExisting);
    PublishWorldSeedSelectionResult(Seed, RequestToken, bValidAcceptedResult,
        ParsedAvailability, ParsedDecision);
}

void AKalmalaMenuPlayerController::PublishWorldSeedSelectionResult(const uint64 Seed,
    const uint64 RequestToken, const bool bAccepted,
    const EKalmalaLocalWorldSaveListingAvailability Availability,
    const EKalmalaWorldSeedReuseDecision Decision)
{
    WorldSeedSelectionResult.Broadcast(RequestToken, Seed, bAccepted, Availability, Decision);
}

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

bool AKalmalaMenuGameMode::TryStartSoloFromServer(APlayerController* RequestingController,
    const uint64 Seed, const bool bConfirmedExisting)
{
    return TryStartGameplayFromServer(RequestingController, Seed, bConfirmedExisting, false);
}

bool AKalmalaMenuGameMode::TryStartListenHostFromServer(APlayerController* RequestingController,
    const uint64 Seed, const bool bConfirmedExisting)
{
    return TryStartGameplayFromServer(RequestingController, Seed, bConfirmedExisting, true);
}

bool AKalmalaMenuGameMode::TryStartGameplayFromServer(APlayerController* RequestingController,
    const uint64 Seed, const bool bConfirmedExisting, const bool bListenHost)
{
    UWorld* World = GetWorld();
    if (!HasAuthority() || World == nullptr || World->GetNetMode() != NM_Standalone
        || bGameplayLaunchInProgress || !IsValid(RequestingController)
        || !RequestingController->IsA<AKalmalaMenuPlayerController>()
        || RequestingController->GetWorld() != World || !RequestingController->HasAuthority()
        || !RequestingController->IsLocalController()) return false;

    EKalmalaLocalWorldSaveListingAvailability Availability = EKalmalaLocalWorldSaveListingAvailability::Unsupported;
    EKalmalaWorldSeedReuseDecision Decision = EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview;
    if (!TrySelectWorldSeedFromServer(Seed, bConfirmedExisting, Availability, Decision)) return false;

    const int32 GeneratorRevision = FKalmalaM7SaveIdentity::ForWorld(PendingWorldSeed).GeneratorRevision;
    if (GeneratorRevision <= 0) return false;

    const FString ListenOption = bListenHost ? TEXT("listen?") : TEXT("");
    const FString LaunchKind = bListenHost ? TEXT("HostCoop") : TEXT("Solo");
    const FString LaunchOptions = FString::Printf(
        TEXT("%sKalmalaMenuLaunch=%s?KalmalaMenuSeed=%llu?KalmalaMenuRevision=%d"),
        *ListenOption, *LaunchKind, PendingWorldSeed, GeneratorRevision);
    bGameplayLaunchInProgress = true;
    UGameplayStatics::OpenLevel(RequestingController,
        FName(TEXT("/Game/Kalmala/Maps/Prototype/L_Prototype")), true, LaunchOptions);
    return true;
}

void AKalmalaMenuGameMode::ResetGameplayLaunchAfterTravelFailure(APlayerController* RequestingController)
{
    UWorld* World = GetWorld();
    if (!HasAuthority() || World == nullptr || World->GetNetMode() != NM_Standalone
        || !bGameplayLaunchInProgress || !IsValid(RequestingController)
        || !RequestingController->IsA<AKalmalaMenuPlayerController>()
        || RequestingController->GetWorld() != World || !RequestingController->HasAuthority()
        || !RequestingController->IsLocalController()) return;

    bGameplayLaunchInProgress = false;
}

void AKalmalaMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) {}
void AKalmalaMenuGameMode::RestartPlayer(AController* NewPlayer) {}
APawn* AKalmalaMenuGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) { return nullptr; }
APawn* AKalmalaMenuGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) { return nullptr; }
