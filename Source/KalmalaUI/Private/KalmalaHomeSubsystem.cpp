#include "KalmalaHomeSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaMenuContext.h"
#include "KalmalaMenuFlowSubsystem.h"

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
        HomeWidget->FocusPlay();
    }
}

void UKalmalaHomeSubsystem::Release()
{
    if (HomeWidget) HomeWidget->RemoveFromParent();
    HomeWidget = nullptr;
    HomeController = nullptr;
}

void UKalmalaHomeSubsystem::Deinitialize()
{
    Release();
    Super::Deinitialize();
}
