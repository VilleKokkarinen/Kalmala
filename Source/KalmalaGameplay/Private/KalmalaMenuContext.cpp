#include "KalmalaMenuContext.h"

#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanTravelPersistenceContract.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

namespace
{
#if WITH_DEV_AUTOMATION_TESTS
    constexpr uint64 WorldSeedPeerUnlistedSeed = 86753091233ull;
    constexpr uint64 WorldSeedPeerExistingSeed = 86753091234ull;
    constexpr uint64 WorldSeedPeerUnlistedToken = 5201ull;
    constexpr uint64 WorldSeedPeerExistingToken = 5202ull;
#endif

    bool IsWorldSeedSelectionRequesterAllowed(const bool bHasAuthority,
        const bool bIsLocalController, const bool bHasWorld)
    {
        return bHasAuthority && bIsLocalController && bHasWorld;
    }
}

AKalmalaMenuPlayerController::AKalmalaMenuPlayerController()
{
    bShowMouseCursor = true;
}

void AKalmalaMenuPlayerController::BeginPlay()
{
    Super::BeginPlay();
#if WITH_DEV_AUTOMATION_TESTS
    BeginWorldSeedSelectionPeerTest();
#endif
    if (!IsLocalController()) return;

    // Establish menu ownership before the Home widget supplies its initial focus.
    // UIOnly releases capture but does not clear the viewport's cursor-hide flag.
    SetShowMouseCursor(true);
    if (UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
    {
        Viewport->SetHideCursorDuringCapture(false);
    }
    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

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

#if WITH_DEV_AUTOMATION_TESTS
bool AKalmalaMenuPlayerController::CanProcessWorldSeedSelectionRequestForTests(
    const bool bHasAuthority, const bool bIsLocalController, const bool bHasWorld)
{
    return IsWorldSeedSelectionRequesterAllowed(bHasAuthority, bIsLocalController, bHasWorld);
}
#endif

void AKalmalaMenuPlayerController::ServerRequestWorldSeedSelection_Implementation(const uint64 Seed,
    const bool bConfirmedExisting, const uint64 RequestToken)
{
    ProcessWorldSeedSelectionRequest(Seed, bConfirmedExisting, RequestToken);
}

void AKalmalaMenuPlayerController::ProcessWorldSeedSelectionRequest(const uint64 Seed,
    const bool bConfirmedExisting, const uint64 RequestToken)
{
#if WITH_DEV_AUTOMATION_TESTS
    const int32 SaveEnumerationCountBefore = FKalmalaLocalWorldSaveListing::GetEnumerationCountForTests();
#endif
    EKalmalaLocalWorldSaveListingAvailability Availability = EKalmalaLocalWorldSaveListingAvailability::Unsupported;
    EKalmalaWorldSeedReuseDecision Decision = EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview;
    bool bAccepted = false;

    if (IsWorldSeedSelectionRequesterAllowed(HasAuthority(), IsLocalController(), GetWorld() != nullptr))
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

#if WITH_DEV_AUTOMATION_TESTS
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldSeedSelectionPeerTest")))
    {
        const AKalmalaMenuGameMode* MenuGameMode = GetWorld()
            ? GetWorld()->GetAuthGameMode<AKalmalaMenuGameMode>() : nullptr;
        const bool bGenericRejection = !bAccepted
            && Availability == EKalmalaLocalWorldSaveListingAvailability::Unsupported
            && Decision == EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview;
        UE_LOG(LogTemp, Display,
            TEXT("World seed selection peer server: Token=%llu Generic=%d EnumerationBefore=%d EnumerationAfter=%d GateEnumerations=%d Pending=%d Seed=%llu Existing=%d"),
            RequestToken, bGenericRejection ? 1 : 0,
            SaveEnumerationCountBefore, FKalmalaLocalWorldSaveListing::GetEnumerationCountForTests(),
            MenuGameMode ? MenuGameMode->GetWorldSeedSaveEnumerationCountForTests() : -1,
            MenuGameMode && MenuGameMode->HasPendingWorldSeedSelection() ? 1 : 0,
            MenuGameMode ? MenuGameMode->GetPendingWorldSeed() : 0,
            MenuGameMode && MenuGameMode->IsPendingWorldSeedExisting() ? 1 : 0);
    }
#endif

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

#if WITH_DEV_AUTOMATION_TESTS
void AKalmalaMenuPlayerController::BeginWorldSeedSelectionPeerTest()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldSeedSelectionPeerTest"))) return;

    if (!HasAuthority())
    {
        if (!IsLocalController() || !GetWorld()
            || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldSeedRemoteTest"))) return;

        WorldSeedSelectionPeerRepliesGeneric = true;
        WorldSeedSelectionPeerReplyMask = 0;
        WorldSeedSelectionPeerReplyHandle = OnWorldSeedSelectionResult().AddUObject(
            this, &AKalmalaMenuPlayerController::HandleWorldSeedSelectionPeerResult);
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &AKalmalaMenuPlayerController::SendWorldSeedSelectionPeerRequests));
        return;
    }

    if (!IsLocalController() || !GetWorld()) return;
    auto* MenuGameMode = GetWorld()->GetAuthGameMode<AKalmalaMenuGameMode>();
    if (!MenuGameMode) return;

    const FKalmalaM7SaveIdentity ExistingIdentity = FKalmalaM7SaveIdentity::ForWorld(WorldSeedPeerExistingSeed);
    auto* ExistingSave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>(this);
    if (!ExistingSave || !ExistingIdentity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("World seed selection peer fixture: Local=Failed Reason=SaveFixture"));
        return;
    }
    ExistingSave->Initialize(ExistingIdentity);
    const FString ExistingSlot = FString::Printf(TEXT("KalmalaOceanTravelWorld_%llu_%d"),
        WorldSeedPeerExistingSeed, ExistingIdentity.GeneratorRevision);
    if (!UGameplayStatics::SaveGameToSlot(ExistingSave, ExistingSlot, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("World seed selection peer fixture: Local=Failed Reason=SaveWrite"));
        return;
    }

    MenuGameMode->ResetWorldSeedSaveEnumerationCountForTests();
    bool bUnlistedResultPassed = false;
    bool bExistingResultPassed = false;
    auto& ResultDelegate = OnWorldSeedSelectionResult();
    const FDelegateHandle ResultHandle = ResultDelegate.AddLambda(
        [&bUnlistedResultPassed, &bExistingResultPassed](const uint64 RequestToken, const uint64 Seed,
            const bool bAccepted, const EKalmalaLocalWorldSaveListingAvailability Availability,
            const EKalmalaWorldSeedReuseDecision Decision)
        {
            if (RequestToken == WorldSeedPeerUnlistedToken)
                bUnlistedResultPassed = Seed == WorldSeedPeerUnlistedSeed && bAccepted
                    && Availability == EKalmalaLocalWorldSaveListingAvailability::Supported
                    && Decision == EKalmalaWorldSeedReuseDecision::NoKnownRecord;
            else if (RequestToken == WorldSeedPeerExistingToken)
                bExistingResultPassed = Seed == WorldSeedPeerExistingSeed && bAccepted
                    && Availability == EKalmalaLocalWorldSaveListingAvailability::Supported
                    && Decision == EKalmalaWorldSeedReuseDecision::ConfirmExisting;
        });
    RequestWorldSeedSelection(WorldSeedPeerUnlistedSeed, false, WorldSeedPeerUnlistedToken);
    const bool bUnlistedPendingPassed = MenuGameMode->HasPendingWorldSeedSelection()
        && MenuGameMode->GetPendingWorldSeed() == WorldSeedPeerUnlistedSeed
        && !MenuGameMode->IsPendingWorldSeedExisting();
    RequestWorldSeedSelection(WorldSeedPeerExistingSeed, true, WorldSeedPeerExistingToken);
    const bool bExistingPendingPassed = MenuGameMode->HasPendingWorldSeedSelection()
        && MenuGameMode->GetPendingWorldSeed() == WorldSeedPeerExistingSeed
        && MenuGameMode->IsPendingWorldSeedExisting();
    ResultDelegate.Remove(ResultHandle);

    const bool bPassed = bUnlistedResultPassed && bExistingResultPassed
        && bUnlistedPendingPassed && bExistingPendingPassed
        && MenuGameMode->GetWorldSeedSaveEnumerationCountForTests() == 2;
    UE_LOG(LogTemp, Display,
        TEXT("World seed selection peer fixture: Local=%s Enumerations=%d Pending=%d Seed=%llu Existing=%d"),
        bPassed ? TEXT("Passed") : TEXT("Failed"), MenuGameMode->GetWorldSeedSaveEnumerationCountForTests(),
        MenuGameMode->HasPendingWorldSeedSelection() ? 1 : 0, MenuGameMode->GetPendingWorldSeed(),
        MenuGameMode->IsPendingWorldSeedExisting() ? 1 : 0);
}

void AKalmalaMenuPlayerController::SendWorldSeedSelectionPeerRequests()
{
    if (!IsLocalController() || HasAuthority()) return;
    RequestWorldSeedSelection(WorldSeedPeerExistingSeed, true, WorldSeedPeerExistingToken);
    RequestWorldSeedSelection(WorldSeedPeerUnlistedSeed, false, WorldSeedPeerUnlistedToken);
}

void AKalmalaMenuPlayerController::HandleWorldSeedSelectionPeerResult(const uint64 RequestToken,
    const uint64 Seed, const bool bAccepted, const EKalmalaLocalWorldSaveListingAvailability Availability,
    const EKalmalaWorldSeedReuseDecision Decision)
{
    bool bExpectedReply = false;
    if (RequestToken == WorldSeedPeerExistingToken)
    {
        bExpectedReply = Seed == WorldSeedPeerExistingSeed;
        WorldSeedSelectionPeerReplyMask |= 1;
    }
    else if (RequestToken == WorldSeedPeerUnlistedToken)
    {
        bExpectedReply = Seed == WorldSeedPeerUnlistedSeed;
        WorldSeedSelectionPeerReplyMask |= 2;
    }

    WorldSeedSelectionPeerRepliesGeneric = WorldSeedSelectionPeerRepliesGeneric
        && bExpectedReply && !bAccepted
        && Availability == EKalmalaLocalWorldSaveListingAvailability::Unsupported
        && Decision == EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview;
    if (WorldSeedSelectionPeerReplyMask == 3)
    {
        UE_LOG(LogTemp, Display,
            TEXT("World seed selection peer client: Passed=%d Replies=2 Generic=%d"),
            WorldSeedSelectionPeerRepliesGeneric ? 1 : 0,
            WorldSeedSelectionPeerRepliesGeneric ? 1 : 0);
        OnWorldSeedSelectionResult().Remove(WorldSeedSelectionPeerReplyHandle);
    }
}
#endif

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
#if WITH_DEV_AUTOMATION_TESTS
    ++WorldSeedSaveEnumerationCountForTests;
#endif
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
