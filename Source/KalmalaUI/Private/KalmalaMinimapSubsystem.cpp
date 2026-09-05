#include "KalmalaMinimapSubsystem.h"

#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIActionRouterBase.h"
#include "KalmalaMinimapWidget.h"
#include "Engine/LocalPlayer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UKalmalaMinimapSubsystem::Tick(float DeltaTime)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld())
    {
        return;
    }

    APlayerController* FoundLocalController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundLocalController)
    {
        ReleaseController();
    }
    if (FoundLocalController == nullptr || !FoundLocalController->IsLocalController())
    {
        return;
    }

    LocalController = FoundLocalController;
    BindLocalInput(LocalController);

    if (MinimapWidget != nullptr)
    {
        VerifyLocalInput();
        if (!MinimapWidget->IsInViewport())
        {
            MinimapWidget->ConfigureViewportPlacement();
            MinimapWidget->AddToPlayerScreen(50);
        }
        return;
    }

    MinimapWidget = CreateWidget<UKalmalaMinimapWidget>(LocalController, UKalmalaMinimapWidget::StaticClass());
    if (MinimapWidget == nullptr)
    {
        return;
    }

    MinimapWidget->InitializeForLocalPlayer(LocalController, SessionZoom);
    MinimapWidget->ConfigureViewportPlacement();
    MinimapWidget->AddToPlayerScreen(50);
}

void UKalmalaMinimapSubsystem::ReleaseController()
{
    if (UInputComponent* Input = BoundInputComponent.Get())
    {
        for (int32 Index = Input->AxisBindings.Num() - 1; Index >= 0; --Index)
        {
            if (Input->AxisBindings[Index].AxisDelegate.IsBoundToObject(this)) Input->AxisBindings.RemoveAt(Index);
        }
    }
    BoundInputComponent.Reset();
    if (MinimapWidget != nullptr)
    {
        SessionZoom = MinimapWidget->GetCurrentZoom();
        MinimapWidget->RemoveFromParent();
        MinimapWidget = nullptr;
    }
    LocalController = nullptr;
}

void UKalmalaMinimapSubsystem::Deinitialize()
{
    ReleaseController();
    Super::Deinitialize();
}

void UKalmalaMinimapSubsystem::BindLocalInput(APlayerController* InLocalController)
{
    if (InLocalController == nullptr || InLocalController->InputComponent == nullptr
        || BoundInputComponent.Get() == InLocalController->InputComponent)
    {
        return;
    }

    if (UInputComponent* PreviousInput = BoundInputComponent.Get())
    {
        for (int32 Index = PreviousInput->AxisBindings.Num() - 1; Index >= 0; --Index)
        {
            if (PreviousInput->AxisBindings[Index].AxisDelegate.IsBoundToObject(this)) PreviousInput->AxisBindings.RemoveAt(Index);
        }
    }
    FInputAxisBinding& ZoomBinding = InLocalController->InputComponent->BindAxis(TEXT("MinimapZoom"), this, &UKalmalaMinimapSubsystem::HandleMinimapZoom);
    ZoomBinding.bConsumeInput = false;
    BoundInputComponent = InLocalController->InputComponent;
}

void UKalmalaMinimapSubsystem::HandleMinimapZoom(const float WheelDelta)
{
    if (MinimapWidget == nullptr || LocalController == nullptr || FMath::IsNearlyZero(WheelDelta))
    {
        return;
    }

    const ULocalPlayer* LocalPlayer = LocalController->GetLocalPlayer();
    const UCommonUIActionRouterBase* ActionRouter = LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UCommonUIActionRouterBase>() : nullptr;
    // Menu mode can permit game input for captured 3D previews; its wheel still belongs to the menu.
    const bool bCanProcessNormalGameInput = ActionRouter == nullptr
        || (ActionRouter->GetActiveInputMode() != ECommonInputMode::Menu && ActionRouter->CanProcessNormalGameInput());
    if (UKalmalaMinimapWidget::ShouldAcceptZoomInput(bCanProcessNormalGameInput))
    {
        MinimapWidget->AdjustZoom(WheelDelta);
    }
}

void UKalmalaMinimapSubsystem::VerifyLocalInput()
{
#if !UE_BUILD_SHIPPING
    if (bVerifiedInput || !BoundInputComponent.IsValid() || LocalController->GetPawn() == nullptr
        || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaMinimapVerification"))) return;
    UCommonUIActionRouterBase* Router = GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>();
    if (Router == nullptr) return;
    bVerifiedInput = true;
    const float OriginalZoom = MinimapWidget->GetCurrentZoom();
    FUIInputConfig OriginalConfig(Router->GetActiveInputMode(), Router->GetActiveMouseCaptureMode());
    OriginalConfig.bIgnoreMoveInput = LocalController->IsMoveInputIgnored();
    OriginalConfig.bIgnoreLookInput = LocalController->IsLookInputIgnored();
    auto Wheel = [this](float Delta)
    {
        for (FInputAxisBinding& Binding : BoundInputComponent->AxisBindings)
        {
            if (Binding.AxisDelegate.IsBoundToObject(this)) Binding.AxisDelegate.Execute(Delta);
        }
    };
    Router->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::NoCapture));
    Wheel(1000.0f);
    const bool bMinPassed = MinimapWidget->GetCurrentZoom() == 2500.0f;
    Wheel(-1000.0f);
    const bool bMaxPassed = MinimapWidget->GetCurrentZoom() == 10000.0f;
    Router->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture));
    Wheel(1000.0f);
    const bool bModalPassed = MinimapWidget->GetCurrentZoom() == 10000.0f;
    Router->SetActiveUIInputConfig(FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::NoCapture));
    Wheel(1.0f);
    const bool bResumePassed = MinimapWidget->GetCurrentZoom() == 9250.0f;
    MinimapWidget->AdjustZoom((MinimapWidget->GetCurrentZoom() - OriginalZoom) / 750.0f);
    Router->SetActiveUIInputConfig(OriginalConfig);
    UE_LOG(LogTemp, Display, TEXT("Minimap input verification: Min=%d Max=%d Modal=%d Resume=%d"),
        bMinPassed, bMaxPassed, bModalPassed, bResumePassed);
#endif
}
