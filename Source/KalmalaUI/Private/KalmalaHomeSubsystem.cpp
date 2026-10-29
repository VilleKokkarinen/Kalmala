#include "KalmalaHomeSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaMenuContext.h"
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

void UKalmalaHomeSubsystem::ReleaseWorldList()
{
    if (WorldList)
    {
        WorldList->OnBackRequested().RemoveAll(this);
        WorldList->OnWorldSelected().RemoveAll(this);
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
    if (PlayerStep) { PlayerStep->OnBackRequested().RemoveAll(this); PlayerStep->OnContinueRequested().RemoveAll(this); PlayerStep->RemoveFromParent(); }
    PlayerStep = nullptr;
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
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::Worlds
                || Coordinator->GetFlow().GetScreen() == EKalmalaMenuScreen::WorldDetails) Coordinator->GetFlow().Shutdown();
    ReleaseWorldList();
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
