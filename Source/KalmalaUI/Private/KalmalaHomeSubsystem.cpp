#include "KalmalaHomeSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/NetDriver.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaJoinAddressWidget.h"
#include "KalmalaLocalWorldSaveListing.h"
#include "KalmalaServerAddressParser.h"
#include "KalmalaMenuContext.h"
#include "KalmalaMenuFlow.h"
#include "KalmalaMenuFlowSubsystem.h"
#include "KalmalaPlayerStepWidget.h"
#include "KalmalaSaveInformationWidget.h"
#include "KalmalaSettingsSubsystem.h"
#include "KalmalaWorldListWidget.h"

namespace
{
    constexpr double JoinConnectionTimeoutSeconds = 30.0;
    constexpr double WorldLaunchWarningSeconds = 60.0;

    FText BackendFailureOrFallback(const FString& BackendMessage, const TCHAR* Fallback)
    {
        FString Message = BackendMessage;
        Message.TrimStartAndEndInline();
        if (Message.IsEmpty()) return FText::FromString(Fallback);
        if (Message.Len() > 180) Message.LeftInline(180, EAllowShrinking::No);
        return FText::FromString(Message);
    }

    FText JoinNetworkFailureMessage(const ENetworkFailure::Type FailureType, const FString& ErrorString)
    {
        switch (FailureType)
        {
        case ENetworkFailure::ConnectionTimeout:
            return FText::FromString(TEXT("Connection timed out. Check the address and try again."));
        case ENetworkFailure::OutdatedClient:
        case ENetworkFailure::OutdatedServer:
            return FText::FromString(TEXT("This game version is not compatible with the host."));
        case ENetworkFailure::FailureReceived:
        case ENetworkFailure::PendingConnectionFailure:
            return BackendFailureOrFallback(ErrorString, TEXT("Could not connect to that host. Try again."));
        case ENetworkFailure::ConnectionLost:
            return FText::FromString(TEXT("The connection to the host was lost. Try again."));
        default:
            return FText::FromString(TEXT("The connection failed. Check the address and try again."));
        }
    }

    FText JoinTravelFailureMessage(const ETravelFailure::Type FailureType)
    {
        switch (FailureType)
        {
        case ETravelFailure::PackageMissing:
        case ETravelFailure::PackageVersion:
        case ETravelFailure::NoDownload:
            return FText::FromString(TEXT("The host's game content is not compatible."));
        default:
            return FText::FromString(TEXT("Could not load the host's world. Check compatibility and try again."));
        }
    }

    FText WorldLaunchTravelFailureMessage(const FString& BackendMessage)
    {
        return BackendFailureOrFallback(BackendMessage,
            TEXT("Could not load the selected world. Return to details and try again."));
    }

    FText MakeSaveInformationText(const bool bLocalIdentityAvailable)
    {
        TArray<FKalmalaLocalWorldSaveRecord> WorldRecords;
        const EKalmalaLocalWorldSaveListingAvailability ListingAvailability =
            FKalmalaLocalWorldSaveListing::Enumerate(WorldRecords);
        FString WorldSummary;
        if (ListingAvailability == EKalmalaLocalWorldSaveListingAvailability::Unsupported)
        {
            WorldSummary = TEXT("World records: Local save-slot listing is not supported on this platform.");
        }
        else if (WorldRecords.IsEmpty())
        {
            WorldSummary = TEXT("World records: No recognized world-scoped save slots were found. Worlds without saved changes may not appear.");
        }
        else
        {
            WorldSummary = FString::Printf(
                TEXT("World records: %d recognized seed group(s) are available locally. This does not establish a complete or compatible world."),
                WorldRecords.Num());
        }

        const TCHAR* IdentitySummary = bLocalIdentityAvailable
            ? TEXT("Player identity: Available to the local menu, but not authenticated or matched to a player save here.")
            : TEXT("Player identity: Unavailable in this menu. Player saves require server-authenticated ownership.");
        TArray<FString> Lines;
        Lines.Add(WorldSummary);
        Lines.Add(TEXT(""));
        Lines.Add(TEXT("World component status: Not loaded or validated on this screen. Open a seed under Play > Your worlds > World details for read-only accepted, rejected, or absent component checks. Those checks do not prove full-world compatibility."));
        Lines.Add(TEXT(""));
        Lines.Add(IdentitySummary);
        Lines.Add(TEXT("Player save match/load status: Not checked in Home. The gameplay server verifies the exact world and authenticated player identity when accessing an existing player save. No player identifier or save path is shown."));
        Lines.Add(TEXT(""));
        Lines.Add(TEXT("Current persistence scope: World-scoped records hold sparse population changes, construction, storage, and ocean travel data; they are not complete world snapshots. Authenticated player records hold discoveries, learned effects, exploration claims, bounded carried tools, and the existing ocean-travel vessel/seat association. Full ordinary inventory, equipped wearables, and skill XP restoration are not promised here."));
        Lines.Add(TEXT(""));
        Lines.Add(TEXT("World save listing reads recognized slot names only; no save contents are loaded or changed."));
        return FText::FromString(FString::Join(Lines, TEXT("\n")));
    }
}

void UKalmalaHomeSubsystem::Tick(float)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer) { Release(); return; }
    UKalmalaMenuFlowSubsystem* Coordinator = LocalPlayer->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) { Release(); return; }
    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    APlayerController* Controller = World ? LocalPlayer->GetPlayerController(World) : nullptr;
    if (Flow.GetScreen() == EKalmalaMenuScreen::PreparingWorld && ActiveLaunchToken != 0)
    {
        const ENetMode ExpectedNetMode = bLaunchIsListenHost ? NM_ListenServer : NM_Standalone;
        if (World && World != LaunchOriginWorld.Get() && Controller && Controller->IsLocalController()
            && World->GetNetMode() == ExpectedNetMode)
        {
            const uint64 CompletedToken = ActiveLaunchToken;
            if (Flow.CompleteSuccess(CompletedToken))
            {
                StopWatchingLaunch();
                ActiveLaunchToken = 0;
                LaunchOriginWorld.Reset();
                LaunchStartedAtSeconds = 0.0;
                bLaunchIsListenHost = false;
                bLaunchDelayWarningShown = false;
                Release();
                return;
            }
        }

        if (!bLaunchDelayWarningShown
            && FPlatformTime::Seconds() - LaunchStartedAtSeconds >= WorldLaunchWarningSeconds)
        {
            bLaunchDelayWarningShown = true;
            if (WorldList) WorldList->ShowWorldLaunchDelayWarning();
        }
        return;
    }
    if (Flow.GetScreen() == EKalmalaMenuScreen::Connecting && ActiveConnectToken != 0)
    {
        if (World && World != ConnectionOriginWorld.Get() && Controller && Controller->IsLocalController()
            && World->GetNetMode() == NM_Client)
        {
            const uint64 CompletedToken = ActiveConnectToken;
            if (Flow.CompleteSuccess(CompletedToken))
            {
                StopWatchingConnection();
                ActiveConnectToken = 0;
                ConnectionOriginWorld.Reset();
                ConnectionStartedAtSeconds = 0.0;
                Release();
                return;
            }
        }

        if (FPlatformTime::Seconds() - ConnectionStartedAtSeconds >= JoinConnectionTimeoutSeconds)
        {
            CancelConnectionAttempt(FText::FromString(TEXT("Connection timed out. Check the address and try again.")));
            return;
        }
        if (Flow.GetScreen() == EKalmalaMenuScreen::Connecting)
        {
            if (JoinAddressWidget) JoinAddressWidget->SetConnecting(true);
            return;
        }
    }
    if (!World || !World->IsGameWorld()) { Release(); return; }
    if (!Controller || !Controller->IsLocalController() || !Controller->IsA<AKalmalaMenuPlayerController>())
    {
        Release();
        return;
    }
    if (Flow.GetScreen() == EKalmalaMenuScreen::Inactive) Flow.Activate();
    if (Flow.GetScreen() == EKalmalaMenuScreen::Options && HomeWidget && HomeController == Controller) return;
    if (Flow.GetScreen() == EKalmalaMenuScreen::Join
        && HomeWidget && PlayerStep && HomeController == Controller)
    {
        ReleaseWorldList();
        if (!JoinAddressWidget)
        {
            JoinAddressWidget = CreateWidget<UKalmalaJoinAddressWidget>(Controller,
                UKalmalaJoinAddressWidget::StaticClass());
            if (!JoinAddressWidget || !JoinAddressWidget->AddToPlayerScreen(200))
            {
                JoinAddressWidget = nullptr;
                Flow.Navigate(EKalmalaMenuScreen::Player);
                PlayerStep->SetVisibility(ESlateVisibility::Visible);
                PlayerStep->FocusContinue();
                return;
            }
            JoinAddressWidget->OnBackRequested().AddUObject(this, &ThisClass::HandleJoinBackRequested);
            JoinAddressWidget->OnCancelRequested().AddUObject(this, &ThisClass::HandleJoinCancelRequested);
            JoinAddressWidget->OnJoinRequested().AddUObject(this, &ThisClass::HandleJoinAddressRequested);
            JoinAddressWidget->FocusAddress();
        }
        return;
    }
    ReleaseJoinAddressWidget();
    if ((Flow.GetScreen() == EKalmalaMenuScreen::Worlds || Flow.GetScreen() == EKalmalaMenuScreen::NewWorld
        || Flow.GetScreen() == EKalmalaMenuScreen::WorldDetails)
        && HomeWidget && PlayerStep && HomeController == Controller)
    {
        if (!WorldList)
        {
            WorldList = CreateWidget<UKalmalaWorldListWidget>(Controller, UKalmalaWorldListWidget::StaticClass());
            if (!WorldList || !WorldList->AddToPlayerScreen(200))
            {
                WorldList = nullptr;
                HandleWorldListBack();
                return;
            }
            WorldList->OnBackRequested().AddUObject(this, &ThisClass::HandleWorldListBack);
            WorldList->OnWorldSelected().AddUObject(this, &ThisClass::HandleWorldSelected);
            WorldList->OnSoloPlayRequested().AddUObject(this, &ThisClass::HandleSoloPlayRequested);
            WorldList->OnHostCoopRequested().AddUObject(this, &ThisClass::HandleHostCoopRequested);
            WorldList->OnNewWorldRequested().AddUObject(this, &ThisClass::HandleNewWorldRequested);
            WorldList->OnReturnToWorldsRequested().AddUObject(this, &ThisClass::HandleReturnToWorldsRequested);
            if (Flow.GetScreen() == EKalmalaMenuScreen::NewWorld && !WorldList->ShowNewWorldSeedEntry())
            {
                Flow.Navigate(EKalmalaMenuScreen::Worlds);
                WorldList->ShowWorldList();
            }
            WorldList->FocusPrimary();
        }
        return;
    }
    ReleaseWorldList();
    if (Flow.GetScreen() == EKalmalaMenuScreen::SaveInformation && HomeWidget && HomeController == Controller)
    {
        if (!SaveInformationWidget)
        {
            SaveInformationWidget = CreateWidget<UKalmalaSaveInformationWidget>(Controller,
                UKalmalaSaveInformationWidget::StaticClass());
            if (!SaveInformationWidget || !SaveInformationWidget->AddToPlayerScreen(190))
            {
                SaveInformationWidget = nullptr;
                Flow.Navigate(EKalmalaMenuScreen::Home);
                HomeWidget->SetVisibility(ESlateVisibility::Visible);
                HomeWidget->FocusSaveInformation();
                HomeWidget->ShowUnavailable();
                return;
            }
            SaveInformationWidget->OnBackRequested().AddUObject(this, &ThisClass::HandleSaveInformationBack);
            const APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>();
            SaveInformationWidget->SetInformation(MakeSaveInformationText(
                PlayerState && PlayerState->GetUniqueId().IsValid()));
            SaveInformationWidget->FocusBack();
        }
        return;
    }
    ReleaseSaveInformationWidget();
    if (Flow.GetScreen() == EKalmalaMenuScreen::Player && HomeWidget && HomeController == Controller)
    {
        if (!PlayerStep)
        {
            PlayerStep = CreateWidget<UKalmalaPlayerStepWidget>(Controller, UKalmalaPlayerStepWidget::StaticClass());
            if (!PlayerStep || !PlayerStep->AddToPlayerScreen(190))
            {
                PlayerStep = nullptr;
                HandlePlayerBack();
                if (HomeWidget) HomeWidget->ShowUnavailable();
                return;
            }
            PlayerStep->OnBackRequested().AddUObject(this, &ThisClass::HandlePlayerBack);
            PlayerStep->OnContinueRequested().AddUObject(this, &ThisClass::HandlePlayerContinue);
            PlayerStep->OnJoinRequested().AddUObject(this, &ThisClass::HandlePlayerJoinRequested);
            PlayerStep->FocusContinue();
        }
        return;
    }
    ReleasePlayerStep();
    if (Flow.GetScreen() != EKalmalaMenuScreen::Home) { Release(); return; }
    if (HomeController != Controller) Release();
    if (!HomeWidget)
    {
        HomeWidget = CreateWidget<UKalmalaHomeWidget>(Controller, UKalmalaHomeWidget::StaticClass());
        if (!HomeWidget) return;
        HomeController = Controller;
        if (!HomeWidget->AddToPlayerScreen(180))
        {
            HomeWidget = nullptr;
            HomeController = nullptr;
            return;
        }
        HomeWidget->OnActionRequested().AddUObject(this, &ThisClass::HandleHomeAction);
        HomeWidget->FocusPlay();
    }
}

void UKalmalaHomeSubsystem::HandleHomeAction(const EKalmalaHomeAction Action)
{
    if (!HomeWidget || !HomeController || !GetLocalPlayer()) return;
    if (Action == EKalmalaHomeAction::Play)
    {
        UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
        if (Coordinator && Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Player))
            HomeWidget->SetVisibility(ESlateVisibility::Collapsed);
        else HomeWidget->ShowUnavailable();
        return;
    }
    if (Action == EKalmalaHomeAction::SaveInformation)
    {
        UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
        if (Coordinator && Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::SaveInformation))
            HomeWidget->SetVisibility(ESlateVisibility::Collapsed);
        else HomeWidget->ShowUnavailable();
        return;
    }
    if (Action != EKalmalaHomeAction::Options) { HomeWidget->ShowUnavailable(); return; }
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    UKalmalaSettingsSubsystem* Settings = GetLocalPlayer()->GetSubsystem<UKalmalaSettingsSubsystem>();
    if (!Coordinator || !Settings || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Options))
    {
        HomeWidget->ShowUnavailable();
        return;
    }
    TWeakObjectPtr<UKalmalaHomeWidget> RequestedHome = HomeWidget;
    HomeWidget->SetVisibility(ESlateVisibility::Collapsed);
    if (!Settings->OpenOptionsFromHome(HomeController) && RequestedHome.IsValid() && HomeWidget == RequestedHome.Get())
    {
        Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Home);
        HomeWidget->SetVisibility(ESlateVisibility::Visible);
        HomeWidget->FocusOptions();
        HomeWidget->ShowUnavailable();
    }
}

void UKalmalaHomeSubsystem::HandleSaveInformationBack()
{
    if (!SaveInformationWidget || !HomeWidget || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::SaveInformation
        || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Home)) return;
    ReleaseSaveInformationWidget();
    HomeWidget->SetVisibility(ESlateVisibility::Visible);
    HomeWidget->FocusSaveInformation();
}

void UKalmalaHomeSubsystem::HandlePlayerContinue()
{
    if (!PlayerStep || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::Player
        || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Worlds)) return;
    PlayerStep->SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaHomeSubsystem::HandlePlayerJoinRequested()
{
    if (!PlayerStep || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::Player
        || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Join)) return;
    PlayerStep->SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaHomeSubsystem::HandleJoinBackRequested()
{
    if (!JoinAddressWidget || !PlayerStep || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::Join
        || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Player)) return;
    ReleaseJoinAddressWidget();
    PlayerStep->SetVisibility(ESlateVisibility::Visible);
    PlayerStep->FocusContinue();
}

void UKalmalaHomeSubsystem::HandleJoinCancelRequested()
{
    CancelConnectionAttempt(FText::FromString(TEXT("Connection cancelled.")));
}

void UKalmalaHomeSubsystem::StartWatchingConnection(const uint64 Token)
{
    StopWatchingConnection();
    if (!GEngine) return;

    NetworkFailureHandle = GEngine->OnNetworkFailure().AddWeakLambda(this,
        [this, Token](UWorld* FailedWorld, UNetDriver* NetDriver,
            const ENetworkFailure::Type FailureType, const FString& ErrorString)
        {
            if (ActiveConnectToken != Token) return;
            const bool bOriginWorldFailure = FailedWorld && FailedWorld == ConnectionOriginWorld.Get();
            const bool bPendingConnectionFailure = FailedWorld == nullptr && NetDriver != nullptr;
            if (!bOriginWorldFailure && !bPendingConnectionFailure) return;
            FinishConnectionFailure(Token, JoinNetworkFailureMessage(FailureType, ErrorString));
        });

    TravelFailureHandle = GEngine->OnTravelFailure().AddWeakLambda(this,
        [this, Token](UWorld* FailedWorld, const ETravelFailure::Type FailureType, const FString&)
        {
            if (ActiveConnectToken != Token || !FailedWorld) return;
            UWorld* OriginWorld = ConnectionOriginWorld.Get();
            if (FailedWorld != OriginWorld && FailedWorld != GetWorld()) return;
            ClearScheduledTravel(FailedWorld);
            FinishConnectionFailure(Token, JoinTravelFailureMessage(FailureType));
        });
}

void UKalmalaHomeSubsystem::StopWatchingConnection()
{
    if (GEngine)
    {
        if (NetworkFailureHandle.IsValid()) GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
        if (TravelFailureHandle.IsValid()) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    NetworkFailureHandle.Reset();
    TravelFailureHandle.Reset();
}

void UKalmalaHomeSubsystem::ClearScheduledTravel(UWorld* OriginWorld)
{
    if (!GEngine || !OriginWorld) return;
    if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(OriginWorld))
    {
        // Engine failure handlers may queue a default-map or ?closed travel before this listener runs.
        Context->TravelURL.Reset();
        Context->TravelType = TRAVEL_Absolute;
    }
}

void UKalmalaHomeSubsystem::FinishConnectionFailure(const uint64 Token, const FText& Message)
{
    if (Token == 0 || ActiveConnectToken != Token) return;
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    UKalmalaMenuFlowSubsystem* Coordinator = LocalPlayer
        ? LocalPlayer->GetSubsystem<UKalmalaMenuFlowSubsystem>() : nullptr;
    const bool bRestoredOrigin = Coordinator && Coordinator->GetFlow().CompleteFailure(Token);
    UWorld* OriginWorld = ConnectionOriginWorld.Get();
    StopWatchingConnection();
    ActiveConnectToken = 0;
    ConnectionOriginWorld.Reset();
    ConnectionStartedAtSeconds = 0.0;
    if (GEngine && OriginWorld) GEngine->CancelPending(OriginWorld);
    ClearScheduledTravel(OriginWorld);
    if (!bRestoredOrigin || !JoinAddressWidget) return;

    JoinAddressWidget->SetConnecting(false);
    JoinAddressWidget->ShowMessage(Message);
    JoinAddressWidget->FocusAddress();
}

void UKalmalaHomeSubsystem::CancelConnectionAttempt(const FText& Message)
{
    const uint64 Token = ActiveConnectToken;
    if (Token == 0) return;
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    UKalmalaMenuFlowSubsystem* Coordinator = LocalPlayer
        ? LocalPlayer->GetSubsystem<UKalmalaMenuFlowSubsystem>() : nullptr;
    if (!Coordinator || !Coordinator->GetFlow().CancelConnect(Token)) return;

    UWorld* OriginWorld = ConnectionOriginWorld.Get();
    StopWatchingConnection();
    ActiveConnectToken = 0;
    ConnectionOriginWorld.Reset();
    ConnectionStartedAtSeconds = 0.0;
    if (GEngine && OriginWorld) GEngine->CancelPending(OriginWorld);
    ClearScheduledTravel(OriginWorld);
    if (!JoinAddressWidget) return;

    JoinAddressWidget->SetConnecting(false);
    JoinAddressWidget->ShowMessage(Message);
    JoinAddressWidget->FocusAddress();
}

void UKalmalaHomeSubsystem::StartWatchingLaunch(const uint64 Token)
{
    StopWatchingLaunch();
    if (!GEngine) return;

    LaunchTravelFailureHandle = GEngine->OnTravelFailure().AddWeakLambda(this,
        [this, Token](UWorld* FailedWorld, const ETravelFailure::Type, const FString& ErrorString)
        {
            if (ActiveLaunchToken != Token || !FailedWorld) return;
            UWorld* OriginWorld = LaunchOriginWorld.Get();
            if (FailedWorld != OriginWorld && FailedWorld != GetWorld()) return;
            ClearScheduledTravel(FailedWorld);
            FinishLaunchFailure(Token, WorldLaunchTravelFailureMessage(ErrorString));
        });
}

void UKalmalaHomeSubsystem::StopWatchingLaunch()
{
    if (GEngine && LaunchTravelFailureHandle.IsValid())
        GEngine->OnTravelFailure().Remove(LaunchTravelFailureHandle);
    LaunchTravelFailureHandle.Reset();
}

void UKalmalaHomeSubsystem::FinishLaunchFailure(const uint64 Token, const FText& Message)
{
    if (Token == 0 || ActiveLaunchToken != Token) return;
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    UKalmalaMenuFlowSubsystem* Coordinator = LocalPlayer
        ? LocalPlayer->GetSubsystem<UKalmalaMenuFlowSubsystem>() : nullptr;
    const bool bRestoredOrigin = Coordinator && Coordinator->GetFlow().CompleteFailure(Token);
    UWorld* OriginWorld = LaunchOriginWorld.Get();
    APlayerController* Controller = LocalPlayer && OriginWorld
        ? LocalPlayer->GetPlayerController(OriginWorld) : nullptr;
    if (OriginWorld)
        if (AKalmalaMenuGameMode* MenuGameMode = OriginWorld->GetAuthGameMode<AKalmalaMenuGameMode>())
            MenuGameMode->ResetGameplayLaunchAfterTravelFailure(Controller);

    StopWatchingLaunch();
    ActiveLaunchToken = 0;
    LaunchOriginWorld.Reset();
    LaunchStartedAtSeconds = 0.0;
    bLaunchIsListenHost = false;
    bLaunchDelayWarningShown = false;
    ClearScheduledTravel(OriginWorld);
    if (!bRestoredOrigin || !WorldList || !OriginWorld || GetWorld() != OriginWorld) return;
    WorldList->ShowWorldLaunchFailure(Message);
}

void UKalmalaHomeSubsystem::HandleJoinAddressRequested(FString Address)
{
    if (!JoinAddressWidget || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::Join) return;

    FKalmalaServerEndpoint Endpoint;
    const EKalmalaServerAddressParseResult ParseResult = FKalmalaServerAddressParser::Parse(
        Address, FURL::UrlConfig.DefaultPort, Endpoint);
    if (ParseResult != EKalmalaServerAddressParseResult::Success)
    {
        JoinAddressWidget->ShowMessage(FText::FromString(FKalmalaServerAddressParser::ErrorMessage(ParseResult)));
        JoinAddressWidget->FocusAddress();
        return;
    }

    const FString TravelAddress = Endpoint.ToTravelAddress();
    if (TravelAddress.IsEmpty())
    {
        JoinAddressWidget->ShowMessage(FText::FromString(TEXT("The address could not be used. Check it and try again.")));
        JoinAddressWidget->FocusAddress();
        return;
    }

    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    uint64 ConnectionToken = 0;
    if (!Flow.TryBegin(EKalmalaMenuRequest::Connect, ConnectionToken)) return;
    ActiveConnectToken = ConnectionToken;
    ConnectionOriginWorld = GetWorld();
    ConnectionStartedAtSeconds = FPlatformTime::Seconds();
    JoinAddressWidget->SetConnecting(true);
    JoinAddressWidget->FocusCancel();
    StartWatchingConnection(ConnectionToken);

    AKalmalaMenuPlayerController* MenuController = Cast<AKalmalaMenuPlayerController>(HomeController);
    bool bTravelStarted = false;
    const bool bCommitted = Flow.TryCommitTravel(ConnectionToken, [&]()
    {
        if (MenuController && MenuController->IsLocalController() && MenuController->GetWorld() == GetWorld())
        {
            MenuController->ClientTravel(TravelAddress, TRAVEL_Absolute);
            bTravelStarted = true;
        }
    });

    if (!bCommitted || !bTravelStarted)
    {
        FinishConnectionFailure(ConnectionToken,
            FText::FromString(TEXT("The address could not be used. Check it and try again.")));
    }
}

void UKalmalaHomeSubsystem::HandleWorldListBack()
{
    if (!PlayerStep || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) return;
    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    if (Flow.GetScreen() == EKalmalaMenuScreen::NewWorld)
    {
        if (!Flow.Navigate(EKalmalaMenuScreen::Worlds)) return;
        if (WorldList) { WorldList->ShowWorldList(); WorldList->FocusPrimary(); }
        return;
    }
    if (Flow.GetScreen() == EKalmalaMenuScreen::WorldDetails)
    {
        if (!Flow.Navigate(EKalmalaMenuScreen::Worlds)) return;
        if (WorldList) { WorldList->ShowWorldList(); WorldList->FocusPrimary(); }
        return;
    }
    if (Flow.GetScreen() != EKalmalaMenuScreen::Worlds || !Flow.Navigate(EKalmalaMenuScreen::Player)) return;
    ReleaseWorldList();
    PlayerStep->SetVisibility(ESlateVisibility::Visible);
    PlayerStep->FocusContinue();
}

void UKalmalaHomeSubsystem::HandleNewWorldRequested()
{
    if (!WorldList || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) return;
    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    if (Flow.GetScreen() != EKalmalaMenuScreen::Worlds || !Flow.Navigate(EKalmalaMenuScreen::NewWorld)) return;
    if (!WorldList->ShowNewWorldSeedEntry())
    {
        Flow.Navigate(EKalmalaMenuScreen::Worlds);
        WorldList->ShowWorldList();
        WorldList->FocusPrimary();
        return;
    }
    WorldList->FocusPrimary();
}

void UKalmalaHomeSubsystem::HandleReturnToWorldsRequested()
{
    if (!WorldList || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) return;
    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    if (Flow.GetScreen() == EKalmalaMenuScreen::NewWorld && !Flow.Navigate(EKalmalaMenuScreen::Worlds)) return;
    if (Flow.GetScreen() != EKalmalaMenuScreen::Worlds) return;
    WorldList->ShowWorldList();
    WorldList->FocusPrimary();
}

void UKalmalaHomeSubsystem::HandleWorldSelected(const uint64 Seed)
{
    if (!WorldList || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) return;
    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    if ((Flow.GetScreen() != EKalmalaMenuScreen::Worlds && Flow.GetScreen() != EKalmalaMenuScreen::NewWorld)
        || !Flow.Navigate(EKalmalaMenuScreen::WorldDetails)) return;
    if (!WorldList->ShowWorldDetails(Seed))
    {
        Flow.Navigate(EKalmalaMenuScreen::Worlds);
        WorldList->ShowWorldList();
        return;
    }
    WorldList->FocusPrimary();
}

void UKalmalaHomeSubsystem::HandleSoloPlayRequested(const uint64 Seed, const bool bConfirmedExisting)
{
    HandleWorldLaunchRequested(Seed, bConfirmedExisting, false);
}

void UKalmalaHomeSubsystem::HandleHostCoopRequested(const uint64 Seed, const bool bConfirmedExisting)
{
    HandleWorldLaunchRequested(Seed, bConfirmedExisting, true);
}

void UKalmalaHomeSubsystem::HandleWorldLaunchRequested(const uint64 Seed, const bool bConfirmedExisting,
    const bool bListenHost)
{
    if (!WorldList || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) return;

    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
    uint64 LaunchToken = 0;
    if (!Flow.TryBegin(EKalmalaMenuRequest::Launch, LaunchToken)) return;
    ActiveLaunchToken = LaunchToken;
    LaunchOriginWorld = GetWorld();
    LaunchStartedAtSeconds = FPlatformTime::Seconds();
    bLaunchIsListenHost = bListenHost;
    bLaunchDelayWarningShown = false;
    WorldList->ShowWorldLaunchInProgress();
    StartWatchingLaunch(LaunchToken);

    AKalmalaMenuPlayerController* MenuController = Cast<AKalmalaMenuPlayerController>(HomeController);
    AKalmalaMenuGameMode* MenuGameMode = GetWorld()->GetAuthGameMode<AKalmalaMenuGameMode>();
    bool bStartedLaunch = false;
    const bool bCommitted = Flow.TryCommitTravel(LaunchToken, [&]()
    {
        if (MenuController && MenuGameMode)
        {
            bStartedLaunch = bListenHost
                ? MenuGameMode->TryStartListenHostFromServer(MenuController, Seed, bConfirmedExisting)
                : MenuGameMode->TryStartSoloFromServer(MenuController, Seed, bConfirmedExisting);
        }
    });

    if (!bCommitted || !bStartedLaunch)
    {
        FinishLaunchFailure(LaunchToken, FText::FromString(TEXT(
            "The server could not validate this world for launch. No travel started and no save was changed.")));
        return;
    }
}

void UKalmalaHomeSubsystem::ReleaseWorldList()
{
    if (WorldList)
    {
        WorldList->OnBackRequested().RemoveAll(this);
        WorldList->OnWorldSelected().RemoveAll(this);
        WorldList->OnSoloPlayRequested().RemoveAll(this);
        WorldList->OnHostCoopRequested().RemoveAll(this);
        WorldList->OnNewWorldRequested().RemoveAll(this);
        WorldList->OnReturnToWorldsRequested().RemoveAll(this);
        WorldList->RemoveFromParent();
    }
    WorldList = nullptr;
}

void UKalmalaHomeSubsystem::HandlePlayerBack()
{
    if (!HomeWidget || !HomeController || !GetLocalPlayer() || !GetWorld()
        || GetLocalPlayer()->GetPlayerController(GetWorld()) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::Player
        || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Home)) return;
    ReleasePlayerStep();
    HomeWidget->SetVisibility(ESlateVisibility::Visible);
    HomeWidget->FocusPlay();
}

void UKalmalaHomeSubsystem::ReleasePlayerStep()
{
    if (PlayerStep)
    {
        PlayerStep->OnBackRequested().RemoveAll(this);
        PlayerStep->OnContinueRequested().RemoveAll(this);
        PlayerStep->OnJoinRequested().RemoveAll(this);
        PlayerStep->RemoveFromParent();
    }
    PlayerStep = nullptr;
}

void UKalmalaHomeSubsystem::ReleaseJoinAddressWidget()
{
    if (JoinAddressWidget)
    {
        JoinAddressWidget->OnBackRequested().RemoveAll(this);
        JoinAddressWidget->OnCancelRequested().RemoveAll(this);
        JoinAddressWidget->OnJoinRequested().RemoveAll(this);
        JoinAddressWidget->RemoveFromParent();
    }
    JoinAddressWidget = nullptr;
}

void UKalmalaHomeSubsystem::ReleaseSaveInformationWidget()
{
    if (SaveInformationWidget)
    {
        SaveInformationWidget->OnBackRequested().RemoveAll(this);
        SaveInformationWidget->RemoveFromParent();
    }
    SaveInformationWidget = nullptr;
}

void UKalmalaHomeSubsystem::ReturnFromOptions()
{
    ULocalPlayer* Player = GetLocalPlayer();
    UWorld* World = GetWorld();
    if (!Player || !World || !HomeWidget || !HomeController
        || Player->GetPlayerController(World) != HomeController) return;
    UKalmalaMenuFlowSubsystem* Coordinator = Player->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator || Coordinator->GetFlow().GetScreen() != EKalmalaMenuScreen::Options
        || !Coordinator->GetFlow().Navigate(EKalmalaMenuScreen::Home)) return;
    HomeWidget->SetVisibility(ESlateVisibility::Visible);
    HomeWidget->FocusOptions();
}

void UKalmalaHomeSubsystem::AbandonOptions()
{
    if (ULocalPlayer* Player = GetLocalPlayer())
        if (UKalmalaMenuFlowSubsystem* Coordinator = Player->GetSubsystem<UKalmalaMenuFlowSubsystem>())
            if (Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Options) Coordinator->GetFlow().Shutdown();
    Release();
}

void UKalmalaHomeSubsystem::Release()
{
    if (ActiveConnectToken != 0)
    {
        if (ULocalPlayer* Player = GetLocalPlayer())
            if (UKalmalaMenuFlowSubsystem* Coordinator = Player->GetSubsystem<UKalmalaMenuFlowSubsystem>())
                Coordinator->GetFlow().CancelConnect(ActiveConnectToken);
        UWorld* OriginWorld = ConnectionOriginWorld.Get();
        StopWatchingConnection();
        ActiveConnectToken = 0;
        ConnectionOriginWorld.Reset();
        ConnectionStartedAtSeconds = 0.0;
        if (GEngine && OriginWorld) GEngine->CancelPending(OriginWorld);
        ClearScheduledTravel(OriginWorld);
    }
    if (ActiveLaunchToken != 0)
    {
        if (ULocalPlayer* Player = GetLocalPlayer())
            if (UKalmalaMenuFlowSubsystem* Coordinator = Player->GetSubsystem<UKalmalaMenuFlowSubsystem>())
                Coordinator->GetFlow().Shutdown();
        StopWatchingLaunch();
        ActiveLaunchToken = 0;
        LaunchOriginWorld.Reset();
        LaunchStartedAtSeconds = 0.0;
        bLaunchIsListenHost = false;
        bLaunchDelayWarningShown = false;
    }
    if (HomeWidget && GetLocalPlayer())
        if (UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>())
            if (Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Options
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Player
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Join
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Worlds
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::WorldDetails
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::SaveInformation) Coordinator->GetFlow().Shutdown();
    ReleaseWorldList();
    ReleaseJoinAddressWidget();
    ReleaseSaveInformationWidget();
    ReleasePlayerStep();
    if (HomeWidget) { HomeWidget->OnActionRequested().RemoveAll(this); HomeWidget->RemoveFromParent(); }
    HomeWidget = nullptr;
    HomeController = nullptr;
}

void UKalmalaHomeSubsystem::Deinitialize()
{
    Release();
    Super::Deinitialize();
}
