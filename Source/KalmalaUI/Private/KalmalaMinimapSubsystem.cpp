#include "KalmalaMinimapSubsystem.h"

#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIActionRouterBase.h"
#include "KalmalaMinimapWidget.h"
#include "Engine/LocalPlayer.h"

void UKalmalaMinimapSubsystem::Tick(float DeltaTime)
{
    if (GetGameInstance() == nullptr)
    {
        return;
    }

    APlayerController* FoundLocalController = GetGameInstance()->GetFirstLocalPlayerController();
    if (FoundLocalController == nullptr || !FoundLocalController->IsLocalController())
    {
        return;
    }

    LocalController = FoundLocalController;
    BindLocalInput(LocalController);

    if (MinimapWidget != nullptr)
    {
        return;
    }

    MinimapWidget = CreateWidget<UKalmalaMinimapWidget>(LocalController, UKalmalaMinimapWidget::StaticClass());
    if (MinimapWidget == nullptr)
    {
        return;
    }

    MinimapWidget->InitializeForLocalPlayer(LocalController);
    MinimapWidget->SetAnchorsInViewport(FAnchors(1.0f, 0.0f));
    MinimapWidget->SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
    MinimapWidget->SetPositionInViewport(FVector2D(-24.0f, 24.0f));
    MinimapWidget->AddToPlayerScreen(50);
}

void UKalmalaMinimapSubsystem::BindLocalInput(APlayerController* InLocalController)
{
    if (bInputBound || InLocalController == nullptr || InLocalController->InputComponent == nullptr)
    {
        return;
    }

    FInputAxisBinding& ZoomBinding = InLocalController->InputComponent->BindAxis(TEXT("MinimapZoom"), this, &UKalmalaMinimapSubsystem::HandleMinimapZoom);
    ZoomBinding.bConsumeInput = false;
    bInputBound = true;
}

void UKalmalaMinimapSubsystem::HandleMinimapZoom(const float WheelDelta)
{
    if (MinimapWidget == nullptr || LocalController == nullptr || FMath::IsNearlyZero(WheelDelta))
    {
        return;
    }

    const ULocalPlayer* LocalPlayer = LocalController->GetLocalPlayer();
    const UCommonUIActionRouterBase* ActionRouter = LocalPlayer != nullptr ? LocalPlayer->GetSubsystem<UCommonUIActionRouterBase>() : nullptr;
    const bool bCanProcessNormalGameInput = ActionRouter == nullptr || ActionRouter->CanProcessNormalGameInput();
    if (UKalmalaMinimapWidget::ShouldAcceptZoomInput(bCanProcessNormalGameInput))
    {
        MinimapWidget->AdjustZoom(WheelDelta);
    }
}
