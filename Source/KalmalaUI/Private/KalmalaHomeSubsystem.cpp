#include "KalmalaHomeSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaJoinAddressWidget.h"
#include "KalmalaServerAddressParser.h"
#include "KalmalaMenuContext.h"
#include "KalmalaMenuFlow.h"
#include "KalmalaMenuFlowSubsystem.h"
#include "KalmalaPlayerStepWidget.h"
#include "KalmalaSettingsSubsystem.h"
#include "KalmalaWorldListWidget.h"

void UKalmalaHomeSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!World || !World->IsGameWorld() || !LocalPlayer) { Release(); return; }
    APlayerController* Controller = LocalPlayer->GetPlayerController(World);
    if (!Controller || !Controller->IsLocalController() || !Controller->IsA<AKalmalaMenuPlayerController>())
    {
        Release();
        return;
    }
    UKalmalaMenuFlowSubsystem* Coordinator = LocalPlayer->GetSubsystem<UKalmalaMenuFlowSubsystem>();
    if (!Coordinator) { Release(); return; }
    FKalmalaMenuFlow& Flow = Coordinator->GetFlow();
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
    JoinAddressWidget->SetJoinEnabled(false);

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
        Flow.CompleteFailure(ConnectionToken);
        JoinAddressWidget->SetJoinEnabled(true);
        JoinAddressWidget->ShowMessage(FText::FromString(TEXT("The address could not be used. Check it and try again.")));
        JoinAddressWidget->FocusAddress();
        return;
    }

    Flow.CompleteSuccess(ConnectionToken);
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
    WorldList->SetWorldLaunchActionsEnabled(false);

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
        Flow.CompleteFailure(LaunchToken);
        WorldList->ShowWorldLaunchFailure();
        return;
    }

    Flow.CompleteSuccess(LaunchToken);
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
        JoinAddressWidget->OnJoinRequested().RemoveAll(this);
        JoinAddressWidget->RemoveFromParent();
    }
    JoinAddressWidget = nullptr;
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
    if (HomeWidget && GetLocalPlayer())
        if (UKalmalaMenuFlowSubsystem* Coordinator = GetLocalPlayer()->GetSubsystem<UKalmalaMenuFlowSubsystem>())
            if (Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Options
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Player
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Join
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Worlds
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::WorldDetails) Coordinator->GetFlow().Shutdown();
    ReleaseWorldList();
    ReleaseJoinAddressWidget();
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
