#include "KalmalaMinimapSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "KalmalaMinimapWidget.h"

void UKalmalaMinimapSubsystem::Tick(float DeltaTime)
{
    if (MinimapWidget != nullptr || GetGameInstance() == nullptr)
    {
        return;
    }

    APlayerController* LocalController = GetGameInstance()->GetFirstLocalPlayerController();
    if (LocalController == nullptr || !LocalController->IsLocalController())
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
